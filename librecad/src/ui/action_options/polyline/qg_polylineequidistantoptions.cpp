/****************************************************************************
**
  * Create option widget used to draw equidistant polylines

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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
#include "qg_polylineequidistantoptions.h"
#include "ui_qg_polylineequidistantoptions.h"

#include "rs_actionpolylineequidistant.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_actionpolylineequidistant.h"
#include "ui_qg_polylineequidistantoptions.h"
#include "rs_debug.h"

/*
  * Create option widget used to draw equidistant polylines
  *
  *@Author Dongxu Li
 */
QG_PolylineEquidistantOptions::QG_PolylineEquidistantOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionPolylineEquidistant, "Draw", "PolylineEquidistant"),
    ui{new Ui::PolylineEquidistantOptions{}}{
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &QG_PolylineEquidistantOptions::onDistEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &QG_PolylineEquidistantOptions::onNumberValueChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineEquidistantOptions::~QG_PolylineEquidistantOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineEquidistantOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_PolylineEquidistantOptions::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionPolylineEquidistant *>(a);

    QString distance;
    int number;

    // settings from action:
    if (update){
        distance = fromDouble(action->getDist());
        number = action->getNumber();
    }
    else {
        distance = load("Dist", "10.0");
        number = loadInt("Copies", 1);
    }

    setDistanceToActionAndView(distance);
    setNumberToActionAndView(number);
}

void QG_PolylineEquidistantOptions::doSaveSettings(){
    save("Dist", ui->leDist->text());
    save("Copies", ui->sbNumber->value());
}

void QG_PolylineEquidistantOptions::onNumberValueChanged(int number){
    setNumberToActionAndView(number);
}

void QG_PolylineEquidistantOptions::onDistEditingFinished(){
    setDistanceToActionAndView(ui->leDist->text());
}

void QG_PolylineEquidistantOptions::setNumberToActionAndView(int number){
    action->setNumber(number);
    ui->sbNumber->setValue(number);
}

void QG_PolylineEquidistantOptions::setDistanceToActionAndView(QString strVal){
    double val;
    if (toDouble(strVal, val, 10.0, false)){
        action->setDist(val);
        ui->leDist->setText(fromDouble(val));
    }
}
