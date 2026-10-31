/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <QGuiApplication>
#include <QThread>
#include <array>
#include <memory>
#include <stdexcept>
#include "lc_actiontestsupport.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"

TEST_CASE("Busy cursor scopes preserve preexisting nested overrides and unwind exceptions", "[gui][offsetcurve][cursor]") {
    (void)lc::test::application();
    const auto* before = QGuiApplication::overrideCursor();
    const auto oldShape = before == nullptr ? Qt::ArrowCursor : before->shape();
    QGuiApplication::setOverrideCursor(Qt::CrossCursor);
    {
        const LC_BusyCursorGuard outer;
        REQUIRE(QGuiApplication::overrideCursor());
        CHECK(QGuiApplication::overrideCursor()->shape() == Qt::WaitCursor);
        try {
            const LC_BusyCursorGuard inner;
            REQUIRE(QGuiApplication::overrideCursor());
            CHECK(QGuiApplication::overrideCursor()->shape() == Qt::WaitCursor);
            throw std::runtime_error("cursor unwind");
        }
        catch (const std::runtime_error&) {}
        CHECK(QGuiApplication::overrideCursor()->shape() == Qt::WaitCursor);
    }
    REQUIRE(QGuiApplication::overrideCursor());
    CHECK(QGuiApplication::overrideCursor()->shape() == Qt::CrossCursor);
    QGuiApplication::restoreOverrideCursor();
    if (before == nullptr) CHECK(QGuiApplication::overrideCursor() == nullptr);
    else {
        REQUIRE(QGuiApplication::overrideCursor());
        CHECK(QGuiApplication::overrideCursor()->shape() == oldShape);
    }
}

TEST_CASE("Busy cursor guard does not push an override on a worker thread", "[gui][offsetcurve][cursor]") {
    (void)lc::test::application();
    QGuiApplication::setOverrideCursor(Qt::CrossCursor);
    auto worker = std::unique_ptr<QThread>(QThread::create([] {
        const LC_BusyCursorGuard ignored;
    }));
    worker->start();
    REQUIRE(worker->wait(5000));
    REQUIRE(QGuiApplication::overrideCursor());
    CHECK(QGuiApplication::overrideCursor()->shape() == Qt::CrossCursor);
    QGuiApplication::restoreOverrideCursor();
}

TEST_CASE("Native refusal text preserves absent stages and distinct producer checker failures", "[gui][offsetcurve][refusal]") {
    using Refusal = LC_OffsetCurveRefusal;
    for (auto refusal : {Refusal::None, Refusal::InvalidInput, Refusal::Undefined, Refusal::Inconsistent,
                        Refusal::TooSmall, Refusal::NothingLeft, Refusal::SeveralPieces,
                        Refusal::NotCacheRepresentable, Refusal::CheckFailed, Refusal::Engine,
                        Refusal::ThroughNotOnCache}) {
        LC_OffsetCurveEditResult result;
        result.refusal = refusal;
        const auto text = LC_OffsetCurveUI::refusalText(result);
        CHECK_FALSE(text.isEmpty());
        CHECK_FALSE(text.contains("Producer:"));
        CHECK_FALSE(text.contains("Checker:"));
    }
    LC_OffsetCurveEditResult producer;
    producer.refusal = Refusal::Engine;
    producer.engineStatus = LC_CurveOffsetStatus::LimitExceeded;
    const auto producerText = LC_OffsetCurveUI::refusalText(producer);
    CHECK(producerText.contains("bounded producer limit"));
    CHECK_FALSE(producerText.contains("Checker:"));

    LC_OffsetCurveEditResult checker;
    checker.refusal = Refusal::CheckFailed;
    checker.engineStatus = LC_CurveOffsetStatus::Ok;
    checker.checkReason = LC_OffsetCheckResult::Reason::LimitExceeded;
    const auto checkerText = LC_OffsetCurveUI::refusalText(checker);
    CHECK(checkerText.contains("bounded cache validation limit"));
    CHECK(checkerText.contains("Producer:"));
    CHECK(checkerText.contains("Checker:"));
    CHECK(checkerText != producerText);

    for (auto status : {LC_CurveOffsetStatus::Ok, LC_CurveOffsetStatus::InvalidRequest,
                        LC_CurveOffsetStatus::InvalidSource, LC_CurveOffsetStatus::UnsupportedNonPlanar,
                        LC_CurveOffsetStatus::InvalidDistance, LC_CurveOffsetStatus::UnsupportedClosedResult,
                        LC_CurveOffsetStatus::AmbiguousSide, LC_CurveOffsetStatus::UndefinedTangent,
                        LC_CurveOffsetStatus::DiscontinuousNormal, LC_CurveOffsetStatus::SingularOffset,
                        LC_CurveOffsetStatus::AmbiguousRegularity, LC_CurveOffsetStatus::AmbiguousTopology,
                        LC_CurveOffsetStatus::FitFailed, LC_CurveOffsetStatus::ToleranceNotMet,
                        LC_CurveOffsetStatus::LimitExceeded}) {
        producer.engineStatus = status;
        CHECK_FALSE(LC_OffsetCurveUI::refusalText(producer).contains("unknown"));
    }
    using Reason = LC_OffsetCheckResult::Reason;
    for (auto reason : {Reason::None, Reason::InvalidInput, Reason::Layout, Reason::TooSmall,
                        Reason::ReferenceUnresolved, Reason::OffCurve, Reason::Ambiguous,
                        Reason::Topology, Reason::LimitExceeded}) {
        checker.checkReason = reason;
        CHECK_FALSE(LC_OffsetCurveUI::refusalText(checker).contains("unknown"));
    }
}

TEST_CASE("Native sheet limits explicitly bound both geometry stages", "[gui][offsetcurve][limits]") {
    const auto limits = LC_OffsetCurveUI::sheetLimits();
    const LC_OffsetCheckLimits hard;
    CHECK(limits.maxSamples > 0);
    CHECK(limits.maxSamples <= LC_CurveOffset::kDefaultMaxSamples);
    CHECK(limits.maxIntersectionPairs > 0);
    CHECK(limits.maxIntersectionPairs <= LC_CurveOffset::kDefaultMaxIntersectionPairs);
    CHECK(limits.checkLimits.maxCacheSpans > 0);
    CHECK(limits.checkLimits.maxCacheSpans <= hard.maxCacheSpans);
    CHECK(limits.checkLimits.maxVertices > 0);
    CHECK(limits.checkLimits.maxVertices <= hard.maxVertices);
    CHECK(limits.checkLimits.maxDistancePairs > 0);
    CHECK(limits.checkLimits.maxDistancePairs <= hard.maxDistancePairs);
    CHECK(limits.checkLimits.maxDepth > 0);
    CHECK(limits.checkLimits.maxDepth <= hard.maxDepth);
    CHECK(limits.checkLimits.maxOrderingComparisons > 0);
    CHECK(limits.checkLimits.maxOrderingComparisons <= hard.maxOrderingComparisons);
}
