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

#include <QAction>
#include <QList>
#include <QMouseEvent>

#include "rs_actionmodifyround.h"

#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_arc.h"

namespace{
// supported entity types for fillet
EntityTypeList eType = {{RS2::EntityLine,
                         RS2::EntityPolyline,
                         RS2::EntityArc,
                         RS2::EntityCircle,
                         RS2::EntityEllipse,
                         RS2::EntitySpline}};

// Whether the point is on an endPoint of the entity
bool atEndPoint(RS_Entity &entity, const RS_Vector &point)
{
    double distance = 1.;
    entity.getNearestEndpoint(point, &distance);
    return distance < RS_TOLERANCE;
}

// Whether the arc meets the entity tangentially at the point, as a fillet does
bool isTangentAt(RS_Entity &entity, const RS_Arc &arc, const RS_Vector &point)
{
    constexpr double tolerance = 1e-3;
    RS_Vector radial = (point - arc.getCenter()).normalized();
    switch (entity.rtti()) {
    case RS2::EntityLine: {
        RS_Vector direction = (entity.getEndpoint() - entity.getStartpoint()).normalized();
        return std::abs(RS_Vector::dotP(radial, direction)) < tolerance;
    }
    case RS2::EntityArc:
    case RS2::EntityCircle: {
        // tangent circles touch on the line through both centres
        RS_Vector toCenter = (point - entity.getCenter()).normalized();
        return std::abs(radial.x * toCenter.y - radial.y * toCenter.x) < tolerance;
    }
    default:
        // no reliable test for ellipses and splines: never take an arc for their fillet
        return false;
    }
}
}

struct RS_ActionModifyRound::Points {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_RoundData data{};
};

RS_ActionModifyRound::RS_ActionModifyRound(RS_EntityContainer& container,
                                           RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Round Entities",
                               container, graphicView)
    , pPoints(std::make_unique<Points>())
    ,lastStatus(SetEntity1)
{
    setActionType(RS2::ActionModifyRound);
}

RS_ActionModifyRound::~RS_ActionModifyRound() = default;

void RS_ActionModifyRound::init(int status) {
    RS_ActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionModifyRound::finish(bool updateTB)
{
    unhighlightEntity();
    RS_PreviewActionInterface::finish(updateTB);
}

/*
    Whether the entity is the old fillet between the two selected entities: a
    visible, unlocked arc running from an endpoint of one entity to an endpoint
    of the other, tangent to both. The tangency test keeps arcs that merely join
    the two entities, such as the arc of a pie slice, from being taken for one.

    - by Melwyn Francis Carlo.
*/
bool RS_ActionModifyRound::isOldFillet(RS_Entity* e) const
{
    if (e == nullptr || e->rtti() != RS2::EntityArc || entity1 == nullptr || entity2 == nullptr)
        return false;
    if (e->isUndone() || !e->isVisible() || e->isLocked())
        return false;

    const auto* arc = static_cast<RS_Arc*>(e);
    const RS_Vector start = arc->getStartpoint();
    const RS_Vector end = arc->getEndpoint();
    auto joins = [arc, &start, &end](RS_Entity& atStart, RS_Entity& atEnd) {
        return atEndPoint(atStart, start) && atEndPoint(atEnd, end)
               && isTangentAt(atStart, *arc, start) && isTangentAt(atEnd, *arc, end);
    };
    return joins(*entity1, *entity2) || joins(*entity2, *entity1);
}


void RS_ActionModifyRound::trigger() {

    RS_DEBUG->print("RS_ActionModifyRound::trigger()");

    if (entity1 && entity1->isAtomic() &&
            entity2 && entity2->isAtomic()) {

        unhighlightEntity();
        deletePreview();

        // A previous fillet between two free entities is replaced by the new one.
        // Arcs inside a polyline are left alone: removing one here would change the
        // polyline outside the undo step round() creates for it, so Undo could not
        // bring it back.
        RS_Entity* oldFillet = nullptr;
        bool inPolyline = entity1->getParent() != nullptr
                && entity1->getParent()->rtti() == RS2::EntityPolyline;
        if (!inPolyline) {
            for (auto* e : graphicView->getContainer()->getEntityList()) {
                if (e != entity1 && e != entity2 && isOldFillet(e)) {
                    oldFillet = e;
                    break;
                }
            }
        }

        // The old fillet arc is still held by the undo cycle of the fillet that
        // created it, so it must be undone, never deleted, in the same step as the
        // new fillet. round() refuses undone or locked entities before it opens its
        // own undo cycle; check the same here before opening the enclosing one,
        // because opening a cycle discards the redo history, which would free an
        // entity undone after it was picked.
        bool replaceOldFillet = oldFillet != nullptr && document != nullptr
                && entity1->isVisible() && !entity1->isLocked()
                && entity2->isVisible() && !entity2->isLocked();
        if (replaceOldFillet)
            document->startUndoCycle();
        RS_Modification m(*container, graphicView);
        bool rounded = m.round(pPoints->coord2,
                               pPoints->coord1,
                               (RS_AtomicEntity*)entity1,
                               pPoints->coord2,
                               (RS_AtomicEntity*)entity2,
                               pPoints->data);
        if (rounded && replaceOldFillet) {
            graphicView->deleteEntity(oldFillet);
            oldFillet->setUndoState(true);
            document->addUndoable(oldFillet);
        }
        if (replaceOldFillet)
            document->endUndoCycle();

        //coord = RS_Vector(false);
        pPoints->coord1 = RS_Vector(false);
        entity1 = nullptr;
        pPoints->coord2 = RS_Vector(false);
        entity2 = nullptr;
        setStatus(SetEntity1);

        RS_DIALOGFACTORY->updateSelectionWidget(
                    container->countSelected(),
                    container->totalSelectedLength());
    }
}



void RS_ActionModifyRound::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyRound::mouseMoveEvent begin");

    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
    RS_Entity* se = catchEntity(e, eType, RS2::ResolveAllButTextImage);

    switch (getStatus()) {
    case SetEntity1:
    if (RS_Information::isTrimmable(se)) {
        entity1 = se;
        pPoints->coord1 = mouse;
    }
    break;

    case SetEntity2:
    if (entity1 != entity2 && entity2 != nullptr && entity2->isHighlighted())
        graphicView->drawEntityHighlighted(entity2, false);

    if (entity1 != se && RS_Information::isTrimmable(se) && se->isAtomic()) {
        entity2 = se;
        pPoints->coord2 = mouse;
        graphicView->drawEntityHighlighted(entity2, true);

        deletePreview();
        RS_Entity* tmp1 = entity1->clone();
        RS_Entity* tmp2 = entity2->clone();
        tmp1->reparent(preview.get());
        tmp2->reparent(preview.get());
        preview->addEntity(tmp1);
        preview->addEntity(tmp2);

        bool trim = pPoints->data.trim;
        pPoints->data.trim = false;
        RS_Modification m(*preview, nullptr, false);
        m.round(pPoints->coord2,
                pPoints->coord1,
                (RS_AtomicEntity*)tmp1,
                pPoints->coord2,
                (RS_AtomicEntity*)tmp2,
                pPoints->data);
        pPoints->data.trim = trim;

        preview->removeEntity(tmp1);
        preview->removeEntity(tmp2);
        drawPreview();
    }
    break;

    default:
    break;
    }

    RS_DEBUG->print("RS_ActionModifyRound::mouseMoveEvent end");
}



void RS_ActionModifyRound::mouseReleaseEvent(QMouseEvent* e) {
    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
    RS_Entity* se = catchEntity(e, eType, RS2::ResolveAll);

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity1:
        graphicView->drawEntityHighlighted(entity1, false);

        entity1 = se;
        pPoints->coord1 = mouse;
        if (entity1 && entity1->isAtomic() &&
                RS_Information::isTrimmable(entity1)) {
            graphicView->drawEntityHighlighted(entity1, true);
            setStatus(SetEntity2);
        }
        break;

        case SetEntity2:
        entity2 = se;
        pPoints->coord2 = mouse;
        if (entity2 && entity2->isAtomic() &&
                RS_Information::isTrimmable(entity1, entity2)) {
            //setStatus(ChooseRounding);
            trigger();
        }
        break;

        default:
        break;
        }
    } else if (e->button()==Qt::RightButton) {
        unhighlightEntity();
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyRound::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(
                    msgAvailableCommands() +
                    getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
    if (checkCommand("radius", c)) {
        e->accept();
        deletePreview();
        lastStatus = (Status)getStatus();
        setStatus(SetRadius);
    } else if (checkCommand("trim", c)) {
        e->accept();
        deletePreview();
        lastStatus = (Status)getStatus();
        setStatus(SetTrim);
        pPoints->data.trim = !pPoints->data.trim;
        RS_DIALOGFACTORY->requestOptions(this, true, true);
    } else {
        bool ok;
        double r = RS_Math::eval(c, &ok);
        if(ok && r > 1.0e-10) {
            e->accept();
            pPoints->data.radius = r;
        } else
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
    break;

    case SetRadius: {
        bool ok;
        double r = RS_Math::eval(c, &ok);
        if (ok) {
            e->accept();
            pPoints->data.radius = r;
        } else
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
    break;

    /*case SetTrim: {
        if (c==cmdYes.lower() || c==cmdYes2) {
        data->trim = true;
    } else if (c==cmdNo.lower() || c==cmdNo2) {
        data->trim = false;
                } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Please enter 'Yes' "
               "or 'No'"));
                }
                RS_DIALOGFACTORY->requestOptions(this, true, true);
                setStatus(lastStatus);
            }
            break;*/

    default:
    break;
    }
}



QStringList RS_ActionModifyRound::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
    cmd += command("radius");
    cmd += command("trim");
    break;
    default:
    break;
    }
    return cmd;
}


void RS_ActionModifyRound::setRadius(double r) {
    pPoints->data.radius = r;
}

double RS_ActionModifyRound::getRadius() const{
    return pPoints->data.radius;
}

void RS_ActionModifyRound::setTrim(bool t) {
    pPoints->data.trim = t;
}

bool RS_ActionModifyRound::isTrimOn() const{
    return pPoints->data.trim;
}

void RS_ActionModifyRound::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyRound::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionModifyRound::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity1:
    RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify first entity or enter radius <%1>").arg(getRadius()),
                tr("Back"));
    break;
    case SetEntity2:
    RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second entity"),
                                        tr("Back"));
    break;
    case SetRadius:
    RS_DIALOGFACTORY->updateMouseWidget(tr("Enter radius:"),
                                        tr("Cancel"));
    break;
    /*case SetTrim:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Trim on? (yes/no):"),
                                                    "");
                break;*/
    default:
    RS_DIALOGFACTORY->updateMouseWidget();
    break;
    }
}



void RS_ActionModifyRound::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

void RS_ActionModifyRound::unhighlightEntity()
{
    graphicView->drawEntityHighlighted(entity1, false);
    graphicView->drawEntityHighlighted(entity2, false);
}
// EOF
