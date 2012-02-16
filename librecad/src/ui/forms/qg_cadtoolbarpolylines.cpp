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
#include "qg_cadtoolbarpolylines.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarPolylines as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarPolylines::QG_CadToolBarPolylines(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
    parentTB=static_cast<QG_CadToolBar*>(parent);
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarPolylines::~QG_CadToolBarPolylines()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarPolylines::languageChange()
{
    retranslateUi(this);
}

void QG_CadToolBarPolylines::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarPolylines::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarPolylines::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarPolylines::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarPolylines::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarPolylines::drawPolyline() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawPolyline();
    }
}

void QG_CadToolBarPolylines::polylineAdd() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineAdd();
    }
}

void QG_CadToolBarPolylines::polylineAppend() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineAppend();
    }
}

void QG_CadToolBarPolylines::polylineDel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineDel();
    }
}

void QG_CadToolBarPolylines::polylineDelBetween() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineDelBetween();
    }
}

void QG_CadToolBarPolylines::polylineTrim() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineTrim();
    }
}

void QG_CadToolBarPolylines::polylineEquidistant() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineEquidistant();
    }
}

void QG_CadToolBarPolylines::polylineSegment() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineSegment();
    }
}

void QG_CadToolBarPolylines::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
//restore action from checked button
void QG_CadToolBarPolylines::restoreAction()
{
    if(actionHandler==NULL) return;
    if ( bPolyline ->isChecked() ) {
        actionHandler->slotDrawPolyline();
        return;
    }
    if ( bPolylineAdd ->isChecked() ) {
        actionHandler->slotPolylineAdd();
        return;
    }
    if ( bPolylineAppend ->isChecked() ) {
        actionHandler->slotPolylineAppend();
        return;
    }
    if ( bPolylineDel ->isChecked() ) {
        actionHandler->slotPolylineDel();
        return;
    }
    if ( bPolylineDelBetween ->isChecked() ) {
        actionHandler->slotPolylineDelBetween();
        return;
    }
    if ( bPolylineTrim ->isChecked() ) {
        actionHandler->slotPolylineTrim();
        return;
    }
    if ( bPolylineEquidistant ->isChecked() ) {
        actionHandler->slotPolylineEquidistant();
        return;
    }
    if ( bPolylineSegment ->isChecked() ) {
        actionHandler->slotPolylineSegment();
        return;
    }
    bHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
    if(currentAction != NULL) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarPolylines::resetToolBar() {
    bHidden->setChecked(true);
}

void QG_CadToolBarPolylines::on_bBack_clicked()
{
    parentTB->showPreviousToolBar();
}

void QG_CadToolBarPolylines::showCadToolBar(RS2::ActionType actionType){
    switch(actionType){
    case RS2::ActionDrawPolyline:
        bPolyline->setChecked(true);
        return;
    case RS2::ActionPolylineAdd:
        bPolylineAdd->setChecked(true);
        return;
    case RS2::ActionPolylineAppend:
        bPolylineAppend->setChecked(true);
        return;
    case RS2::ActionPolylineDel:
        bPolylineDel->setChecked(true);
        return;
    case RS2::ActionPolylineDelBetween:
        bPolylineDelBetween->setChecked(true);
        return;
    case RS2::ActionPolylineTrim:
        bPolylineTrim->setChecked(true);
        return;
    case RS2::ActionPolylineEquidistant:
        bPolylineEquidistant->setChecked(true);
        return;
    case RS2::ActionPolylineSegment:
        bPolylineSegment->setChecked(true);
        return;
        default:
        bHidden->setChecked(true);
        return;
    }
}

//EOF
