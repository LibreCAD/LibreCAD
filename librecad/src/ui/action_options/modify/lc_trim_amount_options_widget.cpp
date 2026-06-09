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
#include "lc_trim_amount_options_widget.h"

#include "lc_action_modify_trim_amount.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_trim_amount_options_widget.h"

/*
 *  Constructs a QG_TrimAmountOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_TrimAmountOptionsWidget::LC_TrimAmountOptionsWidget(): ui(new Ui::LC_TrimAmountOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &LC_TrimAmountOptionsWidget::onDistEditingFinished);
    connect(ui->cbSymmetric, &QCheckBox::toggled, this, &LC_TrimAmountOptionsWidget::onSymmetricToggled);
    connect(ui->cbTotalLength, &QCheckBox::toggled, this, &LC_TrimAmountOptionsWidget::onTotalLengthToggled);

    pickDistanceSetup("length", ui->tbPickLength, ui->leDist);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_TrimAmountOptionsWidget::~LC_TrimAmountOptionsWidget() {
    m_action = nullptr;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_TrimAmountOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_TrimAmountOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = dynamic_cast<LC_ActionModifyTrimAmount*>(a);

    const QString distance = QString("%1").arg(m_action->getDistance());
    const bool byTotal = m_action->isDistanceTotalLength();
    const bool symmetric = m_action->isSymmetricDistance();
    LC_GuardedSignalsBlocker({ui->leDist, ui->cbTotalLength, ui->cbSymmetric});

    ui->leDist->setText(distance);
    ui->cbTotalLength->setChecked(byTotal);
    ui->cbSymmetric->setEnabled(!byTotal);

    ui->cbSymmetric->setChecked(symmetric);}

void LC_TrimAmountOptionsWidget::onDistEditingFinished() {
    const auto strValue = ui->leDist->text();
    double val;
    if (toDouble(strValue, val, 1.0, false)) {
        m_action->setDistance(val);
    }
    m_action->updateOptions();
}

void LC_TrimAmountOptionsWidget::onTotalLengthToggled(const bool checked) {
    m_action->setDistanceIsTotalLength(checked);
    m_action->updateOptions();
}

void LC_TrimAmountOptionsWidget::onSymmetricToggled(const bool checked) {
    m_action->setSymmetricDistance(checked);
    m_action->updateOptions();
}
