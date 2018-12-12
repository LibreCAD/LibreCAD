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


#ifndef RS_FONTCHAR_H
#define RS_FONTCHAR_H

#include "rs_block.h"


/**
 * A character in a font is represented by this special block class.
 *
 * @author Andrew Mustun
 */
class RS_FontChar : public RS_Block {
public:
    /**
     * @param parent The font this block belongs to.
     * @param name The name of the letter (a unicode char) used as 
     *        an identifier.
     * @param basePoint Base point (offset) of the letter (usually 0/0).
     */
    RS_FontChar(RS_EntityContainer* parent,
                const QString& name,
                RS_Vector basePoint)
            : RS_Block(parent, RS_BlockData(name, basePoint, false)) {}

    virtual ~RS_FontChar() {}

    /** @return RS2::EntityFontChar */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityFontChar;
    }


    /*friend std::ostream& operator << (std::ostream& os, const RS_FontChar& b) {
       	os << " name: " << b.getName().latin1() << "\n";
    	os << " entities: " << (RS_EntityContainer&)b << "\n";
       	return os;
}*/


protected:
};


#endif
