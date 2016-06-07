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

#include <QString>
#include "rs_scriptlist.h"

#include "rs_system.h"
#include "rs_debug.h"

RS_ScriptList* RS_ScriptList::uniqueInstance = NULL;

/**
 * Default constructor.
 */
RS_ScriptList::RS_ScriptList() {
    //init();
    //scriptListListeners.setAutoDelete(false);
    //activeScript = NULL;
}


/**
 * Initializes the script list by creating RS_Script 
 * objects, one for each script that could be found.
 */
void RS_ScriptList::init() {

    RS_DEBUG->print("RS_ScriptList::initScripts");

    clearScripts();
    QStringList list = RS_SYSTEM->getScriptList();
    RS_Script* script;

    for ( QStringList::Iterator it = list.begin();
            it != list.end(); ++it ) {
        RS_DEBUG->print("script: %s:", (*it).toLatin1().data());

        QFileInfo fi(*it);
        script = new RS_Script(fi.baseName(), fi.absoluteFilePath());
        scripts.append(script);

        RS_DEBUG->print("base: %s", fi.baseName().toLatin1().data());
        RS_DEBUG->print("path: %s", fi.absoluteFilePath().toLatin1().data());
    }

    //RS_Script* f = new RS_Script("normal");
    //scripts.append(f);
}


/**
 * Removes all scripts in the scriptlist.
 */
void RS_ScriptList::clearScripts() {
    while (!scripts.isEmpty())
        delete scripts.takeFirst();
}



/**
 * Removes a script from the list.
 * Listeners are notified after the script was removed from 
 * the list but before it gets deleted.
 */
void RS_ScriptList::removeScript(RS_Script* script) {
    RS_DEBUG->print("RS_ScriptList::removeScript()");

    // here the script is removed from the list but not deleted
    scripts.removeOne(script);

    //for (unsigned i=0; i<scriptListListeners.count(); ++i) {
    //    RS_ScriptListListener* l = scriptListListeners.at(i);
    //    l->scriptRemoved(script);
    //}

    // activate an other script if necessary:
    //if (activeScript==script) {
    //    activateScript(scripts.first());
    //}

    // now it's save to delete the script
    //delete script;
}



/**
 * @return Pointer to the script with the given name or
 * \p NULL if no such script was found. The script will be loaded into
 * memory if it's not already.
 */
RS_Script* RS_ScriptList::requestScript(const QString& name) {
    RS_DEBUG->print("RS_ScriptList::requestScript %s",  name.toLatin1().data());

    QString name2 = name.toLower();
    RS_Script* foundScript = NULL;

    RS_DEBUG->print("name2: %s", name2.toLatin1().data());

    // Search our list of available scripts:
    for (int i = 0; i < scripts.size(); ++i) {

        if (scripts.at(i)->getName()==name2) {
            foundScript = scripts.at(i);
            break;
        }
    }

    // Script not found:
    return foundScript;
}



/**
 * @return Pointer to the script with the given name or
 * \p NULL if no such script was found.
 */
//RS_Script* RS_ScriptList::loadScript(const QString& name) {
//}


/**
 * Tests the script list and its ability to load scripts.
 */
bool RS_ScriptList::test() {

    //RS_ScriptList* l = RS_ScriptList::instance();

    //std::cout << "RS_ScriptList: " << *l << std::endl;

    return true;
}


// EOF
