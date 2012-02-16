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
#include "qg_roundoptions.h"

#include "rs_actionmodifyround.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_RoundOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_RoundOptions::QG_RoundOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_RoundOptions::~QG_RoundOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_RoundOptions::languageChange()
{
    retranslateUi(this);
}

void QG_RoundOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/RoundRadius", leRadius->text());
    RS_SETTINGS->writeEntry("/RoundTrim", (int)cbTrim->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_RoundOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyRound) {
        action = (RS_ActionModifyRound*)a;

        QString sr;
	QString st;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
            st = QString("%1").arg((int)action->isTrimOn());
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sr = RS_SETTINGS->readEntry("/RoundRadius", "1.0");
            st = RS_SETTINGS->readEntry("/RoundTrim", "1");
            RS_SETTINGS->endGroup();
        }
	leRadius->setText(sr);
    	cbTrim->setChecked(st=="1");
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineParallelOptions::setAction: wrong action type");
        action = NULL;
    }
}


void QG_RoundOptions::updateData() {
    if (action!=NULL) {
        action->setTrim(cbTrim->isChecked());
        action->setRadius(RS_Math::eval(leRadius->text()));
    }
}
