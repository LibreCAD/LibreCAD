/****************************************************************************
**
* Options widget for "LinePoints" action.

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
#include "lc_linepointsoptions.h"
#include "lc_actiondrawlinepoints.h"
#include "ui_lc_linepointsoptions.h"

LC_LinePointsOptions::LC_LinePointsOptions() :
    LC_ActionOptionsWidget(nullptr),
    ui(new Ui::LC_LinePointsOptions),
    m_action(nullptr) {
    ui->setupUi(this);
    connect(ui->sbPointsCount, &QSpinBox::valueChanged, this, &LC_LinePointsOptions::onPointsCountValueChanged);
    connect(ui->cbEdgePoints, &QComboBox::currentIndexChanged, this,
            &LC_LinePointsOptions::onEdgePointsModeIndexChanged);

    connect(ui->cbFixedDistance, &QCheckBox::clicked, this, &LC_LinePointsOptions::onFixedDistanceClicked);
    connect(ui->cbWithinLine, &QCheckBox::clicked, this, &LC_LinePointsOptions::onWithinLineClicked);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LinePointsOptions::onDistanceEditingFinished);

    connect(ui->cbAngle, &QCheckBox::clicked, this, &LC_LinePointsOptions::onAngleClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LinePointsOptions::onAngleEditingFinished);
}

LC_LinePointsOptions::~LC_LinePointsOptions(){
    delete ui;
    m_action = nullptr;
}

bool LC_LinePointsOptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType ==  RS2::ActionDrawLinePoints || actionType == RS2::ActionDrawPointsMiddle;
}

QString LC_LinePointsOptions::getSettingsGroupName() {
    return "Draw";
}

QString LC_LinePointsOptions::getSettingsOptionNamePrefix() {
    switch (m_action->rtti()){
        case RS2::ActionDrawLinePoints:
            return "LinePoints";
        case RS2::ActionDrawPointsMiddle:
            return "PointsMiddle";
        default:
            return "";
    }
}

void LC_LinePointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    if (m_inUpdateCycle){
        return;
    }
    m_inUpdateCycle = true;
    m_action = dynamic_cast<LC_ActionDrawLinePoints *>(a);
    int pointsCount = 1;
    int edgePointMode = 0; /*LC_ActionDrawLinePoints::DRAW_EDGE_NONE*/;
    bool fixedDistanceMode = false;
    bool withinLine = false;
    QString distance;
    QString angle;
    bool angleMode;
    bool showAllControls =  m_action->rtti() == RS2::ActionDrawLinePoints;

    int direction = m_action->getDirection();
    angleMode = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;

    if (!showAllControls){
        angleMode = false;
    }

    if (update){
        pointsCount = m_action->getPointsCount();
        if (showAllControls) {
            edgePointMode = m_action->getEdgePointsMode();
            fixedDistanceMode = m_action->isFixedDistanceMode();
            withinLine = m_action->isWithinLineMode();
            distance = fromDouble(m_action->getPointsDistance());
            angle = fromDouble(m_action->getAngle());
        }
    }
    else{
        pointsCount = loadInt("Count", 1);
        if (showAllControls) {
            edgePointMode = loadInt("EdgeMode", 1);
            fixedDistanceMode = loadBool("UseFixedDistance", false);
            withinLine = loadBool("FitToLine", true);
            distance = load("PointsDistance", "1.0");
            angle = load("Angle", "0.0");
        }
    }

    ui->cbAngle->setVisible(showAllControls);
    ui->leAngle->setVisible(showAllControls);
    ui->line_2->setVisible(showAllControls);
    ui->cbFixedDistance->setVisible(showAllControls);
    ui->frmFixed->setVisible(showAllControls);
    ui->line->setVisible(showAllControls);
    ui->label_2->setVisible(showAllControls);
    ui->cbEdgePoints->setVisible(showAllControls);

    setPointsCountActionAndView(pointsCount);
    setEdgePointsModeToActionAndView(edgePointMode);
    setFixedDistanceModeToActionAndView(fixedDistanceMode);
    setWithinLineModeToActionAndView(withinLine);
    setDistanceToActionAndView(distance);
    setAngleModeToActionAndView(angleMode);
    setAngleToActionAndView(angle, false);
    m_inUpdateCycle = false;
}

void LC_LinePointsOptions::doSaveSettings(){
    save("Count", ui->sbPointsCount->value());
    if (m_action->rtti() == RS2::ActionDrawLinePoints) {
        save("EdgeMode", ui->cbEdgePoints->currentIndex());
        save("UseFixedDistance", ui->cbFixedDistance->isChecked());
        save("FitToLine", ui->cbWithinLine->isChecked());
        save("PointsDistance", ui->leDistance->text());
        save("Angle", ui->leAngle->text());
    }
}

void LC_LinePointsOptions::onPointsCountValueChanged(int value){
    if (m_action != nullptr){
        setPointsCountActionAndView(value);
    }
}

void LC_LinePointsOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LinePointsOptions::setPointsCountActionAndView(int val){
    m_action->setPointsCount(val);
    ui->sbPointsCount->setValue(val);
}

void LC_LinePointsOptions::onEdgePointsModeIndexChanged(int index){
    if (m_action != nullptr){
        setEdgePointsModeToActionAndView(index);
    }
}

void LC_LinePointsOptions::setEdgePointsModeToActionAndView(int index){
    m_action->setEdgePointsMode(index);
    ui->cbEdgePoints->setCurrentIndex(index);
}

void LC_LinePointsOptions::onFixedDistanceClicked(bool value){
    if (m_action != nullptr){
        setFixedDistanceModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::onAngleClicked(bool value){
    if (m_action != nullptr){
        setAngleModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::setFixedDistanceModeToActionAndView(bool value){
    ui->cbFixedDistance->setChecked(value);
    m_action->setFixedDistanceMode(value);
    ui->frmFixed->setVisible(value && m_action->rtti()==RS2::ActionDrawLinePoints);

    if (!value){
        ui->sbPointsCount->setEnabled(true);
    }
}

void LC_LinePointsOptions::setWithinLineModeToActionAndView(bool value){
    ui->cbWithinLine->setChecked(value);
    m_action->setWithinLineMode(value);

    ui->sbPointsCount->setEnabled(!value);
}

void LC_LinePointsOptions::onWithinLineClicked(bool value){
    if (m_action != nullptr){
        setWithinLineModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::onDistanceEditingFinished(){
    if (m_action != nullptr){
        setDistanceToActionAndView(ui->leDistance->text());
    }
}
void LC_LinePointsOptions::onAngleEditingFinished(){
    if (m_action != nullptr){
        setAngleToActionAndView(ui->leAngle->text(), true);
    }
}

void LC_LinePointsOptions::setDistanceToActionAndView(QString val){
    double distance;
    if (toDouble(val, distance, 1.0, true)){
        m_action->setPointsDistance(distance);
        ui->leDistance->setText(fromDouble(distance));
    }
}

void LC_LinePointsOptions::setAngleModeToActionAndView(bool value){
    if (value){
        m_action->setSetAngleDirectionState();
    }
    else{
        if (m_action->getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE){
            m_action->setSetPointDirectionState();
        }
    }
    ui->cbAngle->setChecked(value);
    ui->leAngle->setEnabled(value);
}

void LC_LinePointsOptions::setAngleToActionAndView(QString val, bool affectState){
    double angle;
    if (toDouble(val, angle, 0.0, false)){
        if (affectState){
            m_action->setAngleValue(angle);
        }
        else{
            m_action->setAngle(angle);
        }
        ui->leAngle->setText(fromDouble(angle));
    }
}
