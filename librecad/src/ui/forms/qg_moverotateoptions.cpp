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
#include "qg_moverotateoptions.h"

#include "rs_actionmodifymoverotate.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_moverotateoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_MoveRotateOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MoveRotateOptions::QG_MoveRotateOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_MoveRotateOptions{})
{
	ui->setupUi(this);
}

void QG_MoveRotateOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Modify");
	RS_SETTINGS->writeEntry("/MoveRotate", ui->leAngle->text());
    RS_SETTINGS->endGroup();
}


void QG_MoveRotateOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionModifyMoveRotate) {
		action = static_cast<RS_ActionModifyMoveRotate*>(a);

        QString sa;
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sa = RS_SETTINGS->readEntry("/MoveRotate", "30");
            RS_SETTINGS->endGroup();
            action->setAngle(RS_Math::deg2rad(sa.toDouble()));
        }
		ui->leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CircleOptions::setAction: wrong action type");
		action = nullptr;
    }

}

void QG_MoveRotateOptions::updateAngle(const QString& a) {
    if (action) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}
/*
 *  Destroys the object and frees any allocated resources
 */
QG_MoveRotateOptions::~QG_MoveRotateOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MoveRotateOptions::languageChange()
{
	ui->retranslateUi(this);
}

