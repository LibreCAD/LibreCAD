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


#ifndef RS_COLOR_H
#define RS_COLOR_H

#include <qcolor.h>

#include "rs_flags.h"


//! Color defined by layer not entity
//#define C_BY_LAYER     0x00000001
//! Color defined by block not entity
//#define C_BY_BLOCK     0x00000002

/**
 * Color class.
 *
 * @author Andrew Mustun
 */
class RS_Color: public QColor, public RS_Flags {
public:
    RS_Color() : QColor(), RS_Flags() {}
    RS_Color(int r, int g, int b) : QColor(r, g, b), RS_Flags() {}
    RS_Color(const QColor& c) : QColor(c), RS_Flags() {}
    RS_Color(const RS_Color& c) : QColor(c), RS_Flags() {
        setFlags(c.getFlags());
    }
    RS_Color(unsigned int f) : QColor(), RS_Flags(f) {}

    /** @return A copy of this color without flags. */
    RS_Color stripFlags() const {
        return RS_Color(red(), green(), blue());
    }

    /** @return true if the color is defined by layer. */
    bool isByLayer() const {
        return getFlag(RS2::FlagByLayer);
    }

    /** @return true if the color is defined by block. */
    bool isByBlock() const {
        return getFlag(RS2::FlagByBlock);
    }


    RS_Color& operator = (const RS_Color& c) {
        setRgb(c.red(), c.green(), c.blue());
        setFlags(c.getFlags());

        return *this;
    }

    bool operator == (const RS_Color& c) const {
        return (red()==c.red() &&
                green()==c.green() &&
                blue()==c.blue() &&
                getFlags()==c.getFlags());
    }

    friend std::ostream& operator << (std::ostream& os, const RS_Color& c) {
        os << " color: " << c.name().latin1()
        << " flags: " << (c.getFlag(RS2::FlagByLayer) ? "RS2::FlagByLayer " : "")
        << (c.getFlag(RS2::FlagByBlock) ? "RS2::FlagByBlock " : "");
        return os;
    }
};

#endif

