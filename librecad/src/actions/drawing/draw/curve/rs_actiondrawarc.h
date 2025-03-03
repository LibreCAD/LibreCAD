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

#ifndef RS_ACTIONDRAWARC_H
#define RS_ACTIONDRAWARC_H

#include "rs_previewactioninterface.h"
#include "lc_actiondrawcirclebase.h"

struct RS_ArcData;

/**
 * This action class can handle user events to draw 
 * simple arcs with the center, radius, start- and endangle given.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawArc:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawArc(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawArc() override;
    void reset() override;
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    QStringList getAvailableCommands() override;
    bool isReversed() const override;
    void setReversed(bool r) const override;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetCenter,       /**< Setting the center point. */
        SetRadius,       /**< Setting the radius. */
        SetAngle1,       /**< Setting the startpoint.  */
        SetAngle2,       /**< Setting the endpoint. */
        SetIncAngle,     /**< Setting the included angle. */
        SetChordLength   /**< Setting carc chord length. */
    };

    /**
     * Arc data defined so far.
     */
    std::unique_ptr<RS_ArcData> data;
    void snapMouseToDiameter(RS_Vector &mouse, RS_Vector &arcStart, RS_Vector &halfCircleArcEnd) const;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};
#endif
