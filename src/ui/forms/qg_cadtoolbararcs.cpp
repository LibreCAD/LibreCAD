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
#include "qg_cadtoolbararcs.h"


#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarArcs as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarArcs::QG_CadToolBarArcs(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
    parentTB=static_cast<QG_CadToolBar*>(parent);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarArcs::~QG_CadToolBarArcs()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarArcs::languageChange()
{
    retranslateUi(this);
}

#include <QContextMenuEvent>

void QG_CadToolBarArcs::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

/*void QG_CadToolBarArcs::mousePressEvent(QMouseEvent* e) {
    if (e->button()==RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}*/

void QG_CadToolBarArcs::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarArcs::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarArcs::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarArcs::drawArc() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawArc();
    }
}

void QG_CadToolBarArcs::drawArc3P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawArc3P();
    }
}

void QG_CadToolBarArcs::drawArcParallel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawArcParallel();
    }
}

void QG_CadToolBarArcs::drawArcTangential() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawArcTangential();
    }
}

//restore action from checked button
void QG_CadToolBarArcs::restoreAction()
{
    if(actionHandler==NULL) return;
    if ( bArc ->isChecked() ) {
        actionHandler->slotDrawArc();
        return;
    }
    if ( bArc3P ->isChecked() ) {
        actionHandler->slotDrawArc3P();
        return;
    }
    if ( bArcParallel ->isChecked() ) {
        actionHandler->slotDrawArcParallel();
        return;
    }
    if ( bArcTangential ->isChecked() ) {
        actionHandler->slotDrawArcTangential();
        return;
    }
    //clear all action
    bHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
    if(currentAction != NULL) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarArcs::resetToolBar() {
    bHidden->setChecked(true);
}
void QG_CadToolBarArcs::on_bBack_clicked()
{
   parentTB->showPreviousToolBar();
}
void QG_CadToolBarArcs::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->showPreviousToolBar();
    }
}

void QG_CadToolBarArcs::showCadToolBar(RS2::ActionType actionType) {
    switch(actionType){
    case RS2::ActionDrawArc:
        bArc->setChecked(true);
        return;
    case RS2::ActionDrawArc3P:
        bArc3P->setChecked(true);
        return;
    case RS2::ActionDrawArcParallel:
        bArcParallel->setChecked(true);
        return;
    case RS2::ActionDrawArcTangential:
        bArcTangential->setChecked(true);
        return;
        default:
        bHidden->setChecked(true);
        return;
    }
}
//EOF
