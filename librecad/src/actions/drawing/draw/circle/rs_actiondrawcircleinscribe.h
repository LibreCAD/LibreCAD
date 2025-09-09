/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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

#ifndef RS_ACTIONDRAWCIRCLEINSCRIBE_H
#define RS_ACTIONDRAWCIRCLEINSCRIBE_H

#include "lc_actiondrawcirclebase.h"

class RS_Line;
struct RS_CircleData;

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleInscribe:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleInscribe(LC_ActionContext *actionContext);
    ~RS_ActionDrawCircleInscribe() override;
    void init(int status) override;
    void finish(bool updateTB) override;
    void drawSnapper() override;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetLine1  = InitialActionStatus,   //  Setting the First Line.  */
        SetLine2,   //  Setting the Second Line.  */
        SetLine3   //  Setting the Third Line.  */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    bool m_valid = false;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    bool preparePreview(RS_Line *en);
    /**
	 * @brief clearLines unset highlighten lines, and clear the vector "lines"
	 * @param checkStatus keep lines members according to getStatus()
	 */
    void clearLines(bool checkStatus = false);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void setLine1(RS_Line* line);
    void updateMouseButtonHints() override;
    void doTrigger() override;
};
#endif
