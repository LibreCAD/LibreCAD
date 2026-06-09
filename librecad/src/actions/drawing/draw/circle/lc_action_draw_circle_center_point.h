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

#ifndef RS_ACTIONDRAWCIRCLE_H
#define RS_ACTIONDRAWCIRCLE_H

#include "lc_action_draw_circle_base.h"

struct RS_CircleData;

/**
 * This action class can handle user events to draw 
 * circles with a given center and a point on the circle line.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawCircleCenterPoint : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    explicit LC_ActionDrawCircleCenterPoint(LC_ActionContext* actionContext);
    ~LC_ActionDrawCircleCenterPoint() override;
    void reset() override;

protected:
    /**
 * Action States.
 */
    enum Status {
        SetCenter = InitialActionStatus, /**< Setting the center point. */
        SetRadius /**< Setting the radius. */
    };

    /**
     * Circle data defined so far.
     */
    std::unique_ptr<RS_CircleData> m_circleData;
    bool doProcessCommand(int status, const QString& command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& coord) override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
