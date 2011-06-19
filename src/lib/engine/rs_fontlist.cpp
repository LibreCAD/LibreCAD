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


#include <QHash>
#include "rs_fontlist.h"
#include "rs_debug.h"
#include "rs_font.h"
#include "rs_system.h"

RS_FontList* RS_FontList::uniqueInstance = NULL;



/**
 * Default constructor.
 */
RS_FontList::RS_FontList() {
}



/**
 * Initializes the font list by creating empty RS_Font 
 * objects, one for each font that could be found.
 */
void RS_FontList::init() {
    RS_DEBUG->print("RS_FontList::initFonts");

    QStringList list = RS_SYSTEM->getFontList();
    QHash<QString, int> added; //used to remember added fonts (avoid duplication)
    RS_Font* font;

    for (int i = 0; i < list.size(); ++i) {
        RS_DEBUG->print("font: %s:", list.at(i).toLatin1().data());

        QFileInfo fi( list.at(i) );
        if ( !added.contains(fi.baseName()) ) {
            font = new RS_Font(fi.baseName());
            fonts.append(font);
            added.insert(fi.baseName(), 1);
        }

        RS_DEBUG->print("base: %s", fi.baseName().toLatin1().data());
    }
}



/**
 * Removes all fonts in the fontlist.
 */
void RS_FontList::clearFonts() {
    while (!fonts.isEmpty())
        delete fonts.takeFirst();
}



/**
 * Removes a font from the list.
 * The font was removed from the list and is deleted.
 */
void RS_FontList::removeFont(RS_Font* font) {
    RS_DEBUG->print("RS_FontList::removeFont()");

    int i = fonts.indexOf(font);
    if (i != -1)
        delete fonts.takeAt(i);

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
RS_Font* RS_FontList::requestFont(const QString& name) {
    RS_DEBUG->print("RS_FontList::requestFont %s",  name.toLatin1().data());

    QString name2 = name.toLower();
    RS_Font* foundFont = NULL;

    // QCAD 1 compatibility:
    if (name2.contains('#') && name2.contains('_')) {
        name2 = name2.left(name2.indexOf('_'));
    } else if (name2.contains('#')) {
        name2 = name2.left(name2.indexOf('#'));
    }

    RS_DEBUG->print("name2: %s", name2.toLatin1().data());

    // Search our list of available fonts:
    for (int i = 0; i < fonts.size(); ++i) {
        RS_Font* f = fonts.at(i);

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
    for (int i = 0; i < l.fonts.size(); ++i) {
        RS_Font* f = l.fonts.at(i);

        os << *f << "\n";
    }

    return os;
}




