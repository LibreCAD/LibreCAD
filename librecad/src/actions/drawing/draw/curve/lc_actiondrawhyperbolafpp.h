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

#ifndef LC_ACTIONDRAWHYPERBOLAFPP_H
#define LC_ACTIONDRAWHYPERBOLAFPP_H

#include "rs_previewactioninterface.h"

class LC_Hyperbola;

/**
 * Draw hyperbola by two foci and two points on one branch
 * The two points become start/end points of the drawn arc
 *
 * @author Your Name
 */
class LC_ActionDrawHyperbolaFPP : public RS_PreviewActionInterface {
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

public:
    LC_ActionDrawHyperbolaFPP(LC_ActionContext* actionContext);
    ~LC_ActionDrawHyperbolaFPP() override = default;

    void init(int status) override;
    void trigger() override;

    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void updateMouseButtonHints() override;
    RS2::CursorType doGetMouseCursor(int status) override;

protected:
    RS_Vector focus1{}, focus2{};
    RS_Vector startPoint{}, endPoint{};

    LC_Hyperbola* preparePreview();
    void createHyperbola();
};

#endif // LC_ACTIONDRAWHYPERBOLAFPP_H
