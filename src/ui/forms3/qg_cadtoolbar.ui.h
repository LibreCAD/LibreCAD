/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

void QG_CadToolBar::init() {
    actionHandler = NULL;
    currentTb = NULL;

    tbMain = NULL;

    tbPoints = NULL;
    tbLines = NULL;
    tbArcs = NULL;
    tbCircles = NULL;
    tbEllipses = NULL;
    tbSplines = NULL;
#ifdef RS_PROF
    tbPolylines = NULL;
#endif

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
    // RVT_PORT if (e->button()==Qt::RightButton) {
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

#ifdef RS_PROF
    tbPolylines = new QG_CadToolBarPolylines(this);
    tbPolylines->setCadToolBar(this);
    tbPolylines->hide();
#endif

    tbDim = new QG_CadToolBarDim(this);
    tbDim->setCadToolBar(this);
    tbDim->hide();

    tbInfo = new QG_CadToolBarInfo(this);
    tbInfo->setCadToolBar(this);
    tbInfo->hide();

    tbModify = new QG_CadToolBarModify(this);
    tbModify->setCadToolBar(this);
    tbModify->hide();

    tbSnap = new QG_CadToolBarSnap(this);
    tbSnap->setCadToolBar(this);
    tbSnap->hide();

    tbSelect = new QG_CadToolBarSelect(this);
    tbSelect->setCadToolBar(this);
    tbSelect->hide();

    //showToolBarMain();
}


void QG_CadToolBar::showToolBar(int id) {
    QWidget* newTb = NULL;

    switch (id) {
    default:
    case RS2::ToolBarMain:
        newTb = tbMain;
        break;
    case RS2::ToolBarPoints:
        newTb = tbPoints;
        break;
    case RS2::ToolBarLines:
        newTb = tbLines;
        break;
    case RS2::ToolBarArcs:
        newTb = tbArcs;
        break;
    case RS2::ToolBarEllipses:
        newTb = tbEllipses;
        break;
    case RS2::ToolBarSplines:
        newTb = tbSplines;
        break;
#ifdef RS_PROF
    case RS2::ToolBarPolylines:
        newTb = tbPolylines;
        break;
#endif
    case RS2::ToolBarCircles:
        newTb = tbCircles;
        break;
    case RS2::ToolBarInfo:
        newTb = tbInfo;
        break;
    case RS2::ToolBarModify:
        newTb = tbModify;
        break;
    case RS2::ToolBarDim:
        newTb = tbDim;
        break;
    case RS2::ToolBarSnap:
        newTb = tbSnap;
        break;
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
    currentTb = newTb;
    if (currentTb!=NULL) {
        currentTb->show();
    }
}

void QG_CadToolBar::showToolBarMain() {
    showToolBar(RS2::ToolBarMain);
}

void QG_CadToolBar::showToolBarPoints() {
    showToolBar(RS2::ToolBarPoints);
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
#ifdef RS_PROF
    showToolBar(RS2::ToolBarPolylines);
#endif
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
