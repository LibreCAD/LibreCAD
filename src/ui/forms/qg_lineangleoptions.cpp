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
#include "qg_lineangleoptions.h"

#include <qvariant.h>

/*
 *  Constructs a QG_LineAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineAngleOptions::QG_LineAngleOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineAngleOptions::~QG_LineAngleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineAngleOptions::languageChange()
{
    retranslateUi(this);
}

void QG_LineAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineAngle) {
        action = (RS_ActionDrawLineAngle*)a;
        if (action->hasFixedAngle()) {
            lAngle->hide();
            leAngle->hide();
        }

        QString sa;
        QString sl;
		int sp;

        // settings from action:
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sl = QString("%1").arg(action->getLength());
			sp = action->getSnapPoint();
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry("/LineAngleAngle", "30.0");
            } else {
                sa = QString("%1").arg(action->getAngle());
            }
            sl = RS_SETTINGS->readEntry("/LineAngleLength", "10.0");
            sp = RS_SETTINGS->readNumEntry("/LineAngleSnapPoint", 0);
            RS_SETTINGS->endGroup();
			action->setSnapPoint(sp);
        }

        leAngle->setText(sa);
        leLength->setText(sl);
		cbSnapPoint->setCurrentItem(sp);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineAngleOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_LineAngleOptions::destroy() {
    if (action!=NULL) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!action->hasFixedAngle()) {
            RS_SETTINGS->writeEntry("/LineAngleAngle", RS_Math::rad2deg(action->getAngle()));
        }
        RS_SETTINGS->writeEntry("/LineAngleLength", action->getLength());
        RS_SETTINGS->writeEntry("/LineAngleSnapPoint", action->getSnapPoint());
        RS_SETTINGS->endGroup();
    }
}

void QG_LineAngleOptions::updateAngle(const QString& a) {
    if (action!=NULL && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_LineAngleOptions::updateLength(const QString& l) {
    if (action!=NULL) {
        action->setLength(RS_Math::eval(l));
    }
}

void QG_LineAngleOptions::updateSnapPoint(int sp) {
    if (action!=NULL) {
        action->setSnapPoint(sp);
    }
}
