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
#include "qg_circleoptions.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_actiondrawcirclecr.h"
#include "ui_qg_circleoptions.h"

/*
 *  Constructs a QG_CircleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CircleOptions::QG_CircleOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_CircleOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleOptions::~QG_CircleOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_CircleOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/CircleRadius", ui->leRadius->text());
    RS_SETTINGS->endGroup();
}

void QG_CircleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && ( a->rtti()==RS2::ActionDrawCircleCR ||  a->rtti()==RS2::ActionDrawCircle2PR) ) {
        action = static_cast<RS_ActionDrawCircleCR*>(a);

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
        }
		ui->leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CircleOptions::setAction: wrong action type");
		action = nullptr;
    }

}


/*void QG_CircleOptions::setData(RS_CircleData* d) {
    data = d;

    RS_SETTINGS->beginGroup("/Draw");
    QString r = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
    RS_SETTINGS->endGroup();

    leRadius->setText(r);
}*/

void QG_CircleOptions::updateRadius(const QString& r) {
    if (action) {
        action->setRadius(RS_Math::eval(r));
    }
}
