/****************************************************************************
**
* Options widget for "Rectangle2Points" action.

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
#include "lc_rectangle_2points_options_widget.h"

#include "lc_action_draw_rectangle_2points.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_rectangle_2points_options_widget.h"

LC_Rectangle2PointsOptionsWidget::LC_Rectangle2PointsOptionsWidget() : ui(new Ui::LC_Rectangle2PointsOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptionsWidget::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptionsWidget::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptionsWidget::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptionsWidget::onAngleEditingFinished);
    connect(ui->cbCorners, &QComboBox::currentIndexChanged, this, &LC_Rectangle2PointsOptionsWidget::onCornersIndexChanged);
    connect(ui->cbSnapStart, &QComboBox::currentIndexChanged, this, &LC_Rectangle2PointsOptionsWidget::onInsertionPointSnapIndexChanged);
    connect(ui->cbSnapEnd, &QComboBox::currentIndexChanged, this, &LC_Rectangle2PointsOptionsWidget::onSecondPointSnapIndexChanged);
    connect(ui->cbEdges, &QComboBox::currentIndexChanged, this, &LC_Rectangle2PointsOptionsWidget::onEdgesIndexChanged);
    connect(ui->chkFixedBaseAngle, &QCheckBox::clicked, this, &LC_Rectangle2PointsOptionsWidget::onBaseAngleFixedClicked);

    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_Rectangle2PointsOptionsWidget::onUsePolylineClicked);
    connect(ui->cbSnapRadiusCenter, &QCheckBox::clicked, this, &LC_Rectangle2PointsOptionsWidget::onSnapToCornerArcCenterClicked);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
    pickDistanceSetup("lengthY", ui->tbPickLengthY, ui->leLenY);
    pickDistanceSetup("lengthX", ui->tbPickLengthX, ui->leX);
}

LC_Rectangle2PointsOptionsWidget::~LC_Rectangle2PointsOptionsWidget() {
    delete ui;
}

void LC_Rectangle2PointsOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_Rectangle2PointsOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawRectangle2Points*>(a);

    int cornersMode = 0;
    int insertSnapMode = 0;
    int secondPointSnapMode = 0;
    bool usePolyline = false;
    bool snapRadiusCenter = false;
    int edges = 0;
    bool isRotated = false;

    cornersMode = m_action->getCornersMode();
    insertSnapMode = m_action->getInsertionPointSnapMode();
    secondPointSnapMode = m_action->getSecondPointSnapMode();
    usePolyline = m_action->isUsePolyline();
    edges = m_action->getEdgesDrawMode();

    const double an = m_action->getUcsAngleDegrees();
    const double r = m_action->getCornerRadius();
    const double lX = m_action->getCornerBevelLengthX();
    const double lY = m_action->getCornerBevelLengthY();

    const QString angle = fromDouble(an);
    const QString radius = fromDouble(r);
    const QString lenX = fromDouble(lX);
    const QString lenY = fromDouble(lY);
    snapRadiusCenter = m_action->isSnapToCornerArcCenter();
    isRotated = m_action->hasBaseAngle();

    LC_GuardedSignalsBlocker({
      ui->leAngle,
      ui->leRadius,
      ui->leX,
      ui->leLenY,
      ui->cbCorners,
      ui->cbEdges,
      ui->cbPolyline,
      ui->cbSnapStart,
      ui->cbSnapEnd,
      ui->cbSnapRadiusCenter,
      ui->chkFixedBaseAngle,
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

    const bool straight = cornersMode == LC_ActionDrawRectangleAbstract::CORNER_STRAIGHT;
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbCorners->setCurrentIndex(cornersMode);
    ui->cbSnapStart->setCurrentIndex(insertSnapMode);
    ui->cbSnapEnd->setCurrentIndex(secondPointSnapMode);
    ui->cbPolyline->setChecked(usePolyline);
    ui->cbSnapRadiusCenter->setChecked(snapRadiusCenter);
    ui->cbEdges->setCurrentIndex(edges);

    ui->chkFixedBaseAngle->setChecked(isRotated);
    ui->leAngle->setEnabled(isRotated);
    ui->tbPickAngle->setEnabled(isRotated);
}

void LC_Rectangle2PointsOptionsWidget::onCornersIndexChanged(const int index) {
    m_action->setCornersMode(index);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onLenYEditingFinished() {
    const QString value = ui->leLenY->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerBevelLengthY(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onLenXEditingFinished() {
    const QString value = ui->leX->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerBevelLengthX(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onRadiusEditingFinished() {
    const QString value = ui->leRadius->text();
    double y;
    if (toDouble(value, y, 1.0, true)) {
        m_action->setCornerRadius(y);
    }
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onInsertionPointSnapIndexChanged(const int index) {
    m_action->setInsertionPointSnapMode(index);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onSecondPointSnapIndexChanged(const int index) {
    m_action->setSecondPointSnapMode(index);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onEdgesIndexChanged(const int index) {
    m_action->setEdgesDrawMode(index);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onAngleEditingFinished() {
    const QString& val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setUcsAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onBaseAngleFixedClicked(const bool value) {
    m_action->setBaseAngleFixed(value);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onUsePolylineClicked(const bool value) {
    m_action->setUsePolyline(value);
    m_action->updateOptions();
}

void LC_Rectangle2PointsOptionsWidget::onSnapToCornerArcCenterClicked(const bool value) {
    m_action->setSnapToCornerArcCenter(value);
    m_action->updateOptions();
}
