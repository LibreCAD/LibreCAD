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


#ifndef RS_VARIABLEDICT_H
#define RS_VARIABLEDICT_H


#include "rs_variabledict.h"
#include "rs_variable.h"
#include "rs_dict.h"
#include "rs_string.h"
#include "rs_debug.h"

/**
 * Dictionary of variables. The variables are stored as key / value
 * pairs (string / string).
 *
 * @author Andrew Mustun
 */
class RS_VariableDict {
public:
    RS_VariableDict();
    virtual ~RS_VariableDict() {}

    void clear();
    /**
     * @return Number of variables available.
     */
    int count() {
        return variables.count();
    }

    void add(const RS_String& key, const RS_Vector& value, int code);
    void add(const RS_String& key, const RS_String& value, int code);
    void add(const RS_String& key, int value, int code);
    void add(const RS_String& key, double value, int code);

    RS_Vector getVector(const RS_String& key, const RS_Vector& def);
    RS_String getString(const RS_String& key, const RS_String& def);
    int getInt(const RS_String& key, int def);
    double getDouble(const RS_String& key, double def);

    virtual void remove(const RS_String& key);

	RS_Dict<RS_Variable>& getVariableDict() {
		return variables;
	}

    //void addVariableDictListener(RS_VariableDictListener* listener);

    friend std::ostream& operator << (std::ostream& os, RS_VariableDict& v);

private:
    //! Variables for the graphic
    RS_Dict<RS_Variable> variables;
}
;

#endif
