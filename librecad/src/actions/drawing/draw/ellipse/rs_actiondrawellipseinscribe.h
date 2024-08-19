/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef RS_ACTIONDRAWELLIPSEINSCRIBE_H
#define RS_ACTIONDRAWELLIPSEINSCRIBE_H

#include "lc_actiondrawcirclebase.h"

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawEllipseInscribe:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawEllipseInscribe(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawEllipseInscribe() override;
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    QStringList getAvailableCommands() override;
    void finish(bool updateTB) override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetLine1,   //  Setting the First Line.  */
        SetLine2,   //  Setting the Second Line.  */
        SetLine3,   //  Setting the Third Line.  */
        SetLine4   //  Setting the Last Line.  */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;
    RS2::CursorType doGetMouseCursor(int status) override;
    // 4 points on ellipse
    bool preparePreview(RS_Line* fourthLineCandidate, std::vector<RS_Vector> &tangent);
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void clearLines(bool checkStatus = false);
    void updateMouseButtonHints() override;
};
#endif
