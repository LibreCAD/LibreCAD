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

#ifndef RS_ACTIONDRAWARC3P_H
#define RS_ACTIONDRAWARC3P_H

#include "lc_actiondrawcirclebase.h"

struct RS_ArcData;

/**
 * This action class can handle user events to draw 
 * arcs with three points given.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawArc3P:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawArc3P(LC_ActionContext *actionContext);
    ~RS_ActionDrawArc3P() override;
    void init(int status) override;
    QStringList getAvailableCommands() override;
protected:
    /**
    * Action States.
    */
    enum Status {
        SetPoint1,       /**< Setting the 1st point. */
        SetPoint2,       /**< Setting the 2nd point. */
        SetPoint3        /**< Setting the 3rd point. */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    bool m_alternatedPoints = false;
    void reset() override;
    void preparePreview(bool alternatePoints);
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    void doTrigger() override;
};
#endif
