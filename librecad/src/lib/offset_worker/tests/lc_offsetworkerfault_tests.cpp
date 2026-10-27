/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerclient.h"
#include "rs_line.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QFileInfo>
#include <QPointer>
#include <QProcess>
#include <QThread>
#include <array>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>

// This TU is linked only into its isolated fault executable, not the real
// client test binary. No path, request admission or completion authority is set.
class LC_OffsetWorkerClientTestAccess {
public:
    static void clock(LC_OffsetWorkerClient& client, const qint64& value) {
        client.m_testClock = [&value] { return value; };
    }
    static void tick(LC_OffsetWorkerClient& client) { client.tick(); }
    static QByteArray diagnostics(const LC_OffsetWorkerClient& client) { return client.diagnosticPrefix(); }
    static QProcess* process(const LC_OffsetWorkerClient& client) { return client.m_process.get(); }
};

namespace {
using Client = LC_OffsetWorkerClient;
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Wire = LC_OffsetWorkerWire;

// Keep selector values identical to lc_offsetworkerfaultfixture.cpp. No path,
// environment, command or extra argument selects a fault in production code.
enum class Fault {
    Refusal = 1, NonzeroExit, Crash, EmptyResponse, MalformedCbor,
    TruncatedHeader, TruncatedPayload, DuplicateResponse, ExtraByte,
    WrongRequestId, WrongGeneration, WrongSourceRevision, WrongUndoEpoch,
    WrongOperation, AllocationFailure, InternalFailure, StderrOverflow,
    Hang, CompleteThenHang, LateComplete, CompleteThenNonzero
};
constexpr const char* Tags = "[lc-offsetcurve][worker-fault]";
constexpr int SuiteTimeoutMs = 24000;
constexpr int WaitTimeoutMs = 3500;

class Runtime {
public:
    Runtime() {
        // The parent target supplies the existing settings-isolation listener.
        if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
        elapsed.start();
    }

    void check() const {
        INFO("fault suite watchdog elapsed=" << elapsed.elapsed() << "ms");
        REQUIRE(elapsed.elapsed() < SuiteTimeoutMs);
    }

    void helperAvailable() const {
        check();
        const QFileInfo helper(Client::helperPath());
        REQUIRE(helper.isAbsolute());
        REQUIRE(helper.isFile());
        REQUIRE(helper.isExecutable());
        const QDir directory(QCoreApplication::applicationDirPath());
        REQUIRE(helper.absolutePath() == directory.absolutePath());
        // Fail closed if this suite was accidentally placed by the real helper.
        REQUIRE(directory.dirName() == QStringLiteral("native-offset-fault-tests"));
#ifdef Q_OS_WIN
        REQUIRE(helper.fileName() == QStringLiteral("LibreCADOffsetWorker.exe"));
#else
        REQUIRE(helper.fileName() == QStringLiteral("LibreCADOffsetWorker"));
#endif
    }

    template<class Predicate> void until(Predicate ready) const {
        QElapsedTimer waiting;
        waiting.start();
        while (!ready()) {
            check();
            INFO("fault callback watchdog elapsed=" << waiting.elapsed() << "ms");
            REQUIRE(waiting.elapsed() < WaitTimeoutMs);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (!ready()) QThread::msleep(1);
        }
        check();
    }

    void poll(int milliseconds = 20) const {
        QElapsedTimer waiting;
        waiting.start();
        while (waiting.elapsed() < milliseconds) {
            check();
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QThread::msleep(1);
        }
    }

private:
    int argc = 1;
    char name[26] = "offset-worker-fault-tests";
    char* argv[2] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    QElapsedTimer elapsed;
};

Runtime& runtime() {
    static Runtime instance;
    return instance;
}

struct Receipts {
    std::array<std::optional<Client::Outcome>, 4> outcomes;
    std::array<unsigned, 4> calls{};

    Client::Callback into(std::size_t slot) {
        return [this, slot](Client::Outcome outcome) {
            // Never assert or throw a Catch failure through Qt's callback stack.
            ++calls[slot];
            outcomes[slot].emplace(std::move(outcome));
        };
    }

    const Client::Outcome& at(std::size_t slot) const {
        REQUIRE(calls[slot] == 1);
        REQUIRE(outcomes[slot]);
        return *outcomes[slot];
    }
};

Req::Request request(qint64 id, Fault fault = Fault::Refusal) {
    LC_EllipseOffsetDefinition definition{
        {{0, 0}, {0, 0}, 1, 0, .001, false}, double(fault), false, false, 1e-4};
    return {{id, 11, 13, 17}, Req::Generate{definition, {}, {}}};
}

void bounded(const Client::Outcome& outcome) {
    INFO("fault status=" << int(outcome.status) << " elapsed=" << outcome.elapsedMilliseconds <<
        "ms diagnostic bytes=" << outcome.diagnostics.size());
    CHECK(outcome.reaped);
    CHECK(outcome.elapsedMilliseconds >= 0);
    CHECK(outcome.elapsedMilliseconds < WaitTimeoutMs);
    CHECK(outcome.diagnostics.size() <= 65536);
    CHECK(outcome.transportWork.payloadBytes <= Wire::Limits{}.frameBytes);
    CHECK(outcome.transportWork.items <= Wire::Limits{}.items);
    CHECK(outcome.transportWork.deepest <= Wire::Limits{}.depth);
}

QByteArray receipt(Fault fault) {
    return QByteArray("LC_OFFSET_FAULT_FIXTURE received=") + QByteArray::number(int(fault)) + '\n';
}

void refused(const Client::Outcome& outcome, qint64 id) {
    bounded(outcome);
    REQUIRE(outcome.status == Client::Status::Completed);
    REQUIRE(outcome.completed);
    CHECK(outcome.exactGeometryWorkKnown);
    CHECK(outcome.transportWork.payloadBytes > 0);
    CHECK(outcome.transportWork.items > 0);
    CHECK(outcome.diagnostics.contains(receipt(Fault::Refusal)));
    const auto& completion = *outcome.completed;
    const auto& original = completion.request();
    const auto& reply = completion.reply();
    CHECK(original.token.requestId == id);
    CHECK(original.token.generation == 11);
    CHECK(original.token.sourceRevision == 13);
    CHECK(original.token.undoEpoch == 17);
    CHECK(reply.token.requestId == original.token.requestId);
    CHECK(reply.token.generation == original.token.generation);
    CHECK(reply.token.sourceRevision == original.token.sourceRevision);
    CHECK(reply.token.undoEpoch == original.token.undoEpoch);
    CHECK(reply.operation == original.operation.index());
    CHECK(reply.failure == Reply::Failure::None);
    REQUIRE(std::holds_alternative<Reply::Native>(reply.result));
    const auto& native = std::get<Reply::Native>(reply.result);
    CHECK(native.refusal == LC_OffsetCurveRefusal::InvalidInput);
    CHECK_FALSE(native.changed);
    CHECK_FALSE(native.curve);
    CHECK_FALSE(native.engineStatus);
    CHECK_FALSE(native.checkReason);
    CHECK(native.engineRuns == 0);
    CHECK(native.checkWork.cacheSpans == 0);
    CHECK(native.checkWork.vertices == 0);
    CHECK(native.checkWork.distancePairs == 0);
    CHECK(native.checkWork.orderingComparisons == 0);
    CHECK(native.checkWork.deepest == 0);
    const RS_Line attributes(nullptr, {0, 0}, {1, 0});
    CHECK_FALSE(completion.native(attributes));
}

void failed(const Client::Outcome& outcome, Client::Status expected) {
    bounded(outcome);
    CHECK(outcome.status == expected);
    CHECK_FALSE(outcome.completed);
    CHECK_FALSE(outcome.exactGeometryWorkKnown);
}

void recoverQueue(Runtime& rt, Client& client, Receipts& receipts) {
    REQUIRE_FALSE(client.busy());
    // Exercise the actual active/replaceable-queued path after every fault,
    // retaining monotonic request IDs and the same client (no poison reset).
    REQUIRE(client.submit(request(2), Client::Mode::Preview, receipts.into(1)));
    REQUIRE(client.submit(request(3), Client::Mode::Preview, receipts.into(2)));
    REQUIRE(client.submit(request(4), Client::Mode::Preview, receipts.into(3)));
    CHECK(client.busy());
    rt.until([&] { return receipts.calls[1] && receipts.calls[2] && receipts.calls[3]; });
    REQUIRE_FALSE(client.busy());
    failed(receipts.at(1), Client::Status::Superseded);
    failed(receipts.at(2), Client::Status::Superseded);
    refused(receipts.at(3), 4);
    rt.poll();
    for (auto calls : receipts.calls) CHECK(calls == 1);
}

void exchangeFault(Runtime& rt, Fault fault, Client::Status expected) {
    CAPTURE(int(fault));
    rt.helperAvailable();
    Receipts receipts;
    Client client; // Destroy before its callback captures, even on REQUIRE unwind.
    const auto input = request(1, fault);
    REQUIRE(Req::admitted(input));
    REQUIRE(client.submit(input, Client::Mode::Preview, receipts.into(0)));
    CHECK(receipts.calls[0] == 0);
    rt.until([&] { return receipts.calls[0] != 0; });
    failed(receipts.at(0), expected);
    if (fault == Fault::AllocationFailure || fault == Fault::InternalFailure) {
        // A matched null-result failure is parsed transport, never known work.
        CHECK(receipts.at(0).diagnostics.contains(receipt(fault)));
        CHECK(receipts.at(0).transportWork.payloadBytes > 0);
        CHECK(receipts.at(0).transportWork.items > 0);
    }
    if (expected == Client::Status::TimedOut) {
        // Receipt evidence distinguishes a test-transport stall from startup.
        CHECK(receipts.at(0).diagnostics.contains(receipt(fault)));
        CHECK(receipts.at(0).elapsedMilliseconds >= Client::Deadlines::preview);
#ifdef Q_OS_UNIX
        if (fault == Fault::CompleteThenHang)
            CHECK(receipts.at(0).elapsedMilliseconds >= Client::Deadlines::preview + Client::Deadlines::terminate);
#endif
    }
    recoverQueue(rt, client, receipts);
}
}

TEST_CASE("Worker fault fixture completes only an early typed refusal with known work", Tags) {
    auto& rt = runtime();
    rt.helperAvailable();
    Receipts receipts;
    Client client;
    const auto input = request(1);
    REQUIRE(Req::admitted(input));
    REQUIRE(client.submit(input, Client::Mode::Preview, receipts.into(0)));
    CHECK(receipts.calls[0] == 0);
    rt.until([&] { return receipts.calls[0] != 0; });
    refused(receipts.at(0), 1);
    recoverQueue(rt, client, receipts);
}

TEST_CASE("Worker client reaps failed children and keeps geometry reservations unknown", Tags) {
    auto& rt = runtime();
    for (const auto fault : {Fault::NonzeroExit, Fault::Crash, Fault::AllocationFailure,
                            Fault::InternalFailure, Fault::CompleteThenNonzero}) {
        exchangeFault(rt, fault, Client::Status::ProcessFailed);
    }
}

TEST_CASE("Worker client rejects corrupt extra unmatched and oversized diagnostic replies", Tags) {
    auto& rt = runtime();
    for (const auto fault : {Fault::EmptyResponse, Fault::MalformedCbor, Fault::TruncatedHeader,
                            Fault::TruncatedPayload, Fault::DuplicateResponse, Fault::ExtraByte,
                            Fault::WrongRequestId, Fault::WrongGeneration, Fault::WrongSourceRevision,
                            Fault::WrongUndoEpoch, Fault::WrongOperation, Fault::StderrOverflow}) {
        exchangeFault(rt, fault, Client::Status::ProtocolFailed);
    }
}

TEST_CASE("Worker client watchdog reaps stalled fixture transports even after a complete frame", Tags) {
    auto& rt = runtime();
    // These children stall after receiving EOF, not inside a geometry kernel.
    // Killing them is transport watchdog qualification, not hung geometry/W2.
    for (const auto fault : {Fault::Hang, Fault::CompleteThenHang})
        exchangeFault(rt, fault, Client::Status::TimedOut);
}

TEST_CASE("Worker client rejects a late fixture completion after event dispatch resumes", Tags) {
    auto& rt = runtime();
    rt.helperAvailable();
    Receipts receipts;
    Client client;
    REQUIRE(client.submit(request(1, Fault::LateComplete), Client::Mode::Preview, receipts.into(0)));
    // Give QProcess time to send the frame, then withhold dispatch across the
    // real preview deadline while the fixture can finish. This is a real-time
    // lifecycle test, not a fake-clock boundary proof or a geometry hang test.
    rt.poll(100);
    REQUIRE(receipts.calls[0] == 0);
    QThread::msleep(static_cast<unsigned long>(Client::Deadlines::preview + 150));
    rt.check();
    rt.until([&] { return receipts.calls[0] != 0; });
    failed(receipts.at(0), Client::Status::TimedOut);
    CHECK(receipts.at(0).elapsedMilliseconds >= Client::Deadlines::preview);
    CHECK(receipts.at(0).diagnostics.contains(receipt(Fault::LateComplete)));
    recoverQueue(rt, client, receipts);
}

TEST_CASE("Worker client stateful fake clock keeps inclusive operation and kill boundaries", Tags) {
    auto& rt = runtime(); rt.helperAvailable();
    for (const auto mode : {Client::Mode::Preview, Client::Mode::Commit}) {
        Receipts receipts; qint64 time = 0; Client client;
        LC_OffsetWorkerClientTestAccess::clock(client, time);
        REQUIRE(client.submit(request(1, Fault::CompleteThenHang), mode, receipts.into(0)));
        rt.until([&] { return LC_OffsetWorkerClientTestAccess::diagnostics(client).contains("LC_OFFSET_FAULT_STALLED\n"); });
        auto* child = LC_OffsetWorkerClientTestAccess::process(client);
        REQUIRE(child); REQUIRE(child->state() == QProcess::Running);
        const auto deadline = mode == Client::Mode::Preview ? Client::Deadlines::preview : Client::Deadlines::commit;
        time = deadline - 1; LC_OffsetWorkerClientTestAccess::tick(client);
        CHECK(client.busy()); CHECK(receipts.calls[0] == 0);
        time = deadline; LC_OffsetWorkerClientTestAccess::tick(client);
        time += Client::Deadlines::terminate - 1; LC_OffsetWorkerClientTestAccess::tick(client);
        CHECK_FALSE(child->waitForFinished(1)); // SIGTERM ignored; kill must not be early.
        ++time; LC_OffsetWorkerClientTestAccess::tick(client);
        REQUIRE(child->waitForFinished(int(Client::Deadlines::reap)));
        LC_OffsetWorkerClientTestAccess::tick(client);
        rt.until([&] { return receipts.calls[0] != 0; });
        CHECK(receipts.at(0).status == Client::Status::TimedOut);
        CHECK(receipts.at(0).reaped); CHECK_FALSE(receipts.at(0).completed);
        CHECK_FALSE(receipts.at(0).exactGeometryWorkKnown);
        CHECK(receipts.at(0).elapsedMilliseconds == deadline + Client::Deadlines::terminate);
        recoverQueue(rt, client, receipts);
    }
}

TEST_CASE("Worker client kills before quarantine when one event turn skips the kill boundary", Tags) {
    auto& rt = runtime(); rt.helperAvailable();
    Receipts receipts; qint64 time = 0; Client client;
    LC_OffsetWorkerClientTestAccess::clock(client, time);
    REQUIRE(client.submit(request(1, Fault::CompleteThenHang), Client::Mode::Preview, receipts.into(0)));
    rt.until([&] { return LC_OffsetWorkerClientTestAccess::diagnostics(client).contains("LC_OFFSET_FAULT_STALLED\n"); });
    struct ReapGuard {
        QPointer<QProcess> child;
        ~ReapGuard() {
            if (child && child->state() != QProcess::NotRunning) {
                child->kill(); child->waitForFinished(int(Client::Deadlines::reap));
            }
        }
    } guard{LC_OffsetWorkerClientTestAccess::process(client)};
    REQUIRE(guard.child); REQUIRE(guard.child->state() == QProcess::Running);
    time = Client::Deadlines::preview; LC_OffsetWorkerClientTestAccess::tick(client);
    // Do not observe the 100ms kill tick at all. A late event must still issue
    // kill before quarantining; otherwise this fixture stays alive for 10s.
    time += Client::Deadlines::terminate + Client::Deadlines::reap;
    LC_OffsetWorkerClientTestAccess::tick(client);
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& outcome = receipts.at(0);
    CHECK(outcome.status == Client::Status::ReapFailed); CHECK_FALSE(outcome.reaped);
    CHECK_FALSE(outcome.completed); CHECK_FALSE(outcome.exactGeometryWorkKnown);
    CHECK(outcome.elapsedMilliseconds == time); CHECK_FALSE(client.busy());
    CHECK_FALSE(client.submit(request(2), Client::Mode::Preview, receipts.into(1)));
    rt.until([&] { return guard.child.isNull(); }); // Real exit/notifier cleanup, not a claimed reap.
    rt.poll(); CHECK(receipts.calls[0] == 1); CHECK(receipts.calls[1] == 0);
}
