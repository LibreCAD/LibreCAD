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

void QG_CadToolBarCircles::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

void QG_CadToolBarCircles::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->back();
        e->accept();
    }
}

void QG_CadToolBarCircles::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

void QG_CadToolBarCircles::setCadToolBar(QG_CadToolBar* tb) {
    cadToolBar = tb;
    if (tb!=NULL) {
        actionHandler = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarCircles::setCadToolBar(): No valid toolbar set.");
    }
}

void QG_CadToolBarCircles::drawCircle() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle();
    }
}

void QG_CadToolBarCircles::drawCircleCR() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleCR();
    }
}

void QG_CadToolBarCircles::drawCircle2P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle2P();
    }
}

void QG_CadToolBarCircles::drawCircle3P() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircle3P();
    }
}

void QG_CadToolBarCircles::drawCircleParallel() {
    if (cadToolBar!=NULL && actionHandler!=NULL) {
        actionHandler->slotDrawCircleParallel();
    }
}

void QG_CadToolBarCircles::back() {
    if (cadToolBar!=NULL) {
        cadToolBar->back();
    }
}
