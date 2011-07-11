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
#include "qg_arcoptions.h"

#include "rs_settings.h"

/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcOptions::QG_ArcOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ArcOptions::~QG_ArcOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcOptions::languageChange()
{
    retranslateUi(this);
}

void QG_ArcOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/ArcReversed", (int)rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_ArcOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawArc) {
        action = (RS_ActionDrawArc*)a;

        bool reversed;
        if (update) {
            reversed = action->isReversed();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
            RS_SETTINGS->endGroup();
            action->setReversed(reversed);
        }
        rbNeg->setChecked(reversed);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_ArcOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_ArcOptions::init() {
    data = NULL;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();

    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcOptions::updateDirection(bool /*pos*/) {
    if (action!=NULL) {
        action->setReversed(!(rbPos->isChecked()));
    }
}
