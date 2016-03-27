
/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

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

#ifndef RS_ACTIONDRAWLINEORTHTAN_H
#define RS_ACTIONDRAWLINEORTHTAN_H

#include "rs_previewactioninterface.h"

class RS_Line;

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
class RS_ActionDrawLineOrthTan : public RS_PreviewActionInterface {
        Q_OBJECT
private:
    enum Status {
        SetLine,     /**< Choose the line orthogonal to the tangent line */
		SetCircle    /**< Choose the arc/circle/ellipse to create its tangent line*/
    };

public:
    RS_ActionDrawLineOrthTan(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);

	void trigger() override;
	void finish(bool updateTB = true) override;

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

private:
	void clearLines();
    /** normal to tangent. */
    RS_Line* normal; // the select normal line
    /** tangent. */
    RS_Line* tangent; //holds the tangent line for preview
    /** arc/circle/ellipse to generate tangent */
    RS_Entity* circle;
};

#endif
