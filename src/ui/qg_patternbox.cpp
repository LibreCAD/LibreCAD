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

#include "qg_patternbox.h"

#include <qpixmap.h>
#include <qstringlist.h>

#include "rs_debug.h"


/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_PatternBox::QG_PatternBox(QWidget* parent, const char* name)
        : QComboBox(parent, name) {
}



/**
 * Destructor
 */
QG_PatternBox::~QG_PatternBox() {}


/**
 * Initialisation (called manually and only once).
 */
void QG_PatternBox::init() {
    QStringList patterns;

    for (RS_Pattern* f = RS_PATTERNLIST->firstPattern();
            f!=NULL;
            f = RS_PATTERNLIST->nextPattern()) {

        patterns.append(f->getFileName());
    }

    patterns.sort();
    insertStringList(patterns);

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotPatternChanged(int)));

    setCurrentItem(0);
    slotPatternChanged(currentItem());
}



/**
 * Sets the currently selected width item to the given width.
 */
void QG_PatternBox::setPattern(const RS_String& pName) {

    RS_DEBUG->print("QG_PatternBox::setPattern %s\n", pName.latin1());

    setCurrentText(pName);

    slotPatternChanged(currentItem());
}



/**
 * Called when the pattern has changed. This method 
 * sets the current pattern to the value chosen.
 */
void QG_PatternBox::slotPatternChanged(int index) {

    RS_DEBUG->print("QG_PatternBox::slotPatternChanged %d\n", index);

    currentPattern = RS_PATTERNLIST->requestPattern(currentText());

    if (currentPattern!=NULL) {
        RS_DEBUG->print("Current pattern is (%d): %s\n",
                        index, currentPattern->getFileName().latin1());
    }

    emit patternChanged(currentPattern);
}


