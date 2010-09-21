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

void QG_CadToolBarModify::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarModify::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarModify::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarModify::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarModify::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarModify::modifyMove() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyMove();
    }
}

void QG_CadToolBarModify::modifyRotate() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyRotate();
    }
}

void QG_CadToolBarModify::modifyScale() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyScale();
    }
}

void QG_CadToolBarModify::modifyMirror() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyMirror();
    }
}

void QG_CadToolBarModify::modifyMoveRotate() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyMoveRotate();
    }
}

void QG_CadToolBarModify::modifyRotate2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyRotate2();
    }
}

void QG_CadToolBarModify::modifyTrim() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyTrim();
    }
}

void QG_CadToolBarModify::modifyTrim2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyTrim2();
    }
}

void QG_CadToolBarModify::modifyTrimAmount() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyTrimAmount();
    }
}

void QG_CadToolBarModify::modifyCut() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyCut();
    }
}

void QG_CadToolBarModify::modifyBevel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyBevel();
    }
}

void QG_CadToolBarModify::modifyRound() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyRound();
    }
}

void QG_CadToolBarModify::modifyEntity() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyEntity();
    }
}

void QG_CadToolBarModify::modifyDelete() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyDelete();
    }
}

void QG_CadToolBarModify::modifyAttributes() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyAttributes();
    }
}

void QG_CadToolBarModify::modifyStretch() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyStretch();
    }
}

void QG_CadToolBarModify::modifyExplode() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotBlocksExplode();
    }
}

void QG_CadToolBarModify::modifyExplodeText() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotModifyExplodeText();
    }
}

void QG_CadToolBarModify::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
