/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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


#include "lc_linemath.h"
#include "rs_actioninterface.h"
#include "rs_selection.h"
#include "rs_preview.h"
#include "rs_overlaybox.h"
#include "rs_modification.h"
#include "rs_commandevent.h"
#include "rs_actiondefault.h"
#include "qc_applicationwindow.h"
#include <algorithm>
#include<cmath>

#include<QMouseEvent>

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_snapper.h"
#include "lc_crosshair.h"
#include "lc_defaults.h"
#include "lc_cursoroverlayinfo.h"
#include "rs_units.h"

namespace {

    // whether a floating point is positive by tolerance
    bool isPositive(double x)
    {
        return x > RS_TOLERANCE;
    }

    // A size vector is valid with a positive size
    bool isSizeValid(const RS_Vector& sizeVector) {
        return isPositive(sizeVector.x) || isPositive(sizeVector.x);
    }

    // The valid size magnitude
    double getValidSize(const RS_Vector& sizeVector){
        return std::hypot(std::max(sizeVector.x, RS_TOLERANCE), std::max(sizeVector.y, RS_TOLERANCE));
    }

    // get catching entity distance in graph distance
    double getCatchDistance(double catchDistance, int catchEntityGuiRange, RS_GraphicView* view){
        return (view != nullptr) ? std::min(catchDistance, view->toGraphDX(catchEntityGuiRange)) : catchDistance;
    }
}

/**
  * Disable all snapping.
  *
  * This effectively puts the object into free snap mode.
  *
  * @returns A reference to itself.
  */
RS_SnapMode const & RS_SnapMode::clear(){
    snapIntersection    = false;
    snapOnEntity        = false;
    snapCenter          = false;
    snapDistance        = false;
    snapMiddle          = false;
    snapEndpoint        = false;
    snapGrid            = false;
    snapFree            = false;
    snapAngle           = false;

    restriction = RS2::RestrictNothing;

    return *this;
}

bool RS_SnapMode::operator ==(RS_SnapMode const& rhs) const{
    return snapIntersection == rhs.snapIntersection
           && snapOnEntity == rhs.snapOnEntity
           && snapCenter   == rhs.snapCenter
           && snapDistance == rhs.snapDistance
           && snapMiddle   == rhs.snapMiddle
           && snapEndpoint == rhs.snapEndpoint
           && snapGrid     == rhs.snapGrid
           && snapFree     == rhs.snapFree
           && restriction  == rhs.restriction
           && snapAngle    == rhs.snapAngle;
}

bool RS_SnapMode::operator !=(RS_SnapMode const& rhs) const{
    return ! this->operator ==(rhs);
}

/**
  * snap mode to a flag integer
  */
uint RS_SnapMode::toInt(const RS_SnapMode& s){
    uint ret {0};

    if (s.snapIntersection) ret |= RS_SnapMode::SnapIntersection;
    if (s.snapOnEntity)     ret |= RS_SnapMode::SnapOnEntity;
    if (s.snapCenter)       ret |= RS_SnapMode::SnapCenter;
    if (s.snapDistance)     ret |= RS_SnapMode::SnapDistance;
    if (s.snapMiddle)       ret |= RS_SnapMode::SnapMiddle;
    if (s.snapEndpoint)     ret |= RS_SnapMode::SnapEndpoint;
    if (s.snapGrid)         ret |= RS_SnapMode::SnapGrid;
    if (s.snapFree)         ret |= RS_SnapMode::SnapFree;
    if (s.snapAngle)        ret |= RS_SnapMode::SnapAngle;

    switch (s.restriction) {
        case RS2::RestrictHorizontal:
            ret |= RS_SnapMode::RestrictHorizontal;
            break;
        case RS2::RestrictVertical:
            ret |= RS_SnapMode::RestrictVertical;
            break;
        case RS2::RestrictOrthogonal:
            ret |= RS_SnapMode::RestrictOrthogonal;
            break;
        default:
            break;
    }

    return ret;
}

/**
  * integer flag to snapMode
  */
RS_SnapMode RS_SnapMode::fromInt(unsigned int ret){
    RS_SnapMode s;

    if (RS_SnapMode::SnapIntersection   & ret) s.snapIntersection = true;
    if (RS_SnapMode::SnapOnEntity       & ret) s.snapOnEntity = true;
    if (RS_SnapMode::SnapCenter         & ret) s.snapCenter = true;
    if (RS_SnapMode::SnapDistance       & ret) s.snapDistance = true;
    if (RS_SnapMode::SnapMiddle         & ret) s.snapMiddle = true;
    if (RS_SnapMode::SnapEndpoint       & ret) s.snapEndpoint = true;
    if (RS_SnapMode::SnapGrid           & ret) s.snapGrid = true;
    if (RS_SnapMode::SnapFree           & ret) s.snapFree = true;
    if (RS_SnapMode::SnapAngle          & ret) s.snapAngle = true;

    switch (RS_SnapMode::RestrictOrthogonal & ret) {
        case RS_SnapMode::RestrictHorizontal:
            s.restriction = RS2::RestrictHorizontal;
            break;
        case RS_SnapMode::RestrictVertical:
            s.restriction = RS2::RestrictVertical;
            break;
        case RS_SnapMode::RestrictOrthogonal:
            s.restriction = RS2::RestrictOrthogonal;
            break;
        default:
            s.restriction = RS2::RestrictNothing;
            break;
    }

    return s;
}

/**
  * Methods and structs for class RS_Snapper
  */
struct RS_Snapper::Indicator{
    bool drawLines = false;
    int lines_type;
    RS_Pen lines_pen;

    bool drawShape = false;
    int shape_type;
    RS_Pen shape_pen;
    
    int pointType = LC_DEFAULTS_PDMode;
    int pointSize = LC_DEFAULTS_PDSize;
};

//struct RS_Snapper::

enum SnapType{
    FREE = -1,
    GRID,
    ENTITY,
    ENDPOINT,
    INTERSECTION,
    MIDDLE,
    DISTANCE,
    CENTER,
    ANGLE,
    ANGLE_ON_ENTITY
};

struct RS_Snapper::ImpData {
    RS_Vector snapCoord;
    RS_Vector snapSpot;
    int snapType;
    double angle;
    int restriction = RS2::RestrictNothing;
};

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(RS_EntityContainer& container, RS_GraphicView& graphicView)
    :container(&container)
    ,graphicView(&graphicView)
    ,pImpData(new ImpData)
    ,snap_indicator(new Indicator)
{}

RS_Snapper::~RS_Snapper() = default;
RS_Entity *catchEntity(const RS_Vector &coord, const EntityTypeList &enTypeList, RS2::ResolveLevel level);

QString getSnapName(int snapType);


/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init(){
    snapMode = graphicView->getDefaultSnapMode();
    keyEntity = nullptr;
    pImpData->snapSpot = RS_Vector{false};
    pImpData->snapCoord = RS_Vector{false};
    m_SnapDistance = 1.0;
    RS2::LineType snapIndicatorLineType;
    int snapIndicatorLineWidth;
    LC_GROUP("Appearance");
    {
        snapIndicatorLineWidth = static_cast<RS2::LineType> (LC_GET_INT("indicator_lines_line_width", 1));
        snap_indicator->drawLines = LC_GET_BOOL("indicator_lines_state", true);
        if (snap_indicator->drawLines){
            snap_indicator->lines_type = LC_GET_INT("indicator_lines_type", 0);
            snapIndicatorLineType = static_cast<RS2::LineType> (LC_GET_INT("indicator_lines_line_type", RS2::DashLine));
            QString snap_color_lines = LC_GET_ONE_STR("Colors", "snap_indicator_lines", RS_Settings::snap_indicator_lines);
            snap_indicator->lines_pen = RS_Pen(RS_Color(snap_color_lines), RS2::Width00, snapIndicatorLineType);
            snap_indicator->lines_pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else {
            snap_indicator->lines_type = LC_Crosshair::NoLines;
        }

        snap_indicator->drawShape = LC_GET_BOOL("indicator_shape_state", true);
        if (snap_indicator->drawShape) {
            snap_indicator->shape_type = LC_GET_INT("indicator_shape_type", 0);
            RS_Graphic *graphic = graphicView->getGraphic();
            if (graphic != nullptr) {
                snap_indicator->pointType = graphic->getVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
                snap_indicator->pointSize = graphic->getVariableInt("$PDSIZE", LC_DEFAULTS_PDSize);
            }
            QString snap_color = LC_GET_ONE_STR("Colors", "snap_indicator", RS_Settings::snap_indicator);
            snap_indicator->shape_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::SolidLine);
            snap_indicator->shape_pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else{
            snap_indicator->shape_type = LC_Crosshair::NoShape;
        }
    }
    LC_GROUP_END();


    catchEntityGuiRange = LC_GET_ONE_INT("Snapping", "CatchEntityGuiDistance", 32); // fixme - sand - add to option ui?

    RS_Graphic* graphic = graphicView->getGraphic();
    linearFormat = graphic->getLinearFormat();
    linearPrecision = graphic->getLinearPrecision();
    angleFormat = graphic->getAngleFormat();
    anglePrecision = graphic->getAnglePrecision();
    unit = graphic->getUnit();
}

void RS_Snapper::finish() {
    finished = true;
    deleteSnapper();
    deleteInfoCursor();
}

void RS_Snapper::setSnapMode(const RS_SnapMode& snapMode) {
    this->snapMode = snapMode;
    RS_DIALOGFACTORY->requestSnapDistOptions(&m_SnapDistance, snapMode.snapDistance);
    RS_DIALOGFACTORY->requestSnapMiddleOptions(&middlePoints, snapMode.snapMiddle);
//std::cout<<"RS_Snapper::setSnapMode(): middlePoints="<<middlePoints<<std::endl;
}


RS_SnapMode const* RS_Snapper::getSnapMode() const{
    return &(this->snapMode);
}

RS_SnapMode* RS_Snapper::getSnapMode() {
    return &(this->snapMode);
}

//get current mouse coordinates
RS_Vector RS_Snapper::snapFree(QMouseEvent* e) {
    if (!e) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapFree: event is nullptr");
        return RS_Vector(false);
    }
    pImpData->snapSpot=toGraph(e);
    pImpData->snapCoord=pImpData->snapSpot;
    snap_indicator->drawLines=true;
    return pImpData->snapCoord;
}

/**
 * Snap to a coordinate in the drawing using the current snap mode.
 *
 * @param e A mouse event.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapPoint(QMouseEvent* e){
    pImpData->snapSpot = RS_Vector(false);
    RS_Vector t(false);

    if (!e) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapPoint: event is nullptr");
        return pImpData->snapSpot;
    }

    RS_Vector mouseCoord = toGraph(e);
    double ds2Min=RS_MAXDOUBLE*RS_MAXDOUBLE;

    if (snapMode.snapEndpoint) {
        t = snapEndpoint(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);

        if (t.valid && ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::ENDPOINT;
        }
    }
    if (snapMode.snapCenter) {
        t = snapCenter(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::CENTER;
        }
    }
    if (snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions
        RS_DIALOGFACTORY->requestSnapMiddleOptions(&middlePoints, snapMode.snapMiddle);
        t = snapMiddle(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::MIDDLE;
        }
    }
    if (snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
        RS_DIALOGFACTORY->requestSnapDistOptions(&m_SnapDistance, snapMode.snapDistance);
        t = snapDist(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::DISTANCE;
        }
    }
    if (snapMode.snapIntersection) {
        t = snapIntersection(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::INTERSECTION;
        }
    }

    if (snapMode.snapOnEntity &&
        pImpData->snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
        t = snapOnEntity(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::ENTITY;
        }
    }

    if (snapMode.snapGrid) {
        t = snapGrid(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
//            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::GRID;
        }
    }

    if( !pImpData->snapSpot.valid ) {
        pImpData->snapSpot=mouseCoord; //default to snapFree
        pImpData->snapType = SnapType::FREE;
    } else {

        //retreat to snapFree when distance is more than quarter grid
        // issue #1631: snapFree issues: defines getSnapFree as the minimum graph distance to allow SnapFree
        if(snapMode.snapFree){
            // compare the current graph distance to the closest snap point to the minimum snapping free distance
            if((mouseCoord - pImpData->snapSpot).magnitude() >= getSnapRange()){
                pImpData->snapSpot = mouseCoord;
                pImpData->snapType = SnapType::FREE;
             }
        }
    }
    //if (snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
    // handle snap restrictions that can be activated in addition
    //   to the ones above:
    //apply restriction
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector vpv(rz.x, pImpData->snapSpot.y);
    RS_Vector vph(pImpData->snapSpot.x,rz.y);
    switch (snapMode.restriction) {
        case RS2::RestrictOrthogonal: {
            pImpData->snapCoord = (mouseCoord.distanceTo(vpv) < mouseCoord.distanceTo(vph)) ?
                                  vpv : vph;
            pImpData->restriction = RS2::RestrictOrthogonal;

            break;
        }
        case RS2::RestrictHorizontal: {
            pImpData->snapCoord = vph;
            pImpData->restriction = RS2::RestrictHorizontal;
            break;
        }
        case RS2::RestrictVertical: {
            pImpData->snapCoord = vpv;
            pImpData->restriction = RS2::RestrictVertical;
            break;
        }
            //case RS2::RestrictNothing:
        default: {
            pImpData->snapCoord = pImpData->snapSpot;
            pImpData->restriction = RS2::RestrictNothing;
            break;
        }
    }
    //}
    //else snapCoord = snapSpot;

    snapPoint(pImpData->snapSpot, false);

    return pImpData->snapCoord;
}


/**manually set snapPoint*/
RS_Vector RS_Snapper::snapPoint(const RS_Vector& coord, bool setSpot){
    if(coord.valid){
        pImpData->snapSpot=coord;
        if(setSpot) pImpData->snapCoord = coord;
        // fixme - sand - it seems that the code below is meaning for preview only?
        updateCoordinateWidgetByRelZero(pImpData->snapCoord);
        drawSnapper();
        drawInfoCursor();
    }
    return coord;
}

double RS_Snapper::getSnapRange() const{
    // issue #1631: redefine this method to the minimum graph distance to allow "Snap Free"
    // When the closest of any other snapping point is beyond this distance, free snapping is used.
    constexpr double Min_Snap_Factor = 0.25;
    std::vector<double> distances(3, RS_MAXDOUBLE);
    double& minGui=distances[0];
    double& minGrid=distances[1];
    double& minSize=distances[2];
    if (graphicView != nullptr) {
        minGui = graphicView->toGraphDX(32);
        // if grid is on, less than one quarter of the cell vector
        if (graphicView->isGridOn()) {
            RS_Grid *grid = graphicView->getGrid();
            const RS_Vector &cellVector = grid->getCellVector();
            minGrid = cellVector.magnitude() * Min_Snap_Factor;
        }
    }
    if (container != nullptr && isSizeValid(container->getSize())) {
        // The size bounding box
        minSize = getValidSize(container->getSize());
    }
    if (std::min(minGui, minGrid) < 0.99 * RS_MAXDOUBLE)
        return std::min(minGui, minGrid);
    if (minSize < 0.99 * RS_MAXDOUBLE)
        return minSize;
    // shouldn't happen: no graphicview or a valid size
    // Allow free snapping by returning the floating point tolerance
    return RS_TOLERANCE;
}

/**
 * Snaps to a free coordinate.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapFree(const RS_Vector& coord) {
    keyEntity = nullptr;
    return coord;
}

/**
 * Snaps to the closest endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapEndpoint(const RS_Vector& coord) {
    RS_Vector vec(false);

    vec = container->getNearestEndpoint(coord,
                                        nullptr/*, &keyEntity*/);
    return vec;
}

/**
 * Snaps to a grid point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapGrid(const RS_Vector& coord) {

//    RS_DEBUG->print("RS_Snapper::snapGrid begin");

//    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//    std::cout<<" mouse: = "<<coord<<std::endl;
//    std::cout<<" snapGrid: = "<<graphicView->getGrid()->snapGrid(coord)<<std::endl;
    return  graphicView->getGrid()->snapGrid(coord);
}

/**
 * Snaps to a point on an entity.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapOnEntity(const RS_Vector& coord) {

    RS_Vector vec{};
    vec = container->getNearestPointOnEntity(coord, true, nullptr, &keyEntity);
    return vec;
}

/**
 * Snaps to the closest center.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapCenter(const RS_Vector& coord) {
    RS_Vector vec{};

    vec = container->getNearestCenter(coord, nullptr);
    return vec;
}

/**
 * Snaps to the closest middle.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapMiddle(const RS_Vector& coord) {
//std::cout<<"RS_Snapper::snapMiddle(): middlePoints="<<middlePoints<<std::endl;
    return container->getNearestMiddle(coord,static_cast<double *>(nullptr),middlePoints);
}


/**
 * Snaps to the closest point with a given distance to the endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapDist(const RS_Vector& coord) {
    RS_Vector vec;

//std::cout<<" RS_Snapper::snapDist(RS_Vector coord): distance="<<distance<<std::endl;
    vec = container->getNearestDist(m_SnapDistance,
                                    coord,
                                    nullptr);
    return vec;
}



/**
 * Snaps to the closest intersection point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapIntersection(const RS_Vector& coord) {
    RS_Vector vec{};

    vec = container->getNearestIntersection(coord,
                                            nullptr);
    return vec;
}



/**
 * 'Corrects' the given coordinates to 0, 90, 180, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictOrthogonal(const RS_Vector& coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret(coord);

    RS_Vector retx = RS_Vector(rz.x, ret.y);
    RS_Vector rety = RS_Vector(ret.x, rz.y);

    if (retx.distanceTo(ret) > rety.distanceTo(ret)) {
        ret = rety;
    } else {
        ret = retx;
    }

    return ret;
}

/**
 * 'Corrects' the given coordinates to 0, 180 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictHorizontal(const RS_Vector& coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(coord.x, rz.y);
    return ret;
}


/**
 * 'Corrects' the given coordinates to 90, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictVertical(const RS_Vector& coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(rz.x, coord.y);
    return ret;
}


/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos,
                                   RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_Snapper::catchEntity");

    // set default distance for points inside solids
    double dist (0.);
//    std::cout<<"getSnapRange()="<<getSnapRange()<<"\tsnap distance = "<<dist<<std::endl;

    RS_Entity* entity = container->getNearestEntity(pos, &dist, level);

    int idx = -1;
    if (entity != nullptr && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), catchEntityGuiRange, graphicView)) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    } else {
        RS_DEBUG->print("RS_Snapper::catchEntity: not found");
        return nullptr;
    }
    RS_DEBUG->print("RS_Snapper::catchEntity: OK");
}


/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos, RS2::EntityType enType,
                                   RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_Snapper::catchEntity");
//                    std::cout<<"RS_Snapper::catchEntity(): enType= "<<enType<<std::endl;

    // set default distance for points inside solids
    RS_EntityContainer ec(nullptr,false);
//isContainer
    bool isContainer{false};
    switch(enType){
        case RS2::EntityPolyline:
        case RS2::EntityContainer:
        case RS2::EntitySpline:
            isContainer=true;
            break;
        default:
            break;
    }

    // fixme - iteration over all elements of drawing
    for(RS_Entity* en= container->firstEntity(level);en;en=container->nextEntity(level)){
        if(en->isVisible()==false) continue;
        if(en->rtti() != enType && isContainer){
            //whether this entity is a member of member of the type enType
            RS_Entity* parent(en->getParent());
            bool matchFound{false};
            while(parent ) {
//                    std::cout<<"RS_Snapper::catchEntity(): parent->rtti()="<<parent->rtti()<<" enType= "<<enType<<std::endl;
                if(parent->rtti() == enType) {
                    matchFound=true;
                    ec.addEntity(en);
                    break;
                }
                parent=parent->getParent();
            }
            if(!matchFound) continue;
        }
        if (en->rtti() == enType){
            ec.addEntity(en);
        }
    }
    if (ec.count() == 0 ) return nullptr;
    double dist(0.);

    RS_Entity* entity = ec.getNearestEntity(pos, &dist, RS2::ResolveNone);

    int idx = -1;
    if (entity != nullptr && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), catchEntityGuiRange, graphicView)) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    } else {
        RS_DEBUG->print("RS_Snapper::catchEntity: not found");
        return nullptr;
    }
}


/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e,
                                   RS2::ResolveLevel level) {

    RS_Entity* entity = catchEntity(toGraph(e), level);
    return entity;
}


/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e, RS2::EntityType enType,
                                   RS2::ResolveLevel level) {
    return catchEntity(toGraph(e),enType,level);
}

RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList,
                                   RS2::ResolveLevel level) {
    RS_Vector coord = toGraph(e);
    return catchEntity(coord, enTypeList, level);
}

RS_Entity* RS_Snapper::catchEntity(const RS_Vector& coord, const EntityTypeList& enTypeList,
                                   RS2::ResolveLevel level) {
    RS_Entity *pten = nullptr;
    switch (enTypeList.size()) {
        case 0:
            return catchEntity(coord, level);
        default: {

            RS_EntityContainer ec(nullptr, false);
            for (auto t0: enTypeList) {
                RS_Entity *en = catchEntity(coord, t0, level);
                if (en) ec.addEntity(en);
//			if(en) {
//            std::cout<<__FILE__<<" : "<<__func__<<" : lines "<<__LINE__<<std::endl;
//            std::cout<<"caught id= "<<en->getId()<<std::endl;
//            }
            }
            if (ec.count() > 0){
                ec.getDistanceToPoint(coord, &pten, RS2::ResolveNone);
                return pten;
            }
        }
    }
    return nullptr;
}

void RS_Snapper::suspend() {
// RVT Don't delete the snapper here!
// RVT_PORT (can be deleted)();
    pImpData->snapSpot = pImpData->snapCoord = RS_Vector{false};
}

void RS_Snapper::resume() {
    drawSnapper();
    infoCursorOverlayPrefs = graphicView->getInfoCursorOverlayPreferences();
}

/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideSnapOptions() {
    RS_DIALOGFACTORY->hideSnapOptions();
}

/**
 * Shows the snapper options. Default implementation does nothing.
 */
//void RS_Snapper::showOptions() {
    //not used any more, will be removed
//}

/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper(){
//    LC_ERR<<"Delete Snapper";
    if (!graphicView->isCleanUp()) {
        graphicView->getOverlayContainer(RS2::Snapper)->clear();
        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

void RS_Snapper::deleteInfoCursor(){
//    LC_ERR<<"Delete Info Cursor";
    if (!graphicView->isCleanUp()) {
        graphicView->getOverlayContainer(RS2::InfoCursor)->clear();
        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

/**
 * creates the snap indicator
 */
void RS_Snapper::drawSnapper(){
    graphicView->getOverlayContainer(RS2::Snapper)->clear();
    if (!finished && pImpData->snapSpot.valid){
        if (snap_indicator->drawLines || snap_indicator->drawShape) {
            auto overlayContainer = graphicView->getOverlayContainer(RS2::Snapper);

            auto *crosshair = new LC_Crosshair(overlayContainer, pImpData->snapCoord, snap_indicator->shape_type,
                                               snap_indicator->lines_type, snap_indicator->lines_pen, snap_indicator->pointSize, snap_indicator->pointType);
            crosshair->setPen(snap_indicator->shape_pen);
            overlayContainer->addEntity(crosshair);
//            LC_ERR << "Redraw SNAPPER";
        }
    }

    graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}

void RS_Snapper::clearInfoCursor(){
    auto overlayContainer = graphicView->getOverlayContainer(RS2::InfoCursor);
    LC_InfoCursor * infoCursor = nullptr;
    RS_Entity *entity = overlayContainer->firstEntity(); // note - this is not absolutely safe if someone put another cursor to overlay container!
    infoCursor = dynamic_cast<LC_InfoCursor *>(entity);
    if (infoCursor != nullptr){
        infoCursor->clear();
    }
}

LC_InfoCursor* RS_Snapper::obtainInfoCursor(){
    auto overlayContainer = graphicView->getOverlayContainer(RS2::InfoCursor);
    LC_InfoCursor * result = nullptr;
    RS_Entity *entity = overlayContainer->firstEntity(); // note - this is not absolutely safe if someone put another cursor to overlay container!
    result = dynamic_cast<LC_InfoCursor *>(entity);
    if (result == nullptr && infoCursorOverlayPrefs != nullptr){
        result = new LC_InfoCursor(container, pImpData->snapCoord, &infoCursorOverlayPrefs->options);
        overlayContainer->addEntity(result);
    }
    return result;
}

void RS_Snapper::drawInfoCursor(){
    auto overlayContainer = graphicView->getOverlayContainer(RS2::InfoCursor);
    if (infoCursorOverlayPrefs != nullptr && infoCursorOverlayPrefs->enabled) {
        RS_Entity *entity = overlayContainer->firstEntity(); // note - this is not absolutely safe if someone put another cursor to overlay container!
        auto* infoCursor = dynamic_cast<LC_InfoCursor *>(entity);
        if (infoCursor == nullptr) {
            infoCursor = new LC_InfoCursor(container, pImpData->snapCoord, &infoCursorOverlayPrefs->options);
            overlayContainer->addEntity(infoCursor);
        }
        else{
            infoCursor->setOptions(&infoCursorOverlayPrefs->options);
            infoCursor->setPos(pImpData->snapCoord);
        }
        LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();
        if (prefs->showSnapType) {
            QString snapName = getSnapName(pImpData->snapType);
            QString restrictionName;
            if (pImpData->snapType == ANGLE || pImpData->snapType == ANGLE_ON_ENTITY) {
                restrictionName = RS_Units::formatAngle(pImpData->angle, angleFormat, anglePrecision);
            } else {
                restrictionName = getRestrictionName(pImpData->restriction);
            }
            if (!restrictionName.isEmpty()) {
                snapName = snapName + (prefs->multiLine ? "\n" : " ") + restrictionName;
            }
            infoCursorOverlayData.setZone2(snapName);
        } else {
            infoCursorOverlayData.setZone2("");
        }
        infoCursor->setZonesData(&infoCursorOverlayData);
    }
    graphicView->redraw(RS2::RedrawOverlay);
}

QString RS_Snapper::getRestrictionName(int restriction) {
    switch (restriction) {
        case RS2::RestrictVertical:
            return tr("Vertical");
        case RS2::RestrictHorizontal:
            return tr("Horizontal");
        case RS2::RestrictOrthogonal:
            return tr("Orthogonal");
        default:
            return "";
    }
}

QString RS_Snapper::getSnapName(int snapType){
    switch (snapType){
        case GRID:
            return tr("Grid");
        case ENTITY:
            return tr("Entity");
        case ENDPOINT:
            return tr("Endpoint");
        case INTERSECTION:
            return tr("Intersection");
        case MIDDLE:
            return tr("Middle");
        case DISTANCE:
            return tr("Distance");
        case CENTER:
            return tr("Center");
        case ANGLE:
            return tr("Angle");
        case ANGLE_ON_ENTITY:
            return tr("Angle (on Entity)");
        case FREE:
        default:
            return "";
    }
}

RS_Vector RS_Snapper::snapToRelativeAngle(double baseAngle, const RS_Vector &currentCoord, const RS_Vector &referenceCoord, const double angularResolution){

    if(snapMode.restriction != RS2::RestrictNothing || snapMode.snapGrid){
        return currentCoord;
    }

    double angle = referenceCoord.angleTo(currentCoord)*180.0/M_PI;
    angle -= std::remainder(angle,angularResolution);
    angle *= M_PI/180.;
    angle = angle + baseAngle; // add base angle, so snap is relative
    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord),angle);
    res += referenceCoord;

    if (snapMode.snapOnEntity)
    {
        RS_Vector t(false);
        //RS_Vector mouseCoord = graphicView->toGraph(currentCoord.x(), currentCoord.y());
        t = container->getNearestVirtualIntersection(res,angle,nullptr);

        pImpData->snapSpot = t;
        snapPoint(pImpData->snapSpot, true);
        return t;
    }
    else
    {
        snapPoint(res, true);
        return res;
    }
}

RS_Vector RS_Snapper::snapToAngle(
    const RS_Vector &currentCoord, const RS_Vector &referenceCoord, const double angularResolution) {

    if (snapMode.restriction != RS2::RestrictNothing || snapMode.snapGrid) {
        return currentCoord;
    }

    double angle = referenceCoord.angleTo(currentCoord) * 180.0 / M_PI;
    angle -= std::remainder(angle, angularResolution);
    angle *= M_PI / 180.;
    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), angle);
    res += referenceCoord;

    if (snapMode.snapOnEntity) {
        RS_Vector t(false);
        //RS_Vector mouseCoord = graphicView->toGraph(currentCoord.x(), currentCoord.y());
        t = container->getNearestVirtualIntersection(res, angle, nullptr);

        pImpData->snapSpot = t;
        pImpData->snapType = (t == res) ? SnapType::ANGLE : SnapType::ANGLE_ON_ENTITY;
        pImpData->angle = angle;
        snapPoint(pImpData->snapSpot, true);
        return t;
    } else {
        pImpData->snapType = SnapType::ANGLE;
        pImpData->angle = angle;
        snapPoint(res, true);
        return res;
    }
}

 RS_Vector RS_Snapper::toGraph(const QMouseEvent* e) const{
    RS_Vector result = graphicView->toGraph(e->position());
    return result;
}

void RS_Snapper::updateCoordinateWidgetFormat(){
    updateCoordinateWidget(RS_Vector(0.0,0.0),RS_Vector(0.0,0.0), true);
}

void RS_Snapper::updateCoordinateWidget(const RS_Vector& abs, const RS_Vector& rel, bool updateFormat){
    if (infoCursorOverlayPrefs != nullptr && infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(updateFormat, abs, rel);
    }
    RS_DIALOGFACTORY->updateCoordinateWidget(abs, rel, updateFormat);
}

void RS_Snapper::updateCoordinateWidgetByRelZero(const RS_Vector& abs, bool updateFormat){
    const RS_Vector &relative = abs - graphicView->getRelativeZero();
    if (infoCursorOverlayPrefs != nullptr && infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(updateFormat, abs, relative);
    }
    RS_DIALOGFACTORY->updateCoordinateWidget(abs, relative, updateFormat);
}

LC_InfoCursorOverlayPrefs* RS_Snapper::getInfoCursorOverlayPrefs() const {
    return infoCursorOverlayPrefs;
}

bool RS_Snapper::isInfoCursorForModificationEnabled() const {
    return infoCursorOverlayPrefs != nullptr && infoCursorOverlayPrefs->enabled && infoCursorOverlayPrefs->showEntityInfoOnModification;
}

void RS_Snapper::preparePositionsInfoCursorOverlay(bool updateFormat, const RS_Vector &abs,  const RS_Vector &relative) {
    LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();

    QString coordAbs = "";
    QString coordPolar = "";
    if (prefs != nullptr && (prefs->showAbsolutePosition || prefs->showRelativePositionDistAngle || prefs->showRelativePositionDeltas)){
        RS_Graphic* graphic = graphicView->getGraphic();
        if (graphic != nullptr) {
            if (updateFormat) {
                linearFormat = graphic->getLinearFormat();
                linearPrecision = graphic->getLinearPrecision();
                angleFormat = graphic->getAngleFormat();
                anglePrecision = graphic->getAnglePrecision();
                unit = graphic->getUnit();
            }

            bool showLabels = prefs->showLabels;
            if (prefs->showAbsolutePosition) {
                QString absX = (showLabels ? "X: " : "") + formatLinear(abs.x);
                QString absY = (showLabels ? "Y: " : "") + formatLinear(abs.y);
                coordAbs = absX + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + absY;
            }
            if (prefs->showRelativePositionDistAngle) {
                QString lenStr = (showLabels ? tr("Dist: ") : "@ ") + formatLinear(relative.magnitude());
                QString angleStr = (showLabels ? tr("Angle: ") : "< ") + formatAngle(relative.angle());

                coordPolar = lenStr + (prefs->multiLine ? "\n" : showLabels ? " " : " ") + angleStr;
            }
            if (prefs->showRelativePositionDeltas) {
                QString lenStr = (showLabels ? tr("dX: ") : "@ ") + formatLinear(relative.x);
                QString angleStr = (showLabels ? tr("dY: ") : "") + formatAngle(relative.y);

                QString coordDeltas = lenStr + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + angleStr;
                if (coordPolar.isEmpty()){
                    coordPolar = coordDeltas;
                }
                else{
                    coordPolar = coordPolar + "\n" +  coordDeltas;
                }
            }
        }
    }

    infoCursorOverlayData.setZone1(coordAbs);
    infoCursorOverlayData.setZone3(coordPolar);
}

void RS_Snapper::invalidateSnapSpot() {
    pImpData->snapSpot.valid = false;
}

QString RS_Snapper::formatLinear(double value){
    return RS_Units::formatLinear(value, unit, linearFormat, linearPrecision);
}

QString RS_Snapper::formatAngle(double value){
    return RS_Units::formatAngle(value, angleFormat, anglePrecision);
}

QString RS_Snapper::formatVector(const RS_Vector &value) {
    return formatLinear(value.x).append(" , ").append(formatLinear(value.y));
}

QString RS_Snapper::formatRelative(const RS_Vector &value) {
    return QString("@ ").append(formatLinear(value.x)).append(" , ").append(formatLinear(value.y));
}

QString RS_Snapper::formatPolar(const RS_Vector &value) {
    return formatLinear(value.magnitude()).append(" < ").append(formatAngle(value.angle()));
}

QString RS_Snapper::formatRelativePolar(const RS_Vector &value) {
    return QString("@ ").append(formatLinear(value.magnitude())).append(" < ").append(formatAngle(value.angle()));
}

void RS_Snapper::forceUpdateInfoCursor(const RS_Vector &pos) {
    LC_InfoCursor* infoCursor = obtainInfoCursor();
    infoCursor->setPos(pos);
    infoCursor->setZonesData(&infoCursorOverlayData);
}
