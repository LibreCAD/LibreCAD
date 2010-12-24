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


#include "rs_fontlist.h"

#include "rs_fileinfo.h"
#include "rs_stringlist.h"
#include "rs_system.h"
#include "rs_dict.h"

RS_FontList* RS_FontList::uniqueInstance = NULL;



/**
 * Default constructor.
 */
RS_FontList::RS_FontList() {
    fonts.setAutoDelete(true);
    //fontListListeners.setAutoDelete(false);
}



/**
 * Initializes the font list by creating empty RS_Font 
 * objects, one for each font that could be found.
 */
void RS_FontList::init() {
    RS_DEBUG->print("RS_FontList::initFonts");

    RS_StringList list = RS_SYSTEM->getFontList();
    RS_Dict<char> added; //used to remeber added fonts (avoid duplication)
    RS_Font* font;

    for ( RS_StringList::Iterator it = list.begin();
            it != list.end(); ++it ) {
        RS_DEBUG->print("font: %s:", (*it).latin1());

        RS_FileInfo fi(*it);
        if (!added[fi.baseName()]) {
            font = new RS_Font(fi.baseName());
            fonts.append(font);
            added.insert(fi.baseName(), (char*)1);
        }

        RS_DEBUG->print("base: %s", fi.baseName().latin1());
    }
}



/**
 * Removes all fonts in the fontlist.
 */
void RS_FontList::clearFonts() {
    fonts.clear();
}



/**
 * Removes a font from the list.
 * Listeners are notified after the font was removed from 
 * the list but before it gets deleted.
 */
void RS_FontList::removeFont(RS_Font* font) {
    RS_DEBUG->print("RS_FontList::removeFont()");

    // here the font is removed from the list but not deleted
    fonts.remove(font);

    //for (uint i=0; i<fontListListeners.count(); ++i) {
    //    RS_FontListListener* l = fontListListeners.at(i);
    //    l->fontRemoved(font);
    //}
}



/**
 * @return Pointer to the font with the given name or
 * \p NULL if no such font was found. The font will be loaded into
 * memory if it's not already.
 */
RS_Font* RS_FontList::requestFont(const RS_String& name) {
    RS_DEBUG->print("RS_FontList::requestFont %s",  name.latin1());

    RS_String name2 = name.lower();
    RS_Font* foundFont = NULL;

    // LibreCAD 1 compatibility:
    if (name2.contains('#') && name2.contains('_')) {
        name2 = name2.left(name2.find('_'));
    } else if (name2.contains('#')) {
        name2 = name2.left(name2.find('#'));
    }

    RS_DEBUG->print("name2: %s", name2.latin1());

    // Search our list of available fonts:
    for (RS_Font* f=fonts.first();
            f!=NULL;
            f=fonts.next()) {

        if (f->getFileName()==name2) {
            // Make sure this font is loaded into memory:
            f->loadFont();
            foundFont = f;
            break;
        }
    }

    if (foundFont==NULL && name!="standard") {
        foundFont = requestFont("standard");
    }

    return foundFont;
}



/**
 * Dumps the fonts to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_FontList& l) {

    os << "Fontlist: \n";
    for (RS_Font* f=l.firstFont();
            f!=NULL;
            f=l.nextFont()) {

        os << *f << "\n";
    }

    return os;
}




