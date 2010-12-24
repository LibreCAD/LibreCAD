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


#ifndef RS_COMMANDS_H
#define RS_COMMANDS_H

#include <qobject.h>

#include "rs.h"
#include "rs_dict.h"
#include "rs_string.h"
#include "rs_stringlist.h"

#define RS_COMMANDS RS_Commands::instance()

/**
 * Class which holds all commands for the command line. This 
 * is separated in this module to alow the use of different
 * languages for the gui and the command interface.
 * Implemented as singleton. 
 *
 * @author Andrew Mustun
 */
class RS_Commands : public QObject {
    Q_OBJECT

public:
    /**
     * @return Instance to the unique commands object.
     */
    static RS_Commands* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_Commands();
        }
        return uniqueInstance;
    }

	RS_StringList complete(const RS_String& cmd);
    RS2::ActionType cmdToAction(const RS_String& cmd);
    RS2::ActionType keycodeToAction(const RS_String& code);

    static RS_String command(const RS_String& cmd);

    static bool checkCommand(const RS_String& cmd, const RS_String& str,
                             RS2::ActionType action=RS2::ActionNone);

	static RS_String msgAvailableCommands();

protected:
    static RS_Commands* uniqueInstance;

private:
	RS_Commands();
	RS_Dict<RS2::ActionType> mainCommands;
	RS_Dict<RS2::ActionType> shortCommands;
};

#endif

