/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef RS_ACTIONDRAWCIRCLE3P_H
#define RS_ACTIONDRAWCIRCLE3P_H

#include "lc_action_draw_circle_base.h"

struct RS_CircleData;

/**
 * This action class can handle user events to draw 
 * circles with three points given.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawCircle3Points : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    explicit LC_ActionDrawCircle3Points(LC_ActionContext* actionContext);
    ~LC_ActionDrawCircle3Points() override;

protected:
    /**
 * Action States.
 */
    enum Status {
        SetPoint1 = InitialActionStatus, /**< Setting the 1st point. */
        SetPoint2, /**< Setting the 2nd point. */
        SetPoint3 /**< Setting the 3rd point. */
    };

    /**
     * Circle data defined so far.
     */
    struct Points;
    std::unique_ptr<Points> m_actionData;

    void reset() override;
    void preparePreview() const;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& coord) override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
