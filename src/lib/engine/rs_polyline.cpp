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


#include "rs_polyline.h"

#include "rs_debug.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_graphicview.h"


/**
 * Constructor.
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent)
        :RS_EntityContainer(parent) {

    closingEntity = NULL;
    nextBulge = 0.0;
}


/**
 * Constructor.
 * @param d Polyline data
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent,
                         const RS_PolylineData& d)
        :RS_EntityContainer(parent), data(d) {

    closingEntity = NULL;
    nextBulge = 0.0;
    calculateBorders();
}


/**
 * Destructor
 */
RS_Polyline::~RS_Polyline() {}


/**
 * Removes the last vertex of this polyline.
 */
void RS_Polyline::removeLastVertex() {
        RS_Entity* last = lastEntity();
        if (last!=NULL) {
                removeEntity(last);
                last = lastEntity();
                if (last!=NULL) {
                        if (last->isAtomic()) {
                                data.endpoint = ((RS_AtomicEntity*)last)->getEndpoint();
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
 * @return Pointer to the entity that was addded or NULL if this
 *         was the first vertex added.
 */
RS_Entity* RS_Polyline::addVertex(const RS_Vector& v, double bulge, bool prepend) {

    RS_Entity* entity=NULL;
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
        if (entity!=NULL) {
                        if (prepend==false) {
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
 * Creates a vertex from the endpoint of the last element or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added is the starting point.
 *
 * @param v vertex coordinate
 * @param bulge The bulge of the arc (see DXF documentation)
 * @param prepend true: Prepend instead of append at end
 *
 * @return Pointer to the entity that was created or NULL if this
 *         was the first vertex added.
 */
RS_Entity* RS_Polyline::createVertex(const RS_Vector& v, double bulge, bool prepend) {

    RS_Entity* entity=NULL;

    RS_DEBUG->print("RS_Polyline::createVertex: %f/%f to %f/%f bulge: %f",
                    data.endpoint.x, data.endpoint.y, v.x, v.y, bulge);

    // create line for the polyline:
    if (fabs(bulge)<RS_TOLERANCE) {
                if (prepend==false) {
                entity = new RS_Line(this, RS_LineData(data.endpoint, v));
                }
                else {
                entity = new RS_Line(this, RS_LineData(v, data.startpoint));
                }
        entity->setSelected(isSelected());
        entity->setPen(RS_Pen(RS2::FlagInvalid));
        entity->setLayer(NULL);
        //RS_EntityContainer::addEntity(entity);
        //data.endpoint = v;
    }

    // create arc for the polyline:
    else {
        bool reversed = (bulge<0.0);
        double alpha = atan(bulge)*4.0;

        double radius;
        RS_Vector center;
        RS_Vector middle;
        double dist;
        double angle;

                if (prepend==false) {
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
        radius = fabs(dist / sin(alpha/2.0));

        double wu = fabs(RS_Math::pow(radius, 2.0) - RS_Math::pow(dist, 2.0));
        double h = sqrt(wu);

        if (bulge>0.0) {
            angle+=M_PI/2.0;
        } else {
            angle-=M_PI/2.0;
        }

        if (fabs(alpha)>M_PI) {
            h*=-1.0;
        }

        center.setPolar(h, angle);
        center+=middle;

                double a1;
                double a2;

                if (prepend==false) {
                        a1 = center.angleTo(data.endpoint);
                        a2 = center.angleTo(v);
                }
                else {
                        a1 = center.angleTo(v);
                        a2 = center.angleTo(data.startpoint);
                }

        RS_ArcData d(center, radius,
                     a1, a2,
                     reversed);

        entity = new RS_Arc(this, d);
        entity->setSelected(isSelected());
        entity->setPen(RS_Pen(RS2::FlagInvalid));
        entity->setLayer(NULL);
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
        if (closingEntity!=NULL) {
            removeEntity(closingEntity);
        }

        // add closing entity to the polyline:
        closingEntity = createVertex(data.startpoint, nextBulge);
        if (closingEntity!=NULL) {
            RS_EntityContainer::addEntity(closingEntity);
            //data.endpoint = data.startpoint;
        }
    }
    calculateBorders();
}

//RLZ: rewrite this:
void RS_Polyline::setClosed(bool cl, double bulge) {
    Q_UNUSED(bulge);
    setClosed(cl);
    if (isClosed()) {
        endPolyline();
    } else {
        removeLastVertex();
    }
}


/**
 * @return The bulge of the closing entity.
 */
double RS_Polyline::getClosingBulge() {
    if (isClosed()) {
                RS_Entity* e = lastEntity();
                if (e!=NULL && e->rtti()==RS2::EntityArc) {
                        return ((RS_Arc*)e)->getBulge();
                }
        }

        return 0.0;
}


/**
 * Sets the polylines start and endpoint to match the first and last vertex.
 */
void RS_Polyline::updateEndpoints() {
        RS_Entity* e1 = firstEntity();
        if (e1!=NULL && e1->isAtomic()) {
                RS_Vector v = ((RS_AtomicEntity*)e1)->getStartpoint();
                setStartpoint(v);
        }

        RS_Entity* e2 = lastEntity();
        if (isClosed()) {
                e2 = prevEntity();
        }
        if (e2!=NULL && e2->isAtomic()) {
                RS_Vector v = ((RS_AtomicEntity*)e2)->getEndpoint();
                setEndpoint(v);
    }
}



/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() or addSegment() instead.
 */
void RS_Polyline::addEntity(RS_Entity* entity) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Polyline::addEntity:"
                    " should never be called");

    if (entity==NULL) {
        return;
    }
    delete entity;
}


/**
 * Adds a segment to the polyline.
 */
/*void RS_Polyline::addSegment(RS_Entity* entity) {
        RS_EntityContainer::addEntity(entity);
        // TODO: reorder and check polyline
}*/



RS_VectorSolutions RS_Polyline::getRefPoints() {
    RS_VectorSolutions ret;

    ret.push_back(data.startpoint);

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
         e!=NULL;
         e = nextEntity(RS2::ResolveNone)) {
        if (e->isAtomic()) {
            ret.push_back(((RS_AtomicEntity*)e)->getEndpoint());
        }
    }

    ret.push_back( data.endpoint);

    return ret;
}

RS_Vector RS_Polyline::getNearestRef(const RS_Vector& coord,
                                   double* dist) {

    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Polyline::getNearestSelectedRef(const RS_Vector& coord,
        double* dist) {

    return RS_Entity::getNearestSelectedRef(coord, dist);
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



void RS_Polyline::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    data.startpoint.move(offset);
    data.endpoint.move(offset);
}



void RS_Polyline::rotate(const RS_Vector& center, const double& angle) {
    rotate(center, RS_Vector(angle));
}


void RS_Polyline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
}



void RS_Polyline::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
}



void RS_Polyline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
}



void RS_Polyline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
        RS_EntityContainer::moveRef(ref, offset);
    if (ref.distanceTo(data.startpoint)<1.0e-4) {
       data.startpoint.move(offset);
    }
    if (ref.distanceTo(data.endpoint)<1.0e-4) {
       data.endpoint.move(offset);
    }
    //update();
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
}


/**
 * Slightly optimized drawing for polylines.
 */
void RS_Polyline::draw(RS_Painter* painter,RS_GraphicView* view, double& /*patternOffset*/) {

    if (view==NULL) {
        return;
    }

        // draw first entity and set correct pen:
    RS_Entity* e = firstEntity(RS2::ResolveNone);
    // We get the pen from the entitycontainer and apply it to the
    // first line so that subsequent line are draw in the right color
    RS_Pen p=this->getPen(true);
//prevent segfault if polyline is empty
    double patternOffset=0.;
    if (e != NULL) {
        e->setPen(p);
        view->drawEntity(painter, e);

        // draw subsequent entities with same pen:
        for (RS_Entity* e=nextEntity(RS2::ResolveNone);
             e!=NULL;
             e = nextEntity(RS2::ResolveNone)) {

            view->drawEntityPlain(painter, e, patternOffset);
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

