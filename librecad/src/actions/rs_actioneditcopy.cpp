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

#include <QMouseEvent>

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
RS_ActionEditCopyPaste::RS_ActionEditCopyPaste(ActionMode actionMode,
                                               RS_EntityContainer& container,
                                               RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Edit Copy",container, graphicView)
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

void RS_ActionEditCopyPaste::trigger() {
    switch (mode){
        case CUT:
        case CUT_QUICK:
        case COPY:
        case COPY_QUICK:{
            RS_Modification m(*container, graphicView);
            m.copy(*referencePoint, mode == CUT || mode == CUT_QUICK);

            updateSelectionWidget();
            if (invokedWithControl){
                mode = PASTE;
                invokedWithControl = false;
            }
            else{
                //graphicView->redraw();
                graphicView->killSelectActions();
                finish(false);
            }
            break;
        }
        case PASTE: {
            deletePreview();

            RS_Modification m(*container, graphicView);
            m.paste(RS_PasteData(*referencePoint, 1.0, 0.0, false, ""));

            graphicView->redraw(RS2::RedrawDrawing);
            if (!invokedWithControl) {
                finish(false);
            }
            break;
        }
    }
}

void RS_ActionEditCopyPaste::mouseMoveEvent(QMouseEvent* e) {
    if (getStatus()==SetReferencePoint) {
        switch (mode) {
            case CUT:
            case COPY:{
                (void) snapPoint(e);
                break;
            }
            case PASTE:{
                *referencePoint = snapPoint(e);
                deletePreview();
                preview->addAllFrom(*RS_CLIPBOARD->getGraphic());
                preview->move(*referencePoint);

                if (graphic) {
                    RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
                    RS2::Unit targetUnit = graphic->getUnit();
                    double const f = RS_Units::convert(1.0, sourceUnit, targetUnit);
                    preview->scale(*referencePoint, {f, f});
                }
                drawPreview();
                break;
            }
            default:
                break;
        }
    }
    else
        deleteSnapper();
}

void RS_ActionEditCopyPaste::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    invokedWithControl = isControl(e);
    fireCoordinateEventForSnap(e);
}

void RS_ActionEditCopyPaste::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
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
