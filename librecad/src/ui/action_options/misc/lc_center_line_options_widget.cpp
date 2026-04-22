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

#include "lc_center_line_options_widget.h"

#include "lc_action_draw_center_line.h"
#include "lc_guarded_signals_blocker.h"
#include "lc_guardedconnectionslist.h"
#include "ui_lc_center_line_options_widget.h"

LC_CenterLineOptionsWidget::LC_CenterLineOptionsWidget()
    :ui(new Ui::LC_CenterLineOptionsWidget){
    ui->setupUi(this);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_CenterLineOptionsWidget::onOffsetEditingFinished);
    pickDistanceSetup("offset", ui->tbPickOffset, ui->leOffset);
}

LC_CenterLineOptionsWidget::~LC_CenterLineOptionsWidget(){
    delete ui;
}

void LC_CenterLineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_CenterLineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawCenterLine*>(a);
    const QString offset = fromDouble(m_action->getOffset());
    LC_GuardedSignalsBlocker({ui->leOffset});
    ui->leOffset->setText(offset);
}

void LC_CenterLineOptionsWidget::onOffsetEditingFinished() {
    const QString& expr = ui->leOffset->text();
    double value = 0.;
    if (toDouble(expr, value, 0.0, false)) {
        m_action->setOffset(value);
    }
    m_action->updateOptions();
}
