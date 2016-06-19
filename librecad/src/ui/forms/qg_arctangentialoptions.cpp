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
#include<cmath>
#include <QAction>
#include "qg_arctangentialoptions.h"

#include "rs_actiondrawarctangential.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"

#include "ui_qg_arctangentialoptions.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/*
 *  Constructs a QG_ArcTangentialOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcTangentialOptions::QG_ArcTangentialOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_ArcTangentialOptions{})
{
	ui->setupUi(this);
}

QG_ArcTangentialOptions::~QG_ArcTangentialOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcTangentialOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_ArcTangentialOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/ArcTangentialRadius", ui->leRadius->text());
	RS_SETTINGS->writeEntry("/ArcTangentialAngle", ui->leAngle->text());
	if(ui->rbRadius->isChecked()) {
		RS_SETTINGS->writeEntry("/ArcTangentialByRadius", QString{"1"});
    }else {
		RS_SETTINGS->writeEntry("/ArcTangentialByRadius", QString{"0"});
    }
    RS_SETTINGS->endGroup();
//    delete leRadius->validator();
//    delete leAngle->validator();
}

void QG_ArcTangentialOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawArcTangential) {
		action = static_cast<RS_ActionDrawArcTangential*>(a);

        QString sr,sa;
        bool bbr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
            sa = QString("%1").arg(action->getAngle()*180./M_PI);
            bbr= action->getByRadius();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/ArcTangentialRadius", "1.0");
            sa = RS_SETTINGS->readEntry("/ArcTangentialAngle", "90");
            int br = RS_SETTINGS->readNumEntry("/ArcTangentialByRadius", 1);
            bbr = ( br != 0 );
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
            action->setAngle(sa.toDouble()*M_PI/180.);
            action->setByRadius(bbr);
        }
		ui->leRadius->setText(sr);
		ui->leAngle->setText(sa);
        updateByRadius(bbr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_ArcTangentialOptions::setAction: wrong action type");
		action = nullptr;
    }
}


/*void QG_ArcTangentialOptions::init() {
	data = nullptr;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();

    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcTangentialOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcTangentialOptions::updateRadius(const QString& s) {
	ui->leRadius->setText(s);
}

void QG_ArcTangentialOptions::updateAngle(const QString& s) {
	ui->leAngle->setText(s);
}
void QG_ArcTangentialOptions::updateByRadius(const bool br) {
		ui->rbRadius->setChecked(br);
		ui->rbAngle->setChecked(!br);
		ui->leRadius->setDisabled(!br);
		ui->leAngle->setDisabled(br);
}

void QG_ArcTangentialOptions::on_leRadius_editingFinished()
{
	if(ui->rbRadius->isChecked()) {
        bool ok;
		double d=fabs(RS_Math::eval(ui->leRadius->text(), &ok));
        if(!ok) return;
        if (d<RS_TOLERANCE) d=1.0;
        //updateRadius(QString::number(d,'g',5));
        action->setRadius(d);
        action->setByRadius(true);
		ui->leRadius->setText(QString::number(d,'g', 5));
    }
}

void QG_ArcTangentialOptions::on_leAngle_editingFinished()
{
	if(ui->rbAngle->isChecked()) {
        bool ok;
        double d=RS_Math::correctAngle(RS_Math::eval(ui->leAngle->text(), &ok)*M_PI/180.);
        if(!ok) return;
        if(d<RS_TOLERANCE_ANGLE || d + RS_TOLERANCE_ANGLE > 2. * M_PI) d=M_PI; // can not do full circle
        action->setAngle(d);
        //updateAngle(QString::number(d*180./M_PI,'g',5));
        action->setByRadius(false);
        ui->leAngle->setText(QString::number(RS_Math::rad2deg( d),'g',5));
    }
}

void QG_ArcTangentialOptions::on_rbRadius_clicked(bool /*checked*/)
{
    action->setByRadius(true);
	action->setRadius(ui->leRadius->text().toDouble());
    updateByRadius(true);
}

void QG_ArcTangentialOptions::on_rbAngle_clicked(bool /*checked*/)
{
    action->setByRadius(false);
	action->setAngle(ui->leAngle->text().toDouble()*M_PI/180.);
    updateByRadius(false);
}
