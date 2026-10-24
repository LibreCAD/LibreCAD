/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEPROPERTYBATCH_H
#define LC_OFFSETCURVEPROPERTYBATCH_H

#include "lc_offsetcurve.h"
#include "rs_document.h"
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

/** Native-only preparation. Candidate owners must outlive this object; moving
 * their std::map preserves node/reference addresses. No document changes occur
 * until execute(). This does NOT make generic undo publication allocation-atomic.
 */
class LC_OffsetCurvePropertyBatch final {
public:
    enum class Outcome { Committed, AllocationBeforeHandoff, AllocationAfterHandoff };
    template <typename Pending>
    static std::unique_ptr<LC_OffsetCurvePropertyBatch> tryPrepare(Pending& pending,
                                                                 std::uint64_t& epoch) noexcept {
        if (pending.empty() || pending.size() > std::size_t(std::numeric_limits<qsizetype>::max()) ||
            pending.size() > std::vector<Owner*>{}.max_size()) return {};
        for (const auto& entry : pending) if (!entry.first || !entry.second.candidate) return {};
        try {
            auto result = std::unique_ptr<LC_OffsetCurvePropertyBatch>(
                new LC_OffsetCurvePropertyBatch(pending, epoch));
            result->prepareCallbacks(); // std::function construction is also pre-undo
            return result;
        } catch (const std::bad_alloc&) { return {}; }
    }

    template <typename Pending>
    LC_OffsetCurvePropertyBatch(Pending& pending, std::uint64_t& epoch) : m_epoch(epoch) {
        const auto count = static_cast<qsizetype>(pending.size());
        // All capacity is private/unshared and reserved BEFORE the first pair.
        m_owners.reserve(pending.size());
        m_add.reserve(count); m_remove.reserve(count); m_selected.reserve(count);
        for (auto& entry : pending) {
            auto* candidate = entry.second.candidate.get();
            m_remove.append(const_cast<RS_Entity*>(entry.first));
            m_add.append(candidate);
            if (entry.first->getFlag(RS2::FlagSelected)) m_selected.append(candidate);
            m_owners.push_back(&entry.second.candidate);
        }
    }
    LC_OffsetCurvePropertyBatch(const LC_OffsetCurvePropertyBatch&) = delete;
    LC_OffsetCurvePropertyBatch& operator=(const LC_OffsetCurvePropertyBatch&) = delete;
    LC_OffsetCurvePropertyBatch(LC_OffsetCurvePropertyBatch&&) = delete;
    LC_OffsetCurvePropertyBatch& operator=(LC_OffsetCurvePropertyBatch&&) = delete;

    void prepareCallbacks() {
        m_modify = [this](LC_DocumentModificationBatch& batch) noexcept {
            handoff(batch);
            return true;
        };
        m_select = [this](LC_DocumentModificationBatch&, RS_Document* document) {
            document->select(m_selected);
        };
    }
    Outcome execute(RS_Document& document, LC_GraphicViewport* viewport) {
        try {
            document.undoableModify(viewport, m_modify, m_select);
            return Outcome::Committed;
        } catch (const std::bad_alloc&) {
            if (m_handedOff) {
                // No rollback claim: generic deletion/addition/selection and
                // undo-cycle completion can allocate after ownership transfer.
                return Outcome::AllocationAfterHandoff;
            }
            // Undo entry allocates before changing nesting or listener state.
            // No cycle began, and every candidate is still uniquely owned.
            return Outcome::AllocationBeforeHandoff;
        }
    }
    const RS_Document::FunUndoable& modification() const { return m_modify; }
    bool handedOff() const noexcept { return m_handedOff; }
    qsizetype size() const noexcept { return m_add.size(); }
    qsizetype selectedCount() const noexcept { return m_selected.size(); }

private:
    using Owner = std::unique_ptr<LC_OffsetCurve>;
    void handoff(LC_DocumentModificationBatch& batch) noexcept {
        Q_ASSERT(!m_handedOff && batch.entitiesToAdd.empty() && batch.entitiesToDelete.empty());
        static_assert(noexcept(batch.entitiesToAdd.swap(m_add)));
        static_assert(noexcept(batch.entitiesToDelete.swap(m_remove)));
        static_assert(noexcept(std::declval<Owner&>().release()));
        batch.dontSetActiveLayerAndPen();
        ++m_epoch; // Retire getters only at the nonthrowing ownership boundary.
        for (auto* owner : m_owners) {
            // Qualified base flag operation: no recursive/virtual callbacks.
            (*owner)->RS_Entity::setSelectionFlag(false);
        }
        batch.entitiesToAdd.swap(m_add);
        batch.entitiesToDelete.swap(m_remove);
        for (auto* owner : m_owners) owner->release();
        m_handedOff = true;
    }
    std::uint64_t& m_epoch;
    std::vector<Owner*> m_owners;
    QList<RS_Entity*> m_add, m_remove, m_selected;
    RS_Document::FunUndoable m_modify;
    RS_Document::FunSelection m_select;
    bool m_handedOff = false;
};
#endif
