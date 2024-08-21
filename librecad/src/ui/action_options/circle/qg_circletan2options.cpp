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
QG_CircleTan2Options::QG_CircleTan2Options()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawCircleTan2,"Draw", "CircleTan2")
	 , ui(new Ui::Ui_CircleTan2Options{}){
    ui->setupUi(this);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &QG_CircleTan2Options::onRadiusEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleTan2Options::~QG_CircleTan2Options() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleTan2Options::languageChange(){
	ui->retranslateUi(this);
}

void QG_CircleTan2Options::doSaveSettings(){
	save("Radius", ui->leRadius->text());
}

void QG_CircleTan2Options::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionDrawCircleTan2 *>(a);
    QString radius;
    if (update){
        radius = fromDouble(action->getRadius());
    } else {
        radius = load("Radius", "1.0");
    }
    setRadiusToActionAndView(radius);
}


void QG_CircleTan2Options::setRadiusToActionAndView(QString val){
    double radius;
    if (toDouble(val, radius, 1.0, true)){
        action->setRadius(radius);
        ui->leRadius->setText(fromDouble(radius));
    }
}

void QG_CircleTan2Options::onRadiusEditingFinished(){
    setRadiusToActionAndView(ui->leRadius->text());
}
