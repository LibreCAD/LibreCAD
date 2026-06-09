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
#include "lc_round_options_widget.h"

#include "lc_action_modify_round.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_round_options_widget.h"

/*
 *  Constructs a QG_RoundOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_RoundOptionsWidget::LC_RoundOptionsWidget(): ui(new Ui::LC_RoundOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_RoundOptionsWidget::onRadiusEditingFinished);
    connect(ui->cbTrim, &QCheckBox::toggled, this, &LC_RoundOptionsWidget::onTrimToggled);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_RoundOptionsWidget::~LC_RoundOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_RoundOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_RoundOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyRound*>(a);

    const QString radius = fromDouble(m_action->getRadius());
    const bool trim = m_action->isTrimOn();

    LC_GuardedSignalsBlocker({ui->cbTrim,ui->leRadius});
    ui->cbTrim->setChecked(trim);
    ui->leRadius->setText(radius);
}

void LC_RoundOptionsWidget::onRadiusEditingFinished() {
    const auto val = ui->leRadius->text();
    double radius;
    if (toDouble(val, radius, 0.0, false)) {
        m_action->setRadius(radius);
    }
    m_action->updateOptions();
}

void LC_RoundOptionsWidget::onTrimToggled(const bool checked) {
    m_action->setTrim(checked);
    m_action->updateOptions();
}
