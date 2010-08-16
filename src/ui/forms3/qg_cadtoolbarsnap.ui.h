/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

void QG_CadToolBarSnap::init() {
    cadToolBar = NULL;
    actionHandler = NULL;
}

//void QG_CadToolBarSnap::mousePressEvent(QMouseEvent* e) {
//    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
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
        bFree->setChecked(true);
        actionHandler->slotSnapFree();
    }
}

void QG_CadToolBarSnap::snapGrid() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapGrid();
        disableSnaps();
        bGrid->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapEndpoint() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapEndpoint();
        disableSnaps();
        bEndpoint->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapOnEntity() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapOnEntity();
        disableSnaps();
        bOnEntity->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapCenter() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapCenter();
        disableSnaps();
        bCenter->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapMiddle() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapMiddle();
        disableSnaps();
        bMiddle->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapDist() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapDist();
        disableSnaps();
        bDist->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapIntersection() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapIntersection();
        disableSnaps();
        bIntersection->setChecked(true);
    }
}

void QG_CadToolBarSnap::snapIntersectionManual() {
    if (actionHandler!=NULL) {
        actionHandler->slotSnapIntersectionManual();
        //disableSnaps();
        //bIntersectionManual->setChecked(true);
    }
}

void QG_CadToolBarSnap::restrictNothing() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictNothing();
        disableRestrictions();
        bResNothing->setChecked(true);
    }
}

void QG_CadToolBarSnap::restrictOrthogonal() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictOrthogonal();
        disableRestrictions();
        bResOrthogonal->setChecked(true);
    }
}

void QG_CadToolBarSnap::restrictHorizontal() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictHorizontal();
        disableRestrictions();
        bResHorizontal->setChecked(true);
    }
}

void QG_CadToolBarSnap::restrictVertical() {
    if (actionHandler!=NULL) {
        actionHandler->slotRestrictVertical();
        disableRestrictions();
        bResVertical->setChecked(true);
    }
}

void QG_CadToolBarSnap::disableSnaps() {
    if (bFree->isChecked()) {
        bFree->setChecked(false);
    }
    if (bGrid->isChecked()) {
        bGrid->setChecked(false);
    }
    if (bEndpoint->isChecked()) {
        bEndpoint->setChecked(false);
    }
    if (bOnEntity->isChecked()) {
        bOnEntity->setChecked(false);
    }
    if (bCenter->isChecked()) {
        bCenter->setChecked(false);
    }
    if (bMiddle->isChecked()) {
        bMiddle->setChecked(false);
    }
    if (bDist->isChecked()) {
        bDist->setChecked(false);
    }
    if (bIntersection->isChecked()) {
        bIntersection->setChecked(false);
    }
    if (bIntersectionManual->isChecked()) {
        bIntersectionManual->setChecked(false);
    }
}

void QG_CadToolBarSnap::disableRestrictions() {
    if (bResNothing->isChecked()) {
        bResNothing->setChecked(false);
    }
    if (bResOrthogonal->isChecked()) {
        bResOrthogonal->setChecked(false);
    }
    if (bResHorizontal->isChecked()) {
        bResHorizontal->setChecked(false);
    }
    if (bResVertical->isChecked()) {
        bResVertical->setChecked(false);
    }
}

void QG_CadToolBarSnap::setSnapMode(int sm) {
    switch (sm) {
    case RS2::SnapFree:
        bFree->setChecked(true);
        break;
    case RS2::SnapEndpoint:
        bEndpoint->setChecked(true);
        break;
    case RS2::SnapGrid:
        bGrid->setChecked(true);
        break;
    case RS2::SnapOnEntity:
        bOnEntity->setChecked(true);
        break;
    case RS2::SnapCenter:
        bCenter->setChecked(true);
        break;
    case RS2::SnapMiddle:
        bMiddle->setChecked(true);
        break;
    case RS2::SnapDist:
        bDist->setChecked(true);
        break;
    case RS2::SnapIntersection:
        bIntersection->setChecked(true);
        break;
    default:
        break;
    }
}

void QG_CadToolBarSnap::setSnapRestriction(int sr) {
    switch (sr) {
    default:
    case RS2::RestrictNothing:
        bResNothing->setChecked(true);
        break;
    case RS2::RestrictOrthogonal:
        bResOrthogonal->setChecked(true);
        break;
    case RS2::RestrictHorizontal:
        bResHorizontal->setChecked(true);
        break;
    case RS2::RestrictVertical:
        bResVertical->setChecked(true);
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
    bLockRelZero->setChecked(on);
}

void QG_CadToolBarSnap::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
