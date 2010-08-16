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


#ifndef RS_FLAGS_H
#define RS_FLAGS_H

#include "rs.h"
#include "rs_debug.h"

/**
 * Base class for objects which have flags.
 *
 * @author Andrew Mustun
 */
class RS_Flags {
public:
    /** Default constructor. Resets all flags to 0. */
    RS_Flags() {
        flags = 0;
    }

    /** Constructor with initialisation to the given flags. */
    RS_Flags(unsigned int f) {
        flags = f;
    }

    virtual ~RS_Flags() {}

    unsigned int getFlags() const {
        return flags;
    }

    void resetFlags() {
        flags=0;
    }

    void setFlags(unsigned int f) {
        flags=f;
    }

    void setFlag(unsigned int f) {
        flags=flags|f;
    }

    void delFlag(unsigned int f) {
        flags=flags&(~f);
    }

    void toggleFlag(unsigned int f) {
        flags=flags^f;
    }

    bool getFlag(unsigned int f) const {
        if(flags&f) {
            return true;
        } else {
            return false;
        }
    }

private:
    unsigned int flags;
};

#endif
