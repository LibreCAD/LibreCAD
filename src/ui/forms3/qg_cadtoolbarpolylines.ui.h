/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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
