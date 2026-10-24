/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEFINALINPUT_H
#define LC_OFFSETCURVEFINALINPUT_H

#include <unordered_map>
#include <vector>
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurve.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

/** Native staged-input closure only; not a generic undo/publication guard. */
namespace LC_OffsetCurveFinalInput {
struct Input {
    const LC_OffsetCurve* address;
    const LC_OffsetCurveEditSnapshot* snapshot;
    bool requireUnlocked = true;
    bool requireOwner = true;
    bool matches = false;
};

// No virtual getGraphic()/rtti()/point/length reads. Resolve only raw parent
// and layer storage, validating a document layer pointer before dereference.
inline bool available(const RS_Entity& entity, bool requireUnlocked, bool layerVisibility = true) {
    if (!entity.getFlag(RS2::FlagVisible) || entity.getFlag(RS2::FlagDeleted)) return false;
    RS_Layer* layer = nullptr;
    const RS_Graphic* graphic = nullptr;
    const RS_Entity* ancestor = &entity;
    for (unsigned depth = 0; ancestor && depth < 64; ++depth, ancestor = ancestor->getParent()) {
        if (!layer) layer = ancestor->getLayer(false);
        graphic = dynamic_cast<const RS_Graphic*>(ancestor);
        if (graphic) break;
    }
    if (ancestor && !graphic) return false; // Malformed parent chain/cycle.
    if (layer && !graphic) return false; // No document registry can vouch for this raw pointer.
    if (graphic && layer) {
        auto* layers = const_cast<RS_Graphic*>(graphic)->RS_Graphic::getLayerList();
        if (!layers->contains(layer)) layer = nullptr;
    }
    return !layer || ((!layerVisibility || !layer->isFrozen()) && (!requireUnlocked || !layer->isLocked()));
}

/** Call AFTER observed getters, notifications and private-list preparation.
 * The caller's lifetime/context sentinel must still vouch for document.
 * One document membership walk plus one bounded stored pass per input; no
 * target-by-target document scans and no external callbacks during the seal.
 * Individual results preserve existing per-target refusal where supported.
 */
inline bool validate(const RS_Document& document, std::vector<Input>& inputs) {
    std::unordered_map<const RS_Entity*, std::size_t> missing;
    missing.reserve(inputs.size());
    // Block::isVisible deliberately ignores the block's own layer freezing.
    bool valid = document.isOwner() && available(document, true, !dynamic_cast<const RS_Block*>(&document));
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        inputs[i].matches = false;
        if (!inputs[i].address || !inputs[i].snapshot || !missing.emplace(inputs[i].address, i).second)
            valid = false;
    }
    if (!valid) return false;
    for (const auto* member : document) {
        const auto found = missing.find(member);
        if (found == missing.end()) continue;
        inputs[found->second].matches = dynamic_cast<const LC_OffsetCurve*>(member) == inputs[found->second].address;
        missing.erase(found);
        if (missing.empty()) break;
    }
    valid = missing.empty();
    for (auto& input : inputs) {
        // Membership precedes every dereference of a retained source pointer.
        input.matches = input.matches && input.address->getParent() == &document &&
            (!input.requireOwner || input.address->isOwner()) &&
            available(*input.address, input.requireUnlocked) &&
            input.snapshot->matchesStoredState(*input.address);
        valid = input.matches && valid;
    }
    return valid;
}
}
#endif
