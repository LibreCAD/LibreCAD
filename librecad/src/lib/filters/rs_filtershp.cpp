/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

/**
 * RS_FilterSHP — native ESRI Shapefile import filter implementation.
 * See rs_filtershp.h for design overview.
 */

#include "rs_filtershp.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QStringConverter>

#include "shapefil.h"

#include "lc_linetypenames.h"
#include "rs_color.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

// ---------------------------------------------------------------------------
// RAII wrappers for shapelib handles — mirror the pattern the retired
// importshp plugin used (plugins/importshp/importshp.h:42-59), rewritten
// filter-local so nothing depends on the plugin's headers.
// ---------------------------------------------------------------------------

class ScopedSHP {
public:
    ScopedSHP() = default;
    explicit ScopedSHP(SHPHandle h) : m_h(h) {}
    ~ScopedSHP() { if (m_h) SHPClose(m_h); }
    ScopedSHP(const ScopedSHP&) = delete;
    ScopedSHP& operator=(const ScopedSHP&) = delete;
    SHPHandle get() const { return m_h; }
    explicit operator bool() const { return m_h != nullptr; }
private:
    SHPHandle m_h{nullptr};
};

class ScopedDBF {
public:
    ScopedDBF() = default;
    explicit ScopedDBF(DBFHandle h) : m_h(h) {}
    ~ScopedDBF() { if (m_h) DBFClose(m_h); }
    ScopedDBF(const ScopedDBF&) = delete;
    ScopedDBF& operator=(const ScopedDBF&) = delete;
    DBFHandle get() const { return m_h; }
    explicit operator bool() const { return m_h != nullptr; }
private:
    DBFHandle m_h{nullptr};
};

class ScopedShape {
public:
    ScopedShape() = default;
    explicit ScopedShape(SHPObject* o) : m_o(o) {}
    ~ScopedShape() { if (m_o) SHPDestroyObject(m_o); }
    ScopedShape(const ScopedShape&) = delete;
    ScopedShape& operator=(const ScopedShape&) = delete;
    SHPObject* get() const { return m_o; }
    SHPObject* operator->() const { return m_o; }
    explicit operator bool() const { return m_o != nullptr; }
private:
    SHPObject* m_o{nullptr};
};

// ---------------------------------------------------------------------------
// Import options — auto-detected field names + label defaults.  A future
// options dialog can override these via the RS_Settings group "/ShpImport".
// Kept in one struct so the seam is obvious.
// ---------------------------------------------------------------------------

struct LC_ShpImportOptions {
    QString layerField;      // DBF field carrying the target layer name
    QString colorField;      // ACI index (0-255) or 24-bit RGB
    QString ltypeField;      // linetype name (e.g. "DASHED")
    QString widthField;      // numeric line weight
    QString labelField;      // text to render as RS_MText next to points
    double labelHeight{2.0}; // drawing units
    bool importLabels{true}; // add MText for point shapes when labelField set
};

// Case-insensitive lookup of a field by name; returns -1 if not present.
int findFieldCI(DBFHandle dbf, const std::initializer_list<const char*>& names) {
    if (!dbf) return -1;
    const int nFields = DBFGetFieldCount(dbf);
    for (int f = 0; f < nFields; ++f) {
        char name[16] = {};
        int w = 0, d = 0;
        (void)DBFGetFieldInfo(dbf, f, name, &w, &d);
        for (const char* candidate : names) {
            if (qstricmp(name, candidate) == 0) return f;
        }
    }
    return -1;
}

// Populate options: read RS_Settings overrides first, else auto-detect from
// the DBF header.  Returns the resolved field-index bundle for the caller.
struct ResolvedFields {
    int layer{-1};
    int color{-1};
    int ltype{-1};
    int width{-1};
    int label{-1};
};

ResolvedFields resolveFields(DBFHandle dbf, LC_ShpImportOptions& opt) {
    // Settings-driven overrides (empty = auto-detect).
    LC_GROUP("ShpImport");
    if (opt.layerField.isEmpty())
        opt.layerField = LC_GET_STR("LayerField", "");
    if (opt.colorField.isEmpty())
        opt.colorField = LC_GET_STR("ColorField", "");
    if (opt.ltypeField.isEmpty())
        opt.ltypeField = LC_GET_STR("LineTypeField", "");
    if (opt.widthField.isEmpty())
        opt.widthField = LC_GET_STR("WidthField", "");
    if (opt.labelField.isEmpty())
        opt.labelField = LC_GET_STR("LabelField", "");
    // opt.labelHeight left at the default 2.0; RS_Settings has no double
    // accessor, so plumbing a numeric override into the /ShpImport group is
    // deferred to a future options dialog (Phase 2c non-goal).
    LC_GROUP_END();
    opt.importLabels = LC_GET_ONE_BOOL("ShpImport", "ImportLabels", true);

    ResolvedFields r;
    if (!dbf) return r;

    // Explicit override wins over auto-detect for each field.
    auto lookupOrDetect = [&](const QString& explicitName,
                              std::initializer_list<const char*> defaults) {
        if (!explicitName.isEmpty()) {
            const QByteArray bytes = explicitName.toLatin1();
            return findFieldCI(dbf, {bytes.constData()});
        }
        return findFieldCI(dbf, defaults);
    };

    r.layer = lookupOrDetect(opt.layerField, {"LAYER", "LEVEL", "LYR"});
    r.color = lookupOrDetect(opt.colorField, {"COLOR", "COLOUR"});
    r.ltype = lookupOrDetect(opt.ltypeField, {"LINETYPE", "LTYPE"});
    r.width = lookupOrDetect(opt.widthField, {"WIDTH", "LWEIGHT", "LINEWT"});
    r.label = lookupOrDetect(opt.labelField, {"NAME", "LABEL", "TEXT"});
    return r;
}

// ---------------------------------------------------------------------------
// DBF codepage → QStringDecoder.
// ---------------------------------------------------------------------------

// Decode a DBF string field with the DBF's declared codepage.  Falls back to
// UTF-8 → Latin-1 on invalid sequences (the same pragmatic heuristic
// rs_filterjww.cpp uses for JWW's mixed CP932/Shift_JIS content).
QString decodeDbfString(const char* raw, const QString& codepage) {
    if (raw == nullptr || *raw == '\0') return {};
    const QByteArray bytes(raw);

    // Best-effort mapping from DBF codepage marker to a Qt codec name.
    // shapelib returns strings like "UTF-8", "LDID/87" (Latin-1 / ISO-8859-1),
    // "LDID/13" (CP850), etc.  We only try to hit the common ones; anything
    // unrecognised falls through to UTF-8 with Latin-1 fallback.
    auto encodingFromDbf = [](const QString& cp) -> std::optional<QStringConverter::Encoding> {
        const QString u = cp.toUpper();
        if (u == "UTF-8" || u == "UTF8" || u == "LDID/8B") // LDID 0x8B = UTF-8 (unofficial)
            return QStringConverter::Utf8;
        if (u == "LDID/87" || u == "LDID/57" || u == "ISO-8859-1" || u == "LATIN1"
            || u == "CP1252" || u == "WINDOWS-1252")
            return QStringConverter::Latin1;
        return std::nullopt;
    };

    if (auto enc = encodingFromDbf(codepage)) {
        QStringDecoder decoder{*enc};
        QString result = decoder(bytes);
        if (!decoder.hasError()) return result;
    }
    // UTF-8 first, then Latin-1.
    {
        QStringDecoder utf8{QStringConverter::Utf8};
        QString result = utf8(bytes);
        if (!utf8.hasError()) return result;
    }
    QStringDecoder latin1{QStringConverter::Latin1};
    return latin1(bytes);
}

// ---------------------------------------------------------------------------
// Field-value → RS_Pen fragments.
// ---------------------------------------------------------------------------

// Numeric COLOR field: 0-255 → ACI palette (reuse RS_FilterDXFRW::numberToColor),
// larger → 24-bit RGB (0xRRGGBB).
RS_Color colorFromNumber(int v) {
    if (v >= 0 && v <= 255) return RS_FilterDXFRW::numberToColor(v);
    return RS_Color((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

// Nearest-neighbour: pick the RS2::LineWidth enum value closest to the
// requested numeric weight (interpreted as tenths-of-mm, matching the
// enum's own integer encoding — see rs.h:867-897).
RS2::LineWidth widthFromNumber(double v) {
    const int enumVal = static_cast<int>(std::lround(v));
    const RS2::LineWidth candidates[] = {
        RS2::Width00, RS2::Width01, RS2::Width02, RS2::Width03, RS2::Width04,
        RS2::Width05, RS2::Width06, RS2::Width07, RS2::Width08, RS2::Width09,
        RS2::Width10, RS2::Width11, RS2::Width12, RS2::Width13, RS2::Width14,
        RS2::Width15, RS2::Width16, RS2::Width17, RS2::Width18, RS2::Width19,
        RS2::Width20, RS2::Width21, RS2::Width22, RS2::Width23};
    RS2::LineWidth best = RS2::Width00;
    int bestDist = std::abs(static_cast<int>(RS2::Width00) - enumVal);
    for (RS2::LineWidth w : candidates) {
        const int d = std::abs(static_cast<int>(w) - enumVal);
        if (d < bestDist) { bestDist = d; best = w; }
    }
    return best;
}

// Build an RS_Pen from resolved fields for one DBF record.  If no styling
// fields are present, returns std::nullopt so callers can leave the entity
// at ByLayer defaults.
std::optional<RS_Pen> penFromRecord(DBFHandle dbf, int record,
                                    const ResolvedFields& rf,
                                    const QString& codepage) {
    if (!dbf || record < 0) return std::nullopt;
    bool any = false;
    RS_Pen pen{RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer};

    if (rf.color >= 0 && !DBFIsAttributeNULL(dbf, record, rf.color)) {
        const int v = DBFReadIntegerAttribute(dbf, record, rf.color);
        pen.setColor(colorFromNumber(v));
        any = true;
    }
    if (rf.ltype >= 0 && !DBFIsAttributeNULL(dbf, record, rf.ltype)) {
        const QString name = decodeDbfString(
            DBFReadStringAttribute(dbf, record, rf.ltype), codepage);
        if (!name.isEmpty()) {
            pen.setLineType(LC_LineTypeNames::nameToLineType(name));
            any = true;
        }
    }
    if (rf.width >= 0 && !DBFIsAttributeNULL(dbf, record, rf.width)) {
        const double w = DBFReadDoubleAttribute(dbf, record, rf.width);
        pen.setWidth(widthFromNumber(w));
        any = true;
    }
    if (!any) return std::nullopt;
    return pen;
}

// Ensure a layer with the given name exists on @p g; return it.
RS_Layer* ensureLayer(RS_Graphic& g, const QString& name) {
    if (RS_Layer* existing = g.findLayer(name)) return existing;
    auto* layer = new RS_Layer(name);
    // Default pen — ByBlock via RS_Layer's default constructor.
    g.addLayer(layer);
    return layer;
}

// ---------------------------------------------------------------------------
// Geometry emission helpers.
// ---------------------------------------------------------------------------

void emitPoint(RS_Graphic& g, RS_Layer* layer,
               const std::optional<RS_Pen>& pen,
               double x, double y, double z) {
    RS_Vector v(x, y, z);
    auto* p = new RS_Point(&g, RS_PointData(v));
    p->setLayer(layer);
    if (pen) p->setPen(*pen);
    g.addEntity(p);
}

void emitMultiPoint(RS_Graphic& g, RS_Layer* layer,
                    const std::optional<RS_Pen>& pen,
                    const SHPObject& o) {
    for (int i = 0; i < o.nVertices; ++i) {
        const double z = (o.padfZ != nullptr) ? o.padfZ[i] : 0.0;
        emitPoint(g, layer, pen, o.padfX[i], o.padfY[i], z);
    }
}

// Emit one polyline covering the [start, start+count) vertex slice of @p o.
// Ring convention: for closed polylines the SHP-required duplicated closing
// vertex (first==last) is dropped before setClosed(true).  Degenerate parts
// (< 2 vertices open, < 3 closed) are skipped.
void emitOnePart(RS_Graphic& g, RS_Layer* layer,
                 const std::optional<RS_Pen>& pen,
                 const SHPObject& o, int start, int count, bool closed,
                 int& partsEmitted, int& partsSkipped) {
    if (start < 0 || count <= 0 || start + count > o.nVertices) {
        ++partsSkipped;
        return;
    }
    if (closed && count >= 2) {
        const RS_Vector first(o.padfX[start], o.padfY[start]);
        const RS_Vector last(o.padfX[start + count - 1],
                             o.padfY[start + count - 1]);
        if (first.squaredTo(last) < 1e-20) --count;
    }
    const int minVerts = closed ? 3 : 2;
    if (count < minVerts) { ++partsSkipped; return; }

    auto* pl = new RS_Polyline(&g, RS_PolylineData(
        RS_Vector(false), RS_Vector(false), false));
    pl->setLayer(layer);
    if (pen) pl->setPen(*pen);
    for (int i = 0; i < count; ++i) {
        const int idx = start + i;
        const double z = (o.padfZ != nullptr) ? o.padfZ[idx] : 0.0;
        pl->addVertex(RS_Vector(o.padfX[idx], o.padfY[idx], z));
    }
    if (closed) pl->setClosed(true);
    pl->endPolyline();
    g.addEntity(pl);
    ++partsEmitted;
}

// Emit every ARC/POLYGON part.  ARC parts open, POLYGON parts closed.
void emitParts(RS_Graphic& g, RS_Layer* layer,
               const std::optional<RS_Pen>& pen,
               const SHPObject& o, bool closed,
               int& partsEmitted, int& partsSkipped) {
    const int nParts = std::max(1, o.nParts);
    for (int p = 0; p < nParts; ++p) {
        const int start = (o.panPartStart && o.nParts > 0)
                          ? o.panPartStart[p] : 0;
        const int end = (p + 1 < nParts)
                        ? o.panPartStart[p + 1]
                        : o.nVertices;
        emitOnePart(g, layer, pen, o, start, end - start, closed,
                    partsEmitted, partsSkipped);
    }
}

// Emit a label MText next to a point, if a label field resolved a non-empty
// string for this record.
void emitLabel(RS_Graphic& g, RS_Layer* layer,
               const std::optional<RS_Pen>& pen,
               double x, double y, double z,
               const QString& text, double height) {
    if (text.isEmpty()) return;
    RS_MTextData d(RS_Vector(x + height * 0.5, y + height * 0.5, z),
                   height, /*width=*/0.0,
                   RS_MTextData::VABottom, RS_MTextData::HALeft,
                   RS_MTextData::LeftToRight, RS_MTextData::AtLeast,
                   /*lineSpacingFactor=*/1.0,
                   text, /*style=*/"standard", /*angle=*/0.0,
                   RS2::NoUpdate);
    auto* m = new RS_MText(&g, d);
    m->setLayer(layer);
    if (pen) m->setPen(*pen);
    m->update();
    g.addEntity(m);
}

// Trim trailing spaces (DBF strings are space-padded to field width).
QString trimTrailingSpaces(QString s) {
    while (!s.isEmpty() && s.back().isSpace()) s.chop(1);
    return s;
}

} // namespace

// ---------------------------------------------------------------------------
// RS_FilterSHP::fileImport
// ---------------------------------------------------------------------------

bool RS_FilterSHP::fileImport(RS_Graphic& g, const QString& file,
                              RS2::FormatType /*type*/) {
    m_lastError.clear();

    const QByteArray path = QFile::encodeName(file);
    ScopedSHP shp{SHPOpen(path.constData(), "rb")};
    if (!shp) {
        m_lastError = QObject::tr("Cannot open shapefile %1 (missing or corrupt .shx?)")
                      .arg(file);
        RS_DEBUG->print(RS_Debug::D_WARNING, "%s: %s",
                        __func__, m_lastError.toLatin1().constData());
        return false;
    }

    int nEntities = 0;
    int nShapeType = 0;
    double bboxMin[4] = {0}, bboxMax[4] = {0};
    SHPGetInfo(shp.get(), &nEntities, &nShapeType, bboxMin, bboxMax);
    RS_DEBUG->print("%s: opened %s, %d records, shape type %s (%d)",
                    __func__, path.constData(), nEntities,
                    SHPTypeName(nShapeType), nShapeType);

    // Companion .dbf — geometry-only import is fine if it's missing.
    QString dbfPath = file;
    if (dbfPath.endsWith(".shp", Qt::CaseInsensitive))
        dbfPath.chop(4);
    dbfPath += ".dbf";
    const QByteArray dbfBytes = QFile::encodeName(dbfPath);
    ScopedDBF dbf{DBFOpen(dbfBytes.constData(), "rb")};

    QString codepage;
    if (dbf) {
        const char* cp = DBFGetCodePage(dbf.get());
        codepage = cp ? QString::fromLatin1(cp) : QString();
    }

    LC_ShpImportOptions opts;
    const ResolvedFields rf = resolveFields(dbf.get(), opts);

    RS_Layer* defaultLayer = ensureLayer(g, "0");
    int emitted = 0, skipped = 0;
    int partsEmitted = 0, partsSkipped = 0;

    for (int i = 0; i < nEntities; ++i) {
        ScopedShape rec{SHPReadObject(shp.get(), i)};
        if (!rec) { ++skipped; continue; }
        if (rec->nSHPType == SHPT_NULL) { ++skipped; continue; }

        // Layer for this record: DBF-driven → auto-created; otherwise "0".
        RS_Layer* layer = defaultLayer;
        if (dbf && rf.layer >= 0
            && !DBFIsAttributeNULL(dbf.get(), i, rf.layer)) {
            const QString rawName = decodeDbfString(
                DBFReadStringAttribute(dbf.get(), i, rf.layer), codepage);
            const QString name = trimTrailingSpaces(rawName);
            if (!name.isEmpty()) layer = ensureLayer(g, name);
        }

        const std::optional<RS_Pen> pen =
            penFromRecord(dbf.get(), i, rf, codepage);

        switch (rec->nSHPType) {
        case SHPT_POINT:
        case SHPT_POINTZ:
        case SHPT_POINTM: {
            const double z = (rec->padfZ != nullptr) ? rec->padfZ[0] : 0.0;
            emitPoint(g, layer, pen, rec->padfX[0], rec->padfY[0], z);
            if (opts.importLabels && rf.label >= 0 && dbf
                && !DBFIsAttributeNULL(dbf.get(), i, rf.label)) {
                const QString text = trimTrailingSpaces(decodeDbfString(
                    DBFReadStringAttribute(dbf.get(), i, rf.label), codepage));
                emitLabel(g, layer, pen, rec->padfX[0], rec->padfY[0], z,
                          text, opts.labelHeight);
            }
            ++emitted;
            break;
        }
        case SHPT_MULTIPOINT:
        case SHPT_MULTIPOINTZ:
        case SHPT_MULTIPOINTM:
            emitMultiPoint(g, layer, pen, *rec.get());
            ++emitted;
            break;
        case SHPT_ARC:
        case SHPT_ARCZ:
        case SHPT_ARCM:
            emitParts(g, layer, pen, *rec.get(), /*closed=*/false,
                      partsEmitted, partsSkipped);
            ++emitted;
            break;
        case SHPT_POLYGON:
        case SHPT_POLYGONZ:
        case SHPT_POLYGONM:
            emitParts(g, layer, pen, *rec.get(), /*closed=*/true,
                      partsEmitted, partsSkipped);
            ++emitted;
            break;
        case SHPT_MULTIPATCH:
            // Simplified 2.5D handling: rings become closed polylines; strips
            // and fans render as an open-polyline wireframe.  Full triangle-
            // mesh support is out of scope for the first cut.
            for (int p = 0; p < rec->nParts; ++p) {
                const int ptype = (rec->panPartType != nullptr)
                                  ? rec->panPartType[p] : SHPP_RING;
                const bool ringLike = (ptype == SHPP_OUTERRING
                                       || ptype == SHPP_INNERRING
                                       || ptype == SHPP_RING
                                       || ptype == SHPP_FIRSTRING);
                const int start = rec->panPartStart[p];
                const int end = (p + 1 < rec->nParts)
                                ? rec->panPartStart[p + 1]
                                : rec->nVertices;
                emitOnePart(g, layer, pen, *rec.get(), start, end - start,
                            ringLike, partsEmitted, partsSkipped);
            }
            ++emitted;
            break;
        default:
            RS_DEBUG->print(RS_Debug::D_WARNING,
                "%s: skipping unknown shape type %d at record %d",
                __func__, rec->nSHPType, i);
            ++skipped;
            break;
        }
    }

    if (skipped > 0) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "%s: %d record(s) skipped, %d part(s) skipped",
            __func__, skipped, partsSkipped);
    }
    RS_DEBUG->print("%s: imported %d record(s) (%d polyline part(s))",
                    __func__, emitted, partsEmitted);

    // Fail only if nothing readable was produced from a non-empty file.
    if (emitted == 0 && nEntities > 0) {
        m_lastError = QObject::tr("Shapefile %1 contained %2 records but none were readable")
                      .arg(file).arg(nEntities);
        return false;
    }
    return true;
}
