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

void QG_CadToolBarSplines::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarSplines::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarSplines::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarSplines::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarSplines::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarSplines::drawSpline() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawSpline();
    }
}

void QG_CadToolBarSplines::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
