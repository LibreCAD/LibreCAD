/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 Dongxu Li (github.com/dxli)
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

#include <iostream>

#include "lc_containertraverser.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pen.h"

RS_PolylineData::RS_PolylineData(const RS_Vector& startpoint, const RS_Vector& endpoint, const bool closed) : startpoint(startpoint),
    endpoint(endpoint) {
    if (closed) {
        setFlag(RS2::FlagClosed);
    }
}

std::ostream& operator <<(std::ostream& os, const RS_PolylineData& pd) {
    os << "(" << pd.startpoint << "/" << pd.endpoint << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent)
    : RS_EntityContainer(parent, true) {
}

/**
 * Constructor.
 * @param parent
 * @param d Polyline data
 */
RS_Polyline::RS_Polyline(RS_EntityContainer* parent, const RS_PolylineData& d)
    : RS_EntityContainer(parent, true), m_data(d) {
    RS_Polyline::calculateBorders();
}

RS_Entity* RS_Polyline::clone() const {
    auto* p = new RS_Polyline(*this);
    p->setOwner(isOwner());
    p->detach();
    return p;
}

/**
 * Removes the last vertex of this polyline.
 */
void RS_Polyline::removeLastVertex() {
    RS_Entity* l = last();
    if (l != nullptr) {
        removeEntity(l);
        l = last();
        if (l != nullptr) {
            if (l->isAtomic()) {
                m_data.endpoint = l->getEndpoint();
            }
            else {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Polyline::removeLastVertex: " "polyline contains non-atomic entity");
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
RS_Entity* RS_Polyline::addVertex(const RS_Vector& v, const double bulge, const bool prepend) {
    // very first vertex:
    if (!m_data.startpoint.valid) {
        m_data.startpoint = v;
        m_data.endpoint = v;
        m_nextBulge = bulge;
        return nullptr;
    }
    // consequent vertices:

    // add entity to the polyline:
    std::unique_ptr<RS_Entity> vertex = createVertex(v, m_nextBulge, prepend);
    RS_Entity* entity = vertex.get();
    if (entity != nullptr) {
        if (!prepend) {
            RS_EntityContainer::addEntity(entity);
            m_data.endpoint = v;
        }
        else {
            RS_EntityContainer::insertEntity(0, entity);
            m_data.startpoint = v;
        }
        vertex.release();
    }
    m_nextBulge = bulge;
    endPolyline();

    return entity;
}

/**
 * Appends a vertex list from the endpoint of the last segment
 * sets the startpoint to the first point if not exist.
 *
 * The very first vertex added with this method is the startpoint if not exists.
 *
 * @param vl list of vertexs coordinate to be added of coord and the bulge of the arc or 0 for a line segment (see DXF documentation)
 *
 * @return None
 */
void RS_Polyline::appendVertexs(const std::vector<std::pair<RS_Vector, double>>& vl) {
    //static double nextBulge = 0.0;
    if (vl.empty()) {
        return;
    }
    size_t idx = 0;
    const auto first = vl.at(idx).first;
    // very first vertex:
    if (!m_data.startpoint.valid) {
        m_data.startpoint = first;
        m_data.endpoint = first;
        m_nextBulge = vl.at(idx++).second;
    }

    // consequent vertices:
    for (; idx < vl.size(); idx++) {
        auto current = vl.at(idx);
        std::unique_ptr<RS_Entity> vertex = createVertex(current.first, m_nextBulge, false);
        m_data.endpoint = vertex->getEndpoint();
        const RS_Entity* entity = vertex.get();
        vertex.release();
        RS_EntityContainer::addEntity(entity);
        m_nextBulge = current.second;
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
std::unique_ptr<RS_Entity> RS_Polyline::createVertex(const RS_Vector& v, const double bulge, const bool prepend) {
    std::unique_ptr<RS_Entity> entity;

    RS_DEBUG->print("RS_Polyline::createVertex: %f/%f to %f/%f bulge: %f", m_data.endpoint.x, m_data.endpoint.y, v.x, v.y, bulge);

    // create line for the polyline:
    if (std::abs(bulge) < RS_TOLERANCE || std::abs(bulge) >= RS_MAXDOUBLE) {
        entity = std::make_unique<RS_Line>(this, prepend ? v : m_data.endpoint, prepend ? m_data.startpoint : v);
    }
    else {
        // create arc for the polyline:
        const bool reversed = std::signbit(bulge);
        const double alpha = std::atan(std::abs(bulge)) * 4.0;

        const RS_Vector start = prepend ? m_data.startpoint : m_data.endpoint;
        const RS_Vector middle = (start + v) / 2.0;
        const double dist = start.distanceTo(v) / 2.0;
        const double angle = start.angleTo(v);

        // alpha can't be 0.0 at this point
        const double radius = std::abs(dist / std::sin(alpha / 2.0));

        const double wu = std::abs(radius * radius - dist * dist);
        const double angleNew = reversed ? angle - M_PI_2 : angle + M_PI_2;
        const double h = (std::abs(alpha) > M_PI) ? -std::sqrt(wu) : std::sqrt(wu);

        const auto center = middle + RS_Vector::polar(h, angleNew);
        const double startAngle = center.angleTo(prepend ? v : m_data.endpoint);
        const double endAngle = center.angleTo(prepend ? m_data.startpoint : v);

        // Issue #1946: always create Ellipse for fonts
        // Issue #2067: limit elliptic segments for fonts

        const RS_ArcData d(center, radius, startAngle, endAngle, reversed);

        entity = std::make_unique<RS_Arc>(this, d);
    }
    // entity->setSelectionFlag(isSelected());  // fixme - what for? entity is part of the polyline, selected status is from polyline..
    // entity->setPen(RS_Pen(RS2::FlagInvalid));
    // entity->setLayer(nullptr);
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
        if (m_closingEntity) {
            removeEntity(m_closingEntity);
        }

        // add closing entity to the polyline:
        std::unique_ptr<RS_Entity> vertex = createVertex(m_data.startpoint, m_nextBulge);
        m_closingEntity = vertex.get();
        vertex.release();
        if (m_closingEntity && m_closingEntity->getLength() > 1.0E-4) {
            RS_EntityContainer::addEntity(m_closingEntity);
            //data.endpoint = data.startpoint;
        }
    }
    calculateBorders();
}

//RLZ: rewrite this:
void RS_Polyline::setClosed(const bool cl, [[maybe_unused]] double bulge) {
    const bool areClosed = isClosed();
    setClosed(cl);
    if (isClosed()) {
        endPolyline();
    }
    else if (areClosed) {
        removeLastVertex();
    }
}

/** sets a new start point of the polyline */
void RS_Polyline::setStartpoint(const RS_Vector& v) {
    m_data.startpoint = v;
    if (!m_data.endpoint.valid) {
        m_data.endpoint = v;
    }
}

/** @return Start point of the entity */
RS_Vector RS_Polyline::getStartpoint() const {
    return m_data.startpoint;
}

/** sets a new end point of the polyline */
void RS_Polyline::setEndpoint(const RS_Vector& v) {
    m_data.endpoint = v;
}

/** @return End point of the entity */
RS_Vector RS_Polyline::getEndpoint() const {
    return m_data.endpoint;
}

/**
 * @return The bulge of the closing entity.
 */
double RS_Polyline::getClosingBulge() const {
    if (isClosed()) {
        const RS_Entity* e = last();
        if (e != nullptr && e->rtti() == RS2::EntityEllipse) {
            return static_cast<const RS_Ellipse*>(e)->getBulge();
        }
    }

    return 0.0;
}

bool RS_Polyline::isClosed() const {
    return m_data.getFlag(RS2::FlagClosed);
}

void RS_Polyline::setClosed(const bool cl) {
    if (cl) {
        m_data.setFlag(RS2::FlagClosed);
    }
    else {
        m_data.delFlag(RS2::FlagClosed);
    }
}

/**
 * Sets the polylines start and endpoint to match the first and last vertex.
 */
void RS_Polyline::updateEndpoints() {
    using namespace lc;
    // LC_ContainerTraverser traverser{*this, RS2::ResolveNone};
    const RS_Entity* e1 = firstEntity();
    if (e1 != nullptr && e1->isAtomic()) {
        const RS_Vector& v = e1->getStartpoint();
        setStartpoint(v);
    }

    // last two entities
    LC_ContainerTraverser revTraverser{*this, RS2::ResolveNone, LC_ContainerTraverser::Direction::Backword};

    const RS_Entity* e2 = revTraverser.first();
    if (isClosed()) {
        e2 = revTraverser.next();
    }
    if (e2 != nullptr && e2->isAtomic()) {
        const RS_Vector& v = e2->getEndpoint();
        setEndpoint(v);
    }
}

/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() or addSegment() instead.
 */
void RS_Polyline::addEntity(const RS_Entity*) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Polyline::addEntity:" " should never be called\n" "use addVertex() or addSegment() instead");
    assert(false);
}

RS_VectorSolutions RS_Polyline::getRefPoints() const {
    RS_VectorSolutions ret{{m_data.startpoint}};
    for (const auto e : *this) {
        if (e->isAtomic()) {
            if (e->isArc()) {
                ret.push_back(e->getMiddlePoint());
            }
            ret.push_back(e->getEndpoint());
        }
    }
    ret.push_back(m_data.endpoint);
    return ret;
}

RS_Vector RS_Polyline::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    return RS_Entity::doGetNearestRef(coord, dist);
}

RS_Vector RS_Polyline::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for vertex dragging
    return RS_Entity::doGetNearestSelectedRef(coord, dist);
}

/**
  * this should handle modifyOffset
  *@ coord, indicate direction of offset
  *@ distance of offset
  *
  *@Author, Dongxu Li
  */
bool RS_Polyline::offset(const RS_Vector& coord, double distance) {
    double dist;
    //find the nearest one
    int length = count();
    std::vector<RS_Vector> intersections(length);
    if (length > 1) {
        //sort the polyline entity start/end point order
        RS_Entity* en0(entityAt(0));
        RS_Entity* en1(entityAt(1));

        RS_Vector vStart(en0->getStartpoint());
        RS_Vector vEnd(en0->getEndpoint());
        double d0 = 0., d1 = 0.;
        en1->getNearestEndpoint(vStart, nullptr, &d0);
        en1->getNearestEndpoint(vEnd, nullptr, &d1);
        if (d0 < d1) {
            en0->revertDirection();
        }
        for (int i = 1; i < length; i++) {
            //linked to head-tail chain
            en1 = entityAt(i);
            vStart = en1->getStartpoint();
            vEnd = en1->getEndpoint();
            en0->getNearestEndpoint(vStart, nullptr, &d0);
            en0->getNearestEndpoint(vEnd, nullptr,  &d1);
            if (d0 > d1) {
                en1->revertDirection();
            }
            intersections[i - 1] = (en0->getEndpoint() + en1->getStartpoint()) * 0.5;
            en0 = en1;
        }
        if (isClosed()) {
            en1 = entityAt(0);
            intersections[length - 1] = (en0->getEndpoint() + en1->getStartpoint()) * 0.5;
        }
    }
    RS_Entity* en(getNearestEntity(coord, &dist, RS2::ResolveNone));
    if (en == nullptr) {
        return false;
    }
    int indexNearest = findEntity(en);
    //        RS_Vector vp(en->getNearestPointOnEntity(coord,false));
    //        RS_Vector direction(en->getTangentDirection(vp));
    //        RS_Vector vp1(-direction.y,direction.x);//normal direction
    //        double a2(vp1.squared());
    //        if(a2<RS_TOLERANCE2) return false;
    //        vp1 *= distance/sqrt(a2);
    //        move(vp1);
    //        return true;

    auto* pnew = static_cast<RS_Polyline*>(clone());
    int i = indexNearest;
    int previousIndex = i;
    pnew->entityAt(i)->offset(coord, distance);
    //offset all
    //fixme, this is too ugly
    for (i = indexNearest - 1; i >= 0; i--) {
        auto previousNewEntity = pnew->entityAt(previousIndex);
        RS_VectorSolutions sol0 = RS_Information::getIntersection(previousNewEntity, entityAt(i), true);
        //        RS_VectorSolutions sol1;
        double dmax(RS_TOLERANCE15);
        RS_Vector trimP(false);
        for (const RS_Vector& vp : sol0) {
            double d0 = (vp - previousNewEntity->getStartpoint()).squared(); //potential bug, need to trim better
            if (d0 > dmax) {
                dmax = d0;
                trimP = vp;
            }
        }
        RS_Vector vp;
        if (trimP.valid) {
            static_cast<RS_AtomicEntity*>(previousNewEntity)->trimStartpoint(trimP);
            static_cast<RS_AtomicEntity*>(pnew->entityAt(i))->trimEndpoint(trimP);
            vp = previousNewEntity->getMiddlePoint();
        }
        else {
            vp = previousNewEntity->getStartpoint();
            vp.rotate(entityAt(previousIndex)->getStartpoint(),
                      entityAt(i)->getDirection2() - entityAt(previousIndex)->getDirection1() + M_PI);
        }
        pnew->entityAt(i)->offset(vp, distance);
        previousIndex = i;
    }

    previousIndex = indexNearest;
    for (i = indexNearest + 1; i < length; i++) {
        auto previousEntity = pnew->entityAt(previousIndex);
        RS_VectorSolutions sol0 = RS_Information::getIntersection(previousEntity, entityAt(i), true);
        //        RS_VectorSolutions sol1;
        double dmax(RS_TOLERANCE15);
        RS_Vector trimP(false);
        for (const RS_Vector& vp : sol0) {
            double d0((vp - previousEntity->getEndpoint()).squared());
            //potential bug, need to trim better
            if (d0 > dmax) {
                dmax = d0;
                trimP = vp;
            }
        }
        RS_Vector vp;
        if (trimP.valid) {
            static_cast<RS_AtomicEntity*>(previousEntity)->trimEndpoint(trimP);
            static_cast<RS_AtomicEntity*>(pnew->entityAt(i))->trimStartpoint(trimP);
            vp = previousEntity->getMiddlePoint();
        }
        else {
            vp = previousEntity->getEndpoint();
            vp.rotate(entityAt(previousIndex)->getEndpoint(),
                      entityAt(i)->getDirection1() - entityAt(previousIndex)->getDirection2() + M_PI);
        }
        pnew->entityAt(i)->offset(vp, distance);
        previousIndex = i;
    }
    //trim
    //connect and trim        RS_Modification m(*container, graphicView);
    for (i = 0; i < length; i++) {
        RS_Entity* en0;
        RS_Entity* en1;
        if (i < length - 1) {
            en0 = pnew->entityAt(i);
            en1 = pnew->entityAt(i + 1);
        }
        else {
            if (isClosed()) {
                en0 = pnew->entityAt(i);
                en1 = pnew->entityAt(0);
            }
            else {
                break;
            }
        }
        RS_VectorSolutions sol0 = RS_Information::getIntersection(en0, en1, true);
        if (sol0.getNumber() == 0) {
            sol0 = RS_Information::getIntersection(en0, en1);
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
            sol1 = sol0;
            if (sol1.getNumber() == 0) {
                continue;
            }
            RS_Vector trimP(sol1.getClosest(intersections.at(i)));
            static_cast<RS_AtomicEntity*>(en0)->trimEndpoint(trimP);
            static_cast<RS_AtomicEntity*>(en1)->trimStartpoint(trimP);
        }
        else {
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
    m_data.startpoint.move(offset);
    m_data.endpoint.move(offset);
    calculateBorders();
}

void RS_Polyline::rotate(const RS_Vector& center, const double angle) {
    rotate(center, RS_Vector(angle));
}

void RS_Polyline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    m_data.startpoint.rotate(center, angleVector);
    m_data.endpoint.rotate(center, angleVector);
    calculateBorders();
}

void RS_Polyline::scale(const RS_Vector& center, const RS_Vector& factor) {
    // fixme - this is incorrect in-place editing of polyline - it will break undo!!! First, clone should be created and that clone should be modified
    if (!RS_Math::equal(factor.x, factor.y)) {
        for (size_t i = 0; i < count(); ++i) {
            RS_Entity* e = entityAt(i);
            if (e->rtti() == RS2::EntityArc) {
                const auto* arc = static_cast<RS_Arc*>(e);
                const RS_Vector majorP(arc->getRadius(), 0.0);
                RS_EllipseData ed{arc->getCenter(), majorP, 1.0, arc->getAngle1(), arc->getAngle2(), arc->isReversed()};
                auto* ellipse = new RS_Ellipse(this, ed);
                ellipse->setSelectionFlag(arc->isSelected()); // fixme - review what for? this is part of polyline anyway...
                ellipse->setPen(RS_Pen(RS2::FlagInvalid));
                ellipse->setLayer(nullptr);
                setEntityAt(i, ellipse);
            }
        }
    }
    RS_EntityContainer::scale(center, factor);
    m_data.startpoint.scale(center, factor);
    m_data.endpoint.scale(center, factor);
    calculateBorders();
}

bool RS_Polyline::containsArc() const {
    return std::any_of(cbegin(), cend(), [](const RS_Entity* entity) {
        return entity->rtti() == RS2::EntityArc;
    });
}

void RS_Polyline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    m_data.startpoint.mirror(axisPoint1, axisPoint2);
    m_data.endpoint.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

void RS_Polyline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    RS_EntityContainer::moveRef(ref, offset);
    if (ref.distanceTo(m_data.startpoint) < 1.0e-4) {
        m_data.startpoint.move(offset);
    }
    if (ref.distanceTo(m_data.endpoint) < 1.0e-4) {
        m_data.endpoint.move(offset);
    }
    calculateBorders();
    //update();
}

void RS_Polyline::revertDirection() {
    RS_EntityContainer::revertDirection();
    const RS_Vector tmp = m_data.startpoint;
    m_data.startpoint = m_data.endpoint;
    m_data.endpoint = tmp;
}

void RS_Polyline::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    if (m_data.startpoint.isInWindow(firstCorner, secondCorner)) {
        m_data.startpoint.move(offset);
    }
    if (m_data.endpoint.isInWindow(firstCorner, secondCorner)) {
        m_data.endpoint.move(offset);
    }

    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    calculateBorders();
}

/**
 * Slightly optimized drawing for polylines.
 */
void RS_Polyline::draw(RS_Painter* painter) {
    if (count() == 0) {
        return;
    }
    painter->drawEntityPolyline(this);
}

void RS_Polyline::drawAsChild(RS_Painter* painter) {
    painter->drawEntityPolyline(this);
}

RS_Ellipse* RS_Polyline::convertToEllipse(const std::pair<RS_Arc*, double>& arcPair) {
    const RS_Arc* arc = arcPair.first;
    const double scaleRatio = arcPair.second;
    const double radius = arc->getRadius();
    double major, ratio;
    RS_Vector majorP;
    const bool reversed = arc->isReversed();
    if (scaleRatio >= 1.0) {
        // Major along y
        major = radius * scaleRatio;
        ratio = 1.0 / scaleRatio;
        majorP = RS_Vector(0.0, major);
    }
    else {
        // Major along x
        major = radius;
        ratio = scaleRatio;
        majorP = RS_Vector(major, 0.0);
    }

    const RS_EllipseData d{arc->getCenter(), majorP, ratio, arc->getAngle1(), arc->getAngle2(), reversed};

    auto* ellipse = new RS_Ellipse(arc->getParent(), d);
    ellipse->setSelectionFlag(arc->isSelected());
    ellipse->setPen(RS_Pen(RS2::FlagInvalid));
    ellipse->setLayer(nullptr);

    return ellipse;
}

std::pair<RS_Arc*, double> RS_Polyline::convertToArcPair(const RS_Ellipse* ellipse) {
    const double elRatio = ellipse->getRatio();
    const RS_Vector elMajorP = ellipse->getMajorP();
    const double majorRadius = elMajorP.magnitude();
    const double angle = elMajorP.angle();
    const bool alongX = fabs(angle) < RS_TOLERANCE || fabs(angle - M_PI) < RS_TOLERANCE;
    const bool alongY = fabs(angle - M_PI_2) < RS_TOLERANCE || fabs(angle - 3 * M_PI_2) < RS_TOLERANCE;

    if (!alongX && !alongY) {
        // Not axis-aligned, cannot convert simply
        RS_DEBUG->print(RS_Debug::D_WARNING, "convertToArcPair: Ellipse not axis-aligned");
        return {nullptr, 0.0};
    }

    double scaleRatio;
    double arcRadius;
    if (alongX) {
        arcRadius = majorRadius;
        scaleRatio = elRatio;
    }
    else {
        // alongY
        arcRadius = majorRadius * elRatio; // since minor is along x, arcRadius as x semi
        scaleRatio = 1.0 / elRatio;
    }

    const RS_ArcData d(ellipse->getCenter(), arcRadius, ellipse->getAngle1(), ellipse->getAngle2(), ellipse->isReversed());

    auto arc = new RS_Arc(ellipse->getParent(), d);
    arc->setSelectionFlag(ellipse->isSelected()); // fixme - review what for? this is part of polyline anyway...
    arc->setPen(RS_Pen(RS2::FlagInvalid));
    arc->setLayer(nullptr);

    return {arc, scaleRatio};
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Polyline& l) {
    os << " Polyline: " << l.getData() << " {\n";
    auto asContainer = static_cast<const RS_EntityContainer&>(l);
    os << asContainer;
    os << "\n}\n";
    return os;
}

/**
 * finds vertex that is adjacent to given (previous or next)
 * @param previousSegment - direction of previous segment if true, false - next one
 * @param refPoint
 * @return
 */
RS_Vector RS_Polyline::getRefPointAdjacentDirection(const bool previousSegment, const RS_Vector& refPoint) const {
    RS_Vector previous = getStartpoint();
    if (refPoint == previous) {
        // handle start point
        return entityAt(0)->getEndpoint();
    }
    bool breakOnNextVertex = false;
    for (const RS_Entity* entity : lc::LC_ContainerTraverser{*this, RS2::ResolveAll}.entities()) {
        RS_Vector segmentEndPoint = entity->getEndpoint();
        if (breakOnNextVertex) {
            return segmentEndPoint;
        }
        if (segmentEndPoint == refPoint) {
            if (previousSegment) {
                return previous;
            }
            breakOnNextVertex = true;
        }
        else {
            previous = segmentEndPoint;
        }
    }
    return previous;
}

QList<RS_Vector> RS_Polyline::getVertexes() const {
    QList<RS_Vector> result;
    for (const auto e : getEntityList()) {
        result.push_back(e->getStartpoint());
    }
    result.push_back(getEntityList().last()->getEndpoint());

    return result;
}

// NOTE: It's intentional absense of index range validity check
RS_Vector RS_Polyline::getVertex(const int index) const {
    int idx = 0;
    for (const auto e : getEntityList()) {
        if (index == idx) {
            return e->getStartpoint();
        }
        idx++;
    }
    return getEntityList().last()->getEndpoint();
}

RS_Arc* RS_Polyline::arcFromBulge(const RS_Vector& start, const RS_Vector& end, const double bulge) {
    if (std::abs(bulge) < RS_TOLERANCE || std::abs(bulge) >= RS_MAXDOUBLE) {
        return nullptr;
    }
    const bool reversed = std::signbit(bulge);
    const double alpha = std::atan(std::abs(bulge)) * 4.0;
    const RS_Vector middle = (start + end) / 2.0;
    const double dist = start.distanceTo(end) / 2.0;
    const double angle = start.angleTo(end);
    const double radius = std::abs(dist / std::sin(alpha / 2.0));
    const double wu = std::abs(radius * radius - dist * dist);
    const double h = (std::abs(alpha) > M_PI) ? -std::sqrt(wu) : std::sqrt(wu);
    const double angleNew = reversed ? angle - M_PI_2 : angle + M_PI_2;
    const RS_Vector center = middle + RS_Vector::polar(h, angleNew);
    double a1 = center.angleTo(start);
    double a2 = center.angleTo(end);
    if (reversed) {
        std::swap(a1, a2);
    }
    const RS_ArcData d(center, radius, a1, a2, reversed);
    return new RS_Arc(nullptr, d);
}
