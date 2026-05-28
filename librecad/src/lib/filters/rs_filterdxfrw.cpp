/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2026 LibreCAD (librecad.org)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**********************************************************************/

#include <array>
#include<cstdlib>
#include <cmath>
#include <set>
#include <stack>
#include<utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QStringList>

#include "dxf_format.h"
#include "lc_containertraverser.h"
#include "lc_defaults.h"
#include "lc_dimarc.h"
#include "lc_dimarrowregistry.h"
#include "lc_dimordinate.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_dimstyle.h"
#include "lc_extentitydata.h"
#include "lc_hyperbola.h"
#include "lc_hyperbolaspline.h"
#include "lc_mleader.h"
#include "lc_parabola.h"
#include "lc_parabolaspline.h"
#include "lc_splinepoints.h"
#include "lc_tolerance.h"
#include "lc_wipeout.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_filterdxfrw.h"
#include "rs_graphicview.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "rs_system.h"
#include "rs_text.h"

#ifdef DWGSUPPORT
#include "lc_graphicviewport.h"
#include "lc_ucs.h"
#include "lc_view.h"
#include "libdwgr.h"
#include "rs_debug.h"
#endif // DWGSUPPORT

namespace {

constexpr int kImportedSplineFallbackSamples = 96;

bool differsFromUnitWeight(double weight) {
    return std::fabs(weight - 1.0) > 1e-12;
}

bool hasRationalSplineWeights(const DRW_Spline* data) {
    if (data == nullptr)
        return false;
    return (data->flags & 0x4) != 0 ||
           std::any_of(data->weightlist.begin(), data->weightlist.end(), differsFromUnitWeight);
}

bool buildSplineDataFromDrw(const DRW_Spline* source, RS_SplineData& target) {
    if (source == nullptr || source->degree < 1 || source->controllist.empty())
        return false;

    const size_t degree = static_cast<size_t>(source->degree);
    const size_t controlCount = source->controllist.size();
    if (controlCount < degree + 1)
        return false;

    const size_t requiredKnots = controlCount + degree + 1;
    if (source->knotslist.size() < requiredKnots)
        return false;

    const bool closed = (source->flags & 0x3) != 0;
    target = RS_SplineData(source->degree, closed);
    target.type = closed ? RS_SplineData::SplineType::WrappedClosed
                         : RS_SplineData::SplineType::ClampedOpen;

    target.knotslist.assign(source->knotslist.begin(),
                            source->knotslist.begin() + requiredKnots);

    target.controlPoints.reserve(controlCount);
    target.weights.reserve(controlCount);
    const bool rational = hasRationalSplineWeights(source);
    for (size_t i = 0; i < controlCount; ++i) {
        const auto& control = source->controllist[i];
        if (!control)
            return false;
        target.controlPoints.push_back({control->x, control->y});
        double weight = 1.0;
        if (rational && i < source->weightlist.size())
            weight = source->weightlist[i];
        if (weight <= 0.0 || !std::isfinite(weight))
            return false;
        target.weights.push_back(weight);
    }

    return true;
}

constexpr double kTableFallbackDimension = 1.0;
constexpr double kTableFallbackMinTextHeight = 0.1;

enum class TableFallbackCellKind {
    Empty,
    PlainText,
    PlaceholderField,
    PlaceholderBlock,
    PlaceholderAttribute,
    PlaceholderUnknown
};

struct TableFallbackCellDisplay {
    QString text;
    TableFallbackCellKind kind = TableFallbackCellKind::Empty;
};

TableFallbackCellDisplay tableCellDisplay(const DRW_TableCell& cell,
                                          bool tableParseComplete) {
    auto placeholder = [](TableFallbackCellKind kind, const char *text) {
        TableFallbackCellDisplay display;
        display.kind = kind;
        display.text = QString::fromLatin1(text);
        return display;
    };

    if (!tableParseComplete || cell.m_geometryFlags != 0
        || cell.m_geometryHandle != 0 || cell.m_overrideFlags != 0
        || cell.m_isMerged) {
        return placeholder(TableFallbackCellKind::PlaceholderUnknown, "[TABLE]");
    }

    for (const DRW_TableCellContent& content : cell.m_contents) {
        if (content.m_type == 4)
            return placeholder(TableFallbackCellKind::PlaceholderBlock, "[BLOCK]");
        if (content.m_type == 2 || content.m_handle != 0)
            return placeholder(TableFallbackCellKind::PlaceholderField, "[FIELD]");
        if (!content.m_text.empty()) {
            return {QString::fromUtf8(content.m_text.c_str()),
                    TableFallbackCellKind::PlainText};
        }
        if (!content.m_value.m_valueString.empty()) {
            return {QString::fromUtf8(content.m_value.m_valueString.c_str()),
                    TableFallbackCellKind::PlainText};
        }
        if (content.m_type != 0 && content.m_type != 1)
            return placeholder(TableFallbackCellKind::PlaceholderUnknown, "[TABLE]");
    }
    if (cell.m_blockHandle != 0)
        return placeholder(TableFallbackCellKind::PlaceholderBlock, "[BLOCK]");
    if (cell.m_valueHandle != 0)
        return placeholder(TableFallbackCellKind::PlaceholderField, "[FIELD]");
    for (const DRW_TableCellAttribute& attribute : cell.m_attributes) {
        if (!attribute.m_text.empty()) {
            return {QString::fromUtf8(attribute.m_text.c_str()),
                    TableFallbackCellKind::PlaceholderAttribute};
        }
        if (attribute.m_attdefHandle != 0)
            return placeholder(TableFallbackCellKind::PlaceholderAttribute, "[ATTR]");
    }
    if (!cell.m_attributes.empty())
        return placeholder(TableFallbackCellKind::PlaceholderAttribute, "[ATTR]");
    return {};
}

bool tableFallbackCellIsPlaceholder(TableFallbackCellKind kind) {
    return kind != TableFallbackCellKind::PlainText
           && kind != TableFallbackCellKind::Empty;
}

double tableColumnWidth(const DRW_TableContent& content, size_t index,
                        RS_FilterDXFRW::TableFallbackRenderSummary *summary) {
    if (index < content.m_columns.size()
        && std::isfinite(content.m_columns[index].m_width)
        && content.m_columns[index].m_width > 0.0) {
        return content.m_columns[index].m_width;
    }
    if (summary != nullptr)
        ++summary->clampedDimensionCount;
    return kTableFallbackDimension;
}

double tableRowHeight(const DRW_TableContent& content, size_t index,
                      RS_FilterDXFRW::TableFallbackRenderSummary *summary) {
    if (index < content.m_rows.size()
        && std::isfinite(content.m_rows[index].m_height)
        && content.m_rows[index].m_height > 0.0) {
        return content.m_rows[index].m_height;
    }
    if (summary != nullptr)
        ++summary->clampedDimensionCount;
    return kTableFallbackDimension;
}

DRW_UnsupportedObject rawObjectFromMetadata(
    const LC_DwgAdvancedMetadata::RawObjectRecord& record) {
    DRW_UnsupportedObject object;
    object.m_objectType = record.objectType;
    object.m_handle = record.handle;
    object.m_bodyBitSize = record.bodyBitSize;
    object.m_objectOffset = record.objectOffset;
    object.m_objectSize = record.objectSize;
    object.m_isEntity = record.isEntity;
    object.m_isCustomClass = record.isCustomClass;
    object.m_recordName = record.recordName;
    object.m_className = record.className;
    object.m_rawBytes = record.rawBytes;
    return object;
}

bool isSunRawObject(const LC_DwgAdvancedMetadata::RawObjectRecord& record) {
    return record.recordName == "SUN" || record.className == "AcDbSun";
}

bool isAcDbPlaceholderRawObject(
    const LC_DwgAdvancedMetadata::RawObjectRecord& record) {
    return record.objectType == 80 || record.recordName == "ACDBPLACEHOLDER"
        || record.className == "AcDbPlaceHolder";
}

bool isMLeaderStyleRawObject(
    const LC_DwgAdvancedMetadata::RawObjectRecord& record) {
    return record.recordName == "MLEADERSTYLE"
        || record.className == "AcDbMLeaderStyle";
}

bool hasReplayableRawMLeaderStyle(const LC_DwgAdvancedMetadata& metadata,
                                  duint32 handle) {
    if (handle == 0)
        return false;
    for (const auto& record : metadata.rawObjects()) {
        if (record.handle == handle && isMLeaderStyleRawObject(record)
            && LC_DwgAdvancedMetadata::rawReplayBlocker(record)
                   == LC_DwgAdvancedMetadata::ReplayBlocker::None) {
            return true;
        }
    }
    return false;
}

DRW_AcDbPlaceholder placeholderFromMetadata(
    const LC_DwgAdvancedMetadata::PlaceholderRecord& record) {
    DRW_AcDbPlaceholder placeholder;
    placeholder.handle = record.handle;
    placeholder.parentHandle = static_cast<int>(record.parentHandle);
    return placeholder;
}

DRW_Sun sunFromMetadata(const LC_DwgAdvancedMetadata::SunRecord& record) {
    DRW_Sun sun;
    sun.handle = record.handle;
    sun.parentHandle = static_cast<int>(record.parentHandle);
    sun.m_classVersion = record.classVersion;
    sun.m_isOn = record.isOn;
    sun.m_color = record.color;
    sun.m_intensity = record.intensity;
    sun.m_hasShadow = record.hasShadow;
    sun.m_julianDay = record.julianDay;
    sun.m_milliseconds = record.milliseconds;
    sun.m_isDaylightSavings = record.isDaylightSavings;
    sun.m_shadowType = record.shadowType;
    sun.m_shadowMapSize = record.shadowMapSize;
    sun.m_shadowSoftness = record.shadowSoftness;
    return sun;
}

DRW_MLeaderStyle mleaderStyleFromMetadata(
    const LC_DwgAdvancedMetadata::MLeaderStyleRecord& record) {
    DRW_MLeaderStyle style;
    style.handle = record.handle;
    style.parentHandle = static_cast<int>(record.parentHandle);
    style.name = record.name;
    style.styleVersion = record.styleVersion != 0 ? record.styleVersion : 2;
    style.contentType = record.contentType;
    style.drawMLeaderOrder = record.drawMLeaderOrder;
    style.drawLeaderOrder = record.drawLeaderOrder;
    style.maxLeaderPoints = record.maxLeaderPoints;
    style.firstSegmentAngle = record.firstSegmentAngle;
    style.secondSegmentAngle = record.secondSegmentAngle;
    style.leaderType = record.leaderType;
    style.leaderColor = record.leaderColor;
    style.leaderLineTypeHandle.ref = record.leaderLineTypeHandle;
    style.leaderLineWeight = record.leaderLineWeight;
    style.landingEnabled = record.landingEnabled;
    style.landingGap = record.landingGap;
    style.autoIncludeLanding = record.autoIncludeLanding;
    style.landingDistance = record.landingDistance;
    style.description = record.description;
    style.arrowHeadBlockHandle.ref = record.arrowHeadBlockHandle;
    style.arrowHeadSize = record.arrowHeadSize;
    style.textDefault = record.textDefault;
    style.textStyleHandle.ref = record.textStyleHandle;
    style.leftAttachment = record.leftAttachment;
    style.rightAttachment = record.rightAttachment;
    style.textAngleType = record.textAngleType;
    style.textAlignmentType = record.textAlignmentType;
    style.textColor = record.textColor;
    style.textHeight = record.textHeight;
    style.textFrameEnabled = record.textFrameEnabled;
    style.alwaysAlignTextLeft = record.alwaysAlignTextLeft;
    style.alignSpace = record.alignSpace;
    style.blockHandle.ref = record.blockHandle;
    style.blockColor = record.blockColor;
    style.blockScale = record.blockScale;
    style.blockScaleEnabled = record.blockScaleEnabled;
    style.blockRotation = record.blockRotation;
    style.blockRotationEnabled = record.blockRotationEnabled;
    style.blockConnectionType = record.blockConnectionType;
    style.scaleFactor = record.scaleFactor;
    style.propertyChanged = record.propertyChanged;
    style.isAnnotative = record.isAnnotative;
    style.breakSize = record.breakSize;
    style.attachmentDirection = record.attachmentDirection;
    style.topAttachment = record.topAttachment;
    style.bottomAttachment = record.bottomAttachment;
    style.textExtended = record.textExtended;
    return style;
}

std::unique_ptr<LC_SplinePoints> approximateDrwSpline(
    RS_EntityContainer* parent, const DRW_Spline* data) {
    RS_SplineData splineData;
    if (!buildSplineDataFromDrw(data, splineData))
        return nullptr;

    const size_t degree = static_cast<size_t>(data->degree);
    const size_t controlCount = splineData.controlPoints.size();
    const double tmin = splineData.knotslist[degree];
    const double tmax = splineData.knotslist[controlCount];
    if (!std::isfinite(tmin) || !std::isfinite(tmax) || tmax <= tmin)
        return nullptr;

    LC_SplinePointsData sampled((data->flags & 0x3) != 0, false);
    sampled.useControlPoints = false;
    sampled.splinePoints.reserve(kImportedSplineFallbackSamples + 1);
    for (int i = 0; i <= kImportedSplineFallbackSamples; ++i) {
        const double u = tmin + (tmax - tmin) *
                         (static_cast<double>(i) / kImportedSplineFallbackSamples);
        const RS_Vector point = RS_Spline::evaluateNURBS(splineData, u);
        if (!point.valid)
            return nullptr;
        sampled.splinePoints.push_back(point);
    }

    if (sampled.closed && sampled.splinePoints.size() > 1 &&
        sampled.splinePoints.front().distanceTo(sampled.splinePoints.back()) <= 1e-8) {
        sampled.splinePoints.pop_back();
    }

    if (sampled.splinePoints.size() < 2)
        return nullptr;

    return std::make_unique<LC_SplinePoints>(parent, std::move(sampled));
}

}

/**
 * Default constructor.
 *
 */
RS_FilterDXFRW::RS_FilterDXFRW()
    :RS_FilterInterface(),DRW_Interface() {
    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW()");

    m_currentContainer = nullptr;
    m_graphic = nullptr;
// Init hash to change the QCAD "normal" style to the more correct ISO-3059
// or draftsight symbol (AR*.shx) to sy*.lff
    m_fontList["arastro"] = "syastro";
    m_fontList["armap"] = "symap";
    m_fontList["armeteo"] = "symeteo";
    m_fontList["armusic"] = "symusic";
    m_fontList["math"] = "symath";
    m_fontList["normal"] = "iso";
    m_fontList["normallatin1"] = "iso";
    m_fontList["normallatin2"] = "iso";

    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW(): OK");
}

/**
 * Destructor.
 */
RS_FilterDXFRW::~RS_FilterDXFRW() {
    RS_DEBUG->print("RS_FilterDXFRW::~RS_FilterDXFRW(): OK");
}

// Human-readable display for a DRW::Version. Year suffixes anchor users
// in time so they can decide whether to convert (pre-R13) or update their
// install (post-R2018 future versions).
static QString dwgVersionDisplay(DRW::Version v) {
  switch (v) {
  case DRW::MC00:
    return QStringLiteral("R1.1 (1982)");
  case DRW::AC12:
    return QStringLiteral("R1.2 (1983)");
  case DRW::AC14:
    return QStringLiteral("R1.4 (1983)");
  case DRW::AC150:
    return QStringLiteral("R2.0 (1984)");
  case DRW::AC210:
    return QStringLiteral("R2.10 (1986)");
  case DRW::AC1002:
    return QStringLiteral("R2.5 (1986)");
  case DRW::AC1003:
    return QStringLiteral("R2.6 (1987)");
  case DRW::AC1004:
    return QStringLiteral("R9 (1987)");
  case DRW::AC1006:
    return QStringLiteral("R10 (1988)");
  case DRW::AC1009:
    return QStringLiteral("R11/R12 (1990)");
  case DRW::AC1012:
    return QStringLiteral("R13 (1994)");
  case DRW::AC1014:
    return QStringLiteral("R14 (1997)");
  case DRW::AC1015:
    return QStringLiteral("AutoCAD 2000");
  case DRW::AC1018:
    return QStringLiteral("AutoCAD 2004");
  case DRW::AC1021:
    return QStringLiteral("AutoCAD 2007");
  case DRW::AC1024:
    return QStringLiteral("AutoCAD 2010");
  case DRW::AC1027:
    return QStringLiteral("AutoCAD 2013");
  case DRW::AC1032:
    return QStringLiteral("AutoCAD 2018");
  case DRW::UNKNOWNV:
  default:
    return QStringLiteral("unknown");
  }
}

QString RS_FilterDXFRW::lastError() const{
    switch (errorCode) {
    case DRW::BAD_NONE:
        return (QObject::tr( "no DXF/DWG error", "RS_FilterDXFRW"));
    case DRW::BAD_OPEN:
        return (QObject::tr( "error opening DXF/DWG file", "RS_FilterDXFRW"));
    case DRW::BAD_VERSION:
      if (m_dwgVersion != DRW::UNKNOWNV) {
        return QObject::
            tr("Cannot open DWG: file is %1; LibreCAD supports %2 and newer. "
               "Convert with GNU LibreDWG (dwgread / dwg2dxf) or re-save "
               "from a recent CAD tool.",
               "RS_FilterDXFRW")
                .arg(dwgVersionDisplay(m_dwgVersion))
                .arg(dwgVersionDisplay(DRW::AC1012));
      }
        return (QObject::tr( "unsupported DXF/DWG file version", "RS_FilterDXFRW"));
    case DRW::BAD_READ_METADATA:
        return (QObject::tr( "error reading DXF/DWG meta data", "RS_FilterDXFRW"));
    case DRW::BAD_READ_FILE_HEADER:
        return (QObject::tr( "error reading DXF/DWG file header", "RS_FilterDXFRW"));
    case DRW::BAD_READ_HEADER:
        return (QObject::tr( "error reading DXF/DWG header data", "RS_FilterDXFRW"));
    case DRW::BAD_READ_HANDLES:
        return (QObject::tr( "error reading DXF/DWG object map", "RS_FilterDXFRW"));
    case DRW::BAD_READ_CLASSES:
        return (QObject::tr( "error reading DXF/DWG classes", "RS_FilterDXFRW"));
    case DRW::BAD_READ_TABLES:
        return (QObject::tr( "error reading DXF/DWG tables", "RS_FilterDXFRW"));
    case DRW::BAD_READ_BLOCKS:
        return (QObject::tr( "error reading DXF/DWG blocks", "RS_FilterDXFRW"));
    case DRW::BAD_READ_ENTITIES:
        return (QObject::tr( "error reading DXF/DWG entities", "RS_FilterDXFRW"));
    case DRW::BAD_READ_OBJECTS:
        return (QObject::tr( "error reading DXF/DWG objects", "RS_FilterDXFRW"));
    case DRW::BAD_READ_SECTION:
        return (QObject::tr( "error reading DXF/DWG sections", "RS_FilterDXFRW"));
    case DRW::BAD_CODE_PARSED:
        return (QObject::tr( "error reading DXF/DWG code", "RS_FilterDXFRW"));
    default:
        break;
    }

    return RS_FilterInterface::lastError();
}

/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The m_graphic in which the entities from the m_file
 * will be created or the graphics from which the entities are
 * taken to be stored in a m_file.
 */
bool RS_FilterDXFRW::fileImport(RS_Graphic& g, const QString& file, [[maybe_unused]] RS2::FormatType type) {
    RS_DEBUG->print("RS_FilterDXFRW::fileImport");
    RS_DEBUG->print("DXFRW Filter: importing file '%s'...", (const char*)QFile::encodeName(file));

    m_graphic = &g;
    m_currentContainer = m_graphic;
    m_dummyContainer = new RS_EntityContainer(nullptr, true);

    this->m_file = file;
    // Register the file being loaded into the XREF recursion guard so
    // A → B → A cycles are detected at depth 2 rather than depth 4.
    // Done via QFileInfo::absoluteFilePath() to match resolveXrefPath()
    // (which returns absolute paths). RAII-erased on return so failure
    // paths (BAD_VERSION, parse failure, etc.) don't leak the entry.
    const QString thisFileAbs = QFileInfo(file).absoluteFilePath();
    const bool stackInsertedSelf = m_xrefStack.insert(thisFileAbs).second;
    struct XrefStackGuard {
      std::set<QString> *stack = nullptr;
      QString key;
      bool owned = false;
      ~XrefStackGuard() {
        if (stack && owned)
          stack->erase(key);
      }
    } stackGuard{&m_xrefStack, thisFileAbs, stackInsertedSelf};
    // add some variables that need to be there for DXF drawings:
    m_graphic->addVariable("$DIMSTYLE", "Standard", 2);
    m_dimStyle = "Standard";
    m_codePage = "ANSI_1252";
    m_textStyle = "Standard";
    //reset library version
    m_isLibDxfRw = false;
    m_libDxfRwVersion = 0;
    m_unsupportedDwgObjects.clear();
    m_graphic->dwgAdvancedMetadata().clear();

#ifdef DWGSUPPORT
    if (type == RS2::FormatDWG) {
        dwgR dwgr(QFile::encodeName(file));
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading DWG file");
        if (RS_DEBUG->getLevel()== RS_Debug::D_DEBUGGING)
            dwgr.setDebug(DRW::DebugLevel::Debug);
        bool success = dwgr.read(this, true);
        // Capture the recognized version BEFORE acting on the result so
        // BAD_VERSION error reporting (printDwgError / lastError) can
        // name the format the user supplied. dwgR::version is set by
        // openFile() even on the BAD_VERSION fork.
        m_dwgVersion = dwgr.getVersion();
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading DWG file: OK");
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Opened DWG file version %1.").arg(printDwgVersion(dwgr.getVersion())));
        const size_t parseFailures = dwgr.getEntityParseFailures();
        if (parseFailures > 0) {
          RS_DIALOGFACTORY->commandMessage(
              QObject::tr("DWG load: %1 %2 had parse errors and were skipped. "
                          "Drawing loaded with the rest.")
                  .arg(parseFailures)
                  .arg(parseFailures == 1 ? QObject::tr("entity")
                                          : QObject::tr("entities")));
        }
        // Vendor-extension custom classes (AutoCAD Mechanical AmgStdPart aka
        // STDPART2D, AcmBomRow, etc.) whose proprietary geometry libdxfrw
        // can't decode.  Surface the top breakdown so the user knows what's
        // missing rather than silently rendering a partial drawing.
        const auto skipped = dwgr.getSkippedCustomClasses();
        if (!skipped.empty()) {
          size_t totalSkipped = 0;
          std::vector<std::pair<QString, size_t>> sorted;
          sorted.reserve(skipped.size());
          for (const auto &kv : skipped) {
            totalSkipped += kv.second;
            sorted.emplace_back(QString::fromStdString(kv.first), kv.second);
          }
          std::sort(
              sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
          QStringList top;
          const size_t showN = std::min<size_t>(3, sorted.size());
          for (size_t i = 0; i < showN; ++i)
            top << QString("%1×%2").arg(sorted[i].second).arg(sorted[i].first);
          QString breakdown = top.join(QLatin1String(", "));
          if (sorted.size() > showN)
            breakdown += QObject::tr(", and %n more class(es)", "",
                                     static_cast<int>(sorted.size() - showN));
          RS_DIALOGFACTORY->commandMessage(
              QObject::tr(
                  "DWG load: %1 vendor-extension entities not rendered (%2). "
                  "These are typically AutoCAD Mechanical or other "
                  "vertical-product "
                  "custom classes that libdxfrw cannot decode.")
                  .arg(totalSkipped)
                  .arg(breakdown));
        }
        const auto unsupportedObjects = dwgr.getSkippedUnsupportedObjects();
        if (!unsupportedObjects.empty()) {
          size_t totalSkipped = 0;
          std::vector<std::pair<QString, size_t>> sorted;
          sorted.reserve(unsupportedObjects.size());
          for (const auto &kv : unsupportedObjects) {
            totalSkipped += kv.second;
            sorted.emplace_back(QString::fromStdString(kv.first), kv.second);
          }
          std::sort(
              sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
          QStringList top;
          const size_t showN = std::min<size_t>(3, sorted.size());
          for (size_t i = 0; i < showN; ++i)
            top << QString("%1×%2").arg(sorted[i].second).arg(sorted[i].first);
          QString breakdown = top.join(QLatin1String(", "));
          if (sorted.size() > showN)
            breakdown += QObject::tr(", and %n more object type(s)", "",
                                     static_cast<int>(sorted.size() - showN));
          RS_DIALOGFACTORY->commandMessage(
              QObject::tr("DWG load: %1 unsupported metadata object(s) skipped "
                          "(%2). Drawing geometry may still be complete.")
                  .arg(totalSkipped)
                  .arg(breakdown));
        }
        RS_DEBUG->print("DWG read summary: %d entities, %d blocks, error=%d",
                        m_graphic ? m_graphic->count() : -1,
                        m_graphic ? m_graphic->countBlocks() : -1,
                        dwgr.getError());
        int  lastError = dwgr.getError();
        if (false == success) {
            printDwgError(lastError);
            RS_DEBUG->print(RS_Debug::D_WARNING,"Cannot open DWG file '%s'.", (const char*)QFile::encodeName(file));
            errorCode = dwgr.getError();
            return false;
        }
    } else {
#endif

        m_dxfR = new dxfRW(QFile::encodeName(file));

        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file");
        if (RS_Debug::D_DEBUGGING == RS_DEBUG->getLevel()) {
            m_dxfR->setDebug(DRW::DebugLevel::Debug);
        }
        bool success {false};
        if (file.startsWith(":")) { // load content from resources. It SHOULD be present in resource!
            QFile resourceFile(file);
            if (resourceFile.open(QIODevice::ReadOnly)) {
                QByteArray contentString = resourceFile.readAll();
                resourceFile.close();
                std::string content = contentString.toStdString();
                success = m_dxfR->readAscii(this, true, content);
            }
        }
        else {
            success = m_dxfR->read(this, true);
        }
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file: OK");
        //graphic->setAutoUpdateBorders(true);

        if (false == success) {
            RS_DEBUG->print(RS_Debug::D_WARNING,"Cannot open DXF file '%s'.", (const char*)QFile::encodeName(file));
            errorCode = m_dxfR->getError();
            delete m_dxfR;
            return false;
        }
        else {
            delete m_dxfR;
        }
#ifdef DWGSUPPORT
    }
#endif

    delete m_dummyContainer;
    /*set current layer */
    auto cl = m_graphic->findLayer(m_graphic->getVariableString("$CLAYER", "0"));
	if (cl ){
        //require to notify
        m_graphic->getLayerList()->activate(cl, true);
    }
    RS_DEBUG->print("RS_FilterDXFRW::fileImport: updating inserts");
    m_graphic->updateInserts();

    // Orphan XREF detection. An XREF block whose contents were embedded
    // is still invisible in modelspace unless something INSERTs it.
    // AutoCAD typically renders these through a paper-space layout
    // viewport, which LibreCAD doesn't implement, so the user should
    // know why the externally-referenced geometry isn't visible.
    // Only run for the outer fileImport (m_xrefStack empty); skip when
    // recursively loading an XREF source.
    if (m_xrefStack.empty() && !m_xrefBlockNames.empty()) {
      std::set<QString> referenced;
      for (auto *e : *m_graphic) {
        if (e && e->rtti() == RS2::EntityInsert) {
          referenced.insert(static_cast<RS_Insert *>(e)->getName());
        }
      }
      QStringList orphans;
      for (const QString &xrefName : m_xrefBlockNames) {
        if (!referenced.count(xrefName))
          orphans << xrefName;
      }
      if (!orphans.isEmpty()) {
        RS_DIALOGFACTORY->commandMessage(
            QObject::tr("DWG/DXF load: %1 XREF block(s) (%2) loaded but not "
                        "INSERTed into modelspace. Their externally-referenced "
                        "geometry won't be visible — AutoCAD typically renders "
                        "these through a paper-space layout viewport, which "
                        "LibreCAD doesn't render.")
                .arg(orphans.size())
                .arg(orphans.join(QStringLiteral(", "))));
      }
    }

    RS_DEBUG->print("RS_FilterDXFRW::fileImport OK");
    return true;
}

/**
 * Implementation of the method which handles layers.
 */
void RS_FilterDXFRW::addLayer(const DRW_Layer &data) {
    RS_DEBUG->print("RS_FilterDXF::addLayer");
    RS_DEBUG->print("  adding layer: %s", data.name.c_str());

    RS_DEBUG->print("RS_FilterDXF::addLayer: creating layer");

    QString name = QString::fromUtf8(data.name.c_str());
    if (name != "0" && m_graphic->findLayer(name)) {
        return;
    }
    auto* layer = new RS_Layer(name);
    RS_DEBUG->print("RS_FilterDXF::addLayer: set pen");
    layer->setPen(attributesToPen(&data));

    RS_DEBUG->print("RS_FilterDXF::addLayer: flags");
    if (data.flags&0x01) {
        layer->freeze(true);
    }
    if (data.flags&0x04) {
        layer->lock(true);
    }
    layer->setPrint(data.plotF);

    //parse extended data to read construction flag
    if (!data.extData.empty()){
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::addLayer: layer %s have extended data", layer->getName().toStdString().c_str());
        bool isLCdata = false;
        for (std::vector<DRW_Variant*>::const_iterator it=data.extData.begin(); it!=data.extData.end(); ++it){
            if ((*it)->code() == 1001){
                if (*(*it)->content.s == std::string("LibreCad")) {
                    isLCdata = true;
                }
                else {
                    isLCdata = false;
                }
            } else if (isLCdata && (*it)->code() == 1070){
                if ((*it)->content.i == 1){
                    layer->setConstruction(true);
                }
            }
        }
    }
    //pre dxfrw 0.5.13 plot flag are used to store construction layer
    if( m_isLibDxfRw && m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 5, 13)) {
        layer->setConstruction(! data.plotF);
    }

    if (layer->isConstruction()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::addLayer: layer %s is construction layer", layer->getName().toStdString().c_str());
    }

    RS_DEBUG->print("RS_FilterDXF::addLayer: add layer to graphic");
    m_graphic->addLayer(layer);
    RS_DEBUG->print("RS_FilterDXF::addLayer: OK");
}

/**
 * Implementation of the method which handles dimension styles.
 */
void RS_FilterDXFRW::addDimStyle(const DRW_Dimstyle& data){
    RS_DEBUG->print("RS_FilterDXFRW::addLayer");
    QString dimStyleName = m_graphic->getVariableString("$DIMSTYLE", "standard");

    if (QString::compare(data.name.c_str(), dimStyleName, Qt::CaseInsensitive) == 0) {
        if( m_isLibDxfRw && m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 6, 2)) {
            m_graphic->addVariable("$DIMDEC", m_graphic->getVariableInt("$DIMDEC",
                                            m_graphic->getVariableInt("$LUPREC", 4)), 70);
            m_graphic->addVariable("$DIMADEC", m_graphic->getVariableInt("$DIMADEC",
                                             m_graphic->getVariableInt("$AUPREC", 2)), 70);
            //do nothing;
        } else {
            m_graphic->addVariable("$DIMDEC", data.dimdec, 70);
            m_graphic->addVariable("$DIMADEC", data.dimadec, 70);
        }
    }

    LC_DimStyle* dimStyle = createDimStyle(data);
    m_graphic->addDimStyle(dimStyle);
}

// todo - sand - ucs - rework to direct setup of LC_GraphicViewport instead of RS_GraphicView
// But this modification requires cleanup of the overall file open flow, so leave it as it is for now
/**
 * Implementation of the method which handles vports.
 */
void RS_FilterDXFRW::addVport(const DRW_Vport &data) {
    if (m_graphic != nullptr)
        m_graphic->dwgAdvancedMetadata().addVport(data);
    QString name = QString::fromStdString(data.name);
    if (name.toLower() == "*active") {
        data.grid == 1? m_graphic->setGridOn(true):m_graphic->setGridOn(false);
        m_graphic->setIsometricGrid(data.snapStyle);
        m_graphic->setIsoView( (RS2::IsoGridViewType)data.snapIsopair);
        RS_GraphicView *gv = m_graphic->getGraphicView();  // fixme - sand - review this dependency
        if (gv ) {
            double width = data.height * data.ratio;
            // todo - sand - ucs - investigate support/usage of different x and y factors.
            double factorX= gv->getWidth() / width;
            double factorY= gv->getHeight() / data.height;
            if (factorX > factorY) {
                factorX = factorY;
            }
            int ox = gv->getWidth() - data.center.x*2*factorX;
            int oy = gv->getHeight() - data.center.y*2*factorX;
            gv->getViewPort()->justSetOffsetAndFactor(ox, oy, factorX);
        }
    }
}

void RS_FilterDXFRW::addUCS(const DRW_UCS &data) {
    RS_DEBUG->print("RS_FilterDXF::addUCS");
    RS_DEBUG->print("  adding ucs: %s", data.name.c_str());
    RS_DEBUG->print("RS_FilterDXF::addUCS: creating ucs");

    QString name = QString::fromUtf8(data.name.c_str());
    if (m_graphic != nullptr)
        m_graphic->dwgAdvancedMetadata().addUcs(data);
    if (!name.isEmpty() && m_graphic->findNamedUCS(name) != nullptr) {
        const int existingIndex = m_graphic->getUCSList()->getIndex(name);
        m_graphic->dwgAdvancedMetadata().mapUcsToDocumentItem(
            data.handle, data.name, existingIndex);
        return;
    }

    auto* ucs = new LC_UCS(name);
    auto origin = RS_Vector(data.origin.x, data.origin.y, data.origin.z);
    ucs->setOrigin(origin);

    ucs->setElevation(data.elevation);
    ucs->setOrthoType(data.orthoType);

    auto orthoOrigin = RS_Vector(data.orthoOrigin.x, data.orthoOrigin.y, data.orthoOrigin.z);
    ucs->setOrthoOrigin(orthoOrigin);

    auto xAxis = RS_Vector(data.xAxisDirection.x, data.xAxisDirection.y, data.xAxisDirection.z);
    ucs->setXAxis(xAxis);

    auto yAxis = RS_Vector(data.yAxisDirection.x, data.yAxisDirection.y, data.yAxisDirection.z);
    ucs->setYAxis(yAxis);

    RS_DEBUG->print("RS_FilterDXF::addUCS: add ucs to graphic");
    m_graphic->addUCS(ucs);
    const int documentItemIndex = m_graphic->getUCSList()->getIndex(name);
    m_graphic->dwgAdvancedMetadata().mapUcsToDocumentItem(
        data.handle, data.name, documentItemIndex);
    RS_DEBUG->print("RS_FilterDXF::addUCS: OK");
}

void RS_FilterDXFRW::addView(const DRW_View &data) {
    RS_DEBUG->print("RS_FilterDXF::addView");
    RS_DEBUG->print("  adding view: %s", data.name.c_str());
    RS_DEBUG->print("RS_FilterDXF::addView: creating view");

    if (m_graphic != nullptr) {
        m_graphic->dwgAdvancedMetadata().addView(data);
    }
    QString name = QString::fromUtf8(data.name.c_str());
    if (!name.isEmpty() && m_graphic->findNamedView(name) != nullptr) {
        const int existingIndex = m_graphic->getViewList()->getIndex(name);
        m_graphic->dwgAdvancedMetadata().mapViewToDocumentItem(
            data.handle, data.name, existingIndex);
        return;
    }
    auto* view = new LC_View(name);
    auto center = RS_Vector(data.center.x, data.center.y, data.center.z);
    view->setCenter(center);

    auto size = RS_Vector(data.size.x, data.size.y, data.size.z);
    view->setSize(size);

    auto targetPoint = RS_Vector(data.targetPoint.x, data.targetPoint.y, data.targetPoint.z);
    view->setTargetPoint(targetPoint);

    auto viewDirection = RS_Vector(data.viewDirectionFromTarget.x, data.viewDirectionFromTarget.y, data.viewDirectionFromTarget.z);
    view->setViewDirection(viewDirection);

    view->setLensLen(data.lensLen);
    view->setCameraPlottable(data.cameraPlottable);

    view->setRenderMode(data.renderMode);
    view->setBackClippingPlaneOffset(data.backClippingPlaneOffset);
    view->setFrontClippingPlaneOffset(data.frontClippingPlaneOffset);
    view->setTwistAngle(data.twistAngle);
    view->setFlags(data.flags); // todo - review, use differ properties?
    view->setViewMode(data.viewMode); // todo - probably it might be simpler than long?

    if (data.hasUCS){
        auto ucs = new LC_UCS();

        auto ucsOrigin = RS_Vector(data.ucsOrigin.x, data.ucsOrigin.y, data.ucsOrigin.z);
        auto ucsXAxis = RS_Vector(data.ucsXAxis.x, data.ucsXAxis.y, data.ucsXAxis.z);
        auto ucsYAxis = RS_Vector(data.ucsYAxis.x, data.ucsYAxis.y, data.ucsYAxis.z);
        ucs->setOrthoOrigin(RS_Vector(false));
        ucs->setOrigin(ucsOrigin);
        ucs->setXAxis(ucsXAxis);
        ucs->setYAxis(ucsYAxis);
        ucs->setElevation(data.ucsElevation);
        ucs->setOrthoType(data.ucsOrthoType);
        view->setUCS(ucs);
    }

    RS_DEBUG->print("RS_FilterDXF::addView: set pen");

    RS_DEBUG->print("RS_FilterDXF::addView: add view to graphic");
    m_graphic->addNamedView(view);
    const int documentItemIndex = m_graphic->getViewList()->getIndex(name);
    m_graphic->dwgAdvancedMetadata().mapViewToDocumentItem(
        data.handle, data.name, documentItemIndex);
    RS_DEBUG->print("RS_FilterDXF::addView: OK");
}

void RS_FilterDXFRW::addVisualStyle(const DRW_VisualStyle& data) {
    if (m_graphic != nullptr)
        m_graphic->dwgAdvancedMetadata().addVisualStyle(data);
}

/**
 * Implementation of the method which handles blocks.
 *
 * @todo Adding blocks to blocks (stack for m_currentContainer)
 */
void RS_FilterDXFRW::addBlock(const DRW_Block& data) {
    RS_DEBUG->print("RS_FilterDXF::addBlock");
    RS_DEBUG->print("  adding block: %s", data.name.c_str());
/*TODO correct handle of model-space*/

    QString name = QString::fromUtf8(data.name.c_str());
    QString mid = name.mid(1,11);
// Prevent special blocks (paper_space, model_space) from being added:
    if (mid.toLower() != "paper_space" && mid.toLower() != "model_space") {
            RS_Vector bp(data.basePoint.x, data.basePoint.y);
            auto block = new RS_Block(m_graphic, RS_BlockData(name, bp, false ));
            block->setInsertionUnits(data.insUnits);
            //block->setFlags(flags);

            if (m_graphic->addBlock(block)) {
                m_currentContainer = block;
                m_blockHash.insert(data.parentHandle, m_currentContainer);

                // Bound/unbound XREF: load the external file and embed its
                // modelspace contents into this block. Layers are namespaced
                // `<blockName>|<layerName>` per AutoCAD BIND convention.
                // Bit 0x04 = XREF, bit 0x08 = XREF overlay.
                const bool isXref = (data.flags & 0x0c) != 0;
                if (isXref) {
                  m_xrefBlockNames.insert(name);
                  if (!data.xrefPath.empty()) {
                    embedXref(block, QString::fromUtf8(data.xrefPath.c_str()),
                              name);
                  }
                }
            } else {
              // RS_BlockList::add returns false on name collision and
              // deletes the block. Without updating m_currentContainer,
              // subsequent addEntity calls would leak into whatever the
              // previous endBlock left behind (typically m_graphic). Route
              // them to m_dummyContainer so the failure is contained.
              m_blockHash.insert(data.parentHandle, m_dummyContainer);
              m_currentContainer = m_dummyContainer;
            }
    } else {
        if (mid.toLower() == "model_space") {
            m_blockHash.insert(data.parentHandle, m_graphic);
        } else {
            m_blockHash.insert(data.parentHandle, m_dummyContainer);
        }
    }
}

QString RS_FilterDXFRW::resolveXrefPath(const QString &xrefPath) const {
  if (xrefPath.isEmpty())
    return {};

  // Normalize separators: XREFs authored on Windows store backslashes
  // that QFileInfo doesn't recognize on Unix.
  QString normalized = xrefPath;
  normalized.replace('\\', '/');

  // 1. Try the stored path verbatim (could be absolute on this host).
  if (QFileInfo::exists(normalized))
    return QFileInfo(normalized).absoluteFilePath();

  QFileInfo host(m_file);
  const QString hostDir = host.absolutePath();

  // 2. Treat stored path as relative to host file's directory.
  {
    const QString f = hostDir + "/" + normalized;
    if (QFileInfo::exists(f))
      return QFileInfo(f).absoluteFilePath();
  }

  // 3. host-dir + basename (most common: XREF was authored on a
  //    different machine but file shipped alongside the host).
  const QString basename = QFileInfo(normalized).fileName();
  {
    const QString f = hostDir + "/" + basename;
    if (QFileInfo::exists(f))
      return QFileInfo(f).absoluteFilePath();
  }

  // 4-5. Tolerant match in host-dir: case-insensitive + space-to-underscore
  //      normalization on the stem; accept .dwg or .dxf extension.
  const QString basenameNorm = basename.toLower().replace(' ', '_');
  QFileInfo basenameInfo(basenameNorm);
  const QString stemNorm = basenameInfo.completeBaseName();
  QDir dir(hostDir);
  const QStringList entries =
      dir.entryList({QStringLiteral("*.dwg"), QStringLiteral("*.DWG"),
                     QStringLiteral("*.dxf"), QStringLiteral("*.DXF")},
                    QDir::Files);
  for (const QString &entry : entries) {
    const QString entryNorm = entry.toLower().replace(' ', '_');
    if (entryNorm == basenameNorm) {
      return dir.absoluteFilePath(entry);
    }
    QFileInfo entryInfo(entry);
    const QString entryStemNorm =
        entryInfo.completeBaseName().toLower().replace(' ', '_');
    if (entryStemNorm == stemNorm) {
      return dir.absoluteFilePath(entry);
    }
  }

  return {};
}

bool RS_FilterDXFRW::embedXref(RS_Block *block, const QString &xrefPath,
                               const QString &blockName) {
  if (!block)
    return false;
  RS_DEBUG->print("RS_FilterDXFRW::embedXref: %s -> %s", qPrintable(blockName),
                  qPrintable(xrefPath));

  const QString resolved = resolveXrefPath(xrefPath);
  if (resolved.isEmpty()) {
    RS_DIALOGFACTORY->commandMessage(
        QObject::tr("XREF not resolved for block \"%1\": %2 (file not found in "
                    "host directory). The block will render as empty.")
            .arg(blockName, xrefPath));
    return false;
  }

  if (m_xrefStack.count(resolved) > 0) {
    RS_DEBUG->print("  XREF cycle detected, skipping: %s",
                    qPrintable(resolved));
    return false;
  }
  if (m_xrefStack.size() >= 8) {
    RS_DEBUG->print("  XREF depth limit reached at %s", qPrintable(resolved));
    return false;
  }
  m_xrefStack.insert(resolved);

  RS_Graphic ext;
  RS_FilterDXFRW xrefFilter;
  xrefFilter.m_xrefStack = m_xrefStack; // propagate cycle guard
  const bool isDwg =
      resolved.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive);
  const RS2::FormatType fmt = isDwg ? RS2::FormatDWG : RS2::FormatDXFRW;
  const bool ok = xrefFilter.fileImport(ext, resolved, fmt);

  m_xrefStack.erase(resolved);

  if (!ok) {
    RS_DIALOGFACTORY->commandMessage(
        QObject::tr("XREF load failed for block \"%1\": %2")
            .arg(blockName, resolved));
    return false;
  }

  // Layer namespacing: copy the XREF's layers into the host with prefix.
  RS_LayerList *extLayers = ext.getLayerList();
  RS_LayerList *hostLayers = m_graphic ? m_graphic->getLayerList() : nullptr;
  if (extLayers && hostLayers) {
    for (unsigned i = 0; i < extLayers->count(); ++i) {
      RS_Layer *extLyr = extLayers->at(i);
      if (!extLyr)
        continue;
      const QString nsName = blockName + "|" + extLyr->getName();
      if (!hostLayers->find(nsName)) {
        RS_Layer *clone = extLyr->clone();
        clone->setName(nsName);
        hostLayers->add(clone);
      }
    }
  }

  // Per-entity clone helper: clone @p src, redirect layer pointer to the
  // host's namespaced layer, and (if it's an INSERT) rewrite its block
  // name reference to the namespaced form. Returns the cloned entity
  // owned by no parent yet; caller adds it to the target container.
  auto cloneAndRedirect = [&](const RS_Entity *src,
                              RS_EntityContainer *target) -> RS_Entity * {
    if (!src)
      return nullptr;
    RS_Entity *cloned = src->clone();
    if (!cloned)
      return nullptr;
    cloned->setParent(target);
    if (hostLayers && cloned->getLayer()) {
      const QString lyrNs = blockName + "|" + cloned->getLayer()->getName();
      if (auto *hostLyr = hostLayers->find(lyrNs)) {
        cloned->setLayer(hostLyr);
      }
    }
    if (cloned->rtti() == RS2::EntityInsert) {
      auto *ins = static_cast<RS_Insert *>(cloned);
      const QString ref = ins->getName();
      // Don't rewrite if the reference is already namespaced (defensive
      // — shouldn't happen for a freshly-loaded XREF source, but harmless).
      if (!ref.startsWith(blockName + "|")) {
        ins->setName(blockName + "|" + ref);
      }
    }
    return cloned;
  };

  // Block-def propagation: clone every block definition from the XREF
  // source into the host's blockList with namespaced name. Without this,
  // any cloned RS_Insert (whether at modelspace top-level or inside a
  // cloned block def) would reference a block name that doesn't exist
  // in the host, leaving a dangling reference.
  RS_BlockList *extBlocks = ext.getBlockList();
  RS_BlockList *hostBlocks = m_graphic ? m_graphic->getBlockList() : nullptr;
  int blockDefsCopied = 0;
  if (extBlocks && hostBlocks) {
    for (int i = 0; i < extBlocks->count(); ++i) {
      RS_Block *extBk = extBlocks->at(i);
      if (!extBk)
        continue;
      const QString origName = extBk->getName();
      // Skip special blocks — *MODEL_SPACE/*PAPER_SPACE aren't in
      // blockList anyway, but be defensive.
      if (origName.startsWith(QLatin1String("*Model_Space"),
                              Qt::CaseInsensitive) ||
          origName.startsWith(QLatin1String("*Paper_Space"),
                              Qt::CaseInsensitive)) {
        continue;
      }
      const QString nsName = blockName + "|" + origName;
      if (hostBlocks->find(nsName))
        continue; // already present

      auto nsBlock = std::make_unique<RS_Block>(
          m_graphic, RS_BlockData(nsName, extBk->getBasePoint(), false));
      for (auto *e : *extBk) {
        if (auto *cloned = cloneAndRedirect(e, nsBlock.get())) {
          nsBlock->addEntity(cloned);
        }
      }
      // RS_BlockList::add takes ownership on success and deletes on failure,
      // so either way the raw pointer is consumed once released here.
      if (!hostBlocks->add(nsBlock.release())) {
        // Name collision against an existing host block — already deleted.
      } else {
        ++blockDefsCopied;
      }
    }
  }

  // Move modelspace entities (top-level) into the local block, with
  // their layer pointers + INSERT block-name references redirected.
  int moved = 0;
  QList<RS_Entity *> snapshot;
  for (auto *e : ext)
    snapshot.push_back(e);
  for (RS_Entity *e : snapshot) {
    if (auto *cloned = cloneAndRedirect(e, block)) {
      block->addEntity(cloned);
      ++moved;
    }
  }

  RS_DEBUG->print("  XREF embedded: %d entities + %d block defs into "
                  "block \"%s\"",
                  moved, blockDefsCopied, qPrintable(blockName));
  return true;
}

void RS_FilterDXFRW::setBlock(const int handle){
    if (m_blockHash.contains(handle)) {
        m_currentContainer = m_blockHash.value(handle);
    } else {
        m_currentContainer = m_graphic;
    }
}

/**
 * Implementation of the method which closes blocks.
 */
void RS_FilterDXFRW::endBlock() {
    if (m_currentContainer->rtti() == RS2::EntityBlock) {
        auto bk = static_cast<RS_Block*>(m_currentContainer);
        //remove unnamed blocks *D only if version != R12
        if (m_version!=1009) {
            if (bk->getName().startsWith("*D") ) {
                m_graphic->removeBlock(bk);
            }
        }
    }
    m_currentContainer = m_graphic;
}

/**
 * Implementation of the method which handles point entities.
 */
void RS_FilterDXFRW::addPoint(const DRW_Point& data) {
    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_Point* entity = new RS_Point(m_currentContainer,RS_PointData(v));
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addLine(const DRW_Line& data) {
    RS_DEBUG->print("RS_FilterDXF::addLine");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addLine: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addLine: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addLine: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addLine: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
	}

    RS_DEBUG->print("RS_FilterDXF::addLine: OK");
}

/**
 * Implementation of the method which handles ray entities.
 */
void RS_FilterDXFRW::addRay(const DRW_Ray& data) {
    RS_DEBUG->print("RS_FilterDXF::addRay");

	RS_Vector v1{data.basePoint.x, data.basePoint.y};
	RS_Vector v2{data.basePoint.x+data.secPoint.x,
				data.basePoint.y+data.secPoint.y};

    RS_DEBUG->print("RS_FilterDXF::addRay: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addRay: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addRay: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addRay: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
    }

    RS_DEBUG->print("RS_FilterDXF::addRay: OK");
}

/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addXline(const DRW_Xline& data) {
    RS_DEBUG->print("RS_FilterDXF::addXline");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.basePoint.x+data.secPoint.x, data.basePoint.y+data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addXline: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addXline: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addXline: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addXline: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
	}

    RS_DEBUG->print("RS_FilterDXF::addXline: OK");
}

/**
 * Implementation of the method which handles circle entities.
 */
void RS_FilterDXFRW::addCircle(const DRW_Circle& data) {
    RS_DEBUG->print("RS_FilterDXF::addCircle");

	RS_Vector v{data.basePoint.x, data.basePoint.y};
    auto entity = new RS_Circle(m_currentContainer, {v, data.radious});
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles arc entities.
 *
 * @param angle1 Start angle in deg (!)
 * @param angle2 End angle in deg (!)
 */
void RS_FilterDXFRW::addArc(const DRW_Arc& data) {
    RS_DEBUG->print("RS_FilterDXF::addArc");
    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_ArcData d(v, data.radious,
                 data.staangle,
                 data.endangle,
                 false);
    RS_Arc* entity = new RS_Arc(m_currentContainer, d);
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles ellipse entities.
 *
 * @param angle1 Start angle in rad (!)
 * @param angle2 End angle in rad (!)
 */
void RS_FilterDXFRW::addEllipse(const DRW_Ellipse& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addEllipse");

	RS_Vector v1(data.basePoint.x, data.basePoint.y);
	RS_Vector v2(data.secPoint.x, data.secPoint.y);
	double ang2 = data.endparam;
	if (fabs(ang2 - 2.*M_PI) < RS_TOLERANCE && fabs(data.staparam) < RS_TOLERANCE) {
	    ang2 = 0.;
	}
    auto entity = new RS_Ellipse{m_currentContainer, {v1, v2,data.ratio,
										data.staparam, ang2, false}};
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles trace entities.
 */
void RS_FilterDXFRW::addTrace(const DRW_Trace& data) {
    RS_Solid* entity;
	RS_Vector v1{data.basePoint.x, data.basePoint.y};
	RS_Vector v2{data.secPoint.x, data.secPoint.y};
	RS_Vector v3{data.thirdPoint.x, data.thirdPoint.y};
	RS_Vector v4{data.fourPoint.x, data.fourPoint.y};
    if (v3 == v4) {
        entity = new RS_Solid(m_currentContainer, RS_SolidData(v1, v2, v3));
    }
    else {
        entity = new RS_Solid(m_currentContainer, RS_SolidData(v1, v2, v3,v4));
    }

    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

void RS_FilterDXFRW::addTolerance(const DRW_Tolerance& data) {
    RS_Vector insertionPoint{data.insertionPoint.x, data.insertionPoint.y};
    RS_Vector axisDirectionVector{data.xAxisDirectionVector.x, data.xAxisDirectionVector.y};

    QString text = toNativeString(QString::fromUtf8(data.text.c_str()));

    QString sty = QString::fromUtf8(data.dimStyleName.c_str());
    if (sty.isEmpty()) {
        sty = m_dimStyle;
    }

    LC_ToleranceData tolData = LC_ToleranceData(insertionPoint, axisDirectionVector,
                                               text, sty);

    auto entity = new LC_Tolerance{m_currentContainer, tolData};
    setEntityAttributes(entity, &data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles solid entities.
 */
void RS_FilterDXFRW::addSolid(const DRW_Solid& data) {
    addTrace(data);
}

void RS_FilterDXFRW::addModelerGeometry(const DRW_ModelerGeometry &data) {
    // TODO: Preserve/render ACIS SAT/SAB bodies when LibreCAD grows native
    // modeler-geometry support. For now this shell prevents silent loss.
    if (m_graphic != nullptr) {
        m_graphic->dwgAdvancedMetadata().addModelerGeometry(data);
    }
    RS_DEBUG->print("RS_FilterDXFRW::addModelerGeometry: type %d handle %d history %d",
                    static_cast<int>(data.eType),
                    static_cast<int>(data.handle),
                    static_cast<int>(data.m_historyHandle));
}

void RS_FilterDXFRW::addLight(const DRW_Light &data) {
    if (m_graphic != nullptr) {
        m_graphic->dwgAdvancedMetadata().addLight(data);
    }
    RS_DEBUG->print("RS_FilterDXFRW::addLight: %s handle %d",
                    data.m_name.empty() ? "(unnamed)" : data.m_name.c_str(),
                    static_cast<int>(data.handle));
}

/**
 * Implementation of the method which handles lightweight polyline entities.
 */
void RS_FilterDXFRW::addLWPolyline(const DRW_LWPolyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addLWPolyline");
    if (data.vertlist.empty()) {
        return;
    }
    RS_PolylineData d(RS_Vector{},
                      RS_Vector{},
                      data.flags&0x1);
    auto polyline = std::make_unique<RS_Polyline>(m_currentContainer, d);
    setEntityAttributes(polyline.get(), &data);

    std::vector<std::pair<RS_Vector, double> > verList;
    for (auto const& v: data.vertlist) {
        verList.emplace_back(std::make_pair(RS_Vector{v->x, v->y}, v->bulge));
    }

    polyline->appendVertexs(verList);
    const bool defaultExtrusion = data.extPoint.x == 0.0
        && data.extPoint.y == 0.0
        && data.extPoint.z == 1.0;
    bool hasVertexMetadata = false;
    for (const auto& v : data.vertlist) {
        if (v && (v->stawidth != 0.0 || v->endwidth != 0.0
                  || v->identifier != 0)) {
            hasVertexMetadata = true;
            break;
        }
    }
    if (data.width != 0.0 || data.elevation != 0.0 || data.thickness != 0.0
        || !defaultExtrusion || hasVertexMetadata) {
        std::vector<std::shared_ptr<DRW_Variant>> ext;
        ext.push_back(std::make_shared<DRW_Variant>(
            1001, std::string("LibreCAD_LWPOLYLINE")));
        ext.push_back(std::make_shared<DRW_Variant>(1040, data.width));
        ext.push_back(std::make_shared<DRW_Variant>(1040, data.elevation));
        ext.push_back(std::make_shared<DRW_Variant>(1040, data.thickness));
        ext.push_back(std::make_shared<DRW_Variant>(1010, data.extPoint));
        ext.push_back(std::make_shared<DRW_Variant>(
            1070, dint32{static_cast<int>(data.vertlist.size())}));
        for (const auto& v : data.vertlist) {
            ext.push_back(std::make_shared<DRW_Variant>(
                1040, v ? v->stawidth : 0.0));
            ext.push_back(std::make_shared<DRW_Variant>(
                1040, v ? v->endwidth : 0.0));
            ext.push_back(std::make_shared<DRW_Variant>(
                1071, dint32{v ? v->identifier : 0}));
        }
        polyline->setDrwExtData(std::move(ext));
    }
    m_currentContainer->addEntity(polyline.release());
}

/**
 * Implementation of the method which handles MLINESTYLE table entries.
 * Cache by name so a later addMLine can resolve element offsets/colors.
 */
void RS_FilterDXFRW::addMLineStyle(const DRW_MLineStyle &data) {
  RS_DEBUG->print("RS_FilterDXFRW::addMLineStyle: %s (%zu elements)",
                  data.name.c_str(), data.elements.size());
  QString key = QString::fromUtf8(data.name.c_str());
  if (!key.isEmpty()) {
    m_mlineStyleCache[key] = data;
  }
}

/**
 * Implementation of the method which handles MLINE entities.
 *
 * LibreCAD has no native multiline. Decompose each MLINE into N parallel
 * RS_Polylines (one per element in the referenced MLINESTYLE), with
 * round-trip metadata stored on each polyline's drwExtData so the export
 * path can reconstruct an MLINE on save.
 */
void RS_FilterDXFRW::addMLine(const DRW_MLine *data) {
  RS_DEBUG->print("RS_FilterDXFRW::addMLine");
  if (!data || data->vertlist.empty() || data->numLines == 0)
    return;

  const int N = data->numLines;
  QString styleName = QString::fromUtf8(data->styleName.c_str());
  auto styleIt = m_mlineStyleCache.find(styleName);
  const DRW_MLineStyle *style =
      (styleIt != m_mlineStyleCache.end()) ? &styleIt->second : nullptr;

  // Compute effective per-element offsets after justification + scale.
  // Justification: 0=top, 1=zero/middle, 2=bottom. The reference offset
  // is removed from each element's offset so the chosen line aligns
  // with the baseline polyline path.
  std::vector<double> effOffsets(N, 0.0);
  if (style && static_cast<int>(style->elements.size()) == N) {
    double ref = 0.0;
    if (data->justification == 0) { // top
      for (auto &e : style->elements)
        ref = std::max(ref, e.offset);
    } else if (data->justification == 2) { // bottom
      for (auto &e : style->elements)
        ref = std::min(ref, e.offset);
    }
    for (int i = 0; i < N; ++i) {
      effOffsets[i] = (style->elements[i].offset - ref) * data->scale;
    }
  } else {
    // Fallback: evenly spaced [-(N-1)/2 .. +(N-1)/2] * scale.
    for (int i = 0; i < N; ++i) {
      effOffsets[i] = (i - 0.5 * (N - 1)) * data->scale;
    }
  }

  const QString mlineId = QStringLiteral("mline_%1").arg(data->handle);
  const bool closed = (data->openClosed & 0x1) != 0;

  for (int i = 0; i < N; ++i) {
    RS_PolylineData pd(RS_Vector{}, RS_Vector{}, closed);
    auto polyline = std::make_unique<RS_Polyline>(m_currentContainer, pd);
    setEntityAttributes(polyline.get(), data);

    for (const auto &v : data->vertlist) {
      RS_Vector miter(v.miterDir.x, v.miterDir.y);
      const double mlen = miter.magnitude();
      RS_Vector pos;
      if (mlen < RS_TOLERANCE) {
        pos = RS_Vector{v.position.x, v.position.y};
      } else {
        miter /= mlen;
        pos = RS_Vector{v.position.x + miter.x * effOffsets[i],
                        v.position.y + miter.y * effOffsets[i]};
      }
      polyline->addVertex(pos, 0.0, false);
    }

    // Round-trip metadata as XDATA. Schema per the implementation plan:
    //   1001 "LibreCAD_MLINE", 1000 mlineId, 1000 styleName,
    //   1040 scale, 1070 justification, 1070 elementCount,
    //   1070 elementIndex, 1040 offset, 1070 flags.
    // Anchor (i==0) additionally stores per-vertex baseline + miter
    // so the export side can reconstruct without averaging.
    std::vector<std::shared_ptr<DRW_Variant>> ext;
    ext.push_back(
        std::make_shared<DRW_Variant>(1001, std::string("LibreCAD_MLINE")));
    ext.push_back(std::make_shared<DRW_Variant>(1000, mlineId.toStdString()));
    ext.push_back(std::make_shared<DRW_Variant>(1000, data->styleName));
    ext.push_back(std::make_shared<DRW_Variant>(1040, data->scale));
    ext.push_back(
        std::make_shared<DRW_Variant>(1070, dint32{data->justification}));
    ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{N}));
    ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{i}));
    ext.push_back(std::make_shared<DRW_Variant>(1040, effOffsets[i]));
    ext.push_back(
        std::make_shared<DRW_Variant>(1070, dint32{data->openClosed}));
    if (i == 0) {
      // Anchor carries baseline + miter for each vertex.
      for (const auto &v : data->vertlist) {
        ext.push_back(std::make_shared<DRW_Variant>(
            1011, DRW_Coord(v.position.x, v.position.y, v.position.z)));
        ext.push_back(std::make_shared<DRW_Variant>(
            1013, DRW_Coord(v.miterDir.x, v.miterDir.y, v.miterDir.z)));
      }
    }
    polyline->setDrwExtData(std::move(ext));

    m_currentContainer->addEntity(polyline.release());
  }
}

/**
 * Implementation of the UNDERLAYDEFINITION callback.
 * Caches by handle so addUnderlay (entities arrive earlier) and the
 * export reconstruction can resolve filename + sheet on demand.
 */
void RS_FilterDXFRW::linkUnderlay(const DRW_UnderlayDefinition *d) {
  if (!d)
    return;
  RS_DEBUG->print("RS_FilterDXFRW::linkUnderlay: %s", d->filename.c_str());
  m_underlayDefMap[d->handle] = *d;
  if (m_graphic != nullptr)
    m_graphic->dwgAdvancedMetadata().addUnderlayDefinition(*d);
}

void RS_FilterDXFRW::addShape(const DRW_Shape &data) {
  if (m_graphic != nullptr)
    m_graphic->dwgAdvancedMetadata().addShape(data);
  RS_DEBUG->print("RS_FilterDXFRW::addShape: index %d style %d",
                  static_cast<int>(data.m_shapeIndex),
                  static_cast<int>(data.m_shapeFileHandle));
}

void RS_FilterDXFRW::addOle2Frame(const DRW_Ole2Frame &data) {
  if (m_graphic != nullptr)
    m_graphic->dwgAdvancedMetadata().addOle2Frame(data);
  RS_DEBUG->print("RS_FilterDXFRW::addOle2Frame: bytes %d declared %d",
                  static_cast<int>(data.m_payloadByteCount),
                  static_cast<int>(data.m_declaredPayloadLength));
}

/**
 * Implementation of the UNDERLAY entity callback (PDFUNDERLAY/DGNUNDERLAY/
 * DWFUNDERLAY).  Decomposes the underlay into a single closed RS_Polyline
 * showing the clip-boundary placeholder.  The filename + kind ride along
 * in XDATA so the export side can reconstruct the original UNDERLAY entity.
 *
 * Filename resolution: OBJECTS are parsed AFTER ENTITIES in libdxfrw, so
 * m_underlayDefMap may be empty here.  We store the definitionHandle in
 * XDATA only; on-demand lookups (UI tooltip, export reconstruction) use
 * the cache once it's populated by linkUnderlay.
 */
void RS_FilterDXFRW::addUnderlay(const DRW_Underlay *data) {
  RS_DEBUG->print("RS_FilterDXFRW::addUnderlay");
  if (!data)
    return;

  // Build clip polygon in WCS. clipBoundary is in OCS-2D; for a typical
  // 2D extrusion (extPoint == (0,0,1)), OCS == WCS modulo position +
  // scale + rotation. LibreCAD is 2D so we project z-up regardless.
  std::vector<RS_Vector> verts;
  const bool fallbackPreviewGenerated = true;
  if (data->clipBoundary.size() >= 3) {
    verts.reserve(data->clipBoundary.size());
    for (const auto &v : data->clipBoundary) {
      verts.emplace_back(v.x, v.y);
    }
  } else if (data->clipBoundary.size() == 2) {
    // Two corners of an axis-aligned rectangle (in OCS).
    const auto &a = data->clipBoundary[0];
    const auto &b = data->clipBoundary[1];
    verts.emplace_back(a.x, a.y);
    verts.emplace_back(b.x, a.y);
    verts.emplace_back(b.x, b.y);
    verts.emplace_back(a.x, b.y);
  } else {
    // No clip — synthesize a unit-square placeholder centered on origin.
    // Real underlay sizes need PDF page dims we don't have access to.
    verts.emplace_back(-0.5, -0.5);
    verts.emplace_back(0.5, -0.5);
    verts.emplace_back(0.5, 0.5);
    verts.emplace_back(-0.5, 0.5);
  }

  // Apply scale, rotation, translation (rotation is in radians per DWG).
  const double cs = std::cos(data->rotation);
  const double sn = std::sin(data->rotation);
  for (auto &p : verts) {
    const double sx = p.x * data->scale.x;
    const double sy = p.y * data->scale.y;
    const double rx = sx * cs - sy * sn;
    const double ry = sx * sn + sy * cs;
    p = RS_Vector{rx + data->position.x, ry + data->position.y};
  }

  RS_PolylineData pd(RS_Vector{}, RS_Vector{}, /*closed=*/true);
  auto polyline = new RS_Polyline(m_currentContainer, pd);
  setEntityAttributes(polyline, data);
  for (const auto &p : verts)
    polyline->addVertex(p, 0.0, false);

  // Round-trip metadata as XDATA. App marker LibreCAD_UNDERLAY.
  const QString underlayId = QStringLiteral("underlay_%1").arg(data->handle);
  const char *kindStr = (data->kind == DRW_Underlay::DGN)   ? "DGN"
                        : (data->kind == DRW_Underlay::DWF) ? "DWF"
                                                            : "PDF";
  std::vector<std::shared_ptr<DRW_Variant>> ext;
  ext.push_back(
      std::make_shared<DRW_Variant>(1001, std::string("LibreCAD_UNDERLAY")));
  ext.push_back(std::make_shared<DRW_Variant>(1000, underlayId.toStdString()));
  ext.push_back(std::make_shared<DRW_Variant>(1000, std::string(kindStr)));
  ext.push_back(std::make_shared<DRW_Variant>(
      1071, dint32{static_cast<int>(data->definitionHandle)}));
  ext.push_back(std::make_shared<DRW_Variant>(
      1010, DRW_Coord(data->position.x, data->position.y, data->position.z)));
  ext.push_back(std::make_shared<DRW_Variant>(1040, data->scale.x));
  ext.push_back(std::make_shared<DRW_Variant>(1040, data->scale.y));
  ext.push_back(std::make_shared<DRW_Variant>(1040, data->rotation));
  ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{data->flags}));
  ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{data->contrast}));
  ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{data->fade}));
  polyline->setDrwExtData(std::move(ext));

  if (m_graphic != nullptr)
    m_graphic->dwgAdvancedMetadata().addUnderlay(
        *data, fallbackPreviewGenerated);
  m_currentContainer->addEntity(polyline);
}

/**
 * Implementation of the method which handles polyline entities.
 */
void RS_FilterDXFRW::addPolyline(const DRW_Polyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addPolyline");
    if (data.flags & 0x10) {
        // the polyline is a polygon mesh
        int M = data.vertexcount;
        int N = data.facecount;
        const bool canRenderFallback =
            M > 0 && N > 0
            && data.vertlist.size() == static_cast<size_t>(M * N)
            && data.curvetype == 0;
        if (m_graphic != nullptr)
            m_graphic->dwgAdvancedMetadata().addMeshPolyline(
                data, canRenderFallback);
        if (M <= 0 || N <= 0 || data.vertlist.size() != static_cast<size_t>(M * N)) {
            return; // invalid mesh
        }
        if (data.curvetype != 0) {
            return; // smooth surfaces not handled
        }
        bool closedM = (data.flags & 0x1);  // closed in M direction
        bool closedN = (data.flags & 0x20); // closed in N direction
        const std::string meshId = std::string("polyline_mesh_")
            + std::to_string(data.handle);
        const int meshElementCount = M + N;
        auto makeMeshExtData = [&](int elementIndex, const std::string& role,
                                   int roleIndex, bool anchor) {
            std::vector<std::shared_ptr<DRW_Variant>> ext;
            ext.push_back(std::make_shared<DRW_Variant>(
                1001, std::string("LibreCAD_POLYLINE_MESH")));
            ext.push_back(std::make_shared<DRW_Variant>(1000, meshId));
            ext.push_back(std::make_shared<DRW_Variant>(1000, role));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{elementIndex}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{meshElementCount}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{roleIndex}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{data.flags}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{M}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{N}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{data.smoothM}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{data.smoothN}));
            ext.push_back(std::make_shared<DRW_Variant>(
                1070, dint32{data.curvetype}));
            if (anchor) {
                for (const auto& vertex : data.vertlist) {
                    if (!vertex) {
                        continue;
                    }
                    ext.push_back(std::make_shared<DRW_Variant>(
                        1010,
                        DRW_Coord{vertex->basePoint.x, vertex->basePoint.y,
                                  vertex->basePoint.z}));
                }
            }
            return ext;
        };
        auto addMeshSidecarMetadata = [&](const RS_Entity* entity,
                                          int elementIndex,
                                          const std::string& role,
                                          int roleIndex, bool anchor) {
            if (m_graphic == nullptr || entity == nullptr)
                return;
            LC_DwgAdvancedMetadata::MeshSidecarRecord record;
            record.sourceHandle = data.handle;
            record.fallbackEntityId = entity->getId();
            record.meshId = meshId;
            record.role = role;
            record.elementIndex = elementIndex;
            record.elementCount = meshElementCount;
            record.roleIndex = roleIndex;
            record.flags = data.flags;
            record.mCount = M;
            record.nCount = N;
            record.smoothM = data.smoothM;
            record.smoothN = data.smoothN;
            record.curveType = data.curvetype;
            record.sourceVertexCount = anchor ? data.vertlist.size() : 0u;
            record.anchor = anchor;
            m_graphic->dwgAdvancedMetadata().addMeshSidecar(std::move(record));
        };

        // Add row polylines (along N direction)
        for (int i = 0; i < M; i++) {
            RS_PolylineData pd(RS_Vector(), RS_Vector(), closedN);
            auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
            setEntityAttributes(pl.get(), &data);
            for (int j = 0; j < N; j++) {
                auto v = data.vertlist.at(i * N + j);
                RS_Vector pos(v->basePoint.x, v->basePoint.y);
                pl->addVertex(pos, 0.0, false);
            }
            pl->setDrwExtData(makeMeshExtData(i, "row", i, i == 0));
            addMeshSidecarMetadata(pl.get(), i, "row", i, i == 0);
            m_currentContainer->addEntity(pl.release());
        }

        // Add column polylines (along M direction)
        for (int j = 0; j < N; j++) {
            RS_PolylineData pd(RS_Vector(), RS_Vector(), closedM);
            auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
            setEntityAttributes(pl.get(), &data);
            for (int i = 0; i < M; i++) {
                auto v = data.vertlist.at(i * N + j);
                RS_Vector pos(v->basePoint.x, v->basePoint.y);
                pl->addVertex(pos, 0.0, false);
            }
            pl->setDrwExtData(
                makeMeshExtData(M + j, "column", j, false));
            addMeshSidecarMetadata(pl.get(), M + j, "column", j, false);
            m_currentContainer->addEntity(pl.release());
        }
        return;
    }

    if (data.flags & 0x40) {
        // the polyline is a polyface mesh
        std::vector<RS_Vector> vertices;
        std::vector<DRW_Coord> sourceVertices;
        std::vector<std::shared_ptr<DRW_Vertex>> faceRecords;
        for (const std::shared_ptr<DRW_Vertex>& v : data.vertlist) {
            if (!v) {
                continue;
            }
            const bool coordinateVertex =
                v->dwgSubtype() == DRW_Vertex::DwgSubtype::Polyface
                || ((v->flags & 0x40) != 0 && (v->flags & 0x80) != 0);
            const bool faceRecord =
                v->dwgSubtype() == DRW_Vertex::DwgSubtype::PolyfaceFace
                || ((v->flags & 0x80) != 0 && (v->flags & 0x40) == 0);
            if (coordinateVertex) {
                vertices.emplace_back(v->basePoint.x, v->basePoint.y);
                sourceVertices.emplace_back(v->basePoint.x, v->basePoint.y,
                                            v->basePoint.z);
            }
            else if (faceRecord) {
                faceRecords.push_back(v);
            }
        }
        const std::string polyfaceId = std::string("polyline_pface_")
            + std::to_string(data.handle);
        auto makePolyfaceExtData =
            [&](const DRW_Vertex& face, int faceIndex, bool anchor) {
                std::vector<std::shared_ptr<DRW_Variant>> ext;
                ext.push_back(std::make_shared<DRW_Variant>(
                    1001, std::string("LibreCAD_POLYLINE_PFACE")));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1000, polyfaceId));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{faceIndex}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{static_cast<int>(faceRecords.size())}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{data.flags}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{data.vertexcount}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{data.facecount}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{face.vindex1}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{face.vindex2}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{face.vindex3}));
                ext.push_back(std::make_shared<DRW_Variant>(
                    1070, dint32{face.vindex4}));
                if (anchor) {
                    for (const auto& coord : sourceVertices) {
                        ext.push_back(std::make_shared<DRW_Variant>(
                            1010, coord));
                    }
                }
                return ext;
            };
        // add faces as closed polylines
        for (size_t faceIndex = 0; faceIndex < faceRecords.size();
             ++faceIndex) {
                const auto& f = faceRecords[faceIndex];
                std::vector<int> indices = {{f->vindex1, f->vindex2,
                                             f->vindex3, f->vindex4}};
                int num_points = (f->vindex4 == 0) ? 3 : 4;
                RS_PolylineData pd(RS_Vector(), RS_Vector(), true); // closed
                auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
                setEntityAttributes(pl.get(), &data);
                bool valid = true;
                for (int k = 0; k < num_points; ++k) {
                    int idx = std::abs(indices[k]);
                    if (idx < 1 || idx > static_cast<int>(vertices.size())) {
                        valid = false;
                        break;
                    }
                    pl->addVertex(vertices[idx - 1], 0.0);
                }
                if (valid) {
                    pl->setDrwExtData(
                        makePolyfaceExtData(*f, static_cast<int>(faceIndex),
                                            faceIndex == 0));
                    m_currentContainer->addEntity(pl.release());
                }
        }
        return;
    }

    RS_PolylineData pd(RS_Vector{}, RS_Vector{}, data.flags & 0x1);
    auto polyline = std::make_unique<RS_Polyline>(m_currentContainer, pd);
    setEntityAttributes(polyline.get(), &data);

    if (data.vertlist.empty()) {
        m_currentContainer->addEntity(polyline.release());
        return;
    }

    auto vert0 = data.vertlist[0];
    RS_Vector first_pos(vert0->basePoint.x, vert0->basePoint.y);
    polyline->addVertex(first_pos, 0.0, false);
    RS_Vector prev_pos = first_pos;

    bool closed = (data.flags & 0x1) != 0;
    size_t num_segments = closed ? data.vertlist.size() : data.vertlist.size() - 1;

    for (size_t i = 0; i < num_segments; ++i) {
        size_t vert_idx = (i + 1) % data.vertlist.size();
        auto vert = data.vertlist[vert_idx];
        RS_Vector curr_pos(vert->basePoint.x, vert->basePoint.y);

        size_t bulge_idx = i % data.vertlist.size();
        double bulge = data.vertlist[bulge_idx]->bulge;
        const auto& extData = data.vertlist[bulge_idx]->extData;

        bool is_closed_seg = closed && (i == num_segments - 1);
        addPolylineSegment(*polyline, prev_pos, curr_pos, bulge, extData, is_closed_seg);

        prev_pos = curr_pos;
    }

    if (closed) {
        polyline->setFlag(RS2::FlagClosed);
        polyline->setNextBulge(data.vertlist.back()->bulge);
        polyline->getData().endpoint = polyline->getData().startpoint;
    } else {
        polyline->endPolyline();
    }

    m_currentContainer->addEntity(polyline.release());
}

bool RS_FilterDXFRW::handleQuadraticConicSpline(const DRW_Spline* data) {
    if (data->degree != 2 || data->controllist.size() != 3) {
        return false;
    }

    // Three explicit branches by middle weight: hyperbola (w>1), parabola
    // (weights absent or w=1), or fall-through for ellipse arc (w<1) and
    // anything else the dispatcher can't classify cleanly.
    std::unique_ptr<RS_Entity> en =
        LC_HyperbolaSpline::splineToHyperbola(*data, m_currentContainer);
    if (en == nullptr) {
      en = LC_ParabolaSpline::splineToParabola(*data, m_currentContainer);
    }
    if (en == nullptr) {
      // Not a hyperbola or parabola — likely an ellipse arc (w<1), a
      // collinear/degenerate spline, or a non-canonical knot vector. Let
      // the caller fall through to the generic addSpline path so the
      // entity isn't silently mis-classified as a parabola.
      RS_DEBUG->print("RS_FilterDXFRW::handleQuadraticConicSpline: "
                      "degree-2/3-control spline not classified as "
                      "parabola or hyperbola; falling through to addSpline");
      return false;
    }
    setEntityAttributes(en.get(), data);
    en->update();
    m_currentContainer->addEntity(en.release());

    return true;
}

/**
 * Implementation of the method which handles splines.
 */
void RS_FilterDXFRW::addSpline(const DRW_Spline* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addSpline: degree: %d", data->degree);

    // Special case: rational quadratic conic (hyperbola or parabola)
    if (handleQuadraticConicSpline(data)) {
        return;  // Conic handled successfully
    }

    // Spline points case. Weighted degree-2 splines are exact rational conics
    // or NURBS segments; keep them on the RS_Spline path so weights survive.
    if (data->degree == 2 && !hasRationalSplineWeights(data)) {
        bool closed = (data->flags & 0x1) == 0x1;
        bool controlOnly = data->nfit == 0;

        LC_SplinePointsData d(closed, controlOnly);
        LC_SplinePoints* splinePoints = new LC_SplinePoints(m_currentContainer, d);
        setEntityAttributes(splinePoints, data);

        for (const auto& vert : data->controllist) {
            if (vert) {
                splinePoints->addControlPoint({vert->x, vert->y});
            }
        }
        for (const auto& vert : data->fitlist) {
            if (vert) {
                splinePoints->addPoint({vert->x, vert->y});
            }
        }

        splinePoints->update();
        m_currentContainer->addEntity(splinePoints);
        return;
    }

    // General spline (degree 1-3)
    if (data->degree < 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXFRW::addSpline: unsupported spline degree %d", data->degree);
        return;
    }
    if (data->degree > 3) {
        std::unique_ptr<LC_SplinePoints> sampled = approximateDrwSpline(m_currentContainer, data);
        if (sampled) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_FilterDXFRW::addSpline: approximating unsupported spline degree %d",
                            data->degree);
            setEntityAttributes(sampled.get(), data);
            sampled->update();
            m_currentContainer->addEntity(sampled.release());
            return;
        }

        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXFRW::addSpline: unsupported spline degree %d", data->degree);
        return;
    }

    bool isClosed = (data->flags & 0x1) == 0x1;

    RS_SplineData d(data->degree, isClosed);
    if (!data->knotslist.empty()) {
        double tolknot = (data->tolknot > 0.0) ? data->tolknot : 1e-7;
        for (double k : data->knotslist) {
            d.knotslist.push_back(RS_Math::round(k, tolknot));
        }
    }

    d.type = isClosed ? RS_SplineData::SplineType::Standard : RS_SplineData::SplineType::ClampedOpen;

    RS_Spline* spline = new RS_Spline(m_currentContainer, d);
    setEntityAttributes(spline, data);
    m_currentContainer->addEntity(spline);

    // Control points and weights. Non-rational B-splines have no weight array
    // (weight=1.0 implied); only warn for rational splines (flag bit 2).
    size_t numCtrl = data->controllist.size();
    bool isRational = (data->flags & 0x4) != 0;
    if (isRational && numCtrl != data->weightlist.size()) {
      RS_DEBUG->print(RS_Debug::D_WARNING,
                      "RS_FilterDXFRW::addSpline: rational spline control "
                      "points (%zu) != weights (%zu)",
                      numCtrl, data->weightlist.size());
    }

    for (size_t i = 0; i < numCtrl; ++i) {
        const auto& vert = data->controllist[i];
        double weight = (i < data->weightlist.size()) ? data->weightlist[i] : 1.0;
        if (vert) {
            spline->addControlPointRaw({vert->x, vert->y}, weight);
        }
    }

    // Fit points fallback
    if (numCtrl == 0 && data->degree != 2) {
        std::vector<RS_Vector> fitPoints;
        std::transform(data->fitlist.begin(), data->fitlist.end(), std::back_inserter(fitPoints), [](const std::shared_ptr<DRW_Coord>& coord) {
            return RS_Vector{coord->x, coord->y};
        });
        spline->setFitPoints(fitPoints);
    }

    if (isClosed) {
        spline->setClosed(true);
    }

    spline->update();
}

/**
 * Implementation of the method which handles inserts.
 */
void RS_FilterDXFRW::addInsert(const DRW_Insert& data) {
    RS_DEBUG->print("RS_FilterDXF::addInsert");

    RS_Vector ip(data.basePoint.x, data.basePoint.y);
    RS_Vector sc(data.xscale, data.yscale);
    RS_Vector sp(data.colspace, data.rowspace);

    //cout << "Insert: " << name << " " << ip << " " << cols << "/" << rows << endl;

    RS_InsertData d( QString::fromUtf8(data.name.c_str()),
                    ip, sc, data.angle,
                    data.colcount, data.rowcount,
					sp, nullptr, RS2::NoUpdate);
    RS_Insert* entity = new RS_Insert(m_currentContainer, d);
    setEntityAttributes(entity, &data);
    RS_DEBUG->print("  id: %lu", entity->getId());
//    entity->update();
    m_currentContainer->addEntity(entity);

    // Render visible block attributes (ATTRIB) attached to this INSERT.
    // ATTRIB coordinates are in world space (per AutoCAD convention), so
    // each becomes an independent RS_Text in the current container.
    // Bit 1 of attribFlags marks the attribute as invisible.
    for (const auto &att : data.attlist) {
      if (!att || (att->attribFlags & 0x1) != 0)
        continue;

      // Multi-line ATTRIB (R2018+, ODA spec §20.4.4 Attribute type 2/4):
      // libdxfrw saw the `100 / Embedded Object` DXF subclass marker on the
      // ATTRIB and populated att->mtext from the embedded MTEXT body.
      // Render it as RS_MText so the multi-line content + formatting codes
      // survive; the plain `text` field is the single-line fallback.
      if (att->mtext) {
        auto *mt = mtextEntityFromDRW(*att->mtext);
        mt->setParent(m_currentContainer);
        setEntityAttributes(mt, att.get());
        mt->update();
        m_currentContainer->addEntity(mt);
        continue;
      }

      RS_Vector refPoint(att->basePoint.x, att->basePoint.y);
      RS_Vector secPoint(att->secPoint.x, att->secPoint.y);
      if (att->alignV != 0 || att->alignH != 0 ||
          att->alignH == DRW_Text::HMiddle) {
        if (att->alignH != DRW_Text::HAligned &&
            att->alignH != DRW_Text::HFit) {
          secPoint = RS_Vector(att->basePoint.x, att->basePoint.y);
          refPoint = RS_Vector(att->secPoint.x, att->secPoint.y);
        }
      }

      RS_TextData::VAlign valign = (RS_TextData::VAlign)att->alignV;
      RS_TextData::HAlign halign = (RS_TextData::HAlign)att->alignH;
      RS_TextData::TextGeneration dir;
      if (att->textgen == 2)
        dir = RS_TextData::Backward;
      else if (att->textgen == 4)
        dir = RS_TextData::UpsideDown;
      else
        dir = RS_TextData::None;

      QString sty = QString::fromUtf8(att->style.c_str());
      prepareTextStyleName(sty);
      QString text = toNativeString(QString::fromUtf8(att->text.c_str()));

      RS_TextData td(refPoint, secPoint, att->height, att->widthscale, valign,
                     halign, dir, text, sty, att->angle * M_PI / 180,
                     RS2::NoUpdate);
      auto textEntity = std::make_unique<RS_Text>(m_currentContainer, td);
      setEntityAttributes(textEntity.get(), att.get());
      textEntity->update();
      m_currentContainer->addEntity(textEntity.release());
    }
}

void RS_FilterDXFRW::addTable(const DRW_Table& data) {
    TableFallbackRenderSummary fallbackSummary;
    const bool fallbackRendered = addTableFallback(data, &fallbackSummary);
    if (m_graphic != nullptr) {
        m_graphic->dwgAdvancedMetadata().addTable(data, fallbackRendered);
        LC_DwgAdvancedMetadata::TableFallbackRenderSummary metadataSummary;
        metadataSummary.tableHandle = data.handle;
        metadataSummary.gridEntityCount = fallbackSummary.gridEntityCount;
        metadataSummary.textEntityCount = fallbackSummary.textEntityCount;
        metadataSummary.placeholderEntityCount =
            fallbackSummary.placeholderEntityCount;
        metadataSummary.unresolvedTextStyleCount =
            fallbackSummary.unresolvedTextStyleCount;
        metadataSummary.clampedDimensionCount =
            fallbackSummary.clampedDimensionCount;
        m_graphic->dwgAdvancedMetadata().updateTableFallbackRenderSummary(
            metadataSummary);
    }
    addInsert(data);
}

bool RS_FilterDXFRW::addTableFallback(
    const DRW_Table& data, TableFallbackRenderSummary *summary) {
    if (!data.m_hasSemanticContent || data.m_content.m_rows.empty()
        || data.m_content.m_columns.empty() || m_currentContainer == nullptr) {
        return false;
    }

    const DRW_TableContent& content = data.m_content;
    const RS_Vector origin(data.basePoint.x, data.basePoint.y, data.basePoint.z);
    RS_Vector xAxis(data.m_horizontalDirection.x, data.m_horizontalDirection.y,
                    data.m_horizontalDirection.z);
    if (!xAxis.valid || xAxis.magnitude() <= RS_TOLERANCE) {
        xAxis = RS_Vector::polar(1.0, data.angle);
    } else {
        xAxis.set(xAxis.x / xAxis.magnitude(), xAxis.y / xAxis.magnitude(),
                  xAxis.z / xAxis.magnitude());
    }
    const RS_Vector yAxis = RS_Vector(-xAxis.y, xAxis.x, 0.0);

    std::vector<double> columnOffsets;
    columnOffsets.reserve(content.m_columns.size() + 1);
    columnOffsets.push_back(0.0);
    for (size_t column = 0; column < content.m_columns.size(); ++column) {
        columnOffsets.push_back(
            columnOffsets.back() + tableColumnWidth(content, column, summary));
    }

    std::vector<double> rowOffsets;
    rowOffsets.reserve(content.m_rows.size() + 1);
    rowOffsets.push_back(0.0);
    for (size_t row = 0; row < content.m_rows.size(); ++row) {
        rowOffsets.push_back(
            rowOffsets.back() + tableRowHeight(content, row, summary));
    }

    auto tablePoint = [&](double x, double y) {
        return origin + xAxis * x - yAxis * y;
    };
    auto addFallbackRecord = [&](RS_Entity *entity, int row, int column,
                                 LC_DwgAdvancedMetadata::TableFallbackRole role) {
        if (m_graphic == nullptr || entity == nullptr)
            return;
        LC_DwgAdvancedMetadata::TableFallbackEntityRecord record;
        record.tableHandle = data.handle;
        record.sourceHandle = data.handle;
        record.row = row;
        record.column = column;
        record.role = role;
        record.entityId = entity->getId();
        m_graphic->dwgAdvancedMetadata().addTableFallbackEntity(record);
    };
    auto addBorder = [&](const RS_Vector& a, const RS_Vector& b,
                         int row, int column) {
        auto *line = new RS_Line{m_currentContainer, {a, b}};
        setEntityAttributes(line, &data);
        line->update();
        addFallbackRecord(line, row, column,
                          LC_DwgAdvancedMetadata::TableFallbackRole::GridLine);
        if (summary != nullptr)
            ++summary->gridEntityCount;
        m_currentContainer->addEntity(line);
    };

    for (size_t column = 0; column < columnOffsets.size(); ++column) {
        const double x = columnOffsets[column];
        addBorder(tablePoint(x, 0.0), tablePoint(x, rowOffsets.back()),
                  -1, static_cast<int>(column));
    }
    for (size_t row = 0; row < rowOffsets.size(); ++row) {
        const double y = rowOffsets[row];
        addBorder(tablePoint(0.0, y), tablePoint(columnOffsets.back(), y),
                  static_cast<int>(row), -1);
    }

    bool renderedText = false;
    QString style = m_textStyle;
    prepareTextStyleName(style);
    const double angle = std::atan2(xAxis.y, xAxis.x);
    for (size_t row = 0; row < content.m_rows.size(); ++row) {
        const auto& tableRow = content.m_rows[row];
        const size_t cellCount = std::min(tableRow.m_cells.size(),
                                          content.m_columns.size());
        for (size_t column = 0; column < cellCount; ++column) {
            const DRW_TableCell& tableCell = tableRow.m_cells[column];
            TableFallbackCellDisplay display =
                tableCellDisplay(tableCell, data.m_semanticContentComplete);
            if (display.text.isEmpty())
                continue;
            const bool placeholder =
                tableFallbackCellIsPlaceholder(display.kind);
            QString text = toNativeString(display.text);
            const double left = columnOffsets[column];
            const double right = columnOffsets[column + 1];
            const double top = rowOffsets[row];
            const double bottom = rowOffsets[row + 1];
            const double cellHeight = std::max(0.1, bottom - top);
            const double cellWidth = std::max(0.1, right - left);
            const RS_Vector insertion = tablePoint(left + cellWidth * 0.08,
                                                   top + cellHeight * 0.25);
            double textHeight = cellHeight * 0.35;
            if (tableCell.m_contentHeight > 0.0
                && std::isfinite(tableCell.m_contentHeight)) {
                textHeight = tableCell.m_contentHeight;
            } else if (tableCell.m_height > 0.0
                       && std::isfinite(tableCell.m_height)) {
                textHeight = tableCell.m_height * 0.35;
            }
            textHeight = std::max(kTableFallbackMinTextHeight, textHeight);
            if (summary != nullptr
                && (tableCell.m_textStyleHandle != 0
                    || tableCell.m_textStyleOverrideHandle != 0)) {
                ++summary->unresolvedTextStyleCount;
            }
            RS_MTextData textData(insertion, textHeight, cellWidth * 0.84,
                                  RS_MTextData::VATop, RS_MTextData::HALeft,
                                  RS_MTextData::LeftToRight, RS_MTextData::AtLeast,
                                  1.0, text, style, angle, RS2::NoUpdate);
            auto *mtext = new RS_MText(m_currentContainer, textData);
            setEntityAttributes(mtext, &data);
            mtext->update();
            addFallbackRecord(
                mtext, static_cast<int>(row), static_cast<int>(column),
                placeholder
                    ? LC_DwgAdvancedMetadata::TableFallbackRole::Placeholder
                    : LC_DwgAdvancedMetadata::TableFallbackRole::CellText);
            if (summary != nullptr) {
                ++summary->textEntityCount;
                if (placeholder)
                    ++summary->placeholderEntityCount;
            }
            m_currentContainer->addEntity(mtext);
            renderedText = true;
        }
    }

    return renderedText || columnOffsets.size() > 1 || rowOffsets.size() > 1;
}

void RS_FilterDXFRW::prepareTextStyleName(QString& sty) const {
    // use default style for the drawing:
    if (sty.isEmpty()) {
        // japanese, cyrillic:
        if (m_codePage=="ANSI_932" || m_codePage=="ANSI_1251") {
            sty = "Unicode";
        } else {
            sty = m_textStyle;
        }
    } else {
        sty = m_fontList.value(sty, sty);
    }
}

/**
 * Builds an RS_MText entity from a DRW_MText payload, applying the same
 * alignment/drawing-direction/line-spacing/legacy-correction logic that
 * addMText() uses.  Shared between addMText() (standalone MTEXT entities) and
 * addInsert() (block attributes whose `100 / Embedded Object` DXF subclass
 * produced an att->mtext payload).  Returns a freshly-constructed entity not
 * yet attached to any container; the caller is responsible for
 * setEntityAttributes() and appending.
 */
RS_MText *RS_FilterDXFRW::mtextEntityFromDRW(const DRW_MText &data) {
  RS_MTextData::VAlign valign;
  RS_MTextData::HAlign halign;
  RS_MTextData::MTextDrawingDirection dir;
  RS_MTextData::MTextLineSpacingStyle lss;

  if (data.textgen <= 3) {
    valign = RS_MTextData::VATop;
  } else if (data.textgen <= 6) {
    valign = RS_MTextData::VAMiddle;
  } else {
    valign = RS_MTextData::VABottom;
  }

  if (data.textgen % 3 == 1) {
    halign = RS_MTextData::HALeft;
  } else if (data.textgen % 3 == 2) {
    halign = RS_MTextData::HACenter;
  } else {
    halign = RS_MTextData::HARight;
  }

  if (data.alignH == 1) {
    dir = RS_MTextData::LeftToRight;
  } else if (data.alignH == 3) {
    dir = RS_MTextData::TopToBottom;
  } else {
    dir = RS_MTextData::ByStyle;
  }

  if (data.alignV == 1) {
    lss = RS_MTextData::AtLeast;
  } else {
    lss = RS_MTextData::Exact;
  }
  QString mtext = toNativeString(QString::fromUtf8(data.text.c_str()));
  QString sty = QString::fromUtf8(data.style.c_str());
  prepareTextStyleName(sty);

  double interlin = data.interlin;
  double angle = data.angle * M_PI / 180.;
  RS_Vector ip = RS_Vector(data.basePoint.x, data.basePoint.y);

  if (m_oldMText) {
    interlin = data.interlin * 0.96;
    if (valign == RS_MTextData::VABottom) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList tl = mtext.split('\n', Qt::SkipEmptyParts);
#else
            QStringList tl = mtext.split('\n', QString::SkipEmptyParts);
#endif
            if (!tl.isEmpty()) {
                QString txt = tl.at(tl.size()-1);
                RS_TextData d(RS_Vector(0., 0., 0.), RS_Vector(0., 0., 0.),
                              data.height, 1, RS_TextData::VABaseline,
                              RS_TextData::HALeft, RS_TextData::None, txt, sty,
                              0, RS2::Update);
                auto entity = new RS_Text(nullptr, d);
                double textTail = entity->getMin().y;
                delete entity;
                auto ot = RS_Vector(0.0,textTail).rotate(angle);
                ip.move(ot);
            }
        }
    }

    // drawingDirection was resolved above (1→LeftToRight, 3→TopToBottom,
    // else→ByStyle), matching the DXF MTEXT group 72 spec. There is no DXF
    // value for RightToLeft; that flag round-trips via XDATA below — when
    // an MTEXT carries a "LibreCad" + 1071 marker, restore the RTL setting.
    bool wantRTL = false;
    for (size_t k = 0; k + 1 < data.extData.size(); ++k) {
      const auto &appTag = data.extData[k];
      if (!appTag || appTag->code() != 1001 ||
          appTag->type() != DRW_Variant::STRING ||
          appTag->content.s == nullptr || *appTag->content.s != "LibreCad")
        continue;
      // Walk subsequent entries until the next 1001 (next app block).
      for (size_t j = k + 1; j < data.extData.size(); ++j) {
        const auto &v = data.extData[j];
        if (!v)
          continue;
        if (v->code() == 1001)
          break;
        if (v->code() == 1071 && v->type() == DRW_Variant::INTEGER &&
            v->content.i != 0) {
          wantRTL = true;
        }
      }
      if (wantRTL)
        break;
    }
    if (wantRTL)
      dir = RS_MTextData::RightToLeft;

    RS_MTextData d(ip, data.height, data.widthscale, valign, halign, dir, lss,
                   interlin, mtext, sty, angle, RS2::NoUpdate);
    return new RS_MText(nullptr, d);
}

/**
 * Implementation of the method which handles
 * multi texts (MTEXT).
 */
void RS_FilterDXFRW::addMText(const DRW_MText &data) {
  RS_DEBUG->print("RS_FilterDXF::addMText: %s", data.text.c_str());
  auto *entity = mtextEntityFromDRW(data);
  entity->setParent(m_currentContainer);
  setEntityAttributes(entity, &data);
  entity->update();
  m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * texts (TEXT).
 */
void RS_FilterDXFRW::addText(const DRW_Text& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addText");
    RS_Vector refPoint = RS_Vector(data.basePoint.x, data.basePoint.y);;
    RS_Vector secPoint = RS_Vector(data.secPoint.x, data.secPoint.y);;
    double angle = data.angle;

    if (data.alignV !=0 || data.alignH !=0 ||data.alignH ==DRW_Text::HMiddle){
        if (data.alignH !=DRW_Text::HAligned && data.alignH !=DRW_Text::HFit){
            secPoint = RS_Vector(data.basePoint.x, data.basePoint.y);
            refPoint = RS_Vector(data.secPoint.x, data.secPoint.y);
        }
    }

    RS_TextData::VAlign valign = (RS_TextData::VAlign)data.alignV;
    RS_TextData::HAlign halign = (RS_TextData::HAlign)data.alignH;
    RS_TextData::TextGeneration dir;
    QString sty = QString::fromUtf8(data.style.c_str());

    if (data.textgen==2) {
        dir = RS_TextData::Backward;
    } else if (data.textgen==4) {
        dir = RS_TextData::UpsideDown;
    } else {
        dir = RS_TextData::None;
    }

    QString text = toNativeString(QString::fromUtf8(data.text.c_str()));

    prepareTextStyleName(sty);

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(text);

    RS_TextData d(refPoint, secPoint, data.height, data.widthscale,
                  valign, halign, dir,
                  text, sty, angle*M_PI/180,
                  RS2::NoUpdate);
    auto* entity = new RS_Text(m_currentContainer, d);

    setEntityAttributes(entity, &data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * dimensions (DIMENSION).
 */
RS_DimensionData RS_FilterDXFRW::convDimensionData(const  DRW_Dimension* data) {
    DRW_Coord crd = data->getDefPoint();
    RS_Vector defP(crd.x, crd.y);
    crd = data->getTextPoint();
    RS_Vector midP(crd.x, crd.y);
    RS_MTextData::VAlign valign;
    RS_MTextData::HAlign halign;
    RS_MTextData::MTextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data->getStyle().c_str());

    QString t; //= data.text;

    // middlepoint of text can be 0/0 which is considered to be invalid (!):
    //  0/0 because older QCad versions save the middle of the text as 0/0
    //  although they didn't support saving of the middle of the text.
    if (fabs(crd.x)<1.0e-6 && fabs(crd.y)<1.0e-6) {
        midP = RS_Vector(false);
    }

    if (data->getAlign()<=3) {
        valign=RS_MTextData::VATop;
    } else if (data->getAlign()<=6) {
        valign=RS_MTextData::VAMiddle;
    } else {
        valign=RS_MTextData::VABottom;
    }

    if (data->getAlign()%3==1) {
        halign=RS_MTextData::HALeft;
    } else if (data->getAlign()%3==2) {
        halign=RS_MTextData::HACenter;
    } else {
        halign=RS_MTextData::HARight;
    }

    if (data->getTextLineStyle()==1) {
        lss = RS_MTextData::AtLeast;
    } else {
        lss = RS_MTextData::Exact;
    }

    t = toNativeString(QString::fromUtf8( data->getText().c_str() ));

    if (sty.isEmpty()) {
        sty = m_dimStyle;
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(t);

    bool customTextLocation = data->type >= 128;

    LC_DimStyle*  dimStyleOverride =  nullptr;

    if (!data->extData.empty()) {
        LC_ExtEntityData* extData = extractEntityExtData(data->extData);
        if (extData != nullptr) {
            dimStyleOverride = parseDimStyleOverride(extData);
            delete extData;
        }
    }

    // data needed to add the actual dimension entity
    return RS_DimensionData(defP, midP, valign, halign, lss, data->getTextLineFactor(), t, sty,
        data->getDir(), data->getHDir(), !customTextLocation, dimStyleOverride,
        data->getFlipArrow1(), data->getFlipArrow2());
}

void RS_FilterDXFRW::fillEntityExtData(std::vector<std::shared_ptr<DRW_Variant>> &extData, LC_ExtEntityData* entityData) {
    auto appDatas = entityData->getAppData();
    for (auto appData: *appDatas) {
        extData.push_back(std::make_shared<DRW_Variant>(1001, appData->getName().toStdString())); // application name
        auto groups = appData->getGroups();
        for (auto group: *groups) {
            extData.push_back(std::make_shared<DRW_Variant>(1000, group->getName().toStdString())); // group name
            extData.push_back(std::make_shared<DRW_Variant>(1002, "{")); // start

            auto tagsList = group->getTagsList();

            for (auto tag: *tagsList) {
                if (tag->isAtomic ()) {
                    // fixme - just plain list of tags within the group, no nesting!
                    auto variable = tag->var();
                    int code = variable->getCode();
                    extData.push_back(std::make_shared<DRW_Variant>(1070, code)); // code of variable
                    if (tag->isBinary()) {
                      // Wrap the QByteArray bytes in a BINARY DRW_Variant
                      // (DXF group 1004). The DXF writer hex-encodes them
                      // on emit; the DWG path will use the raw bytes.
                      const QByteArray &bytes = tag->bytes();
                      std::vector<duint8> raw(bytes.size());
                      for (int i = 0; i < bytes.size(); ++i) {
                        raw[i] = static_cast<duint8>(bytes[i]);
                      }
                      extData.push_back(
                          std::make_shared<DRW_Variant>(1004, std::move(raw)));
                      continue;
                    }
                    auto varType = variable->getType();
                    switch (varType) {
                        case RS2::VariableInt: {
                            extData.push_back(std::make_shared<DRW_Variant>(1070, variable->getInt())); // int value
                            break;
                        }
                        case RS2::VariableDouble: {
                            extData.push_back(std::make_shared<DRW_Variant>(1040, variable->getDouble())); // double value
                            break;
                        }
                        case RS2::VariableString: {
                          if (tag->isLayerRef()) {
                            // DXF code 1003 — layer name reference. The
                            // isLayerRef flag rides on the variant so that
                            // the DWG path (when written) can resolve the
                            // name back to a layer-table handle.
                            extData.push_back(std::make_shared<DRW_Variant>(
                                1003, variable->getString().toStdString(),
                                /*isLayerRef=*/true));
                          } else {
                            extData.push_back(std::make_shared<DRW_Variant>(
                                (tag->isRef() ? 1005 : 1003),
                                variable->getString()
                                    .toStdString())); // string value
                          }
                            break;
                        }
                        case RS2::VariableVector: {
                            RS_Vector vec = variable->getVector();
                            extData.push_back(std::make_shared<DRW_Variant>(1010, vec.x)); // vector value
                            extData.push_back(std::make_shared<DRW_Variant>(1011, vec.y)); // vector value
                            extData.push_back(std::make_shared<DRW_Variant>(1012, vec.z)); // vector value
                            break;
                        }
                        case RS2::VariableVoid: {
                            break;
                        }
                    }
                }
            }
            extData.push_back(std::make_shared<DRW_Variant>(1002, "}")); // end
        }
    }
}

// this method is quite generic and may be used for parsing any entity's ext data
// fixme & todo - sand - nesting of tags is not supported so far in code, yet it's supported by DXF !!!!
LC_ExtEntityData* RS_FilterDXFRW::extractEntityExtData(const std::vector<std::shared_ptr<DRW_Variant>> &extData) {
    auto* result = new LC_ExtEntityData();
    LC_ExtDataAppData* currentAppData = nullptr;
    LC_ExtDataGroup* currentGroup = nullptr;

    int currentValType = -1;
    std::stack<LC_ExtDataTag*> tagStack;
    bool expectType = false;
    [[maybe_unused]] int listLevel = 0;
    bool inTagsList = false;
    for (auto& v: extData) {
        int code = v->code();
        switch (code) {
            case 1001: { // application name
                QString applicationName = v->c_str();
                currentAppData = result->addAppData(applicationName);
                break;
            }
            case 1000: { // group name
                QString groupName = v->c_str();
                currentGroup = currentAppData->addGroup(groupName);
                break;
            }
            case 1002: { // control braces
                QString ctrlString = v->c_str();
                if (ctrlString == "{") { // fixme - sand - add support of lists nesting!!!
                    listLevel ++;
                    inTagsList = true;
                    expectType = false; // for later "not", as actually we do expect it
                }
                else { // end of list
                    listLevel --;
                    inTagsList = false;
                }
                break;
            }
            case 1003: {
              if (currentGroup != nullptr) {
                if (v->isLayerRef()) {
                  currentGroup->addLayerRef(currentValType,
                                            QString{v->c_str()});
                } else {
                  currentGroup->add(currentValType, QString{v->c_str()});
                }
              }
              break;
            }
            case 1004: {
              if (currentGroup != nullptr) {
                if (v->type() == DRW_Variant::BINARY) {
                  const auto *bytes = v->binary();
                  if (bytes != nullptr) {
                    currentGroup->add(
                        currentValType,
                        QByteArray(
                            reinterpret_cast<const char *>(bytes->data()),
                            static_cast<int>(bytes->size())));
                  } else {
                    currentGroup->add(currentValType, QByteArray{});
                  }
                } else {
                  // DXF round-trip path: 1004 arrives as a hex-encoded
                  // string. Decode back to raw bytes so the in-memory
                  // representation is consistent regardless of source.
                  QByteArray hex(v->c_str());
                  currentGroup->add(currentValType, QByteArray::fromHex(hex));
                }
              }
              break;
            }
            case 1005:{
                QString val = v->c_str();
                if (currentGroup != nullptr) {
                    currentGroup->addRef(currentValType, val);
                }
                break;
            }
            case 1010:
            case 1011:
            case 1012:
            case 1013: {
                if (currentGroup != nullptr) {
                    auto coord = v->coord(); // fixme - sand - review how actually coordinate is parsed, and why it's on several codes??
                    if (coord != nullptr) {
                        currentGroup->add(currentValType, RS_Vector(coord->x, coord->y, coord->z));
                    }
                }
                break;
            }
            case 1070: // integer
            case 1071:{// long
                int val = v->i_val();
                if (expectType) {
                    // code of var
                    currentValType = val;
                }
                else {
                    // int field
                    if (currentGroup != nullptr) {
                        currentGroup->add(currentValType, val);
                    }
                }
                break;
            }
            case 1040: // real
            case 1041: // distance
            case 1042: { // scale factor
                double val = v->d_val();
                if (currentGroup != nullptr) {
                    currentGroup->add(currentValType, val);
                }
                break;
            }
            default:
                break;
        }
        if (inTagsList) {
            expectType = !expectType;
        }
    }
    return result;
}

bool RS_FilterDXFRW::shouldGenerateExtEntityData(RS_Dimension* entity) {
    // todo - so far, we support only dimension style override as extension data.
    // however, that's logic may be expanded later and store, for example,
    // something like entity-specific meta information or so.
    return entity->getDimStyleOverride() != nullptr;
}

QString RS_FilterDXFRW::toHexStr(int n){
    return QString::number(n, 16).toUpper();
}

void RS_FilterDXFRW::addDimStyleOverrideToExtendedData(LC_ExtEntityData* extEntityData, LC_DimStyle* styleOverride) {
    if (styleOverride == nullptr) {
        return;
    }
    auto appData = extEntityData->addAppData("ACAD");
    auto group = appData->addGroup("DSTYLE");

    auto arrowhead = styleOverride->arrowhead();
    auto linearFormat = styleOverride->linearFormat();
    auto extensionLine = styleOverride->extensionLine();
    auto dimensionLine = styleOverride->dimensionLine();
    auto text = styleOverride->text();
    auto tolerance = styleOverride->latteralTolerance();
    auto zerosSuppression = styleOverride->zerosSuppression();
    auto scaling = styleOverride->scaling();
    auto roundoff = styleOverride->roundOff();
    auto radial = styleOverride->radial();
    auto angularFormat = styleOverride->angularFormat();
    auto fractions = styleOverride->fractions();
    auto leader = styleOverride->leader();
    auto arc = styleOverride->arc();

    auto savedModificationMode = linearFormat->getModifyCheckMode();
    // here we're interested only in actually modified fields
    styleOverride->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);

    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMPOST)) { // $DIMPOST
        group->add(3, linearFormat->prefixOrSuffix());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMAPOST)) { // $DIMAPOST
        group->add(4, linearFormat->altPrefixOrSuffix());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) { // $DIMBLK
        // fixme - restore after test!
        // group->add(5, arrowhead->sameBlockName());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) { // $DIMBLK1
        // fixme - restore after test!
        // group->add(6, arrowhead->arrowHeadBlockNameFirst());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) { // $DIMBLK2
        // fixme - restore after test!
        // group->add(7, arrowhead->arrowHeadBlockNameSecond());
    }
    if (scaling->checkModifyState(LC_DimStyle::Scaling::$DIMSCALE)) { // $DIMSCALE
        group->add(40, scaling->scale());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMASZ)) { // $DIMASZ
        group->add(41, arrowhead->size());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXO)) { // $DIMEXO
        group->add(42, extensionLine->distanceFromOriginPoint());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLI)) {// $DIMDLI
        group->add(43, dimensionLine->baseLineDimLinesSpacing());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXE)) { // $DIMEXE
        group->add(44, extensionLine->distanceBeyondDimLine());
    }
    if (roundoff->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMRND)) { // $DIMRND
        group->add(45, roundoff->roundTo());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLE)) {// $DIMDLE
        group->add(46, dimensionLine->distanceBeyondExtLinesForObliqueStroke());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTP)) {// $DIMDTP
        group->add(47, tolerance->upperToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {// $DIMDTM
        group->add(48, tolerance->lowerToleranceLimit());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXL)) { // $DIMFXL
        group->add(49, extensionLine->fixedLength());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXT)) { // $DIMTXT
        group->add(140, text->height());
    }
    if (radial->checkModifyState(LC_DimStyle::Radial::$DIMCEN)) { // $DIMCEN
        group->add(141, radial->centerCenterMarkOrLineSize());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMTSZ)) { // $DIMTSZ
        group->add(142, arrowhead->tickSize());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTF)) { // $DIMALTF
        group->add(143, linearFormat->altUnitsMultiplier());
    }
    if (scaling->checkModifyState(LC_DimStyle::Scaling::$DIMLFAC)) { // $DIMLFAC
        group->add(144, scaling->linearFactor());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTVP)) { // $DIMTVP
        group->add(145, text->verticalDistanceToDimLine());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTFAC)) {// $DIMTFAC
        group->add(146, tolerance->heightScaleFactorToDimText());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMGAP)) {// $DIMGAP
        group->add(147, dimensionLine->lineGap());
    }
    if (roundoff->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMALTRND)) { // $DIMALTRND
        group->add(148, roundoff->altRoundTo());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILL)) { // $DIMTFILL
        group->add(69, text->backgroundFillMode());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILLCLR)) { // $DIMTFILLCLR
        int colorRgb;
        int colorNumber = colorToNumber(text->explicitBackgroundFillColor(), &colorRgb);
        group->add(70, colorNumber);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOL)) {// $DIMTOL
        group->add(71, tolerance->isAppendTolerancesToDimText() ? 1 : 0);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMLIM)) {// $DIMLIM
        group->add(72, tolerance->isLimitsGeneratedAsDefaultText() ? 1 : 0);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIH)) { // $DIMTIH
        group->add(73, text->orientationInside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTOH)) { // $DIMTOH
        group->add(74, text->orientationOutside());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE1)) { // $DIMSE1
        group->add(75, extensionLine->suppressFirstLine());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE2)) { // $DIMSE2
        group->add(76, extensionLine->suppressSecondLine());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTAD)) { // $DIMTAD
        group->add(77, text->verticalPositioning());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMZIN)) { // $DIMZIN
        group->add(78, zerosSuppression->linearRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMAZIN)) { // $DIMAZIN
        group->add(79, zerosSuppression->angularRaw());
    }
    if (arc->checkModifyState(LC_DimStyle::Arc::$DIMARCSYM)) { // $DIMARCSYM
        group->add(90, arc->arcSymbolPosition());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALT)) { // $DIMALT
        group->add(170, linearFormat->alternateUnits());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTD)) { // $DIMALTD
        group->add(171, linearFormat->altDecimalPlaces());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMTOFL)) {// $DIMTOFL
        group->add(172, dimensionLine->drawPolicyForOutsideText());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH)) { // $DIMSAH
        group->add(173, arrowhead->isUseSeparateArrowHeads()); // fixme - check value
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIX)) { // $DIMTIX
        group->add(174, text->extLinesRelativePlacement());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSOXD)) { // $DIMSOXD
        group->add(175, arrowhead->suppression()); // fixme - check value
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMCLRD)) {// $DIMCLRD
        int colorRgb;
        int colorNumber = colorToNumber(dimensionLine->color(), &colorRgb);
        group->add(176, colorNumber);
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMCLRE)) {// $DIMCLRE
        int colorRgb;
        int color = colorToNumber(extensionLine->color(), &colorRgb);
        group->add(177, color);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMCLRT)) {// $DIMCLRT
        int colorRgb;
        int colorNumber = colorToNumber(text->color(), &colorRgb);
        group->add(178, colorNumber);
    }
    if (angularFormat->checkModifyState(LC_DimStyle::AngularFormat::$DIMADEC)) { // $DIMADEC
        group->add(179, angularFormat->decimalPlaces());
    }
    // case 270: // fixme - sand - obsolete DIMUNIT
    // result->linearFormat()->setUDecimalPlaces(var->getInt());
    // dimunit = reader->getInt32();
    // add("$DIMUNIT", code, dimunit);
    // break;
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMDEC)) { // $DIMDEC
        group->add(271, linearFormat->decimalPlaces());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTDEC)) {// $DIMTDEC
        group->add(272, tolerance->decimalPlaces());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTU)) { // $DIMALTU
        group->add(273, linearFormat->altFormat());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMALTTD)) {// $DIMALTTD
        group->add(274, tolerance->decimalPlacesAltDim());
    }
    if (angularFormat->checkModifyState(LC_DimStyle::AngularFormat::$DIMAUNIT)) { // $DIMAUNIT
        group->add(275, angularFormat->format());
    }
    if (fractions->checkModifyState(LC_DimStyle::Fractions::$DIMFRAC)) { // $DIMFRAC
        group->add(276, fractions->style());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMLUNIT)) { // $DIMLUNIT
        group->add(277, linearFormat->formatRaw());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMSEP)) { // $DIMDSEP
        group->add(278, linearFormat->decimalFormatSeparatorChar());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTMOVE)) { // $DIMTMOVE
        group->add(279, text->positionMovementPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMJUST)) { // $DIMJUST
        group->add(280, text->horizontalPositioning());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD1)) {// $DIMSD1
        group->add(281, dimensionLine->suppressFirstLine());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD2)) {// $DIMSD2
        group->add(282, dimensionLine->suppressSecondLine());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOLJ)) {// $DIMTOLJ
        group->add(283, tolerance->verticalJustification());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMTZIN)) { // $DIMTZIN
        group->add(284, zerosSuppression->toleranceRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTZ)) { // $DIMALTZ
        group->add(285, zerosSuppression->altLinearRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTTZ)) { // $DIMALTTZ
        group->add(286, zerosSuppression->altToleranceRaw());
    }
    //case 287: // fixme - DIMFIT
    // dimfit = reader->getInt32();
    // add("$DIMFIT", code, dimfit);
    if (text->checkModifyState(LC_DimStyle::Text::$DIMUPT)) { // $DIMUPT
        group->add(288, text->cursorControlPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMATFIT)) { // $DIMATFIT
        group->add(289, text->unsufficientSpacePolicy());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXLON)) { // $DIMFXLON
        group->add(290, extensionLine->hasFixedLength() ? 1 : 0);  // fixme - check
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXTDIRECTION)) { // $DIMTXTDIRECTION
        group->add(292, text->readingDirection());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXSTY)) { // $DIMTXSTY
        group->add(340, text->style()); // fixme - ref to style?
    }
    if (leader->checkModifyState(LC_DimStyle::Leader::$DIMLDRBLK)) { //DIMLDRBLK
        auto blockName = leader->arrowBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                group->addRef(341, toHexStr(blkHandle));
            }
        }
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) { //$DIMBLK
        auto blockName = arrowhead->sameBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                group->addRef(342, toHexStr(blkHandle));
            }
        }
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) { //$DIMBLK1
        auto blockName = arrowhead->arrowHeadBlockNameFirst();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                group->addRef(343, toHexStr(blkHandle));
            }
        }
    }

    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) { //$DIMBLK2
        auto blockName = arrowhead->arrowHeadBlockNameSecond();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                group->addRef(344, toHexStr(blkHandle));
            }
        }
    }
/*
                    // case 345: // codes///
                    // fixme - may this code be used for DIMLDRBLK?
                    //      dimblk2 = reader->getUtf8String();
                    //      add("$DIMBLK2", code, dimblk2);
                    //      break;
     */

    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLTYPE)) { // $DIMLTYPE
        int lineTypeHandle = findLineTypeHandleToWrite(dimensionLine->lineTypeName());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(345, handleStr);
        }
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX1)) { // $DIMLTEX1
        int lineTypeHandle = findLineTypeHandleToWrite(extensionLine->lineTypeFirstRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(347, handleStr);
        }
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX2)) { // $DIMLTEX2
        int lineTypeHandle = findLineTypeHandleToWrite(extensionLine->lineTypeSecondRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(348, handleStr);
        }
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLWD)) { // $DIMLWD
        auto lineWidth = dimensionLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        group->add(371, lw);
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLWE)) { // $DIMLWE
        auto lineWidth = extensionLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        group->add(372, lw);
    }

    styleOverride->setModifyCheckMode(savedModificationMode);
}

LC_DimStyle* RS_FilterDXFRW::parseDimStyleOverride(LC_ExtEntityData* extEntityData) const {
    if (extEntityData != nullptr) {
        auto dimStyleGroup = extEntityData->getGroupByName("ACAD", "DSTYLE");
        if (dimStyleGroup != nullptr) {
            auto dimStyleVariables = dimStyleGroup->getTagsList();
            if (dimStyleVariables->empty()) {
                return nullptr;
            }
            auto* result = new LC_DimStyle();
            auto arrowhead = result->arrowhead();
            auto linearFormat = result->linearFormat();
            auto extensionLine = result->extensionLine();
            auto dimensionLine = result->dimensionLine();
            auto text = result->text();
            auto tolerance = result->latteralTolerance();
            auto zerosSuppression = result->zerosSuppression();
            auto arc = result->arc();

            result->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
            auto count = dimStyleVariables->size();
            for (size_t i = 0; i < count; i++) {
                LC_ExtDataTag* tag = dimStyleVariables->at(i);
                RS_Variable* var = tag->var();
                int code = var->getCode();

                switch (code) {
                    case 105:
                        // handle = reader->getHandleString();
                        break;
                    case 3: // "$DIMPOST"
                        linearFormat->setPrefixOrSuffix(var->getString());
                        break;
                    case 4: // "$DIMAPOST"
                        linearFormat->setAltPrefixOrSuffix(var->getString());
                        break;
                    case 5: // $DIMBLK
                        arrowhead->setSameBlockName(var->getString());
                        break;
                    case 6: // "$DIMBLK1"
                        arrowhead->setArrowHeadBlockNameFirst(var->getString());
                        break;
                    case 7: // "$DIMBLK2"
                        arrowhead->setArrowHeadBlockNameSecond(var->getString());
                        break;
                    case 40: // "$DIMSCALE"
                        result->scaling()->setScale(var->getDouble());
                        break;
                    case 41: // "$DIMASZ"
                        arrowhead->setSize(var->getDouble());
                        break;
                    case 42: // "$DIMEXO"
                        extensionLine->setDistanceFromOriginPoint(var->getDouble());
                        break;
                    case 43: //"$DIMDLI"
                        dimensionLine->setBaselineDimLinesSpacing(var->getDouble());
                        break;
                    case 44: //"$DIMEXE"
                        extensionLine->setDistanceBeyondDimLine(var->getDouble());
                        break;
                    case 45: //$DIMRND
                        result->roundOff()->setRoundToValue(var->getDouble());
                        break;
                    case 46: //$DIMDLE
                        dimensionLine->setDistanceBeyondExtLinesForObliqueStroke(var->getDouble());
                        break;
                    case 47: //"$DIMTP"
                        tolerance->setUpperToleranceLimit(var->getDouble());
                        break;
                    case 48: //"$DIMTM"
                        tolerance->setLowerToleranceLimit(var->getDouble());
                        break;
                    case 49: //"$DIMFXL"
                        extensionLine->setFixedLength(var->getDouble());
                        break;
                    case 140:// "$DIMTXT"
                        text->setHeight(var->getDouble());
                        break;
                    case 141: // "$DIMCEN"
                        result->radial()->setCenterMarkOrLineSize(var->getDouble());
                        break;
                    case 142: //"$DIMTSZ"
                        arrowhead->setTickSize(var->getDouble());
                        break;
                    case 143: //"$DIMALTF"
                        linearFormat->setAltUnitsMultiplier(var->getDouble());
                        break;
                    case 144: //"$DIMLFAC"
                        result->scaling()->setLinearFactor(var->getDouble());
                        break;
                    case 145: //"$DIMTVP"
                        text->setVerticalDistanceToDimLine(var->getDouble());
                        break;
                    case 146: //"$DIMTFAC"
                        tolerance->setHeightScaleFactorToDimText(var->getDouble());
                        break;
                    case 147: //"$DIMGAP"
                        dimensionLine->setLineGap(var->getDouble());
                        break;
                    case 148: //"$DIMALTRND"
                        result->roundOff()->setAltRoundToValue(var->getDouble());
                        break;
                    case 69: // "$DIMTFILL"
                        text->setBackgroundFillModeRaw(var->getInt());
                        break;
                    case 70: {
                        //"$DIMTFILLCLR"
                        RS_Color fillClr = numberToColor(var->getInt());
                        text->setExplicitBackgroundFillColor(fillClr);
                        break;
                    }
                    case 71: // "$DIMTOL"
                        tolerance->setAppendTolerancesToDimText(var->getInt() == 1); // fixme - review
                        break;
                    case 72: //"$DIMLIM"
                        tolerance->setLimitsAreGeneratedAsDefaultText(var->getInt() == 1); // fixme - review
                        break;
                    case 73: //"$DIMTIH"
                        text->setOrientationInsideRaw(var->getInt());
                        break;
                    case 74: //"$DIMTOH"
                        text->setOrientationOutsideRaw(var->getInt());
                        break;
                    case 75: //"$DIMSE1"
                        extensionLine->setSuppressFirstRaw(var->getInt());
                        break;
                    case 76: //"$DIMSE2"
                        extensionLine->setSuppressSecondRaw(var->getInt());
                        break;
                    case 77: //"$DIMTAD"
                        text->setVerticalPositioningRaw(var->getInt());
                        break;
                    case 78: // "$DIMZIN"
                        zerosSuppression->setLinearRaw(var->getInt());
                        break;
                    case 79: //"$DIMAZIN"
                        zerosSuppression->setAngularRaw(var->getInt());
                        break;
                    case 170: //"$DIMALT"
                        linearFormat->setAlternateUnitsRaw(var->getInt());
                        break;
                    case 171: //"$DIMALTD"
                        linearFormat->setAltDecimalPlaces(var->getInt());
                        break;
                    case 172: //"$DIMTOFL"
                        dimensionLine->setDrawPolicyForOutsideTextRaw(var->getInt());
                        break;
                    case 173: //"$DIMSAH"
                        arrowhead->setUseSeparateArrowHeads(var->getInt()); // fixme - check value
                        break;
                    case 174: // "$DIMTIX"
                        text->setExtLinesRelativePlacementRaw(var->getInt());
                        break;
                    case 175: //"$DIMSOXD"
                        arrowhead->setSuppressionsRaw(var->getInt()); // fixme - check value
                        break;
                    case 176: { //"$DIMCLRD"
                        RS_Color color = numberToColor(var->getInt());
                        dimensionLine->setColor(color);
                        break;
                    }
                    case 177: { //"$DIMCLRE"
                        RS_Color color = numberToColor(var->getInt());
                        extensionLine->setColor(color);
                        break;
                    }
                    case 178: { //"$DIMCLRT"
                        RS_Color color = numberToColor(var->getInt());
                        text->setColor(color);
                        break;
                    }
                    case 179: //"$DIMADEC"
                        result->angularFormat()->setDecimalPlaces(var->getInt());
                        break;
                    case 270: // fixme - sand - obsolete DIMUNIT
                        // result->linearFormat()->setUDecimalPlaces(var->getInt());
                        // dimunit = reader->getInt32();
                        // add("$DIMUNIT", code, dimunit);
                        break;
                    case 271: //"$DIMDEC"
                        linearFormat->setDecimalPlaces(var->getInt());
                        break;
                    case 272://"$DIMTDEC"
                        tolerance->setDecimalPlaces(var->getInt());
                        break;
                    case 273: //"$DIMALTU"
                        linearFormat->setAltFormatRaw(var->getInt());
                        break;
                    case 274: // "$DIMALTTD"
                        tolerance->setDecimalPlacesAltDim(var->getInt());
                        break;
                    case 275: //"$DIMAUNIT"
                        result->angularFormat()->setFormatRaw(var->getInt());
                        break;
                    case 276: // "$DIMFRAC"
                        result->fractions()->setStyleRaw(var->getInt());
                        break;
                    case 277: // "$DIMLUNIT"
                        linearFormat->setFormatRaw(var->getInt());
                        break;
                    case 278: //"$DIMDSEP"
                        linearFormat->setDecimalFormatSeparatorChar(var->getInt());
                        break;
                    case 279: // "$DIMTMOVE"
                        text->setPositionMovementPolicyRaw(var->getInt());
                        break;
                    case 280: // "$DIMJUST"
                        text->setHorizontalPositioningRaw(var->getInt());
                        break;
                    case 281: // "$DIMSD1"
                        dimensionLine->setSuppressFirstLineRaw(var->getInt());
                        break;
                    case 282: // "$DIMSD2"
                        dimensionLine->setSuppressSecondLineRaw(var->getInt());
                        break;
                    case 283: // "$DIMTOLJ"
                        tolerance->setVerticalJustificationRaw(var->getInt());
                        break;
                    case 284:// "$DIMTZIN"
                        zerosSuppression->setToleranceRaw(var->getInt());
                        break;
                    case 285: // "$DIMALTZ"
                        zerosSuppression->setAltLinearRaw(var->getInt());
                        break;
                    case 286: //"$DIMALTTZ"
                        zerosSuppression->setAltToleranceRaw(var->getInt());
                        break;
                    case 287: // fixme - DIMFIT
                        // dimfit = reader->getInt32();
                        // add("$DIMFIT", code, dimfit);
                        break;
                    case 288: // "$DIMUPT"
                        text->setCursorControlPolicyRaw(var->getInt());
                        break;
                    case 289: //"$DIMATFIT"
                        text->setUnsufficientSpacePolicyRaw(var->getInt());
                        break;
                    case 290: // "$DIMFXLON"
                        extensionLine->setHasFixedLength(var->getInt()); // fixme - check
                        break;
                    case 292: // "$DIMTXTDIRECTION"
                        text->setReadingDirectionRaw(var->getInt());
                        break;
                    case 340: // "$DIMTXSTY"
                    {
                        auto dimtxsty = var->getString();
                        prepareTextStyleName(dimtxsty);
                        text->setStyle(dimtxsty); // fixme - ref to style?
                        // fixme - ref to style?
                        break;
                    }
                    case 341: {
                        // "_$DIMLDRBLK"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                result->leader()->setArrowBlockName(blockName);
                            }
                        }
                        else { // the string is not handle, but a direct name of the block.
                            // fixme - DIMLDRBLK reading!
                            // This is workaround for referring leader by name, not by block ref...
                            // however, it may be not ACad compatible..
                            // if (LC_DimArrowRegistry::isStandardBlockName(refHandleStr)) {
                                // result->leader()->setArrowBlockName(refHandleStr);
                            // }
                        }
                        break;
                    }
                    case 342: {// "_$DIMBLK"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setSameBlockName(blockName);
                            }
                        }
                        break;
                    }
                    case 343: {
                        // "_$DIMBLK1"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setArrowHeadBlockNameFirst(blockName);
                            }
                        }
                        break;
                    }
                    case 344: {
                        // "_$DIMBLK2"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setArrowHeadBlockNameSecond(blockName);
                            }
                        }
                        break;
                    }
                    // case 345: // codes///
                    // fixme - may this code be used for DIMLDRBLK?
                    //      dimblk2 = reader->getUtf8String();
                    //      add("$DIMBLK2", code, dimblk2);
                    //      break;
                    case 345: {
                        // "$DIMLTYPE"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            dimensionLine->setLineType(name);
                        }
                        break;
                    }
                    case 347: { // "$DIMLTEX1"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            extensionLine->setLineTypeFirst(name);
                        }
                        break;
                    }
                    case 348: { //"$DIMLTEX2"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            extensionLine->setLineTypeSecond(name);
                        }
                        break;
                    }
                    case 371: // "$DIMLWD"
                        dimensionLine->setLineWidthRaw(var->getInt());
                        break;
                    case 372: //"$DIMLWE"
                        extensionLine->setLineWidthRaw(var->getInt());
                        break;
                    case 90: //"$DIMARCSYM"
                        arc->setArcSymbolPositionRaw(var->getInt());
                        break;
                    default:
                        break;
                }
            }

            // workaround for AutoCAD - it seems that later versions ignores DIMSAH and consider blocks for arrowhead to be
            // different if individual arrows are different. So we set the flag manually
            // fixme - check whether given condition is enough for properly setting the flag
            result->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);
            if (!arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH) ) {
                if(arrowhead->sameBlockName().isEmpty() && (arrowhead->arrowHeadBlockNameFirst() != arrowhead->arrowHeadBlockNameSecond())) {
                    arrowhead->setUseSeparateArrowHeads(true);
                }
            }

            return result;
        }
    }
    return nullptr;
}

/**
 * Implementation of the method which handles
 * aligned dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAlign(const DRW_DimAligned *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAligned");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector ext1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector ext2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimAlignedData d(ext1, ext2);
    auto* entity = new RS_DimAligned(m_currentContainer,dimensionData, d);
    setEntityAttributes(entity, data);
    entity->updateDimPoint();
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * linear dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimLinear(const DRW_DimLinear *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLinear");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector dxt1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector dxt2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimLinearData d(dxt1, dxt2,
                       RS_Math::deg2rad(data->getAngle()), RS_Math::deg2rad(data->getOblique()));

    auto entity = new RS_DimLinear(m_currentContainer,dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * radial dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimRadial(const DRW_DimRadial* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimRadial");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(data->getDiameterPoint().x, data->getDiameterPoint().y);

    RS_DimRadialData d(dp, data->getLeaderLength());
    auto entity = new RS_DimRadial(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * diametric dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimDiametric(const DRW_DimDiametric* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimDiametric");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(data->getDiameter1Point().x, data->getDiameter1Point().y);

    RS_DimDiametricData d(dp, data->getLeaderLength());
    auto entity = new RS_DimDiametric(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular(const DRW_DimAngular* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine1().x, data->getFirstLine1().y);
    RS_Vector dp2(data->getFirstLine2().x, data->getFirstLine2().y);
    RS_Vector dp3(data->getSecondLine1().x, data->getSecondLine1().y);
    RS_Vector dp4(data->getDimPoint().x, data->getDimPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    auto entity = new RS_DimAngular(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular3P(const DRW_DimAngular3p* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular3P");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine().x, data->getFirstLine().y);
    RS_Vector dp2(data->getSecondLine().x, data->getSecondLine().y);
    RS_Vector dp3(data->getVertexPoint().x, data->getVertexPoint().y);
	RS_Vector dp4 = dimensionData.definitionPoint;
	dimensionData.definitionPoint = RS_Vector(data->getVertexPoint().x, data->getVertexPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    auto entity = new RS_DimAngular(m_currentContainer, dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

void RS_FilterDXFRW::addDimArc(const DRW_DimArc* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimArc");
    RS_DimensionData dd = convDimensionData(data);
    RS_Vector centre(data->getArcCenter().x, data->getArcCenter().y);
    double radius = dd.definitionPoint.distanceTo(centre);
    LC_DimArcData arcData(
        radius,
        radius * std::abs(data->arcEndAngle - data->arcStartAngle),
        centre,
        RS_Vector::polar(1.0, data->arcEndAngle),
        RS_Vector::polar(1.0, data->arcStartAngle)
    );
    arcData.arcSymbol = data->arcSymbol;
    arcData.isPartial = data->isPartial;
    arcData.hasLeader = data->hasLeader;
    arcData.leaderPt1 = RS_Vector(data->getLeaderPt1().x, data->getLeaderPt1().y);
    arcData.leaderPt2 = RS_Vector(data->leaderPt2.x, data->leaderPt2.y);
    auto dimEntity = new LC_DimArc(m_currentContainer, dd, arcData);
    setEntityAttributes(dimEntity, data);
    dimEntity->update();
    m_currentContainer->addEntity(dimEntity);
}

void RS_FilterDXFRW::addDimOrdinate(const DRW_DimOrdinate* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) not yet implemented");
    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector featurePoint{data->getFirstLine().x, data->getFirstLine().y};
    RS_Vector leaderEndPoint{data->getSecondLine().x, data->getSecondLine().y};

    bool ordinateTypeForX = false;
    int type = data->type;
    if (type & 64) {
        ordinateTypeForX = true;
    }
    LC_DimOrdinateData d(featurePoint, leaderEndPoint, ordinateTypeForX);
    auto* entity = new LC_DimOrdinate(m_currentContainer, dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles leader entities.
 */
void RS_FilterDXFRW::addLeader(const DRW_Leader *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLeader");
    RS_LeaderData d(data->arrow!=0, QString::fromUtf8(data->style.c_str()));
    auto leader = new RS_Leader(m_currentContainer, d);
    setEntityAttributes(leader, data);

	for (auto const& vert: data->vertexlist) {
	    leader->addVertex({vert->x, vert->y});
	}

    leader->update();
    m_currentContainer->addEntity(leader);
}

namespace {
// Sample count for the cubic-NURBS → quadratic spline-points sampling branch
// in buildHatchSplineEdge. See plan §C.2.
constexpr int kHatchSplineSamples = 64;
} // namespace

/**
 * Build an LC_SplinePoints from a DRW_Spline hatch-boundary edge.
 * All degrees converge to a quadratic spline-points representation:
 *   degree==2 fit-points-only          → pass-through as splinePoints
 *   degree==2 control-net              → pass-through as controlPoints
 *   degree==1 or degree==3             → sample 64 points via a throwaway
 *                                        RS_Spline evaluator
 * Returns nullptr on degenerate input. See plan §C.1-§C.3.
 */
LC_SplinePoints *
RS_FilterDXFRW::buildHatchSplineEdge(RS_EntityContainer *hatchLoop,
                                     const DRW_Spline *s) {
  if (s == nullptr)
    return nullptr;
  if (s->degree < 1 || s->degree > 3) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "buildHatchSplineEdge: unsupported degree %d", s->degree);
    return nullptr;
  }
  if (s->controllist.empty() && s->fitlist.empty()) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "buildHatchSplineEdge: spline edge has neither control nor fit points");
    return nullptr;
  }

  // DXF entity-level splines store "closed" in bit 0; DWG hatch-boundary
  // streams store "periodic" in bit 1 and never set bit 0 (see
  // libraries/libdxfrw/src/drw_entities.cpp parseDwg at lines 2799-2800).
  // Either bit implies a closed curve.
  const bool closed = (s->flags & 0x3) != 0;
  LC_SplinePointsData sd(closed, /*cut*/ false);

  if (s->degree == 2 && s->controllist.empty() && !s->fitlist.empty()) {
    // Branch 1: degree==2 with fit points only — pass through.
    sd.useControlPoints = false;
    for (const auto &fp : s->fitlist) {
      if (fp)
        sd.splinePoints.push_back(RS_Vector{fp->x, fp->y});
    }
  } else if (s->degree == 2 && !s->controllist.empty()) {
    // Branch 2: degree==2 with control net — pass through.
    sd.useControlPoints = true;
    for (const auto &cp : s->controllist) {
      if (cp)
        sd.controlPoints.push_back(RS_Vector{cp->x, cp->y});
    }
  } else {
    // Branch 3: degree==1 or degree==3. Sample the NURBS at 64 points
    // via a throwaway RS_Spline; the samples become fit points.
    // Fall back to treating control points as a polyline if the knot
    // vector is missing or short.
    const size_t needKnots = s->controllist.size() + size_t(s->degree) + 1;
    if (s->knotslist.size() < needKnots) {
      RS_DEBUG->print(RS_Debug::D_WARNING,
                      "buildHatchSplineEdge: short knot vector (%zu < %zu); "
                      "treating control points as polyline",
                      s->knotslist.size(), needKnots);
      sd.useControlPoints = false;
      for (const auto &cp : s->controllist) {
        if (cp)
          sd.splinePoints.push_back(RS_Vector{cp->x, cp->y});
      }
    } else {
      RS_SplineData td(s->degree, closed);
      td.type = closed ? RS_SplineData::SplineType::Standard
                       : RS_SplineData::SplineType::ClampedOpen;
      const double tolknot = (s->tolknot > 0.0) ? s->tolknot : 1e-7;
      for (double k : s->knotslist) {
        td.knotslist.push_back(RS_Math::round(k, tolknot));
      }

      auto tmp = std::make_unique<RS_Spline>(nullptr, td);
      const bool isRational = (s->flags & 0x4) != 0;
      for (size_t i = 0; i < s->controllist.size(); ++i) {
        const auto &cp = s->controllist[i];
        if (!cp)
          continue;
        // DXF stores rational weights in weightlist; DWG hatch-
        // boundary stream stores them on controllist[i]->z. Check
        // both. See plan §C.3.
        double w = 1.0;
        if (isRational) {
          if (i < s->weightlist.size())
            w = s->weightlist[i];
          else
            w = cp->z;
        }
        tmp->addControlPointRaw({cp->x, cp->y}, w);
      }
      if (closed)
        tmp->setClosed(true);
      tmp->update();

      sd.useControlPoints = false;
      sd.splinePoints.reserve(kHatchSplineSamples);
      tmp->fillStrokePoints(kHatchSplineSamples - 1, sd.splinePoints);
    }
  }

  if (sd.splinePoints.size() < 2 && sd.controlPoints.size() < 2) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "buildHatchSplineEdge: degenerate spline edge (too few points)");
    return nullptr;
  }

  // Some DWG hatch boundaries encode a geometrically-closed spline with
  // both flag bits unset (and an accompanying 0-length line as a closure
  // marker). Detect endpoint coincidence in the populated point list and
  // flip the closed flag so LoopExtractor treats it as a single closed
  // loop instead of an open edge whose start==end. Closed LC_SplinePoints
  // expects a periodic point list without an explicit closing repeat —
  // drop the duplicate tail when present.
  if (!sd.closed) {
    if (!sd.splinePoints.empty() &&
        sd.splinePoints.front().distanceTo(sd.splinePoints.back()) <= 1e-8) {
      sd.closed = true;
      if (sd.splinePoints.size() > 2)
        sd.splinePoints.pop_back();
    } else if (!sd.controlPoints.empty() &&
               sd.controlPoints.front().distanceTo(sd.controlPoints.back()) <=
                   1e-8) {
      sd.closed = true;
      if (sd.controlPoints.size() > 2)
        sd.controlPoints.pop_back();
    }
  }

  return new LC_SplinePoints(hatchLoop, std::move(sd));
}

/**
 * Snap LC_SplinePoints endpoints inside hatchLoop to neighboring line-like
 * edge endpoints when the gap is within 10× ENDPOINT_TOLERANCE (1e-7) but
 * non-zero. Boundary-stream float drift can leave ~1e-9 to ~1e-8 gaps that
 * break LoopExtractor's endpoint-chaining. The 10× window is small enough
 * that geometrically distinct edges never stitch.
 */
void RS_FilterDXFRW::snapSplineEdgeEndpoints(RS_EntityContainer *hatchLoop) {
  if (hatchLoop == nullptr || hatchLoop->count() < 2)
    return;
  constexpr double kSnapWindow = 1e-7; // 10× ENDPOINT_TOLERANCE
  const unsigned int n = hatchLoop->count();

  auto setSplineEnd = [](LC_SplinePoints *sp, bool atFront,
                         const RS_Vector &v) {
    auto &d = sp->getData();
    if (d.useControlPoints && !d.controlPoints.empty()) {
      if (atFront)
        d.controlPoints.front() = v;
      else
        d.controlPoints.back() = v;
    } else if (!d.splinePoints.empty()) {
      if (atFront)
        d.splinePoints.front() = v;
      else
        d.splinePoints.back() = v;
    }
    sp->update();
  };

  for (unsigned int i = 0; i < n; ++i) {
    RS_Entity *cur = hatchLoop->entityAt(i);
    RS_Entity *nxt = hatchLoop->entityAt((i + 1) % n);
    if (cur == nullptr || nxt == nullptr)
      continue;

    const bool curSpline = cur->rtti() == RS2::EntitySplinePoints;
    const bool nxtSpline = nxt->rtti() == RS2::EntitySplinePoints;
    if (curSpline == nxtSpline)
      continue; // line-line or spline-spline

    const RS_Vector curEnd = cur->getEndpoint();
    const RS_Vector nxtBeg = nxt->getStartpoint();
    const double gap = curEnd.distanceTo(nxtBeg);
    if (gap <= 0.0 || gap >= kSnapWindow)
      continue;

    if (curSpline) {
      setSplineEnd(static_cast<LC_SplinePoints *>(cur), /*atFront*/ false,
                   nxtBeg);
    } else {
      setSplineEnd(static_cast<LC_SplinePoints *>(nxt), /*atFront*/ true,
                   curEnd);
    }
  }
}

/**
 * Implementation of the method which handles hatch entities.
 */
void RS_FilterDXFRW::addHatch(const DRW_Hatch *data) {
    RS_DEBUG->print("RS_FilterDXF::addHatch()");
    RS_EntityContainer* hatchLoop;
    auto hatch = new RS_Hatch(m_currentContainer,
                         RS_HatchData(data->solid, data->scale, data->angle,
                                      QString::fromUtf8(data->name.c_str())));
    setEntityAttributes(hatch, data);
    m_currentContainer->appendEntity(hatch);

    for (unsigned int i=0; i < data->looplist.size(); i++) {
        auto& loop = data->looplist.at(i);
        if ((loop->type & 32) == 32) {
            continue;
        }
        hatchLoop = new RS_EntityContainer(hatch);
        hatchLoop->setLayer(nullptr);
        hatch->addEntity(hatchLoop);

        RS_Entity* e = nullptr;
        if ((loop->type & 2) == 2){   //polyline, convert to lines & arcs
            DRW_LWPolyline* pline = static_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
            RS_Polyline polyline{nullptr,
                                 RS_PolylineData(RS_Vector(false), RS_Vector(false), pline->flags)};
            for (auto const& vert: pline->vertlist) {
                polyline.addVertex(RS_Vector{vert->x, vert->y}, vert->bulge);
            }

            for(RS_Entity* e: lc::LC_ContainerTraverser{polyline, RS2::ResolveNone}.entities()) {
         //   for (RS_Entity* e=polyline.firstEntity(); e; e=polyline.nextEntity()) {
                RS_Entity* tmp = e->clone();
                tmp->reparent(hatchLoop);
                tmp->setLayer(nullptr);
                hatchLoop->addEntity(tmp);
            }

        } else {
            for (unsigned int j=0; j<loop->objlist.size(); j++) {
                e = nullptr;
                auto& ent = loop->objlist.at(j);
                switch (ent->eType) {
                    case DRW::LINE: {
                        DRW_Line *e2 = static_cast<DRW_Line*>(ent.get());
                        e = new RS_Line{hatchLoop,
                                        {{e2->basePoint.x, e2->basePoint.y},
                                         {e2->secPoint.x, e2->secPoint.y}}};
                        break;
                    }
                    case DRW::ARC: {
                        DRW_Arc *e2 = static_cast<DRW_Arc*>(ent.get());
                        if (e2->isccw && e2->staangle<1.0e-6 && e2->endangle>RS_Math::deg2rad(360)-1.0e-6) {
                            e = new RS_Circle(hatchLoop,
                                              {{e2->basePoint.x, e2->basePoint.y},
                                               e2->radious});
                        } else {

                            if (e2->isccw) {
                                e = new RS_Arc(hatchLoop,
                                               RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                          RS_Math::correctAngle(e2->staangle),
                                                          RS_Math::correctAngle(e2->endangle),
                                                          false));
                            } else {
                                e = new RS_Arc(hatchLoop,
                                               RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                          RS_Math::correctAngle(2*M_PI-e2->staangle),
                                                          RS_Math::correctAngle(2*M_PI-e2->endangle),
                                                          true));
                            }
                        }
                        break;
                    }
                    case DRW::ELLIPSE: {
                        DRW_Ellipse *e2 = static_cast<DRW_Ellipse*>(ent.get());
                        double ang1 = e2->staparam;
                        double ang2 = e2->endparam;
                        if ( fabs(ang2 - 2.*M_PI) < 1.0e-10 && fabs(ang1) < 1.0e-10 ) {
                            ang2 = 0.0;
                        }
                        else { //convert angle to parameter
                            ang1 = atan(tan(ang1)/e2->ratio);
                            ang2 = atan(tan(ang2)/e2->ratio);
                            if (ang1 < 0) {
                                //quadrant 2 & 4
                                ang1 +=M_PI;
                                if (e2->staparam > M_PI) {
                                    //quadrant 4
                                    ang1 += M_PI;
                                }
                            } else if (e2->staparam > M_PI){//3 quadrant
                                ang1 +=M_PI;
                            }
                            if (ang2 < 0){//quadrant 2 & 4
                                ang2 +=M_PI;
                                if (e2->endparam > M_PI) {
                                    //quadrant 4
                                    ang2 +=M_PI;
                                }
                            } else if (e2->endparam > M_PI){//3 quadrant
                                ang2 +=M_PI;
                            }
                        }
                        e = new RS_Ellipse{hatchLoop,
                                           {{e2->basePoint.x, e2->basePoint.y},
                                            {e2->secPoint.x, e2->secPoint.y},
                                            e2->ratio, ang1, ang2, !e2->isccw}};
                        break;
                    }
                    case DRW::SPLINE: {
                      e = buildHatchSplineEdge(
                          hatchLoop, static_cast<DRW_Spline *>(ent.get()));
                      break;
                    }
                    default:
                        break;
                }
                if (e) {
                    e->setLayer(nullptr);
                    hatchLoop->addEntity(e);
                }
            }
            // After all explicit-segment edges are attached, snap spline
            // endpoints to neighboring line endpoints if a tiny float gap
            // is the only thing standing between LoopExtractor and a closed
            // chain. See plan §C.1.
            snapSplineEdgeEndpoints(hatchLoop);
        }

    }

    RS_DEBUG->print("hatch->update()");
    if (hatch->validate()) {
        hatch->update();
    } else {
        m_graphic->removeEntity(hatch);
        RS_DEBUG->print(RS_Debug::D_ERROR,"RS_FilterDXFRW::endEntity(): updating hatch failed: invalid hatch area");
    }
}

/**
 * Implementation of the method which handles image entities.
 */
void RS_FilterDXFRW::addImage(const DRW_Image *data) {
    RS_DEBUG->print("RS_FilterDXF::addImage");
    if (m_graphic != nullptr && data != nullptr)
        m_graphic->dwgAdvancedMetadata().addRasterImage(*data, false);

    RS_Vector ip(data->basePoint.x, data->basePoint.y);
    RS_Vector uv(data->secPoint.x, data->secPoint.y);
    RS_Vector vv(data->vVector.x, data->vVector.y);
    RS_Vector size(data->sizeu, data->sizev);

    auto image = new RS_Image( m_currentContainer,
            RS_ImageData(data->ref, ip, uv, vv, size,
                         QString(""), data->brightness,
                         data->contrast, data->fade));

    setEntityAttributes(image, data);
    m_currentContainer->appendEntity(image);
}

/**
 * Implementation of the method which handles WIPEOUT entities.
 *
 * WIPEOUT shares the IMAGE binary layout but only the polygon (clipPath) and
 * the IMAGE's frame parameters (basePoint, secPoint=uVector, vVector,
 * sizeu/sizev) are meaningful — there's no raster file behind it.
 *
 * AutoCAD stores the polygon vertices in normalized image-pixel coordinates,
 * with a half-pixel origin offset.  The WCS transform is:
 *     P_wcs = basePoint + (px + 0.5) * sizeu * uVector
 *                       + (py + 0.5) * sizev * vVector
 * (cf. ODA Open Design Specification §20.4.96; verify on samples — see plan.)
 */
void RS_FilterDXFRW::addWipeout(const DRW_Image *data) {
  RS_DEBUG->print("RS_FilterDXFRW::addWipeout");
  if (m_graphic != nullptr && data != nullptr)
    m_graphic->dwgAdvancedMetadata().addRasterImage(*data, true);
  if (data == nullptr || data->clipPath.empty()) {
    return;
  }

  const RS_Vector base(data->basePoint.x, data->basePoint.y);
  const RS_Vector u(data->secPoint.x, data->secPoint.y);
  const RS_Vector v(data->vVector.x, data->vVector.y);
  const double sizeU = data->sizeu;
  const double sizeV = data->sizev;

  std::vector<RS_Vector> wcsVerts;
  wcsVerts.reserve(data->clipPath.size());
  for (const DRW_Coord &c : data->clipPath) {
    const double fx = c.x + 0.5;
    const double fy = c.y + 0.5;
    wcsVerts.push_back(base + u * (fx * sizeU) + v * (fy * sizeV));
  }

  auto w = std::make_unique<LC_Wipeout>(
      m_currentContainer, LC_WipeoutData(std::move(wcsVerts)));
  setEntityAttributes(w.get(), data);
  m_currentContainer->appendEntity(w.release());
}

/**
 * Build an LC_MLeader from a parsed DRW_MLeader (ODA spec §20.4.48).
 *
 * MLEADER carries a multi-root callout structure plus either text or block
 * content.  This conversion captures the geometric structure (roots →
 * leader lines → points) and content reference.  Style-handle resolution
 * (against LC_MLeaderStyleList) is still metadata-only, but we preserve the
 * DWG handle references needed for native re-export.
 */
void RS_FilterDXFRW::addMLeader(const DRW_MLeader *data) {
  RS_DEBUG->print("RS_FilterDXFRW::addMLeader");
  if (data == nullptr)
    return;
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addMLeader(*data);
  }

  LC_MLeaderData md;
  md.roots.reserve(data->context.roots.size());
  for (const auto &r : data->context.roots) {
    LC_MLeaderRoot root;
    root.connectionPoint = RS_Vector(r.connectionPoint.x, r.connectionPoint.y);
    root.direction = RS_Vector(r.direction.x, r.direction.y);
    root.landingDistance = r.landingDistance;
    root.attachmentDirection = r.attachmentDirection;
    root.leaderLines.reserve(r.leaderLines.size());
    for (const auto &ll : r.leaderLines) {
      LC_MLeaderLine line;
      line.leaderLineIndex = ll.leaderLineIndex;
      line.points.reserve(ll.points.size());
      for (const auto &p : ll.points) {
        line.points.emplace_back(p.x, p.y);
      }
      root.leaderLines.push_back(std::move(line));
    }
    md.roots.push_back(std::move(root));
  }

  md.contentBasePoint = RS_Vector(data->context.contentBasePoint.x,
                                  data->context.contentBasePoint.y);
  md.basePoint =
      RS_Vector(data->context.basePoint.x, data->context.basePoint.y);

  md.hasTextContents = data->context.hasTextContents;
  md.hasBlockContents =
      !data->context.hasTextContents && data->context.hasContentsBlock;
  if (md.hasTextContents) {
    md.textLabel =
        toNativeString(QString::fromUtf8(data->context.textLabel.c_str()));
    md.textLocation =
        RS_Vector(data->context.textLocation.x, data->context.textLocation.y);
    md.textHeight = data->context.textHeight;
    md.textRotation = data->context.textRotation;
    md.boundaryWidth = data->context.boundaryWidth;
    md.boundaryHeight = data->context.boundaryHeight;
    md.textColor = data->context.textColor;
  }
  if (md.hasBlockContents) {
    md.blockLocation =
        RS_Vector(data->context.blockLocation.x, data->context.blockLocation.y);
    md.blockScale =
        RS_Vector(data->context.blockScale.x, data->context.blockScale.y,
                  data->context.blockScale.z);
    md.blockRotation = data->context.blockRotation;
    // blockName resolved from blockTableRecordHandle when block lookup
    // is wired via the document — Phase 7 follow-up.
  }

  md.leaderType = data->leaderType;
  md.leaderColor = data->leaderColor;
  md.landingDistance = data->landingDistance;
  md.arrowSize = data->defaultArrowHeadSize;
  md.landingEnabled = data->landingEnabled;
  md.doglegEnabled = data->doglegEnabled;
  md.contentType = data->styleContentType;
  md.scaleFactor = data->scaleFactor;
  md.dwgStyleHandle = data->styleHandle.ref;
  md.dwgLeaderLineTypeHandle = data->leaderLineTypeHandle.ref;
  md.dwgArrowHeadHandle = data->arrowHeadHandle.ref;
  md.dwgTextStyleHandle = data->styleTextStyleHandle.ref != 0
                              ? data->styleTextStyleHandle.ref
                              : data->context.textStyleHandle.ref;
  md.dwgBlockHandle = data->styleBlockHandle.ref != 0
                          ? data->styleBlockHandle.ref
                          : data->context.blockTableRecordHandle.ref;

  auto m = std::make_unique<LC_MLeader>(m_currentContainer, std::move(md));
  setEntityAttributes(m.get(), data);
  m_currentContainer->appendEntity(m.release());
}

/**
 * MLEADERSTYLE dictionary entry capture. Styles are kept in the DWG advanced
 * metadata store for future style resolution and raw/native round-trip work;
 * the current LC_MLeader entity still copies effective scalar values from the
 * entity itself.
 */
void RS_FilterDXFRW::addMLeaderStyle(const DRW_MLeaderStyle *data) {
  if (data == nullptr)
    return;
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addMLeaderStyle(*data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addMLeaderStyle: %s",
                  data->name.empty() ? "(unnamed)" : data->name.c_str());
}

void RS_FilterDXFRW::addDetailViewStyle(const DRW_DetailViewStyle &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addDetailViewStyle(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addDetailViewStyle: %s",
                  data.m_modelDoc.m_displayName.empty()
                      ? data.m_modelDoc.m_description.c_str()
                      : data.m_modelDoc.m_displayName.c_str());
}

void RS_FilterDXFRW::addSectionViewStyle(const DRW_SectionViewStyle &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addSectionViewStyle(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addSectionViewStyle: %s",
                  data.m_modelDoc.m_displayName.empty()
                      ? data.m_modelDoc.m_description.c_str()
                      : data.m_modelDoc.m_displayName.c_str());
}

void RS_FilterDXFRW::addBreakData(const DRW_BreakData &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addBreakData(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addBreakData: %d refs",
                  static_cast<int>(data.m_pointRefHandles.size()));
}

void RS_FilterDXFRW::addBreakPointRef(const DRW_BreakPointRef &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addBreakPointRef(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addBreakPointRef: %d",
                  static_cast<int>(data.handle));
}

void RS_FilterDXFRW::addGroup(const DRW_Group &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addGroup(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addGroup: %s (%d handles)",
                  data.m_description.empty() ? "(unnamed)" : data.m_description.c_str(),
                  static_cast<int>(data.m_entityHandles.size()));
}

void RS_FilterDXFRW::addImageDefinitionReactor(const DRW_ImageDefinitionReactor &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addImageDefinitionReactor(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addImageDefinitionReactor: class version %d",
                  static_cast<int>(data.m_classVersion));
}

void RS_FilterDXFRW::addRasterVariables(const DRW_RasterVariables &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addRasterVariables(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addRasterVariables: frame %d quality %d units %d",
                  data.m_imageFrame,
                  data.m_imageQuality,
                  data.m_units);
}

void RS_FilterDXFRW::addSpatialFilter(const DRW_SpatialFilter &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addSpatialFilter(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addSpatialFilter: %d boundary points",
                  static_cast<int>(data.m_boundaryPoints.size()));
}

void RS_FilterDXFRW::addGeoData(const DRW_GeoData &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addGeoData(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addGeoData: version %d",
                  static_cast<int>(data.m_version));
}

void RS_FilterDXFRW::addTableGeometry(const DRW_TableGeometry &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addTableGeometry(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addTableGeometry: %d x %d",
                  static_cast<int>(data.m_rowCount),
                  static_cast<int>(data.m_columnCount));
}

void RS_FilterDXFRW::addTableStyle(const DRW_TableStyle &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addTableStyle(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addTableStyle: %s",
                  data.m_name.empty() ? "(unnamed)" : data.m_name.c_str());
}

void RS_FilterDXFRW::addTableContent(const DRW_TableContentObject &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addTableContent(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addTableContent: %d x %d",
                  static_cast<int>(data.m_content.m_rows.size()),
                  static_cast<int>(data.m_content.m_columns.size()));
}

void RS_FilterDXFRW::addCellStyleMap(const DRW_CellStyleMap &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addCellStyleMap(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addCellStyleMap: %d styles",
                  static_cast<int>(data.m_cellStyles.size()));
}

void RS_FilterDXFRW::addUnsupportedObject(const DRW_UnsupportedObject &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addUnsupportedObject(data);
  }
  m_unsupportedDwgObjects.push_back(data);
  RS_DEBUG->print("RS_FilterDXFRW::addUnsupportedObject: %s handle %d (%d bytes)",
                  data.m_recordName.empty() ? "(fixed)" : data.m_recordName.c_str(),
                  static_cast<int>(data.m_handle),
                  static_cast<int>(data.m_rawBytes.size()));
}

void RS_FilterDXFRW::addAcDbPlaceholder(const DRW_AcDbPlaceholder &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addAcDbPlaceholder(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addAcDbPlaceholder: %d",
                  static_cast<int>(data.handle));
}

void RS_FilterDXFRW::addSun(const DRW_Sun &data) {
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addSun(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addSun: handle %d on=%d",
                  static_cast<int>(data.handle),
                  data.m_isOn ? 1 : 0);
}

void RS_FilterDXFRW::addAssociativeObject(const DRW_AssociativeObject &data) {
  // TODO: Reconstruct associative dimension/dynamic-block relationship graphs
  // from these shell objects after native consumers exist.
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addAssociativeObject(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addAssociativeObject: %s handle %d",
                  data.m_recordName.empty() ? "(assoc)" : data.m_recordName.c_str(),
                  static_cast<int>(data.handle));
}

void RS_FilterDXFRW::addAcShHistoryObject(const DRW_AcShHistoryObject &data) {
  // TODO: Connect ACSH history nodes to 3DSOLID modeler geometry history.
  if (m_graphic != nullptr) {
    m_graphic->dwgAdvancedMetadata().addAcShObject(data);
  }
  RS_DEBUG->print("RS_FilterDXFRW::addAcShHistoryObject: %s handle %d",
                  data.m_recordName.empty() ? "(acsh)" : data.m_recordName.c_str(),
                  static_cast<int>(data.handle));
}

/**
 * Implementation of the method which links image entities to image files.
 */
void RS_FilterDXFRW::linkImage(const DRW_ImageDef *data) {
    RS_DEBUG->print("RS_FilterDXFRW::linkImage");
    if (m_graphic != nullptr && data != nullptr)
        m_graphic->dwgAdvancedMetadata().addImageDefinition(*data);

    int handle = data->handle;
    QString sfile(QString::fromUtf8(data->name.c_str()));
    QFileInfo fiDxf(m_file);
    QFileInfo fiBitmap(sfile);

    // try to find the image file:

    // first: absolute path:
    if (!fiBitmap.exists()) {
        RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(sfile));
        // try relative path:
        QString f1 = fiDxf.absolutePath() + "/" + sfile;
        if (QFileInfo(f1).exists()) {
            sfile = f1;
        } else {
            RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f1));
            // try drawing path:
            QString f2 = fiDxf.absolutePath() + "/" + fiBitmap.fileName();
            if (QFileInfo(f2).exists()) {
                sfile = f2;
            } else {
                RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f2));
            }
        }
    }

    // Also link images in subcontainers (e.g. inserts):
    for(RS_Entity* e: lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (e->rtti()==RS2::EntityImage) {
            auto img = static_cast<RS_Image*>(e);
            if (img->getHandle()==handle) {
                img->setFile(sfile);
                RS_DEBUG->print("image found: %s", (const char*)QFile::encodeName(img->getFile()));
                img->update();
            }
        }
    }

    // update images in blocks:
    for (unsigned i=0; i<m_graphic->countBlocks(); ++i) {
        RS_Block* b = m_graphic->blockAt(i);
        for(RS_Entity* e: lc::LC_ContainerTraverser{*b, RS2::ResolveNone}.entities()) {
            if (e->rtti()==RS2::EntityImage) {
                auto img = static_cast<RS_Image*>(e);
                if (img->getHandle()==handle) {
                    img->setFile(sfile);
                    RS_DEBUG->print("image in block found: %s",(const char*)QFile::encodeName(img->getFile()));
                    img->update();
                }
            }
        }
    }
    RS_DEBUG->print("linking image: OK");
}

using std::map;
/**
 * Sets the header variables from the DXF file.
 */
void RS_FilterDXFRW::addHeader(const DRW_Header* data){
	RS_Graphic* container = nullptr;
    if (m_currentContainer->rtti()==RS2::EntityGraphic) {
        container = static_cast<RS_Graphic*>(m_currentContainer);
    } else {
        return;
    }

    for (auto it = data->vars.begin() ; it != data->vars.end(); ++it ) {
        QString key = QString::fromStdString((*it).first);
        DRW_Variant *var = (*it).second;
        switch (var->type()) {
        case DRW_Variant::COORD:
            container->addVariable(key,
            RS_Vector(var->content.v->x, var->content.v->y, var->content.v->z), var->code());
            break;
        case DRW_Variant::STRING:
            container->addVariable(key, QString::fromUtf8(var->content.s->c_str()), var->code());
            break;
        case DRW_Variant::INTEGER:
            container->addVariable(key, var->content.i, var->code());
            break;
        case DRW_Variant::DOUBLE:
            container->addVariable(key, var->content.d, var->code());
            break;
        default:
            break;
        }
    }

    for (auto it = data->customVars.begin() ; it != data->customVars.end(); ++it ) {
        QString key = QString::fromStdString((*it).first);
        DRW_Variant *var = (*it).second;
        container->addCustomProperty(key, var->c_str());
    }

    m_codePage = m_graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252");
    m_textStyle = m_graphic->getVariableString("$TEXTSTYLE", "Standard");
    m_dimStyle = m_graphic->getVariableString("$DIMSTYLE", "Standard");
    //initialize units vars if not are present in dxf file
    m_graphic->getVariableInt("$LUNITS", 2);
    m_graphic->getVariableInt("$LUPREC", 4);
    m_graphic->getVariableInt("$AUNITS", 0);
    m_graphic->getVariableInt("$AUPREC", 4);

	//initialize points drawing style vars if not present in dxf file
    if (m_graphic->getVariableInt("$PDMODE", -999) < 0) {
        m_graphic->addVariable("$PDMODE", LC_DEFAULTS_PDMode, DXF_FORMAT_GC_VarName);
    }
    if (m_graphic->getVariableDouble("$PDSIZE", -999.9) < -100.0) {
        m_graphic->addVariable("$PDSIZE", LC_DEFAULTS_PDSize, DXF_FORMAT_GC_VarName);
    }
    if (m_graphic->getVariableDouble("$JOINSTYLE", -999.9) < -100.0) {
        m_graphic->addVariable("$JOINSTYLE", 1, DXF_FORMAT_GC_JoinStyle);
    }
    if( m_graphic->getVariableDouble("$ENDCAPS", -999.9) < -100.0) {
        m_graphic->addVariable("$ENDCAPS", 1, DXF_FORMAT_GC_Endcaps);
    }

    QString acadver = m_versionStr = m_graphic->getVariableString("$ACADVER", "");
    acadver.replace(QRegularExpression("[a-zA-Z]"), "");
    bool ok;
    m_version=acadver.toInt(&ok);
    if (!ok) {
        m_version = 1021;
    }

    //detect if dxf lib are a old dxflib or libdxfrw < 0.5.4 (used to correct mtext alignment)
    m_oldMText = false;
    m_isLibDxfRw = false;
    m_libDxfRwVersion = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    auto option = Qt::SkipEmptyParts;
#else
    auto option = QString::SkipEmptyParts;
#endif
    QStringList commentList = QString::fromStdString( data->getComments()).split('\n', option);
    for( auto commentLine: commentList) {

        QStringList commentWords = commentLine.split(' ', option);
        if( 0 < commentWords.size()) {
            if( "dxflib" == commentWords.at(0)) {
                m_oldMText = true;
                break;
            } else if( "dxfrw" == commentWords.at(0)) {
                QStringList libVersionList = commentWords.at(1).split('.', option);
                if( 2 < libVersionList.size()) {
                    m_isLibDxfRw = true;
                    m_libDxfRwVersion = LIBDXFRW_VERSION( libVersionList.at(0).toInt(),
                                                        libVersionList.at(1).toInt(),
                                                        libVersionList.at(2).toInt() );
                    if( m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 5, 4)) {
                        m_oldMText = true;
                    }
                }
                break;
            }
        }
    }
}

/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the DXF file that will be written.
 */
bool RS_FilterDXFRW::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) {
    RS_DEBUG->print("RS_FilterDXFDW::fileExport: exporting file '%s'...",(const char*)QFile::encodeName(file));
    RS_DEBUG->print("RS_FilterDXFDW::fileExport: file type '%d'", (int)type);

    this->m_graphic = &g;

    // check if we can write to that directory:
#ifndef Q_OS_WIN

    QString path = QFileInfo(file).absolutePath();
    if (QFileInfo(path).isWritable()==false) {
        RS_DEBUG->print("RS_FilterDXFRW::fileExport: can't write file: "
                        "no permission");
        return false;
    }
    //
#endif

#ifdef DWGSUPPORT
    if (type == RS2::FormatDWG || type == RS2::FormatDWG2004) {
        DRW::Version dwgVer = (type == RS2::FormatDWG2004) ? DRW::AC1018 : DRW::AC1015;
        m_version = (dwgVer == DRW::AC1018) ? 1018 : 1015;
        m_exactColor = false;
        m_dwgW = new dwgRW(QFile::encodeName(file).constData());
        bool success = m_dwgW->write(this, dwgVer, false);
        delete m_dwgW;
        m_dwgW = nullptr;
        return success;
    }
#endif

    // set version for DXF filter:
    m_exactColor = false;
    DRW::Version exportVersion;
    if (type==RS2::FormatDXFRW12) {
        exportVersion = DRW::AC1009;
        m_version = 1009;
    } else if (type==RS2::FormatDXFRW14) {
        exportVersion = DRW::AC1014;
        m_version = 1014;
    } else if (type==RS2::FormatDXFRW2000) {
        exportVersion = DRW::AC1015;
        m_version = 1015;
    } else if (type==RS2::FormatDXFRW2004) {
        exportVersion = DRW::AC1018;
        m_version = 1018;
        m_exactColor = true;
    } else if (type==RS2::FormatDXFRW){
        exportVersion = DRW::AC1021;
        m_version = 1021;
        m_exactColor = true;
    } else {
        exportVersion = DRW::AC1032;
        m_version = 1032;
        m_exactColor = true;
    }
    /**
     * fixme - sand - files - RESTORE!!! Under win, encodeName() prevents using unicode file names!!! Due to that, blocks/files may be saved incorrectly if name is localized
     */
    m_dxfW = new dxfRW(QFile::encodeName(file));
    // fixme - sand - save to binary format enabling/disabling!!
    bool binary = false;

//    bool success = m_dxfW->write(this, exportVersion, false); //ascii
    bool success = m_dxfW->write(this, exportVersion, binary); //binary
    delete m_dxfW;

    if (!success) {
        RS_DEBUG->print("RS_FilterDXFDW::fileExport: can't write file");
        return false;
    }
/*RLZ pte*/
/*    RS_DEBUG->print("writing tables...");
    dw->sectionTables();
    // VPORT:
    dxf.writeVPort(*dw);
    dw->tableEnd();

    // VIEW:
    RS_DEBUG->print("writing views...");
    dxf.writeView(*dw);

    // UCS:
    RS_DEBUG->print("writing ucs...");
    dxf.writeUcs(*dw);

    // Appid:
    RS_DEBUG->print("writing appid...");
    dw->tableAppid(1);
    writeAppid(*dw, "ACAD");
    dw->tableEnd();
*/
    return success;
}

/**
 * Prepare unnamed blocks.
 */
void RS_FilterDXFRW::prepareBlocks() {
    RS_Block *blk;
    int dimNum = 0, hatchNum= 0;
    QString prefix, sufix;

    //check for existing *D?? or  *U??
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        prefix = blk->getName().left(2).toUpper();
        sufix = blk->getName().mid(2);
        if (prefix == "*D") {
            if (sufix.toInt() > dimNum) dimNum = sufix.toInt();
        } else if (prefix == "*U") {
            if (sufix.toInt() > hatchNum) hatchNum = sufix.toInt();
        }
    }
    //Add a name to each dimension, in dxfR12 also for hatches
    for(RS_Entity* e: lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (!(e->getFlag(RS2::FlagUndone)) ) {
            switch (e->rtti()) {
            case RS2::EntityDimLinear:
            case RS2::EntityDimOrdinate:
            case RS2::EntityDimAligned:
            case RS2::EntityDimAngular:
            case RS2::EntityDimRadial:
            case RS2::EntityDimDiametric:
            case RS2::EntityDimLeader:
                prefix = "*D" + QString::number(++dimNum);
                m_noNameBlock[e] = prefix;
                break;
            case RS2::EntityHatch:
                if (m_version==1009) {
                    if ( !static_cast<RS_Hatch*>(e)->isSolid() ) {
                        prefix = "*U" + QString::number(++hatchNum);
                        m_noNameBlock[e] = prefix;
                    }
                }
                break;
            default:
                break;
            }//end switch
        }//end if !RS2::FlagUndone
    }
}

/**
 * Writes block records (just the name, not the entities in it).
 */
void RS_FilterDXFRW::writeBlockRecords(){
    //first prepare and send unnamed blocks, the while loop can be omitted for R12
    prepareBlocks();
    QHash<RS_Entity*, QString>::const_iterator it = m_noNameBlock.constBegin();
    while (it != m_noNameBlock.constEnd()) {
        m_dxfW->writeBlockRecord(it.value().toStdString());
        ++it;
    }

    //next send "normal" blocks
    RS_Block *blk;
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        if (!blk->isUndone()){
            RS_DEBUG->print("writing block record: %s", (const char*)blk->getName().toLocal8Bit());
            m_dxfW->writeBlockRecord(blk->getName().toUtf8().data(),
                                     blk->getInsertionUnits());
        }
    }
}

/**
 * Writes blocks.
 */
void RS_FilterDXFRW::writeBlocks() {
#ifdef DWGSUPPORT
    if (m_dwgW) {
        // Register each user block so INSERT entities can reference them by name.
        for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
            RS_Block *blk = m_graphic->blockAt(i);
            if (!blk->isUndone()) {
                DRW_Coord bp{blk->getBasePoint().x, blk->getBasePoint().y,
                             blk->getBasePoint().z};
                m_dwgW->defineBlock(blk->getName().toUtf8().constData(), bp,
                                    blk->getInsertionUnits());
            }
        }
        return;
    }
#endif

    RS_Block *blk;

    //write unnamed blocks
    QHash<RS_Entity*, QString>::const_iterator it = m_noNameBlock.constBegin();
    while (it != m_noNameBlock.constEnd()) {
        DRW_Block block;
        block.name = it.value().toStdString();
        block.basePoint.x = 0.0;
        block.basePoint.y = 0.0;
        block.basePoint.z = 0.0;
        block.flags = 1;//flag for unnamed block
        m_dxfW->writeBlock(&block);
        RS_EntityContainer *ct = (RS_EntityContainer *)it.key();
        for(RS_Entity* e: lc::LC_ContainerTraverser{*ct, RS2::ResolveNone}.entities()) {
            if ( !(e->getFlag(RS2::FlagUndone)) ) {
                writeEntity(e);
            }
        }
        ++it;
    }

    //next write "normal" blocks
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        if (!blk->isUndone()) {
            RS_DEBUG->print("writing block: %s", (const char*)blk->getName().toLocal8Bit());

            DRW_Block block;
            block.name = blk->getName().toUtf8().data();
            block.basePoint.x = blk->getBasePoint().x;
            block.basePoint.y = blk->getBasePoint().y;
            block.basePoint.z = blk->getBasePoint().z;
            m_dxfW->writeBlock(&block);
            for(RS_Entity* e: lc::LC_ContainerTraverser{*blk, RS2::ResolveNone}.entities()) {
                if ( !(e->getFlag(RS2::FlagUndone)) ) {
                    writeEntity(e);
                }
            }
        }
    }
}

void RS_FilterDXFRW::writeHeader(DRW_Header& data){
    RS_Vector v;
/*TODO $ISOMETRICGRID == $SNAPSTYLE and "GRID on/off" not handled because is part of
 active vport to save is required read/write VPORT table */
    QHash<QString, RS_Variable>vars = m_graphic->getVariableDict();
    if (!vars.contains ( "$DWGCODEPAGE" )) {
//RLZ: TODO verify this
        m_codePage = RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit());
//        RS_Variable v( QString(RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit())),0 );
        vars.insert(QString("$DWGCODEPAGE"), RS_Variable(m_codePage, 0) );
    }

    QHash<QString, RS_Variable>::iterator it = vars.begin();
    while (it != vars.end()) {
        auto value = it.value();
        int code = value.getCode();
        auto key = it.key().toStdString();
        switch (value.getType()) {
            case RS2::VariableInt:
                data.addInt(key, value.getInt(), code);
                break;
            case RS2::VariableDouble:
                data.addDouble(key, value.getDouble(), code);
                break;
            case RS2::VariableString:
                data.addStr(key, value.getString().toUtf8().data(), code);
                break;
            case RS2::VariableVector:
                v = value.getVector();
                data.addCoord(key, DRW_Coord(v.x, v.y, v.z), code);
                break;
            default:
                break;
        }
        ++it;
    }
    v = m_graphic->getMin();
    v = m_graphic->getMax();
    data.addCoord("$EXTMIN", DRW_Coord(v.x, v.y, 0.0), 0);
    data.addCoord("$EXTMAX", DRW_Coord(v.x, v.y, 0.0), 0);

    //when saving a block, there is no active layer. ignore it to avoid crash
    if(m_graphic->getActiveLayer()==0) {
        return;
    }
    data.addStr("$CLAYER", (m_graphic->getActiveLayer()->getName()).toUtf8().data(), 8);

    QHash<QString, RS_Variable> customVars = m_graphic->getCustomProperties();

    QHashIterator<QString,RS_Variable> customVar(customVars);
    while (customVar.hasNext()) {
        customVar.next();
        auto val = customVar.value().getString();
        if (!val.isEmpty()) {
            auto key = customVar.key().toStdString();
            data.customVars[key] = new DRW_Variant(1, val.toStdString());
        }
    }
}

void RS_FilterDXFRW::writeDwgClasses() {
#ifdef DWGSUPPORT
    if (m_dwgW == nullptr || m_graphic == nullptr)
        return;
    const auto& metadata = m_graphic->dwgAdvancedMetadata();
    const bool canWriteModernObjects = m_dwgW->getVersion() >= DRW::AC1021;
    std::set<duint32> nativeSunHandles;
    std::set<duint32> nativeMLeaderStyleHandles;
    if (canWriteModernObjects) {
        for (const auto& record : metadata.suns()) {
            if (record.replayState != LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                || record.handle == 0) {
                continue;
            }
            DRW_Sun sun = sunFromMetadata(record);
            if (m_dwgW->registerSunObjectClass(&sun))
                nativeSunHandles.insert(record.handle);
        }
        for (const auto& record : metadata.mleaderStyles()) {
            if (record.replayState != LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                || record.handle == 0
                || hasReplayableRawMLeaderStyle(metadata, record.handle)) {
                continue;
            }
            DRW_MLeaderStyle style = mleaderStyleFromMetadata(record);
            if (m_dwgW->registerMLeaderStyleObjectClass(&style))
                nativeMLeaderStyleHandles.insert(record.handle);
        }
    }

    for (const auto& record : metadata.rawObjects()) {
        if (record.replayState != LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
            || !record.isCustomClass) {
            continue;
        }
        if (nativeSunHandles.count(record.handle) != 0 && isSunRawObject(record))
            continue;
        if (nativeMLeaderStyleHandles.count(record.handle) != 0
            && isMLeaderStyleRawObject(record))
            continue;
        DRW_UnsupportedObject object = rawObjectFromMetadata(record);
        m_dwgW->registerRawDwgObjectClass(&object);
    }
#endif
}

void RS_FilterDXFRW::writeLType(const UTF8STRING& lTypeName, const UTF8STRING& ltDescription, int ltSize,
                                double ltLength, const std::vector<double>& ltPath) {
    DRW_LType ltype;
    ltype.updateValues(lTypeName, ltDescription, ltSize, ltLength, ltPath);
    if (m_dwgW) {
        m_dwgW->addLType(&ltype);
        return;
    }
    m_dxfW->writeLineType(&ltype);
}

void RS_FilterDXFRW::writeLTypes(){
    writeLType("CONTINUOUS", "Solid line", 0, 0, {});
    writeLType("ByLayer", "", 0, 0, {});
    writeLType("ByBlock", "", 0, 0, {});
    writeLType("DOT", "Dot . . . . . . . . . . . . . . . . . . . . . .",
                       2, 6.35, {0.0, -6.35});
    writeLType("DOTTINY", "Dot (.15x) .....................................",
                       2, 0.9525, {0.0, -0.9525});
    writeLType("DOT2", "Dot (.5x) .....................................",
                           2, 3.175, {0.0, -3.175});
    writeLType("DOTX2", "Dot (2x) .  .  .  .  .  .  .  .  .  .  .  .  .",
                          2, 12.7, {0.0, -12.7});
    writeLType("DASHED", "Dashed _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                         2, 19.05, {12.7, -6.35});
    writeLType("DASHEDTINY", "Dashed (.15x) _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                        2, 2.8575, {1.905, -0.9525});
    writeLType("DASHED2", "Dashed (.5x) _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                      2, 9.525, {6.35, -3.175});
    writeLType("DASHEDX2", "Dashed (2x) ____  ____  ____  ____  ____  ___",
                      2, 38.1, {25.4, -12.7});
    writeLType("DASHDOT", "Dash dot __ . __ . __ . __ . __ . __ . __ . __",
                     4, 25.4, {12.7, -6.35, 0.0, -6.35});
    writeLType("DASHDOTTINY", "Dash dot (.15x) _._._._._._._._._._._._._._._.",
                    4, 3.81, {1.905, -0.9525, 0.0, -0.9525});
    writeLType("DASHDOT2", "Dash dot (.5x) _._._._._._._._._._._._._._._.",
                    4, 12.7, {6.35, -3.175, 0.0, -3.175});
    writeLType("DASHDOTX2", "Dash dot (2x) ____  .  ____  .  ____  .  ___",
                    4, 50.8, {25.4, -12.7, 0.0, -12.7});
    writeLType("DIVIDE", "Divide ____ . . ____ . . ____ . . ____ . . ____",
                    6, 31.75, {12.7, -6.35, 0.0, -6.35, 0.0, -6.35});
    writeLType("DIVIDETINY", "Divide (.15x) __..__..__..__..__..__..__..__.._",
                    6, 4.7625, {1.905, -0.9525, 0.0, -0.9525, 0.0, -0.9525});
    writeLType("DIVIDE2", "Divide (.5x) __..__..__..__..__..__..__..__.._",
                   6, 15.875, {6.35, -3.175, 0.0, -3.175, 0.0, -3.175});
    writeLType("DIVIDEX2", "Divide (2x) ________  .  .  ________  .  .  _",
                   6, 63.5, {25.4, -12.7, 0.0, -12.7, 0.0, -12.7});
    writeLType("BORDER", "Border __ __ . __ __ . __ __ . __ __ . __ __ .",
                   6, 44.45, {12.7, -6.35, 12.7, -6.35, 0.0, -6.35});
    writeLType("BORDERTINY", "Border (.15x) __.__.__.__.__.__.__.__.__.__.__.",
               6, 6.6675, {1.905, -0.9525, 1.905, -0.9525, 0.0, -0.9525});
    writeLType("BORDER2", "Border (.5x) __.__.__.__.__.__.__.__.__.__.__.",
               6, 22.225, {6.35, -3.175, 6.35, -3.175, 0.0, -3.175});
    writeLType("BORDERX2", "Border (2x) ____  ____  .  ____  ____  .  ___",
              6, 88.9, {25.4, -12.7, 25.4, -12.7, 0.0, -12.7});
    writeLType("CENTER", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____",
              4, 50.8, {31.75, -6.35, 6.35, -6.35});
    writeLType("CENTERTINY", "Center (.15x) ___ _ ___ _ ___ _ ___ _ ___ _ ___",
                4, 7.62, {4.7625, -0.9525, 0.9525, -0.9525});
    writeLType("CENTER2", "Center (.5x) ___ _ ___ _ ___ _ ___ _ ___ _ ___",
                4, 28.575, {19.05, -3.175, 3.175, -3.175});
    writeLType("CENTERX2", "Center (2x) ________  __  ________  __  _____",
               4, 101.6, {63.5, -12.7, 12.7, -12.7});
}

void RS_FilterDXFRW::writeLayers(){
    DRW_Layer lay;
    RS_LayerList* ll = m_graphic->getLayerList();
    int exact_rgb;
    for (unsigned int i = 0; i < ll->count(); i++) {
        lay.reset();
        RS_Layer* l = ll->at(i);
        RS_Pen pen = l->getPen();
        lay.name = l->getName().toUtf8().data();
        lay.color = colorToNumber(pen.getColor(), &exact_rgb);
        lay.color24 = exact_rgb;
        lay.lWeight = widthToNumber(pen.getWidth());
        lay.lineType = lineTypeToName(pen.getLineType()).toStdString();
        lay.flags = l->isFrozen() ? 0x01 : 0x00;
        if (l->isLocked()) {
            lay.flags |=0x04;
        }
        lay.plotF = l->isPrint();
        if( l->isConstruction()) {
            lay.extData.push_back(new DRW_Variant(1001, "LibreCad"));
            lay.extData.push_back(new DRW_Variant(1070, 1));
            // RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::writeLayers: layer %s saved as construction layer", lay.name.c_str());
        }
        if (m_dwgW) {
            m_dwgW->addLayer(&lay);
        }
        else {
            m_dxfW->writeLayer(&lay);
        }
    }
}

void RS_FilterDXFRW::writeUCSs() {
    LC_UCSList* vl = m_graphic->getUCSList();
    DRW_UCS ucs;
    for (unsigned int i = 1; i < vl->count(); i++) {
        ucs.reset();

        LC_UCS* u = vl->at(i);
        if (u->isTemporary()){ // temporary ucs without name are not persistent
            continue;
        }
        ucs.name = u->getName().toUtf8().data();
        ucs.origin.x = u->getOrigin().x;
        ucs.origin.y = u->getOrigin().y;
        ucs.origin.z = u->getOrigin().z;

        ucs.xAxisDirection.x = u->getXAxis().x;
        ucs.xAxisDirection.y = u->getXAxis().y;
        ucs.xAxisDirection.z = u->getXAxis().z;

        ucs.yAxisDirection.x = u->getYAxis().x;
        ucs.yAxisDirection.y = u->getYAxis().y;
        ucs.yAxisDirection.z = u->getYAxis().z;

        ucs.orthoOrigin.x = u->getOrthoOrigin().x;
        ucs.orthoOrigin.y = u->getOrthoOrigin().y;
        ucs.orthoOrigin.z = u->getOrthoOrigin().z;

        ucs.orthoType = u->getOrthoType();
        ucs.elevation = u->getElevation();

        m_dxfW->writeUCS(&ucs);
    }
}

void RS_FilterDXFRW::writeViews() {
    LC_ViewList* vl = m_graphic->getViewList();
    DRW_View vie;
    for (unsigned int i = 0; i < vl->count(); i++) {
        vie.reset();
        LC_View* view = vl->at(i);
        vie.name = view->getName().toUtf8().data();
        vie.center.x = view->getCenter().x;
        vie.center.y = view->getCenter().y;
        vie.center.z = view->getCenter().z;

        vie.targetPoint.x = view->getTargetPoint().x;
        vie.targetPoint.y = view->getTargetPoint().y;
        vie.targetPoint.z = view->getTargetPoint().z;

        vie.size.x = view->getSize().x;
        vie.size.y = view->getSize().y;
        vie.size.z = view->getSize().z;

        vie.frontClippingPlaneOffset = view->getFrontClippingPlaneOffset();
        vie.backClippingPlaneOffset = view->getBackClippingPlaneOffset();
        vie.lensLen = view->getLensLen();
        vie.flags = view->getFlags();
        vie.viewMode = view->getViewMode();

        vie.viewDirectionFromTarget.x = view->getViewDirection().x;
        vie.viewDirectionFromTarget.y = view->getViewDirection().y;
        vie.viewDirectionFromTarget.z = view->getViewDirection().z;

        vie.cameraPlottable = view->isCameraPlottable();
        vie.renderMode = view->getRenderMode();

        vie.twistAngle = view->getTwistAngle();

        if (view->isHasUCS()){
            vie.hasUCS = true;
            LC_UCS *ucs = view->getUCS();
            vie.ucsOrigin.x = ucs->getOrigin().x;
            vie.ucsOrigin.y = ucs->getOrigin().y;
            vie.ucsOrigin.z = ucs->getOrigin().z;

            vie.ucsOrthoType = ucs->getOrthoType();
            vie.ucsElevation = ucs->getElevation();

            vie.ucsXAxis.x = ucs->getXAxis().x;
            vie.ucsXAxis.y = ucs->getXAxis().y;
            vie.ucsXAxis.z = ucs->getXAxis().z;

            vie.ucsYAxis.x = ucs->getYAxis().x;
            vie.ucsYAxis.y = ucs->getYAxis().y;
            vie.ucsYAxis.z = ucs->getYAxis().z;

            // fixme - complete - base UCS_ID and Named UCS_ID support. That's might be necessary to support views/UCS
            // created outside of LibreCAD.
            // Return to this after normal support of UCS.
//            vie.namedUCS_ID = ucs.
//            vie.baseUCS_ID = ucs.
        }
        if (const auto* viewMetadata =
                m_graphic->dwgAdvancedMetadata().findViewByName(vie.name)) {
            vie.namedUCS_ID = viewMetadata->namedUcsHandle;
            vie.baseUCS_ID = viewMetadata->baseUcsHandle;
            vie.m_useDefaultLights = viewMetadata->useDefaultLights;
            vie.m_defaultLightingType = viewMetadata->defaultLightingType;
            vie.m_brightness = viewMetadata->brightness;
            vie.m_contrast = viewMetadata->contrast;
            vie.m_ambientColor = viewMetadata->ambientColor;
            vie.m_backgroundHandle = viewMetadata->backgroundHandle;
            vie.m_visualStyleHandle = viewMetadata->visualStyleHandle;
            vie.m_sunHandle = viewMetadata->sunHandle;
            vie.m_liveSectionHandle = viewMetadata->liveSectionHandle;
        }
        if (m_dwgW != nullptr) {
            m_dwgW->addView(&vie);
        } else {
            m_dxfW->writeView(&vie);
        }
    }
}

void RS_FilterDXFRW::writeTextstyles(){
    QHash<QString, QString> styles;
    QString sty;
    //Find fonts used by text entities in drawing
    for (RS_Entity* e : lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (!e->isUndone()) {
            auto rtti = e->rtti();
            switch (rtti) {
                case RS2::EntityMText:
                    sty = static_cast<RS_MText*>(e)->getStyle();
                    break;
                case RS2::EntityText:
                    sty = static_cast<RS_Text*>(e)->getStyle();
                    break;
                default:
                    if (RS2::isDimensionalEntity(rtti)) {
                        auto dim = dynamic_cast<RS_Dimension*>(e);
                        if (dim != nullptr) {
                            auto styleOverride = dim->getDimStyleOverride();
                            if (styleOverride != nullptr) {
                                auto dimTextStyle = styleOverride->text()->style();
                                sty = dimTextStyle;
                            }
                        }
                    }
                    else {
                        sty.clear();
                    }
                    break;
            }
            if (!sty.isEmpty() && !styles.contains(sty)) {
                styles.insert(sty, sty);
            }
        }
    }
    //Find fonts used by text entities in blocks
    RS_Block *blk;
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        for(RS_Entity* e: lc::LC_ContainerTraverser{*blk, RS2::ResolveNone}.entities()) {
            if (!e->isUndone()) {
                RS2::EntityType rtti = e->rtti();
                switch (rtti) {
                    case RS2::EntityMText:
                        sty = static_cast<RS_MText*>(e)->getStyle();
                        break;
                    case RS2::EntityText:
                        sty = static_cast<RS_Text*>(e)->getStyle();
                        break;
                    default:
                        if (RS2::isDimensionalEntity(rtti)) {
                            auto dim = dynamic_cast<RS_Dimension*>(e);
                            if (dim != nullptr) {
                                auto styleOverride = dim->getDimStyleOverride();
                                if (styleOverride != nullptr) {
                                    auto dimTextStyle = styleOverride->text()->style();
                                    sty = dimTextStyle;
                                }
                            }
                        }
                        else {
                            sty.clear();
                        }

                        break;
                }
                if (!sty.isEmpty() && !styles.contains(sty)) {
                    styles.insert(sty, sty);
                }
            }
        }
    }

    auto dimStyleList = m_graphic->getDimStyleList();

    for (const auto ds: *dimStyleList->getStylesList()) {
       sty = ds->text()->style();
        if (!sty.isEmpty() && !styles.contains(sty)) {
            styles.insert(sty, sty);
        }
    }

    DRW_Textstyle ts;
    QHash<QString, QString>::const_iterator it = styles.constBegin();
     while (it != styles.constEnd()) {
         ts.name = (it.key()).toStdString();
         ts.font = it.value().toStdString();
//         ts.flags;
         if (m_dwgW) {
             m_dwgW->addTextstyle(&ts);
         }
         else {
             m_dxfW->writeTextstyle(&ts);
         }
         ++it;
     }
}

void RS_FilterDXFRW::writeVports(){
    DRW_Vport vp;
    vp.name = "*Active";
    m_graphic->isGridOn()? vp.grid = 1 : vp.grid = 0;
    RS_Vector spacing = m_graphic->getVariableVector("$GRIDUNIT",RS_Vector(0.0,0.0));
    vp.gridBehavior = 3;
    vp.gridSpacing.x = spacing.x;
    vp.gridSpacing.y = spacing.y;
    vp.snapStyle = m_graphic->isIsometricGrid();
    vp.snapIsopair = m_graphic->getIsoView();
    if (vp.snapIsopair > 2) {
        vp.snapIsopair = 0;
    }
    if (fabs(spacing.x) < 1.0e-6) {
        vp.gridBehavior = 7; //auto
        vp.gridSpacing.x = 10;
    }
    if (fabs(spacing.y) < 1.0e-6) {
        vp.gridBehavior = 7; //auto
        vp.gridSpacing.y = 10;
    }
    RS_GraphicView *gv = m_graphic->getGraphicView();
    if (gv) {
        LC_GraphicViewport *viewport = gv->getViewPort();
        RS_Vector fac = viewport->getFactor();
        vp.height = gv->getHeight() / fac.y;
        vp.ratio = (double) gv->getWidth() / (double) gv->getHeight();
        vp.center.x = (gv->getWidth() - viewport->getOffsetX()) / (fac.x * 2.0);
        vp.center.y = (gv->getHeight() - viewport->getOffsetY()) / (fac.y * 2.0);
    }
    if (m_dwgW) {
        m_dwgW->addVport(&vp);
        return;
    }
    m_dxfW->writeVport(&vp);
}

void RS_FilterDXFRW::writeDimstyles(){
    LC_DimStylesList* dimStylesList = m_graphic->getDimStyleList();
    auto stylesList = dimStylesList->getStylesList();
    for (auto ds: *stylesList) {
        DRW_Dimstyle dst;
        prepareDRWDimStyle(dst, ds);
        if (m_dwgW) {
            m_dwgW->addDimstyle(&dst);
        }
        else {
            m_dxfW->writeDimstyle(&dst);
        }
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleZerosSuppression(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto zeros = ds->zerosSuppression();
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMZIN)) {
        d.add("$DIMZIN", 78, zeros->linearRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMAZIN)) {
        d.add("$DIMAZIN", 79, zeros->angularRaw());
    }

    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMTZIN)) {
        d.add("$DIMTZIN", 284, zeros->toleranceRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTZ)) {
        d.add("$DIMALTZ", 285, zeros->altLinearRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTTZ)) {
        d.add("$DIMALTTZ", 286, zeros->altToleranceRaw());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleArrows(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto arrow = ds->arrowhead();
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) {
        QString blockName = arrow->sameBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                d.add("_$DIMBLK", 342, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK", 5, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) {
        QString blockName = arrow->arrowHeadBlockNameFirst();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                d.add("_$DIMBLK1", 343, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK1", 6, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) {
        QString blockName = arrow->arrowHeadBlockNameSecond();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                d.add("_$DIMBLK2", 344, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK2", 7, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMASZ)) {
        d.add("$DIMASZ", 41, arrow->size());
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMTSZ)) {
        d.add("$DIMTSZ", 142, arrow->tickSize());
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH)) {
        d.add("$DIMSAH", 173, arrow->isUseSeparateArrowHeads()? 1 : 0);
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMSOXD)) {
        d.add("$DIMSOXD", 175, arrow->suppression());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleScaling(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto scale = ds->scaling();
    if (scale->checkModifyState(LC_DimStyle::Scaling::$DIMSCALE)) {
        d.add("$DIMSCALE", 40, scale->scale());
    }
    if (scale->checkModifyState(LC_DimStyle::Scaling::$DIMLFAC)) {
        d.add("$DIMLFAC", 144, scale->linearFactor());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleExtLine(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto extLine = ds->extensionLine();
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXO)) {
        d.add("$DIMEXO", 42, extLine->distanceFromOriginPoint());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXE)) {
        d.add("$DIMEXE", 44, extLine->distanceBeyondDimLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXL)) {
        d.add("$DIMFXL", 49, extLine->fixedLength());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXLON)) {
        d.add("$DIMFXLON", 290, extLine->hasFixedLength() ? 1: 0);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLWE)) {
        auto lineWidth = extLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        d.add("$DIMLWE", 372, lw);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMCLRE)) {
        auto lineColor = extLine->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRE", 177, colNum);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE1)) {
        d.add("$DIMSE1", 75, extLine->suppressFirstLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE2)) {
        d.add("$DIMSE2", 76, extLine->suppressSecondLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX1)) {
        int lineTypeHandle = findLineTypeHandleToWrite(extLine->lineTypeFirstRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTEX1", 347, handleStr.toStdString());
        }
        // auto lineType = extLine->lineTypeFirstRaw().toStdString();
        // d.add("$DIMLTEX1", 347, lineType);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX2)) {
        int lineTypeHandle = findLineTypeHandleToWrite(extLine->lineTypeSecondRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTEX2", 348, handleStr.toStdString());
        }
        // auto lineType = extLine->lineTypeFirstRaw().toStdString();
        // d.add("$DIMLTEX2", 348, lineType);
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleDimLine(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto dimLine = ds->dimensionLine();
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLWD)) {
        auto lineWidth = dimLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        d.add("$DIMLWD", 371, lw);
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLE)) {
        d.add("$DIMDLE", 46, dimLine->distanceBeyondExtLinesForObliqueStroke());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLI)) {
        d.add("$DIMDLI", 43, dimLine->baseLineDimLinesSpacing());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMGAP)) {
        d.add("$DIMGAP", 147, dimLine->lineGap());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMCLRD)) {
        auto lineColor = dimLine->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRD", 176, colNum);
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD1)) {
        d.add("$DIMSD1", 281, dimLine->suppressFirstLine());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD2)) {
        d.add("$DIMSD2", 281, dimLine->suppressSecondLine());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMTOFL)) {
        d.add("$DIMTOFL", 172, dimLine->drawPolicyForOutsideText());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLTYPE)) {
        // auto value = dimLine->lineType();
        // auto lineType = dimLine->lineTypeName().toStdString();
        int lineTypeHandle = findLineTypeHandleToWrite(dimLine->lineTypeName());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTYPE", 345, handleStr.toStdString());
        }
    }
}

int RS_FilterDXFRW::findLineTypeHandleToWrite(const QString& name) const {
    if (m_dxfW == nullptr) {
        return -1;
    }
    std::string lineName = name.toUpper().toStdString();
    for (auto p: m_dxfW->getWritingContext()->lineTypesMap) {
        if (p.first.compare(lineName) == 0) {
            return p.second;
        }
    }
    return -1;
}

void RS_FilterDXFRW::prepareDRWDimStyleText(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto text = ds->text();
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXT)) {
        d.add("$DIMTXT", 140, text->height());
    }

    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXSTY)) {
        auto styleName = text->style().toStdString();
        int styleHandle = m_dxfW != nullptr ? m_dxfW->getTextStyleHandle(styleName) : -1;
        if(styleHandle > 0) {
            d.add("$DIMTXSTY", 340, toHexStr(styleHandle).toStdString());
        }
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTOH)) {
        d.add("$DIMTOH", 74, text->orientationOutside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIH)) {
        d.add("$DIMTIH", 73, text->orientationInside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMJUST)) {
        d.add("$DIMJUST", 280, text->horizontalPositioning());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMCLRT)) {
        auto lineColor = text->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRT", 178, colNum);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTAD)) {
        d.add("$DIMTAD", 77, text->verticalPositioning());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIX)) {
        d.add("$DIMTIX", 174, text->extLinesRelativePlacement());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILL)) {
        d.add("$DIMTFILL", 69, text->backgroundFillMode());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILLCLR)) {
        auto lineColor = text->explicitBackgroundFillColor();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMTFILLCLR", 70, colNum);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXTDIRECTION)) {
        d.add("$DIMTXTDIRECTION", 292, text->readingDirection());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTVP)) {
        d.add("$DIMTVP", 145, text->verticalDistanceToDimLine());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMUPT)) {
        d.add("$DIMUPT", 288, text->cursorControlPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTMOVE)) {
        d.add("$DIMTMOVE", 279, text->positionMovementPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMATFIT)) {
        d.add("$DIMATFIT", 289, text->unsufficientSpacePolicy());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleLinearFormat(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto linear = ds->linearFormat();
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMLUNIT)) {
        d.add("$DIMLUNIT", 277, linear->formatRaw());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMSEP)) {
        d.add("$DIMDSEP", 278, linear->decimalFormatSeparatorChar());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMDEC)) {
        d.add("$DIMDEC", 271, linear->decimalPlaces());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMPOST)) {
        d.add("$DIMPOST", 3, linear->prefixOrSuffix().toStdString());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALT)) {
        d.add("$DIMALT", 170, linear->alternateUnits());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTU)) {
        d.add("$DIMALTU", 273, linear->altFormatRaw());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTD)) {
        d.add("$DIMALTD", 171, linear->altDecimalPlaces());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTF)) {
        d.add("$DIMALTF", 143, linear->altUnitsMultiplier());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMAPOST)) {
        d.add("$DIMAPOST", 4, linear->altPrefixOrSuffix().toStdString());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleFractions(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto fraction = ds->fractions();
    if (fraction->checkModifyState(LC_DimStyle::Fractions::$DIMFRAC)) {
        d.add("$DIMFRAC", 276, fraction->style());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleAngularFormat(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto angular = ds->angularFormat();
    if (angular->checkModifyState(LC_DimStyle::AngularFormat::$DIMAUNIT)) {
        d.add("$DIMAUNIT", 275, angular->format());
    }
    if (angular->checkModifyState(LC_DimStyle::AngularFormat::$DIMADEC)) {
        d.add("$DIMADEC", 179, angular->decimalPlaces());
    }

    auto round = ds->roundOff();
    if (round->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMRND)) {
        d.add("$DIMRND", 45, round->roundTo());
    }
    if (round->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMALTRND)) {
        d.add("$DIMALTRND", 148, round->altRoundTo());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleRadial(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto radial = ds->radial();
    if (radial->checkModifyState(LC_DimStyle::Radial::$DIMCEN)) {
        d.add("$DIMCEN", 141, radial->centerCenterMarkOrLineSize());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleTolerance(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto tolerance = ds->latteralTolerance();
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTDEC)) {
        d.add("$DIMTDEC", 272, tolerance->decimalPlaces());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMALTTD)) {
        d.add("$DIMALTTD", 274, tolerance->decimalPlacesAltDim());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOL)) {
        d.add("$DIMTOL", 71, tolerance->isAppendTolerancesToDimText() ? 1 : 0);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOLJ)) {
        d.add("$DIMTOLJ", 283, tolerance->verticalJustification());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {
        d.add("$DIMTM", 48, tolerance->lowerToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {
        d.add("$DIMTP", 47, tolerance->upperToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTFAC)) {
        d.add("$DIMTFAC", 146, tolerance->heightScaleFactorToDimText());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMLIM)) {
        d.add("$DIMLIM", 72, tolerance->isLimitsGeneratedAsDefaultText() ? 1 : 0);
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleArc(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto arc = ds->arc();
    if (arc->checkModifyState(LC_DimStyle::Arc::$DIMARCSYM)) {
        d.add("$DIMARCSYM", 90, arc->arcSymbolPosition());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleLeader(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto leader = ds->leader();
    if (leader->checkModifyState(LC_DimStyle::Leader::$DIMLDRBLK)) {
        QString blockName = leader->arrowBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW != nullptr ? m_dxfW->getBlockRecordHandleToWrite(blkName) : -1;
            if(blkHandle > 0) {
                d.add("_$DIMLDRBLK", 341, toHexStr(blkHandle).toStdString());
            }
        }
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleExtData(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto tolerance = ds->latteralTolerance();
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTALN)) {
        d.extData.push_back(new DRW_Variant(1001, "ACAD_DSTYLE_DIMTALN"));
        d.extData.push_back(new DRW_Variant(1070, 392));
        d.extData.push_back(new DRW_Variant(1070, tolerance->adjustment()));
    }
    // todo - add support of ACAD_DIMSTYLE_DIMBREAK
    // todo - add support of ACAD_DIMSTYLE_DIMJAG
}

void RS_FilterDXFRW::prepareDRWDimStyle(DRW_Dimstyle &d, LC_DimStyle* ds) {
    d.name = ds->getName().toStdString();

    auto savedMode = ds->getModifyCheckMode();
    if (ds->isBaseStyle()){
        // base styles are written completely, regardless of fields modification state!
        ds->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
    }

    // in AutoCAD, dimstyle that is specific for the type of dimension will contain only
    // values that are really overriden for the type. Other properties will be
    // extracted from the base style.
    // therefore, have need to check for properties that are modified and write only
    // one that has modified flag set...

    prepareDRWDimStyleArrows(d, ds);
    prepareDRWDimStyleScaling(d, ds);
    prepareDRWDimStyleExtLine(d, ds);
    prepareDRWDimStyleDimLine(d, ds);
    prepareDRWDimStyleText(d, ds);
    prepareDRWDimStyleZerosSuppression(d, ds);
    prepareDRWDimStyleLinearFormat(d, ds);
    prepareDRWDimStyleAngularFormat(d, ds);
    prepareDRWDimStyleFractions(d, ds);
    prepareDRWDimStyleRadial(d, ds);
    prepareDRWDimStyleTolerance(d, ds);
    prepareDRWDimStyleArc(d, ds);
    prepareDRWDimStyleLeader(d, ds);
    prepareDRWDimStyleExtData(d, ds);
    // result->setDimunit(s.dimunit); // $DIMLUNIT

    ds->setModifyCheckMode(savedMode); // restore modification flags

    // fixme - return to this later, move to MLeaderStyle
    // auto mleader = ds->mleader();
    // var = s.get("$MLEADERSCALE");
    // if (var != nullptr) {
    //     mleaderStyle->setScale(var->d_val());
    // }
}

void RS_FilterDXFRW::writeObjects() {
    if (m_dwgW) {
        const auto& metadata = m_graphic->dwgAdvancedMetadata();
        const bool canWriteModernObjects = m_dwgW->getVersion() >= DRW::AC1021;
        std::set<duint32> nativeSunHandles;
        std::set<duint32> nativePlaceholderHandles;
        std::set<duint32> nativeMLeaderStyleHandles;
        int nativeSunObjects = 0;
        int nativePlaceholderObjects = 0;
        int nativeMLeaderStyleObjects = 0;
        if (canWriteModernObjects) {
            for (const auto& record : metadata.suns()) {
                if (record.replayState == LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                    && record.handle != 0) {
                    nativeSunHandles.insert(record.handle);
                }
            }
            for (const auto& record : metadata.placeholders()) {
                if (record.replayState == LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                    && record.handle != 0) {
                    nativePlaceholderHandles.insert(record.handle);
                }
            }
            for (const auto& record : metadata.mleaderStyles()) {
                if (record.replayState == LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                    && record.handle != 0
                    && !hasReplayableRawMLeaderStyle(metadata, record.handle)) {
                    nativeMLeaderStyleHandles.insert(record.handle);
                }
            }
        }

        bool hasBlockedReplay = false;
        int blockedInvalidated = 0;
        int blockedReplaced = 0;
        int blockedEntityReplay = 0;
        int blockedMissingRawBytes = 0;
        int blockedMissingClassMetadata = 0;
        int blockedWriterRejected = 0;
        int replayedObjects = 0;
        const LC_DwgAdvancedMetadata::RawObjectFamilyCounts rawFamilyCounts =
            metadata.rawObjectFamilyCounts();
        const LC_DwgAdvancedMetadata::TableNativeWriterBlockerCounts tableBlockers =
            metadata.tableNativeWriterBlockerCounts(m_dwgW->getVersion());
        const LC_DwgAdvancedMetadata::MLeaderWriterBlockerCounts mleaderBlockers =
            metadata.mleaderWriterBlockerCounts();
        const LC_DwgAdvancedMetadata::AdvancedEntityWriterBlockerCounts
            advancedEntityBlockers =
                metadata.advancedEntityWriterBlockerCounts(m_dwgW->getVersion());
        const LC_DwgAdvancedMetadata::ModelerPayloadCounts modelerPayloads =
            metadata.modelerPayloadCounts();
        const LC_DwgAdvancedMetadata::MeshWriterBlockerCounts meshBlockers =
            metadata.meshWriterBlockerCounts();
        const LC_DwgAdvancedMetadata::ExternalReferenceCounts externalRefs =
            metadata.externalReferenceCounts();
        const LC_DwgAdvancedMetadata::ShapeOleWriterBlockerCounts shapeOleBlockers =
            metadata.shapeOleWriterBlockerCounts();
        const LC_DwgAdvancedMetadata::VisualMetadataWriterBlockerCounts
            visualBlockers =
                metadata.visualMetadataWriterBlockerCounts(m_dwgW->getVersion());
        const LC_DwgAdvancedMetadata::AssociativeShellCounts associativeShells =
            metadata.associativeShellCounts();
        const LC_DwgAdvancedMetadata::AssociativePrefixCounts associativePrefixes =
            metadata.associativePrefixCounts();
        LC_DwgAdvancedMetadata::GraphReplayPolicyCounts graphReplayPolicy =
            metadata.graphReplayPolicyCounts();
        for (const auto& record : metadata.rawObjects()) {
            if (nativeSunHandles.count(record.handle) != 0 && isSunRawObject(record)) {
                hasBlockedReplay = true;
                ++blockedReplaced;
                continue;
            }
            if (nativePlaceholderHandles.count(record.handle) != 0
                && isAcDbPlaceholderRawObject(record)) {
                hasBlockedReplay = true;
                ++blockedReplaced;
                continue;
            }
            if (nativeMLeaderStyleHandles.count(record.handle) != 0
                && isMLeaderStyleRawObject(record)) {
                hasBlockedReplay = true;
                ++blockedReplaced;
                continue;
            }
            const LC_DwgAdvancedMetadata::ReplayBlocker blocker =
                LC_DwgAdvancedMetadata::rawReplayBlocker(record);
            if (blocker != LC_DwgAdvancedMetadata::ReplayBlocker::None) {
                hasBlockedReplay = true;
                if (blocker == LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated)
                    ++blockedInvalidated;
                else if (blocker == LC_DwgAdvancedMetadata::ReplayBlocker::Replaced)
                    ++blockedReplaced;
                else if (blocker == LC_DwgAdvancedMetadata::ReplayBlocker::EntityReplayUnsupported)
                    ++blockedEntityReplay;
                else if (blocker == LC_DwgAdvancedMetadata::ReplayBlocker::MissingRawBytes)
                    ++blockedMissingRawBytes;
                else if (blocker == LC_DwgAdvancedMetadata::ReplayBlocker::MissingClassMetadata)
                    ++blockedMissingClassMetadata;
                continue;
            }
            DRW_UnsupportedObject object = rawObjectFromMetadata(record);
            if (m_dwgW->writeRawDwgObject(&object)) {
                ++replayedObjects;
            } else {
                hasBlockedReplay = true;
                ++blockedWriterRejected;
            }
        }
        if (canWriteModernObjects) {
            for (const auto& record : metadata.placeholders()) {
                if (record.replayState != LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                    || record.handle == 0) {
                    continue;
                }
                DRW_AcDbPlaceholder placeholder = placeholderFromMetadata(record);
                if (m_dwgW->writeAcDbPlaceholder(&placeholder)) {
                    ++nativePlaceholderObjects;
                } else {
                    hasBlockedReplay = true;
                    ++blockedWriterRejected;
                }
            }
            for (const auto& record : metadata.suns()) {
                if (record.replayState != LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed
                    || record.handle == 0) {
                    continue;
                }
                DRW_Sun sun = sunFromMetadata(record);
                if (m_dwgW->writeSun(&sun)) {
                    ++nativeSunObjects;
                } else {
                    hasBlockedReplay = true;
                    ++blockedWriterRejected;
                }
            }
            for (const auto& record : metadata.mleaderStyles()) {
                if (nativeMLeaderStyleHandles.count(record.handle) == 0)
                    continue;
                DRW_MLeaderStyle style = mleaderStyleFromMetadata(record);
                if (m_dwgW->writeMLeaderStyle(&style)) {
                    ++nativeMLeaderStyleObjects;
                } else {
                    hasBlockedReplay = true;
                    ++blockedWriterRejected;
                }
            }
        }
        if (replayedObjects > 0) {
            RS_DEBUG->print("RS_FilterDXFRW::writeObjects: replayed %d raw DWG objects",
                            replayedObjects);
        }
        if (rawFamilyCounts.total() > 0) {
            RS_DEBUG->print(
                "RS_FilterDXFRW::writeObjects: raw DWG object families "
                "assoc=%d eval-graph=%d dynamic-block=%d object-context=%d unknown=%d",
                static_cast<int>(rawFamilyCounts.associative),
                static_cast<int>(rawFamilyCounts.evaluationGraph),
                static_cast<int>(rawFamilyCounts.dynamicBlock),
                static_cast<int>(rawFamilyCounts.objectContext),
                static_cast<int>(rawFamilyCounts.unknown));
        }
        if (nativeSunObjects > 0) {
            RS_DEBUG->print("RS_FilterDXFRW::writeObjects: wrote %d native SUN objects",
                            nativeSunObjects);
        }
        if (nativePlaceholderObjects > 0) {
            RS_DEBUG->print(
                "RS_FilterDXFRW::writeObjects: wrote %d native ACDBPLACEHOLDER objects",
                nativePlaceholderObjects);
        }
        if (nativeMLeaderStyleObjects > 0) {
            RS_DEBUG->print(
                "RS_FilterDXFRW::writeObjects: wrote %d native MLEADERSTYLE objects",
                nativeMLeaderStyleObjects);
        }
        if (modelerPayloads.recordCount > 0) {
            const RS_Debug::RS_DebugLevel level =
                modelerPayloads.inconsistentSplit > 0
                    ? RS_Debug::D_WARNING
                    : RS_Debug::D_DEBUGGING;
            RS_DEBUG->print(
                level,
                "DWG modeler geometry payloads: records=%d SAT=%d SAB=%d "
                "unknown=%d split-inconsistent=%d marker-body=%d marker-handle=%d",
                static_cast<int>(modelerPayloads.recordCount),
                static_cast<int>(modelerPayloads.sat),
                static_cast<int>(modelerPayloads.sab),
                static_cast<int>(modelerPayloads.unknown),
                static_cast<int>(modelerPayloads.inconsistentSplit),
                static_cast<int>(modelerPayloads.markerInBody),
                static_cast<int>(modelerPayloads.markerInHandleStream));
        }
        if (associativeShells.recordCount > 0) {
            RS_DEBUG->print(
                "DWG associative/action shells: records=%d network=%d action=%d "
                "dependency=%d geom-dependency=%d action-param=%d value-param=%d/%d "
                "prefix=%d/%d single-dep=%d compound=%d prefix-status=%d "
                "complete=%d partial=%d overflow=%d handles=%d values=%d unknown=%d",
                static_cast<int>(associativeShells.recordCount),
                static_cast<int>(associativeShells.network),
                static_cast<int>(associativeShells.action),
                static_cast<int>(associativeShells.dependency),
                static_cast<int>(associativeShells.geometryDependency),
                static_cast<int>(associativeShells.actionParamRecords),
                static_cast<int>(associativeShells.parsedValueParamRecords),
                static_cast<int>(associativeShells.valueParamRecords),
                static_cast<int>(associativeShells.parsedActionParamPrefixes),
                static_cast<int>(associativeShells.actionParamRecords),
                static_cast<int>(associativeShells.singleDependencyActionParamPrefixes),
                static_cast<int>(associativeShells.compoundActionParamPrefixes),
                static_cast<int>(associativePrefixes.prefixCount),
                static_cast<int>(associativePrefixes.complete),
                static_cast<int>(associativePrefixes.partial),
                static_cast<int>(associativePrefixes.boundedCountOverflow),
                static_cast<int>(associativePrefixes.decodedHandleCount),
                static_cast<int>(associativePrefixes.decodedValueCount),
                static_cast<int>(associativeShells.unknown));
        }
        const size_t graphReplayKnownFamilies =
            (graphReplayPolicy.preserved.total()
             - graphReplayPolicy.preserved.unknown)
            + (graphReplayPolicy.suppressed.total()
               - graphReplayPolicy.suppressed.unknown);
        if (graphReplayKnownFamilies > 0
            || graphReplayPolicy.totalSemanticOnly() > 0
            || graphReplayPolicy.totalReasons() > 0) {
            RS_DEBUG->print(
                graphReplayPolicy.suppressed.total() > 0
                        || graphReplayPolicy.totalReasons() > 0
                    ? RS_Debug::D_WARNING
                    : RS_Debug::D_DEBUGGING,
                "DWG graph replay policy: preserved dimassoc=%d eval=%d "
                "assoc=%d dynamic-block=%d object-context=%d acsh=%d unknown=%d "
                "suppressed dimassoc=%d eval=%d assoc=%d dynamic-block=%d "
                "object-context=%d acsh=%d unknown=%d semantic-only assoc=%d "
                "acsh=%d reasons edited=%d missing-target=%d evaluator=%d "
                "parser-partial=%d fallback-edited=%d native-replaced=%d "
                "cycle-path=%d owner-deleted=%d raw-invalidated=%d "
                "raw-replaced=%d entity=%d missing-bytes=%d missing-class=%d",
                static_cast<int>(graphReplayPolicy.preserved.dimensionAssociation),
                static_cast<int>(graphReplayPolicy.preserved.evaluationGraph),
                static_cast<int>(graphReplayPolicy.preserved.acDbAssoc),
                static_cast<int>(graphReplayPolicy.preserved.dynamicBlock),
                static_cast<int>(graphReplayPolicy.preserved.objectContext),
                static_cast<int>(graphReplayPolicy.preserved.acShHistory),
                static_cast<int>(graphReplayPolicy.preserved.unknown),
                static_cast<int>(graphReplayPolicy.suppressed.dimensionAssociation),
                static_cast<int>(graphReplayPolicy.suppressed.evaluationGraph),
                static_cast<int>(graphReplayPolicy.suppressed.acDbAssoc),
                static_cast<int>(graphReplayPolicy.suppressed.dynamicBlock),
                static_cast<int>(graphReplayPolicy.suppressed.objectContext),
                static_cast<int>(graphReplayPolicy.suppressed.acShHistory),
                static_cast<int>(graphReplayPolicy.suppressed.unknown),
                static_cast<int>(graphReplayPolicy.semanticOnlyAssociative),
                static_cast<int>(graphReplayPolicy.semanticOnlyAcSh),
                static_cast<int>(graphReplayPolicy.editedEntity),
                static_cast<int>(graphReplayPolicy.missingTarget),
                static_cast<int>(graphReplayPolicy.unsupportedEvaluator),
                static_cast<int>(graphReplayPolicy.parserPartial),
                static_cast<int>(graphReplayPolicy.fallbackGeometryEdited),
                static_cast<int>(graphReplayPolicy.nativeReplacement),
                static_cast<int>(graphReplayPolicy.cyclePathInvalidated),
                static_cast<int>(graphReplayPolicy.ownerDeleted),
                static_cast<int>(graphReplayPolicy.invalidated),
                static_cast<int>(graphReplayPolicy.replaced),
                static_cast<int>(graphReplayPolicy.entityReplayUnsupported),
                static_cast<int>(graphReplayPolicy.missingRawBytes),
                static_cast<int>(graphReplayPolicy.missingClassMetadata));
        }
        if (tableBlockers.tableCount > 0 && tableBlockers.totalBlockers() > 0) {
            RS_DEBUG->print(
                RS_Debug::D_WARNING,
                "Native DWG table writing blocked: tables=%d eligible-text=%d "
                "layout-direct=%d layout-separate=%d layout-embedded=%d "
                "layout-unsupported=%d semantic=%d version=%d ambiguous-layout=%d "
                "owner=%d style=%d cell-style=%d unknown-ranges=%d "
                "incomplete-ranges=%d override=%d break=%d geometry=%d "
                "merged=%d field=%d block=%d attributes=%d unknown-content=%d "
                "value-payload=%d edited-fallback=%d missing-fallback-links=%d "
                "anon-block=%d text-style=%d linetype=%d raw-invalidated=%d "
                "raw-replaced=%d dimensions=%d",
                static_cast<int>(tableBlockers.tableCount),
                static_cast<int>(tableBlockers.eligibleTextOnly),
                static_cast<int>(tableBlockers.legacyDirectLayout),
                static_cast<int>(tableBlockers.separateTableContentLayout),
                static_cast<int>(tableBlockers.embeddedTableContentLayout),
                static_cast<int>(tableBlockers.unsupportedLayout),
                static_cast<int>(tableBlockers.noSemanticTableContent),
                static_cast<int>(tableBlockers.unsupportedTableVersion),
                static_cast<int>(tableBlockers.ambiguousTableContentStorage),
                static_cast<int>(tableBlockers.missingOwnerHandle),
                static_cast<int>(tableBlockers.unresolvedTableStyle),
                static_cast<int>(tableBlockers.unresolvedCellStyleMap),
                static_cast<int>(tableBlockers.unknownSubrecordRange),
                static_cast<int>(tableBlockers.incompleteSubrecordRange),
                static_cast<int>(tableBlockers.overrideMask),
                static_cast<int>(tableBlockers.breakData),
                static_cast<int>(tableBlockers.geometryTail),
                static_cast<int>(tableBlockers.mergedCell),
                static_cast<int>(tableBlockers.fieldContent),
                static_cast<int>(tableBlockers.blockContent),
                static_cast<int>(tableBlockers.attributeContent),
                static_cast<int>(tableBlockers.unknownCellContent),
                static_cast<int>(tableBlockers.incompleteValuePayload),
                static_cast<int>(tableBlockers.editedFallback),
                static_cast<int>(tableBlockers.missingFallbackAttachment),
                static_cast<int>(tableBlockers.anonymousBlockPolicyUnresolved),
                static_cast<int>(tableBlockers.unresolvedTextStyle),
                static_cast<int>(tableBlockers.unresolvedLineType),
                static_cast<int>(tableBlockers.rawReplayInvalidated),
                static_cast<int>(tableBlockers.rawReplayReplaced),
                static_cast<int>(tableBlockers.nonPositiveDimension));
        }
        if (mleaderBlockers.mleaderCount > 0 && mleaderBlockers.totalBlockers() > 0) {
            RS_DEBUG->print(
                RS_Debug::D_WARNING,
                "Native DWG MLEADER writing limited: mleaders=%d unresolved-style=%d "
                "missing-text=%d block=%d tolerance=%d overrides=%d geometry=%d "
                "invalidated=%d replaced=%d",
                static_cast<int>(mleaderBlockers.mleaderCount),
                static_cast<int>(mleaderBlockers.unresolvedStyle),
                static_cast<int>(mleaderBlockers.missingTextContent),
                static_cast<int>(mleaderBlockers.blockContent),
                static_cast<int>(mleaderBlockers.toleranceContent),
                static_cast<int>(mleaderBlockers.overrideFlags),
                static_cast<int>(mleaderBlockers.missingLeaderGeometry),
                static_cast<int>(mleaderBlockers.invalidated),
                static_cast<int>(mleaderBlockers.replaced));
        }
        if (meshBlockers.meshCount > 0 && meshBlockers.totalBlockers() > 0) {
            RS_DEBUG->print(
                RS_Debug::D_WARNING,
                "Native DWG mesh writing blocked: meshes=%d sidecars=%d "
                "range-complete=%d range-missing=%d range-incomplete=%d "
                "crease=%d subdivision=%d fallback-only=%d edited-fallback=%d "
                "owner-class=%d malformed-counts=%d invalidated=%d replaced=%d",
                static_cast<int>(meshBlockers.meshCount),
                static_cast<int>(meshBlockers.sidecarCount),
                static_cast<int>(meshBlockers.completeRawRange),
                static_cast<int>(meshBlockers.missingRawRange),
                static_cast<int>(meshBlockers.incompleteRawRange),
                static_cast<int>(meshBlockers.missingCreaseData),
                static_cast<int>(meshBlockers.unsupportedSubdivisionData),
                static_cast<int>(meshBlockers.fallbackOnlyPreview),
                static_cast<int>(meshBlockers.editedFallback),
                static_cast<int>(meshBlockers.missingOwnerOrClassHandle),
                static_cast<int>(meshBlockers.malformedCountRelationships),
                static_cast<int>(meshBlockers.invalidated),
                static_cast<int>(meshBlockers.replaced));
        }
        if (externalRefs.imageEntities > 0 || externalRefs.wipeouts > 0
            || externalRefs.underlays > 0
            || externalRefs.imageDefinitions > 0
            || externalRefs.underlayDefinitions > 0) {
            const bool hasExternalIssues =
                externalRefs.totalPathIssues() > 0
                || externalRefs.missingDefinitionHandles > 0
                || externalRefs.malformedClips > 0;
            RS_DEBUG->print(
                hasExternalIssues ? RS_Debug::D_WARNING
                                  : RS_Debug::D_DEBUGGING,
                "DWG external references: images=%d wipeouts=%d image-defs=%d "
                "underlays=%d underlay-defs=%d raster-vars=%d path-empty=%d "
                "path-relative=%d path-missing=%d external=%d scheme=%d "
                "case-candidate=%d missing-def=%d orphan-def=%d clip-none=%d "
                "clip-rect=%d clip-poly=%d clip-bad=%d inverted=%d hidden-frame=%d",
                static_cast<int>(externalRefs.imageEntities),
                static_cast<int>(externalRefs.wipeouts),
                static_cast<int>(externalRefs.imageDefinitions),
                static_cast<int>(externalRefs.underlays),
                static_cast<int>(externalRefs.underlayDefinitions),
                static_cast<int>(externalRefs.rasterVariables),
                static_cast<int>(externalRefs.emptyPaths),
                static_cast<int>(externalRefs.relativePaths),
                static_cast<int>(externalRefs.absoluteMissingPaths),
                static_cast<int>(externalRefs.externalPaths),
                static_cast<int>(externalRefs.unsupportedSchemes),
                static_cast<int>(externalRefs.caseMismatchCandidates),
                static_cast<int>(externalRefs.missingDefinitionHandles),
                static_cast<int>(externalRefs.definitionsWithoutEntities),
                static_cast<int>(externalRefs.noBoundaryClips),
                static_cast<int>(externalRefs.rectangularClips),
                static_cast<int>(externalRefs.polygonalClips),
                static_cast<int>(externalRefs.malformedClips),
                static_cast<int>(externalRefs.invertedClips),
                static_cast<int>(externalRefs.hiddenFrames));
        }
        if (shapeOleBlockers.shapeCount > 0
            || shapeOleBlockers.ole2FrameCount > 0) {
            RS_DEBUG->print(
                shapeOleBlockers.totalBlockers() > 0
                    ? RS_Debug::D_WARNING
                    : RS_Debug::D_DEBUGGING,
                "DWG shape/OLE writer blockers: shapes=%d ole2=%d "
                "missing-style=%d unresolved-style=%d missing-ole=%d "
                "truncated-ole=%d oversized-ole=%d edited-preview=%d "
                "unsupported-ole=%d raw-missing=%d raw-incomplete=%d "
                "invalidated=%d replaced=%d",
                static_cast<int>(shapeOleBlockers.shapeCount),
                static_cast<int>(shapeOleBlockers.ole2FrameCount),
                static_cast<int>(shapeOleBlockers.missingStyleHandle),
                static_cast<int>(shapeOleBlockers.unresolvedShapeStyle),
                static_cast<int>(shapeOleBlockers.missingOlePayload),
                static_cast<int>(shapeOleBlockers.truncatedOlePayload),
                static_cast<int>(shapeOleBlockers.oversizedOlePayload),
                static_cast<int>(shapeOleBlockers.editedPreviewFrame),
                static_cast<int>(shapeOleBlockers.unsupportedOlePayloadRegeneration),
                static_cast<int>(shapeOleBlockers.missingRawRange),
                static_cast<int>(shapeOleBlockers.incompleteRawRange),
                static_cast<int>(shapeOleBlockers.invalidated),
                static_cast<int>(shapeOleBlockers.replaced));
        }
        if (visualBlockers.recordCount > 0 || visualBlockers.rawPayloads > 0) {
            RS_DEBUG->print(
                visualBlockers.totalBlockers() > 0
                    ? RS_Debug::D_WARNING
                    : RS_Debug::D_DEBUGGING,
                "DWG visual metadata export policy: records=%d raw=%d "
                "raw-replayable=%d raw-suppressed=%d ucs=%d base-ucs=%d "
                "visual-style=%d sun=%d background=%d live-section=%d "
                "owner-layout=%d raw-invalidated=%d raw-replaced=%d "
                "unsupported-visual-style=%d",
                static_cast<int>(visualBlockers.recordCount),
                static_cast<int>(visualBlockers.rawPayloads),
                static_cast<int>(visualBlockers.replayableRawPayloads),
                static_cast<int>(visualBlockers.suppressedRawPayloads),
                static_cast<int>(visualBlockers.unresolvedUcs),
                static_cast<int>(visualBlockers.unresolvedBaseUcs),
                static_cast<int>(visualBlockers.unresolvedVisualStyle),
                static_cast<int>(visualBlockers.unresolvedSun),
                static_cast<int>(visualBlockers.unresolvedBackground),
                static_cast<int>(visualBlockers.unresolvedLiveSection),
                static_cast<int>(visualBlockers.missingOwnerOrLayout),
                static_cast<int>(visualBlockers.invalidatedRawPayload),
                static_cast<int>(visualBlockers.replacedNativeUnavailablePayload),
                static_cast<int>(visualBlockers.unsupportedVisualStyleWriter));
        }
        if (advancedEntityBlockers.recordCount > 0
            && advancedEntityBlockers.totalBlockers() > 0) {
            RS_DEBUG->print(
                RS_Debug::D_WARNING,
                "DWG advanced entity writer readiness: records=%d mesh=%d "
                "shape=%d ole2=%d image=%d wipeout=%d underlay=%d "
                "mleader=%d arc-dim=%d unknown=%d native=%d raw=%d "
                "fallback=%d edited-fallback=%d metadata=%d payload=%d "
                "advanced-content=%d oda-complete=%d oda-partial=%d "
                "oda-absent=%d",
                static_cast<int>(advancedEntityBlockers.recordCount),
                static_cast<int>(advancedEntityBlockers.mesh),
                static_cast<int>(advancedEntityBlockers.shape),
                static_cast<int>(advancedEntityBlockers.ole2Frame),
                static_cast<int>(advancedEntityBlockers.rasterImage),
                static_cast<int>(advancedEntityBlockers.wipeout),
                static_cast<int>(advancedEntityBlockers.underlay),
                static_cast<int>(advancedEntityBlockers.mleader),
                static_cast<int>(advancedEntityBlockers.arcDimension),
                static_cast<int>(advancedEntityBlockers.unknown),
                static_cast<int>(advancedEntityBlockers.nativeWriterAvailable),
                static_cast<int>(advancedEntityBlockers.rawReplayAvailable),
                static_cast<int>(advancedEntityBlockers.fallbackAvailable),
                static_cast<int>(advancedEntityBlockers.editedFallbackInvalidated),
                static_cast<int>(advancedEntityBlockers.missingRequiredMetadata),
                static_cast<int>(advancedEntityBlockers.missingPayloadBytes),
                static_cast<int>(advancedEntityBlockers.unsupportedAdvancedContent),
                static_cast<int>(advancedEntityBlockers.odaComplete),
                static_cast<int>(advancedEntityBlockers.odaPartial),
                static_cast<int>(advancedEntityBlockers.odaAbsent));
        }
        const size_t nativeSemanticRecords =
            static_cast<size_t>(nativeSunObjects + nativePlaceholderObjects
                                + nativeMLeaderStyleObjects);
        const size_t semanticOnlyRecords =
            metadata.semanticOnlyRecordCount() > nativeSemanticRecords
                ? metadata.semanticOnlyRecordCount() - nativeSemanticRecords
                : 0;
        const bool hasSemanticOnlyReplayable = semanticOnlyRecords > 0;
        if (hasBlockedReplay || hasSemanticOnlyReplayable) {
            RS_DEBUG->print(
                RS_Debug::D_WARNING,
                "Some DWG advanced metadata still cannot be emitted natively; "
                "unchanged raw OBJECTS are replayed where safe, while entities "
                "and semantic-only records remain diagnostic-only");
            if (hasBlockedReplay) {
                RS_DEBUG->print(
                    RS_Debug::D_WARNING,
                    "Blocked raw DWG replay: invalidated=%d replaced=%d entity=%d "
                    "missing-bytes=%d missing-class=%d writer-rejected=%d",
                    blockedInvalidated, blockedReplaced, blockedEntityReplay,
                    blockedMissingRawBytes, blockedMissingClassMetadata,
                    blockedWriterRejected);
            }
            if (semanticOnlyRecords > 0) {
                RS_DEBUG->print(
                    RS_Debug::D_WARNING,
                    "Semantic-only DWG metadata records not emitted natively: %d",
                    static_cast<int>(semanticOnlyRecords));
            }
        }
        return;  // DWG writer handles object section internally
    }
    /* PLOTSETTINGS */
    DRW_PlotSettings ps;
    QString horizXvert = QString("%1x%2").arg(m_graphic->getPagesNumHoriz())
                                         .arg(m_graphic->getPagesNumVert());
    ps.plotViewName = horizXvert.toStdString();
    ps.marginLeft = m_graphic->getMarginLeft();
    ps.marginTop = m_graphic->getMarginTop();
    ps.marginRight = m_graphic->getMarginRight();
    ps.marginBottom = m_graphic->getMarginBottom();
    m_dxfW->writePlotSettings(&ps);
}

void RS_FilterDXFRW::writeAppId(){
    DRW_AppId ai;
    ai.name ="LibreCad";
    if (m_dwgW) {
        m_dwgW->addAppId(&ai);
    }
    else {
        m_dxfW->writeAppId(&ai);
    }

    ai.name ="ACAD_DSTYLE_DIMTALN";
    if (m_dwgW) {
        m_dwgW->addAppId(&ai);
    }
    else {
        m_dxfW->writeAppId(&ai);
    }

    ai.name ="ACAD_DSTYLE_DIMJAG_POSITION";
    if (m_dwgW) {
        m_dwgW->addAppId(&ai);
    }
    else {
        m_dxfW->writeAppId(&ai);
    }

    ai.name ="ACAD_DSTYLE_DIMJAG";
    if (m_dwgW) {
        m_dwgW->addAppId(&ai);
    }
    else {
        m_dxfW->writeAppId(&ai);
    }

    // ACAD_DSTYLE_DIMJAG
    // fixme - sand - probably we can add version there, check format
}

void RS_FilterDXFRW::writeEntities(){
  // Pre-pass: reconstruct MLINE entities from decomposed polylines that
  // carry LibreCAD_MLINE XDATA. Consumed polylines are emitted as
  // MLINE; the rest fall through to the normal write path.
  std::set<RS_Entity *> consumed;
  reconstructPolylineSidecars(m_graphic, consumed);
  reconstructMLines(m_graphic, consumed);
  // DWG writer has no UNDERLAY encoder yet — keep that reconstruction DXF-only.
  if (!m_dwgW) {
    reconstructUnderlays(m_graphic, consumed);
  }
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
    if (e->getFlag(RS2::FlagUndone))
      continue;
    if (consumed.find(e) != consumed.end())
      continue;
    writeEntity(e);
  }
}

namespace {
// Parsed view of one polyline's LibreCAD_MLINE XDATA.
struct MLineEntry {
  RS_Entity *entity = nullptr;
  QString mlineId;
  QString styleName;
  double scale = 1.0;
  int justification = 0;
  int elementCount = 0;
  int elementIndex = 0;
  double offset = 0.0;
  int openClosed = 1;
  bool isAnchor = false; // i==0 polyline carries baseline + miter
  std::vector<DRW_Coord> baselineVerts;
  std::vector<DRW_Coord> miterDirs;
};

struct LWPolylineMeta {
  double width = 0.0;
  double elevation = 0.0;
  double thickness = 0.0;
  DRW_Coord extrusion {0.0, 0.0, 1.0};
  int vertexCount = 0;
  std::vector<double> startWidths;
  std::vector<double> endWidths;
  std::vector<int> identifiers;
};

std::optional<LWPolylineMeta> extractLWPolylineMeta(RS_Entity *e) {
  if (!e || !e->hasDrwExtData())
    return std::nullopt;
  const auto &ext = e->getDrwExtData();
  bool inGroup = false;
  LWPolylineMeta meta;
  int seen1040 = 0; // 0=width, 1=elevation, 2=thickness, then vertex widths
  bool gotMarker = false;

  for (const auto &sp : ext) {
    if (!sp)
      continue;
    const int code = sp->code();
    if (code == 1001) {
      inGroup = (std::string{sp->c_str()} == "LibreCAD_LWPOLYLINE");
      if (inGroup)
        gotMarker = true;
      continue;
    }
    if (!inGroup)
      continue;

    switch (code) {
    case 1040: {
      const double d = sp->d_val();
      if (seen1040 == 0)
        meta.width = d;
      else if (seen1040 == 1)
        meta.elevation = d;
      else if (seen1040 == 2)
        meta.thickness = d;
      else if ((seen1040 - 3) % 2 == 0)
        meta.startWidths.push_back(d);
      else
        meta.endWidths.push_back(d);
      ++seen1040;
      break;
    }
    case 1010: {
      const auto *c = sp->coord();
      if (c)
        meta.extrusion = *c;
      break;
    }
    case 1070:
      if (meta.vertexCount == 0)
        meta.vertexCount = static_cast<int>(sp->i_val());
      break;
    case 1071:
      meta.identifiers.push_back(static_cast<int>(sp->i_val()));
      break;
    default:
      break;
    }
  }

  if (!gotMarker)
    return std::nullopt;
  return meta;
}

struct MeshSidecarEntry {
  RS_Entity *entity = nullptr;
  QString meshId;
  QString role;
  int elementIndex = -1;
  int elementCount = 0;
  int roleIndex = -1;
  int flags = 0;
  int mCount = 0;
  int nCount = 0;
  int smoothM = 0;
  int smoothN = 0;
  int curveType = 0;
  std::vector<DRW_Coord> sourceVertices;
};

struct PolyfaceSidecarEntry {
  RS_Entity *entity = nullptr;
  QString polyfaceId;
  int faceIndex = -1;
  int faceCount = 0;
  int flags = 0;
  int vertexCount = 0;
  int originalFaceCount = 0;
  std::array<int, 4> indices {{0, 0, 0, 0}};
  std::vector<DRW_Coord> sourceVertices;
};

std::vector<RS_Vector> collectPolylineVertices(RS_Polyline *polyline) {
  std::vector<RS_Vector> vertices;
  if (polyline == nullptr)
    return vertices;

  const RS_AtomicEntity *lastAtomic = nullptr;
  for (RS_Entity *sub :
       lc::LC_ContainerTraverser{*polyline, RS2::ResolveNone}.entities()) {
    if (sub == nullptr || !sub->isAtomic())
      continue;
    const auto *atomic = static_cast<const RS_AtomicEntity *>(sub);
    vertices.push_back(atomic->getStartpoint());
    lastAtomic = atomic;
  }
  if (lastAtomic != nullptr && !polyline->isClosed())
    vertices.push_back(lastAtomic->getEndpoint());

  return vertices;
}

bool pointsMatch2D(const RS_Vector &point, const DRW_Coord &coord) {
  return std::abs(point.x - coord.x) <= RS_TOLERANCE
      && std::abs(point.y - coord.y) <= RS_TOLERANCE;
}

std::optional<MeshSidecarEntry> extractMeshSidecar(RS_Entity *e) {
  if (!e || !e->hasDrwExtData())
    return std::nullopt;

  MeshSidecarEntry meta;
  meta.entity = e;
  bool inGroup = false;
  bool gotMarker = false;
  int seen1000 = 0;
  int seen1070 = 0;

  for (const auto &sp : e->getDrwExtData()) {
    if (!sp)
      continue;
    const int code = sp->code();
    if (code == 1001) {
      inGroup = (std::string{sp->c_str()} == "LibreCAD_POLYLINE_MESH");
      if (inGroup)
        gotMarker = true;
      continue;
    }
    if (!inGroup)
      continue;

    switch (code) {
    case 1000: {
      const QString value = QString::fromStdString(std::string{sp->c_str()});
      if (seen1000 == 0)
        meta.meshId = value;
      else if (seen1000 == 1)
        meta.role = value;
      ++seen1000;
      break;
    }
    case 1070: {
      const int value = static_cast<int>(sp->i_val());
      if (seen1070 == 0)
        meta.elementIndex = value;
      else if (seen1070 == 1)
        meta.elementCount = value;
      else if (seen1070 == 2)
        meta.roleIndex = value;
      else if (seen1070 == 3)
        meta.flags = value;
      else if (seen1070 == 4)
        meta.mCount = value;
      else if (seen1070 == 5)
        meta.nCount = value;
      else if (seen1070 == 6)
        meta.smoothM = value;
      else if (seen1070 == 7)
        meta.smoothN = value;
      else if (seen1070 == 8)
        meta.curveType = value;
      ++seen1070;
      break;
    }
    case 1010: {
      const auto *coord = sp->coord();
      if (coord)
        meta.sourceVertices.push_back(*coord);
      break;
    }
    default:
      break;
    }
  }

  if (!gotMarker || meta.meshId.isEmpty())
    return std::nullopt;
  return meta;
}

std::optional<PolyfaceSidecarEntry> extractPolyfaceSidecar(RS_Entity *e) {
  if (!e || !e->hasDrwExtData())
    return std::nullopt;

  PolyfaceSidecarEntry meta;
  meta.entity = e;
  bool inGroup = false;
  bool gotMarker = false;
  int seen1070 = 0;

  for (const auto &sp : e->getDrwExtData()) {
    if (!sp)
      continue;
    const int code = sp->code();
    if (code == 1001) {
      inGroup = (std::string{sp->c_str()} == "LibreCAD_POLYLINE_PFACE");
      if (inGroup)
        gotMarker = true;
      continue;
    }
    if (!inGroup)
      continue;

    switch (code) {
    case 1000:
      if (meta.polyfaceId.isEmpty())
        meta.polyfaceId = QString::fromStdString(std::string{sp->c_str()});
      break;
    case 1070: {
      const int value = static_cast<int>(sp->i_val());
      if (seen1070 == 0)
        meta.faceIndex = value;
      else if (seen1070 == 1)
        meta.faceCount = value;
      else if (seen1070 == 2)
        meta.flags = value;
      else if (seen1070 == 3)
        meta.vertexCount = value;
      else if (seen1070 == 4)
        meta.originalFaceCount = value;
      else if (seen1070 >= 5 && seen1070 < 9)
        meta.indices[seen1070 - 5] = value;
      ++seen1070;
      break;
    }
    case 1010: {
      const auto *coord = sp->coord();
      if (coord)
        meta.sourceVertices.push_back(*coord);
      break;
    }
    default:
      break;
    }
  }

  if (!gotMarker || meta.polyfaceId.isEmpty())
    return std::nullopt;
  return meta;
}

// Walk the entity's drwExtData (XDATA stream) and extract LibreCAD_MLINE
// metadata. Returns nullopt if the marker isn't present. The XDATA layout
// matches RS_FilterDXFRW::addMLine's emission schema.
std::optional<MLineEntry> extractMLineMeta(RS_Entity *e) {
  if (!e || !e->hasDrwExtData())
    return std::nullopt;
  const auto &ext = e->getDrwExtData();
  bool inGroup = false;
  MLineEntry m;
  int seen1000 = 0; // 0 = mlineId, 1 = styleName
  int seen1040 = 0; // 0 = scale, 1 = offset
  int seen1070 = 0; // 0 = just, 1 = N, 2 = i, 3 = flags
  for (const auto &sp : ext) {
    if (!sp)
      continue;
    const int code = sp->code();
    if (code == 1001) {
      inGroup = (std::string{sp->c_str()} == "LibreCAD_MLINE");
      continue;
    }
    if (!inGroup)
      continue;
    switch (code) {
    case 1000: {
      QString s = QString::fromStdString(std::string{sp->c_str()});
      if (seen1000 == 0)
        m.mlineId = s;
      else if (seen1000 == 1)
        m.styleName = s;
      ++seen1000;
      break;
    }
    case 1040: {
      const double d = sp->d_val();
      if (seen1040 == 0)
        m.scale = d;
      else if (seen1040 == 1)
        m.offset = d;
      ++seen1040;
      break;
    }
    case 1070: {
      const int v = static_cast<int>(sp->i_val());
      if (seen1070 == 0)
        m.justification = v;
      else if (seen1070 == 1)
        m.elementCount = v;
      else if (seen1070 == 2)
        m.elementIndex = v;
      else if (seen1070 == 3)
        m.openClosed = v;
      ++seen1070;
      break;
    }
    case 1011: {
      // Anchor-only baseline vertex
      const auto *c = sp->coord();
      if (c)
        m.baselineVerts.push_back(*c);
      m.isAnchor = true;
      break;
    }
    case 1013: {
      const auto *c = sp->coord();
      if (c)
        m.miterDirs.push_back(*c);
      break;
    }
    default:
      break;
    }
  }
  if (m.mlineId.isEmpty() || m.elementCount == 0)
    return std::nullopt;
  m.entity = e;
  return m;
}
} // namespace

void RS_FilterDXFRW::reconstructPolylineSidecars(
    RS_EntityContainer *container, std::set<RS_Entity *> &consumed) {
  if (container == nullptr)
    return;

  std::map<QString, std::vector<MeshSidecarEntry>> meshGroups;
  std::map<QString, std::vector<PolyfaceSidecarEntry>> polyfaceGroups;

  for (RS_Entity *entity :
       lc::LC_ContainerTraverser{*container, RS2::ResolveNone}.entities()) {
    if (entity == nullptr || entity->getFlag(RS2::FlagUndone)
        || consumed.find(entity) != consumed.end()
        || entity->rtti() != RS2::EntityPolyline) {
      continue;
    }
    if (auto meshMeta = extractMeshSidecar(entity))
      meshGroups[meshMeta->meshId].push_back(std::move(*meshMeta));
    else if (auto polyfaceMeta = extractPolyfaceSidecar(entity))
      polyfaceGroups[polyfaceMeta->polyfaceId].push_back(
          std::move(*polyfaceMeta));
  }

  for (auto &[meshId, entries] : meshGroups) {
    (void)meshId;
    if (entries.empty())
      continue;

    const MeshSidecarEntry *anchor = nullptr;
    for (const auto &entry : entries) {
      if (!entry.sourceVertices.empty()) {
        anchor = &entry;
        break;
      }
    }
    if (anchor == nullptr || anchor->mCount <= 0 || anchor->nCount <= 0)
      continue;

    const int mCount = anchor->mCount;
    const int nCount = anchor->nCount;
    const int expectedElementCount = mCount + nCount;
    if (anchor->elementCount != expectedElementCount
        || static_cast<int>(entries.size()) != expectedElementCount
        || static_cast<int>(anchor->sourceVertices.size()) != mCount * nCount) {
      continue;
    }

    bool valid = true;
    std::vector<bool> seenElements(expectedElementCount, false);
    for (const auto &entry : entries) {
      if (entry.elementIndex < 0 || entry.elementIndex >= expectedElementCount
          || seenElements[entry.elementIndex] || entry.mCount != mCount
          || entry.nCount != nCount || entry.elementCount != expectedElementCount) {
        valid = false;
        break;
      }
      seenElements[entry.elementIndex] = true;

      auto *polyline = static_cast<RS_Polyline *>(entry.entity);
      const std::vector<RS_Vector> visible = collectPolylineVertices(polyline);
      if (entry.role == "row") {
        if (entry.roleIndex < 0 || entry.roleIndex >= mCount
            || static_cast<int>(visible.size()) != nCount) {
          valid = false;
          break;
        }
        for (int j = 0; j < nCount; ++j) {
          const DRW_Coord &coord =
              anchor->sourceVertices[entry.roleIndex * nCount + j];
          if (!pointsMatch2D(visible[j], coord)) {
            valid = false;
            break;
          }
        }
      }
      else if (entry.role == "column") {
        if (entry.roleIndex < 0 || entry.roleIndex >= nCount
            || static_cast<int>(visible.size()) != mCount) {
          valid = false;
          break;
        }
        for (int i = 0; i < mCount; ++i) {
          const DRW_Coord &coord =
              anchor->sourceVertices[i * nCount + entry.roleIndex];
          if (!pointsMatch2D(visible[i], coord)) {
            valid = false;
            break;
          }
        }
      }
      else {
        valid = false;
      }
      if (!valid)
        break;
    }
    if (!valid)
      continue;

    DRW_Polyline polyline;
    polyline.flags = anchor->flags | 0x10;
    polyline.vertexcount = mCount;
    polyline.facecount = nCount;
    polyline.smoothM = anchor->smoothM;
    polyline.smoothN = anchor->smoothN;
    polyline.curvetype = anchor->curveType;
    getEntityAttributes(&polyline, anchor->entity);
    for (const DRW_Coord &coord : anchor->sourceVertices) {
      DRW_Vertex vertex(coord.x, coord.y, coord.z, 0.0);
      vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Mesh);
      polyline.addVertex(vertex);
    }

    if (m_dwgW)
      m_dwgW->writePolyline(&polyline);
    else if (m_dxfW)
      m_dxfW->writePolyline(&polyline);

    for (const auto &entry : entries)
      consumed.insert(entry.entity);
  }

  for (auto &[polyfaceId, entries] : polyfaceGroups) {
    (void)polyfaceId;
    if (entries.empty())
      continue;

    const PolyfaceSidecarEntry *anchor = nullptr;
    for (const auto &entry : entries) {
      if (!entry.sourceVertices.empty()) {
        anchor = &entry;
        break;
      }
    }
    if (anchor == nullptr || anchor->vertexCount <= 0
        || anchor->faceCount <= 0) {
      continue;
    }

    if (static_cast<int>(entries.size()) != anchor->faceCount
        || static_cast<int>(anchor->sourceVertices.size())
               != anchor->vertexCount) {
      continue;
    }

    bool valid = true;
    std::vector<bool> seenFaces(anchor->faceCount, false);
    for (const auto &entry : entries) {
      if (entry.faceIndex < 0 || entry.faceIndex >= anchor->faceCount
          || seenFaces[entry.faceIndex] || entry.faceCount != anchor->faceCount
          || entry.vertexCount != anchor->vertexCount) {
        valid = false;
        break;
      }
      seenFaces[entry.faceIndex] = true;

      const int expectedPoints = entry.indices[3] == 0 ? 3 : 4;
      auto *polyline = static_cast<RS_Polyline *>(entry.entity);
      const std::vector<RS_Vector> visible = collectPolylineVertices(polyline);
      if (static_cast<int>(visible.size()) != expectedPoints) {
        valid = false;
        break;
      }
      for (int i = 0; i < expectedPoints; ++i) {
        const int vertexIndex = std::abs(entry.indices[i]);
        if (vertexIndex < 1 || vertexIndex > anchor->vertexCount) {
          valid = false;
          break;
        }
        if (!pointsMatch2D(visible[i],
                           anchor->sourceVertices[vertexIndex - 1])) {
          valid = false;
          break;
        }
      }
      if (!valid)
        break;
    }
    if (!valid)
      continue;

    std::sort(entries.begin(), entries.end(),
              [](const PolyfaceSidecarEntry &lhs,
                 const PolyfaceSidecarEntry &rhs) {
                return lhs.faceIndex < rhs.faceIndex;
              });

    DRW_Polyline polyline;
    polyline.flags = anchor->flags | 0x40;
    polyline.vertexcount = anchor->vertexCount;
    polyline.facecount = anchor->faceCount;
    getEntityAttributes(&polyline, anchor->entity);

    for (const DRW_Coord &coord : anchor->sourceVertices) {
      DRW_Vertex vertex(coord.x, coord.y, coord.z, 0.0);
      vertex.flags = 0x40 | 0x80;
      vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Polyface);
      polyline.addVertex(vertex);
    }
    for (const auto &entry : entries) {
      DRW_Vertex face;
      face.flags = 0x80;
      face.vindex1 = entry.indices[0];
      face.vindex2 = entry.indices[1];
      face.vindex3 = entry.indices[2];
      face.vindex4 = entry.indices[3];
      face.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
      polyline.addVertex(face);
    }

    if (m_dwgW)
      m_dwgW->writePolyline(&polyline);
    else if (m_dxfW)
      m_dxfW->writePolyline(&polyline);

    for (const auto &entry : entries)
      consumed.insert(entry.entity);
  }
}

void RS_FilterDXFRW::reconstructMLines(RS_EntityContainer *container,
                                       std::set<RS_Entity *> &consumed) {
  if (!container)
    return;

  // First pass: collect all polylines carrying the MLINE marker.
  std::map<QString, std::vector<MLineEntry>> groups;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{*container, RS2::ResolveNone}.entities()) {
    if (e->getFlag(RS2::FlagUndone))
      continue;
    if (e->rtti() != RS2::EntityPolyline)
      continue;
    auto m = extractMLineMeta(e);
    if (!m)
      continue;
    groups[m->mlineId].push_back(std::move(*m));
  }

  // Second pass: reconstruct + emit each complete group.
  for (auto &[mlineId, entries] : groups) {
    if (entries.empty())
      continue;
    const int N = entries.front().elementCount;
    if (static_cast<int>(entries.size()) != N) {
      // Incomplete group (user deleted siblings) — leave for plain
      // LWPOLYLINE export. Don't mark consumed.
      continue;
    }
    // Sort by elementIndex.
    std::sort(entries.begin(), entries.end(),
              [](const MLineEntry &a, const MLineEntry &b) {
                return a.elementIndex < b.elementIndex;
              });
    // Find the anchor (elementIndex == 0).
    const MLineEntry *anchor = nullptr;
    for (const auto &m : entries) {
      if (m.elementIndex == 0 && m.isAnchor && !m.baselineVerts.empty()) {
        anchor = &m;
        break;
      }
    }
    if (!anchor) {
      // No anchor found — group is malformed, fall through.
      continue;
    }

    // Build DRW_MLine from anchor metadata + baseline vertices.
    DRW_MLine ml;
    ml.styleName = anchor->styleName.toStdString();
    ml.scale = anchor->scale;
    ml.justification = static_cast<duint8>(anchor->justification);
    ml.openClosed = anchor->openClosed;
    ml.numLines = static_cast<duint8>(N);
    ml.numVerts = static_cast<duint16>(anchor->baselineVerts.size());
    if (!anchor->baselineVerts.empty()) {
      ml.basePoint = anchor->baselineVerts.front();
    }
    ml.layer = entries.front().entity->getLayer()
                   ? entries.front().entity->getLayer()->getName().toStdString()
                   : std::string{"0"};
    const size_t V = anchor->baselineVerts.size();
    const size_t M = anchor->miterDirs.size();
    ml.vertlist.reserve(V);
    for (size_t i = 0; i < V; ++i) {
      DRW_MLineVertex v;
      v.position = anchor->baselineVerts[i];
      if (i < M)
        v.miterDir = anchor->miterDirs[i];
      // Default segment params: a single zero parameter per element
      // (sufficient for a continuous unbroken multiline).
      v.segParms.assign(N, std::vector<double>{0.0});
      v.areaFillParms.assign(N, std::vector<double>{});
      ml.vertlist.push_back(std::move(v));
    }

    if (m_dwgW)
      m_dwgW->writeMLine(&ml);
    else if (m_dxfW)
      m_dxfW->writeMLine(&ml);
    for (const auto &m : entries)
      consumed.insert(m.entity);
  }
}

void RS_FilterDXFRW::writeEntity(RS_Entity* e){
    switch (e->rtti()) {
    case RS2::EntityPoint:
        writePoint(static_cast<RS_Point*>(e));
        break;
    case RS2::EntityLine:
        writeLine(static_cast<RS_Line*>(e));
        break;
    case RS2::EntityCircle:
        writeCircle(static_cast<RS_Circle*>(e));
        break;
    case RS2::EntityArc:
        writeArc(static_cast<RS_Arc*>(e));
        break;
    case RS2::EntitySolid:
        writeSolid(static_cast<RS_Solid*>(e));
        break;
    case RS2::EntityEllipse:
        writeEllipse(static_cast<RS_Ellipse*>(e));
        break;
    case RS2::EntityHyperbola:
        writeHyperbola(static_cast<LC_Hyperbola*>(e));
        break;
    case RS2::EntityPolyline:
        writeLWPolyline(static_cast<RS_Polyline*>(e));
        break;
    case RS2::EntitySpline:
        writeSpline(static_cast<RS_Spline*>(e));
        break;
    case RS2::EntitySplinePoints:
        writeSplinePoints(static_cast<LC_SplinePoints*>(e));
        break;
    case RS2::EntityParabola:
        writeParabola(static_cast<LC_Parabola*>(e));
        break;
//    case RS2::EntityVertex:
//        break;
    case RS2::EntityInsert:
        writeInsert(static_cast<RS_Insert*>(e));
        break;
    case RS2::EntityMText:
        writeMText(static_cast<RS_MText*>(e));
        break;
    case RS2::EntityText:
        writeText(static_cast<RS_Text*>(e));
        break;
    case RS2::EntityDimLinear:
    case RS2::EntityDimOrdinate:
    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimRadial:
    case RS2::EntityDimDiametric:
        writeDimension(static_cast<RS_Dimension*>(e));
        break;
    case RS2::EntityDimLeader:
        writeLeader(static_cast<RS_Leader*>(e));
        break;
    case RS2::EntityTolerance:
        writeTolerance(static_cast<LC_Tolerance*>(e));
        break;
    case RS2::EntityHatch:
        writeHatch(static_cast<RS_Hatch*>(e));
        break;
    case RS2::EntityImage:
        writeImage(static_cast<RS_Image*>(e));
        break;
    case RS2::EntityWipeout:
      writeWipeout(static_cast<LC_Wipeout *>(e));
      break;
    case RS2::EntityMLeader:
      writeMLeader(static_cast<LC_MLeader *>(e));
      break;
    default:
        break;
    }
}

void RS_FilterDXFRW::reconstructUnderlays(RS_EntityContainer *container,
                                          std::set<RS_Entity *> &consumed) {
  if (!container)
    return;
  // Single-pass: each polyline carrying LibreCAD_UNDERLAY XDATA
  // reconstructs ONE DRW_Underlay (no group/sibling matching like MLINE).
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{*container, RS2::ResolveNone}.entities()) {
    if (e->getFlag(RS2::FlagUndone))
      continue;
    if (e->rtti() != RS2::EntityPolyline)
      continue;
    if (!e->hasDrwExtData())
      continue;

    const auto &ext = e->getDrwExtData();
    bool inGroup = false;
    DRW_Underlay u;
    DRW_Coord position;
    bool gotPosition = false;
    int seen1000 = 0; // 0=id, 1=kind
    int seen1040 = 0; // 0=scale.x, 1=scale.y, 2=rotation
    int seen1070 = 0; // 0=flags, 1=contrast, 2=fade

    for (const auto &sp : ext) {
      if (!sp)
        continue;
      const int code = sp->code();
      if (code == 1001) {
        inGroup = (std::string{sp->c_str()} == "LibreCAD_UNDERLAY");
        continue;
      }
      if (!inGroup)
        continue;
      switch (code) {
      case 1000: {
        const std::string s = sp->c_str();
        if (seen1000 == 1) {
          // kind
          if (s == "DGN")
            u.kind = DRW_Underlay::DGN;
          else if (s == "DWF")
            u.kind = DRW_Underlay::DWF;
        }
        ++seen1000;
        break;
      }
      case 1071:
        u.definitionHandle = static_cast<duint32>(sp->i_val());
        break;
      case 1010: {
        const auto *c = sp->coord();
        if (c) {
          position = *c;
          gotPosition = true;
        }
        break;
      }
      case 1040: {
        const double d = sp->d_val();
        if (seen1040 == 0)
          u.scale.x = d;
        else if (seen1040 == 1)
          u.scale.y = d;
        else if (seen1040 == 2)
          u.rotation = d;
        ++seen1040;
        break;
      }
      case 1070: {
        const int v = static_cast<int>(sp->i_val());
        if (seen1070 == 0)
          u.flags = static_cast<duint8>(v);
        else if (seen1070 == 1)
          u.contrast = static_cast<duint8>(v);
        else if (seen1070 == 2)
          u.fade = static_cast<duint8>(v);
        ++seen1070;
        break;
      }
      default:
        break;
      }
    }

    if (!gotPosition)
      continue; // not a real LibreCAD_UNDERLAY block
    u.position = position;

    // Reverse-transform polyline vertices back to OCS clip coordinates:
    //   poly_v = position + R(rotation) * scale * clip_v
    // → clip_v = scale^-1 * R^-T * (poly_v - position)
    const auto *pl = static_cast<const RS_Polyline *>(e);
    const double cs = std::cos(u.rotation);
    const double sn = std::sin(u.rotation);
    const double sx = (std::abs(u.scale.x) > RS_TOLERANCE) ? u.scale.x : 1.0;
    const double sy = (std::abs(u.scale.y) > RS_TOLERANCE) ? u.scale.y : 1.0;
    u.clipBoundary.clear();
    for (RS_Entity *sub : *pl) {
      // Polyline sub-entities are line segments; first endpoint of each
      // gives the polygon vertex.
      const RS_Vector p = sub->getStartpoint();
      const double dx = p.x - u.position.x;
      const double dy = p.y - u.position.y;
      const double rx = (dx * cs + dy * sn) / sx;
      const double ry = (-dx * sn + dy * cs) / sy;
      u.clipBoundary.emplace_back(rx, ry, 0.0);
    }

    m_dxfW->writeUnderlay(&u);
    consumed.insert(e);
  }
}

/**
 * Writes the given Point entity to the file.
 */
void RS_FilterDXFRW::writePoint(RS_Point* p) {
    DRW_Point point;
    getEntityAttributes(&point, p);
    point.basePoint.x = p->getStartpoint().x;
    point.basePoint.y = p->getStartpoint().y;
    if (m_dwgW) { m_dwgW->writePoint(&point); return; }
    m_dxfW->writePoint(&point);
}

/**
 * Writes the given Line( entity to the file.
 */
void RS_FilterDXFRW::writeLine(RS_Line* l) {
    DRW_Line line;
    getEntityAttributes(&line, l);
    line.basePoint.x = l->getStartpoint().x;
    line.basePoint.y = l->getStartpoint().y;
    line.secPoint.x = l->getEndpoint().x;
    line.secPoint.y = l->getEndpoint().y;
    if (m_dwgW) { m_dwgW->writeLine(&line); return; }
    m_dxfW->writeLine(&line);
}

/**
 * Writes the given circle entity to the file.
 */
void RS_FilterDXFRW::writeCircle(RS_Circle* c) {
    DRW_Circle circle;
    getEntityAttributes(&circle, c);
    circle.basePoint.x = c->getCenter().x;
    circle.basePoint.y = c->getCenter().y;
    circle.radious = c->getRadius();
    if (m_dwgW) { m_dwgW->writeCircle(&circle); return; }
    m_dxfW->writeCircle(&circle);
}

/**
 * Writes the given arc entity to the file.
 */
void RS_FilterDXFRW::writeArc(RS_Arc* a) {
    DRW_Arc arc;
    getEntityAttributes(&arc, a);
    arc.basePoint.x = a->getCenter().x;
    arc.basePoint.y = a->getCenter().y;
    arc.radious = a->getRadius();
    if (a->isReversed()) {
        arc.staangle = a->getAngle2();
        arc.endangle = a->getAngle1();
    } else {
        arc.staangle = a->getAngle1();
        arc.endangle = a->getAngle2();
    }
    if (m_dwgW) { m_dwgW->writeArc(&arc); return; }
    m_dxfW->writeArc(&arc);
}

/**
 * Writes the given polyline entity to the file as lwpolyline.
 */
void RS_FilterDXFRW::writeLWPolyline(RS_Polyline* l) {
    //skip if are empty polyline
    if (l->isEmpty()) {
        return;
    }
    // version 12 are old style polyline
    if (m_version == 1009) {
        writePolyline(l);
        return;
    }
    bool has_ellipse = false;
    for (RS_Entity* e=l->firstEntity(RS2::ResolveNone); e; e=l->nextEntity(RS2::ResolveNone)) {
        if (e->rtti() == RS2::EntityEllipse) {
            has_ellipse = true;
            break;
        }
    }
    if (has_ellipse) {
        writePolyline(l);
        return;
    }
    DRW_LWPolyline pol;
    RS_Entity* currEntity = nullptr;

    RS_AtomicEntity* ae = nullptr;
    double bulge = 0.0;

    lc::LC_ContainerTraverser traverser{*l, RS2::ResolveNone};
    for (RS_Entity* e = traverser.first(); e != nullptr; e = traverser.next()) {
        currEntity = e;
        // nextEntity = traverser.next();

        if (!e->isAtomic()) {
            continue;
        }
        ae = static_cast<RS_AtomicEntity*>(e);

        // Write vertex:
        if (e->rtti() == RS2::EntityArc) {
            bulge = static_cast<RS_Arc*>(e)->getBulge();
        }
        else {
            bulge = 0.0;
        }
        pol.addVertex(DRW_Vertex2D(ae->getStartpoint().x, ae->getStartpoint().y, bulge));
    }
    if (l->isClosed()) {
        pol.flags = 1;
    }
    else {
        ae = static_cast<RS_AtomicEntity*>(currEntity);
        if (ae->rtti() == RS2::EntityArc) {
            bulge = static_cast<RS_Arc*>(ae)->getBulge();
        }
        pol.addVertex(DRW_Vertex2D(ae->getEndpoint().x, ae->getEndpoint().y, bulge));
    }
    pol.vertexnum = pol.vertlist.size();
    getEntityAttributes(&pol, l);
    auto lwMeta = extractLWPolylineMeta(l);
    if (lwMeta) {
        pol.width = lwMeta->width;
        pol.elevation = lwMeta->elevation;
        pol.thickness = lwMeta->thickness;
        pol.extPoint = lwMeta->extrusion;
        if (lwMeta->vertexCount == static_cast<int>(pol.vertlist.size())) {
            for (size_t i = 0; i < pol.vertlist.size(); ++i) {
                if (i < lwMeta->startWidths.size())
                    pol.vertlist[i]->stawidth = lwMeta->startWidths[i];
                if (i < lwMeta->endWidths.size())
                    pol.vertlist[i]->endwidth = lwMeta->endWidths[i];
                if (i < lwMeta->identifiers.size())
                    pol.vertlist[i]->identifier = lwMeta->identifiers[i];
            }
        }
    }
    if (m_dwgW) { m_dwgW->writeLWPolyline(&pol); return; }
    m_dxfW->writeLWPolyline(&pol);
}

/**
 * Writes the given polyline entity to the file (old style).
 */
void RS_FilterDXFRW::writePolyline(RS_Polyline* p) {
    if (p == nullptr)
        return;

    DRW_Polyline pol;
    if (p->isClosed()) {
        pol.flags = 1;
    }

    RS_Entity* nextEntity = nullptr;
    for (RS_Entity* e=p->firstEntity(RS2::ResolveNone); e != nullptr; e=nextEntity) {
        nextEntity = p->nextEntity(RS2::ResolveNone);

        if (!e->isAtomic()) {
            continue;
        }
        RS_AtomicEntity* ae = static_cast<RS_AtomicEntity*>(e);

        // Write vertex:
        double bulge=0.0;
        bool isElliptic = false;
        double yRadius = 0.0;
        switch(e->rtti()) {
        case RS2::EntityLine:
            break;
        case RS2::EntityArc:
            bulge = ((RS_Arc*)e)->getBulge();
            break;
        case RS2::EntityEllipse: {
            // Issue #1946: prepare to write elliptic arcs as RS_Arc
            RS_Ellipse* ellipse = static_cast<RS_Ellipse*>(e);
            auto pair = RS_Polyline::convertToArcPair(ellipse);
            std::unique_ptr<RS_Arc> arc{ pair.first };
            bulge = arc->getBulge();
            yRadius = arc->getRadius() * pair.second;
            isElliptic = true;
        }
            break;
        default:
            // should not happen: unknown entity type
            continue;
        }
        pol.addVertex( DRW_Vertex(ae->getStartpoint().x,
                                 ae->getStartpoint().y, 0.0, bulge));
        if (isElliptic) {
            // Add flag to indicate the vertex should be elliptic
            pol.vertlist.back()->extData.push_back(std::make_shared<DRW_Variant>(1001, "LibreCad"));
            // Keep to ellipse minor radius to allow elliptic creation at loading
            pol.vertlist.back()->extData.push_back(std::make_shared<DRW_Variant>(1040, yRadius));
        }
    }
    getEntityAttributes(&pol, p);
    if (m_dwgW) { m_dwgW->writePolyline(&pol); return; }
    m_dxfW->writePolyline(&pol);
}

/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXFRW::writeSpline(RS_Spline *s) {
    if (s==nullptr) {
        return;
    }

    if (s->getNumberOfControlPoints() < size_t(s->getDegree()+1)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_FilterDXF::writeSpline: "
                                           "Discarding spline: not enough control points given.");
        return;
    }

    // version 12 do not support Spline write as polyline
    if (m_version==1009) {
        DRW_Polyline pol;
        for(RS_Entity* e: lc::LC_ContainerTraverser{*s, RS2::ResolveNone}.entities()) {
            pol.addVertex( DRW_Vertex(e->getStartpoint().x,
                                     e->getStartpoint().y, 0.0, 0.0));
        }
        if (s->isClosed()) {
            pol.flags = 1;
        } else {
            pol.addVertex( DRW_Vertex(s->getEndpoint().x,s->getEndpoint().y, 0.0, 0.0));
        }
        getEntityAttributes(&pol, s);
        m_dxfW->writePolyline(&pol);
        return;
    }

    DRW_Spline sp{};

    // dxf spline group code=70
    // bit coded: 1: closed; 2: periodic; 4: rational; 8: planar; 16:linear
    sp.flags = (s->isClosed()) ? 0b1011 : 0b1000;

    // write spline control points:
    for (const RS_Vector& v: s->getUnwrappedControlPoints()) {
        sp.controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
    }
    sp.weightlist = s->getUnwrappedWeights();
    if (std::any_of(sp.weightlist.begin(), sp.weightlist.end(), differsFromUnitWeight)) {
        sp.flags |= 0x04;
    }

    sp.ncontrol = sp.controllist.size();
    sp.degree = s->getDegree();

    // knot vector from RS_Spline
    sp.knotslist = s->getUnwrappedKnotVector();
    sp.nknots = sp.knotslist.size();

    getEntityAttributes(&sp, s);
    if (m_dwgW) { m_dwgW->writeSpline(&sp); return; }
    m_dxfW->writeSpline(&sp);
}

/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXFRW::writeSplinePoints(LC_SplinePoints *s){
	int nCtrls = s->getNumberOfControlPoints();
	auto const& cp = s->getControlPoints();

	if(nCtrls < 3){
		if(nCtrls > 1){
			DRW_Line line;
			line.basePoint.x = cp.at(0).x;
			line.basePoint.y = cp.at(0).y;
			line.secPoint.x = cp.at(1).x;
			line.secPoint.y = cp.at(1).y;
			getEntityAttributes(&line, s);
			if (m_dwgW) { m_dwgW->writeLine(&line); return; }
			m_dxfW->writeLine(&line);
		}
		return;
	}

	// version 12 do not support Spline write as polyline
	if(m_version == 1009){
		DRW_Polyline pol;
		auto const& sp = s->getStrokePoints();

		for(size_t i = 0; i < sp.size(); i++){
			pol.addVertex(DRW_Vertex(sp.at(i).x, sp.at(i).y, 0.0, 0.0));
		}

		if (s->isClosed()) {
			pol.flags = 1;
		}

		getEntityAttributes(&pol, s);
		m_dxfW->writePolyline(&pol);
		return;
	}

	DRW_Spline sp;
	if (s->isClosed()) {
		sp.flags = 11;
	}
	else {
		sp.flags = 8;
	}

	LC_SplinePointsData &data = s->getData();
	auto const& fitPoints = data.splinePoints;
	const bool writeFitScenario = !data.useControlPoints && fitPoints.size() >= 2;
	sp.degree = 2;
	sp.nfit = static_cast<dint32>(fitPoints.size());

	if (writeFitScenario) {
		sp.m_scenario = 2;
		sp.m_knotParam = 0;
		sp.ncontrol = 0;
		sp.nknots = 0;
		sp.tolfit = 0.0000001;
		const RS_Vector startTangent = fitPoints[1] - fitPoints[0];
		const RS_Vector endTangent = fitPoints.back() - fitPoints[fitPoints.size() - 2];
		sp.tgStart = DRW_Coord{startTangent.x, startTangent.y, 0.0};
		sp.tgEnd = DRW_Coord{endTangent.x, endTangent.y, 0.0};
	} else {
		sp.ncontrol = nCtrls;
		sp.nknots = nCtrls + 3;
	}

	// write spline knots:
	if (!writeFitScenario) {
		for(int i = 1; i <= sp.nknots; i++){
			if(i <= 3){
				sp.knotslist.push_back(0.0);
			}
			else if(i <= nCtrls){
				sp.knotslist.push_back((i - 3.0)/(nCtrls - 2.0));
			}
			else{
				sp.knotslist.push_back(1.0);
			}
		}
	}

	// write spline control points:
	if (!writeFitScenario) {
		for (auto const& v : cp) {
			sp.controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
		}
	}

	// fit points
	for (auto const& v : fitPoints) {
		sp.fitlist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
	}

	getEntityAttributes(&sp, s);
	if (m_dwgW) {
		if (!writeFitScenario) {
			sp.fitlist.clear();
			sp.nfit = 0;
		}
		m_dwgW->writeSpline(&sp);
		return;
	}
	m_dxfW->writeSpline(&sp);
}

/**
 * Writes the given Ellipse entity to the file.
 */
void RS_FilterDXFRW::writeEllipse(RS_Ellipse* s) {
// version 12 do not support Ellipse but are
// converted in polyline by library
    DRW_Ellipse el;
    getEntityAttributes(&el, s);
    el.basePoint.x = s->getCenter().x;
    el.basePoint.y = s->getCenter().y;
    el.secPoint.x = s->getMajorP().x;
    el.secPoint.y = s->getMajorP().y;
    el.ratio = s->getRatio();
    if (s->isReversed()) {
        el.staparam = s->getAngle2();
        el.endparam = s->getAngle1();
    } else {
        el.staparam = s->getAngle1();
        el.endparam = s->getAngle2();
    }
    if (m_dwgW) { m_dwgW->writeEllipse(&el); return; }
    m_dxfW->writeEllipse(&el);
}

/**
 * Write a hyperbola entity as an exact rational quadratic SPLINE.
 *
 * Uses LC_HyperbolaSpline to create the standard SPLINE representation.
 */
void RS_FilterDXFRW::writeHyperbola(LC_Hyperbola* h) {
    if (h == nullptr || !h->isValid()) return;
    DRW_Spline spl;
    getEntityAttributes(&spl, h);
    if (LC_HyperbolaSpline::hyperbolaToSpline(h->getData(), spl)) {
        if (m_dwgW) { m_dwgW->writeSpline(&spl); return; }
        if (m_dxfW) m_dxfW->writeSpline(&spl);
    }
}

/**
 * Write a parabola entity as its canonical non-rational quadratic SPLINE.
 */
void RS_FilterDXFRW::writeParabola(LC_Parabola* p) {
    if (p == nullptr || !p->getData().isValid())
        return;

    DRW_Spline spl;
    getEntityAttributes(&spl, p);
    if (LC_ParabolaSpline::parabolaToSpline(p->getData(), spl)) {
        if (m_dwgW) { m_dwgW->writeSpline(&spl); return; }
        if (m_dxfW) m_dxfW->writeSpline(&spl);
    }
}

/**
 * Writes the given block insert entity to the file.
 */
void RS_FilterDXFRW::writeInsert(RS_Insert* i) {
    DRW_Insert in;
    getEntityAttributes(&in, i);
    in.basePoint.x = i->getInsertionPoint().x;
    in.basePoint.y = i->getInsertionPoint().y;
    in.basePoint.z = i->getInsertionPoint().z;
    in.name = i->getName().toUtf8().data();
    in.xscale = i->getScale().x;
    in.yscale = i->getScale().y;
    in.zscale = i->getScale().z;
    in.angle = i->getAngle();
    in.colcount = i->getCols();
    in.rowcount = i->getRows();
    in.colspace = i->getSpacing().x;
    in.rowspace =i->getSpacing().y;
    if (m_dwgW) { m_dwgW->writeInsert(&in); return; }
    m_dxfW->writeInsert(&in);
}

/**
 * Writes the given mText entity to the file.
 */
void RS_FilterDXFRW::writeMText(RS_MText* t) {
    DRW_Text *text;
    DRW_Text txt1;
    DRW_MText txt2;

    if (m_version==1009) {
        text = &txt1;
    }
    else {
        text = &txt2;
    }

    getEntityAttributes(text, t);
    text->basePoint.x = t->getInsertionPoint().x;
    text->basePoint.y = t->getInsertionPoint().y;
    text->height = t->getHeight();
    text->angle = t->getAngle()*180/M_PI;
    text->style = t->getStyle().toStdString();

    if (m_version==1009) {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->alignH =DRW_Text::HLeft;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->alignH =DRW_Text::HCenter;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->alignH = DRW_Text::HRight;
        }
        if (t->getVAlign()==RS_MTextData::VATop) {
            text->alignV = DRW_Text::VTop;
        } else if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->alignV = DRW_Text::VMiddle;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->alignV = DRW_Text::VBaseLine;
        }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList txtList = t->getText().split('\n',Qt::KeepEmptyParts);
#else
        QStringList txtList = t->getText().split('\n',QString::KeepEmptyParts);
#endif
        double dist = t->getLineSpacingFactor()*5*t->getHeight()/3;
        bool setSec = false;
        if (text->alignH != DRW_Text::HLeft || text->alignV != DRW_Text::VBaseLine) {
            text->secPoint.x = t->getInsertionPoint().x;
            text->secPoint.y = t->getInsertionPoint().y;
            setSec = true;
        }
        if (text->alignV == DRW_Text::VTop) {
            dist = dist * -1;
        }
        for (int i=0; i<txtList.size();++i){
            if (!txtList.at(i).isEmpty()) {
                text->text = toDxfString(txtList.at(i)).toUtf8().data();
				RS_Vector inc  = RS_Vector::polar(dist*i, t->getAngle()+M_PI_2);
                if (setSec) {
                    text->secPoint.x += inc.x;
                    text->secPoint.y += inc.y;
                } else {
                    text->basePoint.x += inc.x;
                    text->basePoint.y += inc.y;
                }
                m_dxfW->writeText(text);
            }
        }
    } else {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->textgen =1;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->textgen =2;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->textgen = 3;
        }
        if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->textgen += 3;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->textgen += 6;
        }
        // DXF MTEXT group 72: 1=LeftToRight, 3=TopToBottom, 5=ByStyle.
        // RightToLeft has no DXF encoding — fall through to LeftToRight so
        // other readers see a well-defined direction. The RTL preference is
        // round-tripped via XDATA (app id "LibreCad", code 1071) below; on
        // re-import we restore the flag if the marker is present, otherwise
        // the entity reads as LTR.
        switch (t->getDrawingDirection()) {
        case RS_MTextData::TopToBottom:
          text->alignH = static_cast<DRW_Text::HAlign>(3);
          break;
        case RS_MTextData::ByStyle:
          text->alignH = static_cast<DRW_Text::HAlign>(5);
          break;
        case RS_MTextData::LeftToRight:
        case RS_MTextData::RightToLeft:
        default:
          text->alignH = static_cast<DRW_Text::HAlign>(1);
          break;
        }
        if (t->getDrawingDirection() == RS_MTextData::RightToLeft) {
          text->extData.push_back(
              std::make_shared<DRW_Variant>(1001, std::string("LibreCad")));
          text->extData.push_back(
              std::make_shared<DRW_Variant>(1071, dint32{1}));
        }
                if (t->getLineSpacingStyle() == RS_MTextData::AtLeast) {
		    text->alignV = static_cast<DRW_Text::VAlign>(1);
		}
        else {
            text->alignV = static_cast<DRW_Text::VAlign>(2);
        }

        text->text = toDxfString(t->getText()).toUtf8().data();
        //        text->widthscale =t->getWidth();
        text->widthscale =t->getUsedTextWidth(); //getSize().x;
		txt2.interlin = t->getLineSpacingFactor();

        if (m_dwgW) { m_dwgW->writeMText(static_cast<DRW_MText*>(text)); return; }
        m_dxfW->writeMText(static_cast<DRW_MText*>(text));
    }
}

/**
 * Writes the given Text entity to the file.
 */
void RS_FilterDXFRW::writeText(RS_Text* t){
    DRW_Text text;

    getEntityAttributes(&text, t);
    text.basePoint.x = t->getInsertionPoint().x;
    text.basePoint.y = t->getInsertionPoint().y;
    text.height = t->getHeight();
    text.angle = t->getAngle()*180/M_PI;
    text.style = t->getStyle().toStdString();
    text.alignH =(DRW_Text::HAlign)t->getHAlign();
    text.alignV =(DRW_Text::VAlign)t->getVAlign();
    text.widthscale = t->getWidthRel();

    if (text.alignV != DRW_Text::VBaseLine || text.alignH != DRW_Text::HLeft) {
//    if (text.alignV != DRW_Text::VBaseLine || text.alignH == DRW_Text::HMiddle) {
//        if (text.alignH != DRW_Text::HLeft) {
        if (text.alignH == DRW_Text::HAligned || text.alignH == DRW_Text::HFit) {
            text.secPoint.x = t->getSecondPoint().x;
            text.secPoint.y = t->getSecondPoint().y;
        } else {
            text.secPoint.x = t->getInsertionPoint().x;
            text.secPoint.y = t->getInsertionPoint().y;
        }
    }

/*    if (text.alignH == DRW_Text::HAligned || text.alignH == DRW_Text::HFit) {
        text.secPoint.x = t->getSecondPoint().x;
        text.secPoint.y = t->getSecondPoint().y;
    }*/

    if (!t->getText().isEmpty()) {
        text.text = toDxfString(t->getText()).toUtf8().data();
        if (m_dwgW) { m_dwgW->writeText(&text); return; }
        m_dxfW->writeText(&text);
    }
}

/**
 * Writes the given dimension entity to the file.
 */
void RS_FilterDXFRW::writeDimension(RS_Dimension* d) {
    QString blkName;
    if (m_noNameBlock.contains(d)) {
        blkName = m_noNameBlock.take(d);
    }

    // version 12 are inserts of *D blocks
    if (m_version==1009) {
        if (!blkName.isEmpty()) {
            DRW_Insert in;
            getEntityAttributes(&in, d);
            in.basePoint.x = in.basePoint.y = 0.0;
            in.basePoint.z = 0.0;
            in.name = blkName.toStdString();
            in.xscale = in.yscale = 1.0;
            in.zscale = 1.0;
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            m_dxfW->writeInsert(&in);
        }
        return;
    }

    DRW_Dimension* dim;
    int attachmentPoint=1;
    if (d->getHAlign()==RS_MTextData::HALeft) {
        attachmentPoint=1;
    } else if (d->getHAlign()==RS_MTextData::HACenter) {
        attachmentPoint=2;
    } else if (d->getHAlign()==RS_MTextData::HARight) {
        attachmentPoint=3;
    }
    if (d->getVAlign()==RS_MTextData::VATop) {
        attachmentPoint+=0;
    } else if (d->getVAlign()==RS_MTextData::VAMiddle) {
        attachmentPoint+=3;
    } else if (d->getVAlign()==RS_MTextData::VABottom) {
        attachmentPoint+=6;
    }

    switch (d->rtti()) {
        case RS2::EntityDimAligned: {
            auto* da = static_cast<RS_DimAligned*>(d);
            auto dd = new DRW_DimAligned();
            dim = dd;
            dim->type = 1 + 32;
            dd->setDef1Point(DRW_Coord(da->getExtensionPoint1().x, da->getExtensionPoint1().y, 0.0));
            dd->setDef2Point(DRW_Coord(da->getExtensionPoint2().x, da->getExtensionPoint2().y, 0.0));
            break;
        }
        case RS2::EntityDimDiametric: {
            auto* dr = static_cast<RS_DimDiametric*>(d);
            auto dd = new DRW_DimDiametric();
            dim = dd;
            dim->type = 3 + 32;
            dd->setDiameter1Point(DRW_Coord(dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
            dd->setLeaderLength(dr->getLeader());
            break;
        }
        case RS2::EntityDimRadial: {
            auto* dr = static_cast<RS_DimRadial*>(d);
            auto* dd = new DRW_DimRadial();
            dim = dd;
            dim->type = 4 + 32;
            dd->setDiameterPoint(DRW_Coord(dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
            dd->setLeaderLength(dr->getLeader());
            break;
        }
        case RS2::EntityDimAngular: {
            auto* da = static_cast<RS_DimAngular*>(d);
            if (da->getDefinitionPoint3() == da->getData().definitionPoint) {
                auto* dd = new DRW_DimAngular3p();
                dim = dd;
                dim->type = 5 + 32;
                dd->setFirstLine  (DRW_Coord(da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, 0.0)); //13
                dd->setSecondLine (DRW_Coord(da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, 0.0)); //14
                dd->SetVertexPoint(DRW_Coord(da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, 0.0)); //15
                dd->setDimPoint   (DRW_Coord(da->getDefinitionPoint().x,  da->getDefinitionPoint().y,  0.0)); //10
            }
            else {
                auto* dd = new DRW_DimAngular();
                dim = dd;
                dim->type = 2 + 32;
                dd->setFirstLine1(DRW_Coord(da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, 0.0)); //13
                dd->setFirstLine2(DRW_Coord(da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, 0.0)); //14
                dd->setSecondLine1(DRW_Coord(da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, 0.0)); //15
                dd->setDimPoint(DRW_Coord(da->getDefinitionPoint4().x, da->getDefinitionPoint4().y, 0.0)); //16
            }
            break;
        }
        case RS2::EntityDimOrdinate: {
            auto* da = static_cast<LC_DimOrdinate*>(d);
            auto* dd = new DRW_DimOrdinate();
            dim = dd;
            dd->type = 6 + 32;
            auto dimOridinateData = da->getEData();
            if (dimOridinateData.ordinateForX) {
                dd->type = 6 + 64;
            }
            dd->setOriginPoint(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0));
            dd->setSecondLine(DRW_Coord(da->getLeaderEndPoint().x, da->getLeaderEndPoint().y, 0.0));
            dd->setFirstLine(DRW_Coord(da->getFeaturePoint().x, da->getFeaturePoint().y, 0.0));
            break;
        }
        case RS2::EntityDimArc: {
            auto* da = static_cast<LC_DimArc*>(d);
            auto* dd = new DRW_DimArc();
            dim = dd;
            RS_Vector centre = da->getCenter();
            double r = da->getRadius();
            double a0 = da->getStartAngle();
            double a1 = da->getEndAngle();
            double amid = (a0 + a1) / 2.0;
            dd->setArcCenter  (DRW_Coord(centre.x, centre.y, 0.));
            dd->setArcDefPoint(DRW_Coord(centre.x + r * std::cos(amid),
                                          centre.y + r * std::sin(amid), 0.));
            dd->setExtLine1   (DRW_Coord(centre.x + r * std::cos(a0),
                                          centre.y + r * std::sin(a0), 0.));
            dd->setExtLine2   (DRW_Coord(centre.x + r * std::cos(a1),
                                          centre.y + r * std::sin(a1), 0.));
            dd->arcStartAngle = a0;
            dd->arcEndAngle   = a1;
            dd->arcSymbol = da->getArcSymbol();
            dd->isPartial = da->getIsPartial();
            dd->hasLeader = da->getHasLeader();
            if (da->getLeaderPt1().valid)
                dd->setLeaderPt1(DRW_Coord(da->getLeaderPt1().x, da->getLeaderPt1().y, 0.));
            if (da->getLeaderPt2().valid)
                dd->leaderPt2 = DRW_Coord(da->getLeaderPt2().x, da->getLeaderPt2().y, 0.);
            break;
        }
        default: {
            //default to DimLinear
            auto dl = static_cast<RS_DimLinear*>(d);
            auto dd = new DRW_DimLinear();
            dim = dd;
            dim->type = 0 + 32;
            dd->setDef1Point(DRW_Coord(dl->getExtensionPoint1().x, dl->getExtensionPoint1().y, 0.0));
            dd->setDef2Point(DRW_Coord(dl->getExtensionPoint2().x, dl->getExtensionPoint2().y, 0.0));
            dd->setAngle(RS_Math::rad2deg(dl->getAngle()));
            dd->setOblique(dl->getOblique());
            break;
        }
    }
    getEntityAttributes(dim, d);
    dim->setDefPoint(DRW_Coord(d->getDefinitionPoint().x, d->getDefinitionPoint().y, 0));
    dim->setTextPoint(DRW_Coord(d->getMiddleOfText().x, d->getMiddleOfText().y, 0));
    dim->setStyle (d->getStyle().toUtf8().data());
    dim->setAlign (attachmentPoint);
    dim->setTextLineStyle(d->getLineSpacingStyle());
    dim->setText (toDxfString(d->getText()).toUtf8().data());
    dim->setTextLineFactor(d->getLineSpacingFactor());
    dim->setHDir(d->getHDir());
    dim->setFlipArrow1(d->isFlipArrow1());
    dim->setFlipArrow2(d->isFlipArrow2());
    if (d->hasUserDefinedTextLocation()) {
        dim->type = dim->type + 128;
    }
    if (!blkName.isEmpty()) {
        dim->setName(blkName.toStdString());
    }
    if (m_dwgW) { m_dwgW->writeDimension(dim); delete dim; return; }
    LC_DimStyle* override = d->getDimStyleOverride();
    if (override != nullptr) {
        LC_ExtEntityData extEntityData;
        addDimStyleOverrideToExtendedData(&extEntityData, override);
        fillEntityExtData(dim->extData, &extEntityData);
    }
    m_dxfW->writeDimension(dim);
    delete dim;
}

void RS_FilterDXFRW::writeTolerance(LC_Tolerance* t) {
    if (t == nullptr)
        return;

    DRW_Tolerance tol;
    getEntityAttributes(&tol, t);
    const LC_ToleranceData data = t->getData();
    tol.insertionPoint = DRW_Coord(data.m_insertionPoint.x,
                                   data.m_insertionPoint.y, 0.0);
    tol.xAxisDirectionVector = DRW_Coord(data.m_directionVector.x,
                                         data.m_directionVector.y, 0.0);
    tol.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    tol.text = toDxfString(data.m_textCode).toUtf8().constData();
    const QString style = data.m_dimStyleName.isEmpty()
        ? m_dimStyle
        : data.m_dimStyleName;
    tol.dimStyleName = style.toUtf8().constData();

    if (m_dwgW) {
        m_dwgW->writeTolerance(&tol);
        return;
    }
}

/**
 * Writes the given leader entity to the file.
 */
void RS_FilterDXFRW::writeLeader(RS_Leader* l) {
    if (l->count() <= 0) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "dropping leader with no vertices");
    }

    DRW_Leader leader;
    getEntityAttributes(&leader, l);
    leader.style = "Standard";
    leader.arrow = l->hasArrowHead();
    leader.leadertype = 0;
    leader.flag = 3;
    leader.hookline = 0;
    leader.hookflag = 0;
    leader.textheight = 1;
    leader.textwidth = 10;
    leader.vertnum = l->count();
	RS_Line* li =nullptr;
    for(RS_Entity* v: lc::LC_ContainerTraverser{*l, RS2::ResolveNone}.entities()){
        if (v->rtti()==RS2::EntityLine) {
            li = static_cast<RS_Line*>(v);
			leader.vertexlist.push_back(std::make_shared<DRW_Coord>(li->getStartpoint().x, li->getStartpoint().y, 0.0));
        }
    }
	if (li){
		leader.vertexlist.push_back(std::make_shared<DRW_Coord>(li->getEndpoint().x, li->getEndpoint().y, 0.0));
	}

    if (m_dwgW) {
        m_dwgW->writeLeader(&leader);
        return;
    }
    m_dxfW->writeLeader(&leader);
}

namespace {

// Emit an LC_SplinePoints boundary edge as a degree-2 DRW_Spline. Either
// the control net or the fit points are populated depending on the
// LC_SplinePointsData mode. Knotslist is left empty; the libdxfrw reader
// re-derives a clamped uniform knot vector when consuming this struct
// (see drw_entities.cpp parseDwg). See plan §B.1.
std::shared_ptr<DRW_Spline>
makeDrwSplineFromSplinePoints(const LC_SplinePoints *sp) {
  auto drw = std::make_shared<DRW_Spline>();
  drw->degree = 2;
  // Closed boundary edges set BOTH bit 0 (DXF entity-level "closed")
  // AND bit 1 (DWG-boundary / DXF group code 74 "periodic"). The hatch
  // boundary read path accepts either bit; this keeps cross-format
  // round-trip consistent.
  drw->flags = sp->isClosed() ? 0x3 : 0x0;
  const auto &d = sp->getData();
  if (d.useControlPoints && !d.controlPoints.empty()) {
    for (const auto &v : d.controlPoints)
      drw->controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y, 0.0));
    drw->ncontrol = static_cast<dint32>(d.controlPoints.size());
  } else {
    for (const auto &v : d.splinePoints)
      drw->fitlist.push_back(std::make_shared<DRW_Coord>(v.x, v.y, 0.0));
    drw->nfit = static_cast<dint32>(d.splinePoints.size());
  }
  return drw;
}

// Emit an RS_Spline (general NURBS, degree 1-3, possibly rational) boundary
// edge as a DRW_Spline carrying its full data. Mirrors weight into both
// drw->weightlist[i] AND drw->controllist[i]->z so both DXF and DWG
// consumers can read it correctly. See plan §C.3.
std::shared_ptr<DRW_Spline> makeDrwSplineFromRSSpline(const RS_Spline *sp) {
  auto drw = std::make_shared<DRW_Spline>();
  const auto &sd = sp->getData();
  drw->degree = static_cast<dint32>(sd.degree);

  const auto cps = sp->getControlPoints();
  const auto ws = sp->getWeights();
  const bool weightsAlign = (ws.size() == cps.size());
  const bool isRational =
      weightsAlign && std::any_of(ws.begin(), ws.end(), [](double w) {
        return std::abs(w - 1.0) > 1e-9;
      });
  // Set both DXF entity-level closed (bit 0) and hatch-boundary
  // periodic (bit 1) for cross-format consistency. See helper above.
  drw->flags = (sp->isClosed() ? 0x3 : 0x0) | (isRational ? 0x4 : 0x0);

  for (size_t i = 0; i < cps.size(); ++i) {
    const double w = (isRational && i < ws.size()) ? ws[i] : 1.0;
    drw->controllist.push_back(
        std::make_shared<DRW_Coord>(cps[i].x, cps[i].y, w));
  }
  drw->ncontrol = static_cast<dint32>(cps.size());
  if (isRational) {
    drw->weightlist = ws;
  }
  drw->knotslist = sd.knotslist;
  drw->nknots = static_cast<dint32>(sd.knotslist.size());
  return drw;
}

} // namespace

/**
 * Writes the given hatch entity to the file.
 */
void RS_FilterDXFRW::writeHatch(RS_Hatch * h) {
    // version 12 are inserts of *U blocks
    if (m_version==1009) {
        if (m_noNameBlock.contains(h)) {
            DRW_Insert in;
            getEntityAttributes(&in, h);
            in.basePoint.x = in.basePoint.y = 0.0;
            in.basePoint.z = 0.0;
            in.name = m_noNameBlock.value(h).toUtf8().data();
            in.xscale = in.yscale = 1.0;
            in.zscale = 1.0;
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            m_dxfW->writeInsert(&in);
        }
        return;
    }

    bool writeIt = true;
    if (h->countLoops()>0) {
        // check if all of the loops contain entities:
        for(RS_Entity* l: lc::LC_ContainerTraverser{*h, RS2::ResolveNone}.entities()){
            if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
                if (l->count()==0) {
                    writeIt = false;
                }
            }
        }
    } else {
        writeIt = false;
    }

    if (!writeIt) {
        RS_DEBUG->print(RS_Debug::D_WARNING,"RS_FilterDXF::writeHatch: Dropping Hatch");
        return;
    }

    DRW_Hatch ha;
    getEntityAttributes(&ha, h);
    ha.solid = h->isSolid();
    ha.scale = h->getScale();
    ha.angle = h->getAngle();
    if (ha.solid) {
        ha.name = "SOLID";
    }
    else {
        ha.name = h->getPattern().toUtf8().data();
    }
    ha.loopsnum = h->countLoops();

    for (RS_Entity* l=h->firstEntity(RS2::ResolveNone);
         l;
         l=h->nextEntity(RS2::ResolveNone)) {

        // Write hatch loops:
        if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
            auto loop = static_cast<RS_EntityContainer*>(l);
			std::shared_ptr<DRW_HatchLoop> lData = std::make_shared<DRW_HatchLoop>(0);

            for(RS_Entity* ed: lc::LC_ContainerTraverser{*loop, RS2::ResolveNone}.entities()){
                // Write hatch loop edges:
                if (ed->rtti()==RS2::EntityLine) {
                    auto* ln = static_cast<RS_Line*>(ed);
					std::shared_ptr<DRW_Line> line = std::make_shared<DRW_Line>();
                    line->basePoint.x = ln->getStartpoint().x;
                    line->basePoint.y = ln->getStartpoint().y;
                    line->secPoint.x = ln->getEndpoint().x;
                    line->secPoint.y = ln->getEndpoint().y;
                    lData->objlist.push_back(line);
                } else if (ed->rtti()==RS2::EntityArc) {
                    auto ar = static_cast<RS_Arc*>(ed);
					std::shared_ptr<DRW_Arc> arc = std::make_shared<DRW_Arc>();
                    arc->basePoint.x = ar->getCenter().x;
                    arc->basePoint.y = ar->getCenter().y;
                    arc->radious = ar->getRadius();
                    if (!ar->isReversed()) {
                        arc->staangle = ar->getAngle1();
                        arc->endangle = ar->getAngle2();
                        arc->isccw = true;
                    } else {
                        arc->staangle = 2*M_PI-ar->getAngle1();
                        arc->endangle = 2*M_PI-ar->getAngle2();
                        arc->isccw = false;
                    }
                    lData->objlist.push_back(arc);
                } else if (ed->rtti()==RS2::EntityCircle) {
                    auto ci = static_cast<RS_Circle*>(ed);
					std::shared_ptr<DRW_Arc> arc = std::make_shared<DRW_Arc>();
					arc->basePoint.x = ci->getCenter().x;
                    arc->basePoint.y = ci->getCenter().y;
                    arc->radious = ci->getRadius();
                    arc->staangle = 0.0;
                    arc->endangle = 2*M_PI; //2*M_PI;
                    arc->isccw = true;
                    lData->objlist.push_back(arc);
                } else if (ed->rtti()==RS2::EntityEllipse) {
                    auto el = static_cast<RS_Ellipse*>(ed);
					std::shared_ptr<DRW_Ellipse> ell = std::make_shared<DRW_Ellipse>();
                    ell->basePoint.x = el->getCenter().x;
                    ell->basePoint.y = el->getCenter().y;
                    ell->secPoint.x = el->getMajorP().x;
                    ell->secPoint.y = el->getMajorP().y;
                    ell->ratio = el->getRatio();
                    double rot = el->getMajorP().angle();
                    double startAng = el->getCenter().angleTo(el->getStartpoint()) - rot;
                    double endAng = el->getCenter().angleTo(el->getEndpoint()) - rot;
                    if (startAng < 0) {
                        startAng = M_PI*2 + startAng;
                    }
                    if (endAng < 0) {
                        endAng = M_PI*2 + endAng;
                    }
                    ell->staparam = startAng;
                    ell->endparam = endAng;
                    ell->isccw = !el->isReversed();
                    lData->objlist.push_back(ell);
                } else if (ed->rtti() == RS2::EntitySplinePoints) {
                  lData->objlist.push_back(makeDrwSplineFromSplinePoints(
                      static_cast<LC_SplinePoints *>(ed)));
                } else if (ed->rtti() == RS2::EntitySpline) {
                  lData->objlist.push_back(
                      makeDrwSplineFromRSSpline(static_cast<RS_Spline *>(ed)));
                }
            }
            lData->update(); //change to DRW_HatchLoop
            ha.appendLoop(lData);
        }
    }
    if (m_dwgW) { m_dwgW->writeHatch(&ha); return; }
    m_dxfW->writeHatch(&ha);
}

/**
 * Writes the given Solid entity to the file.
 */
void RS_FilterDXFRW::writeSolid(RS_Solid* s) {
    RS_SolidData data;
    DRW_Solid solid;
    RS_Vector corner;
    getEntityAttributes(&solid, s);
    corner = s->getCorner(0);
    solid.basePoint.x = corner.x;
    solid.basePoint.y = corner.y;
    corner = s->getCorner(1);
    solid.secPoint.x = corner.x;
    solid.secPoint.y = corner.y;
    corner = s->getCorner(2);
    solid.thirdPoint.x = corner.x;
    solid.thirdPoint.y = corner.y;
    if (s->isTriangle()) {
        solid.fourPoint.x = solid.thirdPoint.x;
        solid.fourPoint.y = solid.thirdPoint.y;
    } else {
        corner = s->getCorner(3);
        solid.fourPoint.x = corner.x;
        solid.fourPoint.y = corner.y;
    }
    if (m_dwgW) { m_dwgW->writeSolid(&solid); return; }
    m_dxfW->writeSolid(&solid);
}


void RS_FilterDXFRW::writeImage(RS_Image * i) {
    if (m_dwgW) return;
    DRW_Image image;
    getEntityAttributes(&image, i);

    image.basePoint.x = i->getInsertionPoint().x;
    image.basePoint.y = i->getInsertionPoint().y;
    image.secPoint.x = i->getUVector().x;
    image.secPoint.y = i->getUVector().y;
    image.vVector.x = i->getVVector().x;
    image.vVector.y = i->getVVector().y;
    image.sizeu = i->getWidth();
    image.sizev = i->getHeight();
    image.brightness = i->getBrightness();
    image.contrast = i->getContrast();
    image.fade = i->getFade();

    DRW_ImageDef *imgDef = m_dxfW->writeImage(&image, i->getFile().toUtf8().data());
	if (imgDef ) {
        imgDef->loaded = 1;
        imgDef->u = i->getData().size.x;
        imgDef->v = i->getData().size.y;
        imgDef->up = 1;
        imgDef->vp = 1;
        imgDef->resolution = 0;
    }
}

void RS_FilterDXFRW::writeWipeout(LC_Wipeout *w) {
  if (m_dwgW) return;
  if (w == nullptr) {
    return;
  }
  // LC_Wipeout stores the polygon already resolved to WCS, not as
  // image-pixel coords + basis.  On write we pick a trivial basis
  //     basePoint=(0,0), u=(1,0), v=(0,1), sizeU=sizeV=1
  // so that the inverse transform px = v.x - 0.5 / py = v.y - 0.5
  // (chosen so that the reader's `(p + 0.5) * size * axis + base` round-trips
  // back to v) yields exactly the original WCS vertices.  This trades
  // byte-identical round-trip of the original IMAGE-frame fields for a
  // simpler entity model in LibreCAD; the rendered geometry is preserved.
  DRW_Image img;
  getEntityAttributes(&img, w);
  img.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  img.secPoint = DRW_Coord(1.0, 0.0, 0.0);
  img.vVector = DRW_Coord(0.0, 1.0, 0.0);
  img.sizeu = 1.0;
  img.sizev = 1.0;
  img.clip = 1;
  img.brightness = 50;
  img.contrast = 50;
  img.fade = 0;
  img.clipMode = false; // 0 = mask outside the polygon (typical WIPEOUT)
  img.clipPath.clear();
  img.clipPath.reserve(w->getVertices().size());
  for (const RS_Vector &v : w->getVertices()) {
    img.clipPath.emplace_back(v.x - 0.5, v.y - 0.5);
  }
  m_dxfW->writeWipeout(&img);
}

/**
 * Serialize an LC_MLeader. DWG AC1024+ writes a native text-content
 * MULTILEADER subset with context roots, leader lines, and text content.
 * DXF keeps the older scalar/context writer. Unsupported complex content
 * still falls back to visible LEADER/MTEXT geometry on DWG export.
 */
void RS_FilterDXFRW::writeMLeader(LC_MLeader *m) {
  if (m == nullptr)
    return;
  DRW_MLeader e;
  getEntityAttributes(&e, m);
  const auto &d = m->getData();
  e.overrideFlags = 0; // Phase 9 doesn't track override bits;
                       // emit defaults so a re-read picks the
                       // entity-level values as authoritative.
  e.leaderType = d.leaderType;
  e.leaderColor = d.leaderColor;
  e.landingDistance = d.landingDistance;
  e.defaultArrowHeadSize = d.arrowSize;
  e.landingEnabled = d.landingEnabled;
  e.doglegEnabled = d.doglegEnabled;
  e.styleContentType = d.contentType;
  e.scaleFactor = d.scaleFactor;
  e.classVersion = 2;
  e.context.roots.reserve(d.roots.size());
  for (const auto &sourceRoot : d.roots) {
    DRW_MLeaderRoot root;
    root.isContentValid = sourceRoot.connectionPoint.valid;
    root.unknown291 = sourceRoot.direction.valid;
    root.connectionPoint = DRW_Coord(sourceRoot.connectionPoint.x,
                                     sourceRoot.connectionPoint.y,
                                     sourceRoot.connectionPoint.z);
    root.direction = DRW_Coord(sourceRoot.direction.x, sourceRoot.direction.y,
                               sourceRoot.direction.z);
    root.leaderIndex = static_cast<dint32>(e.context.roots.size());
    root.landingDistance = sourceRoot.landingDistance;
    root.attachmentDirection = static_cast<duint16>(sourceRoot.attachmentDirection);
    root.leaderLines.reserve(sourceRoot.leaderLines.size());
    for (const auto &sourceLine : sourceRoot.leaderLines) {
      DRW_MLeaderLeaderLine line;
      line.leaderLineIndex = sourceLine.leaderLineIndex;
      line.leaderType = static_cast<duint16>(d.leaderType);
      line.color = d.leaderColor;
      line.lineWeight = e.lWeight;
      line.arrowSize = d.arrowSize;
      line.overrideFlags = 0;
      line.points.reserve(sourceLine.points.size());
      for (const RS_Vector &point : sourceLine.points)
        line.points.emplace_back(point.x, point.y, point.z);
      root.leaderLines.push_back(std::move(line));
    }
    e.context.roots.push_back(std::move(root));
  }
  e.context.overallScale = d.scaleFactor;
  e.context.contentBasePoint = DRW_Coord(d.contentBasePoint.x,
                                         d.contentBasePoint.y,
                                         d.contentBasePoint.z);
  e.context.textHeight = d.textHeight > 0.0 ? d.textHeight : 1.0;
  e.context.arrowHeadSize = d.arrowSize;
  e.context.landingGap = d.landingDistance;
  e.context.hasTextContents = d.hasTextContents;
  e.context.textLabel = toDxfString(d.textLabel).toUtf8().data();
  e.context.textNormal = DRW_Coord(0.0, 0.0, 1.0);
  const RS_Vector textLocation =
      d.textLocation.valid ? d.textLocation : d.contentBasePoint;
  e.context.textLocation = DRW_Coord(textLocation.x, textLocation.y,
                                     textLocation.z);
  e.context.textDirection = DRW_Coord(std::cos(d.textRotation),
                                      std::sin(d.textRotation), 0.0);
  e.context.textRotation = d.textRotation;
  e.context.boundaryWidth = d.boundaryWidth;
  e.context.boundaryHeight = d.boundaryHeight;
  e.context.lineSpacingFactor = 1.0;
  e.context.lineSpacingStyle = 1;
  e.context.textColor = d.textColor;
  e.context.alignment = 1;
  e.context.flowDirection = 1;
  e.context.bgScaleFactor = 1.5;
  e.context.basePoint = DRW_Coord(d.basePoint.x, d.basePoint.y, d.basePoint.z);
  e.context.baseDirection = DRW_Coord(1.0, 0.0, 0.0);
  e.context.baseVertical = DRW_Coord(0.0, 1.0, 0.0);
  e.styleHandle.ref = d.dwgStyleHandle;
  e.leaderLineTypeHandle.ref = d.dwgLeaderLineTypeHandle;
  e.arrowHeadHandle.ref = d.dwgArrowHeadHandle;
  e.styleTextStyleHandle.ref = d.dwgTextStyleHandle;
  e.styleBlockHandle.ref = d.dwgBlockHandle;
  e.context.textStyleHandle.ref = d.dwgTextStyleHandle;
  e.context.blockTableRecordHandle.ref = d.dwgBlockHandle;
  for (DRW_MLeaderRoot &root : e.context.roots) {
    for (DRW_MLeaderLeaderLine &line : root.leaderLines) {
      line.lineTypeHandle.ref = d.dwgLeaderLineTypeHandle;
      line.arrowHandle.ref = d.dwgArrowHeadHandle;
    }
  }
  if (m_dwgW) {
    if (m_version >= 1024 && d.hasTextContents && !d.textLabel.isEmpty()
        && m_dwgW->writeMLeader(&e)) {
      return;
    }
    if (d.hasBlockContents) {
      RS_DEBUG->print(RS_Debug::D_WARNING,
                      "MLEADER block content is exported as leader fallback geometry; native block-content DWG writing is not implemented");
    }
    bool wroteGeometry = false;
    for (const auto &root : d.roots) {
      for (const auto &line : root.leaderLines) {
        if (line.points.size() < 2)
          continue;
        DRW_Leader leader;
        getEntityAttributes(&leader, m);
        leader.style = d.styleName.isEmpty() ? "Standard" : d.styleName.toStdString();
        leader.arrow = true;
        leader.leadertype = d.leaderType == 2 ? 1 : 0;
        leader.flag = 3;
        leader.hookline = 0;
        leader.hookflag = 0;
        leader.textheight = d.textHeight > 0.0 ? d.textHeight : 1.0;
        leader.textwidth = d.boundaryWidth > 0.0 ? d.boundaryWidth : 10.0;
        leader.vertnum = static_cast<int>(line.points.size());
        leader.vertexlist.reserve(line.points.size());
        for (const RS_Vector &point : line.points) {
          leader.vertexlist.push_back(
              std::make_shared<DRW_Coord>(point.x, point.y, point.z));
        }
        m_dwgW->writeLeader(&leader);
        wroteGeometry = true;
      }
    }
    if (d.hasTextContents && !d.textLabel.isEmpty()) {
      DRW_MText text;
      getEntityAttributes(&text, m);
      const RS_Vector insertion =
          d.textLocation.valid ? d.textLocation : d.contentBasePoint;
      text.basePoint.x = insertion.x;
      text.basePoint.y = insertion.y;
      text.basePoint.z = insertion.z;
      text.height = d.textHeight > 0.0 ? d.textHeight : 1.0;
      text.angle = d.textRotation * 180.0 / M_PI;
      text.style =
          d.textStyleName.isEmpty() ? "Standard" : d.textStyleName.toStdString();
      text.text = toDxfString(d.textLabel).toUtf8().data();
      text.widthscale = d.boundaryWidth;
      text.interlin = 1.0;
      m_dwgW->writeMText(&text);
      wroteGeometry = true;
    }
    if (!wroteGeometry) {
      RS_DEBUG->print(RS_Debug::D_WARNING,
                      "dropping MLEADER with no writable leader/text geometry");
    }
    return;
  }
  m_dxfW->writeMultiLeader(&e);
}

/*void RS_FilterDXFRW::writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                                        const DRW_Entity& attrib) {
    QString blkName;
    blkName = "__CE";

    // Creating an unique ID from the element ID
    int tmp, c=1; // tmp = temporary var c = counter var
    tmp = con->getId();

    while (true) {
        tmp = tmp/c;
        blkName.append((char) tmp %10 + 48);
        c *= 10;
        if (tmp < 10) {
            break;
        }
    }

    //Block definition
    dw.sectionTables();
    dxf.writeBlockRecord(dw);
    dw.dxfString(  0, "BLOCK_RECORD");

    dw.handle();
    dw.dxfHex(330, 1);
    dw.dxfString(100, "AcDbSymbolTableRecord");
    dw.dxfString(100, "AcDbBlockTableRecord");
    dw.dxfString(  2, blkName.toLatin1().data());
    dw.dxfHex(340, 0);
    dw.dxfString(0, "ENDTAB");

    //Block creation
    RS_BlockData blkdata(blkName, RS_Vector(0,0), false);

    RS_Block* blk = new RS_Block(graphic, blkdata);

	for (RS_Entity* e1 = con->firstEntity(); e1 ;
            e1 = con->nextEntity() ) {
        blk->addEntity(e1);
    }
    writeBlock(dw, blk);
    //delete e1;
}*/



/**
 * Writes the atomic entities of the given container to the file.
 */
/*void RS_FilterDXFRW::writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c,
                                       const DRW_Entity& attrib,
                                       RS2::ResolveLevel level) {

    for (RS_Entity* e=c->firstEntity(level);
            e;
            e=c->nextEntity(level)) {

        writeEntity(dw, e, attrib);
    }
}*/


/**
 * Sets the entities attributes according to the attributes
 * that come from a DXF file.
 */
void RS_FilterDXFRW::setEntityAttributes(RS_Entity* entity,
                                       const DRW_Entity* attrib) {
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes");

    RS_Pen pen;
    pen.setColor(Qt::black);
    pen.setLineType(RS2::SolidLine);
    QString layName = toNativeString(QString::fromUtf8(attrib->layer.c_str()));

    // Layer: add layer in case it doesn't exist:
    if (!m_graphic->findLayer(layName)) {
        DRW_Layer lay;
        lay.name = attrib->layer;
        addLayer(lay);
    }
    entity->setLayer(layName);

    // Color:
    RS_Color col;
    if (attrib->color24 >= 0) {
      col = RS_Color(attrib->color24 >> 16, attrib->color24 >> 8 & 0xFF,
                     attrib->color24 & 0xFF);
    }
    else {
      col = numberToColor(attrib->color);
    }
    if (!attrib->colorName.empty()) {
      col.setColorName(QString::fromUtf8(attrib->colorName.c_str()));
    }
    pen.setColor(col);

    // Linetype:
    pen.setLineType(nameToLineType( QString::fromUtf8(attrib->lineType.c_str()) ));

    // Width:
    pen.setWidth(numberToWidth(attrib->lWeight));

    // Transparency (DXF code 440): alpha_raw packs alpha_type in high byte
    // (0/1 = ByLayer/ByBlock, inherits parent), 3 = explicit alpha (low
    // byte 0..255). RS_Pen.alpha is a float 0..1 where 1.0 is opaque.
    // libreDWG common_entity_data.spec:432-446 documents the encoding.
    // Only override the pen default when alpha_type == 3.
    if (attrib->transparency != DRW::Opaque) {
      const unsigned int rawAlpha =
          static_cast<unsigned int>(attrib->transparency);
      const unsigned int alphaType = (rawAlpha >> 24) & 0xFF;
      const unsigned int alphaByte = rawAlpha & 0xFF;
      if (alphaType == 3) {
        pen.setAlpha(static_cast<float>(alphaByte) / 255.0f);
      }
    }

    entity->setPen(pen);

    // Passive metadata sidecars — DXF/DWG fields that don't affect
    // rendering or equality but must round-trip on save. Skip the
    // default sentinels so unset entities stay unchanged.
    if (attrib->material != DRW::MaterialByLayer)
      entity->setMaterialHandle(attrib->material);
    if (attrib->plotStyle != DRW::DefaultPlotStyle)
      entity->setPlotStyleHandle(static_cast<quint32>(attrib->plotStyle));
    if (attrib->shadow != DRW::CastAndReceieveShadows)
      entity->setShadowMode(static_cast<int>(attrib->shadow));
    // Visual-style handles (DWG R2010+ only; libdxfrw's DXF reader has no
    // codes for these, so they round-trip from DWG only).
    entity->setVisualStyleHandles(attrib->fullVisualStyleHandle,
                                  attrib->faceVisualStyleHandle,
                                  attrib->edgeVisualStyleHandle);

    // Preserve any XDATA / EED that came in with the entity. Stored
    // verbatim on RS_Entity so a later getEntityAttributes() can spit
    // it back out unchanged. The dimension-style override path reads
    // the same source independently and is unaffected.
    if (!attrib->extData.empty()) {
      entity->setDrwExtData(attrib->extData);
    }

    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes: OK");
}

/**
 * Gets the entities attributes as a DL_Attributes object.
 */
void RS_FilterDXFRW::getEntityAttributes(DRW_Entity* ent, const RS_Entity* entity) {
//DRW_Entity RS_FilterDXFRW::getEntityAttributes(RS_Entity* /*entity*/) {

    // Layer:
    RS_Layer* layer = entity->getLayer();
    QString layerName;
    if (layer) {
        layerName = layer->getName();
    } else {
        layerName = "0";
    }

    RS_Pen pen = entity->getPen(false);

    // Color:
    int exact_rgb;
    int color = colorToNumber(pen.getColor(), &exact_rgb);
    //printf("Color is: %s -> %d\n", pen.getColor().name().toLatin1().data(), color);

    // Linetype:
    QString lineType = lineTypeToName(pen.getLineType());

    // Width:
    DRW_LW_Conv::lineWidth width = widthToNumber(pen.getWidth());

    ent->layer = toDxfString(layerName).toUtf8().data();
    ent->color = color;
    ent->color24 = exact_rgb;
    if (pen.getColor().hasColorName()) {
      ent->colorName = pen.getColor().colorName().toUtf8().data();
    }
    ent->lWeight = width;
    ent->lineType = lineType.toUtf8().data();

    // Transparency export: encode pen alpha < 1.0 as DXF code 440
    // (alpha_raw). High byte 0x03 = "explicit alpha", low byte = 0..255.
    // Mirrors the import decoder above.
    if (pen.getAlpha() < 1.0f) {
      int alphaByte = static_cast<int>(pen.getAlpha() * 255.0f + 0.5f);
      ent->transparency = (0x03 << 24) | (alphaByte & 0xFF);
    }

    // Passive metadata sidecars — emit only when overridden.
    if (entity->materialHandle() != 0)
      ent->material = entity->materialHandle();
    if (entity->plotStyleHandle() != 0)
      ent->plotStyle = static_cast<int>(entity->plotStyleHandle());
    if (entity->shadowMode() != 0)
      ent->shadow = static_cast<DRW::ShadowMode>(entity->shadowMode());

    // Re-emit any XDATA / EED that was attached on import. Skipped if
    // the dimension-export path (or any other caller) already populated
    // ent->extData with its own structured override.
    if (entity->hasDrwExtData() && ent->extData.empty()) {
      ent->extData = entity->getDrwExtData();
    }
}

/**
 * @return Pen with the same attributes as 'attrib'.
 */
RS_Pen RS_FilterDXFRW::attributesToPen(const DRW_Layer* att) const {

    RS_Color col;
    if (att->color24 >= 0) {
        col = RS_Color(att->color24 >> 16,
                              att->color24 >> 8 & 0xFF,
                              att->color24 & 0xFF);
    }
    else {
        col = numberToColor(att->color);
    }
    if (!att->colorName.empty()) {
      col.setColorName(QString::fromUtf8(att->colorName.c_str()));
    }

    RS_Pen pen(col, numberToWidth(att->lWeight),
               nameToLineType(QString::fromUtf8(att->lineType.c_str())) );
    return pen;
}

/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the dxflib documentation for details.
 *
 * @param num Color number.
 */
RS_Color RS_FilterDXFRW::numberToColor(int num) {
        if (num==0) {
            return RS_Color(RS2::FlagByBlock);
        } else if (num==256) {
            return RS_Color(RS2::FlagByLayer);
        } else if (num<=255 && num>=0) {
            return RS_Color(DRW::dxfColors[num][0],
                            DRW::dxfColors[num][1],
                            DRW::dxfColors[num][2]);
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_FilterDXF::numberToColor: Invalid color number given.");
            return RS_Color(RS2::FlagByLayer);
        }

    return RS_Color();
}

/**
 * Converts a color into a color number in the DXF palette.
 * The color that fits best is chosen.
 */
int RS_FilterDXFRW::colorToNumber(const RS_Color& col, int *rgb) {
    //printf("Searching color for %s\n", col.name().toLatin1().data());
    *rgb = -1;
    // Special color BYBLOCK:
    if (col.getFlag(RS2::FlagByBlock)) {
        return 0;
    }
    // Special color BYLAYER
    else if (col.getFlag(RS2::FlagByLayer)) {
        return 256;
    }
    // Special color black is not in the table but white represents both
    // black and white
    else {
        int red = col.red();
        int green = col.green();
        int blue = col.blue();
        if (red==0 && green==0 && blue==0) {
            return 7;
        }
        // All other colors
        else {
            int num=0;
            int diff=255*3;  // smallest difference to a color in the table found so far

            // Run through the whole table and compare
            for (int i=1; i<=255; i++) {
                int d = abs(red-DRW::dxfColors[i][0])
                    + abs(green-DRW::dxfColors[i][1])
                    + abs(blue-DRW::dxfColors[i][2]);

                if (d<diff) {
                    /*
                printf("color %f,%f,%f is closer\n",
                       dxfColors[i][0],
                       dxfColors[i][1],
                       dxfColors[i][2]);
                */
                    diff = d;
                    num = i;
                    if (d==0) {
                        break;
                    }
                }
            }
            //printf("  Found: %d, diff: %d\n", num, diff);
            if(diff != 0) {
                *rgb = 0;
                *rgb = red<<16 | green<<8 | blue;
            }
            return num;
        }
    }
}

void RS_FilterDXFRW::add3dFace(const DRW_3Dface& data) {
    RS_DEBUG->print("RS_FilterDXFRW::add3dFace");
    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      !data.invisibleflag);
    auto *polyline = new RS_Polyline(m_currentContainer, d);
    setEntityAttributes(polyline, &data);
    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);
    RS_Vector v3(data.thirdPoint.x, data.thirdPoint.y);
    RS_Vector v4(data.fourPoint.x, data.fourPoint.y);

    polyline->addVertex(v1, 0.0);
    polyline->addVertex(v2, 0.0);
    polyline->addVertex(v3, 0.0);
    polyline->addVertex(v4, 0.0);

    m_currentContainer->addEntity(polyline);
}

void RS_FilterDXFRW::addComment(const char*) {
    RS_DEBUG->print("RS_FilterDXF::addComment(const char*) not yet implemented.");
}

void RS_FilterDXFRW::addPlotSettings(const DRW_PlotSettings *data) {
    m_graphic->setPagesNum(QString::fromStdString(data->plotViewName));
    m_graphic->setMargins(data->marginLeft, data->marginTop,
                        data->marginRight, data->marginBottom);
}

/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType RS_FilterDXFRW::nameToLineType(const QString& name) {

    QString uName = name.toUpper();

    // Standard linetypes for QCad II / AutoCAD
    if (uName.isEmpty() || uName=="BYLAYER") {
        return RS2::LineByLayer;
    }
    if (uName=="BYBLOCK") {
        return RS2::LineByBlock;
    }
    if (uName=="CONTINUOUS" || uName=="ACAD_ISO01W100") {
        return RS2::SolidLine;
    }
    if (uName=="ACAD_ISO07W100" || uName=="DOT") {
        return RS2::DotLine;
    }
    if (uName=="DOTTINY") {
        return RS2::DotLineTiny;
    }
    if (uName=="DOT2") {
        return RS2::DotLine2;
    }
    if (uName=="DOTX2") {
        return RS2::DotLineX2;
    }
    if (uName=="ACAD_ISO02W100" || uName=="ACAD_ISO03W100" ||
               uName=="DASHED" || uName=="HIDDEN") {
        return RS2::DashLine;
    }
    if (uName=="DASHEDTINY" || uName=="HIDDEN2") {
        return RS2::DashLineTiny;
    }
    if (uName=="DASHED2" || uName=="HIDDEN2") {
        return RS2::DashLine2;
    }
    if (uName=="DASHEDX2" || uName=="HIDDENX2") {
        return RS2::DashLineX2;
    }
    if (uName=="ACAD_ISO10W100" ||
               uName=="DASHDOT") {
        return RS2::DashDotLine;
    }
    if (uName=="DASHDOTTINY") {
        return RS2::DashDotLineTiny;
    }
    if (uName=="DASHDOT2") {
        return RS2::DashDotLine2;
    }
    if (uName=="ACAD_ISO04W100" ||
               uName=="DASHDOTX2") {
        return RS2::DashDotLineX2;
    }
    if (uName=="ACAD_ISO12W100" || uName=="DIVIDE") {
        return RS2::DivideLine;
    }
    if (uName=="DIVIDETINY") {
        return RS2::DivideLineTiny;
    }
    if (uName=="DIVIDE2") {
        return RS2::DivideLine2;
    }
    if (uName=="ACAD_ISO05W100" || uName=="DIVIDEX2") {
        return RS2::DivideLineX2;
    }
    if (uName=="CENTER") {
        return RS2::CenterLine;
    }
    if (uName=="CENTERTINY") {
        return RS2::CenterLineTiny;
    }
    if (uName=="CENTER2") {
        return RS2::CenterLine2;
    }
    if (uName=="CENTERX2") {
        return RS2::CenterLineX2;
    }
    if (uName=="BORDER") {
        return RS2::BorderLine;
    }
    if (uName=="BORDERTINY") {
        return RS2::BorderLineTiny;
    }
    if (uName=="BORDER2") {
        return RS2::BorderLine2;
    }
    if (uName=="BORDERX2") {
        return RS2::BorderLineX2;
    }

    return RS2::SolidLine;
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
QString RS_FilterDXFRW::lineTypeToName(RS2::LineType lineType) {
    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
        case RS2::SolidLine:
            return "CONTINUOUS";
        case RS2::DotLine:
            return "DOT";
        case RS2::DotLineTiny:
            return "DOTTINY";
        case RS2::DotLine2:
            return "DOT2";
        case RS2::DotLineX2:
            return "DOTX2";
        case RS2::DashLine:
            return "DASHED";
        case RS2::DashLineTiny:
            return "DASHEDTINY";
        case RS2::DashLine2:
            return "DASHED2";
        case RS2::DashLineX2:
            return "DASHEDX2";
        case RS2::DashDotLine:
            return "DASHDOT";
        case RS2::DashDotLineTiny:
            return "DASHDOTTINY";
        case RS2::DashDotLine2:
            return "DASHDOT2";
        case RS2::DashDotLineX2:
            return "DASHDOTX2";
        case RS2::DivideLine:
            return "DIVIDE";
        case RS2::DivideLineTiny:
            return "DIVIDETINY";
        case RS2::DivideLine2:
            return "DIVIDE2";
        case RS2::DivideLineX2:
            return "DIVIDEX2";
        case RS2::CenterLine:
            return "CENTER";
        case RS2::CenterLineTiny:
            return "CENTERTINY";
        case RS2::CenterLine2:
            return "CENTER2";
        case RS2::CenterLineX2:
            return "CENTERX2";
        case RS2::BorderLine:
            return "BORDER";
        case RS2::BorderLineTiny:
            return "BORDERTINY";
        case RS2::BorderLine2:
            return "BORDER2";
        case RS2::BorderLineX2:
            return "BORDERX2";
        case RS2::LineByLayer:
            return "ByLayer";
        case RS2::LineByBlock:
            return "ByBlock";
        default:
            break;
    }
    return "CONTINUOUS";
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
/*QString RS_FilterDXFRW::lineTypeToDescription(RS2::LineType lineType) {

    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
    case RS2::SolidLine:
        return "Solid line";
    case RS2::DotLine:
        return "ISO Dashed __ __ __ __ __ __ __ __ __ __ _";
    case RS2::DashLine:
        return "ISO Dashed with Distance __    __    __    _";
    case RS2::DashDotLine:
        return "ISO Long Dashed Dotted ____ . ____ . __";
    case RS2::DashDotDotLine:
        return "ISO Long Dashed Double Dotted ____ .. __";
    case RS2::LineByLayer:
        return "";
    case RS2::LineByBlock:
        return "";
    default:
        break;
    }

    return "CONTINUOUS";
}*/

/**
 * Converts a DRW_LW_Conv::lineWidth into a RS2::LineWidth.
 */
RS2::LineWidth RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::lineWidth lw) {
    switch (lw) {
        case DRW_LW_Conv::widthByLayer:
            return RS2::WidthByLayer;
        case DRW_LW_Conv::widthByBlock:
            return RS2::WidthByBlock;
        case DRW_LW_Conv::widthDefault:
            return RS2::WidthDefault;
        case DRW_LW_Conv::width00:
            return RS2::Width00;
        case DRW_LW_Conv::width01:
            return RS2::Width01;
        case DRW_LW_Conv::width02:
            return RS2::Width02;
        case DRW_LW_Conv::width03:
            return RS2::Width03;
        case DRW_LW_Conv::width04:
            return RS2::Width04;
        case DRW_LW_Conv::width05:
            return RS2::Width05;
        case DRW_LW_Conv::width06:
            return RS2::Width06;
        case DRW_LW_Conv::width07:
            return RS2::Width07;
        case DRW_LW_Conv::width08:
            return RS2::Width08;
        case DRW_LW_Conv::width09:
            return RS2::Width09;
        case DRW_LW_Conv::width10:
            return RS2::Width10;
        case DRW_LW_Conv::width11:
            return RS2::Width11;
        case DRW_LW_Conv::width12:
            return RS2::Width12;
        case DRW_LW_Conv::width13:
            return RS2::Width13;
        case DRW_LW_Conv::width14:
            return RS2::Width14;
        case DRW_LW_Conv::width15:
            return RS2::Width15;
        case DRW_LW_Conv::width16:
            return RS2::Width16;
        case DRW_LW_Conv::width17:
            return RS2::Width17;
        case DRW_LW_Conv::width18:
            return RS2::Width18;
        case DRW_LW_Conv::width19:
            return RS2::Width19;
        case DRW_LW_Conv::width20:
            return RS2::Width20;
        case DRW_LW_Conv::width21:
            return RS2::Width21;
        case DRW_LW_Conv::width22:
            return RS2::Width22;
        case DRW_LW_Conv::width23:
            return RS2::Width23;
        default:
            break;
    }
    return RS2::WidthDefault;
}

/**
 * Converts a RS2::LineWidth into an DRW_LW_Conv::lineWidth.
 */
DRW_LW_Conv::lineWidth RS_FilterDXFRW::widthToNumber(RS2::LineWidth width) {
    switch (width) {
        case RS2::WidthByLayer:
            return DRW_LW_Conv::widthByLayer;
        case RS2::WidthByBlock:
            return DRW_LW_Conv::widthByBlock;
        case RS2::WidthDefault:
            return DRW_LW_Conv::widthDefault;
        case RS2::Width00:
            return DRW_LW_Conv::width00;
        case RS2::Width01:
            return DRW_LW_Conv::width01;
        case RS2::Width02:
            return DRW_LW_Conv::width02;
        case RS2::Width03:
            return DRW_LW_Conv::width03;
        case RS2::Width04:
            return DRW_LW_Conv::width04;
        case RS2::Width05:
            return DRW_LW_Conv::width05;
        case RS2::Width06:
            return DRW_LW_Conv::width06;
        case RS2::Width07:
            return DRW_LW_Conv::width07;
        case RS2::Width08:
            return DRW_LW_Conv::width08;
        case RS2::Width09:
            return DRW_LW_Conv::width09;
        case RS2::Width10:
            return DRW_LW_Conv::width10;
        case RS2::Width11:
            return DRW_LW_Conv::width11;
        case RS2::Width12:
            return DRW_LW_Conv::width12;
        case RS2::Width13:
            return DRW_LW_Conv::width13;
        case RS2::Width14:
            return DRW_LW_Conv::width14;
        case RS2::Width15:
            return DRW_LW_Conv::width15;
        case RS2::Width16:
            return DRW_LW_Conv::width16;
        case RS2::Width17:
            return DRW_LW_Conv::width17;
        case RS2::Width18:
            return DRW_LW_Conv::width18;
        case RS2::Width19:
            return DRW_LW_Conv::width19;
        case RS2::Width20:
            return DRW_LW_Conv::width20;
        case RS2::Width21:
            return DRW_LW_Conv::width21;
        case RS2::Width22:
            return DRW_LW_Conv::width22;
        case RS2::Width23:
            return DRW_LW_Conv::width23;
        default:
            break;
    }
    return DRW_LW_Conv::widthDefault;
}

/**
 * Converts a native unicode string into a DXF encoded string.
 *
 * DXF endoding includes the following special sequences:
 * - %%%c for a diameter sign
 * - %%%d for a degree sign
 * - %%%p for a plus/minus sign
 */
QString RS_FilterDXFRW::toDxfString(const QString& str) {
    QString res = "";
    int j=0;
    for (int i=0; i<str.length(); ++i) {
        int c = str.at(i).unicode();
        if (c>175 || c<11){
            res.append(str.mid(j,i-j));
            j=i;

            switch (c) {
                case 0x0A:
                    res += "\\P";
                    break;
                // diameter:
                case 0x2205: //RLZ: Empty_set, diameter is 0x2300 need to add in all fonts
                case 0x2300:
                    res += "%%C";
                    break;
                // degree:
                case 0x00B0:
                    res += "%%D";
                    break;
                // plus/minus
                case 0x00B1:
                    res += "%%P";
                    break;
                default:
                    j--;
                    break;
            }
            j++;
        }
    }
    res.append(str.mid(j));
    return res;
}

/**
 * Converts a DXF encoded string into a native Unicode string.
 */
QString RS_FilterDXFRW::toNativeString(const QString& data) {
    QString res;

    // Ignore font tags:
    int j = 0;
    for (int i=0; i<data.length(); ++i) {
        if (data.at(i).unicode() == 0x7B){ //is '{' ?
            if (data.at(i+1).unicode() == 0x5c){ //and is "{\" ?
                //check known codes
                if ( (data.at(i+2).unicode() == 0x66) || //is "\f" ?
                     (data.at(i+2).unicode() == 0x48) || //is "\H" ?
                     (data.at(i+2).unicode() == 0x43)    //is "\C" ?
                   ) {
                    //found tag, append parsed part
                    res.append(data.mid(j,i-j));
                    qsizetype pos = data.indexOf(QChar(0x7D), i+3);//find '}'
                    if (pos <0) break; //'}' not found
                    QString tmp = data.mid(i+1, pos-i-1);
                    do {
                        tmp = tmp.remove(0,tmp.indexOf(QChar{0x3B}, 0)+1 );//remove to ';'
                    } while(tmp.startsWith("\\f") || tmp.startsWith("\\H") || tmp.startsWith("\\C"));
                    res.append(tmp);
                    i = j = pos;
                    ++j;
                }
            }
        }
    }
    res.append(data.mid(j));

    // Line feed:
    res = res.replace(QRegularExpression("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegularExpression("\\\\~"), " ");
    // Tab:
    res = res.replace(QRegularExpression("\\^I"), "    ");//RLZ: change 4 spaces for \t when mtext have support for tab
    // diameter:
    res = res.replace(QRegularExpression("%%[cC]"), QChar(0x2300));//RLZ: Empty_set is 0x2205, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegularExpression("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegularExpression("%%[pP]"), QChar(0x00B1));

    return res;
}

/**
 * Converts the given number from a DXF file into an AngleFormat enum.
 *
 * @param num $DIMAUNIT from DXF (0: decimal deg, 1: deg/min/sec, 2: gradians,
 *                                3: radians, 4: surveyor's units)
 *
 * @ret Matching AngleFormat enum value.
 */
RS2::AngleFormat RS_FilterDXFRW::numberToAngleFormat(int num) {
    switch (num) {
        default:
        case 0:
            return RS2::DegreesDecimal;
        case 1:
            return RS2::DegreesMinutesSeconds;
        case 2:
            return RS2::Gradians;
        case 3:
            return RS2::Radians;
        case 4:
            return RS2::Surveyors;
    }
}

/**
 * Converts AngleFormat enum to DXF number.
 */
int RS_FilterDXFRW::angleFormatToNumber(RS2::AngleFormat af) {
    switch (af) {
        default:
        case RS2::DegreesDecimal:
            return  0;
        case RS2::DegreesMinutesSeconds:
            return 1;
        case RS2::Gradians:
            return 2;
        case RS2::Radians:
            return 3;
        case RS2::Surveyors:
            return 4;
    }
}

/**
 * converts a DXF unit setting (e.g. INSUNITS) to a unit enum.
 */
RS2::Unit RS_FilterDXFRW::numberToUnit(int num) {
    switch (num) {
        default:
        case 0:
            return RS2::None;
        case 1:
            return RS2::Inch;
        case 2:
            return RS2::Foot;
        case 3:
            return RS2::Mile;
        case 4:
            return RS2::Millimeter;
        case 5:
            return RS2::Centimeter;
        case 6:
            return RS2::Meter;
        case 7:
            return RS2::Kilometer;
        case 8:
            return RS2::Microinch;
        case 9:
            return RS2::Mil;
        case 10:
            return RS2::Yard;
        case 11:
            return RS2::Angstrom;
        case 12:
            return RS2::Nanometer;
        case 13:
            return RS2::Micron;
        case 14:
            return RS2::Decimeter;
        case 15:
            return RS2::Decameter;
        case 16:
            return RS2::Hectometer;
        case 17:
            return RS2::Gigameter;
        case 18:
            return RS2::Astro;
        case 19:
            return RS2::Lightyear;
        case 20:
            return RS2::Parsec;
    }
    return RS2::None;
}

/**
 * Converts a unit enum into a DXF unit number e.g. for INSUNITS.
 */
int RS_FilterDXFRW::unitToNumber(RS2::Unit unit) {
    switch (unit) {
        default:
        case RS2::None:
            return 0;
        case RS2::Inch:
            return 1;
        case RS2::Foot:
            return 2;
        case RS2::Mile:
            return 3;
        case RS2::Millimeter:
            return 4;
        case RS2::Centimeter:
            return 5;
        case RS2::Meter:
            return 6;
        case RS2::Kilometer:
            return 7;
        case RS2::Microinch:
            return 8;
        case RS2::Mil:
            return 9;
        case RS2::Yard:
            return 10;
        case RS2::Angstrom:
            return 11;
        case RS2::Nanometer:
            return 12;
        case RS2::Micron:
            return 13;
        case RS2::Decimeter:
            return 14;
        case RS2::Decameter:
            return 15;
        case RS2::Hectometer:
            return 16;
        case RS2::Gigameter:
            return 17;
        case RS2::Astro:
            return 18;
        case RS2::Lightyear:
            return 19;
        case RS2::Parsec:
            return 20;
    }
    return 0;
}

/**
 * Checks if the given variable is two-dimensional (e.g. $LIMMIN).
 */
bool RS_FilterDXFRW::isVariableTwoDimensional(const QString& var) {
    if (var=="$LIMMIN" ||
            var=="$LIMMAX" ||
            var=="$PLIMMIN" ||
            var=="$PLIMMAX" ||
            var=="$GRIDUNIT" ||
            var=="$VIEWCTR") {

        return true;
    } else {
        return false;
    }
}

void RS_FilterDXFRW::addPolylineSegment(RS_Polyline& polyline, RS_Vector prev_pos, RS_Vector curr_pos, double bulge, const std::vector<std::shared_ptr<DRW_Variant>>& extData, bool isClosedSegment) {
    bool isLcData = false;
    double yRadius = 0.0;

    // Issue #1946: LibreCAD may use elliptic arcs internally, but dxf file saving
    // uses arcs only. Minor radius of elliptic is saved as DRW_Variant::content
    // The content is flagged as "LibreCAD"
    for (const std::shared_ptr<DRW_Variant>& var : extData) {
        if (var->code() == 1001) {
            isLcData = *(var->content.s) == "LibreCad";
        } else if (isLcData && var->code() == 1040) {
            yRadius = var->content.d;
        }
    }
    bool isElliptic = yRadius > RS_TOLERANCE;

    if (isElliptic) {
        std::unique_ptr<RS_Arc> arc{ RS_Polyline::arcFromBulge(prev_pos, curr_pos, bulge)};
        if (arc != nullptr && arc->getRadius() >= RS_TOLERANCE) {
            double radius = arc->getRadius();
            double scaleRatio = yRadius / radius;
            RS_Ellipse* ellipse = RS_Polyline::convertToEllipse(std::make_pair(arc.get(), scaleRatio));
            if (ellipse != nullptr) {
                ellipse->setParent(&polyline);
                ellipse->setSelected(polyline.isSelected());
                ellipse->setPen(RS_Pen(RS2::FlagInvalid));
                ellipse->setLayer(nullptr);
                polyline.addEntity(ellipse);
                polyline.getData().endpoint = curr_pos;
            }
        }
    } else {
        polyline.addVertex(curr_pos, bulge, false);
    }

    if (isClosedSegment) {
        polyline.setNextBulge(bulge);
    }
}

#ifdef DWGSUPPORT
QString RS_FilterDXFRW::printDwgVersion(int v){
    switch (v) {
        case DRW::AC1006:
            return "10";
        case DRW::AC1009:
            return "dwg version 11 or 12";
        case DRW::AC1012:
            return "dwg version 13";
        case DRW::AC1014:
            return "dwg version 14";
        case DRW::AC1015:
            return "dwg version 2000";
        case DRW::AC1018:
            return "dwg version 2004";
        case DRW::AC1021:
            return "dwg version 2007";
        case DRW::AC1024:
            return "dwg version 2010";
        case DRW::AC1027:
            return "dwg version 2013";
        case DRW::AC1032:
            return "dwg version 2018";
        default:
            return "unknown";
    }
}

void RS_FilterDXFRW::printDwgError(int le){
    switch (le) {
        case DRW::BAD_UNKNOWN:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("unknown error opening dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_UNKNOWN");
            break;
        case DRW::BAD_OPEN:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("can't open this dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_OPEN");
            break;
        case DRW::BAD_VERSION:
          if (m_dwgVersion != DRW::UNKNOWNV) {
            const QString actual = dwgVersionDisplay(m_dwgVersion);
            RS_DIALOGFACTORY->commandMessage(
                QObject::tr(
                    "Cannot open DWG: file is %1; LibreCAD supports %2 and "
                    "newer. "
                    "Convert with GNU LibreDWG (dwgread / dwg2dxf) or re-save "
                    "from a recent CAD tool.")
                    .arg(actual)
                    .arg(dwgVersionDisplay(DRW::AC1012)));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: BAD_VERSION (%s)",
                            actual.toUtf8().constData());
          } else {
            RS_DIALOGFACTORY->commandMessage(
                QObject::tr("unsupported dwg version"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_VERSION");
          }
            break;
        case DRW::BAD_READ_METADATA:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading file metadata in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_FILE_HEADER");
            break;
        case DRW::BAD_READ_FILE_HEADER:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading file header in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_FILE_HEADER");
            break;
        case DRW::BAD_READ_HEADER:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading header vars in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_HEADER");
            break;
        case DRW::BAD_READ_CLASSES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading classes in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_CLASSES");
            break;
        case DRW::BAD_READ_HANDLES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading offsets in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OFFSETS");
            break;
        case DRW::BAD_READ_TABLES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading tables in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_TABLES");
            break;
        case DRW::BAD_READ_BLOCKS:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading blocks in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OFFSETS");
            break;
        case DRW::BAD_READ_ENTITIES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading entities in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_ENTITIES");
            break;
        case DRW::BAD_READ_OBJECTS:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading objects in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OBJECTS");
            break;
        default:
            break;
    }
}

QString RS_FilterDXFRW::strVal(DRW_Variant* var) {
    return QString::fromUtf8(var->c_str());
}

LC_DimStyle *RS_FilterDXFRW::createDimStyle(const DRW_Dimstyle &s) {
    auto* result = new LC_DimStyle();
    QString name = QString::fromUtf8(s.name.c_str());
    result->setName(name);

    bool styleForEntityType = !result->isBaseStyle();
    if (styleForEntityType) {
        result->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
    }

    auto arrowStyle = result->arrowhead();

    DRW_Variant* var = s.get("$DIMBLK");
    if (var != nullptr) {
        arrowStyle->setSameBlockName(strVal(var));
    }
    var = s.get("$DIMBLK1");
    if (var != nullptr) {
        arrowStyle->setArrowHeadBlockNameFirst(strVal(var));
    }
    var = s.get("$DIMBLK2");
    if (var != nullptr) {
        arrowStyle->setArrowHeadBlockNameSecond(strVal(var));
    }
    var = s.get("$DIMASZ");
    if (var != nullptr) {
        arrowStyle->setSize(var->d_val());
    }
    var = s.get("$DIMTSZ");
    if (var != nullptr) {
        arrowStyle->setTickSize(var->d_val());
    }
    var = s.get("$DIMSAH");
    if (var != nullptr) {
        arrowStyle->setUseSeparateArrowHeads(var->i_val());
    }
    var = s.get("$DIMSOXD");
    if (var != nullptr) {
        arrowStyle->setSuppressionsRaw(var->i_val());
    }

    auto scaleStyle = result->scaling();
    var = s.get("$DIMSCALE");
    if (var != nullptr) {
        scaleStyle->setScale(var->d_val());
    }
    var = s.get("$DIMLFAC");
    if (var != nullptr) {
        scaleStyle->setLinearFactor(var->d_val());
    }

    auto extLineStyle = result->extensionLine();

    var = s.get("$DIMEXO");
    if (var != nullptr) {
        extLineStyle->setDistanceFromOriginPoint(var->d_val());
    }
    var = s.get("$DIMEXE");
    if (var != nullptr) {
        extLineStyle->setDistanceBeyondDimLine(var->d_val());
    }
    var = s.get("$DIMFXL");
    if (var != nullptr) {
        extLineStyle->setFixedLength(var->d_val());
    }
    var = s.get("$DIMFXLON");
    if (var != nullptr) {
        extLineStyle->setHasFixedLength(var->i_val()  == 1);
    }
    var = s.get("$DIMLWE");
    if (var != nullptr) {
        extLineStyle->setLineWidthRaw(var->i_val());
    }
    var = s.get("$DIMCLRE");
    if (var != nullptr) {
        extLineStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMSE1");
    if (var != nullptr) {
        extLineStyle->setSuppressFirstRaw(var->i_val());
    }
    var = s.get("$DIMSE2");
    if (var != nullptr) {
        extLineStyle->setSuppressSecondRaw(var->i_val());
    }
    var = s.get("$DIMLTEX1");
    if (var != nullptr) {
        auto dimltex1 = strVal(var);
        if (!dimltex1.isEmpty()) {
            extLineStyle->setLineTypeFirst(dimltex1);
        }
    }
    var = s.get("$DIMLTEX2");
    if (var != nullptr) {
        auto dimltex2 = strVal(var);
        if (!dimltex2.isEmpty()) {
            extLineStyle->setLineTypeSecond(dimltex2);
        }
    }

    auto dimLineStyle = result->dimensionLine();
    var = s.get("$DIMLWD");
    if (var != nullptr) {
        dimLineStyle->setLineWidthRaw(var->i_val());
    }
    var = s.get("$DIMDLE");
    if (var != nullptr) {
        dimLineStyle->setDistanceBeyondExtLinesForObliqueStroke(var->d_val());
    }
    var = s.get("$DIMDLI");
    if (var != nullptr) {
        dimLineStyle->setBaselineDimLinesSpacing(var->d_val());
    }
    var = s.get("$DIMGAP");
    if (var != nullptr) {
        dimLineStyle->setLineGap(var->d_val());
    }
    var = s.get("$DIMCLRD");
    if (var != nullptr) {
        dimLineStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMSD1");
    if (var != nullptr) {
        dimLineStyle->setSuppressFirstLineRaw(var->i_val());
    }
    var = s.get("$DIMSD2");
    if (var != nullptr) {
        dimLineStyle->setSuppressSecondLineRaw(var->i_val());
    }
    var = s.get("$DIMTOFL");
    if (var != nullptr) {
        dimLineStyle->setDrawPolicyForOutsideTextRaw(var->i_val());
    }
    var = s.get("$DIMLTYPE");
    if (var != nullptr) {
        auto dimltype = strVal(var);
        if (!dimltype.isEmpty()) {
            dimLineStyle->setLineType(dimltype);
        }
    }

    auto textStyle = result->text();
    var = s.get("$DIMTXT");
    if (var != nullptr) {
        textStyle->setHeight(var->d_val());
    }
    var = s.get("$DIMTXSTY");
    if (var != nullptr) {
        auto dimtxsty = strVal(var);
        prepareTextStyleName(dimtxsty);
        textStyle->setStyle(dimtxsty); // fixme - resolve via table?
    }
    var = s.get("$DIMTOH");
    if (var != nullptr) {
        textStyle->setOrientationOutsideRaw(var->i_val());
    }
    var = s.get("$DIMTIH");
    if (var != nullptr) {
        textStyle->setOrientationInsideRaw(var->i_val());
    }
    var = s.get("$DIMJUST");
    if (var != nullptr) {
        textStyle->setHorizontalPositioningRaw(var->i_val());
    }
    var = s.get("$DIMCLRT");
    if (var != nullptr) {
        textStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMTAD");
    if (var != nullptr) {
        textStyle->setVerticalPositioningRaw(var->i_val());
    }
    var = s.get("$DIMTIX");
    if (var != nullptr) {
        textStyle->setExtLinesRelativePlacementRaw(var->i_val());
    }
    var = s.get("$DIMTFILL");
    if (var != nullptr) {
        textStyle->setBackgroundFillModeRaw(var->i_val());
    }
    var = s.get("$DIMTFILLCLR");
    if (var != nullptr) {
        textStyle->setExplicitBackgroundFillColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMTXTDIRECTION");
    if (var != nullptr) {
        textStyle->setReadingDirectionRaw(var->i_val());
    }
    var = s.get("$DIMTVP");
    if (var != nullptr) {
        textStyle->setVerticalDistanceToDimLine(var->d_val());
    }
    var = s.get("$DIMUPT");
    if (var != nullptr) {
        textStyle->setCursorControlPolicyRaw(var->i_val());
    }
    var = s.get("$DIMTMOVE");
    if (var != nullptr) {
        textStyle->setPositionMovementPolicyRaw(var->i_val());
    }
    var = s.get("$DIMATFIT");
    if (var != nullptr) {
        textStyle->setUnsufficientSpacePolicyRaw(var->i_val());
    }

    auto zerosSuppression = result->zerosSuppression();

    var = s.get("$DIMZIN");
    if (var != nullptr) {
        zerosSuppression->setLinearRaw(var->i_val());
    }
    var = s.get("$DIMAZIN");
    if (var != nullptr) {
        zerosSuppression->setAngularRaw(var->i_val());
    }
    var = s.get("$DIMTZIN");
    if (var != nullptr) {
        zerosSuppression->setToleranceRaw(var->i_val());
    }
    var = s.get("$DIMALTZ");
    if (var != nullptr) {
        zerosSuppression->setAltLinearRaw(var->i_val());
    }
    var = s.get("$DIMALTTZ");
    if (var != nullptr) {
        zerosSuppression->setAltToleranceRaw(var->i_val());
    }

    auto linearFormat = result->linearFormat();

    var = s.get("$DIMLUNIT");
    if (var != nullptr) {
        linearFormat->setFormatRaw(var->i_val());
    }
    var = s.get("$DIMDSEP");
    if (var != nullptr) {
        linearFormat->setDecimalFormatSeparatorChar(var->i_val());
    }
    var = s.get("$DIMDEC");
    if (var != nullptr) {
        linearFormat->setDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMPOST");
    if (var != nullptr) {
        linearFormat->setPrefixOrSuffix(strVal(var));
    }
    var = s.get("$DIMALT");
    if (var != nullptr) {
        linearFormat->setAlternateUnitsRaw(var->i_val());
    }
    var = s.get("$DIMALTU");
    if (var != nullptr) {
        linearFormat->setAltFormatRaw(var->i_val());
    }
    var = s.get("$DIMALTD");
    if (var != nullptr) {
        linearFormat->setAltDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMALTF");
    if (var != nullptr) {
        linearFormat->setAltUnitsMultiplier(var->d_val());
    }
    var = s.get("$DIMAPOST");
    if (var != nullptr) {
        linearFormat->setAltPrefixOrSuffix(strVal(var));
    }

    auto angularFormat = result->angularFormat();
    var = s.get("$DIMAUNIT");
    if (var != nullptr) {
        angularFormat->setFormatRaw(var->i_val());
    }
    var = s.get("$DIMADEC");
    if (var != nullptr) {
        angularFormat->setDecimalPlaces(var->i_val());
    }

    auto linearRoundOff = result->roundOff();
    var = s.get("$DIMRND");
    if (var != nullptr) {
        linearRoundOff->setRoundToValue(var->d_val());
    }
    var = s.get("$DIMALTRND");
    if (var != nullptr) {
        linearRoundOff->setAltRoundToValue(var->d_val());
    }

    auto fractionStyle = result->fractions();
    var = s.get("$DIMFRAC");
    if (var != nullptr) {
        fractionStyle->setStyleRaw(var->i_val());
    }

    auto radialStyle = result->radial();
    var = s.get("$DIMCEN");
    if (var != nullptr) {
        radialStyle->setCenterMarkOrLineSize(var->d_val());
    }

    auto toleranceStyle = result->latteralTolerance();

    var = s.get("$DIMTDEC");
    if (var != nullptr) {
        toleranceStyle->setDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMALTTD");
    if (var != nullptr) {
        toleranceStyle->setDecimalPlacesAltDim(var->i_val());
    }
    var = s.get("$DIMTOL");
    if (var != nullptr) {
        toleranceStyle->setAppendTolerancesToDimText(var->i_val()); // fixme - review
    }
    var = s.get("$DIMTOLJ");
    if (var != nullptr) {
        toleranceStyle->setVerticalJustificationRaw(var->i_val());
    }
    var = s.get("$DIMTM");
    if (var != nullptr) {
        toleranceStyle->setLowerToleranceLimit(var->d_val()); // fixme - review
    }
    var = s.get("$DIMTP");
    if (var != nullptr) {
        toleranceStyle->setUpperToleranceLimit(var->d_val());
    }
    var = s.get("$DIMTFAC");
    if (var != nullptr) {
        toleranceStyle->setHeightScaleFactorToDimText(var->d_val());
    }
    var = s.get("$DIMLIM");
    if (var != nullptr) {
        toleranceStyle->setLimitsAreGeneratedAsDefaultText(var->i_val()); // fixme - review
    }

    auto leaderStyle = result->leader();
    var = s.get("$DIMLDRBLK");
    if (var != nullptr) {
        leaderStyle->setArrowBlockName(strVal(var));
    }

    auto arc = result->arc();
    var = s.get("$DIMARCSYM");
    if (var != nullptr) {
      arc->setArcSymbolPositionRaw(var->i_val());
    }

    // fixme - remove to MLeaderStyle
    /*auto mleaderStyle = result->mleader();
    var = s.get("$MLEADERSCALE");
    if (var != nullptr) {
        mleaderStyle->setScale(var->d_val());
    }*/
    // mleaderStyle->setScale(s.mleaderscale);

    parseDimStyleExtData(s, result);

    if (styleForEntityType) {
        result->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);
    }
    return result;
}

bool RS_FilterDXFRW::resolveBlockNameByHandle(duint32 blockHandle, QString& blockName) const {
    std::string name = m_dxfR->getReadingContext()->resolveBlockRecordName(blockHandle);
    if (name.empty()) {
        return false;
    }
    blockName = name.c_str();
    return true;
}

void RS_FilterDXFRW::parseDimStyleExtData(const DRW_Dimstyle& s, LC_DimStyle* result) {
    std::vector<DRW_Variant> tagData;
    int currentValType = 0;
    QString applicationName = "";
    bool expectType = false;
    // https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9
    // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-7943.htm
    for (auto v: s.extData) {
        int code = v->code();
        switch (code) {
            case 1001: { // application name
                if (!applicationName.isEmpty()) {
                    applyParsedDimStyleExtData(result, applicationName, tagData);
                    tagData.clear();
                }
                applicationName = v->c_str();
                expectType = false; // for later "not", as actually we do expect it
                break;
            }
            case 1002: { // control name
                break;
            }
            case 1070: // integer
            case 1071:{// long
                int val = v->i_val();
                if (expectType) {
                    // code of var
                    currentValType = val;
                }
                else {
                    // it fields
                    auto intVar = DRW_Variant(currentValType, val);
                    tagData.push_back(intVar);
                }
                break;
            }
            case 1040: // real
            case 1041: // distance
            case 1042: { // scale factor
                double val = v->d_val();
                auto doubleVar = DRW_Variant(currentValType, val);
                tagData.push_back(doubleVar);
                break;
            }
            default:
                break;
        }
        expectType = !expectType;
    }
    if (!applicationName.isEmpty()) { // process last app name setion
        applyParsedDimStyleExtData(result, applicationName, tagData);
        tagData.clear();
    }
}

void RS_FilterDXFRW::applyParsedDimStyleExtData(LC_DimStyle* dimStyle, const QString& appName, const std::vector<DRW_Variant>& vector) {
    if (vector.empty()) {
        return;
    }
    const DRW_Variant* var = &vector.at(0);
    int code = var->code();

    if (appName == "ACAD_DSTYLE_DIMJAG") {
        if (code == 388) {
            // double val = var->d_val();
            // fixme - decide where to store it. this is "Jog Height Factor"
            // dimStyle->dimensionLine()->set
        }

        // code 388, float
    }
    else if (appName == "ACAD_DSTYLE_DIMTALN") {
        // code 392, int
        if (code == 392) {
            int val = var->i_val();
            dimStyle->latteralTolerance()->setAdjustmentRaw(val);
        }
    }
    else if (appName == "ACAD_DSTYLE_DIMBREAK") {
        // code 391, float
        if (code == 391) {
            // double val = var->d_val();
            // fixme - decide where to store it. This is "Dimension Break" in acad.
        }
    }
}

#endif // DWGSUPPORT
