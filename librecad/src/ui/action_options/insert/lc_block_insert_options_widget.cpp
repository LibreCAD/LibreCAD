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
#include "lc_block_insert_options_widget.h"

#include "lc_action_block_insert.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_block_insert_options_widget.h"

/*
 *  Constructs a QG_InsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_BlockInsertOptionsWidget::LC_BlockInsertOptionsWidget()
    : ui(new Ui::LC_BlockInsertOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_BlockInsertOptionsWidget::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_BlockInsertOptionsWidget::onFactorEditingFinished);
    connect(ui->leColumnSpacing, &QLineEdit::editingFinished, this, &LC_BlockInsertOptionsWidget::onColumnSpacingEditingFinished);
    connect(ui->leRowSpacing, &QLineEdit::editingFinished, this, &LC_BlockInsertOptionsWidget::onRowSpacingEditingFinished);
    connect(ui->sbRows, &QSpinBox::valueChanged, this, &LC_BlockInsertOptionsWidget::onRowsValueChanged);
    connect(ui->sbColumns, &QSpinBox::valueChanged, this, &LC_BlockInsertOptionsWidget::onColumnsValueChanged);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("spacingX", ui->tbPickSpacingX, ui->leColumnSpacing);
    pickDistanceSetup("spacingY", ui->tbPickSpacingY, ui->leRowSpacing);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_BlockInsertOptionsWidget::~LC_BlockInsertOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_BlockInsertOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_BlockInsertOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionBlockInsert*>(a);

    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const QString factor = fromDouble(m_action->getFactor());
    const int columns = m_action->getColumns();
    const int rows = m_action->getRows();
    const QString columnSpacing = fromDouble(m_action->getColumnSpacing());
    const QString rowSpacing = fromDouble(m_action->getRowSpacing());

    LC_GuardedSignalsBlocker({ui->leAngle, ui->leFactor, ui->sbColumns, ui->sbRows, ui->leColumnSpacing, ui->leRowSpacing});

    ui->leAngle->setText(angle);
    ui->leFactor->setText(factor);
    ui->sbColumns->setValue(columns);
    ui->sbRows->setValue(rows);
    ui->leColumnSpacing->setText(columnSpacing);
    ui->leRowSpacing->setText(rowSpacing);
}

void LC_BlockInsertOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_BlockInsertOptionsWidget::onFactorEditingFinished() {
    const auto val = ui->leFactor->text();
    double param;
    if (toDouble(val, param, 0.000001, true)) {
        m_action->setFactor(param);
    }
    m_action->updateOptions();
}

void LC_BlockInsertOptionsWidget::onColumnSpacingEditingFinished() {
    const auto val = ui->leColumnSpacing->text();
    double param;
    if (toDouble(val, param, 0.000001, true)) {
        m_action->setColumnSpacing(param);
    }
    m_action->updateOptions();
}

void LC_BlockInsertOptionsWidget::onRowSpacingEditingFinished() {
    const auto val = ui->leRowSpacing->text();
    double param;
    if (toDouble(val, param, 0.000001, true)) {
        m_action->setRowSpacing(param);
    }
    m_action->updateOptions();
}

void LC_BlockInsertOptionsWidget::onRowsValueChanged([[maybe_unused]] int number) {
    const int val = ui->sbRows->value();
    m_action->setRows(val);
    m_action->updateOptions();
}

void LC_BlockInsertOptionsWidget::onColumnsValueChanged([[maybe_unused]] int number) {
    const int val = ui->sbColumns->value();
    m_action->setColumns(val);
    m_action->updateOptions();
}
