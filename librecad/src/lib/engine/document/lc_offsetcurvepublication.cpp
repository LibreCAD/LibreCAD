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
        unsigned long long id = 0;
    };
    struct Graph { RS_EntityContainer* owner; QList<RS_Entity*> children; };
    RS_Document* document;
    LC_GraphicViewport* viewport;
    RS_Graphic* graphic = nullptr;
    std::uint64_t undoEpoch;
    QList<RS_Entity*> before, entities, selectionBefore, selection;
    std::vector<std::shared_ptr<RS_UndoCycle>> history;
    std::vector<State> states;
    std::vector<Graph> graphs;
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
        if (graphic && graphic != &doc) graphic->getSelection()->addListener(this);
    }
    void selectionChanged() override {}
    void selectedSetDestroyed() override { document = nullptr; }

    State storedState(RS_Entity& entity) const {
        auto* layer = entity.getLayer(false);
        const bool registered = graphic && layer && graphic->RS_Graphic::getLayerList()->contains(layer);
        State state{&entity, entity.getParent(), layer, entity.getFlags(), entity.getMin(), entity.getMax(),
            registered, registered && layer->isFrozen(), registered && layer->isLocked(), {}};
        state.id = entity.getId();
        if (const auto* insert = dynamic_cast<const RS_Insert*>(&entity)) {
            state.blockName = insert->getData().name;
            state.block = graphic ? graphic->RS_Graphic::getBlockList()->find(state.blockName) : nullptr;
            state.frozenBlock = state.block && state.block->isFrozen();
        }
        return state;
    }

    bool captureGraphs(const QList<RS_Entity*>& roots, std::set<RS_Entity*>& known, bool prevalidated = false) {
        const auto cap = kDefaultOffsetDeepEntitiesPerRequest;
        if (std::size_t(roots.size()) > std::numeric_limits<std::size_t>::max() - cap) return false;
        const auto maximum = cap + std::size_t(roots.size());
        struct Node { RS_Entity* entity; unsigned depth; };
        std::vector<Node> pending;
        if (prevalidated) {
            pending.reserve(known.size());
            states.reserve(states.size() + known.size() + std::size_t(before.size() + added.size()));
        }
        for (auto* root : roots) pending.push_back({root, 0});
        for (std::size_t i = 0; i < pending.size(); ++i) {
            auto* entity = pending[i].entity;
            if (!entity || pending[i].depth > 64 ||
                (prevalidated ? !known.count(entity) : !known.insert(entity).second) ||
                dynamic_cast<RS_Document*>(entity)) return false;
            states.push_back(storedState(*entity));
            auto* container = dynamic_cast<RS_EntityContainer*>(entity);
            if (!container || !container->isOwner()) continue;
            graphs.push_back({container, container->m_entities});
            if (std::size_t(container->m_entities.size()) > maximum - pending.size()) return false;
            for (auto* child : std::as_const(container->m_entities)) {
                if (!child) continue;
                if (child->getParent() != container) return false;
                pending.push_back({child, pending[i].depth + 1});
            }
        }
        return true;
    }

    RS_Layer* storedLayer(const RS_Entity& entity) const noexcept {
        const RS_Entity* node = &entity;
        for (unsigned depth = 0; node && depth < 64; ++depth, node = node->getParent()) {
            auto* layer = node->getLayer(false);
            if (layer) return graphic && graphic->RS_Graphic::getLayerList()->contains(layer) ? layer : nullptr;
        }
        return nullptr;
    }
    bool storedVisible(const RS_Entity& entity) const noexcept {
        if (!entity.getFlag(RS2::FlagVisible) || entity.getFlag(RS2::FlagDeleted)) return false;
        const auto* layer = storedLayer(entity);
        if (layer && layer->isFrozen()) return false;
        if (const auto* insert = dynamic_cast<const RS_Insert*>(&entity)) {
            const auto* block = graphic ? graphic->RS_Graphic::getBlockList()->find(insert->getData().name) : nullptr;
            if (block && block->isFrozen()) return false;
        }
        return true;
    }

    bool current() const noexcept {
        if (!document || committed || document->m_refCount || document->m_currentCycle ||
            document->undoStateEpoch() != undoEpoch || document->m_entities != before ||
            document->m_selectedSet->m_entitiesList != selectionBefore ||
            !pointEqual(viewport->getMarkedRelativeZero(), markedZero) ||
            !pointEqual(viewport->getRelativeZero(), zero)) return false;
        // Parent lists are sealed before any retained child address is read.
        for (const auto& graph : graphs) if (graph.owner->m_entities != graph.children) return false;
        for (const auto& state : states) {
            const auto& entity = *state.entity;
            if (entity.getId() != state.id || entity.getParent() != state.parent || entity.getLayer(false) != state.layer ||
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
    return prepareImpl(document, viewport, removed, added, selected, removed, {}, false);
}

std::unique_ptr<LC_OffsetCurvePublication> LC_OffsetCurvePublication::prepareCreation(RS_Document& document,
    LC_GraphicViewport& viewport, const QList<RS_Entity*>& removed, const QList<RS_Entity*>& added,
    const QList<RS_Entity*>& selected, const QList<RS_Entity*>& deselected, const QList<RS_Entity*>& roots) {
    return prepareImpl(document, viewport, removed, added, selected, deselected, roots, true);
}

std::unique_ptr<LC_OffsetCurvePublication> LC_OffsetCurvePublication::prepareImpl(RS_Document& document,
    LC_GraphicViewport& viewport, const QList<RS_Entity*>& removed, const QList<RS_Entity*>& added,
    const QList<RS_Entity*>& selected, const QList<RS_Entity*>& deselected,
    const QList<RS_Entity*>& roots, bool creation) {
    if ((creation ? added.isEmpty() : (removed.isEmpty() || removed.size() != added.size())) || !document.isOwner() ||
        document.m_refCount || document.m_currentCycle ||
        document.undoStateEpoch() == std::numeric_limits<std::uint64_t>::max()) return {};
    auto data = std::make_unique<Data>(document, viewport);
    if (!data->parentChainValid) return {};
    data->before = document.m_entities;
    data->selectionBefore = document.m_selectedSet->m_entitiesList;
    data->removed = removed;
    data->added = added;
    // Large creation batches must not rescan the whole document per source.
    // Keep the strict replacement path and small allocation censuses unchanged.
    std::vector<RS_Entity*> members;
    if (creation && data->before.size() > 32) {
        members.assign(data->before.begin(), data->before.end());
        std::sort(members.begin(), members.end(), std::less<RS_Entity*>{});
        if (std::adjacent_find(members.begin(), members.end()) != members.end()) return {};
    }
    const auto contains = [&](RS_Entity* entity) {
        return members.empty() ? data->before.contains(entity) :
            std::binary_search(members.begin(), members.end(), entity, std::less<RS_Entity*>{});
    };
    if (added.size() > std::numeric_limits<qsizetype>::max() - data->before.size()) return {};
    std::set<RS_Entity*> originals, replacements, selectedReplacements;
    auto cycle = std::make_shared<RS_UndoCycle>();
    if (!creation) for (qsizetype i = 0; i < removed.size(); ++i) {
        if (!data->before.contains(removed[i]) || data->before.contains(added[i])) return {};
        auto* original = dynamic_cast<LC_OffsetCurve*>(removed[i]);
        auto* replacement = dynamic_cast<LC_OffsetCurve*>(added[i]);
        if (!original || !replacement || original == replacement || original->getParent() != &document ||
            !originals.insert(original).second || !replacements.insert(replacement).second ||
            original->isDeleted() || replacement->isDeleted() ||
            (replacement->getParent() && replacement->getParent() != &document)) return {};
        cycle->addUndoable(original); cycle->addUndoable(replacement);
    }
    if (creation) {
        for (auto* original : removed) {
            if (!contains(original) || !original || original->getParent() != &document ||
                original->isDeleted() || !originals.insert(original).second) return {};
            cycle->addUndoable(original);
        }
        for (auto* replacement : added) {
            if (!replacement || contains(replacement) || replacement->isDeleted() ||
                (replacement->getParent() && replacement->getParent() != &document) ||
                !replacements.insert(replacement).second) return {};
            cycle->addUndoable(replacement);
        }
    }
    for (auto* entity : selected)
        if (!replacements.count(entity) || !selectedReplacements.insert(entity).second) return {};
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
    std::set<RS_Entity*> sourceNodes, addedNodes;
    if (creation) {
        std::set<RS_Entity*> inputRoots;
        for (auto* root : roots) {
            if (!contains(root) || !root || root->getParent() != &document) return {};
            inputRoots.insert(root);
        }
        for (auto* root : removed) inputRoots.insert(root);
        for (auto* node : deselected) if (contains(node)) inputRoots.insert(node);
        QList<RS_Entity*> inputs;
        for (auto* root : inputRoots) inputs.append(root);
        if (!data->captureGraphs(inputs, sourceNodes) || !data->captureGraphs(added, addedNodes)) return {};
        for (auto* node : deselected) if (!sourceNodes.count(node)) return {};
    }
    QList<RS_Entity*> retirementRoots;
    for (auto* root : data->retireAddresses) if (!sourceNodes.count(root)) retirementRoots.append(root);
    if (!retirementRoots.isEmpty() && !data->captureGraphs(retirementRoots, data->retireNodes, true)) return {};
    for (auto* entity : selected) {
        const auto* layer = creation ? data->storedLayer(*entity) : nullptr;
        const bool locked = creation ? layer && layer->isLocked() : entity->isLocked();
        if (!data->document || document.m_entities != data->before) return {};
        if (locked) selectedReplacements.erase(entity);
    }
    std::vector<RS_Entity*> replacedNodes;
    if (!creation) {
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
    } else {
        std::set<RS_Entity*> seen;
        std::vector<RS_Entity*> pending(deselected.begin(), deselected.end());
        pending.insert(pending.end(), removed.begin(), removed.end());
        for (std::size_t i = 0; i < pending.size(); ++i) {
            auto* node = pending[i];
            if (!sourceNodes.count(node)) return {};
            if (!seen.insert(node).second) continue;
            replacedNodes.push_back(node);
            const auto* container = dynamic_cast<const RS_EntityContainer*>(node);
            if (container && container->isOwner())
                for (auto* child : *container) if (child) pending.push_back(child);
        }
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
        data->states.push_back(data->storedState(*entity));
        const bool visible = creation ? data->storedVisible(*entity) : entity->isVisible();
        if (!data->document || document.m_entities != data->before) return {};
        if (!originals.count(entity) && !obsolete.count(entity) && visible && entity->hasValidBorders()) {
            data->low = data->low.valid ? RS_Vector::minimum(data->low, entity->getMin()) : entity->getMin();
            data->high = data->high.valid ? RS_Vector::maximum(data->high, entity->getMax()) : entity->getMax();
        }
    }
    if (!data->graphs.empty() && !data->current()) return {};
    if (!creation) for (qsizetype i = 0; i < removed.size(); ++i) {
        for (auto* root : {removed[i], added[i]}) {
            const bool select = root == added[i] && selectedReplacements.count(root);
            data->selectionFlags.emplace_back(root, select);
            for (auto* child : *static_cast<LC_OffsetCurve*>(root)) {
                if (!child || !dynamic_cast<RS_Line*>(child) || child->getParent() != root) return {};
                if (child->getFlag(RS2::FlagVisible)) data->selectionFlags.emplace_back(child, select);
            }
        }
    }
    if (creation) {
        std::set<RS_Entity*> flagged;
        const auto stageFlags = [&](const QList<RS_Entity*>& rootsToFlag, bool selectedFlag) {
            std::vector<RS_Entity*> pending(rootsToFlag.begin(), rootsToFlag.end());
            for (std::size_t i = 0; i < pending.size(); ++i) {
                auto* node = pending[i];
                if (!flagged.insert(node).second) continue;
                data->selectionFlags.emplace_back(node, selectedFlag);
                const auto* container = dynamic_cast<const RS_EntityContainer*>(node);
                if (container && container->isOwner()) for (auto* child : *container)
                    if (child && data->storedVisible(*child)) pending.push_back(child);
            }
        };
        auto toDeselect = deselected; toDeselect.append(removed);
        stageFlags(toDeselect, false);
        for (auto* root : added) stageFlags({root}, selectedReplacements.count(root));
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
