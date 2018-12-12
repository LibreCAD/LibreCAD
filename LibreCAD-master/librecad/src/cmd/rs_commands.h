/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
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

#include <map>
#include "rs.h"

#define RS_COMMANDS RS_Commands::instance()

/**
 * Class which holds all commands for the command line. This
 * is separated in this module to allow the use of different
 * languages for the GUI and the command interface.
 * Implemented as singleton.
 *
 * @author Andrew Mustun
 */
class RS_Commands {

public:
    /**
     * @return Instance to the unique commands object.
     */
    static RS_Commands* instance();

    QStringList complete(const QString& cmd);
    RS2::ActionType cmdToAction(const QString& cmd, bool verbose = true);
    RS2::ActionType keycodeToAction(const QString& code);

    static QString command(const QString& cmd);

    static bool checkCommand(const QString& cmd, const QString& str,
                             RS2::ActionType action=RS2::ActionNone);

    static QString msgAvailableCommands();
    void updateAlias();

    // Prefixes for function-, Meta- and Alt- keys.
    static const char *FnPrefix;
    static const char *AltPrefix;
    static const char *MetaPrefix;
    /**
      * @brief extractCliCal, filter cli calculator math expression
      * @param cmd, cli string
      * @return math expression for RS_Math:eval();
      */
    static QString filterCliCal(const QString& cmd);

protected:
    static RS_Commands* uniqueInstance;

private:
    RS_Commands() ;
    ~RS_Commands()=delete;
    RS_Commands(RS_Commands& ) = delete;
    RS_Commands& operator = ( RS_Commands& ) = delete;

    std::map<QString, RS2::ActionType> mainCommands;
    std::map<QString, RS2::ActionType> shortCommands;
    // key=english command , value = translated
    std::map<QString, QString> cmdTranslation;
};

#endif

