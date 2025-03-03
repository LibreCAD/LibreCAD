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
#include "qg_insertoptions.h"

#include "rs_actioninterface.h"
#include "rs_actionblocksinsert.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_insertoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_InsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_InsertOptions::QG_InsertOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionBlocksInsert, "Insert", "Insert")
	, ui(new Ui::Ui_InsertOptions{}){
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_InsertOptions::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &QG_InsertOptions::onFactorEditingFinished);
    connect(ui->leColumnSpacing, &QLineEdit::editingFinished, this, &QG_InsertOptions::onColumnSpacingEditingFinished);
    connect(ui->leRowSpacing, &QLineEdit::editingFinished, this, &QG_InsertOptions::onRowSpacingEditingFinished);
    connect(ui->sbRows, &QSpinBox::valueChanged, this, &QG_InsertOptions::onRowsValueChanged);
    connect(ui->sbColumns, &QSpinBox::valueChanged, this, &QG_InsertOptions::onColumnsValueChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_InsertOptions::~QG_InsertOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_InsertOptions::languageChange(){
	ui->retranslateUi(this);
}

void QG_InsertOptions::doSaveSettings() {
    save("Angle", ui->leAngle->text());
    save("Factor", ui->leFactor->text());
    save("Columns", ui->sbColumns->text());
    save("Rows", ui->sbRows->text());
    save("ColumnSpacing", ui->leColumnSpacing->text());
    save("RowSpacing", ui->leRowSpacing->text());
}

void QG_InsertOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionBlocksInsert*>(a);

    QString angle;
    QString factor;
    int columns;
    int rows;
    QString columnSpacing;
    QString rowSpacing;
    if (update) {
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        factor = fromDouble(action->getFactor());
        columns = action->getColumns();
        rows = action->getRows();
        columnSpacing = fromDouble(action->getColumnSpacing());
        rowSpacing = fromDouble(action->getRowSpacing());
    } else {
        angle = load("Angle", "0.0");
        factor = load("Factor", "1.0");
        columns = loadInt("Columns", 1);
        rows = loadInt("Rows", 1);
        columnSpacing = load("ColumnSpacing", "1.0");
        rowSpacing = load("RowSpacing", "1.0");
    }
    setAngleToActionAndView(angle);
    setFactorToActionAndView(factor);
    setColumnsToActionAndView(columns);
    setRowsToActionAndView(rows);
    setColumnSpacingActionAndView(columnSpacing);
    setRowSpacingToActionAndView(rowSpacing);
}

// fixme - use proper string to double conversions

void QG_InsertOptions::setRowSpacingToActionAndView(QString val) {
    ui->leRowSpacing->setText(val);
    action->setRowSpacing(RS_Math::eval(val));
}

void QG_InsertOptions::setColumnSpacingActionAndView(QString val) {
    ui->leColumnSpacing->setText(val);
    action->setColumnSpacing(RS_Math::eval(val));
}

void QG_InsertOptions::setColumnsToActionAndView(int columns) {
    action->setColumns(columns);
    ui->sbColumns->setValue(columns);
}

void QG_InsertOptions::setRowsToActionAndView(int rows) {
    ui->sbRows->setValue(rows);
    action->setRows(rows);
}

void QG_InsertOptions::setFactorToActionAndView(QString val) {
    ui->leFactor->setText(val);
    action->setFactor(RS_Math::eval(val));
}

void QG_InsertOptions::setAngleToActionAndView(QString val) {
    ui->leAngle->setText(val);
    action->setAngle(RS_Math::deg2rad(RS_Math::eval(val)));
}

void QG_InsertOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_InsertOptions::onFactorEditingFinished(){
    setAngleToActionAndView(ui->leFactor->text());
}

void QG_InsertOptions::onColumnSpacingEditingFinished() {
    setColumnSpacingActionAndView(ui->leColumnSpacing->text());
}

void QG_InsertOptions::onRowSpacingEditingFinished() {
    setRowSpacingToActionAndView(ui->leRowSpacing->text());
}

void QG_InsertOptions::onRowsValueChanged([[maybe_unused]]int number) {
    setRowsToActionAndView(ui->sbRows->value());
}

void QG_InsertOptions::onColumnsValueChanged([[maybe_unused]]int number) {
    setColumnsToActionAndView(ui->sbColumns->value());
}
