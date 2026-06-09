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

#include "lc_ellipse_1point_options_widget.h"

#include "lc_action_draw_ellipse_1point.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_ellipse_1point_options_widget.h"

LC_Ellipse1PointOptionsWidget::LC_Ellipse1PointOptionsWidget(): ui(new Ui::LC_Ellipse1PointOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptionsWidget::onAngleEditingFinished);
    connect(ui->leMajorRadius, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptionsWidget::onMajorRadiusEditingFinished);
    connect(ui->leMinorRadius, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptionsWidget::onMinorRadiusEditingFinished);
    connect(ui->cbAngle, &QCheckBox::toggled, this, &LC_Ellipse1PointOptionsWidget::onUseAngleClicked);
    connect(ui->cbFreeAngle, &QCheckBox::toggled, this, &LC_Ellipse1PointOptionsWidget::onFreeAngleClicked);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_Ellipse1PointOptionsWidget::onDirectionChanged);
    connect(ui->rbNeg, &QRadioButton::toggled, this, &LC_Ellipse1PointOptionsWidget::onDirectionChanged);

    pickDistanceSetup("major", ui->tbPickMajor, ui->leMajorRadius);
    pickDistanceSetup("minor", ui->tbPickMinor, ui->leMinorRadius);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

LC_Ellipse1PointOptionsWidget::~LC_Ellipse1PointOptionsWidget() {
    delete ui;
}

void LC_Ellipse1PointOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = dynamic_cast<LC_ActionDrawEllipse1Point*>(a);

    const bool arcAction = m_action->rtti() == RS2::ActionDrawEllipseArc1Point;

    const QString majorRadius = fromDouble(m_action->getMajorRadius());
    const QString minorRadius = fromDouble(m_action->getMinorRadius());
    const QString angle = fromDouble(m_action->getUcsMajorAngleDegrees());
    const bool useAngle = m_action->hasAngle();
    const bool freeAngle = m_action->isAngleFree();
    bool negativeDirection = false;
    if (arcAction) {
        negativeDirection = m_action->isReversed();
    }

    LC_GuardedSignalsBlocker({ui->leMajorRadius, ui->leMinorRadius, ui->leAngle, ui->cbFreeAngle, ui->cbAngle, ui->rbNeg, ui->rbPos});

    ui->leMajorRadius->setText(majorRadius);
    ui->leMinorRadius->setText(minorRadius);
    ui->leAngle->setText(angle);

    ui->cbFreeAngle->setChecked(freeAngle);
    ui->leAngle->setEnabled(!freeAngle);
    ui->tbPickAngle->setEnabled(!freeAngle);

    ui->cbAngle->setChecked(useAngle);
    const bool angleInputAllowed = useAngle && !ui->cbFreeAngle->isChecked();
    ui->leAngle->setEnabled(angleInputAllowed);
    ui->tbPickAngle->setEnabled(angleInputAllowed);
    ui->cbFreeAngle->setEnabled(useAngle);

    ui->rbNeg->setChecked(negativeDirection);
    ui->rbPos->setChecked(!negativeDirection);

    ui->rbNeg->setVisible(arcAction);
    ui->rbPos->setVisible(arcAction);
}

void LC_Ellipse1PointOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_Ellipse1PointOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double y;
    if (toDoubleAngleDegrees(val, y, 0, false)) {
        m_action->setUcsMajorAngleDegrees(y);
    }
    m_action->updateOptions();
}

void LC_Ellipse1PointOptionsWidget::onMajorRadiusEditingFinished() {
    const auto val = ui->leMajorRadius->text();
    double y;
    if (toDouble(val, y, 1, true)) {
        m_action->setMajorRadius(y);
    }
    m_action->updateOptions();
}

void LC_Ellipse1PointOptionsWidget::onMinorRadiusEditingFinished() {
    const auto val = ui->leMinorRadius->text();
    double y;
    if (toDouble(val, y, 1, true)) {
        m_action->setMinorRadius(y);
    }
    m_action->updateOptions();
}

void LC_Ellipse1PointOptionsWidget::onUseAngleClicked([[maybe_unused]] bool val) const {
    m_action->setHasAngle(ui->cbAngle->isChecked());
    m_action->updateOptions();
}

void LC_Ellipse1PointOptionsWidget::onFreeAngleClicked([[maybe_unused]] bool val) const {
    m_action->setAngleFree(ui->cbFreeAngle->isChecked());
    m_action->updateOptions();
}

void LC_Ellipse1PointOptionsWidget::onDirectionChanged([[maybe_unused]] bool val) const {
    const bool negative = ui->rbNeg->isChecked();
    m_action->setReversed(negative);
}
