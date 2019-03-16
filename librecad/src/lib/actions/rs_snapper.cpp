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


#include<cmath>
#include<QMouseEvent>
#include "rs_snapper.h"

#include "rs_point.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_settings.h"
#include "rs_overlayline.h"
#include "rs_coordinateevent.h"
#include "rs_entitycontainer.h"
#include "rs_pen.h"
#include "rs_debug.h"

struct RS_Snapper::Indicator
{
    bool lines_state;
    QString lines_type;
    RS_Pen lines_pen;

    bool shape_state;
    QString shape_type;
    RS_Pen shape_pen;
};

struct RS_Snapper::ImpData {
RS_Vector snapCoord;
RS_Vector snapSpot;
};

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(RS_EntityContainer& container, RS_GraphicView& graphicView)
    :container(&container)
    ,graphicView(&graphicView)
	,pImpData(new ImpData{})
    ,snap_indicator(new Indicator{})
{}

RS_Snapper::~RS_Snapper()
{
    delete snap_indicator;
}

/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init() 
{
    snapMode = graphicView->getDefaultSnapMode();
	keyEntity = nullptr;
	pImpData->snapSpot = RS_Vector{false};
	pImpData->snapCoord = RS_Vector{false};
	m_SnapDistance = 1.0;

    RS_SETTINGS->beginGroup("/Appearance");
    snap_indicator->lines_state = RS_SETTINGS->readNumEntry("/indicator_lines_state", 1);
    snap_indicator->lines_type = RS_SETTINGS->readEntry("/indicator_lines_type", "Crosshair");
    snap_indicator->shape_state = RS_SETTINGS->readNumEntry("/indicator_shape_state", 1);
    snap_indicator->shape_type = RS_SETTINGS->readEntry("/indicator_shape_type", "Circle");
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("Colors");
    QString snap_color = RS_SETTINGS->readEntry("/snap_indicator", Colors::snap_indicator);
    RS_SETTINGS->endGroup();

	snap_indicator->lines_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::DashLine2);
	snap_indicator->shape_pen = RS_Pen(RS_Color(snap_color), RS2::Width00, RS2::SolidLine);
	snap_indicator->shape_pen.setScreenWidth(1);

    snapRange=getSnapRange();
}

void RS_Snapper::finish() {
    finished = true;
    deleteSnapper();
}

/**
  * Disable all snapping.
  *
  * This effectively puts the object into free snap mode.
  *
  * @returns A reference to itself.
  */
 RS_SnapMode const & RS_SnapMode::clear()
{
	snapFree     = false;
	snapGrid     = false;
	snapEndpoint     = false;
	snapMiddle       = false;
	snapDistance       = false;
	snapCenter       = false;
	snapOnEntity     = false;
	snapIntersection = false;

	restriction = RS2::RestrictNothing;

	return *this;
}

bool RS_SnapMode::operator ==(RS_SnapMode const& rhs) const{
	if ( snapFree != rhs.snapFree) return false;
	if ( snapGrid != rhs.snapGrid) return false;
	if ( snapEndpoint != rhs.snapEndpoint) return false;
	if ( snapMiddle != rhs.snapMiddle) return false;
	if ( snapDistance != rhs.snapDistance) return false;
	if ( snapCenter != rhs.snapCenter) return false;
	if ( snapOnEntity != rhs.snapOnEntity) return false;
	if ( snapIntersection != rhs.snapIntersection) return false;
	if ( restriction != rhs.restriction) return false;
	return true;
}

void RS_Snapper::setSnapMode(const RS_SnapMode& snapMode) {
    this->snapMode = snapMode;
	RS_DIALOGFACTORY->requestSnapDistOptions(m_SnapDistance, snapMode.snapDistance);
    RS_DIALOGFACTORY->requestSnapMiddleOptions(middlePoints, snapMode.snapMiddle);
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
	pImpData->snapSpot=graphicView->toGraph(e->x(), e->y());
	pImpData->snapCoord=pImpData->snapSpot;
    snap_indicator->lines_state=true;
	return pImpData->snapCoord;
}

/**
 * Snap to a coordinate in the drawing using the current snap mode.
 *
 * @param e A mouse event.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapPoint(QMouseEvent* e)
{
	pImpData->snapSpot = RS_Vector(false);
    RS_Vector t(false);

	if (!e) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Snapper::snapPoint: event is nullptr");
		return pImpData->snapSpot;
    }

    RS_Vector mouseCoord = graphicView->toGraph(e->x(), e->y());
    double ds2Min=RS_MAXDOUBLE*RS_MAXDOUBLE;

    if (snapMode.snapEndpoint) {
        t = snapEndpoint(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);

        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }
    if (snapMode.snapCenter) {
        t = snapCenter(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }
    if (snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions
		RS_DIALOGFACTORY->requestSnapMiddleOptions(middlePoints, snapMode.snapMiddle);
        t = snapMiddle(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }
    if (snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
		RS_DIALOGFACTORY->requestSnapDistOptions(m_SnapDistance, snapMode.snapDistance);
        t = snapDist(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }
    if (snapMode.snapIntersection) {
        t = snapIntersection(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }

    if (snapMode.snapOnEntity &&
		pImpData->snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
        t = snapOnEntity(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }

    if (snapMode.snapGrid) {
        t = snapGrid(mouseCoord);
		double ds2=mouseCoord.squaredTo(t);
        if (ds2 < ds2Min){
//            ds2Min=ds2;
			pImpData->snapSpot = t;
        }
    }

	if( !pImpData->snapSpot.valid ) {
		pImpData->snapSpot=mouseCoord; //default to snapFree
    } else {
//        std::cout<<"mouseCoord.distanceTo(snapSpot)="<<mouseCoord.distanceTo(snapSpot)<<std::endl;
        //        std::cout<<"snapRange="<<snapRange<<std::endl;

        //retreat to snapFree when distance is more than half grid
        if(snapMode.snapFree){
			RS_Vector const& ds=mouseCoord - pImpData->snapSpot;
			RS_Vector const& grid=graphicView->getGrid()->getCellVector()*0.5;
			if( fabs(ds.x) > fabs(grid.x) ||  fabs(ds.y) > fabs(grid.y) ) pImpData->snapSpot = mouseCoord;
        }

        //another choice is to keep snapRange in GUI coordinates instead of graph coordinates
//        if (mouseCoord.distanceTo(snapSpot) > snapRange ) snapSpot = mouseCoord;
    }
    //if (snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
    // handle snap restrictions that can be activated in addition
    //   to the ones above:
    //apply restriction
    RS_Vector rz = graphicView->getRelativeZero();
	RS_Vector vpv(rz.x, pImpData->snapSpot.y);
	RS_Vector vph(pImpData->snapSpot.x,rz.y);
    switch (snapMode.restriction) {
    case RS2::RestrictOrthogonal:
		pImpData->snapCoord= ( mouseCoord.distanceTo(vpv)< mouseCoord.distanceTo(vph))?
                    vpv:vph;
        break;
    case RS2::RestrictHorizontal:
		pImpData->snapCoord = vph;
        break;
    case RS2::RestrictVertical:
		pImpData->snapCoord = vpv;
        break;

    //case RS2::RestrictNothing:
    default:
		pImpData->snapCoord = pImpData->snapSpot;
        break;
    }
    //}
    //else snapCoord = snapSpot;

	snapPoint(pImpData->snapSpot, false);

	return pImpData->snapCoord;
}


/**manually set snapPoint*/
RS_Vector RS_Snapper::snapPoint(const RS_Vector& coord, bool setSpot)
{
    if(coord.valid){
		pImpData->snapSpot=coord;
		if(setSpot) pImpData->snapCoord = coord;
		drawSnapper();
		RS_DIALOGFACTORY->updateCoordinateWidget(
					pImpData->snapCoord,
					pImpData->snapCoord - graphicView->getRelativeZero());
    }
    return coord;
}
double RS_Snapper::getSnapRange() const
{
	if(graphicView )
    return (graphicView->getGrid()->getCellVector()*0.5).magnitude();
    return 20.;
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
		if (entity && entity->getParent()) {
                idx = entity->getParent()->findEntity(entity);
        }

	if (entity && dist<=getSnapRange()) {
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
		if (entity && entity->getParent()) {
                idx = entity->getParent()->findEntity(entity);
        }

	if (entity && dist<=getSnapRange()) {
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

    return catchEntity(
               RS_Vector(graphicView->toGraphX(e->x()),
                         graphicView->toGraphY(e->y())),
               level);
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
    return catchEntity(
			   {graphicView->toGraphX(e->x()), graphicView->toGraphY(e->y())},
				enType,
				level);
}

RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e, const std::initializer_list<RS2::EntityType>& enTypeList,
                                   RS2::ResolveLevel level) {
	RS_Entity* pten = nullptr;
	RS_Vector coord{graphicView->toGraphX(e->x()), graphicView->toGraphY(e->y())};
    switch(enTypeList.size()) {
    case 0:
        return catchEntity(coord, level);
    default:
    {

		RS_EntityContainer ec(nullptr,false);
		for( auto t0: enTypeList){
			RS_Entity* en=catchEntity(coord, t0, level);
			if(en) ec.addEntity(en);
//			if(en) {
//            std::cout<<__FILE__<<" : "<<__func__<<" : lines "<<__LINE__<<std::endl;
//            std::cout<<"caught id= "<<en->getId()<<std::endl;
//            }
        }
        if(ec.count()>0){
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

/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideOptions() {
    //not used any more, will be removed
}

/**
 * Shows the snapper options. Default implementation does nothing.
 */
void RS_Snapper::showOptions() {
    //not used any more, will be removed
}


/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper()
{
    graphicView->getOverlayContainer(RS2::Snapper)->clear();
    graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}



/**
 * creates the snap indicator
 */
void RS_Snapper::drawSnapper()
{
    // We could properly speed this up by calling the draw function of this snapper within the paint event
    // this will avoid creating/deletion of the lines

    graphicView->getOverlayContainer(RS2::Snapper)->clear();
	if (!finished && pImpData->snapSpot.valid)
    {
        RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::Snapper);

        if (snap_indicator->lines_state)
        {
            QString type = snap_indicator->lines_type;

            if (type == "Crosshair")
            {
                RS_OverlayLine *line = new RS_OverlayLine(nullptr,
                    {{0., graphicView->toGuiY(pImpData->snapCoord.y)},
                    {double(graphicView->getWidth()),
                    graphicView->toGuiY(pImpData->snapCoord.y)}});

                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr,
                    {{graphicView->toGuiX(pImpData->snapCoord.x),0.},
                    {graphicView->toGuiX(pImpData->snapCoord.x),
                    double(graphicView->getHeight())}});

                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);
            }
            else if (type == "Crosshair2")
            {
                double xenoRadius=16;

                double snapX=graphicView->toGuiX(pImpData->snapCoord.x);
                double snapY=graphicView->toGuiY(pImpData->snapCoord.y);

                double viewWidth=double(graphicView->getWidth());
                double viewHeight=double(graphicView->getHeight());

                RS_OverlayLine *line;

                // ----O     (Left)
                line=new RS_OverlayLine(nullptr, {
                    {0., snapY},
                    {snapX-xenoRadius, snapY}
                });
                {
                    line->setPen(snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                //     O---- (Right)
                line=new RS_OverlayLine(nullptr, {
                    {snapX+xenoRadius, snapY},
                    {viewWidth, snapY}
                });
                {
                    line->setPen(snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                // (Top)
                line=new RS_OverlayLine(nullptr, {
                    {snapX, 0.},
                    {snapX, snapY-xenoRadius}
                });
                {
                    line->setPen(snap_indicator->lines_pen);
                    container->addEntity(line);
                }

                // (Bottom)
                line=new RS_OverlayLine(nullptr, {
                    {snapX, snapY+xenoRadius},
                    {snapX, viewHeight}
                });
                {
                    line->setPen(snap_indicator->lines_pen);
                    container->addEntity(line);
                }
            }
            else if (type == "Isometric")
            {
                //isometric crosshair
                RS2::CrosshairType chType=graphicView->getCrosshairType();
                RS_Vector direction1;
                RS_Vector direction2(0.,1.);
                double l=graphicView->getWidth()+graphicView->getHeight();
                switch(chType){
                case RS2::RightCrosshair:
                    direction1=RS_Vector(M_PI*5./6.)*l;
                    direction2*=l;
                    break;
                case RS2::LeftCrosshair:
                    direction1=RS_Vector(M_PI*1./6.)*l;
                    direction2*=l;
                    break;
                default:
                    direction1=RS_Vector(M_PI*1./6.)*l;
                    direction2=RS_Vector(M_PI*5./6.)*l;
                }
                RS_Vector center(graphicView->toGui(pImpData->snapCoord));
                RS_OverlayLine *line=new RS_OverlayLine(container,
                {center-direction1,center+direction1});
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);
                line=new RS_OverlayLine(nullptr,
                {center-direction2,center+direction2});
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);
            }
            else if (type == "Spiderweb")
            {
                RS_OverlayLine* line;
                RS_Vector point1;
                RS_Vector point2;

                point1 = RS_Vector{0, 0};
                point2 = RS_Vector{graphicView->toGuiX(pImpData->snapCoord.x),
                                   graphicView->toGuiY(pImpData->snapCoord.y)};
                line=new RS_OverlayLine{nullptr, {point1, point2}};
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(0, graphicView->getHeight());
                line = new RS_OverlayLine{nullptr, {point1, point2}};
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(graphicView->getWidth(), 0);
                line = new RS_OverlayLine(nullptr, {point1, point2});
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);

                point1 = RS_Vector(graphicView->getWidth(), graphicView->getHeight());
                line = new RS_OverlayLine(nullptr, {point1, point2});
                line->setPen(snap_indicator->lines_pen);
                container->addEntity(line);
            }
        }
        if (snap_indicator->shape_state)
        {
            QString type = snap_indicator->shape_type;

            if (type == "Circle")
            {
                RS_Circle *circle=new RS_Circle(container,
                    {pImpData->snapCoord, 4./graphicView->getFactor().x});
                circle->setPen(snap_indicator->shape_pen);
                container->addEntity(circle);
            }
            else if (type == "Point")
            {
                RS_Point *point=new RS_Point(container, pImpData->snapCoord);
                point->setPen(snap_indicator->shape_pen);
                container->addEntity(point);
            }
            else if (type == "Square")
            {
                RS_Vector snap_point{graphicView->toGuiX(pImpData->snapCoord.x),
                                     graphicView->toGuiY(pImpData->snapCoord.y)};

                double a = 6.0;
                RS_Vector p1 = snap_point + RS_Vector(-a, a);
                RS_Vector p2 = snap_point + RS_Vector(a, a);
                RS_Vector p3 = snap_point + RS_Vector(a, -a);
                RS_Vector p4 = snap_point + RS_Vector(-a, -a);

                RS_OverlayLine* line;
                line=new RS_OverlayLine{nullptr, {p1, p2}};
                line->setPen(snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine{nullptr, {p2, p3}};
                line->setPen(snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr, {p3, p4});
                line->setPen(snap_indicator->shape_pen);
                container->addEntity(line);

                line = new RS_OverlayLine(nullptr, {p4, p1});
                line->setPen(snap_indicator->shape_pen);
                container->addEntity(line);
            }
        }
        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

/**
  * snap mode to a flag integer
  */
unsigned int RS_Snapper::snapModeToInt(const RS_SnapMode& s)
{
    unsigned int ret; //initial
    switch (s.restriction) {
    case RS2::RestrictHorizontal:
        ret=1;
        break;
    case RS2::RestrictVertical:
        ret=2;
        break;
    case RS2::RestrictOrthogonal:
        ret=3;
        break;
    default:
        ret=0;
    }
    ret <<=1;ret |= s.snapFree;
    ret <<=1;ret |= s.snapGrid;
    ret <<=1;ret |= s.snapEndpoint;
    ret <<=1;ret |= s.snapMiddle;
    ret <<=1;ret |= s.snapDistance;
    ret <<=1;ret |= s.snapCenter;
    ret <<=1;ret |= s.snapOnEntity;
    ret <<=1;ret |= s.snapIntersection;
   return ret;
}
/**
  * integer flag to snapMode
  */
RS_SnapMode RS_Snapper::intToSnapMode(unsigned int ret)
{
    RS_SnapMode s; //initial
    unsigned int binaryOne(0x1);
    s.snapIntersection =ret & binaryOne;
    ret >>= 1;
    s.snapOnEntity =ret & binaryOne;
    ret >>= 1;
    s.snapCenter =ret & binaryOne;
    ret >>= 1;
    s.snapDistance =ret & binaryOne;
    ret >>= 1;
    s.snapMiddle =ret & binaryOne;
    ret >>= 1;
    s.snapEndpoint =ret & binaryOne;
    ret >>= 1;
    s.snapGrid =ret & binaryOne;
    ret >>= 1;
    s.snapFree =ret & binaryOne;
    ret >>= 1;
    switch (ret) {
    case 1:
            s.restriction=RS2::RestrictHorizontal;
        break;
    case 2:
            s.restriction=RS2::RestrictVertical;
        break;
    case 3:
            s.restriction=RS2::RestrictOrthogonal;
        break;
    default:
            s.restriction=RS2::RestrictNothing;
    }
   return s;
}
