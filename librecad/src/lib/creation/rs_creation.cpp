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

#include <QString>
#include <QFileInfo>

#include "lc_hyperbola.h"
#include "lc_parabola.h"
#include "lc_quadratic.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_constructionline.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_image.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_units.h"

namespace {

/**
 * @brief isArc whether the entity supports tangent
 * @param entity
 * @return  true if the entity can have tangent (RS_Line is not considered as an arc here).
 */
bool isArc(const RS_Entity& entity){
    if (entity.isArc())
        return true;
    switch (entity.rtti()){
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
RS_LineData fromLineCoordinate(const RS_Vector& line)
{
    RS_Vector start = RS_Vector{line}.rotate(-M_PI/4.);
    RS_Vector stop = RS_Vector{line}.rotate(M_PI/4.);
    return {start/(-start.dotP(line)), stop/(-stop.dotP(line))};
}

}

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Creation::RS_Creation(RS_EntityContainer* container,
                         RS_GraphicView* graphicView,
                         bool handleUndo):
    container(container)
  ,graphic(container?container->getGraphic():nullptr)
  ,document(container?container->getDocument():nullptr)
  ,graphicView(graphicView)
  ,handleUndo(handleUndo)
{
}

/**
 * Creates an entity parallel to the given entity e through the given
 * 'coord'.
 *
 * @param coord Coordinate to define the distance / side (typically a
 *              mouse coordinate).
 * @param number Number of parallels.
 * @param e Original entity.
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
RS_Entity* RS_Creation::createParallelThrough(const RS_Vector& coord,
                                              int number,
                                              RS_Entity* e,
                                              bool symmetric) {
    if (!e) {
        return nullptr;
    }

    double dist;

    if (e->rtti()==RS2::EntityLine) {
        auto* l = (RS_Line*)e;
        RS_ConstructionLine cl(nullptr,
                               RS_ConstructionLineData(l->getStartpoint(),
                                                       l->getEndpoint()));
        dist = cl.getDistanceToPoint(coord);
    } else {
        dist = e->getDistanceToPoint(coord);
    }

    if (dist<RS_MAXDOUBLE) {
        return createParallel(coord, dist, number, e, symmetric);
    } else {
        return nullptr;
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
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
RS_Entity* RS_Creation::createParallel(const RS_Vector& coord,
                                       double distance, int number,
                                       RS_Entity* e, bool symmetric) {
    if (!e) {
        return nullptr;
    }

    switch (e->rtti()) {
    case RS2::EntityLine:
        return createParallelLine(coord, distance, number, (RS_Line*)e, symmetric);
        break;

    case RS2::EntityArc:
        return createParallelArc(coord, distance, number, (RS_Arc*)e);
        break;

    case RS2::EntityCircle:
        return createParallelCircle(coord, distance, number, (RS_Circle*)e);
        break;

    case RS2::EntityParabola:
    case RS2::EntitySplinePoints:
        return createParallelSplinePoints(coord, distance, number, (LC_SplinePoints*)e);
        break;

    default:
        break;
    }

    return nullptr;
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
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
RS_Line* RS_Creation::createParallelLine(const RS_Vector& coord,
                                         double distance, int number,
                                         RS_Line* e,
                                         bool symmetric) {

    if (!e) {
        return nullptr;
    }

    double ang = e->getAngle1() + M_PI_2;
    RS_LineData parallelData;
    RS_Line* ret = nullptr;

    LC_UndoSection undo( document, handleUndo);
    for (int num=1; num<=number; ++num) {

        // calculate 1st parallel:
        RS_Vector p1 = RS_Vector::polar(distance*num, ang);
        p1 += e->getStartpoint();
        RS_Vector p2 = RS_Vector::polar(distance*num, ang);
        p2 += e->getEndpoint();
        RS_Line parallel1{p1, p2};

        // calculate 2nd parallel:
        p1.setPolar(distance*num, ang+M_PI);
        p1 += e->getStartpoint();
        p2.setPolar(distance*num, ang+M_PI);
        p2 += e->getEndpoint();
        RS_Line parallel2{p1, p2};

        double dist1 = parallel1.getDistanceToPoint(coord);
        double dist2 = parallel2.getDistanceToPoint(coord);
        double minDist = std::min(dist1, dist2);

        if (minDist<RS_MAXDOUBLE) {

            RS_LineData dataToSkip;
            if (dist1 < dist2){
                parallelData = parallel1.getData();
                dataToSkip = parallel2.getData();
            } else {
                parallelData = parallel2.getData();
                dataToSkip = parallel1.getData();
            }

            auto *newLine = new RS_Line{container, parallelData};
            if (!ret){
                ret = newLine;
            }
            setEntity(newLine);

            if (symmetric){
                auto *symmetricLine = new RS_Line{container, dataToSkip};
                setEntity(symmetricLine);
            }
        }
    }

    return ret;
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
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
RS_Arc* RS_Creation::createParallelArc(const RS_Vector& coord,
                                       double distance, int number,
                                       RS_Arc* e) {

    if (!e) {
        return nullptr;
    }

    RS_ArcData parallelData;
    RS_Arc* ret = nullptr;

    bool inside = (e->getCenter().distanceTo(coord) < e->getRadius());

    if (inside) {
        distance *= -1;
    }

    for (int num=1; num<=number; ++num) {

        // calculate parallel:
        bool ok = true;
        RS_Arc parallel1(nullptr, e->getData());
        parallel1.setRadius(e->getRadius() + distance*num);
        if (parallel1.getRadius()<0.0) {
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

            LC_UndoSection undo( document, handleUndo);
            auto* newArc = new RS_Arc(container, parallelData);
            if (!ret) {
                ret = newArc;
            }
            setEntity(newArc);
        }
    }

    return ret;
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
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
RS_Circle* RS_Creation::createParallelCircle(const RS_Vector& coord,
                                             double distance, int number,
                                             RS_Circle* e) {

    if (!e) {
        return nullptr;
    }

    RS_CircleData parallelData;
    RS_Circle* ret = nullptr;

    bool inside = (e->getCenter().distanceTo(coord) < e->getRadius());

    if (inside) {
        distance *= -1;
    }

    for (int num=1; num<=number; ++num) {

        // calculate parallel:
        bool ok = true;
        RS_Circle parallel1(nullptr, e->getData());
        parallel1.setRadius(e->getRadius() + distance*num);
        if (parallel1.getRadius()<0.0) {
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

            LC_UndoSection undo( document, handleUndo);
            auto newCircle = new RS_Circle(container, parallelData);
            if (!ret) {
                ret = newCircle;
            }
            setEntity(newCircle);
        }
    }
    return ret;
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
 *
 * @return Pointer to the first created parallel or nullptr if no
 *    parallel has been created.
 */
LC_SplinePoints* RS_Creation::createParallelSplinePoints(const RS_Vector& coord,
                                                         double distance, int number, LC_SplinePoints* e)
{
    if(!e) return nullptr;

    LC_SplinePoints *psp, *ret = nullptr;

    LC_UndoSection undo( document, handleUndo);
    for(int i = 1; i <= number; ++i)
    {
        psp = (LC_SplinePoints*)e->clone();
        psp->offset(coord, i*distance);

        psp->setParent(container);
        if(!ret) ret = psp;
        setEntity(psp);
    }

    return ret;
}


/**
 * Creates a bisecting line of the angle between the entities
 * e1 and e2. Out of the 4 possible bisectors, the one closest to
 * the given coordinate is returned.
 *
 * @param coord Coordinate to define which bisector we want (typically a
 *              mouse coordinate).
 * @param length Length of the bisecting line.
 * @param num Number of bisectors
 * @param l1 First line.
 * @param l2 Second line.
 *
 * @return Pointer to the first bisector created or nullptr if no bisectors
 *   were created.
 */
RS_Line* RS_Creation::createBisector(const RS_Vector& coord1,
                                     const RS_Vector& coord2,
                                     double length,
                                     int num,
                                     RS_Line* l1,
                                     RS_Line* l2) {

    // check given entities:
    if (!(l1 && l2))
        return nullptr;
    if (!(l1->rtti()==RS2::EntityLine && l2->rtti()==RS2::EntityLine))
        return nullptr;

    // intersection between entities:
    RS_VectorSolutions const& sol =
            RS_Information::getIntersection(l1, l2, false);
    RS_Vector inters = sol.get(0);
    if (!inters.valid) {
        return nullptr;
    }

    double angle1 = inters.angleTo(l1->getNearestPointOnEntity(coord1));
    double angle2 = inters.angleTo(l2->getNearestPointOnEntity(coord2));
    double angleDiff = RS_Math::getAngleDifference(angle1, angle2);
    if (angleDiff > M_PI) {
        angleDiff = angleDiff - 2.*M_PI;
    }
    RS_Line* ret = nullptr;

    LC_UndoSection undo( document, handleUndo);
    for (int n=1; n <= num; ++n) {

        double angle = angle1 +
                (angleDiff / (num+1) * n);

        RS_Vector const& v = RS_Vector::polar(length, angle);

        auto* newLine = new RS_Line{container, inters, inters + v};
        if (!ret) ret = newLine;
        setEntity(newLine);
    }

    return ret;
}

/**
 * create a tangent line which is orthogonal to the given RS_Line(normal)
 * @coord, the tangent line closest to this point
 * @normal, the line orthogonal to the tangent line
 * @circle, arc/circle/ellipse for tangent line
 *
 * Author: Dongxu Li
 */
RS_Line* RS_Creation::createLineOrthTan(const RS_Vector& coord,
                                        RS_Line* normal,
                                        RS_Entity* circle,
                                        RS_Vector& alternativeTangent) {
    RS_Line* ret = nullptr;

    // check given entities:
    if (!(circle && normal))
        return ret;
    RS2::EntityType rtti = circle->rtti();
    if (!(circle->isArc() || rtti == RS2::EntityParabola))
        return ret;
    //if( normal->getLength()<RS_TOLERANCE) return ret;//line too short
    
//    RS_Vector const& t0 = circle->getNearestOrthTan(coord,*normal,false);
    RS_Vector  t0;
    RS_Vector  t1;
    // todo - potentially, it's possible to move this fragment to appropriate implementations of  getNearestOrthTan - and expand it for returning all tangent points instead of nearest one
    switch (rtti){
        case RS2::EntityCircle: {
            auto *cir = dynamic_cast<RS_Circle *>(circle);
            const RS_Vector &center = cir->getCenter();
            double radius = cir->getRadius();
            RS_Vector vp0(coord - center);
            RS_Vector vp1(normal->getAngle1());
            double d = RS_Vector::dotP(vp0, vp1);
            const RS_Vector &ortVector = vp1 * radius;
            if (d >= 0.){
                t0 = center + ortVector;
                t1 = center - ortVector;
            } else {
                t0 = center - ortVector;
                t1 = center + ortVector;
            }
            break;
        }
        case RS2::EntityArc: {
            auto *cir = dynamic_cast<RS_Arc *>(circle);
            double angle=normal->getAngle1();
            double radius = cir->getRadius();
            RS_Vector vp = RS_Vector::polar(radius, angle);
            std::vector<RS_Vector> sol;
            bool onEntity = false;
            for(int i=0; i <= 1; i++){
                if(!onEntity || RS_Math::isAngleBetween(angle,cir->getAngle1(),cir->getAngle2(),cir->isReversed())) {
                    if (i)
                        sol.push_back(- vp);
                    else
                        sol.push_back(vp);
                }
                angle=RS_Math::correctAngle(angle+M_PI);
            }
            const RS_Vector &center = cir->getCenter();
            switch(sol.size()) {
                case 0:
                    t0 = RS_Vector(false);
                case 2:
                    if(RS_Vector::dotP(sol[1], coord - center) > 0.) {
                        t0 = center + sol[1];
                        t1 = center + sol[0];
                        break;
                    }
                    else{
                        t0 = center + sol[0];
                        t1 = center + sol[1];
                    }
                    // fall-through
                default:
                    vp=sol[0]; // hm?
                    break;
            }
            break;
        }
        case RS2::EntityEllipse: {
            auto *cir = dynamic_cast<RS_Ellipse *>(circle);            
            //scale to ellipse angle
            RS_Vector aV(-cir->getAngle());
            RS_Vector direction = normal->getEndpoint() - normal->getStartpoint();
            direction.rotate(aV);
            double ratio = cir->getRatio();
            double angle = direction.scale(RS_Vector(1., ratio)).angle();
            double ra(cir->getMajorRadius());
            direction.set(ra * cos(angle), ratio * ra * sin(angle));//relative to center
            std::vector<RS_Vector> sol;
            bool onEntity = false;
            for (int i = 0; i < 2; i++) {
                if (!onEntity ||
                    RS_Math::isAngleBetween(angle, cir->getAngle1(), cir->getAngle2(), cir->isReversed())){
                    if (i){
                        sol.push_back(-direction);
                    } else {
                        sol.push_back(direction);
                    }
                }
                angle = RS_Math::correctAngle(angle + M_PI);
            }
            if (sol.size() < 1) {
                t0 = RS_Vector(false);
            }
            else {
                aV.y *= -1.;
                for (auto &v: sol) {
                    v.rotate(aV);
                }               
                const RS_Vector &center = cir->getCenter();
                switch (sol.size()) {
                    case 0:
                        t0 = RS_Vector(false);
                    case 2:
                        if (RS_Vector::dotP(sol[1], coord - center) > 0.){
                            t0 = center + sol[1];
                            t1 = center + sol[0];
                            break;
                        }
                        // fall-through
                    default:
                        t0 = center + sol[0];
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

    if(!t0.valid) return ret;
    RS_Vector const& vp=normal->getNearestPointOnEntity(t0, false);
    LC_UndoSection undo( document, handleUndo);
    ret = new RS_Line{container, vp, t0};
    ret->setLayerToActive();
    ret->setPenToActive();
    return ret;
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
*/
RS_Line* RS_Creation::createTangent1(const RS_Vector& coord,
                                     const RS_Vector& point,
                                     RS_Entity* circle,
                                     RS_Vector& tangentPoint,
                                     RS_Vector& altTangentPoint) {
    RS_Line* ret = nullptr;
    //RS_Vector circleCenter;

    // check given entities:
    if (!(circle && point.valid)) return nullptr;
    //    if (!(circle->isArc() || circle->rtti()==RS2::EntitySplinePoints || circle->rtti() == RS2::EntityParabola)){
    // 	return nullptr;
    // }

    // the two tangent points:
    RS_VectorSolutions sol=circle->getTangentPoint(point);

    if (!sol.getNumber())
        return nullptr;
    RS_Vector const vp2{sol.getClosest(coord)};
    RS_LineData d;
    if( (vp2-point).squared() > RS_TOLERANCE2 ) {
        d={vp2, point};
    }else{//the given point is a tangential point
        d={point+circle->getTangentDirection(point), point};
    }

    tangentPoint = d.startpoint;
    altTangentPoint = (sol[0] == vp2 ) ? sol[1] : sol[0];

    // create the closest tangent:
    LC_UndoSection undo( document, handleUndo);
    ret = new RS_Line{container, d};
    setEntity(ret);

    return ret;
}

/**
* Creates a tangent between two circles or arcs.
* Out of the 4 possible tangents, the one closest to
* the given coordinate is returned.
*
* @param coord Coordinate to define which tangent we want (typically a
*              mouse coordinate).
* @param circle1 1st circle or arc entity.
* @param circle2 2nd circle or arc entity.
*/
std::vector<std::unique_ptr<RS_Line>> RS_Creation::createTangent2(
                                                     RS_Entity* circle1,
                                                     RS_Entity* circle2) {
    // check given entities:
    if(! (circle1 && circle2))
        return {};

    if( !(isArc(*circle1) && isArc(*circle2)))
        return {};

    // Find common tangent lines in line coordinates, i.e. using dual curves
    // A common tangent line is an intersection of the dual curves
    auto sol = LC_Quadratic::getIntersection(circle1->getQuadratic().getDualCurve(),
                                             circle2->getQuadratic().getDualCurve());
    if (sol.empty())
        return {};
    RS_VectorSolutions sol1;
    for (const auto& vp: sol)
    {
        if (!vp.valid)
            continue;
        if (sol1.empty())
            sol1.push_back(vp);
        else if (sol1.getClosestDistance(vp) > RS_TOLERANCE_ANGLE)
                sol1.push_back(vp);
    }

    // verify the tangent lines
    std::vector<std::unique_ptr<RS_Line>> tangents;
    std::transform(sol1.begin(), sol1.end(), std::back_inserter(tangents),
                   [circle1, circle2](const RS_Vector& line) -> std::unique_ptr<RS_Line> {
        auto rsLine = std::make_unique<RS_Line>(nullptr, fromLineCoordinate(line));
        rsLine->setStartpoint(circle1->dualLineTangentPoint(line));
        rsLine->setEndpoint(circle2->dualLineTangentPoint(line));
        return std::unique_ptr<RS_Line>(std::move(rsLine));
    });
    // cleanup invalid lines
    tangents.erase(remove_if(tangents.begin(), tangents.end(), [](const std::unique_ptr<RS_Line>& line) {
        return line == nullptr;
    }), tangents.end());
    if (tangents.empty())
        return {};
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
RS_Line* RS_Creation::createLineRelAngle(const RS_Vector& coord,
                                         RS_Entity* entity,
                                         double angle,
                                         double length) {

    // check given entity / coord:
    if (!(entity && coord))
        return nullptr;

    switch(entity->rtti()){
    default:
        return nullptr;
    case RS2::EntityArc:
    case RS2::EntityCircle:
    case RS2::EntityLine:
    case RS2::EntityEllipse:
        break;
    }

    auto const vp = entity->getNearestPointOnEntity(coord, false);

    double const a1 = angle + entity->getTangentDirection(vp).angle();

    RS_Vector const v1 = RS_Vector::polar(length, a1);

    auto* ret = new RS_Line{container, coord, coord+v1};
    setEntity(ret);

    return ret;
}


/**
     * Creates a polygon with 'number' edges.
     *
     * @param center Center of the polygon.
     * @param corner The first corner of the polygon
     * @param number Number of edges / corners.
     */
RS_Line* RS_Creation::createPolygon(const RS_Vector& center,
                                    const RS_Vector& corner,
                                    int number) {
    // check given coords / number:
    if (!center.valid || !corner.valid || number<3) {
        return nullptr;
    }

    RS_Line* ret = nullptr;

    double const r = center.distanceTo(corner);
    double const angle0 = center.angleTo(corner);
    double const da = 2.*M_PI/number;
    LC_UndoSection undo( document, handleUndo);
    for (int i=0; i < number; ++i) {
        RS_Vector const& c0 = center +
                RS_Vector::polar(r, angle0 + i*da);
        RS_Vector const& c1 = center +
                RS_Vector::polar(r, angle0 + ((i+1)%number)*da);

        auto* line = new RS_Line{container, c0, c1};
        line->setLayerToActive();
        line->setPenToActive();

        if (!ret) ret = line;

        if (container) {
            container->addEntity(line);
        }
        undo.addUndoable(line);
        if (graphicView) {
            graphicView->drawEntity(line);
        }
    }

    return ret;
}



/**
     * Creates a polygon with 'number' edges.
     *
     * @param corner1 The first corner of the polygon.
     * @param corner2 The second corner of the polygon.
     * @param number Number of edges / corners.
     */
RS_Line* RS_Creation::createPolygon2(const RS_Vector& corner1,
                                     const RS_Vector& corner2,
                                     int number) {
    // check given coords / number:
    if (!corner1.valid || !corner2.valid || number<3) {
        return nullptr;
    }

    RS_Line* ret = nullptr;

    LC_UndoSection undo( document, handleUndo);
    double const len = corner1.distanceTo(corner2);
    double const da = 2.*M_PI/number;
    double const r = 0.5*len/sin(0.5*da);
    double const angle1 = corner1.angleTo(corner2);
    RS_Vector center = (corner1 + corner2)*0.5;

    //TODO, the center or the polygon could be at left or right side
    //left is chosen here
    center += RS_Vector::polar(0.5*len/tan(0.5*da), angle1 + M_PI_2);
    double const angle0 = center.angleTo(corner1);


    for (int i=0; i<number; ++i) {
        RS_Vector const& c0 = center +
                RS_Vector::polar(r, angle0 + i*da);
        RS_Vector const& c1 = center +
                RS_Vector::polar(r, angle0 + ((i+1)%number)*da);

        auto* line = new RS_Line{container, c0, c1};
        line->setLayerToActive();
        line->setPenToActive();

        if (!ret) ret = line;

        if (container) {
            container->addEntity(line);
        }
        undo.addUndoable(line);
        if (graphicView) {
            graphicView->drawEntity(line);
        }

    }

    return ret;
}

/**
     * Creates a polygon with 'number' edges.
     *
     * @param center Center of the polygon.
     * @param tangent The first tangent of the polygon with a circle
     * @param number Number of edges / corners.
     */
RS_Line* RS_Creation::createPolygon3(const RS_Vector& center,    //added by txmy
                                     const RS_Vector& tangent,
                                     int number) {
    // check given coords / number:
    if (!center.valid || !tangent.valid || number<3) {
        return nullptr;
    }

    RS_Line* ret = nullptr;

    LC_UndoSection undo( document, handleUndo);
    RS_Vector corner(0, 0);
    double angle = 2.*M_PI/number/2.0;
    corner.x = tangent.x + (center.y - tangent.y) * tan(angle);
    corner.y = tangent.y + (tangent.x - center.x) * tan(angle);

    double const r = center.distanceTo(corner);
    double const angle0 = center.angleTo(corner);
    double const da = 2.*M_PI/number;

    for (int i=0; i < number; ++i) {
        RS_Vector const& c0 = center +
                RS_Vector::polar(r, angle0 + i*da);
        RS_Vector const& c1 = center +
                RS_Vector::polar(r, angle0 + ((i+1)%number)*da);

        auto* line = new RS_Line{container, c0, c1};
        line->setLayerToActive();
        line->setPenToActive();

        if (!ret) ret = line;

        if (container) {
            container->addEntity(line);
        }
        undo.addUndoable(line);
        if (graphicView) {
            graphicView->drawEntity(line);
        }
    }

    return ret;
}

/**
     * Creates an insert with the given data.
     *
     * @param data Insert data (position, block name, ..)
     */
RS_Insert* RS_Creation::createInsert(const RS_InsertData* pdata) {

    RS_DEBUG->print("RS_Creation::createInsert");

    LC_UndoSection undo( document, handleUndo);
    auto ins = new RS_Insert(container, *pdata);
    // inserts are also on layers
    setEntity(ins);

    RS_DEBUG->print("RS_Creation::createInsert: OK");

    return ins;
}



/**
     * Creates an image with the given data.
     */
RS_Image* RS_Creation::createImage(const RS_ImageData* data) {

    LC_UndoSection undo( document, handleUndo);
    auto* img = new RS_Image(container, *data);
    img->update();
    setEntity(img);

    return img;
}


/**
     * Creates a new block from the currently selected entitiies.
     *
     * @param referencePoint Reference point for the block.
     * @param name Block name
     * @param remove true: remove existing entities, false: don't touch entities
     */
RS_Block* RS_Creation::createBlock(const RS_BlockData* data,
                                   const RS_Vector& referencePoint,
                                   const bool remove) {

    // start undo cycle for the container if we're deleting the existing entities
    LC_UndoSection undo(document, remove);
    RS_Block* block;
    // Block cannot contain blocks.
    if (container->rtti() == RS2::EntityBlock) {
        block = new RS_Block(container->getParent(), RS_BlockData(*data));
    } else {
        block = new RS_Block(container, RS_BlockData(*data));
    }

    // copy entities into a block
    for(auto e: *container){ // fixme - iterating all entities for selection
        if (e && e->isSelected()) {

            // delete / redraw entity in graphic view:
            if (remove) {
                if (graphicView) {
                    graphicView->deleteEntity(e);
                }
                e->setSelected(false);
            } else {
                if (graphicView) {
                    graphicView->deleteEntity(e);
                }
                e->setSelected(false);
                if (graphicView) {
                    graphicView->drawEntity(e);
                }
            }

            // add entity to block:
            RS_Entity* c = e->clone();
            c->move(-referencePoint);
            block->addEntity(c);

            if (remove) {
                //container->removeEntity(e);
                //i=0;
                e->changeUndoState();
                undo.addUndoable(e);
            }
        }
    }

    if (graphic) {
        graphic->addBlock(block);
    }

    return block;
}



/**
     * Inserts a library item from the given path into the drawing.
     */
RS_Insert* RS_Creation::createLibraryInsert(RS_LibraryInsertData& data) {

    RS_DEBUG->print("RS_Creation::createLibraryInsert");

    RS_Graphic g;
    if (!g.open(data.file, RS2::FormatUnknown)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Creation::createLibraryInsert: Cannot open file: %s", data.file.toStdString().c_str());
        return nullptr;
    }

    // unit conversion:
    if (graphic) {
        double uf = RS_Units::convert(1.0, g.getUnit(),
                                      graphic->getUnit());
        g.scale(RS_Vector(0.0, 0.0), RS_Vector(uf, uf));
    }

    //g.scale(RS_Vector(data.factor, data.factor));
    //g.rotate(data.angle);

    QString s;
    s = QFileInfo(data.file).completeBaseName();

    RS_Modification m(*container, graphicView);
    m.paste(
                RS_PasteData(
                    data.insertionPoint,
                    data.factor, data.angle, true,
                    s),
                &g);

    RS_DEBUG->print("RS_Creation::createLibraryInsert: OK");

    return nullptr;
}

void RS_Creation::setEntity(RS_Entity* en) const
{
    en->setLayerToActive();
    en->setPenToActive();

    if (container) {
        container->addEntity(en);
    }
    LC_UndoSection undo( document, handleUndo);
    undo.addUndoable(en);
    if (graphicView) {
        graphicView->drawEntity(en);
    }
}
