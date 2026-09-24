/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_copyutils.h"

#include "lc_linemath.h"
#include "rs_block.h"
#include "rs_clipboard.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_modification.h"
#include "rs_units.h"

namespace LC_CopyUtils {
    void doCopyEntity(RS_Entity* e, const RS_Vector& ref, RS_Graphic* clipboardGraphic);
    void doCopyInsert(const RS_Insert* insert, RS_Graphic* graphic, unsigned provenance);

    RS_PasteData::RS_PasteData(const RS_Vector& insertionPoint, const double factor, const double angle) : insertionPoint{insertionPoint},
        factor{factor}, angle{angle} {
    }
}

/**
 * @brief getPasteScale - find scaling factor for pasting
 * @param userFactor user provided factor
 * @param source - source graphic. If source is nullptr, the graphic on the clipboard is used instead
 * @param destination the target graphic
 * @return
 */
RS_Vector LC_CopyUtils::getInterGraphicsScaleFactor(const double userFactor, const RS_Graphic* source, const RS_Graphic* destination) {
    Q_ASSERT(source != nullptr && destination != nullptr);

    // adjust scaling factor for units conversion in case of clipboard paste
    double factor = LC_LineMath::isMeaningful(userFactor) ? userFactor : 1.0;

    // graphics from the clipboard need to be scaled. From the part lib not:
    const RS2::Unit sourceUnit = source->getUnit();
    const RS2::Unit destinationUnit = destination->getUnit();
    if (sourceUnit != destinationUnit) {
        factor = RS_Units::convert(factor, sourceUnit, destinationUnit);
    }
    // scale factor as vector
    return {factor, factor};
}

/**
 * Copies all selected entities from the given container to the clipboard.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param ref Reference point. The entities will be moved by -ref.
 * @param entities
 * @param graphic
 */
void LC_CopyUtils::copy(const RS_Vector& ref, QList<RS_Entity*>& entities, const RS_Graphic* graphic) {
    Q_ASSERT(!entities.empty());

    const auto clipboard = RS_Clipboard::instance();
    clipboard->clear();

    RS_Graphic* clipboardGraphic = clipboard->getGraphic();
    if (clipboardGraphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_CopyUtils::copy: clipboard has no graphic");
        return;
    }
    if (graphic != nullptr) {
        clipboardGraphic->setUnit(graphic->getUnit());
    }
    else {
        clipboardGraphic->setUnit(RS2::None); // fixme - sand - why? For block we may use parent graphic....
    }

    RS_Vector refPoint;
    if (ref.valid) {
        refPoint = ref;
    }
    else {
        // no ref-point set, determine center of selection
        const RS_BoundData bound = RS_Modification::getBoundingRect(entities);
        refPoint = bound.getCenter();
    }

    clipboard->setSourceId(graphic != nullptr ? graphic->getId() : 0);
    clipboard->startCopy();
    for (const auto e : std::as_const(entities)) {
        if (e != nullptr) {
            doCopyEntity(e, refPoint, clipboardGraphic);
        }
    }
    clipboard->endCopy();
}

/**
 * Copies the given entity from the given container to the clipboard.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param e The entity.
 * @param ref Reference point. The entities will be moved by -ref.
 * @param clipboardGraphic
 */
void LC_CopyUtils::doCopyEntity(RS_Entity* e, const RS_Vector& ref, RS_Graphic* clipboardGraphic) {
    if (!e->isSelected()) {
        // fixme - redundant check...
        return;
    }

    const bool isInsert = e->rtti() == RS2::EntityInsert;
    RS_Insert* insert = nullptr;
    // Ensure the insert is updated before copying to populate the container with transformed entities
    if (isInsert) {
        // fixme - what about dimensions??
        insert = static_cast<RS_Insert*>(e);
        insert->update();
    }

    // add entity to clipboard:
    RS_Entity* clone = e->clone();
    clone->move(-ref);

    // issue #1616: copy&paste a rotated block results in a double rotated block
    // At this point the copied block entities are already rotated, but at
    // pasting, RS_Insert::update() would still rotate the entities again and
    // cause double rotation.

    const double angle = isInsert ? insert->getAngle() : 0.;
    // issue #1616: A quick fix: rotate back all block entities in the clipboard back by the
    // rotation angle before pasting
    if (isInsert && std::abs(std::remainder(angle, 2. * M_PI)) > RS_TOLERANCE_ANGLE) {
        auto* insertClone = static_cast<RS_Insert*>(clone);
        //insert->rotate(insert->getData().insertionPoint, - angle);
        insertClone->setAngle(0.);
    }

    clipboardGraphic->addEntity(clone);
    clone->reparent(clipboardGraphic);

    doCopyEntityLayer(clone, clipboardGraphic);
    if (isInsert) {
        doCopyInsert(insert, clipboardGraphic, 0);
    }
}

void LC_CopyUtils::doCopyEntityLayer(RS_Entity* entity, RS_Graphic* graphic) {
    // layer could be null if copy is performed in font file, where block is open. LibreCAD#2110
    const RS_Layer* layer = entity->getLayer(false);
    if (layer != nullptr) {
        RS_Layer* ownLayer = graphic->findLayer(layer->getName());
        if (ownLayer == nullptr) {
            ownLayer = layer->clone();
            graphic->addLayer(ownLayer);
        }
        entity->setLayer(ownLayer);
    }
    if (entity->isContainer() && static_cast<RS_EntityContainer*>(entity)->isOwner()) {
        for (RS_Entity* child : *static_cast<RS_EntityContainer*>(entity)) {
            if (child != nullptr) {
                doCopyEntityLayer(child, graphic);
            }
        }
    }
}

void LC_CopyUtils::doCopyBlock(const RS_Block* block, RS_Graphic* graphic, const unsigned provenance) {
    if (graphic->findBlock(block->getName()) != nullptr) {
        return; // the graphic's own definition wins
    }
    auto* blockClone = static_cast<RS_Block*>(block->clone());
    blockClone->reparent(graphic); // not the source's: that graphic may close first
    blockClone->clearDwgProvenance(provenance);
    graphic->addBlock(blockClone);
    for (RS_Entity* e : *blockClone) {
        doCopyEntityLayer(e, graphic);
    }
    for (const RS_Entity* e : *block) {
        if (e != nullptr && e->rtti() == RS2::EntityInsert) {
            doCopyInsert(static_cast<const RS_Insert*>(e), graphic, provenance);
        }
    }
}

void LC_CopyUtils::doCopyInsert(const RS_Insert* insert, RS_Graphic* graphic, const unsigned provenance) {
    const RS_Block* block = insert->getBlockForInsert();
    if (block == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_CopyUtils::doCopyInsert: could not find block '%s'",
                        insert->getName().toLatin1().data());
    }
    else {
        doCopyBlock(block, graphic, provenance);
    }
}

/**
 * Pastes all entities from the clipboard into the graphic, with the blocks
 * they insert and the layers they are on: a block or layer the graphic already
 * has of the same name is used as it is, missing ones are copied. The pasted
 * entities are new to the drawing, and keep the handles of the drawing's
 * tables only if they were copied from this drawing.
 *
 * @param data Paste data.
 * @param graphic
 * @param ctx
 */
void LC_CopyUtils::paste(const RS_PasteData& data, RS_Graphic* graphic, LC_DocumentModificationBatch& ctx) {
    const RS_Clipboard* clipboard = RS_CLIPBOARD;
    RS_Graphic* src = clipboard->getGraphic();
    Q_ASSERT(src != nullptr);

    const unsigned foreignTables = clipboard->getSourceId() == graphic->getId() ? 0 : RS_Entity::TableRefs;
    const RS_Vector scaleV = getInterGraphicsScaleFactor(data.factor, src, graphic);
    const RS_Vector zero(0, 0);
    // here we iterate over direct children only, to ensure that containers (like polyline) are not exploded.
    for (const RS_Entity* e : *src) {
        if (e == nullptr || e->isDeleted()) {
            continue;
        }
        if (e->rtti() == RS2::EntityInsert) {
            doCopyInsert(static_cast<const RS_Insert*>(e), graphic, RS_Entity::Identity | foreignTables);
        }
        RS_Entity* clone = e->clone();
        // **Symmetric**: scale/rot **around center** → move
        clone->scale(zero, scaleV);
        clone->rotate(zero, data.angle);
        clone->move(data.insertionPoint);
        doCopyEntityLayer(clone, graphic);
        // the undo section clears the identity: paste deletes nothing it could take over
        clone->clearDwgProvenance(foreignTables);
        ctx += clone;
    }
    graphic->updateInserts();
}
