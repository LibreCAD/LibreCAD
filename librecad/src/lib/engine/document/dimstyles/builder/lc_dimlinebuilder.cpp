/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_dimlinebuilder.h"

LC_DimLineBuilder::LC_DimLineBuilder(RS_Graphic* graphic) {
}

void LC_DimLineBuilder::buildDimensionLine(RS_EntityContainer* container, LC_DimStyle* style,
    const RS_Vector& dimLineStart, const RS_Vector& dimLineEnd, bool forceAutoText) {
}

/*
void LC_DimLineBuilder::createDimensionLine(RS_EntityContainer* container, LC_DimStyle* style, const RS_Vector& dimLineStart,
                                       const RS_Vector& dimLineEnd, bool arrow1, bool arrow2, bool forceAutoText) {
    if (getInsideHorizontalText()) {
        createHorizontalTextDimensionLine(dimLineStart, dimLineEnd, arrow1, arrow2, forceAutoText);
    }
    else {
        createAlignedTextDimensionLine(dimLineStart, dimLineEnd, arrow1, arrow2, forceAutoText);
    }
}

void LC_DimLineBuilder::createHorizontalTextDimensionLine(const RS_Vector& p1,
                                                           const RS_Vector& p2, bool arrow1, bool arrow2,
                                                           bool forceAutoText) {

    double dimscale = getGeneralScale();
    double dimtxt = getTextHeight() * dimscale;
    double dimgap = getDimensionLineGap() * dimscale;

    // length of dimension line:
    double distance = p1.distanceTo(p2);
    // arrow size:
    double arrowSize = getArrowSize() * dimscale;

    RS_Pen dimensionLinePen(getDimensionLineColor(),
               getDimensionLineWidth(),
               RS2::LineByBlock);

    // Create dimension line:
    RS_Line* dimensionLine{new RS_Line{this, p1, p2}};
    RS_Line* dimensionLineInside1{nullptr};
    RS_Line* dimensionLineInside2{nullptr};
    RS_Line* dimensionLineOutside1{nullptr};
    RS_Line* dimensionLineOutside2{nullptr};
    dimensionLine->setPen(dimensionLinePen);
    dimensionLine->setLayer(nullptr);

    // Text label:

    RS_Vector textPos;
    double textAngle = 0.0;
    bool autoText = !m_dimGenericData.middleOfText.valid || forceAutoText;

    if (autoText) {
        textPos = dimensionLine->getMiddlePoint();

        //// the next update should still be able to adjust this
        ////   auto text position. leave it invalid
        m_dimGenericData.middleOfText = textPos;
    }
    else {
        textPos = m_dimGenericData.middleOfText;
    }

    auto* text = createDimText(textPos, dimtxt, textAngle);

    // evaluate intersection between dim line and text
    double textIntersectionLength = 0.0;
    double w = text->getUsedTextWidth() / 2 + dimgap;
    double h = text->getUsedTextHeight() / 2 + dimgap;

    // textCorner variables correspond to the corners of the text bounding box
    // if the text were to be positioned in the center of the dimensionLine.
    RS_Vector textCorner1 = dimensionLine->getMiddlePoint() - RS_Vector{w, h};
    RS_Vector textCorner2 = dimensionLine->getMiddlePoint() + RS_Vector{w, h};
    RS_EntityContainer c;
    c.addRectangle(textCorner1, textCorner2);

    // treat line as infinitely long in both directions
    RS_VectorSolutions sol1 = getIntersectionsLineContainer(dimensionLine, &c,true);
    textIntersectionLength = sol1.get(0).distanceTo(sol1.get(1));

    // determine if we should use outside arrows
    bool outsideArrows = (textIntersectionLength + 3 * arrowSize) > distance;

    // add arrows
    // arrow angles:
    double arrowAngle1 = outsideArrows ? dimensionLine->getAngle1() : dimensionLine->getAngle2();
    double arrowAngle2 = outsideArrows ? dimensionLine->getAngle2() : dimensionLine->getAngle1();
    const bool showArrows = arrowSize > 1e-6 * p1.distanceTo(p2);
    if (outsideArrows) {
        // extend dimension line outside arrows
        if (showArrows) {
            auto dir = RS_Vector::polar(arrowSize * 2, dimensionLine->getAngle1());

            dimensionLineOutside1 = new RS_Line{this, p1 - dir, p1};
            dimensionLineOutside1->setPen(dimensionLinePen);

            dimensionLineOutside2 = new RS_Line{this, p2 + dir, p2};
            dimensionLineOutside2->setPen(dimensionLinePen);
        }

        // move text to the side if it won't fit either
        if (textIntersectionLength > distance && autoText) {
            double dist = (textIntersectionLength + distance) / 2.0 + arrowSize * 2;
            RS_Vector distH = RS_Vector::polar(dist, arrowAngle1);
            text->move(distH);
            textPos = text->getInsertionPoint();
            m_dimGenericData.middleOfText = textPos;
        }
    }
    double dimtsz = getTickSize() * dimscale;
    bool displayArrows = dimtsz < 0.01 && showArrows;
    if (displayArrows) {
        //display arrow
        // Arrows:
        RS_SolidData sd{};

        if (arrow1) {
            // arrow 1
            auto arrow = new RS_Solid(this, sd);
            arrow->shapeArrow(p1,arrowAngle1, arrowSize);
            addDimComponentEntity(arrow, dimensionLinePen);
        }

        if (arrow2) {
            // arrow 2:
            auto arrow = new RS_Solid(this, sd);
            arrow->shapeArrow(p2,arrowAngle2, arrowSize);
            addDimComponentEntity(arrow, dimensionLinePen);
        }
    }
    else {
        //display ticks
        RS_Vector tickVector = RS_Vector::polar(dimtsz, arrowAngle1 + M_PI * 0.25); //tick is 45 degree away
        if (arrow1) {
            // tick 1
            addDimComponentLine(p1 - tickVector, p1 + tickVector, dimensionLinePen);
        }
        if (arrow2) {// tick 2:
            addDimComponentLine(p2 - tickVector, p2 + tickVector, dimensionLinePen);
        }
    }

    // calculate split dimension lines
    bool splitDimensionLine = false;
    if (!outsideArrows) {
        w = text->getUsedTextWidth() / 2 + dimgap;
        h = text->getUsedTextHeight() / 2 + dimgap;
        RS_Vector s1 = text->getInsertionPoint() - RS_Vector{w, h};
        RS_Vector s2 = text->getInsertionPoint() + RS_Vector{w, h};
        c = RS_EntityContainer();
        c.addRectangle(s1, s2);
        sol1 = getIntersectionsLineContainer(dimensionLine, &c);
        if (sol1.size() > 1) {
            // the text bounding box intersects dimensionLine on two sides
            splitDimensionLine = true;
            s1 = sol1.get(0);
            s2 = sol1.get(1);
        }
        else if (sol1.size() == 1) {
            // the text bounding box intersects dimensionLine on one side
            splitDimensionLine = true;
            if (RS_Information::isPointInsideContour(p1, &c)) {
                // the dimension line begins inside the text bounds
                s1 = p1;
                s2 = sol1.get(0);
            }
            else {
                // the dimension line ends inside the text bounds
                s1 = sol1.get(0);
                s2 = p2;
            }
        }
        else {
            // the text bounding box does not intersect with dimensionLine, but we
            // should still check if dimensionLine endpoints are completely inside
            // the bounding box.
            if (RS_Information::isPointInsideContour(p1, &c)) {
                splitDimensionLine = true;
                s1 = p1;
                s2 = p2;
            }
        }

        if (splitDimensionLine) {
            dimensionLineInside1 = new RS_Line{this, p1, s1};
            dimensionLineInside1->setPen(dimensionLinePen);
            dimensionLineInside2 = new RS_Line{this, s2, p2};
            dimensionLineInside2->setPen(dimensionLinePen);
        }
    }

    // finally, add the dimension line(s) and text to the drawing
    if (outsideArrows && dimensionLineOutside1) {
        addEntity(dimensionLineOutside1);
        addEntity(dimensionLineOutside2);
    }
    else if (splitDimensionLine && dimensionLineInside1) {
        addEntity(dimensionLineInside1);
        addEntity(dimensionLineInside2);
    }
    else {
        addEntity(dimensionLine);
    }
}
*/
