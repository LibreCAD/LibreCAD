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

#include <QObject>
#include <QTextStream>
#include "rs_commands.h"

#include "rs_system.h"
#include "rs_dialogfactory.h"

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
    // draw:
    cmdTranslation["point"]=QObject::tr("point", "draw point");
    mainCommands[QObject::tr("point", "draw point")]=RS2::ActionDrawPoint;
    cmdTranslation["po"]=QObject::tr("po", "draw point");
    shortCommands[QObject::tr("po", "draw point")]=RS2::ActionDrawPoint;

    cmdTranslation["line"]=QObject::tr("line", "draw line");
    mainCommands[QObject::tr("line", "draw line")]=RS2::ActionDrawLine;
    cmdTranslation["li"]=QObject::tr("li", "draw line");
    shortCommands[QObject::tr("li", "draw line")]=RS2::ActionDrawLine;
    shortCommands[QObject::tr("l", "draw line")]=RS2::ActionDrawLine;

    cmdTranslation["polyline"]=QObject::tr("polyline", "draw polyline");
    mainCommands[QObject::tr("polyline", "draw polyline")]=RS2::ActionDrawPolyline;
    cmdTranslation["pl"]=QObject::tr("pl", "draw polyline");
    shortCommands[QObject::tr("pl", "draw polyline")]=RS2::ActionDrawPolyline;

    cmdTranslation["offset"]=QObject::tr("offset");
    mainCommands[QObject::tr("offset")]=RS2::ActionDrawLineParallel;
    shortCommands[QObject::tr("o", "offset")]=RS2::ActionDrawLineParallel;
    cmdTranslation["parallel"]=QObject::tr("parallel");
    mainCommands[QObject::tr("parallel")]=RS2::ActionDrawLineParallel;
    cmdTranslation["pa"]=QObject::tr("pa", "parallel");
    shortCommands[QObject::tr("pa", "parallel")]=RS2::ActionDrawLineParallel;

    cmdTranslation["arc"]=QObject::tr("arc", "draw arc");
    mainCommands[QObject::tr("arc", "draw arc")]=RS2::ActionDrawArc3P;
    cmdTranslation["ar"]=QObject::tr("ar", "draw arc");
    mainCommands[QObject::tr("ar", "draw arc")]=RS2::ActionDrawArc3P;
    shortCommands[QObject::tr("a", "draw arc")]=RS2::ActionDrawArc3P;

    cmdTranslation["circle"]=QObject::tr("circle", "draw circle");
    mainCommands[QObject::tr("circle", "draw circle")]=RS2::ActionDrawCircle;
    cmdTranslation["ci"]=QObject::tr("ci", "draw circle");
    shortCommands[QObject::tr("ci", "draw circle")]=RS2::ActionDrawCircle;

    cmdTranslation["rectangle"]=QObject::tr("rectangle", "draw rectangle");
    mainCommands[QObject::tr("rectangle", "draw rectangle")]=RS2::ActionDrawLineRectangle;
    cmdTranslation["rect"]=QObject::tr("rect", "draw rectangle");
    shortCommands[QObject::tr("rectang", "draw rectangle")]=RS2::ActionDrawLineRectangle;
    shortCommands[QObject::tr("rect", "draw rectangle")]=RS2::ActionDrawLineRectangle;
    shortCommands[QObject::tr("rec", "draw rectangle")]=RS2::ActionDrawLineRectangle;

    cmdTranslation["mtext"]=QObject::tr("mtext", "draw mtext");
    mainCommands[QObject::tr("mtext", "draw mtext")]=RS2::ActionDrawMText;
    cmdTranslation["text"]=QObject::tr("text", "draw text");
    mainCommands[QObject::tr("text", "draw text")]=RS2::ActionDrawText;

    // zoom:
    cmdTranslation["regen"]=QObject::tr("regen");
    cmdTranslation["redraw"]=QObject::tr("redraw");
    mainCommands[QObject::tr("regen")]=RS2::ActionZoomRedraw;
    mainCommands[QObject::tr("redraw")]=RS2::ActionZoomRedraw;
    shortCommands[QObject::tr("rg", "zoom - redraw")]=RS2::ActionZoomRedraw;

    cmdTranslation["zr"]=QObject::tr("zr", "zoom - redraw");
    shortCommands[QObject::tr("zr", "zoom - redraw")]=RS2::ActionZoomRedraw;

    cmdTranslation["zw"]=QObject::tr("zw");
    mainCommands[QObject::tr("zw", "zoom - window")]=RS2::ActionZoomWindow;

    cmdTranslation["za"]=QObject::tr("za");
    mainCommands[QObject::tr("za", "zoom - auto")]=RS2::ActionZoomAuto;

    cmdTranslation["zp"]=QObject::tr("zp");
    mainCommands[QObject::tr("zp", "zoom - pan")]=RS2::ActionZoomPan;

    cmdTranslation["zv"]=QObject::tr("zv");
    mainCommands[QObject::tr("zv", "zoom - previous")]=RS2::ActionZoomPrevious;

    // edit:
    cmdTranslation["kill"]=QObject::tr("kill");
    mainCommands[QObject::tr("kill")]=RS2::ActionEditKillAllActions;
    cmdTranslation["k"]=QObject::tr("k");
    shortCommands[QObject::tr("k")]=RS2::ActionEditKillAllActions;

    cmdTranslation["undo"]=QObject::tr("undo", "undo");
    mainCommands[QObject::tr("undo", "undo")]=RS2::ActionEditUndo;
    cmdTranslation["u"]=QObject::tr("u", "undo");
    shortCommands[QObject::tr("u", "undo")]=RS2::ActionEditUndo;

    cmdTranslation["redo"]=QObject::tr("redo");
    mainCommands[QObject::tr("redo")]=RS2::ActionEditRedo;
    cmdTranslation["r"]=QObject::tr("r");
    shortCommands[QObject::tr("r")]=RS2::ActionEditRedo;

    // dimensions:
    cmdTranslation["da"]=QObject::tr("da");
    mainCommands[QObject::tr("da", "dimension - aligned")]=RS2::ActionDimAligned;
    shortCommands[QObject::tr("da", "dimension - aligned")]=RS2::ActionDimAligned;

    cmdTranslation["dh"]=QObject::tr("dh", "dimension - horizontal");
    mainCommands[QObject::tr("dh", "dimension - horizontal")]=RS2::ActionDimLinearHor;
    shortCommands[QObject::tr("dh", "dimension - horizontal")]=RS2::ActionDimLinearHor;

    cmdTranslation["dr"]=QObject::tr("dr", "dimension - linear");
    mainCommands[QObject::tr("dr", "dimension - linear")]=RS2::ActionDimLinear;
    shortCommands[QObject::tr("dr", "dimension - linear")]=RS2::ActionDimLinear;

    cmdTranslation["dv"]=QObject::tr("dv", "dimension - vertical");
    mainCommands[QObject::tr("dv", "dimension - vertical")]=RS2::ActionDimLinearVer;
    shortCommands[QObject::tr("dv", "dimension - vertical")]=RS2::ActionDimLinearVer;

    cmdTranslation["ld"]=QObject::tr("ld", "dimension - leader");
    mainCommands[QObject::tr("ld", "dimension - leader")]=RS2::ActionDimLeader;
    shortCommands[QObject::tr("ld", "dimension - leader")]=RS2::ActionDimLeader;

    // tools:
    cmdTranslation["dimregen"]=QObject::tr("dimregen", "dimension - regenerate");
    mainCommands[QObject::tr("dimregen", "dimension - regenerate")]=RS2::ActionToolRegenerateDimensions;

	 // restrictions:
	cmdTranslation["rn"]=QObject::tr("rn", "restrict - nothing");
	mainCommands[QObject::tr("rn", "restrict - nothing")]=RS2::ActionRestrictNothing;
	shortCommands[QObject::tr("rn")]=RS2::ActionRestrictNothing;

    cmdTranslation["rr"]=QObject::tr("rr", "restrict - orthogonal");
    mainCommands[QObject::tr("rr", "restrict - orthogonal")]=RS2::ActionRestrictOrthogonal;
    shortCommands[QObject::tr("rr")]=RS2::ActionRestrictOrthogonal;

    cmdTranslation["rh"]=QObject::tr("rh", "restrict - horizontal");
    mainCommands[QObject::tr("rh", "restrict - horizontal")]=RS2::ActionRestrictHorizontal;
    shortCommands[QObject::tr("rh")]=RS2::ActionRestrictHorizontal;

    cmdTranslation["rv"]=QObject::tr("rv", "restrict - vertical");
    mainCommands[QObject::tr("rv", "restrict - vertical")]=RS2::ActionRestrictVertical;
    shortCommands[QObject::tr("rv")]=RS2::ActionRestrictVertical;

    // modify:
    cmdTranslation["trim2"]=QObject::tr("trim2", "modify - multi trim (extend)");
    mainCommands[QObject::tr("trim2", "modify - multi trim (extend)")]=RS2::ActionModifyTrim2;
    cmdTranslation["tm2"]=QObject::tr("tm2");
    shortCommands[QObject::tr("tm2")]=RS2::ActionModifyTrim2;

    cmdTranslation["trim"]=QObject::tr("trim", "modify - trim (extend)");
    mainCommands[QObject::tr("trim", "modify - trim (extend)")]=RS2::ActionModifyTrim;
    cmdTranslation["tm"]=QObject::tr("tm", "modify - trim (extend)");
    shortCommands[QObject::tr("tm", "modify - trim (extend)")]=RS2::ActionModifyTrim;

//    cmdTranslation["rm"]=QObject::tr("rm");
//    shortCommands[QObject::tr("rm")]=RS2::ActionModifyTrim;

    cmdTranslation["move"]=QObject::tr("move", "modify - move");
    mainCommands[QObject::tr("move", "modify - move")]=RS2::ActionModifyMove;
    cmdTranslation["mv"]=QObject::tr("mv", "modify - move");
    shortCommands[QObject::tr("mv", "modify - move")]=RS2::ActionModifyMove;

    cmdTranslation["bevel"]=QObject::tr("bevel", "modify - bevel (chamfer)");
    mainCommands[QObject::tr("bevel", "modify - bevel (chamfer)")]=RS2::ActionModifyBevel;
    cmdTranslation["ch"]=QObject::tr("ch", "modify - bevel (chamfer)");
    shortCommands[QObject::tr("ch", "modify - bevel (chamfer)")]=RS2::ActionModifyBevel;
    cmdTranslation["fillet"]=QObject::tr("fillet", "modify - fillet");
    mainCommands[QObject::tr("fillet", "modify - fillet")]=RS2::ActionModifyBevel;
    cmdTranslation["fi"]=QObject::tr("fi", "modify - fillet");
    shortCommands[QObject::tr("fi", "modify - fillet")]=RS2::ActionModifyBevel;

    cmdTranslation["divide"]=QObject::tr("divide", "modify - divide");
    mainCommands[QObject::tr("divide", "modify - divide")]=RS2::ActionModifyCut;
    shortCommands[QObject::tr("div", "modify - divide")]=RS2::ActionModifyCut;
    cmdTranslation["cut"]=QObject::tr("cut", "modify - divide");
    mainCommands[QObject::tr("cut", "modify - divide")]=RS2::ActionModifyCut;

    cmdTranslation["mirror"]=QObject::tr("mirror");
    mainCommands[QObject::tr("mirror", "modify - mirror")]=RS2::ActionModifyMirror;
    cmdTranslation["mi"]=QObject::tr("mi");
    shortCommands[QObject::tr("mi", "modify - mirror")]=RS2::ActionModifyMirror;

	cmdTranslation["revert"]=QObject::tr("revert", "modify - revert direction");
	mainCommands[QObject::tr("revert", "modify - revert direction")]=RS2::ActionModifyRevertDirection;
	cmdTranslation["re"]=QObject::tr("re");
	shortCommands[QObject::tr("re", "modify - revert direction")]=RS2::ActionModifyRevertDirection;

	cmdTranslation["rotate"]=QObject::tr("rotate", "modify - rotate");
	mainCommands[QObject::tr("rotate", "modify - rotate")]=RS2::ActionModifyRotate;
	cmdTranslation["ro"]=QObject::tr("ro", "modify - rotate");
	shortCommands[QObject::tr("ro", "modify - rotate")]=RS2::ActionModifyRotate;

    cmdTranslation["scale"]=QObject::tr("scale", "modify - scale");
    mainCommands[QObject::tr("scale", "modify - scale")]=RS2::ActionModifyScale;
    cmdTranslation["sz"]=QObject::tr("sz", "modify - scale");
    shortCommands[QObject::tr("sz", "modify - scale")]=RS2::ActionModifyScale;

    cmdTranslation["stretch"]=QObject::tr("stretch", "modify - stretch");
    mainCommands[QObject::tr("stretch", "modify - stretch")]=RS2::ActionModifyStretch;
    cmdTranslation["ss"]=QObject::tr("ss", "modify - stretch");
    shortCommands[QObject::tr("ss", "modify - stretch")]=RS2::ActionModifyStretch;

    cmdTranslation["delete"]=QObject::tr("delete", "modify - delete (erase)");
    mainCommands[QObject::tr("delete", "modify - delete (erase)")]=RS2::ActionModifyDelete;
    cmdTranslation["er"]=QObject::tr("er", "modify - delete (erase)");
    shortCommands[QObject::tr("er", "modify - delete (erase)")]=RS2::ActionModifyDelete;

    cmdTranslation["undo"]=QObject::tr("undo", "modify - undo (oops)");
    mainCommands[QObject::tr("undo", "modify - undo (oops)")]=RS2::ActionEditUndo;
    cmdTranslation["oo"]=QObject::tr("oo", "modify - undo (oops)");
    shortCommands[QObject::tr("oo", "modify - undo (oops)")]=RS2::ActionEditUndo;

    cmdTranslation["redo"]=QObject::tr("redo", "modify - redo");
    mainCommands[QObject::tr("redo", "modify - redo")]=RS2::ActionEditRedo;
    cmdTranslation["uu"]=QObject::tr("uu", "modify - redo");
    shortCommands[QObject::tr("uu", "modify - redo")]=RS2::ActionEditRedo;

    cmdTranslation["explode"]=QObject::tr("explode");
    mainCommands[QObject::tr("explode", "modify - explode blocks/polylines")]=RS2::ActionBlocksExplode;
    cmdTranslation["xp"]=QObject::tr("xp", "modify - explode blocks/polylines");
    shortCommands[QObject::tr("xp", "modify - explode blocks/polylines")]=RS2::ActionBlocksExplode;

    // snap:
    cmdTranslation["os"]=QObject::tr("os", "snap - free");
    mainCommands[QObject::tr("os", "snap - free")]=RS2::ActionSnapFree;
    shortCommands[QObject::tr("os")]=RS2::ActionSnapFree;

    cmdTranslation["sc"]=QObject::tr("sc");
    mainCommands[QObject::tr("sc", "snap - center")]=RS2::ActionSnapCenter;
    shortCommands[QObject::tr("sc")]=RS2::ActionSnapCenter;

    cmdTranslation["sd"]=QObject::tr("sd");
    mainCommands[QObject::tr("sd", "snap - distance")]=RS2::ActionSnapDist;
    shortCommands[QObject::tr("sd")]=RS2::ActionSnapDist;

    cmdTranslation["se"]=QObject::tr("se");
    mainCommands[QObject::tr("se", "snap - end")]=RS2::ActionSnapEndpoint;
    shortCommands[QObject::tr("se")]=RS2::ActionSnapEndpoint;

    cmdTranslation["sf"]=QObject::tr("sf");
    mainCommands[QObject::tr("sf", "snap - free")]=RS2::ActionSnapFree;
    shortCommands[QObject::tr("sf")]=RS2::ActionSnapFree;

    cmdTranslation["sg"]=QObject::tr("sg");
    mainCommands[QObject::tr("sg", "snap - grid")]=RS2::ActionSnapGrid;
    shortCommands[QObject::tr("sg")]=RS2::ActionSnapGrid;

    cmdTranslation["si"]=QObject::tr("si");
    mainCommands[QObject::tr("si", "snap - intersection")]=RS2::ActionSnapIntersection;
    shortCommands[QObject::tr("si")]=RS2::ActionSnapIntersection;

    cmdTranslation["sm"]=QObject::tr("sm");
    mainCommands[QObject::tr("sm", "snap - middle")]=RS2::ActionSnapMiddle;
    shortCommands[QObject::tr("sm")]=RS2::ActionSnapMiddle;

    cmdTranslation["sn"]=QObject::tr("sn");
    mainCommands[QObject::tr("sn", "snap - nearest")]=RS2::ActionSnapOnEntity;
    shortCommands[QObject::tr("sn")]=RS2::ActionSnapOnEntity;

    cmdTranslation["np"]=QObject::tr("np");
    mainCommands[QObject::tr("np", "snap - nearest point")]=RS2::ActionSnapOnEntity;
    shortCommands[QObject::tr("np")]=RS2::ActionSnapOnEntity;

    cmdTranslation["setrelativezero"]=QObject::tr("setrelativezero");
    mainCommands[QObject::tr("setrelativezero", "snap - set relative zero position")]=RS2::ActionSetRelativeZero;
    cmdTranslation["rz"]=QObject::tr("rz");
    shortCommands[QObject::tr("rz")]=RS2::ActionSetRelativeZero;

    // selection:
    cmdTranslation["sa"]=QObject::tr("sa");
    mainCommands[QObject::tr("sa", "Select all")]=RS2::ActionSelectAll;
    shortCommands[QObject::tr("sa")]=RS2::ActionSelectAll;

    cmdTranslation["tn"]=QObject::tr("tn");
    mainCommands[QObject::tr("tn", "Deselect all")]=RS2::ActionDeselectAll;
    shortCommands[QObject::tr("tn")]=RS2::ActionDeselectAll;

    cmdTranslation["angle"]=QObject::tr("angle");
    cmdTranslation["dpi"]=QObject::tr("dpi");
    cmdTranslation["close"]=QObject::tr("close");
    cmdTranslation["chord length"]=QObject::tr("chord length");
    cmdTranslation["columns"]=QObject::tr("columns");
    cmdTranslation["columnspacing"]=QObject::tr("columnspacing");
    cmdTranslation["factor"]=QObject::tr("factor");
    cmdTranslation["length"]=QObject::tr("length");
    cmdTranslation["length1"]=QObject::tr("length1");
    cmdTranslation["length2"]=QObject::tr("length2");
    cmdTranslation["number"]=QObject::tr("number");
    cmdTranslation["radius"]=QObject::tr("radius");
    cmdTranslation["rows"]=QObject::tr("rows");
    cmdTranslation["rowspacing"]=QObject::tr("rowspacing");
    cmdTranslation["through"]=QObject::tr("through");
    cmdTranslation["trim"]=QObject::tr("trim");

/** following are reversed translation]=i.e.]=from translated to english **/
    //not used as command keywords
// used in function]=checkCommand()
    cmdTranslation[QObject::tr("angle")]="angle";
    cmdTranslation[QObject::tr("ang", "angle")]="angle";
    cmdTranslation[QObject::tr("an", "angle")]="angle";

    cmdTranslation[QObject::tr("center")]="center";
    cmdTranslation[QObject::tr("cen", "center")]="center";
    cmdTranslation[QObject::tr("ce", "center")]="center";

    cmdTranslation[QObject::tr("chord length")]="chord length";
//    cmdTranslation[QObject::tr("length", "chord length")]="chord length";
    cmdTranslation[QObject::tr("cl", "chord length")]="chord length";

    cmdTranslation[QObject::tr("close")]="close";
    cmdTranslation[QObject::tr("c", "close")]="close";

    cmdTranslation[QObject::tr("columns")]="columns";
    cmdTranslation[QObject::tr("cols", "columns")]="columns";
    cmdTranslation[QObject::tr("co", "columns")]="columns";

    cmdTranslation[QObject::tr("columnspacing", "columnspacing for inserts")]="columnspacing";
    cmdTranslation[QObject::tr("colspacing", "columnspacing for inserts")]="columnspacing";
    cmdTranslation[QObject::tr("cs", "columnspacing for inserts")]="columnspacing";

    cmdTranslation[QObject::tr("factor")]="factor";
    cmdTranslation[QObject::tr("fact", "factor")]="factor";
    cmdTranslation[QObject::tr("f", "factor")]="factor";

    cmdTranslation[QObject::tr("help")]="help";
    cmdTranslation[QObject::tr("?", "help")]="help";

    cmdTranslation[QObject::tr("length","length")]="length";
    cmdTranslation[QObject::tr("len","length")]="length";
    cmdTranslation[QObject::tr("l","length")]="length";

    cmdTranslation[QObject::tr("length1","length1")]="length1";
    cmdTranslation[QObject::tr("len1","length1")]="length1";
    cmdTranslation[QObject::tr("l1","length1")]="length1";

    cmdTranslation[QObject::tr("length2","length2")]="length2";
    cmdTranslation[QObject::tr("len2","length2")]="length2";
    cmdTranslation[QObject::tr("l2","length2")]="length2";

    cmdTranslation[QObject::tr("number","number")]="number";
    cmdTranslation[QObject::tr("num","number")]="number";
    cmdTranslation[QObject::tr("n","number")]="number";

    cmdTranslation[QObject::tr("radius")]="radius";
    cmdTranslation[QObject::tr("ra","radius")]="radius";

    cmdTranslation[QObject::tr("reversed","reversed")]="reversed";
    cmdTranslation[QObject::tr("rev","reversed")]="reversed";
    cmdTranslation[QObject::tr("rev","reversed")]="reversed";

    cmdTranslation[QObject::tr("row", "row")]="row";

    cmdTranslation[QObject::tr("rowspacing", "rowspacing for inserts")]="rowspacing";
    cmdTranslation[QObject::tr("rs","rowspacing for inserts")]="rowspacing";

    cmdTranslation[QObject::tr("text")]="text";
    cmdTranslation[QObject::tr("t","text")]="text";

    cmdTranslation[QObject::tr("through")]="through";
    cmdTranslation[QObject::tr("t","through")]="through";

    cmdTranslation[QObject::tr("undo")]="undo";
    cmdTranslation[QObject::tr("u","undo")]="undo";

    cmdTranslation[QObject::tr("redo")]="redo";
    cmdTranslation[QObject::tr("r","redo")]="redo";

    cmdTranslation[QObject::tr("back")]="back";
    cmdTranslation[QObject::tr("b","back")]="back";
    //printer preview
    cmdTranslation[QObject::tr("paperoffset")]="paperoffset";
    cmdTranslation[QObject::tr("graphoffset")]="graphoffset";

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
//    qDebug()<<"alisa file:\t"<<aliasName;
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
            ts << "# alias<\\t>command-untranslated" << endl << endl;
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
//RLZ: to be writen

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
        }

        // find full command to confirm to user:
        if(verbose){
            for(auto const& p: mainCommands){
                if(p.second==ret){
                    if (RS_DIALOGFACTORY) {
                        RS_DEBUG->print("RS_Commands::cmdToAction: commandMessage");
                        //RS_DIALOGFACTORY->commandMessage(QObject::tr("Command: %1")
                        //	.arg(full));
                        RS_DIALOGFACTORY->commandMessage(QObject::tr("Command: %1 (%2)").arg(full).arg(p.first));
                        //                                        RS_DialogFactory::instance()->commandMessage( QObject::tr("Command: %1").arg(full));
                        RS_DEBUG->print("RS_Commands::cmdToAction: "
                                        "commandMessage: ok");
                    }
                    return ret;
                }
            }
            RS_DEBUG->print(QObject::tr("RS_Commands:: command not found: %1").arg(full).toStdString().c_str());
        }
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
            if (RS_DIALOGFACTORY) {
                RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(c));
            }
            return RS2::ActionNone;
        }
    }
    //found
    if (RS_DIALOGFACTORY) {
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Accepted keycode: %1").arg(c));
    }
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
    if (RS_DIALOGFACTORY) {
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(cmd));
    }
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

    QString&& strl = str.toLower();
    QString&& cmdLower = cmd.toLower();
    auto it = instance()->cmdTranslation.find(cmdLower);
    if(it != instance()->cmdTranslation.end()){
        RS2::ActionType type0=instance()->cmdToAction(it->second);
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
