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

#ifndef NO_COMPLEX_ENTITIES

#include "rs_actionparisdebugcreatecontainer.h"

#include "rs_document.h"

/**
 * Constructor.
 */
RS_ActionPARISDebugCreateContainer::RS_ActionPARISDebugCreateContainer(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        : RS_ActionInterface("rs_actionparischeckcont",
                     container, graphicView) {

    //QMessageBox::about(NULL, "info", "check container");
    RS_Document* theDoc = (RS_Document*) &container;

    if (theDoc->countSelected() < 2) {
        return;
	}

    RS_EntityContainer* con = new RS_EntityContainer(theDoc, true);
    QListIterator<RS_Entity*> it = theDoc->createIterator();
    RS_Entity* e;

    while (it.hasNext()) {
        e = it.next();
        if (e->isSelected()) {
            con->addEntity(e);
            e->setParent(con);
        }
    }

    theDoc -> addEntity(con);
}



RS_ActionPARISDebugCreateContainer::~RS_ActionPARISDebugCreateContainer() {
}

#endif


// EOF
