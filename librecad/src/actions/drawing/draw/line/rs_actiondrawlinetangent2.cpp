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

#include <vector>

#include <QMouseEvent>

#include "rs_actiondrawlinetangent2.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_preview.h"

struct RS_ActionDrawLineTangent2::Points {
    /** Closest tangent. */
    std::vector<std::unique_ptr<RS_Line>> tangents;
    /** 1st chosen entity */
    RS_Entity* circle1 = nullptr;
    /** 2nd chosen entity */
    RS_Entity* circle2 = nullptr;
};
namespace {

//list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityParabola};

    double linePointDist(const RS_Line &line, const RS_Vector &point){
        return point.distanceTo(line.getNearestPointOnEntity(point));
    }
}

RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Draw Tangents 2", container, graphicView, RS2::ActionDrawLineTangent2), m_pPoints{std::make_unique<Points>()}{
    init(SetCircle1);
}

RS_ActionDrawLineTangent2::~RS_ActionDrawLineTangent2(){
    init(SetCircle1);
}

void RS_ActionDrawLineTangent2::init(int status){
    setStatus(status);
    switch (status) {
        case SetCircle1:
            cleanup();
            break;
        case SetCircle2:
            m_pPoints->circle2 = nullptr;
            break;
        case SelectLine:
            break;
        default:
            break;
    }
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawLineTangent2::finish(bool updateTB){
    cleanup();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::trigger(){
    RS_PreviewActionInterface::trigger();
    if (m_pPoints->tangents.empty() || m_pPoints->tangents.front() == nullptr)
        return;

    auto *newEntity = new RS_Line{container, m_pPoints->tangents.front()->getData()};

    newEntity->setLayerToActive();
    newEntity->setPenToActive();
    container->addEntity(newEntity);

    addToDocumentUndoable(newEntity);

    graphicView->redraw(RS2::RedrawAll);
    init(SetCircle1);
    cleanup();
}

void RS_ActionDrawLineTangent2::cleanup(){
    this->m_pPoints->circle1 = nullptr;
    this->m_pPoints->circle2 = nullptr;
}


void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent *e){
    //    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");
    deleteSnapper();
    deletePreview();
    deleteHighlights();
    switch (getStatus()) {
        case SetCircle1: {
            deletePreview();
            auto *en = catchEntity(e, circleType, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            drawPreview();
            break;
        }
        case SetCircle2: {
            RS_Entity *en = catchEntity(e, circleType, RS2::ResolveAll);
            highlightSelected(m_pPoints->circle1);
            if (en != nullptr && en != m_pPoints->circle1){
                highlightHover(en);
                m_pPoints->circle2 = en;

                m_pPoints->tangents = RS_Creation{preview.get()}.createTangent2(m_pPoints->circle1, m_pPoints->circle2);
                if (m_pPoints->tangents.empty()){
                } else {
                    preparePreview(e);
                }
            }
            break;
        }
        case SelectLine: {
            highlightSelected(m_pPoints->circle1);
            highlightSelected(m_pPoints->circle2);
            preparePreview(e);
            break;
        }
    }
    drawHighlights();
}

void RS_ActionDrawLineTangent2::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    deleteSnapper();
    switch (status) {
        case SetCircle1: {
            m_pPoints->circle1 = catchEntity(e, circleType, RS2::ResolveAll);
            if (!m_pPoints->circle1) return;
            init(status + 1);
            break;
        }
        case SetCircle2: {
            m_pPoints->tangents = RS_Creation{preview.get()}.createTangent2(m_pPoints->circle1, m_pPoints->circle2);
            if (!m_pPoints->tangents.empty()){
                if (m_pPoints->tangents.size() == 1){
                    trigger();
                } else {
                    init(status + 1);
                    preparePreview(e);
                }
            }
            break;
        }
        case SelectLine: {
            if (!m_pPoints->tangents.empty())
                trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineTangent2::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deleteSnapper();
    deletePreview();
    if (status == SetCircle1){
        if (m_pPoints->circle1 != nullptr){
            m_pPoints->circle1 = nullptr;
        }
    }
    initPrevious(status);
}

void RS_ActionDrawLineTangent2::preparePreview(QMouseEvent *e){
    switch (getStatus()) {
        case SetCircle2:
        case SelectLine: {
            RS_Vector mouse = snapFree(e);
            deleteSnapper();
            deletePreview();
            std::sort(m_pPoints->tangents.begin(), m_pPoints->tangents.end(), [&mouse](
                const std::unique_ptr<RS_Line> &lhs,
                const std::unique_ptr<RS_Line> &rhs){
                return linePointDist(*lhs, mouse) < linePointDist(*rhs, mouse);
            });
            for (const auto &line: m_pPoints->tangents) {
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(line->getData().startpoint);
                }
                previewRefSelectablePoint(line->getData().endpoint);
            }
            const RS_LineData &lineData = m_pPoints->tangents.front()->getData();
            previewLine(lineData.startpoint, lineData.endpoint);
            drawPreview();
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
