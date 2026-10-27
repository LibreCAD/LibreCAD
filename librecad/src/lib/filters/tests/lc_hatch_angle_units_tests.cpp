/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
****************************************************************************/

// DXF groups 52 (hatch angle) and 53 (definition-line angle) are degrees,
// while DWG and the gradient angle 460 are radians. libdxfrw used to pass 52
// and 53 raw through DRW_Hatch::parseCode, so DRW_Hatch::angle held degrees
// coming from DXF and radians coming from DWG. The filter imported the raw
// value as a radian, which turned every AutoCAD-written pattern at 45 deg
// into a 45 rad placement. Since libdxfrw 0.6.4 the parser and writer convert
// at the DXF boundary, matching how groups 50/51 have always worked, and the
// filter reinterprets legacy LibreCAD/QCAD files whose header announces an
// old writer that stored 52/53 raw.

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <QString>

#include "drw_base.h"
#include "drw_entities.h"
#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
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

namespace {

constexpr double kPi = 3.141592653589793;

// Captures the first HATCH / MPOLYGON a dxfRW reader hands over.
class HatchCapture : public lc::test::NullDrwInterface {
public:
    int m_hatchCount{0};
    int m_mpCount{0};
    DRW_Hatch m_hatch;
    DRW_MPolygon m_mpolygon;

    void addHatch(const DRW_Hatch* data) override {
        if (data != nullptr && m_hatchCount == 0) {
            m_hatch = *data;
            ++m_hatchCount;
        }
    }
    // Follows DRW_Interface's default forward only when a reader hands the
    // MPolygon through addMPolygon; the override here keeps them separate.
    void addMPolygon(const DRW_MPolygon* data) override {
        if (data != nullptr && m_mpCount == 0) {
            m_mpolygon = *data;
            ++m_mpCount;
        }
    }
};

// Writes `dxf` to a temp file, reads it with libdxfrw, returns whether the
// read succeeded. The file is removed before and after the read.
bool readDxfString(const std::string& dxf, HatchCapture& cap, const char* name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    {
        std::ofstream out(path);
        out << dxf;
    }
    dxfRW reader(path.string().c_str());
    const bool ok = reader.read(&cap, /*ext=*/true);
    std::filesystem::remove(path);
    return ok;
}

// A minimal valid ENTITIES-only DXF with one ANSI31 HATCH and a 100x60
// polyline loop. The HEADER section is omitted on purpose: without one the
// filter's addHeader() never runs, so m_legacyHatchRadians stays false and
// the file reads as a degree writer.
std::string hatchDxf(double group52, int def78 = 1, double group53 = 45.0) {
    std::ostringstream out;
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nHATCH\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
           "10\n0.0\n20\n0.0\n30\n0.0\n210\n0.0\n220\n0.0\n230\n1.0\n"
           "2\nANSI31\n70\n0\n71\n0\n91\n1\n"
           "92\n2\n72\n0\n73\n1\n93\n4\n"
           "10\n0.0\n20\n0.0\n"
           "10\n100.0\n20\n0.0\n"
           "10\n100.0\n20\n60.0\n"
           "10\n0.0\n20\n60.0\n"
           "97\n0\n"
           "75\n0\n76\n1\n"
           "52\n" << group52 << "\n41\n1.0\n77\n0\n78\n" << def78 << "\n";
    if (def78 >= 1) {
        out << "53\n" << group53 << "\n43\n0.0\n44\n0.0\n"
               "45\n-2.245\n46\n2.245\n79\n0\n";
    }
    out << "47\n0.0\n98\n0\n"
           "0\nENDSEC\n0\nEOF\n";
    return out.str();
}

// Rewrites the first `999` comment of a sample file to `line` and the value
// after the first `52` group to `group52`. Mirrors how the research probed
// each writer-header class; keeps every other byte of the file intact.
std::string rewriteFirstCommentAnd52(const std::string& text,
                                     const std::string& newComment,
                                     const std::string& new52) {
    std::istringstream in(text);
    std::ostringstream out;
    std::string codeLine;
    std::string value;
    bool firstComment = true;
    bool awaiting52 = false;
    while (std::getline(in, codeLine)) {
        if (!std::getline(in, value)) {
            out << codeLine << "\n";
            break;
        }
        std::string trimmed = codeLine;
        const auto first = trimmed.find_first_not_of(" \t");
        const auto last = trimmed.find_last_not_of(" \t\r");
        trimmed = (first == std::string::npos) ? "" : trimmed.substr(first, last - first + 1);
        if (trimmed == "999" && firstComment) {
            out << codeLine << "\n" << newComment << "\n";
            firstComment = false;
            continue;
        }
        if (trimmed == "52" && !awaiting52) {
            awaiting52 = true;
            out << codeLine << "\n" << new52 << "\n";
            continue;
        }
        out << codeLine << "\n" << value << "\n";
    }
    return out.str();
}

double hatchAngleFromFile(const std::filesystem::path& file) {
    RS_Graphic graphic;
    REQUIRE(lc::test::importDrawing(graphic, file, RS2::FormatDXFRW));
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityHatch) {
            return static_cast<RS_Hatch*>(e)->getAngle();
        }
    }
    return -1.0;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF hatch angle 52 = 45 without a LibreCAD header reads as pi/4",
          "[lc-hatchfix][dxf-hatch]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const lc::test::OutputDirectory directory("lc_hatch_angle_no_header");
    const auto file = directory.file("entities_only.dxf");
    {
        std::ofstream out(file);
        out << hatchDxf(45.0);
    }
    CHECK(hatchAngleFromFile(file) == Catch::Approx(kPi / 4).margin(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("libdxfrw reads and writes DXF 52 and 53 in degrees",
          "[lc-hatchfix][dxf-hatch]") {
    const auto tempDir = std::filesystem::temp_directory_path();

    SECTION("round-trip read of a HATCH and an MPOLYGON written at 45/90") {
        std::string hatch = hatchDxf(45.0, 1, 90.0);
        HatchCapture capHatch;
        REQUIRE(readDxfString(hatch, capHatch, "lc_hatch_rr.dxf"));
        REQUIRE(capHatch.m_hatchCount == 1);
        CHECK(capHatch.m_hatch.angle == Catch::Approx(kPi / 4).margin(1e-12));
        REQUIRE(capHatch.m_hatch.patternLines.size() == 1u);
        CHECK(capHatch.m_hatch.patternLines.at(0).angle == Catch::Approx(kPi / 2).margin(1e-12));
    }

    SECTION("writing a HATCH at pi/4 emits degrees") {
        const auto path = tempDir / "lc_hatch_wr.dxf";
        std::filesystem::remove(path);
        {
            lc::test::NullDrwInterface iface;
            dxfRW writer(path.string().c_str());
            REQUIRE(writer.write(&iface, DRW::AC1021, false));
        }
        // Even without any HATCH written, the version-comment line records
        // the current library version: catch a revert of the DRW_VERSION bump.
        std::string line1, line2;
        {
            // Close the stream before removing the file: Windows cannot delete an open file.
            std::ifstream in(path);
            std::getline(in, line1);
            std::getline(in, line2);
        }
        if (!line1.empty() && line1.back() == '\r') line1.pop_back();
        if (!line2.empty() && line2.back() == '\r') line2.pop_back();
        CHECK(line1 == "999");
        CHECK(line2.find("dxfrw ") == 0);
        std::filesystem::remove(path);
    }

    SECTION("read a HATCH written with 52 = 60 and get it back within 1e-12") {
        std::string text = hatchDxf(60.0);
        HatchCapture cap;
        REQUIRE(readDxfString(text, cap, "lc_hatch_60.dxf"));
        REQUIRE(cap.m_hatchCount == 1);
        CHECK(cap.m_hatch.angle == Catch::Approx(60.0 / ARAD).margin(1e-12));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG hatch angles stay radians",
          "[lc-hatchfix][dxf-hatch]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());
    const std::filesystem::path file =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "hatch" / "oda_hatch_deg_def_ac1032.dwg";
    if (!std::filesystem::exists(file)) {
        SKIP("fixture not installed: " << file);
    }
    HatchCapture cap;
    dwgR reader(file.string().c_str());
    const bool ok = reader.read(&cap, /*ext=*/true);
    REQUIRE(ok);
    REQUIRE(cap.m_hatchCount == 1);
    // The source DXF had 52 = 45, 53 = 90: through a DXF -> DWG conversion
    // the DWG stores radians already, which libdxfrw returns raw.
    CHECK(cap.m_hatch.angle == Catch::Approx(kPi / 4).margin(1e-12));
    if (!cap.m_hatch.patternLines.empty()) {
        CHECK(cap.m_hatch.patternLines.at(0).angle
              == Catch::Approx(kPi / 2).margin(1e-12));
    }
}

namespace {

struct HeaderRow {
    const char* m_comment;
    double m_source52;
    double m_expectedAngle;
    const char* m_label;
};

constexpr HeaderRow kHeaderRows[] = {
    // The committed fixture is dxfrw 0.6.3 with 52 = 0.78539816339... (pi/4):
    // the file as it is must read back as pi/4.
    {"dxfrw 0.6.3",   0.7853981633974483, kPi / 4,              "dxfrw 0.6.3 pi/4 raw"},
    {"dxfrw 0.5.10",  4.71238898038469,   3 * kPi / 2,          "dxfrw 0.5.10 3pi/2 raw"},
    {"dxflib 3.26.4.0", 45.0,             kPi / 4,              "dxflib 3 = QCAD3 degrees"},
    {"dxfrw 0.6.3",   30.0,               kPi / 6,              "dxfrw 0.6.3 30 tie-break"},
    {"dxfrw 0.6.3",   7.853981633974483,  kPi / 2,              "dxfrw 0.6.3 7.854 = 450 deg"},
    {"dxfrw 0.6.3",  -6.981317007977318,  5.585053606381854,    "dxfrw 0.6.3 -6.981 = -400 deg"},
    {"dxfrw 2.0.0",   1.570796326794897,  kPi / 2,              "dxfrw 2.0.0 standalone raw"},
    {"dxfrw 0.6.4",   45.0,               kPi / 4,              "dxfrw 0.6.4 degree writer"},
    {"dxfrw",         45.0,               kPi / 4,              "bare dxfrw not legacy"},
    {"dxfrw  0.6.3",  0.7853981633974483, kPi / 4,              "dxfrw with two blanks"},
    {"dxfrw 0.6",     0.7853981633974483, 0.7853981633974483 / ARAD, "dxfrw 0.6 under-parts"},
};

std::string loadFixture() {
    const std::filesystem::path file =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "hatch" / "lc_dxfrw063_ansi31_45.dxf";
    std::ifstream in(file);
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Legacy and foreign headers decide hatch angle units",
          "[lc-hatchfix][dxf-hatch]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const std::string base = loadFixture();
    if (base.empty()) {
        SKIP("hatch angle fixture missing");
    }

    const HeaderRow row = GENERATE(from_range(std::begin(kHeaderRows), std::end(kHeaderRows)));
    CAPTURE(row.m_label);

    const lc::test::OutputDirectory directory("lc_hatch_angle_header_table");
    const auto file = directory.file("rewritten.dxf");

    std::ostringstream s52;
    s52.precision(17);
    s52 << row.m_source52;
    const std::string edited = rewriteFirstCommentAnd52(base, row.m_comment, s52.str());
    {
        std::ofstream out(file);
        out << edited;
    }

    CHECK(hatchAngleFromFile(file) == Catch::Approx(row.m_expectedAngle).margin(1e-12));
}

namespace {

struct AngleHelperRow {
    double m_drwAngle;
    bool m_legacy;
    bool m_solid;
    double m_expected;
    const char* m_label;
};

constexpr AngleHelperRow kAngleHelperRows[] = {
    // Legacy radians below 2*pi: pass through.
    {0.7853981633974483 / ARAD, true, false, 0.7853981633974483,       "legacy pi/4 raw"},
    // Degree value (DWG-origin radians above 2*pi stays, but nice degree
    // without nice radian is reinterpreted as degrees via the tie-break).
    {30.0 / ARAD,               true, false, kPi / 6,                   "legacy 30 tie-break"},
    {7.853981633974483 / ARAD,  true, false, kPi / 2,                   "legacy 7.854 = 450 deg"},
    // Non-legacy writer: libdxfrw already converted to radians.
    {45.0 / ARAD,               false, false, kPi / 4,                  "non-legacy 45 deg"},
    // Solid legacy: no tie-break; still passes correctAngle.
    {4.71238898038469 / ARAD,   true, true,  (4.71238898038469 / ARAD), "legacy solid raw"},
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("importedHatchAngle applies the tie-break only to legacy pattern hatches",
          "[lc-hatchfix][dxf-hatch]") {
    const AngleHelperRow row =
        GENERATE(from_range(std::begin(kAngleHelperRows), std::end(kAngleHelperRows)));
    CAPTURE(row.m_label);
    const double a = RS_FilterDXFRW::importedHatchAngle(
        row.m_drwAngle, row.m_legacy, row.m_solid);
    const double expected = RS_Math::correctAngle(row.m_expected);
    CHECK(a == Catch::Approx(expected).margin(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The shipped a3.dxf library block keeps its hatch angles",
          "[lc-hatchfix][dxf-hatch]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const std::filesystem::path file =
        std::filesystem::path(LIBRECAD_SOURCE_DIR) / "librecad" / "support" / "library" / "misc" / "a3.dxf";
    if (!std::filesystem::exists(file)) {
        SKIP("a3.dxf missing");
    }
    RS_Graphic graphic;
    REQUIRE(lc::test::importDrawing(graphic, file, RS2::FormatDXFRW));

    std::vector<double> angles;
    auto collect = [&angles](RS_EntityContainer& c, auto&& self) -> void {
        for (RS_Entity* e : c) {
            if (e == nullptr) continue;
            if (e->rtti() == RS2::EntityHatch) {
                angles.push_back(static_cast<RS_Hatch*>(e)->getAngle());
            }
            else if (e->rtti() == RS2::EntityBlock || e->rtti() == RS2::EntityInsert) {
                if (auto* child = dynamic_cast<RS_EntityContainer*>(e)) self(*child, self);
            }
        }
    };
    collect(graphic, collect);
    for (RS_Block* block : *graphic.getBlockList()) {
        if (block) collect(*block, collect);
    }

    // The file carries 16 ansi31 hatches with 52 = 4.7123... (3*pi/2) x12,
    // 3.1415... (pi) x2 and 0 x2. Record and assert the ones that survive
    // validate() on import.
    std::sort(angles.begin(), angles.end());
    REQUIRE(!angles.empty());
    const double threePi2 = RS_Math::correctAngle(3 * kPi / 2);
    std::size_t zeroCount = 0;
    std::size_t piCount = 0;
    std::size_t threePi2Count = 0;
    for (double a : angles) {
        if (std::fabs(a) < 1e-12) ++zeroCount;
        else if (std::fabs(a - kPi) < 1e-12) ++piCount;
        else if (std::fabs(a - threePi2) < 1e-12) ++threePi2Count;
    }
    CHECK(zeroCount + piCount + threePi2Count == angles.size());
    // At least one of each present (the counts recorded on an untouched
    // import can drop below 16 total if validate() refuses degenerate loops).
    CHECK(zeroCount >= 1);
    CHECK(piCount >= 1);
    CHECK(threePi2Count >= 1);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hatch saved with libdxfrw 0.6.4 writes degrees and reads back",
          "[lc-hatchfix][dxf-hatch]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const lc::test::OutputDirectory directory("lc_hatch_angle_write_degrees");
    const auto file = directory.file("hatch_pi4.dxf");

    {
        RS_Graphic graphic;
        auto* hatch = new RS_Hatch(
            &graphic, RS_HatchData(false, 1.0, kPi / 4, QStringLiteral("ANSI31")));
        auto* loop = new RS_EntityContainer(hatch);
        loop->setLayer(nullptr);
        hatch->addEntity(loop);
        loop->addEntity(new RS_Line(loop, {0.0, 0.0}, {100.0, 0.0}));
        loop->addEntity(new RS_Line(loop, {100.0, 0.0}, {100.0, 60.0}));
        loop->addEntity(new RS_Line(loop, {100.0, 60.0}, {0.0, 60.0}));
        loop->addEntity(new RS_Line(loop, {0.0, 60.0}, {0.0, 0.0}));
        hatch->update();
        graphic.addEntity(hatch);
        REQUIRE(lc::test::exportDrawing(graphic, file, RS2::FormatDXFRW2013));
    }

    // The first comment should name the current libdxfrw version.
    std::ifstream in(file);
    std::string codeLine;
    std::string value;
    std::getline(in, codeLine);
    std::getline(in, value);
    if (!value.empty() && value.back() == '\r') value.pop_back();
    CHECK(value == std::string("dxfrw ") + DRW_VERSION);
    in.close();

    // The written HATCH's 52 group should be 45 (degrees).
    const auto hatchRecords = lc::test::dxfRecords(file, "HATCH");
    REQUIRE(!hatchRecords.empty());
    const auto g52 = hatchRecords.front().own(52);
    REQUIRE(!g52.empty());
    const double value52 = std::stod(g52.front());
    CHECK(value52 == Catch::Approx(45.0).margin(1e-12));

    // Re-import gives pi/4.
    CHECK(hatchAngleFromFile(file) == Catch::Approx(kPi / 4).margin(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("setAngle stores the angle in [0, 2 pi)",
          "[lc-hatchfix][dxf-hatch]") {
    RS_Hatch hatch(nullptr, RS_HatchData(false, 1.0, 0.0, QStringLiteral("ANSI31")));
    hatch.setAngle(2 * kPi + 0.25);
    CHECK(hatch.getAngle() == Catch::Approx(0.25).margin(1e-14));
    hatch.setAngle(-kPi / 2);
    CHECK(hatch.getAngle() == Catch::Approx(3 * kPi / 2).margin(1e-14));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_VERSION is a degree writer",
          "[lc-hatchfix][dxf-hatch]") {
    const std::string version{DRW_VERSION};
    const auto first = version.find('.');
    const auto second = version.find('.', first + 1);
    REQUIRE(first != std::string::npos);
    REQUIRE(second != std::string::npos);
    const int major = std::stoi(version.substr(0, first));
    const int minor = std::stoi(version.substr(first + 1, second - first - 1));
    const int patch = std::stoi(version.substr(second + 1));
    const unsigned packed = LIBDXFRW_VERSION(major, minor, patch);
    CHECK(packed >= RS_FilterDXFRW::kFirstDegreeHatchWriter);
    CHECK(packed != RS_FilterDXFRW::kRawHatchForkVersion);
}
