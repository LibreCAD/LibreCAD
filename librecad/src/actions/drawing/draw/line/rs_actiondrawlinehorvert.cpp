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

#include "rs_actiondrawlinehorvert.h"

#include "rs_debug.h"
#include "rs_document.h"
#include "rs_line.h"

struct RS_ActionDrawLineHorVert::ActionData {
    /**
  * Line data.
  */
    RS_LineData data;
    /**
  * 2 points
  */
    RS_Vector p1;
    RS_Vector p2;
};

RS_ActionDrawLineHorVert::RS_ActionDrawLineHorVert(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("Draw horizontal/vertical lines", actionContext, RS2::ActionDrawLineHorVert),
      m_actionData(std::make_unique<ActionData>()) {
    reset();
    RS_DEBUG->print("RS_ActionDrawLineHorVert::constructor");
}

RS_ActionDrawLineHorVert::~RS_ActionDrawLineHorVert() = default;

void RS_ActionDrawLineHorVert::reset() const {
    m_actionData->data = {};
}

void RS_ActionDrawLineHorVert::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
    RS_DEBUG->print("RS_ActionDrawLineHorVert::init");
}

RS_Entity* RS_ActionDrawLineHorVert::doTriggerCreateEntity() {
    auto* line = new RS_Line(m_document, m_actionData->data);
    moveRelativeZero(line->getMiddlePoint());
    return line;
}

void RS_ActionDrawLineHorVert::doTriggerCompletion([[maybe_unused]] bool success) {
}

void RS_ActionDrawLineHorVert::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (getStatus() == SetEndpoint && m_actionData->p1.valid) {
        const RS_Vector mouse = e->snapPoint;
        const auto p2x = RS_Vector(mouse.x, m_actionData->p1.y);
        const auto p2y = RS_Vector(m_actionData->p1.x, mouse.y);
        if (mouse.distanceTo(p2y) > mouse.distanceTo(p2x)) {
            m_actionData->p2 = p2x;
        }
        else {
            m_actionData->p2 = p2y;
        }
        m_actionData->data = {m_actionData->p1, m_actionData->p2};
        previewToCreateLine(m_actionData->p1, m_actionData->p2);
    }
}

void RS_ActionDrawLineHorVert::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartpoint: {
            m_actionData->p1 = mouse;
            setStatus(SetEndpoint);
            break;
        }
        case SetEndpoint: {
            m_actionData->p2 = mouse;
            trigger();
            setStatus(SetStartpoint);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineHorVert::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineHorVert::updateActionPrompt() {
    switch (getStatus()) {
        case SetStartpoint:
            updatePromptTRCancel(tr("Specify first point"));
            break;
        case SetEndpoint:
            updatePromptTRBack(tr("Specify second point"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineHorVert::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
