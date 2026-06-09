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
#include "lc_line_bisector_options_widget.h"

#include "lc_action_draw_line_bisector.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_bisector_options_widget.h"

/*
 *  Constructs a QG_LineBisectorOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineBisectorOptionsWidget::LC_LineBisectorOptionsWidget()
    : ui(new Ui::LC_LineBisectorOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineBisectorOptionsWidget::onLengthEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_LineBisectorOptionsWidget::onNumberValueChanged);

    pickDistanceSetup("length", ui->tbPickLength, ui->leLength);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineBisectorOptionsWidget::~LC_LineBisectorOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineBisectorOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineBisectorOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineBisector*>(a);

    const QString length = fromDouble(m_action->getLength());
    const int number = m_action->getNumber();

    LC_GuardedSignalsBlocker({ui->leLength, ui->sbNumber});

    ui->leLength->setText(length);
    ui->sbNumber->setValue(number);
}

void LC_LineBisectorOptionsWidget::onNumberValueChanged(const int number) {
    m_action->setNumber(number);
    m_action->updateOptions();
}

void LC_LineBisectorOptionsWidget::onLengthEditingFinished() {
    const QString& val = ui->leLength->text();
    double len;
    if (toDouble(val, len, 1.0, false)) {
        m_action->setLength(len);
    }
    m_action->updateOptions();
}
