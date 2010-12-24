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


#ifndef RS_FONTLIST_H
#define RS_FONTLIST_H


#include "rs_font.h"
#include "rs_entity.h"
#include "rs_ptrlist.h"

#define RS_FONTLIST RS_FontList::instance()

/**
 * The global list of fonts. This is implemented as a singleton.
 * Use RS_FontList::instance() to get a pointer to the object.
 *
 * @author Andrew Mustun
 */
class RS_FontList {
protected:
    RS_FontList();

public:
    /**
     * @return Instance to the unique font list.
     */
    static RS_FontList* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_FontList();
        }
        return uniqueInstance;
    }

    virtual ~RS_FontList() {}

    void init();

    void clearFonts();
    int countFonts() {
        return fonts.count();
    }
    virtual void removeFont(RS_Font* font);
    RS_Font* requestFont(const RS_String& name);
    //! @return First font of the list.
    RS_Font* firstFont() {
        return fonts.first();
    }
    /** 
	 * @return Next font from the list after
     * calling firstFont() or nextFont().
     */
    RS_Font* nextFont() {
        return fonts.next();
    }

    friend std::ostream& operator << (std::ostream& os, RS_FontList& l);

protected:
    static RS_FontList* uniqueInstance;

private:
    //! fonts in the graphic
    RS_PtrList<RS_Font> fonts;
}
;

#endif
