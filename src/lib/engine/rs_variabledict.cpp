/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#include "rs_variabledict.h"


/**
 * Constructor.
 */
RS_VariableDict::RS_VariableDict() {
    variables.setAutoDelete(true);
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
void RS_VariableDict::add(const RS_String& key,
                                  const RS_String& value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.replace(key, new RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const RS_String& key, int value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.replace(key, new RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const RS_String& key, double value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.replace(key, new RS_Variable(value, code));
}



/**
 * Adds a variable to the variable dictionary. If a variable with the 
 * same name already exists, is will be overwritten.
 */
void RS_VariableDict::add(const RS_String& key,
                                  const RS_Vector& value, int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print("RS_VariableDict::addVariable(): "
                        "No empty keys allowed.",
                        RS_Debug::D_WARNING);
        return;
    }

    variables.replace(key, new RS_Variable(value, code));
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
RS_Vector RS_VariableDict::getVector(const RS_String& key,
        const RS_Vector& def) {

    RS_Vector ret;
    RS_Variable* ptr = variables.find(key);

    if (ptr==NULL || ptr->getType()!=RS2::VariableVector) {
        ret = def;
    } else {
        ret = ptr->getVector();
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
RS_String RS_VariableDict::getString(const RS_String& key,
        const RS_String& def) {

    RS_String ret;

	RS_DEBUG->print("RS_VariableDict::getString: 001");
	RS_DEBUG->print("RS_VariableDict::getString: key: '%s'", key.latin1());
	
    RS_Variable* ptr = variables.find(key);
	RS_DEBUG->print("RS_VariableDict::getString: 002");

    if (ptr==NULL) {
		RS_DEBUG->print("RS_VariableDict::getString: 003");
        ret = def;
	} else if (ptr->getType()!=RS2::VariableString) {
		RS_DEBUG->print("RS_VariableDict::getString: 004");
		ret = def;
    } else {
		RS_DEBUG->print("RS_VariableDict::getString: 005");
        ret = ptr->getString();
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
int RS_VariableDict::getInt(const RS_String& key,
                                    int def) {

    int ret;
    RS_Variable* ptr = variables.find(key);

    if (ptr==NULL || ptr->getType()!=RS2::VariableInt) {
        ret = def;
    } else {
        ret = ptr->getInt();
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
double RS_VariableDict::getDouble(const RS_String& key,
        double def) {

    double ret;
    RS_Variable* ptr = variables.find(key);

    if (ptr==NULL || ptr->getType()!=RS2::VariableDouble) {
        ret = def;
    } else {
        ret = ptr->getDouble();
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
void RS_VariableDict::remove(const RS_String& key) {
    RS_DEBUG->print("RS_VariableDict::removeVariable()");

    // here the block is removed from the list but not deleted
    variables.remove(key);

}



/**
 * Dumps the variables to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_VariableDict& d) {

    os << "Variables: \n";
    RS_DictIterator<RS_Variable> it(d.variables);
    for( ; it.current(); ++it ) {
		// RVT_PORT changed it.currentKey() to it.currentKey().ascii()
        os << it.currentKey().ascii() << ": ";
        switch (it.current()->getType()) {
        case RS2::VariableVoid:
            os << "void\n";
            break;
        case RS2::VariableInt:
            os << "int " << it.current()->getInt() << "\n";
            break;
        case RS2::VariableDouble:
            os << "double " << it.current()->getDouble() << "\n";
            break;
        case RS2::VariableVector:
            os << "vector " << it.current()->getVector() << "\n";
            break;
        case RS2::VariableString:
			// RVT_PORT
            os << "string " << it.current()->getString().ascii() << "\n";
            break;
        }
    }
    os << std::endl;

    return os;
}

