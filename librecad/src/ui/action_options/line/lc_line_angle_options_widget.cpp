/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/
#include "lc_line_angle_options_widget.h"

#include "lc_action_draw_line_angle.h"
#include "ui_lc_line_angle_options_widget.h"

/*
 *  Constructs a QG_LineAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineAngleOptionsWidget::LC_LineAngleOptionsWidget()
    : ui(std::make_unique<Ui::LC_LineAngleOptionsWidget>()){
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineAngleOptionsWidget::onAngleEditingFinished);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineAngleOptionsWidget::onLengthEditingFinished);
    connect(ui->cbSnapPoint, &QComboBox::currentIndexChanged, this, &LC_LineAngleOptionsWidget::onSnapPointCurrentIndexChanged);
    connect(ui->cbForAnglesBasis, &QCheckBox::toggled, this, &LC_LineAngleOptionsWidget::onAnglesBasisToggled);
    connect(ui->cbLengthType,&QComboBox::currentIndexChanged, this, &LC_LineAngleOptionsWidget::onLengthTypeCurrentIndexChanged);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("length", ui->tbPickLength, ui->leLength);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineAngleOptionsWidget::~LC_LineAngleOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineAngleOptionsWidget::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineAngleOptionsWidget::setupLengthType(bool angleIsNotFixed) const {
    ui->cbLengthType->blockSignals(true);
    ui->cbLengthType->clear();
    ui->cbLengthType->addItem(tr("Line"), LC_ActionDrawLineAngle::LengthType::LINE);
    if (angleIsNotFixed) {
        ui->cbLengthType->addItem(tr("By X"), LC_ActionDrawLineAngle::LengthType::BY_X);
        ui->cbLengthType->addItem(tr("By Y"), LC_ActionDrawLineAngle::LengthType::BY_Y);
    }
    ui->cbLengthType->addItem(tr("Free"), LC_ActionDrawLineAngle::LengthType::FREE);
    ui->cbLengthType->blockSignals(false);
}

void LC_LineAngleOptionsWidget::doUpdateByAction(RS_ActionInterface *a){
    m_action = static_cast<LC_ActionDrawLineAngle*>(a);
    const bool angleIsFixed = m_action->hasFixedAngle();

    setupLengthType(!angleIsFixed);

    const QString length = fromDouble(m_action->getLength());
    const int snapPoint = m_action->getLineSnapMode();
    int lenType = m_action->getLengthType();

    ui->leAngle->setVisible(!angleIsFixed);
    ui->tbPickAngle->setVisible(!angleIsFixed && m_interactiveInputControlsVisible);
    ui->lAngle->setVisible(!angleIsFixed);

    if (angleIsFixed) {
        const bool inAngleBasis = m_action->isInAngleBasis();
        const bool hasCustomAnglesBasis = m_action->hasNonDefaultAnglesBasis();
        ui->cbForAnglesBasis->setVisible(hasCustomAnglesBasis);
        setToAngleBasis(inAngleBasis);
    } else {
        const QString angle = fromDouble(m_action->getUcsAngleDegrees());
        setAngleToActionAndView(angle);
        ui->cbForAnglesBasis->setVisible(false);
        ui->leAngle->setText(angle);
    }
    setSnapPointToActionAndView(snapPoint);
    setLengthToActionAndView(length);

    ui->leLength->setText(length);

    ui->cbLengthType->blockSignals(true);
    const int idx = ui->cbLengthType->findData(lenType);
    if (idx != -1) {
        ui->cbLengthType->setCurrentIndex(idx);
        const bool notFreeLength = lenType != LC_ActionDrawLineAngle::FREE;
        ui->leLength->setEnabled(notFreeLength);
    }
    const auto type = static_cast<LC_ActionDrawLineAngle::LengthType>(lenType);
    m_action->setLengthType(type, false);
    ui->cbLengthType->blockSignals(false);
}

void LC_LineAngleOptionsWidget::onSnapPointCurrentIndexChanged(const int number){
    setSnapPointToActionAndView(number);
}

void LC_LineAngleOptionsWidget::onLengthTypeCurrentIndexChanged(int number) {
    int lt = ui->cbLengthType->itemData(number).toInt();
    const auto type = static_cast<LC_ActionDrawLineAngle::LengthType>(lt);
    setLengthTypeToActionAndView(type);
}

void LC_LineAngleOptionsWidget::setLengthTypeToActionAndView(LC_ActionDrawLineAngle::LengthType lenType) {
    m_action->setLengthType(lenType, true);
}

void LC_LineAngleOptionsWidget::onLengthEditingFinished(){
    setLengthToActionAndView(ui->leLength->text());
}
void LC_LineAngleOptionsWidget::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_LineAngleOptionsWidget::onAnglesBasisToggled(const bool val) {
    setToAngleBasis(val);
}

void LC_LineAngleOptionsWidget::setAngleToActionAndView(const QString& val){
    double angle = 0.;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        m_action->setUcsAngleDegrees(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_LineAngleOptionsWidget::setSnapPointToActionAndView(const int val) const {
    m_action->setLineSnapMode(val);
    ui->cbSnapPoint->setCurrentIndex(val);
}

void LC_LineAngleOptionsWidget::setLengthToActionAndView(const QString& val){
    double len = 0.;
    if (toDouble(val, len, 1.0, false)){
        m_action->setLength(len);
        ui->leLength->setText(fromDouble(len));
    }
}

void LC_LineAngleOptionsWidget::setToAngleBasis(const bool val) const {
    ui->cbForAnglesBasis->setChecked(val);
    m_action->setInAngleBasis(val);
}
