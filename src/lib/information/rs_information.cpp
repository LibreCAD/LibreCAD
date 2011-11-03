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

    if (e1==NULL || e2==NULL ) {
        RS_DEBUG->print("RS_Information::getIntersection() for NULL entities");
        return ret;
    }
    if (e1->getId() == e2->getId()) {
        RS_DEBUG->print("RS_Information::getIntersection() of the same entity");
        return ret;
    }

    // unsupported entities / entity combinations:
    if (
        e1->rtti()==RS2::EntityText || e2->rtti()==RS2::EntityText ||
        isDimension(e1->rtti()) || isDimension(e2->rtti())) {
        return ret;
    }
    // a little check to avoid doing unneeded intersections, an attempt to avoid O(N^2) increasing of checking two-entity information
    if (onEntities
            && (
                e1 -> getMin().x > e2 -> getMax().x
                || e1 -> getMax().x < e2 -> getMin().x
                || e1 -> getMin().y > e2 -> getMax().y
                || e1 -> getMax().y < e2 -> getMin().y
                )
            ) {
            return ret;
    }

    // one entity is an ellipse:
    if (e1->rtti()==RS2::EntityEllipse || e2->rtti()==RS2::EntityEllipse) {
        if (e2->rtti()==RS2::EntityEllipse) std::swap( e1, e2);
        if (e2->rtti()==RS2::EntityEllipse) {
            ret = getIntersectionEllipseEllipse((RS_Ellipse*)e1, (RS_Ellipse *) e2);
        }
        if (e2->rtti()==RS2::EntityCircle) {
            ret = getIntersectionCircleEllipse((RS_Circle *)e2, (RS_Ellipse *) e1);
        }
        if (e2->rtti()==RS2::EntityArc) {
            ret = getIntersectionArcEllipse((RS_Arc *)e2, (RS_Ellipse *) e1);
        }
        if (e2->rtti()==RS2::EntityLine) {
            ret = getIntersectionEllipseLine((RS_Line*)e2, (RS_Ellipse*) e1);
//            tol = 1.0e-1;
        }

        // not supported:
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
            RS_Line * line1=(RS_Line*) te1;
            RS_Line * line2=(RS_Line*) te2;
            /* ToDo: 24 Aug 2011, Dongxu Li, if rtti() is not defined for the parent, the following check for splines may still cause segfault */
            if ( line1->getParent() != NULL && line1->getParent() == line2->getParent()) {
                if ( line1->getParent()->rtti()==RS2::EntitySpline ) {
                    //do not calculate intersections from neighboring lines of a spline
                    if ( abs(line1->getParent()->findEntity(line1) - line1->getParent()->findEntity(line2)) <= 1 ) {
                        return ret;
                    }
                }
            }

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
            // ellipse / ellipse
            //
        } else {
            RS_DEBUG->print("RS_Information::getIntersection:: Unsupported entity type.");
        }

    }


    // Check all intersection points for being on entities:
    //
    RS_VectorSolutions ret2;
    for(int i=0;i<ret.getNumber();i++) {
        if ( ! ret.get(i).valid) continue;
        if (onEntities==true) {
                //ignore intersections not on entity
            if (!(e1->isPointOnEntity(ret.get(i), tol) &&
                  e2->isPointOnEntity(ret.get(i), tol))) {
//                std::cout<<"Ignored intersection "<<ret.get(i)<<std::endl;
//                std::cout<<"because: e1->isPointOnEntity(ret.get(i), tol)="<<e1->isPointOnEntity(ret.get(i), tol)
//                    <<"\t(e2->isPointOnEntity(ret.get(i), tol)="<<e2->isPointOnEntity(ret.get(i), tol)<<std::endl;
                continue;
            }
        }
        ret2.push_back(ret.get(i));
    }

    return ret2;
}



/**
 * @return Intersection between two lines.
 */
RS_VectorSolutions RS_Information::getIntersectionLineLine(RS_Line* e1,
        RS_Line* e2) {

    RS_VectorSolutions ret;

    if (e1==NULL || e2==NULL) {
        RS_DEBUG->print("RS_Information::getIntersectionLineLin() for NULL entities");
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

// find intersections between ellipse/arc/circle using quartic equation solver
//
// @author Dongxu Li <dongxuli2011@gmail.com>
//

RS_VectorSolutions RS_Information::getIntersectionEllipseEllipse(RS_Ellipse* e1, RS_Ellipse* e2) {
    RS_VectorSolutions ret;

    if (e1==NULL || e2==NULL ) {
        return ret;
    }
    if (
        (e1->getCenter() - e2 ->getCenter()).squared() < RS_TOLERANCE*RS_TOLERANCE &&
        (e1->getMajorP() - e2 ->getMajorP()).squared() < RS_TOLERANCE*RS_TOLERANCE &&
        fabs(e1->getRatio() - e2 ->getRatio()) < RS_TOLERANCE
    ) { // overlapped ellipses, do not do overlap
        return ret;
    }
    RS_Ellipse ellipse01(NULL,e1->getData());

    RS_Ellipse *e01= & ellipse01;
    if( e01->getMajorRadius() < e01->getMinorRadius() ) e01->switchMajorMinor();
    RS_Ellipse ellipse02(NULL,e2->getData());
    RS_Ellipse *e02= &ellipse02;
    if( e02->getMajorRadius() < e02->getMinorRadius() ) e02->switchMajorMinor();
    //transform ellipse2 to ellipse1's coordinates
    RS_Vector shiftc1=- e01->getCenter();
    double shifta1=-e01->getAngle();
    e02->move(shiftc1);
    e02->rotate(shifta1);
    RS_Vector majorP2=e02->getMajorP();
    double a1=e01->getMajorRadius();
    double b1=e01->getMinorRadius();
    double x2=e02->getCenter().x,
           y2=e02->getCenter().y;
    double a2=e02->getMajorRadius();
    double b2=e02->getMinorRadius();

    if( e01->getMinorRadius() < RS_TOLERANCE || e01 -> getRatio()< RS_TOLERANCE) {
        // treate e01 as a line
        RS_LineData ldata0(RS_Vector(-a1,0.),RS_Vector(a1,0.));
        RS_Line *l0=new RS_Line(e1->getParent(),ldata0);
        ret= getIntersectionEllipseLine(l0, e02);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }
    if( e02->getMinorRadius() < RS_TOLERANCE || e02 -> getRatio()< RS_TOLERANCE) {
        // treate e02 as a line
        RS_LineData ldata0(RS_Vector(-a2,0.),RS_Vector(a2,0.));
        RS_Line *l0=new RS_Line(e1->getParent(),ldata0);
        l0->rotate(RS_Vector(0.,0.),e02->getAngle());
        l0->move(e02->getCenter());
        ret= getIntersectionEllipseLine(l0, e01);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }

    //ellipse01 equation:
    //	x^2/(a1^2) + y^2/(b1^2) - 1 =0
    double t2= - e02->getAngle();
    //ellipse2 equation:
    // ( (x - u) cos(t) - (y - v) sin(t))^2/a^2 + ( (x - u) sin(t) + (y-v) cos(t))^2/b^2 =1
    // ( cos^2/a^2 + sin^2/b^2) x^2 +
    // ( sin^2/a^2 + cos^2/b^2) y^2 +
    //  2 sin cos (1/b^2 - 1/a^2) x y +
    //  ( ( 2 v sin cos - 2 u cos^2)/a^2 - ( 2v sin cos + 2 u sin^2)/b^2) x +
    //  ( ( 2 u sin cos - 2 v sin^2)/a^2 - ( 2u sin cos + 2 v cos^2)/b^2) y +
    //  (u cos - v sin)^2/a^2 + (u sin + v cos)^2/b^2 -1 =0
    // detect whether any ellipse radius is zero
    double cs=cos(t2),si=sin(t2);
    double ucs=x2*cs,usi=x2*si,
           vcs=y2*cs,vsi=y2*si;
    double cs2=cs*cs,si2=1-cs2;
    double tcssi=2.*cs*si;
    double ia2=1./(a2*a2),ib2=1./(b2*b2);
//    std::cout<<"e1: x^2/("<<a1<<")^2+y^2/("<<b1<<")^2-1 =0\n";
//    std::cout<<"e2: ( (x-("<<x2<<"))*("<<cs<<")-(y-("<<y2<<"))*("<<si<<"))^2/"<<a2<<"^2+( ( x - ("<<x2<<"))*("<<si<<")+(y-("<<y2<<"))*("<<cs<<"))^2/"<<b2<<"^2 -1 =0\n";
    double mc1=(ucs - vsi)*(ucs-vsi)*ia2+(usi+vcs)*(usi+vcs)*ib2 -1.;
    double mb10= ( y2*tcssi - 2.*x2*cs2)*ia2 - ( y2*tcssi+2*x2*si2)*ib2; //x
    double mb11= ( x2*tcssi - 2.*y2*si2)*ia2 - ( x2*tcssi+2*y2*cs2)*ib2; //y
    double ma100= cs2*ia2 + si2*ib2; // x^2
    double ma101= cs*si*(ib2 - ia2); // xy term is 2*ma101*x*y
    double ma111= si2*ia2 + cs2*ib2; // y^2
    double ma000= 1./(a1*a1),ma011=1./(b1*b1);
//    std::cout<<"simplified e1: "<<ma000<<"*x^2 + "<<ma011<<"*y^2 -1 =0\n";
//    std::cout<<"simplified e2: "<<ma100<<"*x^2 + 2*("<<ma101<<")*x*y + "<<ma111<<"*y^2 "<<" + ("<<mb10<<")*x + ("<<mb11<<")*y + ("<<mc1<<") =0\n";
    // construct the Bezout determinant
    double v0=2.*ma000*ma101;
    double v2=ma000*mb10;
    double v3=ma000*mb11;
    double v4=ma000*mc1+ma100;
    //double v5= 2.*ma101*ma011;
    //double v6= ma000*ma111;
    //double v7= 2.*ma101;
    double v8= 2.*ma011*mb10;
    //double v9= ma100*ma011;
    double v1=ma000*ma111-ma100*ma011;
    //double v1= v6 - v9;
    double u0 = v4*v4-v2*mb10;
    double u1 = 2.*(v3*v4-v0*mb10);
    double u2 = 2.*(v4*v1-ma101*v0)+v3*v3+0.5*v2*v8;
    double u3 = v0*v8+2.*v3*v1;
    double u4 = v1*v1+2.*ma101*ma011*v0;
    //std::cout<<"u0="<<u0<<"\tu1="<<u1<<"\tu2="<<u2<<"\tu3="<<u3<<"\tu4="<<u4<<std::endl;
    //std::cout<<"("<<u4<<")*x^4+("<<u3<<")*x^3+("<<u2<<")*x^2+("<<u1<<")*x+("<<u0<<")=0\n";
    double ce[4];
    double roots[4];
    unsigned int counts=0;
    if ( fabs(u4) < 1.0e-75) { // this should not happen
        if ( fabs(u3) < 1.0e-75) { // this should not happen
            if ( fabs(u2) < 1.0e-75) { // this should not happen
                if( fabs(u1) > 1.0e-75) {
                    counts=1;
                    roots[0]=-u0/u1;
                } else { // can not determine y. this means overlapped, but overlap should have been detected before, therefore return empty set
                    return ret;
                }
            } else {
                ce[0]=u1/u2;
                ce[1]=u0/u2;
                //std::cout<<"ce[2]={ "<<ce[0]<<' '<<ce[1]<<" }\n";
                counts=RS_Math::quadraticSolver(ce,roots);
            }
        } else {
            ce[0]=u2/u3;
            ce[1]=u1/u3;
            ce[2]=u0/u3;
            //std::cout<<"ce[3]={ "<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<" }\n";
            counts=RS_Math::cubicSolver(ce,roots);
        }
    } else {
        ce[0]=u3/u4;
        ce[1]=u2/u4;
        ce[2]=u1/u4;
        ce[3]=u0/u4;
        //std::cout<<"ce[4]={ "<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<' '<<ce[3]<<" }\n";
        counts=RS_Math::quarticSolver(ce,roots);
    }
//	std::cout<<"Equation for y: y^4";
//        for(int i=3; i>=0; i--) {
//		std::cout<<"+("<<ce[3-i]<<")";
//	    if ( i ) {
//		    std::cout<<"*y^"<<i;
//	    }else {
//		    std::cout<<" ==0\n";
//	    }
//    }

    if (! counts ) { // no intersection found
        return ret;
    }
//      std::cout<<"counts="<<counts<<": ";
//	for(unsigned int i=0;i<counts;i++){
//	std::cout<<roots[i]<<" ";
//	}
//	std::cout<<std::endl;
    RS_VectorSolutions vs0;
    unsigned int ivs0=0;
    for(unsigned int i=0; i<counts; i++) {
        double y=roots[i];
        //double x=(ma100*(ma011*y*y-1.)-ma000*(ma111*y*y+mb11*y+mc1))/(ma000*(2.*ma101*y+mb11));
        double x,d=v0*y+v2;
//        std::cout<<"d= "<<d<<std::endl;
        if( fabs(d)>10.*RS_TOLERANCE*sqrt(RS_TOLERANCE)) {//whether there's x^1 term in bezout determinant
            x=-((v1*y+v3)*y+v4 )/d;
            if(vs0.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                vs0.push_back(RS_Vector(x,y));
        } else { // no x^1 term, have to use x^2 term, then, have to check plus/minus sqrt
            x=a1*sqrt(1-y*y*ma011);
            if(vs0.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                vs0.push_back(RS_Vector(x,y));
            x=-x;
            if(vs0.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                vs0.push_back(RS_Vector(x,y));
        }
        //std::cout<<"eq1="<<ma000*x*x+ma011*y*y-1.<<std::endl;
        //std::cout<<"eq2="<<ma100*x*x + 2.*ma101*x*y+ma111*y*y+mb10*x+mb11*y+mc1<<std::endl;
//            if (
//                fabs(ma100*x*x + 2.*ma101*x*y+ma111*y*y+mb10*x+mb11*y+mc1)< RS_TOLERANCE
//            ) {//found
//                vs0.set(ivs0++, RS_Vector(x,y));
//            }
    }
//    for(unsigned int j=0; j<vs0.getNumber(); j++) {
//        std::cout<<" ( "<<vs0.get(j).x<<" , "<<vs0.get(j).y<<" ) ";
//    }
//    std::cout<<std::endl;
//    std::cout<<"counts= "<<counts<<"\tFound "<<ivs0<<" EllipseEllipse intersections\n";
    //ret.alloc(ivs0);
    shifta1 = - shifta1;
    shiftc1 = - shiftc1;
    for(int i=0; i<vs0.getNumber(); i++) {
        RS_Vector vp=vs0.get(i);
        vp.rotate(shifta1);
        vp.move(shiftc1);
        ret.push_back(vp);
    }
    return ret;
}

//wrapper to do Circle-Ellipse and Arc-Ellipse using Ellipse-Ellipse intersection
RS_VectorSolutions RS_Information::getIntersectionCircleEllipse(RS_Circle* c1,
        RS_Ellipse* e1) {
    RS_VectorSolutions ret;
    if (c1==NULL || e1==NULL) {
        return ret;
    }

    RS_EllipseData d(
        c1->getCenter(),
        RS_Vector(c1->getRadius(),0.),
        1.0,
        0.,
        2.*M_PI,
        false);
    RS_Ellipse * e2= new RS_Ellipse(c1->getParent(),d);
    return getIntersectionEllipseEllipse(e1,e2);
}

RS_VectorSolutions RS_Information::getIntersectionArcEllipse(RS_Arc * a1,
        RS_Ellipse* e1) {
    RS_VectorSolutions ret;
    if (a1==NULL || e1==NULL) {
        return ret;
    }
    RS_EllipseData d(
        a1->getCenter(),
        RS_Vector(a1->getRadius(),0.),
        1.0,
        a1->getAngle1(),
        a1->getAngle2(),
        a1->isReversed());
    RS_Ellipse * e2= new RS_Ellipse(a1->getParent(),d);
    return getIntersectionEllipseEllipse(e1,e2);
}





/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionEllipseLine(RS_Line* line,
        RS_Ellipse* ellipse) {

    RS_VectorSolutions ret;

    if (line==NULL || ellipse==NULL) {
        return ret;
    }

    // rotate into normal position:

    double rx = ellipse->getMajorRadius();
    if(rx<RS_TOLERANCE) {
        //zero radius ellipse
        RS_Vector vp(line->getNearestPointOnEntity(ellipse->getCenter(), true));
        if((vp-ellipse->getCenter()).squared() <RS_TOLERANCE*RS_TOLERANCE){
            //center on line
            ret.push_back(vp);
        }
        return ret;
    }
    RS_Vector angleVector = ellipse->getMajorP().scale(RS_Vector(1./rx,-1./rx));
    double ry = rx*ellipse->getRatio();
    RS_Vector center = ellipse->getCenter();
    RS_Vector a1 = line->getStartpoint().rotate(center, angleVector);
    RS_Vector a2 = line->getEndpoint().rotate(center, angleVector);
//    RS_Vector origin = a1;
    RS_Vector dir = a2-a1;
    RS_Vector diff = a1 - center;
    RS_Vector mDir = RS_Vector(dir.x/(rx*rx), dir.y/(ry*ry));
    RS_Vector mDiff = RS_Vector(diff.x/(rx*rx), diff.y/(ry*ry));

    double a = RS_Vector::dotP(dir, mDir);
    double b = RS_Vector::dotP(dir, mDiff);
    double c = RS_Vector::dotP(diff, mDiff) - 1.0;
    double d = b*b - a*c;

//    std::cout<<"RS_Information::getIntersectionEllipseLine(): d="<<d<<std::endl;
    if (d < - 100.*RS_TOLERANCE*sqrt(RS_TOLERANCE)) {
        RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 0");
        return ret;
    }
    if( d < 0. ) d=0.;
    double root = sqrt(d);
    double t_a = -b/a;
    double t_b = root/a;
    //        double t_b = (-b + root) / a;

    ret.push_back(a1.lerp(a2,t_a+t_b));
    RS_Vector vp(a1.lerp(a2,t_a-t_b));
    if ( (ret.get(0)-vp).squared()>RS_TOLERANCE*RS_TOLERANCE) {
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

