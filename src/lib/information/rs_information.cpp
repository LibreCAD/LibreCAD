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

#include "rs_information.h"

#include "rs_constructionline.h"


/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Information::RS_Information(RS_EntityContainer& container) {
    this->container = &container;
}



/**
 * @return true: if the entity is a dimensioning enity.
 *         false: otherwise
 */
bool RS_Information::isDimension(RS2::EntityType type) {
    if (type==RS2::EntityDimAligned ||
            type==RS2::EntityDimLinear ||
            type==RS2::EntityDimRadial ||
            type==RS2::EntityDimDiametric ||
            type==RS2::EntityDimAngular) {
        return true;
    } else {
        return false;
    }
}



/**
 * @retval true the entity can be trimmed.
 * i.e. it is in a graphic or in a polyline.
 */
bool RS_Information::isTrimmable(RS_Entity* e) {
	if (e!=NULL) {
		if (e->getParent()!=NULL) {
			if (e->getParent()->rtti()==RS2::EntityPolyline) {
				return true;
			}
			else if (e->getParent()->rtti()==RS2::EntityContainer ||
			          e->getParent()->rtti()==RS2::EntityGraphic ||
					  e->getParent()->rtti()==RS2::EntityBlock) {

				// normal entity:
				return true;
			}
		}
	}

	return false;
}


/**
 * @retval true the two entities can be trimmed to each other;
 * i.e. they are in a graphic or in the same polyline.
 */
bool RS_Information::isTrimmable(RS_Entity* e1, RS_Entity* e2) {
	if (e1!=NULL && e2!=NULL) {
		if (e1->getParent()!=NULL && e2->getParent()!=NULL) {
			if (e1->getParent()->rtti()==RS2::EntityPolyline &&
			   e2->getParent()->rtti()==RS2::EntityPolyline &&
			   e1->getParent()==e2->getParent()) {

				// in the same polyline
				RS_EntityContainer* pl = e1->getParent();
				int idx1 = pl->findEntity(e1);
				int idx2 = pl->findEntity(e2);
				RS_DEBUG->print("RS_Information::isTrimmable: "
					"idx1: %d, idx2: %d", idx1, idx2);
				if (abs(idx1-idx2)==1 || abs(idx1-idx2)==pl->count()-1) {
					// directly following entities
					return true;
				}
				else {
					// not directly following entities
					return false;
				}
			}
			else if ((e1->getParent()->rtti()==RS2::EntityContainer ||
			          e1->getParent()->rtti()==RS2::EntityGraphic ||
					  e1->getParent()->rtti()==RS2::EntityBlock) &&
					 (e2->getParent()->rtti()==RS2::EntityContainer ||
			          e2->getParent()->rtti()==RS2::EntityGraphic ||
					  e2->getParent()->rtti()==RS2::EntityBlock)) {

				// normal entities:
				return true;
			}
		}
		else {
			// independent entities with the same parent:
			return (e1->getParent()==e2->getParent());
		}
	}

	return false;
}


/**
 * Gets the nearest end point to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics 
 * container.
 */
RS_Vector RS_Information::getNearestEndpoint(const RS_Vector& coord,
        double* dist) const {
    return container->getNearestEndpoint(coord, dist);
}


/**
 * Gets the nearest point to the given coordinate which is on an entity. 
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a double which will contain the 
 *        measured distance after return or NULL
 * @param entity Pointer to a pointer which will point to the
 *        entity on which the point is or NULL
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics 
 * container.
 */
RS_Vector RS_Information::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity,
        double* dist,
        RS_Entity** entity) const {

    return container->getNearestPointOnEntity(coord, onEntity, dist, entity);
}


/**
 * Gets the nearest entity to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a double which will contain the 
 *             masured distance after return
 * @param level Level of resolving entities.
 *
 * @return the entity found or NULL if there are no elements 
 * at all in this graphics container.
 */
RS_Entity* RS_Information::getNearestEntity(const RS_Vector& coord,
        double* dist,
        RS2::ResolveLevel level) const {

    return container->getNearestEntity(coord, dist, level);
}



/**
 * Calculates the intersection point(s) between two entities.
 *
 * @param onEntities true: only return intersection points which are
 *                   on both entities.
 *                   false: return all intersection points.
 *
 * @todo support more entities
 *
 * @return All intersections of the two entities. The tangent flag in
 * RS_VectorSolutions is set if one intersection is a tangent point.
 */
RS_VectorSolutions RS_Information::getIntersection(RS_Entity* e1,
        RS_Entity* e2, bool onEntities) {

    RS_VectorSolutions ret;
	double tol = 1.0e-4;

    if (e1==NULL || e2==NULL) {
        return ret;
    }

    // unsupported entities / entity combinations:
    if ((e1->rtti()==RS2::EntityEllipse && e2->rtti()==RS2::EntityEllipse) ||
            e1->rtti()==RS2::EntityText || e2->rtti()==RS2::EntityText ||
            isDimension(e1->rtti()) || isDimension(e2->rtti())) {

        return ret;
    }

    // (only) one entity is an ellipse:
    if (e1->rtti()==RS2::EntityEllipse || e2->rtti()==RS2::EntityEllipse) {
        if (e2->rtti()==RS2::EntityEllipse) {
            RS_Entity* tmp = e1;
            e1 = e2;
            e2 = tmp;
        }
        if (e2->rtti()==RS2::EntityLine) {
            RS_Ellipse* ellipse = (RS_Ellipse*)e1;
            ret = getIntersectionLineEllipse((RS_Line*)e2, ellipse);
			tol = 1.0e-1;
        } 
		
		// ellipse / arc, ellipse / ellipse: not supported:
		else {
            return ret;
        }
    } else {

        RS_Entity* te1 = e1;
        RS_Entity* te2 = e2;

        // entity copies - so we only have to deal with lines and arcs
        RS_Line l1(NULL,
                   RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(0.0,0.0)));
        RS_Line l2(NULL,
                   RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(0.0,0.0)));

        RS_Arc a1(NULL,
                  RS_ArcData(RS_Vector(0.0,0.0), 1.0, 0.0, 2*M_PI, false));
        RS_Arc a2(NULL,
                  RS_ArcData(RS_Vector(0.0,0.0), 1.0, 0.0, 2*M_PI, false));

        // convert construction lines to lines:
        if (e1->rtti()==RS2::EntityConstructionLine) {
            RS_ConstructionLine* cl = (RS_ConstructionLine*)e1;

            l1.setStartpoint(cl->getPoint1());
            l1.setEndpoint(cl->getPoint2());

            te1 = &l1;
        }
        if (e2->rtti()==RS2::EntityConstructionLine) {
            RS_ConstructionLine* cl = (RS_ConstructionLine*)e2;

            l2.setStartpoint(cl->getPoint1());
            l2.setEndpoint(cl->getPoint2());

            te2 = &l2;
        }


        // convert circles to arcs:
        if (e1->rtti()==RS2::EntityCircle) {
            RS_Circle* c = (RS_Circle*)e1;

            RS_ArcData data(c->getCenter(), c->getRadius(), 0.0, 2*M_PI, false);
            a1.setData(data);

            te1 = &a1;
        }
        if (e2->rtti()==RS2::EntityCircle) {
            RS_Circle* c = (RS_Circle*)e2;

            RS_ArcData data(c->getCenter(), c->getRadius(), 0.0, 2*M_PI, false);
            a2.setData(data);

            te2 = &a2;
        }


        // line / line:
        //
        //else
        if (te1->rtti()==RS2::EntityLine &&
                te2->rtti()==RS2::EntityLine) {

            RS_Line* line1 = (RS_Line*)te1;
            RS_Line* line2 = (RS_Line*)te2;

            ret = getIntersectionLineLine(line1, line2);
        }

        // line / arc:
        //
        else if (te1->rtti()==RS2::EntityLine &&
                 te2->rtti()==RS2::EntityArc) {

            RS_Line* line = (RS_Line*)te1;
            RS_Arc* arc = (RS_Arc*)te2;

            ret = getIntersectionLineArc(line, arc);
        }

        // arc / line:
        //
        else if (te1->rtti()==RS2::EntityArc &&
                 te2->rtti()==RS2::EntityLine) {

            RS_Arc* arc = (RS_Arc*)te1;
            RS_Line* line = (RS_Line*)te2;

            ret = getIntersectionLineArc(line, arc);
        }

        // arc / arc:
        //
        else if (te1->rtti()==RS2::EntityArc &&
                 te2->rtti()==RS2::EntityArc) {

            RS_Arc* arc1 = (RS_Arc*)te1;
            RS_Arc* arc2 = (RS_Arc*)te2;

            ret = getIntersectionArcArc(arc1, arc2);
        } else {
            RS_DEBUG->print("RS_Information::getIntersection:: Unsupported entity type.");
        }
    }


    // Check all intersection points for being on entities:
    //
    if (onEntities==true) {
        if (!e1->isPointOnEntity(ret.get(0), tol) ||
                !e2->isPointOnEntity(ret.get(0), tol)) {
            ret.set(0, RS_Vector(false));
        }
        if (!e1->isPointOnEntity(ret.get(1), tol) ||
                !e2->isPointOnEntity(ret.get(1), tol)) {
            ret.set(1, RS_Vector(false));
        }
        if (!e1->isPointOnEntity(ret.get(2), tol) ||
                !e2->isPointOnEntity(ret.get(2), tol)) {
            ret.set(2, RS_Vector(false));
        }
        if (!e1->isPointOnEntity(ret.get(3), tol) ||
                !e2->isPointOnEntity(ret.get(3), tol)) {
            ret.set(3, RS_Vector(false));
        }
    }

    int k=0;
    for (int i=0; i<4; ++i) {
        if (ret.get(i).valid) {
            ret.set(k, ret.get(i));
            k++;
        }
    }
    for (int i=k; i<4; ++i) {
        ret.set(i, RS_Vector(false));
    }

    return ret;
}



/**
 * @return Intersection between two lines.
 */
RS_VectorSolutions RS_Information::getIntersectionLineLine(RS_Line* e1,
        RS_Line* e2) {

    RS_VectorSolutions ret;

    if (e1==NULL || e2==NULL) {
        return ret;
    }

    RS_Vector p1 = e1->getStartpoint();
    RS_Vector p2 = e1->getEndpoint();
    RS_Vector p3 = e2->getStartpoint();
    RS_Vector p4 = e2->getEndpoint();

    double num = ((p4.x-p3.x)*(p1.y-p3.y) - (p4.y-p3.y)*(p1.x-p3.x));
    double div = ((p4.y-p3.y)*(p2.x-p1.x) - (p4.x-p3.x)*(p2.y-p1.y));

    if (fabs(div)>RS_TOLERANCE) {
        double u = num / div;

        double xs = p1.x + u * (p2.x-p1.x);
        double ys = p1.y + u * (p2.y-p1.y);
        ret = RS_VectorSolutions(RS_Vector(xs, ys));
    }

    // lines are parallel
    else {
        ret = RS_VectorSolutions();
    }

    return ret;
}



/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionLineArc(RS_Line* line,
        RS_Arc* arc) {

    RS_VectorSolutions ret;

    if (line==NULL || arc==NULL) {
        return ret;
    }

    double dist=0.0;
    RS_Vector nearest;
    nearest = line->getNearestPointOnEntity(arc->getCenter(), false, &dist);

    // special case: arc touches line (tangent):
    if (fabs(dist - arc->getRadius()) < 1.0e-4) {
        ret = RS_VectorSolutions(nearest);
        ret.setTangent(true);
        return ret;
    }

    RS_Vector p = line->getStartpoint();
    RS_Vector d = line->getEndpoint() - line->getStartpoint();
    if (d.magnitude()<1.0e-6) {
        return ret;
    }

    RS_Vector c = arc->getCenter();
    double r = arc->getRadius();
    RS_Vector delta = p - c;

    // root term:
    double term = RS_Math::pow(RS_Vector::dotP(d, delta), 2.0)
                  - RS_Math::pow(d.magnitude(), 2.0)
                  * (RS_Math::pow(delta.magnitude(), 2.0) - RS_Math::pow(r, 2.0));

    // no intersection:
    if (term<0.0) {
        RS_VectorSolutions s;
        ret = s;
    }

    // one or two intersections:
    else {
        double t1 = (- RS_Vector::dotP(d, delta) + sqrt(term))
                    / RS_Math::pow(d.magnitude(), 2.0);
        double t2;
        bool tangent = false;

        // only one intersection:
        if (fabs(term)<RS_TOLERANCE) {
            t2 = t1;
            tangent = true;
        }

        // two intersections
        else {
            t2 = (-RS_Vector::dotP(d, delta) - sqrt(term))
                 / RS_Math::pow(d.magnitude(), 2.0);
        }

        RS_Vector sol1;
        RS_Vector sol2(false);

        sol1 = p + d * t1;

        if (!tangent) {
            sol2 = p + d * t2;
        }

        ret = RS_VectorSolutions(sol1, sol2);
        ret.setTangent(tangent);
    }

    return ret;
}



/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionArcArc(RS_Arc* e1,
        RS_Arc* e2) {

    RS_VectorSolutions ret;

    if (e1==NULL || e2==NULL) {
        return ret;
    }

    RS_Vector c1 = e1->getCenter();
    RS_Vector c2 = e2->getCenter();

    double r1 = e1->getRadius();
    double r2 = e2->getRadius();

    RS_Vector u = c2 - c1;

    // concentric
    if (u.magnitude()<1.0e-6) {
        return ret;
    }

    RS_Vector v = RS_Vector(u.y, -u.x);

    double s, t1, t2, term;

    s = 1.0/2.0 * ((r1*r1 - r2*r2)/(RS_Math::pow(u.magnitude(), 2.0)) + 1.0);

    term = (r1*r1)/(RS_Math::pow(u.magnitude(), 2.0)) - s*s;

    // no intersection:
    if (term<0.0) {
        ret = RS_VectorSolutions();
    }

    // one or two intersections:
    else {
        t1 = sqrt(term);
        t2 = -sqrt(term);
        bool tangent = false;

        RS_Vector sol1 = c1 + u*s + v*t1;
        RS_Vector sol2 = c1 + u*s + v*t2;

        if (sol1.distanceTo(sol2)<1.0e-4) {
            sol2 = RS_Vector(false);
            ret = RS_VectorSolutions(sol1);
            tangent = true;
        } else {
            ret = RS_VectorSolutions(sol1, sol2);
        }

        ret.setTangent(tangent);
    }

    return ret;
}



/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionLineEllipse(RS_Line* line,
        RS_Ellipse* ellipse) {

    RS_VectorSolutions ret;

    if (line==NULL || ellipse==NULL) {
        return ret;
    }

    // rotate into normal position:
    double ang = ellipse->getAngle();

    double rx = ellipse->getMajorRadius();
    double ry = ellipse->getMinorRadius();
    RS_Vector center = ellipse->getCenter();
    RS_Vector a1 = line->getStartpoint().rotate(center, -ang);
    RS_Vector a2 = line->getEndpoint().rotate(center, -ang);
    RS_Vector origin = a1;
    RS_Vector dir = a2-a1;
    RS_Vector diff = origin - center;
    RS_Vector mDir = RS_Vector(dir.x/(rx*rx), dir.y/(ry*ry));
    RS_Vector mDiff = RS_Vector(diff.x/(rx*rx), diff.y/(ry*ry));

    double a = RS_Vector::dotP(dir, mDir);
    double b = RS_Vector::dotP(dir, mDiff);
    double c = RS_Vector::dotP(diff, mDiff) - 1.0;
    double d = b*b - a*c;

    if (d < 0) {
        RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 0");
    } else if ( d > 0 ) {
        double root = sqrt(d);
        double t_a = (-b - root) / a;
        double t_b = (-b + root) / a;

        /*if ( (t_a < 0 || 1 < t_a) && (t_b < 0 || 1 < t_b) ) {
            if ( (t_a < 0 && t_b < 0) || (t_a > 1 && t_b > 1) ) {
                RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 1");
            }
            else {
                RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: inside 1");
            }
        } else {*/
            RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: intersection 1");
            RS_Vector ret1(false);
            RS_Vector ret2(false);
            //if ( 0 <= t_a && t_a <= 1 ) { 
                //RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: 0<=t_a<=1");
                ret1 = a1.lerp(a2, t_a);
                RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: ret1: %f/%f", ret1.x, ret1.y);
            //}
            //if ( 0 <= t_b && t_b <= 1 ) {
                //RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: 0<=t_b<=1");
                ret2 = a1.lerp(a2, t_b);
                RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: ret2: %f/%f", ret2.x, ret2.y);
            //}
            if (ret1.valid && ret2.valid) {
                ret = RS_VectorSolutions(ret1, ret2);
            }
            else {
                if (ret1.valid) {
                    ret = RS_VectorSolutions(ret1);
                }
                if (ret2.valid) {
                    ret = RS_VectorSolutions(ret2);
                }
            }
        //}
    } else {
        double t = -b/a;
        if ( 0 <= t && t <= 1 ) {
            RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: 0<=t<=1");
            RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: intersection 2");
            ret = RS_VectorSolutions(a1.lerp(a2, t));
            RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: ret1: %f/%f", ret.get(0).x, ret.get(0).y);
        } else {
            RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 2");
        }
    }

    ret.rotate(center, ang);
    return ret;



    /*
    RS_Arc* arc = new RS_Arc(NULL,
                             RS_ArcData(ellipse->getCenter(),
                                        ellipse->getMajorRadius(),
                                        ellipse->getAngle1(),
                                        ellipse->getAngle2(),
                                        false));
    RS_Line* other = (RS_Line*)line->clone();
    double angle = ellipse->getAngle();
    //double ratio = ellipse->getRatio();

    // rotate entities:
    other->rotate(ellipse->getCenter(), -angle);
    other->scale(ellipse->getCenter(), RS_Vector(1.0, 1.0/ellipse->getRatio()));

    ret = getIntersectionLineArc(other, arc);

    ret.scale(ellipse->getCenter(), RS_Vector(1.0, ellipse->getRatio()));
    ret.rotate(ellipse->getCenter(), angle);

    delete arc;
    delete other;

    return ret;
    */
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
bool RS_Information::isPointInsideContour(const RS_Vector& point,
        RS_EntityContainer* contour, bool* onContour) {

    if (contour==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Information::isPointInsideContour: contour is NULL");
        return false;
    }

    if (point.x < contour->getMin().x || point.x > contour->getMax().x ||
            point.y < contour->getMin().y || point.y > contour->getMax().y) {
        return false;
    }

    double width = contour->getSize().x+1.0;

    bool sure;
    int counter;
    int tries = 0;
    double rayAngle = 0.0;
    do {
        sure = true;

        // create ray:
        RS_Vector v;
        v.setPolar(width*10.0, rayAngle);
        RS_Line ray(NULL, RS_LineData(point, point+v));
        counter = 0;
        RS_VectorSolutions sol;

        if (onContour!=NULL) {
            *onContour = false;
        }

        for (RS_Entity* e = contour->firstEntity(RS2::ResolveAll);
                e!=NULL;
                e = contour->nextEntity(RS2::ResolveAll)) {

            // intersection(s) from ray with contour entity:
            sol = RS_Information::getIntersection(&ray, e, true);

            for (int i=0; i<=1; ++i) {
                RS_Vector p = sol.get(i);

                if (p.valid) {
                    // point is on the contour itself
                    if (p.distanceTo(point)<1.0e-5) {
                        if (onContour!=NULL) {
                            *onContour = true;
                        }
                    } else {
                        if (e->rtti()==RS2::EntityLine) {
                            RS_Line* line = (RS_Line*)e;

                            // ray goes through startpoint of line:
                            if (p.distanceTo(line->getStartpoint())<1.0e-4) {
                                if (RS_Math::correctAngle(line->getAngle1())<M_PI) {
                                    counter++;
                                    sure = false;
                                }
                            }

                            // ray goes through endpoint of line:
                            else if (p.distanceTo(line->getEndpoint())<1.0e-4) {
                                if (RS_Math::correctAngle(line->getAngle2())<M_PI) {
                                    counter++;
                                    sure = false;
                                }
                            }
                            // ray goes through the line:


                            else {
                                counter++;
                            }
                        } else if (e->rtti()==RS2::EntityArc) {
                            RS_Arc* arc = (RS_Arc*)e;

                            if (p.distanceTo(arc->getStartpoint())<1.0e-4) {
                                double dir = arc->getDirection1();
                                if ((dir<M_PI && dir>=1.0e-5) ||
                                        ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                         arc->getCenter().y>p.y)) {
                                    counter++;
                                    sure = false;
                                }
                            }
                            else if (p.distanceTo(arc->getEndpoint())<1.0e-4) {
                                double dir = arc->getDirection2();
                                if ((dir<M_PI && dir>=1.0e-5) ||
                                        ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                         arc->getCenter().y>p.y)) {
                                    counter++;
                                    sure = false;
                                }
                            } else {
                                counter++;
                            }
                        } else if (e->rtti()==RS2::EntityCircle) {
                            // tangent:
                            if (i==0 && sol.get(1).valid==false) {
                                if (!sol.isTangent()) {
                                    counter++;
                                } else {
                                    sure = false;
                                }
                            } else if (i==1 || sol.get(1).valid==true) {
                                counter++;
                            }
                        }
                    }
                }
            }
        }

        rayAngle+=0.02;
        tries++;
    }
    while (!sure && rayAngle<2*M_PI && tries<6);

    // remove double intersections:
    /*
       QList<RS_Vector> is2;
       bool done;
    RS_Vector* av;
       do {
           done = true;
           double minDist = RS_MAXDOUBLE;
           double dist;
    	av = NULL;
           for (RS_Vector* v = is.first(); v!=NULL; v = is.next()) {
               dist = point.distanceTo(*v);
               if (dist<minDist) {
                   minDist = dist;
                   done = false;
    			av = v;
               }
           }

    	if (!done && av!=NULL) {
    		is2.append(*av);
    	}
       } while (!done);
    */

    return ((counter%2)==1);
}
	
