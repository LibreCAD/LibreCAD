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

#include <iostream>

#include "rs_graphic.h"

#include "dxf_format.h"
#include "lc_containertraverser.h"
#include "lc_dimstyletovariablesmapper.h"
#include "lc_defaults.h"
#include "lc_dimarrowregistry.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "lc_dimstyleslist.h"
#include "rs_dimension.h"

namespace {
// default paper size A4: 210x297 mm
const RS_Vector g_paperSizeA4{210., 297.};

// validate coordinates
bool validCoordinate(double x){
    return x >= RS_MINDOUBLE && x <= RS_MAXDOUBLE;
}

// validate vector
bool validRange(const RS_Vector& vp){
    return vp.valid && validCoordinate(vp.x) && validCoordinate(vp.y);
}

// validate vpMin and vpMax forms a valid bounding box
bool validRange(const RS_Vector& vpMin, const RS_Vector& vpMax){
    bool validCoords = validRange(vpMin) && validRange(vpMax);
    if (validCoords) {
        bool xValid{false};
        double xDelta = vpMax.x - vpMin.x; // vpMax.x == vpMin.x is also valid for vertical lines
        if (std::signbit(xDelta)) {
            xValid = std::abs(xDelta) < RS_TOLERANCE; //
        }
        else {
            xValid = true;
        }
        if (xValid) {
            bool yValid{false};
            double yDelta = vpMax.y - vpMin.y; // vpMax.x == vpMin.x is also valid for horizontal lines
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
        : RS_Document(parent),
        layerList(),
        blockList(true),
        paperScaleFixed(false),
        marginLeft(0.0),
        marginTop(0.0),
        marginRight(0.0),
        marginBottom(0.0),
        pagesNumH(1),
        pagesNumV(1)
        , autosaveFilename{ "Unnamed"}{

    LC_GROUP_GUARD("Defaults");
    {
        setUnit(RS_Units::stringToUnit(LC_GET_ONE_STR("Defaults", "Unit", "None")));
        addVariable("$SNAPSTYLE", static_cast<int>(LC_GET_INT("IsometricGrid", 0)), 70);
        addVariable("$SNAPISOPAIR", static_cast<int>(LC_GET_INT("IsoGridView", 1)), 70);
        setGridOn(!LC_GET_BOOL("GridOffForNewDrawing", false));

        const QString &defaultAnglesBase = LC_GET_STR("AnglesBaseAngle", "0.0");
        bool anglesCounterClockwise = LC_GET_BOOL("AnglesCounterClockwise", true);

        double angleBaseDegrees = RS_Math::eval(defaultAnglesBase, 0.0);
        double angleBaseRadians = RS_Math::deg2rad(angleBaseDegrees);
        setAnglesCounterClockwise(anglesCounterClockwise);
        setAnglesBase(angleBaseRadians);
    }
    RS2::Unit unit = getUnit();

    if (unit == RS2::Inch) {
        addVariable("$DIMASZ", 0.1, DXF_FORMAT_GC_DimASz);
        addVariable("$DIMEXE", 0.05, DXF_FORMAT_GC_DimEXE);
        addVariable("$DIMEXO", 0.025, DXF_FORMAT_GC_DimExO);
        addVariable("$DIMGAP", 0.025, DXF_FORMAT_GC_DimGap);
        addVariable("$DIMTXT", 0.1, DXF_FORMAT_GC_DimTxt);
    } else {
        addVariable("$DIMASZ",
                    RS_Units::convert(2.5, RS2::Millimeter, unit), DXF_FORMAT_GC_DimASz);
        addVariable("$DIMEXE",
                    RS_Units::convert(1.25, RS2::Millimeter, unit), DXF_FORMAT_GC_DimEXE);
        addVariable("$DIMEXO",
                    RS_Units::convert(0.625, RS2::Millimeter, unit), DXF_FORMAT_GC_DimExO);
        addVariable("$DIMGAP",
                    RS_Units::convert(0.625, RS2::Millimeter, unit), DXF_FORMAT_GC_DimGap);
        addVariable("$DIMTXT",
                    RS_Units::convert(2.5, RS2::Millimeter, unit), DXF_FORMAT_GC_DimTxt);
    }
    addVariable("$DIMTIH", 0, DXF_FORMAT_GC_DimTIH);
    //initialize printer vars bug #3602444
    setPaperScale(getPaperScale());
    setPaperInsertionBase(getPaperInsertionBase());

    //set default values for point style
    addVariable("$PDMODE", LC_DEFAULTS_PDMode, DXF_FORMAT_GC_PDMode);
    addVariable("$PDSIZE", LC_DEFAULTS_PDSize, DXF_FORMAT_GC_PDSize);

    addVariable("$JOINSTYLE", 1, DXF_FORMAT_GC_JoinStyle);
    addVariable("$ENDCAPS", 1, DXF_FORMAT_GC_Endcaps);
    modified = false;
}

/**
 * Destructor.
 */
RS_Graphic::~RS_Graphic() = default;

void RS_Graphic::onLoadingCompleted() {
    auto fallBackDimStyleFromVars = dimstyleList.getFallbackDimStyleFromVars();
    fallBackDimStyleFromVars->fillByDefaults(); // cleanup (is it redundant?)
    LC_DimStyleToVariablesMapper dimStyleToVariablesMapper;
    dimStyleToVariablesMapper.fromDictionary(fallBackDimStyleFromVars, getVariableDictObjectRef(), getUnit());

    if (dimstyleList.isEmpty()) { // add content of vars to dimstyle list, to ensure that we have at least one style there
        dimstyleList.addDimStyle(fallBackDimStyleFromVars->getCopy());
    }
    else {
        // some programs (like AutoCAD) may store in DXF not all fields for the dim style definition, but only
        // modified ones.
        // For example, for dimension specific styles (like linear, ordinal, etc) only variables with values that
        // are different to values in the base style is store.
        // Therefore, for dimension type-specific styles, we perform a merge of non-set variables with values
        // from base style.
        dimstyleList.mergeStyles();
    }
    updateDimensions(true);
}

/**
 * Counts the m_entities on the given layer.
 */
unsigned RS_Graphic::countLayerEntities(RS_Layer *layer) const
{
    unsigned c = 0;
    if (layer) {
        for (RS_Entity *t: *this) {
            if (t->getLayer() &&
                t->getLayer()->getName() == layer->getName()) {
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
    if (layer != nullptr) {
        const QString &layerName = layer->getName();
        if (layerName != "0") {
            std::vector<RS_Entity *> toRemove;
            //find entities on layer
            for(RS_Entity *e: *this) {
                if (e->getLayer() &&
                    e->getLayer()->getName() == layerName) {
                    toRemove.push_back(e);
                }
            }
            // remove all entities on that layer:
            if (!toRemove.empty()) {
                startUndoCycle();
                for (RS_Entity *e: toRemove) {
                    e->setUndoState(true);
                    e->setLayer("0");
                    addUndoable(e);
                }
                endUndoCycle();
            }

            toRemove.clear();
            // remove all entities in blocks that are on that layer:
            for (RS_Block *blk: blockList) {
                if (!blk) continue;
                for (auto e: *blk) {
                    if (e->getLayer() &&
                        e->getLayer()->getName() == layerName) {
                        toRemove.push_back(e);
                    }
                }
            }

            for (RS_Entity *e: toRemove) {
                e->setUndoState(true);
                e->setLayer("0");
            }

            layerList.remove(layer);
        }
    }
}

/**
 * Clears all layers, blocks and entities of this graphic.
 * A default layer (0) is created.
 */
void RS_Graphic::newDoc() {
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

QString RS_Graphic::getCustomProperty(const QString &key, const QString& defaultValue) {
   return m_customVariablesDict.getString(key, defaultValue);
}

void RS_Graphic::addCustomProperty(const QString &key, const QString& value) {
    m_customVariablesDict.add(key, value, 1);
    setModified(true);
}

void RS_Graphic::removeCustomProperty(const QString &key) {
    m_customVariablesDict.remove(key);
}

bool RS_Graphic::hasCustomProperty(const QString& key) {
    return m_customVariablesDict.has(key);
}

const QHash<QString, RS_Variable> & RS_Graphic::getCustomProperties() const {
    return m_customVariablesDict.getVariableDict();
}

int RS_Graphic::countVariables() const{
    return m_variableDict.count();
}

void RS_Graphic::addVariable(const QString& key, const RS_Vector& value, int code) {
    m_variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, const QString& value, int code) {
    m_variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, int value, int code) {
    m_variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, bool value, int code) {
    m_variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, double value, int code) {
    m_variableDict.add(key, value, code);
}

void RS_Graphic::removeVariable(const QString& key) {
    m_variableDict.remove(key);
}

RS_Vector RS_Graphic::getVariableVector(const QString& key, const RS_Vector& def) const {
    return m_variableDict.getVector(key, def);
}

QString RS_Graphic::getVariableString(const QString& key, const QString& def) const {
    return m_variableDict.getString(key, def);
}

int RS_Graphic::getVariableInt(const QString& key, int def) const {
    return m_variableDict.getInt(key, def);
}

bool RS_Graphic::getVariableBool(const QString& key, bool def) const {
    return m_variableDict.getInt(key, def ? 1 : 0) != 0;
}

double RS_Graphic::getVariableDouble(const QString& key, double def) const {
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
    int on = getVariableInt("$GRIDMODE", 1);
    return on != 0;
//    return gridOn;
}

/**
 * Enables / disables the grid.
 */
void RS_Graphic::setGridOn(bool on) {
//    gridOn = on;
    addVariable("$GRIDMODE", (int)on, 70);
}

/**
 * @return true if the isometric grid is switched on (visible).
 */
bool RS_Graphic::isIsometricGrid() const{
    //$ISOMETRICGRID == $SNAPSTYLE
    int on = getVariableInt("$SNAPSTYLE", 0);
    return on!=0;
}

/**
 * Enables / disables isometric grid.
 */
void RS_Graphic::setIsometricGrid(bool on) {
    //$ISOMETRICGRID == $SNAPSTYLE
    addVariable("$SNAPSTYLE", (int)on, 70);
}

double RS_Graphic::getAnglesBase() const{
    double result = getVariableDouble("$ANGBASE",0.0);
    return result;
}

void RS_Graphic::setAnglesBase(double baseAngle){
    addVariable("$ANGBASE", baseAngle, 50);
}

bool RS_Graphic::areAnglesCounterClockWise() const{
    int angDir = getVariableInt("$ANGDIR", 0);
    return angDir == 0;
}

void RS_Graphic::setAnglesCounterClockwise(bool on){
    addVariable("$ANGDIR", on ? 0: 1, 70);
}

void RS_Graphic::setCurrentUCS(LC_UCS* ucs){
    QString name = ucs->getName();
    if (!ucs->isUCS()){
        name = "";
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
    QString name = getVariableString("$UCSNAME", "");
    RS_Vector origin = getVariableVector("$UCSORG", RS_Vector(0.0, 0.0));
    int orthoType = getVariableInt("$UCSORTHOVIEW", 0);
    RS_Vector xAxis = getVariableVector("$UCSXDIR", RS_Vector(1.0, 0.0));
    RS_Vector yAxis = xAxis;
    yAxis = getVariableVector("$UCSYDIR", yAxis.rotate(M_PI_2));

    LC_UCS* wcs = ucsList.getWCS();

    auto result = new LC_UCS(name);
    result->setOrigin(origin);
    result->setOrthoType(orthoType);
    result->setXAxis(xAxis);
    result->setYAxis(yAxis);

    if (wcs->isSameTo(result)){
        delete result;
        result = new LC_WCS();
    }
    return result;
}

RS2::IsoGridViewType RS_Graphic::getIsoView() const{
    RS2::IsoGridViewType result = (RS2::IsoGridViewType) getVariableInt("$SNAPISOPAIR", RS2::IsoGridViewType::IsoTop);
    return result;
}

void RS_Graphic::setIsoView(RS2::IsoGridViewType viewType){
    addVariable("$SNAPISOPAIR", viewType, 70);
}

/**
 * Sets the unit of this graphic to 'u'
 */
void RS_Graphic::setUnit(RS2::Unit u) {
    // fixme - sand - add caching
    setPaperSize(RS_Units::convert(getPaperSize(), getUnit(), u));
    addVariable("$INSUNITS", (int)u, 70);
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
RS2::LinearFormat RS_Graphic::getLinearFormat() const{
    // fixme - sand - add caching
    int lunits = getVariableInt("$LUNITS", 2);
    return convertLinearFormatDXF2LC(lunits);
}

void RS_Graphic::replaceCustomVars(const QHash<QString, QString>& vars) {
    m_customVariablesDict.clear();
    QHashIterator<QString,QString> customVar(vars);
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
RS2::LinearFormat RS_Graphic::convertLinearFormatDXF2LC(int f){
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
int RS_Graphic::getLinearPrecision() const{
    // fixme - sand - add caching
    return getVariableInt("$LUPREC", 4);
}

/**
 * @return The angle format type for this document.
 * This is determined by the variable "$AUNITS".
 */
RS2::AngleFormat RS_Graphic::getAngleFormat() const
{
    // fixme - sand - add caching
    int aunits = getVariableInt("$AUNITS", 0);

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

/**
 * @return The linear precision for this document.
 * This is determined by the variable "$LUPREC".
 */
int RS_Graphic::getAnglePrecision() const {
    // fixme - sand - add caching
    return getVariableInt("$AUPREC", 4);
}

/**
 * @return The insertion point of the drawing into the paper space.
 * This is the distance from the lower left paper edge to the zero
 * point of the drawing. DXF: $PINSBASE.
 */
RS_Vector RS_Graphic::getPaperInsertionBase() {
    return getVariableVector("$PINSBASE", RS_Vector(0.0,0.0));
}

/**
 * Sets the PINSBASE variable.
 */
void RS_Graphic::setPaperInsertionBase(const RS_Vector& p) {
    addVariable("$PINSBASE", p, 10);
}

/**
 * @return Paper size in graphic units.
 */
RS_Vector RS_Graphic::getPaperSize() const
{
    bool okX = false,okY = false;
    double sX = 0., sY = 0.;
    LC_GROUP_GUARD("Print");
    {
        sX = LC_GET_STR("PaperSizeX", "0.0").toDouble(&okX);
        sY = LC_GET_STR("PaperSizeY", "0.0").toDouble(&okY);
    }
    RS_Vector def ;
    if(okX&&okY && sX>RS_TOLERANCE && sY>RS_TOLERANCE) {
        def=RS_Units::convert(RS_Vector(sX,sY),
                              RS2::Millimeter, getUnit());
    }else{
        def= RS_Units::convert(g_paperSizeA4,
                               RS2::Millimeter, getUnit());
    }

    RS_Vector v1 = getVariableVector("$PLIMMIN", RS_Vector(0.0,0.0));
    RS_Vector v2 = getVariableVector("$PLIMMAX", def);

    return v2-v1;
}

/**
 * Sets a new paper size.
 */
void RS_Graphic::setPaperSize(const RS_Vector& s) {
    addVariable("$PLIMMIN", RS_Vector(0.0,0.0), 10);
    addVariable("$PLIMMAX", s, 10);
    //set default paper size
    RS_Vector def = RS_Units::convert(s,
                                     getUnit(), RS2::Millimeter);
    LC_GROUP_GUARD("Print");
    {
        LC_SET("PaperSizeX", def.x);
        LC_SET("PaperSizeY", def.y);
    }
}

/**
 * @return Print Area size in graphic units.
 */
RS_Vector RS_Graphic::getPrintAreaSize(bool total) const {
    RS_Vector printArea = getPaperSize();
    RS2::Unit dest = getUnit();
    printArea.x -= RS_Units::convert(marginLeft + marginRight, RS2::Millimeter, dest);
    printArea.y -= RS_Units::convert(marginTop+marginBottom, RS2::Millimeter, dest);
    if (total) {
        printArea.x *= pagesNumH;
        printArea.y *= pagesNumV;
    }
    return printArea;
}

/**
 * @return Paper format.
 * This is determined by the variables "$PLIMMIN" and "$PLIMMAX".
 *
 * @param landscape will be set to true for landscape and false for portrait if not nullptr.
 */
RS2::PaperFormat RS_Graphic::getPaperFormat(bool* landscape) {
    RS_Vector size = RS_Units::convert(getPaperSize(),
                                       getUnit(), RS2::Millimeter);
    if (landscape) {
        *landscape = (size.x>size.y);
    }
    return RS_Units::paperSizeToFormat(size);
}

/**
 * Sets the paper format to the given format.
 */
void RS_Graphic::setPaperFormat(RS2::PaperFormat f, bool landscape) {
    RS_Vector size = RS_Units::paperFormatToSize(f);

    if (landscape != (size.x > size.y)) {
        std::swap(size.x, size.y);
    }

    setPaperSize(RS_Units::convert(size, RS2::Millimeter, getUnit()));
}

/**
 * @return Paper space scaling (DXF: $PSVPSCALE).
 */
double RS_Graphic::getPaperScale() const {
    double paperScale = getVariableDouble("$PSVPSCALE", 1.0);
    return paperScale;
}

/**
 * Sets a new scale factor for the paper space.
 */
void RS_Graphic::setPaperScale(double s) {
    if(paperScaleFixed==false) {
        addVariable("$PSVPSCALE", s, 40);
    }
}

/**
 * Centers drawing on page. Affects DXF variable $PINSBASE.
 */
void RS_Graphic::centerToPage() {
    RS_Vector paperSize = getPrintAreaSize();
    auto graphicSize=getSize();
    auto graphicMin=getMin();
    /** avoid zero size, bug#3573158 */
    if(std::abs(graphicSize.x)<RS_TOLERANCE) {
        graphicSize.x=10.;
        graphicMin.x=-5.;
    }
    if(std::abs(graphicSize.y)<RS_TOLERANCE) {
        graphicSize.y=10.;
        graphicMin.y=-5.;
    }

    const RS2::Unit unit = getUnit();
    RS_Vector paperMin{
        RS_Units::convert(marginLeft, RS2::Millimeter, unit),
        RS_Units::convert(marginBottom, RS2::Millimeter, unit)
    };
    // paper printable area center
    RS_Vector paperCenter = paperMin + paperSize * 0.5;
    const double scale = getPaperScale();
    // graphic center
    RS_Vector scaledCenter = (graphicMin + graphicSize * 0.5) * scale;

    // align graphic center to the paper center
    RS_Vector pinsbase = paperCenter - scaledCenter;

    setPaperInsertionBase(pinsbase);
}

/**
 * Fits drawing on page. Affects DXF variable $PINSBASE.
 */
  // fixme - check margins support in single and tiled modes - looks like they are shown differently
bool RS_Graphic::fitToPage()
{
    RS_Vector printSize = getPrintAreaSize(false);
    RS_Vector graphicSize = getSize();
    /** avoid zero size, bug#3573158 */
    if(std::abs(graphicSize.x)<RS_TOLERANCE)
        graphicSize.x=10.;
    if(std::abs(graphicSize.y)<RS_TOLERANCE)
        graphicSize.y=10.;
    double scaleX = RS_MAXDOUBLE;
    double scaleY = RS_MAXDOUBLE;

    // tin-pot 2011-12-30: TODO: can s.x < 0.0 (==> fx < 0.0) happen?
    if (std::abs(graphicSize.x) > RS_TOLERANCE) {
        scaleX = printSize.x / graphicSize.x;
    }
    if (std::abs(graphicSize.y) > RS_TOLERANCE) {
        scaleY = printSize.y / graphicSize.y;
    }

    double scale = std::min(scaleX, scaleY);
    if (scale >= RS_MAXDOUBLE || scale <= 1.0e-10) {
        setPaperSize(RS_Units::convert(g_paperSizeA4, RS2::Millimeter, getUnit()));
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Invalid printing scale %1. Cannot fit print preview to page").arg(scale));
        return false;
    }
    setPaperScale(scale);
    centerToPage();
    return true;
}

bool RS_Graphic::isBiggerThanPaper() {
    RS_Vector ps = getPrintAreaSize();
    RS_Vector s = getSize() * getPaperScale();
    return !s.isInWindow(RS_Vector(0.0, 0.0), ps);
}

void RS_Graphic::addEntity(RS_Entity *entity) {
    RS_EntityContainer::addEntity(entity);
    if (entity->rtti() == RS2::EntityBlock ||
        entity->rtti() == RS2::EntityContainer) {
        auto *e = dynamic_cast<RS_EntityContainer *>(entity);
        for (auto e1: *e) {
            addEntity(e1);
        }
    }
}

/**
 * Dumps the entities to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Graphic& g) {
    os << "--- Graphic: \n";
    os << "---" << *g.getLayerList() << "\n";
    os << "---" << *g.getBlockList() << "\n";
    os << "---" << (RS_Undo&)g << "\n";
    os << "---" << (RS_EntityContainer&)g << "\n";
    return os;
}

/**
 * Removes invalid objects.
 * @return how many objects were removed
 */
int RS_Graphic::clean() {
    int how_many = 0;

    forcedCalculateBorders();

    for(RS_Entity *e: std::as_const(*this)) {
        if (e != nullptr) {
            if (!validRange(e->getMin(), e->getMax())) {
                // fixme - sand - files restore, the issue with
                // removeEntity(e);
                how_many += 1;
            }
        }
    }
    return how_many;
}

/**
 * Paper margins in graphic units
 */
void RS_Graphic::setMarginsInUnits(double left, double top, double right, double bottom) {
    RS2::Unit src = getUnit();
    setMargins(RS_Units::convert(left, src, RS2::Millimeter),
               RS_Units::convert(top, src, RS2::Millimeter),
               RS_Units::convert(right, src, RS2::Millimeter),
               RS_Units::convert(bottom, src, RS2::Millimeter));
}

double RS_Graphic::getMarginLeftInUnits() {
    return RS_Units::convert(marginLeft, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginTopInUnits() {
    return RS_Units::convert(marginTop, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginRightInUnits() {
    return RS_Units::convert(marginRight, RS2::Millimeter, getUnit());
}

double RS_Graphic::getMarginBottomInUnits() {
    return RS_Units::convert(marginBottom, RS2::Millimeter, getUnit());
}

void RS_Graphic::setPagesNum(int horiz, int vert) {
    if (horiz > 0)
        pagesNumH = horiz;
    if (vert > 0)
        pagesNumV = vert;
}

void RS_Graphic::setPagesNum(const QString &horizXvert) {
    if (horizXvert.contains('x')) {
        bool ok1 = false;
        bool ok2 = false;
        int i = horizXvert.indexOf('x');
        int h = (int)RS_Math::eval(horizXvert.left(i), &ok1);
        int v = (int)RS_Math::eval(horizXvert.mid(i+1), &ok2);
        if (ok1 && ok2)
            setPagesNum(h, v);
    }
}

QString RS_Graphic::formatAngle(double angle) const{
    return RS_Units::formatAngle(angle, getAngleFormat(), getAnglePrecision());
}

QString RS_Graphic::formatLinear(double linear) const{
    return RS_Units::formatLinear(linear, getUnit(), getLinearFormat(), getLinearPrecision(), false);
}

/**
  * @retval true The document has been modified since it was last saved.
  * @retval false The document has not been modified since it was last saved.
  */
bool RS_Graphic::isModified() const{
    return modified
           || layerList.isModified()
           || blockList.isModified()
           || namedViewsList.isModified()
           || ucsList.isModified()
           || dimstyleList.isModified()
        ;}

/**
 * Sets the documents modified status to 'm'.
 */
void RS_Graphic::setModified(bool m) {
    modified = m;
    if (!m) {
        layerList.setModified(m);
        blockList.setModified(m);
        namedViewsList.setModified(m);
        ucsList.setModified(m);
        dimstyleList.setModified(m);
    }
    if (m_modificationListener != nullptr) {
        m_modificationListener->graphicModified(this, m);
    }
}

void RS_Graphic::markSaved(const QDateTime &lastSaveTime){
    setModified(false);
    setLastSaveTime(lastSaveTime);
}

RS2::FormatType RS_Graphic::getFormatType() const {
    return formatType;
}

void RS_Graphic::setFormatType(RS2::FormatType formatType) {
    RS_Graphic::formatType = formatType;
}


const QString &RS_Graphic::getAutosaveFilename() const {
    return autosaveFilename;
}

void RS_Graphic::setAutosaveFileName(const QString &fileName) {
    autosaveFilename = fileName;
}

void RS_Graphic::fireUndoStateChanged(bool undoAvailable, bool redoAvailable) const{
    if (m_modificationListener != nullptr) {
        m_modificationListener->undoStateChanged(this, undoAvailable, redoAvailable);
    }
}


LC_DimStyle* RS_Graphic::getDimStyleByName(const QString& name, RS2::EntityType dimType) const {
    return dimstyleList.resolveByName(name, dimType);
}

LC_DimStyle* RS_Graphic::getFallBackDimStyleFromVars() const {
    return dimstyleList.getFallbackDimStyleFromVars();
}

QString RS_Graphic::getDefaultDimStyleName() const{
    return getVariableString("$DIMSTYLE", "Standard");
}

void RS_Graphic::setDefaultDimStyleName(QString name) {
    addVariable("$DIMSTYLE", name, 2);
}

LC_DimStyle* RS_Graphic::getEffectiveDimStyle(const QString &styleName, RS2::EntityType dimType, LC_DimStyle* styleOverride) const{
  auto globalDimStyle = getResolvedDimStyle(styleName, dimType);
    LC_DimStyle* resolvedDimStyle = nullptr;
    if (styleOverride == nullptr) {
        resolvedDimStyle = globalDimStyle;
    }
    else {
        // NOTE: If there is style override, the returned instance SHOULD BE DELETED by caller code!!!
        // that's pretty ugly, yet avoid to eliminate additinal copy operation for most cases, as
        // it's expected that style override is less commonly used feature comparing to just setting
        // existing styles to the dimension entity
        auto styleOverrideCopy = styleOverride->getCopy();
        styleOverrideCopy->mergeWith(globalDimStyle, LC_DimStyle::ModificationAware::UNSET, LC_DimStyle::ModificationAware::UNSET);
        resolvedDimStyle = styleOverrideCopy;
    }
    return resolvedDimStyle;
}

LC_DimStyle* RS_Graphic::getResolvedDimStyle(const QString &dimStyleName, RS2::EntityType dimType) const {
    LC_DimStyle* result = nullptr;
    if (dimStyleName != nullptr && !dimStyleName.isEmpty()) { // try to get style by explicit name, if any
        result = getDimStyleByName(dimStyleName, dimType);
        if (result != nullptr) {
            return result;
        }
    }

    // try to find a style that is set as default
    QString defaultStyleName = getDefaultDimStyleName();
    if (defaultStyleName != nullptr && !defaultStyleName.isEmpty()) {
        result =  getDimStyleByName(defaultStyleName, dimType);
        if (result != nullptr) {
            return result;
        }
    }
    // nothing found, get default dim style from vars
    return getFallBackDimStyleFromVars();
}

void RS_Graphic::updateFallbackDimStyle(LC_DimStyle* style) {
    if (style != nullptr) {
        LC_DimStyle* fallBackStyle = getFallBackDimStyleFromVars();
        style->copyTo(fallBackStyle);

        LC_DimStyleToVariablesMapper mapper;
        mapper.toDictionary(fallBackStyle, getVariableDictObjectRef());
    }
}

void RS_Graphic::replaceDimStylesList(const QString& defaultStyleName, const QList<LC_DimStyle*>& styles) {
    setDefaultDimStyleName(defaultStyleName);
    dimstyleList.replaceStyles(styles);
    LC_DimArrowRegistry::insertStandardArrowBlocks(this, styles);
}

void RS_Graphic::prepareForSave() {
    // fixme - sand - check what if dimension
    auto entities = lc::LC_ContainerTraverser{*this, RS2::ResolveNone}.entities();
    for (const auto e: entities) {
        if (e->isUndone()) {
            continue;
        }
        QList<LC_DimStyle*> dimStyleOverrides;
        RS2::EntityType rtti = e->rtti();
        if (RS2::isDimensionalEntity(rtti)) {
             auto dim = dynamic_cast<RS_Dimension*>(e);
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
