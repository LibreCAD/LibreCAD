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
#include "qg_imageoptions.h"

#include "rs_actioninterface.h"
#include "rs_actiondrawimage.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "ui_qg_imageoptions.h"

/*
 *  Constructs a QG_ImageOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ImageOptions::QG_ImageOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_ImageOptions)
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ImageOptions::~QG_ImageOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ImageOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_ImageOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Image");
	RS_SETTINGS->writeEntry("/ImageAngle", ui->leAngle->text());
	RS_SETTINGS->writeEntry("/ImageFactor", ui->leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_ImageOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawImage) {
		action = static_cast<RS_ActionDrawImage*>(a);

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/Image");
            sAngle = RS_SETTINGS->readEntry("/ImageAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/ImageFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
	ui->leAngle->setText(sAngle);
	ui->leFactor->setText(sFactor);
        updateData();
        updateFactor();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ImageOptions::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_ImageOptions::updateData() {
    if (action) {
		action->setAngle(RS_Math::deg2rad(RS_Math::eval(ui->leAngle->text())));
    }
}

void QG_ImageOptions::updateDPI() {
    if (action) {

		double f = action->dpiToScale(RS_Math::eval(ui->leDPI->text()));
		ui->leFactor->blockSignals(true);
		ui->leFactor->setText(QString::number(f));
		ui->leFactor->blockSignals(false);
        action->setFactor(f);
    }
}

void QG_ImageOptions::updateFactor() {
    if (action) {
		double f = RS_Math::eval(ui->leFactor->text());
        double dpi = action->scaleToDpi(f);
		ui->leDPI->blockSignals(true);
		ui->leDPI->setText(QString::number(dpi));
		ui->leDPI->blockSignals(false);
        action->setFactor(f);
    }
}




