// lc_ActionDrawHyperbolaFP.h
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

#ifndef LC_ACTIONDRAWHYPERBOLAFP_H
#define LC_ACTIONDRAWHYPERBOLAFP_H

#include "rs_previewactioninterface.h"

class LC_Hyperbola;

/**
 * Draw hyperbola by two foci and two points on one branch
 * The two points become start/end points of the drawn arc
 */
class LC_ActionDrawHyperbolaFP : public RS_PreviewActionInterface {
  Q_OBJECT
public:
  /**
   * Action States.
   */
  enum Status {
    SetFocus1,      // Setting first focus
    SetFocus2,      // Setting second focus
    SetStartPoint,  // Setting first point on branch (start point)
    SetEndPoint     // Setting second point on branch (end point)
  };

  LC_ActionDrawHyperbolaFP(LC_ActionContext* actionContext);
  ~LC_ActionDrawHyperbolaFP() override = default;

  void init(int status) override;
  void doTrigger() override;

  void onMouseLeftButtonRelease(int status, LC_MouseEvent* e) override;
  void onMouseMoveEvent(int status, LC_MouseEvent* event) override;
  void onMouseRightButtonRelease(int status, LC_MouseEvent* e) override;

  void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;

  void updateMouseButtonHints() override;
  RS2::CursorType doGetMouseCursor(int status) override;
  // Added declaration
  void reset();

private:
  RS_Vector focus1{}, focus2{};
  RS_Vector startPoint{}, endPoint{};

  void preparePreview();
  void createHyperbola();
};

#endif // LC_ACTIONDRAWHYPERBOLAFP_H
