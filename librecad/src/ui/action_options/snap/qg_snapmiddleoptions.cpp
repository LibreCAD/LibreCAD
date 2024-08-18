/****************************************************************************
**
Construct option widget for equidistant points on entity

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

#include <QVariant>
#include "qg_snapmiddleoptions.h"

#include "ui_qg_snapmiddleoptions.h"

/*
 *  Constructs a QG_SnapMiddleOptions as a child of 'parent'
 *  and widget flags set to 'f'.
 *@i, number equidistant points, minimum 1, maximum 99
 *
 *@Author: Dongxu Li
 */

QG_SnapMiddleOptions::QG_SnapMiddleOptions(QWidget* parent)
    : QWidget(parent)
    , middlePoints(nullptr)
    , ui(new Ui::Ui_SnapMiddleOptions{}){
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SnapMiddleOptions::~QG_SnapMiddleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SnapMiddleOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_SnapMiddleOptions::saveSettings() {
    LC_SET_ONE("Snap", "MiddlePoints", *middlePoints);
}

void QG_SnapMiddleOptions::useMiddlePointsValue( int* i) {
    middlePoints = i;
    LC_GROUP_GUARD("Snap");
    {
        int points = LC_GET_INT("MiddlePoints", 1);
        if (!(points >= 1 && points <= 99)) {
            points = 1;
            LC_SET("MiddlePoints", points);
        }


        ui->sbMiddlePoints->setValue(points);
        *middlePoints = points;
    }
}

void QG_SnapMiddleOptions::on_sbMiddlePoints_valueChanged(int i){
    if (middlePoints) {
        *middlePoints = i;
        saveSettings();
    }
}

void QG_SnapMiddleOptions::doShow() {
    bool requestFocus = !isVisible();
    show();
    if (requestFocus){
        ui->sbMiddlePoints->setFocus();
    }
}

int *QG_SnapMiddleOptions::getMiddlePointsValue() {
    return middlePoints;
}
