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
                               RS2::ActionDrawHyperbolaFP)
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

bool LC_ActionDrawHyperbolaFP::isOnSameBranch(const RS_Vector& point) const {
    if (!point.valid || !pPoints->startPoint.valid)
        return false;
    const double reference = signedFocalDifference(pPoints->startPoint);
    // 2a must stay away from zero: a point equidistant from both foci lies on
    // the perpendicular bisector, which is not a hyperbola branch at all.
    const double separation = pPoints->focus1.distanceTo(pPoints->focus2);
    if (std::abs(reference) < RS_TOLERANCE || separation < RS_TOLERANCE)
        return false;

    // Only the side matters here. The end point trims the arc - it is
    // projected onto the curve through getParamFromPoint() - so it does not
    // have to lie on the hyperbola, and it never will: a pick 0.01 units off
    // a curve whose foci are 100 apart already moves the focal difference by
    // 0.0098, a hundred times any sane on-curve tolerance. Comparing the
    // magnitude therefore rejected every real mouse pick. The sign of
    // ||PF1| - |PF2|| is what identifies the branch, and that survives a
    // pick anywhere near the curve.
    const double difference = signedFocalDifference(point);
    // Near the perpendicular bisector the sign is arbitrary, so refuse there
    // rather than guess a branch. The dead zone scales with the construction
    // so it behaves the same on a 1 mm and a 1 km hyperbola.
    const double deadZone = std::max(RS_TOLERANCE, separation * 1e-6);
    if (std::abs(difference) < deadZone)
        return false;
    return (difference > 0.0) == (reference > 0.0);
}

void LC_ActionDrawHyperbolaFP::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();
    if (pPoints->valid) {
        auto* en = new LC_Hyperbola{container, pPoints->data};
        container->addEntity(en);
        if (document != nullptr) {
            document->startUndoCycle();
            document->addUndoable(en);
            document->endUndoCycle();
        }
    }
    const RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(rz);
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
    if (!onCurve.valid)
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
        const double phiEnd = candidate.getParamFromPoint(mouse, reversed);
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

    case SetStartPoint: {
        // The start point fixes 2a and the branch, so it must be a point that
        // actually admits a hyperbola through these foci.
        LC_Hyperbola candidate{nullptr, LC_HyperbolaData{pPoints->focus1, pPoints->focus2, mouse}};
        if (!candidate.isValid()) {
            RS_DIALOGFACTORY->commandMessage(
                tr("The point does not define a hyperbola with these foci"));
            return;
        }
        pPoints->startPoint = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetEndPoint);
        break;
    }

    case SetEndPoint:
        if (pPoints->startPoint.distanceTo(mouse) < RS_TOLERANCE) {
            RS_DIALOGFACTORY->commandMessage(
                tr("Start and end points cannot be the same"));
            return;
        }
        if (!isOnSameBranch(mouse)) {
            // Covers both a point off the curve and a point on the opposite
            // branch, which has the opposite signed focal difference.
            RS_DIALOGFACTORY->commandMessage(
                tr("The end point is not on the same hyperbola branch"));
            return;
        }
        pPoints->endPoint = mouse;
        if (preparePreview(mouse) && pPoints->valid)
            trigger();
        break;

    default:
        break;
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
