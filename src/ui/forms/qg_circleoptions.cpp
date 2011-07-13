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
#include "qg_circleoptions.h"

#include "rs_settings.h"

/*
 *  Constructs a QG_CircleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CircleOptions::QG_CircleOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleOptions::~QG_CircleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleOptions::languageChange()
{
    retranslateUi(this);
}

void QG_CircleOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/CircleRadius", leRadius->text());
    RS_SETTINGS->endGroup();
}

void QG_CircleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawCircleCR) {
        action = (RS_ActionDrawCircleCR*)a;

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
        }
        leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CircleOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_CircleOptions::setData(RS_CircleData* d) {
    data = d;

    RS_SETTINGS->beginGroup("/Draw");
    QString r = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
    RS_SETTINGS->endGroup();

    leRadius->setText(r);
}*/

void QG_CircleOptions::updateRadius(const QString& r) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(r));
    }
}
