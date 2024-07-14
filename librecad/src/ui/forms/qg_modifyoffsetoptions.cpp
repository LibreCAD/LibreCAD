/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "qg_modifyoffsetoptions.h"

#include "rs_math.h"
#include "ui_qg_modifyoffsetoptions.h"
#include "rs_actionmodifyoffset.h"

/*
 *  Constructs a QG_ModifyOffsetOptions
 */
QG_ModifyOffsetOptions::QG_ModifyOffsetOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyOffset, "/Draw", "/ModifyOffset")
    , ui(new Ui::Ui_ModifyOffsetOptions{}){
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ModifyOffsetOptions::~QG_ModifyOffsetOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ModifyOffsetOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_ModifyOffsetOptions::doSaveSettings() {
    save("Distance", ui->leDist->text());
    save("DistanceFixed", ui->cbFixedDistance->isChecked());
}

void QG_ModifyOffsetOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyOffset *>(a);

    QString dist;
    bool distanceFixed;
    bool ok;
    if (update) {
        dist = fromDouble(action->getDistance());
        distanceFixed = action->isFixedDistance();
    } else {
        dist = load("Distance", "1.0");
        distanceFixed = loadBool("DistanceFixed", true);
    }
    setDistanceToActionAndView(dist);
    setDistanceFixedToActionAndView(distanceFixed);
}

void QG_ModifyOffsetOptions::on_leDist_editingFinished() {
    setDistanceToActionAndView(ui->leDist->text());
}

void QG_ModifyOffsetOptions::on_cbFixedDistance_clicked(bool val) {
setDistanceFixedToActionAndView(val);
        }

void QG_ModifyOffsetOptions::setDistanceFixedToActionAndView(bool val) {
    action->setDistanceFixed(val);
    ui->leDist->setEnabled(val);
    ui->cbFixedDistance->setChecked(val);
}

void QG_ModifyOffsetOptions::setDistanceToActionAndView(QString val) {
    double distance;
    if (toDouble(val, distance, 1.0, false)) {
        action->setDistance(distance);
        ui->leDist->setText(fromDouble(distance));
    }
}