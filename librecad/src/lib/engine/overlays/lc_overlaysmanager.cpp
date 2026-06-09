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

#include "lc_overlaysmanager.h"

#include "lc_overlayentitiescontainer.h"
#include "rs_entitycontainer.h"

LC_OverlaysManager::LC_OverlaysManager() {
    m_overlays.fill(nullptr);
    m_entities.fill(nullptr);
}

LC_OverlaysManager::~LC_OverlaysManager() {
    for (const auto& e : m_overlays) {
        delete e;
    }
    for (const auto& e : m_entities) {
        delete e;
    }
}

void LC_OverlaysManager::setGraphic(RS_Graphic* g) {
    m_graphic = g;
}

/**
 * utility class - it's necessary for proper drawing of entities (such as RS_Point) in overlay
 * which require Graphic for their drawing.
 * todo - potentially, for usage in preview and overlay, it's better to have separate point entity that will not require variables and will not depend on settings - and so will use own drawing?
 */
class OverlayEntityContainer : public RS_EntityContainer {
public:
    explicit OverlayEntityContainer(RS_Graphic* g) : RS_EntityContainer(nullptr, true) {
        graphic = g;
    }

    RS_Graphic* getGraphic() const override {
        return graphic;
    }

    RS_Graphic* graphic;
};

RS_EntityContainer* LC_OverlaysManager::getEntitiesContainer(const RS2::OverlayGraphics position) {
    RS_EntityContainer* result = m_entities[position];
    if (result != nullptr) {
        return result;
    }
    if (position == RS2::OverlayGraphics::OverlayEffects) {
        m_entities[position] = new OverlayEntityContainer(m_graphic);
        // fixme - check why graphics is needed there... .for ref entities and accessing variables?
    }
    else {
        m_entities[position] = new RS_EntityContainer(nullptr);
    }
    if (position == RS2::OverlayEffects) {
        m_entities[position]->setOwner(true);
    }
    return m_entities[position];
}

LC_OverlayDrawablesContainer* LC_OverlaysManager::getDrawablesContainer(const RS2::OverlayGraphics position) {
    LC_OverlayDrawablesContainer* container = m_overlays[position];
    if (container == nullptr) {
        container = new LC_OverlayDrawablesContainer();
        m_overlays[position] = container;
    }
    return container;
}

RS_EntityContainer* LC_OverlaysManager::entitiesAt(const RS2::OverlayGraphics position) const {
    return m_entities[position];
}

void LC_OverlaysManager::addOverlay(LC_OverlayDrawable* ent, const RS2::OverlayGraphics position) {
    LC_OverlayDrawablesContainer* container = m_overlays[position];
    if (container == nullptr) {
        container = new LC_OverlayDrawablesContainer();
        m_overlays[position] = container;
    }
    container->add(ent);
}

LC_OverlayDrawablesContainer* LC_OverlaysManager::drawablesAt(const RS2::OverlayGraphics position) const {
    return m_overlays[position];
}
