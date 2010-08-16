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

#ifndef RS_SCRIPT_H
#define RS_SCRIPT_H

#include <iostream>

#include "rs_string.h"


/**
 * Class for representing a script. This is implemented as a RS_String
 * containing the script name.
 *
 * OBSOLETE
 *
 * @author Andrew Mustun
 */
class RS_Script {
public:
    RS_Script(const RS_String& name, const RS_String& path);
    //RS_Script(const char* name);

    /** @return the name of this script. */
    RS_String getName() const {
        return name;
    }

    /** @return the full path and file name of this script. */
    RS_String getPath() const {
        return path;
    }

private:
    //! Script name
    RS_String name;

    //! Full path to script
    RS_String path;
};

#endif

