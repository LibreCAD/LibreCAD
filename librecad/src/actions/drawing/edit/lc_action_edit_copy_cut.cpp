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

#include "lc_action_edit_copy_cut.h"

#include "lc_copyutils.h"
#include "rs_clipboard.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_units.h"

/**
 * Constructor.
 *
 * @param actionContext
 * @param actionType
 */

RS_ActionEditCopyPaste::RS_ActionEditCopyPaste(LC_ActionContext *actionContext, const RS2::ActionType actionType)
    :LC_ActionPreSelectionAwareBase("Edit Copy", actionContext, actionType)
    , m_referencePoint{new RS_Vector{}}{
}

RS_ActionEditCopyPaste::~RS_ActionEditCopyPaste() = default;

void RS_ActionEditCopyPaste::init(const int status) {
    LC_ActionPreSelectionAwareBase::init(status);
    *m_referencePoint = RS_Vector(false);
    if (m_actionType  == RS2::ActionEditPaste) {
        m_selectionComplete  = true;
    }
    if (m_selectionComplete) {
        updateActionPrompt();
    }
}

void RS_ActionEditCopyPaste::onSelectionCompleted([[maybe_unused]] const bool singleEntity, const bool fromInit) {
    switch (m_actionType) {
        case RS2::ActionEditCutQuick:
        case RS2::ActionEditCopyQuick: {
            LC_ActionPreSelectionAwareBase::onSelectionCompleted(singleEntity, fromInit);
            break;
        }
        default: {
            setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
            updateActionPrompt();
            break;
        }
    }
}

bool RS_ActionEditCopyPaste::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    switch (m_actionType){
        case  RS2::ActionEditCut:
        case  RS2::ActionEditCutQuick:
        case  RS2::ActionEditCopy:
        case  RS2::ActionEditCopyQuick:{
            QList<RS_Entity*> selection;
            m_document->getSelection()->collectSelectedEntities(selection);
            if (!selection.empty()) {
                const bool cut = m_actionType ==  RS2::ActionEditCut || m_actionType == RS2::ActionEditCutQuick;
                LC_CopyUtils::copy(*m_referencePoint, selection, m_graphic);
                unselectAll();
                if (cut) {
                    ctx-= selection;
                }

                if (m_invokedWithControl){
                    m_actionType = RS2::ActionEditPaste;
                    m_invokedWithControl = false;
                }
                else{
                    finish();
                }
            }
            break;
        }
        case RS2::ActionEditPaste: {
            const LC_CopyUtils::RS_PasteData pasteData(*m_referencePoint);
            LC_CopyUtils::paste(pasteData, m_graphic, ctx);
            ctx.dontSetActiveLayerAndPen();
            if (!m_invokedWithControl) {
                finish();
            }
            break;
        }
        default:
            break;
    }
    return true;
}

void RS_ActionEditCopyPaste::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    LC_ActionPreSelectionAwareBase::doTriggerSelectionUpdate(keepSelected, ctx); // fixme - complete!
}

bool RS_ActionEditCopyPaste::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint);
}

void RS_ActionEditCopyPaste::doTriggerCompletion(const bool success) {
    LC_ActionPreSelectionAwareBase::doTriggerCompletion(success); // fixme - complete!
}

void RS_ActionEditCopyPaste::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    if (status==SetReferencePoint) {
         switch (m_actionType) {
            case RS2::ActionEditCut:
            case RS2::ActionEditCopy:{
                break;
            }
            case RS2::ActionEditPaste:{
                *m_referencePoint = e->snapPoint;
                m_preview->addAllFrom(*RS_CLIPBOARD->getGraphic(), m_viewport);
                m_preview->move(*m_referencePoint);

                if (m_graphic) {
                    const RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
                    const RS2::Unit targetUnit = m_graphic->getUnit();
                    const double f = RS_Units::convert(1.0, sourceUnit, targetUnit);
                    m_preview->scale(*m_referencePoint, {f, f});
                }
                break;
            }
            default:
                break;
        }
    }
    else {
        deleteSnapper();
    }
}

void RS_ActionEditCopyPaste::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    m_invokedWithControl = e->isControl;
    fireCoordinateEventForSnap(e);
}

void RS_ActionEditCopyPaste::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    initPrevious(status);
}

void RS_ActionEditCopyPaste::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    *m_referencePoint = pos;
    trigger();
}

void RS_ActionEditCopyPaste::updateActionPromptForSelection() {
   switch (m_actionType) {
       case RS2::ActionEditCut: {
           updatePromptTRCancel(tr("Select to cut") + getSelectionCompletionHintMsg(),
                                     MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Set point after selection")));
           break;
       }
       case RS2::ActionEditCutQuick: {
           updatePromptTRCancel(tr("Select to cut") + getSelectionCompletionHintMsg(),
                                     MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Cut right after selection")));
           break;
       }
       case RS2::ActionEditCopy: {
           updatePromptTRCancel(tr("Select to copy") + getSelectionCompletionHintMsg(),
                                     MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Set point after selection")));
           break;
       }
       case RS2::ActionEditCopyQuick: {
           updatePromptTRCancel(tr("Select to cut") + getSelectionCompletionHintMsg(),
                                     MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Copy right after selection")));
           break;
       }
       default:
           break;
   }
}

void RS_ActionEditCopyPaste::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint: {
            switch (m_actionType) {
                case RS2::ActionEditCut:
                case RS2::ActionEditCopy: {
                    updatePromptTRCancel(tr("Specify reference point"), MOD_CTRL(tr("Paste Immediately")));
                    break;
                }
                case RS2::ActionEditPaste: {
                    updatePromptTRCancel(tr("Set paste reference point"), MOD_CTRL(tr("Paste Multiple")));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:{
            updatePrompt();
            break;
        }
    }
}

RS2::CursorType RS_ActionEditCopyPaste::doGetMouseCursorSelected([[maybe_unused]]int status){
    return RS2::CadCursor;
}
