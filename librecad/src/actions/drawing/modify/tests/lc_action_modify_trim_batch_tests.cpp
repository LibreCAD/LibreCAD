#include <catch2/catch_test_macros.hpp>

#include "lc_action_modify_trim_batch.h"
#include "lc_actiontestsupport.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_selection.h"

namespace {
using lc::test::ActionFixture;
using lc::test::eventAt;

class TrimBatchProbe final : public LC_ActionModifyTrimBatch {
public:
    using LC_ActionModifyTrimBatch::LC_ActionModifyTrimBatch;
    using LC_ActionModifyTrimBatch::SelectLimit;
    using LC_ActionModifyTrimBatch::SelectTargets;
    using LC_ActionModifyTrimBatch::ChooseSide;

    void click(const RS_Vector& point) {
        LC_MouseEvent event = eventAt(point.x, point.y);
        onMouseLeftButtonRelease(getStatus(), &event);
    }
};

RS_Line* addLine(RS_Graphic& graphic, const RS_Vector& start, const RS_Vector& end) {
    auto* line = new RS_Line(&graphic, RS_LineData(start, end));
    graphic.addEntity(line);
    return line;
}

RS_Entity* lastEntity(const RS_Graphic& graphic) {
    return graphic.entityAt(static_cast<int>(graphic.count()) - 1);
}

bool canUndo(const RS_Graphic& graphic) {
    bool undo = false;
    bool redo = false;
    graphic.collectUndoState(undo, redo);
    return undo;
}
}

TEST_CASE("batch trim selection, limit and side clicks use one undo cycle", "[actions][modify][trim][batch]") {
    ActionFixture<TrimBatchProbe> fixture;
    RS_Line* target = addLine(fixture.m_graphic, {0.0, 0.0}, {10.0, 0.0});
    addLine(fixture.m_graphic, {5.0, -5.0}, {5.0, 5.0});
    fixture.m_graphic.select(QList<RS_Entity*>{target});
    fixture.m_action->init(TrimBatchProbe::SelectTargets);

    REQUIRE(fixture.m_action->getStatus() == TrimBatchProbe::SelectLimit);
    fixture.m_action->click({5.0, 3.0});
    REQUIRE(fixture.m_action->getStatus() == TrimBatchProbe::ChooseSide);
    fixture.m_action->click({8.0, 0.0});

    CHECK(target->isDeleted());
    REQUIRE(lastEntity(fixture.m_graphic) != nullptr);
    CHECK(lastEntity(fixture.m_graphic)->getStartpoint().distanceTo({5.0, 0.0}) < 1e-7);
    CHECK(lastEntity(fixture.m_graphic)->getEndpoint().distanceTo({10.0, 0.0}) < 1e-7);
    CHECK(fixture.m_action->getStatus() == TrimBatchProbe::SelectLimit);
    CHECK(canUndo(fixture.m_graphic));

    REQUIRE(fixture.m_graphic.undo());
    CHECK_FALSE(target->isDeleted());
    CHECK_FALSE(canUndo(fixture.m_graphic));
}
