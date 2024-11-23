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
#include "rs_settings.h"
#include "ui_qg_splineoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_SplineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SplineOptions::QG_SplineOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionNone, "Draw", "Spline"),
    action(nullptr), ui(new Ui::Ui_SplineOptions{}){
    ui->setupUi(this);

    connect(ui->cbDegree, SIGNAL(currentIndexChanged(int)), SLOT(onDegreeIndexChanged(int)));
    connect(ui->cbClosed, &QCheckBox::clicked,  this, &QG_SplineOptions::onClosedClicked);
    connect(ui->bUndo, &QToolButton::clicked,  this, &QG_SplineOptions::undo);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SplineOptions::~QG_SplineOptions() = default;

bool QG_SplineOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawSpline || actionType == RS2::ActionDrawSplinePoints;
}

void QG_SplineOptions::doSaveSettings(){
    bool drawSplineAction = action->rtti() == RS2::ActionDrawSpline;
    if (drawSplineAction){
        save("Degree", ui->cbDegree->currentText().toInt());
    }
    save("Closed", (int) ui->cbClosed->isChecked());
}

void QG_SplineOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionDrawSpline *>(a);
    int degree = 3;
    bool closed;

    bool drawSplineAction = a->rtti() == RS2::ActionDrawSpline;

    if (update){
        degree = action->getDegree();
        closed = action->isClosed();
    } else {
        degree = loadInt("Degree", 3);
        closed = loadBool("Closed", false);
    }
    ui->lDegree->setVisible(drawSplineAction);
    if (drawSplineAction){
        setDegreeToActionAndView(degree);
    }
    ui->cbDegree->setVisible(drawSplineAction);
    setClosedToActionAndView(closed);
}

void QG_SplineOptions::onClosedClicked(bool value){
    setClosedToActionAndView(value);
}

void QG_SplineOptions::undo(){
    if (action) action->undo();
}

void QG_SplineOptions::onDegreeIndexChanged(int index){
    setDegreeToActionAndView(index+1);
}

void QG_SplineOptions::setClosedToActionAndView(bool closed){
    ui->cbClosed->setChecked(closed);
    action->setClosed(closed);
}

void QG_SplineOptions::setDegreeToActionAndView(int degree){
    ui->cbDegree->setCurrentIndex(degree-1);
    action->setDegree(degree);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SplineOptions::languageChange(){
    ui->retranslateUi(this);
}
