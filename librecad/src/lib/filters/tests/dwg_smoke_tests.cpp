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

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
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
    std::map<int, int> entityLWeights;  // lWeight enum -> count
    std::map<std::string, int> layerLWeights; // layer name -> lWeight enum

    void track(const DRW_Entity& e) {
        ++entities;
        entityLWeights[static_cast<int>(e.lWeight)]++;
    }

    // tables
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer& l) override {
        ++layers;
        layerLWeights[l.name] = static_cast<int>(l.lWeight);
    }
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}

    // blocks
    void addBlock(const DRW_Block&) override { ++blocks; }
    void setBlock(const int) override {}
    void endBlock() override {}

    // entities — all increment the counter
    void addPoint(const DRW_Point& e) override { track(e); }
    void addLine(const DRW_Line& e) override { track(e); }
    void addRay(const DRW_Ray& e) override { track(e); }
    void addXline(const DRW_Xline& e) override { track(e); }
    void addArc(const DRW_Arc& e) override { track(e); }
    void addCircle(const DRW_Circle& e) override { track(e); }
    void addEllipse(const DRW_Ellipse& e) override { track(e); }
    void addLWPolyline(const DRW_LWPolyline& e) override { track(e); }
    void addPolyline(const DRW_Polyline& e) override { track(e); }
    void addSpline(const DRW_Spline* e) override { track(*e); }
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert& e) override { track(e); }
    void addTrace(const DRW_Trace& e) override { track(e); }
    void add3dFace(const DRW_3Dface& e) override { track(e); }
    void addSolid(const DRW_Solid& e) override { track(e); }
    void addMText(const DRW_MText& e) override { track(e); }
    void addText(const DRW_Text& e) override { track(e); }
    void addDimAlign(const DRW_DimAligned* e) override { track(*e); }
    void addDimLinear(const DRW_DimLinear* e) override { track(*e); }
    void addDimRadial(const DRW_DimRadial* e) override { track(*e); }
    void addDimDiametric(const DRW_DimDiametric* e) override { track(*e); }
    void addDimAngular(const DRW_DimAngular* e) override { track(*e); }
    void addDimAngular3P(const DRW_DimAngular3p* e) override { track(*e); }
    void addDimOrdinate(const DRW_DimOrdinate* e) override { track(*e); }
    void addLeader(const DRW_Leader* e) override { track(*e); }
    void addHatch(const DRW_Hatch* e) override { track(*e); }
    void addViewport(const DRW_Viewport& e) override { track(e); }
    void addImage(const DRW_Image* e) override { track(*e); }
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

DwgResult readDwg(const std::string& path, bool verbose = false,
                  CountingIface* outIface = nullptr) {
    CountingIface localIface;
    CountingIface& iface = outIface ? *outIface : localIface;
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

// DRW_LW_Conv::lineWidth enum -> human-readable mm
const char* lWeightToMm(int lw) {
    switch (lw) {
        case 0: return "0.00mm";   case 1:  return "0.05mm";  case 2:  return "0.09mm";
        case 3: return "0.13mm";   case 4:  return "0.15mm";  case 5:  return "0.18mm";
        case 6: return "0.20mm";   case 7:  return "0.25mm";  case 8:  return "0.30mm";
        case 9: return "0.35mm";   case 10: return "0.40mm";  case 11: return "0.50mm";
        case 12: return "0.53mm";  case 13: return "0.60mm";  case 14: return "0.70mm";
        case 15: return "0.80mm";  case 16: return "0.90mm";  case 17: return "1.00mm";
        case 18: return "1.06mm";  case 19: return "1.20mm";  case 20: return "1.40mm";
        case 21: return "1.58mm";  case 22: return "2.00mm";  case 23: return "2.11mm";
        case 29: return "ByLayer"; case 30: return "ByBlock"; case 31: return "Default";
        default: return "?";
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
    {"dwgreader21_230.dwg",                                      false}, // 522-byte truncated test file
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
    {"mechanical_example-imperial.dwg",                          true},
    {"#mechanical_example-imperial.dwg",                         false}, // bad magic bytes
    {"#title_block-iso.dwg",                                     false}, // bad magic bytes
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
    // The test corpus lives in a developer-local directory (~/doc/dwg/) and
    // is not shipped with the repo. If it isn't present, skip cleanly so the
    // test passes for everyone else.
    const char* home = getenv("HOME");
    if (!home) {
        SUCCEED("HOME not set; skipping DWG corpus tests");
        return;
    }
    const std::string dir = std::string(home) + "/doc/dwg/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("DWG corpus directory not found at " << dir << "; skipping");
        return;
    }

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
    int filesChecked = 0;

    for (const auto& fi : kTestFiles) {
        const std::string path = dir + fi.name;
        if (!std::filesystem::is_regular_file(path)) {
            std::cout << std::left << std::setw(56) << fi.name
                      << "(missing - skipped)\n";
            continue;
        }
        ++filesChecked;
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
    if (filesChecked == 0)
        std::cout << "No corpus files present; nothing to verify.\n";
    else if (!anyFailed)
        std::cout << "All " << filesChecked << " present file(s) opened as expected.\n";
}

TEST_CASE("DWG lineweights: distribution in lineweights.dwg", "[.dwg_lineweights]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home) + "/doc/dwg/lineweights.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("lineweights.dwg not present; skipping"); return;
    }

    CountingIface iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.error == DRW::BAD_NONE);

    std::cout << "\n=== lineweights.dwg lWeight distribution ===\n";
    std::cout << "Total entities: " << iface.entities << "\n\n";

    std::cout << "Per-entity lWeights:\n";
    for (const auto& [lw, count] : iface.entityLWeights) {
        std::cout << "  enum=" << std::setw(3) << lw
                  << "  width=" << std::setw(8) << lWeightToMm(lw)
                  << "  count=" << count << "\n";
    }

    std::cout << "\nPer-layer lWeights:\n";
    for (const auto& [name, lw] : iface.layerLWeights) {
        std::cout << "  layer=" << std::setw(15) << name
                  << "  enum=" << std::setw(3) << lw
                  << "  width=" << lWeightToMm(lw) << "\n";
    }

    int byLayerCount = iface.entityLWeights.count(29) ? iface.entityLWeights[29] : 0;
    int byBlockCount = iface.entityLWeights.count(30) ? iface.entityLWeights[30] : 0;
    int defaultCount = iface.entityLWeights.count(31) ? iface.entityLWeights[31] : 0;
    std::cout << "\nSummary: entities resolving via layer=" << byLayerCount
              << ", block=" << byBlockCount << ", default=" << defaultCount
              << ", explicit=" << (iface.entities - byLayerCount - byBlockCount - defaultCount)
              << "\n";
}

// Verbose re-run of the two AC1021 files that fail in the main sweep.
// Run this test case individually to see the full libdxfrw debug trace:
//   ./librecad_tests "*DWG verbose*"
TEST_CASE("DWG verbose debug: failing AC1021 files", "[.dwg_verbose]") {
    const char* home = getenv("HOME");
    if (!home) {
        SUCCEED("HOME not set; skipping verbose DWG corpus tests");
        return;
    }
    const std::string dir = std::string(home) + "/doc/dwg/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("DWG corpus directory not found at " << dir << "; skipping");
        return;
    }

    const char* failingFiles[] = {
        "colorwh.dwg",
        "dwgreader21_230.dwg",
    };

    for (const auto* name : failingFiles) {
        const std::string path = dir + name;
        if (!std::filesystem::is_regular_file(path)) {
            std::cout << "\n=== " << name << " (missing - skipped) ===\n";
            continue;
        }
        std::cout << "\n=== " << name << " ===\n";
        const DwgResult r = readDwg(path, /*verbose=*/true);
        std::cout << "Result: " << errorStr(r.error)
                  << "  entities=" << r.entities
                  << "  blocks=" << r.blocks
                  << "  layers=" << r.layers << "\n";
    }
}
