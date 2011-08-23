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
#include "qg_linerelangleoptions.h"

#include <qvariant.h>

/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineRelAngleOptions::QG_LineRelAngleOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineRelAngleOptions::~QG_LineRelAngleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineRelAngleOptions::languageChange()
{
    retranslateUi(this);
}

void QG_LineRelAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineRelAngle) {
        action = (RS_ActionDrawLineRelAngle*)a;
        if (action->hasFixedAngle()) {
            lAngle->setDisabled(true);
            leAngle->setDisabled(true);
        }

        QString sa;
        QString sl;

        // settings from action:
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sl = QString("%1").arg(action->getLength());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry("/LineRelAngleAngle", "30.0");
            } else {
                sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            }
            sl = RS_SETTINGS->readEntry("/LineRelAngleLength", "10.0");
            RS_SETTINGS->endGroup();
        }

        leAngle->setText(sa);
        leLength->setText(sl);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineRelAngleOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_LineRelAngleOptions::destroy() {
    if (action!=NULL) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!action->hasFixedAngle()) {
            RS_SETTINGS->writeEntry("/LineRelAngleAngle", 
				RS_Math::rad2deg(action->getAngle()));
        }
        RS_SETTINGS->writeEntry("/LineRelAngleLength", action->getLength());
        RS_SETTINGS->endGroup();
    }
}

void QG_LineRelAngleOptions::updateAngle(const QString& a) {
    if (action!=NULL && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_LineRelAngleOptions::updateLength(const QString& l) {
    if (action!=NULL) {
        action->setLength(RS_Math::eval(l));
    }
}
