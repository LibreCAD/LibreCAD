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

#include "rs_actioneditundo.h"
#include "rs_snapper.h"

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditUndo::RS_ActionEditUndo(bool undo,
                                     RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Undo",
                    container, graphicView) {

    this->undo = undo;
}



RS_ActionEditUndo::~RS_ActionEditUndo() {}


QAction* RS_ActionEditUndo::createGUIAction(RS2::ActionType type, QObject* parent) {
    QAction* action;
    if (type==RS2::ActionEditUndo) {		
		// tr("Undo")
		action = new QAction(tr("&Undo"), parent);
#if QT_VERSION >= 0x040600
                action->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/actions/undo2.png")));
#else
                action->setIcon(QIcon(":/actions/undo2.png"));
#endif
		action->setShortcut(QKeySequence::Undo);
		//action->zetStatusTip(tr("Undoes last action"));
    } else {
		// tr("Redo")
		action = new QAction(tr("&Redo"), parent);
#if QT_VERSION >= 0x040600
                action->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/actions/redo2.png")));
#else
                action->setIcon(QIcon(":/actions/redo2.png"));
#endif
		action->setShortcut(QKeySequence::Redo);
		//action->zetStatusTip(tr("Redoes last action"));
    }

    return action;
}

void RS_ActionEditUndo::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}



void RS_ActionEditUndo::trigger() {
    if (undo) {
        document->undo();
    } else {
        document->redo();
    }

    document->updateInserts();

	graphicView->redraw(RS2::RedrawDrawing); 


    finish();
    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}


// EOF
