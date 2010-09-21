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

void QG_CadToolBarPoints::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarPoints::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarPoints::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarPoints::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarPoints::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarPoints::drawPoint() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawPoint();
    }
}

void QG_CadToolBarPoints::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
