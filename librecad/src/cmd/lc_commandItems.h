/****************************************************************************
**
* Action that creates a set of lines, with support of angle and "snake" mode

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 Dongxu Li (dongxuli2011 at gmail.com)

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

#ifndef RS_COMMANDITEMS_H
#define RS_COMMANDITEMS_H

#include <QObject>

#include "rs.h"

class QString;

struct LC_CommandText {
    constexpr LC_CommandText(const char* source)
        : source(source) {
    }

    constexpr LC_CommandText(const char* source, bool translatable)
        : source(source)
        , translatable(translatable) {
    }

    constexpr LC_CommandText(const char* source, const char* disambiguation)
        : source(source)
        , disambiguation(disambiguation)
        , translatable(true) {
    }

    const char* source{nullptr};
    const char* disambiguation{nullptr};
    bool translatable{false};
};

struct LC_CommandItem {
    const std::vector<std::pair<LC_CommandText, LC_CommandText>> fullCmdList;
    const std::vector<std::pair<LC_CommandText, LC_CommandText>> shortCmdList;
    RS2::ActionType actionType;
};

/**
 * Constructor. Initiates main command dictionary.
 * mainCommand keeps a map from translated commands to actionType
 * shortCommand keeps a list of translated short commands
 * cmdTranslation contains both ways of mapping between translated and English
 * Command order:
 *      mainCommand (long form): Category (long) + Parameter(s)
 *      shortCommand: 2 letter keycode followed by legacy commands
 * Commands form:
 *    list all <main (full) command and translation string> pairs (category+parameters, i.e "line2p")
 *    Category: (long form for mainCommands, also appear is alias file as "command-untranslated")
 *        line - lin / li / l
 *        ...
 *        (others as req'd)
 *    Parameters:
 *        point - pt / p
 *        circle - cir / c
 *        radius - rad / r
 *        tangent - tan / t
 *        angled - ang / a
 *        vertical - ver / v
 *        horizontal - hor / h
 *        (others as req'd)
 *    Two character key-codes:
 *        first two letters for 'base' command or
 *        first letter of catagory followed by parameter (best choice when possible)
 *           draw line - li
 *           ...
 *           etc.
 */

const LC_CommandItem g_commandList[] = {

        //      draw entity command template
        /*        {
//          mainCommand / long form - full command, appears in alias file (librecad.alias)
            {{"mainCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mainCommand", "translationText")},
             {"alt-mainCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "alt-mainCommand", "translationText")}},
//          Short form(s) - keycode, legacy and single character commands
            {{"keycode", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "altcmd", "translationText")},
             {"(alt-)shortCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "(alt-)shortCommand", "translationText")}}
            RS2::ActionCommand
        },
*/
        {
            {{"cnlayer", LC_CommandText(QT_TRANSLATE_NOOP("QObject", "cnlayer"), true)}},
            {{"cnly", LC_CommandText(QT_TRANSLATE_NOOP("QObject", "cnly"), true)}},
            RS2::ActionLayersAddCmd
        },
        {
            {{"cslayer", LC_CommandText(QT_TRANSLATE_NOOP("QObject", "cslayer"), true)}},
            {{"csly", LC_CommandText(QT_TRANSLATE_NOOP("QObject", "csly"), true)}},
            RS2::ActionLayersActivateCmd
        },
        /* LINE COMMANDS */
        // draw line
        {
            {{"line2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "line2p", "draw line")}},
            {{"li", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "li", "draw line")},
             {"line", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "line", "draw line")},
             {"l", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "l", "draw line")}},
            RS2::ActionDrawLine
        },
        // draw Snake line
        {
            {{"sline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sline", "draw snake line")}},
            {{"sli", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sli", "draw snake line")},
             {"sl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sl", "draw snake line")}},
            RS2::ActionDrawSnakeLine
        },
        // draw Snake-X line
        {
            {{"slinex", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "slinex", "draw snake line (X)")}},
            {{"slix", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "slix", "draw snake line (X)")},
             {"slx", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rlx", "draw snake line (X)")}},
            RS2::ActionDrawSnakeLineX
        },
        // draw Snake-Y line
        {
            {{"sliney", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sliney", "draw snake line (Y)")}},
            {{"sliy", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sliy", "draw snake line (Y)")},
             {"sly", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rly", "draw snake line (Y)")}},
            RS2::ActionDrawSnakeLineY
        },
        // draw line at angle - v2.2.0r2
        {
            {{"lineang", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lineang", "angled line")}},
            {{"la", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "la", "angled line")}},
            RS2::ActionDrawLineAngle
        },
        // draw horizontal line
        {
            {{"linehor", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linehor", "horizontal line")}},   // - v2.2.0r2
            {{"lh", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lh", "horizontal line")}},
            RS2::ActionDrawLineHorizontal
        },
        // draw vertical line
        {
            {{"linever", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linever", "vertical line")}},   // - v2.2.0r2
            {{"lv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lv", "vertical line")}},
            RS2::ActionDrawLineVertical
        },
        // draw rectangle - v2.2.0r2
        {
            {{"linerec", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linerec", "draw rectangle")}},
            {{"re", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "re", "draw rectangle")},
             {"rect", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rect", "draw rectangle")}},
            RS2::ActionDrawLineRectangle
        },
        // draw  rectangle 1 Point
        {
            {{"rect1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rect1", "draw rectangle (1 Point)")}},
            {{"re1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "re1", "draw rectangle (1 Point)")}},
            RS2::ActionDrawRectangle1Point
        },
        // draw  rectangle 2 Points
        {
            {{"rect2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rect2", "draw rectangle (2 Points)")}},
            {{"re2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "re2", "draw rectangle (2 Points)")}},
            RS2::ActionDrawRectangle2Points
        },
        // draw rectangle 3 Points
        {
            {{"rect3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rect3", "draw rectangle (3 Points)")}},
            {{"re3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "re3", "draw rectangle (3 Points)")}},
            RS2::ActionDrawRectangle3Points
        },
        // slice/divide line
        {
            {{"slicel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "slicel", "slice/divide line")}},
            {{"sll", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sll", "slice/divide line")}},
            RS2::ActionDrawSliceDivideLine
        },
        // slice/divide circle/arc
        {
            {{"slicec", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "slicec", "slice/divide circle/arc")}},
            {{"slc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "slc", "slice/divide circle/arc")}},
            RS2::ActionDrawSliceDivideCircle
        },
        // draw star
        {
            {{"star", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "star", "draw star")}},
            {{"st", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "st", "draw star")}},
            RS2::ActionDrawStar
        },
        // draw cross
        {
            {{"cross", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cross", "draw cross for circle")}},
            {{"cx", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cx", "draw cross for circle")}},
            RS2::ActionDrawCenterMark
        },
        {
            {{"bbox", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bbox", "draw bound box")}},
            {{"bb", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bb", "draw bound box")}},
            RS2::ActionDrawBoundingBox
        },
        {
            {{"midline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "midline", "draw middle line")}},
            {{"ml", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ml", "draw mid line")}},
            RS2::ActionDrawCenterLine
        },
        {
                {{"radiant", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "radiant", "draw perspective line")}},
                {{"rl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rl", "draw perspective line")}},
                RS2::ActionDrawLineRadiant
        },
        // draw line of points
        {
            {{"linepoints", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linepoints", "draw line of points")}},
            {{"lpoints", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lpoints", "draw line of points")}},
            RS2::ActionDrawPointsLine
        },
        {
            {{"midpoint", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "midpoint", "draw middle points")}},
            {{"mpoint", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mpoint", "draw middle of points")}},
            RS2::ActionDrawPointsMiddle
        },
        // draw circle by arc
        {
            {{"circlebyarc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circlebyarc", "draw circle by arc")}},
            {{"cba", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cba", "draw circle by arc")}},
            RS2::ActionDrawCircleByArc
        },
        // modify - duplicate
        {
            {{"duplicate", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "duplicate", "duplicate entity")}},
            {{"dup", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dup", "duplicate entity")}},
            RS2::ActionModifyDuplicate
        },
        //  modify - align
        {
            {{"align", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "align", "align entities")}},
            {{"al", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "al", "align entities")}},
            RS2::ActionModifyAlign
        },
        //  modify - align one
        {
            {{"align1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "align1", "align entities")}},
            {{"al1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "al1", "align entities")}},
            RS2::ActionModifyAlignOne
        },
        {
            {{"alignref", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "alignref", "align references")}},
            {{"alr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "alr", "align references")}},
            RS2::ActionModifyAlignRef
        },
        // line join
        {
            {{"linejoin", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linejoin", "lines join")}},
            {{"lj", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lj", "lines join")}},
            RS2::ActionModifyLineJoin
        },
        // break/divide
        {
            {{"breakdivide", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "breakdivide", "break or divide entity")}},
            {{"bd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bd", "break or divide entity")}},
            RS2::ActionModifyBreakDivide
        },
        // Line Gap
        {
            {{"gapline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "gapline", "line gap")}},
            {{"gl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "gl", "line gap")}},
            RS2::ActionModifyBreakDivide
        },
        // draw parallel line
        {
            {{"parallel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "parallel", "create parallel")}},
            {{"linepar", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linepar", "create parallel")},
             {"lineoff", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lineoff", "create parallel")},
             {"pa", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pa", "create parallel")},
             {"ll", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ll", "create parallel")}},
            RS2::ActionDrawLineParallel
        },
        // draw parallel line through point
        {
            {{"lineparthro", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lineparthro", "parallel through point")}},   // - v2.2.0r2
            {{"lp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lp", "parallel through point")},
             {"ptp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ptp", "parallel through point")}},
            RS2::ActionDrawLineParallelThrough
        },
        // draw angle bisector
        {
            {{"linebisect", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linebisect", "angle bisector")}},
            {{"bi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bi", "angle bisector")},
             {"bisect", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bisect", "angle bisector")}},
            RS2::ActionDrawLineBisector
        },
        // draw line tangent to circle from point
        {
            {{"linetancp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linetancp", "tangent point and circle")}},
            {{"lt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lt", "tangent point and circle")},   // - v2.2.0r2
             {"tanpc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tanpc", "tangent point and circle")}},
            RS2::ActionDrawLineTangent1
        },
        // draw line tangent to two existing circles - v2.2.0r2
        {
            {{"linetan2c", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linetan2c", "tangent two circles")}},
            {{"lc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lc", "tangent two circles")}},
            RS2::ActionDrawLineTangent2
        },
        // draw line tangent to an existing circle perpendicular to an existing line - v2.2.0r2
        {
            {{"linetancper", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linetancper", "tangent line and circle")}},
            {{"or", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "or", "tangent line and circle")}},
            RS2::ActionDrawLineOrthTan
        },
        // draw perpendicular line
        {
            {{"lineperp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lineperp", "perpendicular line")}},
            {{"lo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lo", "perpendicular line")},
             {"ortho", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ortho", "perpendicular line")}},
            RS2::ActionDrawLineOrthogonal
        },
        // draw line with relative angle - v2.2.0r2
        {
            {{"linerelang", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linerelang", "relative line")}},
            {{"lr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lr", "relative line")}},
            RS2::ActionDrawLineRelAngle
        },
        // draw polygon by centre and point - v2.2.0r2
        {
            {{"polygoncencor", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polygoncencor", "polygon centre point")}},
            {{"pp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pp", "polygon centre point")},   // - v2.2.0r2
             {"polycp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polycp", "polygon centre point")},
             {"pcp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pcp", "polygon centre point")}},
            RS2::ActionDrawLinePolygonCenCor
        },
        // draw polygon by centre and vertex - v2.2.0r2
        {
            {{"polygoncentan", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polygoncentan", "polygon centre vertex")}},
            {{"pv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pv", "polygon centre vertex")},   // - v2.2.0r2
             {"polyct", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polyct", "polygon centre vertex")}},
            RS2::ActionDrawLinePolygonCenTan
        },
    // draw polygon by vertex and vertex - or side and side
        {
            {{"polygonvv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polygonvv", "polygon vertex vertex")}},
            {{"pvv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pvv", "polygon vertex vertex")}},   // - v2.2.0r2
            RS2::ActionDrawLinePolygonSideSide
        },
        // draw polygon by 2 vertices
        {
            {{"polygon2v", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polygon2v", "polygon by 2 vertices")}},
            {{"p2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "p2", "polygon by 2 vertices")},   // - v2.2.0r2
             {"poly2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "poly2", "polygon by 2 vertices")}},
            RS2::ActionDrawLinePolygonCorCor
        },

        /* CIRCLE COMMANDS */
        // draw circle
        {
            {{"circle", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle", "draw circle")}},
            {{"ci", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ci", "draw circle")},
             {"c", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c", "draw circle")}},   // - v2.2.0r2
            RS2::ActionDrawCircleCenterPoint
        },
        // draw 2 point circle
        {
            {{"circle2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle2p", "circle 2 points")}},
            {{"c2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c2", "circle 2 points")},
             {"c2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c2p", "circle 2 points")}},
            RS2::ActionDrawCircle2Points
        },
        // draw circle 2 points and radius - v2.2.0r2
        {
            {{"circle2pr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle2pr", "circle 2 points radius")}},
            {{"cc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cc", "circle 2 points radius")}},
            RS2::ActionDrawCircle2PointsRadius
        },
        // draw 3 point circle
        {
            {{"circle3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle3p", "circle 3 points")}},
            {{"c3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c3", "circle 3 points")},
             {"c3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c3p", "circle 3 points")}},
            RS2::ActionDrawCircle3Points
        },
        // draw circle with centre point and radius - v2.2.0r2
        {
            {{"circlecr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circlecr", "circle point radius")}},
            {{"cr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cr", "circle point radius")},
             {"ccr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ccr", "circle point radius")}},
            RS2::ActionDrawCircleCenterRadius
        },

        // draw circle tangential to 2 circles and 1 point - v2.2.0r2
        {
            {{"circletan2cp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2cp", "circle 2 tangent point")}},
            {{"tr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tr", "circle 2 tangent point")}},
            RS2::ActionDrawCircleTangental2Entities1Point
        },
        // draw circle Tangential to 2 Points - v2.2.0r2
        {
            {{"circletan2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2p", "circle tangent 2 points")}},
            {{"td", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "td", "circle tangent 2 points")}},
            RS2::ActionDrawCircleTangental1Entity2Points
        },
        //draw circle tangential to 2 circles with specified radius - v2.2.0r2
        {
            {{"circletan2cr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2cr", "circle 2 tangent radius")}},
            {{"tc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tc", "circle 2 tangent radius")}},
            RS2::ActionDrawCircleTan2EntitiesRadius
        },

        // draw circle tangent to 3 objects
        {
            {{"circletan3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan3", "circle tangent to 3")}},
            {{"t3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "t3", "circle tangent to 3")},   // - v2.2.0r2
             {"ct3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ct3", "circle tangent to 3")},
             {"tan3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tan3", "circle tangent to 3")}},
            RS2::ActionDrawCircleTan3Entities
        },

        /* CURVE (ARC) COMMANDS */
        // draw arc by centre point and radius - v2.2.0r2 (Change to previous version)
        {
            {{"arc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc", "arc point radius")}},
            {{"ar", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ar", "arc point radius")},
             {"a", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a", "arc point radius")}},
            RS2::ActionDrawArc
        },
        // draw 3 points arc - v2.2.0r2 (Change to previous version)
        {
            {{"arc3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc3p", "draw 3pt arc")}},
            {{"a3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a3", "draw 3pt arc")}},
            RS2::ActionDrawArc3P
        },
        // draw arc tangential - v2.2.0r2
        {
            {{"arctan", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arctan", "arc tangent")}},
            {{"at", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "at", "arc tangent")}},
            RS2::ActionDrawArcTangential
        },
        // draw 2 points arc - Radius
        {
            {{"arc2pr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc2p3", "draw 2pt arc radius")}},
            {{"a2r", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a2r", "draw 2pt arc radius")}},
            RS2::ActionDrawArc2PRadius
        },
        // draw 2 points arc - Length
        {
            {{"arc2pl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc2pl", "draw 2pt arc length")}},
            {{"a2l", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a2l", "draw 2pt arc length")}},
            RS2::ActionDrawArc2PLength
        },
        // draw 2 points arc - Height
        {
            {{"arc2ph", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc2ph", "draw 2pt arc height")}},
            {{"a2h", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a2h", "draw 2pt arc height")}},
            RS2::ActionDrawArc2PHeight
        },
        // draw 2 points arc - Angle
        {
            {{"arc2pa", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arc2pa", "draw 2pt arc angle")}},
            {{"a2a", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "a2a", "draw 2pt arc angle")}},
            RS2::ActionDrawArc2PAngle
        },

        // draw spline with degrees freedom
        {
            {{"spline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "spline", "draw spline")}},
            {{"sf", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sf", "draw spline")},   // - v2.2.0r2
             {"spl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "spl", "draw spline")}},
            RS2::ActionDrawSpline
        },
        //draw spline through points
        {
            {{"spline2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "spline2", "spline through points")}},
            {{"sp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sp", "spline through points")},   // - v2.2.0r2
             {"stp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "stp", "spline through points")}},
            RS2::ActionDrawSplinePoints
        },
        // draw ellipse arc by axis - v2.2.0r2
        {
            {{"arcellc2ax", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arcellc2ax", "arc ellipse")}},
            {{"ae", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ae", "arc ellipse")}},
            RS2::ActionDrawEllipseArcAxis
        },
        {
            {{"arcellc1ax", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "arcellc1ax", "arc ellipse 1 point")}},
            {{"ae1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ae1", "arc ellipse 1 point")}},
            RS2::ActionDrawEllipseArc1Point
        },
        // draw parabola by 4 points - v2.2.1
        {
            {{"parabola4p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "parabola4p", "Parabola 4 points")}},
            {{"pl4", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pl4", "Parabola 4 points")}},
            RS2::ActionDrawParabola4Points
        },
        // draw parabola by focus directrix - v2.2.1
        {
            {{"parabolafd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "parabolafd", "Parabola focus directrix")}},
            {{"plfd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plfd", "Parabola focus directrix")}},
            RS2::ActionDrawParabolaFocusDiretrix
        },
        //draw freehand line
        {
            {{"free", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "free", "draw freehand line")}},
            {{"fh", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fh", "draw freehand line")},   // - v2.2.0r2
             {"fhl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fhl", "draw freehand line")}},
            RS2::ActionDrawLineFreehand
        },

        /* ELLIPSE COMMANDS */
        // draw ellipse by axis - v2.2.0r2
        {
            {{"ellipsec2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipsec2p", "ellipse axis")}},
            {{"ea", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ea", "ellipse axis")}},
            RS2::ActionDrawEllipseAxis
        },
        {
            {{"ellipsec1p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipsec1p", "ellipse 1 point")}},
            {{"ea1", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ea1", "ellipse 1 point")}},
            RS2::ActionDrawEllipse1Point
        },
        // draw ellipse by foci point - v2.2.0r2
        {
            {{"ellipse3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipse3p", "ellipse foci")}},
            {{"ef", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ef", "ellipse foci")}},
            RS2::ActionDrawEllipseFociPoint
        },
        // draw 4 points ellipse - v2.2.0r2
        {
            {{"ellipse4p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipse4p", "ellipse 4 point")}},
            {{"e4", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "e4", "ellipse 4 point")}},
            RS2::ActionDrawEllipse4Points
        },
        // draw ellipse by center and 3 points - v2.2.0r2
        {
            {{"ellipsec3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipsec3p", "ellipse center 3 point")}},
            {{"e3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "e3", "ellipse center 3 point")}},
            RS2::ActionDrawEllipseCenter3Points
        },
        // draw inscribed ellipse
        {
            {{"ellipseinscribed", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipseinscribed", "inscribed ellipse")}},
            {{"ei", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ei", "inscribed ellipse")},
             {"ie", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ie", "inscribed ellipse")}},
            RS2::ActionDrawEllipseInscribe
        },

        /* POLYLINE COMMANDS */
        // draw polyline
        {
            {{"polyline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "polyline", "draw polyline")}},
            {{"pl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pl", "draw polyline")}},
            RS2::ActionDrawPolyline
        },
        // draw angle line from line
        {
            {{"angleline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "angleline", "draw angle from line")}},
            {{"aline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "aline", "draw angle from line")}},
            RS2::ActionDrawLineAngleRel
        },
        // draw orthogonal line from line
        {
            {{"ortline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rortoline", "draw orthogonal")}},
            {{"oline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rort", "draw orthogonal")}},
            RS2::ActionDrawLineOrthogonalRel
        },
        // draw line from point to line
        {
            {{"point2line", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "point2line", "draw line from point to line")}},
            {{"p2l", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "p2l", "draw line from point to line")}},
            RS2::ActionDrawLineFromPointToLine
        },

        // polyline add node - v2.2.0r2
        {
            {{"plineadd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plineadd", "pl add node")}},
            {{"pi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pi", "pl add node")}},   // - v2.2.0r2
            RS2::ActionPolylineAdd
        },
        // polyline append node - v2.2.0r2
        {
            {{"plineapp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plineapp", "pl append node")}},
            {{"pn", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pn", "pl append node")}},
            RS2::ActionPolylineAppend
        },
        // polyline delete node - v2.2.0r2
        {
            {{"plinedel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plinedel", "pl delete node")}},
            {{"pd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pd", "pl delete node")}},
            RS2::ActionPolylineDel
        },
        // polyline delete between two nodes - v2.2.0r2
        {
            {{"plinedeltwn", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plinedeltwn", "pl del between nodes")}},
            {{"pr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pr", "pl del between nodes")}},
            RS2::ActionPolylineDelBetween
        },
        // polyline trim segments - v2.2.0r2
        {
            {{"plinetrm", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plinetrm", "pl trim segments")}},
            {{"pt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pt", "pl trim segments")}},
            RS2::ActionPolylineTrim
        },
        // equidistant polyline - v2.2.0r2
        {
            {{"plinepar", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plinepar", "pl equidistant")}},
            {{"pe", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pe", "pl equidistant")}},
            RS2::ActionPolylineEquidistant
        },
        // polyline from existing segments - v2.2.0r2
        {
            {{"plinejoin", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "plinejoin", "pl join")}},
            {{"pj", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pj", "pl join")}},
            RS2::ActionPolylineSegment
        },
        // Dual curve
        {
            {{"dual", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dual", "create dual geometries")}},
            {{"du", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "du", "create dual geometries")}},
            RS2::ActionDrawDual
        },
        /* SELECT COMMANDS */
        // Select all entities
        {
            {{"selectall", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "selectall", "Select all entities")}},
            {{"sa", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sa", "Select all entities")}},
            RS2::ActionSelectAll
        },
        // DeSelect all entities
        {
            {{"deselectall", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "deselectall", "deselect all entities")}},
            {{"sx", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sx", "deselect all entities")},
             {"tn", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tn", "deselect all entities")}},   // - v2.2.0r2
            RS2::ActionDeselectAll
        },
        // Invert selection - v2.2.0r2
        {
            {{"invertselect", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "invertselect", "invert select")}},
            {{"is", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "is", "invert select")}},
            RS2::ActionSelectInvert
        },
     {
                {{"selectquick", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "selectquick", "select quick")}},
                {{"sq", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sq", "select quick")}},
                RS2::ActionSelectQuick
        },
      {
             {{"smtoggle", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "smtoggle", "select mode toggle")}},
             {{"smt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "smt", "select mode toggle")}},
            RS2::ActionSelectModeToggle
            },

        /* Remaining select tools require the mouse - no point in adding commands. */

        /* DIMENSION COMMANDS */
        // dimension aligned
        {
            {{"dimaligned", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimaligned", "dimension - aligned")}},
            {{"ds", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ds", "dimension - aligned")}},   // - v2.2.0r2 (Change to previous version)
            RS2::ActionDimAligned
        },
        // dimension linear
        {
            {{"dimlinear", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimlinear", "dimension - linear")}},
            {{"dl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dl", "dimension - linear")}},
            RS2::ActionDimLinear
        },
        // dimension ordinate
        {
            {{"dimord", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimord", "dimension - ordinate")}},
            {{"do", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "do", "dimension - ordinate")}},
            RS2::ActionDimOrdinate
        },
        {
                {{"dimordrebase", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimordrebase", "dimension - ordinate")}},
                {{"dor", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dor", "dimension - ordinate")}},
                RS2::ActionDimOrdRebase
            },
        // dimension horizontal
        {
            {{"dimhorizontal", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimhorizontal", "dimension - horizontal")}},
            {{"dh", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dh", "dimension - horizontal")}},
            RS2::ActionDimLinearHor
        },
        // dimension vertical
        {
            {{"dimvertical", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimvertical", "dimension - vertical")}},
            {{"dv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dv", "dimension - vertical")}},
            RS2::ActionDimLinearVer
        },
        // dimension radius
        {
            {{"dimradius", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimradius", "dimension - radial")},
             {"dimradial", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimradial", "dimension - radial")}},
            {{"dr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dr", "dimension - radial")}},
            RS2::ActionDimRadial
        },
        // dimension diameter
        {
            {{"dimdiameter", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimdiameter", "dimension - diametric")}},
            {{"dd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dd", "dimension - diametric")},
             {"dimdiametric", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimdiametric", "dimension - diametric")}},
            RS2::ActionDimDiametric
        },
        // dimension angular
        {
            {{"dimangular", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimangular", "dimension - angular")}},
            {{"da", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "da", "dimension - angular")},
             {"dan", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dan", "dimension - angular")}},
            RS2::ActionDimAngular
        },
        // dimension leader
        {
            {{"dimleader", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimleader", "dimension - leader")}},
            {{"ld", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ld", "dimension - leader")}},
            RS2::ActionDimLeader
        },
        // dimension regenerate
        {
            {{"dimregen", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dimregen", "dimension - regenerate")}},
            {{"dg", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dg", "dimension - regenerate")}},
            RS2::ActionDimRegenerate
        },

        /* MODIFY COMMANDS */
        // move
        {
            {{"modmove", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modmove", "modify - move (copy)")}},
            {{"mv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mv", "modify - move (copy)")}},
            RS2::ActionModifyMove
        },
        // rotate
        {
            {{"rotate", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rotate", "modify - rotate")}},
            {{"modrotate", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modrotate", "modify - rotate")},
             {"ro", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ro", "modify - rotate")}},
            RS2::ActionModifyRotate
        },
        // scale
        {
            {{"scale", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "scale", "modify - scale")}},
            {{"modscale", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modscale", "modify - scale")},
             {"sz", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sz", "modify - scale")}},
            RS2::ActionModifyScale
        },
        // mirror  (Removed extra space from translation sting.)
        {
            {{"mirror", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mirror", "modify -  mirror")}},
            {{"modmirror", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modmirror", "modify -  mirror")},
             {"mi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mi", "modify -  mirror")}},
            RS2::ActionModifyMirror
        },
        // move and rotate - v2.2.0r2
        {
            {{"modmovrot", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modmovrot", "modify - move rotate")}},
            {{"mr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mr", "modify - move rotate")}},
            RS2::ActionModifyMoveRotate
        },
        // rotate two - v2.2.0r2
        {
            {{"mod2rot", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mod2rot", "modify - rotate2")}},
            {{"r2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "r2", "modify - rotate2")}},
            RS2::ActionModifyRotateTwice
        },
        // revert (Removed extra space from translation sting.)
        {
            {{"modrevert", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modrevert", "modify -  revert direction")}},
            {{"md", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "md", "modify -  revert direction")},
             {"rev", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rev", "modify -  revert direction")}},
            RS2::ActionModifyRevertDirection
        },
        // trim
        {
            {{"trim", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "trim", "modify - trim (extend)")}},
            {{"modtrim", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modtrim", "modify - trim (extend)")},
             {"tm", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tm", "modify - trim (extend)")}},
            RS2::ActionModifyTrim
        },
        // trim2
        {
            {{"modtrim2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modtrim2", "modify - multi trim (extend)")}},
            {{"t2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "t2", "modify - multi trim (extend)")},
             {"tm2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tm2", "modify - multi trim (extend)")}},
            RS2::ActionModifyTrim2
        },
        // lengthen
        {
            {{"modlengthen", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modlengthen", "modify - lengthen")}},
            {{"le", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "le", "modify - lengthen")}},
            RS2::ActionModifyTrimAmount
        },
        // offset
        {
            {{"offset", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "offset", "modify - offset")}},
            {{"modoffset", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modoffset", "modify - offset")},
             {"o", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "o", "modify - offset")},
             {"mo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mo", "modify - offset")},   // - v2.2.0r2
             {"moff", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "moff", "modify - offset")}},
            RS2::ActionModifyOffset
        },
        // bevel
        {
            {{"bevel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bevel", "modify - bevel")},
             {"chamfer", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "chamfer", "modify - bevel")}},
            {{"modbevel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modbevel", "modify - bevel")},
             {"bev", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bev", "modify - bevel")},
             {"ch", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ch", "modify - bevel")}},
            RS2::ActionModifyBevel
        },
        // fillet
        {
            {{"fillet", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fillet", "modify - fillet")}},
            {{"modfillet", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modfillet", "modify - fillet")},
             {"fi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fi", "modify - fillet")}},
            RS2::ActionModifyRound
        },
        // divide
        {
            {{"moddivide", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "moddivide", "modify - divide (cut)")},
             {"cut", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cut", "modify - divide (cut)")}},
            {{"div", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "div", "modify - divide (cut)")},
             {"di", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "di", "modify - divide (cut)")}},
            RS2::ActionModifyCut
        },
        // stretch
        {
            {{"modstretch", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modstretch", "modify - stretch")}},
            {{"ss", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ss", "modify - stretch")}},
            RS2::ActionModifyStretch
        },
        // modify properties
        {
            {{"modproperties", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modproperties", "modify properties")}},
            {{"prop", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "prop", "modify properties")},
             {"mp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mp", "modify properties")}},
            RS2::ActionModifyEntity
        },
        // modify attributes
        {
            {{"modattr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modattr", "modify attribute")}},
            {{"attr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "attr", "modify attribute")},
             {"ma", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ma", "modify attribute")}},
            RS2::ActionModifyAttributes
        },
        // explode text - v2.2.0r2
        {
            {{"modexpltext", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modexpltext", "explode text strings")}},
            {{"xt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "xt", "explode text strings")}},
            RS2::ActionModifyExplodeText
        },
        // explode
        {
            {{"explode", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "explode", "explode block/polyline into entities")}},
            {{"modexplode", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modexplode", "explode block/polyline into entities")},
             {"xp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "xp", "explode block/polyline into entities")}},
            RS2::ActionBlocksExplode
        },
        // delete
        {
            {{"moddelete", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "moddelete", "modify - delete (erase)")}},
            {{"er", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "er", "modify - delete (erase)")},
             {"del", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "del", "modify - delete (erase)")}},
            RS2::ActionModifyDelete
        },

        /* INFO COMMANDS */
        // Distance Point to Point
        {
            {{"infodistance", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "infodistance", "distance point to point")}},
            {{"id", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "id", "distance point to point")},   // - v2.2.0r2
             {"dist", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dist", "distance point to point")},
             {"dpp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dpp", "distance point to point")}},
            RS2::ActionInfoDistPoint2Point
        },
        // Distance Entity to Point
        {
            {{"infodistep", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "infodistep", "distance entity to point")}},
            {{"ii", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ii", "distance entity to point")},   // - v2.2.0r2
             {"dep", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dep", "distance entity to point")}},
            RS2::ActionInfoDistEntity2Point
        },
        // Measure angle
        {
            {{"infoangle", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "infoangle", "measure angle")}},
            {{"ia", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ia", "measure angle")},   // - v2.2.0r2
             {"ang", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ang", "measure angle")}},
            RS2::ActionInfoAngle
        },
        // Measure area
        {
            {{"infoarea", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "infoarea", "measure area")}},
            {{"aa", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "aa", "measure area")}},   // - v2.2.0r2
            RS2::ActionInfoArea
        },

        /* OTHER COMMANDS */
        // draw mtext
        {
            {{"mtext", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mtext", "draw mtext")}},
            {{"mt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mt", "draw mtext")},   // - v2.2.0r2
             {"mtxt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mtxt", "draw mtext")}},
            RS2::ActionDrawMText
        },
        // draw text
        {
            {{"text", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "text", "draw text")}},
            {{"tx", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tx", "draw text")},   // - v2.2.0r2
             {"txt", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "txt", "draw text")}},
            RS2::ActionDrawText
        },
        // draw hatch
        {
            {{"hatch", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "hatch", "draw hatch")}},
            {{"ha", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ha", "draw hatch")}},
            RS2::ActionDrawHatch
        },
        // draw point
        {
            {{"point", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "point", "draw point")}},
            {{"po", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "po", "draw point")}},
            RS2::ActionDrawPoint
        },

        /* SNAP COMMANDS */
        /* snap exclusive - v2.2.0r2
        {
            {{"snapexcl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapexcl", "snap - excl")}},
            {{"sx", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sx", "snap - excl")},
             {"ex", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ex", "snap - excl")}},
            RS2::ActionSnapExcl  // Not present
        }, */
        // snap free
        {
            {{"snapfree", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapfree", "snap - free")}},
            {{"so", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "so", "snap - free")},
             {"os", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "os", "snap - free")}},
            RS2::ActionSnapFree
        },
        // snap center
        {
            {{"snapcenter", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapcenter", "snap - center")}},
            {{"sc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sc", "snap - center")}},
            RS2::ActionSnapCenter
        },
        //snap dist
        {
            {{"snapdist", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapdist", "snap - distance to endpoints")}},
            {{"sd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sd", "snap - distance to endpoints")}},
            RS2::ActionSnapDist
        },
        // snap end
        {
            {{"snapend", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapend", "snap - end points")}},
            {{"se", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "se", "snap - end points")}},
            RS2::ActionSnapEndpoint
        },
        // snap grid
        {
            {{"snapgrid", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapgrid", "snap - grid")}},
            {{"sg", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sg", "snap - grid")}},
            RS2::ActionSnapGrid
        },
        // snap intersection
        {
            {{"snapintersection", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapintersection", "snap - intersection")}},
            {{"si", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "si", "snap - intersection")}},
            RS2::ActionSnapIntersection
        },
        // snap middle
        {
            {{"snapmiddle", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapmiddle", "snap - middle points")}},
            {{"sm", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sm", "snap - middle points")}},
            RS2::ActionSnapMiddle
        },
        // snap on entity
        {
            {{"snaponentity", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snaponentity", "snap - on entity")}},
            {{"sn", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sn", "snap - on entity")},
             {"np", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "np", "snap - on entity")}},
            RS2::ActionSnapOnEntity
        },

        /* Snap Middle Manual */
        {
            //list all <full command, translation> pairs
            {{"snapmiddlemanual", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapmiddlemanual", "snap middle manual")}},
            {{"snapmanual", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "snapmanual", "snap middle manual")},
             {"smm", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "smm", "snap middle manual")}},

            RS2::ActionSnapMiddleManual
        },

        // set relative zero
        {
            {{"setrelativezero", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "setrelativezero", "set relative zero position")}},
            {{"rz", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rz", "set relative zero position")}},
            RS2::ActionSetRelativeZero
        },
        // snap restrictions
        {
            {{"restrictnothing", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "restrictnothing", "restrict - nothing")}},
            {{"rn", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rn", "restrict - nothing")}},
            RS2::ActionRestrictNothing
        },
        // snap orthogonal
        {
            {{"restrictorthogonal", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "restrictorthogonal", "restrict - orthogonal")}},
            {{"rr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rr", "restrict - orthogonal")}},
            RS2::ActionRestrictOrthogonal
        },
        // snap horizontal
        {
            {{"restricthorizontal", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "restricthorizontal", "restrict - horizontal")}},
            {{"rh", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rh", "restrict - horizontal")}},
            RS2::ActionRestrictHorizontal
        },
        // snap vertical
        {
            {{"restrictvertical", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "restrictvertical", "restrict - vertical")}},
            {{"rv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rv", "restrict - vertical")}},
            RS2::ActionRestrictVertical
        },

        /* MENU COMMANDS */
        /* EDIT COMMANDS */
        // kill actions
        {
            {{"kill", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "kill", "kill all actions")}},
            {{"ki", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ki", "kill all actions")},
             {"k", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "k", "kill all actions")}},
            RS2::ActionEditKillAllActions
        },
        // undo cycle
        {
            {{"undo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "undo", "undo cycle")}},
            {{"un", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "un", "undo cycle")},
             {"u", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "u", "undo cycle")}},
            RS2::ActionEditUndo
        },
        // redo cycle
        {
            {{"redo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "redo", "redo cycle")}},
            {{"rd", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rd", "redo cycle")},
             {"r", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "r", "redo cycle")}},
            RS2::ActionEditRedo
        },

        /* OPTIONS COMMANDS */
        // Drawing Prefs - v2.2.0r2
        {
            {{"drawpref", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "drawpref", "drawing preferences")}},
            {{"dp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dp", "drawing preferences")}},
            RS2::ActionOptionsDrawing
        },

        /* VIEW COMMANDS */
        // zoom redraw
        {
            {{"regen", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "regen", "zoom - redraw")},
             {"redraw", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "redraw", "zoom - redraw")}},
            {{"rg", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rg", "zoom - redraw")},
             {"zr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zr", "zoom - redraw")}},
            RS2::ActionZoomRedraw
        },
        // zoom in
        {
            {{"zoomin", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoomin", "zoom - in")}},
            {{"zi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zi", "zoom - in")}},
            RS2::ActionZoomIn
        },
        // zoom out
        {
            {{"zoomout", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoomout", "zoom - out")}},
            {{"zo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zo", "zoom - out")}},
            RS2::ActionZoomOut
        },
        // zoom auto
        {
            {{"zoomauto", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoomauto", "zoom - auto")}},
            {{"za", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "za", "zoom - auto")}},
            RS2::ActionZoomAuto
        },
        // zoom previous
        {
            {{"zoomprevious", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoomprevious", "zoom - previous")}},
            {{"zv", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zv", "zoom - previous")}},
            RS2::ActionZoomPrevious
        },
        // zoom window
        {
            {{"zoomwindow", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoomwindow", "zoom - window")}},
            {{"zw", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zw", "zoom - window")}},
            RS2::ActionZoomWindow
        },
        // zoom pan
        {
            {{"zoompan", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zoompan", "zoom - pan")}},
            {{"zp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "zp", "zoom - pan")}},
            RS2::ActionZoomPan
        }
    };

    // translations
inline std::vector<std::pair<LC_CommandText, LC_CommandText>> g_transList={
        {"angle",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle"), true)},
        {"angle1",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle1"), true)},
        {"angle2",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle2"), true)},
        {"dpi",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "dpi"), true)},
        {"close",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "close"), true)},
        {"chordlen",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "chordlen"), true)},
        {"columns",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "columns"), true)},
        {"columnspacing",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "columnspacing"), true)},
        {"equation",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "equation"), true)},
        {"factor",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "factor"), true)},
        {"length",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "length"), true)},
        {"length1",LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length1", "bevel/fillet length1")},
        {"length2",LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length2", "bevel/fillet length2")},
        {"number",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "number"), true)},
        {"radius",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "radius"), true)},
        {"rows",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "rows"), true)},
        {"rowspacing",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "rowspacing"), true)},
        {"through",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "through"), true)},
        {"trim",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "trim"), true)},

        // commands for relative line drawing actions
        {"x",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "x"), true)},
        {"y",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "y"), true)},
        {"p",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "p"), true)},
        {"anglerel",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "anglerel"), true)},
        {"start",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "start"), true)},

        // commands for line angle rel action
        {"offset",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "offset"), true)},
        {"linesnap",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "linesnap"), true)},
        {"ticksnap",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "ticksnap"), true)},

        // rectangle one point
        {"width",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "width"), true)},
        {"height",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "height"), true)},
        {"pos",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "pos"), true)},
        {"size",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "size"), true)},
        {"bevels",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bevels"), true)},
        {"nopoly",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "nopoly"), true)},
        {"usepoly",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "usepoly"), true)},
        {"corners",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "corners"), true)},
        {"str",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "str"), true)},
        {"round",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "round"), true)},
        {"bevels",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bevels"), true)},
        {"snap1",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "snap1"), true)},
        {"topl",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "topl"), true)},
        {"top",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "top"), true)},
        {"topr",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "topr"), true)},
        {"left",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "left"), true)},
        {"middle",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "middle"), true)},
        {"right",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "right"), true)},
        {"bottoml",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bottoml"), true)},
        {"bottom",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bottom"), true)},
        {"bottomr",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bottomr"), true)},
        {"snapcorner",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "snapcorner"), true)},
        {"snapshift",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "snapshift"), true)},
        {"sizein",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "sizein"), true)},
        {"sizeout",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "sizeout"), true)},
        {"hor",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "hor"), true)},
        {"vert",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "vert"), true)},

        // rect 2 points
        {"snap2",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "snap2"), true)},
        {"corner",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "corner"), true)},
        {"mid-vert",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "mid-vert"), true)},
        {"mid-hor",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "mid-hor"), true)},
        // rect 3 points
        {"quad",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "quad"), true)},
        {"noquad",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "noquad"), true)},
        {"angle_inner",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle_inner"), true)},

        // line points
        {"edges",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "edges"), true)},
        {"edge-none",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "edge-none"), true)},
        {"edge-both",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "edge-both"), true)},
        {"edge-start",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "edge-start"), true)},
        {"edge-end",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "edge-end"), true)},
        {"end",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "end"), true)},
        {"both",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "both"), true)},
        {"none",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "none"), true)},
        {"fit",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "fit"), true)},
        {"nofit",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "nofit"), true)},
        {"dist_fixed",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "dist_fixed"), true)},
        {"dist_flex",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "dist_flex"), true)},
        {"distance",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "distance"), true)},

        // line radiant
         {"radiant",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "radiant"), true)},
         {"active",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "active"), true)},
         {"lentype",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "lentype"), true)},
         {"fixed",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "fixed"), true)},

        // star
        {"sym",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "sym"), true)},
        {"nosym",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "nosym"), true)},
        {"snap",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "snap"), true)},
        {"s",LC_CommandText QT_TRANSLATE_NOOP3("QObject", "s", "snap start")},
        {"m",LC_CommandText QT_TRANSLATE_NOOP3("QObject", "m", "snap middle")},
        {"e",LC_CommandText QT_TRANSLATE_NOOP3("QObject", "e", "snap end")},

        // commands

        /** following are reversed translation,i.e.,from translated to english **/
        //not used as command keywords
        // used in function,checkCommand()
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle"), true),"angle"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle1"), true),"angle1"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle2"), true),"angle2"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ang", "angle"),"angle"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "an", "angle"),"angle"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "center"), true),"center"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cen", "center"),"center"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ce", "center"),"center"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "chordlen"), true),"chordlen"},
        //    {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length", "chord length"),"chord length"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cl", "chordlen"),"chordlen"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "close"), true),"close"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c", "close"),"close"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "columns"), true),"columns"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cols", "columns"),"columns"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "co", "columns"),"columns"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "columnspacing", "columnspacing for inserts"),"columnspacing"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "colspacing", "columnspacing for inserts"),"columnspacing"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cs", "columnspacing for inserts"),"columnspacing"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "factor"), true),"factor"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fact", "factor"),"factor"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "f", "factor"),"factor"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "equation"), true),"equation"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "eqn", "equation"),"equation"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "eq", "equation"),"equation"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "help"), true),"help"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "?", "help"),"help"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length", "length"),"length"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "len", "length"),"length"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "l", "length"),"length"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length1", "length1"),"length1"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "len1", "length1"),"length1"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "l1", "length1"),"length1"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length2", "length2"),"length2"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "len2", "length2"),"length2"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "l2", "length2"),"length2"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "number", "number"),"number"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "num", "number"),"number"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "n", "number"),"number"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "radius"), true),"radius"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ra", "radius"),"radius"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "reversed", "reversed"),"reversed"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rev", "reversed"),"reversed"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rev", "reversed"),"reversed"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "row", "row"),"row"},

        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rowspacing", "rowspacing for inserts"),"rowspacing"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "rs", "rowspacing for inserts"),"rowspacing"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "text"), true),"text"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "t", "text"),"text"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "through"), true),"through"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "t", "through"),"through"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "undo"), true),"undo cycle"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "u", "undo cycle"),"undo"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "redo"), true),"redo cycle"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "r", "redo redo cycle"),"redo"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "back"), true),"back"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "b", "back"),"back"},
        //printer preview
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "bw"), true), "blackwhite"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "blackwhite"), true), "blackwhite"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "color"), true), "color"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "paperoffset"), true),"paperoffset"},
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "graphoffset"), true),"graphoffset"}

        // fixme - add reversive translation for added commands
    };

#endif
