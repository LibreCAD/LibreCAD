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

#ifndef LC_ACTIONDRAWHYPERBOLAFP_H
#define LC_ACTIONDRAWHYPERBOLAFP_H

#include <memory>

#include "rs_previewactioninterface.h"

class RS_Vector;

/**
 * Draw one branch of a hyperbola from its two foci and two points on the
 * branch.
 *
 * The foci fix the axis and the centre; the first point fixes the branch and
 * the semi-major axis, because a point on a hyperbola satisfies
 * ||PF1| - |PF2|| = 2a and the sign of that difference says which branch it
 * lies on. The second point only trims the arc: it has to lie on the same side
 * of the foci (see isOnSameBranch()), and the arc ends where the branch passes
 * nearest to it (see endOnBranch()).
 */
class LC_ActionDrawHyperbolaFP : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetFocus1 = 0,  //  Setting the first focus.
        SetFocus2,      //  Setting the second focus.
        SetStartPoint,  //  Setting the start point on the branch.
        SetEndPoint     //  Setting the end point on the same branch.
    };

    LC_ActionDrawHyperbolaFP(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);
    ~LC_ActionDrawHyperbolaFP() override;

    void init(int status = 0) override;

    void trigger() override;
    bool preparePreview(const RS_Vector& mouse);

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    struct Points;
    std::unique_ptr<Points> pPoints;

    /**
     * @brief signedFocalDifference |PF1| - |PF2| for the current foci.
     * Its magnitude is 2a and its sign identifies the branch.
     */
    double signedFocalDifference(const RS_Vector& point) const;

    /**
     * @brief minimumSize lengths below this count as zero. It scales with the
     * distance between the foci, so a 1 mm and a 1 km hyperbola behave the same.
     */
    double minimumSize() const;

    /**
     * @brief isValidStartPoint true when @p point gives a hyperbola with these
     * foci that is neither flat nor needle-thin: 2a stays minimumSize() away from
     * both 0 and the distance between the foci.
     */
    bool isValidStartPoint(const RS_Vector& point) const;

    /**
     * @brief isOnSameBranch true when @p point lies on the start point's side of
     * the perpendicular bisector of the foci, outside a dead zone of
     * minimumSize(). Only the side is compared: the end point does not have to
     * lie on the curve.
     */
    bool isOnSameBranch(const RS_Vector& point) const;

    /**
     * @brief endOnBranch the point of the start point's branch nearest to
     * @p point, where the arc ends
     */
    RS_Vector endOnBranch(const RS_Vector& point) const;
};

#endif
