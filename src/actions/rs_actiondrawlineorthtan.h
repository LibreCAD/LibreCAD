/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#ifndef RS_ACTIONDRAWLINEORTHTAN_H
#define RS_ACTIONDRAWLINEORTHTAN_H

#include "rs_previewactioninterface.h"


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
    ~RS_ActionDrawLineOrthTan() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

    virtual void trigger();
	
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

private:
    /** normal to tangent. */
    RS_Line* normal; // the select normal line
    /** tangent. */
    RS_Line* tangent; //holds the tangent line for preview
    /** arc/circle/ellipse to generate tangent */
    RS_Entity* circle;
};

#endif
