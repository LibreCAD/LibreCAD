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

void QG_CadToolBarInfo::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarInfo::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarInfo::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarInfo::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarInfo::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarInfo::infoDist() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoDist();
    }
}

void QG_CadToolBarInfo::infoDist2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoDist2();
    }
}

void QG_CadToolBarInfo::infoAngle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoAngle();
    }
}

void QG_CadToolBarInfo::infoTotalLength() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoTotalLength();
    }
}

void QG_CadToolBarInfo::infoArea() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoArea();
    }
}

void QG_CadToolBarInfo::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
