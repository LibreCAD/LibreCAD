/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>

#include <QEventLoop>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

#include "lc_action_block_library_insert.h"
#include "lc_actiontestsupport.h"
#include "../../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_preview.h"

namespace {

class BlockInsertProbe final : public LC_ActionBlockLibraryInsert {
public:
    explicit BlockInsertProbe(LC_ActionContext* context) : LC_ActionBlockLibraryInsert(context) {}
    using LC_ActionBlockLibraryInsert::SetTargetPoint;
    using LC_ActionBlockLibraryInsert::onCoordinateEvent;
    using LC_ActionBlockLibraryInsert::onMouseMoveEvent;
    using RS_PreviewActionInterface::m_preview;
};

void waitForImport(BlockInsertProbe& action) {
    if (!action.isSourceLoading()) return;
    QEventLoop loop;
    QTimer poll, watchdog;
    bool expired = false;
    const QPointer<BlockInsertProbe> alive(&action);
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (!alive || !alive->isSourceLoading()) loop.quit();
    });
    QObject::connect(&watchdog, &QTimer::timeout, &loop, [&] {
        expired = true;
        loop.quit();
    });
    poll.start(1);
    watchdog.setSingleShot(true);
    watchdog.start(35000);
    loop.exec();
    INFO("Staged block-library import exceeded the bounded worker deadline and reap allowance");
    REQUIRE_FALSE(expired);
    REQUIRE(alive);
    REQUIRE_FALSE(alive->isSourceLoading());
}

} // namespace

TEST_CASE("Block library insertion waits for staged native offset import before commit",
          "[offset-import][library-insert][worker]") {
    REQUIRE(lc::test::application() != nullptr);
    const QString helper = LC_OffsetWorkerClient::helperPath();
    INFO("Expected the built offset helper beside the test executable: " << helper.toStdString());
    REQUIRE(QFileInfo(helper).isExecutable());

    lc::test::ActionFixture<BlockInsertProbe> fixture;
    fixture.m_action->init(BlockInsertProbe::SetTargetPoint);
    const QString file = QStringLiteral(LIBRECAD_TEST_DIR) +
        QStringLiteral("/offsetcurve_independent/dxf-binary-R2018-native-degree1.dxf");
    REQUIRE(QFileInfo(file).isFile());

    fixture.m_action->setFile(file);
    CHECK(fixture.m_action->isSourceLoading());
    CHECK_FALSE(fixture.m_action->hasSource());

    fixture.m_action->onCoordinateEvent(BlockInsertProbe::SetTargetPoint, false, {5, 7});
    CHECK(fixture.m_graphic.count() == 0);

    bool cancelledWhilePending = false;
    QEventLoop cancelLoop;
    QTimer::singleShot(1, fixture.m_action.get(), [&] {
        cancelledWhilePending = fixture.m_action->isSourceLoading();
        fixture.m_action->reset();
        cancelLoop.quit();
    });
    cancelLoop.exec();
    REQUIRE(cancelledWhilePending);
    CHECK_FALSE(fixture.m_action->isSourceLoading());
    CHECK_FALSE(fixture.m_action->hasSource());

    QEventLoop drainLoop;
    QTimer::singleShot(50, &drainLoop, &QEventLoop::quit);
    drainLoop.exec();
    CHECK_FALSE(fixture.m_action->hasSource());

    fixture.m_action->setFile(file);
    REQUIRE(fixture.m_action->isSourceLoading());
    waitForImport(*fixture.m_action);
    REQUIRE(fixture.m_action->hasSource());
    CHECK_FALSE(fixture.m_action->isSourceLoading());

    const auto move = lc::test::eventAt(5, 7);
    fixture.m_action->onMouseMoveEvent(BlockInsertProbe::SetTargetPoint, &move);
    REQUIRE(fixture.m_action->m_preview != nullptr);
    CHECK(fixture.m_action->m_preview->count() > 0);

    fixture.m_action->onCoordinateEvent(BlockInsertProbe::SetTargetPoint, false, {5, 7});
    const QString blockName = QFileInfo(file).completeBaseName();
    const RS_Block* inserted = fixture.m_graphic.findBlock(blockName);
    REQUIRE(inserted != nullptr);
    bool keptNativeOffset = false;
    for (const RS_Entity* entity : *inserted) {
        if (entity != nullptr && entity->rtti() == RS2::EntityOffsetCurve) {
            keptNativeOffset = true;
            break;
        }
    }
    CHECK(keptNativeOffset);
}
