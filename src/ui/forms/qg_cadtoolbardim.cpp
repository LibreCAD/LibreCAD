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
#include "qg_cadtoolbardim.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarDim as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarDim::QG_CadToolBarDim(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
    parentTB=static_cast<QG_CadToolBar*>(parent);
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarDim::~QG_CadToolBarDim()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarDim::languageChange()
{
    retranslateUi(this);
}

void QG_CadToolBarDim::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarDim::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarDim::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarDim::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarDim::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarDim::drawDimAligned() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimAligned();
    }
}

void QG_CadToolBarDim::drawDimLinear() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinear();
    }
}

void QG_CadToolBarDim::drawDimLinearHor() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinearHor();
    }
}

void QG_CadToolBarDim::drawDimLinearVer() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinearVer();
    }
}

void QG_CadToolBarDim::drawDimRadial() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimRadial();
    }
}

void QG_CadToolBarDim::drawDimDiametric() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimDiametric();
    }
}

void QG_CadToolBarDim::drawDimAngular() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimAngular();
    }
}

void QG_CadToolBarDim::drawDimLeader() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLeader();
    }
}

//void QG_CadToolBarDim::back() {
//    if (cadToolBar!=NULL) {
//        cadToolBar->back();
//    }
//}
//restore action from checked button
void QG_CadToolBarDim::restoreAction()
{
    if(actionHandler==NULL) return;
    if ( bAligned ->isChecked() ) {
        actionHandler->slotDimAligned();
        return;
    }
    if ( bLinear ->isChecked() ) {
        actionHandler->slotDimLinear();
        return;
    }
    if ( bLinearHor ->isChecked() ) {
        actionHandler->slotDimLinearHor();
        return;
    }
    if ( bLinearVer ->isChecked() ) {
        actionHandler->slotDimLinearVer();
        return;
    }
    if ( bRadial ->isChecked() ) {
        actionHandler->slotDimRadial();
        return;
    }
    if ( bDiametric ->isChecked() ) {
        actionHandler->slotDimDiametric();
        return;
    }
    if ( bAngular ->isChecked() ) {
        actionHandler->slotDimAngular();
        return;
    }
    if ( bLeader ->isChecked() ) {
        actionHandler->slotDimLeader();
        return;
    }
    //clear all action
    bHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
    if(currentAction != NULL) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}
void QG_CadToolBarDim::resetToolBar() {
    bHidden->setChecked(true);
}

void QG_CadToolBarDim::on_bBack_clicked()
{
    parentTB->showPreviousToolBar();
}
