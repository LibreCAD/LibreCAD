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

#include "lc_draw_bounding_box_options_widget.h"

#include "lc_action_draw_bounding_box.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_draw_bounding_box_options_widget.h"

LC_DrawBoundingBoxOptionsWidget::LC_DrawBoundingBoxOptionsWidget()
    : ui(new Ui::LC_DrawBoundingBoxOptionsWidget) {
    ui->setupUi(this);
    connect(ui->cbAsGroup, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptionsWidget::onAsGroupToggled);
    connect(ui->cbCornerPointsOnly, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptionsWidget::onCornerPointsToggled);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptionsWidget::onPolylineToggled);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_DrawBoundingBoxOptionsWidget::onOffsetEditingFinished);

    pickDistanceSetup("offset", ui->tbPickOffset, ui->leOffset);
}

LC_DrawBoundingBoxOptionsWidget::~LC_DrawBoundingBoxOptionsWidget() {
    delete ui;
}

void LC_DrawBoundingBoxOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawBoundingBox*>(a);

    const bool asGroup = m_action->isSelectionAsGroup();
    const bool cornerPoints = m_action->isCornerPointsOnly();
    const bool polyline = m_action->isCreatePolyline();
    const QString offset = fromDouble(m_action->getOffset());

    LC_GuardedSignalsBlocker({ui->cbAsGroup, ui->cbCornerPointsOnly, ui->cbPolyline, ui->leOffset});

    ui->cbAsGroup->setChecked(asGroup);
    ui->cbCornerPointsOnly->setChecked(cornerPoints);
    ui->cbPolyline->setEnabled(!cornerPoints);
    ui->cbPolyline->setChecked(polyline);
    ui->leOffset->setText(offset);
}

void LC_DrawBoundingBoxOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_DrawBoundingBoxOptionsWidget::onAsGroupToggled([[maybe_unused]] bool val) {
    m_action->setSelectionAsGroup(ui->cbAsGroup->isChecked());
    m_action->updateOptions();
}

void LC_DrawBoundingBoxOptionsWidget::onCornerPointsToggled([[maybe_unused]] bool val) {
    m_action->setCornersOnly(ui->cbCornerPointsOnly->isChecked());
    m_action->updateOptions();
}

void LC_DrawBoundingBoxOptionsWidget::onPolylineToggled([[maybe_unused]] bool val) {
    m_action->setCreatePolyline(ui->cbPolyline->isChecked());
    m_action->updateOptions();
}

void LC_DrawBoundingBoxOptionsWidget::onOffsetEditingFinished() {
    const QString& expr = ui->leOffset->text();
    double value = 0.;
    if (toDouble(expr, value, 0.0, false)) {
        m_action->setOffset(value);
    }
    m_action->updateOptions();
}
