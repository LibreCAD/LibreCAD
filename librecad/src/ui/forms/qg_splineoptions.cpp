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
#include "qg_splineoptions.h"

#include "rs_actiondrawspline.h"
#include "lc_actiondrawsplinepoints.h"
#include "rs_settings.h"
#include "ui_qg_splineoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_SplineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SplineOptions::QG_SplineOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, action(nullptr)
	, ui(new Ui::Ui_SplineOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SplineOptions::~QG_SplineOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SplineOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_SplineOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/SplineDegree", ui->cbDegree->currentText().toInt());
	RS_SETTINGS->writeEntry("/SplineClosed", (int)ui->cbClosed->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_SplineOptions::setAction(RS_ActionInterface* a, bool update)
{
    if (a->rtti()!=RS2::ActionDrawSpline && a->rtti()!=RS2::ActionDrawSplinePoints)
    {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_SplineOptions::setAction: wrong action type");
		action = nullptr;
        return;
    }

    action = static_cast<RS_ActionDrawSpline*>(a);
    int degree = 3;
    bool closed;

    if (update)
    {
        if(a->rtti()==RS2::ActionDrawSpline)
        {
            degree = action->getDegree();
        }
        closed = action->isClosed();
    }
    else
    {
        RS_SETTINGS->beginGroup("/Draw");
        if(a->rtti()==RS2::ActionDrawSpline)
        {
            degree = RS_SETTINGS->readNumEntry("/SplineDegree", 3);
            action->setDegree(degree);
        }
        closed = RS_SETTINGS->readNumEntry("/SplineClosed", 0);
        RS_SETTINGS->endGroup();
        action->setClosed(closed);
    }
    if(a->rtti()==RS2::ActionDrawSpline)
    {
		ui->cbDegree->setCurrentIndex(ui->cbDegree->findText(QString::number(degree)));
		ui->lDegree->show();
		ui->cbDegree->show();
    }
    else
    {
		ui->lDegree->hide();
		ui->cbDegree->hide();
    }
	ui->cbClosed->setChecked(closed);
}

void QG_SplineOptions::setClosed(bool c) {
    if (action) action->setClosed(c);
}

void QG_SplineOptions::undo() {
    if (action) action->undo();
}

void QG_SplineOptions::setDegree(const QString& deg) {
    if (action) {
        action->setDegree(deg.toInt());
    }
}
