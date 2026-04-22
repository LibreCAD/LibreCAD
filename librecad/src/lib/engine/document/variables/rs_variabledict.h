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

#ifndef RS_VARIABLEDICT_H
#define RS_VARIABLEDICT_H

#include <QHash>

#include "rs_variable.h"

class RS_Vector;
class QString;

/**
 * Dictionary of variables. The variables are stored as key / value
 * pairs (string / string).
 *
 * @author Andrew Mustun
 */
class RS_VariableDict {
public:
    RS_VariableDict() = default;

    void clear();
    /**
     * @return Number of variables available.
     */
    int count() const {
        return m_variables.count();
    }

    bool add(const QString& key, const QString& value, int code, int type);
    bool add(const QString& key, const RS_Vector& value, int code);
    bool add(const QString& key, const QString& value, int code);
    bool add(const QString& key, int value, int code);
    bool add(const QString& key, double value, int code);
    bool add(const QString& key, bool value, int code);

    RS_Vector getVector(const QString& key, const RS_Vector& def) const;
    QString getString(const QString& key, const QString& def) const;
    int getInt(const QString& key, int def) const;
    bool getBool(const QString& key, bool def) const;
    double getDouble(const QString& key, double def) const;

    void remove(const QString& key);

    bool has(const QString& key) const {
        return m_variables.contains(key);
    }

    const QHash<QString, RS_Variable>& getVariableDict() const {
        return m_variables;
    }

    QHash<QString, RS_Variable>& getVariableDict() {
        return m_variables;
    }

    bool isModified() const {return m_modified;}

    void setModified(const bool val) {m_modified = val;}

    //void addVariableDictListener(RS_VariableDictListener* listener);

    friend std::ostream& operator <<(std::ostream& os, RS_VariableDict& v);

private:
    //! Variables for the graphic
    bool insert(const QString& key, const RS_Variable& value) {
        const QString& actualKey = key;
        const auto it = m_variables.find(actualKey);
        bool modified = false;
        if (it != m_variables.end()) {
            const RS_Variable& oldVar = it.value();
            if (oldVar != value) {
                modified = true;
                m_variables.insert(actualKey, value);
            }
        }
        else {
            m_variables.insert(actualKey, value);
            modified = true;
        }
        if (modified) {
            m_modified = true;
        }
        return modified;
    }
    QHash<QString, RS_Variable> m_variables;
    bool m_modified = false;
};

#endif
