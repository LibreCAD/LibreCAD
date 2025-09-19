/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_lineoptions.h"
#include "ui_lc_lineoptions.h"
#include "lc_actiondrawlinesnake.h"

/*
 *  Constructs a QG_LineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineOptions::LC_LineOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawSnakeLine, "Draw","LineSnake")
    , ui(new Ui::Ui_LineOptionsRel{})
{
    ui->setupUi(this);
    connect(ui->rbX, &QRadioButton::clicked, this, &LC_LineOptions::onXClicked);
    connect(ui->rbY, &QRadioButton::clicked, this, &LC_LineOptions::onYClicked);
    connect(ui->rbPoint, &QRadioButton::clicked, this, &LC_LineOptions::onPointClicked);
    connect(ui->rbAngle, &QRadioButton::toggled, this, &LC_LineOptions::onAngleClicked);
    connect(ui->cbRelAngle, &QCheckBox::clicked, this, &LC_LineOptions::onAngleRelativeClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineOptions::onSetAngle);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

bool LC_LineOptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == RS2::ActionDrawSnakeLine || actionType == RS2::ActionDrawSnakeLineX || actionType == RS2::ActionDrawSnakeLineY;
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineOptions::~LC_LineOptions(){
    m_action = nullptr;
    delete ui;
};

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineOptions::doSaveSettings(){
    save("Angle", ui->leAngle->text());
    save("AngleRelative", ui->cbRelAngle->isChecked());
}

void LC_LineOptions::doSetAction(RS_ActionInterface* a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawLineSnake *>(a);

    // prevent cycle invocation
    if (m_inUpdateCycle){
        return;
    }
    m_inUpdateCycle = true;

    QString angle;
    bool angleRelative;

    ui->bClose->setEnabled(m_action->mayClose());
    ui->bUndo->setEnabled(m_action->mayUndo());
    ui->bRedo->setEnabled(m_action->mayRedo());
    ui->bPolyline->setEnabled(m_action->mayClose());

    int direction = m_action->getDirection();

    ui->rbPoint->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_POINT);
    ui->rbX->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_X);
    ui->rbY->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_Y);
    bool angleDirection = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;

    setupAngleRelatedUI(angleDirection);

    if (update){
        angle = fromDouble(m_action->getAngleDegrees());
        angleRelative = m_action->isAngleRelative();
    } else {
        angle = load("Angle", "0.0");
        angleRelative = loadBool("AngleRelative", false);
    }

    setAngleToActionAndView(angle, false);
    setAngleRelativeToActionAndView(angleRelative);
    m_inUpdateCycle = false;
}

void LC_LineOptions::onXClicked(bool value){
    if (m_action != nullptr){
        setXDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onYClicked(bool value){
    if (m_action != nullptr){
        setYDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onPointClicked(bool value) const {
    if (m_action != nullptr){
        setPointDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onAngleClicked(bool value){
    if (m_action != nullptr){
        setAngleDirectionToActionAndView(value);
    }
}

void LC_LineOptions::closeLine() {
    if (m_action != nullptr) {
        m_action->close();
    }
}

void LC_LineOptions::undo() {
    if (m_action != nullptr) {
        m_action->undo();
    }
}

void LC_LineOptions::redo() {
    if (m_action != nullptr) {
        m_action->redo();
    }
}

void LC_LineOptions::polyline() {
    if (m_action != nullptr) {
        m_action->polyline();
    }
}

void LC_LineOptions::onAngleRelativeClicked(bool value){
    if (m_action != nullptr) {
        setAngleRelativeToActionAndView(value);
    }
}

void LC_LineOptions::onSetAngle() {
    if (m_action != nullptr){
        setAngleToActionAndView(ui->leAngle->text(), true);
    }
}

void LC_LineOptions::start() {
    if (m_action) {
        m_action->setNewStartPointState();
    }
}

void LC_LineOptions::setXDirectionToActionAndView(bool value) const {
    if (value){
        m_action->setSetXDirectionState();
    }
    ui->rbX->setChecked(value);
}

void LC_LineOptions::setYDirectionToActionAndView(bool value) const {
    if (value){
        m_action->setSetYDirectionState();
    }
    ui->rbY->setChecked(value);
}

void LC_LineOptions::setAngleDirectionToActionAndView(bool value) const {
    if (value){
        m_action->setSetAngleDirectionState();
    }
    setupAngleRelatedUI(value);
}

void LC_LineOptions::setupAngleRelatedUI(bool value) const {
    ui->rbAngle->setChecked(value);
    ui->leAngle->setEnabled(value);
    ui->tbPickAngle->setEnabled(value);
    ui->cbRelAngle->setEnabled(value);
}

void LC_LineOptions::setPointDirectionToActionAndView(bool value) const {
    if (value){
        m_action->setSetPointDirectionState();
    }
    ui->rbPoint->setChecked(value);
}

void LC_LineOptions::setAngleToActionAndView(const QString& val, bool affectState){
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        if (affectState){
            m_action->setAngleValueDegrees(angle);
        }
        else {
            m_action->setAngleDegrees(angle);
        }
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_LineOptions::setAngleRelativeToActionAndView(bool relative) const {
    m_action->setAngleIsRelative(relative);
    ui->cbRelAngle->setChecked(relative);
}
