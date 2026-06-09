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
#include "lc_line_parallel_through_options_widget.h"

#include "lc_action_draw_line_parallel_through.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_parallel_through_options_widget.h"

/*
 *  Constructs a QG_LineParallelThroughOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineParallelThroughOptionsWidget::LC_LineParallelThroughOptionsWidget():ui(new Ui::LC_LineParallelThroughOptionsWidget{}) {
    ui->setupUi(this);

    connect(ui->cbSymmetric, &QCheckBox::toggled, this, &LC_LineParallelThroughOptionsWidget::onSymmetricToggled);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_LineParallelThroughOptionsWidget::onNumberValueChanged);
    connect(ui->cbWithin, &QCheckBox::toggled, this, &LC_LineParallelThroughOptionsWidget::onWithinToggled);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineParallelThroughOptionsWidget::~LC_LineParallelThroughOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineParallelThroughOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineParallelThroughOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineParallelThrough*>(a);

    const bool symmetric = m_action->isSymmetric();
    const bool within = m_action->isDistributeWithin();
    const int copyNumber = m_action->getNumber();

    LC_GuardedSignalsBlocker({ui->sbNumber, ui->cbSymmetric, ui->cbWithin});
    ui->sbNumber->setValue(copyNumber);
    ui->cbSymmetric->setChecked(symmetric);
    ui->cbWithin->setChecked(within);

    ui->cbWithin->setEnabled(copyNumber > 1);
}

void LC_LineParallelThroughOptionsWidget::onSymmetricToggled(const bool checked) {
    m_action->setSymmetric(checked);
    m_action->updateOptions();
}

void LC_LineParallelThroughOptionsWidget::onWithinToggled(const bool checked) {
    m_action->setDistributeWithin(checked);
    m_action->updateOptions();
}

void LC_LineParallelThroughOptionsWidget::onNumberValueChanged(const int number) {
    m_action->setNumber(number);
    m_action->updateOptions();
}
