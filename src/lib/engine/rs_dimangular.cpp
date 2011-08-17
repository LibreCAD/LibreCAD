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


#include "rs_dimangular.h"

#include "rs_constructionline.h"
#include "rs_filterdxf.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_solid.h"
#include "rs_text.h"


/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for angular dimension.
 */
RS_DimAngular::RS_DimAngular(RS_EntityContainer* parent,
                             const RS_DimensionData& d,
                             const RS_DimAngularData& ed)
        : RS_Dimension(parent, d), edata(ed) {

    calculateBorders();
}



/**
 * @return Automatically created label for the default 
 * measurement of this dimension.
 */
QString RS_DimAngular::getMeasuredLabel() {
    QString ret;

	int dimaunit = getGraphicVariableInt("$DIMAUNIT", 0);
	int dimadec = getGraphicVariableInt("$DIMADEC", 0);

	ret = RS_Units::formatAngle(getAngle(), 
		RS_FilterDXF::numberToAngleFormat(dimaunit),
		dimadec);
	
    /*
        ret = QString("%1%2")
          .arg(RS_Math::rad2deg(getAngle()))
          .arg(QChar(0xB0));
	*/
    return ret;
}



/**
 * @return Angle of the measured dimension.
 */
double RS_DimAngular::getAngle() {
    double ang1 = 0.0;
    double ang2 = 0.0;
    bool reversed = false;
	RS_Vector p1;
	RS_Vector p2;

    getAngles(ang1, ang2, reversed, p1, p2);
    return reversed ? RS_Math::correctAngle(ang1 - ang2) :  RS_Math::correctAngle(ang2 - ang1);
//
//    if (!reversed) {
//        if (ang2<ang1) {
//            ang2+=2*M_PI;
//        }
//        return ang2-ang1;
//    } else {
//        if (ang1<ang2) {
//            ang1+=2*M_PI;
//        }
//        return ang1-ang2;
//    }
}



/**
 * @return Center of the measured dimension.
 */
RS_Vector RS_DimAngular::getCenter() {
    RS_ConstructionLine l1(NULL, RS_ConstructionLineData(edata.definitionPoint1,
                           edata.definitionPoint2));
    RS_ConstructionLine l2(NULL, RS_ConstructionLineData(edata.definitionPoint3,
                           data.definitionPoint));
    RS_VectorSolutions vs = RS_Information::getIntersection(&l1, &l2, false);

    return vs.get(0);
}



/**
 * finds out which angles this dimension actually measures.
 *
 * @param ang1 Reference will return the start angle
 * @param ang2 Reference will return the end angle
 * @param reversed Reference will return the reversed flag.
 *
 * @return true: on success
 */
bool RS_DimAngular::getAngles(double& ang1, double& ang2, bool& reversed,
                              RS_Vector& p1, RS_Vector& p2) {

RS_Vector vp0(edata.definitionPoint4 - getCenter());
RS_Vector vp1(edata.definitionPoint2 - edata.definitionPoint1);
RS_Vector vp2(data.definitionPoint - edata.definitionPoint3);
// project p0 to the basis of p1 and p2
// p0 = a1 p1 + a2 p2
// <p0.p1>= a1 |p1|^2 + a2 <p1.p2>
// <p0,p2>= a1 <p1.p2> + a2 |p2|^2
// a1 = ( |p2|^2 <p0.p1> - <p1.p2><p0.p2>) /( |p1|^2 |p2|^2 - <p1.p2>^2)
// a2 = ( |p1|^2 <p0.p2> - <p1.p2><p0.p1>) /( |p1|^2 |p2|^2 - <p1.p2>^2)

double rp1=RS_Vector::dotP(vp1,vp1);
double rp2=RS_Vector::dotP(vp2,vp2);
double p0p1=RS_Vector::dotP(vp0,vp1);
double p0p2=RS_Vector::dotP(vp0,vp2);
double p1p2=RS_Vector::dotP(vp1,vp2);
double d= rp1*rp2 - p1p2*p1p2;
double a1=d*(rp2*p0p1-p1p2*p0p2); // we only need the sign, so, use multiply instead of division to avoid divid by zero;
if ( a1 >= 0. ) {
            p1 = edata.definitionPoint2;
} else {
	    vp1 *= -1;
            p1 = edata.definitionPoint1;
}
a1 = d*(rp1*p0p2-p1p2*p0p1);
if ( a1 >= 0. ) {
            p2 = data.definitionPoint;
} else {
	    vp2 *= -1;
            p2 = edata.definitionPoint3;
}

    RS_Vector center = getCenter();
    double ang = center.angleTo(edata.definitionPoint4);
ang1=vp1.angle();
ang2=vp2.angle();
if ( ! RS_Math::isAngleBetween(ang, ang1,ang2,false) ) {
reversed = true;
}
return true;
}
//
//std::cout<<"RS_DimAngular::getAngles(): new algorithm, angle1= "<<vp1.angle()<<"\tangle2= "<<vp2.angle()<<"\treversed= "<<reversed<<std::endl;
//    bool done = false;
//
//    // find out the angles this dimension refers to:
//    for (int f1=0; f1<=1 && !done; ++f1) {
//        ang1 = RS_Math::correctAngle(
//                   edata.definitionPoint1.angleTo(edata.definitionPoint2) + f1*M_PI);
//        if (f1==0) {
//            p1 = edata.definitionPoint2;
//        } else {
//            p1 = edata.definitionPoint1;
//        }
//        for (int f2=0; f2<=1 && !done; ++f2) {
//            ang2 = RS_Math::correctAngle(
//                       edata.definitionPoint3.angleTo(data.definitionPoint) + f2*M_PI);
//            if (f2==0) {
//                p2 = data.definitionPoint;
//            } else {
//                p2 = edata.definitionPoint3;
//            }
//            for (int t=0; t<=1 && !done; ++t) {
//                reversed = (bool)t;
//
//                double angDiff;
//
//                if (!reversed) {
//                    if (ang2<ang1) {
//                        ang2+=2*M_PI;
//                    }
//                    angDiff = ang2-ang1;
//                } else {
//                    if (ang1<ang2) {
//                        ang1+=2*M_PI;
//                    }
//                    angDiff = ang1-ang2;
//                }
//
//                ang1 = RS_Math::correctAngle(ang1);
//                ang2 = RS_Math::correctAngle(ang2);
//
//                if (RS_Math::isAngleBetween(ang, ang1, ang2, reversed) &&
//                        angDiff<=M_PI) {
//                    done = true;
//                    break;
//                }
//            }
//        }
//    }
//std::cout<<"RS_DimAngular::getAngles(): old algorithm, ang1= "<<ang1<<"\tang2= "<<ang2<<"\treversed= "<<reversed<<std::endl;
//
//    return done;
//}



/**
 * Updates the sub entities of this dimension. Called when the 
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimAngular::update(bool /*autoText*/) {

    RS_DEBUG->print("RS_DimAngular::update");

    clear();
	
	if (isUndone()) {
		return;
	}

    // distance from entities (DIMEXO)
    double dimexo = getExtensionLineOffset();
    // extension line extension (DIMEXE)
    double dimexe = getExtensionLineExtension();
    // text height (DIMTXT)
    double dimtxt = getTextHeight();
    // text distance to line (DIMGAP)
    double dimgap = getDimensionLineGap();

    // find out center:
    RS_Vector center = getCenter();

    if (!center.valid) {
        return;
    }

    double ang1 = 0.0;
    double ang2 = 0.0;
    bool reversed = false;
    RS_Vector p1;
    RS_Vector p2;

    getAngles(ang1, ang2, reversed, p1, p2);

    double rad = edata.definitionPoint4.distanceTo(center);

    RS_Line* line;
    RS_Vector dir;
    double len;
    double dist;

    // 1st extension line:
    dist = center.distanceTo(p1);
    len = rad - dist + dimexe;
    dir.setPolar(1.0, ang1);
    line = new RS_Line(this,
                       RS_LineData(center + dir*dist + dir*dimexo,
                                   center + dir*dist + dir*len));
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);

    // 2nd extension line:
    dist = center.distanceTo(p2);
    len = rad - dist + dimexe;
    dir.setPolar(1.0, ang2);
    line = new RS_Line(this,
                       RS_LineData(center + dir*dist + dir*dimexo,
                                   center + dir*dist + dir*len));
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);

    // Create dimension line (arc):
    RS_Arc* arc = new RS_Arc(this,
                             RS_ArcData(center,
                                        rad, ang1, ang2, reversed));
    arc->setPen(RS_Pen(RS2::FlagInvalid));
    arc->setLayer(NULL);
    addEntity(arc);

    // length of dimension arc:
    double distance = arc->getLength();

    // do we have to put the arrows outside of the arc?
    bool outsideArrows = (distance<getArrowSize()*2);

    // arrow angles:
    double arrowAngle1, arrowAngle2;
    double arrowAng;
	if (rad>1.0e-6) {
		arrowAng = getArrowSize() / rad;
	}
	else {
		arrowAng = 0.0;
	}
    RS_Vector v1, v2;
    if (!arc->isReversed()) {
        v1.setPolar(rad, arc->getAngle1()+arrowAng);
    } else {
        v1.setPolar(rad, arc->getAngle1()-arrowAng);
    }
    v1+=arc->getCenter();
    arrowAngle1 = arc->getStartpoint().angleTo(v1);


    if (!arc->isReversed()) {
        v2.setPolar(rad, arc->getAngle2()-arrowAng);
    } else {
        v2.setPolar(rad, arc->getAngle2()+arrowAng);
    }
    v2+=arc->getCenter();
    arrowAngle2 = arc->getEndpoint().angleTo(v2);

    if (!outsideArrows) {
        arrowAngle1 = arrowAngle1+M_PI;
        arrowAngle2 = arrowAngle2+M_PI;
    }

    // Arrows:
    RS_SolidData sd;
    RS_Solid* arrow;

    // arrow 1
    arrow = new RS_Solid(this, sd);
    arrow->shapeArrow(arc->getStartpoint(),
                      arrowAngle1,
                      getArrowSize());
    arrow->setPen(RS_Pen(RS2::FlagInvalid));
    arrow->setLayer(NULL);
    addEntity(arrow);

    // arrow 2:
    arrow = new RS_Solid(this, sd);
    arrow->shapeArrow(arc->getEndpoint(),
                      arrowAngle2,
                      getArrowSize());
    arrow->setPen(RS_Pen(RS2::FlagInvalid));
    arrow->setLayer(NULL);
    addEntity(arrow);


    // text label:
    RS_TextData textData;
    RS_Vector textPos = arc->getMiddlepoint();

    RS_Vector distV;
    double textAngle;
    double dimAngle1 = textPos.angleTo(arc->getCenter())-M_PI/2.0;

    // rotate text so it's readable from the bottom or right (ISO)
    // quadrant 1 & 4
    if (dimAngle1>M_PI/2.0*3.0+0.001 ||
            dimAngle1<M_PI/2.0+0.001) {

        distV.setPolar(dimgap, dimAngle1+M_PI/2.0);
        textAngle = dimAngle1;
    }
    // quadrant 2 & 3
    else {
        distV.setPolar(dimgap, dimAngle1-M_PI/2.0);
        textAngle = dimAngle1+M_PI;
    }

    // move text away from dimension line:
    textPos+=distV;

    textData = RS_TextData(textPos,
                           dimtxt, 30.0,
                           RS2::VAlignBottom,
                           RS2::HAlignCenter,
                           RS2::LeftToRight,
                           RS2::Exact,
                           1.0,
                           getLabel(),
                           "standard",
                           textAngle);

    RS_Text* text = new RS_Text(this, textData);

    // move text to the side:
    text->setPen(RS_Pen(RS2::FlagInvalid));
    text->setLayer(NULL);
    addEntity(text);

    calculateBorders();
}



void RS_DimAngular::move(RS_Vector offset) {
    RS_Dimension::move(offset);

    edata.definitionPoint1.move(offset);
    edata.definitionPoint2.move(offset);
    edata.definitionPoint3.move(offset);
    edata.definitionPoint4.move(offset);
    update();
}



void RS_DimAngular::rotate(RS_Vector center, double angle) {
    RS_Dimension::rotate(center, angle);

    edata.definitionPoint1.rotate(center, angle);
    edata.definitionPoint2.rotate(center, angle);
    edata.definitionPoint3.rotate(center, angle);
    edata.definitionPoint4.rotate(center, angle);
    update();
}



void RS_DimAngular::scale(RS_Vector center, RS_Vector factor) {
    RS_Dimension::scale(center, factor);

    edata.definitionPoint1.scale(center, factor);
    edata.definitionPoint2.scale(center, factor);
    edata.definitionPoint3.scale(center, factor);
    edata.definitionPoint4.scale(center, factor);
    update();
}



void RS_DimAngular::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.definitionPoint1.mirror(axisPoint1, axisPoint2);
    edata.definitionPoint2.mirror(axisPoint1, axisPoint2);
    edata.definitionPoint3.mirror(axisPoint1, axisPoint2);
    edata.definitionPoint4.mirror(axisPoint1, axisPoint2);
    update();
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimAngular& d) {
    os << " DimAngular: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}

