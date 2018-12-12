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

#include "rs_system.h"
#include "rs_dialogfactory.h"
#include "rs_debug.h"

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
}

RS_Commands* RS_Commands::uniqueInstance = nullptr;

const char* RS_Commands::FnPrefix = "Fn";
const char* RS_Commands::AltPrefix = "Alt-";
const char* RS_Commands::MetaPrefix = "Meta-";


RS_Commands* RS_Commands::instance() {
    if (!uniqueInstance) {
        uniqueInstance = new RS_Commands();
    }
    return uniqueInstance;
}

/**
 * Constructor. Initiates main command dictionary.
 * mainCommand keeps a map from translated commands to actionType
 * shortCommand keeps a list of translated short commands
 * cmdTranslation contains both ways of mapping between translated and English
 */
RS_Commands::RS_Commands() {
    std::initializer_list<LC_CommandItem> commandList={
        //draw point
        {
            //list all <full command, translation> pairs
            {{"point", QObject::tr("point", "draw point")}},

            //list all <short command, translation> pairs
            {{"po", QObject::tr("po", "draw point")}},

            //action type
            RS2::ActionDrawPoint
        },
        //draw line
        {
            {{"line", QObject::tr("line", "draw line")}},
            {{"li", QObject::tr("li", "draw line")},
             {"l", QObject::tr("l", "draw line")}},
            RS2::ActionDrawLine
        },
        //draw polyline
        {
            {{"polyline", QObject::tr("polyline", "draw polyline")}},
            {{"pl", QObject::tr("pl", "draw polyline")}},
            RS2::ActionDrawPolyline
        },
        //draw freehand line
        {
            {{"free", QObject::tr("free", "draw freehand line")}},
            {{"fhl", QObject::tr("fhl", "draw freehand line")}},
            RS2::ActionDrawLineFree
        },
        //draw spline
        {
            {{"spline", QObject::tr("spline", "draw spline")}},
            {{"spl", QObject::tr("spl", "draw spline")}},
            RS2::ActionDrawSpline
        },
        //draw spline through points
        {
            {{"spline2", QObject::tr("spline2", "spline through points")}},
            {{"stp", QObject::tr("stp", "spline through points")}},
            RS2::ActionDrawSplinePoints
        },
        //draw parallel line
        {
            {{"offset", QObject::tr("offset", "create offset")},
            {"parallel", QObject::tr("parallel", "create offset")}},
            {{"o", QObject::tr("o", "create offset")},
            {"pa", QObject::tr("pa", "create offset")}},
            RS2::ActionDrawLineParallel
        },
        //draw parallel line through point
        {
            {{"ptp", QObject::tr("ptp", "parallel through point")}},
            {{"pp", QObject::tr("pp", "parallel through point")}},
            RS2::ActionDrawLineParallelThrough
        },
        //draw angle bisector
        {
            {{"bisect", QObject::tr("bisect", "angle bisector")}},
            {{"bi", QObject::tr("bi", "angle bisector")}},
            RS2::ActionDrawLineBisector
        },
        //draw line tangent to circle from point
        {
            {{"tangentpc", QObject::tr("tangentpc", "tangent point and circle")}},
            {{"tanpc", QObject::tr("tanpc", "tangent point and circle")}},
            RS2::ActionDrawLineTangent1
        },
        //draw perpendicular line
        {
            {{"perp", QObject::tr("perp", "perpendicular line")}},
            {{"ortho", QObject::tr("ortho", "perpendicular line")}},
            RS2::ActionDrawLineOrthogonal
        },
        //draw vertical line
        {
            {{"vertical", QObject::tr("vertical", "vertical line")}},
            {{"ver", QObject::tr("ver", "vertical line")}},
            RS2::ActionDrawLineVertical
        },
        //draw horizontal line
        {
            {{"horizontal", QObject::tr("horizontal", "horizontal line")}},
            {{"hor", QObject::tr("hor", "horizontal line")}},
            RS2::ActionDrawLineHorizontal
        },
        //draw rectangle
        {
            {{"rectangle", QObject::tr("rectangle", "draw rectangle")}},
            {{"rectang", QObject::tr("rectang", "draw rectangle")},
             {"rect", QObject::tr("rect", "draw rectangle")},
            {"rec", QObject::tr("rec", "draw rectangle")}},
            RS2::ActionDrawLineRectangle
        },
        //draw polygon by 2 vertices
        {
            {{"polygon2v", QObject::tr("polygon2v", "polygon by 2 vertices")}},
            {{"poly2", QObject::tr("poly2", "polygon by 2 vertices")}},
            RS2::ActionDrawLinePolygonCorCor
        },
        //draw arc
        {
            {{"arc", QObject::tr("arc", "draw arc")}},
			{//{"ar", QObject::tr("ar", "draw arc")},
            {"a", QObject::tr("a", "draw arc")}},
            RS2::ActionDrawArc3P
        },
        //draw circle
        {
            {{"circle", QObject::tr("circle", "draw circle")}},
            {{"ci", QObject::tr("ci", "draw circle")}},
            RS2::ActionDrawCircle
        },        
        //draw 2 point circle
        {
            {{"circle2", QObject::tr("circle2", "circle 2 points")}},
            {{"c2", QObject::tr("c2", "circle 2 points")}},
            RS2::ActionDrawCircle2P
        },
        //draw 3 point circle
        {
            {{"circle3", QObject::tr("circle3", "circle 3 points")}},
            {{"c3", QObject::tr("c3", "circle 3 points")}},
            RS2::ActionDrawCircle3P
        },
	//draw circle with point and radius
	{
		{{"circlecr", QObject::tr("circlecr", "circle with center and radius")}},
		{{"cc", QObject::tr("cc", "circle with center and radius")}},
		RS2::ActionDrawCircleCR
	},
        //draw circle tangent to 3 objects
        {
            {{"tan3", QObject::tr("tan3", "circle tangent to 3")}},
            {{"ct3", QObject::tr("ct3", "circle tangent to 3")}},
            RS2::ActionDrawCircleTan3
        },
        //draw inscribed ellipse
        {
            {{"ellipseinscribed", QObject::tr("ellipseinscribed", "inscribed ellipse")}},
            {{"ei", QObject::tr("ei", "inscribed ellipse")},
            {"ie", QObject::tr("ie", "inscribed ellipse")}},
            RS2::ActionDrawEllipseInscribe
        },
        //draw hatch
        {
            {{"hatch", QObject::tr("hatch", "draw hatch")}},
            {{"ha", QObject::tr("ha", "draw hatch")}},
            RS2::ActionDrawHatchNoSelect
        },
        //draw mtext
        {
            {{"mtext", QObject::tr("mtext", "draw mtext")}},
            {{"mtxt", QObject::tr("mtxt", "draw mtext")}},
            RS2::ActionDrawMText
        },
        //draw text
        {
            {{"text", QObject::tr("text", "draw text")}},
            {{"txt", QObject::tr("txt", "draw text")}},
            RS2::ActionDrawText
        },
        //zoom redraw
        {
            {{"regen", QObject::tr("regen", "zoom - redraw")},
             {"redraw", QObject::tr("redraw", "zoom - redraw")}},
            {{"rg", QObject::tr("rg", "zoom - redraw")},
            {"zr", QObject::tr("zr", "zoom - redraw")}},
            RS2::ActionZoomRedraw
        },
        //zoom window
        {
            {{"zoomwindow", QObject::tr("zoomwindow", "zoom - window")}},
            {{"zw", QObject::tr("zw", "zoom - window")}},
            RS2::ActionZoomWindow
        },
        //zoom auto
        {
            {{"zoomauto", QObject::tr("zoomauto", "zoom - auto")}},
            {{"za", QObject::tr("za", "zoom - auto")}},
            RS2::ActionZoomAuto
        },
        //zoom pan
        {
            {{"zoompan", QObject::tr("zoompan", "zoom - pan")}},
            {{"zp", QObject::tr("zp", "zoom - pan")}},
            RS2::ActionZoomPan
        },
        //zoom previous
        {
            {{"zoomprevious", QObject::tr("zoomprevious", "zoom - previous")}},
            {{"zv", QObject::tr("zv", "zoom - previous")}},
            RS2::ActionZoomPrevious
        },
        //kill actions
        {
            {{"kill", QObject::tr("kill", "kill all actions")}},
            {{"k", QObject::tr("k", "kill all actions")}},
            RS2::ActionEditKillAllActions
        },
        //undo cycle
        {
            {{"undo", QObject::tr("undo", "undo cycle")}},
            {{"u", QObject::tr("u", "undo cycle")}},
            RS2::ActionEditUndo
        },
        //redo cycle
        {
            {{"redo", QObject::tr("redo", "redo cycle")}},
            {{"r", QObject::tr("r", "redo cycle")}},
            RS2::ActionEditRedo
        },
        //dimension aligned
        {
            {{"dimaligned", QObject::tr("dimaligned", "dimension - aligned")}},
            {{"da", QObject::tr("da", "dimension - aligned")}},
            RS2::ActionDimAligned
        },
        //dimension horizontal
        {
            {{"dimhorizontal", QObject::tr("dimhorizontal", "dimension - horizontal")}},
            {{"dh", QObject::tr("dh", "dimension - horizontal")}},
            RS2::ActionDimLinearHor
        },
        //dimension vertical
        {
            {{"dimvertical", QObject::tr("dimvertical", "dimension - vertical")}},
            {{"dv", QObject::tr("dv", "dimension - vertical")}},
            RS2::ActionDimLinearVer
        },
        //dimension linear
        {
            {{"dimlinear", QObject::tr("dimlinear", "dimension - linear")}},
			{{"dl", QObject::tr("dl", "dimension - linear")},
             {"dr", QObject::tr("dr", "dimension - linear")}},
            RS2::ActionDimLinear
        },
	//dimension angular
	{
		{{"dimangular", QObject::tr("dimangular", "dimension - angular")}},
		{{"dan", QObject::tr("dan", "dimension - angular")}},
		RS2::ActionDimAngular
	},
	//dimension radius
	{
		{{"dimradial", QObject::tr("dimradial", "dimension - radial")}},
		{{"dimradius", QObject::tr("dimradius", "dimension - radius")}},
		RS2::ActionDimRadial
	},
	//dimension diameter
	{
		{{"dimdiametric", QObject::tr("dimdiametric", "dimension - diametric")}},
		{{"dimdiameter", QObject::tr("dimdiameter", "dimension - diametric")},
		 {"dd", QObject::tr("dd", "dimension - diametric")}},
		RS2::ActionDimDiametric
	},
        //dimension leader
        {
            {{"dimleader", QObject::tr("dimleader", "dimension - leader")}},
            {{"ld", QObject::tr("ld", "dimension - leader")}},
            RS2::ActionDimLeader
        },
        //dimension regenerate
        {
            {{"dimregen", QObject::tr("dimregen", "dimension - regenerate")}},
            {},
            RS2::ActionToolRegenerateDimensions
        },
        //snap restrictions
        {
            {{"restrictnothing", QObject::tr("restrictnothing", "restrict - nothing")}},
            {{"rn", QObject::tr("rn", "restrict - nothing")}},
            RS2::ActionRestrictNothing
        },
        //snap orthogonal
        {
            {{"restrictorthogonal", QObject::tr("restrictorthogonal", "restrict - orthogonal")}},
            {{"rr", QObject::tr("rr", "restrict - orthogonal")}},
            RS2::ActionRestrictOrthogonal
        },
        //snap horizontal
        {
            {{"restricthorizontal", QObject::tr("restricthorizontal", "restrict - horizontal")}},
            {{"rh", QObject::tr("rh", "restrict - horizontal")}},
            RS2::ActionRestrictHorizontal
        },
        //snap vertical
        {
            {{"restrictvertical", QObject::tr("restrictvertical", "restrict - vertical")}},
            {{"rv", QObject::tr("rv", "restrict - vertical")}},
            RS2::ActionRestrictVertical
        },
        //move
        {
            {{"move", QObject::tr("move", "modify - move (copy)")}},
            {{"mv", QObject::tr("mv", "modify - move (copy)")}},
            RS2::ActionModifyMove
        },
        //bevel
        {
            {{"bevel", QObject::tr("bevel", "modify - bevel")}},
            {{"bev", QObject::tr("bev", "modify - bevel")},
            {"ch", QObject::tr("ch", "modify - bevel")}},
            RS2::ActionModifyBevel
        },
        //fillet
        {
            {{"fillet", QObject::tr("fillet", "modify - fillet")}},
            {{"fi", QObject::tr("fi", "modify - fillet")}},
            RS2::ActionModifyRound
        },
        //divide
        {
            {{"divide", QObject::tr("divide", "modify - divide (cut)")},
             {"cut", QObject::tr("cut", "modify - divide (cut)")}},
            {{"div", QObject::tr("div", "modify - divide (cut)")},
            {"di", QObject::tr("di", "modify - divide (cut)")}},
            RS2::ActionModifyCut
        },
        //mirror
        {
            {{"mirror", QObject::tr("mirror", "modify -  mirror")}},
            {{"mi", QObject::tr("mi", "modify -  mirror")}},
            RS2::ActionModifyMirror
        },
        //revert
        {
            {{"revert", QObject::tr("revert", "modify -  revert direction")}},
            {{"rev", QObject::tr("rev", "modify -  revert direction")}},
            RS2::ActionModifyRevertDirection
        },
        //rotate
        {
            {{"rotate", QObject::tr("rotate", "modify - rotate")}},
            {{"ro", QObject::tr("ro", "modify - rotate")}},
            RS2::ActionModifyRotate
        },
        //scale
        {
            {{"scale", QObject::tr("scale", "modify - scale")}},
            {{"sz", QObject::tr("sz", "modify - scale")}},
            RS2::ActionModifyScale
        },
        //trim
        {
            {{"trim", QObject::tr("trim", "modify - trim (extend)")}},
            {{"tm", QObject::tr("tm", "modify - trim (extend)")}},
            RS2::ActionModifyTrim
        },
        //trim2
        {
            {{"trim2", QObject::tr("trim2", "modify - multi trim (extend)")}},
            {{"tm2", QObject::tr("tm2", "modify - multi trim (extend)")},
             {"t2", QObject::tr("t2", "modify - multi trim (extend)")}},
            RS2::ActionModifyTrim2
        },
        //lengthen
        {
            {{"lengthen", QObject::tr("lengthen", "modify - lengthen")}},
            {{"le", QObject::tr("le", "modify - lengthen")}},
            RS2::ActionModifyTrimAmount
        },
        //stretch
        {
            {{"stretch", QObject::tr("stretch", "modify - stretch")}},
            {{"ss", QObject::tr("ss", "modify - stretch")}},
            RS2::ActionModifyStretch
        },
        //delete
        {
            {{"delete", QObject::tr("delete", "modify - delete (erase)")}},
            {{"er", QObject::tr("er", "modify - delete (erase)")},
             {"del", QObject::tr("del", "modify - delete (erase)")}},
            RS2::ActionModifyDelete
		},
        //explode
        {
            {{"explode", QObject::tr("explode", "explode block/polyline into entities")}},
            {{"xp", QObject::tr("xp", "explode block/polyline into entities")}},
            RS2::ActionBlocksExplode
        },
        //snap free
        {
            {{"snapfree", QObject::tr("snapfree", "snap - free")}},
            {{"os", QObject::tr("os", "snap - free")},
            {"sf", QObject::tr("sf", "snap - free")}},
            RS2::ActionSnapFree
        },
        //snap center
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
        //snap end
        {
            {{"snapend", QObject::tr("snapend", "snap - end points")}},
            {{"se", QObject::tr("se", "snap - end points")}},
            RS2::ActionSnapEndpoint
        },
        //snap grid
        {
            {{"snapgrid", QObject::tr("snapgrid", "snap - grid")}},
            {{"sg", QObject::tr("sg", "snap - grid")}},
            RS2::ActionSnapGrid
        },
        //snap intersection
        {
            {{"snapintersection", QObject::tr("snapintersection", "snap - intersection")}},
            {{"si", QObject::tr("si", "snap - intersection")}},
            RS2::ActionSnapIntersection
        },
        //snap middle
        {
            {{"snapmiddle", QObject::tr("snapmiddle", "snap - middle points")}},
            {{"sm", QObject::tr("sm", "snap - middle points")}},
            RS2::ActionSnapMiddle
        },
        //snap on entity
        {
            {{"snaponentity", QObject::tr("snaponentity", "snap - on entity")}},
            {{"sn", QObject::tr("sn", "snap - on entity")},
            {"np", QObject::tr("np", "snap - on entity")}},
            RS2::ActionSnapOnEntity
        },
        //set relative zero
        {
            {{"setrelativezero", QObject::tr("setrelativezero", "set relative zero position")}},
            {{"rz", QObject::tr("rz", "set relative zero position")}},
            RS2::ActionSetRelativeZero
        },
        //Select all entities
        {
            {{"selectall", QObject::tr("selectall", "Select all entities")}},
            {{"sa", QObject::tr("sa", "Select all entities")}},
            RS2::ActionSelectAll
        },
        //DeSelect all entities
        {
            {{"deselectall", QObject::tr("deselectall", "deselect all entities")}},
            {{"tn", QObject::tr("tn", "deselect all entities")}},
            RS2::ActionDeselectAll
        },
        //Modify Attributes
        {
            {{"modifyattr", QObject::tr("modifyattr", "modify attribute")}},            
            {{"attr", QObject::tr("attr", "modify attribute")},
            {"ma", QObject::tr("ma", "modify attribute")}},
            RS2::ActionModifyAttributes
        },
        //Modify Properties
        {
            {{"properties", QObject::tr("properties", "modify properties")}},
            {{"prop", QObject::tr("prop", "modify properties")},
             {"mp", QObject::tr("mp", "modify properties")}},
            RS2::ActionModifyEntity
        },
        //Distance Point to Point
        {
            {{"distance", QObject::tr("distance", "distance point to point")}},
            {{"dist", QObject::tr("dist", "distance point to point")},
            {"dpp", QObject::tr("dpp", "distance point to point")}},
            RS2::ActionInfoDist
		},
        //Measure angle
        {
            {{"angle", QObject::tr("angle", "measure angle")}},
            {{"ang", QObject::tr("ang", "measure angle")}},
            RS2::ActionInfoAngle
        },
        //Measure area
        {
            {{"area", QObject::tr("area", "measure area")}},
            {{"ar", QObject::tr("ar", "measure area")}},
            RS2::ActionInfoArea
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
                QStringList txtList = line.split(QRegExp(R"(\s)"),QString::SkipEmptyParts);
                if (txtList.size()> 1) {
//                    qDebug()<<"reading: "<<txtList.at(0)<<"\t"<< txtList.at(1);
                    aliasList[txtList.at(0)]=txtList.at(1);
                }
            }
        }
    } else {
    //alias file do no exist, create one with translated shortCommands
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream ts(&f);
            ts << "#LibreCAD alias v1" << endl << endl;
            ts << "# lines starting with # are comments" << endl;
            ts << "# format are:" << endl;
            ts << R"(# <alias>\t<command-untranslated>)" << endl;
            ts << "# example"<<endl;
            ts << "# l\tline"<<endl<<endl;
            for(auto const& p: shortCommands){
                auto const act=p.second;
                for(auto const& pCmd: mainCommands){
                    if(pCmd.second==act){
                        ts<<p.first<<'\t'<<pCmd.first<<endl;
                        break;
                    }
                }
            }
            ts.flush();
        }

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
QStringList RS_Commands::complete(const QString& cmd) {
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
RS2::ActionType RS_Commands::cmdToAction(const QString& cmd, bool verbose) {
    QString full = cmd.toLower();
    RS2::ActionType ret = RS2::ActionNone;

        // find command:
//	RS2::ActionType* retPtr = mainCommands.value(cmd);
        if ( mainCommands.count(cmd) ) {
                ret = mainCommands[cmd];
        } else if ( shortCommands.count(cmd) ) {
                ret = shortCommands[cmd];
		} else
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
RS2::ActionType RS_Commands::keycodeToAction(const QString& code) {
	if(code.size() < 1)
		return RS2::ActionNone;

	QString c;

    if(!(code.startsWith(FnPrefix) ||
         code.startsWith(AltPrefix) ||
         code.startsWith(MetaPrefix))) {
    	if(code.size() < 1 || code.contains(QRegExp("^[a-z].*",Qt::CaseInsensitive)) == false )
			 return RS2::ActionNone;
	    c = code.toLower();
	} else {
		c = code;
	}


//    std::cout<<"regex: "<<qPrintable(c)<<" matches: "<< c.contains(QRegExp("^[a-z].*",Qt::CaseInsensitive))<<std::endl;
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

/**
 * @brief extractCliCal, filter cli calculator math expression
 * @param cmd, cli string
 * @return math expression for RS_Math:eval();
 */
QString RS_Commands::filterCliCal(const QString& cmd)
{

    QString str=cmd.trimmed();
    const QRegExp calCmd(R"(^(cal|calculate))");
    if(!(str.contains(calCmd)
         || str.startsWith(QObject::tr("cal","command to trigger cli calculator"), Qt::CaseInsensitive)
         || str.startsWith(QObject::tr("calculate","command to trigger cli calculator"), Qt::CaseInsensitive)
                           )) {
        return QString();
    }
    int index=str.indexOf(QRegExp(R"(\s)"));
    bool spaceFound=(index>=0);
    str=str.mid(index);
    index=str.indexOf(QRegExp(R"(\S)"));
    if(!(spaceFound && index>=0)) return QString();
    str=str.mid(index);
    return str;
}

// EOF
