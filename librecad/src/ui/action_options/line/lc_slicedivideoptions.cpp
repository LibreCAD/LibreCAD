/****************************************************************************
**
* Options widget for "SliceDivide" action.

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
#include "lc_slicedivideoptions.h"
#include "lc_actiondrawslicedivide.h"
#include "ui_lc_slicedivideoptions.h"

LC_SliceDivideOptions::LC_SliceDivideOptions() :
    LC_ActionOptionsWidget(nullptr),
    ui(new Ui::LC_SliceDivideOptions),
    m_action(nullptr){
    ui->setupUi(this);

    connect(ui->sbCount, &QSpinBox::valueChanged, this, &LC_SliceDivideOptions::onCountChanged);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onDistanceEditingFinished);
    connect(ui->leTickLengh, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickLengthEditingFinished);
    connect(ui->leTickOffset, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickOffsetEditingFinished);
    connect(ui->leTickAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickAngleEditingFinished);
    connect(ui->leCircleStartAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onCircleStartAngleEditingFinished);
    connect(ui->cbEdgeTick, &QComboBox::currentIndexChanged,this, &LC_SliceDivideOptions::onDrawTickOnEdgesIndexChanged);
    connect(ui->cbTickSnap, &QComboBox::currentIndexChanged, this, &LC_SliceDivideOptions::onTickSnapIndexChanged);
    connect(ui->cbRelAngle, &QCheckBox::clicked, this, &LC_SliceDivideOptions::onRelAngleClicked);
    connect(ui->cbDivide, &QCheckBox::clicked, this, &LC_SliceDivideOptions::onDivideClicked);
    connect(ui->cbMode, &QCheckBox::clicked, this, &LC_SliceDivideOptions::onModeClicked);
}

LC_SliceDivideOptions::~LC_SliceDivideOptions(){
    m_action = nullptr;
    delete ui;
}

bool LC_SliceDivideOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawSliceDivideLine || actionType == RS2::ActionDrawSliceDivideCircle;
}

QString LC_SliceDivideOptions::getSettingsOptionNamePrefix(){
    if (m_forCircle){
      return "SliceDivideCircle";
    }
    else{
        return "SliceDivideLine";
    }
}

// just provide indication to the user that some options are not applicable for selected entity
void LC_SliceDivideOptions::updateUI(int mode){
    switch (mode){
        case LC_ActionDrawSliceDivide::SELECTION_NONE:
            ui->frmCircle->setEnabled(true);
            ui->frmEdge->setEnabled(true);
            break;
        case LC_ActionDrawSliceDivide::SELECTION_ARC: // arc
            ui->frmCircle->setEnabled(false);
            ui->frmEdge->setEnabled(true);
            break;
        case LC_ActionDrawSliceDivide::SELECTION_CIRCLE: // circle
            ui->frmCircle->setEnabled(true);
            ui->frmEdge->setEnabled(false);
            break;
        default:
            break;
    }
}

void LC_SliceDivideOptions::doSetAction(RS_ActionInterface *a, bool update){
        m_action = dynamic_cast<LC_ActionDrawSliceDivide *>(a);

        m_forCircle = a->rtti() == RS2::ActionDrawSliceDivideCircle;

        int count;
        QString tickLen;
        QString tickOffset;
        QString tickAngle;
        QString circleStartAngle;
        QString distance;
        int drawEdgesMode;
        int tickSnapMode;
        bool tickAngleRelative;
        bool divide;
        bool fixedDistance;

        if (update){
            count = m_action->getTickCount();
            tickLen = fromDouble(m_action->getTickLength());
            tickOffset = fromDouble(m_action->getTickOffset());
            tickAngle = fromDouble(m_action->getTickAngle());
            circleStartAngle = fromDouble(m_action->getCircleStartAngle());
            distance = fromDouble(m_action->getDistance());
            tickSnapMode = m_action->getTickSnapMode();
            drawEdgesMode = m_action->getDrawTickOnEdgeMode();
            tickAngleRelative = m_action->isTickAngleRelative();
            divide = m_action->isDivideEntity();
            fixedDistance = m_action->isFixedDistance();
        } else {            
            count = loadInt("Count", 1);
            tickLen = load("Length", "1.0");
            tickOffset = load("Offset", "0.0");
            tickAngle = load("Angle", "90.0");
            tickSnapMode = loadInt("TickSnap", 0);
            drawEdgesMode = loadInt("TickEdgeMode", 1);
            circleStartAngle = load("CircleStartAngle", "0.0");
            tickAngleRelative = loadBool("LengthTickAngleRel", true);
            divide = loadBool("DoDivide", true);
            distance = load("Distance", "1.0");
            fixedDistance = loadBool("FixedDistance", false);
        }
        setCountToActionAndView(count);
        setTickLengthToActionAndView(tickLen);
        setTickOffsetToActionAndView(tickOffset);
        setTickAngleToActionAndView(tickAngle);
        setCircleStartAngleToActionAndView(circleStartAngle);
        setTicksSnapModeToActionAndView(tickSnapMode);
        setDrawEdgesTicksModeToActionAndView(drawEdgesMode);
        setTickAngleRelativeToActionAndView(tickAngleRelative);
        setDivideFlagToActionAndView(divide);
        setFixedDistanceFlagToActionAndView(fixedDistance);
        setDistanceToActionAndView(distance);

        ui->frmCircle->setVisible(m_forCircle);
        ui->cbMode->setVisible(!m_forCircle);
        if (m_forCircle){
            ui->frmDistance->setVisible(false);
        }
}

void LC_SliceDivideOptions::doSaveSettings(){    
    save("Count", ui->sbCount->value());
    save("Length", ui->leTickLengh->text());
    save("Offset", ui->leTickOffset->text());
    save("Angle", ui->leTickAngle->text());
    save("TickSnap", ui->cbTickSnap->currentIndex());
    save("TickEdgeMode", ui->cbEdgeTick->currentIndex());
    save("CircleStartAngle", ui->leCircleStartAngle->text());
    save("LengthTickAngleRel", ui->cbRelAngle->isChecked());
    save("DoDivide", ui->cbDivide->isChecked());
    save("Distance", ui->leDistance->text());
    save("FixedDistance", ui->cbMode->isChecked());
}

void LC_SliceDivideOptions::onCountChanged(int value){
    if (m_action != nullptr){
        setCountToActionAndView(value);
    }
}

void LC_SliceDivideOptions::onDistanceEditingFinished(){
    const QString &expr = ui->leDistance->text();
    setDistanceToActionAndView(expr);
}

void LC_SliceDivideOptions::onTickLengthEditingFinished(){
    const QString &expr = ui->leTickLengh->text();
    setTickLengthToActionAndView(expr);
}

void LC_SliceDivideOptions::onTickAngleEditingFinished(){
    const QString &expr = ui->leTickAngle->text();
    setTickAngleToActionAndView(expr);
}

void LC_SliceDivideOptions::onTickOffsetEditingFinished(){
    const QString &expr = ui->leTickOffset->text();
    setTickOffsetToActionAndView(expr);
}

void LC_SliceDivideOptions::onCircleStartAngleEditingFinished(){
    const QString &expr = ui->leCircleStartAngle->text();
    setCircleStartAngleToActionAndView(expr);
}

void LC_SliceDivideOptions::onDrawTickOnEdgesIndexChanged(int index){
    setDrawEdgesTicksModeToActionAndView(index);
}

void LC_SliceDivideOptions::onRelAngleClicked(bool checked){
    setTickAngleRelativeToActionAndView(checked);
}

void LC_SliceDivideOptions::onDivideClicked(bool checked){
    setDivideFlagToActionAndView(checked);
}
void LC_SliceDivideOptions::onModeClicked(bool checked){
    setFixedDistanceFlagToActionAndView (checked);
}

void LC_SliceDivideOptions::onTickSnapIndexChanged(int index){
    setTicksSnapModeToActionAndView(index);
}

void LC_SliceDivideOptions::setDrawEdgesTicksModeToActionAndView(int index){
    m_action->setDrawTickOnEdgeMode(index);
    ui->cbEdgeTick->setCurrentIndex(index);
}

void LC_SliceDivideOptions::setTicksSnapModeToActionAndView(int index){
    m_action->setTickSnapMode(index);
    ui->cbTickSnap->setCurrentIndex(index);
}

void LC_SliceDivideOptions::setTickAngleRelativeToActionAndView(bool relative){
    m_action->setTickAngleRelative(relative);
    ui->cbRelAngle->setChecked(relative);
}

void LC_SliceDivideOptions::setDivideFlagToActionAndView(bool value){
    m_action->setDivideEntity(value);
    ui->cbDivide->setChecked(value);
}

void LC_SliceDivideOptions::setFixedDistanceFlagToActionAndView(bool value){
    m_action->setFixedDistance(value);
    ui->cbMode->setChecked(value);
    ui->frmCount->setVisible(!value);
    ui->frmDistance->setVisible(value);
}

void LC_SliceDivideOptions::setCountToActionAndView(int val){
  m_action->setTickCount(val);
  ui->sbCount->setValue(val);
}

void LC_SliceDivideOptions::setDistanceToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 1.0, true)){
        m_action->setDistance(value);
        ui->leDistance->setText(fromDouble(value));
    }
}

void LC_SliceDivideOptions::setTickLengthToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0.0, true)){
        m_action->setTickLength(value);
        ui->leTickLengh->setText(fromDouble(value));
    }
}

void LC_SliceDivideOptions::setTickAngleToActionAndView(const QString &val){
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        m_action->setTickAngle(angle);
        ui->leTickAngle->setText(fromDouble(angle));
    }
}

void LC_SliceDivideOptions::setTickOffsetToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0.0, false)){
        m_action->setTickOffset(value);
        ui->leTickOffset->setText(fromDouble(value));
    }
}

void LC_SliceDivideOptions::setCircleStartAngleToActionAndView(const QString &val){
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        m_action->setCircleStartTickAngle(angle);
        ui->leCircleStartAngle->setText(fromDouble(angle));
    }
}

void LC_SliceDivideOptions::languageChange(){
    ui->retranslateUi(this);
}
