/****************************************************************************
**
* Options widget for "DrawStar" action.

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
#include "lc_star_options_widget.h"

#include "lc_action_draw_star.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_star_options_widget.h"

LC_StarOptionsWidget::LC_StarOptionsWidget() : ui(new Ui::LC_StarOptionsWidget) {
    ui->setupUi(this);

    ui->sbNumber->setMaximum(STAR_MIN_RAYS);
    ui->sbNumber->setMaximum(STAR_MAX_RAYS);

    connect(ui->cbSymmertix, &QCheckBox::clicked, this, &LC_StarOptionsWidget::onSymmetricClicked);
    connect(ui->cbRadiusInner, &QCheckBox::clicked, this, &LC_StarOptionsWidget::onRadiusInnerClicked);
    connect(ui->leRadiusInner, &QLineEdit::editingFinished, this, &LC_StarOptionsWidget::onRadiusInnerEditingFinished);
    connect(ui->cbRadiusOuter, &QCheckBox::clicked, this, &LC_StarOptionsWidget::onRadiusOuterClicked);
    connect(ui->leRadusOuter, &QLineEdit::editingFinished, this, &LC_StarOptionsWidget::onRadiusOuterEditingFinished);
    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_StarOptionsWidget::onPolylineClicked);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_StarOptionsWidget::onNumberChanged);
    pickDistanceSetup("radiusOuter", ui->tbPickRadiusOuter, ui->leRadusOuter);
    pickDistanceSetup("radiusInner", ui->tbPickRadiusInner, ui->leRadiusInner);
}

LC_StarOptionsWidget::~LC_StarOptionsWidget() {
    m_action = nullptr;
    delete ui;
}

void LC_StarOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_StarOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawStar*>(a);

    const bool polyline = m_action->isPolyline();
    const bool innerEnabled = m_action->isInnerRounded();
    const bool outerEnabled = m_action->isOuterRounded();
    const int number = m_action->getRaysNumber();
    const bool symmetric = m_action->isSymmetric();
    const QString innerRadius = fromDouble(m_action->getRadiusInner());
    const QString outerRadius = fromDouble(m_action->getRadiusOuter());

    LC_GuardedSignalsBlocker({
        ui->sbNumber,
        ui->cbRadiusInner,
        ui->leRadiusInner,
        ui->cbRadiusOuter,
        ui->leRadusOuter,
        ui->cbSymmertix,
        ui->cbPolyline
    });
    ui->sbNumber->setValue(number);
    ui->cbRadiusInner->setChecked(innerEnabled);
    ui->leRadiusInner->setEnabled(innerEnabled);
    ui->tbPickRadiusInner->setEnabled(innerEnabled);
    ui->leRadiusInner->setText(innerRadius);

    ui->cbRadiusOuter->setChecked(outerEnabled);
    ui->leRadusOuter->setEnabled(outerEnabled);
    ui->tbPickRadiusOuter->setEnabled(outerEnabled);
    ui->leRadusOuter->setText(outerRadius);
    ui->cbSymmertix->setChecked(symmetric);
    ui->cbPolyline->setChecked(polyline);
}

void LC_StarOptionsWidget::onSymmetricClicked(const bool value) {
    m_action->setSymmetric(value);
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onRadiusOuterEditingFinished() {
    const auto value = ui->leRadusOuter->text();
    double y;
    if (toDouble(value, y, 0.0, true)) {
        m_action->setRadiusOuter(y);
    }
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onRadiusInnerEditingFinished() {
    const auto value = ui->leRadiusInner->text();
    double y;
    if (toDouble(value, y, 0.0, true)) {
        m_action->setRadiusInner(y);
    }
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onRadiusInnerClicked(const bool value) {
    m_action->setInnerRounded(value);
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onRadiusOuterClicked(const bool value) {
    m_action->setOuterRounded(value);
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onNumberChanged(const int value) {
    m_action->setRaysNumber(value);
    m_action->updateOptions();
}

void LC_StarOptionsWidget::onPolylineClicked(const bool value) {
    m_action->setPolyline(value);
    m_action->updateOptions();
}
