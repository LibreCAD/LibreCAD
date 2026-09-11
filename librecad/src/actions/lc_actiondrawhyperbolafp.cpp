/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

#include <algorithm>
#include <cmath>

#include <QMouseEvent>

#include "lc_actiondrawhyperbolafp.h"

#include "lc_hyperbola.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct LC_ActionDrawHyperbolaFP::Points {
    RS_Vector focus1{false};
    RS_Vector focus2{false};
    RS_Vector startPoint{false};
    RS_Vector endPoint{false};
    LC_HyperbolaData data;
    bool valid = false;
};

LC_ActionDrawHyperbolaFP::LC_ActionDrawHyperbolaFP(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw hyperbola from foci and points", container,
                               graphicView,
                               RS2::ActionDrawHyperbolaFoci2Points)
    , pPoints(std::make_unique<Points>())
{
}

LC_ActionDrawHyperbolaFP::~LC_ActionDrawHyperbolaFP() = default;

void LC_ActionDrawHyperbolaFP::init(int status) {
    RS_PreviewActionInterface::init(status);
    switch (status) {
    case SetFocus1:
        *pPoints = Points{};
        break;
    case SetFocus2:
        pPoints->focus2.valid = false;
        // fall through
    case SetStartPoint:
        pPoints->startPoint.valid = false;
        // fall through
    case SetEndPoint:
        pPoints->endPoint.valid = false;
        break;
    default:
        break;
    }
}

double LC_ActionDrawHyperbolaFP::signedFocalDifference(const RS_Vector& point) const {
    if (!point.valid || !pPoints->focus1.valid || !pPoints->focus2.valid)
        return 0.;
    return point.distanceTo(pPoints->focus1) - point.distanceTo(pPoints->focus2);
}

double LC_ActionDrawHyperbolaFP::minimumSize() const {
    return std::max(RS_TOLERANCE, pPoints->focus1.distanceTo(pPoints->focus2) * 1e-6);
}

bool LC_ActionDrawHyperbolaFP::isValidStartPoint(const RS_Vector& point) const {
    if (!point.valid || !pPoints->focus1.valid || !pPoints->focus2.valid)
        return false;
    // 2a near 0 is the perpendicular bisector, which is not a hyperbola branch at
    // all; 2a near the distance between the foci is the axis outside them, where
    // the branch collapses onto a ray.
    const double twoA = std::abs(signedFocalDifference(point));
    const double separation = pPoints->focus1.distanceTo(pPoints->focus2);
    return twoA >= minimumSize() && separation - twoA >= minimumSize()
        && LC_HyperbolaData{pPoints->focus1, pPoints->focus2, point}.isValid();
}

bool LC_ActionDrawHyperbolaFP::isOnSameBranch(const RS_Vector& point) const {
    if (!point.valid || !pPoints->startPoint.valid)
        return false;
    const double reference = signedFocalDifference(pPoints->startPoint);
    if (std::abs(reference) < minimumSize())
        return false;

    // Only the side matters here. The end point trims the arc - the arc ends
    // where the branch passes nearest to it - so it does not have to lie on the
    // hyperbola, and it never will: a pick 0.01 units off a curve whose foci are
    // 100 apart already moves the focal difference by 0.0098, a hundred times
    // any sane on-curve tolerance. Comparing the magnitude therefore rejected
    // every real mouse pick. The sign of ||PF1| - |PF2|| is what identifies the
    // branch, and that survives a pick anywhere near the curve.
    const double difference = signedFocalDifference(point);
    // Near the perpendicular bisector the sign is arbitrary, so refuse there
    // rather than guess a branch.
    if (std::abs(difference) < minimumSize())
        return false;
    return (difference > 0.0) == (reference > 0.0);
}

RS_Vector LC_ActionDrawHyperbolaFP::endOnBranch(const RS_Vector& point) const {
    if (!point.valid || !pPoints->startPoint.valid)
        return RS_Vector(false);
    const LC_Hyperbola branch{nullptr, LC_HyperbolaData{pPoints->focus1, pPoints->focus2, pPoints->startPoint}};
    if (!branch.isValid())
        return RS_Vector(false);
    // the whole, unbounded branch
    return branch.getNearestPointOnEntity(point, false);
}

void LC_ActionDrawHyperbolaFP::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();
    RS_Vector relativeZero = graphicView->getRelativeZero();
    if (pPoints->valid) {
        auto* en = new LC_Hyperbola{container, pPoints->data};
        container->addEntity(en);
        if (document != nullptr) {
            document->startUndoCycle();
            document->addUndoable(en);
            document->endUndoCycle();
        }
        // at the centre, as the ellipse foci action does
        relativeZero = en->getCenter();
    }
    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(relativeZero);
    drawSnapper();
    setStatus(SetFocus1);
    init(SetFocus1);
}

bool LC_ActionDrawHyperbolaFP::preparePreview(const RS_Vector& mouse) {
    deletePreview();
    pPoints->valid = false;

    if (!pPoints->focus1.valid || !pPoints->focus2.valid)
        return false;

    // The point that defines the branch is the committed start point once we
    // have one, otherwise the point currently under the cursor.
    const RS_Vector& onCurve = pPoints->startPoint.valid ? pPoints->startPoint : mouse;
    if (!isValidStartPoint(onCurve))
        return false;

    LC_HyperbolaData data{pPoints->focus1, pPoints->focus2, onCurve};
    LC_Hyperbola candidate{nullptr, data};
    if (!candidate.isValid())
        return false;

    // LC_HyperbolaData(f0, f1, p) already orients majorP toward the branch the
    // point is on and leaves reversed false, so the flag is the constructor's
    // to set. Deriving it again from the sign of the focal difference mirrors
    // the arc onto the other branch, which is where the picked point is not.
    const bool reversed = candidate.getData().reversed;
    const double phiStart = candidate.getParamFromPoint(onCurve, reversed);
    if (std::isnan(phiStart))
        return false;

    double phi1 = -std::abs(phiStart);
    double phi2 = std::abs(phiStart);
    if (pPoints->startPoint.valid && mouse.valid && isOnSameBranch(mouse)) {
        // The arc ends where the branch passes nearest to the pick. The
        // parameter of the pick itself follows its local y alone, which can put
        // the end far from the click.
        const RS_Vector end = endOnBranch(mouse);
        const double phiEnd = end.valid ? candidate.getParamFromPoint(end, reversed) : std::nan("");
        if (!std::isnan(phiEnd)) {
            phi1 = std::min(phiStart, phiEnd);
            phi2 = std::max(phiStart, phiEnd);
        }
    }

    data.angle1 = phi1;
    data.angle2 = phi2;
    pPoints->data = data;
    pPoints->valid = pPoints->startPoint.valid && pPoints->endPoint.valid;

    auto* preview_entity = new LC_Hyperbola{preview.get(), data};
    preview->addEntity(preview_entity);
    drawPreview();
    return true;
}

void LC_ActionDrawHyperbolaFP::mouseMoveEvent(QMouseEvent* e) {
    const RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetStartPoint:
    case SetEndPoint:
        preparePreview(mouse);
        break;
    default:
        break;
    }
}

void LC_ActionDrawHyperbolaFP::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        RS_CoordinateEvent ce{snapPoint(e)};
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton) {
        deletePreview();
        // RS_ActionInterface::init() treats a negative status as "finished",
        // so stepping below the first status is what ends the action. Clamping
        // here would trap the user inside it.
        init(getStatus() - 1);
        // Relative coordinates typed next start from the last point still
        // committed, as in Parabola4Points.
        switch (getStatus()) {
        case SetFocus2:
            graphicView->moveRelativeZero(pPoints->focus1);
            break;
        case SetStartPoint:
            graphicView->moveRelativeZero(pPoints->focus2);
            break;
        default:
            break;
        }
    }
}

void LC_ActionDrawHyperbolaFP::coordinateEvent(RS_CoordinateEvent* e) {
    if (e == nullptr)
        return;
    const RS_Vector mouse = e->getCoordinate();
    if (!mouse.valid)
        return;

    switch (getStatus()) {
    case SetFocus1:
        pPoints->focus1 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetFocus2);
        break;

    case SetFocus2:
        if (pPoints->focus1.distanceTo(mouse) < RS_TOLERANCE) {
            RS_DIALOGFACTORY->commandMessage(tr("Foci cannot be coincident"));
            return;
        }
        pPoints->focus2 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetStartPoint);
        break;

    case SetStartPoint:
        // The start point fixes 2a and the branch, so it must be a point that
        // admits a hyperbola through these foci that is not degenerate.
        if (!isValidStartPoint(mouse)) {
            RS_DIALOGFACTORY->commandMessage(
                tr("The point does not define a hyperbola with these foci"));
            return;
        }
        pPoints->startPoint = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetEndPoint);
        break;

    case SetEndPoint: {
        if (!isOnSameBranch(mouse)) {
            // Covers both a point off the curve and a point on the opposite
            // branch, which has the opposite signed focal difference.
            RS_DIALOGFACTORY->commandMessage(
                tr("The end point is not on the same hyperbola branch"));
            return;
        }
        // The arc ends where the branch passes nearest to the pick. When that is
        // the start point itself the arc has no length, and at the vertex both
        // of its angles would be 0, which reads as the whole unbounded branch.
        const RS_Vector end = endOnBranch(mouse);
        if (!end.valid || end.distanceTo(pPoints->startPoint) < minimumSize()) {
            RS_DIALOGFACTORY->commandMessage(
                tr("Start and end points cannot be the same"));
            return;
        }
        pPoints->endPoint = mouse;
        if (preparePreview(mouse) && pPoints->valid)
            trigger();
        break;
    }

    default:
        break;
    }
}

void LC_ActionDrawHyperbolaFP::commandEvent(RS_CommandEvent* e) {
    const QString cmd = e->getCommand().toLower();
    if (checkCommand("help", cmd)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", ")
                                         + tr("specify the two foci, then the start and end points on one branch"));
        e->accept();
    }
}

QStringList LC_ActionDrawHyperbolaFP::getAvailableCommands() {
    return {};
}

void LC_ActionDrawHyperbolaFP::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFocus1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first focus"),
                                            tr("Cancel"));
        break;
    case SetFocus2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second focus"),
                                            tr("Back"));
        break;
    case SetStartPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the start point on the branch"),
                                            tr("Back"));
        break;
    case SetEndPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the end point on the same branch"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void LC_ActionDrawHyperbolaFP::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
