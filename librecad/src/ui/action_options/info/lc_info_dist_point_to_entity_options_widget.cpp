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

#include "lc_info_dist_point_to_entity_options_widget.h"

#include "lc_action_info_dist_point_to_entity.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_info_dist_point_to_entity_options_widget.h"

LC_InfoDistPointToEntityOptionsWidget::LC_InfoDistPointToEntityOptionsWidget()
    : ui(new Ui::LC_InfoDistPointToEntityOptionsWidget){
    ui->setupUi(this);
    connect(ui->cbOnEntity, &QCheckBox::clicked, this, &LC_InfoDistPointToEntityOptionsWidget::onOnEntityClicked);
}

LC_InfoDistPointToEntityOptionsWidget::~LC_InfoDistPointToEntityOptionsWidget(){
    delete ui;
    m_action = nullptr;
}

void LC_InfoDistPointToEntityOptionsWidget::doUpdateByAction(RS_ActionInterface *a){
    m_action = static_cast<LC_ActionInfoDistPointToEntity *>(a);
    const bool onEntity = m_action->isUseNearestPointOnEntity();
    LC_GuardedSignalsBlocker({ui->cbOnEntity});
    ui->cbOnEntity->setChecked(onEntity);
}

void LC_InfoDistPointToEntityOptionsWidget::onOnEntityClicked([[maybe_unused]]bool value) const {
      m_action->setUseNearestPointOnEntity(ui->cbOnEntity->isChecked());
}

void LC_InfoDistPointToEntityOptionsWidget::languageChange(){
    ui->retranslateUi(this);
}
