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

#ifndef RS_ACTIONSELECTCONTOURS_H
#define RS_ACTIONSELECTCONTOURS_H

#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to select contours.
 *
 * @author Andrew Mustun
 */
 // fixme - check whether this action is actually used
class RS_ActionSelectContour:public RS_PreviewActionInterface {
Q_OBJECT
public:
    RS_ActionSelectContour(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *event) override;
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
private:
    RS_Entity *en = nullptr;
};

#endif
