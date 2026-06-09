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
#include "lc_block_library_insert_options_widget.h"

#include "lc_action_block_library_insert.h"
#include "lc_convert.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_block_library_insert_options_widget.h"

/*
 *  Constructs a QG_LibraryInsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_BlockLibraryInsertOptionsWidget::LC_BlockLibraryInsertOptionsWidget()
    : ui(std::make_unique<Ui::LC_BlockLibraryInsertOptionsWidget>()) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_BlockLibraryInsertOptionsWidget::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_BlockLibraryInsertOptionsWidget::onFactorEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_BlockLibraryInsertOptionsWidget::~LC_BlockLibraryInsertOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_BlockLibraryInsertOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_BlockLibraryInsertOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionBlockLibraryInsert*>(a);

    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const QString factor = fromDouble(m_action->getFactor());

    LC_GuardedSignalsBlocker({ui->leAngle, ui->leFactor});

    ui->leAngle->setText(angle);
    ui->leFactor->setText(factor);
}

void LC_BlockLibraryInsertOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double param;
    if (LC_Convert::toDoubleAngleRad(val, param, 0.000001, true)) {
        m_action->setAngle(param);
    }
    m_action->updateOptions();
}

void LC_BlockLibraryInsertOptionsWidget::onFactorEditingFinished() {
    const auto val = ui->leFactor->text();
    double param;
    if (toDouble(val, param, 0.000001, true)) {
        m_action->setFactor(param);
    }
    m_action->updateOptions();
}
