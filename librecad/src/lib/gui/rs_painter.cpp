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

#include<QPolygon>
#include "rs_pen.h"
#include "rs_color.h"
#include "rs_painter.h"
#include "rs_math.h"

void RS_Painter::createArc(QPolygon& pa,
                             const RS_Vector& cp, double radius,
                             double a1, double a2,
                             bool reversed) {

    if (radius<1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "RS_Painter::createArc: invalid radius: %f", radius);
        return;
    }

    double aStep=fabs(2.0/radius);         // Angle Step (rad)
    if(aStep>=0.5) aStep=0.5;
    if(reversed) {
        if(a1<=a2+RS_TOLERANCE) a1+=2.*M_PI;
        aStep *= -1;
    }else{
        if(a2<=a1+RS_TOLERANCE) a2+=2.*M_PI;
    }
    double a;             // Current Angle (rad)

//    aStep=aStep/2.0;
    //if (aStep<0.05) {
    //    aStep = 0.05;
    //}

    // less than a pixel long lines:
    //if (radius*aStep<1.0) {
    //	aStep =
    //}

    //QPointArray pa;
    pa.clear();
    //    pa<<QPoint(toScreenX(cp.x+cos(aStart)*radius), toScreenY(cp.y-sin(aStart)*radius));
    double da=fabs(a2-a1);
    for(a=a1; fabs(a-a1)<da; a+=aStep) {
        pa<<QPoint(toScreenX(cp.x+cos(a)*radius), toScreenY(cp.y-sin(a)*radius));
    }

    QPoint pt2(toScreenX(cp.x+cos(a2)*radius), toScreenY(cp.y-sin(a2)*radius));
    if(pa.size()>0 && pa.last() != pt2) pa<<pt2;
}


void RS_Painter::createEllipse(QPolygon& pa,
        const RS_Vector& cp,
                         double radius1, double radius2,
                         double angle,
                         double angle1, double angle2,
                         bool reversed)
{

    const RS_Vector vr(radius1,radius2);
    const RS_Vector rvp(radius2,radius1);
    const double ab=radius1*radius2;
    double ea1=angle1;
    double ea2;
    double dA=RS_Math::getAngleDifference(angle1, angle2, reversed);
    if(dA <= RS_TOLERANCE_ANGLE) {
        dA=2.*M_PI;
        ea2 =ea1 + dA;
    }else
        ea2 = ea1 +(reversed?-dA:dA);
    const RS_Vector angleVector(-angle);
    /*
      draw a new line after tangent changes by 0.01 rad
      ds^2 = (a^2 sin^2 + b^2 cos^2) da^2
      */
    RS_Vector vp(-ea1);
    vp.scale(vr);
    vp.rotate(angleVector);
    vp.move(cp);
    //    vp.set(cp.x+cos(a1)*radius1,
    //           cp.y-sin(a1)*radius2);
    //    vp.rotate(vc, -angle);
    pa.clear();
//    pa<<QPoint(toScreenX(vp.x),
//               toScreenY(vp.y));
//    moveTo(toScreenX(vp.x),
//           toScreenY(vp.y));
    const double minDea=fabs(ea2-ea1)/2048.;
    // Arc Counterclockwise:
    do {

        RS_Vector va(-ea1);
        vp=va;
        double r2=va.scale(rvp).squared();
        if( r2<RS_TOLERANCE15) r2=RS_TOLERANCE15;
        double aStep=ab/(r2*sqrt(r2));
        if(aStep < minDea) aStep=minDea;
        if(aStep > M_PI/4.) aStep=M_PI/4.;
        ea1 += reversed?-aStep:aStep;
        vp.scale(vr);
        vp.rotate(angleVector);
        vp.move(cp);
        pa<<QPoint(toScreenX(vp.x),
               toScreenY(vp.y));
    } while(fabs(angle1-ea1)<dA);

    vp.set(cos(ea2)*radius1,
           -sin(ea2)*radius2);
    vp.rotate(angleVector);
    vp.move(cp);
    pa<<QPoint(toScreenX(vp.x),
           toScreenY(vp.y));
}

void RS_Painter::drawRect(const RS_Vector& p1, const RS_Vector& p2) {
    drawPolygon(QRect(int(p1.x+0.5), int(p1.y+0.5), int(p2.x - p1.x+0.5), int(p2.y - p1.y+0.5)));
//    drawLine(RS_Vector(p1.x, p1.y), RS_Vector(p2.x, p1.y));
//    drawLine(RS_Vector(p2.x, p1.y), RS_Vector(p2.x, p2.y));
//    drawLine(RS_Vector(p2.x, p2.y), RS_Vector(p1.x, p2.y));
//    drawLine(RS_Vector(p1.x, p2.y), RS_Vector(p1.x, p1.y));
}

void RS_Painter::drawHandle(const RS_Vector& p, const RS_Color& c, int size) {
    if (size<0) {
        size = 2;
    }
    fillRect((int)(p.x-size), (int)(p.y-size), 2*size, 2*size, c);
}

int RS_Painter::toScreenX(double x) const {
	return RS_Math::round(offset.x + x);
}

int RS_Painter::toScreenY(double y) const{
	return RS_Math::round(offset.y + y);
}

