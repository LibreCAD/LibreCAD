/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pavel Krejcir (pavel@pamsoft.cz)

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
**********************************************************************/

#ifndef LC_ACTIONDRAWSPLINEPOINTS_H
#define LC_ACTIONDRAWSPLINEPOINTS_H


#include "lc_action_draw_spline.h"

/**
 * This action class can handle user events to draw splines through points.
 *
 * @author Pavel Krejcir
 */
class LC_ActionDrawSplinePoints:public LC_ActionDrawSpline {
    Q_OBJECT
public:
    explicit LC_ActionDrawSplinePoints(LC_ActionContext *actionContext);
    ~LC_ActionDrawSplinePoints() override;
    void reset();
    void init(int status) override;
    QStringList getAvailableCommands() override;
    void setClosed(bool c) override;
    bool isClosed() override;
    void undo() override;
    //using degree=2 only
    void setDegree(int /*deg*/) override{}
protected:
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;

    void redo();
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
