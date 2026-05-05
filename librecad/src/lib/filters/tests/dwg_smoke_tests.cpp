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
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_interface.h"
#include "intern/drw_dbg.h"
#include "libdwgr.h"

// LibreCAD entity headers for end-to-end DRW→RS_Hatch pipeline tests.
#include "lc_containertraverser.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"

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
    void addWipeout(const DRW_Image* e) override {
        track(*e);
        wipeouts++;
        wipeoutVertices += static_cast<int>(e->clipPath.size());
    }
    void addMLeader(const DRW_MLeader* e) override {
        track(*e);
        mleaders++;
        mleaderRoots += static_cast<int>(e->context.roots.size());
        for (const auto& r : e->context.roots) {
            mleaderLines += static_cast<int>(r.leaderLines.size());
            for (const auto& ll : r.leaderLines) {
                mleaderPoints += static_cast<int>(ll.points.size());
            }
        }
    }
    void addMLeaderStyle(const DRW_MLeaderStyle*) override { ++mleaderStyles; }
    int wipeouts = 0;
    int wipeoutVertices = 0;
    int mleaders = 0;
    int mleaderRoots = 0;
    int mleaderLines = 0;
    int mleaderPoints = 0;
    int mleaderStyles = 0;
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
    size_t wipeoutVertexCount = 0;

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
    void addInsert(const DRW_Insert& e)                 override {
        trackT(e, "INSERT");
        // Attached attribute entities (DWG attlist model) — surface them
        // alongside the INSERT count so tests can assert ATTRIB delivery.
        for (const auto& a : e.attlist) {
            if (!a) continue;
            typeCounts[a->eType == DRW::ATTDEF ? "ATTDEF" : "ATTRIB"]++;
        }
    }
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
    void addMLeader(const DRW_MLeader* e)               override { trackT(*e, "MLEADER"); }
    void addWipeout(const DRW_Image* e)                 override {
        trackT(*e, "WIPEOUT");
        // Surface the polygon size so tests can sanity-check that the boundary
        // actually came through — empty clipPath is the historical bug shape.
        wipeoutVertexCount += e->clipPath.size();
    }
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

// ---- DRW_Hatch → RS_Hatch pipeline (mirrors RS_FilterDXFRW::addHatch) ----------

// Builds an RS_Hatch with boundary loops populated from DRW_Hatch data.
// Replicates the boundary-construction logic of RS_FilterDXFRW::addHatch without
// needing a live document context.  Does NOT call update(); caller owns the result.
RS_Hatch* buildRS_Hatch(const DRW_Hatch* data) {
    auto* hatch = new RS_Hatch(nullptr,
        RS_HatchData(data->solid != 0, data->scale, data->angle,
                     QString::fromUtf8(data->name.c_str())));

    for (const auto& loop : data->looplist) {
        if (loop->type & 32) continue;  // skip textbox-boundary loops

        auto* hatchLoop = new RS_EntityContainer(hatch);
        hatchLoop->setLayer(nullptr);
        hatch->addEntity(hatchLoop);

        if (loop->type & 2) {
            // Polyline boundary: convert to line/arc segments via RS_Polyline.
            if (loop->objlist.empty()) continue;
            auto* pline = static_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
            RS_Polyline poly{nullptr,
                             RS_PolylineData(RS_Vector(false), RS_Vector(false), pline->flags)};
            for (const auto& v : pline->vertlist)
                poly.addVertex(RS_Vector{v->x, v->y}, v->bulge);
            for (RS_Entity* e : lc::LC_ContainerTraverser{poly, RS2::ResolveNone}.entities()) {
                RS_Entity* tmp = e->clone();
                tmp->reparent(hatchLoop);
                tmp->setLayer(nullptr);
                hatchLoop->addEntity(tmp);
            }
        } else {
            // Explicit-segment boundary: line, arc, ellipse (spline skipped same as filter).
            for (const auto& seg : loop->objlist) {
                RS_Entity* e = nullptr;
                switch (seg->eType) {
                case DRW::LINE: {
                    auto* l = static_cast<DRW_Line*>(seg.get());
                    e = new RS_Line(hatchLoop,
                                    {{l->basePoint.x, l->basePoint.y},
                                     {l->secPoint.x,  l->secPoint.y}});
                    break;
                }
                case DRW::ARC: {
                    auto* a = static_cast<DRW_Arc*>(seg.get());
                    RS_Vector ctr{a->basePoint.x, a->basePoint.y};
                    if (a->isccw && a->staangle < 1e-6
                                 && a->endangle > RS_Math::deg2rad(360) - 1e-6) {
                        e = new RS_Circle(hatchLoop, {ctr, a->radious});
                    } else if (a->isccw) {
                        e = new RS_Arc(hatchLoop,
                            RS_ArcData(ctr, a->radious,
                                       RS_Math::correctAngle(a->staangle),
                                       RS_Math::correctAngle(a->endangle), false));
                    } else {
                        e = new RS_Arc(hatchLoop,
                            RS_ArcData(ctr, a->radious,
                                       RS_Math::correctAngle(2*M_PI - a->staangle),
                                       RS_Math::correctAngle(2*M_PI - a->endangle), true));
                    }
                    break;
                }
                case DRW::ELLIPSE: {
                    auto* el = static_cast<DRW_Ellipse*>(seg.get());
                    double a1 = el->staparam, a2 = el->endparam;
                    if (std::abs(a2 - 2.*M_PI) < 1e-10 && std::abs(a1) < 1e-10) {
                        a2 = 0.;
                    } else {
                        // Mirror of RS_FilterDXFRW::addHatch ellipse angle conversion.
                        a1 = std::atan(std::tan(a1) / el->ratio);
                        a2 = std::atan(std::tan(a2) / el->ratio);
                        if (a1 < 0) { a1 += M_PI; if (el->staparam > M_PI) a1 += M_PI; }
                        else if (el->staparam > M_PI) a1 += M_PI;
                        if (a2 < 0) { a2 += M_PI; if (el->endparam > M_PI) a2 += M_PI; }
                        else if (el->endparam > M_PI) a2 += M_PI;
                    }
                    e = new RS_Ellipse(hatchLoop,
                        {{el->basePoint.x, el->basePoint.y},
                         {el->secPoint.x,  el->secPoint.y},
                         el->ratio, a1, a2, !el->isccw});
                    break;
                }
                default:
                    break;
                }
                if (e) { e->setLayer(nullptr); hatchLoop->addEntity(e); }
            }
        }
    }
    return hatch;
}

struct HatchFillResult {
    std::string pattern;
    bool solid;
    int declaredLoops;    // loopsnum from DWG
    int rootLoops;        // countLoops() — root loops after hierarchy assignment
    int allLoops;         // countAllLoops() — roots + all nested children
    RS_Hatch::RS_HatchError error;
    double area;
};

// DRW_Interface that builds + validates/updates an RS_Hatch for every DRW_Hatch received.
// For solid hatches the full update() pipeline runs (validate + fill + area).
// For pattern hatches only validate() is called — updatePatternHatch() requires the
// LibreCAD pattern library singleton which is not initialised in the headless test binary.
class HatchFillIface : public TypeTrackingIface {
public:
    std::vector<HatchFillResult> hatches;

    void addHatch(const DRW_Hatch* e) override {
        TypeTrackingIface::addHatch(e);
        std::unique_ptr<RS_Hatch> hatch{buildRS_Hatch(e)};

        RS_Hatch::RS_HatchError error;
        int rootLoops, allLoops;
        double area = 0.0;

        if (hatch->isSolid()) {
            hatch->update();
            error     = hatch->getUpdateError();
            rootLoops = hatch->countLoops();
            allLoops  = hatch->countAllLoops();
            area      = hatch->getTotalArea();
        } else {
            // Pattern hatch: validate boundary only; don't attempt pattern rendering.
            bool valid = hatch->validate();
            error      = valid ? RS_Hatch::HATCH_OK : RS_Hatch::HATCH_INVALID_CONTOUR;
            rootLoops  = hatch->countLoops();
            allLoops   = hatch->countAllLoops();
        }

        hatches.push_back({e->name, e->solid != 0, e->loopsnum,
                           rootLoops, allLoops, error, area});
    }
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
// Hidden tag [.dwg3]: one-shot diagnostic load of ~/doc/dwg3/ so failures
// can be enumerated without polluting the default test run. Run with:
//   ./librecad_tests "[.dwg3]" -s
TEST_CASE("DWG corpus: load all files in ~/doc/dwg3/", "[.dwg3]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/doc/dwg3/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("~/doc/dwg3/ not found; skipping");
        return;
    }

    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        const std::string ext = p.extension().string();
        if (ext != ".dwg" && ext != ".DWG") continue;
        if (p.filename().string().front() == '#') continue;
        paths.push_back(p.string());
    }
    std::sort(paths.begin(), paths.end());
    if (paths.empty()) {
        SUCCEED("No .dwg files in ~/doc/dwg3/; skipping");
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
        if (r.error != DRW::BAD_NONE) ++failed; else ++passed;
    }
    std::cout << std::string(110, '-') << "\n";
    std::cout << "Passed: " << passed << "  Failed: " << failed
              << "  Total: " << paths.size() << "\n";
}

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

// Verifies that the DRW→RS_Hatch pipeline produces properly filled hatches for
// every HATCH entity in Architectural-Modern-Building-Design.dwg:
//   • Solid hatches (42 of 48): must fully succeed (HATCH_OK, area > 0).
//   • Pattern hatches (6 GLASS): boundary must be valid (no HATCH_INVALID_CONTOUR);
//     HATCH_PATTERN_NOT_FOUND is accepted because the test env has no pattern library.
// Run:  ./librecad_tests "[.dwg_arch_hatch_fill]" -s
TEST_CASE("DWG Architectural-Modern-Building-Design: hatch fill pipeline", "[.dwg_arch_hatch_fill]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Architectural-Modern-Building-Design.dwg not present; skipping"); return;
    }

    HatchFillIface iface;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::None);
        bool ok = reader.read(&iface, true);
        REQUIRE(ok);
    }

    REQUIRE(iface.hatches.size() == 48);

    int solidFailed = 0;
    int contourErrors = 0;
    int loopLoss = 0;    // hatches where allLoops < declaredLoops (real boundary loss)

    // Columns: pattern | solid | decl | roots | all | err | area
    std::cout << "\n  "
              << std::setw(20) << std::left << "pattern"
              << std::setw(6) << "solid"
              << std::setw(6) << "decl"
              << std::setw(6) << "roots"
              << std::setw(6) << "all"
              << std::setw(6) << "err"
              << "area\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (const auto& r : iface.hatches) {
        std::cout << "  " << std::setw(20) << std::left << r.pattern
                  << std::setw(6) << r.solid
                  << std::setw(6) << r.declaredLoops
                  << std::setw(6) << r.rootLoops
                  << std::setw(6) << r.allLoops
                  << std::setw(6) << r.error
                  << r.area << "\n";

        // No hatch should have a broken boundary topology.
        CHECK(r.error != RS_Hatch::HATCH_INVALID_CONTOUR);
        if (r.error == RS_Hatch::HATCH_INVALID_CONTOUR) ++contourErrors;

        // Every declared loop must be successfully extracted (no under-extraction from
        // cross-contamination at shared corners).  allLoops > declaredLoops is acceptable:
        // a self-intersecting DWG loop can legitimately yield multiple extracted loops.
        CHECK(r.allLoops >= r.declaredLoops);
        if (r.allLoops < r.declaredLoops) ++loopLoss;

        if (r.solid) {
            // Solid hatches must produce a filled result with positive area.
            CHECK(r.error == RS_Hatch::HATCH_OK);
            CHECK(r.rootLoops > 0);
            CHECK(r.area > 0.0);
            if (r.error != RS_Hatch::HATCH_OK) ++solidFailed;
        }
    }

    std::cout << "\n  Solid failed: " << solidFailed
              << "  Contour errors: " << contourErrors
              << "  Loop-loss hatches: " << loopLoss
              << "  Total: " << iface.hatches.size() << "\n";
}

// Verifies that trolley_structure.dwg (AC1024/R2010, ~3218 entities, 79 blocks,
// 13 layers) loads cleanly with every entity type properly dispatched into a
// handler.  No oTypes should land in the unhandled bucket.
// Run:  ./librecad_tests "[.dwg_trolley]" -s
TEST_CASE("DWG trolley_structure: entity population", "[.dwg_trolley]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/trolley_structure.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("trolley_structure.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);
    REQUIRE(dr.version == DRW::AC1024);

    // Inventory of standard graphical-entity oTypes (visible 2D geometry).
    // OBJECTS-section types (DICTIONARY=42, MLINESTYLE=73, XRECORD=79,
    // PLACEHOLDER=80, LAYOUT=82) and custom oTypes >=500 (AutoCAD Mechanical
    // proxy/AM_ classes) are NOT entities and may legitimately appear in the
    // unhandled bucket — they don't carry drawable geometry.
    static const std::set<int> kGraphicalOTypes = {
        1, 7, 8, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101
    };
    int unhandledGraphicalLost = 0;
    for (const auto& [oType, count] : dr.unhandledTypes) {
        if (kGraphicalOTypes.count(oType)) {
            std::cout << "  *** LOST graphical entity oType=" << oType
                      << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                      << " count=" << count << "\n";
            unhandledGraphicalLost += count;
        }
    }
    CHECK(unhandledGraphicalLost == 0);

    // Counts observed at 2026-05-04 from libdxfrw against this file.
    // Drift in any of these flags either reader regression or upstream-file change.
    CHECK(dr.iface.total() == 3218);
    CHECK(dr.iface.blocks == 79);
    CHECK(dr.iface.layers == 13);
    CHECK(dr.iface.typeCounts.at("LINE")       == 1652);
    CHECK(dr.iface.typeCounts.at("ARC")        == 711);
    CHECK(dr.iface.typeCounts.at("POINT")      == 652);
    CHECK(dr.iface.typeCounts.at("CIRCLE")     == 161);
    CHECK(dr.iface.typeCounts.at("LWPOLYLINE") == 41);
    CHECK(dr.iface.typeCounts.at("VIEWPORT")   == 1);

    printDeepReport("trolley_structure.dwg", dr);
}

// Verifies that gear_pump_subassy.dwg (AC1024/R2010) loads cleanly with every
// graphical entity properly dispatched into a typed handler.
// Run:  ./librecad_tests "[.dwg_gear_pump]" -s
TEST_CASE("DWG gear_pump_subassy: entity population", "[.dwg_gear_pump]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("gear_pump_subassy.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);
    REQUIRE(dr.version == DRW::AC1024);

    // No standard graphical entity (oType in valid 2D-graphic set) may be lost
    // to the unhandled bucket. OBJECTS-section types and custom oTypes >=500
    // are non-graphical and are accepted as unhandled.
    static const std::set<int> kGraphicalOTypes = {
        1, 7, 8, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101
    };
    int unhandledGraphicalLost = 0;
    for (const auto& [oType, count] : dr.unhandledTypes) {
        if (kGraphicalOTypes.count(oType)) {
            std::cout << "  *** LOST graphical entity oType=" << oType
                      << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                      << " count=" << count << "\n";
            unhandledGraphicalLost += count;
        }
    }
    CHECK(unhandledGraphicalLost == 0);

    CHECK(dr.iface.total() > 0);
    CHECK(dr.iface.blocks > 0);
    CHECK(dr.iface.layers > 0);

    // ATTRIB / ATTDEF / SEQEND no longer leak into the unhandled-entity bucket
    // since the DWG dispatcher routes them via the deferred-INSERT flush
    // (oType 2 = ATTRIB, 3 = ATTDEF, 6 = SEQEND).
    CHECK(dr.unhandledTypes.count(2) == 0);
    CHECK(dr.unhandledTypes.count(3) == 0);
    CHECK(dr.unhandledTypes.count(6) == 0);

    // Custom-class objects (oType >= 500, AutoCAD Mechanical proxy entities)
    // are skipped via the [custom-class-skipped] token, not the unhandled
    // entity-type token, so no oType >= 500 should appear here.
    int customLeak = 0;
    for (const auto& [oType, count] : dr.unhandledTypes)
        if (oType >= 500) customLeak += count;
    CHECK(customLeak == 0);

    // ATTRIBs ride attached to their owning INSERTs via DRW_Insert::attlist.
    // The file declares 17 visible-attribute instances; require at least 14
    // to flow through (the 3 outliers exercise an unimplemented MText-style
    // ATTRIB variant tracked separately).
    if (dr.iface.typeCounts.count("ATTRIB"))
        CHECK(dr.iface.typeCounts.at("ATTRIB") >= 14);

    printDeepReport("gear_pump_subassy.dwg", dr);
}

// End-to-end pipeline test: load gear_pump_subassy.dwg via the full
// RS_FilterDXFRW reader and verify that visible block attribute text
// flows through to RS_Text entities in the resulting graphic.
// Run: ./librecad_tests "[.dwg_gear_pump_attrib]" -s
TEST_CASE("DWG gear_pump_subassy: ATTRIB pipeline", "[.dwg_gear_pump_attrib]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("gear_pump_subassy.dwg not present; skipping"); return;
    }

    HatchFillIface iface;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::None);
        bool ok = reader.read(&iface, true);
        REQUIRE(ok);
    }

    const int attribs = iface.typeCounts.count("ATTRIB")
                          ? iface.typeCounts.at("ATTRIB") : 0;
    const int inserts = iface.typeCounts.count("INSERT")
                          ? iface.typeCounts.at("INSERT") : 0;

    std::cout << "\n  INSERT count = " << inserts
              << "\n  ATTRIB count = " << attribs << "\n";

    // The file has 7 INSERTs and 17 declared ATTRIBs.  At least 14 should be
    // routed via attlist (3 outliers exercise the MText-style attribute path
    // that this iteration does not yet decode).
    CHECK(inserts == 7);
    CHECK(attribs >= 14);
}

// Deep diagnostic for House-Dwgfree.com_-1.dwg (AC1021/R2007, ~988 entities).
// Run: ./librecad_tests "[.dwg_house]" -s
TEST_CASE("DWG House-Dwgfree.com_-1: entity population", "[.dwg_house]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/House-Dwgfree.com_-1.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("House-Dwgfree.com_-1.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);

    static const std::set<int> kGraphicalOTypes = {
        1, 7, 8, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101
    };
    int unhandledGraphicalLost = 0;
    for (const auto& [oType, count] : dr.unhandledTypes) {
        if (kGraphicalOTypes.count(oType)) {
            std::cout << "  *** LOST graphical entity oType=" << oType
                      << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                      << " count=" << count << "\n";
            unhandledGraphicalLost += count;
        }
    }
    CHECK(unhandledGraphicalLost == 0);

    CHECK(dr.iface.total() > 0);
    CHECK(dr.iface.blocks > 0);
    CHECK(dr.iface.layers > 0);

    printDeepReport("House-Dwgfree.com_-1.dwg", dr);
}

// Deep diagnostic for pump_wheel.dwg (AC1024/R2010, 45 entities).
// Run: ./librecad_tests "[.dwg_pump_wheel]" -s
TEST_CASE("DWG pump_wheel: entity population", "[.dwg_pump_wheel]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/pump_wheel.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("pump_wheel.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);

    static const std::set<int> kGraphicalOTypes = {
        1, 7, 8, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101
    };
    int unhandledGraphicalLost = 0;
    for (const auto& [oType, count] : dr.unhandledTypes) {
        if (kGraphicalOTypes.count(oType)) {
            std::cout << "  *** LOST graphical entity oType=" << oType
                      << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                      << " count=" << count << "\n";
            unhandledGraphicalLost += count;
        }
    }
    CHECK(unhandledGraphicalLost == 0);

    CHECK(dr.iface.total() > 0);
    CHECK(dr.iface.blocks > 0);
    CHECK(dr.iface.layers > 0);

    printDeepReport("pump_wheel.dwg", dr);
}

// Deep diagnostic for robot_handling_cell.dwg (AC1024/R2010, 7341 entities).
// Run: ./librecad_tests "[.dwg_robot]" -s
TEST_CASE("DWG robot_handling_cell: entity population", "[.dwg_robot]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/robot_handling_cell.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("robot_handling_cell.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);

    static const std::set<int> kGraphicalOTypes = {
        1, 7, 8, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101
    };
    int unhandledGraphicalLost = 0;
    for (const auto& [oType, count] : dr.unhandledTypes) {
        if (kGraphicalOTypes.count(oType)) {
            std::cout << "  *** LOST graphical entity oType=" << oType
                      << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                      << " count=" << count << "\n";
            unhandledGraphicalLost += count;
        }
    }
    CHECK(unhandledGraphicalLost == 0);

    CHECK(dr.iface.total() > 0);
    CHECK(dr.iface.blocks > 0);
    CHECK(dr.iface.layers > 0);

    printDeepReport("robot_handling_cell.dwg", dr);
}

// Scans the corpus for WIPEOUT entities (custom-class oType >= 500 with class
// recName "WIPEOUT").  Pre-fix these were silently dropped via the
// [custom-class-skipped] log path; this test asserts the dispatch-and-parse
// path is wired up.  The test is informational by default — it always passes
// when the corpus contains zero WIPEOUTs — and prints per-file counts when
// any are present so visual / round-trip work has a starting point.
TEST_CASE("DWG corpus: WIPEOUT entity inventory", "[.dwg_wipeout]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/doc/dwg2/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("~/doc/dwg2/ not found; skipping");
        return;
    }

    int totalWipeouts = 0;
    int totalVertices = 0;
    int filesWithWipeout = 0;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        const std::string ext = p.extension().string();
        if (ext != ".dwg" && ext != ".DWG") continue;
        if (p.filename().string().front() == '#') continue;

        CountingIface iface;
        try {
            dwgR reader(p.string().c_str());
            reader.read(&iface, true);
        } catch (...) {
            continue;
        }

        if (iface.wipeouts > 0) {
            ++filesWithWipeout;
            totalWipeouts += iface.wipeouts;
            totalVertices += iface.wipeoutVertices;
            std::cout << "  " << p.filename().string()
                      << ": " << iface.wipeouts << " wipeouts, "
                      << iface.wipeoutVertices << " polygon vertices total\n";
            // A WIPEOUT with empty clipPath is the historical bug shape (the
            // dwg-side polygon was being read and discarded).  Ensure that
            // every wipeout we delivered carries actual geometry.
            CHECK(iface.wipeoutVertices >= iface.wipeouts * 3);
        }
    }

    std::cout << "\nCorpus WIPEOUT summary: "
              << totalWipeouts << " entities across "
              << filesWithWipeout << " files, "
              << totalVertices << " vertices\n";
    SUCCEED();
}

// MULTILEADER (MLEADER) inventory.  Pre-fix the dispatcher logged WipeoutVar /
// MLEADER as [custom-class-skipped]; with Phase 2 in place, MLEADERs reach
// addMLeader() and get counted.  Phases 3-4 add structural assertions
// (entity-level fields populated, AnnotContext roots/lines non-empty).
TEST_CASE("DWG corpus: MULTILEADER entity inventory", "[.dwg_mleader]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::array<std::string, 2> dirs = {
        std::string(home) + "/doc/dwg/",   // primary corpus (has the
                                            // architectural multileaders file)
        std::string(home) + "/doc/dwg2/",  // secondary corpus
    };

    int totalMLeaders = 0;
    int filesWithMLeader = 0;

    for (const auto& dir : dirs) {
        if (!std::filesystem::is_directory(dir)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            const auto& p = entry.path();
            const std::string ext = p.extension().string();
            if (ext != ".dwg" && ext != ".DWG") continue;
            if (p.filename().string().front() == '#') continue;

            CountingIface iface;
            try {
                dwgR reader(p.string().c_str());
                reader.read(&iface, true);
            } catch (...) {
                continue;
            }

            if (iface.mleaders > 0) {
                ++filesWithMLeader;
                totalMLeaders += iface.mleaders;
                std::cout << "  " << p.filename().string()
                          << ": " << iface.mleaders << " MLEADERs, "
                          << iface.mleaderRoots << " roots, "
                          << iface.mleaderLines << " leader lines, "
                          << iface.mleaderPoints << " points, "
                          << iface.mleaderStyles << " styles\n";
            }
        }
    }

    std::cout << "\nCorpus MLEADER summary: "
              << totalMLeaders << " entities across "
              << filesWithMLeader << " files\n";
    SUCCEED();
}
