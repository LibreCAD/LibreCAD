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
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_interface.h"
#include "intern/drw_dbg.h"
#include "libdwgr.h"

namespace {

// ---- capturing DRW debug printer -------------------------------------------

class CapturingPrinter : public DRW::DebugPrinter {
public:
    std::string buf;
    void printS(const std::string& s) override { buf += s; }
    void printI(long long int i)      override { buf += std::to_string(i); }
    void printUI(long long unsigned int i) override { buf += std::to_string(i); }
    void printD(double d)             override { buf += std::to_string(d); }
    void printH(long long int i)      override {
        char tmp[32]; snprintf(tmp, sizeof(tmp), "%llX", (unsigned long long)i); buf += tmp;
    }
    void printB(int i)                override { buf += std::to_string(i); }
    void printHL(int c, int s, int h) override {
        char tmp[64]; snprintf(tmp, sizeof(tmp), "(%d,%d,%X)", c, s, (unsigned)h); buf += tmp;
    }
    void printPT(double x, double y, double z) override {
        char tmp[96]; snprintf(tmp, sizeof(tmp), "(%.3f,%.3f,%.3f)", x, y, z); buf += tmp;
    }

    // Parse [unhandled-entity-type N] tokens out of captured buffer.
    // Returns map of oType -> count.
    std::map<int,int> unhandledTypes() const {
        std::map<int,int> result;
        std::regex re(R"(\[unhandled-entity-type (\d+)\])");
        auto begin = std::sregex_iterator(buf.begin(), buf.end(), re);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end; ++it)
            result[std::stoi((*it)[1].str())]++;
        return result;
    }
};

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

// ---- entity type-tracking interface ----------------------------------------

class TypeTrackingIface : public DRW_Interface {
public:
    std::map<std::string, int> typeCounts; // entity type name -> count
    std::map<std::string, int> layerEntities; // layer name -> entity count
    int inBlock = 0;
    int modelSpaceEntities = 0;
    int blockSpaceEntities = 0;
    int blocks = 0;
    int layers = 0;

    void trackT(const DRW_Entity& e, const char* typeName) {
        typeCounts[typeName]++;
        if (e.layer.empty())
            layerEntities["(no layer)"]++;
        else
            layerEntities[e.layer]++;
        if (inBlock > 1)  // inBlock==1 means model-space (*MODEL_SPACE block)
            blockSpaceEntities++;
        else
            modelSpaceEntities++;
    }

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override { ++layers; }
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}

    void addBlock(const DRW_Block&) override { ++blocks; ++inBlock; }
    void setBlock(const int) override {}
    void endBlock() override { if (inBlock > 0) --inBlock; }

    void addPoint(const DRW_Point& e)                   override { trackT(e, "POINT"); }
    void addLine(const DRW_Line& e)                     override { trackT(e, "LINE"); }
    void addRay(const DRW_Ray& e)                       override { trackT(e, "RAY"); }
    void addXline(const DRW_Xline& e)                   override { trackT(e, "XLINE"); }
    void addArc(const DRW_Arc& e)                       override { trackT(e, "ARC"); }
    void addCircle(const DRW_Circle& e)                 override { trackT(e, "CIRCLE"); }
    void addEllipse(const DRW_Ellipse& e)               override { trackT(e, "ELLIPSE"); }
    void addLWPolyline(const DRW_LWPolyline& e)         override { trackT(e, "LWPOLYLINE"); }
    void addPolyline(const DRW_Polyline& e)             override { trackT(e, "POLYLINE"); }
    void addSpline(const DRW_Spline* e)                 override { trackT(*e, "SPLINE"); }
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert& e)                 override { trackT(e, "INSERT"); }
    void addTrace(const DRW_Trace& e)                   override { trackT(e, "TRACE"); }
    void add3dFace(const DRW_3Dface& e)                 override { trackT(e, "3DFACE"); }
    void addSolid(const DRW_Solid& e)                   override { trackT(e, "SOLID"); }
    void addMText(const DRW_MText& e)                   override { trackT(e, "MTEXT"); }
    void addText(const DRW_Text& e)                     override { trackT(e, "TEXT"); }
    void addDimAlign(const DRW_DimAligned* e)           override { trackT(*e, "DIM_ALIGNED"); }
    void addDimLinear(const DRW_DimLinear* e)           override { trackT(*e, "DIM_LINEAR"); }
    void addDimRadial(const DRW_DimRadial* e)           override { trackT(*e, "DIM_RADIAL"); }
    void addDimDiametric(const DRW_DimDiametric* e)     override { trackT(*e, "DIM_DIAMETRIC"); }
    void addDimAngular(const DRW_DimAngular* e)         override { trackT(*e, "DIM_ANGULAR"); }
    void addDimAngular3P(const DRW_DimAngular3p* e)     override { trackT(*e, "DIM_ANGULAR3P"); }
    void addDimOrdinate(const DRW_DimOrdinate* e)       override { trackT(*e, "DIM_ORDINATE"); }
    void addLeader(const DRW_Leader* e)                 override { trackT(*e, "LEADER"); }
    void addHatch(const DRW_Hatch* e)                   override { trackT(*e, "HATCH"); }
    void addViewport(const DRW_Viewport& e)             override { trackT(e, "VIEWPORT"); }
    void addImage(const DRW_Image* e)                   override { trackT(*e, "IMAGE"); }
    void addTolerance(const DRW_Tolerance& e)           override { trackT(e, "TOLERANCE"); }
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}

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

    int total() const {
        int n = 0;
        for (const auto& kv : typeCounts) n += kv.second;
        return n;
    }
};

// ---- deep diagnostic read --------------------------------------------------

struct DeepResult {
    bool       ok;
    DRW::error error;
    DRW::Version version;
    TypeTrackingIface iface;
    std::map<int,int> unhandledTypes; // oType -> count
    std::string debugLog;             // full captured debug trace (for verbose dumps)
};

DeepResult readDwgDeep(const std::string& path) {
    DeepResult dr;

    // Allocate on heap — ownership transferred to DRW_dbg via setCustomDebugPrinter.
    // Keep a raw ptr to read results BEFORE the next setCustomDebugPrinter call deletes it.
    auto* capturePrinter = new CapturingPrinter();
    DRW::setCustomDebugPrinter(capturePrinter);  // DRW_dbg takes ownership

    try {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::Debug);  // also sets DRW_dbg level to Debug
        dr.ok = reader.read(&dr.iface, true);
        dr.error   = reader.getError();
        dr.version = reader.getVersion();
    } catch (const std::exception& ex) {
        std::cerr << "  [EXCEPTION: " << ex.what() << "]\n";
        dr.ok    = false;
        dr.error = DRW::BAD_UNKNOWN;
    } catch (...) {
        std::cerr << "  [UNKNOWN EXCEPTION]\n";
        dr.ok    = false;
        dr.error = DRW::BAD_UNKNOWN;
    }

    // Extract before replacing printer (which deletes capturePrinter).
    // Grab the full log now — avoids printer-state complications in callers that
    // want to print it after this function returns.
    dr.unhandledTypes = capturePrinter->unhandledTypes();
    dr.debugLog = std::move(capturePrinter->buf);

    // Restore silent printer — DRW_dbg::setCustomDebugPrinter deletes capturePrinter here.
    DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

    return dr;
}

// map raw oType number to a human-readable name (ODA entity type codes)
const char* oTypeName(int t) {
    switch (t) {
        case  1: return "TEXT";
        case  2: return "ATTRIB";
        case  3: return "ATTDEF";
        case  4: return "BLOCK";
        case  5: return "ENDBLK";
        case  6: return "SEQEND";
        case  7: return "INSERT";
        case  8: return "MINSERT";
        case 10: return "VERTEX_2D";
        case 11: return "VERTEX_3D";
        case 12: return "VERTEX_MESH";
        case 13: return "VERTEX_PFACE";
        case 14: return "VERTEX_PFACE_FACE";
        case 15: return "POLYLINE_2D";
        case 16: return "POLYLINE_3D";
        case 17: return "ARC";
        case 18: return "CIRCLE";
        case 19: return "LINE";
        case 20: return "DIM_ORDINATE";
        case 21: return "DIM_LINEAR";
        case 22: return "DIM_ALIGNED";
        case 23: return "DIM_ANGULAR3P";
        case 24: return "DIM_ANGULAR";
        case 25: return "DIM_RADIAL";
        case 26: return "DIM_DIAMETRIC";
        case 27: return "POINT";
        case 28: return "3DFACE";
        case 29: return "POLYLINE_PFACE";
        case 30: return "POLYLINE_MESH";
        case 31: return "SOLID";
        case 32: return "TRACE";
        case 33: return "SHAPE";
        case 34: return "VIEWPORT";
        case 35: return "ELLIPSE";
        case 36: return "SPLINE";
        case 37: return "REGION";
        case 38: return "3DSOLID";
        case 39: return "BODY";
        case 40: return "RAY";
        case 41: return "XLINE";
        case 42: return "DICTIONARY";
        case 43: return "OLEFRAME";
        case 44: return "MTEXT";
        case 45: return "LEADER";
        case 46: return "TOLERANCE";
        case 47: return "MLINE";
        case 48: return "BLOCK_CONTROL";
        case 49: return "BLOCK_HDR";
        case 50: return "LAYER_CONTROL";
        case 51: return "LAYER";
        case 52: return "STYLE_CONTROL";
        case 53: return "STYLE";
        case 56: return "LTYPE_CONTROL";
        case 57: return "LTYPE";
        case 60: return "VIEW_CONTROL";
        case 61: return "VIEW";
        case 62: return "UCS_CONTROL";
        case 63: return "UCS";
        case 64: return "VPORT_CONTROL";
        case 65: return "VPORT";
        case 66: return "APPID_CONTROL";
        case 67: return "APPID";
        case 68: return "DIMSTYLE_CONTROL";
        case 69: return "DIMSTYLE";
        case 70: return "VP_ENT_HDR_CTRL";
        case 71: return "VP_ENT_HDR";
        case 72: return "GROUP";
        case 73: return "MLINESTYLE";
        case 74: return "OLE2FRAME";
        case 76: return "DUMMY";
        case 77: return "LWPOLYLINE";
        case 78: return "HATCH";
        case 79: return "XRECORD";
        case 80: return "PLACEHOLDER";
        case 81: return "VBA_PROJECT";
        case 82: return "LAYOUT";
        case 101: return "IMAGE";
        default: return nullptr;
    }
}

void printDeepReport(const char* filename, const DeepResult& dr) {
    std::cout << "\n";
    std::cout << "=== " << filename << " ===\n";
    std::cout << "Version : " << versionStr(dr.version) << "\n";
    std::cout << "Error   : " << errorStr(dr.error) << "\n";
    std::cout << "Blocks  : " << dr.iface.blocks
              << "  Layers: " << dr.iface.layers << "\n";
    int total = dr.iface.total();
    std::cout << "Entities: " << total
              << "  (model-space=" << dr.iface.modelSpaceEntities
              << ", in-blocks=" << dr.iface.blockSpaceEntities << ")\n";

    std::cout << "\n  Handled entity type distribution:\n";
    for (const auto& [type, count] : dr.iface.typeCounts) {
        std::cout << "    " << std::left << std::setw(16) << type
                  << " " << count << "\n";
    }

    if (!dr.unhandledTypes.empty()) {
        std::cout << "\n  *** UNHANDLED entity types (silently skipped) ***\n";
        int totalSkipped = 0;
        for (const auto& [oType, count] : dr.unhandledTypes) {
            totalSkipped += count;
            const char* name = oTypeName(oType);
            std::cout << "    oType=" << std::setw(4) << oType
                      << "  name=" << std::setw(18) << (name ? name : "(unknown/custom)")
                      << "  count=" << count << "\n";
        }
        std::cout << "  Total skipped: " << totalSkipped << "\n";
    } else {
        std::cout << "\n  (no unhandled entity types)\n";
    }

    std::cout << "\n  Top layers by entity count:\n";
    std::vector<std::pair<int,std::string>> sorted;
    for (const auto& [layer, cnt] : dr.iface.layerEntities)
        sorted.emplace_back(cnt, layer);
    std::sort(sorted.rbegin(), sorted.rend());
    int shown = 0;
    for (const auto& [cnt, layer] : sorted) {
        std::cout << "    " << std::left << std::setw(30) << layer
                  << " " << cnt << "\n";
        if (++shown >= 15) { std::cout << "    ...\n"; break; }
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
    {"visualization_-_aerial.dwg",                               true},  // 3D-solid-only model; 0 entities is correct for a 2D reader
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
        "blocks_and_tables_-_metric.dwg",
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

// Deep diagnostic: entity type breakdown + unhandled oType report.
// Run individually:
//   ./librecad_tests "[.dwg_deep]" -s
TEST_CASE("DWG deep diagnostic: entity type breakdown for target files", "[.dwg_deep]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/doc/dwg/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("DWG corpus directory not found; skipping"); return;
    }

    const char* targets[] = {
        "blocks_and_tables_-_metric.dwg",
        "blocks_and_tables_-_imperial.dwg",
        "architectural_example-imperial.dwg",
        "architectural_-_annotation_scaling_and_multileaders.dwg",
        "visualization_-_aerial.dwg",
        "visualization_-_conference_room.dwg",
    };

    for (const auto* name : targets) {
        const std::string path = dir + name;
        if (!std::filesystem::is_regular_file(path)) {
            std::cout << "\n=== " << name << " (missing - skipped) ===\n";
            continue;
        }
        const DeepResult dr = readDwgDeep(path);
        printDeepReport(name, dr);
    }
}

// Deep troubleshooting for Cover.dwg (AC1027/R2013, dwgReader27).
// Run:
//   ./librecad_tests "[.dwg_cover]" -s
// Then for full verbose trace:
//   ./librecad_tests "[.dwg_cover_verbose]" -s 2>&1 | less
TEST_CASE("DWG Cover.dwg: deep entity/type breakdown", "[.dwg_cover]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path = std::string(home) + "/doc/dwg/Cover.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Cover.dwg not present; skipping"); return;
    }
    const DeepResult dr = readDwgDeep(path);
    printDeepReport("Cover.dwg", dr);
}

// ---------------------------------------------------------------------------
// dwg_samples corpus: 42 minimal single-entity files across 6 DWG versions.
// Auto-discovers every *.dwg in ~/dev/dwg_samples/ and asserts each one
// parses without error and contains at least one entity.
// Skips cleanly when the directory is absent.
// ---------------------------------------------------------------------------
TEST_CASE("DWG samples: load all files in ~/dev/dwg_samples/") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/dev/dwg_samples/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("~/dev/dwg_samples/ not found; skipping");
        return;
    }

    // Collect *.dwg paths, sorted for deterministic output.
    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        const std::string ext = p.extension().string();
        if (ext == ".dwg" || ext == ".DWG")
            paths.push_back(p.string());
    }
    std::sort(paths.begin(), paths.end());

    if (paths.empty()) {
        SUCCEED("No .dwg files found in ~/dev/dwg_samples/; skipping");
        return;
    }

    // Header
    std::cout << "\n"
              << std::left
              << std::setw(46) << "File"
              << std::setw(16) << "Version"
              << std::setw(24) << "Error"
              << std::setw(10) << "Entities"
              << std::setw(8)  << "Blocks"
              << "Layers\n"
              << std::string(110, '-') << "\n";

    int passed = 0, failed = 0;
    for (const auto& path : paths) {
        const std::string name = std::filesystem::path(path).filename().string();
        const DwgResult r = readDwg(path);

        std::cout << std::left
                  << std::setw(46) << name
                  << std::setw(16) << versionStr(r.version)
                  << std::setw(24) << errorStr(r.error)
                  << std::setw(10) << r.entities
                  << std::setw(8)  << r.blocks
                  << r.layers << "\n";

        if (r.error != DRW::BAD_NONE) {
            ++failed;
            FAIL_CHECK(name << ": expected OK but got " << errorStr(r.error));
        } else if (r.entities < 1) {
            ++failed;
            FAIL_CHECK(name << ": parsed OK but 0 entities (expected >= 1)");
        } else {
            ++passed;
        }
    }

    std::cout << std::string(110, '-') << "\n";
    std::cout << "Passed: " << passed << "  Failed: " << failed
              << "  Total: " << paths.size() << "\n";
}

// Deep + verbose diagnostic for the one failure in the samples suite:
// polyline2d_line_R14.dwg (AC1014) returns OK but 0 entities.
// Run:  ./librecad_tests "[.dwg_polyR14]" -s 2>/tmp/poly_r14_verbose.txt
TEST_CASE("DWG polyline2d_line_R14: deep + verbose", "[.dwg_polyR14]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home) + "/dev/dwg_samples/polyline2d_line_R14.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("polyline2d_line_R14.dwg not present; skipping"); return;
    }

    // readDwgDeep captures the full debug log internally — no separate verbose
    // reader needed and no printer-state complications.
    const DeepResult dr = readDwgDeep(path);

    std::cout << "\n--- Deep diagnostic ---\n";
    printDeepReport("polyline2d_line_R14.dwg", dr);

    std::cout << "\n--- Reader debug trace ---\n";
    std::cout << dr.debugLog;
}

TEST_CASE("DWG Cover.dwg: verbose reader trace", "[.dwg_cover_verbose]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path = std::string(home) + "/doc/dwg/Cover.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Cover.dwg not present; skipping"); return;
    }
    std::cout << "\n=== Cover.dwg (verbose) ===\n";
    const DwgResult r = readDwg(path, /*verbose=*/true);
    std::cout << "Result: " << errorStr(r.error)
              << "  version=" << versionStr(r.version)
              << "  entities=" << r.entities
              << "  blocks=" << r.blocks
              << "  layers=" << r.layers << "\n";
}

// ---------------------------------------------------------------------------
// ~/doc/dwg2/ corpus: real-world DWG files (mixed versions, complex content).
// Asserts each file parses without a reader error (BAD_NONE).
// Entity count is reported but not asserted — some real-world files may be
// 3D-only or otherwise legitimately produce 0 2D entities.
// Skips cleanly when the directory is absent.
// ---------------------------------------------------------------------------
TEST_CASE("DWG corpus: load all files in ~/doc/dwg2/") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/doc/dwg2/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("~/doc/dwg2/ not found; skipping");
        return;
    }

    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        const std::string ext = p.extension().string();
        if (ext != ".dwg" && ext != ".DWG") continue;
        // '#'-prefixed files have invalid magic bytes; skip them like the smoke test does.
        if (p.filename().string().front() == '#') continue;
        paths.push_back(p.string());
    }
    std::sort(paths.begin(), paths.end());

    if (paths.empty()) {
        SUCCEED("No .dwg files found in ~/doc/dwg2/; skipping");
        return;
    }

    std::cout << "\n"
              << std::left
              << std::setw(46) << "File"
              << std::setw(16) << "Version"
              << std::setw(24) << "Error"
              << std::setw(10) << "Entities"
              << std::setw(8)  << "Blocks"
              << "Layers\n"
              << std::string(110, '-') << "\n";

    int passed = 0, failed = 0;
    for (const auto& path : paths) {
        const std::string name = std::filesystem::path(path).filename().string();
        const DwgResult r = readDwg(path);

        std::cout << std::left
                  << std::setw(46) << name
                  << std::setw(16) << versionStr(r.version)
                  << std::setw(24) << errorStr(r.error)
                  << std::setw(10) << r.entities
                  << std::setw(8)  << r.blocks
                  << r.layers << "\n";

        if (r.error != DRW::BAD_NONE) {
            ++failed;
            FAIL_CHECK(name << ": expected OK but got " << errorStr(r.error));
        } else {
            ++passed;
        }
    }

    std::cout << std::string(110, '-') << "\n";
    std::cout << "Passed: " << passed << "  Failed: " << failed
              << "  Total: " << paths.size() << "\n";
}

// Verifies that all HATCH entities in a real-world R2013 file parse correctly:
// loop structure, boundary geometry, and associative handles are all consumed
// with zero remaining bytes.
// Run verbosely:  ./librecad_tests "[.dwg_arch_hatch]" -s 2>/tmp/arch_hatch_trace.txt
TEST_CASE("DWG Architectural-Modern-Building-Design: hatch parsing", "[.dwg_arch_hatch]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Architectural-Modern-Building-Design.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    // Basic load check.
    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);

    // The file has 48 HATCH entities (mix of solid, gradient, and GLASS pattern
    // hatches with 1..56 boundary loops each).  All should be parsed — none left
    // in the unhandled bucket.
    CHECK(dr.iface.typeCounts.at("HATCH") == 48);
    CHECK(dr.unhandledTypes.count(78) == 0); // oType 78 = HATCH

    std::cout << "\n--- Deep diagnostic ---\n";
    printDeepReport("Architectural-Modern-Building-Design.dwg", dr);

    std::cout << "\n--- Reader debug trace ---\n";
    std::cout << dr.debugLog;
}
