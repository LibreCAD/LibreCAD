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
#include "qg_cadtoolbarcircles.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarCircles as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarCircles::QG_CadToolBarCircles(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarCircles::~QG_CadToolBarCircles()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarCircles::languageChange()
{
    retranslateUi(this);
}

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
