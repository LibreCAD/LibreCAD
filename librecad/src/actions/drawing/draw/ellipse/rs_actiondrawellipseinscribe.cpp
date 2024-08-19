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
#include "rs_dialogfactory.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
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
RS_ActionDrawEllipseInscribe::RS_ActionDrawEllipseInscribe(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :LC_ActionDrawCircleBase("Draw ellipse inscribed",
                           container, graphicView)
    , pPoints(std::make_unique<Points>())
{
	actionType=RS2::ActionDrawEllipseInscribe;
}

RS_ActionDrawEllipseInscribe::~RS_ActionDrawEllipseInscribe() = default;

void RS_ActionDrawEllipseInscribe::clearLines(bool checkStatus){
    while (pPoints->lines.size()) {
        if (checkStatus && (int) pPoints->lines.size() <= getStatus())
            break;
        pPoints->lines.back()->setHighlighted(false);
        graphicView->drawEntity(pPoints->lines.back());
        pPoints->lines.pop_back();
    }
}

void RS_ActionDrawEllipseInscribe::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_Snapper::suspend();
    }
    clearLines(true);
}

void RS_ActionDrawEllipseInscribe::finish(bool updateTB){
    clearLines(false);
    LC_ActionDrawCircleBase::finish(updateTB);
}

void RS_ActionDrawEllipseInscribe::trigger(){
    LC_ActionDrawCircleBase::trigger();

    auto *ellipse = new RS_Ellipse(container, pPoints->eData);

    deletePreview();
    container->addEntity(ellipse);

    addToDocumentUndoable(ellipse);

    for (RS_Line *const p: pPoints->lines) {
        if (!p) continue;
        p->setHighlighted(false);
        graphicView->drawEntity(p);

    }
    drawSnapper();

    if (moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(ellipse->getCenter());
    }

    clearLines(false);
    setStatus(SetLine1);

    RS_DEBUG->print("RS_ActionDrawEllipse4Line::trigger():"
                    " entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseInscribe::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawEllipse4Line::mouseMoveEvent begin");

    snapPoint(e);
    deleteHighlights();
    deletePreview();
    int status = getStatus();

    for(RS_AtomicEntity* const pc: pPoints->lines) { // highlight already selected
        highlightSelected(pc);
    }

    RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);
   // bool shouldIgnore = false;
    if (en != nullptr){
        auto *line = dynamic_cast<RS_Line *>(en);
        bool uniqueLine = true;
        for (int i = 0; i < getStatus(); ++i) { //do not pull in the same line again
            if (en->getId() == pPoints->lines[i]->getId()){
                uniqueLine = false;
                break;
            };
        }
        if (uniqueLine){
            switch (status) {
                case SetLine1: {
                    highlightHover(line);
                    break;
                }
                case SetLine2: {
                    if (line != pPoints->lines[SetLine1]){
                        highlightHover(line);
                    }
                    break;
                }
                case SetLine3: {
                    if (line != pPoints->lines[SetLine1] && line != pPoints->lines[SetLine2]){
                        highlightHover(line);
                    }
                    break;
                }
                case SetLine4: {
                    if (line != pPoints->lines[SetLine1] && line != pPoints->lines[SetLine2] && line != pPoints->lines[SetLine3]){
//                        clearLines(true);
                        std::vector<RS_Vector> tangent;
                        tangent.reserve(4);
                        if (preparePreview(line, tangent)){
                            highlightHover(line);
                            auto ellipse = previewEllipse(pPoints->eData);
                            if (showRefEntitiesOnPreview) {
                                RS_Vector ellipseCenter = ellipse->getCenter();

                                for (const auto &i: tangent) {
                                    previewRefPoint(ellipseCenter + i);
                                }

                                previewRefPoint(ellipseCenter);
                            }
                        } else {
                            // nothing, can't build the ellipse
                        }
                    }
                    drawPreview();
                    break;
                }
                default:
                    break;
            }
        }

    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDrawEllipse4Line::mouseMoveEvent end");
}

bool RS_ActionDrawEllipseInscribe::preparePreview(RS_Line* fourthLineCandidate, std::vector<RS_Vector> &tangent){
    pPoints->valid = false;
    pPoints->lines.push_back(fourthLineCandidate);
    RS_Ellipse e{preview.get(), RS_EllipseData()};
    pPoints->valid = e.createInscribeQuadrilateral(pPoints->lines, tangent);
    if (pPoints->valid){
        pPoints->eData = e.getData();
//    } else if (RS_DIALOGFACTORY){
//        RS_DIALOGFACTORY->commandMessage(tr("Can not determine uniquely an ellipse"));
    }
    pPoints->lines.pop_back();
    return pPoints->valid;
}

void RS_ActionDrawEllipseInscribe::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);

    if (en != nullptr){
        for (int i = 0; i < getStatus(); ++i) {
            if (en->getId() == pPoints->lines[i]->getId()) return; //do not pull in the same line again
        }
        if (en->getParent()){
            if (en->getParent()->ignoredOnModification()) return;
        }
        clearLines(true);

        auto *line = dynamic_cast<RS_Line *>(en);
        switch (status) {
            case SetLine1:
            case SetLine2:
            case SetLine3: {
                pPoints->lines.push_back(line);
                setStatus(getStatus() + 1);
                break;
            }
            case SetLine4: {
                std::vector<RS_Vector> tangent;
                tangent.reserve(4);
                if (preparePreview(line, tangent)){
                    pPoints->lines.push_back(line);
                    trigger();
                } else {
                        commandMessage(tr("Can not determine uniquely an ellipse"));
                }
                break;
            }
            default:
                break;
        }
    }
}

void RS_ActionDrawEllipseInscribe::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    // Return to last status:
    if (status > 0){
        clearLines(true);
//            pPoints->lines.back()->setHighlighted(false);
//            graphicView->drawEntity(pPoints->lines.back());
        pPoints->lines.pop_back();
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

void RS_ActionDrawEllipseInscribe::updateMouseButtonHints(){
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

RS2::CursorType RS_ActionDrawEllipseInscribe::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
