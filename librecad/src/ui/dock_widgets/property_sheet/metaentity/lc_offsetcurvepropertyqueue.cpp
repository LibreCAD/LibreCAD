/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurvepropertybatch.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"

#include <QCoreApplication>
#include <QMap>
#include <QScopeGuard>
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

/** Retain only the executing value witness; this does not own the live source entity. */
struct LC_OffsetCurvePropertyQueue::SourceSnapshot {
    unsigned long long id;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> value;
    static std::shared_ptr<SourceSnapshot> capture(const LC_OffsetCurve& curve) {
        auto value = LC_OffsetCurveEditSnapshot::capture(curve);
        if (!value) return {};
        return std::shared_ptr<SourceSnapshot>(new SourceSnapshot{curve.getId(), std::move(value)});
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
    const auto source = found->second.source;
    const auto* candidate = found->second.candidate.get();
    if (!candidate) return nullptr; // A private clone operation currently retains this owner.
    const auto serial = m_contextSerial;
    const QPointer<const LC_OffsetCurvePropertyQueue> alive(this);
    const bool matched = source->matches(*curve); // Retain the executing snapshot across cancel().
    if (!alive || epoch != m_epoch || serial != m_contextSerial || !currentContext()) return nullptr;
    const auto current = m_pending.find(original); // Never reuse an iterator from before the getter.
    return matched && current != m_pending.end() && current->second.source == source &&
        current->second.candidate.get() == candidate ? candidate : nullptr;
}
void LC_OffsetCurvePropertyQueue::apply(Epoch epoch, const QList<RS_Entity*>& originals, const Edit& edit) {
    m_lastAttempts.clear();
    if (!edit) return;
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const auto serial = m_contextSerial;
    const auto workset = originals;
    const Edit operation = edit;
    const auto limits = m_limits;
    std::vector<Attempt> attempts;
    bool cancelled = false;
    {
        const LC_BusyCursorGuard busy;
        for (auto* original : workset) {
            Attempt attempt;
            attempt.original = original;
            const auto sessionCurrent = [&] {
                return alive && epoch == m_epoch && serial == m_contextSerial && currentContext();
            };
            const auto stage = [&] {
                if (!sessionCurrent() || !isLive(original)) {
                    attempt.contextRefusal = queueTr("the property editing session is no longer current");
                    return;
                }
                if (!unlocked(original) || m_document->isLocked()) {
                    attempt.contextRefusal = queueTr("the source or editable container is locked");
                    return;
                }
                auto found = m_pending.find(original);
                const bool hadPending = found != m_pending.end();
                std::shared_ptr<SourceSnapshot> snapshot;
                const LC_OffsetCurve* previous = nullptr;
                if (hadPending) {
                    snapshot = found->second.source;
                    previous = found->second.candidate.get();
                    if (!previous) {
                        attempt.contextRefusal = queueTr("a pending replacement is already being prepared");
                        return;
                    }
                    const bool matched = snapshot->matches(*static_cast<const LC_OffsetCurve*>(original));
                    if (!sessionCurrent()) {
                        if (alive) attempt.contextRefusal = queueTr("the property editing session is no longer current");
                        return;
                    }
                    found = m_pending.find(original);
                    if (found == m_pending.end() || found->second.source != snapshot ||
                        found->second.candidate.get() != previous) {
                        attempt.contextRefusal = queueTr("the pending replacement changed during a source read");
                        return;
                    }
                    if (!matched) {
                        m_pending.erase(found);
                        cancelled = true;
                        attempt.contextRefusal = queueTr("the source changed after this edit was staged; the pending edit was cancelled");
                        return;
                    }
                } else {
                    snapshot = SourceSnapshot::capture(*static_cast<const LC_OffsetCurve*>(original));
                    if (!sessionCurrent()) {
                        if (alive) attempt.contextRefusal = queueTr("the property editing session is no longer current");
                        return;
                    }
                    if (!snapshot) {
                        attempt.contextRefusal = queueTr("the source cannot be snapshotted within the bounded native read limits");
                        return;
                    }
                    found = m_pending.find(original);
                    if (found != m_pending.end()) {
                        attempt.contextRefusal = queueTr("the pending replacement changed during a source read");
                        return;
                    }
                }

                // A child clone may cancel the queue too. Temporarily retain the
                // current private curve so cancel() cannot delete an executing clone.
                std::unique_ptr<LC_OffsetCurve> retained;
                if (hadPending) retained = std::move(found->second.candidate);
                const auto restore = [&] {
                    if (!retained || !alive || serial != m_contextSerial) return;
                    const auto current = m_pending.find(original);
                    if (current != m_pending.end() && current->second.source == snapshot && !current->second.candidate)
                        current->second.candidate = std::move(retained);
                };
                const auto restoreOnExit = qScopeGuard(restore);
                const auto* current = hadPending ? retained.get() : static_cast<const LC_OffsetCurve*>(original);
                auto candidate = std::unique_ptr<LC_OffsetCurve>(
                    static_cast<LC_OffsetCurve*>(current->clone()));
                restore();
                if (!sessionCurrent()) {
                    if (alive) attempt.contextRefusal = queueTr("the property editing session is no longer current");
                    return;
                }
                attempt.editInvoked = true;
                attempt.result = operation(*candidate, limits);
                if (!sessionCurrent()) {
                    if (alive) attempt.contextRefusal = queueTr("the property editing session is no longer current");
                    return;
                }
                found = m_pending.find(original);
                if ((hadPending && (found == m_pending.end() || found->second.source != snapshot ||
                    found->second.candidate.get() != previous)) || (!hadPending && found != m_pending.end())) {
                    attempt.contextRefusal = queueTr("the pending replacement changed while an edit was prepared");
                    return;
                }
                if (attempt.result && attempt.result.changed) {
                    if (!hadPending)
                        m_pending.emplace(original, Pending{std::move(candidate), std::move(snapshot)});
                    else found->second.candidate = std::move(candidate);
                }
            };
            stage();
            if (!alive) return;
            attempts.push_back(std::move(attempt));
            if (serial != m_contextSerial) break;
        }
    }
    m_lastAttempts = std::move(attempts);
    report();
    if (!alive) return;
    if (cancelled) emit pendingCancelled();
    if (!alive || serial != m_contextSerial || !currentContext()) return;
    if (!m_pending.empty()) scheduleCommit();
}

void LC_OffsetCurvePropertyQueue::report() const {
    if (m_context == nullptr || m_lastAttempts.empty()) return;
    int changed = 0, unchanged = 0, refused = 0;
    QMap<QString, int> reasons;
    for (const auto& attempt : m_lastAttempts) {
        if (attempt.contextRefusal.isEmpty() && attempt.editInvoked && attempt.result) {
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
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const auto reportFailure = [this, alive](const char* message) noexcept {
        try { if (alive && m_context) m_context->commandMessage(queueTr(message)); }
        catch (const std::bad_alloc&) {} // Reporting must not escape the timer on OOM.
    };
    std::unique_ptr<LC_OffsetCurvePropertyBatch> prepared;
    bool lateCancelled = false;
    const auto notifyLateCancellation = [&] {
        // This notification is after handoff or on an abort path, never
        // between the last stored seal and native prepared publication.
        if (lateCancelled && alive) emit pendingCancelled();
    };
    try {
        if (!currentContext()) { cancel(); return; }
        auto* const admittedDocument = m_document;
        const auto admittedView = m_view;
        const auto admittedUndoEpoch = m_document->undoStateEpoch();
        QStringList notifications;
        bool cancelled = false;
        struct ObservedSource {
            const RS_Entity* address;
            std::shared_ptr<SourceSnapshot> source;
            const LC_OffsetCurve* candidate;
        };
        std::vector<ObservedSource> workset;
        workset.reserve(m_pending.size());
        for (const auto& entry : m_pending)
            workset.push_back({entry.first, entry.second.source, entry.second.candidate.get()});
        for (const auto& entry : workset) {
            // A previous getter may have replaced or retired this pending node.
            // The retained source keeps its executing snapshot alive even when
            // cancel() clears the entire map from inside a virtual point read.
            const auto samePending = [&] {
                const auto current = m_pending.find(entry.address);
                return current != m_pending.end() && current->second.source == entry.source &&
                    current->second.candidate.get() == entry.candidate;
            };
            if (!alive || contextSerial != m_contextSerial || !currentContext() || !samePending()) return;
            QString reason;
            if (!isLive(entry.address)) reason = queueTr("the source is no longer live in its editable container");
            else if (!unlocked(entry.address) || m_document->isLocked()) reason = queueTr("the source or editable container is locked");
            else if (!entry.source->matches(*static_cast<const LC_OffsetCurve*>(entry.address)))
                reason = queueTr("the source changed after the edit was staged");
            if (!alive || contextSerial != m_contextSerial || !currentContext() || !samePending()) return;
            if (!reason.isEmpty()) {
                notifications.append(queueTr("Offset Curve: pending edit of entity %1 cancelled: %2.")
                    .arg(entry.source->id).arg(reason));
                m_pending.erase(entry.address);
                cancelled = true;
            }
        }
        // Notify only after retiring failed entries: listeners may replace the
        // document, cancel the queue, or change another still-pending source.
        for (const auto& message : notifications) {
            m_context->commandMessage(message);
            if (!alive || contextSerial != m_contextSerial || !currentContext()) return;
        }
        if (cancelled) {
            emit pendingCancelled();
            if (!alive || contextSerial != m_contextSerial || !currentContext()) return;
        }
        if (m_document != admittedDocument || m_view != admittedView ||
            m_document->undoStateEpoch() != admittedUndoEpoch) { cancel(); return; }
        if (m_pending.empty()) return;
        std::vector<LC_OffsetCurveFinalInput::Input> inputs;
        inputs.reserve(m_pending.size());
        std::vector<std::shared_ptr<SourceSnapshot>> retainedSources;
        retainedSources.reserve(m_pending.size());
        const auto collectInputs = [&] {
            inputs.clear();
            retainedSources.clear();
            for (const auto& entry : m_pending) {
                retainedSources.push_back(entry.second.source);
                inputs.push_back({static_cast<const LC_OffsetCurve*>(entry.first), entry.second.source->value.get()});
            }
        };
        const auto discardInvalid = [&] {
            std::size_t i = 0;
            for (auto it = m_pending.begin(); it != m_pending.end(); ++i) {
                if (!inputs[i].matches) { it = m_pending.erase(it); lateCancelled = true; }
                else ++it;
            }
        };
        collectInputs();
        // Preparation reads selection flags through the original pointers.
        // Establish liveness after every notification before it may do so.
        if (!LC_OffsetCurveFinalInput::validate(*m_document, inputs)) {
            discardInvalid(); // Per-target refusal: retain unaffected accepted edits.
            if (m_pending.empty()) { notifyLateCancellation(); return; }
            collectInputs();
        }
        prepared = LC_OffsetCurvePropertyBatch::tryPrepare(m_pending, m_epoch);
        if (!alive || contextSerial != m_contextSerial || !currentContext() ||
            m_document != admittedDocument || m_view != admittedView ||
            m_document->undoStateEpoch() != admittedUndoEpoch) return;
        if (prepared && !LC_OffsetCurveFinalInput::validate(*m_document, inputs)) {
            prepared.reset();
            discardInvalid();
            reportFailure("Offset Curve: a source changed during batch preparation; unaffected accepted edits remain pending.");
            notifyLateCancellation();
            return;
        }
    } catch (const std::bad_alloc&) {
        if (!alive || contextSerial != m_contextSerial) return;
        reportFailure("Offset Curve: allocation failed before publication; accepted edits remain pending.");
        notifyLateCancellation();
        return;
    }
    if (!prepared) {
        reportFailure("Offset Curve: batch preparation failed before publication; accepted edits remain pending.");
        notifyLateCancellation();
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
    notifyLateCancellation();
}
