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

#include "rs_fontlist.h"

#include <QFileInfo>
#include <QLocale>
#include <QString>
#include <iostream>

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_system.h"

RS_FontList* RS_FontList::m_uniqueInstance = nullptr;

RS_FontList* RS_FontList::instance() {
    if (m_uniqueInstance == nullptr) {
        m_uniqueInstance = new RS_FontList();
    }
    return m_uniqueInstance;
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

    for (const auto& i : list) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "font: %s:", i.toLatin1().data());

        QFileInfo fi(i);
        if (!added.contains(fi.baseName())) {
            m_fonts.emplace_back(new RS_Font(fi.baseName()));
            added.insert(fi.baseName(), 1);
        }

        RS_DEBUG->print(RS_Debug::D_ERROR, "base: %s", fi.baseName().toLatin1().data());
    }
}

size_t RS_FontList::countFonts() const {
    return m_fonts.size();
}

std::vector<std::unique_ptr<RS_Font>>::const_iterator RS_FontList::begin() const {
    return m_fonts.begin();
}

std::vector<std::unique_ptr<RS_Font>>::const_iterator RS_FontList::end() const {
    return m_fonts.end();
}

/**
 * Removes all m_fonts in the fontlist.
 */
void RS_FontList::clearFonts() {
    m_fonts.clear();
}

/**
 * @return Pointer to the font with the given name or
 * \p NULL if no such font was found. The font will be loaded into
 * memory if it's not already.
 */
RS_Font* RS_FontList::requestFont(const QString& name) {
    RS_DEBUG->print("RS_FontList::requestFont %s", name.toLatin1().data());

    QString name2 = name.toLower();
    RS_Font* foundFont = nullptr;

    if (name.isEmpty()) {
        return foundFont;
    }

    // QCAD 1 compatibility:
    if (name2.contains('#') && name2.contains('_')) {
        name2 = name2.left(name2.indexOf('_'));
    }
    else if (name2.contains('#')) {
        name2 = name2.left(name2.indexOf('#'));
    }

    RS_DEBUG->print("name2: %s", name2.toLatin1().data());

    // Search our list of available fonts:
    for (const auto& f : m_fonts) {
        if (f->getFileName().toLower() == name2) {
            // Make sure this font is loaded into memory:
            f->loadFont();
            foundFont = f.get();
            break;
        }
    }

    if ((foundFont == nullptr) && name != "standard") {
        foundFont = requestFont("standard");
    }

    return foundFont;
}

QString RS_FontList::getDefaultFont() {
    const QLocale loc = QLocale::system();
    const QLocale::Script script = loc.script();

    switch (script) {
        case QLocale::ArabicScript:
            return "amiri-regular";
        case QLocale::CyrillicScript:
            return "OpenGostTypeA-Regular";
        case QLocale::GreekScript:
            return "greeks";
        case QLocale::JapaneseScript:
            return "kochigothic";
        case QLocale::HangulScript:
            return "kst32b";
        case QLocale::SimplifiedHanScript:
        case QLocale::TraditionalHanScript:
            return "unicode";
        default:
            return "unicode";
    }
}

/**
 * Dumps the m_fonts to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_FontList& l) {
    os << "Fontlist: \n";
    for (const auto& f : l.m_fonts) {
        os << *f << "\n";
    }

    return os;
}
