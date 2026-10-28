/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_offsetcurvepublication.h"

#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_offsetcurve.h"
#include "lc_undoablerelzero.h"
#include "rs_document.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"
#include "rs_undocycle.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <set>
#include <utility>

namespace {
bool pointEqual(const RS_Vector& a, const RS_Vector& b) noexcept {
    return a.valid == b.valid && a.x == b.x && a.y == b.y && a.z == b.z;
}
}

struct LC_OffsetCurvePublication::Data final : LC_SelectedSetListener {
    struct State {
        RS_Entity* entity;
        RS_EntityContainer* parent;
        RS_Layer* layer;
        unsigned flags;
        RS_Vector low, high;
        bool registeredLayer, frozenLayer, lockedLayer;
        QString blockName;
        RS_Block* block = nullptr;
        bool frozenBlock = false;
    };
    RS_Document* document;
    LC_GraphicViewport* viewport;
    RS_Graphic* graphic = nullptr;
    std::uint64_t undoEpoch;
    QList<RS_Entity*> before, entities, selectionBefore, selection;
    std::vector<std::shared_ptr<RS_UndoCycle>> history;
    std::vector<State> states;
    std::vector<std::pair<RS_Entity*, bool>> selectionFlags;
    QList<RS_Entity*> removed, added;
    std::vector<std::unique_ptr<RS_Entity>> retired;
    std::vector<RS_Entity*> retireAddresses;
    std::set<RS_Entity*> retireNodes;
    std::unique_ptr<RS_Undoable> relativeZero;
    RS_Vector markedZero, zero, low{false}, high{false};
    bool committed = false, notified = false, parentChainValid = true;

    Data(RS_Document& doc, LC_GraphicViewport& port)
        : document(&doc), viewport(&port), undoEpoch(doc.undoStateEpoch()),
          markedZero(port.getMarkedRelativeZero()), zero(port.getRelativeZero()) {
        doc.getSelection()->addListener(this);
        auto* parent = static_cast<RS_Entity*>(&doc);
        for (unsigned depth = 0; parent && depth < 64; ++depth, parent = parent->getParent()) {
            graphic = dynamic_cast<RS_Graphic*>(parent);
            if (graphic) break;
        }
        parentChainValid = !parent || graphic;
    }
    void selectionChanged() override {}
    void selectedSetDestroyed() override { document = nullptr; }

    bool current() const noexcept {
        if (!document || committed || document->m_refCount || document->m_currentCycle ||
            document->undoStateEpoch() != undoEpoch || document->m_entities != before ||
            document->m_selectedSet->m_entitiesList != selectionBefore ||
            !pointEqual(viewport->getMarkedRelativeZero(), markedZero) ||
            !pointEqual(viewport->getRelativeZero(), zero)) return false;
        for (const auto& state : states) {
            const auto& entity = *state.entity;
            if (entity.getParent() != state.parent || entity.getLayer(false) != state.layer ||
                entity.getFlags() != state.flags || !pointEqual(entity.getMin(), state.low) ||
                !pointEqual(entity.getMax(), state.high)) return false;
            const bool registered = graphic && state.layer &&
                graphic->RS_Graphic::getLayerList()->contains(state.layer);
            if (registered != state.registeredLayer || (registered &&
                (state.layer->isFrozen() != state.frozenLayer || state.layer->isLocked() != state.lockedLayer)))
                return false;
            if (!state.blockName.isEmpty()) {
                auto* block = graphic ? graphic->RS_Graphic::getBlockList()->find(state.blockName) : nullptr;
                if (block != state.block || (block && block->isFrozen() != state.frozenBlock)) return false;
            }
        }
        return true;
    }
};

LC_OffsetCurvePublication::LC_OffsetCurvePublication(std::unique_ptr<Data> data) : m_data(std::move(data)) {}
LC_OffsetCurvePublication::~LC_OffsetCurvePublication() = default;

std::unique_ptr<LC_OffsetCurvePublication> LC_OffsetCurvePublication::prepare(RS_Document& document,
    LC_GraphicViewport& viewport, const QList<RS_Entity*>& removed,
    const QList<RS_Entity*>& added, const QList<RS_Entity*>& selected) {
    if (removed.isEmpty() || removed.size() != added.size() || !document.isOwner() ||
        document.m_refCount || document.m_currentCycle ||
        document.undoStateEpoch() == std::numeric_limits<std::uint64_t>::max()) return {};
    auto data = std::make_unique<Data>(document, viewport);
    if (!data->parentChainValid) return {};
    data->before = document.m_entities;
    data->selectionBefore = document.m_selectedSet->m_entitiesList;
    data->removed = removed;
    data->added = added;
    if (added.size() > std::numeric_limits<qsizetype>::max() - data->before.size()) return {};
    std::set<RS_Entity*> originals, replacements, selectedReplacements;
    auto cycle = std::make_shared<RS_UndoCycle>();
    for (qsizetype i = 0; i < removed.size(); ++i) {
        if (!data->before.contains(removed[i]) || data->before.contains(added[i])) return {};
        auto* original = dynamic_cast<LC_OffsetCurve*>(removed[i]);
        auto* replacement = dynamic_cast<LC_OffsetCurve*>(added[i]);
        if (!original || !replacement || original == replacement || original->getParent() != &document ||
            !originals.insert(original).second || !replacements.insert(replacement).second ||
            original->isDeleted() || replacement->isDeleted() ||
            (replacement->getParent() && replacement->getParent() != &document)) return {};
        cycle->addUndoable(original); cycle->addUndoable(replacement);
    }
    for (auto* entity : selected)
        if (!replacements.count(entity) || !selectedReplacements.insert(entity).second) return {};
    for (auto* entity : selected) {
        const bool locked = entity->isLocked();
        if (!data->document || document.m_entities != data->before) return {};
        if (locked) selectedReplacements.erase(entity);
    }
    if (LC_LineMath::isMeaningfulDistance(data->markedZero, data->zero)) {
        data->relativeZero = std::make_unique<LC_UndoableRelZero>(&viewport, data->markedZero, data->zero);
        cycle->addUndoable(data->relativeZero.get());
    }

    // Build the new history, including its set nodes and vector capacity, before
    // touching flags, ownership, selection or the redo tail's entities.
    const auto undoCount = std::size_t(document.m_redoPointer - document.m_undoList.cbegin());
    data->history.reserve(undoCount + 1);
    data->history.insert(data->history.end(), document.m_undoList.cbegin(), document.m_redoPointer);
    data->history.push_back(std::move(cycle));
    std::set<RS_Undoable*> keep, obsolete;
    for (const auto& entry : data->history)
        keep.insert(entry->getUndoables().begin(), entry->getUndoables().end());
    for (auto it = document.m_redoPointer; it != document.m_undoList.cend(); ++it)
        for (auto* entity : (*it)->getUndoables()) if (!keep.count(entity)) obsolete.insert(entity);

    data->entities.reserve(data->before.size() + added.size());
    for (auto* entity : std::as_const(data->before)) {
        if (entity && obsolete.count(entity) && entity->isDeleted()) data->retireAddresses.push_back(entity);
        else data->entities.append(entity);
    }
    data->entities.append(added);
    data->retired.resize(data->retireAddresses.size());
    auto pendingRetirement = data->retireAddresses;
    for (std::size_t i = 0; i < pendingRetirement.size(); ++i) {
        auto* entity = pendingRetirement[i];
        if (!entity || dynamic_cast<RS_Document*>(entity) || !data->retireNodes.insert(entity).second) return {};
        const auto* container = dynamic_cast<RS_EntityContainer*>(entity);
        if (container && container->isOwner())
            for (auto* child : *container) if (child) pendingRetirement.push_back(child);
    }
    std::vector<RS_Entity*> replacedNodes;
    std::size_t replacedCount = std::size_t(removed.size());
    for (auto* root : removed) {
        const auto count = static_cast<LC_OffsetCurve*>(root)->size();
        if (count > replacedNodes.max_size() - replacedCount) return {};
        replacedCount += count;
    }
    replacedNodes.reserve(replacedCount);
    for (auto* root : removed) {
        replacedNodes.push_back(root);
        for (auto* child : *static_cast<LC_OffsetCurve*>(root)) if (child) replacedNodes.push_back(child);
    }
    std::sort(replacedNodes.begin(), replacedNodes.end(), std::less<RS_Entity*>{});
    if (std::adjacent_find(replacedNodes.begin(), replacedNodes.end()) != replacedNodes.end()) return {};
    data->selection.reserve(data->selectionBefore.size() + selected.size());
    for (auto* entity : std::as_const(data->selectionBefore))
        if (!std::binary_search(replacedNodes.begin(), replacedNodes.end(), entity, std::less<RS_Entity*>{}) &&
            !data->retireNodes.count(entity))
            data->selection.append(entity);
    for (auto* entity : selected) if (selectedReplacements.count(entity)) data->selection.append(entity);

    // Value seals and stored bounds are prepared before the caller's final seal.
    // The commit below does not invoke visibility or geometry virtual getters.
    for (const auto* list : {&data->before, &data->added}) for (auto* entity : std::as_const(*list)) {
        if (!entity) continue;
        auto* layer = entity->getLayer(false);
        const bool registered = data->graphic && layer && data->graphic->RS_Graphic::getLayerList()->contains(layer);
        Data::State state{entity, entity->getParent(), layer, entity->getFlags(), entity->getMin(), entity->getMax(),
            registered, registered && layer->isFrozen(), registered && layer->isLocked(), {}};
        if (const auto* insert = dynamic_cast<const RS_Insert*>(entity)) {
            state.blockName = insert->getData().name;
            state.block = data->graphic ? data->graphic->RS_Graphic::getBlockList()->find(state.blockName) : nullptr;
            state.frozenBlock = state.block && state.block->isFrozen();
        }
        data->states.push_back(std::move(state));
        const bool visible = entity->isVisible();
        if (!data->document || document.m_entities != data->before) return {};
        if (!originals.count(entity) && !obsolete.count(entity) && visible && entity->hasValidBorders()) {
            data->low = data->low.valid ? RS_Vector::minimum(data->low, entity->getMin()) : entity->getMin();
            data->high = data->high.valid ? RS_Vector::maximum(data->high, entity->getMax()) : entity->getMax();
        }
    }
    for (qsizetype i = 0; i < removed.size(); ++i) {
        for (auto* root : {removed[i], added[i]}) {
            const bool select = root == added[i] && selectedReplacements.count(root);
            data->selectionFlags.emplace_back(root, select);
            for (auto* child : *static_cast<LC_OffsetCurve*>(root)) {
                if (!child || !dynamic_cast<RS_Line*>(child) || child->getParent() != root) return {};
                if (child->getFlag(RS2::FlagVisible)) data->selectionFlags.emplace_back(child, select);
            }
        }
    }
    return std::unique_ptr<LC_OffsetCurvePublication>(new LC_OffsetCurvePublication(std::move(data)));
}

bool LC_OffsetCurvePublication::commit() noexcept {
    auto& data = *m_data;
    if (!data.current()) return false;
    auto& document = *data.document;
    for (auto* original : std::as_const(data.removed)) original->setFlag(RS2::FlagDeleted);
    for (const auto& item : data.selectionFlags) item.first->RS_Entity::setSelectionFlag(item.second);
    for (auto* entity : std::as_const(data.added)) entity->setParent(&document);
    document.m_entities.swap(data.entities);
    document.m_entIdx = -1; document.m_subContainer = nullptr;
    document.m_minV = data.low; document.m_maxV = data.high;
    document.m_selectedSet->m_entitiesList.swap(data.selection);
    document.m_undoList.swap(data.history);
    document.m_redoPointer = document.m_undoList.cend();
    ++document.m_undoStateEpoch;
    document.m_modified = true;
    if (data.relativeZero) { data.relativeZero.release(); data.viewport->markRelativeZero(); }
    for (auto* entity : data.retireNodes) entity->RS_Entity::setSelectionFlag(false);
    for (std::size_t i = 0; i < data.retireAddresses.size(); ++i) {
        auto* entity = data.retireAddresses[i];
        entity->setParent(nullptr); entity->RS_Entity::setSelectionFlag(false);
        data.retired[i].reset(entity);
    }
    data.committed = true;
    return true;
}

void LC_OffsetCurvePublication::notify() noexcept {
    auto& data = *m_data;
    if (!data.committed || std::exchange(data.notified, true)) return;
    data.retired.clear();
    // Notifications are postcommit. A throwing observer cannot turn an already
    // installed, undoable replacement into a reported preparation failure.
    try { if (data.document) data.document->setModified(true); } catch (...) {}
    try { if (data.document) data.document->updateUndoState(); } catch (...) {}
    try { if (data.document) data.document->getSelection()->fireSelectionChanged(); } catch (...) {}
}
