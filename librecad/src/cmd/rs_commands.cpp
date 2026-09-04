/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include<vector>

#include <QObject>
#include <QTextStream>

#include "rs_commands.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "rs_system.h"

namespace {
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
    std::vector<std::pair<LC_CommandText, LC_CommandText>> const fullCmdList;
    std::vector<std::pair<LC_CommandText, LC_CommandText>> const shortCmdList;
    RS2::ActionType actionType;
};

QString resolveCommandText(const LC_CommandText& text) {
    return text.translatable
               ? RS_SYSTEM->translateCommand(text.source, text.disambiguation)
               : QString::fromUtf8(text.source);
}

// Prefixes for function-, Meta- and Alt- keys.
const char* g_FnPrefix = "Fn";
const char* g_AltPrefix = "Alt-";
const char* g_MetaPrefix = "Meta-";

// helper function to check and report command collision
template<typename T1, typename T2>
bool isCollisionFree(std::map<T1, T2> const& lookUp, T1 const& key, T2 const& value, QString cmd = {})
{
    if(!lookUp.count(key) || lookUp.at(key) == value)
        return true;

    //report command string collision
    QString msg = __FILE__ + QObject::tr(": duplicated command: %1 is already taken by %2");
    if constexpr (std::is_same_v<T2, RS2::ActionType>)
        msg = msg.arg(key).arg(cmd);
    else
        msg = msg.arg(key).arg(value);

    RS_DEBUG->print(RS_Debug::D_ERROR, "%s\n", msg.toStdString().c_str());
    return false;
}

// write alias file
void writeAliasFile(QFile& file,
                    const std::map<QString, RS2::ActionType>& shortCommands,
                    const std::map<QString, RS2::ActionType>& mainCommands
                    )
{
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QTextStream ts(&file);
    ts << "#LibreCAD alias v1\n\n";
    ts << "# lines starting with # are comments\n";
    ts << "# format are:\n";
    ts << R"(# <alias>\t<command-untranslated>)" "\n";
    ts << "# example\n";
    ts << "# l\tline\n\n";

    // the reverse look up from action type to avoid quadratic time complexity
    auto actionToMain = std::map<RS2::ActionType, QString>();
    for(auto const& pairMain: mainCommands)
        if (actionToMain.count(pairMain.second) == 0)
            actionToMain.emplace(pairMain.second, pairMain.first);
    for(auto const& pairShort: shortCommands)
        if (actionToMain.count(pairShort.second) == 1)
            ts<<pairShort.first<<'\t'<<actionToMain.at(pairShort.second)<<'\n';
}
}


RS_Commands* RS_Commands::instance() {
    static RS_Commands* uniqueInstance = new RS_Commands();
    return uniqueInstance;
}


/**
 * Constructor. Initiates main command dictionary.
 * mainCommand keeps a map from translated commands to actionType
 * shortCommand keeps a list of translated short commands
 * m_cmdTranslation contains both ways of mapping between translated and English
 * Command order:
 *      mainCommand (long form): Category (long) + Parameter(s)
 *      shortCommand: 2 letter keycode followed by legacy commands
 * Commands form:
 *    list all <main (full) command and translation string> pairs (category+parameters, i.e "line2p")
 *    Category: (long form for m_mainCommands, also appear is alias file as "command-untranslated")
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

RS_Commands::RS_Commands() {
    const std::vector<LC_CommandItem> commandList{

        //      draw entity command template
        /*        {
//          mainCommand / long form - full command, appears in alias file (librecad.alias)
            {{"mainCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mainCommand", "translationText")},
             {"alt-mainCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "alt-mainCommand", "translationText")}},
//          Short form(s) - keycode, legacy and single character commands
            {{"keycode", QObject::tr("altcmd, "translationText")},
             {"(alt-)shortCommand", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "(alt-)shortCommand", "translationText")}}
            RS2::ActionCommand
        },
*/

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
            RS2::ActionDrawCross
        },
        // draw line of points
        {
            {{"linepoints", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linepoints", "draw line of points")}},
            {{"lpoints", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lpoints", "draw line of points")}},
            RS2::ActionDrawLinePoints
        },
        // draw circle by arc
        {
            {{"circlebyarc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "criclebyarc", "draw circle by arc")}},
            {{"cba", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cba", "draw circle by arc")}},
            RS2::ActionDrawCircleByArc
        },
        // draw circle by arc
        {
            {{"duplicate", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "duplicate", "duplicate entity")}},
            {{"dup", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dup", "duplicate entity")}},
            RS2::ActionModifyDuplicate
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
            {{"linepar", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "linepar", "create parallel")},
                {"lineoff", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "lineoff", "create parallel")}},
            {{"pa", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "pa", "create parallel")},
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
            RS2::ActionDrawCircle
        },
        // draw 2 point circle
        {
            {{"circle2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle2p", "circle 2 points")}},
            {{"c2", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c2", "circle 2 points")},
                {"c2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c2p", "circle 2 points")}},
            RS2::ActionDrawCircle2P
        },
        // draw circle 2 points and radius - v2.2.0r2
        {
            {{"circle2pr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle2pr", "circle 2 points radius")}},
            {{"cc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cc", "circle 2 points radius")}},
            RS2::ActionDrawCircle2PR
        },
        // draw 3 point circle
        {
            {{"circle3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circle3p", "circle 3 points")}},
            {{"c3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c3", "circle 3 points")},
                {"c3p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "c3p", "circle 3 points")}},
            RS2::ActionDrawCircle3P
        },
        // draw circle with centre point and radius - v2.2.0r2
        {
            {{"circlecr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circlecr", "circle point radius")}},
            {{"cr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cr", "circle point radius")},
                {"ccr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ccr", "circle point radius")}},
            RS2::ActionDrawCircleCR
        },

        // draw circle tangential to 2 circles and 1 point - v2.2.0r2
        {
            {{"circletan2cp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2cp", "circle 2 tangent point")}},
            {{"tr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tr", "circle 2 tangent point")}},
            RS2::ActionDrawCircleTan2_1P
        },
        // draw circle Tangential to 2 Points - v2.2.0r2
        {
            {{"circletan2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2p", "circle tangent 2 points")}},
            {{"td", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "td", "circle tangent 2 points")}},
            RS2::ActionDrawCircleTan1_2P
        },
        //draw circle tangential to 2 circles with specified radius - v2.2.0r2
        {
            {{"circletan2cr", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan2cr", "circle 2 tangent radius")}},
            {{"tc", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tc", "circle 2 tangent radius")}},
            RS2::ActionDrawCircleTan2
        },

        // draw circle tangent to 3 objects
        {
            {{"circletan3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "circletan3", "circle tangent to 3")}},
            {{"t3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "t3", "circle tangent to 3")},   // - v2.2.0r2
                {"ct3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ct3", "circle tangent to 3")},
                {"tan3", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tan3", "circle tangent to 3")}},
            RS2::ActionDrawCircleTan3
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
            RS2::ActionDrawParabolaFD
        },
        //draw freehand line
        {
            {{"free", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "free", "draw freehand line")}},
            {{"fh", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fh", "draw freehand line")},   // - v2.2.0r2
                {"fhl", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fhl", "draw freehand line")}},
            RS2::ActionDrawLineFree
        },

        /* ELLIPSE COMMANDS */
        // draw ellipse by axis - v2.2.0r2
        {
            {{"ellipsec2p", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ellipsec2p", "ellipse axis")}},
            {{"ea", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ea", "ellipse axis")}},
            RS2::ActionDrawEllipseAxis
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
            {{"aline", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "angleline", "draw angle from line")}},
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
            RS2::ActionToolRegenerateDimensions
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
            {{"modrotate", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modrotate", "modify - rotate")}},
            {{"ro", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ro", "modify - rotate")}},
            RS2::ActionModifyRotate
        },
        // scale
        {
            {{"modscale", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modscale", "modify - scale")}},
            {{"sz", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "sz", "modify - scale")}},
            RS2::ActionModifyScale
        },
        // mirror  (Removed extra space from translation sting.)
        {
            {{"modmirror", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modmirror", "modify -  mirror")}},
            {{"mi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mi", "modify -  mirror")}},
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
            RS2::ActionModifyRotate2
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
            {{"modtrim", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modtrim", "modify - trim (extend)")}},
            {{"tm", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "tm", "modify - trim (extend)")}},
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
            {{"modoffset", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modoffset", "modify - offset")}},
            {{"mo", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "mo", "modify - offset")},   // - v2.2.0r2
                {"moff", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "moff", "modify - offset")}},
            RS2::ActionModifyOffset
        },
        // bevel
        {
            {{"modbevel", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modbevel", "modify - bevel")}},
            {{"bev", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "bev", "modify - bevel")},
                {"ch", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ch", "modify - bevel")}},
            RS2::ActionModifyBevel
        },
        // fillet
        {
            {{"modfillet", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modfillet", "modify - fillet")}},
            {{"fi", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "fi", "modify - fillet")}},
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
            {{"modexplode", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "modexplode", "explode block/polyline into entities")}},
            {{"xp", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "xp", "explode block/polyline into entities")}},
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
            RS2::ActionInfoDist
        },
        // Distance Entity to Point
        {
            {{"infodistep", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "infodistep", "distance entity to point")}},
            {{"ii", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ii", "distance entity to point")},   // - v2.2.0r2
                {"dep", LC_CommandText QT_TRANSLATE_NOOP3("QObject", "dep", "distance entity to point")}},
            RS2::ActionInfoDist2
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
            RS2::ActionDrawHatchNoSelect
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

    for(auto const& c0: commandList){
        auto const act=c0.actionType;
        //add full commands
        for(auto const& p0: c0.fullCmdList){
            const QString original = resolveCommandText(p0.first);
            const QString translated = resolveCommandText(p0.second);
            if (isCollisionFree(m_cmdTranslation, original, translated))
                m_cmdTranslation[original] = translated;
            if (isCollisionFree(m_mainCommands, translated, act, m_actionToCommand.count(act) ? m_actionToCommand[act] : QString{})) {
                m_mainCommands[translated] = act;
                m_actionToCommand.emplace(act, translated);
            }
        }
        for(auto const& p0: c0.fullCmdList){
            const QString original = resolveCommandText(p0.first);
            if(isCollisionFree(m_mainCommands, original, act, m_actionToCommand.count(act) ? m_actionToCommand[act] : QString{})) {
                // enable english commands, if no conflict is found
                m_mainCommands[original]=act;
                m_actionToCommand.emplace(act, original);
            }
        }
        //add short commands
        for(auto const& p1: c0.shortCmdList){
            const QString original = resolveCommandText(p1.first);
            const QString translated = resolveCommandText(p1.second);
            if(isCollisionFree(m_cmdTranslation, original, translated))
                m_cmdTranslation[original]=translated;
            if(isCollisionFree(m_shortCommands, translated, act, m_actionToCommand.count(act) ? m_actionToCommand[act] : QString{})) {
                m_shortCommands[translated]=act;
                if (m_actionToCommand.count(act) == 0)
                    m_actionToCommand.emplace(act, translated);
            }
        }
        for(auto const& p1: c0.shortCmdList){
            const QString original = resolveCommandText(p1.first);
            if(isCollisionFree(m_shortCommands, original, act, m_actionToCommand.count(act) ? m_actionToCommand[act] : QString{})) {
                // enable english short commands, if no conflict is found
                m_shortCommands[original]=act;
                if (m_actionToCommand.count(act) == 0)
                    m_actionToCommand.emplace(act, original);
            }
        }
    }

    // translations
    std::vector<std::pair<LC_CommandText, LC_CommandText>> transList={
        {"angle",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle"), true)},
        {"dpi",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "dpi"), true)},
        {"close",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "close"), true)},
        {"chord length",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "chord length"), true)},
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


        // star
        {"sym",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "sym"), true)},
        {"nosym",LC_CommandText(QT_TRANSLATE_NOOP("QObject", "nosym"), true)},
        // commands

        /** following are reversed translation,i.e.,from translated to english **/
        //not used as command keywords
        // used in function,checkCommand()
        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "angle"), true),"angle"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ang", "angle"),"angle"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "an", "angle"),"angle"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "center"), true),"center"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cen", "center"),"center"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "ce", "center"),"center"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "chord length"), true),"chord length"},
        //    {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "length", "chord length"),"chord length"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "cl", "chord length"),"chord length"},

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

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "undo"), true),"undo"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "u", "undo"),"undo"},

        {LC_CommandText(QT_TRANSLATE_NOOP("QObject", "redo"), true),"redo"},
        {LC_CommandText QT_TRANSLATE_NOOP3("QObject", "r", "redo"),"redo"},

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
    for(auto const& p: transList){
        m_cmdTranslation[resolveCommandText(p.first)] = resolveCommandText(p.second);
    }
}

/**
 * Read existing alias file or create one new.
 * In OS_WIN32 "c:\documents&settings\<user>\local configuration\application data\LibreCAD\librecad.alias"
 * In OS_MAC "/Users/<user>/Library/Application Support/LibreCAD/librecad.alias"
 * In OS_LINUX "/home/<user>/.local/share/data/LibreCAD/librecad.alias"
 */
void RS_Commands::updateAlias(){
    QString aliasName = RS_SYSTEM->getAppDataDir();
    if (aliasName.isEmpty())
        return;
    aliasName += "/librecad.alias";
    //    qDebug()<<"alias file:\t"<<aliasName;
    QFile f(aliasName);
    QString line;
    std::map<QString, QString> aliasList;
    if (f.exists()) {

        //alias file exists, read user defined alias
        if (f.open(QIODevice::ReadOnly)) {
            //        qDebug()<<"alias File: "<<aliasName;
            QTextStream ts(&f);
            //check if is empty file or not alias file
            //            if(!line.isNull() || line == "#LibreCAD alias v1") {
            //                while (!ts.atEnd())
            while(!ts.atEnd())
            {
                line=ts.readLine().trimmed();
                if (line.isEmpty() || line.at(0)=='#' ) continue;
                // Read alias
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                QStringList txtList = line.split(QRegExp(R"(\s)"),Qt::SkipEmptyParts);
#else
                QStringList txtList = line.split(QRegExp(R"(\s)"),QString::SkipEmptyParts);
#endif
                if (txtList.size()> 1) {
                    //                    qDebug()<<"reading: "<<txtList.at(0)<<"\t"<< txtList.at(1);
                    aliasList[txtList.at(0)]=txtList.at(1);
                }
            }
        }
    } else {
        //alias file does no exist, create one with translated shortCommands
        writeAliasFile(f, m_shortCommands, m_mainCommands);
    }
    //update alias file with non present commands
    //RLZ: to be written

    //add alias to shortCommands
    for(auto const& p: aliasList){
        // Issue #2656: allow a user alias to override a built-in short command
        // (e.g. remap "c" from circle to modmove). Only refuse to redefine a
        // full/main command, which would break command lookup / cause cyclic
        // resolution. Ported from the master alias cleanup (Issue #2019).
        if(m_mainCommands.count(p.first)) continue;
        if(m_mainCommands.count(p.second)){
            RS_DEBUG->print("adding command alias: %s\t%s\n", p.first.toStdString().c_str(), p.second.toStdString().c_str());
            m_shortCommands[p.first]=m_mainCommands[p.second];
        }else if(m_cmdTranslation.count(p.second)){
            RS_DEBUG->print("adding command alias: %s\t%s\n", p.first.toStdString().c_str(), m_cmdTranslation[p.second].toStdString().c_str());
            m_shortCommands[p.first]=m_mainCommands[m_cmdTranslation[p.second]];
        }
    }
    f.close();
}


/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
QStringList RS_Commands::complete(const QString& cmd) const {
    QStringList ret;
    for(auto const& p: m_mainCommands){
        if(p.first.startsWith(cmd, Qt::CaseInsensitive)){
            ret << p.first;
        }
    }
    ret.sort();

    return ret;
}



/**
 * @return Command for triggering the given action in the currently chosen
 * language for commands.
 *
 * @param action ID of the action who's command will be returned.
 * @param num Number of the command. There might be multiple commands
 *            for the same action (e.g. 'line' and 'l')
 *
 * @return The translated command.
 */
RS2::ActionType RS_Commands::cmdToAction(const QString& cmd, bool verbose) const {
    QString full = cmd.toLower();
    RS2::ActionType ret = RS2::ActionNone;

    // find command:
    for(const auto& table: {m_mainCommands, m_shortCommands})
    {
        if (table.count(cmd)) {
            ret = table.at(cmd);
            break;
        }
    }
    if (ret==RS2::ActionNone)
        return ret;

    if (!verbose) return ret;
    // find full command to confirm to user:
    for(auto const& p: m_mainCommands){
        if(p.second==ret){
            RS_DEBUG->print("RS_Commands::cmdToAction: commandMessage");
            RS_DIALOGFACTORY->commandMessage(QObject::tr("Command: %1 (%2)").arg(full).arg(p.first));
            //                                        RS_DialogFactory::instance()->commandMessage( QObject::tr("Command: %1").arg(full));
            RS_DEBUG->print("RS_Commands::cmdToAction: "
                            "commandMessage: ok");
            return ret;
        }
    }
    RS_DEBUG->print(QObject::tr("RS_Commands:: command not found: %1").arg(full).toStdString().c_str());
    return ret;
}

/**
 * Gets the action for the given keycode. A keycode is a sequence
 * of key-strokes that is entered like hotkeys.
 */
RS2::ActionType RS_Commands::keycodeToAction(const QString& code) const {
    if(code.size() < 1)
        return RS2::ActionNone;

    QString c;

    if(!(code.startsWith(g_FnPrefix) ||
         code.startsWith(g_AltPrefix) ||
         code.startsWith(g_MetaPrefix))) {
    	if(code.size() < 1 || code.contains(QRegExp("^[a-z].*",Qt::CaseInsensitive)) == false )
            return RS2::ActionNone;
        c = code.toLower();
    } else {
        c = code;
    }


    //    std::cout<<"regex: "<<qPrintable(c)<<" matches: "<< c.contains(QRegularExpression("^[a-z].*",Qt::CaseInsensitive))<<std::endl;
    //    std::cout<<"RS2::ActionType RS_Commands::keycodeToAction("<<qPrintable(c)<<")"<<std::endl;

    auto it = m_shortCommands.find(c);

    if( it == m_shortCommands.end() ) {

        //not found, searching for main commands
        it = m_mainCommands.find(c);
        if( it == m_mainCommands.end() ){
            //			RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(c));
            return RS2::ActionNone;
        }
    }
    //found
    RS_DIALOGFACTORY->commandMessage(QObject::tr("Accepted keycode: %1").arg(c));
    //fixme, need to handle multiple hits
    return it->second;
}


/**
 * @return translated command for the given English command.
 */
QString RS_Commands::command(const QString& cmd) {
    auto it= instance()->m_cmdTranslation.find(cmd);
    if(it != instance()->m_cmdTranslation.end()){
        return instance()->m_cmdTranslation[cmd];
    }
    RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(cmd));
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Commands::command: command '%s' unknown", cmd.toLatin1().data());
    return "";
}

QString RS_Commands::localizedCommand(const char* source, const char* disambiguation,
                                      const char* context) {
    return RS_SYSTEM->translateCommand(source, disambiguation, context);
}

bool RS_Commands::matchesLocalizedCommand(const QString& command, const char* source,
                                          const char* disambiguation, const char* context) {
    return command.compare(QLatin1String(source), Qt::CaseInsensitive) == 0
           || command.compare(localizedCommand(source, disambiguation, context), Qt::CaseInsensitive) == 0;
}



/**
 * Checks if the given string 'str' matches the given command 'cmd' for action
 * 'action'.
 *
 * @param cmd The command we want to check for (e.g. 'angle').
 * @param action The action which wants to know.
 * @param str The string typically entered by the user.
 */
bool RS_Commands::checkCommand(const QString& cmd, const QString& str,
                               RS2::ActionType /*action*/) {

    QString const& strl = str.toLower();
    QString const& cmdLower = cmd.toLower();
    auto it = instance()->m_cmdTranslation.find(cmdLower);
    if(it != instance()->m_cmdTranslation.end()){
        RS2::ActionType type0=instance()->cmdToAction(it->second, false);
        if( type0  != RS2::ActionNone ) {
            return  type0 ==instance()->cmdToAction(strl);
        }
    }

    it =  instance()->m_cmdTranslation.find(strl);
    if(it !=  instance()->m_cmdTranslation.end()) return it->second == cmdLower;
    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
QString RS_Commands::msgAvailableCommands() {
    return QObject::tr("Available commands:");
}

// EOF
