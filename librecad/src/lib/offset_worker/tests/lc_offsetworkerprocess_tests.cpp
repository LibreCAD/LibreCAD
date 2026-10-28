/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerresult.h"
#include "lc_offsetworkerwire.h"
#include "rs_ellipse.h"

#include <catch2/catch_test_macros.hpp>
#include <QCborValue>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

#ifndef LIBRECAD_OFFSET_HELPER_PATH
#error "Native subprocess tests require the built LIBRECAD_OFFSET_HELPER_PATH target"
#endif

namespace {
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Wire = LC_OffsetWorkerWire;
namespace Trim = LC_OffsetCacheTrim;
namespace Geo = LC_EllipseOffsetGeometry;

constexpr int TotalTimeoutMs = 24000;
constexpr int ChildTimeoutMs = 2500;
constexpr int ReapTimeoutMs = 1000;
constexpr qsizetype InputCap = 64 * 1024;
constexpr qsizetype OutputCap = 256 * 1024;
constexpr qsizetype StderrCap = 16 * 1024;
const Req::Token Token{7, 11, 13, 17};

struct ScopedProcess {
    std::unique_ptr<QProcess> process;
    ScopedProcess() {
        REQUIRE(retainedProcess() == nullptr);
        process = std::make_unique<QProcess>();
    }
    ~ScopedProcess() {
        if (process && process->state() != QProcess::NotRunning) {
            process->kill();
            // Also runs when REQUIRE or an allocation exception unwinds the exchange.
            if (!process->waitForFinished(ReapTimeoutMs) && process->state() != QProcess::NotRunning) {
                // QProcess destruction can wait again. Retain at most one parentless
                // notifier until process exit instead, with no test callbacks or owner.
                // The assertion/exception has already failed this case; forbid relaunch.
                process->blockSignals(true);
                QObject::disconnect(process.get(), nullptr, nullptr, nullptr);
                retainedProcess() = process.release();
            }
        }
    }

private:
    static QProcess*& retainedProcess() {
        static QProcess* retained = nullptr;
        return retained;
    }
};

struct Completion {
    QByteArray output, diagnostic;
    QProcess::ExitStatus status = QProcess::CrashExit;
    int code = -1;
    qint64 elapsedMilliseconds = 0;
};

class Helper {
public:
    Helper() {
        const QFileInfo executable(QString::fromUtf8(LIBRECAD_OFFSET_HELPER_PATH));
        REQUIRE(executable.isAbsolute());
        REQUIRE(executable.isFile());
        REQUIRE(executable.isExecutable());
        total.start();
    }

    Completion exchange(const QByteArray& input,
                        const QStringList& arguments = {QStringLiteral("--protocol=4")},
                        bool partial = false) {
        REQUIRE(input.size() <= InputCap);
        ScopedProcess child;
        auto& process = *child.process;
        Completion completed;
        QElapsedTimer elapsed;
        elapsed.start();
        process.setProcessChannelMode(QProcess::SeparateChannels);
        process.setProgram(QString::fromUtf8(LIBRECAD_OFFSET_HELPER_PATH));
        process.setArguments(arguments);
        process.start(QIODevice::ReadWrite);
        const auto started = process.waitForStarted(remaining(elapsed));
        INFO("helper startup: " << process.errorString().toStdString());
        REQUIRE(started);

        const auto flush = [&] {
            while (process.bytesToWrite() > 0 && process.state() != QProcess::NotRunning) {
                process.waitForBytesWritten(std::min(10, remaining(elapsed)));
                drain(process, completed);
            }
        };
        if (partial) {
            REQUIRE(input.size() > 10);
            const std::array<qsizetype, 7> ends{
                1, 3, 4, 5, input.size() / 2, input.size() - 1, input.size()};
            qsizetype offset = 0;
            for (const auto end : ends) {
                REQUIRE(process.write(input.constData() + offset, end - offset) == end - offset);
                offset = end;
                flush();
                process.waitForFinished(std::min(5, remaining(elapsed)));
                drain(process, completed);
                // Even a complete frame cannot be acted on before EOF: extra bytes may follow.
                REQUIRE(process.state() == QProcess::Running);
                REQUIRE(completed.output.isEmpty());
            }
        } else if (!input.isEmpty()) {
            REQUIRE(process.write(input) == input.size());
            flush();
        }
        process.closeWriteChannel();
        while (process.state() != QProcess::NotRunning) {
            process.waitForFinished(std::min(10, remaining(elapsed)));
            drain(process, completed);
        }
        drain(process, completed);
        completed.status = process.exitStatus();
        completed.code = process.exitCode();
        completed.elapsedMilliseconds = elapsed.elapsed();
        REQUIRE(total.elapsed() < TotalTimeoutMs);
        return completed;
    }

    Reply::Reply run(const Req::Request& request, bool partial = false) {
        const auto fields = Req::encode(request);
        REQUIRE(fields);
        const auto input = Wire::encode(*fields);
        REQUIRE_FALSE(input.isEmpty());
        const auto completed = exchange(input, {QStringLiteral("--protocol=4")}, partial);
        INFO("helper stderr: " << completed.diagnostic.toStdString());
        REQUIRE(completed.status == QProcess::NormalExit);
        REQUIRE(completed.code == 0);
        REQUIRE_FALSE(completed.output.isEmpty());
        Wire::Receiver receiver;
        REQUIRE(receiver.append(completed.output));
        const auto message = receiver.finish();
        REQUIRE(message);
        auto reply = Reply::decode(message.fields, request);
        REQUIRE(reply);
        REQUIRE(reply->failure == Reply::Failure::None);
        CHECK(reply->operation == request.operation.index());
        CHECK(reply->token.requestId == request.token.requestId);
        CHECK(reply->token.generation == request.token.generation);
        CHECK(reply->token.sourceRevision == request.token.sourceRevision);
        CHECK(reply->token.undoEpoch == request.token.undoEpoch);
        std::fprintf(stderr, "LC_OFFSET_PROCESS operation=%zu elapsed_ms=%lld\n",
            request.operation.index(), static_cast<long long>(completed.elapsedMilliseconds));
        return std::move(*reply);
    }

    void reject(const QByteArray& input,
                const QStringList& arguments = {QStringLiteral("--protocol=4")}) {
        INFO("rejected bytes=" << input.size() << " arguments=" << arguments.join(' ').toStdString());
        const auto completed = exchange(input, arguments);
        INFO("helper stderr: " << completed.diagnostic.toStdString());
        REQUIRE(completed.status == QProcess::NormalExit);
        REQUIRE(completed.code != 0);
        REQUIRE(completed.output.isEmpty());
        REQUIRE_FALSE(completed.diagnostic.isEmpty());
    }

    void rejectFields(const QCborArray& fields) {
        const auto frame = Wire::encode(fields);
        REQUIRE_FALSE(frame.isEmpty());
        reject(frame);
    }

private:
    QElapsedTimer total;

    int remaining(const QElapsedTimer& child) const {
        const auto left = std::min(qint64(TotalTimeoutMs) - total.elapsed(),
                                   qint64(ChildTimeoutMs) - child.elapsed());
        INFO("native helper watchdog: total=" << total.elapsed() << "ms child=" << child.elapsed() << "ms");
        REQUIRE(left > 0);
        return int(left);
    }

    static void drain(QProcess& process, Completion& completed) {
        std::array<char, 4096> buffer{};
        const auto read = [&](QProcess::ProcessChannel channel, QByteArray& output, qsizetype cap) {
            process.setReadChannel(channel);
            while (process.bytesAvailable() > 0) {
                REQUIRE(process.bytesAvailable() <= cap - output.size());
                const auto size = process.read(buffer.data(),
                    std::min(qint64(buffer.size()), process.bytesAvailable()));
                REQUIRE(size > 0);
                output.append(buffer.data(), qsizetype(size));
            }
        };
        read(QProcess::StandardOutput, completed.output, OutputCap);
        read(QProcess::StandardError, completed.diagnostic, StderrCap);
    }
};

LC_EllipseOffsetDefinition definition() {
    return {{{0, 0}, {1, 0}, 1, 0, .001, false}, 1, false, false, 1e-4};
}

RS_SplineData lineCache() {
    RS_SplineData data{1, false};
    data.controlPoints = {{-3, .5}, {3, .5}};
    data.knotslist = {0, 0, 1, 1};
    data.weights = {1, 1};
    return data;
}

RS_SplineData arcCache() {
    auto data = lineCache();
    data.controlPoints = {{2, 0}, {2 * std::cos(.001), 2 * std::sin(.001)}};
    return data;
}

const Reply::Native& native(const Reply::Reply& reply) {
    REQUIRE(std::holds_alternative<Reply::Native>(reply.result));
    const auto& output = std::get<Reply::Native>(reply.result);
    if (reply.operation != 0 || output.refusal == LC_OffsetCurveRefusal::None ||
        output.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache) {
        CHECK(output.ordinaryStatus == Reply::OrdinaryStatus::None); CHECK(output.ordinary.empty());
        CHECK(output.ordinaryUsage.cubicPieces == 0); CHECK(output.ordinaryUsage.outputEntities == 0);
        CHECK(output.ordinaryUsage.deepEntities == 0);
    }
    return output;
}

const Trim::Result& trimmed(const Reply::Reply& reply) {
    REQUIRE(std::holds_alternative<Trim::Result>(reply.result));
    return std::get<Trim::Result>(reply.result);
}

void sameWork(const Geo::CheckWork& a, const Geo::CheckWork& b) {
    CHECK(a.cacheSpans == b.cacheSpans);
    CHECK(a.vertices == b.vertices);
    CHECK(a.distancePairs == b.distancePairs);
    CHECK(a.deepest == b.deepest);
    CHECK(a.orderingComparisons == b.orderingComparisons);
}

void checkedCurve(const Reply::Native& value) {
    REQUIRE(value.refusal == LC_OffsetCurveRefusal::None);
    REQUIRE(value.changed);
    REQUIRE(value.curve);
    const auto& curve = *value.curve;
    // Recheck actual child output; decoding alone grants no native authority.
    const auto checked = Geo::checkCache(curve.cache, curve.definition, curve.worldTolerance);
    REQUIRE(checked);
    REQUIRE(checked.analysis);
    sameWork(value.checkWork, checked.work);
    CHECK(curve.closed == checked.analysis->closed());
    REQUIRE_FALSE(curve.grips.empty());
    CHECK(curve.grips.size() <= 256);
    for (const auto& point : curve.grips) CHECK(Geo::isFinite2D(point));
    CHECK(Geo::isFinite2D(curve.midpoint));
    REQUIRE(value.checkReason);
    CHECK(*value.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(value.checkWork.cacheSpans > 0);
    CHECK(value.checkWork.vertices > 0);
    CHECK(value.checkWork.distancePairs > 0);
}

void lineComplement(const Trim::Result& value) {
    REQUIRE(value.refusal == Trim::Refusal::None);
    REQUIRE(value.fragments.size() == 2);
    CHECK(value.fragmentCount == 2);
    CHECK(value.rootCount == 2);
    REQUIRE(value.removed);
    CHECK(value.work.inputSpans > 0);
    CHECK(value.work.intersections.totalBoxPairs > 0);
    const auto root = std::sqrt(4 - .25);
    for (const auto& fragment : value.fragments) {
        CHECK(fragment.data.degree == 1);
        REQUIRE(fragment.data.controlPoints.size() == 2);
        CHECK(fragment.data.knotslist == std::vector<double>{0, 0, 1, 1});
        CHECK(fragment.data.weights == std::vector<double>{1, 1});
        CHECK_FALSE(fragment.data.m_closedFlag);
        CHECK_FALSE(fragment.data.m_periodicFlag);
        const auto& a = fragment.data.controlPoints.front();
        const auto& b = fragment.data.controlPoints.back();
        CHECK(std::abs(a.y - .5) < 1e-12);
        CHECK(std::abs(b.y - .5) < 1e-12);
        CHECK(a.x * b.x > 0);
        CHECK(std::abs(std::max(std::abs(a.x), std::abs(b.x)) - 3) < 1e-10);
        CHECK(std::abs(std::min(std::abs(a.x), std::abs(b.x)) - root) < 1e-6);
    }
}

void refusedTrim(const Trim::Result& value) {
    CHECK(value.refusal != Trim::Refusal::None);
    CHECK(value.fragments.empty());
    CHECK(value.conicFragments.empty());
    CHECK(value.fragmentCount == 0);
    CHECK_FALSE(value.removed);
}

QCborArray patched(QCborArray fields, std::initializer_list<qsizetype> path, const QCborValue& value) {
    const auto patch = [&](auto&& self, QCborArray& current, auto at) -> void {
        const auto index = *at++;
        if (at == path.end()) { current[index] = value; return; }
        auto child = current.at(index).toArray();
        self(self, child, at);
        current[index] = child;
    };
    patch(patch, fields, path.begin());
    return fields;
}

QByteArray header(quint32 size) {
    QByteArray bytes(4, '\0');
    qToBigEndian(size, bytes.data());
    return bytes;
}
}

// Each case shares an absolute watchdog across its real processes, including failures.
TEST_CASE("Built native offset helper qualifies computations and one-request process framing",
          "[lc-offsetcurve][worker-process]") {
    int argc = 1;
    char name[] = "offset-worker-process-tests";
    char* argv[] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
    Helper helper;

    {
        INFO("Generate executes the native producer and C check, including inward offsets");
        for (const bool inward : {false, true}) {
            CAPTURE(inward);
            auto def = definition();
            def.towardCentre = inward;
            def.distance = inward ? .25 : 1.;
            const auto reply = helper.run({Token, Req::Generate{def, {}, {}}}, !inward);
            const auto& value = native(reply);
            checkedCurve(value);
            CHECK(value.engineRuns == 1);
            REQUIRE(value.engineStatus);
            CHECK(*value.engineStatus == LC_CurveOffsetStatus::Ok);
            CHECK(value.curve->definition.distance == def.distance);
            CHECK(value.curve->definition.towardCentre == inward);
            CHECK(value.attemptedOutputUsage.cubicPieces > 0);
            CHECK(value.outputUsage.outputEntities == 1);
            CHECK(value.sampleUnits > 0);
            sameWork(value.throughWork, {});
        }
    }
    {
        INFO("Through success and refusals retain distinct actual C and membership work");
        Req::Generate operation{definition(), {}, RS_Vector{2, 0}};
        const auto good = helper.run({Token, operation});
        const auto& accepted = native(good);
        checkedCurve(accepted);
        CHECK(accepted.engineRuns == 1);
        sameWork(accepted.throughWork, {0, 1, 1});

        operation.limits.checkLimits.maxVertices = accepted.checkWork.vertices;
        const auto limited = helper.run({Token, operation});
        const auto& exhausted = native(limited);
        CHECK(exhausted.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK_FALSE(exhausted.changed);
        CHECK_FALSE(exhausted.curve);
        CHECK(exhausted.engineRuns == 1);
        sameWork(exhausted.checkWork, accepted.checkWork);
        sameWork(exhausted.throughWork, {});
        CHECK(exhausted.sampleUnits == accepted.sampleUnits);

        operation.limits = {};
        operation.throughPoint = RS_Vector{50, 50};
        const auto far = helper.run({Token, operation});
        const auto& missed = native(far);
        CHECK(missed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK_FALSE(missed.changed);
        CHECK_FALSE(missed.curve);
        CHECK(missed.engineRuns == 1);
        sameWork(missed.checkWork, accepted.checkWork);
        CHECK(missed.throughWork.vertices > 0);
        CHECK(missed.throughWork.distancePairs > 0);
    }
    {
        INFO("Validate accepts the original cache and refuses raw layouts without repairs");
        Req::ValidateCache operation{arcCache(), definition(), .01, {}};
        const auto original = Req::Values::encodeCache(operation.cache);
        REQUIRE(original);
        const auto good = helper.run({Token, operation});
        const auto& accepted = native(good);
        checkedCurve(accepted);
        CHECK(accepted.engineRuns == 0);
        CHECK_FALSE(accepted.engineStatus);
        CHECK(Req::Values::encodeCache(accepted.curve->cache) == original);
        sameWork(accepted.throughWork, {});
        for (const int malformed : {0, 1, 2, 3}) {
            CAPTURE(malformed);
            auto raw = operation;
            if (malformed == 0) raw.cache.degree = 2;
            if (malformed == 1) raw.cache.weights[0] = 2;
            if (malformed == 2) raw.cache.fitPoints.push_back({0, 0});
            if (malformed == 3) raw.cache.knotslist[2] = -1;
            const auto reply = helper.run({Token, raw});
            const auto& refused = native(reply);
            CHECK(refused.refusal == LC_OffsetCurveRefusal::CheckFailed);
            REQUIRE(refused.checkReason);
            CHECK(*refused.checkReason == LC_OffsetCheckResult::Reason::Layout);
            CHECK_FALSE(refused.curve);
            CHECK_FALSE(refused.changed);
            CHECK(refused.engineRuns == 0);
            CHECK_FALSE(refused.engineStatus);
            sameWork(refused.throughWork, {});
        }
    }
    {
        INFO("CheckThrough returns only structural and membership work without C or native geometry");
        Req::CheckThrough operation{lineCache(), definition(), .01, {-3, .5}, {}};
        const auto original = Req::Values::encodeCache(operation.cache); REQUIRE(original);
        const auto good = helper.run({Token, operation}, true);
        const auto& accepted = native(good);
        REQUIRE(accepted.refusal == LC_OffsetCurveRefusal::None);
        CHECK_FALSE(accepted.curve); CHECK_FALSE(accepted.changed);
        CHECK(accepted.engineRuns == 0); CHECK_FALSE(accepted.engineStatus); CHECK_FALSE(accepted.checkReason);
        sameWork(accepted.checkWork, {}); sameWork(accepted.throughWork, {1, 3, 1});
        CHECK(accepted.sampleUnits == 0); CHECK(accepted.outputUsage.outputEntities == 0);
        CHECK(Req::Values::encodeCache(operation.cache) == original);

        operation.point = {50, 50};
        const auto far = helper.run({Token, operation});
        const auto& missed = native(far);
        CHECK(missed.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        CHECK_FALSE(missed.curve); CHECK_FALSE(missed.changed); CHECK_FALSE(missed.checkReason);
        sameWork(missed.checkWork, {}); CHECK(missed.throughWork.cacheSpans == 1);
        CHECK(missed.throughWork.vertices > 2); CHECK(missed.throughWork.distancePairs > 0);

        operation.point = {-3, .5}; operation.limits.maxVertices = 2;
        const auto limited = helper.run({Token, operation});
        const auto& exhausted = native(limited);
        CHECK(exhausted.refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        sameWork(exhausted.throughWork, {1, 2, 0}); sameWork(exhausted.checkWork, {});
        operation.limits = {}; operation.cache.weights[0] = 2;
        const auto malformed = helper.run({Token, operation});
        CHECK(native(malformed).refusal == LC_OffsetCurveRefusal::ThroughNotOnCache);
        sameWork(native(malformed).throughWork, {1, 2, 0});
        operation.cache = lineCache(); operation.worldTolerance = 0;
        const auto invalid = helper.run({Token, operation});
        CHECK(native(invalid).refusal == LC_OffsetCurveRefusal::InvalidInput);
        sameWork(native(invalid).checkWork, {}); sameWork(native(invalid).throughWork, {});

        operation.worldTolerance = .01;
        const auto fields = Req::encode({Token, operation}); REQUIRE(fields);
        helper.rejectFields(patched(*fields, {0}, 1));
        helper.rejectFields(patched(*fields, {3, 3, 3}, false));
        helper.rejectFields(patched(*fields, {3, 4, 1}, qint64(LC_OffsetCheckLimits{}.maxVertices + 1)));
        auto body = fields->at(3).toArray(); body.removeLast();
        helper.rejectFields(patched(*fields, {3}, body));
    }
    {
        INFO("Both trim operations execute with the off-axis two-root fixture");
        const Req::Cutter circle = RS_CircleData{{0, 0}, 2};
        for (const bool reverse : {false, true}) {
            CAPTURE(reverse);
            auto data = lineCache();
            if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
            const auto cacheReply = helper.run({Token, Req::TrimCache{data, circle, {{0, .5}}, {}}});
            const auto& cache = trimmed(cacheReply);
            lineComplement(cache);
            CHECK(cache.conicFragments.empty());
            const auto conicReply = helper.run({Token,
                Req::TrimConic{data, circle, {{0, 2}}, Trim::Pick{{0, .5}}, {}}});
            const auto& conic = trimmed(conicReply);
            lineComplement(conic);
            REQUIRE_FALSE(conic.conicFragments.empty());
            for (const auto& fragment : conic.conicFragments) {
                CHECK(fragment.data.center.x == 0);
                CHECK(fragment.data.center.y == 0);
                CHECK(fragment.data.majorP.x == 2);
                CHECK(fragment.data.majorP.y == 0);
                CHECK(fragment.data.ratio == 1);
            }
        }
        // Do not instantiate RS_Ellipse: its constructor would round this to a whole turn.
        const RS_EllipseData tiny{{0, 0}, {2, 0}, 1, 0, 1e-12, false};
        const auto cacheReply = helper.run({Token, Req::TrimCache{lineCache(), tiny, {{0, .5}}, {}}});
        refusedTrim(trimmed(cacheReply));
        CHECK(trimmed(cacheReply).rootCount < 2);
        const auto conicReply = helper.run({Token,
            Req::TrimConic{lineCache(), tiny, {{0, 2}}, Trim::Pick{{0, .5}}, {}}});
        refusedTrim(trimmed(conicReply));
        CHECK(trimmed(conicReply).rootCount < 2);
    }
    {
        INFO("Invalid input/invocation exits nonzero and never emits a partial reply");
        const Req::Request request{Token, Req::Generate{definition(), {}, {}}};
        const auto fields = Req::encode(request);
        REQUIRE(fields);
        const auto frame = Wire::encode(*fields);
        REQUIRE(frame.size() > 10);
        helper.reject({});
        for (const auto size : {qsizetype{1}, qsizetype{3}, qsizetype{4}, frame.size() - 1}) {
            CAPTURE(size);
            helper.reject(frame.left(size));
        }
        helper.reject(frame + QByteArray(1, '\0'));
        helper.reject(frame + frame);
        helper.reject(header(0));
        // Only the oversized claim is sent; the test never allocates a huge body.
        helper.reject(header(quint32(Wire::Limits{}.frameBytes) + 1));
        helper.reject(header(1) + QByteArray(1, char(0x81)));
        helper.rejectFields(patched(*fields, {0}, qint64(1)));
        helper.rejectFields(patched(*fields, {0}, qint64(2)));
        helper.rejectFields(patched(*fields, {0}, qint64(3)));
        helper.rejectFields(patched(*fields, {1, 0}, 7.0));
        helper.rejectFields(patched(*fields, {2}, qint64(5)));
        helper.rejectFields(patched(*fields, {3, 0, 1}, qint64(1)));
        helper.rejectFields(patched(*fields, {3, 1, 3, 1}, true));
        helper.reject({}, QStringList{});
        helper.reject(frame, {QStringLiteral("--protocol=1")});
        helper.reject(frame, {QStringLiteral("--protocol=2")});
        helper.reject(frame, {QStringLiteral("--protocol=3")});
        helper.reject({}, {QStringLiteral("--protocol=4"), QStringLiteral("extra")});
    }
}

TEST_CASE("Built native offset helper preserves same-invocation ordinary fallback",
          "[lc-offsetcurve][worker-process][ordinary]") {
    int argc = 1;
    char name[] = "offset-worker-process-tests";
    char* argv[] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
    Helper helper;
    {
        INFO("Native C and display refusals transport checked ordinary output from one engine run");
        for (bool reversed : {false, true}) for (bool display : {false, true}) {
            auto def = definition(); def.cacheReversed = reversed; def.basis.center = {1e7, 0};
            Req::Generate operation{def, {}, {}};
            if (display) operation.limits.outputBudget.maxDeepEntities = 31;
            else { operation.limits.checkLimits.maxVertices = 1; operation.throughPoint = RS_Vector{50, 50}; }
            const auto reply = helper.run({Token, operation});
            const auto& value = native(reply);
            CHECK(value.refusal == (display ? LC_OffsetCurveRefusal::NotCacheRepresentable : LC_OffsetCurveRefusal::CheckFailed));
            CHECK(value.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(value.engineRuns == 1);
            CHECK_FALSE(value.curve); CHECK_FALSE(value.changed);
            REQUIRE(value.ordinaryStatus == Reply::OrdinaryStatus::Complete);
            REQUIRE(value.ordinary.size() == 1); REQUIRE(std::holds_alternative<Req::Segment>(value.ordinary[0]));
            const auto& line = std::get<Req::Segment>(value.ordinary[0]);
            RS_SplineData cache{1, false}; cache.controlPoints = {line.start, line.end};
            cache.knotslist = {0, 0, 1, 1}; cache.weights = {1, 1};
            const RS_Ellipse source{nullptr, def.basis};
            const auto tolerance = LC_CurveOffset::toleranceFor(source, def.distance, def.relTolerance);
            REQUIRE(Geo::checkCache(cache, def, tolerance));
            CHECK((line.start.y > line.end.y) == reversed);
            CHECK(value.ordinaryUsage.cubicPieces == 1); CHECK(value.ordinaryUsage.outputEntities == 1);
            CHECK(value.ordinaryUsage.deepEntities == 1); CHECK(value.sampleUnits > 0);
            sameWork(value.throughWork, {});
            if (display) CHECK(value.checkReason == LC_OffsetCheckResult::Reason::None);
            else { CHECK(value.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded); CHECK(value.checkWork.vertices <= 1); }
        }
    }
    {
        INFO("Real SeveralPieces and NothingLeft preserve the producer's complete ordinary result");
        const double pi = std::acos(-1.0);
        for (bool several : {false, true}) {
            auto def = definition(); def.towardCentre = true;
            def.basis = several ? RS_EllipseData{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false} :
                RS_EllipseData{{0, 0}, {5, 0}, 1};
            def.distance = several ? 4 : 5; def.relTolerance = several ? 1e-5 : 1e-4;
            const auto reply = helper.run({Token, Req::Generate{def, {}, {}}});
            const auto& value = native(reply);
            CHECK(value.refusal == (several ? LC_OffsetCurveRefusal::SeveralPieces : LC_OffsetCurveRefusal::NothingLeft));
            CHECK(value.engineStatus == LC_CurveOffsetStatus::Ok); CHECK(value.engineRuns == 1);
            REQUIRE(value.ordinaryStatus == Reply::OrdinaryStatus::Complete);
            CHECK_FALSE(value.curve); CHECK_FALSE(value.changed);
            CHECK(value.ordinary.empty() == !several);
            CHECK(value.ordinaryUsage.outputEntities == value.ordinary.size());
            std::size_t spans = 0;
            for (const auto& item : value.ordinary) {
                if (std::holds_alternative<Req::Segment>(item)) ++spans;
                else {
                    REQUIRE(std::holds_alternative<RS_SplineData>(item));
                    const auto& data = std::get<RS_SplineData>(item);
                    REQUIRE((data.degree == 1 || data.degree == 3));
                    spans += (data.controlPoints.size() - 1) / data.degree;
                }
            }
            CHECK(value.ordinaryUsage.cubicPieces == value.attemptedOutputUsage.cubicPieces);
            CHECK(spans <= makeDefaultOffsetSourceBudget().maxCubicPieces);
            CHECK(spans == value.ordinaryUsage.cubicPieces + (several ? 1 : 0));
            if (several) CHECK(value.ordinary.size() > 1);
            else CHECK(value.ordinaryUsage.deepEntities == 0);
        }
    }
}


TEST_CASE("Built helper transports Produce series grouping native admission work and ordinary native-off cache offsets",
          "[lc-offsetcurve][worker-process][produce]") {
    int argc = 1; char name[] = "offset-worker-produce-tests"; char* argv[] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
    Helper helper;
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    Req::Produce op; op.source = source.getData(); op.minimum = source.getMin(); op.maximum = source.getMax();
    op.context.topLevel = true; op.coordinate = {3, 2}; op.distance = .25; op.copies = 2; op.asOffsetCurve = true;
    const auto reply = helper.run({Token, op}, true);
    REQUIRE(std::holds_alternative<Reply::Produced>(reply.result));
    const auto& value = std::get<Reply::Produced>(reply.result);
    REQUIRE(value.source.status == LC_OffsetSourceStatus::Succeeded);
    REQUIRE(value.outputStatus == Reply::OrdinaryStatus::Complete);
    CHECK(value.copyOutputCounts == std::vector<std::size_t>{1, 1});
    CHECK(value.source.adoptionAttempts == 2); CHECK(value.source.adoptionCheckWork.vertices > 0);
    REQUIRE(value.source.resolvedSide);
    REQUIRE(value.outputs.size() == 2);
    for (std::size_t i = 0; i < value.outputs.size(); ++i) {
        REQUIRE(std::holds_alternative<Reply::Curve>(value.outputs[i]));
        const auto& curve = std::get<Reply::Curve>(value.outputs[i]);
        CHECK(curve.definition.distance == double(i + 1) * op.distance);
        REQUIRE(Geo::checkCache(curve.cache, curve.definition, curve.worldTolerance));
    }
    RS_Spline cache{nullptr, std::get<Reply::Curve>(value.outputs[0]).cache};
    op.source = cache.getData(); op.minimum = cache.getMin(); op.maximum = cache.getMax();
    op.context.nativeSource = true; op.context.nativeDefinition = std::get<Reply::Curve>(value.outputs[0]).definition;
    op.asOffsetCurve = false;
    const auto ordinary = helper.run({Token, op});
    REQUIRE(std::holds_alternative<Reply::Produced>(ordinary.result));
    const auto& plain = std::get<Reply::Produced>(ordinary.result);
    REQUIRE(plain.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(plain.source.rebaseEngineRuns == 0); CHECK(plain.source.adoptionAttempts == 0);
    CHECK(plain.rebases.empty()); REQUIRE(plain.outputStatus == Reply::OrdinaryStatus::Complete);
    for (const auto& output : plain.outputs) CHECK_FALSE(std::holds_alternative<Reply::Curve>(output));
    op.asOffsetCurve = true; op.limits.maxSamples = 1;
    const auto failed = helper.run({Token, op});
    REQUIRE(std::holds_alternative<Reply::Produced>(failed.result));
    const auto& refusal = std::get<Reply::Produced>(failed.result);
    CHECK(refusal.source.status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(refusal.copyOutputCounts.empty()); CHECK(refusal.outputs.empty());
    CHECK(refusal.outputStatus == Reply::OrdinaryStatus::None);
    CHECK(refusal.source.sampleUnits > 1); CHECK(refusal.source.rebaseEngineRuns == 1);
}

TEST_CASE("Fresh helper processes preserve cached preview hits while native adoption remains checked",
          "[lc-offsetcurve][worker-process][preview-portable]") {
    int argc = 1; char name[] = "offset-worker-cached-produce-tests"; char* argv[] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
    Helper helper;
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    Req::CachedProduce op;
    op.produce.source = source.getData(); op.produce.minimum = source.getMin(); op.produce.maximum = source.getMax();
    op.produce.context.topLevel = true; op.produce.coordinate = {3, 2};
    op.produce.distance = .25; op.produce.copies = 2; op.produce.asOffsetCurve = true; op.produce.preview = true;
    const auto first = helper.run({Token, op}, true);
    const auto& preview = std::get<Reply::Produced>(first.result);
    REQUIRE(preview.previewCache); REQUIRE(preview.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(preview.source.cacheEngineRuns == 2);
    op.cache = *preview.previewCache; op.produce.preview = false;
    op.produce.limits.maxSamples = 1; op.produce.limits.maxIntersectionPairs = 1;
    const auto second = helper.run({Token, op}); // A different process, not a persistent child cache.
    const auto& committed = std::get<Reply::Produced>(second.result);
    REQUIRE(committed.source.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(committed.source.reusedCacheCopies == 2); CHECK(committed.source.cacheEngineRuns == 0);
    CHECK(committed.source.sampleUnits == 0); CHECK(committed.source.intersectionPairAllowance == 0);
    CHECK(committed.source.adoptionAttempts == 2); CHECK(committed.source.adoptionCheckWork.vertices > 0);
    REQUIRE(committed.outputs.size() == 2);
    for (const auto& output : committed.outputs) {
        REQUIRE(std::holds_alternative<Reply::Curve>(output));
        const auto& curve = std::get<Reply::Curve>(output);
        REQUIRE(Geo::checkCache(curve.cache, curve.definition, curve.worldTolerance));
    }
    const auto wire = Req::encode({Token, op}); REQUIRE(wire);
    auto bytes = Wire::encode(*wire); REQUIRE(bytes.size() > 8);
    bytes.chop(1); helper.reject(bytes);
    op.produce.limits = {}; op.produce.coordinate = {0, 0};
    const auto opposite = helper.run({Token, op});
    CHECK(std::get<Reply::Produced>(opposite.result).source.reusedCacheCopies == 0);
}
