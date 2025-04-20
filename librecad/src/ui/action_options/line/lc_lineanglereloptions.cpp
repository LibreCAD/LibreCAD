/****************************************************************************
**
* Options widget for Angle Line from line action.

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

#include "lc_lineanglereloptions.h"
#include "ui_lc_lineanglereloptions.h"
#include "lc_actiondrawlineanglerel.h"

LC_LineAngleRelOptions::LC_LineAngleRelOptions() :
    LC_ActionOptionsWidget(nullptr),
    ui(std::make_unique<Ui::LC_LineAngleRelOptions>()){
    ui->setupUi(this);

    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onLengthEditingFinished);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onOffsetEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onAngleEditingFinished);
    connect(ui->cbRelativeAngle, &QCheckBox::clicked, this, &LC_LineAngleRelOptions::onAngleRelatedClicked);
    connect(ui->cbDivide, &QCheckBox::clicked, this, &LC_LineAngleRelOptions::onDivideClicked);
    connect(ui->cbFree, &QCheckBox::clicked, this, &LC_LineAngleRelOptions::onFreeLengthClicked);
    connect(ui->cbTickSnapMode, &QComboBox::currentIndexChanged,this,  &LC_LineAngleRelOptions::onTickSnapModeIndexChanged);
    connect(ui->cbLineSnapMode, &QComboBox::currentIndexChanged,this, &LC_LineAngleRelOptions::onLineSnapModeIndexChanged);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onDistanceEditingFinished);
}

LC_LineAngleRelOptions::~LC_LineAngleRelOptions(){
    m_action = nullptr;
}

bool LC_LineAngleRelOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineAngleRel || actionType == RS2::ActionDrawLineOrthogonalRel;
}

void LC_LineAngleRelOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionDrawLineAngleRel*>(a);
    m_fixedAngle = a->rtti()==RS2::ActionDrawLineOrthogonalRel;
    QString length;
    QString offset;
    QString angle;
    int lineSnapMode = 0;
    int tickSnapMode = 0;
    bool angleIsRelative = false;
    bool lengthIsFree = false;
    bool divide = false;
    QString distance;
    if (update) {
        length = fromDouble(m_action->getTickLength());
        offset = fromDouble(m_action->getTickOffset());
        angle = fromDouble(m_action->getTickAngle());
        lineSnapMode = m_action->getLineSnapMode();
        tickSnapMode = m_action->getTickSnapMode();
        angleIsRelative = m_action->isAngleRelative();
        lengthIsFree = m_action->isLengthFree();
        divide = m_action->isDivideLine();
        distance = QString("%1").arg(m_action->getSnapDistance());
    } else {
        length = load("Length", "1.0");
        offset = load("Offset", "1.0");
        if (!m_fixedAngle){
            angle = load("Angle", "1.0");
            angleIsRelative = loadBool("AngleIsRelative", true);
        }
        lengthIsFree = loadBool("LengthIsFree", true);
        lineSnapMode = loadInt("LineSnapMode", 0);
        tickSnapMode = loadInt("TickSnapMode", 1);
        divide = loadBool("DoDivide", false);
        distance = load("SnapDistance", "0.0");
    }
    ui->leAngle->setVisible(!m_fixedAngle);
    ui->lblAngle->setVisible(!m_fixedAngle);
    ui->cbRelativeAngle->setVisible(!m_fixedAngle);

    setLengthIsFreeToActionAndView(lengthIsFree);
    setLengthToActionAndView(length);
    setOffsetToActionAndView(offset);
    if (!m_fixedAngle){
        setAngleToActionAndView(angle);
        setAngleIsRelativeToActionAndView(angleIsRelative);
    }
    setLineSnapModeToActionAndView(lineSnapMode);
    setTickSnapModeToActionAndView(tickSnapMode);
    setDivideToActionAndView(divide);
    setDistanceToActionAndView(distance);
}

QString LC_LineAngleRelOptions::getSettingsOptionNamePrefix(){
    return m_fixedAngle ? "LineOrthogonalRel" : "LineAngleRel";
}

void LC_LineAngleRelOptions::doSaveSettings(){
    save("Length", ui->leLength->text());
    if (!m_fixedAngle){
        save("Angle", ui->leAngle->text());
        save("AngleIsRelative", ui->cbRelativeAngle->isChecked());
    }
    save("LengthIsFree", ui->cbFree->isChecked());
    save("Offset", ui->leOffset->text());
    save("LineSnapMode", ui->cbLineSnapMode->currentIndex());
    save("TickSnapMode", ui->cbTickSnapMode->currentIndex());
    save("DoDivide", ui->cbDivide->isChecked());
    save("SnapDistance", ui->leDistance->text());
}

void LC_LineAngleRelOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineAngleRelOptions::setAngleToActionAndView(const QString &expr){
    double angle = 0.;
    if (toDoubleAngleDegrees(expr, angle, 0.0, false)){
        m_action->setTickAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_LineAngleRelOptions::setLengthToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 1.0, false)){
        m_action->setTickLength(value);
        ui->leLength->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setDistanceToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 0.0, false)){
        m_action->setSnapDistance(value);
        ui->leDistance->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setOffsetToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 0.0, false)){
        m_action->setTickOffset(value);
        ui->leOffset->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setAngleIsRelativeToActionAndView(bool relative){
    m_action->setAngleIsRelative(relative);
    ui->cbRelativeAngle->setChecked(relative);
}

void LC_LineAngleRelOptions::setDivideToActionAndView(bool divide){
    m_action->setDivideLine(divide);
    ui->cbDivide->setChecked(divide);
}

void LC_LineAngleRelOptions::setLengthIsFreeToActionAndView(bool free){
    m_action->setLengthIsFree(free);
    ui->cbFree->setChecked(free);
}

void LC_LineAngleRelOptions::setTickSnapModeToActionAndView(int mode){
    m_action->setTickSnapMode(mode);
    ui->cbTickSnapMode->setCurrentIndex(mode);
}

void LC_LineAngleRelOptions::setLineSnapModeToActionAndView(int mode){
    m_action->setLineSnapMode(mode);
    ui->cbLineSnapMode->setCurrentIndex(mode);
    bool notFreeSnap = mode != 0;
    ui->lblDistance->setVisible(notFreeSnap);
    ui->leDistance->setVisible(notFreeSnap);
}

void LC_LineAngleRelOptions::onLengthEditingFinished(){
    if (m_action != nullptr){
        const QString &expr = ui->leLength->text();
        setLengthToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onDistanceEditingFinished(){
    if (m_action != nullptr){
        const QString &expr = ui->leDistance->text();
        setDistanceToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onOffsetEditingFinished(){
    if (m_action != nullptr){
        const QString &expr = ui->leOffset->text();
        setOffsetToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onAngleEditingFinished(){
    if (m_action != nullptr){
        const QString &expr = ui->leAngle->text();
        setAngleToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onLineSnapModeIndexChanged(int index){
    if (m_action != nullptr){
        setLineSnapModeToActionAndView(index);
    }
}

void LC_LineAngleRelOptions::onTickSnapModeIndexChanged(int index){
    if (m_action != nullptr){
        setTickSnapModeToActionAndView(index);
    }
}

void LC_LineAngleRelOptions::onFreeLengthClicked(bool clicked){
    if (m_action != nullptr){
        setLengthIsFreeToActionAndView(clicked);
    }
}

void LC_LineAngleRelOptions::onAngleRelatedClicked(bool clicked){
    if (m_action != nullptr){
        setAngleIsRelativeToActionAndView(clicked);
    }
}

void LC_LineAngleRelOptions::onDivideClicked(bool clicked){
    if (m_action != nullptr){
        setDivideToActionAndView(clicked);
    }
}
