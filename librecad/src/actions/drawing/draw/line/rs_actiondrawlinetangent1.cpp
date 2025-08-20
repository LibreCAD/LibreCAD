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

#include "rs_actiondrawlinetangent1.h"

#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
    //list of entity types supported by current action
    const auto g_supportedEntityTypes = EntityTypeList{ RS2::EntityArc,
                                                      RS2::EntityCircle,
                                                      RS2::EntityEllipse,
                                                      RS2::EntityParabola,
                                                      RS2::EntitySplinePoints };
}

RS_ActionDrawLineTangent1::RS_ActionDrawLineTangent1(LC_ActionContext *actionContext)
	:RS_PreviewActionInterface("Draw Tangents 1", actionContext,RS2::ActionDrawLineTangent1)
	,m_tangent(nullptr)
	,m_point(new RS_Vector{}){
}

RS_ActionDrawLineTangent1::~RS_ActionDrawLineTangent1() = default;

void RS_ActionDrawLineTangent1::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        auto polyline = static_cast<RS_Polyline*> (contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    RS2::EntityType rtti = entity->rtti();
    if (g_supportedEntityTypes.contains(rtti)) {
        m_setCircleFirst = true;
        m_entity = entity;
        setStatus(SetPoint);
    }
}

void RS_ActionDrawLineTangent1::doTrigger() {
    if (m_tangent != nullptr){
        auto *newEntity = new RS_Line(m_container, m_tangent->getData());

        setPenAndLayerToActive(newEntity);
        undoCycleAdd(newEntity);
        setStatus(SetPoint);
        m_tangent.reset();
    } else {
        RS_DEBUG->print("RS_ActionDrawLineTangent1::trigger: Entity is nullptr\n");
    }
}

void RS_ActionDrawLineTangent1::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    const RS_Vector &snap = e->snapPoint;

    switch (status) {
        case SetPoint: {
            *m_point = snap;
            if (!trySnapToRelZeroCoordinateEvent(e) &&  m_setCircleFirst) {
                highlightSelected(m_entity);
                RS_Vector tangentPoint;
                RS_Vector altTangentPoint;
                RS_Creation creation(nullptr, nullptr);
                auto* tangentLine = creation.createTangent1(mouse, *m_point, m_entity, tangentPoint, altTangentPoint);
                m_tangent.reset(tangentLine);
                if (tangentLine != nullptr) {
                    if (e->isControl) {
                        tangentLine = creation.createTangent1(altTangentPoint, *m_point, m_entity, tangentPoint, altTangentPoint);
                        m_tangent.reset(tangentLine);
                    }

                    if (tangentLine != nullptr) {
                        previewEntityToCreate(tangentLine->clone(), true);
                        previewRefSelectablePoint(tangentPoint);
                        previewRefSelectablePoint(altTangentPoint);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(*m_point);
                        }
                    }
                }
            }
            break;
        }
        case SetCircle: {
            deleteSnapper();

            RS_Entity *en = catchAndDescribe(e, g_supportedEntityTypes, RS2::ResolveAll);
            if (en != nullptr) {
                auto rtti = en->rtti();
                if (en->isArc() || rtti == RS2::EntityParabola || rtti == RS2::EntitySplinePoints) {
                    if (m_setCircleFirst) {
                        highlightHover(en);
                        m_entity = en;
                    }
                    else {
                        RS_Vector tangentPoint;
                        RS_Vector altTangentPoint;
                        RS_Creation creation(nullptr, nullptr);
                        auto* tangentLine = creation.createTangent1(mouse, *m_point, en, tangentPoint, altTangentPoint);
                        m_tangent.reset(tangentLine);

                        if (tangentLine != nullptr) {
                            highlightHover(en);
                            previewEntityToCreate(m_tangent->clone(), true);
                            previewRefSelectablePoint(tangentPoint);
                            previewRefSelectablePoint(altTangentPoint);
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(*m_point);
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetPoint: {
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetCircle: {
            if (m_tangent != nullptr){
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineTangent1::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPoint: {
            *m_point = pos;
            moveRelativeZero(*m_point);
            if (m_setCircleFirst) {
                trigger();
            }
            else {
                setStatus(SetCircle);
            }
            invalidateSnapSpot();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent1::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint:
            if (m_setCircleFirst) {
                updateMouseWidgetTRCancel(tr("Specify point"), MOD_CTRL(tr("Alternate Point")));
            }
            else {
                updateMouseWidgetTRCancel(tr("Specify point"), MOD_SHIFT_RELATIVE_ZERO);
            }
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
