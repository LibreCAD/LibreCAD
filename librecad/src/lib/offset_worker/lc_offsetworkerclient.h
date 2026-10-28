/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERCLIENT_H
#define LC_OFFSETWORKERCLIENT_H

#include "lc_offsetworkerresult.h"
#include "lc_offsetworkerwire.h"
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <cstdint>
#include <functional>
#include <memory>

class QProcess;
class LC_OffsetWorkerClientTestAccess;

/** Only an owning, successfully joined built-helper job can create this value.
 * Callers must still recheck their document/source/undo snapshot at publication.
 * Public wire replies alone never confer native entity eligibility.
 */
class LC_OffsetWorkerCompletion final {
public:
    LC_OffsetWorkerCompletion(LC_OffsetWorkerCompletion&&) noexcept;
    LC_OffsetWorkerCompletion& operator=(LC_OffsetWorkerCompletion&&) noexcept;
    LC_OffsetWorkerCompletion(const LC_OffsetWorkerCompletion&) = delete;
    LC_OffsetWorkerCompletion& operator=(const LC_OffsetWorkerCompletion&) = delete;
    const LC_OffsetWorkerRequest::Request& request() const noexcept { return m_request; }
    const LC_OffsetWorkerResult::Reply& reply() const noexcept { return m_reply; }
    /** Ownership/lifetime only, not a public geometric certificate. */
    bool isJoined() const noexcept { return m_joined; }
    /** No generation/C/grip proof is repeated in the parent. Attributes stay local. */
    std::unique_ptr<LC_OffsetCurve> native(const RS_Entity& attributes) const;
    /** Complete ordinary output, including an intentional empty result, or no
     * value when unavailable/materialization refuses. New graph identities;
     * no native definition, generation, or C admission in the parent.
     */
    std::optional<std::vector<std::unique_ptr<RS_Entity>>> ordinary(const RS_Entity& attributes) const;
    struct PreparedSource {
        LC_OffsetSourceOutcome outcome;
        std::vector<std::unique_ptr<RS_Entity>> entities;
    };
    /** Complete Produce receipt and owned outputs, or no value for unsupported
     * representation/display expansion. Identity and publication stay local;
     * outcome has no source/created-entity pointers or deletion authority.
     */
    std::optional<PreparedSource> produced(const RS_Entity& attributes) const;
    /** Joined operation6 only; the caller still seals source/job lifetime. */
    bool updatePreviewCache(LC_OffsetPreviewCache&, const RS_Entity* source) const;
    /** Apply to a caller-owned clone only after its retained input seal matches.
     * Live document publication/undo remains the caller's separate transaction.
     */
    LC_OffsetCurveEditResult apply(LC_OffsetCurve& clone) const;
private:
    friend class LC_OffsetWorkerClient;
    LC_OffsetWorkerCompletion(LC_OffsetWorkerRequest::Request&&, LC_OffsetWorkerResult::Reply&&);
    LC_OffsetWorkerRequest::Request m_request;
    LC_OffsetWorkerResult::Reply m_reply;
    bool m_joined = true;
};

/** Per editing context: one active child and one replaceable queued request.
 * All methods/callbacks run on the owning Qt thread. No live document object is
 * sent to the child; callbacks own the final document snapshot/publication test.
 */
class LC_OffsetWorkerClient final : public QObject {
public:
    enum class Mode { Preview, Commit };
    enum class Status {
        Completed, Superseded, Cancelled, StartFailed, TimedOut, ProcessFailed,
        ProtocolFailed, AllocationFailed, ReapFailed
    };
    struct Deadlines {
        static constexpr qint64 startup = 2000, preview = 2000, commit = 30000;
        static constexpr qint64 terminate = 100, reap = 1000;
        // Pure monotonic boundary rule, also exercised without sleeping.
        static bool expired(qint64 elapsed, qint64 limit) noexcept { return elapsed >= limit; }
    };
    struct Outcome {
        Status status = Status::ProcessFailed;
        std::unique_ptr<LC_OffsetWorkerCompletion> completed;
        QByteArray diagnostics;
        LC_OffsetWorkerWire::Work transportWork{};
        bool reaped = true;
        /** False means the full reserved geometry allowance remains charged. */
        bool exactGeometryWorkKnown = false;
        qint64 elapsedMilliseconds = 0;
    };
    using Callback = std::function<void(Outcome)>;
    explicit LC_OffsetWorkerClient(QObject* parent = nullptr);
    ~LC_OffsetWorkerClient() override;
    /** Admission/copy failure returns false without invoking callback. Positive
     * request IDs must increase within this context. No retry with fresh caps.
     */
    bool submit(const LC_OffsetWorkerRequest::Request&, Mode, Callback);
    void cancel();
    bool busy() const noexcept;
    static QString helperPath();
private:
    friend class LC_OffsetWorkerClientTestAccess;
    struct Job;
    void start();
    void pump();
    void drain();
    void tick();
    void stop(Status);
    void finish(bool reaped);
    void deliver(Callback, Outcome);
    qint64 now() const noexcept;
    qint64 elapsedSince(qint64 origin) const noexcept;
    QByteArray diagnosticPrefix() const;
    std::unique_ptr<Job> m_active, m_queued;
    std::unique_ptr<QProcess> m_process;
    QElapsedTimer m_clock;
    qint64 m_startedAt = 0, m_stoppedAt = 0;
    // Private deterministic lifecycle seam; never set by product inputs.
    std::function<qint64()> m_testClock;
    QTimer m_timer;
    qint64 m_lastRequestId = 0;
    qint64 m_cancelledThrough = 0;
    std::uint64_t m_deliveryGeneration = 0;
    bool m_poisoned = false;
    // Production has no path/command setter. Tests may only lower these bounds.
    qint64 m_startupLimit = Deadlines::startup;
    qint64 m_previewLimit = Deadlines::preview;
    qint64 m_commitLimit = Deadlines::commit;
};
#endif
