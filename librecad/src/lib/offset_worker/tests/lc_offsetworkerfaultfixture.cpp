/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerresult.h"
#include "lc_offsetworkerwire.h"

#include <QCborValue>
#include <QCoreApplication>
#include <QFile>
#include <QThread>
#include <array>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <utility>

namespace {
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Wire = LC_OffsetWorkerWire;

// Mirrored in lc_offsetworkerfault_tests.cpp. Only the isolated test target
// installs this executable as LibreCADOffsetWorker, never the application target.
enum class Fault {
    Refusal = 1, NonzeroExit, Crash, EmptyResponse, MalformedCbor,
    TruncatedHeader, TruncatedPayload, DuplicateResponse, ExtraByte,
    WrongRequestId, WrongGeneration, WrongSourceRevision, WrongUndoEpoch,
    WrongOperation, AllocationFailure, InternalFailure, StderrOverflow,
    Hang, CompleteThenHang, LateComplete, CompleteThenNonzero
};
constexpr qsizetype InputCap = 64 * 1024;
constexpr unsigned long HangMs = 10000;
constexpr unsigned long LateMs = 2040; // Beyond the public 2-second preview limit.

bool writeAll(QFile& output, const QByteArray& bytes) {
    qsizetype written = 0;
    while (written < bytes.size()) {
        const auto n = output.write(bytes.constData() + written, bytes.size() - written);
        if (n <= 0) return false;
        written += n;
    }
    return output.flush();
}

int run() {
    QFile input, output, diagnostic;
    if (!input.open(stdin, QIODevice::ReadOnly) || !output.open(stdout, QIODevice::WriteOnly) ||
        !diagnostic.open(stderr, QIODevice::WriteOnly)) return 2;
    Wire::Limits limits;
    limits.frameBytes = InputCap;
    limits.items = 4096;
    limits.arrayItems = 1024;
    limits.depth = 16;
    Wire::Receiver receiver{limits};
    std::array<char, 4096> chunk{};
    for (;;) {
        const auto n = input.read(chunk.data(), qsizetype(chunk.size()));
        if (n < 0) return 2;
        if (!n) break;
        if (!receiver.append(QByteArray(chunk.data(), n))) return 2;
    }
    const auto message = receiver.finish();
    if (!message) return 2;
    const auto request = Req::decode(message.fields);
    if (!request) return 2;
    const auto* generate = std::get_if<Req::Generate>(&request->operation);
    if (!generate) return 2;
    const auto& definition = generate->definition;
    // A zero major axis is admitted wire syntax but semantically invalid. Unlike
    // a negative distance, it also passes the current parent's request admission.
    if (definition.basis.majorP.x != 0 || definition.basis.majorP.y != 0 ||
        definition.distance < double(Fault::Refusal) ||
        definition.distance > double(Fault::CompleteThenNonzero)) return 2;
    const auto selector = int(definition.distance);
    if (definition.distance != double(selector)) return 2;
    const auto fault = static_cast<Fault>(selector);
#ifdef Q_OS_UNIX
    if (fault == Fault::CompleteThenHang) std::signal(SIGTERM, SIG_IGN);
#endif
    if (!writeAll(diagnostic, QByteArray("LC_OFFSET_FAULT_FIXTURE received=") +
        QByteArray::number(selector) + '\n')) return 2;

    Reply::Reply reply;
    reply.token = request->token;
    reply.operation = request->operation.index();
    reply.failure = Reply::Failure::None;
    // This is an early typed refusal, not fabricated geometry, work or authority.
    Reply::Native refusal;
    refusal.refusal = LC_OffsetCurveRefusal::InvalidInput;
    reply.result = std::move(refusal);
    if (fault == Fault::AllocationFailure || fault == Fault::InternalFailure) {
        reply.failure = fault == Fault::AllocationFailure ? Reply::Failure::AllocationFailure :
            Reply::Failure::InternalFailure;
        reply.result = std::monostate{};
    }
    auto fields = Reply::encode(reply, *request);
    if (!fields) return 2;
    if (fault >= Fault::WrongRequestId && fault <= Fault::WrongUndoEpoch) {
        auto token = fields->at(1).toArray();
        const auto index = int(fault) - int(Fault::WrongRequestId);
        token[index] = qint64(token.at(index).toInteger() == 1 ? 2 : 1);
        (*fields)[1] = token;
    } else if (fault == Fault::WrongOperation) (*fields)[2] = 1;
    auto frame = Wire::encode(*fields);
    if (frame.isEmpty()) return 2;
    switch (fault) {
    case Fault::NonzeroExit: return 23;
    case Fault::Crash:
#ifdef Q_OS_UNIX
        std::raise(SIGKILL); // CrashExit without a core file or crash-report dialog.
#else
        std::signal(SIGABRT, SIG_DFL);
        std::raise(SIGABRT);
#endif
        return 24;
    case Fault::EmptyResponse: return 0;
    case Fault::MalformedCbor: frame = QByteArray::fromHex("00000001ff"); break;
    case Fault::TruncatedHeader: frame.truncate(3); break;
    case Fault::TruncatedPayload: frame.chop(1); break;
    case Fault::DuplicateResponse: frame += frame; break;
    case Fault::ExtraByte: frame.append('\0'); break;
    case Fault::StderrOverflow:
        if (!writeAll(diagnostic, QByteArray(65537, 'x'))) return 2;
        break;
    case Fault::Hang:
        QThread::msleep(HangMs);
        return 0;
    case Fault::LateComplete:
        QThread::msleep(LateMs);
        break;
    default: break;
    }
    if (!writeAll(output, frame)) return 2;
    if (fault == Fault::CompleteThenHang) {
        if (!writeAll(diagnostic, QByteArray("LC_OFFSET_FAULT_STALLED\n"))) return 2;
        QThread::msleep(HangMs);
    }
    return fault == Fault::CompleteThenNonzero ? 23 : 0;
}
}

int main(int argc, char** argv) {
    if (argc != 2 || std::strcmp(argv[1], "--protocol=4") != 0) return 2;
    try {
        QCoreApplication application(argc, argv);
        return run();
    } catch (...) { return 2; }
}
