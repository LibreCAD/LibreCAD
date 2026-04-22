/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "rs_actiondrawellipseinscribe.h"

#include <vector>

#include "lc_creation_ellipse.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_preview.h"

// fixme do cleanup
struct RS_ActionDrawEllipseInscribe::Points {
    std::vector<RS_Line*> lines;
    RS_EllipseData ellipseData;
    bool valid{false};
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseInscribe::RS_ActionDrawEllipseInscribe(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("Draw ellipse inscribed", actionContext, RS2::ActionDrawEllipseInscribe),
      m_actionData(std::make_unique<Points>()) {
}

RS_ActionDrawEllipseInscribe::~RS_ActionDrawEllipseInscribe() = default;

void RS_ActionDrawEllipseInscribe::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    const auto entityParent = contextEntity->getParent();
    if (entityParent != nullptr) {
        if (entityParent->ignoredOnModification()) {
            return;
        }
    }
    if (isLine(contextEntity)) {
        // fixme- support of polyline
        m_actionData->lines.push_back(static_cast<RS_Line*>(contextEntity));
        setStatus(SetLine2);
    }
}

void RS_ActionDrawEllipseInscribe::clearLines(const bool checkStatus) const {
    while (!m_actionData->lines.empty()) {
        if (checkStatus && static_cast<int>(m_actionData->lines.size()) <= getStatus()) {
            break;
        }
        m_actionData->lines.back()->setHighlighted(false);
        m_actionData->lines.pop_back();
    }
    redrawDrawing();
}

void RS_ActionDrawEllipseInscribe::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0) {
        RS_Snapper::suspend();
    }
    clearLines(true);
}

void RS_ActionDrawEllipseInscribe::finish() {
    clearLines(false);
    LC_ActionDrawCircleBase::finish();
}

RS_Entity* RS_ActionDrawEllipseInscribe::doTriggerCreateEntity() {
    auto* ellipse = new RS_Ellipse(m_document, m_actionData->ellipseData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(ellipse->getCenter());
    }
    return ellipse;
}

void RS_ActionDrawEllipseInscribe::doTriggerCompletion([[maybe_unused]] bool success) {
    for (RS_Line* const p : m_actionData->lines) {
        if (p == nullptr) {
            continue;
        }
        p->setHighlighted(false);
    }
    clearLines(false);
    setStatus(SetLine1);
}

void RS_ActionDrawEllipseInscribe::drawSnapper() {
    // disable snapper
}

void RS_ActionDrawEllipseInscribe::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    for (RS_AtomicEntity* const pc : m_actionData->lines) {
        // highlight already selected
        highlightSelected(pc);
    }

    RS_Entity* en = catchModifiableAndDescribe(e, RS2::EntityLine);
    // bool shouldIgnore = false;
    if (en != nullptr) {
        auto* line = dynamic_cast<RS_Line*>(en);
        bool uniqueLine = true;
        for (size_t i = 0; i < status; ++i) {
            //do not pull in the same line again
            if (en->getId() == m_actionData->lines[i]->getId()) {
                uniqueLine = false;
                break;
            }
        }
        if (uniqueLine) {
            switch (status) {
                case SetLine1: {
                    highlightHover(line);
                    break;
                }
                case SetLine2: {
                    if (line != m_actionData->lines[SetLine1]) {
                        highlightHover(line);
                    }
                    break;
                }
                case SetLine3: {
                    if (line != m_actionData->lines[SetLine1] && line != m_actionData->lines[SetLine2]) {
                        highlightHover(line);
                    }
                    break;
                }
                case SetLine4: {
                    if (line != m_actionData->lines[SetLine1] && line != m_actionData->lines[SetLine2] && line != m_actionData->lines[
                        SetLine3]) {
                        //                        clearLines(true);
                        std::vector<RS_Vector> tangent;
                        tangent.reserve(4);
                        if (preparePreview(line, tangent)) {
                            highlightHover(line);
                            const auto ellipse = previewToCreateEllipse(m_actionData->ellipseData);
                            if (m_showRefEntitiesOnPreview) {
                                const RS_Vector ellipseCenter = ellipse->getCenter();

                                for (const auto& i : tangent) {
                                    previewRefPoint(ellipseCenter + i);
                                }

                                previewRefPoint(ellipseCenter);
                            }
                        }
                        else {
                            // nothing, can't build the ellipse
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

bool RS_ActionDrawEllipseInscribe::preparePreview(RS_Line* fourthLineCandidate, std::vector<RS_Vector>& tangent) const {
    m_actionData->valid = false;
    m_actionData->lines.push_back(fourthLineCandidate);
    RS_EllipseData data;
    m_actionData->valid = LC_CreationEllipse::createEllipseInscribeQuadrilateral(m_actionData->lines, tangent, data);
    if (m_actionData->valid) {
        m_actionData->ellipseData = data;
        //    } else if (RS_DIALOGFACTORY){
        //        RS_DIALOGFACTORY->commandMessage(tr("Can not determine uniquely an ellipse"));
    }
    m_actionData->lines.pop_back();
    return m_actionData->valid;
}

void RS_ActionDrawEllipseInscribe::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine);

    if (en != nullptr) {
        for (size_t i = 0; i < status; ++i) {
            if (en->getId() == m_actionData->lines[i]->getId()) {
                return; //do not pull in the same line again
            }
        }
        if (en->getParent() != nullptr) {
            if (en->getParent()->ignoredOnModification()) {
                return;
            }
        }
        clearLines(true);

        auto* line = dynamic_cast<RS_Line*>(en);
        switch (status) {
            case SetLine1:
            case SetLine2:
            case SetLine3: {
                m_actionData->lines.push_back(line);
                setStatus(getStatus() + 1);
                break;
            }
            case SetLine4: {
                std::vector<RS_Vector> tangent;
                tangent.reserve(4);
                if (preparePreview(line, tangent)) {
                    m_actionData->lines.push_back(line);
                    trigger();
                }
                else {
                    commandMessage(tr("Can not determine uniquely an ellipse"));
                }
                break;
            }
            default:
                break;
        }
    }
}

void RS_ActionDrawEllipseInscribe::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    if (status > 0) {
        clearLines(true);
        m_actionData->lines.pop_back();
        deletePreview();
    }
    initPrevious(status);
}

/*
void RS_ActionDrawEllipse4Line::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    default:
        break;
    }
}
*/

QStringList RS_ActionDrawEllipseInscribe::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipseInscribe::updateActionPrompt() {
    switch (getStatus()) {
        case SetLine1:
            updatePromptTRCancel(tr("Specify the first line"));
            break;
        case SetLine2:
            updatePromptTRBack(tr("Specify the second line"));
            break;
        case SetLine3:
            updatePromptTRBack(tr("Specify the third line"));
            break;
        case SetLine4:
            updatePromptTRBack(tr("Specify the fourth line"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionDrawEllipseInscribe::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
