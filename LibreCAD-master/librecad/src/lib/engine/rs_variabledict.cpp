/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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
#include<iostream>
#include <QString>
#include "rs_variabledict.h"
#include "rs_debug.h"

/**
 * Removes all variables in the blocklist.
 */
void RS_VariableDict::clear()
{
    variables.clear();
}


/**
 * Activates the given block.
 * Listeners are notified.
 */
//void RS_VariableDict::activateBlock(const QString& name) {
//	activateBlock(findBlock(name));
//}

/**
 * Activates the given block.
 * Listeners are notified.
 */
/*void RS_VariableDict::activateBlock(RS_Block* block)
{
	activeBlock = block;
	
    for (unsigned i=0; i<blockListListeners.count(); ++i) {
		RS_VariableDictListener* l = blockListListeners.at(i);
 
		l->blockActivated(activeBlock);
	}
}*/


/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key,
                          const QString& value, int code)
{
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_VariableDict::addVariable(): No empty keys allowed.");
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}


/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key, int value, int code)
{
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_VariableDict::addVariable(): No empty keys allowed.");
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}


/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key, double value, int code)
{
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_VariableDict::addVariable(): No empty keys allowed.");
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}


/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key,
                          const RS_Vector& value, int code)
{
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_VariableDict::addVariable(): No empty keys allowed.");
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}


/**
 * Gets the value for the given variable.
 *
 * @param key Key of the variable.
 * @param def Default value.
 *
 * @return The value for the given variable or the given default value
 * if the variable couldn't be found.
 */
RS_Vector RS_VariableDict::getVector(const QString& key, const RS_Vector& def) const
{
    RS_Vector ret;

	auto i = variables.find(key);
    if (variables.end() != i && RS2::VariableVector == i.value().getType()) {
        ret = i.value().getVector();
    } else {
        ret = def;
    }

    return ret;
}


/**
 * Gets the value for the given variable.
 *
 * @param key Key of the variable.
 * @param def Default value.
 *
 * @return The value for the given variable or the given default value
 * if the variable couldn't be found.
 */
QString RS_VariableDict::getString(const QString& key, const QString& def) const
{
    QString ret;

    RS_DEBUG->print("RS_VariableDict::getString: key: '%s'", key.toLatin1().data());

	auto i = variables.find(key);
    if (variables.end() != i && RS2::VariableString == i.value().getType()) {
        ret = i.value().getString();
    }
    else {
        ret = def;
    }

    return ret;
}


/**
 * Gets the value as int for the given variable.
 *
 * @param key Key of the variable.
 * @param def Default value.
 *
 * @return The value for the given variable or the given default value
 * if the variable couldn't be found.
 */
int RS_VariableDict::getInt(const QString& key, int def) const
{
    int ret = 0;

	auto i = variables.find(key);
    if (variables.end() != i && RS2::VariableInt == i.value().getType()) {
        ret = i.value().getInt();
    } else {
        ret = def;
    }

    return ret;
}


/**
 * Gets the value as double for the given variable.
 *
 * @param key Key of the variable.
 * @param def Default value.
 *
 * @return The value for the given variable or the given default value
 * if the variable couldn't be found.
 */
double RS_VariableDict::getDouble(const QString& key, double def) const
{
    double ret = 0.0;

	auto i = variables.find(key);
     if (variables.end() != i && RS2::VariableDouble == i.value().getType()) {
        ret = i.value().getDouble();
    } else {
        ret = def;
    }

    return ret;
}


/**
 * Notifies the listeners about layers that were added. This can be
 * used after adding a lot of variables without auto-update.
 */
/*
void RS_VariableDict::addBlockNotification()
{
    for (unsigned i=0; i<blockListListeners.count(); ++i) {
        RS_VariableDictListener* l = blockListListeners.at(i);
        l->blockAdded(NULL);
    }
}
*/


/**
 * Removes a variable from the list.
 * TODO: Listeners are notified after the block was removed from 
 * the list but before it gets deleted.
 */
void RS_VariableDict::remove(const QString& key)
{
    RS_DEBUG->print("RS_VariableDict::removeVariable()");

    // here the block is removed from the list but not deleted
    variables.remove(key);
}


/**
 * Dumps the variables to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_VariableDict& d)
{
    os << "Variables: \n";
	auto it = d.variables.begin();
    while (it != d.variables.end()) {
        os << it.key().toLatin1().data() << ": ";
        switch (it.value().getType()) {
        case RS2::VariableVoid:
            os << "void\n";
            break;
        case RS2::VariableInt:
            os << "int " << it.value().getInt() << "\n";
            break;
        case RS2::VariableDouble:
            os << "double " << it.value().getDouble() << "\n";
            break;
        case RS2::VariableVector:
            os << "vector " << it.value().getVector() << "\n";
            break;
        case RS2::VariableString:
            os << "string " << it.value().getString().toLatin1().data() << "\n";
            break;
        }
        ++it;
    }
    os << std::endl;

    return os;
}

