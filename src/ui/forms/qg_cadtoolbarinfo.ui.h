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

void QG_CadToolBarInfo::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarInfo::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarInfo::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarInfo::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarInfo::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarInfo::infoDist() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoDist();
    }
}

void QG_CadToolBarInfo::infoDist2() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoDist2();
    }
}

void QG_CadToolBarInfo::infoAngle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoAngle();
    }
}

void QG_CadToolBarInfo::infoTotalLength() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoTotalLength();
    }
}

void QG_CadToolBarInfo::infoArea() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotInfoArea();
    }
}

void QG_CadToolBarInfo::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
