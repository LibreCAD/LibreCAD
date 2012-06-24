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

#include "rs_math.h"
#include "lc_quadratic.h"
#include "rs_arc.h"
#include "rs_circle.h"
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
    m_bValid(false){}

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
    m_bIsQuadratic=false;
    if(ce.size()==3){
        m_vLinear(0)=ce[0];
        m_vLinear(1)=ce[1];
        m_dConst=ce[2];
        m_bValid=true;
        return;
    }
        m_bValid=false;
}

/** construct a ellipse or hyperbola as the path of center of tangent circles
  passing the point */
LC_Quadratic::LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point)
    :m_bIsQuadratic(true)
    ,m_mQuad(2,2)
    ,m_vLinear(2)
    ,m_bValid(true)
{
    if(circle==NULL) {
        m_bValid=false;
        return;
    }
    RS_Vector center;
    double r;
    if(circle->rtti()==RS2::EntityArc) {
        const RS_Arc* p= static_cast<const RS_Arc*>(circle);
        center=p->getCenter();
        r=p->getRadius();
    }else if (circle->rtti()==RS2::EntityCircle) {
        const RS_Circle* p= static_cast<const RS_Circle*>(circle);
        center=p->getCenter();
        r=p->getRadius();
    }else{
        m_bValid=false;
        return;
    }
    double c=0.5*(center.distanceTo(point));
    double d=0.5*r;
    if(fabs(c)<RS_TOLERANCE ||fabs(d)<RS_TOLERANCE || fabs(c-d)<RS_TOLERANCE){
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
    auto&& m=rotationMatrix(angle);
    m_vLinear = prod(m, m_vLinear);
    if(m_bIsQuadratic){
        m_mQuad=prod(m_mQuad,m);
        m_mQuad=prod( trans(m), m_mQuad);
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
    if( l1.isValid() && l2.isValid() == false ) return ret;
    auto p1=&l1;
    auto p2=&l2;
    if(p1->isQuadratic()==false){
        std::swap(p1,p2);
    }
    if(p1->isQuadratic()==false){
        //two lines
        QVector<QVector<double> > ce(2,QVector<double>(3,0.));
        ce[0][0]=p1->m_vLinear(0);
        ce[0][1]=p1->m_vLinear(1);
        ce[0][2]=-p1->m_dConst;
        ce[1][0]=p2->m_vLinear(0);
        ce[1][1]=p2->m_vLinear(1);
        ce[1][2]=-p2->m_dConst;
        QVector<double> sn(2,0.);
        if(RS_Math::linearSolver(ce,sn)){
            ret.push_back(RS_Vector(sn[0],sn[1]));
        }
        return ret;
    }
    if(p2->isQuadratic()==false){
        //one line, one quadratic
        //avoid division by zero
        if(fabs(p2->m_vLinear(0))<fabs(p2->m_vLinear(1))){
            return getIntersection(p1->flipXY(),p2->flipXY()).flipXY();
        }

    }
    std::vector<std::vector<double> >  ce(0);
    ce.push_back(p1->getCoefficients());
    ce.push_back(p2->getCoefficients());
//DEBUG_HEADER();
//std::cout<<*p1<<std::endl;
//std::cout<<*p2<<std::endl;
    return RS_Math::simultaneousQuadraticSolverFull(ce);

}

/**
   rotation matrix:

   cos x, -sin x
   sin x, cos x
   */
boost::numeric::ublas::matrix<double>  LC_Quadratic::rotationMatrix(const double& angle)
{
    boost::numeric::ublas::matrix<double> ret(2,2);
    ret(0,0)=cos(angle);
    ret(0,1)=sin(angle);
    ret(1,0)=-ret(0,1);
    ret(1,1)=ret(0,0);
    return ret;
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const LC_Quadratic& q) {

    os << " quadratic form: ";
    if(q.isValid()==false) {
        os<<" invalid quadratic form"<<std::endl;
        return os;
    }
    os<<std::endl;
    auto&& ce=q.getCoefficients();
    unsigned short i=0;
    if(ce.size()==6){
        os<<ce[0]<<"*x^2 "<<( (ce[1]>=0.)?"+":" ")<<ce[1]<<"*x*y  "<< ((ce[2]>=0.)?"+":" ")<<ce[2]<<" y^2 ";
        i=3;
    }
    if(q.isQuadratic() && ce[i]>=0.) os<<"+";
        os<<ce[i]<<"*x "<<((ce[i+1]>=0.)?"+":" ")<<ce[i+1]<<"*y "<< ((ce[i+2]>=0.)?"+":" ")<<ce[i+2]<<" == 0"
                                                                              <<std::endl;
    return os;
}
//EOF
