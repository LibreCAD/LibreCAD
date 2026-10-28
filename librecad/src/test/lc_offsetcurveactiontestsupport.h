/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <algorithm>

#include <catch2/catch_test_macros.hpp>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QPointer>
#include <QScopeGuard>
#include <QTimer>

#include "lc_action_offsetcurve_updatesource.h"
#include "lc_action_modify_offset.h"
#include "../lib/offset_worker/lc_offsetworkerclient.h"

namespace lc::test {

inline void awaitCreation(LC_ActionModifyOffset& action) {
    if (!action.isPreparing()) return;
    REQUIRE(QFileInfo(LC_OffsetWorkerClient::helperPath()).isExecutable());
    const QPointer<LC_ActionModifyOffset> alive(&action);
    QEventLoop loop; QTimer poll, watchdog; bool expired = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (!alive || !alive->isPreparing()) loop.quit();
    });
    QObject::connect(&watchdog, &QTimer::timeout, &loop, [&] { expired = true; loop.quit(); });
    const auto cancelOnUnwind = qScopeGuard([&] { if (alive && alive->isPreparing()) alive->suspend(); });
    watchdog.setSingleShot(true); watchdog.start(35000); poll.start(1); loop.exec();
    INFO("Creation worker exceeded its bounded production deadline plus reap allowance");
    REQUIRE_FALSE(expired); REQUIRE(alive); REQUIRE_FALSE(alive->isPreparing());
}

// Test-only event pumping; the action retains asynchronous production ownership.
inline void awaitUpdate(LC_ActionOffsetCurveUpdateSource& action) {
    if (!action.isPreparing()) return;
    REQUIRE(QFileInfo(LC_OffsetWorkerClient::helperPath()).isExecutable());
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(&action);
    QElapsedTimer total, peer; total.start(); peer.start();
    const auto peers = std::max<qsizetype>(1, action.lastReport().outcomes.size());
    const auto totalLimit = std::min<qint64>(LC_OffsetWorkerClient::Deadlines::commit, peers * 3500);
    qsizetype runningPeer = -1;
    QEventLoop loop;
    QTimer poll; bool expired = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (!alive || !alive->isPreparing()) { loop.quit(); return; }
        if (total.elapsed() >= totalLimit || peer.elapsed() >= 3500) { expired = true; loop.quit(); return; }
        const auto& outcomes = alive->lastReport().outcomes;
        for (qsizetype i = 0; i < outcomes.size(); ++i) {
            const auto& outcome = outcomes.at(i);
            if (outcome.workerReservation && !outcome.edit && outcome.refusal.isEmpty() && i != runningPeer) {
                runningPeer = i; peer.restart(); break;
            }
        }
    });
    const auto cancelOnUnwind = qScopeGuard([&] { if (alive && alive->isPreparing()) alive->suspend(); });
    poll.start(1); loop.exec();
    INFO("Update worker exceeded its 3500ms per-peer / shared 30000ms GUI-test bounds");
    REQUIRE_FALSE(expired); REQUIRE(alive); REQUIRE_FALSE(alive->isPreparing());
}

}
