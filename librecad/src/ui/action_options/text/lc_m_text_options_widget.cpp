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
#include "lc_m_text_options_widget.h"

#include "lc_action_draw_mtext.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_m_text_options_widget.h"

/*
 *  Constructs a QG_TextOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_MTextOptionsWidget::LC_MTextOptionsWidget(): ui{std::make_unique<Ui::LC_MTextOptionsWidget>()} {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_MTextOptionsWidget::updateAngle);
    connect(ui->teText, &QTextEdit::textChanged, this, &LC_MTextOptionsWidget::updateText);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_MTextOptionsWidget::~LC_MTextOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_MTextOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_MTextOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawMText*>(a);

    const QString text = m_action->getText();
    const QString angle = fromDouble(m_action->getUcsAngleDegrees());

    LC_GuardedSignalsBlocker({ ui->teText,ui->leAngle });

    ui->teText->blockSignals(true);
    ui->teText->setText(text);
    ui->leAngle->setText(angle);
    ui->teText->blockSignals(false);
}

void LC_MTextOptionsWidget::updateText() const {
    m_action->setText(ui->teText->toPlainText());
    m_action->updateOptions();
}

void LC_MTextOptionsWidget::updateAngle() {
    double angle = 0.;
    const QString val = ui->leAngle->text();
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setUcsAngleDegrees(angle);
    }
    m_action->updateOptions();
}
