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
#include "ui_qg_imageoptions.h"

/*
 *  Constructs a QG_ImageOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ImageOptions::QG_ImageOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawImage, "Image", "Image")
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
    if (update) {
        sAngle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        sFactor = fromDouble(action->getFactor());
    } else {
        sAngle = load("Angle", "0.0");
        sFactor =load("Factor", "1.0");
    }
    setAngleToActionAndView(sAngle);
    setFactorToActionAndView(sFactor);
}

void QG_ImageOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_ImageOptions::onDpiEditingFinished() {
    setDPIToActionAndView(ui->leDPI->text());
}

void QG_ImageOptions::onFactorEditingFinished() {
    setFactorToActionAndView(ui->leFactor->text());
}

void QG_ImageOptions::setDPIToActionAndView(const QString& val) {
//    double dpi = RS_Math::eval(val);
    double dpi;
    bool ok = toDouble(val, dpi, 72, true);
    if (ok) {
        double factor = action->dpiToScale(dpi);
        ui->leFactor->blockSignals(true);
        ui->leFactor->setText(QString::number(factor));
        ui->leFactor->blockSignals(false);
        action->setFactor(factor);
        ui->leDPI->blockSignals(true);
        ui->leDPI->setText(fromDouble(dpi));
        ui->leDPI->blockSignals(false);
    }
}

void QG_ImageOptions::setAngleToActionAndView(const QString& val) {
    double angleDegree = RS_Math::eval(val);
    action->setAngle(RS_Math::deg2rad(angleDegree));
    ui->leAngle->setText(val);
}

void QG_ImageOptions::setFactorToActionAndView(const QString& val) {
//    double factor = RS_Math::eval(val);
    double factor;
    bool ok = toDouble(val, factor, 1.0, true);
    if (ok) {
        double dpi = action->scaleToDpi(factor);
        ui->leDPI->blockSignals(true);
        ui->leDPI->setText(QString::number(dpi));
        ui->leDPI->blockSignals(false);
        ui->leFactor->blockSignals(true);
        ui->leFactor->setText(fromDouble(factor));
        ui->leFactor->blockSignals(false);
        action->setFactor(factor);
    }
}
