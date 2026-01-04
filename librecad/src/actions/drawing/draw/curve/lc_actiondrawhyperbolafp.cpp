// lc_ActionDrawHyperbolaFP.cpp
/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 Dongxu Li (github.com/dxli)

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
******************************************************************************/

#include <cmath>
#include <memory>

#include "lc_actiondrawhyperbolafp.h"
#include "lc_hyperbola.h"
#include "rs_preview.h"
#include "qg_graphicview.h"

LC_ActionDrawHyperbolaFP::LC_ActionDrawHyperbolaFP(LC_ActionContext* actionContext)
    : RS_PreviewActionInterface("Draw Hyperbola by Foci and Two Points",
                                actionContext, RS2::ActionDrawHyperbolaFP)
{
  reset();
}

void LC_ActionDrawHyperbolaFP::init(int status)
{
  RS_PreviewActionInterface::init(status);
  reset();
}

void LC_ActionDrawHyperbolaFP::reset()
{
  focus1 = focus2 = startPoint = endPoint = RS_Vector(false);
  setStatus(SetFocus1);
}

void LC_ActionDrawHyperbolaFP::preparePreview()
{
  deletePreview();

  if (!focus1.valid || !focus2.valid || !startPoint.valid)
    return;

  auto hyperbola = std::make_unique<LC_Hyperbola>(m_preview.get(), LC_HyperbolaData{focus1, focus2, startPoint});
  if (!hyperbola->isValid()) {
    return;
  }

  bool rev = (startPoint.distanceTo(focus1) - startPoint.distanceTo(focus2) < 0.0);

  double phiStart = hyperbola->getParamFromPoint(startPoint, rev);
  if (std::isnan(phiStart)) {
    return;
  }

  double phi1 = phiStart;
  double phi2 = phiStart;

  if (getStatus() == SetStartPoint) {
    phi1 = -std::abs(phiStart);
    phi2 =  std::abs(phiStart);
  } else if (getStatus() == SetEndPoint && endPoint.valid) {
    double phiEnd = hyperbola->getParamFromPoint(endPoint, rev);
    if (!std::isnan(phiEnd)) {
      phi1 = std::min(phiStart, phiEnd);
      phi2 = std::max(phiStart, phiEnd);
    }
  }

  hyperbola->setAngle1(phi1);
  hyperbola->setAngle2(phi2);

  previewEntity(hyperbola.release());
}

// lc_ActionDrawHyperbolaFP.cpp - fixed createHyperbola() using temporary hyperbola

void LC_ActionDrawHyperbolaFP::createHyperbola()
{
  if (!focus1.valid || !focus2.valid || !startPoint.valid || !endPoint.valid) {
    reset();
    return;
  }

  if (focus1.distanceTo(focus2) < RS_TOLERANCE) {
    commandMessage(tr("Foci cannot be coincident"));
    reset();
    return;
  }

  if (startPoint.distanceTo(endPoint) < RS_TOLERANCE) {
    commandMessage(tr("Start and end points cannot be the same"));
    reset();
    return;
  }

         // Temporary hyperbola to access getParamFromPoint()
  LC_Hyperbola temp(nullptr, LC_HyperbolaData(focus1, focus2, startPoint));
  if (!temp.isValid()) {
    commandMessage(tr("Invalid foci or point"));
    reset();
    return;
  }

  //bool rev = (startPoint.distanceTo(focus1) - startPoint.distanceTo(focus2) < 0.0);
  bool rev = false;

  double phiStart = temp.getParamFromPoint(startPoint, rev);
  double phiEnd   = temp.getParamFromPoint(endPoint, rev);

  if (std::isnan(phiStart) || std::isnan(phiEnd)) {
    commandMessage(tr("Points not on hyperbola"));
    reset();
    return;
  }

         // Final data with correct angles
  LC_HyperbolaData data(temp.getData().getFocus1(), temp.getData().getFocus2(), startPoint);
  data.angle1 = std::min(phiStart, phiEnd);
  data.angle2 = std::max(phiStart, phiEnd);
  data.reversed = rev;

  auto hyperbola = std::make_unique<LC_Hyperbola>(m_container, data);
  if (hyperbola != nullptr && hyperbola->isValid()) {
    moveRelativeZero(hyperbola->getCenter());
    hyperbola->calculateBorders();
    hyperbola->setFlag(RS2::FlagVisible);
    setPenAndLayerToActive(hyperbola.get());
    undoCycleAdd(hyperbola.release());
  }

}

void LC_ActionDrawHyperbolaFP::doTrigger()
{
  createHyperbola();
  setStatus(SetFocus1);
}

void LC_ActionDrawHyperbolaFP::onMouseLeftButtonRelease(int status, LC_MouseEvent* e)
{
  RS_Vector snapped = e->snapPoint;
  if (!snapped.valid)
    return;

  onCoordinateEvent(status, false, snapped);
}

void LC_ActionDrawHyperbolaFP::onMouseMoveEvent(int status, LC_MouseEvent* event)
{
  if (!event)
    return;

  RS_Vector mouse = event->snapPoint;

  switch (status) {
  case SetFocus2:
    focus2 = mouse;
    break;
  case SetStartPoint: {
    startPoint = mouse;
    endPoint.valid = false;
  }
    break;
  case SetEndPoint:
    endPoint = mouse;
    break;
  default:
    break;
  }

  preparePreview();
  drawPreview();
}

void LC_ActionDrawHyperbolaFP::onMouseRightButtonRelease(int status, LC_MouseEvent*)
{
  deletePreview();
  initPrevious(status);
}

void LC_ActionDrawHyperbolaFP::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector& pos)
{
  if (!pos.valid)
    return;

  moveRelativeZero(pos);

  switch (status) {
  case SetFocus1:
    focus1 = pos;
    setStatus(SetFocus2);
    break;

  case SetFocus2:
    if (focus1.distanceTo(pos) < RS_TOLERANCE) {
      commandMessage(tr("Foci cannot be coincident"));
      return;
    }
    focus2 = pos;
    setStatus(SetStartPoint);
    break;

  case SetStartPoint:
    startPoint = pos;
    setStatus(SetEndPoint);
    break;

  case SetEndPoint:
    if (startPoint.distanceTo(pos) < RS_TOLERANCE) {
      commandMessage(tr("Start and end points cannot be the same"));
      return;
    }
    endPoint = pos;
    trigger();
    break;
  }

  updateMouseButtonHints();
  m_graphicView->redraw(RS2::RedrawOverlay);
}

void LC_ActionDrawHyperbolaFP::updateMouseButtonHints()
{
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

RS2::CursorType LC_ActionDrawHyperbolaFP::doGetMouseCursor([[maybe_unused]] int status)
{
  return RS2::CadCursor;
}
