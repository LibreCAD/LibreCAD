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
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

// This definition is linked only into the isolated qualification executable.
// It changes neither helper paths, admission, deadlines nor completion authority.
class LC_OffsetWorkerClientTestAccess {
public:
    static void clock(LC_OffsetWorkerClient& client, std::function<qint64()> clock) {
        client.m_testClock = std::move(clock);
    }
    static qint64 startedAt(const LC_OffsetWorkerClient& client) { return client.m_startedAt; }
    static void tick(LC_OffsetWorkerClient& client) { client.tick(); }
    static QByteArray diagnostics(const LC_OffsetWorkerClient& client) { return client.diagnosticPrefix(); }
    static QProcess* process(const LC_OffsetWorkerClient& client) { return client.m_process.get(); }
};

namespace {
using Client = LC_OffsetWorkerClient;
using Access = LC_OffsetWorkerClientTestAccess;
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Wire = LC_OffsetWorkerWire;

constexpr int SuiteTimeoutMs = 24000;
constexpr int WaitTimeoutMs = 3500;
constexpr const char* Marker = "LC_OFFSET_GEOMETRY_ENTERED ";

// This qualifies timeout AFTER genuine engine + C entry. The dedicated helper
// deliberately never returns from Generate and repeatedly executes real numeric
// work. It does NOT reproduce a naturally hung numeric primitive or establish an
// OS sandbox. Its one-pass diagnostic is neither exact total work nor a refund.
class Runtime {
public:
    Runtime() {
        // The micro-test target supplies the existing settings/temp isolation.
        if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
        elapsed.start();
    }

    void check() const {
        INFO("geometry timeout suite watchdog elapsed=" << elapsed.elapsed() << "ms");
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
        REQUIRE(directory.dirName() == QStringLiteral("native-offset-geometry-timeout-tests"));
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
            INFO("geometry timeout callback watchdog elapsed=" << waiting.elapsed() << "ms");
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
    char name[38] = "offset-worker-geometry-timeout-tests";
    char* argv[2] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    QElapsedTimer elapsed;
};

class WitnessClock {
public:
    WitnessClock() { real.start(); }
    qint64 now() const { return offset + real.elapsed(); }
    void jumpTo(qint64 deadline) { offset = deadline - real.elapsed(); }
private:
    QElapsedTimer real;
    qint64 offset = 0;
};

struct Receipts {
    std::array<std::optional<Client::Outcome>, 3> outcomes;
    std::array<unsigned, 3> calls{};

    Client::Callback into(std::size_t slot) {
        return [this, slot](Client::Outcome outcome) {
            // Do not throw Catch assertions through Qt's dispatch stack.
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

LC_EllipseOffsetDefinition definition() {
    return {{{0, 0}, {1, 0}, 1, 0, .001, false}, 1, false, false, 1e-4};
}

Req::Request generate(qint64 id) { return {{id, 11, 13, 17}, Req::Generate{definition(), {}, {}}}; }

Req::Request validate(qint64 id) {
    RS_SplineData cache{1, false};
    cache.controlPoints = {{2, 0}, {2 * std::cos(.001), 2 * std::sin(.001)}};
    cache.knotslist = {0, 0, 1, 1};
    cache.weights = {1, 1};
    return {{id, 11, 13, 17}, Req::ValidateCache{std::move(cache), definition(), .01, {}}};
}

bool witnessed(const QByteArray& diagnostics) {
    const auto start = diagnostics.indexOf(Marker);
    return start >= 0 && diagnostics.indexOf('\n', start) >= 0;
}

void geometryEntry(const QByteArray& diagnostics) {
    INFO("geometry entry diagnostic: " << diagnostics.toStdString());
    REQUIRE(diagnostics.size() <= 4096); // Only two bounded records, not per-pass output.
    REQUIRE(diagnostics.count(Marker) == 1);
    REQUIRE(diagnostics.count("LC_OFFSET_PREFIX ") == 1);
    const QRegularExpression entry(QStringLiteral(
        "(?m)^LC_OFFSET_GEOMETRY_ENTERED completed=1 native_prepare=1 check=1 "
        "engine_runs=([0-9]+) engine_status=([0-9]+) check_reason=([0-9]+) "
        "c_spans=([0-9]+) c_vertices=([0-9]+) c_pairs=([0-9]+) c_order=([0-9]+) c_depth=([0-9]+) "
        "sigterm_ignored=([01])\\r?\\n"));
    const auto entered = entry.match(QString::fromLatin1(diagnostics));
    REQUIRE(entered.hasMatch());
    CHECK(entered.captured(1).toULongLong() == 1);
    CHECK(entered.captured(2).toInt() == int(LC_CurveOffsetStatus::Ok));
    CHECK(entered.captured(3).toInt() == int(LC_OffsetCheckResult::Reason::None));
    CHECK(entered.captured(4).toULongLong() > 0);
    CHECK(entered.captured(5).toULongLong() > 0);
#ifdef Q_OS_UNIX
    CHECK(entered.captured(9) == QStringLiteral("1"));
#else
    CHECK(entered.captured(9) == QStringLiteral("0"));
#endif
    const QRegularExpression prefix(QStringLiteral(
        "(?m)^LC_OFFSET_PREFIX completed=1 native_prepare=1 check=1 through=0 trim=0 "
        "c_spans=([0-9]+) c_vertices=([0-9]+) c_pairs=([0-9]+) c_order=([0-9]+) c_depth=([0-9]+) "));
    const auto actual = prefix.match(QString::fromLatin1(diagnostics));
    REQUIRE(actual.hasMatch());
    for (int field = 1; field <= 5; ++field) CHECK(actual.captured(field) == entered.captured(field + 3));
}

void bounded(const Client::Outcome& outcome) {
    CHECK(outcome.reaped);
    CHECK(outcome.elapsedMilliseconds >= 0);
    CHECK(outcome.diagnostics.size() <= 65536);
    CHECK(outcome.transportWork.payloadBytes <= Wire::Limits{}.frameBytes);
    CHECK(outcome.transportWork.items <= Wire::Limits{}.items);
    CHECK(outcome.transportWork.deepest <= Wire::Limits{}.depth);
}
}

TEST_CASE("Worker times out after witnessed real geometry and recovers without refund or ID reuse",
          "[lc-offsetcurve][worker-geometry-timeout]") {
    Runtime rt;
    rt.helperAvailable();
    WitnessClock clock;
    Receipts receipts;
    unsigned heartbeats = 0;
    bool childFinished = false;
    QProcess::ExitStatus childExit = QProcess::NormalExit;
    QTimer heartbeat;
    heartbeat.setInterval(1);
    QObject::connect(&heartbeat, &QTimer::timeout, &heartbeat, [&] { ++heartbeats; });
    heartbeat.start();
    Client client; // Destroy before callback captures and the injected clock.
    Access::clock(client, [&] { return clock.now(); });
    const auto input = generate(1);
    REQUIRE(Req::admitted(input));
    REQUIRE(client.submit(input, Client::Mode::Commit, receipts.into(0)));
    const auto origin = Access::startedAt(client);
    rt.until([&] { return witnessed(Access::diagnostics(client)) || receipts.calls[0]; });
    REQUIRE(receipts.calls[0] == 0);
    const auto diagnostic = Access::diagnostics(client);
    geometryEntry(diagnostic);
    QPointer<QProcess> child = Access::process(client);
    REQUIRE(child);
    REQUIRE(child->state() == QProcess::Running);
    REQUIRE(child->processId() > 0);
    QObject::connect(child.data(), &QProcess::finished, &client, [&](int, QProcess::ExitStatus status) {
        childFinished = true;
        childExit = status;
    });

    const auto entryHeartbeats = heartbeats;
    rt.poll(); // Parent dispatch stays responsive while the child repeats real work.
    CHECK(heartbeats > entryHeartbeats);
    REQUIRE(client.busy());
    REQUIRE(receipts.calls[0] == 0);
    CHECK(Access::diagnostics(client) == diagnostic);
    REQUIRE(clock.now() - origin < Client::Deadlines::commit);

    // Only the witnessed child permits the monotonic jump. Before this point,
    // startup and computation see real elapsed time; afterward the unchanged
    // 100ms terminate + 1000ms reap windows also run on real monotonic elapsed.
    clock.jumpTo(origin + Client::Deadlines::commit);
    Access::tick(client);
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& timeout = receipts.at(0);
    bounded(timeout);
    CHECK(timeout.status == Client::Status::TimedOut);
    CHECK_FALSE(timeout.completed);
    // This is the client's full-reservation-kept contract. The observed C prefix
    // is NOT an exact total for the nonreturning job and cannot authorize refund.
    CHECK_FALSE(timeout.exactGeometryWorkKnown);
    CHECK(timeout.elapsedMilliseconds >= Client::Deadlines::commit);
    CHECK(timeout.elapsedMilliseconds < Client::Deadlines::commit + Client::Deadlines::terminate + Client::Deadlines::reap);
    CHECK(timeout.diagnostics == diagnostic);
    CHECK(childFinished);
    CHECK(child.isNull()); // The owning client destroyed its reaped QProcess.
#ifdef Q_OS_UNIX
    CHECK(timeout.elapsedMilliseconds >= Client::Deadlines::commit + Client::Deadlines::terminate);
    CHECK(childExit == QProcess::CrashExit); // SIGTERM was ignored; kill escalation ended it.
#endif
    REQUIRE_FALSE(client.busy());

    CHECK_FALSE(client.submit(input, Client::Mode::Commit, receipts.into(1)));
    const auto recovery = validate(2);
    REQUIRE(Req::admitted(recovery));
    REQUIRE(client.submit(recovery, Client::Mode::Commit, receipts.into(2)));
    rt.until([&] { return receipts.calls[2] != 0; });
    const auto& recovered = receipts.at(2);
    bounded(recovered);
    REQUIRE(recovered.status == Client::Status::Completed);
    REQUIRE(recovered.completed);
    CHECK(recovered.exactGeometryWorkKnown);
    CHECK(recovered.elapsedMilliseconds < WaitTimeoutMs);
    CHECK(recovered.transportWork.payloadBytes > 0);
    CHECK(recovered.transportWork.items > 0);
    INFO("recovery diagnostics: " << recovered.diagnostics.toStdString());
    // Normal startup can report RS_DEBUG::setLevel(0). No diagnostic from the
    // previous killed job may leak into this new, successfully joined request.
    CHECK_FALSE(recovered.diagnostics.contains(Marker));
    CHECK_FALSE(recovered.diagnostics.contains("LC_OFFSET_PREFIX "));
    const auto& completion = *recovered.completed;
    CHECK(completion.request().token.requestId == 2);
    CHECK(completion.request().operation.index() == 1);
    const auto& reply = completion.reply();
    CHECK(reply.token.requestId == 2);
    CHECK(reply.token.generation == 11);
    CHECK(reply.token.sourceRevision == 13);
    CHECK(reply.token.undoEpoch == 17);
    CHECK(reply.operation == 1);
    REQUIRE(reply.failure == Reply::Failure::None);
    REQUIRE(std::holds_alternative<Reply::Native>(reply.result));
    const auto& native = std::get<Reply::Native>(reply.result);
    REQUIRE(native.curve);
    CHECK(native.refusal == LC_OffsetCurveRefusal::None);
    CHECK(native.changed);
    CHECK(native.engineRuns == 0);
    CHECK_FALSE(native.engineStatus);
    REQUIRE(native.checkReason);
    CHECK(*native.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(native.checkWork.cacheSpans > 0);
    CHECK(native.checkWork.vertices > 0);
    const RS_Line attributes(nullptr, {0, 0}, {1, 0});
    const auto entity = completion.native(attributes);
    REQUIRE(entity);
    CHECK(entity->hasEditableDefinition());
    CHECK_FALSE(client.busy());
    rt.poll();
    CHECK(receipts.calls[0] == 1);
    CHECK(receipts.calls[1] == 0);
    CHECK(receipts.calls[2] == 1);
    CHECK(heartbeats > entryHeartbeats);
}
