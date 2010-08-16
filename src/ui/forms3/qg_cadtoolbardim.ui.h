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

void QG_CadToolBarDim::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarDim::mousePressEvent(QMouseEvent* e) {
// RVT_PORT    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
	if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarDim::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarDim::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarDim::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarDim::drawDimAligned() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimAligned();
    }
}

void QG_CadToolBarDim::drawDimLinear() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinear();
    }
}

void QG_CadToolBarDim::drawDimLinearHor() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinearHor();
    }
}

void QG_CadToolBarDim::drawDimLinearVer() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLinearVer();
    }
}

void QG_CadToolBarDim::drawDimRadial() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimRadial();
    }
}

void QG_CadToolBarDim::drawDimDiametric() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimDiametric();
    }
}

void QG_CadToolBarDim::drawDimAngular() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimAngular();
    }
}

void QG_CadToolBarDim::drawDimLeader() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDimLeader();
    }
}

void QG_CadToolBarDim::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
