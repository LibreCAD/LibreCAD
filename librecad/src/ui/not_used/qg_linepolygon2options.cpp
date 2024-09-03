/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/
#include "qg_linepolygon2options.h"

#include "rs_actiondrawlinepolygon2.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "ui_qg_linepolygon2options.h"

/*
 *  Constructs a QG_LinePolygon2Options as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LinePolygon2Options::QG_LinePolygon2Options(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_LinePolygon2Options{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LinePolygon2Options::~QG_LinePolygon2Options() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LinePolygon2Options::languageChange()
{
	ui->retranslateUi(this);
}

void QG_LinePolygon2Options::saveSettings() {
    LC_SET_ONE("Draw", "LinePolygon2Number", ui->sbNumber->text());
}

void QG_LinePolygon2Options::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawLinePolygonCorCor) {
        action = static_cast<RS_ActionDrawLinePolygonCorCor*>(a);

        QString sn;
        if (update) {
            sn = QString("%1").arg(action->getNumber());
        } else {
            sn = LC_GET_ONE_STR("Draw", "LinePolygon2Number", "3");
        }
        ui->sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LinePolygon2Options::setAction: wrong action type");
        action = nullptr;
    }

}

void QG_LinePolygon2Options::updateNumber(int n) {
    if (action) {
        action->setNumber(n);
        saveSettings();
    }
}
