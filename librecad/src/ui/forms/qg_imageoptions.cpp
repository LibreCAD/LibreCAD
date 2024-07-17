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
QG_ImageOptions::QG_ImageOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawImage, "/Image", "/Image")
    , ui(new Ui::Ui_ImageOptions)
{
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_ImageOptions::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &QG_ImageOptions::onFactorEditingFinished);
    connect(ui->leDPI, &QLineEdit::editingFinished, this, &QG_ImageOptions::onDpiEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ImageOptions::~QG_ImageOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ImageOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_ImageOptions::doSaveSettings() {
    save("Angle", ui->leAngle->text());
    save("Factor", ui->leFactor->text());
}

void QG_ImageOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionDrawImage*>(a);

    QString sAngle;
    QString sFactor;
    QString dpi;
    if (update) {
        sAngle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        sFactor = fromDouble(action->getFactor());
    } else {
        sAngle = load("/ImageAngle", "0.0");
        sFactor =load("/ImageFactor", "1.0");
    }
    setAngleToActionAndView(sAngle);
    setFactorToActionAndView(sFactor);
}

void QG_ImageOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_ImageOptions::onDpiEditingFinished() {

}

void QG_ImageOptions::onFactorEditingFinished() {
    setFactorToActionAndView(ui->leFactor->text());
}

void QG_ImageOptions::setAngleToActionAndView(QString val) {
    ui->leAngle->setText(val);
    // fixme - proper conversion
    action->setAngle(RS_Math::deg2rad(RS_Math::eval(val)));

}

void QG_ImageOptions::setFactorToActionAndView(QString val) {
    double f = action->dpiToScale(RS_Math::eval(val));
    ui->leFactor->setText(QString::number(f));
    action->setFactor(f);
}


/*void QG_ImageOptions::updateData() {
    if (action) {
		action->setAngle(RS_Math::deg2rad(RS_Math::eval(ui->leAngle->text())));
        saveSettings();
    }
}*/
/*
void QG_ImageOptions::setDPIToActionAndView(QString val) {
		double f = action->dpiToScale(RS_Math::eval(ui->leDPI->text()));
		ui->leFactor->blockSignals(true);
		ui->leFactor->setText(QString::number(f));
		ui->leFactor->blockSignals(false);
        action->setFactor(f);
}

void QG_ImageOptions::updateFactor() {

		double f = RS_Math::eval(ui->leFactor->text());
        double dpi = action->scaleToDpi(f);
		ui->leDPI->blockSignals(true);
		ui->leDPI->setText(QString::number(dpi));
		ui->leDPI->blockSignals(false);
        action->setFactor(f);


}
*/








