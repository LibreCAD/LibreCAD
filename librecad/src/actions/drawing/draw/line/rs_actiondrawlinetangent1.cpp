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

#include "rs_actiondrawlinetangent1.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

RS_ActionDrawLineTangent1::RS_ActionDrawLineTangent1(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw Tangents 1", container, graphicView)
	,tangent(nullptr)
	,point(new RS_Vector{}){
    actionType=RS2::ActionDrawLineTangent1;
}

RS_ActionDrawLineTangent1::~RS_ActionDrawLineTangent1() = default;

void RS_ActionDrawLineTangent1::trigger(){
    RS_PreviewActionInterface::trigger();

    if (tangent){
        auto *newEntity = new RS_Line(container, tangent->getData());

        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        container->addEntity(newEntity);

        addToDocumentUndoable(newEntity);

        graphicView->redraw(RS2::RedrawDrawing);

        setStatus(SetPoint);

        tangent.reset();
    } else {
        RS_DEBUG->print("RS_ActionDrawLineTangent1::trigger:"
                        " Entity is nullptr\n");
    }
}

void RS_ActionDrawLineTangent1::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineTangent1::mouseMoveEvent begin");

    RS_Vector mouse{toGraph(e)};

    switch (getStatus()) {
        case SetPoint: {
            *point = snapPoint(e);
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetCircle: {
            deletePreview();
            deleteHighlights();
            RS_Entity *en = catchEntity(e, circleType, RS2::ResolveAll);
            if (en && (en->isArc() ||
                       en->rtti() == RS2::EntityParabola ||
                       en->rtti() == RS2::EntitySplinePoints)){


                RS_Vector tangentPoint;
                RS_Vector altTangentPoint;
                RS_Creation creation(nullptr, nullptr);

                auto *tangentLine = creation.createTangent1(mouse, *point, en, tangentPoint, altTangentPoint);
                tangent.reset(tangentLine);

                if (tangentLine != nullptr){
                    highlightHover(en);
                    previewEntity(tangent->clone());
                    previewRefSelectablePoint(tangentPoint);
                    previewRefSelectablePoint(altTangentPoint);
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(*point);
                    }
                }
            }
            drawHighlights();
            drawPreview();
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLineTangent1::mouseMoveEvent end");
}

void RS_ActionDrawLineTangent1::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetPoint: {
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetCircle: {
            if (tangent){
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineTangent1::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPoint: {
            *point = pos;
            moveRelativeZero(*point);
            setStatus(SetCircle);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint:
            updateMouseWidgetTRCancel(tr("Specify point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCircle:
            updateMouseWidgetTRBack(tr("Select circle, arc or ellipse"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineTangent1::doGetMouseCursor([[maybe_unused]] int status){
    switch (status){
        case SetPoint:
            return RS2::CadCursor;
        case SetCircle:
            return RS2::SelectCursor;
        default:
            return RS2::NoCursorChange;
    }
}
