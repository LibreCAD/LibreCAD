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
#include "qg_trimamountoptions.h"

#include <qvariant.h>

/*
 *  Constructs a QG_TrimAmountOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TrimAmountOptions::QG_TrimAmountOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TrimAmountOptions::~QG_TrimAmountOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TrimAmountOptions::languageChange()
{
    retranslateUi(this);
}

void QG_TrimAmountOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/TrimAmount", leDist->text());
    RS_SETTINGS->endGroup();
}

void QG_TrimAmountOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyTrimAmount) {
        action = (RS_ActionModifyTrimAmount*)a;

        QString sd;
        // settings from action:
        if (update) {
            sd = QString("%1").arg(action->getDistance());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Modify");
            sd = RS_SETTINGS->readEntry("/TrimAmount", "1.0");
            RS_SETTINGS->endGroup();
        }

        leDist->setText(sd);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ModifyTrimAmountOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_TrimAmountOptions::updateDist(const QString& d) {
    if (action!=NULL) {
        action->setDistance(RS_Math::eval(d, 1.0));
    }
}
