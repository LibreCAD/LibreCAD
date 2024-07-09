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

#include<QAction>
#include <QMouseEvent>

#include "rs_actiondrawlinetangent1.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"

// fixme - add reference points
RS_ActionDrawLineTangent1::RS_ActionDrawLineTangent1(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw Tangents 1", container, graphicView)
	,tangent(nullptr)
	,point(new RS_Vector{})
{
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
                    previewRefPoint(*point);
                    previewRefSelectablePoint(tangentPoint, true);
                    previewRefSelectablePoint(altTangentPoint, true);
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

void RS_ActionDrawLineTangent1::mouseReleaseEvent(QMouseEvent *e){

    if (e->button() == Qt::RightButton){
        deletePreview();
        init(getStatus() - 1);
    } else {
        switch (getStatus()) {
            case SetPoint: {
                fireCoordinateEventForSnap(e);
                break;
            }
            case SetCircle:
                if (tangent){
                    trigger();
                }
                break;
        }
    }
}

void RS_ActionDrawLineTangent1::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) return;
    switch (getStatus()) {
        case SetPoint:
            *point = e->getCoordinate();
            moveRelativeZero(*point);
            setStatus(SetCircle);
            break;

        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint:
            updateMouseWidgetTRCancel("Specify point", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCircle:
            updateMouseWidgetTRBack("Select circle, arc or ellipse");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionDrawLineTangent1::updateMouseCursor(){
    switch (getStatus()){
        case SetPoint:
            setMouseCursor(RS2::CadCursor);
            break;
        case SetCircle:
            setMouseCursor(RS2::SelectCursor);
            break;
    }
}

// EOF
