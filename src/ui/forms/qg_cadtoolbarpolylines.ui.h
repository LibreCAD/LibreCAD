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

void QG_CadToolBarPolylines::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarPolylines::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarPolylines::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarPolylines::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarPolylines::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarPolylines::drawPolyline() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawPolyline();
    }
}

void QG_CadToolBarPolylines::polylineAdd() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineAdd();
    }
}

void QG_CadToolBarPolylines::polylineAppend() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineAppend();
    }
}

void QG_CadToolBarPolylines::polylineDel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineDel();
    }
}

void QG_CadToolBarPolylines::polylineDelBetween() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineDelBetween();
    }
}

void QG_CadToolBarPolylines::polylineTrim() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotPolylineTrim();
    }
}

void QG_CadToolBarPolylines::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
