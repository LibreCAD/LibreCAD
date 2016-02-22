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

#include "qg_snapmiddleoptions.h"

#include <QVariant>
#include "ui_qg_snapmiddleoptions.h"

/*
 *  Constructs a QG_SnapMiddleOptions as a child of 'parent'
 *  and widget flags set to 'f'.
 *@i, number equidistant points, minimum 1, maximum 99
 *
 *@Author: Dongxu Li
 */
QG_SnapMiddleOptions::QG_SnapMiddleOptions(int& i, QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, middlePoints(&i)
	, ui(new Ui::Ui_SnapMiddleOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SnapMiddleOptions::~QG_SnapMiddleOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SnapMiddleOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_SnapMiddleOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Snap");
    RS_SETTINGS->writeEntry("/MiddlePoints", *middlePoints);
    RS_SETTINGS->endGroup();
}

void QG_SnapMiddleOptions::setMiddlePoints( int& i, bool initial) {
    middlePoints = &i;
	if (initial) {
    RS_SETTINGS->beginGroup("/Snap");
    *middlePoints=RS_SETTINGS->readNumEntry("/MiddlePoints", 1);
	if( !(*middlePoints>=1 && *middlePoints<=99)) {
        *middlePoints=1;
        RS_SETTINGS->writeEntry("/MiddlePoints", 1);
    }
	ui->sbMiddlePoints->setValue(*middlePoints);
    RS_SETTINGS->endGroup();
    } else {
		*middlePoints=ui->sbMiddlePoints->value();
    }

}

void QG_SnapMiddleOptions::updateMiddlePoints() {
	if (middlePoints) {
		*middlePoints = ui->sbMiddlePoints->value();
    }
}

void QG_SnapMiddleOptions::on_sbMiddlePoints_valueChanged(int i)
{
	if (middlePoints) {
        *middlePoints = i;/*
    RS_SETTINGS->beginGroup("/Snap");
    RS_SETTINGS->writeEntry("/MiddlePoints", *middlePoints);
    RS_SETTINGS->endGroup();*/
    }
}
//EOF
