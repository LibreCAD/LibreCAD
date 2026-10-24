/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurvepropertybatch.h"
#include "lc_offsetcurveeditsnapshot.h"

#include <QCoreApplication>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <type_traits>
#include <utility>
#include "lc_actioncontext.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_pen.h"

namespace {
QString queueTr(const char* text) {
    return QCoreApplication::translate("LC_OffsetCurveUI", text);
}
bool unlocked(const RS_Entity* entity) {
    const auto* layer = entity->getLayer(true);
    return layer == nullptr || !layer->isLocked();
}

}
static_assert(LC_OffsetCurvePropertyQueue::MaximumSnapshotItems == LC_OffsetCurveEditSnapshot::MaximumSnapshotItems);
static_assert(LC_OffsetCurvePropertyQueue::MaximumSnapshotBytes == LC_OffsetCurveEditSnapshot::MaximumSnapshotBytes);

/** Keep the private queue ABI/header and diagnostic source ID unchanged. */
struct LC_OffsetCurvePropertyQueue::SourceSnapshot {
    unsigned long long id;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> value;
    static std::unique_ptr<SourceSnapshot> capture(const LC_OffsetCurve& curve) {
        auto value = LC_OffsetCurveEditSnapshot::capture(curve);
        if (!value) return {};
        return std::unique_ptr<SourceSnapshot>(new SourceSnapshot{curve.getId(), std::move(value)});
    }
    bool matches(const LC_OffsetCurve& curve) const { return value->matches(curve); }
};

LC_OffsetCurvePropertyQueue::LC_OffsetCurvePropertyQueue(QObject* parent, LC_ActionContext* context)
    : QObject(parent), m_context(context), m_limits(LC_OffsetCurveUI::sheetLimits()) {
}
LC_OffsetCurvePropertyQueue::~LC_OffsetCurvePropertyQueue() = default;

void LC_OffsetCurvePropertyQueue::invalidateGetters() { ++m_epoch; }

void LC_OffsetCurvePropertyQueue::cancel() {
    invalidateGetters();
    ++m_contextSerial;
    m_scheduled = false;
    m_pending.clear(); // originals may already be dead; never dereference them here
}
void LC_OffsetCurvePropertyQueue::setContext(RS_Document* document, RS_GraphicView* view) {
    if (m_document == document && m_view.data() == view) return;
    cancel();
    m_document = document;
    m_view = view;
}
bool LC_OffsetCurvePropertyQueue::currentContext() const {
    return m_context && m_document && m_view &&
        m_context->getDocument() == m_document && m_context->getGraphicView() == m_view.data() &&
        m_view->getDocument() == m_document &&
        (m_document->rtti() == RS2::EntityGraphic || m_document->rtti() == RS2::EntityBlock);
}
bool LC_OffsetCurvePropertyQueue::isLive(const RS_Entity* original) const {
    // Membership/owner precede dereferencing a pointer retained across an event turn.
    return currentContext() && original && m_document->findEntityIndex(original) >= 0 &&
        original->getParent() == m_document && !original->isDeleted() &&
        original->rtti() == RS2::EntityOffsetCurve && dynamic_cast<const LC_OffsetCurve*>(original);
}
const LC_OffsetCurve* LC_OffsetCurvePropertyQueue::value(Epoch epoch, const RS_Entity* original) const {
    if (epoch != m_epoch || !isLive(original)) return nullptr;
    const auto* curve = static_cast<const LC_OffsetCurve*>(original);
    const auto found = m_pending.find(original);
    if (found == m_pending.end()) return curve;
    return found->second.source->matches(*curve) ? found->second.candidate.get() : nullptr;
}
void LC_OffsetCurvePropertyQueue::apply(Epoch epoch, const QList<RS_Entity*>& originals, const Edit& edit) {
    m_lastAttempts.clear();
    if (!edit) return;
    bool cancelled = false;
    {
        const LC_BusyCursorGuard busy;
        for (auto* original : originals) {
            Attempt attempt;
            attempt.original = original;
            auto found = m_pending.find(original);
            const LC_OffsetCurve* current = nullptr;
            std::unique_ptr<SourceSnapshot> snapshot;
            if (epoch != m_epoch || !isLive(original)) {
                attempt.contextRefusal = queueTr("the property editing session is no longer current");
            }
            else if (!unlocked(original) || m_document->isLocked()) {
                attempt.contextRefusal = queueTr("the source or editable container is locked");
            }
            else if (found != m_pending.end() &&
                     !found->second.source->matches(*static_cast<const LC_OffsetCurve*>(original))) {
                m_pending.erase(found);
                cancelled = true;
                attempt.contextRefusal = queueTr("the source changed after this edit was staged; the pending edit was cancelled");
            }
            else {
                current = found == m_pending.end() ? static_cast<const LC_OffsetCurve*>(original) :
                    found->second.candidate.get();
                if (found == m_pending.end()) snapshot = SourceSnapshot::capture(*current);
                if (found == m_pending.end() && !snapshot)
                    attempt.contextRefusal = queueTr("the source cannot be snapshotted within the bounded native read limits");
            }
            if (current && attempt.contextRefusal.isEmpty()) {
                auto candidate = std::unique_ptr<LC_OffsetCurve>(
                    static_cast<LC_OffsetCurve*>(current->clone()));
                attempt.editInvoked = true;
                attempt.result = edit(*candidate, m_limits);
                if (attempt.result && attempt.result.changed) {
                    if (found == m_pending.end())
                        m_pending.emplace(original, Pending{std::move(candidate), std::move(snapshot)});
                    else found->second.candidate = std::move(candidate);
                }
            }
            m_lastAttempts.push_back(std::move(attempt));
        }
    }
    report();
    if (cancelled) emit pendingCancelled();
    if (!m_pending.empty()) scheduleCommit();
}

void LC_OffsetCurvePropertyQueue::report() const {
    if (m_context == nullptr || m_lastAttempts.empty()) return;
    int changed = 0, unchanged = 0, refused = 0;
    QMap<QString, int> reasons;
    for (const auto& attempt : m_lastAttempts) {
        if (attempt.editInvoked && attempt.result) {
            if (attempt.result.changed) ++changed;
            else ++unchanged;
        }
        else {
            ++refused;
            const auto text = attempt.contextRefusal.isEmpty()
                ? LC_OffsetCurveUI::refusalText(attempt.result) : attempt.contextRefusal;
            ++reasons[text];
        }
    }
    QStringList detail;
    for (auto it = reasons.cbegin(); it != reasons.cend(); ++it) {
        detail.append(queueTr("%1: %2").arg(it.value()).arg(it.key()));
    }
    auto summary = queueTr("Offset Curve: %1 changed, %2 unchanged, %3 refused.")
        .arg(changed).arg(unchanged).arg(refused);
    if (!detail.isEmpty()) summary += " " + detail.join(queueTr("; "));
    m_context->commandMessage(summary);
}

void LC_OffsetCurvePropertyQueue::scheduleCommit() {
    if (m_scheduled) return;
    m_scheduled = true;
    const auto serial = m_contextSerial;
    QTimer::singleShot(30, this, [this, serial] {
        if (serial == m_contextSerial) commit(serial);
    });
}
void LC_OffsetCurvePropertyQueue::commit(Epoch contextSerial) {
    if (contextSerial != m_contextSerial) return;
    m_scheduled = false;
    const auto reportFailure = [this](const char* message) noexcept {
        try { if (m_context) m_context->commandMessage(queueTr(message)); }
        catch (const std::bad_alloc&) {} // Reporting must not escape the timer on OOM.
    };
    std::unique_ptr<LC_OffsetCurvePropertyBatch> prepared;
    try {
        if (!currentContext()) { cancel(); return; }
        bool cancelled = false;
        for (auto it = m_pending.begin(); it != m_pending.end();) {
            QString reason;
            if (!isLive(it->first)) reason = queueTr("the source is no longer live in its editable container");
            else if (!unlocked(it->first) || m_document->isLocked()) reason = queueTr("the source or editable container is locked");
            else if (!it->second.source->matches(*static_cast<const LC_OffsetCurve*>(it->first)))
                reason = queueTr("the source changed after the edit was staged");
            if (!reason.isEmpty()) {
                m_context->commandMessage(queueTr("Offset Curve: pending edit of entity %1 cancelled: %2.")
                    .arg(it->second.source->id).arg(reason));
                it = m_pending.erase(it);
                cancelled = true;
            } else ++it;
        }
        if (cancelled) emit pendingCancelled();
        if (m_pending.empty()) return;
        prepared = LC_OffsetCurvePropertyBatch::tryPrepare(m_pending, m_epoch);
    } catch (const std::bad_alloc&) {
        reportFailure("Offset Curve: allocation failed before publication; accepted edits remain pending.");
        return;
    }
    if (!prepared) {
        reportFailure("Offset Curve: batch preparation failed before publication; accepted edits remain pending.");
        return;
    }
    // std::map moves preserve the candidate-owner addresses stored by prepared.
    static_assert(std::is_nothrow_move_constructible_v<decltype(m_pending)>);
    auto pending = std::move(m_pending);
    m_pending.clear();
    const auto outcome = prepared->execute(*m_document, m_view->getViewPort());
    if (outcome == LC_OffsetCurvePropertyBatch::Outcome::AllocationBeforeHandoff) {
        static_assert(noexcept(m_pending.swap(pending)));
        m_pending.swap(pending);
        reportFailure("Offset Curve: allocation failed before publication; accepted edits remain pending.");
    } else if (outcome == LC_OffsetCurvePropertyBatch::Outcome::AllocationAfterHandoff) {
        reportFailure("Offset Curve: allocation failed after handoff; generic undo publication is not allocation-atomic.");
    }
}
