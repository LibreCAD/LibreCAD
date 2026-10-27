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
}

int main(int argc, char** argv) {
    if (argc != 2 || std::strcmp(argv[1], "--protocol=2") != 0) {
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
