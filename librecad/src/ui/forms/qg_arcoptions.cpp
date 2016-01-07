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
#include "qg_arcoptions.h"

#include "rs_actiondrawarc.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include "ui_qg_arcoptions.h"

/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcOptions::QG_ArcOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_ArcOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ArcOptions::~QG_ArcOptions() {
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_ArcOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/ArcReversed", (int)ui->rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_ArcOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawArc) {
		action = static_cast<RS_ActionDrawArc*>(a);

        bool reversed;
        if (update) {
            reversed = action->isReversed();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
            RS_SETTINGS->endGroup();
            action->setReversed(reversed);
        }
		ui->rbNeg->setChecked(reversed);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_ArcOptions::setAction: wrong action type");
		action = nullptr;
    }

}


/*void QG_ArcOptions::init() {
	data = nullptr;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();

    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcOptions::updateDirection(bool /*pos*/) {
    if (action) {
		action->setReversed(!(ui->rbPos->isChecked()));
    }
}
