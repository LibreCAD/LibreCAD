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

#include "lc_hyperbola.h"

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_information.h"
#include "rs_linetypepattern.h"
#include "lc_quadratic.h"

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& _center,
			   const RS_Vector& _majorP,
			   double _ratio,
			   double _angle1, double _angle2,
			   bool _reversed):
	center(_center)
	,majorP(_majorP)
	,ratio(_ratio)
	,angle1(_angle1)
	,angle2(_angle2)
	,reversed(_reversed)
{
}

std::ostream& operator << (std::ostream& os, const LC_HyperbolaData& ed) {
	os << "(" << ed.center <<
	   "/" << ed.majorP <<
	   " " << ed.ratio <<
	   " " << ed.angle1 <<
	   "," << ed.angle2 <<
	   ")";
	return os;
}

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif

/**
 * Constructor.
 */
LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent,
                       const LC_HyperbolaData& d)
    :RS_AtomicEntity(parent)
    ,data(d)
    ,m_bValid(true)
{
    if(data.majorP.squared()<RS_TOLERANCE2) {
        m_bValid=false;
        return;
    }
    //calculateEndpoints();
    calculateBorders();
}

/** create data based on foci and a point on hyperbola */
LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& focus0,
                 const RS_Vector& focus1,
                 const RS_Vector& point):
    center((focus0+focus1)*0.5)
{
    double ds0=focus0.distanceTo(point);
    ds0 -= focus1.distanceTo(point);

    majorP= (ds0>0.)?focus0-center:focus1-center;
    double dc=focus0.distanceTo(focus1);
    double dd=fabs(ds0);
    //no hyperbola for middle equidistant
    if(dc<RS_TOLERANCE||dd<RS_TOLERANCE) {
        majorP.set(0.,0.);
        return;
    }
    ratio= dc/dd;
    majorP /= ratio;
    ratio=sqrt(ratio*ratio - 1.);

}

///** create data based on foci and a point on hyperbola */
//LC_Hyperbola::LC_Hyperbola(const RS_Vector& focus0,
//                 const RS_Vector& focus1,
//                 const RS_Vector& point):
//    data(focus0,focus1,point)
//{
//    m_bValid = data.majorP.squared()> RS_TOLERANCE2;
//}
/**
 * Recalculates the endpoints using the angles and the radius.
 */
/*
void LC_Hyperbola::calculateEndpoints() {
   double angle = data.majorP.angle();
   double radius1 = getMajorRadius();
   double radius2 = getMinorRadius();

   startpoint.set(data.center.x + cos(data.angle1) * radius1,
                  data.center.y + sin(data.angle1) * radius2);
   startpoint.rotate(data.center, angle);
   endpoint.set(data.center.x + cos(data.angle2) * radius1,
                data.center.y + sin(data.angle2) * radius2);
   endpoint.rotate(data.center, angle);
}
*/


/**
 * Calculates the boundary box of this ellipse.
 */


RS_Entity* LC_Hyperbola::clone() const {
	LC_Hyperbola* e = new LC_Hyperbola(*this);
	e->initId();
	return e;
}


/**
  * return the foci of ellipse
  *
  *@Author: Dongxu Li
  */

RS_VectorSolutions LC_Hyperbola::getFoci() const {
    RS_Vector vp(getMajorP()*sqrt(1.-getRatio()*getRatio()));
	return RS_VectorSolutions({getCenter()+vp, getCenter()-vp});
}

RS_VectorSolutions LC_Hyperbola::getRefPoints() const{
	RS_VectorSolutions ret({data.center});
    ret.push_back(getFoci());
    return ret;
}

bool LC_Hyperbola::isPointOnEntity(const RS_Vector& coord,
                             double tolerance) const
{
    double a=data.majorP.magnitude();
    double b=a*data.ratio;
    if(fabs(a)<tolerance || fabs(b)<tolerance) return false;
    RS_Vector vp(coord - data.center);
    vp=vp.rotate(-data.majorP.angle());
    return fabs( vp.x*vp.x/(a*a)- vp.y*vp.y/(b*b) -1.)<tolerance;
}


LC_Quadratic LC_Hyperbola::getQuadratic() const
{
    std::vector<double> ce(6,0.);
    ce[0]=data.majorP.squared();
    ce[2]=-data.ratio*data.ratio*ce[0];
    if(ce[0]>RS_TOLERANCE2) ce[0]=1./ce[0];
    if(fabs(ce[2])>RS_TOLERANCE2) ce[2]=1./ce[2];
    ce[5]=-1.;
    LC_Quadratic ret(ce);
    if(ce[0]<RS_TOLERANCE2 || fabs(ce[2])<RS_TOLERANCE2) {
		ret.setValid(false);
        return ret;
    }
    ret.rotate(data.majorP.angle());
    ret.move(data.center);
    return ret;
}

//RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector& /*coord*/,
//                                         double* /*dist*/ = NULL) const
//{
//}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const LC_Hyperbola& a) {
    os << " Hyperbola: " << a.data << "\n";
    return os;
}

