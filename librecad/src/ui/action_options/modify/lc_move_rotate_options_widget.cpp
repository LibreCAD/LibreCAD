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
#include "lc_move_rotate_options_widget.h"

#include "lc_action_modify_move_rotate.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_move_rotate_options_widget.h"

/*
 *  Constructs a QG_MoveRotateOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_MoveRotateOptionsWidget::LC_MoveRotateOptionsWidget(): ui(new Ui::LC_MoveRotateOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->cbSameAngleForCopies, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbSameAngleForCopiesClicked);
    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_MoveRotateOptionsWidget::cbFreeAngleForClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_MoveRotateOptionsWidget::onAngleEditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_MoveRotateOptionsWidget::onCopiesCountChanged);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_MoveRotateOptionsWidget::~LC_MoveRotateOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_MoveRotateOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_MoveRotateOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyMoveRotate*>(a);

    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const int copiesNumber = m_action->getCopiesNumber();
    const bool keepOriginals = m_action->isKeepOriginals();
    const bool useMultipleCopies = m_action->isUseMultipleCopies();
    const bool sameAngle = m_action->isUseSameAngleForCopies();
    const bool angleIsFree = m_action->isAngleFree();
    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));

    LC_GuardedSignalsBlocker({
        ui->leAngle,
        ui->cbCurrentAttr,
        ui->cbKeepOriginals,
        ui->cbFreeAngle,
        ui->cbCurrentLayer,
        ui->cbMultipleCopies,
        ui->cbSameAngleForCopies,
        ui->sbNumberOfCopies
    });

    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);
    ui->cbSameAngleForCopies->setEnabled(useMultipleCopies);
    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
    ui->cbSameAngleForCopies->setChecked(sameAngle);
    ui->leAngle->setEnabled(!angleIsFree);
    ui->tbPickAngle->setEnabled(!angleIsFree);
    ui->cbFreeAngle->setChecked(angleIsFree);
    ui->leAngle->setText(angle);
}

void LC_MoveRotateOptionsWidget::onAngleEditingFinished() {
    const QString& val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbSameAngleForCopiesClicked(const bool val) {
    m_action->setUseSameAngleForCopies(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::cbFreeAngleForClicked(const bool val) {
    m_action->setAngleIsFree(val);
    m_action->updateOptions();
}

void LC_MoveRotateOptionsWidget::onCopiesCountChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}
