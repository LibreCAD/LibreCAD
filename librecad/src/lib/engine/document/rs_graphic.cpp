/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_graphic.h"

#include <iostream>

#include "dxf_format.h"
#include "lc_containertraverser.h"
#include "lc_defaults.h"
#include "lc_dimarrowregistry.h"
#include "lc_dimstyleslist.h"
#include "lc_dimstyletovariablesmapper.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_dimension.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"

namespace {
    // default paper size A4: 210x297 mm
    const RS_Vector g_paperSizeA4{210., 297.};

    // validate coordinates
    bool validCoordinate(const double x) {
        return x >= RS_MINDOUBLE && x <= RS_MAXDOUBLE;
    }

    // validate vector
    bool validRange(const RS_Vector& vp) {
        return vp.valid && validCoordinate(vp.x) && validCoordinate(vp.y);
    }

    // validate vpMin and vpMax forms a valid bounding box
    bool validRange(const RS_Vector& vpMin, const RS_Vector& vpMax) {
        const bool validCoords = validRange(vpMin) && validRange(vpMax);
        if (validCoords) {
            bool xValid{false};
            const double xDelta = vpMax.x - vpMin.x; // vpMax.x == vpMin.x is also valid for vertical lines
            if (std::signbit(xDelta)) {
                xValid = std::abs(xDelta) < RS_TOLERANCE; //
            }
            else {
                xValid = true;
            }
            if (xValid) {
                bool yValid{false};
                const double yDelta = vpMax.y - vpMin.y; // vpMax.x == vpMin.x is also valid for horizontal lines
                if (std::signbit(yDelta)) {
                    yValid = std::abs(yDelta) < RS_TOLERANCE;
                }
                else {
                    yValid = true;
                }
                return yValid;
            }
        }
        return false;
    }
}

/**
 * Default constructor.
 */
RS_Graphic::RS_Graphic(RS_EntityContainer* parent)
    : RS_Document(parent), m_autosaveFilename{"Unnamed"}, m_plotSettings{std::make_unique<LC_PlotSettings>(this)} {
    LC_GROUP_GUARD("Defaults");
    {
        setUnit(RS_Units::stringToUnit(LC_GET_ONE_STR("Defaults", "Unit", "None")));
        addVariable("$SNAPSTYLE", LC_GET_INT("IsometricGrid", 0), 70);
        addVariable("$SNAPISOPAIR", LC_GET_INT("IsoGridView", 1), 70);
        setGridOn(!LC_GET_BOOL("GridOffForNewDrawing", false));

        const QString& defaultAnglesBase = LC_GET_STR("AnglesBaseAngle", "0.0");
        const bool anglesCounterClockwise = LC_GET_BOOL("AnglesCounterClockwise", true);

        const double angleBaseDegrees = RS_Math::eval(defaultAnglesBase, 0.0);
        const double angleBaseRadians = RS_Math::deg2rad(angleBaseDegrees);
        setAnglesCounterClockwise(anglesCounterClockwise);
        setAnglesBase(angleBaseRadians);
    }
    const RS2::Unit unit = getUnit();

    if (unit == RS2::Inch) {
        addVariable("$DIMASZ", 0.1, DXF_FORMAT_GC_DimASz);
        addVariable("$DIMEXE", 0.05, DXF_FORMAT_GC_DimEXE);
        addVariable("$DIMEXO", 0.025, DXF_FORMAT_GC_DimExO);
        addVariable("$DIMGAP", 0.025, DXF_FORMAT_GC_DimGap);
        addVariable("$DIMTXT", 0.1, DXF_FORMAT_GC_DimTxt);
    }
    else {
        addVariable("$DIMASZ", RS_Units::convert(2.5, RS2::Millimeter, unit), DXF_FORMAT_GC_DimASz);
        addVariable("$DIMEXE", RS_Units::convert(1.25, RS2::Millimeter, unit), DXF_FORMAT_GC_DimEXE);
        addVariable("$DIMEXO", RS_Units::convert(0.625, RS2::Millimeter, unit), DXF_FORMAT_GC_DimExO);
        addVariable("$DIMGAP", RS_Units::convert(0.625, RS2::Millimeter, unit), DXF_FORMAT_GC_DimGap);
        addVariable("$DIMTXT", RS_Units::convert(2.5, RS2::Millimeter, unit), DXF_FORMAT_GC_DimTxt);
    }
    addVariable("$DIMTIH", 0, DXF_FORMAT_GC_DimTIH);
    //initialize printer vars bug #3602444
    m_plotSettings->setPaperScale(m_plotSettings->getPaperScale());
    setPaperInsertionBase(getPaperInsertionBase());

    //set default values for point style
    addVariable("$PDMODE", LC_DEFAULTS_PDMode, DXF_FORMAT_GC_PDMode);
    addVariable("$PDSIZE", LC_DEFAULTS_PDSize, DXF_FORMAT_GC_PDSize);

    addVariable("$JOINSTYLE", 1, DXF_FORMAT_GC_JoinStyle);
    addVariable("$ENDCAPS", 1, DXF_FORMAT_GC_Endcaps);
    m_modified = false;
}

/**
 * Destructor.
 */
RS_Graphic::~RS_Graphic() = default;

void RS_Graphic::onLoadingCompleted() {
    const auto fallBackDimStyleFromVars = m_dimstyleList.getFallbackDimStyleFromVars();
    fallBackDimStyleFromVars->fillByDefaults(); // cleanup (is it redundant?)
    LC_DimStyleToVariablesMapper dimStyleToVariablesMapper;
    dimStyleToVariablesMapper.fromDictionary(fallBackDimStyleFromVars, getVariableDictObjectRef(), getUnit());

    m_anglesCounterClockWize = getVariableBool("$ANGDIR", false);

    if (m_dimstyleList.isEmpty()) {
        // add content of vars to dimstyle list, to ensure that we have at least one style there
        m_dimstyleList.addDimStyle(fallBackDimStyleFromVars->getCopy());
    }
    else {
        // some programs (like AutoCAD) may store in DXF not all fields for the dim style definition, but only
        // modified ones.
        // For example, for dimension specific styles (like linear, ordinal, etc) only variables with values that
        // are different to values in the base style is store.
        // Therefore, for dimension type-specific styles, we perform a merge of non-set variables with values
        // from base style.
        m_dimstyleList.mergeStyles();
    }
    updateDimensions(true);
}

/**
 * Counts the m_entities on the given layer.
 */
unsigned RS_Graphic::countLayerEntities(RS_Layer* layer) const {
    unsigned c = 0;
    if (layer != nullptr) {
        for (const RS_Entity* t : *this) {
            if ((t->getLayer() != nullptr) && t->getLayer()->getName() == layer->getName()) {
                c += t->countDeep();
            }
        }
    }
    return c;
}

/**
 * Removes the given layer and undoes all m_entities on it.
 */
void RS_Graphic::removeLayer(RS_Layer* layer) {
    // fixme - move to other class?
    if (layer != nullptr) {
        const QString& layerName = layer->getName();
        if (layerName != "0") {
            std::vector<RS_Entity*> toRemove;
            //find entities on layer
            for (RS_Entity* e : *this) {
                const auto lay = e->getLayer();
                if (lay != nullptr && lay->getName() == layerName) {
                    toRemove.push_back(e);
                }
            }
            // remove all entities on that layer:
            if (!toRemove.empty()) {
                startUndoCycle();
                for (RS_Entity* e : toRemove) {
                    e->setLayer("0");
                    undoableDelete(e);
                }
                endUndoCycle();
            }

            toRemove.clear();

            // remove all entities in blocks that are on that layer:
            for (RS_Block* blk : m_blockList) {
                // fixme - REVIEW this logic, why entities are deleted in blocks??
                if (blk == nullptr) {
                    continue;
                }
                for (auto e : *blk) {
                    const auto lay = e->getLayer();
                    if ((lay != nullptr) && lay->getName() == layerName) {
                        toRemove.push_back(e);
                    }
                }
            }

            for (RS_Entity* e : toRemove) {
                e->mark(true);
                e->setLayer("0");
            }

            m_layerList.remove(layer);
        }
        validateSelection();
    }
}

/**
 * Clears all layers, blocks and entities of this graphic.
 * A default layer (0) is created.
 */
void RS_Graphic::initForNewDocument() {
    RS_DEBUG->print("RS_Graphic::newDoc");
    clear();
    clearLayers();
    clearBlocks();
    addLayer(new RS_Layer("0"));
    setModified(false);
}

void RS_Graphic::clearVariables() {
    m_variableDict.clear();
}

QString RS_Graphic::getCustomProperty(const QString& key, const QString& defaultValue) const {
    return m_customVariablesDict.getString(key, defaultValue);
}

void RS_Graphic::addCustomProperty(const QString& key, const QString& value) {
    m_customVariablesDict.add(key, value, 1);
    setModified(true);
}

void RS_Graphic::removeCustomProperty(const QString& key) {
    m_customVariablesDict.remove(key);
}

bool RS_Graphic::hasCustomProperty(const QString& key) const {
    return m_customVariablesDict.has(key);
}

const QHash<QString, RS_Variable>& RS_Graphic::getCustomProperties() const {
    return m_customVariablesDict.getVariableDict();
}

int RS_Graphic::countVariables() const {
    return m_variableDict.count();
}

void RS_Graphic::addVariable(const QString& key, const RS_Vector& value, const int code) {
    if (m_variableDict.add(key, value, code)) {
        setModified(true);
    }
}

void RS_Graphic::addVariable(const QString& key, const QString& value, const int code) {
    if (m_variableDict.add(key, value, code)) {
        setModified(true);
    }
}

void RS_Graphic::addVariable(const QString& key, const int value, const int code) {
    if (m_variableDict.add(key, value, code)) {
        setModified(true);
    }
}

void RS_Graphic::addVariable(const QString& key, const bool value, const int code) {
    if (m_variableDict.add(key, value, code)) {
        setModified(true);
    }
}

void RS_Graphic::addVariable(const QString& key, const double value, const int code) {
    if (m_variableDict.add(key, value, code)) {
        setModified(true);
    }
}

void RS_Graphic::removeVariable(const QString& key) {
    m_variableDict.remove(key);
    setModified(true); // fixme - should we check for existence of such key?
}

RS_Vector RS_Graphic::getVariableVector(const QString& key, const RS_Vector& def) const {
    return m_variableDict.getVector(key, def);
}

QString RS_Graphic::getVariableString(const QString& key, const QString& def) const {
    return m_variableDict.getString(key, def);
}

int RS_Graphic::getVariableInt(const QString& key, const int def) const {
    return m_variableDict.getInt(key, def);
}

bool RS_Graphic::getVariableBool(const QString& key, const bool def) const {
    return m_variableDict.getInt(key, def ? 1 : 0) != 0;
}

double RS_Graphic::getVariableDouble(const QString& key, const double def) const {
    return m_variableDict.getDouble(key, def);
}

QHash<QString, RS_Variable>& RS_Graphic::getVariableDict() {
    return m_variableDict.getVariableDict();
}

//
// fixme - sand - actually, some additional caching of variables may be used,
// in order to avoid loading/writing them into hashmap on each access...
// need to measure and provile the cost of direct access to variables map first.
// so it might be something like common initialization below..
//  void RS_Graphic::loadVariables(){
//    gridOn = getVariableInt("$GRIDMODE", 1) != 0;
//     etc...
//  }
//

/**
 * @return true if the grid is switched on (visible).
 */
bool RS_Graphic::isGridOn() const {
    const int on = getVariableInt("$GRIDMODE", 1);
    return on != 0;
    //    return gridOn;
}

/**
 * Enables / disables the grid.
 */
void RS_Graphic::setGridOn(const bool on) {
    //    gridOn = on;
    addVariable("$GRIDMODE", on, 70);
}

/**
 * @return true if the isometric grid is switched on (visible).
 */
bool RS_Graphic::isIsometricGrid() const {
    //$ISOMETRICGRID == $SNAPSTYLE
    const int on = getVariableInt("$SNAPSTYLE", 0);
    return on != 0;
}

/**
 * Enables / disables isometric grid.
 */
void RS_Graphic::setIsometricGrid(const bool on) {
    //$ISOMETRICGRID == $SNAPSTYLE
    addVariable("$SNAPSTYLE", on, 70);
}

double RS_Graphic::getAnglesBase() const {
    const double result = getVariableDouble("$ANGBASE", 0.0);
    return result;
}

void RS_Graphic::setAnglesBase(const double baseAngle) {
    addVariable("$ANGBASE", baseAngle, 50);
}

bool RS_Graphic::areAnglesCounterClockWise() const {
    return m_anglesCounterClockWize;
}

void RS_Graphic::setAnglesCounterClockwise(const bool on) {
    addVariable("$ANGDIR", on, 70);
    m_anglesCounterClockWize = on;
}

RS_Vector RS_Graphic::getUserGridSpacing() const {
    return getVariableVector("$GRIDUNIT", {0.0, 0.0});
}

void RS_Graphic::setUserGridSpacing(const RS_Vector &spacing) {
    addVariable("$GRIDUNIT", spacing, 10);
}

void RS_Graphic::setCurrentUCS(const LC_UCS* ucs) {
    QString name = ucs->getName();
    if (!ucs->isUCS()) {
        name.clear();
    }
    addVariable("$UCSNAME", name, 2);
    addVariable("$UCSORG", ucs->getOrigin(), 10);

    // so far we don't support the following variables
    // http://entercad.ru/acad_dxf.en/ws1a9193826455f5ff18cb41610ec0a2e719-7a6f.htm
    /*
    $UCSORGBACK
    $UCSORGBOTTOM
    $UCSORGFRONT
    $UCSORGLEFT
    $UCSORGRIGHT
    $UCSORGTOP
    */

    // as for these variables... well, so far it' snot clear who they are related to $SNAPSTYLE.... should we rely on them for isometric mode?
    /*
      $UCSORTHOREF
    */

    addVariable("$UCSORTHOVIEW", ucs->getOrthoType(), 70);
    addVariable("$UCSXDIR", ucs->getXAxis(), 10);
    addVariable("$UCSYDIR", ucs->getYAxis(), 10);
}

LC_UCS* RS_Graphic::getCurrentUCS() const {
    const QString name = getVariableString("$UCSNAME", "");
    const RS_Vector origin = getVariableVector("$UCSORG", RS_Vector(0.0, 0.0));
    const int orthoType = getVariableInt("$UCSORTHOVIEW", 0);
    const RS_Vector xAxis = getVariableVector("$UCSXDIR", RS_Vector(1.0, 0.0));
    RS_Vector yAxis = xAxis;
    yAxis = getVariableVector("$UCSYDIR", yAxis.rotate(M_PI_2));

    const LC_UCS* wcs = m_ucsList.getWCS();

    auto result = new LC_UCS(name);
    result->setOrigin(origin);
    result->setOrthoType(orthoType);
    result->setXAxis(xAxis);
    result->setYAxis(yAxis);

    if (wcs->isSameTo(result)) {
        delete result;
        result = new LC_WCS();
    }
    return result;
}

RS2::IsoGridViewType RS_Graphic::getIsoView() const {
    const auto result = static_cast<RS2::IsoGridViewType>(getVariableInt("$SNAPISOPAIR", RS2::IsoGridViewType::IsoTop));
    return result;
}

void RS_Graphic::setIsoView(const RS2::IsoGridViewType viewType) {
    addVariable("$SNAPISOPAIR", viewType, 70);
}

/**
 * Sets the unit of this graphic to 'u'
 */
void RS_Graphic::setUnit(const RS2::Unit u) {
    // fixme - sand - add caching
    m_plotSettings->setPaperSize(RS_Units::convert(m_plotSettings->getPaperSize(), getUnit(), u));
    addVariable("$INSUNITS", u, 70);
}

/**
 * Gets the unit of this graphic
 */
RS2::Unit RS_Graphic::getUnit() const {
    // fixme - sand - add caching
    return static_cast<RS2::Unit>(getVariableInt("$INSUNITS", 0));
}

/**
 * @return The linear format type for this document.
 * This is determined by the variable "$LUNITS".
 */
RS2::LinearFormat RS_Graphic::getLinearFormat() const {
    // fixme - sand - add caching
    const int lunits = getVariableInt("$LUNITS", 2);
    return convertLinearFormatDXF2LC(lunits);
}

void RS_Graphic::setLinearFormat(const RS2::LinearFormat linearFormat) {
    addVariable("$LUNITS", linearFormat + 1, 70);
}

void RS_Graphic::replaceCustomVars(const QHash<QString, QString>& vars) {
    m_customVariablesDict.clear();
    QHashIterator<QString, QString> customVar(vars);
    while (customVar.hasNext()) {
        customVar.next();
        m_customVariablesDict.add(customVar.key(), customVar.value(), 1);
    }
    setModified(true);
}

/**
 * @return The linear format type used by the variable "$LUNITS" & "$DIMLUNIT".
 */
// fixme - sand - move to generic utils!
RS2::LinearFormat RS_Graphic::convertLinearFormatDXF2LC(const int f) {
    switch (f) {
        case 1:
            return RS2::Scientific;
        case 2:
            return RS2::Decimal;
        case 3:
            return RS2::Engineering;
        case 4:
            return RS2::Architectural;
        case 5:
            return RS2::Fractional;
        case 6:
            return RS2::ArchitecturalMetric;
        default:
            return RS2::Decimal;
    }
}

/**
 * @return The linear precision for this document.
 * This is determined by the variable "$LUPREC".
 */
int RS_Graphic::getLinearPrecision() const {
    // fixme - sand - add caching
    return getVariableInt("$LUPREC", 4);
}

void RS_Graphic::setLinearPrecision(const int value)  {
    // fixme - sand - add caching
    addVariable("$LUPREC", value, 70);
}

/**
 * @return The angle format type for this document.
 * This is determined by the variable "$AUNITS".
 */
RS2::AngleFormat RS_Graphic::getAngleFormat() const {
    // fixme - sand - add caching
    const int aunits = getVariableInt("$AUNITS", 0);

    switch (aunits) {
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
        default:
            return RS2::DegreesDecimal;
    }
}

void RS_Graphic::setAngleFormat(const RS2::AngleFormat angleFormat) {
    addVariable("$AUNITS", angleFormat, 70);
}

/**
 * @return The angle precision for this document.
 * This is determined by the variable "$AUPREC".
 */
int RS_Graphic::getAnglePrecision() const {
    // fixme - sand - add caching
    return getVariableInt("$AUPREC", 4);
}

void RS_Graphic::addAnglePrecision(const int value) {
    addVariable("$AUPREC", value, 70);
}

/**
 * @return The insertion point of the drawing into the paper space.
 * This is the distance from the lower left paper edge to the zero
 * point of the drawing. DXF: $PINSBASE.
 */
RS_Vector RS_Graphic::getPaperInsertionBase() const {
    return getVariableVector("$PINSBASE", RS_Vector(0.0, 0.0));
}

/**
 * Sets the PINSBASE variable.
 */
void RS_Graphic::setPaperInsertionBase(const RS_Vector& p) {
    addVariable("$PINSBASE", p, 10);
}

/**
 * Centers drawing on page. Affects DXF variable $PINSBASE.
 */
void RS_Graphic::centerToPage() {
    const RS_Vector paperSize = m_plotSettings->getPrintAreaSize(getUnit() != 0u);
    auto graphicSize = getSize();
    auto graphicMin = getMin();
    /** avoid zero size, bug#3573158 */
    if (std::abs(graphicSize.x) < RS_TOLERANCE) {
        graphicSize.x = 10.;
        graphicMin.x = -5.;
    }
    if (std::abs(graphicSize.y) < RS_TOLERANCE) {
        graphicSize.y = 10.;
        graphicMin.y = -5.;
    }

    const RS_Vector paperMin{
        m_plotSettings->getMarginLeftInUnits(),
        m_plotSettings->getMarginBottomInUnits()
    };
    // paper printable area center
    const RS_Vector paperCenter = paperMin + paperSize * 0.5;
    const double scale = m_plotSettings->getPaperScale();
    // graphic center
    const RS_Vector scaledCenter = (graphicMin + graphicSize * 0.5) * scale;

    // align graphic center to the paper center
    const RS_Vector pinsbase = paperCenter - scaledCenter;

    setPaperInsertionBase(pinsbase);
}

/**
 * Fits drawing on page. Affects DXF variable $PINSBASE.
 */
// fixme - check margins support in single and tiled modes - looks like they are shown differently
bool RS_Graphic::fitToPage() {
    const RS_Vector printSize = m_plotSettings->getPrintAreaSize(false);
    RS_Vector graphicSize = getSize();
    if (std::abs(graphicSize.x) < RS_TOLERANCE) {
        graphicSize.x = 10.;
    }
    if (std::abs(graphicSize.y) < RS_TOLERANCE) {
        graphicSize.y = 10.;
    }
    double scaleX = RS_MAXDOUBLE;
    double scaleY = RS_MAXDOUBLE;

    // tin-pot 2011-12-30: TODO: can s.x < 0.0 (==> fx < 0.0) happen?
    if (std::abs(graphicSize.x) > RS_TOLERANCE) {
        scaleX = printSize.x / graphicSize.x;
    }
    if (std::abs(graphicSize.y) > RS_TOLERANCE) {
        scaleY = printSize.y / graphicSize.y;
    }

    const double scale = std::min(scaleX, scaleY);
    if (scale >= RS_MAXDOUBLE || scale <= 1.0e-10) {
        m_plotSettings->setPaperSize(RS_Units::convert(g_paperSizeA4, RS2::Millimeter, getUnit()));
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Invalid printing scale %1. Cannot fit print preview to page").arg(scale));
        return false;
    }
    m_plotSettings->setPaperScale(scale);
    centerToPage();
    return true;
}


void RS_Graphic::addEntity(const RS_Entity* entity) {
    RS_Document::addEntity(entity);
    if ( /*entity->rtti() == RS2::EntityBlock ||*/
        entity->rtti() == RS2::EntityContainer) {
        auto* e = static_cast<const RS_EntityContainer*>(entity);
        for (const auto e1 : *e) {
            addEntity(e1);
        }
    }
}

/**
 * Dumps the entities to stdout.
 */
std::ostream& operator <<(std::ostream& os, RS_Graphic& g) {
    os << "--- Graphic: \n";
    os << "---" << *g.getLayerList() << "\n";
    os << "---" << *g.getBlockList() << "\n";
    os << "---" << (RS_Undo&)g << "\n";
    auto asContainer = static_cast<const RS_EntityContainer&>(g);
    os << "---" << asContainer << "\n";
    return os;
}

LC_PlotSettings* RS_Graphic::getPlotSettings() const {
    return m_plotSettings.get();
}

/**
 * Removes invalid objects.
 * @return how many objects were removed
 */
int RS_Graphic::clean() {
    int howMany = 0;

    forcedCalculateBorders();

    for (const RS_Entity* e : std::as_const(*this)) {
        if (e != nullptr) {
            if (!validRange(e->getMin(), e->getMax())) {
                // fixme - sand - files restore, the issue with
                // removeEntity(e);
                howMany += 1;
            }
        }
    }
    return howMany;
}

QString RS_Graphic::formatAngle(const double angle) const {
    return RS_Units::formatAngle(angle, getAngleFormat(), getAnglePrecision());
}

QString RS_Graphic::formatLinear(const double linear) const {
    return RS_Units::formatLinear(linear, getUnit(), getLinearFormat(), getLinearPrecision(), false);
}

/**
  * @retval true The document has been modified since it was last saved.
  * @retval false The document has not been modified since it was last saved.
  */
bool RS_Graphic::isModified() const {
    return m_modified || m_layerList.isModified() || m_blockList.isModified() || m_namedViewsList.isModified() || m_ucsList.isModified() ||
        m_dimstyleList.isModified() || m_variableDict.isModified();
}

/**
 * Sets the documents modified status to 'm'.
 */
void RS_Graphic::setModified(const bool m) {
    m_modified = m;
    if (!m) {
        m_layerList.setModified(m);
        m_blockList.setModified(m);
        m_namedViewsList.setModified(m);
        m_ucsList.setModified(m);
        m_dimstyleList.setModified(m);
        m_variableDict.setModified(m);
    }
    if (m_modificationListener != nullptr) {
        m_modificationListener->graphicModified(this, m);
    }
}

void RS_Graphic::markSaved(const QDateTime& lastSaveTime) {
    setModified(false);
    setLastSaveTime(lastSaveTime);
}

RS2::FormatType RS_Graphic::getFormatType() const {
    return m_formatType;
}

void RS_Graphic::setFormatType(const RS2::FormatType formatType) {
    m_formatType = formatType;
}

const QString& RS_Graphic::getAutosaveFilename() const {
    return m_autosaveFilename;
}

void RS_Graphic::setAutosaveFileName(const QString& fileName) {
    m_autosaveFilename = fileName;
}

LC_DimStyle* RS_Graphic::getDimStyleByName(const QString& name, const RS2::EntityType dimType) const {
    return m_dimstyleList.resolveByName(name, dimType);
}

LC_DimStyle* RS_Graphic::getFallBackDimStyleFromVars() const {
    return m_dimstyleList.getFallbackDimStyleFromVars();
}

QString RS_Graphic::getDefaultDimStyleName() const {
    return getVariableString("$DIMSTYLE", "Standard");
}

void RS_Graphic::setDefaultDimStyleName(const QString& name) {
    addVariable("$DIMSTYLE", name, 2);
}

LC_DimStyle* RS_Graphic::getEffectiveDimStyle(const QString& styleName, const RS2::EntityType dimType,
                                              const LC_DimStyle* styleOverride) const {
    const auto globalDimStyle = getResolvedDimStyle(styleName, dimType);
    LC_DimStyle* resolvedDimStyle = nullptr;
    if (styleOverride == nullptr) {
        resolvedDimStyle = globalDimStyle;
    }
    else {
        // NOTE: If there is style override, the returned instance SHOULD BE DELETED by caller code!!!
        // that's pretty ugly, yet avoid to eliminate additional copy operation for most cases, as
        // it's expected that style override is less commonly used feature comparing to just setting
        // existing styles to the dimension entity
        const auto styleOverrideCopy = styleOverride->getCopy();
        styleOverrideCopy->mergeWith(globalDimStyle, LC_DimStyle::ModificationAware::UNSET, LC_DimStyle::ModificationAware::UNSET);
        resolvedDimStyle = styleOverrideCopy;
    }
    return resolvedDimStyle;
}

LC_DimStyle* RS_Graphic::getEffectiveDimStyleForEdit(const QString& styleName, const RS2::EntityType dimType,
                                                     const LC_DimStyle* styleOverride) const {
    const auto globalDimStyle = getResolvedDimStyle(styleName, dimType);
    LC_DimStyle* resolvedDimStyle = nullptr;
    if (styleOverride == nullptr) {
        resolvedDimStyle = globalDimStyle->getCopy();
    }
    else {
        const auto styleOverrideCopy = styleOverride->getCopy();
        styleOverrideCopy->mergeWith(globalDimStyle, LC_DimStyle::ModificationAware::UNSET, LC_DimStyle::ModificationAware::UNSET);
        resolvedDimStyle = styleOverrideCopy;
    }
    return resolvedDimStyle;
}

LC_DimStyle* RS_Graphic::getResolvedDimStyle(const QString& dimStyleName, const RS2::EntityType dimType) const {
    LC_DimStyle* result = nullptr;
    if (dimStyleName != nullptr && !dimStyleName.isEmpty()) {
        // try to get style by explicit name, if any
        result = getDimStyleByName(dimStyleName, dimType);
        if (result != nullptr) {
            return result;
        }
    }

    // try to find a style that is set as default
    const QString defaultStyleName = getDefaultDimStyleName();
    if (defaultStyleName != nullptr && !defaultStyleName.isEmpty()) {
        result = getDimStyleByName(defaultStyleName, dimType);
        if (result != nullptr) {
            return result;
        }
    }
    // nothing found, get default dim style from vars
    return getFallBackDimStyleFromVars();
}

void RS_Graphic::updateFallbackDimStyle(const LC_DimStyle* fromStyle) {
    if (fromStyle != nullptr) {
        LC_DimStyle* fallBackStyle = getFallBackDimStyleFromVars();
        fromStyle->copyTo(fallBackStyle);

        LC_DimStyleToVariablesMapper mapper;
        mapper.toDictionary(fallBackStyle, getVariableDictObjectRef());
    }
}

void RS_Graphic::replaceDimStylesList(const QString& defaultStyleName, const QList<LC_DimStyle*>& styles) {
    setDefaultDimStyleName(defaultStyleName);
    m_dimstyleList.replaceStyles(styles);
    LC_DimArrowRegistry::insertStandardArrowBlocks(this, styles);
}

void RS_Graphic::prepareForSave() {
    // fixme - sand - check what if dimension
    const auto entities = lc::LC_ContainerTraverser{*this, RS2::ResolveNone}.entities();
    for (const auto e : entities) {
        if (e->isDeleted()) {
            continue;
        }
        QList<LC_DimStyle*> dimStyleOverrides;
        const RS2::EntityType rtti = e->rtti();
        if (RS2::isDimensionalEntity(rtti)) {
            const auto dim = dynamic_cast<RS_Dimension*>(e);
            if (dim != nullptr) {
                LC_DimStyle* override = dim->getDimStyleOverride();
                if (override != nullptr) {
                    dimStyleOverrides.append(override);
                }
            }
        }
        if (!dimStyleOverrides.isEmpty()) {
            LC_DimArrowRegistry::insertStandardArrowBlocks(this, dimStyleOverrides);
        }
    }
}
