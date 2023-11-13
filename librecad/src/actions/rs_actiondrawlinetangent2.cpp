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

#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawlinetangent2.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_preview.h"
#include "rs_debug.h"

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

double linePointDist(const RS_Line& line, const RS_Vector& point)
{
    return point.distanceTo(line.getNearestPointOnEntity(point));
}
}

RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangents 2", container, graphicView, RS2::ActionDrawLineTangent2)
    , m_pPoints{std::make_unique<Points>()}
{
    init(SetCircle1);
}

RS_ActionDrawLineTangent2::~RS_ActionDrawLineTangent2()
{
    init(SetCircle1);
    for (RS_Entity* circle: {m_pPoints->circle1, m_pPoints->circle2})
    {
        if (circle != nullptr) {
            circle->setHighlighted(false);
            graphicView->drawEntity(circle);
        }
    }
}

void RS_ActionDrawLineTangent2::init(int status)
{
    setStatus(status);
    switch (status) {
    case SetCircle1:
        if (m_pPoints->circle1 != nullptr) {
            m_pPoints->circle1->setHighlighted(false);
            graphicView->drawEntity(m_pPoints->circle1);
            m_pPoints->circle1 = nullptr;
        }
        [[fallthrough]];
    case SetCircle2:
    case SelectLine:
        if (m_pPoints->circle2 != nullptr) {
            m_pPoints->circle2->setHighlighted(false);
            graphicView->drawEntity(m_pPoints->circle2);
            m_pPoints->circle2 = nullptr;
        }
        break;
    }
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawLineTangent2::finish(bool updateTB){
    clearHighlighted();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::trigger() {
    RS_PreviewActionInterface::trigger();
    if (m_pPoints->tangents.empty() || m_pPoints->tangents.front() == nullptr)
        return;

    auto* newEntity = new RS_Line{container, m_pPoints->tangents.front()->getData()};

    if (newEntity != nullptr) {
        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        container->addEntity(newEntity);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(newEntity);
            document->endUndoCycle();
        }
        init(SetCircle1);
        for (RS_Entity* circle: {m_pPoints->circle1, m_pPoints->circle2})
        {
            if (circle != nullptr) {
                circle->setHighlighted(false);
                graphicView->drawEntity(circle);
            }
        }
        m_pPoints->circle1 = nullptr;
        m_pPoints->circle2 = nullptr;
    }
}

void RS_ActionDrawLineTangent2::clearHighlighted()
{
    auto clearHighlight = [this](RS_Entity** pCircle) {
        if (*pCircle != nullptr) {
            (*pCircle)->setHighlighted(false);
            graphicView->drawEntity(*pCircle);
            *pCircle = nullptr;
        }
    };
    switch(getStatus()) {
    case SetCircle1:
        if (m_pPoints->circle2 == nullptr)
            clearHighlight(&m_pPoints->circle1);
        [[fallthrough]];
    case SelectLine:
    case SetCircle2:
        clearHighlight(&m_pPoints->circle2);
        break;
    default:
        break;
    }
}

void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e) {
    //    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");
    deleteSnapper();
    deletePreview();

    switch(getStatus())
    {
    case SetCircle1:
        return;
    case SetCircle2:
    {
        RS_Entity* en= catchEntity(e, circleType, RS2::ResolveAll);
        if(en == nullptr || en==m_pPoints->circle1)
    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));
            return;
        clearHighlighted();
        m_pPoints->circle2=en;
        m_pPoints->circle2->setHighlighted(true);
        graphicView->drawEntity(m_pPoints->circle2);
        m_pPoints->tangents = RS_Creation{preview.get()}.createTangent2(m_pPoints->circle1, m_pPoints->circle2);
        if (m_pPoints->tangents.empty()) {
            m_pPoints->circle2->setHighlighted(false);
            graphicView->drawEntity(m_pPoints->circle2);
        } else {
            preparePreivew(e);
        }
    }
        break;
    case SelectLine:
        preparePreivew(e);
    }
}

void RS_ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e)
{
    deleteSnapper();
    if (e->button()==Qt::RightButton) {
        deletePreview();
        if (getStatus() == SetCircle1) {
            if (m_pPoints->circle1 != nullptr) {
                m_pPoints->circle1->setHighlighted(true);
                graphicView->drawEntity(m_pPoints->circle1);
                m_pPoints->circle1 = nullptr;
            }
        }
        init(getStatus() - 1);
        clearHighlighted();
        return;
    }
    switch (getStatus()) {
    case SetCircle1:
    {
        m_pPoints->circle1 = catchEntity(e, circleType, RS2::ResolveAll);
        if(!m_pPoints->circle1) return;
        m_pPoints->circle1->setHighlighted(true);
        graphicView->drawEntity(m_pPoints->circle1);
        init(getStatus()+1);
    }
        break;

    case SetCircle2:
    {
        m_pPoints->tangents = RS_Creation{preview.get()}.createTangent2(m_pPoints->circle1, m_pPoints->circle2);
        if (!m_pPoints->tangents.empty()) {
            if (m_pPoints->tangents.size() == 1) {
                trigger();
            } else {
                init(getStatus()+1);
                preparePreivew(e);
            }
        }
    }
        break;
    case SelectLine:
    {
        if (!m_pPoints->tangents.empty())
            trigger();
    }
        break;
    default:
        break;
    }
}

void RS_ActionDrawLineTangent2::preparePreivew(QMouseEvent* e)
{
    switch(getStatus()) {
    case SetCircle2:
    case SelectLine:
    {
        RS_Vector mouse = snapFree(e);
        deleteSnapper();
        deletePreview();
        std::sort(m_pPoints->tangents.begin(), m_pPoints->tangents.end(), [&mouse](
                  const std::unique_ptr<RS_Line>& lhs,
                  const std::unique_ptr<RS_Line>& rhs) {
            return linePointDist(*lhs, mouse) < linePointDist(*rhs, mouse);
        });
        for(const auto& line: m_pPoints->tangents){
            auto newEndpoint = new RS_Point{preview.get(), line->getData().startpoint};
            preview->addEntity(newEndpoint);
            newEndpoint = new RS_Point{preview.get(), line->getData().endpoint};
            preview->addEntity(newEndpoint);
        }
        auto* newLine = new RS_Line{preview.get(), m_pPoints->tangents.front()->getData()};
        preview->addEntity(newLine);
        drawPreview();
    }
        break;
    default:
        break;
    }
    deleteSnapper();
}


void RS_ActionDrawLineTangent2::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCircle1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first circle/ellipse/parabola"),
                                            tr("Cancel"));
        break;
    case SetCircle2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second circle/ellipse/parabola"),
                                            tr("Back"));
        break;
    case SelectLine:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select the tangent line closest to cursor"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawLineTangent2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
