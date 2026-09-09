/****************************************************************************
** Byte-identity harness (scaffolding, not part of the shipped suite).
**
** Section 1 of the DWG/DXF improvement plan: a behaviour-preserving refactor
** of the encoders cannot be validated by the unit tests alone, because most of
** the encoder surface is only reachable through a full write. This harness
** exports one fixed document to every writable format and prints a SHA-256 per
** output. Run it before a refactor, save the output, run it after, and diff.
**
**   ./librecad_tests "[byteid]" 2>/dev/null | grep BYTEID | sort > before.txt
**   ...refactor...
**   ./librecad_tests "[byteid]" 2>/dev/null | grep BYTEID | sort > after.txt
**   diff before.txt after.txt
**
** Any difference must be explained or the change reverted.
**********************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>

#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_text.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance()
                                        ? QCoreApplication::instance()
                                        : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

// Populate a graphic with a spread of entity kinds so the export touches as
// much of the writer surface as one document reasonably can.
void buildFixture(RS_Graphic& g) {
    g.initForNewDocument();

    auto* extra = new RS_Layer("HARNESS");
    extra->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width05, RS2::DashLine));
    g.addLayer(extra);

    g.addEntity(new RS_Line(&g, {{0.0, 0.0}, {100.0, 50.0}}));
    g.addEntity(new RS_Line(&g, {{100.0, 50.0}, {0.0, 50.0}}));
    g.addEntity(new RS_Circle(&g, RS_CircleData(RS_Vector(20.0, 20.0), 7.5)));
    g.addEntity(new RS_Arc(
        &g, RS_ArcData(RS_Vector(40.0, 20.0), 5.0, 0.0, 1.57, false)));
    g.addEntity(new RS_Ellipse(
        &g, {RS_Vector(60.0, 20.0), RS_Vector(10.0, 0.0), 0.5, 0.0, 0.0, false}));

    auto* poly = new RS_Polyline(&g);
    poly->addVertex(RS_Vector(0.0, 0.0));
    poly->addVertex(RS_Vector(10.0, 5.0));
    poly->addVertex(RS_Vector(20.0, 0.0), 0.25);
    poly->endPolyline();
    g.addEntity(poly);

    g.addEntity(new RS_Text(
        &g, RS_TextData(RS_Vector(5.0, 40.0), RS_Vector(5.0, 40.0), 2.5, 1.0,
                        RS_TextData::VABaseline, RS_TextData::HALeft,
                        RS_TextData::None, "harness", "standard", 0.0)));

    // Spline-bordered hatch: exercises the boundary-edge encoder.
    auto* hatch = new RS_Hatch(&g, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    LC_SplinePointsData sd(/*closed*/ true, /*cut*/ false);
    sd.useControlPoints = true;
    sd.controlPoints = {RS_Vector(70.0, 40.0), RS_Vector(65.0, 45.0),
                        RS_Vector(70.0, 50.0), RS_Vector(75.0, 45.0)};
    loop->addEntity(new LC_SplinePoints(loop, std::move(sd)));
    g.addEntity(hatch);
    hatch->update();

    // A block plus an insert of it.
    auto* block = new RS_Block(&g, RS_BlockData("HARNESS_BLOCK",
                                                RS_Vector(0.0, 0.0), false));
    block->addEntity(new RS_Line(block, {{0.0, 0.0}, {5.0, 0.0}}));
    block->addEntity(new RS_Line(block, {{5.0, 0.0}, {5.0, 5.0}}));
    g.addBlock(block);
    g.addEntity(new RS_Insert(
        &g, RS_InsertData("HARNESS_BLOCK", RS_Vector(80.0, 5.0),
                          RS_Vector(1.0, 1.0), 0.0, 1, 1,
                          RS_Vector(0.0, 0.0))));
}

std::string sha256Of(const std::string& path) {
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly))
        return "<unreadable>";
    QCryptographicHash h(QCryptographicHash::Sha256);
    h.addData(&f);
    return h.result().toHex().toStdString();
}

struct Target {
    const char* label;
    RS2::FormatType format;
    const char* suffix;
};

} // namespace

TEST_CASE("byte-identity harness: export a fixed document to every format",
          "[.][byteid]") {
    ensureSettings();

    const std::vector<Target> targets{
        {"DXFRW  ", RS2::FormatDXFRW, "dxf"},
        {"DWG_R15", RS2::FormatDWG, "dwg"},
        {"DWG2004", RS2::FormatDWG2004, "dwg"},
        {"DWG2007", RS2::FormatDWG2007, "dwg"},
        {"DWG2010", RS2::FormatDWG2010, "dwg"},
        {"DWG2013", RS2::FormatDWG2013, "dwg"},
        {"DWG2018", RS2::FormatDWG2018, "dwg"},
    };

    const auto dir = std::filesystem::temp_directory_path() / "lc_byteid";
    std::filesystem::create_directories(dir);

    for (const Target& t : targets) {
        RS_Graphic g;
        buildFixture(g);

        const std::string out =
            (dir / (std::string("harness_") + t.label + "." + t.suffix)).string();
        std::filesystem::remove(out);

        RS_FilterDXFRW filter;
        const bool ok =
            filter.fileExport(g, QString::fromStdString(out), t.format);
        const std::string digest = ok ? sha256Of(out) : std::string("<write-failed>");
        const std::uintmax_t size =
            ok && std::filesystem::exists(out) ? std::filesystem::file_size(out) : 0;
        std::fprintf(stderr, "BYTEID %s %-12s %10ju  %s\n", t.label,
                     ok ? "ok" : "FAILED", static_cast<std::uintmax_t>(size),
                     digest.c_str());
    }
    SUCCEED("harness output is on stderr");
}
