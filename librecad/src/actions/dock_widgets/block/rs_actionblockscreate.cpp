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

void RS_ActionBlocksCreate::onSelectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    if (m_selectionComplete) {
        updateMouseButtonHints();
        updateSelectionWidget();
    }
}

void RS_ActionBlocksCreate::doTrigger([[maybe_unused]]bool keepSelected) {
    if (m_graphic != nullptr) {
        RS_BlockList* blockList = m_graphic->getBlockList();
        if (blockList != nullptr) {
            RS_BlockData d =
                RS_DIALOGFACTORY->requestNewBlockDialog(blockList);

            if (!d.name.isEmpty()) {
                RS_Creation creation(m_container, getViewPort());
                creation.createBlock(&d, *m_referencePoint, true);
                RS_InsertData id(d.name, *m_referencePoint, RS_Vector(1.0, 1.0), 0.0,
                                 1, 1, RS_Vector(0.0, 0.0));
                creation.createInsert(&id);
            }
        }
    }

    redrawDrawing();
    setStatus(getStatus()+1); // clear mouse button hints
    updateMouseButtonHints();
    finish(false);
}

void RS_ActionBlocksCreate::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, LC_MouseEvent* pEvent) {
    fireCoordinateEventForSnap(pEvent);
}

void RS_ActionBlocksCreate::onMouseRightButtonReleaseSelected([[maybe_unused]]int status,[[maybe_unused]] LC_MouseEvent* pEvent) {
    init(getStatus()-1);
}

void RS_ActionBlocksCreate::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
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

void RS_ActionBlocksCreate::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to create block (Enter to complete)"), MOD_SHIFT_LC(tr("Select contour")));
}

void RS_ActionBlocksCreate::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionBlocksCreate::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
