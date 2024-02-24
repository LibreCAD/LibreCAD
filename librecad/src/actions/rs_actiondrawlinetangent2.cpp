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
#include "rs_actiondrawlinetangent2.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace {
double linePointDist(const RS_Line& line, const RS_Vector& point)
{
    return point.distanceTo(line.getNearestPointOnEntity(point));
}
}
RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangents 2", container, graphicView)
    ,circle1(nullptr)
    ,circle2(nullptr)
    ,valid(false)
{
    m_tangents.clear();
    actionType=RS2::ActionDrawLineTangent2;
    init(SetCircle1);
}

RS_ActionDrawLineTangent2::~RS_ActionDrawLineTangent2()
{
    init(SetCircle1);
    clearHighlighted();
}


void RS_ActionDrawLineTangent2::finish(bool updateTB){
    clearHighlighted();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::trigger() {
    RS_PreviewActionInterface::trigger();
    if (m_tangents.empty() || m_tangents.front() == nullptr)
        return;

    RS_Entity* newEntity = new RS_Line{container, m_tangents.front()->getData()};

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
        clearHighlighted();
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
        if (circle2 == nullptr)
            clearHighlight(&circle1);
        [[fallthrough]];
    case SetCircle2:
        clearHighlight(&circle2);
        break;
    default:
        break;
    }
}

void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e) {
    //    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");
    e->accept();
    deleteSnapper();
    deletePreview();

    switch(getStatus())
    {
    case SetCircle1:
        return;
    case SetCircle2:
    {
        RS_Entity* en= catchEntity(e, circleType, RS2::ResolveAll);
        if(en == nullptr || en==circle1)
            return;
        clearHighlighted();
        circle2=en;
        circle2->setHighlighted(true);
        graphicView->drawEntity(circle2);
        m_tangents = RS_Creation{preview.get()}.createTangent2(circle1, circle2);
        if (m_tangents.empty()) {
            circle2->setHighlighted(false);
            graphicView->drawEntity(circle2);
        } else {
            preparePreivew(e);
        }
    }
        break;
    case SelectLine:
        preparePreivew(e);
    }
}

void RS_ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        deletePreview();
        if (getStatus() != SetCircle1)
            init(getStatus()-1);
        clearHighlighted();
        return;
    }
    switch (getStatus()) {
    case SetCircle1:
    {
        circle1 = catchEntity(e, circleType, RS2::ResolveAll);
        if(!circle1) return;
        circle1->setHighlighted(true);
        graphicView->drawEntity(circle1);
        setStatus(getStatus()+1);
    }
        break;

    case SetCircle2:
    {
        m_tangents = RS_Creation{preview.get()}.createTangent2(circle1, circle2);
        if (!m_tangents.empty()) {
            if (m_tangents.size() == 1) {
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
        if (!m_tangents.empty())
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
        std::sort(m_tangents.begin(), m_tangents.end(), [&mouse](
                  const std::unique_ptr<RS_Line>& lhs,
                  const std::unique_ptr<RS_Line>& rhs) {
            return linePointDist(*lhs, mouse) < linePointDist(*rhs, mouse);
        });
        for(const auto& line: m_tangents){
            auto newEndpoint = new RS_Point{preview.get(), line->getData().startpoint};
            preview->addEntity(newEndpoint);
            newEndpoint = new RS_Point{preview.get(), line->getData().endpoint};
            preview->addEntity(newEndpoint);
        }
        auto newLine = new RS_Line{preview.get(), m_tangents.front()->getData()};
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
