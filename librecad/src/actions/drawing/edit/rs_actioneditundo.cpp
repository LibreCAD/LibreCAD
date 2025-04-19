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

#include "rs_actioneditundo.h"

#include "rs_document.h"
#include "rs_graphic.h"

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditUndo::RS_ActionEditUndo(bool undo,LC_ActionContext *actionContext)
  :RS_ActionInterface("Edit Undo", actionContext,undo? RS2::ActionEditUndo: RS2::ActionEditRedo)
 , m_performUndo(undo){
}

void RS_ActionEditUndo::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionEditUndo::trigger(){
    if (!m_graphic){
        qWarning("undo: graphic is null");
        return;
    }

    if (m_performUndo) {
        if(!m_document->undo())
            commandMessage(tr("Nothing to undo!"));
    } else {
        if(!m_document->redo())
            commandMessage(tr("Nothing to redo!"));
    }

    m_graphic->addBlockNotification();
    m_graphic->setModified(true);
    m_document->updateInserts();
    redrawDrawing();
    finish(false);
    updateSelectionWidget();
}
