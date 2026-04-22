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

#include "rs_creation.h"

#include "lc_quadratic.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_constructionline.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_selection.h"

// fixme - sand - refactor - review this class completely
namespace {
    /**
     * @brief isArc whether the entity supports tangent
     * @param entity
     * @return  true if the entity can have tangent (RS_Line is not considered as an arc here).
     */
    bool isArc(const RS_Entity& entity) {
        if (entity.isArc()) {
            return true;
        }
        switch (entity.rtti()) {
            case RS2::EntityCircle: // fixme - merge - sand - review why
            case RS2::EntityEllipse: // fixme - merge - sand - review why
            case RS2::EntityHyperbola:
            case RS2::EntityParabola:
                return true;
            default:
                return false;
        }
    }

    /**
         * @brief fromLineCoordinate create a line from line coordinates (x, y, 1)
         * @param line the line coordinates, (x, y, 1), the last coordinate is always one, and the z-component of the input
         * vector is not used
         * @return the same line in RS_LineData
         */
    RS_LineData fromLineCoordinate(const RS_Vector& line) {
        const RS_Vector start = RS_Vector{line}.rotate(-M_PI / 4.);
        const RS_Vector stop = RS_Vector{line}.rotate(M_PI / 4.);
        return {start / (-start.dotP(line)), stop / (-stop.dotP(line))};
    }

    // tangent condition tolerance
    // two circles are considered tangent, if the distance is within this factor of the radii
    constexpr double TANGENT_TOLERANCE_FACTOR = 1e-6; // fixme - sand - options candidate?


}


/**
 * Creates an entity parallel to the given entity e through the given
 * 'coord'.
 *
 * @param coord Coordinate to define the distance / side (typically a
 *              mouse coordinate).
 * @param number Number of parallels.
 * @param e Original entity.
 * @param symmetric
 * @param createdEntities
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
void RS_Creation::createParallelThrough(const RS_Vector& coord, const int number, RS_Entity* e, const bool symmetric,
                                        bool distributeWithin,
                                        QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    double dist = 0.;
    if (e->rtti() == RS2::EntityLine) {
        const auto* l = static_cast<RS_Line*>(e);
        const RS_ConstructionLine cl(nullptr, RS_ConstructionLineData(l->getStartpoint(), l->getEndpoint()));
        dist = cl.getDistanceToPoint(coord);
    }
    else {
        dist = e->getDistanceToPoint(coord);
    }

    if (distributeWithin) {
        dist = dist / number;
    }

    if (dist < RS_MAXDOUBLE) {
        return createParallel(coord, dist, number, e, symmetric,  createdEntities);
    }
}

/**
 * Creates an entity parallel to the given entity e.
 * Out of the 2 possible parallels, the one closest to
 * the given coordinate is returned.
 * Lines, Arcs and Circles can have parallels.
 *
 * @param coord Coordinate to define which parallel we want (typically a
 *              mouse coordinate).
 * @param distance Distance of the parallel.
 * @param number Number of parallels.
 * @param e Original entity.
 * @param symmetric
 * @param createdEntities
 *
 */
void RS_Creation::createParallel(const RS_Vector& coord, const double distance, const int number, RS_Entity* e, const bool symmetric,
                                 QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    switch (e->rtti()) {
        case RS2::EntityLine:
            createParallelLine(coord, distance, number, static_cast<RS_Line*>(e), symmetric, createdEntities);
            break;
        case RS2::EntityArc:
            createParallelArc(coord, distance, number, static_cast<RS_Arc*>(e), createdEntities);
            break;
        case RS2::EntityCircle:
            createParallelCircle(coord, distance, number, static_cast<RS_Circle*>(e), createdEntities);
            break;
        case RS2::EntityParabola:
        case RS2::EntitySplinePoints:
            createParallelSplinePoints(coord, distance, number, static_cast<LC_SplinePoints*>(e), createdEntities);
            break;
        default:
            break;
    }
}

/**
 * Creates a line parallel to the given line e.
 * Out of the 2 possible parallels, the one closest to
 * the given coordinate is returned.
 *
 * @param coord Coordinate to define which parallel we want (typically a
 *              mouse coordinate).
 * @param distance Distance of the parallel.
 * @param number Number of parallels.
 * @param e Original entity.
 * @param symmetric
 * @param createdEntities
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
void RS_Creation::createParallelLine(const RS_Vector& coord, double distance, int number, const RS_Line* e, bool symmetric,
                                     QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    double ang = e->getAngle1() + M_PI_2;
    RS_LineData parallelData;

    for (int num = 1; num <= number; ++num) {
        // calculate 1st parallel:
        RS_Vector p1 = RS_Vector::polar(distance * num, ang);
        p1 += e->getStartpoint();
        RS_Vector p2 = RS_Vector::polar(distance * num, ang);
        p2 += e->getEndpoint();
        RS_Line parallel1{p1, p2};

        // calculate 2nd parallel:
        p1.setPolar(distance * num, ang + M_PI);
        p1 += e->getStartpoint();
        p2.setPolar(distance * num, ang + M_PI);
        p2 += e->getEndpoint();
        RS_Line parallel2{p1, p2};

        double dist1 = parallel1.getDistanceToPoint(coord);
        double dist2 = parallel2.getDistanceToPoint(coord);
        double minDist = std::min(dist1, dist2);

        if (minDist < RS_MAXDOUBLE) {
            RS_LineData dataToSkip;
            if (dist1 < dist2) {
                parallelData = parallel1.getData();
                dataToSkip = parallel2.getData();
            }
            else {
                parallelData = parallel2.getData();
                dataToSkip = parallel1.getData();
            }

            auto newLine = new RS_Line{nullptr, parallelData};
            createdEntities.push_back(newLine);

            if (symmetric) {
                auto symmetricLine = new RS_Line{nullptr, dataToSkip};
                createdEntities.push_back(symmetricLine);
            }
        }
    }
}

/**
 * Creates a arc parallel to the given arc e.
 * Out of the 2 possible parallels, the one closest to
 * the given coordinate is returned.
 *
 * @param coord Coordinate to define which parallel we want (typically a
 *              mouse coordinate).
 * @param distance Distance of the parallel.
 * @param number Number of parallels.
 * @param e Original entity.
 * @param createdEntities
 *
 */
void RS_Creation::createParallelArc(const RS_Vector& coord, double distance, const int number, RS_Arc* e, QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    RS_ArcData parallelData{};
    const bool inside = e->getCenter().distanceTo(coord) < e->getRadius();

    if (inside) {
        distance *= -1;
    }

    for (int num = 1; num <= number; ++num) {
        // calculate parallel:
        bool ok = true;
        RS_Arc parallel1(nullptr, e->getData());
        parallel1.setRadius(e->getRadius() + (distance * num));
        if (parallel1.getRadius() < 0.0) {
            parallel1.setRadius(RS_MAXDOUBLE);
            ok = false;
        }

        // calculate 2nd parallel:
        //RS_Arc parallel2(nullptr, e->getData());
        //parallel2.setRadius(e->getRadius()+distance*num);

        //double dist1 = parallel1.getDistanceToPoint(coord);
        //double dist2 = parallel2.getDistanceToPoint(coord);
        //double minDist = min(dist1, dist2);

        //if (minDist<RS_MAXDOUBLE) {
        if (ok) {
            //if (dist1<dist2) {
            parallelData = parallel1.getData();
            //} else {
            //    parallelData = parallel2.getData();
            //}

            auto* newArc = new RS_Arc(nullptr, parallelData);
            createdEntities.push_back(newArc);
        }
    }
}

/**
 * Creates a circle parallel to the given circle e.
 * Out of the 2 possible parallels, the one closest to
 * the given coordinate is returned.
 *
 * @param coord Coordinate to define which parallel we want (typically a
 *              mouse coordinate).
 * @param distance Distance of the parallel.
 * @param number Number of parallels.
 * @param e Original entity.
 * @param createdEntities
 *
 */
void RS_Creation::createParallelCircle(const RS_Vector& coord, double distance, const int number, const RS_Circle* e,
                                       QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    RS_CircleData parallelData{};

    const bool inside = (e->getCenter().distanceTo(coord) < e->getRadius());
    if (inside) {
        distance *= -1;
    }

    for (int num = 1; num <= number; ++num) {
        // calculate parallel:
        bool ok = true;
        RS_Circle parallel1(nullptr, e->getData());
        parallel1.setRadius(e->getRadius() + (distance * num));
        if (parallel1.getRadius() < 0.0) {
            parallel1.setRadius(RS_MAXDOUBLE);
            ok = false;
        }

        // calculate 2nd parallel:
        //RS_Circle parallel2(nullptr, e->getData());
        //parallel2.setRadius(e->getRadius()+distance*num);

        //double dist1 = parallel1.getDistanceToPoint(coord);
        //double dist2 = parallel2.getDistanceToPoint(coord);
        //double minDist = min(dist1, dist2);

        //if (minDist<RS_MAXDOUBLE) {
        if (ok) {
            //if (dist1<dist2) {
            parallelData = parallel1.getData();
            //} else {
            //    parallelData = parallel2.getData();
            //}

            const auto newCircle = new RS_Circle(nullptr, parallelData);
            createdEntities.push_back(newCircle);
        }
    }
}

/**
 * Creates a spline pseudo-parallel to the given circle e.
 * Out of the 2 possible parallels, the one closest to
 * the given coordinate is returned.
 *
 * @param coord Coordinate to define which parallel we want (typically a
 *              mouse coordinate).
 * @param distance Distance of the parallel.
 * @param number Number of parallels.
 * @param e Original entity.
 * @param createdEntities
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
void RS_Creation::createParallelSplinePoints(const RS_Vector& coord, const double distance, const int number, const LC_SplinePoints* e,
                                             QList<RS_Entity*>& createdEntities) {
    Q_ASSERT(e != nullptr);

    LC_SplinePoints* psp = nullptr;
    for (int i = 1; i <= number; ++i) {
        psp = static_cast<LC_SplinePoints*>(e->clone());
        psp->offset(coord, i * distance);
        psp->setParent(nullptr);

        createdEntities.push_back(psp);
    }
}

/**
 * Creates a bisecting line of the angle between the entities
 * e1 and e2. Out of the 4 possible bisectors, the one closest to
 * the given coordinate is returned.
 *
 * @param coord1
 * @param coord2
 * @param length Length of the bisecting line.
 * @param num Number of bisectors
 * @param l1 First line.
 * @param l2 Second line.
 * @param createdLines
 *
 * @return Pointer to the first bisector created or nullptr if no bisectors
 *   were created.
 */
bool RS_Creation::createBisector(const RS_Vector& coord1, const RS_Vector& coord2, const double length, const int num, const RS_Line* l1,
                                 const RS_Line* l2, QList<RS_Entity*>& createdLines) {
    // check given entities:
    if (l1 == nullptr || l2 == nullptr) {
        return false;
    }
    if (l1->rtti() != RS2::EntityLine || l2->rtti() != RS2::EntityLine) {
        return false;
    }

    // intersection between entities:
    const RS_VectorSolutions& sol = RS_Information::getIntersection(l1, l2, false);
    const RS_Vector inters = sol.get(0);
    if (!inters.valid) {
        return false;
    }

    const double angle1 = inters.angleTo(l1->getNearestPointOnEntity(coord1));
    const double angle2 = inters.angleTo(l2->getNearestPointOnEntity(coord2));
    double angleDiff = RS_Math::getAngleDifference(angle1, angle2);
    if (angleDiff > M_PI) {
        angleDiff = angleDiff - (2. * M_PI);
    }
    for (int n = 1; n <= num; ++n) {
        const double angle = angle1 + (angleDiff / (num + 1) * n);
        const RS_Vector& v = RS_Vector::polar(length, angle);
        auto* newLine = new RS_Line{nullptr, inters, inters + v};
        createdLines.push_back(newLine);
    }
    return true;
}

/**
 * create a tangent line which is orthogonal to the given RS_Line(normal)
 * @coord, the tangent line closest to this point
 * @normal, the line orthogonal to the tangent line
 * @circle, arc/circle/ellipse for tangent line
 *
 * Author: Dongxu Li
 */
std::unique_ptr<RS_Line> RS_Creation::createLineOrthTan(const RS_Vector& coord, const RS_Line* normal, RS_Entity* circle,
                                                        RS_Vector& alternativeTangent) {
    // check given entities:
    if (!(circle != nullptr && normal != nullptr)) {
        return {};
    }

    // Allow construction of a tangent normal to a given line using getTangentDirection()
    const RS_Vector& tangent = circle->getNearestOrthTan(coord,*normal,false);
    if (tangent.valid) {
      double tangentDirection = circle->getTangentDirection(tangent).angle();
      double normalDirection = normal->getDirection1();
      if (RS_Math::equal(RS_Math::correctAngle0ToPi(tangentDirection - normalDirection), M_PI/2, RS_TOLERANCE_ANGLE)) {
          RS_Vector linePoint = normal->getNearestPointOnEntity(tangent, false);
        return std::make_unique<RS_Line>(nullptr, RS_LineData{linePoint, tangent});
      }
    }

    RS2::EntityType rtti = circle->rtti();
    if (!(circle->isArc())) {
        return {};
    }

    RS_Vector  tangentPoint0;
    RS_Vector  t1;
    // todo - potentially, it's possible to move this fragment to appropriate implementations of  getNearestOrthTan - and expand it for returning all tangent points instead of nearest one
    switch (rtti){
        case RS2::EntityCircle: {
            auto* cir = static_cast<RS_Circle*>(circle);
            const RS_Vector& center = cir->getCenter();
            double radius = cir->getRadius();
            RS_Vector vp0(coord - center);
            RS_Vector vp1(normal->getAngle1());
            double d = RS_Vector::dotP(vp0, vp1);
            const RS_Vector& ortVector = vp1 * radius;
            if (d >= 0.) {
                tangentPoint0 = center + ortVector;
                t1 = center - ortVector;
            }
            else {
                tangentPoint0 = center - ortVector;
                t1 = center + ortVector;
            }
            break;
        }
        case RS2::EntityArc: {
            auto* cir = static_cast<RS_Arc*>(circle);
            double angle = normal->getAngle1();
            double radius = cir->getRadius();
            RS_Vector vp = RS_Vector::polar(radius, angle);
            std::vector<RS_Vector> sol;
            bool onEntity = false;
            for (int i = 0; i <= 1; i++) {
                if (!onEntity || RS_Math::isAngleBetween(angle, cir->getAngle1(), cir->getAngle2(), cir->isReversed())) {
                    if (i != 0) {
                        sol.push_back(-vp);
                    }
                    else {
                        sol.push_back(vp);
                    }
                }
                angle = RS_Math::correctAngle(angle + M_PI);
            }
            const RS_Vector& center = cir->getCenter();
            switch (sol.size()) {
                case 0:
                    // tangentPoint0 = RS_Vector(false);
                    [[fallthrough]];
                case 2:
                    if (RS_Vector::dotP(sol[1], coord - center) > 0.) {
                        tangentPoint0 = center + sol[1];
                        t1 = center + sol[0];
                        break;
                    }
                    [[fallthrough]];
                default:
                    tangentPoint0 = center + sol[0];
                    t1 = center + sol[1];
                    break;
            }
            break;
        }
        case RS2::EntityEllipse: {
            auto* cir = static_cast<RS_Ellipse*>(circle);
            //scale to ellipse angle
            RS_Vector aV(-cir->getAngle());
            RS_Vector direction = normal->getEndpoint() - normal->getStartpoint();
            direction.rotate(aV);
            double ratio = cir->getRatio();
            double angle = direction.scale(RS_Vector(1., ratio)).angle();
            double ra(cir->getMajorRadius());
            direction.set(ra * cos(angle), ratio * ra * sin(angle)); //relative to center
            std::vector<RS_Vector> sol;
            bool onEntity = false; // fixme - always false? what for this flag?
            for (int i = 0; i < 2; i++) {
                if (!onEntity || RS_Math::isAngleBetween(angle, cir->getAngle1(), cir->getAngle2(), cir->isReversed())) {
                    if (i != 0) {
                        sol.push_back(-direction);
                    }
                    else {
                        sol.push_back(direction);
                    }
                }
                angle = RS_Math::correctAngle(angle + M_PI);
            }
            if (sol.empty()) {
                tangentPoint0 = RS_Vector(false);
            }
            else {
                aV.y *= -1.;
                for (auto& v : sol) {
                    v.rotate(aV);
                }
                const RS_Vector& center = cir->getCenter();
                switch (sol.size()) {
                    case 0:
                        // tangentPoint0 = RS_Vector(false);
                        [[fallthrough]];
                    case 2:
                        if (RS_Vector::dotP(sol[1], coord - center) > 0.) {
                            tangentPoint0 = center + sol[1];
                            t1 = center + sol[0];
                            break;
                        }
                        [[fallthrough]];
                    default:
                        tangentPoint0 = center + sol[0];
                        t1 = center + sol[1];
                        break;
                }
            }
            break;
        }
        default:
            break;
    }

    alternativeTangent = t1;

    if (!tangentPoint0.valid) {
        return {};
    }
    const RS_Vector& vp = normal->getNearestPointOnEntity(tangentPoint0, false);
    return std::make_unique<RS_Line>(nullptr, vp, tangentPoint0);
}

/**
* Creates a tangent between a given point and a circle or arc.
* Out of the 2 possible tangents, the one closest to
* the given coordinate is returned.
*
* @param coord Coordinate to define which tangent we want (typically a
*              mouse coordinate).
* @param point Point.
* @param circle Circle, arc or ellipse entity.
* @param tangentPoint
* @param altTangentPoint
*/
RS_Line* RS_Creation::createTangent1(const RS_Vector& coord, const RS_Vector& point, const RS_Entity* circle, RS_Vector& tangentPoint,
                                     RS_Vector& altTangentPoint) {
    RS_Line* ret = nullptr;
    // check given entities:
    if (!(circle != nullptr && point.valid && circle->isAtomic())) {
        return nullptr;
    }

    if (isArc(*circle)) {
      // Find tangent lines through dual curves

      // the dual line of the given point
      const LC_Quadratic dualLine{{point.x, point.y, 1.}};
      const LC_Quadratic dualCircle = circle->getQuadratic().getDualCurve();
      // tangent lines by intersection of dual curves
      RS_VectorSolutions dualSol = LC_Quadratic::getIntersection(dualLine, dualCircle);

      if (dualSol.empty()) {
          return nullptr;
      }

      // distance to coord
      auto distCoord = [&coord](const RS_Vector& uv) {
        return std::abs(uv.dotP(coord) + 1.);
      };

      if (dualSol.getNumber() == 2 && distCoord(dualSol[1]) < distCoord(dualSol[0])) {
          std::swap(dualSol[0], dualSol[1]);
      }
      if (dualSol.getNumber() == 2) {
        altTangentPoint = circle->dualLineTangentPoint(dualSol[1]);
      }
      tangentPoint = circle->dualLineTangentPoint(dualSol[0]);

      // in case the point is on circle
      if (tangentPoint == point) {
        tangentPoint.move(dualSol[0].rotate(M_PI/2.) * circle->getLength());
      }

      ret = new RS_Line{nullptr, {point, tangentPoint}};
      return ret;
    }

    // the two tangent points:
    RS_VectorSolutions sol = circle->getTangentPoint(point);

    // second port of if condition means that there is no solution - yet some entities (like circle) put coordinate...
    if (sol.isEmpty() || (sol.getNumber() == 1 && sol[0] == point)) {
        return nullptr;
    }
    const RS_Vector vp2{sol.getClosest(coord)};
    RS_LineData d{};
    if ((vp2 - point).squared() > RS_TOLERANCE2) {
        d = {vp2, point};
    }
    else {
        //the given point is a tangential point
        d = {point + circle->getTangentDirection(point), point};
    }

    tangentPoint = d.startpoint;
    if (sol.getNumber() == 1) {
        // spline by points support...
        altTangentPoint = sol[0];
    }
    else {
        altTangentPoint = (sol[0] == vp2) ? sol[1] : sol[0];
    }
    ret = new RS_Line{nullptr, d};
    return ret;
}

/**
* Creates a tangent between two circles or arcs.
* Out of the 4 possible tangents, the one closest to
* the given coordinate is returned.
*
* @param circle1 1st circle or arc entity.
* @param circle2 2nd circle or arc entity.
*/
std::vector<std::unique_ptr<RS_Line>> RS_Creation::createTangent2(const RS_Entity* circle1, const RS_Entity* circle2) {
    // check given entities:
    if (circle1 == nullptr || circle2 == nullptr) {
        return {};
    }

    if (!(isArc(*circle1) && isArc(*circle2))) {
        return {};
    }

    // Find common tangent lines in line coordinates, i.e. using dual curves
    // A common tangent line is an intersection of the dual curves
    auto sol = LC_Quadratic::getIntersection(circle1->getQuadratic().getDualCurve(), circle2->getQuadratic().getDualCurve());
    if (sol.empty()) {
        return {};
    }
    RS_VectorSolutions sol1;
    for (const auto& vp : sol) {
        if (!vp.valid) {
            continue;
        }
        if (sol1.empty()) {
            sol1.push_back(vp);
        }
        else if (sol1.getClosestDistance(vp) > RS_TOLERANCE_ANGLE) {
            sol1.push_back(vp);
        }
    }

    // verify the tangent lines
    std::vector<std::unique_ptr<RS_Line>> tangents;
    std::transform(sol1.begin(), sol1.end(), std::back_inserter(tangents),
                   [circle1, circle2](const RS_Vector& line) -> std::unique_ptr<RS_Line> {
        auto rsLine = std::make_unique<RS_Line>(nullptr, fromLineCoordinate(line));
        rsLine->setStartpoint(circle1->dualLineTangentPoint(line));
        rsLine->setEndpoint(circle2->dualLineTangentPoint(line));
        if (!(rsLine->getStartpoint().isValid() && rsLine->getEndpoint().isValid())) {
            return nullptr;
        }
        return std::unique_ptr<RS_Line>(std::move(rsLine));
    });
    // cleanup invalid lines
    tangents.erase(remove_if(tangents.begin(), tangents.end(), [](const std::unique_ptr<RS_Line>& line) {
        return line == nullptr;
    }), tangents.end());

    if (tangents.empty()) {
        return {};
    }
    return tangents;
}

/**
     * Creates a line with a relative angle to the given entity.
     *
     * @param coord Coordinate to define the point where the line should end.
     *              (typically a mouse coordinate).
     * @param entity Pointer to basis entity. The angle is relative to the
     *               angle of this entity.
     * @param angle Angle of the line relative to the angle of the basis entity.
     * @param length Length of the line we're creating.
     */
RS_Line* RS_Creation::createLineRelAngle(const RS_Vector& coord, const RS_Entity* entity, const double angle, const double length) {
    // check given entity / coord:
    if (!((entity != nullptr) && coord)) {
        return {};
    }

    switch (entity->rtti()) {
        case RS2::EntityArc:
        case RS2::EntityCircle:
        case RS2::EntityLine:
        case RS2::EntityEllipse:
            break;
        default:
            return {};
    }

    const RS_Vector vp = entity->getNearestPointOnEntity(coord, false);
    const double a1 = angle + entity->getTangentDirection(vp).angle();
    const RS_Vector v1 = RS_Vector::polar(length, a1);
    const auto ret = new RS_Line(nullptr, coord, coord + v1);
    return ret;
}

RS_Block* RS_Creation::createBlock(const RS_BlockData* data, const RS_Vector& referencePoint, const QList<RS_Entity*>& selectedEntities) {
    // start undo cycle for the container if we're deleting the existing entities
    auto* block = new RS_Block(nullptr, RS_BlockData(*data));

    // copy entities into a block
    for (const auto e : selectedEntities) {
        if (e != nullptr && e->isAlive()) {
            // add entity to block:
            RS_Entity* clone = e->clone();
            clone->clearSelectionFlag();
            clone->move(-referencePoint);
            block->addEntity(clone);
        }
    }
    return block;
}
