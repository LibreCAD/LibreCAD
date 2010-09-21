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

void QG_CadToolBarSnap::init() {
    cadToolBar = NULL;
    actionHandler = NULL;
}

//void QG_CadToolBarSnap::mousePressEvent(QMouseEvent* e) {
//    if (e->button()==RightButton && cadToolBar!=NULL) {
//cadToolBar->back();
//        e->accept();
//    }
//}

void QG_CadToolBarSnap::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarSnap::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
        actionHandler->setCadToolBarSnap(this);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarSnap::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarSnap::snapFree() {
    if (actionHandler!=NULL) {
        disableSnaps();
        bFree->setOn(true);
        actionHandler->slotSnapFree();
    }
}

void QG_CadToolBarSnap::snapGrid() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapGrid();
        disableSnaps();
        bGrid->setOn(true);
    }
}

void QG_CadToolBarSnap::snapEndpoint() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapEndpoint();
        disableSnaps();
        bEndpoint->setOn(true);
    }
}

void QG_CadToolBarSnap::snapOnEntity() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapOnEntity();
        disableSnaps();
        bOnEntity->setOn(true);
    }
}

void QG_CadToolBarSnap::snapCenter() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapCenter();
        disableSnaps();
        bCenter->setOn(true);
    }
}

void QG_CadToolBarSnap::snapMiddle() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapMiddle();
        disableSnaps();
        bMiddle->setOn(true);
    }
}

void QG_CadToolBarSnap::snapDist() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapDist();
        disableSnaps();
        bDist->setOn(true);
    }
}

void QG_CadToolBarSnap::snapIntersection() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapIntersection();
        disableSnaps();
        bIntersection->setOn(true);
    }
}

void QG_CadToolBarSnap::snapIntersectionManual() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapIntersectionManual();
        //disableSnaps();
        //bIntersectionManual->setOn(true);
    }
}

void QG_CadToolBarSnap::restrictNothing() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictNothing();
        disableRestrictions();
        bResNothing->setOn(true);
    }
}

void QG_CadToolBarSnap::restrictOrthogonal() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictOrthogonal();
        disableRestrictions();
        bResOrthogonal->setOn(true);
    }
}

void QG_CadToolBarSnap::restrictHorizontal() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictHorizontal();
        disableRestrictions();
        bResHorizontal->setOn(true);
    }
}

void QG_CadToolBarSnap::restrictVertical() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictVertical();
        disableRestrictions();
        bResVertical->setOn(true);
    }
}

void QG_CadToolBarSnap::disableSnaps() {
    if (bFree->isOn()) {
        bFree->setOn(false);
    }
    if (bGrid->isOn()) {
        bGrid->setOn(false);
    }
    if (bEndpoint->isOn()) {
        bEndpoint->setOn(false);
    }
    if (bOnEntity->isOn()) {
        bOnEntity->setOn(false);
    }
    if (bCenter->isOn()) {
        bCenter->setOn(false);
    }
    if (bMiddle->isOn()) {
        bMiddle->setOn(false);
    }
    if (bDist->isOn()) {
        bDist->setOn(false);
    }
    if (bIntersection->isOn()) {
        bIntersection->setOn(false);
    }
    if (bIntersectionManual->isOn()) {
        bIntersectionManual->setOn(false);
    }
}

void QG_CadToolBarSnap::disableRestrictions() {
    if (bResNothing->isOn()) {
        bResNothing->setOn(false);
    }
    if (bResOrthogonal->isOn()) {
        bResOrthogonal->setOn(false);
    }
    if (bResHorizontal->isOn()) {
        bResHorizontal->setOn(false);
    }
    if (bResVertical->isOn()) {
        bResVertical->setOn(false);
    }
}

void QG_CadToolBarSnap::setSnapMode(int sm) {
    switch (sm) {
    case RS2::SnapFree:
        bFree->setOn(true);
        break;
    case RS2::SnapEndpoint:
        bEndpoint->setOn(true);
        break;
    case RS2::SnapGrid:
        bGrid->setOn(true);
        break;
    case RS2::SnapOnEntity:
        bOnEntity->setOn(true);
        break;
    case RS2::SnapCenter:
        bCenter->setOn(true);
        break;
    case RS2::SnapMiddle:
        bMiddle->setOn(true);
        break;
    case RS2::SnapDist:
        bDist->setOn(true);
        break;
    case RS2::SnapIntersection:
        bIntersection->setOn(true);
        break;
    default:
        break;
    }
}

void QG_CadToolBarSnap::setSnapRestriction(int sr) {
    switch (sr) {
    default:
    case RS2::RestrictNothing:
        bResNothing->setOn(true);
        break;
    case RS2::RestrictOrthogonal:
        bResOrthogonal->setOn(true);
        break;
    case RS2::RestrictHorizontal:
        bResHorizontal->setOn(true);
        break;
    case RS2::RestrictVertical:
        bResVertical->setOn(true);
        break;
    }
}

void QG_CadToolBarSnap::setRelativeZero() {
     if (cadToolBar!=NULL && actionHandler!=NULL) {
         actionHandler->slotSetRelativeZero();
     }
}

void QG_CadToolBarSnap::lockRelativeZero(bool on) {
     if (cadToolBar!=NULL && actionHandler!=NULL) {
         actionHandler->slotLockRelativeZero(on);
     }
}

void QG_CadToolBarSnap::setLockRelativeZero(bool on) {
    bLockRelZero->setOn(on);
}

void QG_CadToolBarSnap::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
