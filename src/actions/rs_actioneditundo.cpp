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

#include "rs_actioneditundo.h"
#include "rs_snapper.h"
//Added by qt3to4:
#include <q3mimefactory.h>

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
        //icon = QPixmap(editundo_xpm);
/* RVT_PORT        action = new QAction(tr("Undo"),
                             qPixmapFromMimeSource("undo2.png"), tr("&Undo"),
                             Qt::CTRL+Qt::Key_Z, parent); */
        action = new QAction(qPixmapFromMimeSource("undo2.png"), tr("Undo"), parent);
        action->setStatusTip(tr("Undoes last action"));
    } else {
        //icon = QPixmap(editredo_xpm);
 /* RVT_PORT       action = new QAction(tr("Redo"),
                             qPixmapFromMimeSource("redo2.png"), tr("&Redo"),
                             Qt::CTRL+Qt::SHIFT+Qt::Key_Z, parent); */
        action = new QAction(qPixmapFromMimeSource("redo2.png"), tr("Redo"), parent);
        action->setStatusTip(tr("Redoes last action"));
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
