/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#ifndef RS_VARIABLE_H
#define RS_VARIABLE_H


#include "rs.h"
#include "rs_string.h"
#include "rs_vector.h"
#include "rs_debug.h"

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
		RS_String s;
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
    RS_Variable(const RS_String& v, int c) {
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

	void setString(const RS_String& str) {
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

	RS_String getString() {
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
