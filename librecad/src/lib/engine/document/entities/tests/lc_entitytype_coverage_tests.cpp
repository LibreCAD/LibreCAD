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

#include "rs.h"

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
    case RS2::EntityMLeader: return drawing(type);
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
