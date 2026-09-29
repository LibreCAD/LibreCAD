/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// An arc-length dimension (ARC_DIMENSION, LC_DimArc) used to leak an RS_Arc
// every time it was rebuilt: the "reference arc" doUpdateDim() placed its text
// with was allocated on the heap and never added nor freed. Each update()
// rebuilds twice, and an imported one is updated twice, so every arc
// dimension read from a DWG or DXF file leaked four arcs.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <cstddef>
#include <optional>

#include <QFile>
#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "lc_dimarc.h"
#include "rs_arc.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_mtext.h"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define LC_DIMARC_TEST_ASAN 1
#endif
#endif
#if !defined(LC_DIMARC_TEST_ASAN) && defined(__SANITIZE_ADDRESS__)
#define LC_DIMARC_TEST_ASAN 1
#endif

#if defined(LC_DIMARC_TEST_ASAN)
#include <sanitizer/allocator_interface.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#endif

namespace {

// Bytes the heap holds right now, where the allocator can tell us; otherwise
// the leak checks below are skipped and only the drawing is checked.
std::optional<std::size_t> heapBytesInUse() {
#if defined(LC_DIMARC_TEST_ASAN)
    return __sanitizer_get_current_allocated_bytes();
#elif defined(__APPLE__)
    malloc_statistics_t stats{};
    malloc_zone_statistics(nullptr, &stats);
    return stats.size_in_use;
#else
    return std::nullopt;
#endif
}

// Rebuilds the dimension many times: once it has been drawn, a rebuild frees
// everything it allocates, so the heap must not grow and the dimension must
// keep the same parts.
void checkRebuildsLeaveNothingBehind(LC_DimArc& dimension) {
    dimension.update(); // warm up: fonts, the resolved dimension style
    const unsigned parts = dimension.count();
    REQUIRE(parts > 0);

    const std::optional<std::size_t> before = heapBytesInUse();
    for (int i = 0; i < 50; ++i) {
        dimension.update();
    }
    const std::optional<std::size_t> after = heapBytesInUse();

    if (before.has_value() && after.has_value()) {
        CHECK(*after <= *before);
    }
    CHECK(dimension.count() == parts);
}

struct Parts {
    int m_arcs = 0;
    int m_lines = 0;
    int m_texts = 0;
};

Parts partsOf(const LC_DimArc& dimension) {
    Parts parts;
    for (const RS_Entity* e : dimension) {
        switch (e->rtti()) {
            case RS2::EntityArc:
                ++parts.m_arcs;
                break;
            case RS2::EntityLine:
                ++parts.m_lines;
                break;
            case RS2::EntityMText:
                ++parts.m_texts;
                break;
            default:
                break;
        }
    }
    return parts;
}

// A quarter circle of radius 10 about the origin, measured by an arc-length
// dimension 5 units outside it (written by ezdxf's add_arc_dim_cra, with the
// arc's start and end angles filled in).
constexpr char kArcDimensionDxf[] =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nARC_DIMENSION\n"
    "100\nAcDbEntity\n8\n0\n"
    "100\nAcDbDimension\n3\nStandard\n"
    "10\n10.606601717798213\n20\n10.606601717798211\n30\n0.0\n"
    "11\n10.765700743565185\n21\n10.765700743565183\n31\n0.0\n"
    "70\n37\n71\n5\n1\n<>\n"
    "100\nAcDbArcDimension\n"
    "13\n10.0\n23\n0.0\n33\n0.0\n"
    "14\n0.0\n24\n10.0\n34\n0.0\n"
    "15\n0.0\n25\n0.0\n35\n0.0\n"
    "40\n0.0\n41\n1.5707963267948966\n"
    "70\n0\n71\n0\n"
    "16\n0.0\n26\n0.0\n36\n0.0\n"
    "17\n0.0\n27\n0.0\n37\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("Rebuilding an arc dimension leaves nothing behind", "[dimension][dimarc][ownership]") {
    const bool partial = GENERATE(false, true);
    const int arcSymbol = GENERATE(0, 1, 2);
    CAPTURE(partial, arcSymbol);

    REQUIRE(lc::test::application() != nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();

    RS_DimensionData dimensionData;
    dimensionData.definitionPoint = RS_Vector(10.0, 0.0);
    dimensionData.middleOfText = RS_Vector(0.0, 10.0);
    dimensionData.text = "<>";
    dimensionData.style = "Standard";
    dimensionData.autoText = true;
    LC_DimArcData arcData(15.0, 10.0 * M_PI_2, RS_Vector(0.0, 0.0), RS_Vector(0.0, 1.0), RS_Vector(1.0, 0.0));
    arcData.arcSymbol = arcSymbol;
    arcData.isPartial = partial;

    auto* dimension = new LC_DimArc(&graphic, dimensionData, arcData);
    graphic.addEntity(dimension);

    const Parts parts = partsOf(*dimension);
    // The dimension arc, split around the text; an extension line at each end,
    // or at the end only for a partial one; the "∩" symbol above the text is
    // text of its own.
    CHECK(parts.m_arcs == 2);
    CHECK(parts.m_lines == (partial ? 1 : 2));
    CHECK(parts.m_texts == (arcSymbol == 1 ? 2 : 1));

    checkRebuildsLeaveNothingBehind(*dimension);
}

TEST_CASE("An imported ARC_DIMENSION is drawn, and rebuilding it leaves nothing behind",
          "[dimension][dimarc][dxf][filter][ownership]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("arc_dimension.dxf"));
    {
        QFile file(path);
        REQUIRE(file.open(QIODevice::WriteOnly));
        REQUIRE(file.write(kArcDimensionDxf) == static_cast<qint64>(sizeof(kArcDimensionDxf) - 1));
    }

    RS_Graphic graphic;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(graphic, path, RS2::FormatDXFRW));
    }

    REQUIRE(graphic.count() == 1);
    RS_Entity* entity = graphic.entityAt(0);
    REQUIRE(entity != nullptr);
    REQUIRE(entity->rtti() == RS2::EntityDimArc);
    auto* dimension = static_cast<LC_DimArc*>(entity);

    CHECK(dimension->getCenter().x == Catch::Approx(0.0).margin(1e-9));
    CHECK(dimension->getCenter().y == Catch::Approx(0.0).margin(1e-9));
    CHECK(dimension->getRadius() == Catch::Approx(15.0));

    // The dimension arc, on either side of the text, and both extension lines.
    const Parts parts = partsOf(*dimension);
    CHECK(parts.m_arcs == 2);
    CHECK(parts.m_lines == 2);
    CHECK(parts.m_texts == 1);
    for (const RS_Entity* e : *dimension) {
        if (e->rtti() == RS2::EntityArc) {
            const auto* arc = static_cast<const RS_Arc*>(e);
            CHECK(arc->getCenter().distanceTo(RS_Vector(0.0, 0.0)) == Catch::Approx(0.0).margin(1e-9));
            CHECK(arc->getRadius() == Catch::Approx(15.0));
        }
    }

    checkRebuildsLeaveNothingBehind(*dimension);
}
