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
#include <QRegularExpression>
#include <QTextStream>

#include "rs_commands.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "rs_system.h"

namespace {
struct LC_CommandItem {
    std::vector<std::pair<QString, QString>> const fullCmdList;
    std::vector<std::pair<QString, QString>> const shortCmdList;
    RS2::ActionType actionType;
};

// helper function to check and report command collision
template<typename T1, typename T2>
bool isCollisionFree(std::map<T1, T2> const& lookUp, T1 const& key, T2 const& value)
{
    if(!lookUp.count(key)) return true;

    //report command string collision
    QString msg=__FILE__+QObject::tr(": duplicated command: %1 is already taken by %2")
            .arg(key).arg(value);

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
        if (actionToMain.count(pairShort.second) == 0)
            ts<<pairShort.first<<'\t'<<actionToMain.at(pairShort.second)<<'\n';

    ts.flush();
}
}

const char* RS_Commands::FnPrefix = "Fn";
const char* RS_Commands::AltPrefix = "Alt-";
const char* RS_Commands::MetaPrefix = "Meta-";


RS_Commands* RS_Commands::instance() {
    static RS_Commands* uniqueInstance = new RS_Commands();
    return uniqueInstance;
}


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

RS_Commands::RS_Commands() {
    const std::vector<LC_CommandItem> commandList{

        //      draw entity command template
        /*        {
//          mainCommand / long form - full command, appears in alias file (librecad.alias)
            {{"mainCommand", QObject::tr("mainCommand", "translationText")},
             {"alt-mainCommand", QObject::tr("alt-mainCommand", "translationText")}},
//          Short form(s) - keycode, legacy and single character commands
            {{"keycode", QObject::tr("altcmd, "translationText")},
             {"(alt-)shortCommand", QObject::tr("(alt-)shortCommand", "translationText")}}
            RS2::ActionCommand
        },
*/

        /* LINE COMMANDS */
        // draw line
        {
            {{"line2p", QObject::tr("line2p", "draw line")}},
            {{"li", QObject::tr("li", "draw line")},
                {"line", QObject::tr("line", "draw line")},
                {"l", QObject::tr("l", "draw line")}},
            RS2::ActionDrawLine
        },
        // draw Snake line
        {
            {{"sline", QObject::tr("sline", "draw snake line")}},
            {{"sli", QObject::tr("sli", "draw snake line")},
              {"sl", QObject::tr("sl", "draw snake line")}},
            RS2::ActionDrawSnakeLine
        },
        // draw Snake-X line
        {
            {{"slinex", QObject::tr("slinex", "draw snake line (X)")}},
            {{"slix", QObject::tr("slix", "draw snake line (X)")},
                {"slx", QObject::tr("rlx", "draw snake line (X)")}},
            RS2::ActionDrawSnakeLineX
        },
        // draw Snake-Y line
        {
            {{"sliney", QObject::tr("sliney", "draw snake line (Y)")}},
            {{"sliy", QObject::tr("sliy", "draw snake line (Y)")},
                {"sly", QObject::tr("rly", "draw snake line (Y)")}},
            RS2::ActionDrawSnakeLineY
        },
        // draw line at angle - v2.2.0r2
        {
            {{"lineang", QObject::tr("lineang", "angled line")}},
            {{"la", QObject::tr("la", "angled line")}},
            RS2::ActionDrawLineAngle
        },
        // draw horizontal line
        {
            {{"linehor", QObject::tr("linehor", "horizontal line")}},   // - v2.2.0r2
            {{"lh", QObject::tr("lh", "horizontal line")}},
            RS2::ActionDrawLineHorizontal
        },
        // draw vertical line
        {
            {{"linever", QObject::tr("linever", "vertical line")}},   // - v2.2.0r2
            {{"lv", QObject::tr("lv", "vertical line")}},
            RS2::ActionDrawLineVertical
        },
        // draw rectangle - v2.2.0r2
        {
            {{"linerec", QObject::tr("linerec", "draw rectangle")}},
            {{"re", QObject::tr("re", "draw rectangle")},
                {"rect", QObject::tr("rect", "draw rectangle")}},
            RS2::ActionDrawLineRectangle
        },
        // draw  rectangle 1 Point
        {
            {{"rect1", QObject::tr("rect1", "draw rectangle (1 Point)")}},
            {{"re1", QObject::tr("re1", "draw rectangle (1 Point)")}},
            RS2::ActionDrawRectangle1Point
        },
        // draw  rectangle 2 Points
        {
            {{"rect2", QObject::tr("rect2", "draw rectangle (2 Points)")}},
            {{"re2", QObject::tr("re2", "draw rectangle (2 Points)")}},
            RS2::ActionDrawRectangle2Points
        },
        // draw rectangle 3 Points
        {
            {{"rect3", QObject::tr("rect3", "draw rectangle (3 Points)")}},
            {{"re3", QObject::tr("re3", "draw rectangle (3 Points)")}},
            RS2::ActionDrawRectangle3Points
        },
        // slice/divide line
        {
            {{"slicel", QObject::tr("slicel", "slice/divide line")}},
            {{"sll", QObject::tr("sll", "slice/divide line")}},
            RS2::ActionDrawSliceDivideLine
        },
        // slice/divide circle/arc
        {
            {{"slicec", QObject::tr("slicec", "slice/divide circle/arc")}},
            {{"slc", QObject::tr("slc", "slice/divide circle/arc")}},
            RS2::ActionDrawSliceDivideCircle
        },
        // draw star
        {
            {{"star", QObject::tr("star", "draw star")}},
            {{"st", QObject::tr("st", "draw star")}},
            RS2::ActionDrawStar
        },
        // draw cross
        {
            {{"cross", QObject::tr("cross", "draw cross for circle")}},
            {{"cx", QObject::tr("cx", "draw cross for circle")}},
            RS2::ActionDrawCross
        },
        // draw line of points
        {
            {{"linepoints", QObject::tr("linepoints", "draw line of points")}},
            {{"lpoints", QObject::tr("lpoints", "draw line of points")}},
            RS2::ActionDrawLinePoints
        },
        // draw circle by arc
        {
            {{"circlebyarc", QObject::tr("criclebyarc", "draw circle by arc")}},
            {{"cba", QObject::tr("cba", "draw circle by arc")}},
            RS2::ActionDrawCircleByArc
        },
        // draw circle by arc
        {
            {{"duplicate", QObject::tr("duplicate", "duplicate entity")}},
            {{"dup", QObject::tr("dup", "duplicate entity")}},
            RS2::ActionModifyDuplicate
        },
        // line join
        {
            {{"linejoin", QObject::tr("linejoin", "lines join")}},
            {{"lj", QObject::tr("lj", "lines join")}},
            RS2::ActionModifyLineJoin
        },
        // break/divide
        {
            {{"breakdivide", QObject::tr("breakdivide", "break or divide entity")}},
            {{"bd", QObject::tr("bd", "break or divide entity")}},
            RS2::ActionModifyBreakDivide
        },
        // Line Gap
        {
            {{"gapline", QObject::tr("gapline", "line gap")}},
            {{"gl", QObject::tr("gl", "line gap")}},
            RS2::ActionModifyBreakDivide
        },
        // draw parallel line
        {
            {{"linepar", QObject::tr("linepar", "create parallel")},
                {"lineoff", QObject::tr("lineoff", "create parallel")}},
            {{"pa", QObject::tr("pa", "create parallel")},
                {"ll", QObject::tr("ll", "create parallel")}},
            RS2::ActionDrawLineParallel
        },
        // draw parallel line through point
        {
            {{"lineparthro", QObject::tr("lineparthro", "parallel through point")}},   // - v2.2.0r2
            {{"lp", QObject::tr("lp", "parallel through point")},
                {"ptp", QObject::tr("ptp", "parallel through point")}},
            RS2::ActionDrawLineParallelThrough
        },
        // draw angle bisector
        {
            {{"linebisect", QObject::tr("linebisect", "angle bisector")}},
            {{"bi", QObject::tr("bi", "angle bisector")},
                {"bisect", QObject::tr("bisect", "angle bisector")}},
            RS2::ActionDrawLineBisector
        },
        // draw line tangent to circle from point
        {
            {{"linetancp", QObject::tr("linetancp", "tangent point and circle")}},
            {{"lt", QObject::tr("lt", "tangent point and circle")},   // - v2.2.0r2
                {"tanpc", QObject::tr("tanpc", "tangent point and circle")}},
            RS2::ActionDrawLineTangent1
        },
        // draw line tangent to two existing circles - v2.2.0r2
        {
            {{"linetan2c", QObject::tr("linetan2c", "tangent two circles")}},
            {{"lc", QObject::tr("lc", "tangent two circles")}},
            RS2::ActionDrawLineTangent2
        },
        // draw line tangent to an existing circle perpendicular to an existing line - v2.2.0r2
        {
            {{"linetancper", QObject::tr("linetancper", "tangent line and circle")}},
            {{"or", QObject::tr("or", "tangent line and circle")}},
            RS2::ActionDrawLineOrthTan
        },
        // draw perpendicular line
        {
            {{"lineperp", QObject::tr("lineperp", "perpendicular line")}},
            {{"lo", QObject::tr("lo", "perpendicular line")},
                {"ortho", QObject::tr("ortho", "perpendicular line")}},
            RS2::ActionDrawLineOrthogonal
        },
        // draw line with relative angle - v2.2.0r2
        {
            {{"linerelang", QObject::tr("linerelang", "relative line")}},
            {{"lr", QObject::tr("lr", "relative line")}},
            RS2::ActionDrawLineRelAngle
        },
        // draw polygon by centre and point - v2.2.0r2
        {
            {{"polygoncencor", QObject::tr("polygoncencor", "polygon centre point")}},
            {{"pp", QObject::tr("pp", "polygon centre point")},   // - v2.2.0r2
                {"polycp", QObject::tr("polycp", "polygon centre point")},
                {"pcp", QObject::tr("pcp", "polygon centre point")}},
            RS2::ActionDrawLinePolygonCenCor
        },
        // draw polygon by centre and vertex - v2.2.0r2
        {
            {{"polygoncentan", QObject::tr("polygoncentan", "polygon centre vertex")}},
            {{"pv", QObject::tr("pv", "polygon centre vertex")},   // - v2.2.0r2
                {"polyct", QObject::tr("polyct", "polygon centre vertex")}},
            RS2::ActionDrawLinePolygonCenTan
        },
        // draw polygon by 2 vertices
        {
            {{"polygon2v", QObject::tr("polygon2v", "polygon by 2 vertices")}},
            {{"p2", QObject::tr("p2", "polygon by 2 vertices")},   // - v2.2.0r2
                {"poly2", QObject::tr("poly2", "polygon by 2 vertices")}},
            RS2::ActionDrawLinePolygonCorCor
        },

        /* CIRCLE COMMANDS */
        // draw circle
        {
            {{"circle", QObject::tr("circle", "draw circle")}},
            {{"ci", QObject::tr("ci", "draw circle")},
                {"c", QObject::tr("c", "draw circle")}},   // - v2.2.0r2
            RS2::ActionDrawCircle
        },
        // draw 2 point circle
        {
            {{"circle2p", QObject::tr("circle2p", "circle 2 points")}},
            {{"c2", QObject::tr("c2", "circle 2 points")},
                {"c2p", QObject::tr("c2p", "circle 2 points")}},
            RS2::ActionDrawCircle2P
        },
        // draw circle 2 points and radius - v2.2.0r2
        {
            {{"circle2pr", QObject::tr("circle2pr", "circle 2 points radius")}},
            {{"cc", QObject::tr("cc", "circle 2 points radius")}},
            RS2::ActionDrawCircle2PR
        },
        // draw 3 point circle
        {
            {{"circle3p", QObject::tr("circle3p", "circle 3 points")}},
            {{"c3", QObject::tr("c3", "circle 3 points")},
                {"c3p", QObject::tr("c3p", "circle 3 points")}},
            RS2::ActionDrawCircle3P
        },
        // draw circle with centre point and radius - v2.2.0r2
        {
            {{"circlecr", QObject::tr("circlecr", "circle point radius")}},
            {{"cr", QObject::tr("cr", "circle point radius")},
                {"ccr", QObject::tr("ccr", "circle point radius")}},
            RS2::ActionDrawCircleCR
        },

        // draw circle tangential to 2 circles and 1 point - v2.2.0r2
        {
            {{"circletan2cp", QObject::tr("circletan2cp", "circle 2 tangent point")}},
            {{"tr", QObject::tr("tr", "circle 2 tangent point")}},
            RS2::ActionDrawCircleTan2_1P
        },
        // draw circle Tangential to 2 Points - v2.2.0r2
        {
            {{"circletan2p", QObject::tr("circletan2p", "circle tangent 2 points")}},
            {{"td", QObject::tr("td", "circle tangent 2 points")}},
            RS2::ActionDrawCircleTan1_2P
        },
        //draw circle tangential to 2 circles with specified radius - v2.2.0r2
        {
            {{"circletan2cr", QObject::tr("circletan2cr", "circle 2 tangent radius")}},
            {{"tc", QObject::tr("tc", "circle 2 tangent radius")}},
            RS2::ActionDrawCircleTan2
        },

        // draw circle tangent to 3 objects
        {
            {{"circletan3", QObject::tr("circletan3", "circle tangent to 3")}},
            {{"t3", QObject::tr("t3", "circle tangent to 3")},   // - v2.2.0r2
                {"ct3", QObject::tr("ct3", "circle tangent to 3")},
                {"tan3", QObject::tr("tan3", "circle tangent to 3")}},
            RS2::ActionDrawCircleTan3
        },

        /* CURVE (ARC) COMMANDS */
        // draw arc by centre point and radius - v2.2.0r2 (Change to previous version)
        {
            {{"arc", QObject::tr("arc", "arc point radius")}},
            {{"ar", QObject::tr("ar", "arc point radius")},
                {"a", QObject::tr("a", "arc point radius")}},
            RS2::ActionDrawArc
        },
        // draw 3 points arc - v2.2.0r2 (Change to previous version)
        {
            {{"arc3p", QObject::tr("arc3p", "draw 3pt arc")}},
            {{"a3", QObject::tr("a3", "draw 3pt arc")}},
            RS2::ActionDrawArc3P
        },
        // draw arc tangential - v2.2.0r2
        {
            {{"arctan", QObject::tr("arctan", "arc tangent")}},
            {{"at", QObject::tr("at", "arc tangent")}},
            RS2::ActionDrawArcTangential
        },
        // draw spline with degrees freedom
        {
            {{"spline", QObject::tr("spline", "draw spline")}},
            {{"sf", QObject::tr("sf", "draw spline")},   // - v2.2.0r2
                {"spl", QObject::tr("spl", "draw spline")}},
            RS2::ActionDrawSpline
        },
        //draw spline through points
        {
            {{"spline2", QObject::tr("spline2", "spline through points")}},
            {{"sp", QObject::tr("sp", "spline through points")},   // - v2.2.0r2
                {"stp", QObject::tr("stp", "spline through points")}},
            RS2::ActionDrawSplinePoints
        },
        // draw ellipse arc by axis - v2.2.0r2
        {
            {{"arcellc2ax", QObject::tr("arcellc2ax", "arc ellipse")}},
            {{"ae", QObject::tr("ae", "arc ellipse")}},
            RS2::ActionDrawEllipseArcAxis
        },
        // draw parabola by 4 points - v2.2.1
        {
            {{"parabola4p", QObject::tr("parabola4p", "Parabola 4 points")}},
            {{"pl4", QObject::tr("pl4", "Parabola 4 points")}},
            RS2::ActionDrawParabola4Points
        },
        // draw parabola by focus directrix - v2.2.1
        {
            {{"parabolafd", QObject::tr("parabolafd", "Parabola focus directrix")}},
            {{"plfd", QObject::tr("plfd", "Parabola focus directrix")}},
            RS2::ActionDrawParabolaFD
        },
        //draw freehand line
        {
            {{"free", QObject::tr("free", "draw freehand line")}},
            {{"fh", QObject::tr("fh", "draw freehand line")},   // - v2.2.0r2
                {"fhl", QObject::tr("fhl", "draw freehand line")}},
            RS2::ActionDrawLineFree
        },

        /* ELLIPSE COMMANDS */
        // draw ellipse by axis - v2.2.0r2
        {
            {{"ellipsec2p", QObject::tr("ellipsec2p", "ellipse axis")}},
            {{"ea", QObject::tr("ea", "ellipse axis")}},
            RS2::ActionDrawEllipseAxis
        },
        // draw ellipse by foci point - v2.2.0r2
        {
            {{"ellipse3p", QObject::tr("ellipse3p", "ellipse foci")}},
            {{"ef", QObject::tr("ef", "ellipse foci")}},
            RS2::ActionDrawEllipseFociPoint
        },
        // draw 4 points ellipse - v2.2.0r2
        {
            {{"ellipse4p", QObject::tr("ellipse4p", "ellipse 4 point")}},
            {{"e4", QObject::tr("e4", "ellipse 4 point")}},
            RS2::ActionDrawEllipse4Points
        },
        // draw ellipse by center and 3 points - v2.2.0r2
        {
            {{"ellipsec3p", QObject::tr("ellipsec3p", "ellipse center 3 point")}},
            {{"e3", QObject::tr("e3", "ellipse center 3 point")}},
            RS2::ActionDrawEllipseCenter3Points
        },
        // draw inscribed ellipse
        {
            {{"ellipseinscribed", QObject::tr("ellipseinscribed", "inscribed ellipse")}},
            {{"ei", QObject::tr("ei", "inscribed ellipse")},
                {"ie", QObject::tr("ie", "inscribed ellipse")}},
            RS2::ActionDrawEllipseInscribe
        },

        /* POLYLINE COMMANDS */
        // draw polyline
        {
            {{"polyline", QObject::tr("polyline", "draw polyline")}},
            {{"pl", QObject::tr("pl", "draw polyline")}},
            RS2::ActionDrawPolyline
        },
        // draw angle line from line
        {
            {{"angleline", QObject::tr("angleline", "draw angle from line")}},
            {{"aline", QObject::tr("angleline", "draw angle from line")}},
            RS2::ActionDrawLineAngleRel
        },
        // draw orthogonal line from line
        {
            {{"ortline", QObject::tr("rortoline", "draw orthogonal")}},
            {{"oline", QObject::tr("rort", "draw orthogonal")}},
            RS2::ActionDrawLineOrthogonalRel
        },
        // draw line from point to line
        {
            {{"point2line", QObject::tr("point2line", "draw line from point to line")}},
            {{"p2l", QObject::tr("p2l", "draw line from point to line")}},
            RS2::ActionDrawLineFromPointToLine
        },

        // polyline add node - v2.2.0r2
        {
            {{"plineadd", QObject::tr("plineadd", "pl add node")}},
            {{"pi", QObject::tr("pi", "pl add node")}},   // - v2.2.0r2
            RS2::ActionPolylineAdd
        },
        // polyline append node - v2.2.0r2
        {
            {{"plineapp", QObject::tr("plineapp", "pl append node")}},
            {{"pn", QObject::tr("pn", "pl append node")}},
            RS2::ActionPolylineAppend
        },
        // polyline delete node - v2.2.0r2
        {
            {{"plinedel", QObject::tr("plinedel", "pl delete node")}},
            {{"pd", QObject::tr("pd", "pl delete node")}},
            RS2::ActionPolylineDel
        },
        // polyline delete between two nodes - v2.2.0r2
        {
            {{"plinedeltwn", QObject::tr("plinedeltwn", "pl del between nodes")}},
            {{"pr", QObject::tr("pr", "pl del between nodes")}},
            RS2::ActionPolylineDelBetween
        },
        // polyline trim segments - v2.2.0r2
        {
            {{"plinetrm", QObject::tr("plinetrm", "pl trim segments")}},
            {{"pt", QObject::tr("pt", "pl trim segments")}},
            RS2::ActionPolylineTrim
        },
        // equidistant polyline - v2.2.0r2
        {
            {{"plinepar", QObject::tr("plinepar", "pl equidistant")}},
            {{"pe", QObject::tr("pe", "pl equidistant")}},
            RS2::ActionPolylineEquidistant
        },
        // polyline from existing segments - v2.2.0r2
        {
            {{"plinejoin", QObject::tr("plinejoin", "pl join")}},
            {{"pj", QObject::tr("pj", "pl join")}},
            RS2::ActionPolylineSegment
        },

        /* SELECT COMMANDS */
        // Select all entities
        {
            {{"selectall", QObject::tr("selectall", "Select all entities")}},
            {{"sa", QObject::tr("sa", "Select all entities")}},
            RS2::ActionSelectAll
        },
        // DeSelect all entities
        {
            {{"deselectall", QObject::tr("deselectall", "deselect all entities")}},
            {{"sx", QObject::tr("sx", "deselect all entities")},
                {"tn", QObject::tr("tn", "deselect all entities")}},   // - v2.2.0r2
            RS2::ActionDeselectAll
        },
        // Invert selection - v2.2.0r2
        {
            {{"invertselect", QObject::tr("invertselect", "invert select")}},
            {{"is", QObject::tr("is", "invert select")}},
            RS2::ActionSelectInvert
        },
        /* Remaining select tools require the mouse - no point in adding commands. */

        /* DIMENSION COMMANDS */
        // dimension aligned
        {
            {{"dimaligned", QObject::tr("dimaligned", "dimension - aligned")}},
            {{"ds", QObject::tr("ds", "dimension - aligned")}},   // - v2.2.0r2 (Change to previous version)
            RS2::ActionDimAligned
        },
        // dimension linear
        {
            {{"dimlinear", QObject::tr("dimlinear", "dimension - linear")}},
            {{"dl", QObject::tr("dl", "dimension - linear")}},
            RS2::ActionDimLinear
        },
        // dimension horizontal
        {
            {{"dimhorizontal", QObject::tr("dimhorizontal", "dimension - horizontal")}},
            {{"dh", QObject::tr("dh", "dimension - horizontal")}},
            RS2::ActionDimLinearHor
        },
        // dimension vertical
        {
            {{"dimvertical", QObject::tr("dimvertical", "dimension - vertical")}},
            {{"dv", QObject::tr("dv", "dimension - vertical")}},
            RS2::ActionDimLinearVer
        },
        // dimension radius
        {
            {{"dimradius", QObject::tr("dimradius", "dimension - radial")},
                {"dimradial", QObject::tr("dimradial", "dimension - radial")}},
            {{"dr", QObject::tr("dr", "dimension - radial")}},
            RS2::ActionDimRadial
        },
        // dimension diameter
        {
            {{"dimdiameter", QObject::tr("dimdiameter", "dimension - diametric")}},
            {{"dd", QObject::tr("dd", "dimension - diametric")},
                {"dimdiametric", QObject::tr("dimdiametric", "dimension - diametric")}},
            RS2::ActionDimDiametric
        },
        // dimension angular
        {
            {{"dimangular", QObject::tr("dimangular", "dimension - angular")}},
            {{"da", QObject::tr("da", "dimension - angular")},
                {"dan", QObject::tr("dan", "dimension - angular")}},
            RS2::ActionDimAngular
        },
        // dimension leader
        {
            {{"dimleader", QObject::tr("dimleader", "dimension - leader")}},
            {{"ld", QObject::tr("ld", "dimension - leader")}},
            RS2::ActionDimLeader
        },
        // dimension regenerate
        {
            {{"dimregen", QObject::tr("dimregen", "dimension - regenerate")}},
            {{"dg", QObject::tr("dg", "dimension - regenerate")}},
            RS2::ActionToolRegenerateDimensions
        },

        /* MODIFY COMMANDS */
        // move
        {
            {{"modmove", QObject::tr("modmove", "modify - move (copy)")}},
            {{"mv", QObject::tr("mv", "modify - move (copy)")}},
            RS2::ActionModifyMove
        },
        // rotate
        {
            {{"modrotate", QObject::tr("modrotate", "modify - rotate")}},
            {{"ro", QObject::tr("ro", "modify - rotate")}},
            RS2::ActionModifyRotate
        },
        // scale
        {
            {{"modscale", QObject::tr("modscale", "modify - scale")}},
            {{"sz", QObject::tr("sz", "modify - scale")}},
            RS2::ActionModifyScale
        },
        // mirror  (Removed extra space from translation sting.)
        {
            {{"modmirror", QObject::tr("modmirror", "modify -  mirror")}},
            {{"mi", QObject::tr("mi", "modify -  mirror")}},
            RS2::ActionModifyMirror
        },
        // move and rotate - v2.2.0r2
        {
            {{"modmovrot", QObject::tr("modmovrot", "modify - move rotate")}},
            {{"mr", QObject::tr("mr", "modify - move rotate")}},
            RS2::ActionModifyMoveRotate
        },
        // rotate two - v2.2.0r2
        {
            {{"mod2rot", QObject::tr("mod2rot", "modify - rotate2")}},
            {{"r2", QObject::tr("r2", "modify - rotate2")}},
            RS2::ActionModifyRotate2
        },
        // revert (Removed extra space from translation sting.)
        {
            {{"modrevert", QObject::tr("modrevert", "modify -  revert direction")}},
            {{"md", QObject::tr("md", "modify -  revert direction")},
                {"rev", QObject::tr("rev", "modify -  revert direction")}},
            RS2::ActionModifyRevertDirection
        },
        // trim
        {
            {{"modtrim", QObject::tr("modtrim", "modify - trim (extend)")}},
            {{"tm", QObject::tr("tm", "modify - trim (extend)")}},
            RS2::ActionModifyTrim
        },
        // trim2
        {
            {{"modtrim2", QObject::tr("modtrim2", "modify - multi trim (extend)")}},
            {{"t2", QObject::tr("t2", "modify - multi trim (extend)")},
                {"tm2", QObject::tr("tm2", "modify - multi trim (extend)")}},
            RS2::ActionModifyTrim2
        },
        // lengthen
        {
            {{"modlengthen", QObject::tr("modlengthen", "modify - lengthen")}},
            {{"le", QObject::tr("le", "modify - lengthen")}},
            RS2::ActionModifyTrimAmount
        },
        // offset
        {
            {{"modoffset", QObject::tr("modoffset", "modify - offset")}},
            {{"mo", QObject::tr("mo", "modify - offset")},   // - v2.2.0r2
                {"moff", QObject::tr("moff", "modify - offset")}},
            RS2::ActionModifyOffset
        },
        // bevel
        {
            {{"modbevel", QObject::tr("modbevel", "modify - bevel")}},
            {{"bev", QObject::tr("bev", "modify - bevel")},
                {"ch", QObject::tr("ch", "modify - bevel")}},
            RS2::ActionModifyBevel
        },
        // fillet
        {
            {{"modfillet", QObject::tr("modfillet", "modify - fillet")}},
            {{"fi", QObject::tr("fi", "modify - fillet")}},
            RS2::ActionModifyRound
        },
        // divide
        {
            {{"moddivide", QObject::tr("moddivide", "modify - divide (cut)")},
                {"cut", QObject::tr("cut", "modify - divide (cut)")}},
            {{"div", QObject::tr("div", "modify - divide (cut)")},
                {"di", QObject::tr("di", "modify - divide (cut)")}},
            RS2::ActionModifyCut
        },
        // stretch
        {
            {{"modstretch", QObject::tr("modstretch", "modify - stretch")}},
            {{"ss", QObject::tr("ss", "modify - stretch")}},
            RS2::ActionModifyStretch
        },
        // modify properties
        {
            {{"modproperties", QObject::tr("modproperties", "modify properties")}},
            {{"prop", QObject::tr("prop", "modify properties")},
                {"mp", QObject::tr("mp", "modify properties")}},
            RS2::ActionModifyEntity
        },
        // modify attributes
        {
            {{"modattr", QObject::tr("modattr", "modify attribute")}},
            {{"attr", QObject::tr("attr", "modify attribute")},
                {"ma", QObject::tr("ma", "modify attribute")}},
            RS2::ActionModifyAttributes
        },
        // explode text - v2.2.0r2
        {
            {{"modexpltext", QObject::tr("modexpltext", "explode text strings")}},
            {{"xt", QObject::tr("xt", "explode text strings")}},
            RS2::ActionModifyExplodeText
        },
        // explode
        {
            {{"modexplode", QObject::tr("modexplode", "explode block/polyline into entities")}},
            {{"xp", QObject::tr("xp", "explode block/polyline into entities")}},
            RS2::ActionBlocksExplode
        },
        // delete
        {
            {{"moddelete", QObject::tr("moddelete", "modify - delete (erase)")}},
            {{"er", QObject::tr("er", "modify - delete (erase)")},
                {"del", QObject::tr("del", "modify - delete (erase)")}},
            RS2::ActionModifyDelete
        },

        /* INFO COMMANDS */
        // Distance Point to Point
        {
            {{"infodistance", QObject::tr("infodistance", "distance point to point")}},
            {{"id", QObject::tr("id", "distance point to point")},   // - v2.2.0r2
                {"dist", QObject::tr("dist", "distance point to point")},
                {"dpp", QObject::tr("dpp", "distance point to point")}},
            RS2::ActionInfoDist
        },
        // Distance Entity to Point
        {
            {{"infodistep", QObject::tr("infodistep", "distance entity to point")}},
            {{"ii", QObject::tr("ii", "distance entity to point")},   // - v2.2.0r2
                {"dep", QObject::tr("dep", "distance entity to point")}},
            RS2::ActionInfoDist2
        },
        // Measure angle
        {
            {{"infoangle", QObject::tr("infoangle", "measure angle")}},
            {{"ia", QObject::tr("ia", "measure angle")},   // - v2.2.0r2
                {"ang", QObject::tr("ang", "measure angle")}},
            RS2::ActionInfoAngle
        },
        // Measure area
        {
            {{"infoarea", QObject::tr("infoarea", "measure area")}},
            {{"aa", QObject::tr("aa", "measure area")}},   // - v2.2.0r2
            RS2::ActionInfoArea
        },

        /* OTHER COMMANDS */
        // draw mtext
        {
            {{"mtext", QObject::tr("mtext", "draw mtext")}},
            {{"mt", QObject::tr("mt", "draw mtext")},   // - v2.2.0r2
                {"mtxt", QObject::tr("mtxt", "draw mtext")}},
            RS2::ActionDrawMText
        },
        // draw text
        {
            {{"text", QObject::tr("text", "draw text")}},
            {{"tx", QObject::tr("tx", "draw text")},   // - v2.2.0r2
                {"txt", QObject::tr("txt", "draw text")}},
            RS2::ActionDrawText
        },
        // draw hatch
        {
            {{"hatch", QObject::tr("hatch", "draw hatch")}},
            {{"ha", QObject::tr("ha", "draw hatch")}},
            RS2::ActionDrawHatchNoSelect
        },
        // draw point
        {
            {{"point", QObject::tr("point", "draw point")}},
            {{"po", QObject::tr("po", "draw point")}},
            RS2::ActionDrawPoint
        },

        /* SNAP COMMANDS */
        /* snap exclusive - v2.2.0r2
        {
            {{"snapexcl", QObject::tr("snapexcl", "snap - excl")}},
            {{"sx", QObject::tr("sx", "snap - excl")},
             {"ex", QObject::tr("ex", "snap - excl")}},
            RS2::ActionSnapExcl  // Not present
        }, */
        // snap free
        {
            {{"snapfree", QObject::tr("snapfree", "snap - free")}},
            {{"so", QObject::tr("so", "snap - free")},
                {"os", QObject::tr("os", "snap - free")}},
            RS2::ActionSnapFree
        },
        // snap center
        {
            {{"snapcenter", QObject::tr("snapcenter", "snap - center")}},
            {{"sc", QObject::tr("sc", "snap - center")}},
            RS2::ActionSnapCenter
        },
        //snap dist
        {
            {{"snapdist", QObject::tr("snapdist", "snap - distance to endpoints")}},
            {{"sd", QObject::tr("sd", "snap - distance to endpoints")}},
            RS2::ActionSnapDist
        },
        // snap end
        {
            {{"snapend", QObject::tr("snapend", "snap - end points")}},
            {{"se", QObject::tr("se", "snap - end points")}},
            RS2::ActionSnapEndpoint
        },
        // snap grid
        {
            {{"snapgrid", QObject::tr("snapgrid", "snap - grid")}},
            {{"sg", QObject::tr("sg", "snap - grid")}},
            RS2::ActionSnapGrid
        },
        // snap intersection
        {
            {{"snapintersection", QObject::tr("snapintersection", "snap - intersection")}},
            {{"si", QObject::tr("si", "snap - intersection")}},
            RS2::ActionSnapIntersection
        },
        // snap middle
        {
            {{"snapmiddle", QObject::tr("snapmiddle", "snap - middle points")}},
            {{"sm", QObject::tr("sm", "snap - middle points")}},
            RS2::ActionSnapMiddle
        },
        // snap on entity
        {
            {{"snaponentity", QObject::tr("snaponentity", "snap - on entity")}},
            {{"sn", QObject::tr("sn", "snap - on entity")},
                {"np", QObject::tr("np", "snap - on entity")}},
            RS2::ActionSnapOnEntity
        },

        /* Snap Middle Manual */
        {
            //list all <full command, translation> pairs
            {{"snapmiddlemanual", QObject::tr("snapmiddlemanual", "snap middle manual")}}, 
            {{"snapmanual", QObject::tr("snapmanual", "snap middle manual")}, 
             {"smm", QObject::tr("smm", "snap middle manual")}}, 

            RS2::ActionSnapMiddleManual
        },

        // set relative zero
        {
            {{"setrelativezero", QObject::tr("setrelativezero", "set relative zero position")}},
            {{"rz", QObject::tr("rz", "set relative zero position")}},
            RS2::ActionSetRelativeZero
        },
        // snap restrictions
        {
            {{"restrictnothing", QObject::tr("restrictnothing", "restrict - nothing")}},
            {{"rn", QObject::tr("rn", "restrict - nothing")}},
            RS2::ActionRestrictNothing
        },
        // snap orthogonal
        {
            {{"restrictorthogonal", QObject::tr("restrictorthogonal", "restrict - orthogonal")}},
            {{"rr", QObject::tr("rr", "restrict - orthogonal")}},
            RS2::ActionRestrictOrthogonal
        },
        // snap horizontal
        {
            {{"restricthorizontal", QObject::tr("restricthorizontal", "restrict - horizontal")}},
            {{"rh", QObject::tr("rh", "restrict - horizontal")}},
            RS2::ActionRestrictHorizontal
        },
        // snap vertical
        {
            {{"restrictvertical", QObject::tr("restrictvertical", "restrict - vertical")}},
            {{"rv", QObject::tr("rv", "restrict - vertical")}},
            RS2::ActionRestrictVertical
        },

        /* MENU COMMANDS */
        /* EDIT COMMANDS */
        // kill actions
        {
            {{"kill", QObject::tr("kill", "kill all actions")}},
            {{"ki", QObject::tr("ki", "kill all actions")},
                {"k", QObject::tr("k", "kill all actions")}},
            RS2::ActionEditKillAllActions
        },
        // undo cycle
        {
            {{"undo", QObject::tr("undo", "undo cycle")}},
            {{"un", QObject::tr("un", "undo cycle")},
                {"u", QObject::tr("u", "undo cycle")}},
            RS2::ActionEditUndo
        },
        // redo cycle
        {
            {{"redo", QObject::tr("redo", "redo cycle")}},
            {{"rd", QObject::tr("rd", "redo cycle")},
                {"r", QObject::tr("r", "redo cycle")}},
            RS2::ActionEditRedo
        },

        /* OPTIONS COMMANDS */
        // Drawing Prefs - v2.2.0r2
        {
            {{"drawpref", QObject::tr("drawpref", "drawing preferences")}},
            {{"dp", QObject::tr("dp", "drawing preferences")}},
            RS2::ActionOptionsDrawing
        },

        /* VIEW COMMANDS */
        // zoom redraw
        {
            {{"regen", QObject::tr("regen", "zoom - redraw")},
                {"redraw", QObject::tr("redraw", "zoom - redraw")}},
            {{"rg", QObject::tr("rg", "zoom - redraw")},
                {"zr", QObject::tr("zr", "zoom - redraw")}},
            RS2::ActionZoomRedraw
        },
        // zoom auto
        {
            {{"zoomauto", QObject::tr("zoomauto", "zoom - auto")}},
            {{"za", QObject::tr("za", "zoom - auto")}},
            RS2::ActionZoomAuto
        },
        // zoom previous
        {
            {{"zoomprevious", QObject::tr("zoomprevious", "zoom - previous")}},
            {{"zv", QObject::tr("zv", "zoom - previous")}},
            RS2::ActionZoomPrevious
        },
        // zoom window
        {
            {{"zoomwindow", QObject::tr("zoomwindow", "zoom - window")}},
            {{"zw", QObject::tr("zw", "zoom - window")}},
            RS2::ActionZoomWindow
        },
        // zoom pan
        {
            {{"zoompan", QObject::tr("zoompan", "zoom - pan")}},
            {{"zp", QObject::tr("zp", "zoom - pan")}},
            RS2::ActionZoomPan
        }
    };

    for(auto const& c0: commandList){
        auto const act=c0.actionType;
        //add full commands
        for(auto const& p0: c0.fullCmdList){
            if(isCollisionFree(cmdTranslation, p0.first, p0.second))
                cmdTranslation[p0.first]=p0.second;
            if(isCollisionFree(mainCommands, p0.second, act))
                mainCommands[p0.second]=act;
        }
        //add short commands
        for(auto const& p1: c0.shortCmdList){
            if(isCollisionFree(cmdTranslation, p1.first, p1.second))
                cmdTranslation[p1.first]=p1.second;
            if(isCollisionFree(shortCommands, p1.second, act))
                shortCommands[p1.second]=act;
        }
    }

    // translations
    std::vector<std::pair<QString, QString>> transList={
        {"angle",QObject::tr("angle")},
        {"dpi",QObject::tr("dpi")},
        {"close",QObject::tr("close")},
        {"chord length",QObject::tr("chord length")},
        {"columns",QObject::tr("columns")},
        {"columnspacing",QObject::tr("columnspacing")},
        {"equation",QObject::tr("equation")},
        {"factor",QObject::tr("factor")},
        {"length",QObject::tr("length")},
        {"length1",QObject::tr("length1", "bevel/fillet length1")},
        {"length2",QObject::tr("length2", "bevel/fillet length2")},
        {"number",QObject::tr("number")},
        {"radius",QObject::tr("radius")},
        {"rows",QObject::tr("rows")},
        {"rowspacing",QObject::tr("rowspacing")},
        {"through",QObject::tr("through")},
        {"trim",QObject::tr("trim")},

        // commands for relative line drawing actions
        {"x",QObject::tr("x")},
        {"y",QObject::tr("y")},
        {"p",QObject::tr("p")},
        {"anglerel",QObject::tr("anglerel")},
        {"start",QObject::tr("start")},

        // commands for line angle rel action
        {"offset",QObject::tr("offset")},
        {"linesnap",QObject::tr("linesnap")},
        {"ticksnap",QObject::tr("ticksnap")},

        // rectangle one point
        {"width",QObject::tr("width")},
        {"height",QObject::tr("height")},
        {"pos",QObject::tr("pos")},
        {"size",QObject::tr("size")},
        {"bevels",QObject::tr("bevels")},
        {"nopoly",QObject::tr("nopoly")},
        {"usepoly",QObject::tr("usepoly")},
        {"corners",QObject::tr("corners")},
        {"str",QObject::tr("str")},
        {"round",QObject::tr("round")},
        {"bevels",QObject::tr("bevels")},
        {"snap1",QObject::tr("snap1")},
        {"topl",QObject::tr("topl")},
        {"top",QObject::tr("top")},
        {"topr",QObject::tr("topr")},
        {"left",QObject::tr("left")},
        {"middle",QObject::tr("middle")},
        {"right",QObject::tr("right")},
        {"bottoml",QObject::tr("bottoml")},
        {"bottom",QObject::tr("bottom")},
        {"bottomr",QObject::tr("bottomr")},
        {"snapcorner",QObject::tr("snapcorner")},
        {"snapshift",QObject::tr("snapshift")},
        {"sizein",QObject::tr("sizein")},
        {"sizeout",QObject::tr("sizeout")},
        {"hor",QObject::tr("hor")},
        {"vert",QObject::tr("vert")},

        // rect 2 points
        {"snap2",QObject::tr("snap2")},
        {"corner",QObject::tr("corner")},
        {"mid-vert",QObject::tr("mid-vert")},
        {"mid-hor",QObject::tr("mid-hor")},
        // rect 3 points
        {"quad",QObject::tr("quad")},
        {"noquad",QObject::tr("noquad")},
        {"angle_inner",QObject::tr("angle_inner")},

        // line points
        {"edges",QObject::tr("edges")},
        {"edge-none",QObject::tr("edge-none")},
        {"edge-both",QObject::tr("edge-both")},
        {"edge-start",QObject::tr("edge-start")},
        {"edge-end",QObject::tr("edge-end")},
        {"end",QObject::tr("end")},
        {"both",QObject::tr("both")},
        {"none",QObject::tr("none")},
        {"fit",QObject::tr("fit")},
        {"nofit",QObject::tr("nofit")},
        {"dist_fixed",QObject::tr("dist_fixed")},
        {"dist_flex",QObject::tr("dist_flex")},
        {"distance",QObject::tr("distance")},


        // star
        {"sym",QObject::tr("sym")},
        {"nosym",QObject::tr("nosym")},
        // commands

        /** following are reversed translation,i.e.,from translated to english **/
        //not used as command keywords
        // used in function,checkCommand()
        {QObject::tr("angle"),"angle"},
        {QObject::tr("ang", "angle"),"angle"},
        {QObject::tr("an", "angle"),"angle"},

        {QObject::tr("center"),"center"},
        {QObject::tr("cen", "center"),"center"},
        {QObject::tr("ce", "center"),"center"},

        {QObject::tr("chord length"),"chord length"},
        //    {QObject::tr("length", "chord length"),"chord length"},
        {QObject::tr("cl", "chord length"),"chord length"},

        {QObject::tr("close"),"close"},
        {QObject::tr("c", "close"),"close"},

        {QObject::tr("columns"),"columns"},
        {QObject::tr("cols", "columns"),"columns"},
        {QObject::tr("co", "columns"),"columns"},

        {QObject::tr("columnspacing", "columnspacing for inserts"),"columnspacing"},
        {QObject::tr("colspacing", "columnspacing for inserts"),"columnspacing"},
        {QObject::tr("cs", "columnspacing for inserts"),"columnspacing"},

        {QObject::tr("factor"),"factor"},
        {QObject::tr("fact", "factor"),"factor"},
        {QObject::tr("f", "factor"),"factor"},

        {QObject::tr("equation"),"equation"},
        {QObject::tr("eqn", "equation"),"equation"},
        {QObject::tr("eq", "equation"),"equation"},

        {QObject::tr("help"),"help"},
        {QObject::tr("?", "help"),"help"},

        {QObject::tr("length","length"),"length"},
        {QObject::tr("len","length"),"length"},
        {QObject::tr("l","length"),"length"},

        {QObject::tr("length1","length1"),"length1"},
        {QObject::tr("len1","length1"),"length1"},
        {QObject::tr("l1","length1"),"length1"},

        {QObject::tr("length2","length2"),"length2"},
        {QObject::tr("len2","length2"),"length2"},
        {QObject::tr("l2","length2"),"length2"},

        {QObject::tr("number","number"),"number"},
        {QObject::tr("num","number"),"number"},
        {QObject::tr("n","number"),"number"},

        {QObject::tr("radius"),"radius"},
        {QObject::tr("ra","radius"),"radius"},

        {QObject::tr("reversed","reversed"),"reversed"},
        {QObject::tr("rev","reversed"),"reversed"},
        {QObject::tr("rev","reversed"),"reversed"},

        {QObject::tr("row", "row"),"row"},

        {QObject::tr("rowspacing", "rowspacing for inserts"),"rowspacing"},
        {QObject::tr("rs","rowspacing for inserts"),"rowspacing"},

        {QObject::tr("text"),"text"},
        {QObject::tr("t","text"),"text"},

        {QObject::tr("through"),"through"},
        {QObject::tr("t","through"),"through"},

        {QObject::tr("undo"),"undo"},
        {QObject::tr("u","undo"),"undo"},

        {QObject::tr("redo"),"redo"},
        {QObject::tr("r","redo"),"redo"},

        {QObject::tr("back"),"back"},
        {QObject::tr("b","back"),"back"},
        //printer preview
        {QObject::tr("bw"), "blackwhite"},
        {QObject::tr("blackwhite"), "blackwhite"},
        {QObject::tr("color"), "color"},
        {QObject::tr("paperoffset"),"paperoffset"},
        {QObject::tr("graphoffset"),"graphoffset"}

        // fixme - add reversive translation for added commands
    };
    for(auto const& p: transList){
        cmdTranslation[p.first] = p.second;
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
                QStringList txtList = line.split(QRegularExpression(R"(\s)"),Qt::SkipEmptyParts);
#else
                QStringList txtList = line.split(QRegularExpression(R"(\s)"),QString::SkipEmptyParts);
#endif
                if (txtList.size()> 1) {
                    //                    qDebug()<<"reading: "<<txtList.at(0)<<"\t"<< txtList.at(1);
                    aliasList[txtList.at(0)]=txtList.at(1);
                }
            }
        }
    } else {
        //alias file does no exist, create one with translated shortCommands
        writeAliasFile(f, shortCommands, mainCommands);
    }
    //update alias file with non present commands
    //RLZ: to be written

    //add alias to shortCommands
    for(auto const& p: aliasList){
        if(shortCommands.count(p.first)) continue;
        if(mainCommands.count(p.first)) continue;
        if(mainCommands.count(p.second)){
            RS_DEBUG->print("adding command alias: %s\t%s\n", p.first.toStdString().c_str(), p.second.toStdString().c_str());
            shortCommands[p.first]=mainCommands[p.second];
        }else if(cmdTranslation.count(p.second)){
            RS_DEBUG->print("adding command alias: %s\t%s\n", p.first.toStdString().c_str(), cmdTranslation[p.second].toStdString().c_str());
            shortCommands[p.first]=mainCommands[cmdTranslation[p.second]];
        }
    }
    f.close();
}


/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
QStringList RS_Commands::complete(const QString& cmd) const {
    QStringList ret;
    for(auto const& p: mainCommands){
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
    for(const auto& table: {mainCommands, shortCommands})
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
    for(auto const& p: mainCommands){
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

    if(!(code.startsWith(FnPrefix) ||
         code.startsWith(AltPrefix) ||
         code.startsWith(MetaPrefix))) {
        if(code.size() < 1 || code.contains(QRegularExpression("^[a-zA-Z].*")) == false )
            return RS2::ActionNone;
        c = code.toLower();
    } else {
        c = code;
    }


    //    std::cout<<"regex: "<<qPrintable(c)<<" matches: "<< c.contains(QRegularExpression("^[a-z].*",Qt::CaseInsensitive))<<std::endl;
    //    std::cout<<"RS2::ActionType RS_Commands::keycodeToAction("<<qPrintable(c)<<")"<<std::endl;

    auto it = shortCommands.find(c);

    if( it == shortCommands.end() ) {

        //not found, searching for main commands
        it = mainCommands.find(c);
        if( it == mainCommands.end() ){
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
    auto it= instance()->cmdTranslation.find(cmd);
    if(it != instance()->cmdTranslation.end()){
        return instance()->cmdTranslation[cmd];
    }
    RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(cmd));
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Commands::command: command '%s' unknown", cmd.toLatin1().data());
    return "";
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
    auto it = instance()->cmdTranslation.find(cmdLower);
    if(it != instance()->cmdTranslation.end()){
        RS2::ActionType type0=instance()->cmdToAction(it->second, false);
        if( type0  != RS2::ActionNone ) {
            return  type0 ==instance()->cmdToAction(strl);
        }
    }

    it =  instance()->cmdTranslation.find(strl);
    if(it !=  instance()->cmdTranslation.end()) return it->second == cmdLower;
    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
QString RS_Commands::msgAvailableCommands() {
    return QObject::tr("Available commands:");
}

// EOF
