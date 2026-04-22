/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "rs_information.h"

#include <random>

#include "lc_containertraverser.h"
#include "lc_parabola.h"
#include "lc_quadratic.h"
#include "lc_rect.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"

class LC_Parabola;

namespace {
    // The tolerance in finding tangent point
    // Tangent point is assumed, if the gap between to curves is less than this factor
    // times the curve length
    constexpr double g_tangentTolerance = 1e-7;

    // whether the entity is circular (circle or arc)
    bool isArc(const RS_Entity* e) {
        if (e == nullptr) {
            return false;
        }
        switch (e->rtti()) {
            case RS2::EntityArc:
            case RS2::EntityCircle:
                return true;
            default:
                return false;
        }
    }

    // whether the entity is a line
    bool isLine(const RS_Entity* e) {
        return e != nullptr && e->rtti() == RS2::EntityLine;
    }

    // whether the entity is an ellipse
    bool isEllipse(const RS_Entity* e) {
        return e != nullptr && e->rtti() == RS2::EntityEllipse;
    }

    // whether the entity is an ellipse
    bool isParabola(const RS_Entity* e) {
        return e != nullptr && e->rtti() == RS2::EntityParabola;
    }

    /**
     * @brief The TangentFinder class, a helper class for tangent point finding
     * Assuming there's no intersection between the two curves (e1, e2)
     */
    class TangentFinder {
        const RS_Entity* m_e1 = nullptr;
        const RS_Entity* m_e2 = nullptr;

    public:
        TangentFinder(const RS_Entity* e1, const RS_Entity* e2) : m_e1{e1}, m_e2{e2} {
        }

        // Find tangent point
        RS_VectorSolutions getTangent() const;
    };

    // Type specific algorithms for tangent point finding
    class TangentFinderAlgo {
    public:
        virtual ~TangentFinderAlgo() = default;

        /**
         * @brief operator () main API to find a tangent point
         * @param e1 - the first entity
         * @param e2 - the second entity
         * @return std::pair<RS_VectorSolutions, bool> -
         *      RS_VectorSolutions, the tangent point found, could be empty
         *      bool, whether the algorithm has been applied. If true is returned,
         *      the algorithm has been applied, no extra algorithm is necessary;
         *      if false is applied, the current algorithm is not applicable due to
         *      mismatched entity types.
         */
        virtual std::pair<RS_VectorSolutions, bool> operator ()(const RS_Entity* e1, const RS_Entity* e2) const = 0;

    protected:
        // Find tangent points by trying offsets of two sides
        RS_VectorSolutions findTangent(const RS_Entity* e1, const RS_Entity* e2) const {
            const double tol = g_tangentTolerance * std::min(e1->getLength(), e2->getLength());

            RS_VectorSolutions ret = findOffsetIntersections(e1, e2, tol);

            if (ret.empty()) {
                ret = findOffsetIntersections(e1, e2, -tol);
            }
            return ret;
        }

    private:
        // Find tangent point by bisection of the offset distance
        // If two intersections are found, keep reducing the offset distance, until the two intersections
        // are close enough
        RS_VectorSolutions findOffsetIntersections(const RS_Entity* e1, const RS_Entity* e2, const double aOffsetValue) const {
            double offsetValue = aOffsetValue;
            std::unique_ptr<RS_Entity> offset = createOffset(e1, offsetValue);
            RS_VectorSolutions ret = LC_Quadratic::getIntersection(offset->getQuadratic(), e2->getQuadratic());
            if (ret.size() <= 1) {
                return ret;
            }

            // invariant: offset: low (zero intersection), high: 2 intersection
            double low = 0.;
            double high = offsetValue;
            while (ret.at(0).distanceTo(ret.at(1)) > 1e-7 && high - low > RS_TOLERANCE) {
                offsetValue = (low + high) * 0.5;
                offset = createOffset(e1, offsetValue);
                RS_VectorSolutions retMid = LC_Quadratic::getIntersection(offset->getQuadratic(), e2->getQuadratic());
                switch (retMid.size()) {
                    case 0:
                        low = offsetValue;
                        break;
                    case 1:
                        retMid.setTangent(true);
                        return retMid;
                    default:
                        ret = std::move(retMid);
                        high = offsetValue;
                }
            }
            RS_Vector tangent = (ret.at(0) + ret.at(1)) * 0.5;
            tangent = e2->getNearestPointOnEntity(tangent, false);
            ret = RS_VectorSolutions{tangent};
            ret.setTangent(true);
            return ret;
        }

        // Create an offset curve, according to the distance factor
        // The sign of the factor is used to indicate the two sides of offsets
        virtual std::unique_ptr<RS_Entity> createOffset([[maybe_unused]] const RS_Entity* e1, [[maybe_unused]] double offsetValue) const {
            return {};
        }
    };

    // Algorithm for cases with one entity being a line, and assuming the other entity is not a line
    class TangentFinderAlgoLine : public TangentFinderAlgo {
    public:
        std::pair<RS_VectorSolutions, bool> operator ()(const RS_Entity* e1, const RS_Entity* e2) const override {
            if (isLine(e2)) {
                std::swap(e1, e2);
            }
            if (!isLine(e1)) {
                return {};
            }
            // Line-Line doesn't have any tangent point
            if (isLine(e2)) {
                return {};
            }

            RS_VectorSolutions ret = findTangent(e1, e2);
            return {ret, true};
        }

        std::unique_ptr<RS_Entity> createOffset(const RS_Entity* e1, const double offsetValue) const override {
            std::unique_ptr<RS_Entity> line{e1->clone()};
            const auto normal = RS_Vector{e1->getDirection1() + M_PI / 2.};
            line->move(normal * offsetValue);
            return line;
        }
    };

    // Algorithm for cases with one entity being circular, i.e. an arc or circle
    class TangentFinderAlgoArc : public TangentFinderAlgo {
    public:
        std::pair<RS_VectorSolutions, bool> operator ()(const RS_Entity* e1, const RS_Entity* e2) const override {
            if (isArc(e2)) {
                std::swap(e1, e2);
            }
            if (!isArc(e1)) {
                return {};
            }
            RS_VectorSolutions ret = findTangent(e1, e2);
            return {ret, true};
        }

        std::unique_ptr<RS_Entity> createOffset(const RS_Entity* e1, const double offsetValue) const override {
            std::unique_ptr<RS_Entity> circle{e1->clone()};
            circle->setRadius(e1->getRadius() * (1. + offsetValue));
            return circle;
        }
    };

    // Algorithm for cases with one entity being an ellipse
    class TangentFinderAlgoEllipse : public TangentFinderAlgo {
    public:
        std::pair<RS_VectorSolutions, bool> operator ()(const RS_Entity* e1, const RS_Entity* e2) const override {
            if (isEllipse(e2)) {
                std::swap(e1, e2);
            }
            if (!isEllipse(e1)) {
                return {};
            }
            const auto ellipse = static_cast<const RS_Ellipse*>(e1);
            const RS_Circle c1{nullptr, {ellipse->getCenter(), ellipse->getMinorRadius() * (1.0 + g_tangentTolerance)}};
            const std::unique_ptr<RS_Entity> other{e2->clone()};
            other->rotate(ellipse->getCenter(), -ellipse->getAngle());
            other->scale(ellipse->getCenter(), {ellipse->getRatio(), 1.});

            RS_VectorSolutions ret;
            bool processed = false;
            std::tie(ret, processed) = TangentFinderAlgoArc{}(&c1, other.get());
            ret.scale(ellipse->getCenter(), {1. / ellipse->getRatio(), 1.});
            ret.rotate(ellipse->getCenter(), ellipse->getAngle());
            return {ret, true};
        }
    };

    // Algorithm for cases with one entity being a parabola
    class TangentFinderAlgoParabola : public TangentFinderAlgo {
    public:
        std::pair<RS_VectorSolutions, bool> operator ()(const RS_Entity* e1, const RS_Entity* e2) const override {
            if (isParabola(e2)) {
                std::swap(e1, e2);
            }
            if (!isParabola(e1)) {
                return {};
            }

            RS_VectorSolutions ret = findTangent(e1, e2);
            return {ret, true};
        }

        std::unique_ptr<RS_Entity> createOffset(const RS_Entity* e1, const double offsetValue) const override {
            const auto parabola = static_cast<const LC_Parabola*>(e1);
            return parabola->approximateOffset(offsetValue);
        }
    };

    RS_VectorSolutions TangentFinder::getTangent() const {
        // TODO: handle splinepoints, parabola, and hyperbola
        std::unique_ptr<TangentFinderAlgo> algos[] = {
            std::make_unique<TangentFinderAlgoLine>(),
            std::make_unique<TangentFinderAlgoArc>(),
            std::make_unique<TangentFinderAlgoEllipse>(),
            std::make_unique<TangentFinderAlgoParabola>()
        };

        for (const std::unique_ptr<TangentFinderAlgo>& algo : algos) {
            const auto& [points, processed] = (*algo)(m_e1, m_e2);
            if (processed) {
                RS_VectorSolutions ret = points;
                //
                if (points.size() == 2) {
                    const RS_Vector center = (points.at(0) + points.at(1)) * 0.5;
                    RS_Vector projection = m_e1->getNearestPointOnEntity(center, false);
                    projection = m_e2->getNearestPointOnEntity(projection, false);
                    ret = {projection};
                }
                if (!ret.empty()) {
                    ret.setTangent(true);
                }

                return ret;
            }
        }
        return {};
    }
}

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Information::RS_Information(RS_EntityContainer& container) : m_container(&container) {
}

/**
 * @return true: if the entity is a dimensioning entity.
 *         false: otherwise
 */
bool RS_Information::isDimension(const RS2::EntityType type) {
    return RS2::isDimensionalEntity(type);
}

/**
 * @retval true the entity can be trimmed.
 * i.e. it is in a graphic or in a polyline.
 */
bool RS_Information::isTrimmable(const RS_Entity* e) {
    if (e != nullptr) {
        if (e->getParent() != nullptr) {
            switch (e->getParent()->rtti()) {
                case RS2::EntityPolyline:
                case RS2::EntityContainer:
                case RS2::EntityGraphic:
                case RS2::EntityBlock:
                    return true;
                default:
                    return false;
            }
        }
    }

    return false;
}

/**
 * Gets the nearest end point to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics
 * container.
 */
RS_Vector RS_Information::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    return m_container->getNearestEndpoint(coord, nullptr,  dist);
}

/**
 * Gets the nearest point to the given coordinate which is on an entity.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param onEntity
 * @param dist Pointer to a double which will contain the
 *        measured distance after return or nullptr
 * @param entity Pointer to a pointer which will point to the
 *        entity on which the point is or nullptr
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics
 * container.
 */
RS_Vector RS_Information::getNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    return m_container->getNearestPointOnEntity(coord, onEntity, dist, entity);
}

/**
 * Gets the nearest entity to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a double which will contain the
 *             masured distance after return
 * @param level Level of resolving entities.
 *
 * @return the entity found or nullptr if there are no elements
 * at all in this graphics container.
 */
RS_Entity* RS_Information::getNearestEntity(const RS_Vector& coord, double* dist, const RS2::ResolveLevel level) const {
    return m_container->getNearestEntity(coord, dist, level);
}

/**
 * Calculates the intersection point(s) between two entities.
 *
 * @param entity1
 * @param entity2
 * @param onEntities true: only return intersection points which are
 *                   on both entities.
 *                   false: return all intersection points.
 *
 * @todo support more entities
 *
 * @return All intersections of the two entities. The tangent flag in
 * RS_VectorSolutions is set if one intersection is a tangent point.
 */
RS_VectorSolutions RS_Information::getIntersection(const RS_Entity* entity1, const RS_Entity* entity2, const bool onEntities) {
    RS_VectorSolutions ret;

    if (!((entity1 != nullptr) && (entity2 != nullptr))) {
        RS_DEBUG->print("RS_Information::getIntersection() for nullptr entities");
        return ret;
    }
    if (entity1->getId() == entity2->getId()) {
        RS_DEBUG->print("RS_Information::getIntersection() of the same entity");
        return ret;
    }

    // unsupported entities / entity combinations:
    const RS2::EntityType e1Type = entity1->rtti();
    const RS2::EntityType e2Type = entity2->rtti();
    if (e1Type == RS2::EntityMText || e2Type == RS2::EntityMText || e1Type == RS2::EntityText || e2Type == RS2::EntityText ||
        isDimension(e1Type) || isDimension(e2Type)) {
        return ret;
    }

    const bool e1Constructional =  e1Type == RS2::EntityConstructionLine ||  e1Type == RS2::EntitySnapConstructionLine || entity1->isConstruction();
    const bool e2Constructional =  e2Type == RS2::EntityConstructionLine ||  e2Type == RS2::EntitySnapConstructionLine || entity2->isConstruction();

    if (onEntities && !(e1Constructional || e2Constructional)) {
        // a little check to avoid doing unneeded intersections, an attempt to avoid O(N^2) increasing of checking two-entity information
        const LC_Rect rect1{entity1->getMin(), entity1->getMax()};
        const LC_Rect rect2{entity2->getMin(), entity2->getMax()};

        if (onEntities && !rect1.intersects(rect2, RS_TOLERANCE)) {
            return ret;
        }
    }

    const auto parentOne = entity1->getParent();
    //avoid intersections between line segments the same spline
    /* ToDo: 24 Aug 2011, Dongxu Li, if rtti() is not defined for the parent, the following check for splines may still cause segfault */
    if (parentOne != nullptr && parentOne == entity2->getParent()) {
        if (parentOne->rtti() == RS2::EntitySpline) {
            //do not calculate intersections from neighboring lines of a spline
            if (parentOne->areNeighborsEntities(entity1, entity2)) {
                return ret;
            }
        }
    }

    if (e1Type == RS2::EntitySplinePoints || e2Type == RS2::EntitySplinePoints) {
        ret = LC_SplinePoints::getIntersection(entity1, entity2);
    }
    else {
        // issue #484 , quadratic intersection solver is not robust enough for quadratic-quadratic
        // TODO, implement a robust algorithm for quadratic based solvers, and detecting entity type
        // circles/arcs can be removed
        // issue #523: TangentFinder cannot handle line-line
        const bool firstIsLine = e1Type == RS2::EntityLine || e1Type == RS2::EntityConstructionLine || e1Type == RS2::EntitySnapConstructionLine;
        const bool secondIsLine = e2Type == RS2::EntityLine || e2Type == RS2::EntityConstructionLine || e2Type == RS2::EntitySnapConstructionLine;
        const bool isLineLine = firstIsLine && secondIsLine;
        if (isLineLine) {
            ret = getIntersectionLineLine(entity1, entity2);
        }
        else if (isArc(entity1)) {
            std::swap(entity1, entity2);
            if (isArc(entity1)) {
                //use specialized arc-arc intersection solver
                ret = getIntersectionArcArc(entity1, entity2);
            }
            else if (firstIsLine) {
                ret = getIntersectionLineArc(entity1, entity2);
            }
        }

        if (ret.empty()) {
            ret = LC_Quadratic::getIntersection(entity1->getQuadratic(), entity2->getQuadratic());
            if (!isLineLine && ret.empty()) {
                // TODO: the following tangent point recovery only works if there's no other intersection
                // Issue #523: direct intersection finding may fail for tangent points
                // Accept small distance gap as tangent points. While this will allow some false tangent
                // points from close but non-contacting curves, it's more important to reliably find all
                // tangent points with rounding errors.
                ret = TangentFinder{entity1, entity2}.getTangent();
            }
        }
    }
    RS_VectorSolutions ret2;
    for (const RS_Vector& vp : ret) {
        constexpr double tol = 1.0e-4;  // fixme - sand - use tolerance settings for intersection???
        if (!vp.valid) {
            continue;
        }
        if (onEntities) {
            //ignore intersections not on entity
            if (!((entity1->isConstruction(true) || entity1->isPointOnEntity(vp, tol)) &&
                 ((entity2->isConstruction(true) || entity2-> isPointOnEntity(vp, tol))))) {
                //				std::cout<<"Ignored intersection "<<vp<<std::endl;
                //				std::cout<<"because: e1->isPointOnEntity(ret.get(i), tol)="<<e1->isPointOnEntity(vp, tol)
                //					<<"\t(e2->isPointOnEntity(ret.get(i), tol)="<<e2->isPointOnEntity(vp, tol)<<std::endl;
                continue;
            }
        }
        // need to test whether the intersection is tangential
        RS_Vector direction1 = entity1->getTangentDirection(vp);
        RS_Vector direction2 = entity2->getTangentDirection(vp);
        if (direction1.valid && direction2.valid && fabs(
            fabs(direction1.dotP(direction2)) - sqrt(direction1.squared() * direction2.squared())) < sqrt(tol) * tol) {
            ret2.setTangent(true);
        }
        //TODO, make the following tangential test, nearest test work for all entity types

        //        RS_Entity   *lpLine = nullptr,
        //                    *lpCircle = nullptr;
        //        if( RS2::EntityLine == e1->rtti() && RS2::EntityCircle == e2->rtti()) {
        //            lpLine = e1;
        //            lpCircle = e2;
        //        }
        //        else if( RS2::EntityCircle == e1->rtti() && RS2::EntityLine == e2->rtti()) {
        //            lpLine = e2;
        //            lpCircle = e1;
        //        }
        //        if( nullptr != lpLine && nullptr != lpCircle) {
        //            double dist = 0.0;
        //            RS_Vector nearest = lpLine->getNearestPointOnEntity( lpCircle->getCenter(), false, &dist);

        //            // special case: line touches circle tangent
        //            if( nearest.valid && fabs( dist - lpCircle->getRadius()) < tol) {
        //                ret.set(i,nearest);
        //                ret2.setTangent(true);
        //            }
        //        }
        ret2.push_back(vp);
    }

    return ret2;
}

/**
 * @return Intersection between two lines.
 */
RS_VectorSolutions RS_Information::getIntersectionLineLine(const RS_Entity* line1, const RS_Entity* line2) {
    if (line1 == nullptr || line2 == nullptr) {
        return {};
    }

    if (line1->rtti() != RS2::EntityLine || line2->rtti() != RS2::EntityLine) {
        return {};
    }

    RS_Vector p1 = line1->getStartpoint();
    const RS_Vector p2 = line1->getEndpoint();
    const RS_Vector p3 = line2->getStartpoint();
    const RS_Vector p4 = line2->getEndpoint();

    const double div = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);

    // parallel condition
    const double dAngle = static_cast<const RS_Line*>(line1)->getAngle1() - static_cast<const RS_Line*>(line2)->getAngle1();
    if (std::abs(div) > RS_TOLERANCE && std::abs(std::remainder(dAngle, M_PI)) >= RS_TOLERANCE_ANGLE) {
        const double num = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
        const double u = num / div;

        const double xs = p1.x + u * (p2.x - p1.x);
        const double ys = p1.y + u * (p2.y - p1.y);
        return {RS_Vector{xs, ys}};
    }

    // handle zero-length lines
    if (line1->getLength() < line2->getLength()) {
        std::swap(line1, line2);
    }
    if (line1->getLength() <= RS_TOLERANCE) {
        // both are zero length lines. Still check distance from points
        if (p1.squaredTo(p3) <= RS_TOLERANCE2) {
            return {p1};
        }
        return {};
    }
    if (line2->getLength() <= RS_TOLERANCE) {
        // one line is zero length, only consider if it's close enough to the non-zero line
        RS_Vector projection = line1->getNearestPointOnEntity(line2->getStartpoint(), true);
        if (projection.squaredTo(line2->getStartpoint()) <= RS_TOLERANCE2) {
            return {projection};
        }
    }
    // lines are parallel
    return {};
}

/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionLineArc(const RS_Entity* line, const RS_Entity* arc) {
    if (line == nullptr || arc == nullptr) {
        return {};
    }

    if (line->rtti() != RS2::EntityLine || !isArc(arc)) {
        return {};
    }

    const RS_Vector p = line->getStartpoint();
    const RS_Vector d = line->getEndpoint() - line->getStartpoint();
    const double d2 = d.squared();
    const RS_Vector c = arc->getCenter();
    const double r = arc->getRadius();
    const RS_Vector delta = p - c;
    if (d2 < RS_TOLERANCE2) {
        //line too short, still check the whether the line touches the arc
        if (std::abs(delta.squared() - r * r) < 2. * RS_TOLERANCE * r) {
            return RS_VectorSolutions({line->getMiddlePoint()});
        }
        return {};
    }

    // Arc center projection on line
    double dist = 0.;
    RS_Vector projection = line->getNearestPointOnEntity(c, false, &dist);
    RS_Vector dP = projection - c;
    dP -= d * (d.dotP(dP) / d2); // reduce rounding errors
    projection = c + dP;

    const double dr = dP.magnitude() - r;
    const double tol = 1e-5 * r;
    if (dr > tol) {
        return {};
    }

    if (dr < -tol) {
        // two solutions
        const double dt = std::sqrt(r * r - dP.squared());
        const RS_Vector dT = d * (dt / d.magnitude());
        return RS_VectorSolutions({projection + dT, projection - dT});
    }

    // Tangential
    RS_VectorSolutions ret{projection};
    ret.setTangent(true);
    //        std::cout<<"Tangential point: "<<ret<<std::endl;
    return ret;
}

/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionArcArc(const RS_Entity* arc1, const RS_Entity* arc2) {
    if (!((arc1 != nullptr) && (arc2 != nullptr))) {
        return {};
    }

    if (!isArc(arc1) || !isArc(arc2)) {
        return {};
    }

    const RS_Vector c1 = arc1->getCenter();
    const RS_Vector c2 = arc2->getCenter();

    const double r1 = arc1->getRadius();
    const double r2 = arc2->getRadius();

    const RS_Vector u = c2 - c1;

    // concentric
    if (u.magnitude() < 1.0e-7 * (r1 + r2)) {
        return {};
    }

    // perpendicular to the line passing both centers
    const auto v = RS_Vector{u.y, -u.x};

    const double r12 = r1 * r1;
    const double r22 = r2 * r2;
    // r1/|u|*cos(angle): the angle is at the arc center 1, between an intersection and the line passing arc centers
    const double s = 0.5 * ((r12 - r22) / u.squared() + 1.0);
    // term = (r12/|u|^2)*sin^2(angle)
    const double term = r12 / u.squared() - s * s;

    // no intersection:
    if (term < -RS_TOLERANCE) {
        return {};
    }

    // one or two intersections:
    const double t1 = std::sqrt(std::max(0., term));

    RS_Vector sol1 = c1 + u * s + v * t1;
    RS_Vector sol2 = c1 + u * s - v * t1;

    if (sol1.distanceTo(sol2) < 1.0e-5 * (r1 + r2)) {
        RS_VectorSolutions ret{sol1};
        ret.setTangent(true);
        return ret;
    }

    return {sol1, sol2};
}

// find intersections between ellipse/arc/circle using quartic equation solver
//
// @author Dongxu Li <dongxuli2011@gmail.com>
//

RS_VectorSolutions RS_Information::getIntersectionEllipseEllipse(const RS_Ellipse* ellipse1, const RS_Ellipse* ellipse2) {
    RS_VectorSolutions ret;

    if (!((ellipse1 != nullptr) && (ellipse2 != nullptr))) {
        return ret;
    }
    if ((ellipse1->getCenter() - ellipse2->getCenter()).squared() < RS_TOLERANCE2 && (ellipse1->getMajorP() - ellipse2->getMajorP()).squared() < RS_TOLERANCE2 &&
        fabs(ellipse1->getRatio() - ellipse2->getRatio()) < RS_TOLERANCE) {
        // overlapped ellipses, do not do overlap
        return ret;
    }
    RS_Ellipse ellipse01(nullptr, ellipse1->getData());

    RS_Ellipse* e01 = &ellipse01;
    if (e01->getMajorRadius() < e01->getMinorRadius()) {
        e01->switchMajorMinor();
    }
    RS_Ellipse ellipse02(nullptr, ellipse2->getData());
    RS_Ellipse* e02 = &ellipse02;
    if (e02->getMajorRadius() < e02->getMinorRadius()) {
        e02->switchMajorMinor();
    }
    //transform ellipse2 to ellipse1's coordinates
    RS_Vector shiftc1 = -e01->getCenter();
    double shifta1 = -e01->getAngle();
    e02->move(shiftc1);
    e02->rotate(shifta1);
    //    RS_Vector majorP2=e02->getMajorP();
    double a1 = e01->getMajorRadius();
    double b1 = e01->getMinorRadius();
    double x2 = e02->getCenter().x, y2 = e02->getCenter().y;
    double a2 = e02->getMajorRadius();
    double b2 = e02->getMinorRadius();

    if (e01->getMinorRadius() < RS_TOLERANCE || e01->getRatio() < RS_TOLERANCE) {
        // treat e01 as a line
        RS_Line line{ellipse1->getParent(), {{-a1, 0.}, {a1, 0.}}};
        ret = getIntersectionEllipseLine(&line, e02);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }
    if (e02->getMinorRadius() < RS_TOLERANCE || e02->getRatio() < RS_TOLERANCE) {
        // treat e02 as a line
        RS_Line line{ellipse1->getParent(), {{-a2, 0.}, {a2, 0.}}};
        line.rotate({0., 0.}, e02->getAngle());
        line.move(e02->getCenter());
        ret = getIntersectionEllipseLine(&line, e01);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }

    //ellipse01 equation:
    //	x^2/(a1^2) + y^2/(b1^2) - 1 =0
    double t2 = -e02->getAngle();
    //ellipse2 equation:
    // ( (x - u) cos(t) - (y - v) sin(t))^2/a^2 + ( (x - u) sin(t) + (y-v) cos(t))^2/b^2 =1
    // ( cos^2/a^2 + sin^2/b^2) x^2 +
    // ( sin^2/a^2 + cos^2/b^2) y^2 +
    //  2 sin cos (1/b^2 - 1/a^2) x y +
    //  ( ( 2 v sin cos - 2 u cos^2)/a^2 - ( 2v sin cos + 2 u sin^2)/b^2) x +
    //  ( ( 2 u sin cos - 2 v sin^2)/a^2 - ( 2u sin cos + 2 v cos^2)/b^2) y +
    //  (u cos - v sin)^2/a^2 + (u sin + v cos)^2/b^2 -1 =0
    // detect whether any ellipse radius is zero
    double cs = cos(t2), si = sin(t2);
    double ucs = x2 * cs, usi = x2 * si, vcs = y2 * cs, vsi = y2 * si;
    double cs2 = cs * cs, si2 = 1 - cs2;
    double tcssi = 2. * cs * si;
    double ia2 = 1. / (a2 * a2), ib2 = 1. / (b2 * b2);
    std::vector<double> m(0, 0.);
    m.push_back(1. / (a1 * a1)); //ma000
    m.push_back(1. / (b1 * b1)); //ma011
    m.push_back(cs2 * ia2 + si2 * ib2); //ma100
    m.push_back(cs * si * (ib2 - ia2)); //ma101
    m.push_back(si2 * ia2 + cs2 * ib2); //ma111
    m.push_back((y2 * tcssi - 2. * x2 * cs2) * ia2 - (y2 * tcssi + 2 * x2 * si2) * ib2); //mb10
    m.push_back((x2 * tcssi - 2. * y2 * si2) * ia2 - (x2 * tcssi + 2 * y2 * cs2) * ib2); //mb11
    m.push_back((ucs - vsi) * (ucs - vsi) * ia2 + (usi + vcs) * (usi + vcs) * ib2 - 1.); //mc1
    auto vs0 = RS_Math::simultaneousQuadraticSolver(m);
    shifta1 = -shifta1;
    shiftc1 = -shiftc1;
    for (RS_Vector vp : vs0) {
        vp.rotate(shifta1);
        vp.move(shiftc1);
        ret.push_back(vp);
    }
    return ret;
}

//wrapper to do Circle-Ellipse and Arc-Ellipse using Ellipse-Ellipse intersection
RS_VectorSolutions RS_Information::getIntersectionCircleEllipse(const RS_Circle* circle, const RS_Ellipse* ellipse) {
    if (circle == nullptr || ellipse == nullptr) {
        return {};
    }

    const RS_Ellipse e2{circle->getParent(), {circle->getCenter(), {circle->getRadius(), 0.}, 1.0, 0., 2. * M_PI, false}};
    return getIntersectionEllipseEllipse(ellipse, &e2);
}

RS_VectorSolutions RS_Information::getIntersectionArcEllipse(const RS_Arc* arc, const RS_Ellipse* ellipse) {
    if (arc == nullptr || ellipse == nullptr) {
        return {};
    }
    const RS_Ellipse e2{arc->getParent(), {arc->getCenter(), {arc->getRadius(), 0.}, 1.0, arc->getAngle1(), arc->getAngle2(), arc->isReversed()}};
    return getIntersectionEllipseEllipse(ellipse, &e2);
}

/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionEllipseLine(const RS_Line* line, const RS_Ellipse* ellipse) {
    RS_VectorSolutions ret;

    if (!((line != nullptr) && (ellipse != nullptr))) {
        return ret;
    }
    // rotate into normal position:

    const double rx = ellipse->getMajorRadius();
    if (rx < RS_TOLERANCE) {
        //zero radius ellipse
        const RS_Vector vp(line->getNearestPointOnEntity(ellipse->getCenter(), true));
        if ((vp - ellipse->getCenter()).squared() < RS_TOLERANCE2) {
            //center on line
            ret.push_back(vp);
        }
        return ret;
    }
    const RS_Vector factor(1. / rx, -1. / rx);
    RS_Vector angleVector = ellipse->getMajorP().scaled(factor);
    const double ry = rx * ellipse->getRatio();
    const RS_Vector center = ellipse->getCenter();
    const RS_Vector a1 = line->getStartpoint().rotate(center, angleVector);
    const RS_Vector a2 = line->getEndpoint().rotate(center, angleVector);
    //    RS_Vector origin = a1;
    const RS_Vector dir = a2 - a1;
    const RS_Vector diff = a1 - center;
    const auto mDir = RS_Vector(dir.x / (rx * rx), dir.y / (ry * ry));
    const auto mDiff = RS_Vector(diff.x / (rx * rx), diff.y / (ry * ry));

    const double a = RS_Vector::dotP(dir, mDir);
    const double b = RS_Vector::dotP(dir, mDiff);
    const double c = RS_Vector::dotP(diff, mDiff) - 1.0;
    double d = b * b - a * c;

    //    std::cout<<"RS_Information::getIntersectionEllipseLine(): d="<<d<<std::endl;
    if (d < -1.e3 * RS_TOLERANCE * sqrt(RS_TOLERANCE)) {
        RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 0");
        return ret;
    }
    if (d < 0.) {
        d = 0.;
    }
    const double root = sqrt(d);
    const double t_a = -b / a;
    const double t_b = root / a;
    //        double t_b = (-b + root) / a;

    ret.push_back(a1.lerp(a2, t_a + t_b));
    const RS_Vector vp(a1.lerp(a2, t_a - t_b));
    if ((ret.get(0) - vp).squared() > RS_TOLERANCE2) {
        ret.push_back(vp);
    }
    angleVector.y *= -1.;
    ret.rotate(center, angleVector);
    //    std::cout<<"found Ellipse-Line intersections: "<<ret.getNumber()<<std::endl;
    //    std::cout<<ret<<std::endl;
    RS_DEBUG->print("RS_Information::getIntersectionEllipseLine(): done");
    return ret;
}

/**
 * Checks if the given coordinate is inside the given contour.
 *
 * @param point Coordinate to check.
 * @param contour One or more entities which shape a contour.
 *         If the given contour is not closed, the result is undefined.
 *         The entities don't need to be in a specific order.
 * @param onContour Will be set to true if the given point it exactly
 *         on the contour.
 */
/**
 * Checks if the given coordinate is inside the given contour.
 *
 * @param point Coordinate to check.
 * @param contour One or more entities which shape a contour.
 * If the given contour is not closed, the result is undefined.
 * The entities don't need to be in a specific order.
 * @param onContour Will be set to true if the given point it exactly
 * on the contour.
 */
/**
 * Checks if the given coordinate is inside the given contour.
 *
 * @param point Coordinate to check.
 * @param contour One or more entities which shape a contour.
 * If the given contour is not closed, the result is undefined.
 * The entities don't need to be in a specific order.
 * @param onContour Will be set to true if the given point it exactly
 * on the contour.
 */
/**
 * Checks if the given coordinate is inside the given contour.
 *
 * @param point Coordinate to check.
 * @param contour One or more entities which shape a contour.
 * If the given contour is not closed, the result is undefined.
 * The entities don't need to be in a specific order.
 * @param onContour Will be set to true if the given point it exactly
 * on the contour.
 */
bool RS_Information::isPointInsideContour(const RS_Vector& point, const RS_EntityContainer* contour, bool* onContour) {
    if (contour == nullptr || !LC_Rect{contour->getMin(), contour->getMax()}.inArea(point)) {
        return false;
    }

    constexpr double onTol = 1.0e-4; // fixme - sand - is it candidate for setting?
    for (const RS_Entity* e : lc::LC_ContainerTraverser{*contour, RS2::ResolveAll}.entities()) {
        if (e->isPointOnEntity(point, onTol)) {
            if (onContour != nullptr) {
                *onContour = true;
            }
            return true;
        }
    }

    const double width = contour->getSize().magnitude() + 1.0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 2.0 * M_PI);

    auto intersections = [&] {
        const RS_Vector vector = RS_Vector::polar(width, dist(gen));
        const RS_Line ray{point, point + vector};
        size_t counter = 0;
        for (const RS_Entity* en : lc::LC_ContainerTraverser{*contour, RS2::ResolveAll}.entities()) {
            RS_VectorSolutions sol = getIntersection(en, &ray, true);
            if (sol.isTangent()) {
                counter += sol.size() + 1;
            }
            else {
                counter += sol.size();
            }
        }
        return counter;
    };

    // use two random rays to find the minimum intersections
    const size_t counter = std::min(intersections(), intersections());

    return counter % 2 == 1;
}

RS_VectorSolutions RS_Information::createQuadrilateral(const RS_EntityContainer& container) {
    RS_VectorSolutions ret;
    static constexpr int QUAD_SIZE = 4;
    if (container.count() != QUAD_SIZE) {
        return ret;
    }
    RS_EntityContainer c(container);
    std::vector<RS_Line*> lines;
    for (const auto e : c) {
        if (e->rtti() != RS2::EntityLine) {
            return ret;
        }
        lines.push_back(static_cast<RS_Line*>(e));
    }
    if (lines.size() != QUAD_SIZE) {
        return ret;
    }

    //find intersections
    std::vector<RS_Vector> vertices;
    for (auto it = lines.begin() + 1; it != lines.end(); ++it) {
        for (auto jt = lines.begin(); jt != it; ++jt) {
            const RS_VectorSolutions& sol = RS_Information::getIntersectionLineLine(*it, *jt);
            if (sol.size() != 0u) {
                vertices.push_back(sol.at(0));
            }
        }
    }

    //	std::cout<<"vertices.size()="<<vertices.size()<<std::endl;

    switch (vertices.size()) {
        default:
            return ret;
        case QUAD_SIZE:
            break;
        case 5:
        case 6:
            for (const RS_Line* pl : lines) {
                const double a0 = pl->getDirection1();
                std::vector<std::vector<RS_Vector>::iterator> left;
                std::vector<std::vector<RS_Vector>::iterator> right;
                for (auto it = vertices.begin(); it != vertices.end(); ++it) {
                    const RS_Vector& dir = *it - pl->getNearestPointOnEntity(*it, false);
                    if (dir.squared() < RS_TOLERANCE15) {
                        continue;
                    }
                    //				std::cout<<"angle="<<remainder(dir.angle() - a0, 2.*M_PI)<<std::endl;
                    if (remainder(dir.angle() - a0, 2. * M_PI) > 0.) {
                        left.push_back(it);
                    }
                    else {
                        right.push_back(it);
                    }

                    if (left.size() == 2 && right.size() == 1) {
                        vertices.erase(right[0]);
                        break;
                    }
                    if (left.size() == 1 && right.size() == 2) {
                        vertices.erase(left[0]);
                        break;
                    }
                }
                if (vertices.size() == QUAD_SIZE) {
                    break;
                }
            }
            break;
    }

    //order vertices
    RS_Vector center{0., 0.};
    for (const RS_Vector& vp : vertices) {
        center += vp;
    }
    center *= 0.25;
    std::sort(vertices.begin(), vertices.end(), [&center](const RS_Vector& a, const RS_Vector& b)-> bool {
        return center.angleTo(a) < center.angleTo(b);
    });
    for (const RS_Vector& vp : vertices) {
        ret.push_back(vp);
        //		std::cout<<"vp="<<vp<<std::endl;
    }
    return ret;
}
