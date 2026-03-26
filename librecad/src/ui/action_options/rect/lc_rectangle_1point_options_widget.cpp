/****************************************************************************
**
* Options widget for "Rectangle1Point" action.

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
#include "lc_rectangle_1point_options_widget.h"

#include "lc_action_draw_rectangle_1point.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_rectangle_1point_options_widget.h"

LC_Rectangle1PointOptionsWidget::LC_Rectangle1PointOptionsWidget() : ui(new Ui::LC_Rectangle1PointOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leWidth, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onWidthEditingFinished);
    connect(ui->leHeight, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onHeightEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptionsWidget::onAngleEditingFinished);
    connect(ui->cbCorners, &QComboBox::currentIndexChanged, this, &LC_Rectangle1PointOptionsWidget::onCornersIndexChanged);
    connect(ui->cbSnapPoint, &QComboBox::currentIndexChanged, this, &LC_Rectangle1PointOptionsWidget::onSnapPointIndexChanged);

    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_Rectangle1PointOptionsWidget::onUsePolylineClicked);
    connect(ui->cbSnapRadiusCenter, &QCheckBox::clicked, this, &LC_Rectangle1PointOptionsWidget::onSnapToCornerArcCenterClicked);
    connect(ui->cbInnerSize, &QCheckBox::clicked, this, &LC_Rectangle1PointOptionsWidget::onInnerSizeClicked);
    connect(ui->cbEdges, &QComboBox::currentIndexChanged, this, &LC_Rectangle1PointOptionsWidget::onEdgesIndexChanged);
    connect(ui->chkFixedBaseAngle, &QCheckBox::clicked, this, &LC_Rectangle1PointOptionsWidget::onBaseAngleFixedClicked);
    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_Rectangle1PointOptionsWidget::onFreeAngleClicked);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("width", ui->tbPickWidth, ui->leWidth);
    pickDistanceSetup("height", ui->tbPickHeight, ui->leHeight);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
    pickDistanceSetup("lengthY", ui->tbPickLenghtY, ui->leLenY);
    pickDistanceSetup("lengthX", ui->tbPickLengthX, ui->leX);
}

LC_Rectangle1PointOptionsWidget::~LC_Rectangle1PointOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_Rectangle1PointOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_Rectangle1PointOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawRectangle1Point*>(a);

    QString width;
    QString height;
    QString angle;
    QString radius;
    QString lenX;
    QString lenY;

    int cornersMode;
    int snapMode;
    bool usePolyline;
    bool snapRadiusCenter;
    bool sizeIsInner;
    int edges;
    bool hasBaseAngle;
    bool baseAngleIsFree;

    cornersMode = m_action->getCornersMode();
    snapMode = m_action->getInsertionPointSnapMode();
    usePolyline = m_action->isUsePolyline();
    edges = m_action->getEdgesDrawMode();

    const double w = m_action->getWidth();
    const double h = m_action->getHeight();
    const double an = m_action->getUcsAngleDegrees();
    const double r = m_action->getCornerRadius();
    const double lX = m_action->getCornerBevelLengthX();
    const double lY = m_action->getCornerBevelLengthY();

    width = fromDouble(w);
    height = fromDouble(h);
    angle = fromDouble(an);
    radius = fromDouble(r);
    lenX = fromDouble(lX);
    lenY = fromDouble(lY);
    snapRadiusCenter = m_action->isSnapToCornerArcCenter();
    sizeIsInner = m_action->isSizeInner();
    hasBaseAngle = m_action->hasBaseAngle();
    baseAngleIsFree = m_action->isBaseAngleFree();

    LC_GuardedSignalsBlocker({
        ui->leWidth,
        ui->leHeight,
        ui->leAngle,
        ui->leRadius,
        ui->leX,
        ui->leLenY,
        ui->cbCorners,
        ui->cbEdges,
        ui->cbFreeAngle,
        ui->cbInnerSize,
        ui->cbPolyline,
        ui->cbSnapPoint,
        ui->cbSnapRadiusCenter,
        ui->chkFixedBaseAngle,
    });

    ui->leWidth->setText(width);
    ui->leHeight->setText(height);
    ui->leAngle->setText(angle);
    ui->leRadius->setText(radius);
    ui->leX->setText(lenX);
    ui->leLenY->setText(lenY);

    const bool round = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_RADIUS;
    const bool bevel = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->tbPickRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->tbPickRadius->setVisible(round);

    ui->cbSnapRadiusCenter->setVisible(round);
    ui->cbInnerSize->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->tbPickLenghtY->setVisible(bevel);
    ui->leX->setVisible(bevel);
    ui->tbPickLengthX->setVisible(bevel);

    ui->cbCorners->setCurrentIndex(cornersMode);

    const bool straight = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_STRAIGHT;
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbSnapPoint->setCurrentIndex(snapMode);
    ui->cbPolyline->setChecked(usePolyline);
    ui->cbSnapRadiusCenter->setChecked(snapRadiusCenter);
    ui->cbInnerSize->setChecked(sizeIsInner);
    ui->cbEdges->setCurrentIndex(edges);
    ui->cbFreeAngle->setChecked(baseAngleIsFree);
    ui->leAngle->setEnabled(!baseAngleIsFree);
    ui->tbPickAngle->setEnabled(!baseAngleIsFree);

    ui->chkFixedBaseAngle->setChecked(hasBaseAngle);
    const bool angleEnabled = hasBaseAngle && !ui->cbFreeAngle->isChecked();
    ui->leAngle->setEnabled(angleEnabled);
    ui->tbPickAngle->setEnabled(angleEnabled);
    ui->cbFreeAngle->setEnabled(hasBaseAngle);
}

void LC_Rectangle1PointOptionsWidget::onCornersIndexChanged(const int index) {
    m_action->setCornersMode(index);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::updateUI(const int mode, [[maybe_unused]]const QVariant* value) {
    if (mode == UPDATE_ANGLE) {
        const double angle = m_action->getUcsAngleDegrees();
        ui->leAngle->blockSignals(true);
        ui->leAngle->setText(fromDouble(angle));
        ui->leAngle->blockSignals(false);
    }
}

void LC_Rectangle1PointOptionsWidget::onLenYEditingFinished() {
    const QString value = ui->leLenY->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerBevelLengthY(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onLenXEditingFinished() {
    const QString value = ui->leX->text();
    double x;
    if (toDouble(value, x, 1.0, true)) {
        m_action->setCornerBevelLengthX(x);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onRadiusEditingFinished() {
    const QString value = ui->leRadius->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerRadius(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onHeightEditingFinished() {
    const QString value = ui->leHeight->text();
    double y;
    if (toDouble(value, y, 0, true)) {
        m_action->setHeight(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onWidthEditingFinished() {
    const QString value = ui->leWidth->text();
    double y;
    if (toDouble(value, y, 0, true)) {
        m_action->setWidth(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onSnapPointIndexChanged(const int index) {
    m_action->setInsertionPointSnapMode(index);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onAngleEditingFinished() {
    const QString& val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setUcsAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onUsePolylineClicked(const bool value) {
    m_action->setUsePolyline(value);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onSnapToCornerArcCenterClicked(const bool value) {
    m_action->setSnapToCornerArcCenter(value);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onInnerSizeClicked(const bool value) {
    m_action->setSizeInner(value);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onFreeAngleClicked(const bool value) {
    m_action->setBaseAngleFree(value);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onBaseAngleFixedClicked(const bool value) {
    m_action->setBaseAngleFixed(value);
    m_action->updateOptions();
}

void LC_Rectangle1PointOptionsWidget::onEdgesIndexChanged(const int index) {
    m_action->setEdgesDrawMode(index);
    m_action->updateOptions();
}
