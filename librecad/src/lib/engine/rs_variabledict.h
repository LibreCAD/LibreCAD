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
        return variables.count();
    }

    void add(const QString& key, const RS_Vector& value, int code);
    void add(const QString& key, const QString& value, int code);
    void add(const QString& key, int value, int code);
    void add(const QString& key, double value, int code);

	RS_Vector getVector(const QString& key, const RS_Vector& def) const;
	QString getString(const QString& key, const QString& def) const;
	int getInt(const QString& key, int def) const;
	double getDouble(const QString& key, double def) const;

	void remove(const QString& key);

	QHash<QString, RS_Variable> const& getVariableDict() const {
        return variables;
    }
	QHash<QString, RS_Variable>& getVariableDict() {
		return variables;
	}

    //void addVariableDictListener(RS_VariableDictListener* listener);

    friend std::ostream& operator << (std::ostream& os, RS_VariableDict& v);

private:
    //! Variables for the graphic
    QHash<QString, RS_Variable> variables;
};

#endif
