/****************************************************************************
**
* Options widget for "DrawCross" action.

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
#include "lc_center_mark_options_widget.h"

#include "lc_action_draw_center_mark.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_center_mark_options_widget.h"

LC_CenterMarkOptionsWidget::LC_CenterMarkOptionsWidget() : ui(new Ui::LC_CenterMarkOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_CenterMarkOptionsWidget::onXEditingFinished);
    connect(ui->leY, &QLineEdit::editingFinished, this, &LC_CenterMarkOptionsWidget::onYEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_CenterMarkOptionsWidget::onAngleEditingFinished);
    connect(ui->cbMode, &QComboBox::currentIndexChanged, this, &LC_CenterMarkOptionsWidget::onModeIndexChanged);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("x", ui->tbPickX, ui->leX);
    pickDistanceSetup("y", ui->tbPickY, ui->leY);
}

LC_CenterMarkOptionsWidget::~LC_CenterMarkOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_CenterMarkOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawCenterMark*>(a);

    const QString x = fromDouble(m_action->getLenX());
    const QString y = fromDouble(m_action->getLenY());
    const QString angle = fromDouble(m_action->getCrossAngleDegrees());
    const int mode = m_action->getCrossMode();

    LC_GuardedSignalsBlocker({ui->leX, ui->leY, ui->leAngle, ui->cbMode});

    ui->leX->setText(x);
    ui->leY->setText(y);
    ui->leAngle->setText(angle);
    ui->cbMode->setCurrentIndex(mode);
}

void LC_CenterMarkOptionsWidget::onXEditingFinished() {
    const QString& expr = ui->leX->text();
    double x;
    if (toDouble(expr, x, 1.0, true)) {
        m_action->setXLength(x);
    }
    m_action->updateOptions();
}

void LC_CenterMarkOptionsWidget::onYEditingFinished() {
    const QString& expr = ui->leY->text();
    double y;
    if (toDouble(expr, y, 1.0, true)) {
        m_action->setYLength(y);
    }
    m_action->updateOptions();
}

void LC_CenterMarkOptionsWidget::onAngleEditingFinished() {
    const QString& expr = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(expr, angle, 0.0, false)) {
        m_action->setCrossAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_CenterMarkOptionsWidget::onModeIndexChanged(const int index) const {
    m_action->setCrossMode(index);
    m_action->updateOptions();
}

void LC_CenterMarkOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
