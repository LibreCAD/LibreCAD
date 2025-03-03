/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2024 LibreCAD.org
** Copyright (C) 2015-2024 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include <algorithm>
#include <cfloat>
#include <numeric>
#include <QDebug>
#include "rs_math.h"
#include "rs_information.h"
#include "lc_quadratic.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_debug.h"

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif

/**
 * Constructor.
 */

LC_Quadratic::LC_Quadratic():
    m_mQuad(2,2),
    m_vLinear(2),
	m_bValid(false)
{}

LC_Quadratic::LC_Quadratic(const LC_Quadratic& lc0):
  m_bIsQuadratic(lc0.isQuadratic())
  ,m_bValid(lc0)
{
    if(m_bValid==false) return;
  if(m_bIsQuadratic) m_mQuad=lc0.getQuad();
  m_vLinear=lc0.getLinear();
  m_dConst=lc0.m_dConst;
}

LC_Quadratic& LC_Quadratic::operator = (const LC_Quadratic& lc0)
{
    if(lc0.isQuadratic()){
        m_mQuad.resize(2,2,false);
        m_mQuad=lc0.getQuad();
    }
    m_vLinear.resize(2);
    m_vLinear=lc0.getLinear();
    m_dConst=lc0.m_dConst;
    m_bIsQuadratic=lc0.isQuadratic();
	m_bValid=lc0.m_bValid;
    return *this;
}


LC_Quadratic::LC_Quadratic(std::vector<double> ce):
    m_mQuad(2,2),
    m_vLinear(2)
{
    if(ce.size()==6){
        //quadratic
        m_mQuad(0,0)=ce[0];
        m_mQuad(0,1)=0.5*ce[1];
        m_mQuad(1,0)=m_mQuad(0,1);
        m_mQuad(1,1)=ce[2];
        m_vLinear(0)=ce[3];
        m_vLinear(1)=ce[4];
        m_dConst=ce[5];
        m_bIsQuadratic=true;
        m_bValid=true;
        return;
    }
    if(ce.size()==3){
        m_vLinear(0)=ce[0];
        m_vLinear(1)=ce[1];
        m_dConst=ce[2];
        m_bIsQuadratic=false;
        m_bValid=true;
        return;
    }
        m_bValid=false;
}

/** construct a parabola, ellipse or hyperbola as the path of center of tangent circles
  passing the point
*@circle, an entity
*@point, a point
*@return, a path of center tangential circles which pass the point
*/
LC_Quadratic::LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point)
    : m_mQuad(2,2)
    ,m_vLinear(2)
    ,m_bIsQuadratic(true)
    ,m_bValid(true)
{
	if(circle==nullptr) {
        m_bValid=false;
        return;
    }
    switch(circle->rtti()){
    case RS2::EntityArc:
    case RS2::EntityCircle:
    {//arc/circle and a point
        RS_Vector center;
        double r;

        center=circle->getCenter();
        r=circle->getRadius();
		if(center == false){
            m_bValid=false;
            return;
        }
        double c=0.5*(center.distanceTo(point));
        double d=0.5*r;
        if(std::abs(c)<RS_TOLERANCE ||std::abs(d)<RS_TOLERANCE || std::abs(c-d)<RS_TOLERANCE){
            m_bValid=false;
            return;
        }
        m_mQuad(0,0)=1./(d*d);
        m_mQuad(0,1)=0.;
        m_mQuad(1,0)=0.;
        m_mQuad(1,1)=1./(d*d - c*c);
        m_vLinear(0)=0.;
        m_vLinear(1)=0.;
        m_dConst=-1.;
        center=(center + point)*0.5;
        rotate(center.angleTo(point));
        move(center);
        return;
    }
    case RS2::EntityLine:
    {//line and a point
        const RS_Line* line=static_cast<const RS_Line*>(circle);

        RS_Vector direction=line->getEndpoint() - line->getStartpoint();
        double l2=direction.squared();
        if(l2<RS_TOLERANCE2) {
            m_bValid=false;
            return;
        }
        RS_Vector projection=line->getNearestPointOnEntity(point,false);
//        DEBUG_HEADER
//        std::cout<<"projection="<<projection<<std::endl;
        double p2=(projection-point).squared();
        if(p2<RS_TOLERANCE2) {
            //point on line, return a straight line
            m_bIsQuadratic=false;
            m_vLinear(0)=direction.y;
            m_vLinear(1)=-direction.x;
            m_dConst = direction.x*point.y-direction.y*point.x;
            return;
        }
		RS_Vector center= (projection+point)*0.5;
//        std::cout<<"point="<<point<<std::endl;
//        std::cout<<"center="<<center<<std::endl;
        double p=sqrt(p2);
        m_bIsQuadratic=true;
        m_bValid=true;
        m_mQuad(0,0)=0.;
        m_mQuad(0,1)=0.;
        m_mQuad(1,0)=0.;
        m_mQuad(1,1)=1.;
        m_vLinear(0)=-2.*p;
        m_vLinear(1)=0.;
        m_dConst=0.;
//        DEBUG_HEADER
//        std::cout<<*this<<std::endl;
//        std::cout<<"rotation by ";
//        std::cout<<"angle="<<center.angleTo(point)<<std::endl;
        rotate(center.angleTo(point));
//        std::cout<<"move by ";
//        std::cout<<"center="<<center<<std::endl;
        move(center);
//        std::cout<<*this<<std::endl;
//        std::cout<<"point="<<point<<std::endl;
//        std::cout<<"finished"<<std::endl;
        return;
    }
    default:
        m_bValid=false;
        return;
    }

}


bool LC_Quadratic::isQuadratic() const {
	return m_bIsQuadratic;
}

LC_Quadratic::operator bool() const
{
	return m_bValid;
}

bool LC_Quadratic::isValid() const
{
	return m_bValid;
}

void LC_Quadratic::setValid(bool value)
{
	m_bValid=value;
}


bool LC_Quadratic::operator == (bool valid) const
{
	return m_bValid == valid;
}

bool LC_Quadratic::operator != (bool valid) const
{
	return m_bValid != valid;
}

boost::numeric::ublas::vector<double>& LC_Quadratic::getLinear()
{
	return m_vLinear;
}

const boost::numeric::ublas::vector<double>& LC_Quadratic::getLinear() const
{
	return m_vLinear;
}

boost::numeric::ublas::matrix<double>& LC_Quadratic::getQuad()
{
	return m_mQuad;
}

const boost::numeric::ublas::matrix<double>& LC_Quadratic::getQuad() const
{
	return m_mQuad;
}

double const& LC_Quadratic::constTerm()const
{
	return m_dConst;
}

double& LC_Quadratic::constTerm()
{
	return m_dConst;
}

/** construct a ellipse or hyperbola as the path of center of common tangent circles
  of this two given entities*/
LC_Quadratic::LC_Quadratic(const RS_AtomicEntity* circle0,
                           const RS_AtomicEntity* circle1,
                           bool mirror):
    m_mQuad(2,2)
    ,m_vLinear(2)
    ,m_bValid(false)
{
//    DEBUG_HEADER

	if(!( circle0->isArcCircleLine() && circle1->isArcCircleLine())) {
		return;
	}

	if(circle1->rtti() != RS2::EntityLine)
        std::swap(circle0, circle1);
    if(circle0->rtti() == RS2::EntityLine) {
        //two lines
        RS_Line* line0=(RS_Line*) circle0;
        RS_Line* line1=(RS_Line*) circle1;

		auto centers=RS_Information::getIntersection(line0,line1);
//        DEBUG_HEADER
        if(centers.size()!=1) return;
        double angle=0.5*(line0->getAngle1()+line1->getAngle1());
        m_bValid=true;
        m_bIsQuadratic=true;
        m_mQuad(0,0)=0.;
        m_mQuad(0,1)=0.5;
        m_mQuad(1,0)=0.5;
        m_mQuad(1,1)=0.;
        m_vLinear(0)=0.;
        m_vLinear(1)=0.;
        m_dConst=0.;
        rotate(angle);
        move(centers.get(0));
//        DEBUG_HEADER
//        std::cout<<*this<<std::endl;
        return;
    }
    if(circle1->rtti() == RS2::EntityLine) {
//        DEBUG_HEADER
        //one line, one circle
        const RS_Line* line1=static_cast<const RS_Line*>(circle1);
        RS_Vector normal=line1->getNormalVector()*circle0->getRadius();
        RS_Vector disp=line1->getNearestPointOnEntity(circle0->getCenter(),
                                                           false)-circle0->getCenter();
	if(normal.dotP(disp)>0.) normal *= -1.;
    if(mirror) normal *= -1.;
							   
		RS_Line directrix{line1->getStartpoint()+normal,
										   line1->getEndpoint()+normal};
        LC_Quadratic lc0(&directrix,circle0->getCenter());
        *this = lc0;
        return;

        m_mQuad=lc0.getQuad();
        m_vLinear=lc0.getLinear();
        m_bIsQuadratic=true;
        m_bValid=true;
        m_dConst=lc0.m_dConst;

        return;
    }
    //two circles

	double const f=(circle0->getCenter()-circle1->getCenter()).magnitude()*0.5;
    double const a=std::abs(circle0->getRadius()+circle1->getRadius())*0.5;
    double const c=std::abs(circle0->getRadius()-circle1->getRadius())*0.5;
//    DEBUG_HEADER
//    qDebug()<<"circle center to center distance="<<2.*f<<"\ttotal radius="<<2.*a;
    if(a<RS_TOLERANCE) return;
	RS_Vector center=(circle0->getCenter()+circle1->getCenter())*0.5;
    double angle=center.angleTo(circle0->getCenter());
    if( f<a){
        //ellipse
		double const ratio=sqrt(a*a - f*f)/a;
		RS_Vector const& majorP=RS_Vector{angle}*a;
		RS_Ellipse const ellipse{nullptr, {center,majorP,ratio,0.,0.,false}};
		auto const& lc0=ellipse.getQuadratic();

        m_mQuad=lc0.getQuad();
        m_vLinear=lc0.getLinear();
        m_bIsQuadratic=lc0.isQuadratic();
        m_bValid=lc0.isValid();
        m_dConst=lc0.m_dConst;
//        DEBUG_HEADER
//        std::cout<<"ellipse: "<<*this;
        return;
    }

//       DEBUG_HEADER
	if(c<RS_TOLERANCE){
		//two circles are the same radius
		//degenerate hypberbola: straight lines
		//equation xy = 0
		m_bValid=true;
		m_bIsQuadratic=true;
		m_mQuad(0,0)=0.;
		m_mQuad(0,1)=0.5;
		m_mQuad(1,0)=0.5;
		m_mQuad(1,1)=0.;
		m_vLinear(0)=0.;
		m_vLinear(1)=0.;
		m_dConst=0.;
		rotate(angle);
		move(center);
		return;
	}
//hyperbola
	// equation: x^2/c^2 - y^2/(f^2 -c ^2) = 1
	// f: from hyperbola center to one circle center
	// c: half of difference of two circles

    double b2= f*f - c*c;
    m_bValid=true;
    m_bIsQuadratic=true;
	m_mQuad(0,0)=1./(c*c);
    m_mQuad(0,1)=0.;
    m_mQuad(1,0)=0.;
    m_mQuad(1,1)=-1./b2;
    m_vLinear(0)=0.;
    m_vLinear(1)=0.;
    m_dConst=-1.;
    rotate(angle);
    move(center);
    return;
}

/**
 * @brief LC_Quadratic, construct a Perpendicular bisector line, which is the path of circles passing point0 and point1
 * @param point0
 * @param point1
 */
LC_Quadratic::LC_Quadratic(const RS_Vector& point0, const RS_Vector& point1)
{
    RS_Vector vStart=(point0+point1)*0.5;
    RS_Vector vEnd=vStart + (point0-vStart).rotate(0.5*M_PI);
    *this=RS_Line(vStart, vEnd).getQuadratic();
}

std::vector<double>  LC_Quadratic::getCoefficients() const
{
    std::vector<double> ret(0,0.);
    if(isValid()==false) return ret;
    if(m_bIsQuadratic){
        ret.push_back(m_mQuad(0,0));
        ret.push_back(m_mQuad(0,1)+m_mQuad(1,0));
        ret.push_back(m_mQuad(1,1));
    }
    ret.push_back(m_vLinear(0));
    ret.push_back(m_vLinear(1));
    ret.push_back(m_dConst);
    return ret;
}

LC_Quadratic LC_Quadratic::move(const RS_Vector& v)
{
    if(m_bValid==false || v.valid == false) return *this;

    m_dConst -= m_vLinear(0) * v.x + m_vLinear(1)*v.y;

    if(m_bIsQuadratic){
        m_vLinear(0) -= 2.*m_mQuad(0,0)*v.x + (m_mQuad(0,1)+m_mQuad(1,0))*v.y;
        m_vLinear(1) -= 2.*m_mQuad(1,1)*v.y + (m_mQuad(0,1)+m_mQuad(1,0))*v.x;
        m_dConst += m_mQuad(0,0)*v.x*v.x + (m_mQuad(0,1)+m_mQuad(1,0))*v.x*v.y+ m_mQuad(1,1)*v.y*v.y ;
    }
    return *this;
}


LC_Quadratic LC_Quadratic::rotate(const double& angle)
{
    using namespace boost::numeric::ublas;
    matrix<double> m=rotationMatrix(angle);
    matrix<double> t=trans(m);
    m_vLinear = prod(t, m_vLinear);
    if(m_bIsQuadratic){
        m_mQuad=prod(m_mQuad,m);
        m_mQuad=prod(t, m_mQuad);
    }
    return *this;
}

LC_Quadratic LC_Quadratic::rotate(const RS_Vector& center, const double& angle)
{
    move(-center);
    rotate(angle);
    move(center);
    return *this;
}

/**
 * @author{Dongxu Li}
 */
LC_Quadratic LC_Quadratic::shear(double k)
{
    if(isQuadratic()){
        auto getCes = [this]() -> std::array<double, 6>{
            std::vector<double> cev = getCoefficients();
            return {cev[0], cev[1], cev[2], cev[3], cev[4], cev[5]};
        };
        const auto& [a,b,c,d,e,f] = getCes();

        const std::vector<double> sheared = {{
            a, -2.*k*a + b, k*(k*a - b) + c,
            d, e - k*d, f
        }};
        return {sheared};
    }
    LC_Quadratic qf(*this);
    qf.m_vLinear(1) -= k * qf.m_vLinear(0);
    return qf;
}

/** switch x,y coordinates */
LC_Quadratic LC_Quadratic::flipXY(void) const
{
    LC_Quadratic qf(*this);
    if(isQuadratic()){
        std::swap(qf.m_mQuad(0,0),qf.m_mQuad(1,1));
        std::swap(qf.m_mQuad(0,1),qf.m_mQuad(1,0));
    }
    std::swap(qf.m_vLinear(0),qf.m_vLinear(1));
    return qf;
}

RS_VectorSolutions LC_Quadratic::getIntersection(const LC_Quadratic& l1, const LC_Quadratic& l2)
{
    RS_VectorSolutions ret;
	if( l1 == false || l2 == false ) {
//        DEBUG_HEADER
//        std::cout<<l1<<std::endl;
//        std::cout<<l2<<std::endl;
        return ret;
    }
    auto p1=&l1;
    auto p2=&l2;
    if(p1->isQuadratic()==false){
        std::swap(p1,p2);
    }
	if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
		DEBUG_HEADER
		std::cout<<*p1<<std::endl;
		std::cout<<*p2<<std::endl;
	}
    if(p1->isQuadratic()==false){
        //two lines
		std::vector<std::vector<double> > ce(2,std::vector<double>(3,0.));
        ce[0][0]=p1->m_vLinear(0);
        ce[0][1]=p1->m_vLinear(1);
        ce[0][2]=-p1->m_dConst;
        ce[1][0]=p2->m_vLinear(0);
        ce[1][1]=p2->m_vLinear(1);
        ce[1][2]=-p2->m_dConst;
		std::vector<double> sn(2,0.);
        if(RS_Math::linearSolver(ce,sn)){
            ret.push_back(RS_Vector(sn[0],sn[1]));
        }
        return ret;
    }
    if(p2->isQuadratic()==false){
        //one line, one quadratic
        //avoid division by zero
        if(std::abs(p2->m_vLinear(0))+DBL_EPSILON<std::abs(p2->m_vLinear(1))){
            ret=getIntersection(p1->flipXY(),p2->flipXY()).flipXY();
//            for(size_t j=0;j<ret.size();j++){
//                DEBUG_HEADER
//                std::cout<<j<<": ("<<ret[j].x<<", "<< ret[j].y<<")"<<std::endl;
//            }
            return ret;
        }
        std::vector<std::vector<double> >  ce(0);
        if(std::abs(p2->m_vLinear(1))<RS_TOLERANCE){
            const double angle=0.25*M_PI;
            LC_Quadratic p11(*p1);
            LC_Quadratic p22(*p2);
            ce.push_back(p11.rotate(angle).getCoefficients());
            ce.push_back(p22.rotate(angle).getCoefficients());
            ret=RS_Math::simultaneousQuadraticSolverMixed(ce);
            ret.rotate(-angle);
//            for(size_t j=0;j<ret.size();j++){
//                DEBUG_HEADER
//                std::cout<<j<<": ("<<ret[j].x<<", "<< ret[j].y<<")"<<std::endl;
//            }
            return ret;
        }
        ce.push_back(p1->getCoefficients());
        ce.push_back(p2->getCoefficients());
        ret=RS_Math::simultaneousQuadraticSolverMixed(ce);
//        for(size_t j=0;j<ret.size();j++){
//            DEBUG_HEADER
//            std::cout<<j<<": ("<<ret[j].x<<", "<< ret[j].y<<")"<<std::endl;
//        }
        return ret;
    }
    if( std::abs(p1->m_mQuad(0,0))<RS_TOLERANCE && std::abs(p1->m_mQuad(0,1))<RS_TOLERANCE
            &&
            std::abs(p2->m_mQuad(0,0))<RS_TOLERANCE && std::abs(p2->m_mQuad(0,1))<RS_TOLERANCE
            ){
        if(std::abs(p1->m_mQuad(1,1))<RS_TOLERANCE && std::abs(p2->m_mQuad(1,1))<RS_TOLERANCE){
            //linear
            std::vector<double> ce(0);
            ce.push_back(p1->m_vLinear(0));
            ce.push_back(p1->m_vLinear(1));
            ce.push_back(p1->m_dConst);
            LC_Quadratic lc10(ce);
            ce.clear();
            ce.push_back(p2->m_vLinear(0));
            ce.push_back(p2->m_vLinear(1));
            ce.push_back(p2->m_dConst);
            LC_Quadratic lc11(ce);
            return getIntersection(lc10,lc11);
        }
        return getIntersection(p1->flipXY(),p2->flipXY()).flipXY();
    }
    std::vector<std::vector<double> >  ce = { p1->getCoefficients(),
                                              p2->getCoefficients()};
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
        DEBUG_HEADER
        std::cout<<*p1<<std::endl;
        std::cout<<*p2<<std::endl;
    }
	auto sol= RS_Math::simultaneousQuadraticSolverFull(ce);
    bool valid= sol.size()>0;
	for(auto & v: sol){
        if(v.magnitude()>=RS_MAXDOUBLE){
            valid=false;
            break;
        }
        const std::vector<double> xyi = {v.x * v.x, v.x * v.y, v.y * v.y, v.x, v.y, 1.};
        const double e0 = std::inner_product(xyi.cbegin(), xyi.cend(), ce.front().cbegin(), 0.);
        const double e1 = std::inner_product(xyi.cbegin(), xyi.cend(), ce.back().cbegin(), 0.);
        LC_LOG<<__func__<<"(): "<<v.x<<","<<v.y<<": equ0= "<<e0;
        LC_LOG<<__func__<<"(): "<<v.x<<","<<v.y<<": equ1= "<<e1;
    }
    if(valid) return sol;
    ce.clear();
	ce.push_back(p1->getCoefficients());
	ce.push_back(p2->getCoefficients());
    sol=RS_Math::simultaneousQuadraticSolverFull(ce);
    ret.clear();
	for(auto const& v: sol){
		if(v.magnitude()<=RS_MAXDOUBLE){
			ret.push_back(v);
			if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
				DEBUG_HEADER
				std::cout<<v<<std::endl;
			}
		}
	}
    return ret;
}

/**
   rotation matrix:

   cos x, sin x
   -sin x, cos x
   */
boost::numeric::ublas::matrix<double> LC_Quadratic::rotationMatrix(const double& angle)
{
    boost::numeric::ublas::matrix<double> ret(2,2);
    ret(0,0)=cos(angle);
    ret(0,1)=sin(angle);
    ret(1,0)=-ret(0,1);
    ret(1,1)=ret(0,0);
    return ret;
}

/**
 * @description: Given a general conic section with homogeneous coordinates $[a,b,c,d,e,f]$:
        $$ax^2+b x y+c y^2+d x + e y + f = 0 $$
    The dual curve is:
        $$
        \begin{split}
        (e^2-4 c f)A^2&+&(4b f-2 d e) A B&+&(d^2-4 a f)B^2 &\\
        &+&(4c d -2 b e) A C&+&(4a e - 2b d) B C&\\
        & & &+&(b^2 - 4 a c) C^2&= 0
        \end{split}
        $$
 */
LC_Quadratic LC_Quadratic::getDualCurve() const
{
    if (!isQuadratic())
        return LC_Quadratic{};
    auto getCes = [this]() -> std::array<double, 6>{
        std::vector<double> cev = getCoefficients();
        return {cev[0], cev[1], cev[2], cev[3], cev[4], cev[5]};
    };
    const auto& [a,b,c,d,e,f] = getCes();

    const std::vector<double> dualCe = {{
                                   e*e - 4.*c*f, 4.*b*f - 2.*d*e, d*d - 4.*a*f,
                                   4.*c*d-2.*b*e, 4.*a*e-2.*b*d,
                                   b*b-4.*a*c
                               }};
    return {dualCe};
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const LC_Quadratic& q) {

    os << " quadratic form: ";
	if(q == false) {
        os<<" invalid quadratic form"<<std::endl;
        return os;
    }
    os<<std::endl;
	auto ce=q.getCoefficients();
    unsigned short i=0;
    if(ce.size()==6){
        os<<ce[0]<<"*x^2 "<<( (ce[1]>=0.)?"+":" ")<<ce[1]<<"*x*y  "<< ((ce[2]>=0.)?"+":" ")<<ce[2]<<"*y^2 ";
        i=3;
    }
    if(q.isQuadratic() && ce[i]>=0.) os<<"+";
        os<<ce[i]<<"*x "<<((ce[i+1]>=0.)?"+":" ")<<ce[i+1]<<"*y "<< ((ce[i+2]>=0.)?"+":" ")<<ce[i+2]<<" == 0"
                                                                              <<std::endl;
    return os;
}
//EOF
