/****************************************************************************
*
* Actions that is used for selecting entity for which properties should be
* shown in QuickInfo widget
*
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
**********************************************************************/
#ifndef LC_ACTIONINFOPROPERTIES_H
#define LC_ACTIONINFOPROPERTIES_H

#include "rs_previewactioninterface.h"

class LC_ActionInfoProperties:public RS_PreviewActionInterface
{
public:
    LC_ActionInfoProperties(RS_EntityContainer &container, RS_GraphicView &graphicView);
    void mouseMoveEvent(QMouseEvent *event) override;
    void init(int status) override;
private:
    RS_Entity* highlightedEntity = nullptr;
    void updateQuickInfoWidget(RS_Entity *pEntity);
    void clearHighLighting();
    void clearQuickInfoWidget();
    void highlightHoveredEntity(QMouseEvent *event);
    void highlightEntity(RS_Entity *entity);
    void highlightAndShowEntityInfo(QMouseEvent *e);
protected:
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONINFOPROPERTIES_H
