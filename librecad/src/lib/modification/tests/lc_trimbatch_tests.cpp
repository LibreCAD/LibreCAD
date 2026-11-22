#include <catch2/catch_test_macros.hpp>

#include "lc_trimbatch.h"
#include "rs_document.h"
#include "rs_line.h"

TEST_CASE("batch trim keeps the side containing the cursor", "[modify][trim][batch]") {
    RS_Line first{nullptr, {0.0, -2.0}, {10.0, -2.0}};
    RS_Line second{nullptr, {0.0, 2.0}, {10.0, 2.0}};
    RS_Line limit{nullptr, {5.0, -5.0}, {5.0, 5.0}};
    QList<RS_Entity*> targets{&first, &second};
    LC_TrimLimitSet limits;
    limits.add(limit);
    limits.build();
    LC_TrimBatch batch{targets, limits};
    REQUIRE(batch.anyCut());

    const auto sides = batch.cursorSides({8.0, 0.0});
    const auto removed = batch.removedPieces(sides);
    REQUIRE(removed.size() == 2);
    CHECK(removed[0]->getEndpoint().distanceTo({5.0, -2.0}) < 1e-7);
    CHECK(removed[1]->getEndpoint().distanceTo({5.0, 2.0}) < 1e-7);
}

TEST_CASE("batch trim leaves targets that do not intersect unchanged", "[modify][trim][batch]") {
    RS_Line target{nullptr, {0.0, 2.0}, {4.0, 2.0}};
    RS_Line limit{nullptr, {5.0, -5.0}, {5.0, 5.0}};
    LC_TrimLimitSet limits;
    limits.add(limit);
    limits.build();
    LC_TrimBatch batch{{&target}, limits};
    CHECK_FALSE(batch.anyCut());
}

TEST_CASE("batch trim applies all changed targets in one document batch", "[modify][trim][batch]") {
    RS_Line first{nullptr, {0.0, -2.0}, {10.0, -2.0}};
    RS_Line second{nullptr, {0.0, 2.0}, {10.0, 2.0}};
    RS_Line limit{nullptr, {5.0, -5.0}, {5.0, 5.0}};
    LC_TrimLimitSet limits;
    limits.add(limit);
    limits.build();
    LC_TrimBatch batch{{&first, &second}, limits};
    LC_DocumentModificationBatch context;

    const auto stats = batch.apply(batch.cursorSides({8.0, 0.0}), false, context);
    CHECK(stats.trimmed == 2);
    CHECK(context.entitiesToDelete.size() == 2);
    CHECK(context.entitiesToAdd.size() == 2);
    CHECK(context.entitiesToDelete.contains(&first));
    CHECK(context.entitiesToDelete.contains(&second));
    for (RS_Entity* entity : context.entitiesToAdd) delete entity;
}

TEST_CASE("a target tangent to a closed chain corner stays on the same side", "[modify][trim][batch]") {
    RS_EntityContainer rectangle;
    rectangle.addEntity(new RS_Line(&rectangle, {-5.0, -5.0}, {5.0, -5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {5.0, -5.0}, {5.0, 5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {5.0, 5.0}, {-5.0, 5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {-5.0, 5.0}, {-5.0, -5.0}));
    LC_TrimLimitSet limits;
    limits.add(rectangle);
    limits.build();
    RS_Line tangent{nullptr, {-4.0, 6.0}, {-6.0, 4.0}};
    LC_TrimBatch batch{{&tangent}, limits};

    REQUIRE(batch.anyCut());
    CHECK(limits.isClosed(0));
    CHECK(batch.removedPieces(batch.cursorSides({-8.0, 8.0})).empty());
}

TEST_CASE("a target crossing a closed chain at a corner changes sides", "[modify][trim][batch]") {
    RS_EntityContainer rectangle;
    rectangle.addEntity(new RS_Line(&rectangle, {-5.0, -5.0}, {5.0, -5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {5.0, -5.0}, {5.0, 5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {5.0, 5.0}, {-5.0, 5.0}));
    rectangle.addEntity(new RS_Line(&rectangle, {-5.0, 5.0}, {-5.0, -5.0}));
    LC_TrimLimitSet limits;
    limits.add(rectangle);
    limits.build();
    RS_Line crossing{nullptr, {-6.0, 6.0}, {-4.0, 4.0}};
    LC_TrimBatch batch{{&crossing}, limits};

    const auto removed = batch.removedPieces(batch.cursorSides({0.0, 0.0}));
    REQUIRE(removed.size() == 1);
    CHECK(removed[0]->getEndpoint().distanceTo({-5.0, 5.0}) < 1e-7);
}

TEST_CASE("two limit components trim to the region containing the cursor", "[modify][trim][batch]") {
    RS_Line target{nullptr, {0.0, 0.0}, {10.0, 0.0}};
    RS_Line firstLimit{nullptr, {3.0, -2.0}, {3.0, 2.0}};
    RS_Line secondLimit{nullptr, {7.0, -2.0}, {7.0, 2.0}};
    LC_TrimLimitSet limits;
    limits.add(firstLimit);
    limits.add(secondLimit);
    limits.build();
    LC_TrimBatch batch{{&target}, limits};

    const auto removed = batch.removedPieces(batch.cursorSides({5.0, 0.0}));
    REQUIRE(removed.size() == 2);
    CHECK(removed[0]->getEndpoint().distanceTo({3.0, 0.0}) < 1e-7);
    CHECK(removed[1]->getEndpoint().distanceTo({10.0, 0.0}) < 1e-7);
}

TEST_CASE("a cut close to the end of a short limit is classified locally", "[modify][trim][batch]") {
    RS_Line target{nullptr, {0.0, 0.9}, {10.0, 0.9}};
    RS_Line shortLimit{nullptr, {5.0, -1.0}, {5.0, 1.0}};
    LC_TrimLimitSet limits;
    limits.add(shortLimit);
    limits.build();
    LC_TrimBatch batch{{&target}, limits};

    const auto removed = batch.removedPieces(batch.cursorSides({8.0, 2.0}));
    REQUIRE(removed.size() == 1);
    CHECK(removed[0]->getEndpoint().distanceTo({5.0, 0.9}) < 1e-7);
}

TEST_CASE("review toggles can be undone and identify hovered pieces", "[modify][trim][batch]") {
    RS_Line target{nullptr, {0.0, 0.0}, {10.0, 0.0}};
    RS_Line limit{nullptr, {5.0, -2.0}, {5.0, 2.0}};
    LC_TrimLimitSet limits;
    limits.add(limit);
    limits.build();
    LC_TrimBatch batch{{&target}, limits};
    const auto cursor = batch.cursorSides({8.0, 0.0});
    int targetIndex = -1;
    int pieceIndex = -1;
    REQUIRE(batch.pieceAt({2.0, 0.0}, 1.0, targetIndex, pieceIndex));
    CHECK(targetIndex == 0);
    CHECK(pieceIndex == 0);
    CHECK(batch.verdict(0, 0, cursor) == LC_TrimBatch::Verdict::Removed);
    batch.toggle(0, 0);
    CHECK(batch.verdict(0, 0, cursor) == LC_TrimBatch::Verdict::Kept);
    CHECK(batch.undoToggle());
    CHECK(batch.verdict(0, 0, cursor) == LC_TrimBatch::Verdict::Removed);
    CHECK_FALSE(batch.undoToggle());
}
