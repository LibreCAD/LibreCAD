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
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_constructionline.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_line.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_insert.h"
#include "rs_image.h"
#include "lc_hyperbola.h"
#include "lc_splinepoints.h"
#include "rs_modification.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "lc_undosection.h"

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
                                              RS_Entity* e) {
	if (!e) {
		return nullptr;
    }

    double dist;

    if (e->rtti()==RS2::EntityLine) {
        RS_Line* l = (RS_Line*)e;
		RS_ConstructionLine cl(nullptr,
                               RS_ConstructionLineData(l->getStartpoint(),
                                                       l->getEndpoint()));
        dist = cl.getDistanceToPoint(coord);
    } else {
        dist = e->getDistanceToPoint(coord);
    }

    if (dist<RS_MAXDOUBLE) {
        return createParallel(coord, dist, number, e);
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
                                       RS_Entity* e) {
	if (!e) {
		return nullptr;
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
                                         RS_Line* e) {

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
            if (dist1<dist2) {
                parallelData = parallel1.getData();
            } else {
                parallelData = parallel2.getData();
            }

			RS_Line* newLine = new RS_Line{container, parallelData};
			if (!ret) {
				ret = newLine;
			}
			setEntity(newLine);
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
            RS_Arc* newArc = new RS_Arc(container, parallelData);
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
            RS_Circle* newCircle = new RS_Circle(container, parallelData);
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

		RS_Line* newLine = new RS_Line{container, inters, inters + v};
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
                                        RS_Entity* circle) {
	RS_Line* ret = nullptr;

    // check given entities:
	if (!(circle && normal))
		return ret;
	if (!circle->isArc())
		return ret;
    //if( normal->getLength()<RS_TOLERANCE) return ret;//line too short
	RS_Vector const& t0 = circle->getNearestOrthTan(coord,*normal,false);
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
                                     RS_Entity* circle) {
	RS_Line* ret = nullptr;
    //RS_Vector circleCenter;

    // check given entities:
	if (!(circle && point.valid)) return nullptr;
	if (!(circle->isArc() || circle->rtti()==RS2::EntitySplinePoints)){
		return nullptr;
	}

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
RS_Line* RS_Creation::createTangent2(const RS_Vector& coord,
                                     RS_Entity* circle1,
                                     RS_Entity* circle2) {
	RS_Line* ret = nullptr;
    RS_Vector circleCenter1;
    RS_Vector circleCenter2;
    double circleRadius1 = 0.0;
    double circleRadius2 = 0.0;

    // check given entities:
	if(! (circle1 && circle2))
		return nullptr;
	if( !(circle1->isArc() && circle2->isArc()))
		return nullptr;

	std::vector<RS_Line*> poss;
    //        for (int i=0; i<4; ++i) {
	//            poss[i] = nullptr;
    //        }
    RS_LineData d;
    if( circle1->rtti() == RS2::EntityEllipse) {
        std::swap(circle1,circle2);//move Ellipse to the second place
    }
    circleCenter1=circle1->getCenter();
    circleRadius1=circle1->getRadius();
    circleCenter2=circle2->getCenter();
    circleRadius2=circle2->getRadius();
    if(circle2->rtti() != RS2::EntityEllipse) {
        //no ellipse

        // create all possible tangents:

        double angle1 = circleCenter1.angleTo(circleCenter2);
        double dist1 = circleCenter1.distanceTo(circleCenter2);

        if (dist1>1.0e-6) {
            // outer tangents:
            double dist2 = circleRadius2 - circleRadius1;
            if (dist1>dist2) {
                double angle2 = asin(dist2/dist1);
				double angt1 = angle1 + angle2 + M_PI_2;
				double angt2 = angle1 - angle2 - M_PI_2;
				RS_Vector offs1 = RS_Vector::polar(circleRadius1, angt1);
				RS_Vector offs2 = RS_Vector::polar(circleRadius2, angt1);

				poss.push_back( new RS_Line{circleCenter1 + offs1,
													  circleCenter2 + offs2});


                offs1.setPolar(circleRadius1, angt2);
                offs2.setPolar(circleRadius2, angt2);

				poss.push_back( new RS_Line{circleCenter1 + offs1,
													  circleCenter2 + offs2});
            }

            // inner tangents:
            double dist3 = circleRadius2 + circleRadius1;
            if (dist1>dist3) {
                double angle3 = asin(dist3/dist1);
				double angt3 = angle1 + angle3 + M_PI_2;
				double angt4 = angle1 - angle3 - M_PI_2;
                RS_Vector offs1;
                RS_Vector offs2;

                offs1.setPolar(circleRadius1, angt3);
                offs2.setPolar(circleRadius2, angt3);

				poss.push_back( new RS_Line{circleCenter1 - offs1,
													  circleCenter2 + offs2});


                offs1.setPolar(circleRadius1, angt4);
                offs2.setPolar(circleRadius2, angt4);

				poss.push_back( new RS_Line{circleCenter1 - offs1,
													  circleCenter2 + offs2});
            }

        }
    }else{
        //circle2 is Ellipse
		std::unique_ptr<RS_Ellipse> e2((RS_Ellipse*)circle2->clone());
//        RS_Ellipse* e2=new RS_Ellipse(nullptr,RS_EllipseData(RS_Vector(4.,1.),RS_Vector(2.,0.),0.5,0.,0.,false));
//        RS_Ellipse  e3(nullptr,RS_EllipseData(RS_Vector(4.,1.),RS_Vector(2.,0.),0.5,0.,0.,false));
//        RS_Ellipse* circle1=new RS_Ellipse(nullptr,RS_EllipseData(RS_Vector(0.,0.),RS_Vector(1.,0.),1.,0.,0.,false));
        RS_Vector m0(circle1->getCenter());
//        std::cout<<"translation: "<<-m0<<std::endl;
        e2->move(-m0); //circle1 centered at origin

        double a,b;
        double a0(0.);
        if(circle1->rtti() != RS2::EntityEllipse){//circle1 is either arc or circle
            a=fabs(circle1->getRadius());
            b=a;
			if(fabs(a)<RS_TOLERANCE) return nullptr;
        }else{//circle1 is ellipse
            RS_Ellipse* e1=static_cast<RS_Ellipse*>(circle1);
            a0=e1->getAngle();
//            std::cout<<"rotation: "<<-a0<<std::endl;
            e2->rotate(-a0);//e1 major axis along x-axis
            a=e1->getMajorRadius();
            b=e1->getRatio()*a;
			if(fabs(a)<RS_TOLERANCE || fabs(b)<RS_TOLERANCE) return nullptr;
        }
        RS_Vector factor1(1./a,1./b);
//        std::cout<<"scaling: factor1="<<factor1<<std::endl;
        e2->scale(RS_Vector(0.,0.),factor1);//circle1 is a unit circle
        factor1.set(a,b);
        double a2(e2->getAngle());
//        std::cout<<"rotation: a2="<<-a2<<std::endl;
        e2->rotate(-a2); //ellipse2 with major axis in x-axis direction
        a=e2->getMajorP().x;
        b=a*e2->getRatio();
        RS_Vector v(e2->getCenter());
//        std::cout<<"Center: (x,y)="<<v<<std::endl;


        std::vector<double> m(0,0.);
        m.push_back(1./(a*a)); //ma000
        m.push_back(1./(b*b)); //ma000
        m.push_back(v.y*v.y-1.); //ma100
        m.push_back(v.x*v.y); //ma101
        m.push_back(v.x*v.x-1.); //ma111
        m.push_back(2.*a*b*v.y); //mb10
        m.push_back(2.*a*b*v.x); //mb11
        m.push_back(a*a*b*b); //mc1

		auto vs0=RS_Math::simultaneousQuadraticSolver(m); //to hold solutions
		if (vs0.getNumber()<1) return nullptr;
//        for(size_t i=0;i<vs0.getNumber();i++){
		for(RS_Vector vpec: vs0){
			RS_Vector vpe2(e2->getCenter()+
						   RS_Vector(vpec.y/e2->getRatio(),vpec.x*e2->getRatio()));
            vpec.x *= -1.;//direction vector of tangent
            RS_Vector vpe1(vpe2 - vpec*(RS_Vector::dotP(vpec,vpe2)/vpec.squared()));
//            std::cout<<"vpe1.squared()="<<vpe1.squared()<<std::endl;
			RS_Line *l=new RS_Line{vpe1, vpe2};
            l->rotate(a2);
            l->scale(factor1);
            l->rotate(a0);
            l->move(m0);
            poss.push_back(l);

        }
        //debugging

    }
    // find closest tangent:
	if(poss.size()<1) return nullptr;
    double minDist = RS_MAXDOUBLE;
    double dist;
    int idx = -1;
	for (size_t i=0; i<poss.size(); ++i) {
		if (poss[i]) {
            poss[i]->getNearestPointOnEntity(coord,false,&dist);
//        std::cout<<poss.size()<<": i="<<i<<" dist="<<dist<<"\n";
            if (dist<minDist) {
                minDist = dist;
                idx = i;
            }
        }
    }
//idx=static_cast<int>(poss.size()*(random()/(double(1.0)+RAND_MAX)));
    if (idx!=-1) {
        RS_LineData d = poss[idx]->getData();
		for(auto p: poss){
			if(p)
				delete p;
		}

        LC_UndoSection undo( document, handleUndo);
        ret = new RS_Line{container, d};
		setEntity(ret);
    } else {
		ret = nullptr;
    }

    return ret;
}

/**
  * create the path of centers of common tangent circles of the two given circles
  *@ return nullptr, if failed
  *@ at success return either an ellipse or hyperbola
  */
 std::vector<RS_Entity*> RS_Creation::createCircleTangent2( RS_Entity* circle1,RS_Entity* circle2)
 {
	std::vector<RS_Entity*> ret(0, nullptr);
	if (!(circle1 && circle2)) return ret;
	RS_Entity* e1=circle1;
	RS_Entity* e2=circle2;

	if (e1->getRadius() < e2->getRadius()) std::swap(e1,e2);

	RS_Vector center1=e1->getCenter();
	RS_Vector center2=e2->getCenter();
	RS_Vector cp=(center1+center2)*0.5;
    double dist=center1.distanceTo(center2);
    if(dist<RS_TOLERANCE) return ret;
	RS_Vector vp= center1 - cp;
     double c=dist/(e1->getRadius()+e2->getRadius());
	 if( c < 1. - RS_TOLERANCE) {
		 //two circles intersection or one circle in the other, there's an ellipse path
		 ret.push_back(
					 new RS_Ellipse(nullptr,
									{cp, vp, sqrt(1. - c*c), 0., 0., false}
					 ));
	 }
    if( dist + e2 ->getRadius() < e1->getRadius() +RS_TOLERANCE ) {
        //one circle inside of another, the path is an ellipse
        return ret;
    }
    if(c > 1. + RS_TOLERANCE) {
        //not circle in circle, there's a hyperbola path
    c= (e1->getRadius()  - e2->getRadius())/dist;
	ret.push_back(new LC_Hyperbola(nullptr, LC_HyperbolaData(cp,vp*c,sqrt(1. - c*c),0.,0.,false)));
    return ret;
}
	ret.push_back(new RS_Line{cp, {cp.x - vp.y, cp.y+vp.x}});
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
    //RS_ConstructionLineData(coord-v1, coord+v1);

    LC_UndoSection undo( document, handleUndo);
    RS_Line* ret = new RS_Line{container, coord-v1, coord+v1};
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

		RS_Line* line = new RS_Line{container, c0, c1};
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

		RS_Line* line = new RS_Line{container, c0, c1};
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

        RS_Line* line = new RS_Line{container, c0, c1};
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
    RS_Insert* ins = new RS_Insert(container, *pdata);
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
    RS_Image* img = new RS_Image(container, *data);
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
    LC_UndoSection undo( document, remove);
    RS_Block* block =
            new RS_Block(container,
						 RS_BlockData(*data));

	// copy entities into a block
	for(auto e: *container){
        //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

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
                        "RS_Creation::createLibraryInsert: Cannot open file: %s");
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


// EOF
