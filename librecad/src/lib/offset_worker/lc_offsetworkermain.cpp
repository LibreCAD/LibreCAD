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
#include <QSettings>
#include <QTemporaryDir>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef LC_OFFSET_WORKER_GEOMETRY_TIMEOUT_TEST_ONLY
#include <csignal>
#include <cstdlib>
#endif

namespace {
bool readInput(char* data, qsizetype capacity, qsizetype& size) noexcept {
#ifdef Q_OS_WIN
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (!input || input == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0;
    if (!ReadFile(input, data, DWORD(capacity), &count, nullptr)) {
        if (GetLastError() == ERROR_BROKEN_PIPE) { size = 0; return true; }
        return false;
    }
    size = qsizetype(count);
    return true;
#else
    for (;;) {
        const auto count = ::read(STDIN_FILENO, data, std::size_t(capacity));
        if (count >= 0) { size = qsizetype(count); return true; }
        if (errno != EINTR) return false;
    }
#endif
}

bool writeOutput(const QByteArray& data) noexcept {
    qsizetype offset = 0;
#ifdef Q_OS_WIN
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!output || output == INVALID_HANDLE_VALUE) return false;
    while (offset < data.size()) {
        const auto chunk = DWORD(std::min<qsizetype>(data.size() - offset, 64 * 1024));
        DWORD written = 0;
        if (!WriteFile(output, data.constData() + offset, chunk, &written, nullptr) || written == 0) return false;
        offset += qsizetype(written);
    }
#else
    while (offset < data.size()) {
        const auto count = ::write(STDOUT_FILENO, data.constData() + offset,
                                   std::size_t(data.size() - offset));
        if (count > 0) offset += qsizetype(count);
        else if (count < 0 && errno == EINTR) continue;
        else return false;
    }
#endif
    return true;
}

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
    if (argc != 2 || std::strcmp(argv[1], "--protocol=4") != 0) {
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

        LC_OffsetWorkerWire::Receiver receiver;
        std::array<char, 65536> chunk{};
        for (;;) {
            qsizetype size = 0;
            if (!readInput(chunk.data(), qsizetype(chunk.size()), size)) return 3;
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
        return writeOutput(frame) ? 0 : 5;
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
