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


#include "rs_dimension.h"
#include "rs_solid.h"
#include "rs_text.h"
#include "rs_units.h"

/**
 * Constructor.
 */
RS_Dimension::RS_Dimension(RS_EntityContainer* parent,
                           const RS_DimensionData& d)
        : RS_EntityContainer(parent), data(d) {
}



RS_Vector RS_Dimension::getNearestRef(const RS_Vector& coord,
                                      double* dist) {

    return RS_Entity::getNearestRef(coord, dist);
}


RS_Vector RS_Dimension::getNearestSelectedRef(const RS_Vector& coord,
        double* dist) {

    return RS_Entity::getNearestSelectedRef(coord, dist);
}



/**
 * @return Dimension text. Either a text the user defined or 
 *         the measured text.
 *
 * @param resolve false: return plain value. true: return measured 
 *      label if appropriate.
 * @see getMeasuredLabel
 */
QString RS_Dimension::getLabel(bool resolve) {
	if (!resolve) {
		return data.text;
	}

    QString ret="";

    // One space suppresses the text:
    if (data.text==" ") {
        ret = "";
    }

    // No text prints actual measurement:
    else if (data.text=="") {
        ret = getMeasuredLabel();
    }

    // Others print the text (<> is replaced by the measurement)
    else {
        ret = data.text;
        ret = ret.replace(QString("<>"), getMeasuredLabel());
    }

    return ret;
}


/**
 * Sets a new text for the label.
 */
void RS_Dimension::setLabel(const QString& l) {
	data.text = l;
}



/**
 * Creates a dimensioning line (line with one, two or no arrows and a text).
 *
 * @param forceAutoText Automatically reposition the text label.
 */
void RS_Dimension::updateCreateDimensionLine(const RS_Vector& p1,
        const RS_Vector& p2, bool arrow1, bool arrow2, bool forceAutoText) {

    // text height (DIMTXT)
    double dimtxt = getTextHeight();
    // text distance to line (DIMGAP)
    double dimgap = getDimensionLineGap();

    // length of dimension line:
    double distance = p1.distanceTo(p2);

    // do we have to put the arrows outside of the line?
    bool outsideArrows = (distance<getArrowSize()*2.5);

    // arrow angles:
    double arrowAngle1, arrowAngle2;

    // Create dimension line:
    RS_Line* dimensionLine = new RS_Line(this, RS_LineData(p1, p2));
    dimensionLine->setPen(RS_Pen(RS2::FlagInvalid));
    dimensionLine->setLayer(NULL);
    addEntity(dimensionLine);

    if (outsideArrows==false) {
        arrowAngle1 = dimensionLine->getAngle2();
        arrowAngle2 = dimensionLine->getAngle1();
    } else {
        arrowAngle1 = dimensionLine->getAngle1();
        arrowAngle2 = dimensionLine->getAngle2();
       
        // extend dimension line outside arrows
        RS_Vector dir;
        dir.setPolar(getArrowSize()*2, arrowAngle2);
        dimensionLine->setStartpoint(p1 + dir);
        dimensionLine->setEndpoint(p2 - dir);
    }

    // Arrows:
    RS_SolidData sd;
    RS_Solid* arrow;

    if (arrow1) {
        // arrow 1
        arrow = new RS_Solid(this, sd);
        arrow->shapeArrow(p1,
                          arrowAngle1,
                          getArrowSize());
        arrow->setPen(RS_Pen(RS2::FlagInvalid));
        arrow->setLayer(NULL);
        addEntity(arrow);
    }

    if (arrow2) {
        // arrow 2:
        arrow = new RS_Solid(this, sd);
        arrow->shapeArrow(p2,
                          arrowAngle2,
                          getArrowSize());
        arrow->setPen(RS_Pen(RS2::FlagInvalid));
        arrow->setLayer(NULL);
        addEntity(arrow);
    }

    // Text label:
    RS_TextData textData;
    RS_Vector textPos;

	double dimAngle1 = dimensionLine->getAngle1();
	double textAngle;
	bool corrected=false;
	textAngle = RS_Math::makeAngleReadable(dimAngle1, true, &corrected);

    if (data.middleOfText.valid && !forceAutoText) {
        textPos = data.middleOfText;
    } else {
        textPos = dimensionLine->getMiddlepoint();

        RS_Vector distV;

        // rotate text so it's readable from the bottom or right (ISO)
        // quadrant 1 & 4
        if (corrected) {
            distV.setPolar(dimgap + dimtxt/2.0, dimAngle1-M_PI/2.0);
        } else {
            distV.setPolar(dimgap + dimtxt/2.0, dimAngle1+M_PI/2.0);
        }

        // move text away from dimension line:
        textPos+=distV;
        //// the next update should still be able to adjust this
        ////   auto text position. leave it invalid
		data.middleOfText = textPos;
    }

    textData = RS_TextData(textPos,
                           dimtxt, 30.0,
                           RS2::VAlignMiddle,
                           RS2::HAlignCenter,
                           RS2::LeftToRight,
                           RS2::Exact,
                           1.0,
                           getLabel(),
                           "standard",
                           textAngle);

    RS_Text* text = new RS_Text(this, textData);

    // move text to the side:
    RS_Vector distH;
    if (text->getUsedTextWidth()>distance) {
        distH.setPolar(text->getUsedTextWidth()/2.0
                       +distance/2.0+dimgap, textAngle);
        text->move(distH);
    }
    text->setPen(RS_Pen(RS2::FlagInvalid));
    text->setLayer(NULL);
    addEntity(text);
}



/**
 * @return arrow size in drawing units.
 */
double RS_Dimension::getArrowSize() {
    return getGraphicVariable("$DIMASZ", 2.5, 40);
}



/**
 * @return extension line overlength in drawing units.
 */
double RS_Dimension::getExtensionLineExtension() {
    return getGraphicVariable("$DIMEXE", 1.25, 40);
}



/**
 * @return extension line offset from entities in drawing units.
 */
double RS_Dimension::getExtensionLineOffset() {
    return getGraphicVariable("$DIMEXO", 0.625, 40);
}



/**
 * @return extension line gap to text in drawing units.
 */
double RS_Dimension::getDimensionLineGap() {
    return getGraphicVariable("$DIMGAP", 0.625, 40);
}



/**
 * @return Dimension lables text height.
 */
double RS_Dimension::getTextHeight() {
    return getGraphicVariable("$DIMTXT", 2.5, 40);
}



/**
 * @return the given graphic variable or the default value given in mm 
 * converted to the graphic unit.
 * If the variable is not found it is added with the given default
 * value converted to the local unit.
 */
double RS_Dimension::getGraphicVariable(const QString& key, double defMM,
                                        int code) {

    double v = getGraphicVariableDouble(key, RS_MINDOUBLE);
    if (v<=RS_MINDOUBLE) {
        addGraphicVariable(
            key,
            RS_Units::convert(defMM, RS2::Millimeter, getGraphicUnit()),
            code);
        v = getGraphicVariableDouble(key, 1.0);
    }

    return v;
}




void RS_Dimension::move(RS_Vector offset) {
    data.definitionPoint.move(offset);
    data.middleOfText.move(offset);
}



void RS_Dimension::rotate(RS_Vector center, double angle) {
    data.definitionPoint.rotate(center, angle);
    data.middleOfText.rotate(center, angle);
    data.angle = RS_Math::correctAngle(data.angle+angle);
}



void RS_Dimension::scale(RS_Vector center, RS_Vector factor) {
    data.definitionPoint.scale(center, factor);
    data.middleOfText.scale(center, factor);
}



void RS_Dimension::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    data.definitionPoint.mirror(axisPoint1, axisPoint2);
    data.middleOfText.mirror(axisPoint1, axisPoint2);
}

// EOF
