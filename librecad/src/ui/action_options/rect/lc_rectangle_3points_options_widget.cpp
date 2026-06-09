/****************************************************************************
**
* Options widget for "Rectangle3Point" action.

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
#include "lc_rectangle_3points_options_widget.h"

#include "lc_action_draw_rectangle_3points.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_rectangle_3points_options_widget.h"

LC_Rectangle3PointsOptionsWidget::LC_Rectangle3PointsOptionsWidget() : ui(new Ui::LC_Rectangle3PointsOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptionsWidget::onAngleEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptionsWidget::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptionsWidget::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptionsWidget::onLenXEditingFinished);
    connect(ui->leInnerAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptionsWidget::onInnerAngleEditingFinished);
    connect(ui->cbCorners, &QComboBox::currentIndexChanged, this, &LC_Rectangle3PointsOptionsWidget::onCornersIndexChanged);
    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptionsWidget::onUsePolylineClicked);
    connect(ui->cbQuadrangle, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptionsWidget::onQuadrangleClicked);
    connect(ui->cbFixedInnerAngle, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptionsWidget::onInnerAngleFixedClicked);
    connect(ui->chkFixedBaseAngle, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptionsWidget::onBaseAngleFixedClicked);
    connect(ui->cbSnapRadiusCenter, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptionsWidget::onSnapToCornerArcCenterClicked);
    connect(ui->cbEdges, &QComboBox::currentIndexChanged, this, &LC_Rectangle3PointsOptionsWidget::onEdgesIndexChanged);

    pickAngleSetup("angleBase", ui->tbPickAngleBase, ui->leAngle);
    pickAngleSetup("angleInner", ui->tbPickAngleInner, ui->leInnerAngle);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
    pickDistanceSetup("lengthY", ui->tbPickLengthY, ui->leLenY);
    pickDistanceSetup("lengthX", ui->tbPickLengthX, ui->leX);
}

LC_Rectangle3PointsOptionsWidget::~LC_Rectangle3PointsOptionsWidget() {
    m_action = nullptr;
    delete ui;
}

void LC_Rectangle3PointsOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_Rectangle3PointsOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawRectangle3Points*>(a);

    const int cornersMode = m_action->getCornersMode();
    const bool usePolyline = m_action->isUsePolyline();

    const double an = m_action->getUcsAngleDegrees();
    const double r = m_action->getCornerRadius();
    const double lX = m_action->getCornerBevelLengthX();
    const double lY = m_action->getCornerBevelLengthY();

    const int edges = m_action->getEdgesDrawMode();
    const QString angle = fromDouble(an);
    const QString radius = fromDouble(r);
    const QString lenX = fromDouble(lX);
    const QString lenY = fromDouble(lY);
    const bool snapRadiusCenter = m_action->isSnapToCornerArcCenter();
    const QString innerAngle = fromDouble(m_action->getFixedInnerAngle());
    const bool quadrangle = m_action->isCreateQuadrangle();
    const bool fixedInnerAngle = m_action->isInnerAngleFixed();
    const bool fixedBaseAngle = m_action->hasBaseAngle();

    LC_GuardedSignalsBlocker({
        ui->leAngle,
        ui->leInnerAngle,
        ui->leRadius,
        ui->leX,
        ui->leLenY,
        ui->cbCorners,
        ui->cbEdges,
        ui->cbFixedInnerAngle,
        ui->chkFixedBaseAngle,
        ui->cbPolyline,
        ui->cbSnapRadiusCenter
    });
    ui->leAngle->setText(angle);
    ui->leRadius->setText(radius);
    ui->leX->setText(lenX);
    ui->leLenY->setText(lenY);

    const bool round = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_RADIUS;
    const bool bevel = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->tbPickRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->tbPickLengthY->setVisible(bevel);
    ui->leX->setVisible(bevel);
    ui->tbPickLengthX->setVisible(bevel);

    ui->line_2->setVisible(!quadrangle);

    const bool straight = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_STRAIGHT || quadrangle;
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbCorners->setCurrentIndex(cornersMode);

    ui->cbPolyline->setChecked(usePolyline);

    ui->cbSnapRadiusCenter->setChecked(snapRadiusCenter);

    ui->cbQuadrangle->setChecked(quadrangle);
    ui->frmQuad->setVisible(quadrangle);
    ui->frmRectSettings->setVisible(!quadrangle);

    ui->cbFixedInnerAngle->setChecked(fixedInnerAngle);
    ui->leInnerAngle->setEnabled(fixedInnerAngle);
    ui->tbPickAngleInner->setEnabled(fixedInnerAngle);

    ui->leInnerAngle->setText(innerAngle);
    ui->cbEdges->setCurrentIndex(edges);

    ui->chkFixedBaseAngle->setChecked(fixedBaseAngle);

    ui->leAngle->setEnabled(fixedBaseAngle);
    ui->tbPickAngleBase->setEnabled(fixedBaseAngle);
}

void LC_Rectangle3PointsOptionsWidget::onCornersIndexChanged(const int index) {
    m_action->setCornersMode(index);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onLenYEditingFinished() {
    const QString value = ui->leLenY->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerBevelLengthY(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onLenXEditingFinished() {
    const QString value = ui->leX->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerBevelLengthX(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onInnerAngleEditingFinished() {
    const QString value = ui->leInnerAngle->text();
    double y;
    if (toDoubleAngleDegrees(value, y, 1.0, true)) {
        m_action->setFixedInnerAngle(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onRadiusEditingFinished() {
    const QString value = ui->leRadius->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerRadius(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onAngleEditingFinished() {
    const QString& val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setUcsAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onUsePolylineClicked(const bool value) {
    m_action->setUsePolyline(value);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onSnapToCornerArcCenterClicked(const bool value) {
    m_action->setSnapToCornerArcCenter(value);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onQuadrangleClicked(const bool value) {
    m_action->setCreateQuadrangle(value);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onInnerAngleFixedClicked(const bool value) {
    m_action->setInnerAngleFixed(value);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onBaseAngleFixedClicked(const bool value) {
    m_action->setBaseAngleFixed(value);
    m_action->updateOptions();
}

void LC_Rectangle3PointsOptionsWidget::onEdgesIndexChanged(const int index) {
    m_action->setEdgesDrawMode(index);
    m_action->updateOptions();
}
