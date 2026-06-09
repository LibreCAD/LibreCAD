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

#ifndef LC_ACTIONDRAWLINEPOLYGON4_H
#define LC_ACTIONDRAWLINEPOLYGON4_H

#include "lc_action_draw_polygon_base.h"

// fixme - sand - add support for vertex/vertex from command line
class LC_ActionDrawLinePolygonSideSide:public LC_ActionDrawPolygonBase {
    Q_OBJECT
public:
    explicit LC_ActionDrawLinePolygonSideSide(LC_ActionContext *actionContext);
    ~LC_ActionDrawLinePolygonSideSide() override;
    void setVertexVertexMode(const bool val){m_useVertexVertexMode = val;}
    bool isVertexVertexMode() const {return m_useVertexVertexMode;}
protected:
    bool m_useVertexVertexMode = false;
    QString getPoint2Hint() const override;
    QString getPoint1Hint() const override;
    void preparePolygonInfo(PolygonInfo &polygonInfo, const RS_Vector &snap) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
};

#endif
