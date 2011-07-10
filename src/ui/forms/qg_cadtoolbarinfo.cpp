/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#include "qg_cadtoolbarinfo.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarInfo as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarInfo::QG_CadToolBarInfo(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarInfo::~QG_CadToolBarInfo()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarInfo::languageChange()
{
    retranslateUi(this);
}

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
