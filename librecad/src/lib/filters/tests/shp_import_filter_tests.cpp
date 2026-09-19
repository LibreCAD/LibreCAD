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

/**
 * Filter-level test matrix for RS_FilterSHP.  Extends the Phase-2d smoke test
 * with per-geometry-type happy paths, multi-part rings/arcs, DBF-driven
 * layers and label MText, codepage decoding, edge cases (empty / null /
 * missing-shx) and hostile inputs (truncated headers).
 *
 * Pinned expectations come from the Phase-0 shapelib probe on the same
 * corpus — see shp_shapelib_tests.cpp — so the two suites move together
 * whenever a fixture changes.  Fixtures under test_data/shp/ are strictly
 * read-only; the filter never calls SHPRestoreSHX or bRestoreSHX=1.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QString>

#include "lc_containertraverser.h"
#include "rs.h"
#include "rs_entitycontainer.h"
#include "rs_filterdxfrw.h"
#include "rs_filtershp.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"

namespace {

QString corpusPath(const char* relative) {
    return QString(LIBRECAD_SOURCE_DIR) + "/test_data/shp/" + relative;
}

// RS_Graphic's ctor + LC_GROUP-driven RS_Settings reads dereference null
// without a QCoreApplication context — mirror the bootstrap that
// dwg_header_app_vars_tests.cpp / rs_graphic_layouts_tests.cpp use.
void ensureQtContext() {
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = {qarg0, nullptr};
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    (void)qapp;
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
}

struct EntityCounts {
    int points = 0;
    int polylines = 0;
    int closedPolylines = 0;
    int mtexts = 0;
    int total = 0;
};

// Recursive count that descends only into non-primitive containers: since
// RS_Polyline and RS_MText both inherit from RS_EntityContainer, we must
// stop descending once we hit them and count the polyline/mtext itself
// rather than its inner segments/characters.
void countEntitiesRecursive(const RS_EntityContainer& c, EntityCounts& out);

void countOne(RS_Entity* e, EntityCounts& out) {
    if (!e) return;
    ++out.total;
    switch (e->rtti()) {
    case RS2::EntityPoint: ++out.points; return;
    case RS2::EntityMText: ++out.mtexts; return;
    case RS2::EntityPolyline: {
        ++out.polylines;
        auto* pl = static_cast<RS_Polyline*>(e);
        if (pl->isClosed()) ++out.closedPolylines;
        return;
    }
    default: break;
    }
    // Only recurse into non-terminal containers (blocks, groupings).
    if (auto* container = dynamic_cast<RS_EntityContainer*>(e)) {
        countEntitiesRecursive(*container, out);
    }
}

void countEntitiesRecursive(const RS_EntityContainer& c, EntityCounts& out) {
    for (RS_Entity* e :
         lc::LC_ContainerTraverser{c, RS2::ResolveNone}.entities()) {
        countOne(e, out);
    }
}

EntityCounts countEntities(RS_Graphic& g) {
    EntityCounts c;
    countEntitiesRecursive(g, c);
    return c;
}

} // namespace

// ---------------------------------------------------------------------------
// Happy-path per geometry type (Phase-4b filter matrix, mirrors the Phase-0
// shapelib coverage but one level up: through RS_FilterSHP::fileImport,
// so entity emission + layer routing + pen assignment is exercised too).
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: imports a basic POINT shapefile into native RS_Points",
          "[shp][filter][smoke]") {
    ensureQtContext();
    const QString path = corpusPath("points.shp");
    REQUIRE(std::filesystem::is_regular_file(path.toStdString()));

    RS_Graphic graphic;
    RS_FilterSHP filter;

    REQUIRE(filter.canImport(path, RS2::FormatSHP));
    REQUIRE_FALSE(filter.canExport(path, RS2::FormatSHP));

    const bool ok = filter.fileImport(graphic, path, RS2::FormatSHP);
    REQUIRE(ok);

    // Corpus inventory: points.shp has 9 records, all SHPT_POINT.  Filter
    // emits exactly one RS_Point per record — no MText (label field absent
    // from points.dbf; only FID is present).
    const EntityCounts c = countEntities(graphic);
    CHECK(c.points == 9);
    CHECK(c.polylines == 0);
    CHECK(c.mtexts == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: ARC -> open RS_Polylines",
          "[shp][filter][matrix]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("polylines.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 2 ARC records: some are multi-part so the filter emits >= 2 polylines,
    // all open (no closing).  No RS_Points from ARC records.
    CHECK(c.polylines >= 2);
    CHECK(c.closedPolylines == 0);
    CHECK(c.points == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: POLYGON -> closed RS_Polylines",
          "[shp][filter][matrix]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("polygons.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 3 POLYGON records; some may be multi-part (>= 1 ring each), so the
    // emitted polyline count is >= 3.  All must be closed.
    CHECK(c.polylines >= 3);
    CHECK(c.closedPolylines == c.polylines);
    CHECK(c.points == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: MULTIPOINT -> one RS_Point per vertex",
          "[shp][filter][matrix]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("multipoints.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 2 MULTIPOINT records, first has 5 vertices; probe:
    //   multipoints.shp : type=MULTIPOINT n=2 rec0 nVerts=5
    // The second record's vertex count isn't hard-pinned in the probe but
    // the filter emits >= 5 points overall.
    CHECK(c.points >= 5);
    CHECK(c.polylines == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: multi-part POLYGON emits one closed polyline per ring",
          "[shp][filter][matrix][multipart]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("world_borders.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 246 POLYGON records over ~world countries; many have multiple rings.
    // Assertion is loose in absolute count but the shape is: at least one
    // polyline per record + non-zero closed polylines, all closed.
    CHECK(c.polylines >= 246);
    CHECK(c.closedPolylines == c.polylines);
    CHECK(c.points == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: Chi-SDOH 791-polygon corpus imports without loss",
          "[shp][filter][matrix][large]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("real_world/Chi-SDOH.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    CHECK(c.polylines >= 791);
    CHECK(c.closedPolylines == c.polylines);
}

// ---------------------------------------------------------------------------
// DBF-driven fidelity: layers, MText labels, codepage-aware string decoding.
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: bc_hospitals emits NAME-field MText next to each POINT",
          "[shp][filter][matrix][labels]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic,
                              corpusPath("multi_part/bc_hospitals.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 44 POINT records with a NAME (auto-detected label field) — filter emits
    // one RS_Point + one RS_MText per record.
    CHECK(c.points == 44);
    CHECK(c.mtexts == 44);
    // First MText content should match the DBF probe: "Chilliwack General
    // Hospital".  Locate it and check.
    bool foundChilliwack = false;
    for (RS_Entity* e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
        if (e && e->rtti() == RS2::EntityMText) {
            auto* m = static_cast<RS_MText*>(e);
            if (m->getText().contains("Chilliwack")) {
                foundChilliwack = true;
                break;
            }
        }
    }
    CHECK(foundChilliwack);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: UTF-8 codepage fixture imports without crash",
          "[shp][filter][matrix][codepage]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    // The fixture's DBF field name is literally "☃" (U+2603) so the
    // auto-detected NAME/LABEL/TEXT fallbacks don't match, and no MText
    // is emitted.  The codepage-decode path is still exercised on the
    // layer-name lookup (also empty) and on any string reads shapelib
    // makes internally.  The load must complete cleanly and produce the
    // single POINT the fixture contains.
    REQUIRE(filter.fileImport(graphic,
                              corpusPath("real_world/utf8-property.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    CHECK(c.points == 1);
    // The Latin-1 companion test exercises the decode path end-to-end
    // against a NAME-field fixture; here we only pin "no crash / opens
    // successfully" on the UTF-8-declared corpus.
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: Latin-1 codepage decodes accented characters",
          "[shp][filter][matrix][codepage]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic,
                              corpusPath("real_world/latin1-property.shp"),
                              RS2::FormatSHP));
    // Fixture has 1 POINT + a Latin-1 "name" string "México".  MText should
    // contain the U+00E9 (é) character after the LDID/87 codec decode.
    bool foundEAcute = false;
    for (RS_Entity* e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
        if (e && e->rtti() == RS2::EntityMText) {
            const QString t = static_cast<RS_MText*>(e)->getText();
            if (t.contains(QChar(0x00E9))) { foundEAcute = true; break; }
        }
    }
    CHECK(foundEAcute);
}

// ---------------------------------------------------------------------------
// Edge cases: empty, missing-shx, singleton, and the "hostile" fixtures
// whose whole point is to prove RS_FilterSHP fails gracefully.
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: real_world/empty.shp -> 0 entities, still returns true",
          "[shp][filter][matrix][edge]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    // Header is valid, nRecords == 0.  Filter returns true (empty is not an
    // error) and emits no entities beyond the default layer "0".
    REQUIRE(filter.fileImport(graphic, corpusPath("real_world/empty.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    CHECK(c.total == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: real_world/null.shp opens; SHPT_NULL records skipped",
          "[shp][filter][matrix][edge]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("real_world/null.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // Shapelib reports n=9 records (SHPGetInfo), but a subset of them are
    // encoded as SHPT_NULL (that's the whole point of the fixture: mixed
    // POINT + NULL records).  The filter skips NULL records by design, so
    // the emitted POINT count is < 9 but > 0.  Pin the range.
    CHECK(c.points > 0);
    CHECK(c.points <= 9);
}

// Fixtures with corrupt/truncated .shx are pinned in the Phase-0 shapelib
// tests as "SHPOpen returns null".  At the filter level, this must surface
// as: fileImport returns false, no crash, and no partial entities emitted.

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: missing_shx.shp -> false, no crash, no entities",
          "[shp][filter][matrix][edge]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    CHECK_FALSE(filter.fileImport(graphic, corpusPath("missing_shx.shp"),
                                  RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    CHECK(c.total == 0);
    CHECK_FALSE(filter.lastError().isEmpty());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: malformed .shx (pointz.shp) -> false, no crash",
          "[shp][filter][matrix][hostile]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    CHECK_FALSE(filter.fileImport(graphic, corpusPath("pointz.shp"),
                                  RS2::FormatSHP));
    CHECK(countEntities(graphic).total == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: malformed_truncated.shp -> false, process survives",
          "[shp][filter][matrix][hostile]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    // The whole point of this fixture: shapelib rejects the truncated
    // header, filter surfaces false, no crash, no ASan reports.  CVE-2023-
    // 30259-class regression guard.
    CHECK_FALSE(filter.fileImport(graphic,
                                  corpusPath("malformed_truncated.shp"),
                                  RS2::FormatSHP));
    CHECK(countEntities(graphic).total == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: malformed_dbf.shp -> false, no crash",
          "[shp][filter][matrix][hostile]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    CHECK_FALSE(filter.fileImport(graphic, corpusPath("malformed_dbf.shp"),
                                  RS2::FormatSHP));
    CHECK(countEntities(graphic).total == 0);
}

// ---------------------------------------------------------------------------
// Phase-4a generated fixtures — Z types and MULTIPATCH parts + CVE-class DoS.
// These and ltype_point.* are the only fixtures under test_data/shp/ produced
// by scripts/make_shp_fixtures.py; the rest of the corpus is fixed reference
// data.  See the plan's Phase 4a for the rationale.
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: generated POLYGONZ ring imports as one closed polyline",
          "[shp][filter][matrix][z]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("polygonz.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // Single POLYGONZ record, single ring — one closed polyline.
    CHECK(c.polylines == 1);
    CHECK(c.closedPolylines == 1);
    CHECK(c.points == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: generated POLYLINEZ (ARCZ) 2-part -> 2 open polylines",
          "[shp][filter][matrix][z]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("polylinez.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // Two parts, both open — Z carried per vertex but the polyline is open.
    CHECK(c.polylines == 2);
    CHECK(c.closedPolylines == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: generated MULTIPOINTZ -> one RS_Point per vertex",
          "[shp][filter][matrix][z]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("multipointz.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 4 vertices in the MULTIPOINTZ record.
    CHECK(c.points == 4);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: MULTIPATCH — rings closed, strips open wireframe",
          "[shp][filter][matrix][multipatch]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, corpusPath("multipatch.shp"),
                              RS2::FormatSHP));
    const EntityCounts c = countEntities(graphic);
    // 3 parts: OUTER_RING + INNER_RING + TRIANGLE_STRIP
    // -> two closed polylines (outer + inner rings)
    // -> one open polyline (the strip, as documented 2.5D simplification).
    CHECK(c.polylines == 3);
    CHECK(c.closedPolylines == 2);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: DoS crafted nPoints=60M rejected without allocation",
          "[shp][filter][matrix][hostile][cve]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    // Shapelib caps nPoints at 50 M (shpopen.cpp:2259); this fixture claims
    // 60 M in the record content.  fileImport still returns true (the file
    // header is valid and nEntities > 0 with 0 readable records isn't a
    // hard error), but SHPReadObject returns null internally so 0 entities
    // are emitted — no gigabyte allocation, no crash, process survives.
    // CVE-2023-30259-class regression guard.
    const bool ok = filter.fileImport(graphic, corpusPath("dos_npoints.shp"),
                                      RS2::FormatSHP);
    // Either result is acceptable — filter may report false because zero
    // records readable, or true because header was fine.  The load-bearing
    // assertion is: no entities emitted, no crash, no huge alloc.
    (void)ok;
    CHECK(countEntities(graphic).total == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: DoS crafted nParts=15M rejected without allocation",
          "[shp][filter][matrix][hostile][cve]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterSHP filter;
    // Shapelib caps nParts at 10 M (shpopen.cpp:2259); this fixture claims
    // 15 M.  Same expectation as dos_npoints: no crash, no allocation
    // blow-up, no partial entities.
    const bool ok = filter.fileImport(graphic, corpusPath("dos_nparts.shp"),
                                      RS2::FormatSHP);
    (void)ok;
    CHECK(countEntities(graphic).total == 0);
}

// ---------------------------------------------------------------------------
// canImport / canExport polarity — locks the import-only contract that
// Phase 3's UI wiring depends on.
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: canExport is always false (import-only)",
          "[shp][filter][contract]") {
    RS_FilterSHP filter;
    CHECK(filter.canImport("dummy.shp", RS2::FormatSHP));
    CHECK_FALSE(filter.canImport("dummy.shp", RS2::FormatDXFRW));
    CHECK_FALSE(filter.canExport("dummy.shp", RS2::FormatSHP));
    CHECK_FALSE(filter.canExport("dummy.shp", RS2::FormatDXFRW));
}

// ---------------------------------------------------------------------------
// The DBF LTYPE column keeps its name.  ltype_point.* holds one POINT per
// value: "VENDOR_TAB", which is no built-in, "Dashed", "ACAD_ISO02W100",
// "DASHED\t", "\t" and a blank one.
// ---------------------------------------------------------------------------

namespace {

// Group `code` values of every `recordType` record, or only of the one whose
// group 2 is `name` when one is given; a twin of the helpers in
// dxf_roundtrip_tests.cpp, whose namespace is private to that file.
std::vector<std::string> recordGroupValues(const std::string& path,
                                           const std::string& recordType,
                                           const std::string& code,
                                           const std::string& name = {}) {
    std::ifstream in(path);
    std::string codeLine, valueLine;
    std::vector<std::string> values;
    bool inRecord = false;
    bool nameMatched = false;
    auto trim = [](std::string value) {
        if (!value.empty() && value.back() == '\r')
            value.pop_back();
        const size_t first = value.find_first_not_of(" \t");
        return first == std::string::npos ? std::string() : value.substr(first);
    };
    while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
        const std::string groupCode = trim(codeLine);
        const std::string value = trim(valueLine);
        if (groupCode == "0") {
            inRecord = value == recordType;
            nameMatched = name.empty();
        } else if (inRecord && !name.empty() && groupCode == "2") {
            nameMatched = value == name;
        } else if (inRecord && nameMatched && groupCode == code) {
            values.push_back(value);
        }
    }
    return values;
}

std::vector<RS_Point*> importedPoints(RS_Graphic& g) {
    std::vector<RS_Point*> points;
    for (RS_Entity* e :
         lc::LC_ContainerTraverser{g, RS2::ResolveNone}.entities()) {
        if (e != nullptr && e->rtti() == RS2::EntityPoint)
            points.push_back(static_cast<RS_Point*>(e));
    }
    return points;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_FilterSHP: the DBF LTYPE column reaches the pen as a name",
          "[shp][filter][linetype][named]") {
    ensureQtContext();
    const QString path = corpusPath("ltype_point.shp");
    REQUIRE(std::filesystem::is_regular_file(path.toStdString()));

    RS_Graphic graphic;
    RS_FilterSHP filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatSHP));

    const std::vector<RS_Point*> points = importedPoints(graphic);
    REQUIRE(points.size() == 6);
    // shapelib trims the spaces that pad a C field; the pen trims tabs too.
    CHECK(points[0]->getPen(false).getLineTypeName().toStdString()
          == "VENDOR_TAB");
    std::vector<RS2::LineType> lineTypes;
    for (const RS_Point* point : points)
        lineTypes.push_back(point->getPen(false).getLineType());
    CHECK(lineTypes == std::vector<RS2::LineType>{
                           RS2::SolidLine, RS2::DashLine, RS2::DashLine,
                           RS2::DashLine, RS2::LineByLayer, RS2::LineByLayer});

    const std::string out = (std::filesystem::temp_directory_path() /
                             "shp_named_linetype_out.dxf").string();
    std::filesystem::remove(out);
    {
        RS_FilterDXFRW dxf;
        REQUIRE(dxf.fileExport(graphic, QString::fromStdString(out),
                               RS2::FormatDXFRW));
    }
    // A built-in in another case is saved as before, the alias by its name.
    CHECK(recordGroupValues(out, "POINT", "6")
          == std::vector<std::string>{"VENDOR_TAB", "DASHED", "ACAD_ISO02W100",
                                      "DASHED", "ByLayer", "ByLayer"});
    // A shapefile has no LTYPE table, so the export gives each name a record:
    // no dashes for one LibreCAD does not know, the family's for the alias.
    CHECK(recordGroupValues(out, "LTYPE", "73", "VENDOR_TAB")
          == std::vector<std::string>{"0"});
    CHECK(recordGroupValues(out, "LTYPE", "73", "ACAD_ISO02W100")
          == std::vector<std::string>{"2"});

    std::filesystem::remove(out);
}
