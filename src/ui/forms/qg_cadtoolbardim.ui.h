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

void QG_CadToolBarDim::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
