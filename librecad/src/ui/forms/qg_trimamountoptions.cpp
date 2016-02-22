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
#include "qg_trimamountoptions.h"

#include "rs_actionmodifytrimamount.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_trimamountoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_TrimAmountOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TrimAmountOptions::QG_TrimAmountOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_TrimAmountOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TrimAmountOptions::~QG_TrimAmountOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TrimAmountOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_TrimAmountOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Modify");
	RS_SETTINGS->writeEntry("/TrimAmount", ui->leDist->text());
	QString const total = ui->cbTotalLength->isChecked()?"1":"0";
	RS_SETTINGS->writeEntry("/TrimAmountTotal", total);
    RS_SETTINGS->endGroup();
}

void QG_TrimAmountOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionModifyTrimAmount) {
		action = static_cast<RS_ActionModifyTrimAmount*>(a);

        QString sd;
        bool byTotal;
        // settings from action:
        if (update) {
            sd = QString("%1").arg(action->getDistance());
            byTotal=action->getByTotal();
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Modify");
            sd = RS_SETTINGS->readEntry("/TrimAmount", "1.0");
            byTotal= (RS_SETTINGS->readEntry("/TrimAmountTotal", "0") == QString("1"));
            RS_SETTINGS->endGroup();
        }

		ui->leDist->setText(sd);
		//initialize trim amount distance for action
		updateDist(sd);

		ui->cbTotalLength->setChecked(byTotal);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_ModifyTrimAmountOptions::setAction: wrong action type");
		this->action = nullptr;
    }
}

void QG_TrimAmountOptions::updateDist(const QString& d) {
    if (action) {
        action->setDistance(RS_Math::eval(d, 1.0));
    }
}

void QG_TrimAmountOptions::on_leDist_editingFinished()
{
	updateDist(ui->leDist->text());
}

void QG_TrimAmountOptions::on_cbTotalLength_toggled(bool checked)
{
	if (action) {
		action->setByTotal(checked);
	}
}
