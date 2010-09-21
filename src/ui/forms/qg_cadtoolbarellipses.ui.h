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

void QG_CadToolBarEllipses::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarEllipses::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarEllipses::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarEllipses::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarEllipses::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarEllipses::drawEllipseAxis() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawEllipseAxis();
    }
}

void QG_CadToolBarEllipses::drawEllipseArcAxis() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawEllipseArcAxis();
    }
}

void QG_CadToolBarEllipses::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
