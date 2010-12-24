/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
