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


#ifndef RS_VARIABLE_H
#define RS_VARIABLE_H


#include "rs.h"
#include "rs_vector.h"

/**
 * A variable of type int, double, string or vector.
 * The variable also contains its type and an int code
 * which can identify its contents in any way.
 *
 * @author Andrew Mustun
 */
class RS_Variable {
public:
	typedef struct {
                QString s;
		int i;
		double d;
		RS_Vector v;
	} RS_VariableContents;
	
    RS_Variable() {
		type = RS2::VariableVoid;
		code = 0;
	}
    RS_Variable(const RS_Vector& v, int c) {
		setVector(v);
		code = c;
	}
    RS_Variable(const QString& v, int c) {
		setString(v);
		code = c;
	}
    RS_Variable(int v, int c) {
		setInt(v);
		code = c;
	}
    RS_Variable(double v, int c) {
		setDouble(v);
		code = c;
	}
    virtual ~RS_Variable() {}

        void setString(const QString& str) {
		contents.s = str;
		type = RS2::VariableString;
	}
	void setInt(int i) {
		contents.i = i;
		type = RS2::VariableInt;
	}
	void setDouble(double d) {
		contents.d = d;
		type = RS2::VariableDouble;
	}
	void setVector(const RS_Vector& v) {
		contents.v = v;
		type = RS2::VariableVector;
	}

        QString getString() {
		return contents.s;
	}
	int getInt() {
		return contents.i;
	}
	double getDouble() {
		return contents.d;
	}
	RS_Vector getVector() {
		return contents.v;
	}

	RS2::VariableType getType() {
		return type;
	}
	int getCode() {
		return code;
	}

    //friend std::ostream& operator << (std::ostream& os, RS_Variable& v);

private:
	RS_VariableContents contents;
	RS2::VariableType type;
	int code;
};

#endif
