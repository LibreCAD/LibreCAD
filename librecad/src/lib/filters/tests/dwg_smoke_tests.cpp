/**
 * DWG smoke tests: open each test file with dwgR and report error code +
 * entity/block/layer counts.  No assertions are made on empty drawings —
 * this is a diagnostic harness, not a correctness test.
 *
 * Results are printed to stdout in tab-separated form regardless of pass/fail
 * so the output is easy to scan after one test run.
 *
 * Files that are known to have bad magic bytes (e.g., #mechanical_example-*)
 * are expected to return BAD_VERSION and are NOT checked.
 */

#include <catch2/catch_test_macros.hpp>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "drw_interface.h"
#include "libdwgr.h"

namespace {

// ---- minimal counting DRW_Interface ----------------------------------------

class CountingIface : public DRW_Interface {
public:
    int entities = 0;
    int blocks   = 0;
    int layers   = 0;

    // tables
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override { ++layers; }
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}

    // blocks
    void addBlock(const DRW_Block&) override { ++blocks; }
    void setBlock(const int) override {}
    void endBlock() override {}

    // entities — all increment the counter
    void addPoint(const DRW_Point&) override { ++entities; }
    void addLine(const DRW_Line&) override { ++entities; }
    void addRay(const DRW_Ray&) override { ++entities; }
    void addXline(const DRW_Xline&) override { ++entities; }
    void addArc(const DRW_Arc&) override { ++entities; }
    void addCircle(const DRW_Circle&) override { ++entities; }
    void addEllipse(const DRW_Ellipse&) override { ++entities; }
    void addLWPolyline(const DRW_LWPolyline&) override { ++entities; }
    void addPolyline(const DRW_Polyline&) override { ++entities; }
    void addSpline(const DRW_Spline*) override { ++entities; }
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override { ++entities; }
    void addTrace(const DRW_Trace&) override { ++entities; }
    void add3dFace(const DRW_3Dface&) override { ++entities; }
    void addSolid(const DRW_Solid&) override { ++entities; }
    void addMText(const DRW_MText&) override { ++entities; }
    void addText(const DRW_Text&) override { ++entities; }
    void addDimAlign(const DRW_DimAligned*) override { ++entities; }
    void addDimLinear(const DRW_DimLinear*) override { ++entities; }
    void addDimRadial(const DRW_DimRadial*) override { ++entities; }
    void addDimDiametric(const DRW_DimDiametric*) override { ++entities; }
    void addDimAngular(const DRW_DimAngular*) override { ++entities; }
    void addDimAngular3P(const DRW_DimAngular3p*) override { ++entities; }
    void addDimOrdinate(const DRW_DimOrdinate*) override { ++entities; }
    void addLeader(const DRW_Leader*) override { ++entities; }
    void addHatch(const DRW_Hatch*) override { ++entities; }
    void addViewport(const DRW_Viewport&) override { ++entities; }
    void addImage(const DRW_Image*) override { ++entities; }
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}

    // write callbacks (not used when reading)
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
};

// ---- helpers ----------------------------------------------------------------

struct DwgResult {
    bool       ok;
    DRW::error error;
    DRW::Version version;
    int entities;
    int blocks;
    int layers;
};

DwgResult readDwg(const std::string& path, bool verbose = false) {
    CountingIface iface;
    try {
        dwgR reader(path.c_str());
        if (verbose)
            reader.setDebug(DRW::DebugLevel::Debug);
        bool ok = reader.read(&iface, true);
        return {ok, reader.getError(), reader.getVersion(),
                iface.entities, iface.blocks, iface.layers};
    } catch (const std::exception& e) {
        std::cerr << "  [EXCEPTION: " << e.what() << "]\n";
        return {false, DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
                iface.entities, iface.blocks, iface.layers};
    } catch (...) {
        std::cerr << "  [UNKNOWN EXCEPTION]\n";
        return {false, DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
                iface.entities, iface.blocks, iface.layers};
    }
}

const char* versionStr(DRW::Version v) {
    switch (v) {
        case DRW::AC1012: return "AC1012/R13";
        case DRW::AC1014: return "AC1014/R14";
        case DRW::AC1015: return "AC1015/R2000";
        case DRW::AC1018: return "AC1018/R2004";
        case DRW::AC1021: return "AC1021/R2007";
        case DRW::AC1024: return "AC1024/R2010";
        case DRW::AC1027: return "AC1027/R2013";
        case DRW::AC1032: return "AC1032/R2018";
        default:          return "UNKNOWN";
    }
}

const char* errorStr(DRW::error e) {
    switch (e) {
        case DRW::BAD_NONE:             return "OK";
        case DRW::BAD_UNKNOWN:          return "BAD_UNKNOWN";
        case DRW::BAD_OPEN:             return "BAD_OPEN";
        case DRW::BAD_VERSION:          return "BAD_VERSION";
        case DRW::BAD_READ_METADATA:    return "BAD_READ_METADATA";
        case DRW::BAD_READ_FILE_HEADER: return "BAD_READ_FILE_HEADER";
        case DRW::BAD_READ_HEADER:      return "BAD_READ_HEADER";
        case DRW::BAD_READ_CLASSES:     return "BAD_READ_CLASSES";
        case DRW::BAD_READ_HANDLES:     return "BAD_READ_HANDLES";
        case DRW::BAD_READ_TABLES:      return "BAD_READ_TABLES";
        case DRW::BAD_READ_BLOCKS:      return "BAD_READ_BLOCKS";
        case DRW::BAD_READ_ENTITIES:    return "BAD_READ_ENTITIES";
        case DRW::BAD_READ_OBJECTS:     return "BAD_READ_OBJECTS";
        case DRW::BAD_READ_SECTION:     return "BAD_READ_SECTION";
        default:                        return "UNKNOWN_ERROR";
    }
}

struct FileInfo {
    const char* name;
    bool expectSuccess;
};

const FileInfo kTestFiles[] = {
    // AC1018 (R2004)
    {"Extruder2.dwg",                                            true},
    {"ET-Drawing-with-Border.dwg",                               true},
    // AC1021 (R2007)
    {"colorwh.dwg",                                              true},
    {"dwgreader21_230.dwg",                                      true},
    {"blocks_and_tables_-_imperial.dwg",                         true},
    {"blocks_and_tables_-_metric.dwg",                           true},
    {"lineweights.dwg",                                          true},
    {"truetype.dwg",                                             true},
    {"tablet.dwg",                                               true},
    // AC1024 (R2010)
    {"architectural_-_annotation_scaling_and_multileaders.dwg",  true},
    {"architectural_example-imperial.dwg",                       true},
    {"children-room-decoration.dwg",                             true},
    {"civil_example-imperial.dwg",                               true},
    {"mechanical_example-imperial (1).dwg",                      true},
    {"mechanical_example-imperial.dwg",                          true},
    {"#mechanical_example-imperial.dwg",                         false}, // bad magic bytes
    {"plot_screening_and_fill_patterns.dwg",                     true},
    {"title_block-arch.dwg",                                     true},
    {"title_block-iso.dwg",                                      true},
    {"visualization_-_aerial.dwg",                               true},
    {"visualization_-_condominium_with_skylight.dwg",            true},
    {"visualization_-_conference_room.dwg",                      true},
    {"visualization_-_sun_and_sky_demo.dwg",                     true},
};

} // namespace

TEST_CASE("DWG smoke test: read ~/doc/dwg/*.dwg and report entity counts") {
    const char* home = getenv("HOME");
    if (!home) {
        SKIP("HOME not set — cannot locate ~/doc/dwg/");
    }
    const std::string dir = std::string(home) + "/doc/dwg/";

    // Print a header once
    std::cout << "\n";
    std::cout << std::left
              << std::setw(56) << "File"
              << std::setw(16) << "Version"
              << std::setw(24) << "Error"
              << std::setw(10) << "Entities"
              << std::setw(8)  << "Blocks"
              << "Layers\n";
    std::cout << std::string(120, '-') << "\n";

    bool anyFailed = false;

    for (const auto& fi : kTestFiles) {
        const std::string path = dir + fi.name;
        const DwgResult r = readDwg(path);

        std::cout << std::left
                  << std::setw(56) << fi.name
                  << std::setw(16) << versionStr(r.version)
                  << std::setw(24) << errorStr(r.error)
                  << std::setw(10) << r.entities
                  << std::setw(8)  << r.blocks
                  << r.layers << "\n";

        if (fi.expectSuccess && r.error != DRW::BAD_NONE) {
            anyFailed = true;
            FAIL_CHECK("  ^^^ " << fi.name << " expected OK but got " << errorStr(r.error));
        }
    }

    std::cout << std::string(120, '-') << "\n";
    if (!anyFailed)
        std::cout << "All expected-success files opened without error.\n";
}

// Verbose re-run of the two AC1021 files that fail in the main sweep.
// Run this test case individually to see the full libdxfrw debug trace:
//   ./librecad_tests "*DWG verbose*"
TEST_CASE("DWG verbose debug: failing AC1021 files", "[.dwg_verbose]") {
    const char* home = getenv("HOME");
    if (!home) SKIP("HOME not set");
    const std::string dir = std::string(home) + "/doc/dwg/";

    const char* failingFiles[] = {
        "colorwh.dwg",
        "dwgreader21_230.dwg",
    };

    for (const auto* name : failingFiles) {
        std::cout << "\n=== " << name << " ===\n";
        const DwgResult r = readDwg(dir + name, /*verbose=*/true);
        std::cout << "Result: " << errorStr(r.error)
                  << "  entities=" << r.entities
                  << "  blocks=" << r.blocks
                  << "  layers=" << r.layers << "\n";
    }
}
