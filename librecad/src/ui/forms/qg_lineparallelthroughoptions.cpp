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
#include "qg_lineparallelthroughoptions.h"

#include "rs_actiondrawlineparallelthrough.h"
#include "rs_settings.h"
#include "ui_qg_lineparallelthroughoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_LineParallelThroughOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineParallelThroughOptions::QG_LineParallelThroughOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_LineParallelThroughOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineParallelThroughOptions::~QG_LineParallelThroughOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineParallelThroughOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_LineParallelThroughOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/LineParallelNumber", ui->sbNumber->text());
    RS_SETTINGS->endGroup();
}

void QG_LineParallelThroughOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawLineParallelThrough) {
        action = (RS_ActionDrawLineParallelThrough*)a;

        QString sn;
        if (update) {
            sn = QString("%1").arg(action->getNumber());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sn = RS_SETTINGS->readEntry("/LineParallelNumber", "1");
            RS_SETTINGS->endGroup();
        }
		ui->sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineParallelThroughOptions::setAction: wrong action type");
		action = nullptr;
    }

}

void QG_LineParallelThroughOptions::updateNumber(int n) {
    if (action) {
        action->setNumber(n);
    }
}
