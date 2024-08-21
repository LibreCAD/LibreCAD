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
#include "ui_lc_linepointsoptions.h"
#include "rs_settings.h"
#include "rs_math.h"

LC_LinePointsOptions::LC_LinePointsOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionDrawLinePoints, "Draw", "LinePoints"),
    ui(new Ui::LC_LinePointsOptions),
    action(nullptr){
    ui->setupUi(this);
    connect(ui->sbPointsCount, SIGNAL(valueChanged(int)), this, SLOT(onPointsCountValueChanged(int)));
    connect(ui->cbEdgePoints, SIGNAL(currentIndexChanged(int)), SLOT(onEdgePointsModeIndexChanged(int)));

    connect(ui->cbFixedDistance, SIGNAL(clicked(bool)), this, SLOT(onFixedDistanceClicked(bool)));
    connect(ui->cbWithinLine, SIGNAL(clicked(bool)), this, SLOT(onWithinLineClicked(bool)));
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LinePointsOptions::onDistanceEditingFinished);

    connect(ui->cbAngle, &QCheckBox::clicked, this, &LC_LinePointsOptions::onAngleClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LinePointsOptions::onAngleEditingFinished);
}

LC_LinePointsOptions::~LC_LinePointsOptions(){
    delete ui;
    action = nullptr;
}

void LC_LinePointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    if (inUpdateCycle){
        return;
    }
    inUpdateCycle = true;
    action = dynamic_cast<LC_ActionDrawLinePoints *>(a);
    int pointsCount;
    int edgePointMode;
    bool fixedDistanceMode;
    bool withinLine;
    QString distance;
    QString angle;
    bool angleMode;
    int direction = action->getDirection();
    angleMode = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;
    if (update){
        pointsCount = action->getPointsCount();
        edgePointMode = action->getEdgePointsMode();
        fixedDistanceMode = action->isFixedDistanceMode();
        withinLine = action->isWithinLineMode();
        distance = fromDouble(action->getPointsDistance());

        angle = fromDouble(action->getAngle());
    }
    else{
        pointsCount = loadInt("Count", 1);
        edgePointMode = loadInt("EdgeMode", 1);
        fixedDistanceMode = loadBool("UseFixedDistance", false);
        withinLine = loadBool("FitToLine",true);
        distance = load("PointsDistance", "1.0");
        angle = load("Angle", "0.0");
    }
    setPointsCountActionAndView(pointsCount);
    setEdgePointsModeToActionAndView(edgePointMode);
    setFixedDistanceModeToActionAndView(fixedDistanceMode);
    setWithinLineModeToActionAndView(withinLine);
    setDistanceToActionAndView(distance);
    setAngleModeToActionAndView(angleMode);
    setAngleToActionAndView(angle, false);
    inUpdateCycle = false;
}

void LC_LinePointsOptions::doSaveSettings(){
    save("Count", ui->sbPointsCount->value());
    save("EdgeMode", ui->cbEdgePoints->currentIndex());
    save("UseFixedDistance", ui->cbFixedDistance->isChecked());
    save("FitToLine", ui->cbWithinLine->isChecked());
    save("PointsDistance", ui->leDistance->text());
    save("Angle", ui->leAngle->text());
}

void LC_LinePointsOptions::onPointsCountValueChanged(int value){
    if (action != nullptr){
        setPointsCountActionAndView(value);
    }
}

void LC_LinePointsOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LinePointsOptions::setPointsCountActionAndView(int val){
    action->setPointsCount(val);
    ui->sbPointsCount->setValue(val);
}

void LC_LinePointsOptions::onEdgePointsModeIndexChanged(int index){
    if (action != nullptr){
        setEdgePointsModeToActionAndView(index);
    }
}

void LC_LinePointsOptions::setEdgePointsModeToActionAndView(int index){
    action->setEdgePointsMode(index);
    ui->cbEdgePoints->setCurrentIndex(index);
}

void LC_LinePointsOptions::onFixedDistanceClicked(bool value){
    if (action != nullptr){
        setFixedDistanceModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::onAngleClicked(bool value){
    if (action != nullptr){
        setAngleModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::setFixedDistanceModeToActionAndView(bool value){
    ui->cbFixedDistance->setChecked(value);
    action->setFixedDistanceMode(value);
    ui->frmFixed->setVisible(value);

    if (!value){
        ui->sbPointsCount->setEnabled(true);
    }
}

void LC_LinePointsOptions::setWithinLineModeToActionAndView(bool value){
   ui->cbWithinLine->setChecked(value);
   action->setWithinLineMode(value);

   ui->sbPointsCount->setEnabled(!value);
}

void LC_LinePointsOptions::onWithinLineClicked(bool value){
    if (action != nullptr){
        setWithinLineModeToActionAndView(value);
    }
}

void LC_LinePointsOptions::onDistanceEditingFinished(){
    if (action != nullptr){
        setDistanceToActionAndView(ui->leDistance->text());
    }
}
void LC_LinePointsOptions::onAngleEditingFinished(){
    if (action != nullptr){
        setAngleToActionAndView(ui->leAngle->text(), true);
    }
}

void LC_LinePointsOptions::setDistanceToActionAndView(QString val){
    double distance;
    if (toDouble(val, distance, 1.0, true)){
        action->setPointsDistance(distance);
        ui->leDistance->setText(fromDouble(distance));
    }
}

void LC_LinePointsOptions::setAngleModeToActionAndView(bool value){
    if (value){
        action->setSetAngleDirectionState();
    }
    else{
        if (action->getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE){
            action->setSetPointDirectionState();
        }
    }
    ui->cbAngle->setChecked(value);
    ui->leAngle->setEnabled(value);
}

void LC_LinePointsOptions::setAngleToActionAndView(QString val, bool affectState){
    double angle;
    if (toDouble(val, angle, 0.0, false)){
        if (affectState){
            action->setAngleValue(angle);
        }
        else{
            action->setAngle(angle);
        }
        ui->leAngle->setText(fromDouble(angle));
    }
}
