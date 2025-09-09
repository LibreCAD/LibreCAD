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

#include <vector>

#include "rs_actiondrawellipseinscribe.h"

#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_preview.h"

// fixme do cleanup
struct RS_ActionDrawEllipseInscribe::Points {
    std::vector<RS_Line*> lines;
    RS_EllipseData eData;
    bool valid{false};
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseInscribe::RS_ActionDrawEllipseInscribe(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("Draw ellipse inscribed", actionContext, RS2::ActionDrawEllipseInscribe)
      , m_actionData(std::make_unique<Points>()) {
}

RS_ActionDrawEllipseInscribe::~RS_ActionDrawEllipseInscribe() = default;

void RS_ActionDrawEllipseInscribe::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    auto entityParent = contextEntity->getParent();
    if (entityParent != nullptr) {
        if (entityParent->ignoredOnModification()) {
            return;
        }
    }
    if (isLine(contextEntity)) { // fixme- support of polyline
        m_actionData->lines.push_back(static_cast<RS_Line*>(contextEntity));
        setStatus(SetLine2);
    }
}

void RS_ActionDrawEllipseInscribe::clearLines(bool checkStatus) {
    while (m_actionData->lines.size()) {
        if (checkStatus && (int)m_actionData->lines.size() <= getStatus()) {
            break;
        }
        m_actionData->lines.back()->setHighlighted(false);
        m_actionData->lines.pop_back();
    }
    redrawDrawing();
}

void RS_ActionDrawEllipseInscribe::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0) {
        RS_Snapper::suspend();
    }
    clearLines(true);
}

void RS_ActionDrawEllipseInscribe::finish(bool updateTB) {
    clearLines(false);
    LC_ActionDrawCircleBase::finish(updateTB);
}

void RS_ActionDrawEllipseInscribe::doTrigger() {
    auto* ellipse = new RS_Ellipse(m_container, m_actionData->eData);

    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(ellipse->getCenter());
    }

    undoCycleAdd(ellipse);

    for (RS_Line* const p : m_actionData->lines) {
        if (p == nullptr) {
            continue;
        }
        p->setHighlighted(false);
    }

    clearLines(false);
    setStatus(SetLine1);

    RS_DEBUG->print("RS_ActionDrawEllipse4Line::trigger():entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseInscribe::drawSnapper() {
    // disable snapper
}

void RS_ActionDrawEllipseInscribe::onMouseMoveEvent(int status, LC_MouseEvent* e) {
    for (RS_AtomicEntity* const pc : m_actionData->lines) { // highlight already selected
        highlightSelected(pc);
    }

    RS_Entity* en = catchModifiableAndDescribe(e, RS2::EntityLine);
    // bool shouldIgnore = false;
    if (en != nullptr) {
        auto* line = dynamic_cast<RS_Line*>(en);
        bool uniqueLine = true;
        for (int i = 0; i < getStatus(); ++i) { //do not pull in the same line again
            if (en->getId() == m_actionData->lines[i]->getId()) {
                uniqueLine = false;
                break;
            };
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
                    if (line != m_actionData->lines[SetLine1] && line != m_actionData->lines[SetLine2] && line !=
                        m_actionData->lines[SetLine3]) {
                        //                        clearLines(true);
                        std::vector<RS_Vector> tangent;
                        tangent.reserve(4);
                        if (preparePreview(line, tangent)) {
                            highlightHover(line);
                            auto ellipse = previewToCreateEllipse(m_actionData->eData);
                            if (m_showRefEntitiesOnPreview) {
                                RS_Vector ellipseCenter = ellipse->getCenter();

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

bool RS_ActionDrawEllipseInscribe::preparePreview(RS_Line* fourthLineCandidate, std::vector<RS_Vector>& tangent) {
    m_actionData->valid = false;
    m_actionData->lines.push_back(fourthLineCandidate);
    RS_Ellipse e{m_preview.get(), RS_EllipseData()};
    m_actionData->valid = e.createInscribeQuadrilateral(m_actionData->lines, tangent);
    if (m_actionData->valid) {
        m_actionData->eData = e.getData();
        //    } else if (RS_DIALOGFACTORY){
        //        RS_DIALOGFACTORY->commandMessage(tr("Can not determine uniquely an ellipse"));
    }
    m_actionData->lines.pop_back();
    return m_actionData->valid;
}

void RS_ActionDrawEllipseInscribe::onMouseLeftButtonRelease(int status, LC_MouseEvent* e) {
    RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine);

    if (en != nullptr) {
        for (int i = 0; i < getStatus(); ++i) {
            if (en->getId() == m_actionData->lines[i]->getId()){
                return; //do not pull in the same line again
            }
        }
        if (en->getParent()) {
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

void RS_ActionDrawEllipseInscribe::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent* e) {
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

void RS_ActionDrawEllipseInscribe::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Specify the first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRBack(tr("Specify the second line"));
            break;
        case SetLine3:
            updateMouseWidgetTRBack(tr("Specify the third line"));
            break;
        case SetLine4:
            updateMouseWidgetTRBack(tr("Specify the fourth line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawEllipseInscribe::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
