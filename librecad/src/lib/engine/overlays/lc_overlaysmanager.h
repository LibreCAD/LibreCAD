/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_OVERLAYSMANAGER_H
#define LC_OVERLAYSMANAGER_H

#include <array>

#include "rs.h"

class LC_OverlayDrawable;
class RS_Graphic;
class RS_EntityContainer;
class LC_OverlayDrawablesContainer;

class LC_OverlaysManager{
public:
    LC_OverlaysManager();
    virtual ~LC_OverlaysManager();
    RS_EntityContainer* getEntitiesContainer(RS2::OverlayGraphics position);
    LC_OverlayDrawablesContainer* getDrawablesContainer(RS2::OverlayGraphics position);
    RS_EntityContainer* entitiesAt(RS2::OverlayGraphics position) const;
    LC_OverlayDrawablesContainer* drawablesAt(RS2::OverlayGraphics position) const;
    void addOverlay(LC_OverlayDrawable* ent, RS2::OverlayGraphics position);
    void setGraphic(RS_Graphic* g);
protected:
    RS_Graphic* m_graphic = nullptr;
    std::array<RS_EntityContainer*,RS2::OverlayGraphics::LAST> m_entities;
    std::array<LC_OverlayDrawablesContainer*,RS2::OverlayGraphics::LAST> m_overlays;
};

#endif
