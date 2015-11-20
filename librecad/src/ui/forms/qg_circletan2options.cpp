/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2012 LibreCAD.org

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "qg_circletan2options.h"

#include "rs_actiondrawcircletan2.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"

#include "ui_qg_circletan2options.h"

/*
 *  Constructs a QG_CircleTan2Options as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CircleTan2Options::QG_CircleTan2Options(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_CircleTan2Options{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleTan2Options::~QG_CircleTan2Options()
{
	saveSettings();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleTan2Options::languageChange()
{
	ui->retranslateUi(this);
}

void QG_CircleTan2Options::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/CircleTan2Radius", ui->leRadius->text());
    RS_SETTINGS->endGroup();
}

void QG_CircleTan2Options::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawCircleTan2) {
		action = static_cast<RS_ActionDrawCircleTan2*>(a);

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/CircleTan2Radius", "1.0");
            RS_SETTINGS->endGroup();
        }
		ui->leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
            "QG_CircleTan2Options::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_CircleTan2Options::updateRadius(const QString& r) {
    if (action) {
        bool ok;
        double radius=RS_Math::eval(r,&ok);
        if(ok){
            action->setRadius(radius);
        }/*else{
			ui->leRadius->setText("10.0");
        }*/
    }
}

