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


#include <QMouseEvent>

#include "lc_actioncontext.h"
#include "lc_containertraverser.h"
#include "lc_crosshair.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_defaults.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_overlayentitiescontainer.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_math.h"
#include "rs_pen.h"
#include "rs_snapper.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "rs_vector.h"

namespace {

    // whether a floating point is positive by tolerance
    bool isPositive(double x){
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
}

/**
  * Disable all snapping.
  *
  * This effectively puts the object into free snap mode.
  *
  * @returns A reference to itself.
  */
RS_SnapMode const & RS_SnapMode::clear(){
    *this = RS_SnapMode{};

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
    int lines_type = 0;
    RS_Pen lines_pen;

    bool drawShape = false;
    int shape_type = 0;
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
    ANGLE_REL,
    ANGLE_ON_ENTITY
};

struct RS_Snapper::ImpData {
    RS_Vector snapCoord;
    RS_Vector snapSpot;
    int snapType = 0;
    double angle = 0.;
    int restriction = RS2::RestrictNothing;
};

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(LC_ActionContext *actionContext)
    :m_container(actionContext->getEntityContainer())
    ,m_graphicView(actionContext->getGraphicView())
    ,m_actionContext(actionContext)
    ,m_infoCursorOverlayData{std::make_unique<LC_InfoCursorData>()}
    ,pImpData(new ImpData),
    m_snapIndicator(new Indicator)
{
    m_viewport = m_graphicView->getViewPort();
    m_infoCursorOverlayPrefs = m_graphicView->getInfoCursorOverlayPreferences();
}

RS_Snapper::~RS_Snapper() = default;


/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init(){
    m_snapMode = m_graphicView->getDefaultSnapMode();
    m_keyEntity = nullptr;
    pImpData->snapSpot = RS_Vector{false};
    pImpData->snapCoord = RS_Vector{false};
    m_SnapDistance = 1.0;
    initSettings();
}

void RS_Snapper::initSettings() {
    initFromSettings();

    RS_Graphic* graphic = m_graphicView->getGraphic();
    if (graphic != nullptr) {
        initFromGraphic(graphic);
    }
}

void RS_Snapper::initFromSettings() {
    LC_GROUP("Appearance");
    {
        int snapIndicatorLineWidth = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_width", 1));
        m_snapIndicator->drawLines = LC_GET_BOOL("indicator_lines_state", true);
        if (m_snapIndicator->drawLines){
            m_snapIndicator->lines_type = LC_GET_INT("indicator_lines_type", 0);
            RS2::LineType snapIndicatorLineType = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_type", RS2::DashLine));
            QString snap_color_lines = LC_GET_ONE_STR("Colors", "snap_indicator_lines", RS_Settings::snap_indicator_lines);
            m_snapIndicator->lines_pen = RS_Pen(RS_Color(snap_color_lines), RS2::Width00, snapIndicatorLineType);
            m_snapIndicator->lines_pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else {
            m_snapIndicator->lines_type = LC_Crosshair::NoLines;
        }

        m_snapIndicator->drawShape = LC_GET_BOOL("indicator_shape_state", true);
        if (m_snapIndicator->drawShape) {
            m_snapIndicator->shape_type = LC_GET_INT("indicator_shape_type", 0);
            QString snap_color = LC_GET_ONE_STR("Colors", "snap_indicator", RS_Settings::snap_indicator);
            m_snapIndicator->shape_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::SolidLine);
            m_snapIndicator->shape_pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else{
            m_snapIndicator->shape_type = LC_Crosshair::NoShape;
        }

        m_ignoreSnapToGridIfNoGrid = LC_GET_BOOL("SnapGridIgnoreIfNoGrid", false);

    }
    LC_GROUP_END();

    LC_GROUP("Snap");
    {
        m_distanceBeforeSwitchToFreeSnap = LC_GET_INT("AdvSnapOnEntitySwitchToFreeDistance", 500) / 100.0;
        m_catchEntityGuiRange =  LC_GET_INT("AdvSnapEntityCatchRange", 32);
        m_minGridCellSnapFactor = LC_GET_INT("AdvSnapGridCellSnapFactor", 25) / 100.0;
    }
    LC_GROUP_END();

    m_catchEntityGuiRange = LC_GET_ONE_INT("Snapping", "CatchEntityGuiDistance", 32); // fixme - sand - add to option ui?
}

void RS_Snapper::initFromGraphic(RS_Graphic *graphic) {
    if (graphic != nullptr) {
        updateUnitFormat(graphic);

        m_snapIndicator->pointType = graphic->getVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
        m_snapIndicator->pointSize = graphic->getVariableInt("$PDSIZE", LC_DEFAULTS_PDSize);

        m_anglesBase = graphic->getAnglesBase();
        m_anglesCounterClockWise = graphic->areAnglesCounterClockWise();
    }
}

void RS_Snapper::finish() {
    m_finished = true;
    deleteSnapper();
    deleteInfoCursor();
}

void RS_Snapper::setSnapMode(const RS_SnapMode& snapMode) {
    this->m_snapMode = snapMode;
    m_actionContext->requestSnapDistOptions(&m_SnapDistance, snapMode.snapDistance);
    m_actionContext->requestSnapMiddleOptions(&m_middlePoints, snapMode.snapMiddle);
}

RS_SnapMode const* RS_Snapper::getSnapMode() const{
    return &(this->m_snapMode);
}

RS_SnapMode* RS_Snapper::getSnapMode() {
    return &(this->m_snapMode);
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
    m_snapIndicator->drawLines=true;
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

    if (m_snapMode.snapEndpoint) {
        t = snapEndpoint(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);

        if (t.valid && ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::ENDPOINT;
        }
    }
    if (m_snapMode.snapCenter) {
        t = snapCenter(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::CENTER;
        }
    }
    if (m_snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions

        m_actionContext->requestSnapMiddleOptions(&m_middlePoints, m_snapMode.snapMiddle);
        t = snapMiddle(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::MIDDLE;
        }
    }
    if (m_snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
        m_actionContext->requestSnapDistOptions(&m_SnapDistance, m_snapMode.snapDistance);
        t = snapDist(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::DISTANCE;
        }
    }
    if (m_snapMode.snapIntersection) {
        t = snapIntersection(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::INTERSECTION;
        }
    }

    if (m_snapMode.snapOnEntity && pImpData->snapSpot.distanceTo(mouseCoord) > m_distanceBeforeSwitchToFreeSnap) {
        t = snapOnEntity(mouseCoord);
        double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
            pImpData->snapSpot = t;
            pImpData->snapType = SnapType::ENTITY;
        }
    }

    if (isSnapToGrid()) {
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
        if(m_snapMode.snapFree){
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
    RS_Vector vpv, vph;
    if (m_snapMode.restriction != RS2::RestrictNothing) {
        RS_Vector rz = m_viewport->getRelativeZero();
        if (m_viewport->hasUCS()) {
            RS_Vector ucsRZ = m_viewport->toUCS(rz);
            RS_Vector ucsSnap = m_viewport->toUCS(pImpData->snapSpot);
            vpv = m_viewport->toWorld(RS_Vector(ucsRZ.x, ucsSnap.y));
            vph = m_viewport->toWorld(RS_Vector(ucsSnap.x, ucsRZ.y));
        } else {
            vpv = RS_Vector(rz.x, pImpData->snapSpot.y);
            vph = RS_Vector(pImpData->snapSpot.x, rz.y);
        }
    }

    switch (m_snapMode.restriction) {
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
    std::vector<double> distances(3, RS_MAXDOUBLE);
    double& minGui=distances[0];
    double& minGrid=distances[1];
    double& minSize=distances[2];
    if (m_graphicView != nullptr) {
        minGui = toGraphDX(32);
        // if grid is on, less than one quarter of the cell vector
//        if (viewport->isGridOn()) {
// todo - sand - check whether it's correct apply this check only if "Snap to Grid" is enabled
        if (m_viewport->isGridOn() && m_snapMode.snapGrid) {
            RS_Grid *grid = m_viewport->getGrid();
            const RS_Vector &cellVector = grid->getCellVector();
            minGrid = cellVector.magnitude() * m_minGridCellSnapFactor;
        }
    }
    if (m_container != nullptr && isSizeValid(m_container->getSize())) {
        // The size bounding box
        minSize = getValidSize(m_container->getSize());
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
    m_keyEntity = nullptr;
    return coord;
}

/**
 * Snaps to the closest endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapEndpoint(const RS_Vector& coord) {
    RS_Vector vec = m_container->getNearestEndpoint(coord, nullptr/*, &keyEntity*/);
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
    return  m_viewport->snapGrid(coord);
}

/**
 * Snaps to a point on an entity.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapOnEntity(const RS_Vector& coord) {
    RS_Vector vec{};
    vec = m_container->getNearestPointOnEntity(coord, true, nullptr, &m_keyEntity);
    return vec;
}

/**
 * Snaps to the closest center.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapCenter(const RS_Vector& coord) {
    RS_Vector vec = m_container->getNearestCenter(coord, nullptr);
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
    return m_container->getNearestMiddle(coord,static_cast<double *>(nullptr),m_middlePoints);
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
    vec = m_container->getNearestDist(m_SnapDistance,
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
    vec = m_container->getNearestIntersection(coord,nullptr);
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
    RS_Vector rz = m_viewport->getRelativeZero();
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
//    RS_Vector rz = graphicView->getRelativeZero();
////    RS_Vector ret = RS_Vector(coord.x, rz.y);
//    RS_Vector ucsRZ = graphicView->toUCS(rz);
//    RS_Vector ret = RS_Vector(coord.x, ucsRZ.y);
//    return ret;

    return m_viewport->restrictHorizontal(m_viewport->getRelativeZero(), coord);
}

/**
 * 'Corrects' the given coordinates to 90, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictVertical(const RS_Vector& coord) {
//    RS_Vector rz = graphicView->getRelativeZero();
//    RS_Vector ret = RS_Vector(rz.x, coord.y);
//    return ret;
    return m_viewport->restrictVertical(m_viewport->getRelativeZero(), coord);
}

RS_Vector RS_Snapper::restrictVertical(const RS_Vector &base, const RS_Vector &coord) const{
    return m_viewport->restrictVertical(base, coord);
}

RS_Vector RS_Snapper::restrictHorizontal(const RS_Vector &base, const RS_Vector &coord) const{
    return m_viewport->restrictHorizontal(base, coord);
}

RS_Vector RS_Snapper::restrictAngle(const RS_Vector &basePoint, const RS_Vector& snap, double angle){
    RS_Vector possibleEndPoint;
    double realAngle = toWorldAngle(angle);
    RS_Vector infiniteTickEndPoint = basePoint.relative(10.0, realAngle);
    RS_Vector pointOnInfiniteTick =  LC_LineMath::getNearestPointOnInfiniteLine(snap, basePoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
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

    RS_Entity* entity = m_container->getNearestEntity(pos, &dist, level);

    int idx = -1;
    if (entity != nullptr && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), m_catchEntityGuiRange)) {
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
    for(RS_Entity* en: lc::LC_ContainerTraverser{*m_container, level}.entities()){
        if(!en->isVisible())
            continue;
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

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), m_catchEntityGuiRange)) {
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
RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e,RS2::ResolveLevel level) {
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
RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e, RS2::EntityType enType,RS2::ResolveLevel level) {
    return catchEntity(toGraph(e),enType,level);
}

RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList,RS2::ResolveLevel level) {
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
    initSettings();
    m_infoCursorOverlayPrefs = m_graphicView->getInfoCursorOverlayPreferences(); // fixme - review/rework this, load from settings as other overlays action(??)
}

/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideSnapOptions() {
    m_actionContext->hideSnapOptions();
}


/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper(){
//    LC_ERR<<"Delete Snapper";
    if (m_graphicView != nullptr && !m_graphicView->isCleanUp()) {
        m_viewport->clearOverlayDrawablesContainer(RS2::Snapper);
        m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

void RS_Snapper::deleteInfoCursor(){
//    LC_ERR<<"Delete Info Cursor";
    if (m_graphicView != nullptr && !m_graphicView->isCleanUp()) {
        m_viewport->clearOverlayDrawablesContainer(RS2::InfoCursor);
        m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

/**
 * creates the snap indicator
 */
void RS_Snapper::drawSnapper(){
    LC_OverlayDrawablesContainer *snapperOverlay = m_viewport->getOverlaysDrawablesContainer(RS2::Snapper);
    snapperOverlay->clear();
    if (!m_finished && pImpData->snapSpot.valid){
        if (m_snapIndicator->drawLines || m_snapIndicator->drawShape) {
            auto *crosshair = new LC_Crosshair(pImpData->snapCoord, m_snapIndicator->shape_type,
                                               m_snapIndicator->lines_type, m_snapIndicator->lines_pen, m_snapIndicator->pointSize, m_snapIndicator->pointType);
            crosshair->setShapesPen(m_snapIndicator->shape_pen);
            snapperOverlay->add(crosshair);
        }
    }
    m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}


LC_OverlayInfoCursor* RS_Snapper::obtainInfoCursor(){
    auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::InfoCursor);
    LC_OverlayInfoCursor * result = nullptr;
    // fixme - this is not absolutely safe if someone put another cursor to overlay container! Rework later!!
    auto entity = overlayContainer->first(); // note - this is not absolutely safe if someone put another cursor to overlay container!
    result = dynamic_cast<LC_OverlayInfoCursor *>(entity);
    if (result == nullptr && m_infoCursorOverlayPrefs != nullptr){
        result = new LC_OverlayInfoCursor(pImpData->snapCoord, &m_infoCursorOverlayPrefs->options);
        overlayContainer->add(result);
    }
    return result;
}

void RS_Snapper::drawInfoCursor(){
    auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::InfoCursor);
    if (m_infoCursorOverlayPrefs != nullptr && m_infoCursorOverlayPrefs->enabled) {
        // fixme - this is not absolutely safe if someone put another cursor to overlay container! Rework later!!
        auto entity = overlayContainer->first();
        auto* infoCursor = dynamic_cast<LC_OverlayInfoCursor *>(entity);
        if (infoCursor == nullptr) {
            infoCursor = new LC_OverlayInfoCursor(pImpData->snapCoord, &m_infoCursorOverlayPrefs->options);
            overlayContainer->add(infoCursor);
        }
        else{
            infoCursor->setOptions(&m_infoCursorOverlayPrefs->options);
            infoCursor->setPos(pImpData->snapCoord);
        }
        auto prefs = getInfoCursorOverlayPrefs();
        if (prefs->showSnapType) {
            QString snapName = getSnapName(pImpData->snapType);
            QString restrictionName;
            if (pImpData->snapType == ANGLE || pImpData->snapType == ANGLE_REL || pImpData->snapType == ANGLE_ON_ENTITY) {
                double ucsAbsSnapAngle = pImpData->angle;
                double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAbsSnapAngle, m_anglesBase, m_anglesCounterClockWise);
                restrictionName = RS_Units::formatAngle(ucsBasisAngle, m_angleFormat, m_anglePrecision);
            } else {
                restrictionName = getRestrictionName(pImpData->restriction);
            }
            if (!restrictionName.isEmpty()) {
                snapName = snapName + (prefs->multiLine ? "\n" : " ") + restrictionName;
            }
            m_infoCursorOverlayData->setZone2(snapName);
        } else {
            m_infoCursorOverlayData->setZone2("");
        }
        infoCursor->setZonesData(m_infoCursorOverlayData.get());
    }
    m_graphicView->redraw(RS2::RedrawOverlay);
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
        case ANGLE_REL:
            return tr("Angle Relative");
        case ANGLE_ON_ENTITY:
            return tr("Angle (on Entity)");
        case FREE:
        default:
            return "";
    }
}

bool RS_Snapper::isSnapToGrid(){
    bool result = m_snapMode.snapGrid;
    if (result) {
        if (m_ignoreSnapToGridIfNoGrid) {
            result = m_viewport->isGridOn();
        }
    }
    return result;
}

RS_Vector RS_Snapper::snapToRelativeAngle(double baseAngle, const RS_Vector &currentCoord, const RS_Vector &referenceCoord, const double angularResolution){

    if(m_snapMode.restriction != RS2::RestrictNothing || isSnapToGrid()){
        return currentCoord;
    }

    double wcsAngleRaw = referenceCoord.angleTo(currentCoord);
    double ucsAngleRaw = toUCSAngle(wcsAngleRaw);
    double ucsAngleSnapped = ucsAngleRaw - std::remainder(ucsAngleRaw, angularResolution);
    double wcsAngleSnappedAbsolute = toWorldAngle(ucsAngleSnapped);

    double wcsAngleSnapped = wcsAngleSnappedAbsolute  + baseAngle;  // add base angle, so snap is relative

    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), wcsAngleSnapped);
    res += referenceCoord;

    if (m_snapMode.snapOnEntity) {
        RS_Vector t = m_container->getNearestVirtualIntersection(res, wcsAngleSnapped, nullptr);
        pImpData->snapSpot = t;
        pImpData->snapType = (t == res) ? SnapType::ANGLE_REL : SnapType::ANGLE_ON_ENTITY;
        pImpData->angle = ucsAngleSnapped;
        snapPoint(pImpData->snapSpot, true);
        return t;
    } else {
        pImpData->snapType = SnapType::ANGLE_REL;
        pImpData->angle = ucsAngleSnapped;
        snapPoint(res, true);
        return res;
    }
}

RS_Vector RS_Snapper::snapToAngle(const RS_Vector &currentCoord, const RS_Vector &referenceCoord, const double angularResolution) {
    if (m_snapMode.restriction != RS2::RestrictNothing || isSnapToGrid()) {
        return currentCoord;
    }
    return doSnapToAngle(currentCoord, referenceCoord, angularResolution);
}

RS_Vector RS_Snapper::doSnapToAngle(const RS_Vector &currentCoord, const RS_Vector &referenceCoord, const double angularResolution) {
    double wcsAngleRaw = referenceCoord.angleTo(currentCoord);
    double ucsAngleAbs = this->toUCSAngle(wcsAngleRaw);

    double ucsAngle = ucsAngleAbs - this->m_anglesBase;
    double ucsAngleSnapped = ucsAngleAbs - remainder(ucsAngle, angularResolution);

//    LC_ERR << "BASE " << RS_Math::rad2deg(m_anglesBase) << " UCSabs " << RS_Math::rad2deg(ucsAngleAbs) << " UCS " << RS_Math::rad2deg(ucsAngle) << " Snapped " << RS_Math::rad2deg(ucsAngleSnapped) << " UCSRel " << RS_Math::rad2deg(ucsAngleSnapped);
    double wcsAngleSnapped = this->toWorldAngle(ucsAngleSnapped);

    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), wcsAngleSnapped);
    res += referenceCoord;

    if (this->m_snapMode.snapOnEntity) {
        RS_Vector t = this->m_container->getNearestVirtualIntersection(res, wcsAngleSnapped, nullptr);
        this->pImpData->snapSpot = t;
        this->pImpData->snapType = (t == res) ? ANGLE : ANGLE_ON_ENTITY;
        this->pImpData->angle = ucsAngleSnapped;
        this->snapPoint(this->pImpData->snapSpot, true);
        return t;
    } else {
        this->pImpData->snapType = ANGLE;
        this->pImpData->angle = ucsAngleSnapped;
        this->snapPoint(res, true);
        return res;
    }
}

RS_Vector RS_Snapper::toGraph(const QMouseEvent* e) const{
     const QPointF &pointF = e->position();
     RS_Vector result = m_viewport->toWorldFromUi(pointF.x(), pointF.y());
    return result;
}

double RS_Snapper::toGuiDX(double wcsDX) const {
    return m_viewport->toGuiDX(wcsDX);
}

double RS_Snapper::toGraphDX(int wcsDX) const {
    return m_viewport->toUcsDX(wcsDX);
}

RS_Vector const &RS_Snapper::getRelativeZero() const {
    return m_viewport->getRelativeZero();
}

void RS_Snapper::updateCoordinateWidgetFormat(){
    m_actionContext->updateCoordinateWidget(toWorld(RS_Vector(0.0,0.0)),toWorld(RS_Vector(0.0,0.0)), true);
}

void RS_Snapper::updateCoordinateWidget(const RS_Vector& abs, const RS_Vector& rel, bool updateFormat){
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(updateFormat, abs, rel);
    }
    m_actionContext->updateCoordinateWidget(abs, rel, updateFormat);
}

void RS_Snapper::updateCoordinateWidgetByRelZero(const RS_Vector& abs, bool updateFormat){
    const RS_Vector &relative = abs - m_viewport->getRelativeZero();
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(updateFormat, abs, relative);
    }
    m_actionContext->updateCoordinateWidget(abs, relative, updateFormat);
}

LC_InfoCursorOverlayPrefs* RS_Snapper::getInfoCursorOverlayPrefs() const {
    return m_infoCursorOverlayPrefs;
}

bool RS_Snapper::isInfoCursorForModificationEnabled() const {
    return m_infoCursorOverlayPrefs->enabled && m_infoCursorOverlayPrefs->showEntityInfoOnModification;
}

void RS_Snapper::preparePositionsInfoCursorOverlay(bool updateFormat, const RS_Vector &abs,  const RS_Vector &relative) {
    LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();

    QString coordAbs = "";
    QString coordPolar = "";
    if (prefs != nullptr && (prefs->showAbsolutePosition || prefs->showRelativePositionDistAngle || prefs->showRelativePositionDeltas)){
        RS_Graphic* graphic = m_graphicView->getGraphic();
        if (graphic != nullptr) {
            if (updateFormat) {
                updateUnitFormat(graphic);
            }

            bool showLabels = prefs->showLabels;
            if (prefs->showAbsolutePosition) {
                RS_Vector ucs = toUCS(abs);
                QString absX = (showLabels ? "X: " : "") + formatLinear(ucs.x);
                QString absY = (showLabels ? "Y: " : "") + formatLinear(ucs.y);
                coordAbs = absX + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + absY;
            }

            bool hasUCS = m_viewport->hasUCS();
            if (prefs->showAbsolutePositionWCS && hasUCS){
                QString absX = (showLabels ? "WX: " : "W") + formatLinear(abs.x);
                QString absY = (showLabels ? "WY: " : "") + formatLinear(abs.y);

                QString coordAbsWCS = absX + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + absY;

                if (coordAbs.isEmpty()){
                    coordAbs = coordAbsWCS;
                }
                else{
                    coordAbs = coordAbs + "\n" +  coordAbsWCS;
                }
            }

            RS_Vector relativeToUse;
            if (hasUCS){
                relativeToUse = m_viewport->toUCSDelta(relative);
            }
            else{
                relativeToUse = relative;
            }

            if (prefs->showRelativePositionDistAngle) {
                QString lenStr = (showLabels ? tr("Dist: ") : "@ ") + formatLinear(relativeToUse.magnitude());
                // as we're in ucs coordinates there, use raw formatAngle instead of method

                double relativeAngle = relativeToUse.angle();
                double ucsBasisAngle = ucsAbsToBasisAngle(relativeAngle);
                QString angleStr = (showLabels ? tr("Angle: ") : "< ") + formatAngleRaw(ucsBasisAngle);

                coordPolar = lenStr + (prefs->multiLine ? "\n" : showLabels ? " " : " ") + angleStr;
            }
            if (prefs->showRelativePositionDeltas) {
                QString lenStr = (showLabels ? tr("dX: ") : "@ ") + formatLinear(relativeToUse.x);
                QString angleStr = (showLabels ? tr("dY: ") : "") + formatLinear(relativeToUse.y);

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

    m_infoCursorOverlayData->setZone1(coordAbs);
    m_infoCursorOverlayData->setZone3(coordPolar);
}

void RS_Snapper::updateUnitFormat(RS_Graphic* graphic){
    m_linearFormat = graphic->getLinearFormat();
    m_linearPrecision = graphic->getLinearPrecision();
    m_angleFormat = graphic->getAngleFormat();
    m_anglePrecision = graphic->getAnglePrecision();
    m_unit = graphic->getUnit();
}

void RS_Snapper::invalidateSnapSpot() {
    pImpData->snapSpot.valid = false;
}

QString RS_Snapper::formatLinear(double value) const{
    return RS_Units::formatLinear(value, m_unit, m_linearFormat, m_linearPrecision);
}

QString RS_Snapper::formatWCSAngle(double wcsAngle) const{
    double ucsAbsAngle;
    if (m_viewport->hasUCS()){
        ucsAbsAngle = toUCSAngle(wcsAngle);
    }
    else{
        ucsAbsAngle = wcsAngle;
    }
    double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAbsAngle, m_anglesBase, m_anglesCounterClockWise);
    return RS_Units::formatAngle(ucsBasisAngle, m_angleFormat, m_anglePrecision);
}

QString RS_Snapper::formatAngleRaw(double angle) const {
    return RS_Units::formatAngle(angle, m_angleFormat, m_anglePrecision);
}
// fixme - ucs-  move to coordinate mapper?
QString RS_Snapper::formatVector(const RS_Vector &value) const{
    double x, y;
    if (m_viewport->hasUCS()){
        RS_Vector ucsValue = m_viewport->toUCS(value);
        x = ucsValue.x;
        y = ucsValue.y;
    }
    else {
        x = value.x;
        y = value.y;
    }
    return formatLinear(x).append(" , ").append(formatLinear(y));
}

QString RS_Snapper::formatVectorWCS(const RS_Vector &value) const {
    return QString("W ").append(formatLinear(value.x)).append(" , ").append(formatLinear(value.y));
}

QString RS_Snapper::formatRelative(const RS_Vector &value) const {
    double x, y;
    m_viewport->toUCSDelta(value, x, y);
    return QString("@ ").append(formatLinear(x)).append(" , ").append(formatLinear(y));
}

QString RS_Snapper::formatPolar(const RS_Vector &value) const {
    return formatLinear(value.magnitude()).append(" < ").append(formatWCSAngle(value.angle()));
}

QString RS_Snapper::formatRelativePolar(const RS_Vector &wcsAngle) const {
    return QString("@ ").append(formatLinear(wcsAngle.magnitude())).append(" < ").append(formatWCSAngle(wcsAngle.angle()));
}

void RS_Snapper::forceUpdateInfoCursor(const RS_Vector &pos) {
    LC_OverlayInfoCursor* infoCursor = obtainInfoCursor();
    infoCursor->setPos(pos);
    infoCursor->setZonesData(m_infoCursorOverlayData.get());
}

double RS_Snapper::toWorldAngle(double ucsAbsAngle) const{
    return m_viewport->toWorldAngle(ucsAbsAngle);
}

double RS_Snapper::toWorldAngleDegrees(double ucsAbsAngleDegrees) const{
    return m_viewport->toWorldAngleDegrees(ucsAbsAngleDegrees);
}

double RS_Snapper::toUCSAngle(double wcsAngle) const{
    return m_viewport->toUCSAngle(wcsAngle);
}

double RS_Snapper::ucsAbsToBasisAngle(double ucsAbsAngle) const{
    return m_viewport->toBasisUCSAngle(ucsAbsAngle);
}

double RS_Snapper::ucsBasisToAbsAngle(double ucsRelAngle) const{
    return m_viewport->toAbsUCSAngle(ucsRelAngle);
}

double RS_Snapper::adjustRelativeAngleSignByBasis(double relativeAngle) const{
    double result;
    if (m_anglesCounterClockWise){
        result = relativeAngle;
    }
    else{
        result = -relativeAngle;
    }
    return result;
}

double RS_Snapper::toUCSBasisAngleDegrees(double wcsAngle) const{
    double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAngle, m_anglesBase, m_anglesCounterClockWise);
    double result = RS_Math::rad2deg(ucsBasisAngle);
    return result;
}

double RS_Snapper::toWorldAngleFromUCSBasisDegrees(double ucsBasisAngleDegrees) const{
    double ucsBasisAngle = RS_Math::deg2rad(ucsBasisAngleDegrees);
    double ucsAngle = m_viewport->toUCSAbsAngle(ucsBasisAngle, m_anglesBase, m_anglesCounterClockWise);
    double wcsAngle = m_viewport->toWorldAngle(ucsAngle);
    return wcsAngle;
}

double RS_Snapper::toWorldAngleFromUCSBasis(double ucsBasisAngle) const{
    double ucsAngle = m_viewport->toUCSAbsAngle(ucsBasisAngle, m_anglesBase, m_anglesCounterClockWise);
    double wcsAngle = m_viewport->toWorldAngle(ucsAngle);
    return wcsAngle;
}

double RS_Snapper::toUCSBasisAngle(double wcsAngle) const{
    double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAngle, m_anglesBase, m_anglesCounterClockWise);
    return ucsBasisAngle;
}

RS_Vector RS_Snapper::toWorld(const RS_Vector &ucsPos) const {
    return m_viewport->toWorld(ucsPos);
}

RS_Vector RS_Snapper::toUCS(const RS_Vector &worldPos) const {
    return m_viewport->toUCS(worldPos);
}

RS_Vector RS_Snapper::toWorldDelta(const RS_Vector &ucsDelta) const {
    return m_viewport->toWorldDelta(ucsDelta);
}

RS_Vector RS_Snapper::toUCSDelta(const RS_Vector &worldDelta) const {
    return m_viewport->toUCSDelta(worldDelta);
}

// todo - sand - ucs - move to coordinates mapper?
void RS_Snapper::calcRectCorners(const RS_Vector &worldCorner1, const RS_Vector &worldCorner3, RS_Vector &worldCorner2, RS_Vector &worldCorner4) const {
    RS_Vector ucsCorner1 = toUCS(worldCorner1);
    RS_Vector ucsCorner3 = toUCS(worldCorner3);
    RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
    RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);
    worldCorner2 = toWorld(ucsCorner2);
    worldCorner4 = toWorld(ucsCorner4);
}

bool RS_Snapper::hasNonDefaultAnglesBasis(){
    return  LC_LineMath::isMeaningfulAngle(m_anglesBase) || !m_anglesCounterClockWise;
}

// get catching entity distance in graph distance
double RS_Snapper::getCatchDistance(double catchDistance, int catchEntityGuiRange){
    return (m_graphicView != nullptr) ? std::min(catchDistance, toGraphDX(catchEntityGuiRange)) : catchDistance;
}

// fixme - sand - review
void RS_Snapper::enableCoordinateInput(){
    m_graphicView->enableCoordinateInput();
}

void RS_Snapper::disableCoordinateInput(){
    m_graphicView->disableCoordinateInput();
};

void RS_Snapper::redraw(RS2::RedrawMethod method) {
    // fixme - sand - ucs - decide how it's better to invoke redraw
//    viewport->requestRedraw(method);
    m_graphicView->redraw(method);
}

void RS_Snapper::redrawDrawing() {
    redraw(RS2::RedrawMethod::RedrawDrawing);
}

void RS_Snapper::redrawAll() {
    redraw(RS2::RedrawMethod::RedrawAll);
}
