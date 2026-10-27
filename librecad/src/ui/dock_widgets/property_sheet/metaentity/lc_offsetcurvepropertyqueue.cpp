/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurvepropertybatch.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "../../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "../../../../lib/offset_worker/lc_offsetworkeredit.h"

#include <QCoreApplication>
#include <QMap>
#include <QElapsedTimer>
#include <QSet>
#include <QScopeGuard>
#include <QStringList>
#include <QTimer>
#include <type_traits>
#include <utility>
#include <limits>
#include <array>
#include <algorithm>
#include "lc_actioncontext.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"
#include "lc_graphicviewport.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_pen.h"
#include "lc_selectedset.h"

namespace {
QString queueTr(const char* text) {
    return QCoreApplication::translate("LC_OffsetCurveUI", text);
}
bool unlocked(const RS_Entity* entity) {
    const auto* layer = entity->getLayer(true);
    return layer == nullptr || !layer->isLocked();
}
bool registeredLayer(const LC_OffsetCurve& candidate, const RS_Document& document) {
    auto* layer = candidate.RS_Entity::getLayer(false);
    if (!layer) return true;
    const RS_Entity* ancestor = &document;
    for (unsigned depth = 0; ancestor && depth < 64; ++depth, ancestor = ancestor->getParent()) {
        const auto* graphic = dynamic_cast<const RS_Graphic*>(ancestor);
        if (graphic)
            return const_cast<RS_Graphic*>(graphic)->RS_Graphic::getLayerList()->contains(layer);
    }
    return false;
}

}
static_assert(LC_OffsetCurvePropertyQueue::MaximumSnapshotItems == LC_OffsetCurveEditSnapshot::MaximumSnapshotItems);
static_assert(LC_OffsetCurvePropertyQueue::MaximumSnapshotBytes == LC_OffsetCurveEditSnapshot::MaximumSnapshotBytes);

/** Retain only the executing value witness; this does not own the live source entity. */
struct LC_OffsetCurvePropertyQueue::SourceSnapshot {
    unsigned long long id;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> value;
    static std::shared_ptr<SourceSnapshot> capture(const LC_OffsetCurve& curve) {
        const auto id = curve.getId();
        auto value = LC_OffsetCurveEditSnapshot::capture(curve);
        if (!value) return {};
        return std::shared_ptr<SourceSnapshot>(new SourceSnapshot{id, std::move(value)});
    }
    bool matches(const LC_OffsetCurve& curve) const { return value->matches(curve); }
};

struct LC_OffsetCurvePropertyQueue::Work {
    struct Row {
        const LC_OffsetCurve* previous = nullptr;
        std::shared_ptr<SourceSnapshot> source;
        std::shared_ptr<SourceSnapshot> previousInput, input;
        bool hadPending = false;
    };
    Epoch epoch = 0, serial = 0;
    std::uint64_t undoEpoch = 0;
    DefinitionEdit edit;
    LC_OffsetRunLimits limits;
    std::shared_ptr<std::vector<Attempt>> attempts = std::make_shared<std::vector<Attempt>>();
    std::shared_ptr<std::vector<Attempt>> frozen = std::make_shared<std::vector<Attempt>>();
    std::vector<Row> rows;
    std::map<const RS_Entity*, Pending> staged;
    std::size_t next = 0, deepCharged = 0, deepLimit = 0;
    QElapsedTimer elapsed;
    QString cancelled;
    std::vector<const RS_Entity*> selection;
    std::array<RS_Vector, 3> world;
    double angle = 0;
    bool counterClockwise = true;
    std::shared_ptr<const std::vector<Attempt>> freeze(const QString& refusal = {}) noexcept {
        static_assert(std::is_nothrow_copy_assignable_v<Attempt>);
        std::copy(attempts->begin(), attempts->end(), frozen->begin());
        if (!refusal.isEmpty()) for (auto& attempt : *frozen) attempt.contextRefusal = refusal;
        return frozen;
    }
    bool selectionMatches(const RS_Document& document) const {
        const auto* selected = document.getSelection();
        if (!selected) return false;
        auto it = selected->begin();
        for (const auto* address : selection) {
            if (it == selected->end() || *it != address) return false;
            ++it;
        }
        return it == selected->end();
    }
    ~Work() {
        // A virtual snapshot/clone may destroy the document while this stack
        // still retains its private candidates. Never follow their old owner.
        for (auto& item : staged) if (item.second.candidate) item.second.candidate->setParent(nullptr);
    }
};

const std::vector<LC_OffsetCurvePropertyQueue::Attempt>& LC_OffsetCurvePropertyQueue::lastAttempts() const {
    if (m_work) return *m_work->attempts;
    return m_workerAttempts ? *m_workerAttempts : m_lastAttempts;
}

LC_OffsetCurvePropertyQueue::LC_OffsetCurvePropertyQueue(QObject* parent, LC_ActionContext* context)
    : QObject(parent), m_context(context), m_limits(LC_OffsetCurveUI::sheetLimits()),
      m_worker(std::make_unique<LC_OffsetWorkerClient>(this)), m_workTimer(std::make_unique<QTimer>(this)),
      m_commitTimer(std::make_unique<QTimer>(this)) {
    m_workTimer->setSingleShot(true);
    m_workTimer->setTimerType(Qt::PreciseTimer);
    connect(m_workTimer.get(), &QTimer::timeout, this, [this] {
        const auto work = m_work;
        if (work) finishPreparation(work, queueTr("the property preparation deadline expired"));
    });
    m_commitTimer->setSingleShot(true);
    connect(m_commitTimer.get(), &QTimer::timeout, this, [this] {
        if (m_scheduled) commit(m_contextSerial);
    });
}
LC_OffsetCurvePropertyQueue::~LC_OffsetCurvePropertyQueue() = default;

void LC_OffsetCurvePropertyQueue::invalidateGetters() {
    const auto work = m_work;
    const bool retain = work && currentWork(work);
    ++m_epoch;
    if (retain) {
        // A type-row/refill retires providers, not an already accepted value-owned
        // edit. Actual selection/context/undo changes still cancel that work.
        work->epoch = m_epoch;
        return;
    }
    const bool busy = isPreparing();
    cancelPreparation();
    if (busy) {
        if (!m_pending.empty()) scheduleCommit();
        emit preparationChanged(false);
    }
}

void LC_OffsetCurvePropertyQueue::cancel() {
    ++m_epoch;
    ++m_contextSerial;
    cancelPreparation();
    m_scheduled = false;
    m_commitTimer->stop();
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
    if (!candidate || !registeredLayer(*candidate, *m_document)) return nullptr;
    const auto serial = m_contextSerial;
    const QPointer<const LC_OffsetCurvePropertyQueue> alive(this);
    const bool matched = source->matches(*curve); // Retain the executing snapshot across cancel().
    if (!alive || epoch != m_epoch || serial != m_contextSerial || !currentContext()) return nullptr;
    const auto current = m_pending.find(original); // Never reuse an iterator from before the getter.
    return matched && current != m_pending.end() && current->second.source == source &&
        current->second.candidate.get() == candidate && registeredLayer(*candidate, *m_document) ? candidate : nullptr;
}

bool LC_OffsetCurvePropertyQueue::currentWork(const std::shared_ptr<Work>& work) const {
    if (m_work != work || work->epoch != m_epoch || work->serial != m_contextSerial ||
        work->elapsed.elapsed() >= LC_OffsetWorkerClient::Deadlines::commit ||
        !currentContext() || m_document->undoStateEpoch() != work->undoEpoch || !work->selectionMatches(*m_document)) return false;
    auto* viewport = m_view->getViewPort();
    const std::array<RS_Vector, 3> world{viewport->toWorld({0, 0}),
        viewport->toWorld({1, 0}), viewport->toWorld({0, 1})};
    for (std::size_t i = 0; i < world.size(); ++i)
        if (world[i].x != work->world[i].x || world[i].y != work->world[i].y ||
            world[i].z != work->world[i].z || world[i].valid != work->world[i].valid) return false;
    return viewport->toWorldAngle(viewport->toAbsUCSAngle(0)) == work->angle &&
        viewport->areAnglesCounterClockwise() == work->counterClockwise;
}

bool LC_OffsetCurvePropertyQueue::currentRow(const std::shared_ptr<Work>& work, std::size_t index) {
    if (!currentWork(work)) return false;
    auto* original = (*work->attempts)[index].original;
    const auto& row = work->rows[index];
    if (!isLive(original) || !unlocked(original) || m_document->isLocked()) return false;
    auto found = m_pending.find(original);
    if (row.hadPending ? found == m_pending.end() || found->second.source != row.source ||
        found->second.candidate.get() != row.previous : found != m_pending.end()) return false;
    // Retain an executing private witness across a cancellation from a getter.
    std::unique_ptr<LC_OffsetCurve> retained;
    if (row.hadPending) retained = std::move(found->second.candidate);
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const auto restore = qScopeGuard([&] {
        if (!retained) return;
        if (alive && work->serial == m_contextSerial) {
            const auto current = m_pending.find(original);
            if (current != m_pending.end() && current->second.source == row.source && !current->second.candidate)
                current->second.candidate = std::move(retained);
        }
        if (retained) retained->setParent(nullptr);
    });
    const bool matches = row.source && row.source->matches(*static_cast<const LC_OffsetCurve*>(original));
    if (!alive || !currentWork(work) || !matches) return false;
    if (retained && (!row.previousInput || !row.previousInput->matches(*retained))) return false;
    if (!alive || !currentWork(work)) return false;
    if (retained && !registeredLayer(*retained, *m_document)) return false;
    const auto staged = work->staged.find(original);
    const bool inputMatches = staged != work->staged.end() && staged->second.candidate && row.input &&
        row.input->matches(*staged->second.candidate);
    return alive && currentWork(work) && inputMatches && registeredLayer(*staged->second.candidate, *m_document);
}

void LC_OffsetCurvePropertyQueue::cancelPreparation() {
    m_workTimer->stop();
    m_worker->cancel();
    auto work = std::move(m_work);
    if (!work) return;
    m_workerAttempts = work->freeze(work->cancelled);
}

void LC_OffsetCurvePropertyQueue::rejectPreparation() {
    if (!isPreparing()) return;
    cancel();
    const auto serial = m_contextSerial;
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    emit preparationChanged(false);
    if (alive && serial == m_contextSerial && !isPreparing()) emit pendingCancelled();
}

void LC_OffsetCurvePropertyQueue::applyDefinition(Epoch epoch, const QList<RS_Entity*>& originals,
    const DefinitionEdit& edit) {
    if (!edit) return;
    if (isPreparing()) {
        if (m_context) m_context->commandMessage(queueTr("Offset Curve: wait for the current property preparation."));
        return;
    }
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const auto serial = m_contextSerial;
    try {
        // This existing snapshot item bound also bounds the private batch inventory.
        if (originals.size() > qsizetype(MaximumSnapshotItems) || epoch != m_epoch || !currentContext() ||
            m_document->undoStateEpoch() > std::uint64_t(std::numeric_limits<qint64>::max())) {
            if (m_context) m_context->commandMessage(queueTr("Offset Curve: the property workset is not admissible."));
            return;
        }
        auto work = std::make_shared<Work>(); work->elapsed.start();
        work->epoch = epoch; work->serial = serial; work->undoEpoch = m_document->undoStateEpoch();
        work->limits = m_limits; work->edit = edit;
        // The existing request quota is four maximal sources. Lower source
        // quotas lower that aggregate too; multiplication is bounded first.
        work->deepLimit = std::min(kDefaultOffsetDeepEntitiesPerRequest,
            std::size_t{4} * std::min(makeDefaultOffsetSourceBudget().maxDeepEntities,
                work->limits.outputBudget.maxDeepEntities));
        work->cancelled = queueTr("the property editing session was cancelled");
        for (const auto* selected : *m_document->getSelection()) {
            if (work->selection.size() == MaximumSnapshotItems) {
                m_context->commandMessage(queueTr("Offset Curve: the selected workset exceeds the bounded snapshot inventory."));
                return;
            }
            work->selection.push_back(selected); // Identity only: do not dereference arbitrary selection entries.
        }
        auto* viewport = m_view->getViewPort();
        work->world = {viewport->toWorld({0, 0}), viewport->toWorld({1, 0}), viewport->toWorld({0, 1})};
        work->angle = viewport->toWorldAngle(viewport->toAbsUCSAngle(0));
        work->counterClockwise = viewport->areAnglesCounterClockwise();
        QSet<RS_Entity*> seen;
        for (auto* original : originals) if (!seen.contains(original)) {
            seen.insert(original);
            Attempt attempt; attempt.original = original; (*work->attempts).push_back(std::move(attempt));
            work->staged.emplace(original, Pending{}); // All map nodes precede computation/publication.
        }
        if ((*work->attempts).empty()) return;
        work->rows.resize((*work->attempts).size());
        work->frozen->resize(work->attempts->size()); // Cancellation freezes without allocating or moving executing records.
        m_commitTimer->stop(); m_scheduled = false;
        m_work = work;
        const auto remaining = LC_OffsetWorkerClient::Deadlines::commit - work->elapsed.elapsed();
        m_workTimer->start(int(std::max<qint64>(1, remaining)));
        emit preparationChanged(true);
        if (!alive) return;
        if (!currentWork(work)) {
            if (m_work == work) finishPreparation(work, work->cancelled);
            return;
        }
        QTimer::singleShot(0, this, [this, work] { advance(work); });
    } catch (const std::bad_alloc&) {
        if (!alive || serial != m_contextSerial) return;
        if (m_work) finishPreparation(m_work, queueTr("allocation failed before property publication"));
        else if (m_context) m_context->commandMessage(queueTr("Offset Curve: allocation failed before property preparation."));
    }
}

void LC_OffsetCurvePropertyQueue::advance(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    if (!currentWork(work)) { finishPreparation(work, work->cancelled); return; }
    if (work->elapsed.elapsed() >= LC_OffsetWorkerClient::Deadlines::commit) {
        finishPreparation(work, queueTr("the property preparation deadline expired")); return;
    }
    if (work->next == (*work->attempts).size()) { finishPreparation(work); return; }
    const auto index = work->next++;
    auto& attempt = (*work->attempts)[index]; auto& row = work->rows[index];
    auto* const original = attempt.original;
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const auto current = [this, alive, work] {
        if (!alive || m_work != work) return false;
        if (currentWork(work)) return true;
        finishPreparation(work, work->cancelled);
        return false;
    };
    const auto next = [this, alive, work] {
        if (alive && m_work == work) QTimer::singleShot(0, this, [this, work] { advance(work); });
    };
    try {
        if (!isLive(original) || !unlocked(original) || m_document->isLocked()) {
            attempt.contextRefusal = queueTr("the source is not a live unlocked offset curve"); next(); return;
        }
        auto found = m_pending.find(original);
        row.hadPending = found != m_pending.end();
        if (row.hadPending) {
            row.source = found->second.source; row.previous = found->second.candidate.get();
            if (!row.previous) { attempt.contextRefusal = queueTr("a pending replacement is already being read"); next(); return; }
        } else row.source = SourceSnapshot::capture(*static_cast<const LC_OffsetCurve*>(original));
        if (!current()) return;
        if (!row.source) { attempt.contextRefusal = queueTr("the source exceeds the bounded snapshot limits"); next(); return; }
        const bool matched = row.source->matches(*static_cast<const LC_OffsetCurve*>(original));
        if (!current()) return;
        if (!matched) { attempt.contextRefusal = queueTr("the source changed before preparation"); next(); return; }
        found = m_pending.find(original);
        if (row.hadPending ? found == m_pending.end() || found->second.source != row.source ||
            found->second.candidate.get() != row.previous : found != m_pending.end()) {
            attempt.contextRefusal = queueTr("the pending replacement changed before preparation"); next(); return;
        }
        std::unique_ptr<LC_OffsetCurve> retained;
        if (row.hadPending) retained = std::move(found->second.candidate);
        const auto restore = [&] {
            if (!retained) return;
            if (alive && work->serial == m_contextSerial) {
                const auto current = m_pending.find(original);
                if (current != m_pending.end() && current->second.source == row.source && !current->second.candidate)
                    current->second.candidate = std::move(retained);
            }
            if (retained) retained->setParent(nullptr);
        };
        const auto restoreOnExit = qScopeGuard(restore);
        const auto* input = retained ? retained.get() : static_cast<const LC_OffsetCurve*>(original);
        if (retained) row.previousInput = SourceSnapshot::capture(*retained);
        if (!current()) return;
        auto candidate = std::unique_ptr<LC_OffsetCurve>(static_cast<LC_OffsetCurve*>(input->clone()));
        restore();
        auto& staged = work->staged.at(original);
        staged.candidate = std::move(candidate); staged.source = row.source; staged.undoEpoch = work->undoEpoch;
        if (!current()) return;
        row.input = SourceSnapshot::capture(*staged.candidate);
        if (!current()) return;
        attempt.editInvoked = true;
        const auto proposal = work->edit(*staged.candidate);
        if (!current()) return;
        if (!proposal || !row.input) { staged.candidate.reset(); next(); return; }
        auto prepared = LC_OffsetWorkerEdit::prepare(*staged.candidate, *proposal, work->limits);
        if (!current()) return;
        if (!currentRow(work, index)) {
            if (!alive || m_work != work) return;
            attempt.contextRefusal = queueTr("the property input changed during preparation");
            staged.candidate.reset(); next(); return;
        }
        if (!prepared.generate()) {
            attempt.result = LC_OffsetWorkerEdit::applyLocal(*staged.candidate, std::move(prepared));
            if (!current()) return;
            if (!attempt.result || !attempt.result.changed) staged.candidate.reset();
            else {
                const auto cost = measureOffsetOutput({staged.candidate.get()},
                    work->deepLimit - work->deepCharged);
                if (cost.status != LC_OffsetTreeStatus::Ok) {
                    attempt.contextRefusal = queueTr("the property batch output budget was exhausted"); staged.candidate.reset();
                } else work->deepCharged += cost.deepEntities;
            }
            next(); return;
        }
        if (m_requestId == std::numeric_limits<qint64>::max() ||
            work->deepCharged == work->deepLimit) {
            attempt.contextRefusal = queueTr("the property batch worker budget was exhausted");
            staged.candidate.reset(); next(); return;
        }
        auto generated = *prepared.generate();
        generated.limits.outputBudget.maxDeepEntities = std::min(generated.limits.outputBudget.maxDeepEntities,
            work->deepLimit - work->deepCharged);
        attempt.workerReservation = generated.limits;
        const auto reserved = generated.limits.outputBudget.maxDeepEntities;
        work->deepCharged += reserved; // Unknown/lost work never refunds this reservation.
        const auto id = ++m_requestId;
        const LC_OffsetWorkerRequest::Request request{{id, id, id, qint64(work->undoEpoch)}, std::move(generated)};
        const std::weak_ptr<Work> weak = work;
        const bool accepted = m_worker->submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, weak, index, reserved](LC_OffsetWorkerClient::Outcome outcome) {
                const auto work = weak.lock();
                if (!alive || !work || alive->m_work != work) return;
                auto& attempt = (*work->attempts)[index];
                auto& staged = work->staged.at(attempt.original);
                try {
                    std::optional<std::size_t> knownDeep;
                    bool childSucceeded = false;
                    if (outcome.completed) {
                        if (const auto* value = std::get_if<LC_OffsetWorkerResult::Native>(&outcome.completed->reply().result)) {
                            attempt.result = static_cast<const LC_OffsetCurveEditResult&>(*value);
                            childSucceeded = bool(*value);
                            attempt.exactWorkerWorkKnown = outcome.exactGeometryWorkKnown;
                            if (outcome.exactGeometryWorkKnown && value->attemptedDeepCountComplete) {
                                const auto used = std::max(value->attemptedOutputUsage.deepEntities, value->outputUsage.deepEntities);
                                if (used <= reserved) knownDeep = used;
                            }
                        }
                    }
                    if (!alive->currentWork(work)) { alive->finishPreparation(work, work->cancelled); return; }
                    if (outcome.status != LC_OffsetWorkerClient::Status::Completed || !outcome.completed) {
                        attempt.contextRefusal = outcome.status == LC_OffsetWorkerClient::Status::TimedOut ?
                            queueTr("the native worker timed out") : queueTr("the native worker could not complete the property edit");
                        staged.candidate.reset();
                    } else {
                        const bool current = alive->currentRow(work, index);
                        if (!alive || alive->m_work != work) return;
                        if (!current) {
                            attempt.contextRefusal = queueTr("the property input changed before worker adoption");
                            staged.candidate.reset();
                        } else {
                            attempt.result = outcome.completed->apply(*staged.candidate);
                            if (!alive || alive->m_work != work) return;
                            if (!attempt.result || !attempt.result.changed) staged.candidate.reset();
                            else {
                                // Receiving a line cache can rebuild more display
                                // leaves than the producer retained in its shell.
                                const auto cost = measureOffsetOutput({staged.candidate.get()}, reserved);
                                if (cost.status != LC_OffsetTreeStatus::Ok) {
                                    attempt.contextRefusal = queueTr("the received property output exceeds its reserved budget");
                                    staged.candidate.reset(); knownDeep.reset();
                                } else if (knownDeep) *knownDeep = std::max(*knownDeep, cost.deepEntities);
                            }
                        }
                    }
                    if (knownDeep && (!childSucceeded || (staged.candidate && attempt.result && attempt.result.changed)))
                        work->deepCharged -= reserved - *knownDeep;
                    QTimer::singleShot(0, alive.data(), [alive, work] { if (alive) alive->advance(work); });
                } catch (const std::bad_alloc&) {
                    if (alive && alive->m_work == work)
                        alive->finishPreparation(work, queueTr("allocation failed before property publication"));
                }
            });
        if (!accepted) {
            attempt.contextRefusal = queueTr("the native worker could not admit the property edit");
            staged.candidate.reset(); next();
        }
    } catch (const std::bad_alloc&) {
        if (alive && m_work == work) finishPreparation(work, queueTr("allocation failed before property publication"));
    }
}

void LC_OffsetCurvePropertyQueue::finishPreparation(std::shared_ptr<Work> work, const QString& refusal) {
    if (m_work != work) return;
    const QPointer<LC_OffsetCurvePropertyQueue> alive(this);
    const bool expired = work->elapsed.elapsed() >= LC_OffsetWorkerClient::Deadlines::commit;
    const auto reason = !refusal.isEmpty() ? refusal : expired ?
        queueTr("the property preparation deadline expired") : !currentWork(work) ? work->cancelled : QString{};
    if (!reason.isEmpty()) {
        for (auto& attempt : (*work->attempts)) attempt.contextRefusal = reason;
    } else {
        // Preallocated map nodes move without allocation; the existing commit()
        // still performs final per-source and undo seals before one publication.
        static_assert(std::is_nothrow_move_assignable_v<Pending>);
        for (std::size_t index = 0; index < (*work->attempts).size(); ++index) {
            const auto& attempt = (*work->attempts)[index];
            auto staged = work->staged.find(attempt.original);
            if (!attempt.contextRefusal.isEmpty() || !attempt.result || !attempt.result.changed || !staged->second.candidate) continue;
            auto node = work->staged.extract(staged);
            const auto found = m_pending.find(attempt.original);
            if (found == m_pending.end()) m_pending.insert(std::move(node));
            else found->second = std::move(node.mapped());
        }
    }
    m_workTimer->stop(); m_worker->cancel(); m_work.reset();
    m_workerAttempts = work->freeze();
    const auto serial = m_contextSerial;
    try { report(); }
    catch (const std::bad_alloc&) {} // Reporting must not strand accepted edits or the busy state.
    if (!alive || serial != m_contextSerial || isPreparing()) return;
    if (!m_pending.empty()) scheduleCommit();
    emit preparationChanged(false);
}

void LC_OffsetCurvePropertyQueue::apply(Epoch epoch, const QList<RS_Entity*>& originals, const Edit& edit) {
    if (isPreparing()) {
        if (m_context) m_context->commandMessage(queueTr("Offset Curve: wait for the current property preparation."));
        return;
    }
    m_lastAttempts.clear();
    m_workerAttempts.reset();
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
    const auto& attempts = lastAttempts();
    if (m_context == nullptr || attempts.empty()) return;
    int changed = 0, unchanged = 0, refused = 0;
    QMap<QString, int> reasons;
    for (const auto& attempt : attempts) {
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
    m_scheduled = true;
    m_commitTimer->start(30);
}
void LC_OffsetCurvePropertyQueue::commit(Epoch contextSerial) {
    if (contextSerial != m_contextSerial) return;
    m_scheduled = false;
    if (isPreparing()) return;
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
            std::optional<std::uint64_t> undoEpoch;
        };
        std::vector<ObservedSource> workset;
        workset.reserve(m_pending.size());
        for (const auto& entry : m_pending)
            workset.push_back({entry.first, entry.second.source, entry.second.candidate.get(), entry.second.undoEpoch});
        for (const auto& entry : workset) {
            // A previous getter may have replaced or retired this pending node.
            // The retained source keeps its executing snapshot alive even when
            // cancel() clears the entire map from inside a virtual point read.
            const auto samePending = [&] {
                const auto current = m_pending.find(entry.address);
                return current != m_pending.end() && current->second.source == entry.source &&
                    current->second.candidate.get() == entry.candidate;
            };
            if (!alive || contextSerial != m_contextSerial || !currentContext() || isPreparing() || !samePending()) return;
            QString reason;
            if (!isLive(entry.address)) reason = queueTr("the source is no longer live in its editable container");
            else if (entry.undoEpoch && *entry.undoEpoch != admittedUndoEpoch)
                reason = queueTr("the undo history changed after worker preparation");
            else if (!unlocked(entry.address) || m_document->isLocked()) reason = queueTr("the source or editable container is locked");
            else if (!entry.source->matches(*static_cast<const LC_OffsetCurve*>(entry.address)))
                reason = queueTr("the source changed after the edit was staged");
            if (!alive || contextSerial != m_contextSerial || !currentContext() || isPreparing() || !samePending()) return;
            if (!reason.isEmpty()) {
                notifications.append(queueTr("Offset Curve: pending edit of entity %1 cancelled: %2.")
                    .arg(entry.source->id).arg(reason));
                m_pending.erase(entry.address);
                cancelled = true;
            }
        }
        for (auto it = m_pending.begin(); it != m_pending.end();) {
            if (it->second.candidate && !registeredLayer(*it->second.candidate, *m_document)) {
                notifications.append(queueTr("Offset Curve: pending edit of entity %1 cancelled: its target layer was removed.")
                    .arg(it->second.source->id));
                it = m_pending.erase(it); cancelled = true;
            } else ++it;
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
        std::vector<const LC_OffsetCurve*> retainedCandidates;
        retainedCandidates.reserve(m_pending.size());
        const auto collectInputs = [&] {
            inputs.clear();
            retainedSources.clear();
            retainedCandidates.clear();
            for (const auto& entry : m_pending) {
                retainedSources.push_back(entry.second.source);
                retainedCandidates.push_back(entry.second.candidate.get());
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
        if (!alive || contextSerial != m_contextSerial || !currentContext() || isPreparing() ||
            m_document != admittedDocument || m_view != admittedView ||
            m_document->undoStateEpoch() != admittedUndoEpoch) return;
        if (prepared && !LC_OffsetCurveFinalInput::validate(*m_document, inputs)) {
            prepared.reset();
            discardInvalid();
            reportFailure("Offset Curve: a source changed during batch preparation; unaffected accepted edits remain pending.");
            notifyLateCancellation();
            return;
        }
        if (!alive || contextSerial != m_contextSerial || !currentContext() || isPreparing() ||
            m_pending.size() != inputs.size()) return;
        std::size_t index = 0;
        for (const auto& entry : m_pending) {
            if (entry.first != inputs[index].address || entry.second.source != retainedSources[index] ||
                entry.second.candidate.get() != retainedCandidates[index]) return;
            if (!registeredLayer(*entry.second.candidate, *m_document)) {
                prepared.reset();
                m_pending.erase(entry.first); lateCancelled = true;
                reportFailure("Offset Curve: a target layer was removed before publication; unaffected accepted edits remain pending.");
                notifyLateCancellation();
                return;
            }
            ++index;
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
