//Added by qt3to4:
#include <QContextMenuEvent>
#include <QMouseEvent>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

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

void QG_CadToolBarCircles::drawCircleParallel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleParallel();
    }
}

void QG_CadToolBarCircles::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
