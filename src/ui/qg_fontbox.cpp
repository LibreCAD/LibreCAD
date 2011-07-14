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

#include "qg_fontbox.h"

#include "rs_font.h"
#include "rs_fontlist.h"

#include "rs_debug.h"


/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_FontBox::QG_FontBox(QWidget* parent)
        : QComboBox(parent) {}



/**
 * Destructor
 */
QG_FontBox::~QG_FontBox() {}


/**
 * Initialisation (called from constructor or manually but only
 * once).
 */
void QG_FontBox::init() {
    QStringList fonts;

    QListIterator<RS_Font *> i = RS_FONTLIST->getIteretor();
    while (i.hasNext()) {
        fonts.append( i.next()->getFileName() );
    }

    fonts.sort();
    addItems(fonts);

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotFontChanged(int)));

    setCurrentIndex(0);
    slotFontChanged(currentIndex());
}



/**
 * Sets the currently selected width item to the given width.
 */
void QG_FontBox::setFont(const QString& fName) {

    RS_DEBUG->print("QG_FontBox::setFont %s\n", fName.toLatin1().data());

    setItemText(currentIndex(),fName);

    slotFontChanged(currentIndex());
}



/**
 * Called when the font has changed. This method 
 * sets the current font to the value chosen.
 */
void QG_FontBox::slotFontChanged(int index) {

    RS_DEBUG->print("QG_FontBox::slotFontChanged %d\n", index);

    currentFont = RS_FONTLIST->requestFont(currentText());

    if (currentFont!=NULL) {
        RS_DEBUG->print("Current font is (%d): %s\n",
                        index, currentFont->getFileName().toLatin1().data());
    }

    emit fontChanged(currentFont);
}


