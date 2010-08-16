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

#ifndef NO_COMPLEX_ENTITIES

#include "rs_actionparisdebugcreatecontainer.h"

#include "rs_ptrlist.h"

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
    RS_PtrListIterator<RS_Entity> it = theDoc->createIterator();
    RS_Entity* e;

    while ( (e = it.current()) != 0) {
        ++it;
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
