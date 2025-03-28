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

#include "rs_actioneditcopy.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_modification.h"
#include "rs_clipboard.h"
#include "rs_preview.h"
#include "rs_units.h"

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
// fixme - sand - no action type set!!!
RS_ActionEditCopyPaste::RS_ActionEditCopyPaste(ActionMode actionMode, LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Edit Copy", actionContext)
    , mode{actionMode}
    , referencePoint{new RS_Vector{}}{
}

RS_ActionEditCopyPaste::~RS_ActionEditCopyPaste() = default;

void RS_ActionEditCopyPaste::init(int status) {
    RS_PreviewActionInterface::init(status);
    *referencePoint = RS_Vector(false);
    if (mode == CUT_QUICK || mode == COPY_QUICK) {
        trigger();
    }
}

void RS_ActionEditCopyPaste::doTrigger() {
    switch (mode){
        case CUT:
        case CUT_QUICK:
        case COPY:
        case COPY_QUICK:{
            RS_Modification m(*m_container, m_viewport);
            m.copy(*referencePoint, mode == CUT || mode == CUT_QUICK);

            if (invokedWithControl){
                mode = PASTE;
                invokedWithControl = false;
            }
            else{
                //graphicView->redraw();
                m_graphicView->killSelectActions();
                finish(false);
            }
            break;
        }
        case PASTE: {
            RS_Modification m(*m_container, m_viewport);
            m.paste(RS_PasteData(*referencePoint, 1.0, 0.0, false, ""));

            if (!invokedWithControl) {
                finish(false);
            }
            break;
        }
    }
}

void RS_ActionEditCopyPaste::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    if (status==SetReferencePoint) {
        switch (mode) {
            case CUT:
            case COPY:{
//                (void) e->snapPoint;
                break;
            }
            case PASTE:{
                *referencePoint = e->snapPoint;
                m_preview->addAllFrom(*RS_CLIPBOARD->getGraphic(), m_viewport);
                m_preview->move(*referencePoint);

                if (m_graphic) {
                    RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
                    RS2::Unit targetUnit = m_graphic->getUnit();
                    double const f = RS_Units::convert(1.0, sourceUnit, targetUnit);
                    m_preview->scale(*referencePoint, {f, f});
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

void RS_ActionEditCopyPaste::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    invokedWithControl = e->isControl;
    fireCoordinateEventForSnap(e);
}

void RS_ActionEditCopyPaste::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

void RS_ActionEditCopyPaste::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    *referencePoint = pos;
    trigger();
}

void RS_ActionEditCopyPaste::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        switch (mode) {
            case CUT:
            case COPY: {
                updateMouseWidgetTRCancel(tr("Specify reference point"), MOD_CTRL(tr("Paste Immediately")));
                break;
            }
            case PASTE:{
                updateMouseWidgetTRCancel(tr("Set paste reference point"), MOD_CTRL(tr("Paste Multiple")));
                break;
            }
            default:
                break;
        }
        break;
    default:
        updateMouseWidget();
        break;
    }
}

RS2::CursorType RS_ActionEditCopyPaste::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::CadCursor;
}
