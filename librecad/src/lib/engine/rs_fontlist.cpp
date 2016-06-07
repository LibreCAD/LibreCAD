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

#include <iostream>
#include <QHash>
#include "rs_fontlist.h"
#include "rs_debug.h"
#include "rs_font.h"
#include "rs_system.h"

RS_FontList* RS_FontList::uniqueInstance = nullptr;

RS_FontList* RS_FontList::instance() {
	if (!uniqueInstance) {
		uniqueInstance = new RS_FontList();
	}
	return uniqueInstance;
}


/**
 * Initializes the font list by creating empty RS_Font 
 * objects, one for each font that could be found.
 */
void RS_FontList::init() {
    RS_DEBUG->print("RS_FontList::initFonts");

    QStringList list = RS_SYSTEM->getNewFontList();
    list.append(RS_SYSTEM->getFontList());
    QHash<QString, int> added; //used to remember added fonts (avoid duplication)

    for (int i = 0; i < list.size(); ++i) {
        RS_DEBUG->print("font: %s:", list.at(i).toLatin1().data());

        QFileInfo fi( list.at(i) );
        if ( !added.contains(fi.baseName()) ) {
			fonts.emplace_back(new RS_Font(fi.baseName()));
            added.insert(fi.baseName(), 1);
        }

        RS_DEBUG->print("base: %s", fi.baseName().toLatin1().data());
    }
}

size_t RS_FontList::countFonts() const{
	return fonts.size();
}

std::vector<std::unique_ptr<RS_Font> >::const_iterator RS_FontList::begin() const
{
	return fonts.begin();
}

std::vector<std::unique_ptr<RS_Font> >::const_iterator RS_FontList::end() const
{
	return fonts.end();
}

/**
 * Removes all fonts in the fontlist.
 */
void RS_FontList::clearFonts() {
	fonts.clear();
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
	for( auto const& f: fonts){

        if (f->getFileName()==name2) {
            // Make sure this font is loaded into memory:
            f->loadFont();
			foundFont = f.get();
            break;
        }
    }

	if (!foundFont && name!="standard") {
        foundFont = requestFont("standard");
    }

    return foundFont;
}

/**
 * Dumps the fonts to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_FontList& l) {

    os << "Fontlist: \n";
	for(auto const& f: l.fonts){
        os << *f << "\n";
    }

    return os;
}




