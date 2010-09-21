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

void QG_CadToolBarLines::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

/*void QG_CadToolBarLines::mousePressEvent(QMouseEvent* e) {
    if (e->button()==RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}*/



void QG_CadToolBarLines::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarLines::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarLines::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarLines::drawLine() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLine();
    }
}

void QG_CadToolBarLines::drawLineAngle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineAngle();
    }
}

void QG_CadToolBarLines::drawLineHorizontal() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineHorizontal();
    }
}

void QG_CadToolBarLines::drawLineHorVert() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineHorVert();
    }
}

void QG_CadToolBarLines::drawLineVertical() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineVertical();
    }
}

void QG_CadToolBarLines::drawLineParallel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineParallel();
    }
}

void QG_CadToolBarLines::drawLineParallelThrough() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineParallelThrough();
    }
}

void QG_CadToolBarLines::drawLineRectangle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineRectangle();
    }
}

void QG_CadToolBarLines::drawLineBisector() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineBisector();
    }
}

void QG_CadToolBarLines::drawLineTangent1() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineTangent1();
    }
}

void QG_CadToolBarLines::drawLineTangent2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineTangent2();
    }
}

void QG_CadToolBarLines::drawLineOrthogonal() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineOrthogonal();
    }
}

void QG_CadToolBarLines::drawLineRelAngle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineRelAngle();
    }
}

void QG_CadToolBarLines::drawLineFree() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLineFree();
    }
}

void QG_CadToolBarLines::drawLinePolygon() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLinePolygon();
    }
}

void QG_CadToolBarLines::drawLinePolygon2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawLinePolygon2();
    }
}

void QG_CadToolBarLines::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
