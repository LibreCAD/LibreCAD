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

#include "rs_commands.h"

#include "rs_dialogfactory.h"

RS_Commands* RS_Commands::uniqueInstance = NULL;


/**
 * Constructor. Initiates main command dictionary.
 * mainCommand keeps a map from translated commands to actionType
 * shortCommand keeps a list of translated short commands
 * cmdTranslation contains both ways of mapping between translated and English
 */
RS_Commands::RS_Commands() {
    // draw:
    cmdTranslation.insert("point", tr("point"));
    mainCommands.insert(tr("point"), RS2::ActionDrawPoint);
    cmdTranslation.insert("po", tr("po"));
    shortCommands.insert(tr("po"), RS2::ActionDrawPoint);

    cmdTranslation.insert("line", tr("line"));
    mainCommands.insert(tr("line"), RS2::ActionDrawLine);
    cmdTranslation.insert("li", tr("li"));
    shortCommands.insert(tr("li"), RS2::ActionDrawLine);
    shortCommands.insert(tr("l"), RS2::ActionDrawLine);

    cmdTranslation.insert("polyline", tr("polyline"));
    mainCommands.insert(tr("polyline"), RS2::ActionDrawPolyline);
    cmdTranslation.insert("pl", tr("pl"));
    shortCommands.insert(tr("pl"), RS2::ActionDrawPolyline);

    cmdTranslation.insert("offset", tr("offset"));
    mainCommands.insert(tr("offset"), RS2::ActionDrawLineParallel);
    shortCommands.insert(tr("o", "offset"), RS2::ActionDrawLineParallel);
    cmdTranslation.insert("parallel", tr("parallel"));
    mainCommands.insert(tr("parallel"), RS2::ActionDrawLineParallel);
    cmdTranslation.insert("pa", tr("pa"));
    shortCommands.insert(tr("pa", "parallel"), RS2::ActionDrawLineParallel);

    cmdTranslation.insert("arc", tr("arc"));
    mainCommands.insert(tr("arc"), RS2::ActionDrawArc3P);
    cmdTranslation.insert("ar", tr("ar"));
    mainCommands.insert(tr("ar"), RS2::ActionDrawArc3P);
    shortCommands.insert(tr("a"), RS2::ActionDrawArc3P);

    cmdTranslation.insert("circle", tr("circle"));
    mainCommands.insert(tr("circle"), RS2::ActionDrawCircle);
    cmdTranslation.insert("ci", tr("ci"));
    shortCommands.insert(tr("ci"), RS2::ActionDrawCircle);

    cmdTranslation.insert("rectangle", tr("rectangle"));
    mainCommands.insert(tr("rectangle"), RS2::ActionDrawLineRectangle);
    cmdTranslation.insert("rect", tr("rect"));
    shortCommands.insert(tr("rectang"), RS2::ActionDrawLineRectangle);
    shortCommands.insert(tr("rect"), RS2::ActionDrawLineRectangle);
    shortCommands.insert(tr("rec"), RS2::ActionDrawLineRectangle);

    cmdTranslation.insert("text", tr("text"));
    mainCommands.insert(tr("text"), RS2::ActionDrawText);

    // zoom:
    cmdTranslation.insert("regen", tr("regen"));
    cmdTranslation.insert("redraw", tr("redraw"));
    mainCommands.insert(tr("regen"), RS2::ActionZoomRedraw);
    mainCommands.insert(tr("redraw"), RS2::ActionZoomRedraw);
    shortCommands.insert(tr("rg", "zoom - redraw"), RS2::ActionZoomRedraw);

    cmdTranslation.insert("zr", tr("zr"));
    shortCommands.insert(tr("zr", "zoom - redraw"), RS2::ActionZoomRedraw);

    cmdTranslation.insert("zw", tr("zw"));
    mainCommands.insert(tr("zw", "zoom - window"), RS2::ActionZoomWindow);

    cmdTranslation.insert("za", tr("za"));
    mainCommands.insert(tr("za", "zoom - auto"), RS2::ActionZoomAuto);

    cmdTranslation.insert("zp", tr("zp"));
    mainCommands.insert(tr("zp", "zoom - pan"), RS2::ActionZoomPan);

    cmdTranslation.insert("zv", tr("zv"));
    mainCommands.insert(tr("zv", "zoom - previous"), RS2::ActionZoomPrevious);

    // edit:
    cmdTranslation.insert("kill", tr("kill"));
    mainCommands.insert(tr("kill"), RS2::ActionEditKillAllActions);
    cmdTranslation.insert("k", tr("k"));
    shortCommands.insert(tr("k"), RS2::ActionEditKillAllActions);

    cmdTranslation.insert("undo", tr("undo"));
    mainCommands.insert(tr("undo"), RS2::ActionEditUndo);
    cmdTranslation.insert("u", tr("u"));
    shortCommands.insert(tr("u", "undo"), RS2::ActionEditUndo);

    cmdTranslation.insert("redo", tr("redo"));
    mainCommands.insert(tr("redo"), RS2::ActionEditRedo);
    cmdTranslation.insert("r", tr("r"));
    shortCommands.insert(tr("r"), RS2::ActionEditRedo);

    // dimensions:
    cmdTranslation.insert("da", tr("da"));
    mainCommands.insert(tr("da", "dimension - aligned"), RS2::ActionDimAligned);
    shortCommands.insert(tr("da"), RS2::ActionDimAligned);

    cmdTranslation.insert("dh", tr("dh"));
    mainCommands.insert(tr("dh", "dimension - horizontal"), RS2::ActionDimLinearHor);
    shortCommands.insert(tr("dh"), RS2::ActionDimLinearHor);

    cmdTranslation.insert("dr", tr("dr"));
    mainCommands.insert(tr("dr", "dimension - linear"), RS2::ActionDimLinear);
    shortCommands.insert(tr("dr"), RS2::ActionDimLinear);

    cmdTranslation.insert("dv", tr("dv"));
    mainCommands.insert(tr("dv", "dimension - vertical"), RS2::ActionDimLinearVer);
    shortCommands.insert(tr("dv"), RS2::ActionDimLinearVer);

    cmdTranslation.insert("ld", tr("ld"));
    mainCommands.insert(tr("ld", "dimension - leader"), RS2::ActionDimLeader);
    shortCommands.insert(tr("ld"), RS2::ActionDimLeader);

    // tools:
    cmdTranslation.insert("dimregen", tr("dimregen"));
    mainCommands.insert(tr("dimregen"), RS2::ActionToolRegenerateDimensions);

    // modify:
    cmdTranslation.insert("tm", tr("tm"));
    mainCommands.insert(tr("tm", "modify - multi trim (extend)"), RS2::ActionModifyTrim2);
    shortCommands.insert(tr("tm"), RS2::ActionModifyTrim2);

    cmdTranslation.insert("xt", tr("xt"));
    mainCommands.insert(tr("xt", "modify - trim (extend)"), RS2::ActionModifyTrim);
    shortCommands.insert(tr("xt"), RS2::ActionModifyTrim);

    cmdTranslation.insert("rm", tr("rm"));
    mainCommands.insert(tr("rm", "modify - trim"), RS2::ActionModifyTrim);
    shortCommands.insert(tr("rm"), RS2::ActionModifyTrim);

    cmdTranslation.insert("mv", tr("mv"));
    mainCommands.insert(tr("mv", "modify - move"), RS2::ActionModifyMove);
    shortCommands.insert(tr("mv"), RS2::ActionModifyMove);

    cmdTranslation.insert("ch", tr("ch"));
    mainCommands.insert(tr("ch", "modify - bevel (chamfer)"), RS2::ActionModifyBevel);
    shortCommands.insert(tr("ch"), RS2::ActionModifyBevel);

    cmdTranslation.insert("mi", tr("mi"));
    mainCommands.insert(tr("mi", "modify - mirror"), RS2::ActionModifyMirror);
    shortCommands.insert(tr("mi"), RS2::ActionModifyMirror);

    cmdTranslation.insert("ro", tr("ro"));
    mainCommands.insert(tr("ro", "modify - rotate"), RS2::ActionModifyRotate);
    shortCommands.insert(tr("ro"), RS2::ActionModifyRotate);

    cmdTranslation.insert("sz", tr("sz"));
    mainCommands.insert(tr("sz", "modify - scale"), RS2::ActionModifyMove);
    shortCommands.insert(tr("sz"), RS2::ActionModifyMove);

    cmdTranslation.insert("ss", tr("ss"));
    mainCommands.insert(tr("ss", "modify - stretch"), RS2::ActionModifyStretch);
    shortCommands.insert(tr("ss"), RS2::ActionModifyStretch);

    cmdTranslation.insert("er", tr("er"));
    mainCommands.insert(tr("er", "modify - delete (erase)"), RS2::ActionModifyDelete);
    shortCommands.insert(tr("er"), RS2::ActionModifyDelete);

    cmdTranslation.insert("oo", tr("oo"));
    mainCommands.insert(tr("oo", "modify - undo (oops)"), RS2::ActionEditUndo);
    shortCommands.insert(tr("oo"), RS2::ActionEditUndo);

    cmdTranslation.insert("uu", tr("uu"));
    mainCommands.insert(tr("uu", "modify - redo"), RS2::ActionEditRedo);
    shortCommands.insert(tr("uu"), RS2::ActionEditRedo);

    cmdTranslation.insert("xp", tr("xp"));
    mainCommands.insert(tr("xp", "modify - explode"), RS2::ActionBlocksExplode);
    shortCommands.insert(tr("xp"), RS2::ActionBlocksExplode);

    // snap:
    cmdTranslation.insert("os", tr("os"));
    mainCommands.insert(tr("os", "snap - none"), RS2::ActionSnapFree);
    shortCommands.insert(tr("os"), RS2::ActionSnapFree);

    cmdTranslation.insert("sc", tr("sc"));
    mainCommands.insert(tr("sc", "snap - center"), RS2::ActionSnapCenter);
    shortCommands.insert(tr("sc"), RS2::ActionSnapCenter);

    cmdTranslation.insert("sd", tr("sd"));
    mainCommands.insert(tr("sd", "snap - distance"), RS2::ActionSnapDist);
    shortCommands.insert(tr("sd"), RS2::ActionSnapDist);

    cmdTranslation.insert("se", tr("se"));
    mainCommands.insert(tr("se", "snap - end"), RS2::ActionSnapEndpoint);
    shortCommands.insert(tr("se"), RS2::ActionSnapEndpoint);

    cmdTranslation.insert("sf", tr("sf"));
    mainCommands.insert(tr("sf", "snap - free"), RS2::ActionSnapFree);
    shortCommands.insert(tr("sf"), RS2::ActionSnapFree);

    cmdTranslation.insert("sg", tr("sg"));
    mainCommands.insert(tr("sg", "snap - grid"), RS2::ActionSnapGrid);
    shortCommands.insert(tr("sg"), RS2::ActionSnapGrid);

    cmdTranslation.insert("si", tr("si"));
    mainCommands.insert(tr("si", "snap - intersection"), RS2::ActionSnapIntersection);
    shortCommands.insert(tr("si"), RS2::ActionSnapIntersection);

    cmdTranslation.insert("sm", tr("sm"));
    mainCommands.insert(tr("sm", "snap - middle"), RS2::ActionSnapMiddle);
    shortCommands.insert(tr("sm"), RS2::ActionSnapMiddle);

    cmdTranslation.insert("sn", tr("sn"));
    mainCommands.insert(tr("sn", "snap - nearest"), RS2::ActionSnapOnEntity);
    shortCommands.insert(tr("sn"), RS2::ActionSnapOnEntity);

    cmdTranslation.insert("np", tr("np"));
    mainCommands.insert(tr("np", "snap - nearest point"), RS2::ActionSnapOnEntity);
    shortCommands.insert(tr("np"), RS2::ActionSnapOnEntity);

    // selection:
    cmdTranslation.insert("sa", tr("sa"));
    mainCommands.insert(tr("sa", "Select all"), RS2::ActionSelectAll);
    shortCommands.insert(tr("sa"), RS2::ActionSelectAll);

    cmdTranslation.insert("tn", tr("tn"));
    mainCommands.insert(tr("tn", "Deselect all"), RS2::ActionDeselectAll);
    shortCommands.insert(tr("tn"), RS2::ActionDeselectAll);

    cmdTranslation.insert( "angle", tr("angle"));
    cmdTranslation.insert( "close", tr("close"));
    cmdTranslation.insert( "chord length", tr("chord length"));
    cmdTranslation.insert( "columns", tr("columns"));
    cmdTranslation.insert( "columnspacing", tr("columnspacing"));
    cmdTranslation.insert( "factor", tr("factor"));
    cmdTranslation.insert( "length", tr("length"));
    cmdTranslation.insert( "length1", tr("length1"));
    cmdTranslation.insert( "length2", tr("length2"));
    cmdTranslation.insert( "number", tr("number"));
    cmdTranslation.insert( "radius", tr("radius"));
    cmdTranslation.insert( "rows", tr("rows"));
    cmdTranslation.insert( "rowspacing", tr("rowspacing"));
    cmdTranslation.insert( "through", tr("through"));
    cmdTranslation.insert( "trim", tr("trim"));

/** following are reversed translation, i.e., from translated to english **/
    //not used as command keywords
// used in function, checkCommand()
    cmdTranslation.insert(tr("angle"), "angle");
    cmdTranslation.insert(tr("ang", "angle"), "angle");
    cmdTranslation.insert(tr("a", "angle"), "angle");

    cmdTranslation.insert(tr("center"), "center");
    cmdTranslation.insert(tr("cen", "center"), "center");
    cmdTranslation.insert(tr("c", "center"), "center");

    cmdTranslation.insert(tr("chord length"), "chord length");
    cmdTranslation.insert(tr("length", "chord length"), "chord length");
    cmdTranslation.insert(tr("l", "chord length"), "chord length");

    cmdTranslation.insert(tr("close"), "close");
    cmdTranslation.insert(tr("c", "close"), "close");

    cmdTranslation.insert(tr("columns"), "columns");
    cmdTranslation.insert(tr("cols", "columns"), "columns");
    cmdTranslation.insert(tr("c", "columns"), "columns");

    cmdTranslation.insert(tr("columnspacing", "columnspacing for inserts"), "columnspacing");
    cmdTranslation.insert(tr("colspacing", "columnspacing for inserts"), "columnspacing");
    cmdTranslation.insert(tr("cs", "columnspacing for inserts"), "columnspacing");

    cmdTranslation.insert(tr("factor"), "factor");
    cmdTranslation.insert(tr("fact", "factor"), "factor");
    cmdTranslation.insert(tr("f", "factor"), "factor");

    cmdTranslation.insert(tr("help"), "help");
    cmdTranslation.insert(tr("?", "help"), "help");

    cmdTranslation.insert(tr("length","length"), "length");
    cmdTranslation.insert(tr("len","length"), "length");
    cmdTranslation.insert(tr("l","length"), "length");

    cmdTranslation.insert(tr("length1","length1"), "length1");
    cmdTranslation.insert(tr("len1","length1"), "length1");
    cmdTranslation.insert(tr("l1","length1"), "length1");

    cmdTranslation.insert(tr("length2","length2"), "length2");
    cmdTranslation.insert(tr("len2","length2"), "length2");
    cmdTranslation.insert(tr("l2","length2"), "length2");

    cmdTranslation.insert(tr("number","number"), "number");
    cmdTranslation.insert(tr("num","number"), "number");
    cmdTranslation.insert(tr("n","number"), "number");

    cmdTranslation.insert(tr("radius"), "radius");
    cmdTranslation.insert(tr("r","radius"), "radius");

    cmdTranslation.insert(tr("reversed","reversed"), "reversed");
    cmdTranslation.insert(tr("rev","reversed"), "reversed");
    cmdTranslation.insert(tr("r","reversed"), "reversed");

    cmdTranslation.insert(tr("row"), "row");
    cmdTranslation.insert(tr("r","row"), "row");

    cmdTranslation.insert(tr("rowspacing", "rowspacing for inserts"), "rowspacing");
    cmdTranslation.insert(tr("rs","rowspacing for inserts"), "rowspacing");

    cmdTranslation.insert(tr("text"), "text");
    cmdTranslation.insert(tr("t","text"), "text");

    cmdTranslation.insert(tr("through"), "through");
    cmdTranslation.insert(tr("t","through"), "through");

    cmdTranslation.insert(tr("undo"), "undo");
    cmdTranslation.insert(tr("u","undo"), "undo");

    cmdTranslation.insert(tr("redo"), "redo");
    cmdTranslation.insert(tr("r","redo"), "redo");

    cmdTranslation.insert(tr("back"), "back");
    cmdTranslation.insert(tr("b","back"), "back");
}



/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
QStringList RS_Commands::complete(const QString& cmd) {
    QStringList ret;
    QHash<QString, RS2::ActionType>::const_iterator it = mainCommands.constBegin();
    while (it != mainCommands.constEnd()) {
        if (it.key().startsWith(cmd)) {
            ret << it.key();
        }
        ++it;
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
    QString c = cmd.toLower();
    QString full = cmd;          // full command defaults to given command
    RS2::ActionType ret = RS2::ActionNone;

        // find command:
//	RS2::ActionType* retPtr = mainCommands.value(cmd);
        if ( mainCommands.contains(cmd) ) {
                ret = mainCommands.value(cmd);
        } else
        if ( shortCommands.contains(cmd) ) {
                ret = shortCommands.value(cmd);
        }

        // find full command to confirm to user:
        if(verbose){
            QHash<QString, RS2::ActionType>::const_iterator it = mainCommands.constBegin();
            while (it != mainCommands.constEnd()) {
                if (it.value()==ret) {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DEBUG->print("RS_Commands::cmdToAction: commandMessage");
                        //RS_DIALOGFACTORY->commandMessage(tr("Command: %1")
                        //	.arg(full));
                        RS_DIALOGFACTORY->commandMessage(tr("Command: %1").arg(full));
                        //                                        RS_DialogFactory::instance()->commandMessage( tr("Command: %1").arg(full));
                        RS_DEBUG->print("RS_Commands::cmdToAction: "
                                        "commandMessage: ok");
                        //}
                    }
                    else {
                        RS_DEBUG->print("RS_Commands::cmdToAction: dialog "
                                        "factory instance is NULL");
                    }
                    break;
                }
                ++it;
            }
        }
        return ret;
}



/**
 * Gets the action for the given keycode. A keycode is a sequence
 * of key-strokes that is entered like hotkeys.
 */
RS2::ActionType RS_Commands::keycodeToAction(const QString& code) {
    QString c = code.toLower();
    QMultiHash<QString, RS2::ActionType>::iterator it = shortCommands.find(c);
    if( it == shortCommands.end() ) {

        //not found, searching for main commands
        it = mainCommands.find(c);
        if( it == mainCommands.end() ){
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->commandMessage(tr("Command not found: %1").arg(c));
            }
            return RS2::ActionNone;
        }
    }
    //found
    //fixme, need to handle multiple hits
    return it.value();
}


/**
 * @return translated command for the given English command.
 */
QString RS_Commands::command(const QString& cmd) {
    auto it= instance()->cmdTranslation.find(cmd);
    if(it != instance()->cmdTranslation.end()){
        return instance()->cmdTranslation[cmd];
    }
    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->commandMessage(tr("Command not found: %1").arg(cmd));
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
        RS2::ActionType type0=instance()->cmdToAction(it.value());
        if( type0  != RS2::ActionNone ) {
            return  type0 ==instance()->cmdToAction(strl);
        }
    }

    it =  instance()->cmdTranslation.find(strl);
    if(it !=  instance()->cmdTranslation.end()) return it.value() == cmdLower;
    return false;

//    if (cmd=="angle") {
//        if (strl==tr("angle") || strl==tr("ang", "angle") ||
//                strl==tr("a", "angle")) {
//            return true;
//        }
//    } else if (cmd=="center") {
//        if (strl==tr("center") || strl==tr("cen", "center") ||
//                strl==tr("c", "center")) {
//            return true;
//        }
//    } else if (cmd=="chord length") {
//        if (strl==tr("length", "chord length") ||
//                strl==tr("l", "chord length")) {
//            return true;
//        }
//    } else if (cmd=="close") {
//        if (strl==tr("close") ||
//                strl==tr("c", "close")) {
//            return true;
//        }
//    } else if (cmd=="columns") {
//        if (strl==tr("columns") || strl==tr("cols", "columns") ||
//                strl==tr("c", "columns")) {
//            return true;
//        }
//    } else if (cmd=="columnspacing") {
//        if (strl==tr("columnspacing", "columnspacing for inserts") ||
//                strl==tr("colspacing", "columnspacing for inserts") ||
//                strl==tr("cs", "columnspacing for inserts")) {
//            return true;
//        }
//    } else if (cmd=="factor") {
//        if (strl==tr("factor") || strl==tr("fact", "factor") ||
//                strl==tr("f", "factor")) {
//            return true;
//        }
//    } else if (cmd=="help") {
//        if (strl==tr("help") || strl==tr("?", "help")) {
//            return true;
//        }
//    } else if (cmd=="length") {
//        if (strl==tr("length", "length") ||
//                strl==tr("len", "length") ||
//                strl==tr("l", "length")) {
//            return true;
//        }
//    } else if (cmd=="length1") {
//        if (strl==tr("length1", "length1") ||
//                strl==tr("len1", "length1") ||
//                strl==tr("l1", "length1")) {
//            return true;
//        }
//    } else if (cmd=="length2") {
//        if (strl==tr("length2", "length2") ||
//                strl==tr("len2", "length2") ||
//                strl==tr("l2", "length2")) {
//            return true;
//        }
//    } else if (cmd=="number") {
//        if (strl==tr("number") ||
//                strl==tr("num", "number") ||
//                strl==tr("n", "number")) {
//            return true;
//        }
//    } else if (cmd=="radius") {
//        if (strl==tr("radius") ||
//                strl==tr("r", "radius")) {
//            return true;
//        }
//    } else if (cmd=="reversed") {
//        if (strl==tr("reversed", "reversed arc") ||
//                strl==tr("rev", "reversed arc") ||
//                strl==tr("r", "reversed arc")) {
//            return true;
//        }
//    } else if (cmd=="rows") {
//        if (strl==tr("rows") || strl==tr("r", "rows")) {
//            return true;
//        }
//    } else if (cmd=="rowspacing") {
//        if (strl==tr("rowspacing", "rowspacing for inserts") ||
//                strl==tr("rs", "rowspacing for inserts")) {
//            return true;
//        }
//    } else if (cmd=="text") {
//        if (strl==tr("text") ||
//                strl==tr("t", "text")) {
//            return true;
//        }
//    } else if (cmd=="through") {
//        if (strl==tr("through") ||
//                strl==tr("t", "through")) {
//            return true;
//        }
//    } else if (cmd=="undo") {
//        if (strl==tr("undo") ||
//                strl==tr("u", "undo")) {
//            return true;
//        }
//    } else if (cmd=="redo") {
//        if (strl==tr("redo") ||
//                strl==tr("r", "redo")) {
//            return true;
//        }
//    } else if (cmd=="back") {
//        if (strl==tr("back") ||
//                strl==tr("b", "back")) {
//            return true;
//        }
//    }

    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
QString RS_Commands::msgAvailableCommands() {
    return tr("Available commands:");
}


// EOF
