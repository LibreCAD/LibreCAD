/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_ACTIONDRAWLINEFREE_H
#define RS_ACTIONDRAWLINEFREE_H

#include "lc_undoabledocumentmodificationaction.h"

class RS_Polyline;

/**
 * This action class can handle user events to draw freehand lines.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawLineFreehand : public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    explicit LC_ActionDrawLineFreehand(LC_ActionContext *actionContext);
    ~LC_ActionDrawLineFreehand() override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetStartpoint = InitialActionStatus,   /**< Setting the startpoint.  */
        Dragging      /**< Setting the endpoint. */
    };
    QPointF m_oldMousePosition = QPointF(-100,-100);
    RS_Vector m_vertex;
    RS_Polyline* m_polyline;

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateActionPrompt() override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void onMouseLeftButtonPress(int status, const LC_MouseEvent* e) override;
};
#endif
