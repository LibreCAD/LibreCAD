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

#include "lc_stretch_options_widget.h"

#include "lc_action_modify_stretch.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_stretch_options_widget.h"

LC_StretchOptionsWidget::LC_StretchOptionsWidget(): ui(new Ui::LC_StretchOptionsWidget){
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_StretchOptionsWidget::onKeepOriginalsClicked);
}

LC_StretchOptionsWidget::~LC_StretchOptionsWidget(){
    delete ui;
    m_action = nullptr;
}

void LC_StretchOptionsWidget::doUpdateByAction(RS_ActionInterface *a) {
    m_action = static_cast<LC_ActionModifyStretch *>(a);
    const bool keepOriginals = !m_action->isRemoveOriginals();

    LC_GuardedSignalsBlocker({ui->cbKeepOriginals});
    ui->cbKeepOriginals->setChecked(keepOriginals);
}

void LC_StretchOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_StretchOptionsWidget::onKeepOriginalsClicked(const bool val) {
    m_action->setRemoveOriginals(!val);
    m_action->updateOptions();
}
