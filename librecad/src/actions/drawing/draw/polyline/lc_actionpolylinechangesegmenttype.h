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

#ifndef LC_ACTIONPOLYLINECHANGESEGMENTTYPE_H
#define LC_ACTIONPOLYLINECHANGESEGMENTTYPE_H

#include "rs_entitycontainer.h"
#include "rs_previewactioninterface.h"
#include "rs_polyline.h"

class LC_ActionPolylineChangeSegmentType:public RS_PreviewActionInterface {
 Q_OBJECT
public:
    LC_ActionPolylineChangeSegmentType(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionPolylineChangeSegmentType() override;
    void mouseMoveEvent(QMouseEvent *event) override;
protected:
    enum State{
        SetEntity,
        SetSegment,
        SetArcPoint
    };

    RS_Polyline* polyline;
    RS_Entity* polylineSegment;
    RS_Vector arcPoint;

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    RS_Polyline* createModifiedPolyline();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void doTrigger() override;
};

#endif // LC_ACTIONPOLYLINECHANGESEGMENTTYPE_H
