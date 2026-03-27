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


#include "rs_actionblockscreate.h"

#include "rs_block.h"
#include "rs_creation.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_insert.h"

class RS_BlockList;
/**
 * Constructor.
 */
RS_ActionBlocksCreate::RS_ActionBlocksCreate(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("Blocks Create",actionContext, RS2::ActionBlocksCreate)
    ,m_referencePoint(new RS_Vector{}){
}

RS_ActionBlocksCreate::~RS_ActionBlocksCreate() = default;

void RS_ActionBlocksCreate::onSelectionCompleted([[maybe_unused]]bool singleEntity, const bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    if (m_selectionComplete) {
        updateActionPrompt();
    }
}

bool RS_ActionBlocksCreate::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_graphic == nullptr) {
        return false;
    }
    RS_BlockList* blockList = m_graphic->getBlockList();
    if (blockList != nullptr) {
        const RS_BlockData d = RS_DIALOGFACTORY->requestNewBlockDialog(blockList);

        if (!d.name.isEmpty()) {
            RS_Block* block = RS_Creation::createBlock(&d, *m_referencePoint, m_selectedEntities);
            const RS_InsertData insertData(d.name, *m_referencePoint, RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0));
            const auto insert = new RS_Insert(m_document, insertData);
            m_graphic->addBlock(block);

            ctx += insert;
            ctx -= m_selectedEntities;

            insert->update();
        }
    }
    return true;
}

void RS_ActionBlocksCreate::doTriggerCompletion([[maybe_unused]]bool success) {
    setStatus(getStatus()+1); // clear mouse button hints
    updateActionPrompt();
    finish();
}

void RS_ActionBlocksCreate::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionBlocksCreate::onMouseRightButtonReleaseSelected([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* event) {
    init(getStatus()-1);
}

void RS_ActionBlocksCreate::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint: {
            *m_referencePoint = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionBlocksCreate::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to create block") + getSelectionCompletionHintMsg(), MOD_SHIFT_LC(tr("Select contour")));
}

void RS_ActionBlocksCreate::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            updatePromptTRCancel(tr("Specify reference point"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionBlocksCreate::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
