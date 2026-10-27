/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerclient.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <algorithm>
#include <limits>

namespace {
constexpr qsizetype Chunk = 65536, DiagnosticCap = 65536;
bool matchesInputs(const LC_OffsetWorkerResult::Reply& reply, const LC_OffsetWorkerRequest::Request& request) {
    namespace Req = LC_OffsetWorkerRequest;
    const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&reply.result);
    if (!result || !result->curve) return true;
    const auto& curve = *result->curve;
    auto definition = request.operation.index() == 0 ? std::get<Req::Generate>(request.operation).definition :
        std::get<Req::ValidateCache>(request.operation).definition;
    if (!LC_EllipseOffsetGeometry::normaliseBasis(definition.basis) ||
        Req::Values::encodeDefinition(curve.definition) != Req::Values::encodeDefinition(definition)) return false;
    if (request.operation.index() == 1) {
        const auto& raw = std::get<Req::ValidateCache>(request.operation);
        return curve.worldTolerance == raw.worldTolerance &&
            Req::Values::encodeCache(curve.cache) == Req::Values::encodeCache(raw.cache);
    }
    const RS_Ellipse source{nullptr, definition.basis};
    return curve.worldTolerance == LC_CurveOffset::toleranceFor(source, definition.distance, definition.relTolerance);
}
void quarantine(std::unique_ptr<QProcess>& process) {
    // An OS that has not reaped after kill must not block the GUI destructor.
    // Retain only the process notifier, with no client/document/callback links.
    auto* retained = process.release();
    QObject::disconnect(retained, nullptr, nullptr, nullptr);
    QObject::connect(retained, &QProcess::finished, retained, &QObject::deleteLater);
    if (retained->state() == QProcess::NotRunning) retained->deleteLater();
}
}

struct LC_OffsetWorkerClient::Job {
    LC_OffsetWorkerRequest::Request request;
    Mode mode;
    Callback callback;
    QByteArray frame, diagnostics;
    LC_OffsetWorkerWire::Receiver receiver{};
    qsizetype sent = 0;
    bool started = false, inputClosed = false;
    std::optional<Status> stopping;
};

LC_OffsetWorkerClient::LC_OffsetWorkerClient(QObject* parent) : QObject(parent) {
    m_timer.setParent(this);
    m_timer.setInterval(5);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &LC_OffsetWorkerClient::tick);
}

LC_OffsetWorkerClient::~LC_OffsetWorkerClient() {
    m_timer.stop();
    if (m_process) {
        disconnect(m_process.get(), nullptr, this, nullptr);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            if (!m_process->waitForFinished(int(Deadlines::reap))) quarantine(m_process);
        }
    }
}

QString LC_OffsetWorkerClient::helperPath() {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(
#ifdef Q_OS_WIN
        QStringLiteral("LibreCADOffsetWorker.exe")
#else
        QStringLiteral("LibreCADOffsetWorker")
#endif
    );
}

bool LC_OffsetWorkerClient::busy() const noexcept { return bool(m_active) || bool(m_queued); }

bool LC_OffsetWorkerClient::submit(const LC_OffsetWorkerRequest::Request& request, Mode mode, Callback callback) {
    if (!QCoreApplication::instance() || QThread::currentThread() != thread() || m_poisoned || !callback ||
        (mode != Mode::Preview && mode != Mode::Commit) ||
        m_deliveryGeneration == std::numeric_limits<std::uint64_t>::max() ||
        request.token.requestId <= m_lastRequestId || !LC_OffsetWorkerRequest::admitted(request)) return false;
    std::unique_ptr<Job> job;
    try {
        const auto fields = LC_OffsetWorkerRequest::encode(request);
        if (!fields) return false;
        auto frame = LC_OffsetWorkerWire::encode(*fields);
        if (frame.isEmpty()) return false;
        job = std::make_unique<Job>(Job{request, mode, std::move(callback), std::move(frame)});
    } catch (...) { return false; }
    m_lastRequestId = request.token.requestId;
    ++m_deliveryGeneration;
    if (m_queued) {
        auto old = std::move(m_queued);
        Outcome outcome; outcome.status = Status::Superseded;
        deliver(std::move(old->callback), std::move(outcome));
    }
    m_queued = std::move(job);
    if (m_active) stop(Status::Superseded);
    else start();
    return true;
}

void LC_OffsetWorkerClient::cancel() {
    Q_ASSERT(QThread::currentThread() == thread());
    m_cancelledThrough = m_lastRequestId;
    if (m_queued) {
        auto old = std::move(m_queued);
        Outcome outcome; outcome.status = Status::Cancelled;
        deliver(std::move(old->callback), std::move(outcome));
    }
    if (m_active) stop(Status::Cancelled);
}

void LC_OffsetWorkerClient::start() {
    if (!m_queued || m_active || m_poisoned) return;
    m_active = std::move(m_queued);
    m_elapsed.start(); m_stopping.invalidate();
    try {
        m_process = std::make_unique<QProcess>();
        m_process->setProcessChannelMode(QProcess::SeparateChannels);
        connect(m_process.get(), &QProcess::started, this, [this] {
            if (!m_active) return;
            m_active->started = true;
            if (m_active->stopping) m_process->terminate();
            else pump();
        });
        connect(m_process.get(), &QProcess::bytesWritten, this, [this] { pump(); });
        connect(m_process.get(), &QProcess::readyReadStandardOutput, this, [this] { drain(); });
        connect(m_process.get(), &QProcess::readyReadStandardError, this, [this] { drain(); });
        // Finalization is polled, not performed inside QProcess's signal stack.
        m_process->setProgram(helperPath());
        m_process->setArguments({QStringLiteral("--protocol=1")});
        m_process->start();
        m_timer.start();
    } catch (...) {
        if (m_process && m_process->state() != QProcess::NotRunning) stop(Status::AllocationFailed);
        else { m_active->stopping = Status::AllocationFailed; finish(true); }
    }
}

void LC_OffsetWorkerClient::pump() {
    if (!m_active || m_active->stopping || !m_active->started || m_active->inputClosed) return;
    auto& job = *m_active;
    while (job.sent < job.frame.size() && m_process->bytesToWrite() < Chunk) {
        const auto n = m_process->write(job.frame.constData() + job.sent,
            std::min(Chunk, job.frame.size() - job.sent));
        if (n <= 0) { stop(Status::ProcessFailed); return; }
        job.sent += n;
    }
    if (job.sent == job.frame.size()) { job.inputClosed = true; m_process->closeWriteChannel(); }
}

void LC_OffsetWorkerClient::drain() {
    if (!m_active || m_active->stopping || !m_process) return;
    try {
        for (const auto channel : {QProcess::StandardOutput, QProcess::StandardError}) {
            m_process->setReadChannel(channel);
            while (m_process->bytesAvailable() > 0) {
                auto bytes = m_process->read(Chunk);
                if (bytes.isEmpty()) break;
                if (channel == QProcess::StandardOutput) {
                    if (!m_active->receiver.append(bytes)) { stop(Status::ProtocolFailed); return; }
                } else {
                    if (bytes.size() > DiagnosticCap - m_active->diagnostics.size()) {
                        stop(Status::ProtocolFailed); return;
                    }
                    m_active->diagnostics.append(bytes);
                }
            }
        }
        m_process->setReadChannel(QProcess::StandardOutput);
    } catch (...) { stop(Status::AllocationFailed); }
}

void LC_OffsetWorkerClient::stop(Status status) {
    if (!m_active || m_active->stopping) return;
    m_active->stopping = status;
    m_stopping.start();
    // Before started(), a forked child may still carry the parent's signal
    // handlers and open report files. Wait for exec confirmation before SIGTERM;
    // the fixed kill deadline still bounds a child stuck in Starting.
    if (m_process && m_active->started && m_process->state() == QProcess::Running) m_process->terminate();
    m_timer.start();
}

void LC_OffsetWorkerClient::tick() {
    if (!m_active) { m_timer.stop(); return; }
    if (!m_active->stopping && ((!m_active->started && Deadlines::expired(m_elapsed.elapsed(), m_startupLimit)) ||
        Deadlines::expired(m_elapsed.elapsed(), m_active->mode == Mode::Preview ? m_previewLimit : m_commitLimit)))
        stop(Status::TimedOut);
    if (!m_process || m_process->state() == QProcess::NotRunning) { finish(true); return; }
    if (m_active->stopping) {
        if (Deadlines::expired(m_stopping.elapsed(), Deadlines::terminate + Deadlines::reap)) {
            m_poisoned = true; quarantine(m_process); finish(false);
        } else if (Deadlines::expired(m_stopping.elapsed(), Deadlines::terminate)) m_process->kill();
    }
}

void LC_OffsetWorkerClient::finish(bool reaped) {
    if (!m_active) return;
    if (!m_active->stopping) drain();
    Outcome outcome;
    outcome.reaped = reaped;
    outcome.elapsedMilliseconds = m_elapsed.elapsed();
    try {
        if (!reaped) outcome.status = Status::ReapFailed;
        else if (m_active->stopping) outcome.status = *m_active->stopping;
        else if (!m_process || !m_active->started) outcome.status = Status::StartFailed;
        else if (!m_active->inputClosed || m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode())
            outcome.status = Status::ProcessFailed;
        else {
            auto message = m_active->receiver.finish();
            outcome.transportWork = message.work;
            auto reply = message ? LC_OffsetWorkerResult::decode(message.fields, m_active->request) : std::nullopt;
            if (!reply || !matchesInputs(*reply, m_active->request)) outcome.status = Status::ProtocolFailed;
            else if (reply->failure != LC_OffsetWorkerResult::Failure::None) outcome.status = Status::ProcessFailed;
            else if (Deadlines::expired(m_elapsed.elapsed(),
                m_active->mode == Mode::Preview ? m_previewLimit : m_commitLimit)) outcome.status = Status::TimedOut;
            else {
                outcome.completed.reset(new LC_OffsetWorkerCompletion(std::move(m_active->request), std::move(*reply)));
                outcome.status = Status::Completed;
                outcome.exactGeometryWorkKnown = true;
            }
        }
    } catch (...) { outcome.status = Status::AllocationFailed; outcome.completed.reset(); }
    outcome.diagnostics = std::move(m_active->diagnostics);
    auto callback = std::move(m_active->callback);
    m_active.reset(); m_process.reset(); m_timer.stop();
    deliver(std::move(callback), std::move(outcome));
    if (!m_poisoned) start();
    else if (m_queued) {
        auto queued = std::move(m_queued);
        Outcome refused; refused.status = Status::ReapFailed; refused.reaped = false;
        deliver(std::move(queued->callback), std::move(refused));
    }
}

void LC_OffsetWorkerClient::deliver(Callback callback, Outcome outcome) {
    // Deferred delivery avoids reentrancy while replacing a queued job. Owner
    // destruction drops the functor before it can reach a document callback.
    const auto generation = m_deliveryGeneration;
    QTimer::singleShot(0, this, [this, generation, callback = std::move(callback), outcome = std::move(outcome)]() mutable {
        if (outcome.status == Status::Completed && outcome.completed &&
            (m_poisoned || generation != m_deliveryGeneration ||
             outcome.completed->request().token.requestId <= m_cancelledThrough)) {
            outcome.status = outcome.completed->request().token.requestId <= m_cancelledThrough ?
                Status::Cancelled : Status::Superseded;
            outcome.completed.reset();
            outcome.exactGeometryWorkKnown = false;
        }
        callback(std::move(outcome));
    });
}
