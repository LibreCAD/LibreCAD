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

void QG_CadToolBarMain::init() {
}

void QG_CadToolBarMain::setCadToolBar(QG_CadToolBar* tb) {
    QG_ActionHandler* ah = NULL;
    if (tb!=NULL) {
        ah = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarMain::setCadToolBar(): No valid toolbar set.");
    }
    if (ah!=NULL) {
        connect(bMenuPoint, SIGNAL(clicked()),
                ah, SLOT(slotDrawPoint()));
        connect(bMenuLine, SIGNAL(clicked()),
                tb, SLOT(showToolBarLines()));
        connect(bMenuArc, SIGNAL(clicked()),
                tb, SLOT(showToolBarArcs()));
        connect(bMenuCircle, SIGNAL(clicked()),
                tb, SLOT(showToolBarCircles()));
        connect(bMenuEllipse, SIGNAL(clicked()),
                tb, SLOT(showToolBarEllipses()));
        connect(bMenuSpline, SIGNAL(clicked()),
                ah, SLOT(slotDrawSpline()));
        connect(bMenuPolyline, SIGNAL(clicked()),
                tb, SLOT(showToolBarPolylines()));
        
        connect(bMenuText, SIGNAL(clicked()),
                ah, SLOT(slotDrawText()));
        connect(bMenuDim, SIGNAL(clicked()),
                tb, SLOT(showToolBarDim()));
        connect(bMenuHatch, SIGNAL(clicked()),
                ah, SLOT(slotDrawHatch()));
        connect(bMenuImage, SIGNAL(clicked()),
                ah, SLOT(slotDrawImage()));
        
        connect(bMenuModify, SIGNAL(clicked()),
                tb, SLOT(showToolBarModify()));
        connect(bMenuInfo, SIGNAL(clicked()),
                tb, SLOT(showToolBarInfo()));

        connect(bMenuBlock, SIGNAL(clicked()),
                ah, SLOT(slotBlocksCreate()));
        connect(bMenuSelect, SIGNAL(clicked()),
                tb, SLOT(showToolBarSelect()));
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarMain::setCadToolBar(): No valid action handler set.");
    }
}
