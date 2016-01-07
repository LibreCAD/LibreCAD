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
QG_InsertOptions::QG_InsertOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_InsertOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_InsertOptions::~QG_InsertOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_InsertOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_InsertOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Insert");
	RS_SETTINGS->writeEntry("/InsertAngle", ui->leAngle->text());
	RS_SETTINGS->writeEntry("/InsertFactor", ui->leFactor->text());
	RS_SETTINGS->writeEntry("/InsertColumns", ui->sbColumns->text());
	RS_SETTINGS->writeEntry("/InsertRows", ui->sbRows->text());
	RS_SETTINGS->writeEntry("/InsertColumnSpacing", ui->leColumnSpacing->text());
	RS_SETTINGS->writeEntry("/InsertRowSpacing", ui->leRowSpacing->text());
    RS_SETTINGS->endGroup();
}

void QG_InsertOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionBlocksInsert) {
		action = static_cast<RS_ActionBlocksInsert*>(a);

        QString sAngle;
        QString sFactor;
	QString sColumns;
    	QString sRows;
        QString sColumnSpacing;
        QString sRowSpacing;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
            sColumns = QString("%1").arg(action->getColumns());
            sRows = QString("%1").arg(action->getRows());
            sColumnSpacing = QString("%1").arg(action->getColumnSpacing());
            sRowSpacing = QString("%1").arg(action->getRowSpacing());
        } else {
            RS_SETTINGS->beginGroup("/Insert");
            sAngle = RS_SETTINGS->readEntry("/InsertAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/InsertFactor", "1.0");
            sColumns = RS_SETTINGS->readEntry("/InsertColumns", "1");
            sRows = RS_SETTINGS->readEntry("/InsertRows", "1");
            sColumnSpacing = RS_SETTINGS->readEntry("/InsertColumnSpacing", "1.0");
            sRowSpacing = RS_SETTINGS->readEntry("/InsertRowSpacing", "1.0");
            RS_SETTINGS->endGroup();
        }
	ui->leAngle->setText(sAngle);
	ui->leFactor->setText(sFactor);
		ui->sbColumns->setValue(sColumns.toInt());
		ui->sbRows->setValue(sRows.toInt());
		ui->leColumnSpacing->setText(sColumnSpacing);
		ui->leRowSpacing->setText(sRowSpacing);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_InsertOptions::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_InsertOptions::updateData() {
    if (action) {
		action->setAngle(RS_Math::deg2rad(RS_Math::eval(ui->leAngle->text())));
		action->setFactor(RS_Math::eval(ui->leFactor->text()));
		action->setColumns(ui->sbColumns->value());
		action->setRows(ui->sbRows->value());
		action->setColumnSpacing(RS_Math::eval(ui->leColumnSpacing->text()));
		action->setRowSpacing(RS_Math::eval(ui->leRowSpacing->text()));
    }
}
