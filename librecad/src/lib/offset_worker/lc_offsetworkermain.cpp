/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerkernel.h"
#include "lc_offsetworkerwire.h"
#include "rs_debug.h"
#include "rs_settings.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <array>
#include <cstdio>
#include <cstring>

#ifdef LC_OFFSET_WORKER_GEOMETRY_TIMEOUT_TEST_ONLY
#include <csignal>
#include <cstdlib>
#endif

namespace {
void prefix(const LC_OffsetWorkerKernel::Trace& trace) noexcept {
    const auto& c = trace.checkWork;
    const auto& t = trace.throughWork;
    const auto& w = trace.trimWork;
    // A diagnostic prefix is not a refund, a final work total or a proof token.
    std::fprintf(stderr, "LC_OFFSET_PREFIX completed=%d native_prepare=%d check=%d through=%d trim=%d "
        "c_spans=%llu c_vertices=%llu c_pairs=%llu c_order=%llu c_depth=%u "
        "through_spans=%llu through_vertices=%llu through_pairs=%llu through_order=%llu through_depth=%u "
        "trim_input=%llu trim_records=%llu trim_items=%llu trim_pairs=%llu\n",
        int(trace.computationCompleted), int(trace.nativePrepareEntered), int(trace.checkEntered),
        int(trace.throughEntered), int(trace.trimEntered),
        static_cast<unsigned long long>(c.cacheSpans), static_cast<unsigned long long>(c.vertices),
        static_cast<unsigned long long>(c.distancePairs), static_cast<unsigned long long>(c.orderingComparisons), c.deepest,
        static_cast<unsigned long long>(t.cacheSpans), static_cast<unsigned long long>(t.vertices),
        static_cast<unsigned long long>(t.distancePairs), static_cast<unsigned long long>(t.orderingComparisons), t.deepest,
        static_cast<unsigned long long>(w.inputSpans), static_cast<unsigned long long>(w.inputRecords),
        static_cast<unsigned long long>(w.kernelItems), static_cast<unsigned long long>(w.intersections.totalBoxPairs));
}

#ifdef LC_OFFSET_WORKER_GEOMETRY_TIMEOUT_TEST_ONLY
// Qualification fixture only: deliberately nonreturning, repeatedly executing
// real numeric work after witnessed geometry entry. This does NOT reproduce a
// naturally hung numeric primitive and is NOT an OS sandbox. No product input,
// environment variable or command-line option can enable this compiled-out path.
[[noreturn]] void geometryTimeoutFixture(const LC_OffsetWorkerRequest::Request& input,
                                       LC_OffsetWorkerKernel::Trace& trace) {
    bool witnessed = false;
    for (;;) {
        const auto reply = LC_OffsetWorkerKernel::run(input, &trace);
        const auto* native = std::get_if<LC_OffsetWorkerResult::Native>(&reply.result);
        if (witnessed || reply.failure != LC_OffsetWorkerResult::Failure::None || !native ||
            !trace.computationCompleted || !trace.nativePrepareEntered || !trace.checkEntered ||
            native->refusal != LC_OffsetCurveRefusal::None || !native->changed || !native->curve ||
            native->engineRuns == 0 || native->engineStatus != LC_CurveOffsetStatus::Ok ||
            native->checkReason != LC_OffsetCheckResult::Reason::None ||
            trace.checkWork.cacheSpans == 0 || trace.checkWork.vertices == 0) continue;

        // Ignore termination only AFTER a real successful engine + C pass, and
        // before publishing its witness, so POSIX must exercise kill escalation.
        int sigtermIgnored = 0;
#ifdef Q_OS_UNIX
        if (std::signal(SIGTERM, SIG_IGN) == SIG_ERR) std::abort();
        sigtermIgnored = 1;
#endif
        prefix(trace);
        const auto& c = trace.checkWork;
        std::fprintf(stderr, "LC_OFFSET_GEOMETRY_ENTERED completed=%d native_prepare=%d check=%d "
            "engine_runs=%zu engine_status=%d check_reason=%d "
            "c_spans=%llu c_vertices=%llu c_pairs=%llu c_order=%llu c_depth=%u sigterm_ignored=%d\n",
            int(trace.computationCompleted), int(trace.nativePrepareEntered), int(trace.checkEntered),
            native->engineRuns, int(*native->engineStatus), int(*native->checkReason),
            static_cast<unsigned long long>(c.cacheSpans), static_cast<unsigned long long>(c.vertices),
            static_cast<unsigned long long>(c.distancePairs), static_cast<unsigned long long>(c.orderingComparisons),
            c.deepest, sigtermIgnored);
        std::fflush(stderr);
        witnessed = true; // Exactly one bounded diagnostic; never write a result frame.
    }
}
#endif
}

int main(int argc, char** argv) {
    if (argc != 2 || std::strcmp(argv[1], "--protocol=3") != 0) {
        std::fputs("offset-worker: invalid invocation\n", stderr);
        return 2;
    }
    LC_OffsetWorkerKernel::Trace trace;
    try {
        QCoreApplication application(argc, argv);
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCADOffsetWorker");
        // Library display/default lookups must not read or write the user's profile.
        QTemporaryDir settings;
        if (!settings.isValid()) return 6;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settings.path());
        RS_Settings::init("LibreCAD", "LibreCADOffsetWorker");
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

        QFile input;
        if (!input.open(stdin, QIODevice::ReadOnly)) return 3;
        LC_OffsetWorkerWire::Receiver receiver;
        std::array<char, 65536> chunk{};
        for (;;) {
            const auto size = input.read(chunk.data(), qsizetype(chunk.size()));
            if (size < 0) return 3;
            if (size == 0) break;
            if (!receiver.append(QByteArray(chunk.data(), size))) {
                std::fputs("offset-worker: invalid input frame\n", stderr);
                return 2;
            }
        }
        const auto message = receiver.finish();
        if (!message) {
            std::fputs("offset-worker: incomplete or invalid input\n", stderr);
            return 2;
        }
        const auto request = LC_OffsetWorkerRequest::decode(message.fields);
        if (!request) {
            std::fputs("offset-worker: invalid request\n", stderr);
            return 2;
        }
#ifdef LC_OFFSET_WORKER_GEOMETRY_TIMEOUT_TEST_ONLY
        if (std::holds_alternative<LC_OffsetWorkerRequest::Generate>(request->operation))
            geometryTimeoutFixture(*request, trace);
#endif
        const auto reply = LC_OffsetWorkerKernel::run(*request, &trace);
        if (reply.failure != LC_OffsetWorkerResult::Failure::None) prefix(trace);
        const auto fields = LC_OffsetWorkerResult::encode(reply, *request);
        if (!fields) {
            prefix(trace);
            std::fputs("offset-worker: result admission failed\n", stderr);
            return 4;
        }
        const auto frame = LC_OffsetWorkerWire::encode(*fields);
        if (frame.isEmpty()) {
            prefix(trace);
            std::fputs("offset-worker: result framing failed\n", stderr);
            return 4;
        }
        QFile output;
        if (!output.open(stdout, QIODevice::WriteOnly)) return 5;
        qsizetype written = 0;
        while (written < frame.size()) {
            const auto size = output.write(frame.constData() + written, frame.size() - written);
            if (size <= 0) return 5;
            written += size;
        }
        return output.flush() ? 0 : 5;
    } catch (const std::bad_alloc&) {
        prefix(trace);
        std::fputs("offset-worker: allocation failure; total work unknown\n", stderr);
        return 6;
    } catch (...) {
        prefix(trace);
        std::fputs("offset-worker: internal failure; total work unknown\n", stderr);
        return 6;
    }
}
