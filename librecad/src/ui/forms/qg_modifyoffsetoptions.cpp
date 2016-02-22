/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

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

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "qg_modifyoffsetoptions.h"

#include "rs_actionmodifyoffset.h"
#include "rs_settings.h"
#include "ui_qg_modifyoffsetoptions.h"
#include "rs_math.h"

/*
 *  Constructs a QG_ModifyOffsetOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ModifyOffsetOptions::QG_ModifyOffsetOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_ModifyOffsetOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ModifyOffsetOptions::~QG_ModifyOffsetOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ModifyOffsetOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_ModifyOffsetOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/ModifyOffsetDistance", ui->leDist->text());
    RS_SETTINGS->endGroup();
}

void QG_ModifyOffsetOptions::setDist(double& d, bool initial) {
    dist = &d;
        bool ok;
    if(initial) {
        RS_SETTINGS->beginGroup("/Draw");
        QString r = RS_SETTINGS->readEntry("/ModifyOffsetDistance", "1.0");
        RS_SETTINGS->endGroup();

		ui->leDist->setText(r);
        *dist=RS_Math::eval(r,&ok);
		if (!ok) *dist=1.;
    } else {
		*dist=RS_Math::eval(ui->leDist->text(),&ok);
		if (!ok) *dist=1.;
    }
}

void QG_ModifyOffsetOptions::updateDist(const QString& d) {
    if (dist) {
        *dist=RS_Math::eval(d);
    }
}
