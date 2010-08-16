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

void QG_CadToolBarLines::init() {
    actionHandler = NULL;
    cadToolBar = NULL;
}

/*void QG_CadToolBarLines::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
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
