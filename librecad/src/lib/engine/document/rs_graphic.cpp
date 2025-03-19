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
#include <cmath>

#include <QDir>

#include "rs_graphic.h"

#include "dxf_format.h"
#include "lc_defaults.h"
#include "rs_block.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_actioninterface.h"
#include "rs_fileio.h"
#include "rs_layer.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "lc_graphicviewport.h"

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
        , autosaveFilename{ "Unnamed"} {

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
    setModified(false);
}

/**
 * Destructor.
 */
RS_Graphic::~RS_Graphic() = default;


/**
 * Counts the entities on the given layer.
 */
unsigned long int RS_Graphic::countLayerEntities(RS_Layer *layer) {
    int c = 0;
    if (layer) {
        for (RS_Entity *t: entities) {
            if (t->getLayer() &&
                t->getLayer()->getName() == layer->getName()) {
                c += t->countDeep();
            }
        }
    }
    return c;
}

/**
 * Removes the given layer and undoes all entities on it.
 */
void RS_Graphic::removeLayer(RS_Layer* layer) {
    if (layer != nullptr) {
        const QString &layerName = layer->getName();
        if (layerName != "0") {
            std::vector<RS_Entity *> toRemove;
//find entities on layer
            for (RS_Entity *e: entities) {
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
/**
 * Loads the given file into this graphic.
 */
bool RS_Graphic::open(const QString &filename, RS2::FormatType type) {
    // return graphicIo.open(this, filename, type);
}

void RS_Graphic::clearVariables() {
    variableDict.clear();
}

int RS_Graphic::countVariables() {
    return variableDict.count();
}

void RS_Graphic::addVariable(const QString& key, const RS_Vector& value, int code) {
    variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, const QString& value, int code) {
    variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, int value, int code) {
    variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, bool value, int code) {
    variableDict.add(key, value, code);
}

void RS_Graphic::addVariable(const QString& key, double value, int code) {
    variableDict.add(key, value, code);
}

void RS_Graphic::removeVariable(const QString& key) {
    variableDict.remove(key);
}

RS_Vector RS_Graphic::getVariableVector(const QString& key, const RS_Vector& def) const {
    return variableDict.getVector(key, def);
}

QString RS_Graphic::getVariableString(const QString& key, const QString& def) const {
    return variableDict.getString(key, def);
}

int RS_Graphic::getVariableInt(const QString& key, int def) const {
    return variableDict.getInt(key, def);
}

bool RS_Graphic::getVariableBool(const QString& key, bool def) const {
    return variableDict.getInt(key, def ? 1 : 0) != 0;
}

double RS_Graphic::getVariableDouble(const QString& key, double def) const {
    return variableDict.getDouble(key, def);
}

QHash<QString, RS_Variable>& RS_Graphic::getVariableDict() {
    return variableDict.getVariableDict();
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

double RS_Graphic::getAnglesBase(){
    double result = getVariableDouble("$ANGBASE",0.0);
    return result;
}

void RS_Graphic::setAnglesBase(double baseAngle){
    addVariable("$ANGBASE", baseAngle, 50);
}

bool RS_Graphic::areAnglesCounterClockWise(){
    int on = getVariableInt("$ANGDIR", 0);
    return on == 0;
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

LC_UCS* RS_Graphic::getCurrentUCS(){
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
RS2::LinearFormat RS_Graphic::getLinearFormat() {
    // fixme - sand - add caching
    int lunits = getVariableInt("$LUNITS", 2);
    return getLinearFormat(lunits);
}

/**
 * @return The linear format type used by the variable "$LUNITS" & "$DIMLUNIT".
 */
RS2::LinearFormat RS_Graphic::getLinearFormat(int f){
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
int RS_Graphic::getLinearPrecision() {
    // fixme - sand - add caching
    return getVariableInt("$LUPREC", 4);
}

/**
 * @return The angle format type for this document.
 * This is determined by the variable "$AUNITS".
 */
RS2::AngleFormat RS_Graphic::getAngleFormat() {
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
int RS_Graphic::getAnglePrecision() {
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
RS_Vector RS_Graphic::getPaperSize() {
    bool okX,okY;
    double sX, sY;
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
        def= RS_Units::convert(RS_Vector(210.0,297.0),
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
RS_Vector RS_Graphic::getPrintAreaSize(bool total) {
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
    RS_Vector size = getPrintAreaSize();
    double scale = getPaperScale();
    auto s=getSize();
    auto sMin=getMin();
    /** avoid zero size, bug#3573158 */
    if(std::abs(s.x)<RS_TOLERANCE) {
        s.x=10.;
        sMin.x=-5.;
    }
    if(std::abs(s.y)<RS_TOLERANCE) {
        s.y=10.;
        sMin.y=-5.;
    }

    RS_Vector pinsbase = (size-s*scale)/2.0 - sMin*scale;
    RS2::Unit unit = getUnit();
    pinsbase.x += RS_Units::convert(marginLeft, RS2::Millimeter, unit);
    pinsbase.y += RS_Units::convert(marginBottom, RS2::Millimeter, unit);

    setPaperInsertionBase(pinsbase);
}

/**
 * Fits drawing on page. Affects DXF variable $PINSBASE.
 */
  // fixme - check margins support in single and tiled modes - looks like they are shown differently
bool RS_Graphic::fitToPage() {
    bool ret = true;
    RS_Vector ps = getPrintAreaSize(false);
    RS_Vector s = getSize();
    /** avoid zero size, bug#3573158 */
    if(std::abs(s.x)<RS_TOLERANCE) s.x=10.;
    if(std::abs(s.y)<RS_TOLERANCE) s.y=10.;
    double fx = RS_MAXDOUBLE;
    double fy = RS_MAXDOUBLE;
    //ps = RS_Units::convert(ps, getUnit(), RS2::Millimeter);

    // tin-pot 2011-12-30: TODO: can s.x < 0.0 (==> fx < 0.0) happen?
    if (std::abs(s.x) > RS_TOLERANCE) {
        fx = ps.x / s.x;
        // ret=false;
    }
    if (std::abs(s.y) > RS_TOLERANCE) {
        fy = ps.y / s.y;
        // ret=false;
    }

    double fxy = std::min(fx, fy);
    if (fxy >= RS_MAXDOUBLE || fxy <= 1.0e-10) {
        setPaperSize(RS_Units::convert(RS_Vector(210.,297.), RS2::Millimeter, getUnit()));
        ret=false;
    }
    setPaperScale(fxy);
    centerToPage();
    return ret;
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
    // author: ravas

    int how_many = 0;

        foreach (RS_Entity *e, entities) {
            const RS_Vector &min = e->getMin();
            const RS_Vector &max = e->getMax();
            if (min.x > max.x
                || min.y > max.y
                || min.x > RS_MAXDOUBLE
                || max.x > RS_MAXDOUBLE
                || min.x < RS_MINDOUBLE
                || max.x < RS_MINDOUBLE
                || min.y > RS_MAXDOUBLE
                || max.y > RS_MAXDOUBLE
                || min.y < RS_MINDOUBLE
                || max.y < RS_MINDOUBLE) {
                removeEntity(e);
                how_many += 1;
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

QString RS_Graphic::formatAngle(double angle) {
    return RS_Units::formatAngle(angle, getAngleFormat(), getAnglePrecision());
}

QString RS_Graphic::formatLinear(double linear) {
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
    }
    if (modificationListener != nullptr) {
        modificationListener->graphicModified(this, m);
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
