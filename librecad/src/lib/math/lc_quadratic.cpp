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

#include "lc_quadratic.h"

#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"

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
    if(!m_bValid)
        return;

    if(m_bIsQuadratic)
        m_mQuad=lc0.getQuad();

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
        RS_Vector center=circle->getCenter();
        double r=circle->getRadius();
        if(!center.valid){
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
  if (m_mQuad.size1() == 2 && m_mQuad.size2() == 2) {
    if ( RS_Math::equal(m_mQuad(0,0), 0.)
        && RS_Math::equal(m_mQuad(0,1), 0.)
        && RS_Math::equal(m_mQuad(1,0), 0.)
        && RS_Math::equal(m_mQuad(1,1), 0.)
        )
      return false;
  }
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

double LC_Quadratic::constTerm()const
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

// In lc_quadratic.cpp – Fixed move() transformation for linear terms

/**
 * @brief move
 * Translates the conic by the given offset vector.
 *
 * For primal conic A x² + B xy + C y² + D x + E y + F = 0,
 * translation by (dx, dy) transforms linear terms:
 *   D' = D - 2 A dx - B dy
 *   E' = E - B dx - 2 C dy
 *   F' = F - D dx - E dy + A dx² + B dx dy + C dy²
 *
 * @param offset Translation vector (dx, dy)
 * @return Translated LC_Quadratic
 */
LC_Quadratic& LC_Quadratic::move(const RS_Vector& offset)
{
  if (!isValid()) {
    return *this;
  }

  std::vector<double> coeffs = getCoefficients();
  double A = coeffs[0];
  double B = coeffs[1];
  double C = coeffs[2];
  double D = coeffs[3];
  double E = coeffs[4];
  double F = coeffs[5];

  double dx = offset.x;
  double dy = offset.y;

  m_vLinear(0) = D - 2.0 * A * dx - B * dy;
  m_vLinear(1) = E - B * dx - 2.0 * C * dy;
  m_dConst = F + A * dx * dx + B * dx * dy + C * dy * dy - D * dx - E * dy;

  return *this;
}

LC_Quadratic& LC_Quadratic::rotate(double angle)
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

LC_Quadratic& LC_Quadratic::rotate(const RS_Vector& center, double angle)
{
    move(-center);
    rotate(angle);
    move(center);
    return *this;
}

/**
 * @brief scale
 * Scales the conic non-uniformly from the given center point (in-place).
 *
 * Modifies the current conic by applying non-uniform scaling by factors (sx, sy)
 * from center (cx, cy):
 *   x' = cx + sx (x - cx)
 *   y' = cy + sy (y - cy)
 *
 * The transformation is applied directly to the coefficients.
 *
 * @param center Center of scaling
 * @param factor Scaling factors (sx, sy)
 * @return Reference to this (modified) LC_Quadratic
 */
LC_Quadratic& LC_Quadratic::scale(const RS_Vector& center, const RS_Vector& factor)
{
  if (!isValid() || factor.magnitude() < RS_TOLERANCE) {
    m_bValid = false;
    return *this;
  }

  double sx = factor.x;
  double sy = factor.y;
  double cx = center.x;
  double cy = center.y;

  if (std::abs(sx) < RS_TOLERANCE || std::abs(sy) < RS_TOLERANCE) {
    m_bValid = false;
    return *this;
  }

  double A = m_mQuad(0,0);
  double B = 2.0 * m_mQuad(0,1);  // full B coefficient
  double C = m_mQuad(1,1);
  double D = m_vLinear(0);
  double E = m_vLinear(1);
  double F = m_dConst;

         // Apply non-uniform scaling transformation
  double A_new = A / (sx * sx);
  double B_new = B / (sx * sy);
  double C_new = C / (sy * sy);

  double D_new = (D - 2.0 * A * cx - B * cy) / (sx * sx) + (B * cy) / (sx * sy);
  double E_new = (E - B * cx - 2.0 * C * cy) / (sy * sy) + (B * cx) / (sx * sy);

  double F_new = F + A * cx * cx + B * cx * cy + C * cy * cy
                 - D * cx - E * cy;

         // Update internal representation
  m_mQuad(0,0) = A_new;
  m_mQuad(0,1) = m_mQuad(1,0) = B_new * 0.5;
  m_mQuad(1,1) = C_new;
  m_vLinear(0) = D_new;
  m_vLinear(1) = E_new;
  m_dConst = F_new;

  m_bValid = true;
  return *this;
}

/**
 * @author{Dongxu Li}
 */
LC_Quadratic& LC_Quadratic::shear(double k)
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
        *this = {sheared};
        return *this;
    }
    m_vLinear(1) -= k * m_vLinear(0);
    return *this;
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
    if( !l1 || !l2 ) {
        //        DEBUG_HEADER
        //        std::cout<<l1<<std::endl;
        //        std::cout<<l2<<std::endl;
        return ret;
    }
    auto p1=&l1;
    auto p2=&l2;
    if(!p1->isQuadratic()){
        std::swap(p1,p2);
    }
    // if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
    //     DEBUG_HEADER;
    //     std::cout<<*p1<<std::endl;
    //     std::cout<<*p2<<std::endl;
    // }
    if(!p1->isQuadratic()){
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
    if(!p2->isQuadratic()){
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
    }
    LC_ERR<<__LINE__;
    if(valid)
      return sol;
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
boost::numeric::ublas::matrix<double> LC_Quadratic::rotationMatrix(double angle)
{
    boost::numeric::ublas::matrix<double> ret(2,2);
    ret(0,0)=cos(angle);
    ret(0,1)=sin(angle);
    ret(1,0)=-ret(0,1);
    ret(1,1)=ret(0,0);
    return ret;
}
/**
 * @brief getDualCurve
 * Returns the dual (polar reciprocal) conic using the line convention u x + v y + 1 = 0.
 *
 * In projective geometry, the dual conic represents the envelope of polar lines.
 * The standard adjugate gives coefficients for the dual equation in the form:
 *   A' u² + B' u v + C' v² + D' u + E' v + F' = 0
 *
 * However, many CAD/geometry systems (including LibreCAD's dualLineTangentPoint)
 * adopt the normalized line form: u x + v y + 1 = 0
 *
 * To match this convention, we scale the dual coefficients so that the constant term
 * becomes +1 (corresponding to the +1 in the line equation).
 *
 * If F' = 0 (degenerate case, e.g., parabola), the dual is at infinity and we return
 * an invalid quadratic.
 *
 * @return Dual conic with constant term normalized to +1, or invalid if degenerate
 */
LC_Quadratic LC_Quadratic::getDualCurve() const
{
  if (!isQuadratic()) {
    return LC_Quadratic{};
  }

         // Primal coefficients: A x² + B xy + C y² + D x + E y + F = 0
  std::vector<double> primal = getCoefficients();
  double A = primal[0];
  double B = primal[1];
  double C = primal[2];
  double D = primal[3];
  double E = primal[4];
  double F = primal[5];

         // Dual coefficients via adjugate of conic matrix
  double A_prime = 4 * C * F - E * E;
  double B_prime = 2 * D * E - 4 * B * F;
  double C_prime = 4 * A * F - D * D;
  double D_prime = 2 * B * E - 4 * C * D;
  double E_prime = 2 * B * D - 4 * A * E;
  double F_prime = 4 * A * C - B * B;

  // Degenerate if F_prime == 0 (dual at infinity)
  if (std::abs(F_prime) < RS_TOLERANCE) {
    return LC_Quadratic{};
  }

  return LC_Quadratic({
      A_prime,
      B_prime,
      C_prime,
      D_prime,
      E_prime,
      F_prime
  });
}

// Evaluate the quadratic form at a given point (x, y)
double LC_Quadratic::evaluateAt(const RS_Vector& p) const
{
  if (!p.valid) return 0.0;  // or NaN / throw — but consistent with project style

  double x = p.x;
  double y = p.y;

         // General conic: A x² + B xy + C y² + D x + E y + F
  double result = m_mQuad(0,0) * x * x +                  // A x²
                  2.0 * m_mQuad(0,1) * x * y +            // B xy (since matrix stores B/2)
                  m_mQuad(1,1) * y * y +                  // C y²
                  m_vLinear(0) * x +                      // D x
                  m_vLinear(1) * y +                      // E y
                  m_dConst;                               // F

  return result;
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const LC_Quadratic& q) {

    os << " quadratic form: ";
    if(!q) {
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
