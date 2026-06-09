/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */
#include "rs_variabledict.h"

#include<iostream>

#include "rs_debug.h"

/**
 * Removes all variables in the blocklist.
 */
void RS_VariableDict::clear() {
    m_variables.clear();
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
bool RS_VariableDict::add(const QString& key, const QString& value, const int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }

    return insert(key, RS_Variable(value, code));
}

bool RS_VariableDict::add(const QString& key, const QString& value, const int code, const int type) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }

    RS_Variable result;
    bool ok{false};
    switch (type) {
        case RS2::VariableString: {
            ok = true;
            result = RS_Variable(value, code);
            break;
        }
        case RS2::VariableInt: {
            const int val = value.toInt(&ok);
            if (ok) {
                result = RS_Variable(val, code);
            }
            break;
        }
        case RS2::VariableDouble: {
            const double val = value.toDouble(&ok);
            if (ok) {
                result = RS_Variable(val, code);
            }
            break;
        }
        case RS2::VariableVector: {
            const int separatorPos = value.trimmed().indexOf(' ');
            if (separatorPos == -1) {
                break;
            }
            const QString left = value.left(separatorPos);
            const QString right = value.right(separatorPos);
            const double x = left.toDouble(&ok);
            if (!ok) {
                break;
            }
            const double y = right.toDouble(&ok);
            if (!ok) {
                break;
            }
            const RS_Vector vect(x, y);
            result = RS_Variable(vect, code);
            break;
        }
        default:
            ok = false;
    }
    if (ok) {
        return insert(key, result);
    }

    RS_DEBUG->print(QString("RS_VariableDict::addVariable(): Cant convert var from string. Name: %1, value: %2.").arg(key, value));

    return false;
}



/**
 * Adds a variable to the variable dictionary. If a variable with the
 * same name already exists, is will be overwritten.
 */
bool RS_VariableDict::add(const QString& key, const int value, const int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }

    return insert(key, RS_Variable(value, code));
}

bool RS_VariableDict::add(const QString& key, const bool value, const int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }
    return insert(key, RS_Variable(value ? 1 : 0, code));
}

/**
 * Adds a variable to the variable dictionary. If a variable with the
 * same name already exists, is will be overwritten.
 */
bool RS_VariableDict::add(const QString& key, const double value, const int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }
    return insert(key, RS_Variable(value, code));
}

/**
 * Adds a variable to the variable dictionary. If a variable with the
 * same name already exists, is will be overwritten.
 */
bool RS_VariableDict::add(const QString& key, const RS_Vector& value, const int code) {
    RS_DEBUG->print("RS_VariableDict::addVariable()");

    if (key.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_VariableDict::addVariable(): No empty keys allowed.");
        return false;
    }

    return insert(key, RS_Variable(value, code));
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
RS_Vector RS_VariableDict::getVector(const QString& key, const RS_Vector& def) const {
    RS_Vector ret;

    const auto i = m_variables.find(key);
    if (m_variables.end() != i && RS2::VariableVector == i.value().getType()) {
        ret = i.value().getVector();
    }
    else {
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
QString RS_VariableDict::getString(const QString& key, const QString& def) const {
    QString ret;

    RS_DEBUG->print("RS_VariableDict::getString: key: '%s'", key.toLatin1().data());

    const auto i = m_variables.find(key);
    if (m_variables.end() != i && RS2::VariableString == i.value().getType()) {
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
int RS_VariableDict::getInt(const QString& key, const int def) const {
    int ret = 0;

    const auto i = m_variables.find(key);
    if (m_variables.end() != i && RS2::VariableInt == i.value().getType()) {
        ret = i.value().getInt();
    }
    else {
        ret = def;
    }

    return ret;
}

bool RS_VariableDict::getBool(const QString& key, const bool def) const {
    const bool defValue = (def ? 1 : 0) != 0;
    return getInt(key, static_cast<int>(defValue)) != 0;
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
double RS_VariableDict::getDouble(const QString& key, const double def) const {
    double ret = 0.0;

    const auto i = m_variables.find(key);
    if (m_variables.end() != i && RS2::VariableDouble == i.value().getType()) {
        ret = i.value().getDouble();
    }
    else {
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
void RS_VariableDict::remove(const QString& key) {
    RS_DEBUG->print("RS_VariableDict::removeVariable()");

    // here the block is removed from the list but not deleted
    m_modified = true;
    m_variables.remove(key);
}

/**
 * Dumps the variables to stdout.
 */
std::ostream& operator <<(std::ostream& os, RS_VariableDict& v) {
    os << "Variables: \n";
    auto it = v.m_variables.begin();
    while (it != v.m_variables.end()) {
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
