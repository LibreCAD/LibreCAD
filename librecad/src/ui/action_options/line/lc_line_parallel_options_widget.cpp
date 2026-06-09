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
#include "lc_line_parallel_options_widget.h"

#include "lc_action_draw_line_parallel.h"
#include "lc_guarded_signals_blocker.h"
#include "rs_graphicview.h"
#include "ui_lc_line_parallel_options_widget.h"

/*
 *  Constructs a QG_LineParallelOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineParallelOptionsWidget::LC_LineParallelOptionsWidget():ui(new Ui::LC_LineParallelOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &LC_LineParallelOptionsWidget::onDistEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_LineParallelOptionsWidget::onNumberValueChanged);

    pickDistanceSetup("distance", ui->tbPickDistance, ui->leDist);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineParallelOptionsWidget::~LC_LineParallelOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineParallelOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineParallelOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineParallel*>(a);

    const QString distance = fromDouble(m_action->getDistance());
    const int copiesNumber = m_action->getNumber();
    LC_GuardedSignalsBlocker({ui->leDist, ui->sbNumber});
    ui->leDist->setText(distance);
    ui->sbNumber->setValue(copiesNumber);
}

void LC_LineParallelOptionsWidget::onNumberValueChanged(const int number) {
    m_action->setNumber(number);
    m_action->updateOptions();
}

void LC_LineParallelOptionsWidget::onDistEditingFinished() {
    const QString& val = ui->leDist->text();
    double distance;
    if (toDouble(val, distance, 1.0, false)) {
        m_action->setDistance(distance);
        ui->leDist->setText(fromDouble(distance));
    }
    m_action->updateOptions();
}
