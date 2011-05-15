/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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

#include <iostream>

#include "rs_translator.h"
#include "rs_dialogfactory.h"

RS_Commands* RS_Commands::uniqueInstance = NULL;


/**
 * Constructor. Initiates main command dictionary.
 */
RS_Commands::RS_Commands() {
	mainCommands.setAutoDelete(true);
	shortCommands.setAutoDelete(true);

	// draw:
	mainCommands.insert(tr("point"), new RS2::ActionType(RS2::ActionDrawPoint));
	shortCommands.insert(tr("po"), new RS2::ActionType(RS2::ActionDrawPoint));
	
        mainCommands.insert(tr("line"), new RS2::ActionType(RS2::ActionDrawLine));
	shortCommands.insert(tr("ln"), new RS2::ActionType(RS2::ActionDrawLine));
	shortCommands.insert(tr("l"), new RS2::ActionType(RS2::ActionDrawLine));
	
	mainCommands.insert(tr("polyline"), new RS2::ActionType(RS2::ActionDrawPolyline));

    mainCommands.insert(tr("offset"), new RS2::ActionType(RS2::ActionDrawLineParallel));
	shortCommands.insert(tr("o", "offset"), new RS2::ActionType(RS2::ActionDrawLineParallel));
    mainCommands.insert(tr("parallel"), new RS2::ActionType(RS2::ActionDrawLineParallel));
	shortCommands.insert(tr("par", "parallel"), new RS2::ActionType(RS2::ActionDrawLineParallel));

	mainCommands.insert(tr("arc"), new RS2::ActionType(RS2::ActionDrawArc3P));
	shortCommands.insert(tr("a"), new RS2::ActionType(RS2::ActionDrawArc3P));
	
	mainCommands.insert(tr("circle"), new RS2::ActionType(RS2::ActionDrawCircle));
	shortCommands.insert(tr("ci"), new RS2::ActionType(RS2::ActionDrawCircle));
	
	mainCommands.insert(tr("rectangle"), new RS2::ActionType(RS2::ActionDrawLineRectangle));
	shortCommands.insert(tr("rec"), new RS2::ActionType(RS2::ActionDrawLineRectangle));
	shortCommands.insert(tr("rectang"), new RS2::ActionType(RS2::ActionDrawLineRectangle));
	
	mainCommands.insert(tr("polyline"), new RS2::ActionType(RS2::ActionDrawPolyline));
	
	mainCommands.insert(tr("text"), new RS2::ActionType(RS2::ActionDrawText));

	// zoom:
	mainCommands.insert(tr("regen"), new RS2::ActionType(RS2::ActionZoomRedraw));
	shortCommands.insert(tr("rg", "zoom - redraw"), new RS2::ActionType(RS2::ActionZoomRedraw));
	shortCommands.insert(tr("zr", "zoom - redraw"), new RS2::ActionType(RS2::ActionZoomRedraw));
	
	mainCommands.insert(tr("zw", "zoom - window"), new RS2::ActionType(RS2::ActionZoomWindow));
	
	mainCommands.insert(tr("za", "zoom - auto"), new RS2::ActionType(RS2::ActionZoomAuto));
	
	mainCommands.insert(tr("zp", "zoom - pan"), new RS2::ActionType(RS2::ActionZoomPan));
	
	mainCommands.insert(tr("zv", "zoom - previous"), new RS2::ActionType(RS2::ActionZoomPrevious));

	// edit:
        mainCommands.insert(tr("kill"), new RS2::ActionType(RS2::ActionEditKillAllActions));
        shortCommands.insert(tr("k"), new RS2::ActionType(RS2::ActionEditKillAllActions));

        mainCommands.insert(tr("undo"), new RS2::ActionType(RS2::ActionEditUndo));
	shortCommands.insert(tr("u", "undo"), new RS2::ActionType(RS2::ActionEditUndo));
	
        mainCommands.insert(tr("redo"), new RS2::ActionType(RS2::ActionEditRedo));
        shortCommands.insert(tr("r"), new RS2::ActionType(RS2::ActionEditRedo));

    // dimensions:
    mainCommands.insert(tr("da", "dimension - aligned"), new RS2::ActionType(RS2::ActionDimAligned));
    shortCommands.insert(tr("da"), new RS2::ActionType(RS2::ActionDimAligned));
    
    mainCommands.insert(tr("dh", "dimension - horizontal"), new RS2::ActionType(RS2::ActionDimLinearHor));
    shortCommands.insert(tr("dh"), new RS2::ActionType(RS2::ActionDimLinearHor));
    
    mainCommands.insert(tr("dr", "dimension - linear"), new RS2::ActionType(RS2::ActionDimLinear));
    shortCommands.insert(tr("dr"), new RS2::ActionType(RS2::ActionDimLinear));

    mainCommands.insert(tr("dv", "dimension - vertical"), new RS2::ActionType(RS2::ActionDimLinearVer));
    shortCommands.insert(tr("dv"), new RS2::ActionType(RS2::ActionDimLinearVer));

    mainCommands.insert(tr("ld", "dimension - leader"), new RS2::ActionType(RS2::ActionDimLeader));
    shortCommands.insert(tr("ld"), new RS2::ActionType(RS2::ActionDimLeader));

	// tools:
	mainCommands.insert(tr("dimregen"), new RS2::ActionType(RS2::ActionToolRegenerateDimensions));

    // modify:
    mainCommands.insert(tr("tm", "modify - multi trim (extend)"), new RS2::ActionType(RS2::ActionModifyTrim2));
    shortCommands.insert(tr("tm"), new RS2::ActionType(RS2::ActionModifyTrim2));

    mainCommands.insert(tr("xt", "modify - trim (extend)"), new RS2::ActionType(RS2::ActionModifyTrim));
    shortCommands.insert(tr("xt"), new RS2::ActionType(RS2::ActionModifyTrim));

    mainCommands.insert(tr("rm", "modify - trim"), new RS2::ActionType(RS2::ActionModifyTrim));
    shortCommands.insert(tr("rm"), new RS2::ActionType(RS2::ActionModifyTrim));

    mainCommands.insert(tr("mv", "modify - move"), new RS2::ActionType(RS2::ActionModifyMove));
    shortCommands.insert(tr("mv"), new RS2::ActionType(RS2::ActionModifyMove));

    mainCommands.insert(tr("ch", "modify - bevel (chamfer)"), new RS2::ActionType(RS2::ActionModifyBevel));
    shortCommands.insert(tr("ch"), new RS2::ActionType(RS2::ActionModifyBevel));

    mainCommands.insert(tr("mi", "modify - mirror"), new RS2::ActionType(RS2::ActionModifyMirror));
    shortCommands.insert(tr("mi"), new RS2::ActionType(RS2::ActionModifyMirror));

    mainCommands.insert(tr("ro", "modify - rotate"), new RS2::ActionType(RS2::ActionModifyRotate));
    shortCommands.insert(tr("ro"), new RS2::ActionType(RS2::ActionModifyRotate));

    mainCommands.insert(tr("sz", "modify - scale"), new RS2::ActionType(RS2::ActionModifyMove));
    shortCommands.insert(tr("sz"), new RS2::ActionType(RS2::ActionModifyMove));

    mainCommands.insert(tr("ss", "modify - stretch"), new RS2::ActionType(RS2::ActionModifyStretch));
    shortCommands.insert(tr("ss"), new RS2::ActionType(RS2::ActionModifyStretch));

    mainCommands.insert(tr("er", "modify - delete (erase)"), new RS2::ActionType(RS2::ActionModifyDelete));
    shortCommands.insert(tr("er"), new RS2::ActionType(RS2::ActionModifyDelete));

    mainCommands.insert(tr("oo", "modify - undo (oops)"), new RS2::ActionType(RS2::ActionEditUndo));
    shortCommands.insert(tr("oo"), new RS2::ActionType(RS2::ActionEditUndo));

    mainCommands.insert(tr("uu", "modify - redo"), new RS2::ActionType(RS2::ActionEditRedo));
    shortCommands.insert(tr("uu"), new RS2::ActionType(RS2::ActionEditRedo));

    mainCommands.insert(tr("xp", "modify - explode"), new RS2::ActionType(RS2::ActionBlocksExplode));
    shortCommands.insert(tr("xp"), new RS2::ActionType(RS2::ActionBlocksExplode));

    // snap:
    mainCommands.insert(tr("os", "snap - none"), new RS2::ActionType(RS2::ActionSnapFree));
    shortCommands.insert(tr("os"), new RS2::ActionType(RS2::ActionSnapFree));

    mainCommands.insert(tr("sg", "snap - grid"), new RS2::ActionType(RS2::ActionSnapGrid));
    shortCommands.insert(tr("sg"), new RS2::ActionType(RS2::ActionSnapGrid));

    mainCommands.insert(tr("se", "snap - end"), new RS2::ActionType(RS2::ActionSnapEndpoint));
    shortCommands.insert(tr("se"), new RS2::ActionType(RS2::ActionSnapEndpoint));

    mainCommands.insert(tr("si", "snap - intersection"), new RS2::ActionType(RS2::ActionSnapIntersection));
    shortCommands.insert(tr("si"), new RS2::ActionType(RS2::ActionSnapIntersection));

    mainCommands.insert(tr("sn", "snap - center"), new RS2::ActionType(RS2::ActionSnapCenter));
    shortCommands.insert(tr("sn"), new RS2::ActionType(RS2::ActionSnapCenter));

    mainCommands.insert(tr("sm", "snap - middle"), new RS2::ActionType(RS2::ActionSnapMiddle));
    shortCommands.insert(tr("sm"), new RS2::ActionType(RS2::ActionSnapMiddle));

    mainCommands.insert(tr("sn", "snap - nearest"), new RS2::ActionType(RS2::ActionSnapMiddle));
    shortCommands.insert(tr("sn"), new RS2::ActionType(RS2::ActionSnapMiddle));

    mainCommands.insert(tr("np", "snap - nearest point"), new RS2::ActionType(RS2::ActionSnapOnEntity));
    shortCommands.insert(tr("np"), new RS2::ActionType(RS2::ActionSnapOnEntity));

    // selection:
    mainCommands.insert(tr("tn", "Deselect all"), new RS2::ActionType(RS2::ActionDeselectAll));
    shortCommands.insert(tr("tn"), new RS2::ActionType(RS2::ActionDeselectAll));
}

	

/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
RS_StringList RS_Commands::complete(const RS_String& cmd) {
	RS_DictIterator<RS2::ActionType> it(mainCommands);
	RS_StringList ret;
    for (; it.current(); ++it) {
        //cout << it.currentKey() << ": " << it.current()->text() << endl;
		if (it.currentKey().startsWith(cmd)) {
			ret << it.currentKey();
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
RS2::ActionType RS_Commands::cmdToAction(const RS_String& cmd) {
    RS_String c = cmd.lower();
    RS_String full = cmd;          // full command defaults to given command
    RS2::ActionType ret = RS2::ActionNone;

	// find command:
	RS2::ActionType* retPtr = mainCommands[cmd];
	if (retPtr!=NULL) {
		ret = *retPtr;
	}
	else {
		retPtr = shortCommands[cmd];
		if (retPtr!=NULL) {
			ret = *retPtr;
		}
	}

	// find full command to confirm to user:
	RS_DictIterator<RS2::ActionType> it(mainCommands);
    for (; it.current(); ++it) {
		if (*it.current()==ret) {
			if (RS_DialogFactory::instance()!=NULL) {
				//if (RS_DIALOGFACTORY!=NULL) {
					RS_DEBUG->print("RS_Commands::cmdToAction: "
						"commandMessage");
					//RS_DIALOGFACTORY->commandMessage(tr("Command: %1")
					//	.arg(full));
					RS_DialogFactory::instance()->commandMessage(
						tr("Command: %1").arg(full));
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
	}
	
	return ret;
}



/**
 * Gets the action for the given keycode. A keycode is a sequence
 * of key-strokes that is entered like hotkeys.
 */
RS2::ActionType RS_Commands::keycodeToAction(const RS_String& code) {
    RS_String c = code.lower();
    RS2::ActionType ret = RS2::ActionNone;

    // draw:
    if (c==tr("po", "point")) {
        ret = RS2::ActionDrawPoint;
    } else if (c==tr("li", "line")) {
        ret = RS2::ActionDrawLine;
    } else if (c==tr("pa", "parallel")) {
        ret = RS2::ActionDrawLine;
    } else if (c==tr("re", "rectangle")) {
        ret = RS2::ActionDrawLineRectangle;
    } else if (c==tr("rp", "regular polygon")) {
        ret = RS2::ActionDrawLinePolygon;
    } else if (c==tr("ci", "circle")) {
        ret = RS2::ActionDrawCircle;
    } else if (c==tr("c2", "2 point circle")) {
        ret = RS2::ActionDrawCircle2P;
    } else if (c==tr("c3", "3 point circle")) {
        ret = RS2::ActionDrawCircle3P;
    } else if (c==tr("ar", "arc")) {
        ret = RS2::ActionDrawArc;
    } else if (c==tr("a3", "3 point arc")) {
        ret = RS2::ActionDrawArc3P;
    } else if (c==tr("ep", "ellipse")) {
        ret = RS2::ActionDrawEllipseAxis;
    } else if (c==tr("tx", "text") || c==tr("mt", "text")) {
        ret = RS2::ActionDrawText;
    }

	// dimensions:
    else if (c==tr("da", "dimension - aligned")) {
        ret = RS2::ActionDimAligned;
    } else if (c==tr("dh", "dimension - horizontal")) {
        ret = RS2::ActionDimLinearHor;
    } else if (c==tr("dv", "dimension - vertical")) {
        ret = RS2::ActionDimLinearVer;
    } else if (c==tr("dr", "dimension - linear")) {
        ret = RS2::ActionDimLinear;
    } else if (c==tr("ld", "dimension - leader")) {
        ret = RS2::ActionDimLeader;
	}

    // zoom:
    else if (c==tr("rd", "redraw")) {
        ret = RS2::ActionZoomRedraw;
    } else if (c==tr("zw", "zoom - window")) {
        ret = RS2::ActionZoomWindow;
    } else if (c==tr("za", "zoom - auto")) {
        ret = RS2::ActionZoomAuto;
    } else if (c==tr("zi", "zoom - in")) {
        ret = RS2::ActionZoomIn;
    } else if (c==tr("zo", "zoom - out")) {
        ret = RS2::ActionZoomOut;
    } else if (c==tr("zp", "zoom - pan")) {
        ret = RS2::ActionZoomPan;
    } else if (c==tr("zv", "zoom - previous")) {
        ret = RS2::ActionZoomPrevious;
    }

	// snap:
	else if (c==tr("os", "snap - none")) {
		ret = RS2::ActionSnapFree;
	} else if (c==tr("sg", "snap - grid")) {
		ret = RS2::ActionSnapGrid;
	} else if (c==tr("se", "snap - end")) {
		ret = RS2::ActionSnapEndpoint;
	} else if (c==tr("si", "snap - intersection")) {
		ret = RS2::ActionSnapIntersection;
	} else if (c==tr("sn", "snap - center")) {
		ret = RS2::ActionSnapCenter;
	} else if (c==tr("sm", "snap - middle")) {
		ret = RS2::ActionSnapMiddle;
	} else if (c==tr("sn", "snap - nearest")) {
		ret = RS2::ActionSnapMiddle;
	} else if (c==tr("np", "snap - nearest point")) {
		ret = RS2::ActionSnapOnEntity;
	}

	// layer:
	else if (c==tr("fr*", "layers - freeze all")) {
		ret = RS2::ActionLayersFreezeAll;
	} else if (c==tr("th*", "layers - defreeze all")) {
		ret = RS2::ActionLayersDefreezeAll;
	}

	// selection:
	else if (c==tr("tn", "Deselect all")) {
		ret = RS2::ActionDeselectAll;
	}

	// modify:
	else if (c==tr("ch", "modify - bevel (chamfer)")) {
		ret = RS2::ActionModifyBevel;
	} else if (c==tr("tm", "modify - multi trim (extend)")) {
		ret = RS2::ActionModifyTrim2;
	} else if (c==tr("xt", "modify - trim (extend)")) {
		ret = RS2::ActionModifyTrim;
	} else if (c==tr("rm", "modify - trim")) {
		ret = RS2::ActionModifyTrim;
	} else if (c==tr("mv", "modify - move")) {
		ret = RS2::ActionModifyMove;
	} else if (c==tr("mi", "modify - mirror")) {
		ret = RS2::ActionModifyMirror;
	} else if (c==tr("ro", "modify - rotate")) {
		ret = RS2::ActionModifyRotate;
	} else if (c==tr("sz", "modify - scale")) {
		ret = RS2::ActionModifyMove;
	} else if (c==tr("ss", "modify - stretch")) {
		ret = RS2::ActionModifyStretch;
	} else if (c==tr("er", "modify - delete (erase)")) {
		ret = RS2::ActionModifyDelete;
	} else if (c==tr("oo", "modify - undo (oops)")) {
		ret = RS2::ActionEditUndo;
	} else if (c==tr("uu", "modify - redo")) {
		ret = RS2::ActionEditRedo;
	} else if (c==tr("xp", "modify - explode") || 
			c==tr("ex", "modify - explode")) {
		ret = RS2::ActionBlocksExplode;
	}

    return ret;
}


/**
 * @return translated command for the given English command.
 */
RS_String RS_Commands::command(const RS_String& cmd) {
    if (cmd=="angle") {
        return tr("angle");
    } else if (cmd=="close") {
        return tr("close");
    } else if (cmd=="chord length") {
        return tr("chord length");
    } else if (cmd=="columns") {
        return tr("columns");
    } else if (cmd=="columnspacing") {
        return tr("columnspacing");
    } else if (cmd=="factor") {
        return tr("factor");
    } else if (cmd=="length") {
        return tr("length");
    } else if (cmd=="length1") {
        return tr("length1");
    } else if (cmd=="length2") {
        return tr("length2");
    } else if (cmd=="number") {
        return tr("number");
    } else if (cmd=="radius") {
        return tr("radius");
    } else if (cmd=="rows") {
        return tr("rows");
    } else if (cmd=="rowspacing") {
        return tr("rowspacing");
    } else if (cmd=="text") {
        return tr("text");
    } else if (cmd=="through") {
        return tr("through");
    } else if (cmd=="trim") {
        return tr("trim");
    } else if (cmd=="undo") {
        return tr("undo");
    } else if (cmd=="back") {
        return tr("back");
    }

    RS_DEBUG->print(RS_Debug::D_WARNING,
		"RS_Commands::command: command '%s' unknown", cmd.latin1());
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
bool RS_Commands::checkCommand(const RS_String& cmd, const RS_String& str,
                               RS2::ActionType /*action*/) {

    RS_String strl = str.lower();

    if (cmd=="angle") {
        if (strl==tr("angle") || strl==tr("ang", "angle") ||
                strl==tr("a", "angle")) {
            return true;
        }
    } else if (cmd=="center") {
        if (strl==tr("center") || strl==tr("cen", "center") ||
                strl==tr("c", "center")) {
            return true;
        }
    } else if (cmd=="chord length") {
        if (strl==tr("length", "chord length") ||
                strl==tr("l", "chord length")) {
            return true;
        }
    } else if (cmd=="close") {
        if (strl==tr("close") ||
                strl==tr("c", "close")) {
            return true;
        }
    } else if (cmd=="columns") {
        if (strl==tr("columns") || strl==tr("cols", "columns") ||
                strl==tr("c", "columns")) {
            return true;
        }
    } else if (cmd=="columnspacing") {
        if (strl==tr("columnspacing", "columnspacing for inserts") ||
                strl==tr("colspacing", "columnspacing for inserts") ||
                strl==tr("cs", "columnspacing for inserts")) {
            return true;
        }
    } else if (cmd=="factor") {
        if (strl==tr("factor") || strl==tr("fact", "factor") ||
                strl==tr("f", "factor")) {
            return true;
        }
    } else if (cmd=="help") {
        if (strl==tr("help") || strl==tr("?", "help")) {
            return true;
        }
    } else if (cmd=="length") {
        if (strl==tr("length", "length") ||
                strl==tr("len", "length") ||
                strl==tr("l", "length")) {
            return true;
        }
    } else if (cmd=="length1") {
        if (strl==tr("length1", "length1") ||
                strl==tr("len1", "length1") ||
                strl==tr("l1", "length1")) {
            return true;
        }
    } else if (cmd=="length2") {
        if (strl==tr("length2", "length2") ||
                strl==tr("len2", "length2") ||
                strl==tr("l2", "length2")) {
            return true;
        }
    } else if (cmd=="number") {
        if (strl==tr("number") ||
                strl==tr("num", "number") ||
                strl==tr("n", "number")) {
            return true;
        }
    } else if (cmd=="radius") {
        if (strl==tr("radius") ||
                strl==tr("r", "radius")) {
            return true;
        }
    } else if (cmd=="reversed") {
        if (strl==tr("reversed", "reversed arc") ||
                strl==tr("rev", "reversed arc") ||
                strl==tr("r", "reversed arc")) {
            return true;
        }
    } else if (cmd=="rows") {
        if (strl==tr("rows") || strl==tr("r", "rows")) {
            return true;
        }
    } else if (cmd=="rowspacing") {
        if (strl==tr("rowspacing", "rowspacing for inserts") ||
                strl==tr("rs", "rowspacing for inserts")) {
            return true;
        }
    } else if (cmd=="text") {
        if (strl==tr("text") ||
                strl==tr("t", "text")) {
            return true;
        }
    } else if (cmd=="through") {
        if (strl==tr("through") ||
                strl==tr("t", "through")) {
            return true;
        }
    } else if (cmd=="undo") {
        if (strl==tr("undo") ||
                strl==tr("u", "undo")) {
            return true;
        }
    } else if (cmd=="back") {
        if (strl==tr("back") ||
                strl==tr("b", "back")) {
            return true;
        }
    }

    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
RS_String RS_Commands::msgAvailableCommands() {
    return tr("Available commands:");
}


// EOF
