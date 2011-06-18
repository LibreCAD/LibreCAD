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

#include "qg_fontbox.h"

#include <qpixmap.h>
#include <qstringlist.h>

#include "rs_debug.h"


/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_FontBox::QG_FontBox(QWidget* parent, const char* name)
        : QComboBox(parent, name) {}



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
    insertStringList(fonts);

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotFontChanged(int)));

    setCurrentItem(0);
    slotFontChanged(currentItem());
}



/**
 * Sets the currently selected width item to the given width.
 */
void QG_FontBox::setFont(const RS_String& fName) {

    RS_DEBUG->print("QG_FontBox::setFont %s\n", fName.latin1());

    setCurrentText(fName);

    slotFontChanged(currentItem());
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
                        index, currentFont->getFileName().latin1());
    }

    emit fontChanged(currentFont);
}


