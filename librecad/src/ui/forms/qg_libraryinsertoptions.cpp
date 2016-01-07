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
#include "qg_libraryinsertoptions.h"

#include "rs_actioninterface.h"
#include "rs_actionlibraryinsert.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_libraryinsertoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_LibraryInsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LibraryInsertOptions::QG_LibraryInsertOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_LibraryInsertOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LibraryInsertOptions::~QG_LibraryInsertOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LibraryInsertOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_LibraryInsertOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/LibraryInsert");
	RS_SETTINGS->writeEntry("/LibraryInsertAngle", ui->leAngle->text());
	RS_SETTINGS->writeEntry("/LibraryInsertFactor", ui->leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_LibraryInsertOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionLibraryInsert) {
		action = static_cast<RS_ActionLibraryInsert*>(a);

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/LibraryInsert");
            sAngle = RS_SETTINGS->readEntry("/LibraryInsertAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/LibraryInsertFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
	ui->leAngle->setText(sAngle);
	ui->leFactor->setText(sFactor);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LibraryInsertOptions::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_LibraryInsertOptions::updateData() {
    if (action) {
		action->setAngle(RS_Math::deg2rad(RS_Math::eval(ui->leAngle->text())));
		action->setFactor(RS_Math::eval(ui->leFactor->text()));
    }
}
