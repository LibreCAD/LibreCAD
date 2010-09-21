//Added by qt3to4:
#include <QContextMenuEvent>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

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

void QG_CadToolBarArcs::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
