/****************************************************************************
**
* Options widget for "CircleByArc" action.

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

#include "lc_circle_by_arc_options_widget.h"

#include "lc_action_draw_circle_by_arc.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_circle_by_arc_options_widget.h"

LC_CircleByArcOptionsWidget::LC_CircleByArcOptionsWidget() : ui(new Ui::LC_CircleByArcOptionsWidget), m_action(nullptr) {
    ui->setupUi(this);
    connect(ui->cbReplace, &QCheckBox::clicked, this, &LC_CircleByArcOptionsWidget::onReplaceClicked);
    connect(ui->cbPen, &QComboBox::currentIndexChanged, this, &LC_CircleByArcOptionsWidget::onPenModeIndexChanged);
    connect(ui->cbLayer, &QComboBox::currentIndexChanged, this, &LC_CircleByArcOptionsWidget::onLayerModeIndexChanged);
    connect(ui->leRadiusShift, &QLineEdit::editingFinished, this, &LC_CircleByArcOptionsWidget::onRadiusShiftEditingFinished);

    pickDistanceSetup("radiusShift", ui->tbPickRadius, ui->leRadiusShift);
}

LC_CircleByArcOptionsWidget::~LC_CircleByArcOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_CircleByArcOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawCircleByArc*>(a);

    const bool replace = m_action->isReplaceArcByCircle();
    const int penMode = m_action->getPenMode();
    const int layerMode = m_action->getLayerMode();
    const QString radiusShift = fromDouble(m_action->getRadiusShift());

    LC_GuardedSignalsBlocker({ui->cbReplace, ui->leRadiusShift, ui->cbPen, ui->cbLayer, ui->leRadiusShift});

    ui->cbReplace->setChecked(replace);

    ui->leRadiusShift->setEnabled(!replace);
    ui->lblRadiusShift->setEnabled(!replace);
    ui->tbPickRadius->setEnabled(!replace);

    ui->cbPen->setCurrentIndex(penMode);
    ui->cbLayer->setCurrentIndex(layerMode);
    ui->leRadiusShift->setText(radiusShift);
}

void LC_CircleByArcOptionsWidget::onPenModeIndexChanged(const int mode) const {
    m_action->setPenMode(mode);
    m_action->updateOptions();
}

void LC_CircleByArcOptionsWidget::onLayerModeIndexChanged(const int mode) const {
    m_action->setLayerMode(mode);
    m_action->updateOptions();
}

void LC_CircleByArcOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_CircleByArcOptionsWidget::onReplaceClicked(const bool value) const {
    m_action->setReplaceArcByCircle(value);
    m_action->updateOptions();
}


void LC_CircleByArcOptionsWidget::onRadiusShiftEditingFinished() {
    const QString& val = ui->leRadiusShift->text();
    double len;
    if (toDouble(val, len, 0.0, false)) {
        m_action->setRadiusShift(len);
    }
    m_action->updateOptions();
}
