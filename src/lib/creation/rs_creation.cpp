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

#include "rs_information.h"
#include "rs_fileinfo.h"
#include "rs_graphic.h"
#include "rs_constructionline.h"
#include "rs_graphicview.h"
#include "rs_modification.h"

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Creation::RS_Creation(RS_EntityContainer* container,
                         RS_GraphicView* graphicView,
                         bool handleUndo) {
    this->container = container;
    this->graphicView = graphicView;
    this->handleUndo = handleUndo;
    if (container!=NULL) {
        graphic = container->getGraphic();
        document = container->getDocument();
    } else {
        graphic = NULL;
        document = NULL;
    }
}


/**
 * Creates a point entity.
 *
 * E.g.:<br>
 * <code>
 * creation.createPoint(RS_Vector(10.0, 15.0));
 * </code>
 *
 * @param p position
 */
/*void RS_Creation::createPoint(const RS_Vector& p) {
    entityContainer->addEntity(new RS_Point(entityContainer, p));
}*/


/**
 * Creates a line with two points given.
 *
 * E.g.:<br>
 * <code>
 * creation.createLine2P(RS_Vector(10.0, 10.0), RS_Vector(100.0, 200.0));
 * </code>
 *
 * @param p1 start point
 * @param p2 end point
 */
/*void RS_Creation::createLine2P(const RS_Vector& p1, const RS_Vector& p2) {
    entityContainer->addEntity(new RS_Line(entityContainer,
                                           RS_LineData(p1, p2)));
}*/

/**
 * Creates a rectangle with two edge points given.
 *
 * E.g.:<br>
 * <code>
 * creation.createRectangle(RS_Vector(5.0, 2.0), RS_Vector(7.5, 3.0));
 * </code>
 *
 * @param p1 edge one
 * @param p2 edge two
 */
/*void RS_Creation::createRectangle(const RS_Vector& e1, const RS_Vector& e2) {
    RS_Vector e21(e2.x, e1.y);
    RS_Vector e12(e1.x, e2.y);
    entityContainer->addEntity(new RS_Line(entityContainer,
                                           RS_LineData(e1, e12)));
    entityContainer->addEntity(new RS_Line(entityContainer,
                                           RS_LineData(e12, e2)));
    entityContainer->addEntity(new RS_Line(entityContainer,
                                           RS_LineData(e2, e21)));
    entityContainer->addEntity(new RS_Line(entityContainer,
                                           RS_LineData(e21, e1)));
}*/


/**
 * Creates a polyline from the given array of entities.
 * No checking if the entities actually fit together.
 * Currently this is like a group.
 *
 * E.g.:<br>
 * <code>
 * RS_Polyline *pl = creation.createPolyline(RS_Vector(25.0, 55.0));<br>
 * pl->addVertex(RS_Vector(50.0, 75.0));<br>
 * </code>
 *
 * @param entities array of entities
 * @param startPoint Start point of the polyline
 */
/*RS_Polyline* RS_Creation::createPolyline(const RS_Vector& startPoint) {
    RS_Polyline* pl = new RS_Polyline(entityContainer, 
		RS_PolylineData(startPoint, RS_Vector(0.0,0.0), 0));
    entityContainer->addEntity(pl);
    return pl;
}*/



/**
 * Creates an entity parallel to the given entity e through the given 
 * 'coord'.
 *
 * @param coord Coordinate to define the distance / side (typically a 
 *              mouse coordinate).
 * @param number Number of parallels.
 * @param e Original entity.
 *
 * @return Pointer to the first created parallel or NULL if no 
 *    parallel has been created.
 */
RS_Entity* RS_Creation::createParallelThrough(const RS_Vector& coord,
        int number,
        RS_Entity* e) {
    if (e==NULL) {
        return NULL;
    }

    double dist;

    if (e->rtti()==RS2::EntityLine) {
        RS_Line* l = (RS_Line*)e;
        RS_ConstructionLine cl(NULL,
                               RS_ConstructionLineData(l->getStartpoint(),
                                                       l->getEndpoint()));
        dist = cl.getDistanceToPoint(coord);
    } else {
        dist = e->getDistanceToPoint(coord);
    }

    if (dist<RS_MAXDOUBLE) {
        return createParallel(coord, dist, number, e);
    } else {
        return NULL;
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
 * @return Pointer to the first created parallel or NULL if no 
 *    parallel has been created.
 */
RS_Entity* RS_Creation::createParallel(const RS_Vector& coord,
                                       double distance, int number,
                                       RS_Entity* e) {
    if (e==NULL) {
        return NULL;
    }

    switch (e->rtti()) {
    case RS2::EntityLine:
        return createParallelLine(coord, distance, number, (RS_Line*)e);
        break;

    case RS2::EntityArc:
        return createParallelArc(coord, distance, number, (RS_Arc*)e);
        break;

    case RS2::EntityCircle:
        return createParallelCircle(coord, distance, number, (RS_Circle*)e);
        break;

    default:
        break;
    }

    return NULL;
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
 * @return Pointer to the first created parallel or NULL if no 
 *    parallel has been created.
 */
RS_Line* RS_Creation::createParallelLine(const RS_Vector& coord,
        double distance, int number,
        RS_Line* e) {

    if (e==NULL) {
        return NULL;
    }

    double ang = e->getAngle1() + M_PI/2.0;
    RS_Vector p1, p2;
    RS_LineData parallelData;
    RS_Line* ret = NULL;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    for (int num=1; num<=number; ++num) {

        // calculate 1st parallel:
        p1.setPolar(distance*num, ang);
        p1 += e->getStartpoint();
        p2.setPolar(distance*num, ang);
        p2 += e->getEndpoint();
        RS_Line parallel1(NULL, RS_LineData(p1, p2));

        // calculate 2nd parallel:
        p1.setPolar(distance*num, ang+M_PI);
        p1 += e->getStartpoint();
        p2.setPolar(distance*num, ang+M_PI);
        p2 += e->getEndpoint();
        RS_Line parallel2(NULL, RS_LineData(p1, p2));

        double dist1 = parallel1.getDistanceToPoint(coord);
        double dist2 = parallel2.getDistanceToPoint(coord);
        double minDist = std::min(dist1, dist2);

        if (minDist<RS_MAXDOUBLE) {
            if (dist1<dist2) {
                parallelData = parallel1.getData();
            } else {
                parallelData = parallel2.getData();
            }


            RS_Line* newLine = new RS_Line(container, parallelData);
            newLine->setLayerToActive();
            newLine->setPenToActive();
            if (ret==NULL) {
                ret = newLine;
            }
            if (container!=NULL) {
                container->addEntity(newLine);
            }
            if (document!=NULL && handleUndo) {
                document->addUndoable(newLine);
                //document->endUndoCycle();
            }
            if (graphicView!=NULL) {
                graphicView->drawEntity(newLine);
            }
        }
    }

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
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
 * @return Pointer to the first created parallel or NULL if no 
 *    parallel has been created.
 */
RS_Arc* RS_Creation::createParallelArc(const RS_Vector& coord,
                                       double distance, int number,
                                       RS_Arc* e) {

    if (e==NULL) {
        return NULL;
    }

    RS_ArcData parallelData;
    RS_Arc* ret = NULL;

    bool inside = (e->getCenter().distanceTo(coord) < e->getRadius());

    if (inside) {
        distance *= -1;
    }

    for (int num=1; num<=number; ++num) {

        // calculate parallel:
        bool ok = true;
        RS_Arc parallel1(NULL, e->getData());
        parallel1.setRadius(e->getRadius() + distance*num);
        if (parallel1.getRadius()<0.0) {
            parallel1.setRadius(RS_MAXDOUBLE);
            ok = false;
        }

        // calculate 2nd parallel:
        //RS_Arc parallel2(NULL, e->getData());
        //parallel2.setRadius(e->getRadius()+distance*num);

        //double dist1 = parallel1.getDistanceToPoint(coord);
        //double dist2 = parallel2.getDistanceToPoint(coord);
        //double minDist = min(dist1, dist2);

        //if (minDist<RS_MAXDOUBLE) {
        if (ok==true) {
            //if (dist1<dist2) {
            parallelData = parallel1.getData();
            //} else {
            //    parallelData = parallel2.getData();
            //}

            if (document!=NULL && handleUndo) {
                document->startUndoCycle();
            }

            RS_Arc* newArc = new RS_Arc(container, parallelData);
            newArc->setLayerToActive();
            newArc->setPenToActive();
            if (ret==NULL) {
                ret = newArc;
            }
            if (container!=NULL) {
                container->addEntity(newArc);
            }
            if (document!=NULL && handleUndo) {
                document->addUndoable(newArc);
                document->endUndoCycle();
            }
            if (graphicView!=NULL) {
                graphicView->drawEntity(newArc);
            }
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
 * @return Pointer to the first created parallel or NULL if no 
 *    parallel has been created.
 */
RS_Circle* RS_Creation::createParallelCircle(const RS_Vector& coord,
        double distance, int number,
        RS_Circle* e) {

    if (e==NULL) {
        return NULL;
    }

    RS_CircleData parallelData;
    RS_Circle* ret = NULL;

    bool inside = (e->getCenter().distanceTo(coord) < e->getRadius());

    if (inside) {
        distance *= -1;
    }

    for (int num=1; num<=number; ++num) {

        // calculate parallel:
        bool ok = true;
        RS_Circle parallel1(NULL, e->getData());
        parallel1.setRadius(e->getRadius() + distance*num);
        if (parallel1.getRadius()<0.0) {
            parallel1.setRadius(RS_MAXDOUBLE);
            ok = false;
        }

        // calculate 2nd parallel:
        //RS_Circle parallel2(NULL, e->getData());
        //parallel2.setRadius(e->getRadius()+distance*num);

        //double dist1 = parallel1.getDistanceToPoint(coord);
        //double dist2 = parallel2.getDistanceToPoint(coord);
        //double minDist = min(dist1, dist2);

        //if (minDist<RS_MAXDOUBLE) {
        if (ok==true) {
            //if (dist1<dist2) {
            parallelData = parallel1.getData();
            //} else {
            //    parallelData = parallel2.getData();
            //}

            if (document!=NULL && handleUndo) {
                document->startUndoCycle();
            }

            RS_Circle* newCircle = new RS_Circle(container, parallelData);
            newCircle->setLayerToActive();
            newCircle->setPenToActive();
            if (ret==NULL) {
                ret = newCircle;
            }
            if (container!=NULL) {
                container->addEntity(newCircle);
            }
            if (document!=NULL && handleUndo) {
                document->addUndoable(newCircle);
                document->endUndoCycle();
            }
            if (graphicView!=NULL) {
                graphicView->drawEntity(newCircle);
            }
        }
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
 * @return Pointer to the first bisector created or NULL if no bisectors
 *   were created.
 */
RS_Line* RS_Creation::createBisector(const RS_Vector& coord1,
                                     const RS_Vector& coord2,
                                     double length,
                                     int num,
                                     RS_Line* l1,
                                     RS_Line* l2) {

    RS_VectorSolutions sol;

    // check given entities:
    if (l1==NULL || l2==NULL ||
            l1->rtti()!=RS2::EntityLine || l2->rtti()!=RS2::EntityLine) {
        return NULL;
    }

    // intersection between entities:
    sol = RS_Information::getIntersection(l1, l2, false);
    RS_Vector inters = sol.get(0);
    if (inters.valid==false) {
        return NULL;
    }

    double angle1 = inters.angleTo(l1->getNearestPointOnEntity(coord1));
    double angle2 = inters.angleTo(l2->getNearestPointOnEntity(coord2));
    double angleDiff = RS_Math::getAngleDifference(angle1, angle2);
    if (angleDiff>M_PI) {
        angleDiff = angleDiff - 2*M_PI;
    }
    RS_Line* ret = NULL;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    for (int n=1; n<=num; ++n) {

        double angle = angle1 +
                       (angleDiff / (num+1) * n);

        RS_LineData d;
        RS_Vector v;

        RS_Vector c;
        v.setPolar(length, angle);
        d = RS_LineData(inters, inters + v);

        RS_Line* newLine = new RS_Line(container, d);
        if (container!=NULL) {
            newLine->setLayerToActive();
            newLine->setPenToActive();
            container->addEntity(newLine);
        }
        if (document!=NULL && handleUndo) {
            document->addUndoable(newLine);
        }
        if (graphicView!=NULL) {
            graphicView->drawEntity(newLine);
        }
        if (ret==NULL) {
            ret = newLine;
        }
    }
    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

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
                                     RS_Entity* circle) {
    RS_Line* ret = NULL;
    RS_Vector circleCenter;

    // check given entities:
    if (circle==NULL || !point.valid ||
            (circle->rtti()!=RS2::EntityArc && circle->rtti()!=RS2::EntityCircle
             && circle->rtti()!=RS2::EntityEllipse)) {

        return NULL;
    }

    if (circle->rtti()==RS2::EntityCircle) {
        circleCenter = ((RS_Circle*)circle)->getCenter();
    } else if (circle->rtti()==RS2::EntityArc) {
        circleCenter = ((RS_Arc*)circle)->getCenter();
    } else if (circle->rtti()==RS2::EntityEllipse) {
        circleCenter = ((RS_Ellipse*)circle)->getCenter();
    }

    // the two tangent points:
    RS_VectorSolutions sol;

    // calculate tangent points for arcs / circles:
    if (circle->rtti()!=RS2::EntityEllipse) {
        // create temp. thales circle:
        RS_Vector tCenter = (point + circleCenter)/2.0;
        double tRadius = point.distanceTo(tCenter);

        RS_Circle tmp(NULL, RS_CircleData(tCenter, tRadius));

        // get the two intersection points which are the tangent points:
        sol = RS_Information::getIntersection(&tmp, circle, false);
    }

    // calculate tangent points for ellipses:
    else {
        RS_Ellipse* el = (RS_Ellipse*)circle;
        sol.alloc(2);
        //sol.set(0, circleCenter);
        //sol.set(1, circleCenter);


        double a = el->getMajorRadius();     // the length of the major axis / 2
        double b = el->getMinorRadius();     // the length of the minor axis / 2

		// rotate and move point:
		RS_Vector point2 = point;
		point2.move(-el->getCenter());
		point2.rotate(-el->getAngle());
		
        double xp = point2.x;             // coordinates of the given point
        double yp = point2.y;
		
        double xt1;                      // Tangent point 1
        double yt1;
        double xt2;                      // Tangent point 2
        double yt2;

        double a2 = a * a;
        double b2 = b * b;
        double d = a2 / b2 * yp / xp;
        double e = a2 / xp;
        double af = b2 * d * d + a2;
        double bf = -b2 * d * e * 2.0;
        double cf = b2 * e * e - a2 * b2;
        double t = sqrt(bf * bf - af * cf * 4.0);
        yt1 = (t - bf) / (af * 2.0);
        xt1 = e - d * yt1;
        yt2 = (-t - bf) / (af * 2.0);
        xt2 = e - d * yt2;

		RS_Vector s1 = RS_Vector(xt1, yt1);
		RS_Vector s2 = RS_Vector(xt2, yt2);

		s1.rotate(el->getAngle());
		s1.move(el->getCenter());
	
		s2.rotate(el->getAngle());
		s2.move(el->getCenter());
		
		sol.set(0, s1);
		sol.set(1, s2);

		
    }

    if (!sol.get(0).valid || !sol.get(1).valid) {
        return NULL;
    }

    // create all possible tangents:
    RS_Line* poss[2];

    RS_LineData d;

    d = RS_LineData(sol.get(0), point);
    poss[0] = new RS_Line(NULL, d);
    d = RS_LineData(sol.get(1), point);
    poss[1] = new RS_Line(NULL, d);

    // find closest tangent:
    double minDist = RS_MAXDOUBLE;
    double dist;
    int idx = -1;
    for (int i=0; i<2; ++i) {
        dist = poss[i]->getDistanceToPoint(coord);
        if (dist<minDist) {
            minDist = dist;
            idx = i;
        }
    }

    // create the closest tangent:
    if (idx!=-1) {
        RS_LineData d = poss[idx]->getData();

        for (int i=0; i<2; ++i) {
            delete poss[i];
        }

        if (document!=NULL && handleUndo) {
            document->startUndoCycle();
        }

        ret = new RS_Line(container, d);
        ret->setLayerToActive();
        ret->setPenToActive();
        if (container!=NULL) {
            container->addEntity(ret);
        }
        if (document!=NULL && handleUndo) {
            document->addUndoable(ret);
            document->endUndoCycle();
        }
        if (graphicView!=NULL) {
            graphicView->drawEntity(ret);
        }
    } else {
        ret = NULL;
    }

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
RS_Line* RS_Creation::createTangent2(const RS_Vector& coord,
                                     RS_Entity* circle1,
                                     RS_Entity* circle2) {
    RS_Line* ret = NULL;
    RS_Vector circleCenter1;
    RS_Vector circleCenter2;
    double circleRadius1 = 0.0;
    double circleRadius2 = 0.0;

    // check given entities:
    if (circle1==NULL || circle2==NULL ||
            (circle1->rtti()!=RS2::EntityArc &&
             circle1->rtti()!=RS2::EntityCircle) ||
            (circle2->rtti()!=RS2::EntityArc &&
             circle2->rtti()!=RS2::EntityCircle) ) {

        return NULL;
    }

    if (circle1->rtti()==RS2::EntityCircle) {
        circleCenter1 = ((RS_Circle*)circle1)->getCenter();
        circleRadius1 = ((RS_Circle*)circle1)->getRadius();
    } else if (circle1->rtti()==RS2::EntityArc) {
        circleCenter1 = ((RS_Arc*)circle1)->getCenter();
        circleRadius1 = ((RS_Arc*)circle1)->getRadius();
    }

    if (circle2->rtti()==RS2::EntityCircle) {
        circleCenter2 = ((RS_Circle*)circle2)->getCenter();
        circleRadius2 = ((RS_Circle*)circle2)->getRadius();
    } else if (circle2->rtti()==RS2::EntityArc) {
        circleCenter2 = ((RS_Arc*)circle2)->getCenter();
        circleRadius2 = ((RS_Arc*)circle2)->getRadius();
    }

    // create all possible tangents:
    RS_Line* poss[4];
    for (int i=0; i<4; ++i) {
        poss[i] = NULL;
    }

    RS_LineData d;

    double angle1 = circleCenter1.angleTo(circleCenter2);
    double dist1 = circleCenter1.distanceTo(circleCenter2);

    if (dist1>1.0e-6) {
        // outer tangents:
        double dist2 = circleRadius2 - circleRadius1;
        if (dist1>dist2) {
            double angle2 = asin(dist2/dist1);
            double angt1 = angle1 + angle2 + M_PI/2.0;
            double angt2 = angle1 - angle2 - M_PI/2.0;
            RS_Vector offs1;
            RS_Vector offs2;

            offs1.setPolar(circleRadius1, angt1);
            offs2.setPolar(circleRadius2, angt1);

            d = RS_LineData(circleCenter1 + offs1,
                            circleCenter2 + offs2);
            poss[0] = new RS_Line(NULL, d);


            offs1.setPolar(circleRadius1, angt2);
            offs2.setPolar(circleRadius2, angt2);

            d = RS_LineData(circleCenter1 + offs1,
                            circleCenter2 + offs2);
            poss[1] = new RS_Line(NULL, d);
        }

        // inner tangents:
        double dist3 = circleRadius2 + circleRadius1;
        if (dist1>dist3) {
            double angle3 = asin(dist3/dist1);
            double angt3 = angle1 + angle3 + M_PI/2.0;
            double angt4 = angle1 - angle3 - M_PI/2.0;
            RS_Vector offs1;
            RS_Vector offs2;

            offs1.setPolar(circleRadius1, angt3);
            offs2.setPolar(circleRadius2, angt3);

            d = RS_LineData(circleCenter1 - offs1,
                            circleCenter2 + offs2);
            poss[2] = new RS_Line(NULL, d);


            offs1.setPolar(circleRadius1, angt4);
            offs2.setPolar(circleRadius2, angt4);

            d = RS_LineData(circleCenter1 - offs1,
                            circleCenter2 + offs2);
            poss[3] = new RS_Line(NULL, d);
        }

    }

    // find closest tangent:
    double minDist = RS_MAXDOUBLE;
    double dist;
    int idx = -1;
    for (int i=0; i<4; ++i) {
        if (poss[i]!=NULL) {
            dist = poss[i]->getDistanceToPoint(coord);
            if (dist<minDist) {
                minDist = dist;
                idx = i;
            }
        }
    }

    if (idx!=-1) {
        RS_LineData d = poss[idx]->getData();
        for (int i=0; i<4; ++i) {
            if (poss[i]!=NULL) {
                delete poss[i];
            }
        }

        if (document!=NULL && handleUndo) {
            document->startUndoCycle();
        }

        ret = new RS_Line(container, d);
        ret->setLayerToActive();
        ret->setPenToActive();
        if (container!=NULL) {
            container->addEntity(ret);
        }
        if (document!=NULL && handleUndo) {
            document->addUndoable(ret);
            document->endUndoCycle();
        }
        if (graphicView!=NULL) {
            graphicView->drawEntity(ret);
        }
    } else {
        ret = NULL;
    }

    return ret;
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
    if (entity==NULL || !coord.valid ||
            (entity->rtti()!=RS2::EntityArc && entity->rtti()!=RS2::EntityCircle
             && entity->rtti()!=RS2::EntityLine)) {

        return NULL;
    }

    double a1=0.0;

    switch (entity->rtti()) {
    case RS2::EntityLine:
        a1 = ((RS_Line*)entity)->getAngle1();
        break;
    case RS2::EntityArc:
        a1 = ((RS_Arc*)entity)->getCenter().angleTo(coord) + M_PI/2.0;
        break;
    case RS2::EntityCircle:
        a1 = ((RS_Circle*)entity)->getCenter().angleTo(coord);
        break;
    default:
        // never reached
        break;
    }

    a1 += angle;

    RS_Vector v1;
    v1.setPolar(length, a1);
    //RS_ConstructionLineData(coord-v1, coord+v1);
    RS_LineData d(coord-v1, coord+v1);
    RS_Line* ret;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    ret = new RS_Line(container, d);
    ret->setLayerToActive();
    ret->setPenToActive();
    if (container!=NULL) {
        container->addEntity(ret);
    }
    if (document!=NULL && handleUndo) {
        document->addUndoable(ret);
        document->endUndoCycle();
    }
    if (graphicView!=NULL) {
        graphicView->drawEntity(ret);
    }

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
        return NULL;
    }

    RS_Line* ret = NULL;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    RS_Vector c1(false);
    RS_Vector c2 = corner;
    RS_Line* line;

    for (int n=1; n<=number; ++n) {
        c1 = c2;
        c2 = c2.rotate(center, (M_PI*2)/number);

        line = new RS_Line(container, RS_LineData(c1, c2));
        line->setLayerToActive();
        line->setPenToActive();

        if (ret==NULL) {
            ret = line;
        }

        if (container!=NULL) {
            container->addEntity(line);
        }
        if (document!=NULL && handleUndo) {
            document->addUndoable(line);
        }
        if (graphicView!=NULL) {
            graphicView->drawEntity(line);
        }
    }

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
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
        return NULL;
    }

    RS_Line* ret = NULL;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    double len = corner1.distanceTo(corner2);
    double ang1 = corner1.angleTo(corner2);
    double ang = ang1;

    RS_Vector c1(false);
    RS_Vector c2 = corner1;
    RS_Vector edge;
    RS_Line* line;

    for (int n=1; n<=number; ++n) {
        c1 = c2;
        edge.setPolar(len, ang);
        c2 = c1 + edge;

        line = new RS_Line(container, RS_LineData(c1, c2));
        line->setLayerToActive();
        line->setPenToActive();

        if (ret==NULL) {
            ret = line;
        }

        if (container!=NULL) {
            container->addEntity(line);
        }
        if (document!=NULL && handleUndo) {
            document->addUndoable(line);
        }
        if (graphicView!=NULL) {
            graphicView->drawEntity(line);
        }

        // more accurate than incrementing the angle:
        ang = ang1 + (2*M_PI)/number*n;
    }

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    return ret;
}



/**
 * Creates an insert with the given data.
 *
 * @param data Insert data (position, block name, ..)
 */
RS_Insert* RS_Creation::createInsert(RS_InsertData& data) {

    RS_DEBUG->print("RS_Creation::createInsert");

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    RS_Insert* ins = new RS_Insert(container, data);
    // inserts are also on layers
    ins->setLayerToActive();
    ins->setPenToActive();

    if (container!=NULL) {
        container->addEntity(ins);
    }
    if (document!=NULL && handleUndo) {
        document->addUndoable(ins);
        document->endUndoCycle();
    }
    if (graphicView!=NULL) {
        graphicView->drawEntity(ins);
    }

    RS_DEBUG->print("RS_Creation::createInsert: OK");

    return ins;
}



/**
 * Creates an image with the given data.
 */
RS_Image* RS_Creation::createImage(RS_ImageData& data) {

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    RS_Image* img = new RS_Image(container, data);
    img->setLayerToActive();
    img->setPenToActive();
    img->update();

    if (container!=NULL) {
        container->addEntity(img);
    }
    if (document!=NULL && handleUndo) {
        document->addUndoable(img);
        document->endUndoCycle();
    }
    if (graphicView!=NULL) {
        graphicView->drawEntity(img);
    }

    return img;
}


/**
 * Creates a new block from the currently selected entitiies.
 *
 * @param referencePoint Reference point for the block.
 * @param name Block name
 * @param remove true: remove existing entities, false: don't touch entities
 */
RS_Block* RS_Creation::createBlock(const RS_BlockData& data,
                                   const RS_Vector& referencePoint,
                                   const bool remove) {

    // start undo cycle for the container if we're deleting the existing entities
    if (remove && document!=NULL) {
        document->startUndoCycle();
    }

    RS_Block* block =
        new RS_Block(container,
                     RS_BlockData(data.name, data.basePoint, data.frozen));

    // copy entities into a block
    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {
        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e!=NULL && e->isSelected()) {

            // delete / redraw entity in graphic view:
            if (remove) {
                if (graphicView!=NULL) {
                    graphicView->deleteEntity(e);
                }
                e->setSelected(false);
            } else {
                if (graphicView!=NULL) {
                    graphicView->deleteEntity(e);
                }
                e->setSelected(false);
                if (graphicView!=NULL) {
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
                if (document!=NULL) {
                    document->addUndoable(e);
                }
            }
        }
    }

    if (remove && document!=NULL) {
        document->endUndoCycle();
    }

    if (graphic!=NULL) {
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
			"RS_Creation::createLibraryInsert: Cannot open file: %s");
        return NULL;
    }

    // unit conversion:
    if (graphic!=NULL) {
        double uf = RS_Units::convert(1.0, g.getUnit(),
                                      graphic->getUnit());
        g.scale(RS_Vector(0.0, 0.0), RS_Vector(uf, uf));
    }

    //g.scale(RS_Vector(data.factor, data.factor));
    //g.rotate(data.angle);

    RS_String s;
    s = RS_FileInfo(data.file).baseName(true);

    RS_Modification m(*container, graphicView);
    m.paste(
        RS_PasteData(
            data.insertionPoint,
            data.factor, data.angle, true,
            s),
        &g);

    RS_DEBUG->print("RS_Creation::createLibraryInsert: OK");

    return NULL;
}

// EOF
