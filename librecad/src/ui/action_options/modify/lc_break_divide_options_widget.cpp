/****************************************************************************
**
* Options widget for ModifyBreakDivide action that breaks line, arc or circle
* to segments by points of intersection with other entities.

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
**********************************************************************/
#include "lc_break_divide_options_widget.h"

#include "lc_action_modify_break_divide.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_break_divide_options_widget.h"

LC_BreakDivideOptionsWidget::LC_BreakDivideOptionsWidget() : ui(new Ui::LC_BreakDivideOptionsWidget) {
    ui->setupUi(this);
    connect(ui->cbRemoveSegments, &QCheckBox::clicked, this, &LC_BreakDivideOptionsWidget::onRemoveSegmentsClicked);
    connect(ui->cbRemoveSelected, &QCheckBox::clicked, this, &LC_BreakDivideOptionsWidget::onRemoveSelectedClicked);
}

void LC_BreakDivideOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyBreakDivide*>(a);

    const bool removeSelected = m_action->isRemoveSelected();
    const bool removeSegments = m_action->isRemoveSegment();

    LC_GuardedSignalsBlocker({ui->cbRemoveSegments, ui->cbRemoveSelected});

    ui->cbRemoveSegments->setChecked(removeSegments);
    ui->cbRemoveSelected->setEnabled(removeSegments);
    ui->cbRemoveSelected->setChecked(removeSelected);
}

void LC_BreakDivideOptionsWidget::onRemoveSegmentsClicked(const bool clicked) const {
    m_action->setRemoveSegment(clicked);
    m_action->updateOptions();
}

void LC_BreakDivideOptionsWidget::onRemoveSelectedClicked(const bool clicked) const {
    m_action->setRemoveSelected(clicked);
    m_action->updateOptions();
}

void LC_BreakDivideOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
