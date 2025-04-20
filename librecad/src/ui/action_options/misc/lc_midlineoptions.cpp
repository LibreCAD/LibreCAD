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

#include "lc_midlineoptions.h"
#include "lc_actiondrawmidline.h"
#include "ui_lc_midlineoptions.h"

LC_MidLineOptions::LC_MidLineOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawLineMiddle, "Draw", "LineMiddle")
    , ui(new Ui::LC_MidLineOptions){
    ui->setupUi(this);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_MidLineOptions::onOffsetEditingFinished);
}

LC_MidLineOptions::~LC_MidLineOptions(){
    delete ui;
}

void LC_MidLineOptions::doSaveSettings() {
    save("Offset", ui->leOffset->text());
}

void LC_MidLineOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_MidLineOptions::doSetAction(RS_ActionInterface* a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawMidLine*>(a);
    QString offset;
    if (update) {
        offset = fromDouble(m_action->getOffset());
    }
    else {
        offset = load("Offset", "0.0");
    }
    setOffsetToActionAndView(offset);
}

void LC_MidLineOptions::onOffsetEditingFinished() {
    const QString& expr = ui->leOffset->text();
    setOffsetToActionAndView(expr);
}

void LC_MidLineOptions::setOffsetToActionAndView(const QString& val) {
    double value = 0.;
    if (toDouble(val, value, 0.0, false)) {
        m_action->setOffset(value);
        ui->leOffset->setText(fromDouble(value));
    }
}
