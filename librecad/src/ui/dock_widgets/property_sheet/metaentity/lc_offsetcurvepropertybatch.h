/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEPROPERTYBATCH_H
#define LC_OFFSETCURVEPROPERTYBATCH_H

#include "lc_offsetcurve.h"
#include "lc_offsetcurvepublication.h"
#include "rs_document.h"
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

/** Native-only preparation. Candidate owners must outlive this object; moving
 * their std::map preserves node/reference addresses. No document changes occur
 * until execute(). Native publication is fully staged independently of the
 * generic undo API; observers run only after ownership and history are installed.
 */
class LC_OffsetCurvePropertyBatch final {
public:
    enum class Outcome { Committed, Refused };
    template <typename Pending>
    static std::unique_ptr<LC_OffsetCurvePropertyBatch> tryPrepare(Pending& pending,
        std::uint64_t& epoch, RS_Document& document, LC_GraphicViewport& viewport) noexcept {
        if (pending.empty() || pending.size() > std::size_t(std::numeric_limits<qsizetype>::max()) ||
            pending.size() > std::vector<Owner*>{}.max_size()) return {};
        for (const auto& entry : pending) if (!entry.first || !entry.second.candidate) return {};
        try {
            auto result = std::unique_ptr<LC_OffsetCurvePropertyBatch>(
                new LC_OffsetCurvePropertyBatch(pending, epoch));
            result->m_publication = LC_OffsetCurvePublication::prepare(document, viewport,
                result->m_remove, result->m_add, result->m_selected);
            if (!result->m_publication) return {};
            return result;
        } catch (...) { return {}; }
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

    Outcome execute() noexcept {
        if (m_handedOff || !m_publication || !m_publication->commit()) return Outcome::Refused;
        ++m_epoch;
        for (auto* owner : m_owners) owner->release();
        m_handedOff = true;
        m_publication->notify();
        return Outcome::Committed;
    }
    bool handedOff() const noexcept { return m_handedOff; }
    qsizetype size() const noexcept { return m_add.size(); }
    qsizetype selectedCount() const noexcept { return m_selected.size(); }

private:
    using Owner = std::unique_ptr<LC_OffsetCurve>;
    std::uint64_t& m_epoch;
    std::vector<Owner*> m_owners;
    QList<RS_Entity*> m_add, m_remove, m_selected;
    bool m_handedOff = false;
    std::unique_ptr<LC_OffsetCurvePublication> m_publication;
};
#endif
