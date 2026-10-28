/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// HATCH export: ODA File Converter 27.1 deletes every non-solid HATCH that
// LibreCAD before this change wrote with 78 = 0. The exporter now writes at
// least one realized definition line (today a placeholder; a later step
// will add exact families), code 76 = 2 (pattern type), and a 1001 LibreCad
// / 1000 HATCH_LINES_APPROXIMATE XDATA tag whose APPID writeAppId always
// registers. The reader strips the tag so a re-export never duplicates it.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QString>
#include <QStringLiteral>

#include "drw_base.h"
#include "drw_entities.h"
#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
#include "lc_hatchdefinitionlines.h"
#include "lc_settingguard.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_entitycontainer.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_spline.h"

namespace {

constexpr double kPi = 3.141592653589793;

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

// A rectangular hatch boundary (one container, four line edges). Draw Hatch
// builds its boundary that way.
void addRectBoundary(RS_Hatch& hatch, double x1, double y1,
                      double x2, double y2) {
    auto* loop = new RS_EntityContainer(&hatch);
    loop->setLayer(nullptr);
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y1), RS_Vector(x2, y1))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y1), RS_Vector(x2, y2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y2), RS_Vector(x1, y2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y2), RS_Vector(x1, y1))));
    hatch.addEntity(loop);
}

// Builds a graphic that holds exactly one hatch over (0,0)-(100,60), with the
// given pattern, angle (radians) and scale, and runs its update(). The
// Paths/Patterns setting is set on each call: the fast-lane way (ensureSettings
// + SettingGuard) so no QApplication is required.
std::unique_ptr<RS_Graphic> makeOneHatchGraphic(const QString& pattern,
                                                 double angle, double scale,
                                                 bool solid = false) {
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(solid, scale, angle, pattern));
    addRectBoundary(*hatch, 0.0, 0.0, 100.0, 60.0);
    graphic->addEntity(hatch);
    hatch->update();
    return graphic;
}

// Reads a DXF file into DxfRecord vectors through the shared helper.
using lc::test::DxfRecord;
using lc::test::dxfRecords;

// Returns every APPID defined in the file, normalized to upper case.
std::set<std::string> registeredAppIds(const std::filesystem::path& file) {
    std::set<std::string> names;
    for (const auto& record : dxfRecords(file, "APPID")) {
        const auto own = record.own(2);
        for (const auto& name : own) {
            names.insert(toUpper(name));
        }
    }
    return names;
}

// Counts the number of records in the file that mention a given APPID (via a
// 1001 group), case-insensitive.
std::size_t countXDataAppUses(const std::filesystem::path& file,
                               const std::string& appName) {
    const std::string wanted = toUpper(appName);
    std::size_t uses = 0;
    for (const auto& record : dxfRecords(file)) {
        for (const auto& group : record.m_groups) {
            if (group.m_code == 1001 && toUpper(group.m_value) == wanted) {
                ++uses;
            }
        }
    }
    return uses;
}

// Captures the first HATCH DRW_Hatch handed to a reader.
class HatchCapture : public lc::test::NullDrwInterface {
public:
    int m_hatchCount{0};
    DRW_Hatch m_hatch;

    void addHatch(const DRW_Hatch* data) override {
        if (data != nullptr && m_hatchCount == 0) {
            m_hatch = *data;
            ++m_hatchCount;
        }
    }
};

// Reads the HATCH record out of a DXF written by the filter.
DxfRecord firstHatchRecord(const std::filesystem::path& file) {
    const auto records = dxfRecords(file, "HATCH");
    REQUIRE(records.size() >= 1u);
    return records.front();
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Realized definition lines match ezdxf's ISO patterns",
          "[lc-hatchfix][hatch-export]") {
    using LC_HatchDefinitionLines::LC_HatchLineFamily;
    using LC_HatchDefinitionLines::realize;

    // frame.origin = (0, 0), tileMin = (0, 0): PAT-local, same as proto_realize.
    const auto frameAt = [](double scale, double angleRad) {
        RS_HatchPatternFrame f;
        f.origin = RS_Vector{0.0, 0.0};
        f.scale = scale;
        f.angle = angleRad;
        f.tileMin = RS_Vector{0.0, 0.0};
        f.tileSize = RS_Vector{1.0, 1.0};
        f.tileValid = true;
        return f;
    };

    SECTION("ANSI31 at theta=0, s=4") {
        LC_HatchLineFamily f{45.0, {0.0, 0.0}, {0.0, 3.175}, {}};
        const auto out = realize(f, frameAt(4.0, 0.0));
        REQUIRE(out.has_value());
        CHECK(out->angle == Catch::Approx(45.0 * kPi / 180.0).margin(1e-9));
        CHECK(out->baseX == Catch::Approx(0.0).margin(1e-9));
        CHECK(out->baseY == Catch::Approx(0.0).margin(1e-9));
        CHECK(out->offsetX == Catch::Approx(-8.9802561212).margin(1e-9));
        CHECK(out->offsetY == Catch::Approx(8.9802561212).margin(1e-9));
        CHECK(out->dashList.empty());
    }

    SECTION("ANSI31 at theta=30 deg, s=0.25") {
        LC_HatchLineFamily f{45.0, {0.0, 0.0}, {0.0, 3.175}, {}};
        const auto out = realize(f, frameAt(0.25, kPi / 6.0));
        REQUIRE(out.has_value());
        CHECK(out->angle == Catch::Approx(75.0 * kPi / 180.0).margin(1e-9));
        CHECK(out->offsetX == Catch::Approx(-0.7667036246).margin(1e-9));
        CHECK(out->offsetY == Catch::Approx(0.2054376171).margin(1e-9));
    }

    SECTION("BRICK three families at theta=117.5 deg, s=7") {
        const auto f = frameAt(7.0, 117.5 * kPi / 180.0);
        const auto b1 = realize({0.0, {0.0, 0.0}, {0.0, 6.35}, {}}, f);
        REQUIRE(b1.has_value());
        CHECK(b1->angle == Catch::Approx(117.5 * kPi / 180.0).margin(1e-9));
        CHECK(b1->baseX == Catch::Approx(0.0).margin(1e-9));
        CHECK(b1->baseY == Catch::Approx(0.0).margin(1e-9));
        CHECK(b1->offsetX == Catch::Approx(-39.4276315348).margin(1e-9));
        CHECK(b1->offsetY == Catch::Approx(-20.5247258583).margin(1e-9));
        CHECK(b1->dashList.empty());

        const auto b2 = realize({90.0, {0.0, 0.0}, {0.0, 12.7}, {6.35, -6.35}},
                                 f);
        REQUIRE(b2.has_value());
        // Normalize (a + theta) % 360 for the angle check.
        const double b2DegExpected = std::fmod(90.0 + 117.5, 360.0);
        CHECK(std::fmod(b2->angle * 180.0 / kPi + 360.0, 360.0)
              == Catch::Approx(b2DegExpected).margin(1e-9));
        CHECK(b2->baseX == Catch::Approx(0.0).margin(1e-9));
        CHECK(b2->baseY == Catch::Approx(0.0).margin(1e-9));
        CHECK(b2->offsetX == Catch::Approx(41.0494517166).margin(1e-9));
        CHECK(b2->offsetY == Catch::Approx(-78.8552630695).margin(1e-9));
        REQUIRE(b2->dashList.size() == 2u);
        CHECK(b2->dashList.at(0) == Catch::Approx(44.45).margin(1e-9));
        CHECK(b2->dashList.at(1) == Catch::Approx(-44.45).margin(1e-9));

        const auto b3 = realize({90.0, {6.35, 0.0}, {0.0, 12.7}, {-6.35, 6.35}},
                                 f);
        REQUIRE(b3.has_value());
        CHECK(b3->baseX == Catch::Approx(-20.5247258583).margin(1e-9));
        CHECK(b3->baseY == Catch::Approx(39.4276315348).margin(1e-9));
        CHECK(b3->offsetX == Catch::Approx(41.0494517166).margin(1e-9));
        CHECK(b3->offsetY == Catch::Approx(-78.8552630695).margin(1e-9));
        REQUIRE(b3->dashList.size() == 2u);
        CHECK(b3->dashList.at(0) == Catch::Approx(-44.45).margin(1e-9));
        CHECK(b3->dashList.at(1) == Catch::Approx(44.45).margin(1e-9));
    }

    SECTION("AR-HBONE at theta=117.5 deg, s=7") {
        const auto f = frameAt(7.0, 117.5 * kPi / 180.0);
        const auto h1 = realize({45.0, {0.0, 0.0}, {101.6, 101.6},
                                 {304.8, -101.6}}, f);
        REQUIRE(h1.has_value());
        CHECK(h1->offsetX == Catch::Approx(-892.1454599795).margin(1e-6));
        CHECK(h1->offsetY == Catch::Approx(-464.4215307647).margin(1e-6));
        REQUIRE(h1->dashList.size() == 2u);
        CHECK(h1->dashList.at(0) == Catch::Approx(2133.6).margin(1e-9));
        CHECK(h1->dashList.at(1) == Catch::Approx(-711.2).margin(1e-9));

        const auto h2 = realize({135.0, {71.842, 71.842}, {101.6, -101.6},
                                 {304.8, -101.6}}, f);
        REQUIRE(h2.has_value());
        CHECK(h2->baseX == Catch::Approx(-678.2830330445).margin(1e-6));
        CHECK(h2->baseY == Catch::Approx(213.8618188361).margin(1e-6));
        CHECK(h2->offsetX == Catch::Approx(-892.1454599795).margin(1e-6));
        CHECK(h2->offsetY == Catch::Approx(-464.4215307647).margin(1e-6));
    }

    SECTION("AR-HBONE at theta=30 deg, s=1") {
        const auto f = frameAt(1.0, kPi / 6.0);
        const auto h1 = realize({45.0, {0.0, 0.0}, {101.6, 101.6},
                                 {304.8, -101.6}}, f);
        REQUIRE(h1.has_value());
        CHECK(h1->offsetX == Catch::Approx(-71.8420489685).margin(1e-9));
        CHECK(h1->offsetY == Catch::Approx(124.4340789334).margin(1e-9));

        const auto h2 = realize({135.0, {71.842, 71.842}, {101.6, -101.6},
                                 {304.8, -101.6}}, f);
        REQUIRE(h2.has_value());
        CHECK(h2->baseX == Catch::Approx(26.2959970587).margin(1e-9));
        CHECK(h2->baseY == Catch::Approx(98.1379970587).margin(1e-9));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A family with zero spacing is rejected",
          "[lc-hatchfix][hatch-export]") {
    using LC_HatchDefinitionLines::LC_HatchLineFamily;
    using LC_HatchDefinitionLines::realize;

    RS_HatchPatternFrame frame;
    frame.origin = RS_Vector{0.0, 0.0};
    frame.scale = 1.0;
    frame.angle = 0.0;
    frame.tileMin = RS_Vector{0.0, 0.0};
    frame.tileSize = RS_Vector{1.0, 1.0};
    frame.tileValid = true;

    CHECK_FALSE(realize({0.0, {0.0, 0.0}, {5.0, 0.0}, {}}, frame).has_value());
    CHECK_FALSE(realize({0.0, {0.0, 0.0}, {5.0, 1e-12}, {}}, frame).has_value());
    CHECK(realize({0.0, {0.0, 0.0}, {0.0, 3.175}, {}}, frame).has_value());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The placeholder line runs 45 degrees to the hatch at a quarter "
          "of the tile's mean size",
          "[lc-hatchfix][hatch-export]") {
    using LC_HatchDefinitionLines::placeholderLine;

    RS_HatchPatternFrame valid;
    valid.origin = RS_Vector{0.0, 0.0};
    valid.scale = 2.0;
    valid.angle = kPi / 6.0;
    valid.tileMin = RS_Vector{0.0, 0.0};
    valid.tileSize = RS_Vector{20.0, 10.0};
    valid.tileValid = true;
    const auto line = placeholderLine(valid);
    CHECK(line.angle == Catch::Approx(1.3089969389957472).margin(1e-12));
    CHECK(line.baseX == Catch::Approx(0.0).margin(1e-12));
    CHECK(line.baseY == Catch::Approx(0.0).margin(1e-12));
    CHECK(line.offsetX == Catch::Approx(-6.830127018922193).margin(1e-12));
    CHECK(line.offsetY == Catch::Approx(1.8301270189221932).margin(1e-12));
    CHECK(line.dashList.empty());

    RS_HatchPatternFrame invalid;
    invalid.origin = RS_Vector{0.0, 0.0};
    invalid.scale = 3.0;
    invalid.angle = 0.0;
    invalid.tileValid = false;
    const auto fallback = placeholderLine(invalid);
    const double mag = std::hypot(fallback.offsetX, fallback.offsetY);
    CHECK(mag == Catch::Approx(3.0 / 4.0).margin(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An exported pattern hatch has a definition line, pattern type 2 "
          "and the approximate tag",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_paisley");
    const auto file = out.file("paisley.dxf");

    // PAISLEY is incommensurate so a later exact-family step keeps the
    // placeholder on it; that step removes the tag from ANSI31 instead.
    auto graphic = makeOneHatchGraphic(QStringLiteral("PAISLEY"), kPi / 6.0, 1.0);
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(78) == std::vector<std::string>{"1"});
    CHECK(record.own(76) == std::vector<std::string>{"2"});
    CHECK(record.own(75) == std::vector<std::string>{"0"});

    const auto group52 = record.own(52);
    REQUIRE(group52.size() == 1u);
    CHECK(std::stod(group52.front()) == Catch::Approx(30.0).margin(1e-9));
    const auto group53 = record.own(53);
    REQUIRE(group53.size() == 1u);
    CHECK(std::stod(group53.front()) == Catch::Approx(75.0).margin(1e-9));

    // The placeholder offset: for LibreCAD's PAISLEY tile |offset| = s / 4
    // once scaled by the geometric mean sqrt(W*H)/4. The precise tile size
    // comes from the pattern file; use the per-hatch patternFrame() as the
    // oracle instead of hard-coding bytes.
    RS_Hatch* hatchPtr = nullptr;
    for (RS_Entity* e : *graphic) {
        if (e && e->rtti() == RS2::EntityHatch) {
            hatchPtr = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(hatchPtr != nullptr);
    const auto expected =
        LC_HatchDefinitionLines::placeholderLine(hatchPtr->patternFrame());
    const auto group45 = record.own(45);
    const auto group46 = record.own(46);
    REQUIRE(group45.size() == 1u);
    REQUIRE(group46.size() == 1u);
    CHECK(std::stod(group45.front())
          == Catch::Approx(expected.offsetX).margin(1e-9));
    CHECK(std::stod(group46.front())
          == Catch::Approx(expected.offsetY).margin(1e-9));
    // LibreCAD's PAISLEY tile at e8dd54132 is roughly 100 x 100, so the
    // offset magnitude lives near 25 at scale 1.
    const double mag = std::hypot(expected.offsetX, expected.offsetY);
    CHECK(mag == Catch::Approx(25.0).margin(1e-3));

    // XDATA tag: the hatch record carries 1001 LibreCad and 1000 kApproximateTag.
    const auto group1001 = record.own(1001);
    bool hasLibreCad = false;
    for (const auto& name : group1001) {
        if (toUpper(name) == toUpper("LibreCad"))
            hasLibreCad = true;
    }
    CHECK(hasLibreCad);
    const auto group1000 = record.own(1000);
    bool hasTag = false;
    for (const auto& value : group1000) {
        if (value == "HATCH_LINES_APPROXIMATE")
            hasTag = true;
    }
    CHECK(hasTag);

    // Every 1001 in the file has exactly one APPID record (case-insensitive)
    // and the registered APPID set includes LibreCad.
    const auto apps = registeredAppIds(file);
    CHECK(apps.count("LIBRECAD") == 1u);
    for (const auto& record2 : dxfRecords(file)) {
        for (const auto& group : record2.m_groups) {
            if (group.m_code == 1001) {
                CAPTURE(record2.type(), group.m_value);
                CHECK(apps.count(toUpper(group.m_value)) == 1u);
            }
        }
    }

    // The file starts with 999 / dxfrw 0.6.4.
    std::ifstream in(file);
    std::string line1;
    std::string line2;
    std::string line3;
    std::string line4;
    std::getline(in, line1);
    std::getline(in, line2);
    std::getline(in, line3);
    std::getline(in, line4);
    in.close();
    const auto strip = [](std::string& s) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        const auto first = s.find_first_not_of(" \t");
        const auto last = s.find_last_not_of(" \t");
        s = (first == std::string::npos) ? std::string()
                                          : s.substr(first, last - first + 1);
    };
    strip(line1);
    strip(line2);
    strip(line3);
    strip(line4);
    // DXF record: a group starts with its code, so the first comment is
    // 999 / <text>.
    CHECK((line1 == "999" || line3 == "999"));
    const std::string& comment = (line1 == "999") ? line2 : line4;
    CHECK(comment.rfind("dxfrw ", 0) == 0u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("SQUARE, BRICK and arcs exports keep one line and re-import "
          "through the bundled tile",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    struct Row {
        const char* m_pattern;
        const char* m_tag;
    };
    const auto name = GENERATE(
        Row{"SQUARE", "square"},
        Row{"BRICK", "brick"},
        Row{"ARCS", "arcs"});

    const lc::test::OutputDirectory out(std::string("lc_hatch_export_") +
                                         name.m_tag);
    const auto file = out.file(std::string(name.m_tag) + ".dxf");
    const auto file2 = out.file(std::string(name.m_tag) + "_second.dxf");

    auto graphic = makeOneHatchGraphic(QString::fromUtf8(name.m_pattern), 0.0, 1.0);
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    REQUIRE(record.own(78).size() == 1u);
    const int defLines = std::stoi(record.own(78).front());
    CHECK(defLines >= 1);

    RS_Hatch* hatchPtr = nullptr;
    for (RS_Entity* e : *graphic) {
        if (e && e->rtti() == RS2::EntityHatch) {
            hatchPtr = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(hatchPtr != nullptr);
    const auto definition = LC_HatchDefinitionLines::forExport(*hatchPtr);
    const auto group1000 = record.own(1000);
    const bool hasTag = std::any_of(group1000.begin(), group1000.end(),
                                     [](const std::string& v) {
                                         return v == "HATCH_LINES_APPROXIMATE";
                                     });
    CHECK(hasTag == definition.approximate);
    // Until SQUARE and BRICK are realized as exact families this check pins
    // today's placeholder-plus-tag output.
    CHECK(definition.approximate);

    // Re-import, round-trip
    RS_Graphic back;
    REQUIRE(lc::test::importDrawing(back, file, RS2::FormatDXFRW));
    RS_Hatch* reHatch = nullptr;
    for (RS_Entity* e : back) {
        if (e && e->rtti() == RS2::EntityHatch) {
            reHatch = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(reHatch != nullptr);
    CHECK(reHatch->getPattern() == QString::fromUtf8(name.m_pattern));
    CHECK(reHatch->getAngle() == Catch::Approx(0.0).margin(1e-12));
    CHECK(reHatch->getScale() == Catch::Approx(1.0).margin(1e-12));
    reHatch->update();
    CHECK(reHatch->getTotalArea() ==
          Catch::Approx(hatchPtr->getTotalArea()).epsilon(1e-9));

    // The hatch's DRW extData holds no HATCH_LINES_APPROXIMATE after import.
    const auto& importedExt = reHatch->getDrwExtData();
    bool imported = false;
    bool inLibreCad = false;
    for (const auto& tag : importedExt) {
        if (!tag) continue;
        if (tag->code() == 1001 && tag->type() == DRW_Variant::STRING) {
            inLibreCad = tag->content.s && toUpper(*tag->content.s) == "LIBRECAD";
            continue;
        }
        if (inLibreCad && tag->code() == 1000 &&
            tag->type() == DRW_Variant::STRING &&
            tag->content.s && *tag->content.s == "HATCH_LINES_APPROXIMATE") {
            imported = true;
        }
    }
    CHECK_FALSE(imported);

    // A second export holds the tag exactly once.
    REQUIRE(lc::test::exportDrawing(back, file2, RS2::FormatDXFRW2013));
    const auto record2 = firstHatchRecord(file2);
    const auto g1000 = record2.own(1000);
    std::size_t tagCount = 0;
    for (const auto& v : g1000) {
        if (v == "HATCH_LINES_APPROXIMATE") ++tagCount;
    }
    CHECK(tagCount == 1u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A solid hatch is written as before",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_solid");
    const auto file = out.file("solid.dxf");

    // Build a solid hatch whose source already carries the stale approximate
    // tag; the writer must drop it. One container with 4 lines.
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(true, 1.0, 0.0,
                                            QStringLiteral("SOLID")));
    addRectBoundary(*hatch, 0.0, 0.0, 100.0, 60.0);
    // Attach a stale LibreCad / HATCH_LINES_APPROXIMATE XDATA so we can
    // observe the writer's drop.
    std::vector<std::shared_ptr<DRW_Variant>> stale;
    stale.push_back(std::make_shared<DRW_Variant>(1001, std::string("LibreCad")));
    stale.push_back(std::make_shared<DRW_Variant>(1000,
                                                   std::string("HATCH_LINES_APPROXIMATE")));
    hatch->setDrwExtData(stale);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(78) == std::vector<std::string>{"0"});
    CHECK(record.own(76) == std::vector<std::string>{"1"});
    const auto group1000 = record.own(1000);
    for (const auto& v : group1000) {
        CHECK(v != "HATCH_LINES_APPROXIMATE");
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hatch with a missing pattern still writes one line",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_missing");
    const auto file = out.file("no_such.dxf");

    auto graphic = makeOneHatchGraphic(QStringLiteral("NO_SUCH_PATTERN"), 0.0, 2.0);
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(78) == std::vector<std::string>{"1"});
    const auto group45 = record.own(45);
    const auto group46 = record.own(46);
    REQUIRE(group45.size() == 1u);
    REQUIRE(group46.size() == 1u);
    const double mag = std::hypot(std::stod(group45.front()),
                                   std::stod(group46.front()));
    CHECK(mag == Catch::Approx(0.5).margin(1e-9));
    const auto group1000 = record.own(1000);
    bool hasTag = std::any_of(group1000.begin(), group1000.end(),
                               [](const std::string& v) {
                                   return v == "HATCH_LINES_APPROXIMATE";
                               });
    CHECK(hasTag);
}

namespace {

// A Draw-Hatch island: one container holding the outer rectangle followed
// by the inner rectangle, with 8 lines total. The hatch is pattern or solid
// per the caller's choice. Mirrors how RS_ActionDrawHatch builds a boundary.
void addIslandBoundary(RS_Hatch& hatch,
                        double ox1, double oy1, double ox2, double oy2,
                        double ix1, double iy1, double ix2, double iy2) {
    auto* loop = new RS_EntityContainer(&hatch);
    loop->setLayer(nullptr);
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ox1, oy1), RS_Vector(ox2, oy1))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ox2, oy1), RS_Vector(ox2, oy2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ox2, oy2), RS_Vector(ox1, oy2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ox1, oy2), RS_Vector(ox1, oy1))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ix1, iy1), RS_Vector(ix2, iy1))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ix2, iy1), RS_Vector(ix2, iy2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ix2, iy2), RS_Vector(ix1, iy2))));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(ix1, iy2), RS_Vector(ix1, iy1))));
    hatch.addEntity(loop);
}

std::vector<int> sortedInts(std::vector<std::string> xs) {
    std::vector<int> out;
    out.reserve(xs.size());
    for (auto& s : xs) out.push_back(std::stoi(s));
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> sortedStrings(std::vector<std::string> xs) {
    std::sort(xs.begin(), xs.end());
    return xs;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A Draw Hatch island exports two boundary paths flagged External "
          "and Outermost",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_island");
    const auto file = out.file("island.dxf");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, 0.0,
                                            QStringLiteral("ANSI31")));
    addIslandBoundary(*hatch, 0, 0, 100, 60, 30, 20, 70, 40);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(91) == std::vector<std::string>{"2"});
    CHECK(sortedInts(record.own(92)) == std::vector<int>{1, 16});
    CHECK(sortedStrings(record.own(93)) == std::vector<std::string>{"4", "4"});
    CHECK(record.own(75) == std::vector<std::string>{"0"});

    // Re-import gives one container, two loops in it, area 100*60 - 40*20 = 5200.
    RS_Graphic back;
    REQUIRE(lc::test::importDrawing(back, file, RS2::FormatDXFRW));
    RS_Hatch* reHatch = nullptr;
    for (RS_Entity* e : back) {
        if (e && e->rtti() == RS2::EntityHatch) {
            reHatch = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(reHatch != nullptr);
    reHatch->update();
    CHECK(reHatch->countLoops() == 1);
    CHECK(reHatch->countAllLoops() == 2);
    CHECK(reHatch->getTotalArea() == Catch::Approx(5200.0).epsilon(1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A three-level nest exports flags 1, 16 and 0",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_nest3");
    const auto file = out.file("nest3.dxf");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, 0.0,
                                            QStringLiteral("ANSI31")));
    // Three containers, one each per nest level.
    auto make = [&](double x1, double y1, double x2, double y2) {
        auto* loop = new RS_EntityContainer(hatch);
        loop->setLayer(nullptr);
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y1), RS_Vector(x2, y1))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y1), RS_Vector(x2, y2))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y2), RS_Vector(x1, y2))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y2), RS_Vector(x1, y1))));
        hatch->addEntity(loop);
    };
    make(0, 0, 100, 60);
    make(20, 10, 80, 50);
    make(40, 20, 60, 40);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(91) == std::vector<std::string>{"3"});
    CHECK(sortedInts(record.own(92)) == std::vector<int>{0, 1, 16});
    CHECK(record.own(75) == std::vector<std::string>{"0"});

    // Re-import area: outer 100*60 - mid 60*40 + inner 20*20 = 6000-2400+400 = 4000.
    RS_Graphic back;
    REQUIRE(lc::test::importDrawing(back, file, RS2::FormatDXFRW));
    RS_Hatch* reHatch = nullptr;
    for (RS_Entity* e : back) {
        if (e && e->rtti() == RS2::EntityHatch) {
            reHatch = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(reHatch != nullptr);
    reHatch->update();
    CHECK(reHatch->getTotalArea() == Catch::Approx(4000.0).epsilon(1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Disjoint roots are both External",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_disjoint");
    const auto file = out.file("disjoint.dxf");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, 0.0,
                                            QStringLiteral("ANSI31")));
    auto make = [&](double x1, double y1, double x2, double y2) {
        auto* loop = new RS_EntityContainer(hatch);
        loop->setLayer(nullptr);
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y1), RS_Vector(x2, y1))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y1), RS_Vector(x2, y2))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x2, y2), RS_Vector(x1, y2))));
        loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector(x1, y2), RS_Vector(x1, y1))));
        hatch->addEntity(loop);
    };
    make(0, 0, 10, 10);
    make(50, 50, 60, 60);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(91) == std::vector<std::string>{"2"});
    CHECK(sortedInts(record.own(92)) == std::vector<int>{1, 1});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A rotated pattern hatch writes its boundary in WCS",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_rotated");
    const auto file = out.file("rotated.dxf");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    const double angle = 37.0 * kPi / 180.0;
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, angle,
                                            QStringLiteral("ANSI31")));
    addIslandBoundary(*hatch, 0, 0, 100, 60, 30, 20, 70, 40);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    // 10/20 at the HATCH level is the hatch base point (0,0); the per-edge
    // 10/20 and 11/21 pairs come from the exported boundary edges, each one
    // of which must match one of the 8 WCS source corners within 1e-9.
    const auto xs = record.own(10);
    const auto ys = record.own(20);
    const auto xs2 = record.own(11);
    const auto ys2 = record.own(21);
    REQUIRE(xs.size() == 9u);  // 1 base + 8 edge starts
    REQUIRE(ys.size() == 9u);
    REQUIRE(xs2.size() == 8u);
    REQUIRE(ys2.size() == 8u);

    const RS_Vector sourceCorners[] = {
        RS_Vector(0.0, 0.0), RS_Vector(100.0, 0.0),
        RS_Vector(100.0, 60.0), RS_Vector(0.0, 60.0),
        RS_Vector(30.0, 20.0), RS_Vector(70.0, 20.0),
        RS_Vector(70.0, 40.0), RS_Vector(30.0, 40.0),
    };
    const auto matchesACorner = [&](double x, double y) {
        for (const auto& c : sourceCorners) {
            if (std::fabs(c.x - x) < 1e-9 && std::fabs(c.y - y) < 1e-9)
                return true;
        }
        return false;
    };
    for (std::size_t i = 1; i < xs.size(); ++i) {
        CAPTURE(xs[i], ys[i]);
        CHECK(matchesACorner(std::stod(xs[i]), std::stod(ys[i])));
    }
    for (std::size_t i = 0; i < xs2.size(); ++i) {
        CAPTURE(xs2[i], ys2[i]);
        CHECK(matchesACorner(std::stod(xs2[i]), std::stod(ys2[i])));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A solid hatch with an island is flagged 1 and 16",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const lc::test::OutputDirectory out("lc_hatch_export_solid_island");
    const auto file = out.file("solid_island.dxf");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(true, 1.0, 0.0,
                                            QStringLiteral("SOLID")));
    addIslandBoundary(*hatch, 0, 0, 100, 60, 30, 20, 70, 40);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDXFRW2013));

    const auto record = firstHatchRecord(file);
    CHECK(record.own(91) == std::vector<std::string>{"2"});
    CHECK(sortedInts(record.own(92)) == std::vector<int>{1, 16});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("exportLoops hands a single-spline boundary back as one SPLINE "
          "edge instead of chord lines",
          "[lc-hatchfix][hatch-export]") {
    // Exercise RS_Hatch::exportLoops()'s single-spline exception without
    // round-tripping the write path. A pre-existing mismatch in
    // makeDrwSplineFromRSSpline (unwrapped controls vs wrapped knots on a
    // closed RS_Spline) refuses the HATCH payload at validateHatchPayload;
    // that lives outside this step's scope.
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, 0.0,
                                            QStringLiteral("ANSI31")));
    auto* loop = new RS_EntityContainer(hatch);
    loop->setLayer(nullptr);
    auto* spline = new RS_Spline(loop, RS_SplineData(3, false));
    spline->addControlPoint(RS_Vector(0.0, 0.0));
    spline->addControlPoint(RS_Vector(60.0, 0.0));
    spline->addControlPoint(RS_Vector(60.0, 40.0));
    spline->addControlPoint(RS_Vector(0.0, 40.0));
    spline->setClosed(true);
    spline->update();
    loop->addEntity(spline);
    hatch->addEntity(loop);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const auto exported = hatch->exportLoops();
    REQUIRE(exported.size() == 1u);
    REQUIRE(exported.front().edges != nullptr);
    REQUIRE(exported.front().depth == 0);
    int nSpline = 0, nLine = 0, nOther = 0;
    for (RS_Entity* e : *exported.front().edges) {
        if (!e) continue;
        if (e->rtti() == RS2::EntitySpline) ++nSpline;
        else if (e->rtti() == RS2::EntityLine) ++nLine;
        else ++nOther;
    }
    CHECK(nSpline == 1);
    CHECK(nLine == 0);
    CHECK(nOther == 0);
}

#ifdef DWGSUPPORT
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The DWG export carries the boundary flags",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_dwg_island");
    const auto file = out.file("dwg_island.dwg");

    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(),
                               RS_HatchData(false, 1.0, 0.0,
                                            QStringLiteral("ANSI31")));
    addIslandBoundary(*hatch, 0, 0, 100, 60, 30, 20, 70, 40);
    graphic->addEntity(hatch);
    hatch->update();
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDWG2018));

    HatchCapture cap;
    {
        dwgR reader(file.string().c_str());
        REQUIRE(reader.read(&cap, /*ext=*/true));
    }
    REQUIRE(cap.m_hatchCount == 1);
    REQUIRE(cap.m_hatch.looplist.size() == 2u);
    std::vector<int> types;
    for (const auto& l : cap.m_hatch.looplist) {
        types.push_back(l->type);
    }
    std::sort(types.begin(), types.end());
    CHECK(types == std::vector<int>{1, 16});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The DWG export carries the line, pattern type 2 and the tag",
          "[lc-hatchfix][hatch-export]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    lc::test::SettingGuard patterns{RS_SETTINGS, "Paths", "Patterns"};
    patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));

    const lc::test::OutputDirectory out("lc_hatch_export_dwg");
    const auto file = out.file("paisley.dwg");

    auto graphic = makeOneHatchGraphic(QStringLiteral("PAISLEY"), kPi / 6.0, 1.0);
    REQUIRE(lc::test::exportDrawing(*graphic, file, RS2::FormatDWG2018));

    HatchCapture cap;
    {
        dwgR reader(file.string().c_str());
        REQUIRE(reader.read(&cap, /*ext=*/true));
    }
    REQUIRE(cap.m_hatchCount == 1);
    REQUIRE(cap.m_hatch.patternLines.size() == 1u);
    CHECK(cap.m_hatch.hpattern == 2);
    CHECK(cap.m_hatch.angle == Catch::Approx(kPi / 6.0).margin(1e-12));
    CHECK(cap.m_hatch.patternLines.at(0).angle
          == Catch::Approx(5.0 * kPi / 12.0).margin(1e-12));

    // 1001 LibreCad followed by 1000 HATCH_LINES_APPROXIMATE in the extData.
    bool found = false;
    bool inLibreCad = false;
    for (const auto& tag : cap.m_hatch.extData) {
        if (!tag) continue;
        if (tag->code() == 1001 && tag->type() == DRW_Variant::STRING) {
            inLibreCad = tag->content.s && toUpper(*tag->content.s) == "LIBRECAD";
            continue;
        }
        if (inLibreCad && tag->code() == 1000 &&
            tag->type() == DRW_Variant::STRING &&
            tag->content.s && *tag->content.s == "HATCH_LINES_APPROXIMATE") {
            found = true;
        }
    }
    CHECK(found);

    // After importDrawing the tag is stripped from the entity's extData.
    RS_Graphic back;
    REQUIRE(lc::test::importDrawing(back, file, RS2::FormatDWG));
    RS_Hatch* reHatch = nullptr;
    for (RS_Entity* e : back) {
        if (e && e->rtti() == RS2::EntityHatch) {
            reHatch = static_cast<RS_Hatch*>(e);
            break;
        }
    }
    REQUIRE(reHatch != nullptr);
    bool importedTag = false;
    bool inLibreCad2 = false;
    for (const auto& tag : reHatch->getDrwExtData()) {
        if (!tag) continue;
        if (tag->code() == 1001 && tag->type() == DRW_Variant::STRING) {
            inLibreCad2 = tag->content.s && toUpper(*tag->content.s) == "LIBRECAD";
            continue;
        }
        if (inLibreCad2 && tag->code() == 1000 &&
            tag->type() == DRW_Variant::STRING &&
            tag->content.s && *tag->content.s == "HATCH_LINES_APPROXIMATE") {
            importedTag = true;
        }
    }
    CHECK_FALSE(importedTag);
}
#endif
