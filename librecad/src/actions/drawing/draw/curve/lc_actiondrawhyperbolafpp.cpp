/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD.org
** Copyright (C) 2025 Your Name <your.email@example.com>
**
** License: GPL-2.0-or-later
**
******************************************************************************/

#include "lc_actiondrawhyperbolafpp.h"
#include "lc_hyperbola.h"
#include "rs.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_dialogfactory.h"
#include "rs_preview.h"

LC_ActionDrawHyperbolaFPP::LC_ActionDrawHyperbolaFPP(LC_ActionContext* actionContext)
    : RS_PreviewActionInterface("Draw Hyperbola by Foci and Two Points",
                                actionContext, RS2::ActionDrawHyperbolaFPP)
{
}

void LC_ActionDrawHyperbolaFPP::init(int status) {
    RS_PreviewActionInterface::init(status);
    focus1 = focus2 = startPoint = endPoint = RS_Vector(false);
}

void LC_ActionDrawHyperbolaFPP::trigger() {
    if (focus1.valid && focus2.valid && startPoint.valid && endPoint.valid) {
        createHyperbola();
    }
    init(SetFocus1);
}

void LC_ActionDrawHyperbolaFPP::createHyperbola() {
    // Use LC_Hyperbola constructor from two foci and one point — then extend to second point
    LC_Hyperbola temp(nullptr, {focus1, focus2, startPoint});
    if (!temp.isValid()) {
        commandMessage(tr("Invalid foci or point — cannot create hyperbola"));
        return;
    }
    temp.setAngle1(temp.getParamFromPoint(startPoint, false));

    // Determine which branch the start point belongs to
    RS_VectorSolutions foci = temp.getFoci();
    double d1 = startPoint.distanceTo(foci.get(0));
    double d2 = startPoint.distanceTo(foci.get(1));
    bool branchReversed = (d1 - d2 < 0);

    // Create final hyperbola using first point, then set angles based on second point
    LC_Hyperbola* hyperbola = new LC_Hyperbola(m_document, LC_HyperbolaData(focus1, focus2, startPoint));
    if (!hyperbola->isValid()) {
        delete hyperbola;
        return;
    }

    //hyperbola->setReversed(branchReversed);

    // Compute parametric angles for start and end points
    double phi1 = hyperbola->getParamFromPoint(startPoint, branchReversed);
    double phi2 = hyperbola->getParamFromPoint(endPoint, branchReversed);

    if (std::isnan(phi1) || std::isnan(phi2)) {
        delete hyperbola;
        commandMessage(tr("Points not on hyperbola"));
        return;
    }

    hyperbola->setAngle1(phi1);
    hyperbola->setAngle2(phi2);

    m_document->startUndoCycle();
    m_document->addEntity(hyperbola);
    m_document->endUndoCycle();

    drawPreview();
}

LC_Hyperbola* LC_ActionDrawHyperbolaFPP::preparePreview() {
    deletePreview();

    if (!focus1.valid || !focus2.valid || !startPoint.valid)
        return nullptr;

    LC_Hyperbola* hyperbola = new LC_Hyperbola(m_preview.get(), {focus1, focus2, startPoint});
    if (!hyperbola->isValid()) {
        delete hyperbola;
        return nullptr;
    }

    if (endPoint.valid) {
      bool rev = false; //hyperbola->isReversed();
        double phi1 = hyperbola->getParamFromPoint(startPoint, rev);
        double phi2 = hyperbola->getParamFromPoint(endPoint, rev);
        if (!std::isnan(phi1) && !std::isnan(phi2)) {
            hyperbola->setAngle1(phi1);
            hyperbola->setAngle2(phi2);
        }
    }

    previewEntity(hyperbola);
    return hyperbola;
}

void LC_ActionDrawHyperbolaFPP::onMouseMoveEvent(int status, LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;

    switch (getStatus()) {
    case SetFocus1:
        break;
    case SetFocus2:
        focus2 = mouse;
        break;
    case SetStartPoint:
        startPoint = mouse;
        break;
    case SetEndPoint:
        endPoint = mouse;
        break;
    }

    preparePreview();
}

// lc_actiondrawhyperbolafpp.cpp - implementation of corrected onCoordinateEvent

void LC_ActionDrawHyperbolaFPP::onCoordinateEvent(int status, bool isZero, const RS_Vector& pos)
{
  if (!pos.valid) return;

         // Handle relative zero
  if (isZero) {
    moveRelativeZero(pos);
  }

  switch (status) {
  case SetFocus1:
    focus1 = pos;
    moveRelativeZero(focus1);
    setStatus(SetFocus2);
    break;

  case SetFocus2:
    if (focus1.distanceTo(pos) > RS_TOLERANCE) {
      focus2 = pos;
      moveRelativeZero(focus2);
      setStatus(SetStartPoint);
    }
    break;

  case SetStartPoint:
    startPoint = pos;
    setStatus(SetEndPoint);
    break;

  case SetEndPoint:
    endPoint = pos;
    trigger();
    break;
  }

  updateMouseButtonHints();
  m_graphicView->redraw(RS2::RedrawOverlay);
}

void LC_ActionDrawHyperbolaFPP::onMouseLeftButtonRelease(int status, LC_MouseEvent* e)
{
  // Snap the point according to current snap mode
  RS_Vector snapped = e->snapPoint;
  if (!snapped.valid) return;

         // Forward to coordinate event handler
         // isZero = false (not from command line zero input)
  onCoordinateEvent(status, false, snapped);

         // Redraw overlay (preview, crosshair, etc.)
  m_graphicView->redraw(RS2::RedrawOverlay);
}

void LC_ActionDrawHyperbolaFPP::onMouseRightButtonRelease(int status, LC_MouseEvent* e) {
    deletePreview();
    initPrevious(getStatus());
}

void LC_ActionDrawHyperbolaFPP::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFocus1:
        updateMouseWidgetTRCancel(tr("Specify first focus"));
        break;
    case SetFocus2:
        updateMouseWidgetTRCancel(tr("Specify second focus"));
        break;
    case SetStartPoint:
        updateMouseWidgetTRCancel(tr("Specify start point on branch"));
        break;
    case SetEndPoint:
        updateMouseWidgetTRCancel(tr("Specify end point on branch"));
        break;
    default:
        updateMouseWidget();
        break;
    }
}

RS2::CursorType LC_ActionDrawHyperbolaFPP::doGetMouseCursor(int status) {
    return RS2::CadCursor;
}
