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
#include "qg_dimlinearoptions.h"

#include "rs_settings.h"

/*
 *  Constructs a QG_DimLinearOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimLinearOptions::QG_DimLinearOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimLinearOptions::~QG_DimLinearOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimLinearOptions::languageChange()
{
    retranslateUi(this);
}

void QG_DimLinearOptions::destroy() {
    RS_SETTINGS->beginGroup("/Dimension");
    RS_SETTINGS->writeEntry("/Angle", leAngle->text());
    RS_SETTINGS->endGroup();
}

void QG_DimLinearOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDimLinear) {
        action = (RS_ActionDimLinear*)a;

        QString sa;
        if (!update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            RS_SETTINGS->beginGroup("/Dimension");
            sa = RS_SETTINGS->readEntry("/Angle", "0.0");
            RS_SETTINGS->endGroup();
        }
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_DimLinearOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_DimLinearOptions::updateAngle(const QString & a) {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_DimLinearOptions::setHor() {
    leAngle->setText("0");
}

void QG_DimLinearOptions::setVer() {
    leAngle->setText("90");
}
