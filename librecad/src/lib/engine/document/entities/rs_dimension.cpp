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

#include <QRegularExpression>

#include "muParser.h"
#include "rs_arc.h"
#include "rs_filterdxfrw.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_solid.h"
#include "rs_units.h"

namespace
{
    // Issue #1953: Use calculations in custom Dimension text #1953
    // Evaluate a single expression
    // If the expression is not valid, the input expression is returned
    QString evaluateFunction(const QString& expression, double dimValue) {
        // the minimum expression must contain "{<>}", minimum size = 4
        if (expression.size() < 4)
            return expression;
        // Assuming front/end "{}"
        QString expr = expression.mid(1, expression.size() - 2);
        const QString variable("a");
        expr.replace("<>", variable);
        try {
            mu::Parser p;

#ifdef _UNICODE
            p.DefineVar(variable.toStdWString(), &dimValue);
            p.SetExpr(expr.toStdWString());
#else
        p.DefineVar(variable.toStdString(), &dimValue);
        p.SetExpr(expr.toStdString());
#endif

            double functionValue = p.Eval();
            return QString::number(functionValue);
        }
        catch (...) {
            return expression;
        }
    }

    /**
     * @brief functionalText evaluate expressions of dimension value
     * @param dimText   functional expressions of dimension value:
     *                  must be specified within curly brackets {};
     *                  must be valid expressions:
     *                  {<>*<>} will be replaced by the value squared x^2
     *                  {exp(<>) * <>} will be replaced by the function value of x*exp(x)
     *                  {sin(<>) + 1} will be replaced by the value sin(x) + 1
     * @param dimValue the dimension value used to evaluate the expression
     * @return the text with functional expressions evaluated
     */
    QString functionalText(const QString& dimText, double dimValue) {
        // matching "{*<>*<>*}"
        static const QRegularExpression re{R"({([^{}<>]*(<>)[^{}<>]*)*})"};
        QRegularExpressionMatch match = re.match(dimText);
        if (!match.hasMatch())
            return dimText;

        QString ret = dimText;
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);
            QString functionValue = evaluateFunction(captured, dimValue);
            ret.replace(captured, functionValue);
        }
        return ret;
    }
}

RS_DimensionData::RS_DimensionData():
    definitionPoint(false),
    middleOfText(false),
    valign(RS_MTextData::VABottom),
    halign(RS_MTextData::HALeft),
    lineSpacingStyle(RS_MTextData::Exact),
    lineSpacingFactor(0.0),
    text(""),
    style(""),
    angle(0.0),
    horizontalAxisDirection(0.0),
    autoText{true}{
}

/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point.
 * @param middleOfText Middle point of dimension text.
 * @param valign Vertical alignment.
 * @param halign Horizontal alignment.
 * @param lineSpacingStyle Line spacing style.
 * @param lineSpacingFactor Line spacing factor.
 * @param text Text string entered explicitly by user or null
 *         or "<>" for the actual measurement or " " (one blank space).
 *         for suppressing the text.
 * @param style Dimension style name.
 * @param angle Rotation angle of dimension text away from
 *         default orientation.
 */
RS_DimensionData::RS_DimensionData(const RS_Vector& _definitionPoint,
                                   const RS_Vector& _middleOfText,
                                   RS_MTextData::VAlign _valign,
                                   RS_MTextData::HAlign _halign,
                                   RS_MTextData::MTextLineSpacingStyle _lineSpacingStyle,
                                   double _lineSpacingFactor,
                                   QString _text,
                                   QString _style,
                                   double _angle,
                                   double hdir,
                                   bool autoTextLocation):
    definitionPoint(_definitionPoint)
    , middleOfText(_middleOfText)
    , valign(_valign)
    , halign(_halign)
    , lineSpacingStyle(_lineSpacingStyle)
    , lineSpacingFactor(_lineSpacingFactor)
    , text(_text)
    , style(_style)
    , angle(_angle)
    , horizontalAxisDirection(hdir)
    , autoText{autoTextLocation} {
}

std::ostream& operator <<(std::ostream& os,
                          const RS_DimensionData& dd) {
    os << "("
        << dd.definitionPoint << ','
        << dd.middleOfText << ','
        << dd.valign << ','
        << dd.halign << ','
        << dd.lineSpacingStyle << ','
        << dd.lineSpacingFactor << ','
        << dd.text.toLatin1().data() << ','
        << dd.style.toLatin1().data() << ','
        << dd.angle
        << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Dimension::RS_Dimension(RS_EntityContainer* parent,const RS_DimensionData& d)
    : RS_EntityContainer(parent)
    , m_dimGenericData(std::move(d)){
}

RS_Vector RS_Dimension::getNearestRef(const RS_Vector& coord,
                                      double* dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for refpoint dragging
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Dimension::getNearestSelectedRef(const RS_Vector& coord,
                                              double* dist /*= nullptr*/) const {
    // override the RS_EntityContainer method
    // use RS_Entity instead for refpoint dragging
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
        return m_dimGenericData.text;
    }

    QString ret = "";

    // One space suppresses the text:
    if (m_dimGenericData.text == " ") {
        ret = "";
    }

    // No text prints actual measurement:
    else if (m_dimGenericData.text == "") {
        ret = getMeasuredLabel();
    }

    // Others print the text (<> is replaced by the measurement)
    else {
        QString measuredStr = getMeasuredLabel();
        bool okay = false;
        double measured = measuredStr.toDouble(&okay);

        // Issue #1953: support expressions of measured
        // {<>*<>} for the squared value
        // {sqrt(<>)} for the square root
        if (okay) {
            ret = functionalText(m_dimGenericData.text, measured);
        }
        ret = ret.replace(QString("<>"), getMeasuredLabel());
    }

    return ret;
}

/**
 * Sets a new text for the label.
 */
void RS_Dimension::setLabel(const QString& l) {
    m_dimGenericData.text = l;
}

/**
 * Find intersections between a line and an EntityContainer.  Solutions are
 * sorted along the line before returning.
 *
 * @param infiniteLine Treat the line as infinitely long in both directions.
 */
RS_VectorSolutions RS_Dimension::getIntersectionsLineContainer(
    const RS_Line* l, const RS_EntityContainer* c, bool infiniteLine) {
    RS_VectorSolutions solutions_initial;
    RS_VectorSolutions solutions_filtered;
    const double tol = 1.0e-4;

    // Find all intersections, including those beyond limits of container
    // entities.
    for (RS_Entity* e : *c) {
        solutions_initial.push_back(
            RS_Information::getIntersection(l, e, false)
            );
    }

    // Filter solutions based on whether they are actually on any entities.
    for (const RS_Vector& vp : solutions_initial) {
        for (RS_Entity* e : *c) {
            if (e->isConstruction(true) || e->isPointOnEntity(vp, tol)) {
                // the intersection is at least on the container, now check the line:
                if (infiniteLine) {
                    // The line is treated as infinitely long so we don't need to
                    // check if the intersection is on the line.
                    solutions_filtered.push_back(vp);
                    break;
                }
                else if (l->isConstruction(true) || l->isPointOnEntity(vp, tol)) {
                    solutions_filtered.push_back(vp);
                    break;
                }
            }
        }
    }

    /**
     * We cannot sort the solutions in place because getVector() returns a
     * const vector, so first construct a copy:
     */
    std::vector<RS_Vector> solutions_sorted(solutions_filtered.getVector());
    std::sort(solutions_sorted.begin(), solutions_sorted.end(),
              [l](const RS_Vector& lhs, const RS_Vector& rhs)
              {
                  return l->getProjectionValueAlongLine(lhs)
                      < l->getProjectionValueAlongLine(rhs);
              });

    return RS_VectorSolutions(solutions_sorted);
}

RS_Pen RS_Dimension::getPenForText() {
    RS_Pen result(getTextColor(), RS2::WidthByBlock, RS2::SolidLine);
    return result;
}

RS_Pen RS_Dimension::getPenExtensionLine() {
    RS_Pen result(getExtensionLineColor(), getExtensionLineWidth(), RS2::LineByBlock);
    return result;
}

RS_Pen RS_Dimension::getPenDimensionLine() {
    RS_Pen result(getDimensionLineColor(), getDimensionLineWidth(), RS2::LineByBlock);
    return result;
}

RS_MText* RS_Dimension::createDimText(RS_Vector textPos, double textHeight, double textAngle) {
    RS_MTextData data = createDimTextData(textPos, textHeight, textAngle);
    return addDimText(data);
}

RS_MText* RS_Dimension::addDimText(RS_MTextData &textData) {
    auto mtext = new RS_MText(this, textData);
    addDimComponentEntity(mtext, getPenForText());
    mtext->update();
    return mtext;
}

RS_MTextData RS_Dimension::createDimTextData(RS_Vector textPos, double textHeight, double textAngle) {
    RS_MTextData textData = RS_MTextData(textPos,
                            textHeight, 30.0,
                            RS_MTextData::VAMiddle,
                            RS_MTextData::HACenter,
                            RS_MTextData::LeftToRight,
                            RS_MTextData::Exact,
                            1.0,
                            getLabel(),
                            getTextStyle(),
                            textAngle);
    return textData;
}

/**
 * Creates a horizontal-text dimensioning line (line with one, two or no arrows
 * and "inside horizontal" text).
 *
 * @param forceAutoText Automatically reposition the text label.
 */
void RS_Dimension::createHorizontalTextDimensionLine(const RS_Vector& p1,
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

/**
 * Creates an aligned-text dimensioning line (line with one, two or no arrows
 * and aligned text).
 *
 * @param forceAutoText Automatically reposition the text label.
 */
void RS_Dimension::createAlignedTextDimensionLine(const RS_Vector& p1,
                                                        const RS_Vector& p2, bool arrow1, bool arrow2,
                                                        bool forceAutoText) {
    double dimscale = getGeneralScale();
    double dimtxt = getTextHeight() * dimscale;
    double dimgap = getDimensionLineGap() * dimscale;

    // length of dimension line:
    double distance = p1.distanceTo(p2);
    // arrow size:
    double arrowSize = getArrowSize() * dimscale;

    // do we have to put the arrows outside of the line?
    bool outsideArrows = (distance < arrowSize * 2.5);

    // arrow angles:
    double arrowAngle1, arrowAngle2;

    RS_Pen dimensionPen = getPenDimensionLine();

    // Create dimension line:
    auto* dimensionLine = addDimComponentLine(p1, p2, dimensionPen);

    // Text label:
    RS_Vector textPos;
    double dimAngle1 = dimensionLine->getAngle1();
    bool corrected = false;
    double textAngle = RS_Math::makeAngleReadable(dimAngle1, true, &corrected);

    if (m_dimGenericData.middleOfText.valid && !forceAutoText) {
        textPos = m_dimGenericData.middleOfText;
    }
    else {
        textPos = dimensionLine->getMiddlePoint();

        // rotate text so it's readable from the bottom or right (ISO)
        // quadrant 1 & 4
        double const a = corrected ? -M_PI_2 : M_PI_2;
        RS_Vector distV = RS_Vector::polar(dimgap + dimtxt / 2.0, dimAngle1 + a);

        // move text away from dimension line:
        textPos += distV;

        //// the next update should still be able to adjust this
        ////   auto text position. leave it invalid
        m_dimGenericData.middleOfText = textPos;
    }

    auto text = createDimText(textPos, dimtxt, textAngle);

    // move text to the side:
    RS_Vector distH;
    double usedTextWidth = text->getUsedTextWidth();
    if (usedTextWidth > distance) {
        distH.setPolar(usedTextWidth / 2.0 + distance / 2.0 + dimgap, textAngle);
        text->move(distH);
    }

    // add arrows
    if (outsideArrows == false) {
        arrowAngle1 = dimensionLine->getAngle2();
        arrowAngle2 = dimensionLine->getAngle1();
    }
    else {
        arrowAngle1 = dimensionLine->getAngle1();
        arrowAngle2 = dimensionLine->getAngle2();

        // extend dimension line outside arrows
        RS_Vector dir = RS_Vector::polar(arrowSize * 2, arrowAngle2);
        dimensionLine->setStartpoint(p1 + dir);
        dimensionLine->setEndpoint(p2 - dir);
    }
    double dimtsz = getTickSize() * dimscale;
    if (dimtsz < 0.01) {
        //display arrow
        // Arrows:
        RS_SolidData sd;
        RS_Solid* arrow;

        if (arrow1) {
            // arrow 1
            arrow = new RS_Solid(this, sd);
            arrow->shapeArrow(p1, arrowAngle1, arrowSize);
            addDimComponentEntity(arrow, dimensionPen);
        }

        if (arrow2) {
            // arrow 2:
            arrow = new RS_Solid(this, sd);
            arrow->shapeArrow(p2,arrowAngle2, arrowSize);
            addDimComponentEntity(arrow, dimensionPen);
        }
    }
    else {
        //display ticks
        // Arrows:
        RS_Vector tickVector = RS_Vector::polar(dimtsz, arrowAngle1 + M_PI * 0.25); //tick is 45 degree away

        if (arrow1) { // tick 1
            addDimComponentLine(p1 - tickVector, p1 + tickVector, dimensionPen);
        }

        if (arrow2) { // tick 2:
            addDimComponentLine(p2 - tickVector, p2 + tickVector, dimensionPen);
        }
    }
}

/**
 * Creates a dimensioning line (line with one, two or no arrows and a text).
 *
 * @param forceAutoText Automatically reposition the text label.
 */
void RS_Dimension::createDimensionLine(const RS_Vector& dimLineStart, const RS_Vector& dimLineEnd, bool arrow1, bool arrow2, bool forceAutoText) {
    if (getInsideHorizontalText()) {
        createHorizontalTextDimensionLine(dimLineStart, dimLineEnd, arrow1, arrow2, forceAutoText);
    }
    else {
        createAlignedTextDimensionLine(dimLineStart, dimLineEnd, arrow1, arrow2, forceAutoText);
    }
}

/**
 * @return general factor for linear dimensions. $DIMLFAC
 */
double RS_Dimension::getGeneralFactor() {
    return getGraphicVariable("$DIMLFAC", 1.0, 40);
}

/**
 * @return General scale for dimensions (DIMSCALE)
 */
double RS_Dimension::getGeneralScale() {
    return getGraphicVariable("$DIMSCALE", 1.0, 40);
}

/**
 * @return arrow size in drawing units - $DIMASZ
 */
double RS_Dimension::getArrowSize() {
    return getGraphicVariable("$DIMASZ", 2.5, 40);
}

/**
 * @return tick size in drawing units - $DIMTSZ
 */
double RS_Dimension::getTickSize() {
    return getGraphicVariable("$DIMTSZ", 0., 40);
}

/**
 * @return extension line overlength in drawing units. definition line definition (DIMEXE)
 */
double RS_Dimension::getExtensionLineExtension() {
    return getGraphicVariable("$DIMEXE", 1.25, 40);
}

/**
 * @return extension line offset from entities in drawing units. // distance from entities (DIMEXO)
 */
double RS_Dimension::getExtensionLineOffset() {
    return getGraphicVariable("$DIMEXO", 0.625, 40);
}

/**
 * @return extension line gap to text in drawing units. // text distance to line (DIMGAP)
 */
double RS_Dimension::getDimensionLineGap() {
    return getGraphicVariable("$DIMGAP", 0.625, 40);
}

/**
 * @return Dimension labels text height. // text height (DIMTXT)
 */
double RS_Dimension::getTextHeight() {
    return getGraphicVariable("$DIMTXT", 2.5, 40);
}

/**
 * @return Dimension labels alignment text true= horizontal, false= aligned. - $DIMTIH
 */
bool RS_Dimension::getInsideHorizontalText() {
    int v = getGraphicVariableInt("$DIMTIH", 1);
    if (v > 0) {
        addGraphicVariable("$DIMTIH", 1, 70);
        getGraphicVariableInt("$DIMTIH", 1);
        return true;
    }
    return false;
}

/**
 * @return Dimension fixed length for extension lines true= fixed, false= not fixed - $DIMFXLON
 */
bool RS_Dimension::getFixedLengthOn() {
    int v = getGraphicVariableInt("$DIMFXLON", 0);
    if (v == 1) {
        addGraphicVariable("$DIMFXLON", 1, 70);
        getGraphicVariableInt("$DIMFXLON", 0);
        return true;
    }
    return false;
}

/**
 * @return Dimension fixed length for extension lines.
 */
double RS_Dimension::getFixedLength() {
    return getGraphicVariable("$DIMFXL", 1.0, 40);
}

/**
 * @return extension line Width.
 */
RS2::LineWidth RS_Dimension::getExtensionLineWidth() {
    return RS2::intToLineWidth(getGraphicVariableInt("$DIMLWE", -2)); //default -2 (RS2::WidthByBlock)
}

/**
 * @return dimension line Width.
 */
RS2::LineWidth RS_Dimension::getDimensionLineWidth() {
    return RS2::intToLineWidth(getGraphicVariableInt("$DIMLWD", -2)); //default -2 (RS2::WidthByBlock)
}

/**
 * @return dimension line Color.
 */
RS_Color RS_Dimension::getDimensionLineColor() {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRD", 0));
}

/**
 * @return extension line Color.
 */
RS_Color RS_Dimension::getExtensionLineColor() {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRE", 0));
}

/**
 * @return dimension text Color.
 */
RS_Color RS_Dimension::getTextColor() {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRT", 0));
}

/**
 * @return text style for dimensions.
 */
QString RS_Dimension::getTextStyle() {
    return getGraphicVariableString("$DIMTXSTY", "standard");
}

int RS_Dimension::getDimLinearFormat() {
    return getGraphicVariableInt("$DIMLUNIT", 2);
}

int RS_Dimension::getDimDecimalPlaces() {
    return getGraphicVariableInt("$DIMDEC", 4);
}

int RS_Dimension::getDimTrailingZerosSuppressionMode() {
    return getGraphicVariableInt("$DIMZIN", 1);
}

int RS_Dimension::getDimDecimalFormatSeparatorChar() {
    return getGraphicVariableInt(QStringLiteral("$DIMDSEP"), 0);
}

/**
 * @return the given graphic variable or the default value given in mm
 * converted to the graphic unit.
 * If the variable is not found it is added with the given default
 * value converted to the local unit.
 */
double RS_Dimension::getGraphicVariable(const QString& key, double defMM, int code) {
    double v = getGraphicVariableDouble(key, RS_MINDOUBLE);
    if (v <= RS_MINDOUBLE) {
        addGraphicVariable(key, RS_Units::convert(defMM, RS2::Millimeter, getGraphicUnit()), code);
        v = getGraphicVariableDouble(key, 1.0);
    }
    return v;
}

/**
 * Removes zeros from angle string.
 *
 * @param angle The string representing angle.
 * @param zeros Zeros suppression (0 none, 1 suppress leading, 2 suppress trailing, 3 both)
 * Decimal separator are '.'
 *
 * @ret String with the formatted angle.
 */
QString RS_Dimension::stripZerosAngle(QString angle, int zeros) {
    if (zeros == 0) {
        //do nothing
        return angle;
    }
    if (zeros & 2 && (angle.contains(QString('.')) || angle.contains(QString(',')))) {
        int end = angle.size() - 1;
        QChar format = angle[end--]; //stores & skip format char
        while (end > 0 && angle[end] == QChar('0')) {
            // locate first 0 from end
            end--;
        }
        if (angle[end] == QChar('.')) {
            end--;
        }
        angle.truncate(end + 1);
        angle.append(format);
    }
    if (zeros & 1) {
        if (angle[0] == QChar('0') && angle[1] == QChar('.')) {
            angle = angle.remove(0, 1);
        }
    }
    return angle;
}

/**
 * Removes zeros from linear string.
 *
 * @param linear The string representing linear measure.
 * @param zeros Zeros suppression (see dimzin)
 *
 * @ret String with the formatted linear measure.
 */
QString RS_Dimension::stripZerosLinear(QString linear, int zeros) {
    //do nothing
    if (zeros == 1) {
        return linear;
    }

    // return at least 1 character in string
    if (linear.size() <= 1) {
        return linear;
    }

    // if removing of trailing zeroes is needed
    if (zeros & 8 && (linear.contains(QString('.')) || linear.contains(QString(',')))) {
        // search index
        int i = linear.size() - 1;
        // locate first 0 in row from right
        while (i > 0 && linear[i] == QChar('0')) {
            i--;
        }
        // strip decimal point
        if ((linear[i] == QChar('.') || linear[i] == QChar(',')) && i > 0) {
            i--;
        }
        // strip zeros. Leave at least one character at the beginning
        linear = linear.remove(i + 1, linear.size() - i);
    }
    // if removing of initial zeroes is needed
    if (zeros & 4) {
        int i = 0;
        // locate last 0 in row from left
        while (i < linear.size() - 1 && linear[i] == QChar('0')) {
            i++;
        }
        linear = linear.remove(0, i);
    }
    return linear;
}

void RS_Dimension::move(const RS_Vector& offset) {
    m_dimGenericData.definitionPoint.move(offset);
    m_dimGenericData.middleOfText.move(offset);
}

void RS_Dimension::rotate(const RS_Vector& center, double angle) {
    RS_Vector angleVector(angle);
    m_dimGenericData.definitionPoint.rotate(center, angleVector);
    m_dimGenericData.middleOfText.rotate(center, angleVector);
    m_dimGenericData.angle = RS_Math::correctAngle(m_dimGenericData.angle + angle);
}

void RS_Dimension::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_dimGenericData.definitionPoint.rotate(center, angleVector);
    m_dimGenericData.middleOfText.rotate(center, angleVector);
    m_dimGenericData.angle = RS_Math::correctAngle(m_dimGenericData.angle + angleVector.angle());
}

void RS_Dimension::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_dimGenericData.definitionPoint.scale(center, factor);
    m_dimGenericData.middleOfText.scale(center, factor);
}

void RS_Dimension::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_dimGenericData.definitionPoint.mirror(axisPoint1, axisPoint2);
    m_dimGenericData.middleOfText.mirror(axisPoint1, axisPoint2);
}

void RS_Dimension::update() {
    clear();
    if (isUndone()) {
        return;
    }

    doUpdateDim();

    calculateBorders();
}

void RS_Dimension::updateDim(bool autoText) {
    m_dimGenericData.autoText = autoText;
    clear();
    if (isUndone()) {
        return;
    }
    doUpdateDim();
}

void RS_Dimension::addDimComponentEntity(RS_Entity* en, const RS_Pen &pen) {
    en->setPen(pen);
    en->setLayer(nullptr);
    addEntity(en);
}

RS_Line* RS_Dimension::addDimExtensionLine(RS_Vector start, RS_Vector end) {
    return addDimComponentLine(start, end, getPenExtensionLine());
}


RS_Line* RS_Dimension::addDimDimensionLine(RS_Vector start, RS_Vector end) {
    return addDimComponentLine(start, end, getPenDimensionLine());
}

RS_Arc* RS_Dimension::addDimArc(RS_ArcData& arcData) {
    auto arc =  new RS_Arc(this, arcData);
    RS_Pen pen = getPenDimensionLine();
    addDimComponentEntity(arc, pen);
    return arc;
}


RS_Line* RS_Dimension::addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen &pen) {
    auto line = new RS_Line(this, {start, end});
    line->setPen(pen);
    line->setLayer(nullptr);
    addEntity(line);
    return line;
}

QString RS_Dimension::createLinearMeasuredLabel(double dist) {
    RS_Graphic* graphic = getGraphic();
    QString ret;
    if (graphic) {
        int dimlunit = getDimLinearFormat();
        int dimdec = getDimDecimalPlaces();
        int dimzin = getDimTrailingZerosSuppressionMode();
        RS2::LinearFormat format = graphic->convertLinearFormatDXF2LC(dimlunit); // fixme - sand - move to generic utils!

        ret = RS_Units::formatLinear(dist, getGraphicUnit(), format, dimdec);
        if (format == RS2::Decimal) {
            ret = stripZerosLinear(ret, dimzin);
        }
        //verify if units are decimal and comma separator
        if (format == RS2::Decimal || format == RS2::ArchitecturalMetric){
            if (getDimDecimalFormatSeparatorChar() == 44)
                ret.replace(QChar('.'), QChar(','));
        }
    }
    else {
        ret = QString("%1").arg(dist);
    }
    return ret;
}


double RS_Dimension::prepareLabelLinearDistance(double distance) {
    double dist = distance  * getGeneralFactor();
    if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)) {
        dist = RS_Units::convert(dist);
    }
    return dist;
}
