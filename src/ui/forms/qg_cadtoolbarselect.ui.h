//Added by qt3to4:
#include <QMouseEvent>
#include <QContextMenuEvent>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_CadToolBarSelect::init() {
    cadToolBar = NULL;
    actionHandler = NULL;
    selectAction = NULL;
    nextAction = -1;
}

void QG_CadToolBarSelect::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarSelect::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarSelect::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
        //actionHandler->setCadToolBarSelect(this);
    }
    else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarSelect::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarSelect::selectSingle() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectSingle();
    }
}

void QG_CadToolBarSelect::selectContour() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectContour();
    }
}

void QG_CadToolBarSelect::deselectAll() {
    if (actionHandler!=NULL) {
        actionHandler->slotDeselectAll();
    }
}

void QG_CadToolBarSelect::selectAll() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectAll();
    }
}

void QG_CadToolBarSelect::selectWindow() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectWindow();
    }
}

void QG_CadToolBarSelect::deselectWindow() {
    if (actionHandler!=NULL) {
        actionHandler->slotDeselectWindow();
    }
}

void QG_CadToolBarSelect::selectIntersected() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectIntersected();
    }
}

void QG_CadToolBarSelect::deselectIntersected() {
    if (actionHandler!=NULL) {
        actionHandler->slotDeselectIntersected();
    }
}

void QG_CadToolBarSelect::selectInvert() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectInvert();
    }
}

void QG_CadToolBarSelect::selectLayer() {
    if (actionHandler!=NULL) {
        actionHandler->slotSelectLayer();
    }
}

void QG_CadToolBarSelect::setSelectAction(RS_ActionInterface* selectAction) {
    this->selectAction = selectAction;
}

void QG_CadToolBarSelect::setNextAction(int nextAction) {
    this->nextAction = nextAction;
    if (nextAction==-1) {
        bDoit->hide();
    } else {
        bDoit->show();
    }
}

void QG_CadToolBarSelect::runNextAction() {
    if (selectAction!=NULL) {
        selectAction->finish();
        selectAction = NULL;
    }
    if (nextAction!=-1) {
        actionHandler->killSelectActions();
    	actionHandler->setCurrentAction((RS2::ActionType)nextAction);
    }
}

void QG_CadToolBarSelect::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
