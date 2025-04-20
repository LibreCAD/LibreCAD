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
    for (int i = 0; i < RS2::OverlayGraphics::LAST; i++){
        overlays[i] = nullptr;
    }
    for (int i = 0; i < RS2::OverlayGraphics::LAST; i++){
        entities[i] = nullptr;
    }
}

LC_OverlaysManager::~LC_OverlaysManager() {
    for (auto & e : overlays){
        delete e;
    }
    for (auto & e : entities){
        delete e;
    }
}

void LC_OverlaysManager::setGraphic(RS_Graphic *g) {
    graphic  = g;
}

/**
 * utility class - it's necessary for proper drawing of entities (such as RS_Point) in overlay
 * which require Graphic for their drawing.
 * todo - potentially, for usage in preview and overlay, it's better to have separate point entity that will not require variables and will not depend on settings - and so will use own drawing?
 */
class OverlayEntityContainer:public RS_EntityContainer {
public:
    explicit OverlayEntityContainer(RS_Graphic *g):RS_EntityContainer(nullptr) {
        graphic = g;
    }

    RS_Graphic *getGraphic() const override {
        return graphic;
    }

    RS_Graphic *graphic;
};


RS_EntityContainer *LC_OverlaysManager::getEntitiesContainer(RS2::OverlayGraphics position) {
    RS_EntityContainer* result = entities[position];
    if (result != nullptr) {
        return result;
    }
    if (position == RS2::OverlayGraphics::OverlayEffects) {
        entities[position] = new OverlayEntityContainer(graphic); // fixme - check why graphics is needed there... .for ref entities and accessing variables?
    } else {
        entities[position] = new RS_EntityContainer(nullptr);
    }
    if (position == RS2::OverlayEffects) {
        entities[position]->setOwner(true);
    }
    return entities[position];
}

LC_OverlayDrawablesContainer *LC_OverlaysManager::getDrawablesContainer(RS2::OverlayGraphics position) {
    LC_OverlayDrawablesContainer* container = overlays[position];
    if (container == nullptr) {
        container = new LC_OverlayDrawablesContainer();
        overlays[position] = container;
    }
    return container;
}

RS_EntityContainer *LC_OverlaysManager::entitiesAt(RS2::OverlayGraphics position) {
    return entities[position];
}

void LC_OverlaysManager::addOverlay(LC_OverlayDrawable *ent, RS2::OverlayGraphics position) {
    LC_OverlayDrawablesContainer* container = overlays[position];
    if (container == nullptr) {
        container = new LC_OverlayDrawablesContainer();
        overlays[position] = container;
    }
    container->add(ent);
}

LC_OverlayDrawablesContainer *LC_OverlaysManager::drawablesAt(RS2::OverlayGraphics position) {
    return overlays[position];
}
