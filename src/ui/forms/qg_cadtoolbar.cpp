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
#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBar as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBar::QG_CadToolBar(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(name);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBar::~QG_CadToolBar()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBar::languageChange()
{
    retranslateUi(this);
}


void QG_CadToolBar::init() {
    actionHandler = NULL;
    currentTb = NULL;
    previousID = RS2::ToolBarNone;
    savedID = RS2::ToolBarNone;

    tbMain = NULL;

    tbPoints = NULL;
    tbLines = NULL;
    tbArcs = NULL;
    tbCircles = NULL;
    tbEllipses = NULL;
    tbSplines = NULL;
    tbPolylines = NULL;

    tbDim = NULL;

    tbModify = NULL;
    tbInfo = NULL;
    tbSelect = NULL;

    tbSnap = NULL;
}

/**
 * @return Pointer to action handler or NULL.
 */
QG_ActionHandler* QG_CadToolBar::getActionHandler() {
    return actionHandler;
}

/**
 * Called from the sub toolbar
 */
void QG_CadToolBar::back() {
    emit(signalBack());
}

/**
 * Called from the application.
 */
void QG_CadToolBar::forceNext() {
    if (currentTb!=NULL && currentTb==tbSelect) {
        tbSelect->runNextAction();
    }
}

void QG_CadToolBar::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        back();
        e->accept();
    }
}

void QG_CadToolBar::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

/**
 * Creates all tool bars and shows the main toolbar.
 *
 * @param ah Pointer to action handler which will deal with the actions in this tool bar.
 */
void QG_CadToolBar::createSubToolBars(QG_ActionHandler* ah) {
    actionHandler = ah;
    tbMain = new QG_CadToolBarMain(this);
    tbMain->setCadToolBar(this);

    tbPoints = new QG_CadToolBarPoints(this);
    tbPoints->setCadToolBar(this);
    tbPoints->hide();

    tbLines = new QG_CadToolBarLines(this);
    tbLines->setCadToolBar(this);
    tbLines->hide();

    tbArcs = new QG_CadToolBarArcs(this);
    tbArcs->setCadToolBar(this);
    tbArcs->hide();

    tbCircles = new QG_CadToolBarCircles(this);
    tbCircles->setCadToolBar(this);
    tbCircles->hide();

    tbEllipses = new QG_CadToolBarEllipses(this);
    tbEllipses->setCadToolBar(this);
    tbEllipses->hide();

    tbSplines = new QG_CadToolBarSplines(this);
    tbSplines->setCadToolBar(this);
    tbSplines->hide();

    tbPolylines = new QG_CadToolBarPolylines(this);
    tbPolylines->setCadToolBar(this);
    tbPolylines->hide();

    tbDim = new QG_CadToolBarDim(this);
    tbDim->setCadToolBar(this);
    tbDim->hide();

    tbInfo = new QG_CadToolBarInfo(this);
    tbInfo->setCadToolBar(this);
    tbInfo->hide();

    tbModify = new QG_CadToolBarModify(this);
    tbModify->setCadToolBar(this);
    tbModify->hide();

    tbSnap = NULL;
    //tbSnap = new QG_CadToolBarSnap(this);
    //tbSnap->setCadToolBar(this);
    //tbSnap->hide();

    tbSelect = new QG_CadToolBarSelect(this);
    tbSelect->setCadToolBar(this);
    tbSelect->hide();

    showToolBarMain();
}


void QG_CadToolBar::showPreviousToolBar() {
    if(previousID != RS2::ToolBarNone) {
        showToolBar(previousID);
    }
    if(actionHandler != NULL) {
        RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
        if(currentAction != NULL) {
            currentAction->finish(false); //finish the action, but do not update toolBar
        }
    }
}

void QG_CadToolBar::showToolBar(RS2::ToolBarId id) {
    QWidget* newTb = NULL;

    switch (id) {
    default:
    case RS2::ToolBarMain:
        tbMain->restoreAction();
        newTb = tbMain;
        break;
        /* not needed any more
    case RS2::ToolBarPoints:
        newTb = tbPoints;
        break;
        */
    case RS2::ToolBarLines:
        tbLines->restoreAction();
        newTb = tbLines;
        break;
    case RS2::ToolBarArcs:
        tbArcs->restoreAction();
        newTb = tbArcs;
        break;
    case RS2::ToolBarEllipses:
        tbEllipses->restoreAction();
        newTb = tbEllipses;
        break;
        /*
    case RS2::ToolBarSplines:
        newTb = tbSplines;
        break;
        */
    case RS2::ToolBarPolylines:
        tbPolylines->restoreAction();
        newTb = tbPolylines;
        break;
    case RS2::ToolBarCircles:
        tbCircles->restoreAction();
        newTb = tbCircles;
        break;
    case RS2::ToolBarInfo:
        tbInfo->restoreAction();
        newTb = tbInfo;
        break;
    case RS2::ToolBarModify:
        tbModify->restoreAction();
        newTb = tbModify;
        break;
    case RS2::ToolBarDim:
        tbDim->restoreAction();
        newTb = tbDim;
        break;
        /* not needed any more
    case RS2::ToolBarSnap:
        newTb = tbSnap;
        break;
        */
    case RS2::ToolBarSelect:
        newTb = tbSelect;
        break;
    }

    if (currentTb==newTb) {
        return;
    }
    if (currentTb!=NULL) {
        currentTb->hide();
    }
    previousID=savedID;
    savedID=id;
    currentTb = newTb;
    if (currentTb!=NULL) {
        //currentTb->move(0,20);
        currentTb->show();
    }
}

void QG_CadToolBar::resetToolBar() {
    if(currentTb == tbMain) {
        tbMain->resetToolBar();
        return;
    }
    if(currentTb == tbLines) {
        tbLines->resetToolBar();
        return;
    }
    if(currentTb == tbArcs) {
        tbArcs->resetToolBar();
        return;
    }
    if(currentTb == tbCircles) {
        tbCircles->resetToolBar();
        return;
    }
    if(currentTb == tbEllipses) {
        tbEllipses->resetToolBar();
        return;
    }
    if(currentTb == tbPolylines) {
        tbPolylines->resetToolBar();
        return;
    }
    if(currentTb == tbDim) {
        tbDim->resetToolBar();
        return;
    }
    if(currentTb == tbInfo) {
        tbInfo->resetToolBar();
        return;
    }
}

void QG_CadToolBar::showToolBarMain() {
    showToolBar(RS2::ToolBarMain);
}

void QG_CadToolBar::showToolBarPoints() {
//not needed
        return;
    //showToolBar(RS2::ToolBarPoints);
}

void QG_CadToolBar::showToolBarLines() {
    showToolBar(RS2::ToolBarLines);
}

void QG_CadToolBar::showToolBarArcs() {
    showToolBar(RS2::ToolBarArcs);
}

void QG_CadToolBar::showToolBarEllipses() {
    showToolBar(RS2::ToolBarEllipses);
}

void QG_CadToolBar::showToolBarSplines() {
    showToolBar(RS2::ToolBarSplines);
}

void QG_CadToolBar::showToolBarPolylines() {
    showToolBar(RS2::ToolBarPolylines);
}

void QG_CadToolBar::showToolBarCircles() {
    showToolBar(RS2::ToolBarCircles);
}

void QG_CadToolBar::showToolBarInfo() {
    showToolBar(RS2::ToolBarInfo);
}

void QG_CadToolBar::showToolBarModify() {
    showToolBar(RS2::ToolBarModify);
}

void QG_CadToolBar::showToolBarSnap() {
    showToolBar(RS2::ToolBarSnap);
}

void QG_CadToolBar::showToolBarDim() {
    showToolBar(RS2::ToolBarDim);
}

void QG_CadToolBar::showToolBarSelect() {
    showToolBarSelect(NULL, -1);
}

void QG_CadToolBar::showToolBarSelect(RS_ActionInterface* selectAction,
                                      int nextAction) {

    tbSelect->setNextAction(nextAction);
    tbSelect->setSelectAction(selectAction);
    showToolBar(RS2::ToolBarSelect);
}

void QG_CadToolBar::setSnapFree() {
    //not needed any more, will be removed
//    return;
//        if (tbSnap != NULL ) {
//                tbSnap->snapFree();
//        }
}
