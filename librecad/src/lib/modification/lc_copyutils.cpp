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
    void doCopyEntityLayer(const RS_Entity* entity, RS_Entity* clone, RS_Graphic* clipboardGraphic);
    void doCopyBlock(RS_Block* block, RS_Graphic* graphic);
    void doCopyInsert(const RS_Insert* insert, RS_Graphic* clipboardGraphic);
    bool pasteLayers(RS_Graphic* source, RS_Graphic* destination);
    bool pasteContainer(RS_Entity* entity, RS_EntityContainer* containerToPaste, QHash<QString, QString> blocksDict,
                        const RS_Vector& insertionPoint, RS_Graphic* destination);
    bool pasteEntity(const RS_Entity* entity, RS_EntityContainer* containerToPaste, RS_Graphic* graphic);

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

    doCopyEntityLayer(e, clone, clipboardGraphic);
    if (isInsert) {
        doCopyInsert(insert, clipboardGraphic);
    }
}

void LC_CopyUtils::doCopyEntityLayer(const RS_Entity* entity, RS_Entity* clone, RS_Graphic* clipboardGraphic) {
    const RS_Layer* originalLayer = entity->getLayer(false);
    if (originalLayer != nullptr) {
        const QString layerName = originalLayer->getName();
        auto layerCopy = clipboardGraphic->findLayer(layerName);
        if (layerCopy == nullptr) {
            layerCopy = originalLayer->clone();
            clipboardGraphic->addLayer(layerCopy);
        }
        // set layer to the layer clone:
        // layer could be null if copy is performed in font file, where block is open. LibreCAD#2110
        clone->setLayer(layerCopy);
    }
}

/**
 * Copies all layers of the given entity to the clipboard.
 */
void LC_CopyUtils::doCopyBlock(RS_Block* block, RS_Graphic* graphic) {
    // add layer(s) of the entity insert can also be into any layer
    // special handling of inserts:
    // insert: add layer(s) of subentities:

    const QString blockName = block->getName();
    const auto existingBlock = graphic->findBlock(blockName);
    if (existingBlock == nullptr) {
        const auto blockClone = static_cast<RS_Block*>(block->clone());
        graphic->addBlock(blockClone);
        for (const auto clone : *blockClone) {
            doCopyEntityLayer(clone, clone, graphic);
        }
        for (const auto e2 : *block) {
            const bool entityIsInsert = e2->rtti() == RS2::EntityInsert;
            if (entityIsInsert) {
                const auto insert = static_cast<RS_Insert*>(e2);
                doCopyInsert(insert, graphic);
            }
        }
    }
}

void LC_CopyUtils::doCopyInsert(const RS_Insert* insert, RS_Graphic* clipboardGraphic) {
    RS_Block* block = insert->getBlockForInsert();
    if (block == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyLayers: could not find block for insert entity ");
        // fixme - sand diagnostic?
    }
    else {
        doCopyBlock(block, clipboardGraphic);
    }
}

/**
 * Pastes all entities from the clipboard into the container.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param data Paste data.
 * @param graphic
 * @param ctx
 */
void LC_CopyUtils::paste(const RS_PasteData& data, RS_Graphic* graphic, LC_DocumentModificationBatch& ctx) {
    RS_Graphic* src = RS_CLIPBOARD->getGraphic();
    Q_ASSERT(src != nullptr);

    // FIXME - SAND - COMPLETE PASTE FUNCTIONALITY

    if (src == graphic) {
        // copy within the same graphics
        // Scale (units)
        const auto scaleV = RS_Vector(data.factor, data.factor);

        const RS_Vector zero(0, 0);
        // here we iterate over direct children only, to ensure that containers (like polyline) are not exploded.
        for (const RS_Entity* e : *src) {
            if (e == nullptr || e->isDeleted()) {
                continue;
            }
            RS_Entity* clone = e->clone();
            // **Symmetric**: scale/rot **around center** → move
            clone->scale(zero, scaleV);
            clone->rotate(zero, data.angle);
            clone->move(data.insertionPoint);
            // fixme - should we call update???? polyline, dimension
            ctx += clone;

            // FIXME - SUPPORT: copy to layers (absent, locked), dim style (if absent), inserts blocks. What if layer to copy is locked????
            // FIXME - so far, only simplest minimalistic case is supported!!!
            // FIXME - ensure that paste data contains complete snapshots (with layers, dimstyles all all externals for entities)
        }
    }
    else {
        const RS_Vector scaleV = getInterGraphicsScaleFactor(data.factor, src, graphic);
        src->calculateBorders();
        // const RS_Vector center = (src->getMin() + src->getMax()) * 0.5;
        // FIXME - JUST A TEMPORARY IMPLEMENTATION SO FAR!!! PASTE WITH LAYERS AND OTHER EXTERNALS!!!
        const RS_Vector zero(0, 0);
        for (const RS_Entity* e : *src) {
            // we iterate over children only, to ensure that containers (like polyline) are not exploded.
            if (e == nullptr || e->isDeleted()) {
                continue;
            }
            RS_Entity* clone = e->clone();
            // **Symmetric**: scale/rot **around center** → move
            clone->scale(zero, scaleV);
            clone->rotate(zero, data.angle);
            clone->move(data.insertionPoint);
            // fixme - should we call update???? polyline, dimension
            ctx += clone;

            // FIXME - SUPPORT: copy to layers (absent, locked), dim style (if absent), inserts blocks
            // FIXME - so far, only simplest minimalistic case is supported!!!
            // FIXME - ensure that paste data contains complete snapshots (with layers, dimstyles all all externals for entities)
        }
    }
    graphic->updateInserts();
}

/**
 * Create layers in destination graphic corresponding to entity to be copied
 *
 **/
bool LC_CopyUtils::pasteLayers(RS_Graphic* source, RS_Graphic* destination) {
    if (source == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteLayers: no valid graphic found");
        return false;
    }

    RS_LayerList* lrs = source->getLayerList();
    for (const RS_Layer* layer : *lrs) {
        if (layer == nullptr) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::pasteLayers: nullptr layer in source");
            continue;
        }

        // add layers if absent
        QString ln = layer->getName();
        if (destination->findLayer(ln) == nullptr) {
            destination->addLayer(layer->clone());
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteLayers: layer added: %s", ln.toLatin1().data());
        }
    }
    return true;
}

RS_Block* addNewBlock(const QString& name, RS_Graphic& graphic) {
    const auto db = RS_BlockData(name, {0.0, 0.0}, false);
    const auto b = new RS_Block(&graphic, db);
    b->reparent(&graphic);
    graphic.addBlock(b);
    return b;
}

/**
 * Create inserts and blocks in destination graphic corresponding to entity to be copied
 *
 **/
bool LC_CopyUtils::pasteContainer(RS_Entity* entity, RS_EntityContainer* containerToPaste, QHash<QString, QString> blocksDict,
                                  const RS_Vector& insertionPoint, RS_Graphic* destination) {
    auto* insert = static_cast<RS_Insert*>(entity);

    // get block for this insert object
    const RS_Block* insertBlock = insert->getBlockForInsert();
    if (insertBlock == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: no block to process");
        return false;
    }
    // get name for this insert object
    const QString name_old = insertBlock->getName();
    QString name_new = name_old;
    if (name_old != insert->getName()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: block and insert names don't coincide");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: processing container: %s", name_old.toLatin1().data());
    // rename if needed
    if (destination->findBlock(name_old) != nullptr) {
        if (insertBlock->getParent() == destination) {
            // If block is already in graphic, only paste a new insert
            pasteEntity(entity, destination, destination);
            return true;
        }
        name_new = destination->getBlockList()->newName(name_old);
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: new block name: %s", name_new.toLatin1().data());
    }
    blocksDict[name_old] = name_new;
    // make new block in the destination
    RS_Block* blockClone = addNewBlock(name_new, *destination);
    // create insert for the new block
    const auto di = RS_InsertData(name_new, insertionPoint, RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0));
    auto* insertClone = new RS_Insert(containerToPaste, di);
    insertClone->reparent(containerToPaste);
    containerToPaste->addEntity(insertClone);

    // set the same layer in clone as in source
    const QString layerName = entity->getLayer()->getName();
    RS_Layer* layer = destination->getLayerList()->find(layerName);
    if (layer == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in: %s",
                        layerName.toLatin1().data());
        return false;
    }
    insertClone->setLayer(layer);
    /*
    insertClone->setPen(entity->getPen(false));
    */
    // fixme - sand - merge - review this change to double-check that it is actually needed - potentially, it may break copying of non-blocks!
    // Set pen to ByLayer so that inner entities with ByBlock
    // resolve to the insert's layer color (fixes #2522)
    insertClone->setPen(RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer));


    // get relative insertion point
    RS_Vector ip{0.0, 0.0};
    if (containerToPaste->getId() != destination->getId()) {
        ip = blockClone->getBasePoint();
    }

    // copy content of block/insert to destination
    for (auto* e : *insert) {
        if (e == nullptr) {
            RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Modification::pasteInsert: nullptr entity in block");
            continue;
        }
        if (e->rtti() == RS2::EntityInsert) {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: process sub-insert for %s",
                            static_cast<RS_Insert*>(e)->getName().toLatin1().data());
            if (!pasteContainer(e, blockClone, blocksDict, ip, destination)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity to sub-insert");
                return false;
            }
        }
        else {
            if (!pasteEntity(e, blockClone, destination)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity");
                return false;
            }
        }
    }

    insertClone->update();
    insertClone->clearSelectionFlag();

    return true;
}

/**
 * Paste entity in supplied container
 *
 **/
bool LC_CopyUtils::pasteEntity(const RS_Entity* entity, RS_EntityContainer* containerToPaste, RS_Graphic* graphic) {
    if (entity == nullptr) {
        return false;
    }

    // set the same layer in clone as in source
    const QString ln = entity->getLayer()->getName();
    RS_Layer* layer = graphic->getLayerList()->find(ln); // fixme - perf- layer search is not needed if copy paste within the same document
    if (layer == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in: %s", ln.toLatin1().data());
        return false;
    }


    RS_Entity* clone = entity->clone();
    clone->setLayer(layer);
    clone->setPen(entity->getPen(false));

    // scaling entity doesn't needed as it scaled with insert object
    // paste entity
    clone->reparent(containerToPaste);
    clone->clearSelectionFlag();
    containerToPaste->addEntity(clone);

    return true;
}
