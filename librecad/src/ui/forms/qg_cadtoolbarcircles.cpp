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
#include "qg_cadtoolbarcircles.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarCircles as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarCircles::QG_CadToolBarCircles(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
    parentTB=static_cast<QG_CadToolBar*>(parent);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarCircles::~QG_CadToolBarCircles()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarCircles::languageChange()
{
    retranslateUi(this);
}

void QG_CadToolBarCircles::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarCircles::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarCircles::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarCircles::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarCircles::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarCircles::drawCircle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle();
    }
}

void QG_CadToolBarCircles::drawCircleCR() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleCR();
    }
}

void QG_CadToolBarCircles::drawCircle2P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle2P();
    }
}

void QG_CadToolBarCircles::drawCircle3P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle3P();
    }
}

void QG_CadToolBarCircles::drawCircle1_2P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleTan1_2P();
    }
}

void QG_CadToolBarCircles::drawCircle2_1P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleTan2_1P();
    }
}
void QG_CadToolBarCircles::drawCircleParallel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleParallel();
    }
}
void QG_CadToolBarCircles::drawCircleInscribe() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleInscribe();
    }
}
void QG_CadToolBarCircles::drawCircleTan2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleTan2();
    }
}
void QG_CadToolBarCircles::drawCircleTan3() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleTan3();
    }
}
void QG_CadToolBarCircles::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
//restore action from checked button
void QG_CadToolBarCircles::restoreAction()
{
    if(actionHandler==NULL) return;
    if ( bCircle ->isChecked() ) {
        actionHandler->slotDrawCircle();
        return;
    }
    if ( bCircleCR ->isChecked() ) {
        actionHandler->slotDrawCircleCR();
        return;
    }
    if ( bCircle2P ->isChecked() ) {
        actionHandler->slotDrawCircle2P();
        return;
    }
    if ( bCircle3P ->isChecked() ) {
        actionHandler->slotDrawCircle3P();
        return;
    }
    if ( bCircleParallel ->isChecked() ) {
        actionHandler->slotDrawCircleParallel();
        return;
    }
    if ( bCircleInscribe ->isChecked() ) {
        actionHandler->slotDrawCircleInscribe();
        return;
    }
    if ( bCircleTan2 ->isChecked() ) {
        actionHandler->slotDrawCircleTan2();
        return;
    }
    if ( bCircleTan3 ->isChecked() ) {
        actionHandler->slotDrawCircleTan3();
        return;
    }
    //clear all action
    bHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
    if(currentAction != NULL) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarCircles::resetToolBar() {
    bHidden->setChecked(true);
}
void QG_CadToolBarCircles::on_bBack_clicked()
{
   parentTB->showPreviousToolBar();
}

void QG_CadToolBarCircles::showCadToolBar(RS2::ActionType actionType){
    switch(actionType){
    case RS2::ActionDrawCircle:
        bCircle->setChecked(true);
        return;
    case RS2::ActionDrawCircle2P:
        bCircle2P->setChecked(true);
        return;
    case RS2::ActionDrawCircle3P:
        bCircle3P->setChecked(true);
        return;
    case RS2::ActionDrawCircleCR:
        bCircleCR->setChecked(true);
        return;
    case RS2::ActionDrawCircleParallel:
        bCircleParallel->setChecked(true);
        return;
    case RS2::ActionDrawCircleInscribe:
        bCircleInscribe->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan2:
        bCircleTan2->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan3:
        bCircleTan3->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan1_2P:
        bCircleTan1_2P->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan2_1P:
        bCircleTan2_1P->setChecked(true);
        return;
    default:
        bHidden->setChecked(true);
        return;
    }
}
//EOF
