/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ******************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "lc_filtertestsupport.h"
#include "lc_sourcefonts.h"
#include "lc_testentityfactory.h"
#include "rs.h"
#include "rs_constructionline.h"

namespace {

enum class Kind : unsigned char { NotAnEnumerator, DrawingEntity, NotApplicable };
enum Check : unsigned {
    DxfRoundTrip = 1u << 0,
    DwgRoundTrip = 1u << 1,
    InsertExpansion = 1u << 2,
    CloneClass = 1u << 3
};

struct Row {
    Kind kind;
    const char* reason;
    RS2::EntityType reloadsAs;
    unsigned knownGaps;
    unsigned notRun;
    const char* gapNote;
};

constexpr Row drawing(RS2::EntityType type) {
    return {Kind::DrawingEntity, nullptr, type, 0, 0, nullptr};
}

constexpr Row withGap(Row row, unsigned gaps, const char* note) {
    row.knownGaps = gaps;
    row.gapNote = note;
    return row;
}

constexpr Row notApplicable(const char* reason) {
    return {Kind::NotApplicable, reason, RS2::EntityUnknown, 0, 0, nullptr};
}

constexpr Row notAnEnumerator() {
    return {Kind::NotAnEnumerator, nullptr, RS2::EntityUnknown, 0, 0, nullptr};
}

// The switch also catches appended enumerators, which the row-count assertion cannot see.
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(1 : 4062)
#pragma warning(error : 4062)
#endif
constexpr Row rowOf(RS2::EntityType type) {
    switch (type) {
    case RS2::EntityUnknown: return notApplicable("Fallback sentinel, not a persistent drawing type");
    case RS2::EntityContainer: return notApplicable("JWW-only top-level group; not saved or expanded by INSERT");
    case RS2::EntityBlock: return notApplicable("Block definitions are written separately by writeBlocks");
    case RS2::EntityFontChar: return notApplicable("Font glyph geometry intentionally clones as an RS_Block");
    case RS2::EntityInsert: return drawing(type);
    case RS2::EntityGraphic: return notApplicable("Drawing document, not an entity record");
    case RS2::EntityPoint: return drawing(type);
    case RS2::EntityLine: return drawing(type);
    case RS2::EntityPolyline: return drawing(type);
    case RS2::EntityVertex: return notApplicable("Owned polyline geometry, not a standalone drawing record");
    case RS2::EntityArc: return drawing(type);
    case RS2::EntityCircle: return drawing(type);
    case RS2::EntityEllipse: return drawing(type);
    case RS2::EntityHyperbola: return drawing(type);
    case RS2::EntitySolid: return drawing(type);
    case RS2::EntityConstructionLine: return notApplicable("Nonpersistent construction entity with native INSERT support; XLINE/RAY import as RS_Line");
    case RS2::EntityMText: return drawing(type);
    case RS2::EntityText: return drawing(type);
    case RS2::EntityDimAligned: return drawing(type);
    case RS2::EntityDimLinear: return drawing(type);
    case RS2::EntityDimRadial: return drawing(type);
    case RS2::EntityDimDiametric: return drawing(type);
    case RS2::EntityDimAngular: return drawing(type);
    case RS2::EntityDimArc: return drawing(type);
    case RS2::EntityDimOrdinate: return drawing(type);
    case RS2::EntityTolerance: return drawing(type);
    case RS2::EntityDimLeader: return drawing(type);
    case RS2::EntityHatch: return drawing(type);
    case RS2::EntityImage: return drawing(type);
    case RS2::EntityWipeout: return drawing(type);
    case RS2::EntityMLeader:
        return withGap(drawing(type), DwgRoundTrip,
                       "Generated MLEADER uses LEADER + MTEXT fallback without an imported source handle");
    case RS2::EntitySpline: return drawing(type);
    case RS2::EntitySplinePoints: return drawing(type);
    case RS2::EntityParabola: return drawing(type);
    case RS2::EntityOverlayBox: return notApplicable("Transient overlay geometry, not a standalone drawing record");
    case RS2::EntityPreview: return notApplicable("Transient preview container, not a drawing document entity");
    case RS2::EntityPattern: return notApplicable("Hatch pattern definition, not an independent drawing entity");
    case RS2::EntityOverlayLine: return notApplicable("Transient overlay line, not persistent drawing geometry");
    case RS2::EntityRefPoint: return notApplicable("Transient reference-point overlay, not a drawing entity");
    case RS2::EntityRefLine: return notApplicable("Transient reference-line overlay, not a drawing entity");
    case RS2::EntityRefConstructionLine: return notApplicable("Transient reference construction-line overlay, not a drawing entity");
    case RS2::EntityRefArc: return notApplicable("Transient reference-arc overlay, not a drawing entity");
    case RS2::EntityRefCircle: return notApplicable("Transient reference-circle overlay, not a drawing entity");
    case RS2::EntityRefEllipse: return notApplicable("Transient reference-ellipse overlay, not a drawing entity");
    case RS2::EntitySnapMark: return notApplicable("Transient snap marker, not persistent drawing geometry");
    case RS2::EntitySnapLine: return notApplicable("Transient snap line, not persistent drawing geometry");
    case RS2::EntitySnapArc: return notApplicable("Transient snap arc, not persistent drawing geometry");
    case RS2::EntitySnapCircle: return notApplicable("Transient snap circle, not persistent drawing geometry");
    case RS2::EntitySnapConstructionLine: return notApplicable("Transient snap construction line, not persistent drawing geometry");
    case RS2::EntityDimArrowBlock: return notApplicable("Owned dimension arrow block, written as its primitives");
    }
    return notAnEnumerator();
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

constexpr unsigned countRows() {
    unsigned count = 0;
    while (rowOf(static_cast<RS2::EntityType>(count)).kind != Kind::NotAnEnumerator)
        ++count;
    return count;
}

constexpr unsigned kRowCount = countRows();
static_assert(kRowCount == static_cast<unsigned>(RS2::EntityDimArrowBlock) + 1u,
              "RS2::EntityType has a value without a row");

constexpr unsigned textLength(const char* text) {
    unsigned length = 0;
    if (text != nullptr) {
        while (text[length] != '\0')
            ++length;
    }
    return length;
}

constexpr bool rowsWellFormed() {
    constexpr unsigned checks = DxfRoundTrip | DwgRoundTrip | InsertExpansion | CloneClass;
    for (unsigned index = 0; index < kRowCount; ++index) {
        const Row row = rowOf(static_cast<RS2::EntityType>(index));
        if ((row.knownGaps & row.notRun) != 0 || ((row.knownGaps | row.notRun) & ~checks) != 0)
            return false;
        if (row.kind == Kind::DrawingEntity) {
            if (textLength(row.reason) != 0 || rowOf(row.reloadsAs).kind != Kind::DrawingEntity)
                return false;
            if ((row.knownGaps | row.notRun) != 0 && textLength(row.gapNote) <= 10)
                return false;
        } else if (row.kind == Kind::NotApplicable) {
            if (textLength(row.reason) <= 10 || row.knownGaps != 0 || row.notRun != 0
                || textLength(row.gapNote) != 0)
                return false;
        } else {
            return false;
        }
    }
    return true;
}

static_assert(rowsWellFormed(), "Entity coverage rows have inconsistent dispositions or gap metadata");

std::vector<RS2::EntityType> drawingTypes() {
    std::vector<RS2::EntityType> types;
    for (unsigned index = 0; index < kRowCount; ++index) {
        const auto type = static_cast<RS2::EntityType>(index);
        if (rowOf(type).kind == Kind::DrawingEntity)
            types.push_back(type);
    }
    return types;
}

void requireDrawingFixture(const RS_Entity* entity, RS2::EntityType type, const RS_EntityContainer* owner) {
    REQUIRE(entity != nullptr);
    REQUIRE(entity->rtti() == type);
    REQUIRE(entity->getParent() == owner);
    REQUIRE(entity->hasValidBorders());
    if (entity->isContainer())
        REQUIRE(static_cast<const RS_EntityContainer*>(entity)->count() > 0);
    switch (type) {
    case RS2::EntityHyperbola:
        REQUIRE(static_cast<const LC_Hyperbola*>(entity)->isValid());
        break;
    case RS2::EntityParabola:
        REQUIRE(static_cast<const LC_Parabola*>(entity)->getData().isValid());
        break;
    case RS2::EntitySpline:
        REQUIRE(static_cast<const RS_Spline*>(entity)->validate());
        break;
    case RS2::EntitySplinePoints:
        REQUIRE(static_cast<const LC_SplinePoints*>(entity)->getNumberOfControlPoints() >= 3);
        break;
    default:
        break;
    }
}

bool hasDxf2018Header(const std::filesystem::path& path) {
    for (const auto& record : lc::test::dxfRecords(path)) {
        for (std::size_t i = 0; i + 1 < record.m_groups.size(); ++i) {
            if (record.m_groups[i].m_code == 9 && record.m_groups[i].m_value == "$ACADVER")
                return record.m_groups[i + 1].m_code == 1 && record.m_groups[i + 1].m_value == "AC1032";
        }
    }
    return false;
}

struct RoundTripObservation {
    bool exported = false;
    bool imported = false;
    bool usableGeometry = false;
    std::vector<RS2::EntityType> loadedTypes;
};

bool runRoundTrip(Check check, RS2::EntityType type, RS2::EntityType reloadsAs, std::string* diagnostic,
                  RoundTripObservation* observation) {
    if (observation != nullptr)
        *observation = {};
    const bool dwg = check == DwgRoundTrip;
    const lc::test::OutputDirectory output("lc_entity_coverage_" + std::to_string(static_cast<unsigned>(type)) +
                                            (dwg ? "_dwg" : "_dxf"));
    const auto path = output.file(dwg ? "entity.dwg" : "entity.dxf");
    RS2::FormatType format = RS2::FormatDXFRW2018;
#ifdef DWGSUPPORT
    if (dwg)
        format = RS2::FormatDWG2018;
#else
    REQUIRE_FALSE(dwg);
#endif
    RS_Graphic source;
    source.initForNewDocument();
    auto entity = lc::test::makeDrawingEntity(type, source, &source);
    requireDrawingFixture(entity.get(), type, &source);
    source.addEntity(entity.release());
    const bool exported = lc::test::exportDrawing(source, path, format);
    if (observation != nullptr)
        observation->exported = exported;
    if (diagnostic != nullptr)
        *diagnostic = "export=" + std::to_string(exported);
    INFO("exported " << exported);
    if (!exported)
        return false;
    if (dwg) {
        REQUIRE(lc::test::fileBytes(path).substr(0, 6) == "AC1032");
    } else {
        REQUIRE(hasDxf2018Header(path));
    }
    RS_Graphic reloaded;
    const bool imported = lc::test::importDrawing(reloaded, path, format);
    if (observation != nullptr)
        observation->imported = imported;
    if (diagnostic != nullptr)
        *diagnostic += " import=" + std::to_string(imported);
    INFO("imported " << imported);
    if (!imported)
        return false;
    reloaded.onLoadingCompleted();
    if (observation != nullptr)
        observation->usableGeometry = true;
    std::vector<const RS_Entity*> live;
    for (unsigned i = 0; i < reloaded.count(); ++i) {
        const RS_Entity* current = reloaded.entityAt(i);
        if (current != nullptr && !current->isDeleted()) {
            live.push_back(current);
            if (observation != nullptr) {
                observation->loadedTypes.push_back(current->rtti());
                observation->usableGeometry = observation->usableGeometry && current->hasValidBorders() &&
                    (!current->isContainer() || static_cast<const RS_EntityContainer*>(current)->count() > 0);
            }
            if (diagnostic != nullptr)
                *diagnostic += " loaded_type=" + std::to_string(static_cast<unsigned>(current->rtti()));
        }
    }
    INFO("live entity count " << live.size());
    if (diagnostic != nullptr)
        *diagnostic += " live=" + std::to_string(live.size()) + " expected_type=" + std::to_string(reloadsAs);
    if (live.size() != 1 || live.front()->rtti() != reloadsAs || !live.front()->hasValidBorders())
        return false;
    return !live.front()->isContainer() || static_cast<const RS_EntityContainer*>(live.front())->count() > 0;
}

bool runCheck(Check check, RS2::EntityType type, bool mixedBlock = false,
              RS2::EntityType reloadsAs = RS2::EntityUnknown, std::string* diagnostic = nullptr,
              RoundTripObservation* observation = nullptr) {
    if (check == DxfRoundTrip || check == DwgRoundTrip)
        return runRoundTrip(check, type, reloadsAs, diagnostic, observation);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    if (check == CloneClass) {
        const auto original = lc::test::makeDrawingEntity(type, graphic, &graphic);
        requireDrawingFixture(original.get(), type, &graphic);
        const std::unique_ptr<RS_Entity> clone{original->clone()};
        return clone != nullptr && clone.get() != original.get() && clone->rtti() == type;
    }

    REQUIRE(check == InsertExpansion);
    const QString blockName = QStringLiteral("LC_ENTITY_COVERAGE");
    auto block = std::make_unique<RS_Block>(&graphic, RS_BlockData(blockName, {0, 0}, false));
    if (mixedBlock)
        block->appendEntity(new RS_Line(block.get(), RS_LineData({-3, 0}, {-1, 1})));
    auto source = lc::test::makeDrawingEntity(type, graphic, block.get());
    requireDrawingFixture(source.get(), type, block.get());
    const RS_Entity* sourceIdentity = source.get();
    // addEntity intentionally prepends images/hatches; this fixture tests the explicit {line, instance} order.
    block->appendEntity(source.release());
    REQUIRE(block->entityAt(mixedBlock ? 1 : 0) == sourceIdentity);
    REQUIRE(graphic.addBlock(block.release()));

    RS_Insert insert(&graphic, RS_InsertData(blockName, {10, 5}, {2, 2}, M_PI / 6.0, 1, 1, {0, 0}));
    insert.update();
    const unsigned expectedCount = mixedBlock ? 2u : 1u;
    if (insert.count() != expectedCount)
        return false;
    const RS_Entity* last = insert.entityAt(expectedCount - 1u);
    const auto expectedType = type == RS2::EntityInsert ? RS2::EntityLine : type;
    if (last == nullptr || last == sourceIdentity || last->rtti() != expectedType || last->getParent() != &insert)
        return false;
    if (mixedBlock) {
        const RS_Entity* first = insert.entityAt(0);
        if (first == nullptr || first->rtti() != RS2::EntityLine || first->getParent() != &insert)
            return false;
    }
    return true;
}

void checkOutcome(const Row& row, Check check, bool passed) {
    if ((row.knownGaps & check) != 0) {
        INFO(row.gapNote);
        CHECK_FALSE(passed);
    } else {
        CHECK(passed);
    }
}

} // namespace

TEST_CASE("Every entity type has an explicit coverage disposition", "[entity-coverage]") {
    unsigned drawingRows = 0;
    unsigned exemptRows = 0;
    for (unsigned index = 0; index < kRowCount; ++index) {
        const Row row = rowOf(static_cast<RS2::EntityType>(index));
        CAPTURE(index);
        REQUIRE(row.kind != Kind::NotAnEnumerator);
        if (row.kind == Kind::DrawingEntity) {
            ++drawingRows;
        } else {
            ++exemptRows;
            CHECK(textLength(row.reason) > 10);
        }
    }
    CHECK(kRowCount == 50);
    CHECK(drawingRows == 27);
    CHECK(exemptRows == 23);
}

TEST_CASE("Drawing entity factories produce usable fixtures", "[entity-coverage]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    lc::test::ensureSettings();
    const lc::test::SourceFonts fonts;
    const lc::test::QuietLog quiet;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const auto entity = lc::test::makeDrawingEntity(type, graphic, &graphic);
    requireDrawingFixture(entity.get(), type, &graphic);
}

TEST_CASE("Every drawing entity clone keeps its runtime class", "[entity-coverage]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    const Row row = rowOf(type);
    REQUIRE((row.notRun & CloneClass) == 0);
    lc::test::ensureSettings();
    const lc::test::SourceFonts fonts;
    const lc::test::QuietLog quiet;
    checkOutcome(row, CloneClass, runCheck(CloneClass, type));
}

TEST_CASE("Every drawing entity expands as the sole block member", "[entity-coverage]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    const Row row = rowOf(type);
    REQUIRE((row.notRun & InsertExpansion) == 0);
    lc::test::ensureSettings();
    const lc::test::SourceFonts fonts;
    const lc::test::QuietLog quiet;
    checkOutcome(row, InsertExpansion, runCheck(InsertExpansion, type));
}

TEST_CASE("Every drawing entity expands beside a line in a block", "[entity-coverage]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    const Row row = rowOf(type);
    REQUIRE((row.notRun & InsertExpansion) == 0);
    lc::test::ensureSettings();
    const lc::test::SourceFonts fonts;
    const lc::test::QuietLog quiet;
    checkOutcome(row, InsertExpansion, runCheck(InsertExpansion, type, true));
}

TEST_CASE("Construction lines retain native INSERT expansion", "[entity-coverage]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const QString blockName = QStringLiteral("LC_COVERAGE_CONSTRUCTION");
    auto block = std::make_unique<RS_Block>(&graphic, RS_BlockData(blockName, {0, 0}, false));
    block->addEntity(new RS_ConstructionLine(block.get(), RS_ConstructionLineData({0, 0}, {5, 2})));
    REQUIRE(graphic.addBlock(block.release()));
    RS_Insert insert(&graphic, RS_InsertData(blockName, {10, 5}, {2, 2}, M_PI / 6.0, 1, 1, {0, 0}));
    insert.update();
    REQUIRE(insert.count() == 1);
    CHECK(insert.entityAt(0)->rtti() == RS2::EntityConstructionLine);
    CHECK(insert.entityAt(0)->getParent() == &insert);
}

TEST_CASE("An unsupported container rejects the complete mixed INSERT", "[entity-coverage]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    const QString blockName = QStringLiteral("LC_COVERAGE_CONTAINER");
    auto block = std::make_unique<RS_Block>(&graphic, RS_BlockData(blockName, {0, 0}, false));
    block->addEntity(new RS_Line(block.get(), RS_LineData({-3, 0}, {-1, 1})));
    auto container = std::make_unique<RS_EntityContainer>(block.get(), true);
    container->addEntity(new RS_Line(container.get(), RS_LineData({0, 0}, {5, 2})));
    block->addEntity(container.release());
    REQUIRE(graphic.addBlock(block.release()));
    RS_Insert insert(&graphic, RS_InsertData(blockName, {10, 5}, {2, 2}, M_PI / 6.0, 1, 1, {0, 0}));
    insert.update();
    CHECK(insert.count() == 0);
}

TEST_CASE("Every drawing entity has a DXF 2018 round-trip disposition", "[entity-coverage][roundtrip][dxf]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    const Row row = rowOf(type);
    REQUIRE((row.notRun & DxfRoundTrip) == 0);
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    const lc::test::SourceFonts fonts;
    std::string diagnostic;
    const bool passed = runCheck(DxfRoundTrip, type, false, row.reloadsAs, &diagnostic);
    INFO(diagnostic);
    checkOutcome(row, DxfRoundTrip, passed);
}

#ifdef DWGSUPPORT
TEST_CASE("Every drawing entity has a DWG 2018 round-trip disposition", "[entity-coverage][roundtrip][dwg]") {
    const auto types = drawingTypes();
    const auto type = GENERATE_COPY(from_range(types.begin(), types.end()));
    CAPTURE(static_cast<unsigned>(type));
    const Row row = rowOf(type);
    REQUIRE((row.notRun & DwgRoundTrip) == 0);
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    const lc::test::SourceFonts fonts;
    std::string diagnostic;
    RoundTripObservation observation;
    const bool passed = runCheck(DwgRoundTrip, type, false, row.reloadsAs, &diagnostic, &observation);
    INFO(diagnostic);
    checkOutcome(row, DwgRoundTrip, passed);
    if ((row.knownGaps & DwgRoundTrip) != 0) {
        REQUIRE(type == RS2::EntityMLeader);
        REQUIRE(observation.exported);
        REQUIRE(observation.imported);
        const std::vector<RS2::EntityType> expected{RS2::EntityDimLeader, RS2::EntityMText};
        CHECK(observation.loadedTypes == expected);
        CHECK(observation.usableGeometry);
    }
}
#endif

TEST_CASE("Construction lines and standalone containers are not DXF entity records", "[entity-coverage][roundtrip][dxf]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    const lc::test::OutputDirectory output("lc_entity_coverage_nonpersistent");
    for (const bool construction : {false, true}) {
        CAPTURE(construction);
        RS_Graphic graphic;
        graphic.initForNewDocument();
        if (construction) {
            graphic.addEntity(new RS_ConstructionLine(&graphic, RS_ConstructionLineData({0, 0}, {5, 2})));
        } else {
            auto container = std::make_unique<RS_EntityContainer>(&graphic, true);
            container->addEntity(new RS_Line(container.get(), RS_LineData({0, 0}, {5, 2})));
            graphic.addEntity(container.release());
        }
        const auto path = output.file(construction ? "construction.dxf" : "container.dxf");
        REQUIRE(lc::test::exportDrawing(graphic, path, RS2::FormatDXFRW2018));
        RS_Graphic reloaded;
        REQUIRE(lc::test::importDrawing(reloaded, path, RS2::FormatDXFRW2018));
        reloaded.onLoadingCompleted();
        CHECK(reloaded.count() == 0);
    }
}
