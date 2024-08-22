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

#include "rs_actioneditpaste.h"
#include "rs_clipboard.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_units.h"
#include "rs_vector.h"

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditPaste::RS_ActionEditPaste( RS_EntityContainer& container,
                                        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Edit Paste",
						   container, graphicView)
        , targetPoint(std::make_unique<RS_Vector>()){}


RS_ActionEditPaste::~RS_ActionEditPaste() = default;

void RS_ActionEditPaste::init(int status) {
    RS_PreviewActionInterface::init(status);
}

void RS_ActionEditPaste::trigger() {
    deletePreview();

    RS_Modification m(*container, graphicView);
    m.paste(RS_PasteData(*targetPoint, 1.0, 0.0, false, ""));

    graphicView->redraw(RS2::RedrawDrawing);
    if (finishOnTrigger) {
       finish(false);
    }
}

void RS_ActionEditPaste::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
        case SetTargetPoint: {
            *targetPoint = snapPoint(e);

            deletePreview();
            preview->addAllFrom(*RS_CLIPBOARD->getGraphic());
            preview->move(*targetPoint);

            if (graphic) {
                RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
                RS2::Unit targetUnit = graphic->getUnit();
                double const f = RS_Units::convert(1.0, sourceUnit, targetUnit);
                preview->scale(*targetPoint, {f, f});
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionEditPaste::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    finishOnTrigger = !isControl(e);
    fireCoordinateEventForSnap(e);
}

void RS_ActionEditPaste::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionEditPaste::onCoordinateEvent([[maybe_unused]]int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    *targetPoint = pos;
    trigger();
}

void RS_ActionEditPaste::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetTargetPoint:
            updateMouseWidgetTRCancel(tr("Set paste reference point"), MOD_CTRL("Paste Multiple"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionEditPaste::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
