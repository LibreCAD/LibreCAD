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

#include "rs_actiondrawlinetangent2.h"

#include "rs_creation.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_preview.h"

struct RS_ActionDrawLineTangent2::ActionData {
    /** Closest tangent. */
    std::vector<std::unique_ptr<RS_Line>> tangents;
    /** 1st chosen entity */
    RS_Entity* circle1 = nullptr;
    /** 2nd chosen entity */
    RS_Entity* circle2 = nullptr;
};

namespace {
    //list of entity types supported by current action
    const auto g_circleType = EntityTypeList{RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse,
                                         RS2::EntityHyperbola,
                                         RS2::EntityParabola};

    double linePointDist(const RS_Line &line, const RS_Vector &point){
        return point.distanceTo(line.getNearestPointOnEntity(point));
    }
}

RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Tangents 2", actionContext, RS2::ActionDrawLineTangent2), m_actionData{std::make_unique<ActionData>()}{
}

RS_ActionDrawLineTangent2::~RS_ActionDrawLineTangent2(){
    cleanup();
}

void RS_ActionDrawLineTangent2::init(int status){
    if (status == SetCircle1) {
        cleanup();
    }
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawLineTangent2::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    RS2::EntityType rtti = entity->rtti();
    if (g_circleType.contains(rtti)) {
        m_actionData->circle1 = entity;
        m_actionData->circle2 = nullptr;
        setStatus(SetCircle2);
        highlightHover(entity);
        drawPreview();
    }
}

void RS_ActionDrawLineTangent2::finish(bool updateTB){
    cleanup();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::doTrigger() {
    if (m_actionData->tangents.empty() || m_actionData->tangents.front() == nullptr) {
        return;
    }

    auto *newEntity = new RS_Line{m_container, m_actionData->tangents.front()->getData()};

    setPenAndLayerToActive(newEntity);
    undoCycleAdd(newEntity);
    cleanup();
    setStatus(SetCircle1);
}

void RS_ActionDrawLineTangent2::cleanup(){
    m_actionData->circle1 = nullptr;
    m_actionData->circle2 = nullptr;
}

void RS_ActionDrawLineTangent2::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    deleteSnapper();
    switch (status) {
        case SetCircle1: {
            deletePreview();
            auto *en = catchAndDescribe(e, g_circleType, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetCircle2: {
            RS_Entity *en = catchAndDescribe(e, g_circleType, RS2::ResolveAll);
            highlightSelected(m_actionData->circle1);
            if (en != nullptr && en != m_actionData->circle1){
                highlightHover(en);
                m_actionData->circle2 = en;

                m_actionData->tangents = RS_Creation{m_preview.get(), nullptr, false}.createTangent2(m_actionData->circle1, m_actionData->circle2);
                if (m_actionData->tangents.empty()){
                } else {
                    preparePreview(status, e);
                }
            }
            break;
        }
        case SelectLine: {
            // FIXME _ SAND _ is there not-necessary click?
            highlightSelected(m_actionData->circle1);
            highlightSelected(m_actionData->circle2);
            preparePreview(status, e);
            break;
        }
    }
}

void RS_ActionDrawLineTangent2::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    deleteSnapper();
    switch (status) {
        case SetCircle1: {
            m_actionData->circle1 = catchEntityByEvent(e, g_circleType, RS2::ResolveAll);
            if (m_actionData->circle1 == nullptr) {
                return;
            }
            setStatus(SetCircle2);
            break;
        }
        case SetCircle2: {
            m_actionData->tangents = RS_Creation{m_preview.get()}.createTangent2(m_actionData->circle1, m_actionData->circle2);
            if (!m_actionData->tangents.empty()){
                if (m_actionData->tangents.size() == 1){
                    trigger();
                } else {
                    setStatus(SelectLine);
                    preparePreview(status, e);
                    invalidateSnapSpot();
                }
            }
            break;
        }
        case SelectLine: {
            if (!m_actionData->tangents.empty()) {
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent2::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deleteSnapper();
    deletePreview();
    if (status == SetCircle1) {
        m_actionData->circle1 = nullptr;
    }
    initPrevious(status);
}

void RS_ActionDrawLineTangent2::preparePreview(int status, LC_MouseEvent *e){
    switch (status) {
        case SetCircle2:
        case SelectLine: {
            RS_Vector mouse = e->graphPoint;
            std::sort(m_actionData->tangents.begin(), m_actionData->tangents.end(), [&mouse](
                const std::unique_ptr<RS_Line> &lhs,
                const std::unique_ptr<RS_Line> &rhs){
                return linePointDist(*lhs, mouse) < linePointDist(*rhs, mouse);
            });
            for (const auto &line: m_actionData->tangents) {
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(line->getData().startpoint);
                }
                previewRefSelectablePoint(line->getData().endpoint);
            }
            const RS_LineData &lineData = m_actionData->tangents.front()->getData();
            previewToCreateLine(lineData.startpoint, lineData.endpoint);
            break;
        }
        default:
            break;
    }
    deleteSnapper();
}

void RS_ActionDrawLineTangent2::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCircle1:
            updateMouseWidgetTRCancel(tr("Select first circle/ellipse/parabola"));
            break;
        case SetCircle2:
            updateMouseWidgetTRBack(tr("Select second circle/ellipse/parabola"));
            break;
        case SelectLine:
            updateMouseWidgetTRBack(tr("Select the tangent line closest to cursor"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionDrawLineTangent2::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
