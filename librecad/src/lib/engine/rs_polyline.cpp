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
#include<iostream>
#include<cmath>
#include<cassert>
#include "rs_polyline.h"

#include "rs_debug.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_information.h"

RS_PolylineData::RS_PolylineData():
	startpoint(false)
	,endpoint(false)
{
}

RS_PolylineData::RS_PolylineData(const RS_Vector& _startpoint,
				const RS_Vector& _endpoint,
				bool _closed):
	startpoint(_startpoint)
	,endpoint(_endpoint)
{

	if (_closed) {
		setFlag(RS2::FlagClosed);
	}
}

std::ostream& operator << (std::ostream& os,
								  const RS_PolylineData& pd) {
	os << "(" << pd.startpoint <<
	"/" << pd.endpoint <<
	")";
	return os;
}
/**
 * Constructor.
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent)
	:RS_EntityContainer(parent, true)
	,closingEntity(nullptr)
	,nextBulge(0.)
{
}


/**
 * Constructor.
 * @param d Polyline data
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent,
                         const RS_PolylineData& d)
		:RS_EntityContainer(parent, true)
		,data(d)
		,closingEntity(nullptr)
		,nextBulge(0.)
{
	calculateBorders();
}

RS_Entity* RS_Polyline::clone() const {
	RS_Polyline* p = new RS_Polyline(*this);
	p->setOwner(isOwner());
	p->initId();
	p->detach();
	return p;
}

/**
 * Removes the last vertex of this polyline.
 */
void RS_Polyline::removeLastVertex() {
		RS_Entity* l = last();
		if (l) {
				removeEntity(l);
				l = last();
				if (l) {
						if (l->isAtomic()) {
								data.endpoint = l->getEndpoint();
                        }
                        else {
                                RS_DEBUG->print(RS_Debug::D_WARNING,
                                        "RS_Polyline::removeLastVertex: "
                                        "polyline contains non-atomic entity");
                        }
                }
        }
}


/**
 * Adds a vertex from the endpoint of the last segment or
 * from the startpoint of the first segment to 'v' or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added with this method is the startpoint.
 *
 * @param v vertex coordinate to be added
 * @param bulge The bulge of the arc or 0 for a line segment (see DXF documentation)
 * @param prepend true: prepend at start instead of append at end
 *
 * @return Pointer to the entity that was added or nullptr if this
 *         was the first vertex added.
 */
RS_Entity* RS_Polyline::addVertex(const RS_Vector& v, double bulge, bool prepend) {

	RS_Entity* entity=nullptr;
    //static double nextBulge = 0.0;

    // very first vertex:
    if (!data.startpoint.valid) {
        data.startpoint = data.endpoint = v;
        nextBulge = bulge;
    }

    // consequent vertices:
    else {
        // add entity to the polyline:
        entity = createVertex(v, nextBulge, prepend);
		if (entity) {
						if (!prepend) {
                RS_EntityContainer::addEntity(entity);
                                data.endpoint = v;
                        }
                        else {
                RS_EntityContainer::insertEntity(0, entity);
                                data.startpoint = v;
                        }
        }
        nextBulge = bulge;
        endPolyline();
    }
    //data.endpoint = v;

    return entity;
}


/**
 * Appends a vertex list from the endpoint of the last segment
 * sets the startpoint to the first point if not exist.
 *
 * The very first vertex added with this method is the startpoint if not exists.
 *
 * @param vl list of vertexs coordinate to be added
 * @param Pair are RS_Vector of coord and the bulge of the arc or 0 for a line segment (see DXF documentation)
 *
 * @return None
 */
void RS_Polyline::appendVertexs(const std::vector< std::pair<RS_Vector, double> >& vl) {
	RS_Entity* entity=nullptr;
    //static double nextBulge = 0.0;
	if (!vl.size()) return;
	size_t idx = 0;
    // very first vertex:
    if (!data.startpoint.valid) {
		data.startpoint = data.endpoint = vl.at(idx).first;
        nextBulge = vl.at(idx++).second;
    }

    // consequent vertices:
    for (; idx< vl.size();idx++){
		entity = createVertex(vl.at(idx).first, nextBulge, false);
        data.endpoint = entity->getEndpoint();
        RS_EntityContainer::addEntity(entity);
        nextBulge = vl.at(idx).second;
    }

    endPolyline();
}


/**
 * Creates a vertex from the endpoint of the last element or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added is the starting point.
 *
 * @param v vertex coordinate
 * @param bulge The bulge of the arc (see DXF documentation)
 * @param prepend true: Prepend instead of append at end
 *
 * @return Pointer to the entity that was created or nullptr if this
 *         was the first vertex added.
 */
RS_Entity* RS_Polyline::createVertex(const RS_Vector& v, double bulge, bool prepend) {

	RS_Entity* entity=nullptr;

    RS_DEBUG->print("RS_Polyline::createVertex: %f/%f to %f/%f bulge: %f",
                    data.endpoint.x, data.endpoint.y, v.x, v.y, bulge);

    // create line for the polyline:
    if (fabs(bulge)<RS_TOLERANCE) {
		if (prepend) {
			entity = new RS_Line{this, v, data.startpoint};
		} else {
			entity = new RS_Line{this, data.endpoint, v};
		}
        entity->setSelected(isSelected());
        entity->setPen(RS_Pen(RS2::FlagInvalid));
		entity->setLayer(nullptr);
        //RS_EntityContainer::addEntity(entity);
        //data.endpoint = v;
    }

    // create arc for the polyline:
    else {
        bool reversed = (bulge<0.0);
        double alpha = atan(bulge)*4.0;

        RS_Vector middle;
        double dist;
        double angle;

				if (!prepend) {
                middle = (data.endpoint+v)/2.0;
            dist = data.endpoint.distanceTo(v)/2.0;
                angle = data.endpoint.angleTo(v);
                }
                else {
                middle = (data.startpoint+v)/2.0;
            dist = data.startpoint.distanceTo(v)/2.0;
                angle = v.angleTo(data.startpoint);
                }

        // alpha can't be 0.0 at this point
		double const radius = fabs(dist / sin(alpha/2.0));

		double const wu = fabs(radius*radius - dist*dist);
        double h = sqrt(wu);

        if (bulge>0.0) {
			angle+=M_PI_2;
        } else {
			angle-=M_PI_2;
        }

        if (fabs(alpha)>M_PI) {
            h*=-1.0;
        }

		RS_Vector center = RS_Vector::polar(h, angle);
        center+=middle;

                double a1;
                double a2;

				if (!prepend) {
                        a1 = center.angleTo(data.endpoint);
                        a2 = center.angleTo(v);
                }
                else {
                        a1 = center.angleTo(v);
                        a2 = center.angleTo(data.startpoint);
                }

		RS_ArcData const d(center, radius,
                     a1, a2,
                     reversed);

        entity = new RS_Arc(this, d);
        entity->setSelected(isSelected());
        entity->setPen(RS_Pen(RS2::FlagInvalid));
		entity->setLayer(nullptr);
    }

    return entity;
}


/**
 * Ends polyline and adds the last entity if the polyline is closed
 */
void RS_Polyline::endPolyline() {
        RS_DEBUG->print("RS_Polyline::endPolyline");

    if (isClosed()) {
                RS_DEBUG->print("RS_Polyline::endPolyline: adding closing entity");

        // remove old closing entity:
		if (closingEntity) {
            removeEntity(closingEntity);
        }

        // add closing entity to the polyline:
        closingEntity = createVertex(data.startpoint, nextBulge);
		if (closingEntity && closingEntity->getLength()>1.0E-4) {
            RS_EntityContainer::addEntity(closingEntity);
            //data.endpoint = data.startpoint;
        }
    }
    calculateBorders();
}

//RLZ: rewrite this:
void RS_Polyline::setClosed(bool cl, double bulge) {
    Q_UNUSED(bulge);
    bool areClosed = isClosed();
    setClosed(cl);
    if (isClosed()) {
        endPolyline();
    } else if (areClosed){
        removeLastVertex();
    }
}

/** sets a new start point of the polyline */
void RS_Polyline::setStartpoint(RS_Vector const& v) {
	data.startpoint = v;
	if (!data.endpoint.valid) {
		data.endpoint = v;
	}
}

/** @return Start point of the entity */
RS_Vector RS_Polyline::getStartpoint() const {
	return data.startpoint;
}

/** sets a new end point of the polyline */
void RS_Polyline::setEndpoint(RS_Vector const& v) {
	data.endpoint = v;
}

void RS_Polyline::setLayer(const QString& name) {
    RS_Entity::setLayer(name);
    // set layer for sub-entities
    for (auto *e : entities) {
        e->setLayer(layer);
    }
}

void RS_Polyline::setLayer(RS_Layer* l) {
    layer = l;
    // set layer for sub-entities
    for (auto *e : entities) {
        e->setLayer(layer);
    }
}

/** @return End point of the entity */
RS_Vector RS_Polyline::getEndpoint() const {
	return data.endpoint;
}

/**
 * @return The bulge of the closing entity.
 */
double RS_Polyline::getClosingBulge() const{
	if (isClosed()) {
		RS_Entity const* e = last();
		if (e && e->rtti()==RS2::EntityArc) {
			return static_cast<RS_Arc const*>(e)->getBulge();
		}
	}

	return 0.0;
}

bool RS_Polyline::isClosed() const {
	return data.getFlag(RS2::FlagClosed);
}

void RS_Polyline::setClosed(bool cl) {
	if (cl) {
		data.setFlag(RS2::FlagClosed);
	}
	else {
		data.delFlag(RS2::FlagClosed);
	}
}

/**
 * Sets the polylines start and endpoint to match the first and last vertex.
 */
void RS_Polyline::updateEndpoints() {
        RS_Entity* e1 = firstEntity();
		if (e1 && e1->isAtomic()) {
				RS_Vector const& v = e1->getStartpoint();
                setStartpoint(v);
        }

		RS_Entity const* e2 = last();
        if (isClosed()) {
                e2 = prevEntity();
        }
		if (e2 && e2->isAtomic()) {
				RS_Vector const& v = e2->getEndpoint();
                setEndpoint(v);
    }
}



/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() or addSegment() instead.
 */
void RS_Polyline::addEntity(RS_Entity* /*entity*/) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Polyline::addEntity:"
					" should never be called\n"
					"use addVertex() or addSegment() instead"
					);
	assert(false);
}


/**
 * Adds a segment to the polyline.
 */
/*void RS_Polyline::addSegment(RS_Entity* entity) {
        RS_EntityContainer::addEntity(entity);
        // TODO: reorder and check polyline
}*/



RS_VectorSolutions RS_Polyline::getRefPoints() const{
	RS_VectorSolutions ret{{data.startpoint}};
	for(auto e: *this){
		if (e->isAtomic()) {
			ret.push_back(e->getEndpoint());
		}
	}

    ret.push_back( data.endpoint);

    return ret;
}

RS_Vector RS_Polyline::getNearestRef( const RS_Vector& coord,
                                      double* dist /*= nullptr*/) const
{
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    return RS_Entity::getNearestRef( coord, dist);
}

RS_Vector RS_Polyline::getNearestSelectedRef( const RS_Vector& coord,
                                              double* dist /*= nullptr*/) const
{
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    return RS_Entity::getNearestSelectedRef( coord, dist);
}

/*
void RS_Polyline::reorder() {
        // current point:
        RS_Vector cp;

        bool done = false;
        do {

        } while(!done);
}
*/


/**
  * this should handle modifyOffset
  *@ coord, indicate direction of offset
  *@ distance of offset
  *
  *@Author, Dongxu Li
  */
bool RS_Polyline::offset(const RS_Vector& coord, const double& distance){
    double dist;
    //find the nearest one
    int length=count();
		std::vector<RS_Vector> intersections(length);
    if(length>1){//sort the polyline entity start/end point order
        int i(0);
        double d0,d1;
        RS_Entity* en0(entityAt(0));
        RS_Entity* en1(entityAt(1));

        RS_Vector vStart(en0->getStartpoint());
        RS_Vector vEnd(en0->getEndpoint());
        en1->getNearestEndpoint(vStart,&d0);
        en1->getNearestEndpoint(vEnd,&d1);
        if(d0<d1) en0->revertDirection();
        for(i=1;i<length;i++){
                //linked to head-tail chain
            en1=entityAt(i);
            vStart=en1->getStartpoint();
            vEnd=en1->getEndpoint();
            en0->getNearestEndpoint(vStart,&d0);
            en0->getNearestEndpoint(vEnd,&d1);
            if(d0>d1) en1->revertDirection();
            intersections[i-1]=(en0->getEndpoint()+en1->getStartpoint())*0.5;
            en0=en1;
        }
		if (isClosed()) {
			en1=entityAt(0);
            intersections[length-1]=(en0->getEndpoint()+en1->getStartpoint())*0.5;
		}

    }
    RS_Entity* en(getNearestEntity(coord, &dist, RS2::ResolveNone));
	if(!en) return false;
    int indexNearest=findEntity(en);
    //        RS_Vector vp(en->getNearestPointOnEntity(coord,false));
    //        RS_Vector direction(en->getTangentDirection(vp));
    //        RS_Vector vp1(-direction.y,direction.x);//normal direction
    //        double a2(vp1.squared());
    //        if(a2<RS_TOLERANCE2) return false;
    //        vp1 *= distance/sqrt(a2);
    //        move(vp1);
    //        return true;

    RS_Polyline* pnew= static_cast<RS_Polyline*>(clone());
    int i;
    i=indexNearest;
    int previousIndex(i);
    pnew->entityAt(i)->offset(coord,distance);
    RS_Vector vp;
    //offset all
    //fixme, this is too ugly
    for(i=indexNearest-1;i>=0;i--){
        RS_VectorSolutions sol0=RS_Information::getIntersection(pnew->entityAt(previousIndex),entityAt(i),true);
//        RS_VectorSolutions sol1;
        double dmax(RS_TOLERANCE15);
        RS_Vector trimP(false);
		for(const RS_Vector& vp: sol0){

			double d0( (vp - pnew->entityAt(previousIndex)->getStartpoint()).squared());//potential bug, need to trim better
            if(d0>dmax) {
                dmax=d0;
				trimP=vp;
            }
        }
        if(trimP.valid){
            static_cast<RS_AtomicEntity*>(pnew->entityAt(previousIndex))->trimStartpoint(trimP);
            static_cast<RS_AtomicEntity*>(pnew->entityAt(i))->trimEndpoint(trimP);
            vp=pnew->entityAt(previousIndex)->getMiddlePoint();
        }else{
            vp=pnew->entityAt(previousIndex)->getStartpoint();
            vp.rotate(entityAt(previousIndex)->getStartpoint(),entityAt(i)->getDirection2()-entityAt(previousIndex)->getDirection1()+M_PI);
        }
        pnew->entityAt(i)->offset(vp,distance);
        previousIndex=i;
    }

    previousIndex=indexNearest;
    for(i=indexNearest+1;i<length;i++){
        RS_VectorSolutions sol0=RS_Information::getIntersection(pnew->entityAt(previousIndex),entityAt(i),true);
//        RS_VectorSolutions sol1;
        double dmax(RS_TOLERANCE15);
        RS_Vector trimP(false);
		for(const RS_Vector& vp: sol0){
			double d0( (vp - pnew->entityAt(previousIndex)->getEndpoint()).squared());//potential bug, need to trim better
            if(d0>dmax) {
                dmax=d0;
				trimP=vp;
            }
        }
        if(trimP.valid){
            static_cast<RS_AtomicEntity*>(pnew->entityAt(previousIndex))->trimEndpoint(trimP);
            static_cast<RS_AtomicEntity*>(pnew->entityAt(i))->trimStartpoint(trimP);
            vp=pnew->entityAt(previousIndex)->getMiddlePoint();
        }else{
            vp=pnew->entityAt(previousIndex)->getEndpoint();
            vp.rotate(entityAt(previousIndex)->getEndpoint(),entityAt(i)->getDirection1()-entityAt(previousIndex)->getDirection2()+M_PI);
        }
        pnew->entityAt(i)->offset(vp,distance);
        previousIndex=i;
    }
    //trim
    //connect and trim        RS_Modification m(*container, graphicView);
    for(i=0;i<length;i++){
		RS_Entity* en0;
		RS_Entity* en1;
		if (i<length-1){
			en0=pnew->entityAt(i);
			en1=pnew->entityAt(i+1);
		}else{
			if (isClosed()) {
				en0=pnew->entityAt(i);
				en1=pnew->entityAt(0);
			}else{
				break;			
			}
		}
		RS_VectorSolutions sol0=RS_Information::getIntersection(en0,en1,true);
        if(sol0.getNumber()==0){
            sol0=RS_Information::getIntersection(en0,en1);
//            RS_Vector vp0(pnew->entityAt(i)->getEndpoint());
//            RS_Vector vp1(pnew->entityAt(i+1)->getStartpoint());
//            double a0(intersections.at(i).angleTo(vp0));
//            double a1(intersections.at(i).angleTo(vp1));
            RS_VectorSolutions sol1;
			//This lead result isn't connected.
			//for(const RS_Vector& vp: sol0){
			//	if(!RS_Math::isAngleBetween(intersections.at(i).angleTo(vp),
            //                               pnew->entityAt(i)->getDirection2(),
            //                               pnew->entityAt(i+1)->getDirection1(),
			//							   false)){
			//		sol1.push_back(vp);
            //    }
            //}
			sol1=sol0;
            if(sol1.getNumber()==0) continue;
            RS_Vector trimP(sol1.getClosest(intersections.at(i)));
            static_cast<RS_AtomicEntity*>(en0)->trimEndpoint(trimP);
            static_cast<RS_AtomicEntity*>(en1)->trimStartpoint(trimP);
        }else{
            RS_Vector trimP(sol0.getClosest(intersections.at(i)));
            static_cast<RS_AtomicEntity*>(en0)->trimEndpoint(trimP);
            static_cast<RS_AtomicEntity*>(en1)->trimStartpoint(trimP);
        }

    }

    *this = *pnew;
    return true;


}

void RS_Polyline::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    data.startpoint.move(offset);
    data.endpoint.move(offset);
    calculateBorders();
}



void RS_Polyline::rotate(const RS_Vector& center, const double& angle) {
    rotate(center, RS_Vector(angle));
}


void RS_Polyline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    calculateBorders();
}



void RS_Polyline::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
    calculateBorders();
}



void RS_Polyline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}



void RS_Polyline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
        RS_EntityContainer::moveRef(ref, offset);
    if (ref.distanceTo(data.startpoint)<1.0e-4) {
       data.startpoint.move(offset);
    }
    if (ref.distanceTo(data.endpoint)<1.0e-4) {
       data.endpoint.move(offset);
    }
    calculateBorders();
    //update();
}

void RS_Polyline::revertDirection() {
	RS_EntityContainer::revertDirection();
	RS_Vector tmp = data.startpoint;
	data.startpoint = data.endpoint;
	data.endpoint = tmp;
}

void RS_Polyline::stretch(const RS_Vector& firstCorner,
                          const RS_Vector& secondCorner,
                          const RS_Vector& offset) {

    if (data.startpoint.isInWindow(firstCorner, secondCorner)) {
        data.startpoint.move(offset);
    }
    if (data.endpoint.isInWindow(firstCorner, secondCorner)) {
        data.endpoint.move(offset);
    }

        RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    calculateBorders();
}


/**
 * Slightly optimized drawing for polylines.
 */
void RS_Polyline::draw(RS_Painter* painter,RS_GraphicView* view, double& /*patternOffset*/) {

	if (!view) return;

    // draw first entity and set correct pen:
    RS_Entity* e = firstEntity(RS2::ResolveNone);
    // We get the pen from the entitycontainer and apply it to the
    // first line so that subsequent line are draw in the right color
    //prevent segfault if polyline is empty
	if (e) {
        RS_Pen p=this->getPen(true);
        e->setPen(p);
        double patternOffset=0.;
        view->drawEntity(painter, e, patternOffset);

        e = nextEntity(RS2::ResolveNone);
		while(e) {
            view->drawEntityPlain(painter, e, patternOffset);
            e = nextEntity(RS2::ResolveNone);
        }
    }
}




/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Polyline& l) {
    os << " Polyline: " << l.getData() << " {\n";

    os << (RS_EntityContainer&)l;

    os << "\n}\n";

    return os;
}

