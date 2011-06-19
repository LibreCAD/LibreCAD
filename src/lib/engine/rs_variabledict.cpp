/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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


#include "rs_variabledict.h"
#include "rs_debug.h"

/**
 * Constructor.
 */
RS_VariableDict::RS_VariableDict() {
}


/**
 * Removes all variables in the blocklist.
 */
void RS_VariableDict::clear() {
    variables.clear();
}


/**
 * Activates the given block.
 * Listeners are notified.
 */
//void RS_VariableDict::activateBlock(const RS_String& name) {
//	activateBlock(findBlock(name));
//}

/**
 * Activates the given block.
 * Listeners are notified.
 */
/*void RS_VariableDict::activateBlock(RS_Block* block) {
	activeBlock = block;
	
	for (uint i=0; i<blockListListeners.count(); ++i) {
		RS_VariableDictListener* l = blockListListeners.at(i);
 
		l->blockActivated(activeBlock);
	}
}*/




/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key,
                                  const QString& value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key, int value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key, double value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.insert(key, RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const QString& key,
                                  const RS_Vector& value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
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
RS_Vector RS_VariableDict::getVector(const QString& key,
        const RS_Vector& def) {

    RS_Vector ret;
    QHash<QString, RS_Variable>::iterator i = variables.find(key);
     if (i != variables.end() || i.value().getType()==RS2::VariableVector) {
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
QString RS_VariableDict::getString(const QString& key,
        const QString& def) {

    QString ret;

	RS_DEBUG->print("RS_VariableDict::getString: 001");
        RS_DEBUG->print("RS_VariableDict::getString: key: '%s'", key.toLatin1().data());
	
    QHash<QString, RS_Variable>::iterator i = variables.find(key);
        RS_DEBUG->print("RS_VariableDict::getString: 002");

    if (i == variables.end()) {
		RS_DEBUG->print("RS_VariableDict::getString: 003");
        ret = def;
        } else if (i.value().getType() != RS2::VariableString) {
		RS_DEBUG->print("RS_VariableDict::getString: 004");
		ret = def;
    } else {
		RS_DEBUG->print("RS_VariableDict::getString: 005");
        ret = i.value().getString();
    }
	RS_DEBUG->print("RS_VariableDict::getString: 006");

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
int RS_VariableDict::getInt(const QString& key,
                                    int def) {

    int ret;
    QHash<QString, RS_Variable>::iterator i = variables.find(key);
     if (i != variables.end() || i.value().getType()==RS2::VariableInt) {
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
double RS_VariableDict::getDouble(const QString& key,
        double def) {

    double ret;
    QHash<QString, RS_Variable>::iterator i = variables.find(key);
     if (i != variables.end() || i.value().getType()==RS2::VariableDouble) {
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
void RS_VariableDict::addBlockNotification() {
    for (uint i=0; i<blockListListeners.count(); ++i) {
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
void RS_VariableDict::remove(const QString& key) {
    RS_DEBUG->print("RS_VariableDict::removeVariable()");

    // here the block is removed from the list but not deleted
    variables.remove(key);

}



/**
 * Dumps the variables to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_VariableDict& d) {

    os << "Variables: \n";
    QHash<QString, RS_Variable>::iterator it = d.variables.begin();
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

