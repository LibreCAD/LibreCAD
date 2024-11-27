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

#ifndef LC_ACTIONREMOVESPLINEPOINTS_H
#define LC_ACTIONREMOVESPLINEPOINTS_H

#include "rs_previewactioninterface.h"
#include "lc_actionsplinemodifybase.h"

class LC_ActionRemoveSplinePoints:public LC_ActionSplineModifyBase{
    Q_OBJECT
public:
    LC_ActionRemoveSplinePoints(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionRemoveSplinePoints() override = default;
    void setStatus(int status) override;
protected:
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
    RS_Entity *createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool direction) override;
    bool mayModifySplineEntity(RS_Entity *e) override;
    void doAfterTrigger() override;
    void onMouseMove(RS_Vector mouse, int status, QMouseEvent *e) override;
};

#endif // LC_ACTIONREMOVESPLINEPOINTS_H
