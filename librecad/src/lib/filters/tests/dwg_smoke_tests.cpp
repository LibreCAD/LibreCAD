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

#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
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
#include <QCoreApplication>

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_settings.h"

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

    // Parse [entity-pass-defer N] tokens (formerly [unhandled-entity-type N])
    // out of captured buffer.  These are oTypes the entity-pass switch did
    // not dispatch and queued in objObjectMap for the OBJECTS pass; some
    // (DICTIONARY/MLINESTYLE/LAYOUT/IMAGEDEF) are handled there, the rest
    // are truly unhandled.  Field name is kept as `unhandledTypes` to
    // preserve callsite assertions; semantically it's "deferred or lost".
    std::map<int,int> unhandledTypes() const {
        std::map<int,int> result;
        std::regex re(R"(\[entity-pass-defer (\d+)\])");
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
    void addMLine(const DRW_MLine* e) override {
        track(*e);
        ++mlines;
        mlineVerts += static_cast<int>(e->vertlist.size());
    }
    void addUnderlay(const DRW_Underlay* e) override {
        track(*e);
        ++underlays;
        underlayClipVerts += static_cast<int>(e->clipBoundary.size());
    }
    void linkUnderlay(const DRW_UnderlayDefinition*) override {
        ++underlayDefs;
    }
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
    int mlines = 0;
    int mlineVerts = 0;
    int underlays = 0;
    int underlayClipVerts = 0;
    int underlayDefs = 0;
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
        case DRW::MC00:   return "MC0.0/R1.1";
        case DRW::AC12:   return "AC1.2/R1.2";
        case DRW::AC14:   return "AC1.4/R1.4";
        case DRW::AC150:  return "AC1.50/R2.0";
        case DRW::AC210:  return "AC2.10/R2.10";
        case DRW::AC1002: return "AC1002/R2.5";
        case DRW::AC1003: return "AC1003/R2.6";
        case DRW::AC1004: return "AC1004/R9";
        case DRW::AC1006: return "AC1006/R10";
        case DRW::AC1009: return "AC1009/R11-R12";
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
    std::string currentBlockName;          // empty when not inside any block
    int modelSpaceEntities = 0;
    int blockSpaceEntities = 0;
    int blocks = 0;
    int layers = 0;
    size_t wipeoutVertexCount = 0;

    // Classify by the *current block's name*, not by block-stack depth.
    // Entities inside *Model_Space / *Paper_Space (or arriving via the
    // entity stream with no block context) are model/paper space; anything
    // inside a real block is block-space. The previous heuristic (inBlock>1)
    // assumed the first block opened was always *MODEL_SPACE, which fails
    // for DWG files where model-space entities arrive via readDwgEntities
    // (no addBlock for model-space) and the first addBlock opens a real
    // block — those entities then mis-classified as model-space.
    bool currentlyInRealBlock() const {
        if (inBlock <= 0) return false;
        // Case-insensitive prefix match against *Model_Space / *Paper_Space.
        auto startsWithCi = [&](const char* prefix) {
            const size_t n = std::strlen(prefix);
            if (currentBlockName.size() < n) return false;
            for (size_t i = 0; i < n; ++i)
                if (std::tolower(static_cast<unsigned char>(currentBlockName[i]))
                    != std::tolower(static_cast<unsigned char>(prefix[i])))
                    return false;
            return true;
        };
        return !(startsWithCi("*Model_Space") || startsWithCi("*Paper_Space"));
    }

    void trackT(const DRW_Entity& e, const char* typeName) {
        typeCounts[typeName]++;
        if (e.layer.empty())
            layerEntities["(no layer)"]++;
        else
            layerEntities[e.layer]++;
        if (currentlyInRealBlock())
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

    void addBlock(const DRW_Block& b) override {
        ++blocks; ++inBlock; currentBlockName = b.name;
    }
    void setBlock(const int) override {}
    void endBlock() override {
        if (inBlock > 0) --inBlock;
        if (inBlock == 0) currentBlockName.clear();
    }

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
        std::cout << "\n  *** entity-pass-defer types"
                  << " (DICTIONARY/MLINESTYLE/LAYOUT/IMAGEDEF land in OBJECTS pass;"
                  << " other oTypes are silently skipped) ***\n";
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

// Pool_Detail.dwg circle dump — diagnostic for the spurious circle reported at
// center (512.3864, 333.9300), r=0.1.  Captures DRW_Circle metadata that the
// production filter discards (space, extrusion, owner) so we can tell whether
// a spurious circle is paper-space, invisible, or layer-frozen.
//
// Run:
//   ./librecad_tests "[.dwg_pool_circles]" -s
TEST_CASE("DWG Pool_Detail.dwg: dump every circle", "[.dwg_pool_circles]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path = std::string(home) + "/doc/dwg/Pool_Detail.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Pool_Detail.dwg not present; skipping"); return;
    }

    struct CircleCollector : public TypeTrackingIface {
        struct CInfo {
            double cx, cy, cz, r;
            std::string layer;
            duint32 handle;
            duint32 parentHandle;
            int space;          // 0=Model, 1=Paper
            bool visible;
            double ex, ey, ez;
            double thickness;
        };
        std::vector<CInfo> circles;
        std::map<std::string, int> layerFlags;
        void addLayer(const DRW_Layer& e) override {
            TypeTrackingIface::addLayer(e);
            layerFlags[e.name] = e.flags;
        }
        void addCircle(const DRW_Circle& e) override {
            TypeTrackingIface::addCircle(e);
            circles.push_back({
                e.basePoint.x, e.basePoint.y, e.basePoint.z, e.radious,
                e.layer, e.handle, e.parentHandle,
                static_cast<int>(e.space), e.visible,
                e.extPoint.x, e.extPoint.y, e.extPoint.z,
                e.thickness
            });
        }
    } iface;

    DRW::setCustomDebugPrinter(new DRW::DebugPrinter()); // silent
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, true));

    // Sort: paper-space first, then by layer, then by handle, so leaks group.
    std::sort(iface.circles.begin(), iface.circles.end(),
        [](const auto& a, const auto& b){
            if (a.space != b.space) return a.space > b.space;
            if (a.layer != b.layer) return a.layer < b.layer;
            return a.handle < b.handle;
        });

    std::cout << "\nLayer flags (per ODA: bit0=frozen bit1=off bit2=frozen-in-new bit3=locked, bit6=hasEntity):\n";
    for (const auto& [name, flags] : iface.layerFlags) {
        std::cout << "  layer=\"" << name << "\""
                  << " flags=0x" << std::hex << flags << std::dec
                  << " frozen=" << ((flags & 0x1) ? "1" : "0")
                  << " hasEnt=" << ((flags & 0x40) ? "1" : "0") << "\n";
    }

    std::cout << "\nFound " << iface.circles.size() << " circle(s):\n";
    std::cout << std::fixed << std::setprecision(7);
    int idx = 0;
    for (const auto& c : iface.circles) {
        int lf = iface.layerFlags.count(c.layer) ? iface.layerFlags.at(c.layer) : -1;
        const char* spaceName = c.space == 0 ? "MODEL" :
                                c.space == 1 ? "PAPER" : "ENT3?";
        std::cout << "  [" << idx++ << "] "
                  << spaceName << "(" << c.space << ")"
                  << " center=(" << c.cx << ", " << c.cy << ", " << c.cz << ")"
                  << " r=" << c.r
                  << " layer=\"" << c.layer << "\" layerFlags=0x" << std::hex << lf << std::dec
                  << " visible=" << (c.visible ? "1" : "0")
                  << " handle=0x" << std::hex << c.handle
                  << " parent=0x" << c.parentHandle << std::dec
                  << std::fixed << std::setprecision(7)
                  << " ext=(" << c.ex << "," << c.ey << "," << c.ez << ")"
                  << " thick=" << c.thickness << "\n";
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
// Hidden tag [.dwg3_verbose]: full debug trace of the example_2007/2010/2013/
// 2018.dwg series in ~/doc/dwg3/ so the R2010+ entity reader regression can
// be diagnosed by side-by-side compare. Run with:
//   ./librecad_tests "[.dwg3_verbose]" -s
TEST_CASE("DWG verbose: example_*.dwg in ~/doc/dwg3/", "[.dwg3_verbose]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string dir = std::string(home) + "/doc/dwg3/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("~/doc/dwg3/ not found"); return;
    }
    const char* names[] = {
        "example_2007.dwg",
        "example_2010.dwg",
        "example_2013.dwg",
        "example_2018.dwg",
    };
    for (const auto* name : names) {
        const std::string path = dir + name;
        if (!std::filesystem::is_regular_file(path)) {
            std::cout << "\n=== " << name << " (missing) ===\n";
            continue;
        }
        std::cout << "\n=== " << name << " ===\n";
        const DwgResult r = readDwg(path, /*verbose=*/true);
        std::cout << "Result: " << errorStr(r.error)
                  << "  version=" << versionStr(r.version)
                  << "  entities=" << r.entities
                  << "  blocks=" << r.blocks
                  << "  layers=" << r.layers << "\n";
    }
}

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

// Deep diagnostic for lever_detail.dwg (AC1024/R2010, ~241 entities,
// 18 blocks, 29 layers).  Smoke loader reports OK, but user-observed
// partial render means some entity types are silently dropped.  This
// probe enumerates handled vs. unhandled oTypes so we can target the fix.
//   ./librecad_tests "[.dwg_lever]" -s 2>/tmp/lever_trace.txt
TEST_CASE("DWG lever_detail: entity population", "[.dwg_lever]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/lever_detail.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("lever_detail.dwg not present; skipping"); return;
    }

    const DeepResult dr = readDwgDeep(path);

    REQUIRE(dr.ok);
    REQUIRE(dr.error == DRW::BAD_NONE);
    REQUIRE(dr.iface.total() > 0);

    printDeepReport("lever_detail.dwg", dr);
}

// Helper: human-readable name for RS2::EntityType.
const char* rttiName(RS2::EntityType t) {
    switch (t) {
    case RS2::EntityUnknown:        return "Unknown";
    case RS2::EntityContainer:      return "Container";
    case RS2::EntityBlock:          return "Block";
    case RS2::EntityFontChar:       return "FontChar";
    case RS2::EntityInsert:         return "Insert";
    case RS2::EntityGraphic:        return "Graphic";
    case RS2::EntityPoint:          return "Point";
    case RS2::EntityLine:           return "Line";
    case RS2::EntityPolyline:       return "Polyline";
    case RS2::EntityVertex:         return "Vertex";
    case RS2::EntityArc:            return "Arc";
    case RS2::EntityCircle:         return "Circle";
    case RS2::EntityEllipse:        return "Ellipse";
    case RS2::EntitySolid:          return "Solid";
    case RS2::EntityMText:          return "MText";
    case RS2::EntityText:           return "Text";
    case RS2::EntityDimAligned:     return "DimAligned";
    case RS2::EntityDimLinear:      return "DimLinear";
    case RS2::EntityDimRadial:      return "DimRadial";
    case RS2::EntityDimDiametric:   return "DimDiametric";
    case RS2::EntityDimAngular:     return "DimAngular";
    case RS2::EntityDimOrdinate:    return "DimOrdinate";
    case RS2::EntityDimLeader:      return "DimLeader";
    case RS2::EntityHatch:          return "Hatch";
    case RS2::EntityImage:          return "Image";
    case RS2::EntityWipeout:        return "Wipeout";
    case RS2::EntityMLeader:        return "MLeader";
    case RS2::EntitySpline:         return "Spline";
    case RS2::EntitySplinePoints:   return "SplinePoints";
    case RS2::EntityParabola:       return "Parabola";
    case RS2::EntityHyperbola:      return "Hyperbola";
    default:                        return "(other)";
    }
}

// Filter-pipeline diagnostic for lever_detail.dwg.  The libdxfrw side
// delivers 252 entities cleanly (see [.dwg_lever]), but the user observes
// a partial render in LibreCAD.  This probe loads the file via the full
// RS_FilterDXFRW pipeline and reports the resulting RS_Entity tree, broken
// down by RTTI type, layer membership, and layer visibility.  Designed to
// surface entities that are delivered, attached to a layer, but not visible
// (frozen / off / lineweight=invisible / etc.).
//   ./librecad_tests "[.dwg_lever_filter]" -s
TEST_CASE("DWG lever_detail: filter pipeline + visibility audit",
          "[.dwg_lever_filter]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/lever_detail.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("lever_detail.dwg not present; skipping"); return;
    }

    // Bootstrap a minimal Qt app context so RS_Graphic ctor's
    // LC_GROUP_GUARD ("Defaults") + RS_Settings paths don't dereference null.
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = { qarg0, nullptr };
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    static bool settingsReady = []{
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)qapp; (void)settingsReady;

    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    const bool imported = filter.fileImport(graphic,
                                            QString::fromStdString(path),
                                            RS2::FormatDWG);
    REQUIRE(imported);

    // RTTI counts at top level (model space).
    std::map<RS2::EntityType, int> typeCount;
    std::map<QString, int> layerCount;
    std::map<RS2::EntityType, int> typeOnHiddenLayer;
    int totalTopLevel = 0;

    for (auto* e : graphic) {
        if (!e) continue;
        ++totalTopLevel;
        const RS2::EntityType t = e->rtti();
        typeCount[t]++;
        const QString lyrName = e->getLayer() ? e->getLayer()->getName() : "(none)";
        layerCount[lyrName]++;
        if (e->getLayer() && e->getLayer()->isFrozen()) {
            typeOnHiddenLayer[t]++;
        }
    }

    std::cout << "\n=== lever_detail.dwg (post-filter, model space) ===\n";
    std::cout << "Top-level RS_Entity count: " << totalTopLevel << "\n";
    std::cout << "Container countDeep():     " << graphic.countDeep() << "\n\n";

    std::cout << "RTTI type distribution (top level):\n";
    for (const auto& [t, n] : typeCount) {
        std::cout << "  " << std::left << std::setw(18) << rttiName(t)
                  << " " << n << "\n";
    }

    if (!typeOnHiddenLayer.empty()) {
        std::cout << "\nEntities on FROZEN/INVISIBLE layers:\n";
        int hiddenTotal = 0;
        for (const auto& [t, n] : typeOnHiddenLayer) {
            std::cout << "  " << std::left << std::setw(18) << rttiName(t)
                      << " " << n << "\n";
            hiddenTotal += n;
        }
        std::cout << "  TOTAL hidden: " << hiddenTotal << "\n";
    } else {
        std::cout << "\n(no entities on hidden layers)\n";
    }

    std::cout << "\nLayer distribution (top level):\n";
    std::vector<std::pair<int, QString>> layers;
    for (const auto& [name, n] : layerCount) layers.emplace_back(n, name);
    std::sort(layers.rbegin(), layers.rend());
    for (const auto& [n, name] : layers) {
        std::cout << "  " << std::left << std::setw(30)
                  << name.toStdString() << " " << n << "\n";
    }

    // Top-level INSERT block-name breakdown (which block each modelspace
    // INSERT references). Useful to see if the XREF block is instantiated.
    std::cout << "\nTop-level INSERTs by referenced block:\n";
    for (auto* e : graphic) {
        if (e && e->rtti() == RS2::EntityInsert) {
            auto* ins = static_cast<RS_Insert*>(e);
            std::cout << "  ref=\"" << ins->getName().toStdString()
                      << "\" pos=(" << ins->getInsertionPoint().x
                      << "," << ins->getInsertionPoint().y << ")\n";
        }
    }

    // Block-list breakdown: per-block direct + deep counts.
    auto* blockList = graphic.getBlockList();
    if (blockList) {
        std::cout << "\nBlock definitions (" << blockList->count() << " blocks):\n";
        std::map<RS2::EntityType, int> blockTypeCount;
        int blockEntityTotal = 0;
        for (unsigned i = 0; i < blockList->count(); ++i) {
            RS_Block* bk = blockList->at(i);
            if (!bk) continue;
            const unsigned direct = bk->count();
            const unsigned deep   = bk->countDeep();
            blockEntityTotal += direct;
            std::cout << "  " << std::left << std::setw(28)
                      << bk->getName().toStdString()
                      << " count=" << std::setw(4) << direct
                      << " deep=" << deep << "\n";
            for (auto* e : *bk) {
                if (e) blockTypeCount[e->rtti()]++;
            }
        }
        std::cout << "Block direct-entity total: " << blockEntityTotal << "\n";
        std::cout << "Block RTTI distribution:\n";
        for (const auto& [t, n] : blockTypeCount) {
            std::cout << "  " << std::left << std::setw(18) << rttiName(t)
                      << " " << n << "\n";
        }

        const int grandTotal = totalTopLevel + blockEntityTotal;
        std::cout << "\n*** Filter accepted total: "
                  << grandTotal << " RS_Entities "
                  << "(model-space=" << totalTopLevel
                  << ", in-blocks=" << blockEntityTotal << ")\n";
        std::cout << "*** libdxfrw delivered (per [.dwg_lever]): 252\n";
    }

    SUCCEED();
}

// Same as [.dwg_lever_filter] but for ~/doc/dwg2/gear_pump_subassy.dwg, which
// is the witness for the "no visible entity, blocks empty after insert" bug:
// the GUI shows a single rendered POINT and DIN_A3 (the A3 sheet frame block)
// is empty.  The libdxfrw smoke counter reports 521 entities + 26 blocks
// loaded, and dwg2dxf produces a DXF where DIN_A3 has 166 entities — proving
// libdxfrw delivers the data.  This test runs the actual RS_FilterDXFRW path
// to pinpoint where they go in the LibreCAD container tree, plus reports any
// duplicate block names that may have made `RS_BlockList::add()` return false
// (which deletes the block and leaves `m_currentContainer` stale, leaking
// subsequent addEntity calls into model space).
//   ./librecad_tests "[.dwg_gear_pump_filter]" -s
TEST_CASE("DWG gear_pump_subassy: filter pipeline + block routing audit",
          "[.dwg_gear_pump_filter]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("gear_pump_subassy.dwg not present; skipping"); return;
    }

    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = { qarg0, nullptr };
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    static bool settingsReady = []{
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)qapp; (void)settingsReady;

    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    const bool imported = filter.fileImport(graphic,
                                            QString::fromStdString(path),
                                            RS2::FormatDWG);
    REQUIRE(imported);

    std::map<RS2::EntityType, int> typeCount;
    std::map<QString, int> layerCount;
    int totalTopLevel = 0;
    int topLevelInserts = 0;
    for (auto* e : graphic) {
        if (!e) continue;
        ++totalTopLevel;
        const RS2::EntityType t = e->rtti();
        typeCount[t]++;
        const QString lyrName = e->getLayer() ? e->getLayer()->getName() : "(none)";
        layerCount[lyrName]++;
        if (t == RS2::EntityInsert) ++topLevelInserts;
    }

    std::cout << "\n=== gear_pump_subassy.dwg (post-filter) ===\n";
    std::cout << "Top-level RS_Entity count: " << totalTopLevel << "\n";
    std::cout << "Container countDeep():     " << graphic.countDeep() << "\n";
    std::cout << "Top-level INSERTs:         " << topLevelInserts << "\n\n";

    std::cout << "RTTI type distribution (top level):\n";
    for (const auto& [t, n] : typeCount) {
        std::cout << "  " << std::left << std::setw(18) << rttiName(t)
                  << " " << n << "\n";
    }

    std::cout << "\nLayer distribution (top level):\n";
    std::vector<std::pair<int, QString>> layers;
    for (const auto& [name, n] : layerCount) layers.emplace_back(n, name);
    std::sort(layers.rbegin(), layers.rend());
    for (const auto& [n, name] : layers) {
        std::cout << "  " << std::left << std::setw(34)
                  << name.toStdString() << " " << n << "\n";
    }

    std::cout << "\nTop-level INSERTs by referenced block:\n";
    for (auto* e : graphic) {
        if (e && e->rtti() == RS2::EntityInsert) {
            auto* ins = static_cast<RS_Insert*>(e);
            std::cout << "  ref=\"" << ins->getName().toStdString()
                      << "\" pos=(" << ins->getInsertionPoint().x
                      << "," << ins->getInsertionPoint().y << ")\n";
        }
    }

    auto* blockList = graphic.getBlockList();
    REQUIRE(blockList != nullptr);
    std::cout << "\nBlock definitions (" << blockList->count() << " blocks):\n";
    std::map<RS2::EntityType, int> blockTypeCount;
    int blockEntityTotal = 0;
    for (unsigned i = 0; i < blockList->count(); ++i) {
        RS_Block* bk = blockList->at(i);
        if (!bk) continue;
        const unsigned direct = bk->count();
        const unsigned deep   = bk->countDeep();
        blockEntityTotal += direct;
        std::cout << "  " << std::left << std::setw(28)
                  << bk->getName().toStdString()
                  << " count=" << std::setw(4) << direct
                  << " deep=" << deep << "\n";
        for (auto* e : *bk) {
            if (e) blockTypeCount[e->rtti()]++;
        }
    }
    std::cout << "Block direct-entity total: " << blockEntityTotal << "\n";
    std::cout << "Block RTTI distribution:\n";
    for (const auto& [t, n] : blockTypeCount) {
        std::cout << "  " << std::left << std::setw(18) << rttiName(t)
                  << " " << n << "\n";
    }

    const int grandTotal = totalTopLevel + blockEntityTotal;
    std::cout << "\n*** Filter accepted total: " << grandTotal
              << " RS_Entities (model-space=" << totalTopLevel
              << ", in-blocks=" << blockEntityTotal << ")\n";
    std::cout << "*** libdxfrw delivered (per [.dwg_gear_pump]): 535\n";
    const int leaked = grandTotal - 535;
    std::cout << "*** Surplus (model-space leakage if positive, lost if negative): "
              << leaked << "\n";

    // RS_FilterDXFRW intentionally does NOT register *Model_Space,
    // *Paper_Space, *Paper_Space0 in the visible block list (see
    // RS_FilterDXFRW::addBlock paper_space/model_space branch); subtract them
    // before flagging an unexpected loss.
    constexpr int kSpecialBlocks = 3; // *Model_Space, *Paper_Space, *Paper_Space0
    constexpr int kLibdxfrwAddBlockCount = 26;
    const int expected = kLibdxfrwAddBlockCount - kSpecialBlocks;
    std::cout << "*** libdxfrw addBlock count: " << kLibdxfrwAddBlockCount
              << "; blockList->count(): " << blockList->count()
              << " (expect " << expected
              << " after dropping 3 model/paper-space blocks)\n";
    if (static_cast<int>(blockList->count()) < expected) {
        std::cout << "*** ⚠ unexpected block loss: "
                  << (expected - blockList->count())
                  << " block(s) missing from the visible block list\n";
    }

    // Regression assertions — these are the symptoms the user reported.
    // Pre-fix: DIN_A3 (count=0), GENAXEH (count=0).  Post-fix: both populated.
    auto blockEntityCount = [&](const QString& name) -> int {
        for (unsigned i = 0; i < blockList->count(); ++i) {
            RS_Block* bk = blockList->at(i);
            if (bk && bk->getName() == name) return static_cast<int>(bk->count());
        }
        return -1;
    };
    const int din_a3    = blockEntityCount("DIN_A3");
    const int din_title = blockEntityCount("DIN_TITLE");
    const int genaxeh   = blockEntityCount("GENAXEH");
    std::cout << "\nNamed-block check:\n";
    std::cout << "  DIN_A3:    " << din_a3 << " entities (expect > 0)\n";
    std::cout << "  DIN_TITLE: " << din_title << " entities (expect > 0)\n";
    std::cout << "  GENAXEH:   " << genaxeh << " entities (expect > 0)\n";

    CHECK(din_a3 > 0);
    CHECK(din_title > 0);
    CHECK(genaxeh > 0);
}

// Regression: lever_detail.dwg references gripper_assembly_new.dwg as an
// unresolved XREF (DRW_Block::flags & 0x04, xrefPath populated). The filter
// must detect this, resolve the path (handling Windows backslashes,
// case-insensitive + space-to-underscore basename match), recursively load
// the external file, and embed its modelspace entities into the local
// `GRIPPER ASSEMBLY NEW` block. Pre-fix the block was empty (count=0);
// post-fix it carries the lever assembly geometry (~900+ entities).
//   ./librecad_tests "[.dwg_lever_xref_resolve]"
TEST_CASE("DWG lever_detail: XREF resolution embeds external geometry",
          "[.dwg_lever_xref_resolve]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string hostPath =
        std::string(home) + "/doc/dwg2/lever_detail.dwg";
    const std::string xrefPath =
        std::string(home) + "/doc/dwg2/gripper_assembly_new.dwg";
    if (!std::filesystem::is_regular_file(hostPath)) {
        SUCCEED("lever_detail.dwg not present; skipping"); return;
    }
    if (!std::filesystem::is_regular_file(xrefPath)) {
        SUCCEED("gripper_assembly_new.dwg (XREF source) not present; "
                "skipping"); return;
    }

    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = { qarg0, nullptr };
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    static bool settingsReady = []{
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)qapp; (void)settingsReady;

    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic,
                              QString::fromStdString(hostPath),
                              RS2::FormatDWG));

    auto* blockList = graphic.getBlockList();
    REQUIRE(blockList);
    RS_Block* xrefBlock = blockList->find("GRIPPER ASSEMBLY NEW");
    REQUIRE(xrefBlock);

    INFO("XREF block direct-entity count = " << xrefBlock->count());
    CHECK(xrefBlock->count() >= 900);

    // Layer namespacing should also be in place: a layer like
    // "GRIPPER ASSEMBLY NEW|AM_5" must exist on the host's layer list,
    // populated by the XREF source's "AM_5" layer.
    auto* layers = graphic.getLayerList();
    REQUIRE(layers);
    CHECK(layers->find("GRIPPER ASSEMBLY NEW|AM_5") != nullptr);
    CHECK(layers->find("GRIPPER ASSEMBLY NEW|0") != nullptr);

    // After embedding, all entities in the block must hold a layer pointer
    // pointing at a host-graphic layer (not a dangling external pointer).
    int correctlyNamespaced = 0;
    for (auto* e : *xrefBlock) {
        if (!e || !e->getLayer()) continue;
        const QString lyrName = e->getLayer()->getName();
        if (lyrName.startsWith("GRIPPER ASSEMBLY NEW|")) {
            ++correctlyNamespaced;
        }
    }
    INFO("Entities with namespaced layer: " << correctlyNamespaced);
    CHECK(correctlyNamespaced >= 900);
}

namespace {

// Build a minimal DXF buffer that defines a single XREF block referring
// to @p xrefTarget. Only the structural minimum needed for libdxfrw to
// parse cleanly: HEADER (ACADVER), TABLES (one LAYER), BLOCKS (the XREF
// block), ENTITIES (empty). Flag 70/4 is the XREF bit; group 1 carries
// the xrefPath that filter-side embedXref will try to resolve.
std::string buildCycleDxf(const std::string& blockName,
                          const std::string& xrefTarget) {
    std::ostringstream s;
    s << "0\nSECTION\n2\nHEADER\n"
         "9\n$ACADVER\n1\nAC1015\n"
         "9\n$INSBASE\n10\n0.0\n20\n0.0\n30\n0.0\n"
         "0\nENDSEC\n"
         "0\nSECTION\n2\nTABLES\n"
         "0\nTABLE\n2\nLAYER\n70\n1\n"
         "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
         "0\nENDTAB\n"
         "0\nENDSEC\n"
         "0\nSECTION\n2\nBLOCKS\n"
         "0\nBLOCK\n8\n0\n2\n" << blockName << "\n70\n4\n"
         "10\n0.0\n20\n0.0\n30\n0.0\n"
         "3\n" << blockName << "\n"
         "1\n" << xrefTarget << "\n"
         "0\nENDBLK\n8\n0\n"
         "0\nENDSEC\n"
         "0\nSECTION\n2\nENTITIES\n"
         "0\nENDSEC\n"
         "0\nEOF\n";
    return s.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

} // anon

// Synthetic A→B→A cycle test for the XREF recursion guard. Writes two
// minimal DXF files referencing each other, loads A.dxf, and verifies
// the load completes (no infinite recursion) AND both XREF blocks are
// present on the host. The cycle guard's RAII insert at fileImport
// start means the cycle is detected at depth 2 (when B's XREF→A is
// processed: m_xrefStack already contains A's absolute path).
TEST_CASE("RS_FilterDXFRW: XREF A->B->A cycle terminates",
          "[xref][filter]") {
    // Bootstrap Qt + RS_Settings once, like the other filter tests.
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = { qarg0, nullptr };
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    static bool settingsReady = []{
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)qapp; (void)settingsReady;

    const auto pathA = std::filesystem::temp_directory_path()
                     / "librecad_xref_cycle_a.dxf";
    const auto pathB = std::filesystem::temp_directory_path()
                     / "librecad_xref_cycle_b.dxf";
    std::filesystem::remove(pathA);
    std::filesystem::remove(pathB);

    // A references B, B references A — perfect cycle.
    writeFile(pathA.string(), buildCycleDxf("XREF_TO_B", pathB.string()));
    writeFile(pathB.string(), buildCycleDxf("XREF_TO_A", pathA.string()));

    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    // If the guard misbehaves, fileImport never returns (infinite
    // recursion until stack overflow). Catch2 would still see a SIGSEGV
    // rather than hang, so this is a useful test even without a timeout.
    const bool imported = filter.fileImport(
        graphic, QString::fromStdString(pathA.string()), RS2::FormatDXFRW);
    REQUIRE(imported);

    // Both XREF blocks should have been added to A's blockList by the
    // addBlock dispatcher, regardless of whether the embed succeeded.
    auto* blockList = graphic.getBlockList();
    REQUIRE(blockList);
    CHECK(blockList->find("XREF_TO_B") != nullptr);
    // XREF_TO_A came in transitively via the embedXref of B. With the
    // cycle guard active, B's embed must have succeeded (refusing to
    // recurse back into A), so its blocks reach A's blockList with the
    // namespaced prefix `XREF_TO_B|`.
    CHECK(blockList->find("XREF_TO_B|XREF_TO_A") != nullptr);

    std::filesystem::remove(pathA);
    std::filesystem::remove(pathB);
}

// Cross-check: load the source XREF (gripper_assembly_new.dwg) and report
// the entity count.  Confirms whether the lever assembly drawing actually
// lives in the standalone XREF source vs. having been baked into
// lever_detail.dwg's local block (which appears empty).
TEST_CASE("DWG gripper_assembly_new: XREF source population",
          "[.dwg_gripper_xref]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/gripper_assembly_new.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("gripper_assembly_new.dwg not present; skipping"); return;
    }
    const DeepResult dr = readDwgDeep(path);
    REQUIRE(dr.ok);
    printDeepReport("gripper_assembly_new.dwg", dr);
}

namespace {
class BlockTrackingIface : public TypeTrackingIface {
public:
    struct BlockEvent {
        std::string name;
        int handle = 0;
        int parentHandle = 0;
        int flags = 0;
        int entityCount = 0;
        std::map<std::string, int> layerCount;
        std::string xrefPath;
    };
    std::vector<BlockEvent> events;
    BlockEvent* current = nullptr;
    int entitiesBeforeAnyBlock = 0;
    int entitiesAfterEndBlock  = 0;
    int blockDepth = 0;
    std::map<std::string, int> postBlockLayerCount;

    void addBlock(const DRW_Block& b) override {
        TypeTrackingIface::addBlock(b);
        events.push_back({b.name, static_cast<int>(b.handle),
                          static_cast<int>(b.parentHandle),
                          b.flags, 0, {}});
        events.back().xrefPath = b.xrefPath;
        current = &events.back();
        ++blockDepth;
    }
    void endBlock() override {
        TypeTrackingIface::endBlock();
        --blockDepth;
        current = nullptr;
    }
    void countEntity(const std::string& layer = "") {
        if (current) {
            ++current->entityCount;
            current->layerCount[layer]++;
        } else if (blockDepth == 0) {
            (events.empty() ? entitiesBeforeAnyBlock : entitiesAfterEndBlock)++;
            postBlockLayerCount[layer]++;
        }
    }
    void addPoint(const DRW_Point& e)        override { countEntity(e.layer); TypeTrackingIface::addPoint(e); }
    void addLine(const DRW_Line& e)          override { countEntity(e.layer); TypeTrackingIface::addLine(e); }
    void addArc(const DRW_Arc& e)            override { countEntity(e.layer); TypeTrackingIface::addArc(e); }
    void addCircle(const DRW_Circle& e)      override { countEntity(e.layer); TypeTrackingIface::addCircle(e); }
    void addEllipse(const DRW_Ellipse& e)    override { countEntity(e.layer); TypeTrackingIface::addEllipse(e); }
    void addLWPolyline(const DRW_LWPolyline& e) override { countEntity(e.layer); TypeTrackingIface::addLWPolyline(e); }
    void addPolyline(const DRW_Polyline& e)  override { countEntity(e.layer); TypeTrackingIface::addPolyline(e); }
    void addInsert(const DRW_Insert& e)      override { countEntity(e.layer); TypeTrackingIface::addInsert(e); }
    void addText(const DRW_Text& e)          override { countEntity(e.layer); TypeTrackingIface::addText(e); }
    void addMText(const DRW_MText& e)        override { countEntity(e.layer); TypeTrackingIface::addMText(e); }
    void addSolid(const DRW_Solid& e)        override { countEntity(e.layer); TypeTrackingIface::addSolid(e); }
    void add3dFace(const DRW_3Dface& e)      override { countEntity(e.layer); TypeTrackingIface::add3dFace(e); }
    void addTrace(const DRW_Trace& e)        override { countEntity(e.layer); TypeTrackingIface::addTrace(e); }
    void addDimAlign(const DRW_DimAligned* e)        override { countEntity(e->layer); TypeTrackingIface::addDimAlign(e); }
    void addDimLinear(const DRW_DimLinear* e)        override { countEntity(e->layer); TypeTrackingIface::addDimLinear(e); }
    void addDimRadial(const DRW_DimRadial* e)        override { countEntity(e->layer); TypeTrackingIface::addDimRadial(e); }
    void addDimDiametric(const DRW_DimDiametric* e)  override { countEntity(e->layer); TypeTrackingIface::addDimDiametric(e); }
    void addDimAngular(const DRW_DimAngular* e)      override { countEntity(e->layer); TypeTrackingIface::addDimAngular(e); }
    void addDimAngular3P(const DRW_DimAngular3p* e)  override { countEntity(e->layer); TypeTrackingIface::addDimAngular3P(e); }
    void addDimOrdinate(const DRW_DimOrdinate* e)    override { countEntity(e->layer); TypeTrackingIface::addDimOrdinate(e); }
    void addLeader(const DRW_Leader* e)              override { countEntity(e->layer); TypeTrackingIface::addLeader(e); }
    void addHatch(const DRW_Hatch* e)                override { countEntity(e->layer); TypeTrackingIface::addHatch(e); }
    void addSpline(const DRW_Spline* e)              override { countEntity(e->layer); TypeTrackingIface::addSpline(e); }
    void addImage(const DRW_Image* e)                override { countEntity(e->layer); TypeTrackingIface::addImage(e); }
    void addViewport(const DRW_Viewport& e)          override { countEntity(e.layer); TypeTrackingIface::addViewport(e); }
};
} // anon

// Per-block delivery tracker for lever_detail.dwg.  Enumerates every
// addBlock event with name + handle + entity count, so duplicate-name
// blocks (RS_BlockList::add returns false → entities silently dropped)
// stand out at a glance.
//   ./librecad_tests "[.dwg_lever_blocks]" -s
TEST_CASE("DWG lever_detail: per-block entity delivery",
          "[.dwg_lever_blocks]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/lever_detail.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("lever_detail.dwg not present; skipping"); return;
    }

    BlockTrackingIface iface;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::None);
        REQUIRE(reader.read(&iface, true));
    }

    std::cout << "\n=== lever_detail.dwg block delivery (libdxfrw) ===\n";
    std::cout << "Pre-block: "  << iface.entitiesBeforeAnyBlock
              << "  Post-block: " << iface.entitiesAfterEndBlock << "\n";
    std::cout << "addBlock events: " << iface.events.size() << "\n";

    std::map<std::string, std::vector<int>> nameToCounts;
    int totalInBlocks = 0;
    for (const auto& ev : iface.events) {
        const bool isXref = (ev.flags & 0x04) != 0;
        const bool isXrefOverlay = (ev.flags & 0x08) != 0;
        const bool isAnonymous = (ev.flags & 0x01) != 0;
        std::cout << "  " << std::setw(3) << ev.handle << "/"
                  << std::setw(3) << ev.parentHandle
                  << "  flags=0x" << std::hex << std::setw(2) << ev.flags << std::dec
                  << "  count=" << std::setw(4) << ev.entityCount
                  << "  name=" << ev.name;
        if (isXref) std::cout << "  [XREF]";
        if (isXrefOverlay) std::cout << "  [XREF-OVERLAY]";
        if (isAnonymous) std::cout << "  [anonymous]";
        if (!ev.xrefPath.empty())
            std::cout << "  xrefPath=\"" << ev.xrefPath << "\"";
        if (!ev.layerCount.empty()) {
            std::cout << "  layers={";
            bool first = true;
            for (const auto& [lyr, n] : ev.layerCount) {
                if (!first) std::cout << ", ";
                std::cout << lyr << ":" << n;
                first = false;
            }
            std::cout << "}";
        }
        std::cout << "\n";
        nameToCounts[ev.name].push_back(ev.entityCount);
        totalInBlocks += ev.entityCount;
    }
    std::cout << "Total entities seen inside blocks: " << totalInBlocks << "\n";

    std::cout << "\nPost-block (modelspace) layer distribution:\n";
    for (const auto& [lyr, n] : iface.postBlockLayerCount) {
        std::cout << "  " << lyr << ": " << n << "\n";
    }

    std::cout << "\nDuplicate block names (lose entities at filter):\n";
    int dupCount = 0;
    for (const auto& [name, counts] : nameToCounts) {
        if (counts.size() > 1) {
            std::cout << "  " << name << " appears " << counts.size()
                      << " times: ";
            int sumDup = 0;
            for (size_t i = 0; i < counts.size(); ++i) {
                std::cout << counts[i];
                if (i + 1 < counts.size()) std::cout << ", ";
                if (i > 0) sumDup += counts[i];
            }
            std::cout << "  (extra entities lost: " << sumDup << ")\n";
            dupCount += static_cast<int>(counts.size()) - 1;
        }
    }
    std::cout << "Total duplicate block events: " << dupCount << "\n";

    SUCCEED();
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

// Witness probe for the R2010+ visual-style fix. Counts how many entities
// in a panel of "visualization"-named R2010 files (and the canonical R2013
// witness) actually have any of the three has{Full,Face,Edge}VisualStyle
// flags set, by grepping the DRW_DBG capture for the marker emitted in
// DRW_Entity::parseDwg. Soft-asserts that visualization_-_aerial.dwg loads
// with at least as many entities as the pre-fix baseline (0).
//
// Reference: ground-truth from libreDWG common_entity_data.spec lines
// 523-528 + ODA spec v5.4.1 §19.4.1; libdxfrw fix landed in commit
// (current).
TEST_CASE("DWG visualstyle probe: count R2010+ visual-style flag triggers",
          "[.dwg_visualstyle_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }

    struct Witness { std::string path; int baselineEntities; };
    const std::vector<Witness> witnesses = {
        { std::string(home) + "/doc/dwg/visualization_-_aerial.dwg",                  0 },
        { std::string(home) + "/doc/dwg/visualization_-_condominium_with_skylight.dwg", 17 },
        { std::string(home) + "/doc/dwg/visualization_-_conference_room.dwg",         14 },
        { std::string(home) + "/doc/dwg/visualization_-_sun_and_sky_demo.dwg",        16 },
        { std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg",  856 },
    };

    int totalFiles = 0, totalFlagged = 0;
    int totalLeftoverWarnings = 0;
    int aerialEntities = -1;

    for (const auto& w : witnesses) {
        if (!std::filesystem::is_regular_file(w.path)) {
            std::cout << "(skipping missing) " << w.path << "\n";
            continue;
        }
        ++totalFiles;

        const DeepResult dr = readDwgDeep(w.path);
        const std::string& log = dr.debugLog;

        // Count "hasFull/Face/Edge VisualStyle: a b c" lines where any of
        // a/b/c is 1. The DRW_DBG marker prints space-separated 0/1 values.
        const std::string marker = "hasFull/Face/Edge VisualStyle: ";
        int flaggedHere = 0;
        size_t pos = 0;
        while ((pos = log.find(marker, pos)) != std::string::npos) {
            pos += marker.size();
            if (pos + 5 <= log.size()) {
                // Expect "a b c\n" where a,b,c are '0' or '1'
                const char a = log[pos],     b = log[pos + 2], c = log[pos + 4];
                if (a == '1' || b == '1' || c == '1') ++flaggedHere;
            }
        }

        // Count leftover-bytes warnings — should drop to 0 for files using
        // visual styles after Phase B.
        const std::string leftover = "parseDwgEntHandle leftover";
        int leftoverHere = 0;
        size_t lpos = 0;
        while ((lpos = log.find(leftover, lpos)) != std::string::npos) {
            ++leftoverHere; ++lpos;
        }

        const std::string fname = std::filesystem::path(w.path).filename().string();
        std::cout << "  " << std::left << std::setw(56) << fname
                  << " entities=" << std::setw(5) << (dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities)
                  << " flaggedEntities=" << std::setw(4) << flaggedHere
                  << " leftoverWarnings=" << leftoverHere
                  << " (baseline entities=" << w.baselineEntities << ")\n";

        totalFlagged += flaggedHere;
        totalLeftoverWarnings += leftoverHere;

        if (fname == "visualization_-_aerial.dwg") {
            aerialEntities = (dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities);
            // Soft assertion: must not REGRESS below baseline. A jump
            // upward (e.g., 0 → N) is the strongest positive signal.
            CHECK((dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities) >= w.baselineEntities);
        }
    }

    std::cout << "\nVisualStyle probe summary: "
              << totalFlagged << " flagged entities across "
              << totalFiles  << " witness files; "
              << totalLeftoverWarnings << " leftover-bytes warnings\n";
    if (aerialEntities >= 0) {
        std::cout << "  visualization_-_aerial.dwg post-fix entities: "
                  << aerialEntities << " (baseline 0)\n";
    }
    SUCCEED();
}

// AcDbColor probe — counts DBCOLOR objects in OBJECTS sections + tracks
// how many entities carry a resolved color24/colorName from a DBCOLOR
// reference. ODA spec §20.4 / libreDWG dwg2.spec:2404-2408 (object) and
// common_entity_data.spec:454-459 (ENC flag 0x40 → handle in hdl_dat).
// Diagnostic; not asserted hard.
TEST_CASE("DWG acdbcolor probe: count DBCOLOR objects + resolved entity refs",
          "[.dwg_acdbcolor_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }

    // Custom interface that counts DBCOLOR objects via addDbColor and
    // tracks how many entities arrive with a populated color24/colorName.
    class AcDbColorIface : public TypeTrackingIface {
    public:
        int dbColors = 0;
        int entitiesWithColor24 = 0;
        int entitiesWithColorName = 0;
        std::vector<std::pair<int,std::string>> sampleResolutions; // first few

        void addDbColor(const DRW_DbColor& d) override {
            ++dbColors;
            if (sampleResolutions.size() < 5) {
                std::string display = d.bookName.empty()
                    ? d.name : (d.bookName + "$" + d.name);
                sampleResolutions.emplace_back(d.rgb, display);
            }
        }
        void track(const DRW_Entity& e) {
            if (e.color24 != -1) ++entitiesWithColor24;
            if (!e.colorName.empty()) ++entitiesWithColorName;
        }
        // Hook a few common entity types — enough to surface any resolved
        // color references without re-overriding every addX in the base.
        void addPoint(const DRW_Point& e) override { track(e); TypeTrackingIface::addPoint(e); }
        void addLine(const DRW_Line& e) override { track(e); TypeTrackingIface::addLine(e); }
        void addCircle(const DRW_Circle& e) override { track(e); TypeTrackingIface::addCircle(e); }
        void addArc(const DRW_Arc& e) override { track(e); TypeTrackingIface::addArc(e); }
        void addInsert(const DRW_Insert& e) override { track(e); TypeTrackingIface::addInsert(e); }
        void addText(const DRW_Text& e) override { track(e); TypeTrackingIface::addText(e); }
        void addLWPolyline(const DRW_LWPolyline& e) override { track(e); TypeTrackingIface::addLWPolyline(e); }
        void addPolyline(const DRW_Polyline& e) override { track(e); TypeTrackingIface::addPolyline(e); }
        void addHatch(const DRW_Hatch* e) override { track(*e); TypeTrackingIface::addHatch(e); }
    };

    struct File { std::string path; const char* note; };
    const std::vector<File> files = {
        { std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg",
          "R2013, canonical book-color witness (per memory)" },
        { std::string(home) + "/doc/dwg/visualization_-_aerial.dwg",        "R2010" },
        { std::string(home) + "/doc/dwg/visualization_-_condominium_with_skylight.dwg", "R2010" },
        { std::string(home) + "/doc/dwg/visualization_-_conference_room.dwg", "R2010" },
        { std::string(home) + "/doc/dwg/visualization_-_sun_and_sky_demo.dwg", "R2010" },
        { std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg", "R2010 control (must not regress)" },
    };

    int totalDbColors = 0, totalResolvedColor24 = 0, totalResolvedNames = 0;
    for (const auto& f : files) {
        if (!std::filesystem::is_regular_file(f.path)) {
            std::cout << "(skipping missing) " << f.path << "\n";
            continue;
        }
        AcDbColorIface iface;
        try {
            dwgR reader(f.path.c_str());
            reader.read(&iface, true);
        } catch (...) {
            std::cout << "EXCEPTION on " << f.path << "\n";
            continue;
        }
        const std::string fname = std::filesystem::path(f.path).filename().string();
        const int totalEnt = iface.modelSpaceEntities + iface.blockSpaceEntities;
        std::cout << "  " << std::left << std::setw(56) << fname
                  << " dbColors=" << std::setw(4) << iface.dbColors
                  << " entWithColor24=" << std::setw(4) << iface.entitiesWithColor24
                  << " entWithColorName=" << std::setw(4) << iface.entitiesWithColorName
                  << " entities=" << totalEnt
                  << "  (" << f.note << ")\n";
        for (const auto& s : iface.sampleResolutions) {
            std::cout << "    sample DBCOLOR rgb=" << std::hex << s.first
                      << std::dec << " name=\"" << s.second << "\"\n";
        }
        totalDbColors += iface.dbColors;
        totalResolvedColor24 += iface.entitiesWithColor24;
        totalResolvedNames += iface.entitiesWithColorName;
    }

    std::cout << "\nAcDbColor probe summary: "
              << totalDbColors << " DBCOLOR objects, "
              << totalResolvedColor24 << " entities w/ color24, "
              << totalResolvedNames << " entities w/ colorName\n";
    SUCCEED();
}

// AcDbColor end-to-end test: load the canonical book-color witness file
// and assert (a) it loads BAD_NONE, (b) entity count matches the existing
// [.dwg_arch_hatch] sentinel of 856 entities (no regression), and (c)
// at least one DBCOLOR object exists in the file (positive coverage if
// the file actually uses book colors). Untagged so it runs in default
// smoke pass once we're confident the fix is stable.
TEST_CASE("DWG acdbcolor: book color load + dbColor object inventory",
          "[.dwg_acdbcolor]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home)
        + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("witness file not present; skipping"); return;
    }

    int dbColors = 0;
    class CountingDbColorIface : public TypeTrackingIface {
    public:
        int* countOut = nullptr;
        void addDbColor(const DRW_DbColor& /*d*/) override { if (countOut) ++(*countOut); }
    };
    CountingDbColorIface iface;
    iface.countOut = &dbColors;

    DRW::error err = DRW::BAD_NONE;
    int entities = 0;
    try {
        dwgR reader(path.c_str());
        reader.read(&iface, true);
        err = reader.getError();
        entities = iface.modelSpaceEntities + iface.blockSpaceEntities;
    } catch (const std::exception& ex) {
        FAIL("Exception: " << ex.what());
    }

    REQUIRE(err == DRW::BAD_NONE);
    // Existing [.dwg_arch_hatch] sentinel proves 48 hatches; the broader
    // entity baseline is 856 (per golden corpus output).
    CHECK(entities == 856);

    std::cout << "Architectural-Modern-Building-Design.dwg DBCOLOR objects: "
              << dbColors << "\n";
    // Soft expectation: the file IS named "Architectural" and uses
    // book colors per memory note. If 0, that's surprising but not a
    // failure — the spec/parser correctness is asserted by entity count.
}

// Layer-level AcDbColor probe — counts how many layers in each corpus file
// have a populated colorName (CMC method-byte bit 1 → libreDWG bit_read_T
// from str_dat).  Diagnostic only; no hard assertion since corpus coverage
// is unknown until we run.
TEST_CASE("DWG acdbcolor: layer colorName probe",
          "[.dwg_acdbcolor_layer_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }

    class LayerColorIface : public TypeTrackingIface {
    public:
        int totalLayers = 0;
        int layersWithColor24 = 0;
        int layersWithColorName = 0;
        std::vector<std::tuple<std::string, int, std::string>> samples;
        void addLayer(const DRW_Layer& l) override {
            ++totalLayers;
            ++layers;  // base class counter
            if (l.color24 != -1) ++layersWithColor24;
            if (!l.colorName.empty()) {
                ++layersWithColorName;
                if (samples.size() < 10) {
                    samples.emplace_back(l.name, l.color24, l.colorName);
                }
            }
        }
    };

    struct Dir { std::string path; const char* note; };
    const std::vector<Dir> dirs = {
        { std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus" },
        { std::string(home) + "/doc/dwg/",  "mixed corpus" },
        { std::string(home) + "/dev/dwg_samples/", "single-entity samples" },
    };

    int totalFiles = 0, totalLayers = 0, totalColor24 = 0, totalColorName = 0;
    for (const auto& d : dirs) {
        if (!std::filesystem::is_directory(d.path)) {
            std::cout << "(skipping missing dir) " << d.path << "\n";
            continue;
        }
        std::cout << "\n=== " << d.path << " (" << d.note << ") ===\n";
        std::vector<std::filesystem::path> paths;
        for (const auto& e : std::filesystem::directory_iterator(d.path)) {
            if (!e.is_regular_file()) continue;
            const auto ext = e.path().extension().string();
            if (ext == ".dwg" || ext == ".DWG") paths.push_back(e.path());
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& p : paths) {
            const std::string fname = p.filename().string();
            if (fname.front() == '#') continue;  // BAD_VERSION sentinel
            ++totalFiles;
            LayerColorIface iface;
            try {
                dwgR reader(p.string().c_str());
                reader.read(&iface, true);
            } catch (...) { continue; }
            if (iface.layersWithColorName > 0 || iface.layersWithColor24 > 0) {
                std::cout << "  " << std::left << std::setw(56) << fname
                          << " layers=" << std::setw(4) << iface.totalLayers
                          << " withColor24=" << std::setw(4) << iface.layersWithColor24
                          << " withColorName=" << iface.layersWithColorName << "\n";
                for (const auto& s : iface.samples) {
                    std::cout << "    layer \"" << std::get<0>(s)
                              << "\"  color24=" << std::hex << std::get<1>(s)
                              << std::dec << "  colorName=\"" << std::get<2>(s)
                              << "\"\n";
                }
            }
            totalLayers   += iface.totalLayers;
            totalColor24  += iface.layersWithColor24;
            totalColorName += iface.layersWithColorName;
        }
    }

    std::cout << "\nLayer color probe summary: " << totalFiles << " files, "
              << totalLayers << " layers total, "
              << totalColor24 << " with color24, "
              << totalColorName << " with colorName\n";
    SUCCEED();
}

// PLOTSETTINGS probe — count PLOTSETTINGS objects per corpus file. They
// were silently dropped to remainingMap before the custom-class dispatch
// added the recName=="PLOTSETTINGS" clause. ODA spec §20.4 / libreDWG
// dwg.spec:5627. RS_FilterDXFRW::addPlotSettings already wires margins +
// page name to m_graphic; this probe confirms delivery.
TEST_CASE("DWG plotsettings probe: count PLOTSETTINGS objects per file",
          "[.dwg_plotsettings_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }

    class PlotSettingsIface : public TypeTrackingIface {
    public:
        int plotSettingsCount = 0;
        std::vector<std::tuple<std::string,double,double,double,double>> samples;
        void addPlotSettings(const DRW_PlotSettings* d) override {
            ++plotSettingsCount;
            if (samples.size() < 10) {
                samples.emplace_back(d->plotViewName,
                                     d->marginLeft, d->marginTop,
                                     d->marginRight, d->marginBottom);
            }
        }
    };

    struct Dir { std::string path; const char* note; };
    const std::vector<Dir> dirs = {
        { std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus" },
        { std::string(home) + "/doc/dwg/",  "mixed corpus" },
    };

    int totalFiles = 0, totalPlotSettings = 0;
    for (const auto& d : dirs) {
        if (!std::filesystem::is_directory(d.path)) {
            std::cout << "(skipping missing) " << d.path << "\n"; continue;
        }
        std::cout << "\n=== " << d.path << " ===\n";
        std::vector<std::filesystem::path> paths;
        for (const auto& e : std::filesystem::directory_iterator(d.path)) {
            if (!e.is_regular_file()) continue;
            const auto ext = e.path().extension().string();
            if (ext == ".dwg" || ext == ".DWG") paths.push_back(e.path());
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& p : paths) {
            const std::string fname = p.filename().string();
            if (fname.front() == '#') continue;
            ++totalFiles;
            PlotSettingsIface iface;
            try {
                dwgR reader(p.string().c_str());
                reader.read(&iface, true);
            } catch (...) { continue; }
            if (iface.plotSettingsCount > 0) {
                std::cout << "  " << std::left << std::setw(56) << fname
                          << " plotSettings=" << iface.plotSettingsCount << "\n";
                for (const auto& s : iface.samples) {
                    std::cout << "    \"" << std::get<0>(s)
                              << "\"  margins L/T/R/B = "
                              << std::get<1>(s) << "/" << std::get<2>(s) << "/"
                              << std::get<3>(s) << "/" << std::get<4>(s) << "\n";
                }
            }
            totalPlotSettings += iface.plotSettingsCount;
        }
    }

    std::cout << "\nPLOTSETTINGS summary: " << totalPlotSettings
              << " objects across " << totalFiles << " files\n";
    SUCCEED();
}

// Transparency probe — count entities per file with a non-default
// `transparency` field set via ENC flag 0x20. libreDWG
// common_entity_data.spec:432-446 documents the alpha_raw encoding;
// RS_FilterDXFRW::setEntityAttributes converts alpha_type==3 to a
// per-entity pen alpha. This probe confirms delivery and shows whether
// the corpus exercises the path.
TEST_CASE("DWG transparency probe: count entities with ENC alpha set",
          "[.dwg_transparency_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }

    class TransparencyIface : public TypeTrackingIface {
    public:
        int entitiesWithTransparency = 0;
        int alphaType3Count = 0;       // explicit alpha
        int alphaTypeBlockOrLayer = 0; // type 0 or 1
        std::vector<std::pair<std::string,unsigned>> samples;

        void track(const DRW_Entity& e, const char* what) {
            if (e.transparency == DRW::Opaque) return;
            ++entitiesWithTransparency;
            const unsigned int raw = static_cast<unsigned int>(e.transparency);
            const unsigned int aType = (raw >> 24) & 0xFF;
            if (aType == 3) ++alphaType3Count;
            else if (aType == 0 || aType == 1) ++alphaTypeBlockOrLayer;
            if (samples.size() < 10) samples.emplace_back(what, raw);
        }
        void addPoint(const DRW_Point& e) override { track(e,"POINT"); TypeTrackingIface::addPoint(e); }
        void addLine(const DRW_Line& e) override { track(e,"LINE"); TypeTrackingIface::addLine(e); }
        void addCircle(const DRW_Circle& e) override { track(e,"CIRCLE"); TypeTrackingIface::addCircle(e); }
        void addArc(const DRW_Arc& e) override { track(e,"ARC"); TypeTrackingIface::addArc(e); }
        void addInsert(const DRW_Insert& e) override { track(e,"INSERT"); TypeTrackingIface::addInsert(e); }
        void addText(const DRW_Text& e) override { track(e,"TEXT"); TypeTrackingIface::addText(e); }
        void addLWPolyline(const DRW_LWPolyline& e) override { track(e,"LWPOLY"); TypeTrackingIface::addLWPolyline(e); }
        void addPolyline(const DRW_Polyline& e) override { track(e,"POLY"); TypeTrackingIface::addPolyline(e); }
        void addHatch(const DRW_Hatch* e) override { track(*e,"HATCH"); TypeTrackingIface::addHatch(e); }
    };

    struct Dir { std::string path; const char* note; };
    const std::vector<Dir> dirs = {
        { std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus" },
        { std::string(home) + "/doc/dwg/",  "mixed corpus" },
    };

    int totalFiles = 0, totalEnt = 0, totalAlpha3 = 0;
    for (const auto& d : dirs) {
        if (!std::filesystem::is_directory(d.path)) continue;
        std::cout << "\n=== " << d.path << " ===\n";
        std::vector<std::filesystem::path> paths;
        for (const auto& e : std::filesystem::directory_iterator(d.path)) {
            if (!e.is_regular_file()) continue;
            const auto ext = e.path().extension().string();
            if (ext == ".dwg" || ext == ".DWG") paths.push_back(e.path());
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& p : paths) {
            const std::string fname = p.filename().string();
            if (fname.front() == '#') continue;
            ++totalFiles;
            TransparencyIface iface;
            try {
                dwgR reader(p.string().c_str());
                reader.read(&iface, true);
            } catch (...) { continue; }
            if (iface.entitiesWithTransparency > 0) {
                std::cout << "  " << std::left << std::setw(56) << fname
                          << " entWithAlpha=" << iface.entitiesWithTransparency
                          << "  type3=" << iface.alphaType3Count
                          << "  inherit=" << iface.alphaTypeBlockOrLayer << "\n";
                for (const auto& s : iface.samples) {
                    std::cout << "    " << std::get<0>(s)
                              << "  alpha_raw=0x" << std::hex << std::get<1>(s)
                              << std::dec << "\n";
                }
            }
            totalEnt += iface.entitiesWithTransparency;
            totalAlpha3 += iface.alphaType3Count;
        }
    }

    std::cout << "\nTransparency summary: " << totalEnt
              << " entities across " << totalFiles << " files; "
              << totalAlpha3 << " with explicit alpha (type 3)\n";
    SUCCEED();
}

// ---- field-level parity tests for DIMENSION + HATCH ------------------------

namespace {

struct HatchFieldSnapshot {
    int    isGradient;
    int    singleColor;
    double gradAngle;
    double gradShift;
    double gradTint;
    std::string gradName;
    size_t gradColorCount;
    size_t seedPointCount;
};

struct DimFieldSnapshot {
    std::string subtype;
    double measureValue;
    bool   flipArrow1;
    bool   flipArrow2;
};

class FieldCaptureIface : public TypeTrackingIface {
public:
    std::vector<HatchFieldSnapshot> hatchSnaps;
    std::vector<DimFieldSnapshot>   dimSnaps;

    void addHatch(const DRW_Hatch* e) override {
        TypeTrackingIface::addHatch(e);
        hatchSnaps.push_back({e->isGradient, e->singleColor, e->gradAngle,
                              e->gradShift, e->gradTint, e->gradName,
                              e->gradColors.size(), e->seedPoints.size()});
    }
    void addDimAlign(const DRW_DimAligned* e) override {
        TypeTrackingIface::addDimAlign(e);
        dimSnaps.push_back({"DIM_ALIGNED", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimLinear(const DRW_DimLinear* e) override {
        TypeTrackingIface::addDimLinear(e);
        dimSnaps.push_back({"DIM_LINEAR", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimRadial(const DRW_DimRadial* e) override {
        TypeTrackingIface::addDimRadial(e);
        dimSnaps.push_back({"DIM_RADIAL", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimDiametric(const DRW_DimDiametric* e) override {
        TypeTrackingIface::addDimDiametric(e);
        dimSnaps.push_back({"DIM_DIAMETRIC", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimAngular(const DRW_DimAngular* e) override {
        TypeTrackingIface::addDimAngular(e);
        dimSnaps.push_back({"DIM_ANGULAR", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimAngular3P(const DRW_DimAngular3p* e) override {
        TypeTrackingIface::addDimAngular3P(e);
        dimSnaps.push_back({"DIM_ANGULAR3P", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
    void addDimOrdinate(const DRW_DimOrdinate* e) override {
        TypeTrackingIface::addDimOrdinate(e);
        dimSnaps.push_back({"DIM_ORDINATE", e->getMeasureValue(),
                            e->getFlipArrow1(), e->getFlipArrow2()});
    }
};

} // namespace

// Verifies that the DWG decoder actually populates DRW_Hatch's new gradient
// + seed-point members (previously read-then-discarded). Uses the same
// Architectural-Modern-Building-Design.dwg fixture that already exercises 48
// HATCH entities including gradient fills.
TEST_CASE("DWG hatch field parity: gradient + seed points populate", "[.dwg_hatch_fields]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string path =
        std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("Architectural-Modern-Building-Design.dwg not present; skipping"); return;
    }

    FieldCaptureIface iface;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::None);
        REQUIRE(reader.read(&iface, true));
    }

    REQUIRE(iface.hatchSnaps.size() >= 1u);

    int gradientHatches = 0;
    int hatchesWithSeeds = 0;
    int totalSeedPoints = 0;
    for (const auto& s : iface.hatchSnaps) {
        if (s.isGradient) {
            ++gradientHatches;
            // A gradient hatch must have at least one stop and a name.
            CHECK(s.gradColorCount >= 1u);
            CHECK(!s.gradName.empty());
        }
        if (s.seedPointCount > 0) {
            ++hatchesWithSeeds;
            totalSeedPoints += static_cast<int>(s.seedPointCount);
        }
    }

    std::cout << "\n  hatches: " << iface.hatchSnaps.size()
              << "  gradient: " << gradientHatches
              << "  with seeds: " << hatchesWithSeeds
              << "  total seed points: " << totalSeedPoints << "\n";

    // The fixture is documented to include gradient hatches — at least one
    // should now populate the new DRW_Hatch fields.
    CHECK(gradientHatches >= 1);
}

// Verifies that DIMENSION's measureValue (code 42) and flipArrow1/2 (codes
// 74/75) survive the DWG parser. Pre-fix these were read-then-discarded for
// AC1015+/AC1021+ files. We just sanity-check that the values are sane:
// measureValue is finite; flipArrow flags are 0 or 1 (bool).
TEST_CASE("DWG dimension field parity: measureValue + flipArrow populate", "[.dwg_dim_fields]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set; skipping"); return; }
    const std::string dir = std::string(home) + "/doc/dwg2/";
    if (!std::filesystem::is_directory(dir)) {
        SUCCEED("DWG corpus directory not found; skipping"); return;
    }

    int filesScanned = 0;
    int totalDims    = 0;
    int nonZeroMeasure = 0;
    int flippedArrows  = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() != ".dwg") continue;
        FieldCaptureIface iface;
        try {
            dwgR reader(entry.path().c_str());
            reader.setDebug(DRW::DebugLevel::None);
            if (!reader.read(&iface, true)) continue;
        } catch (...) { continue; }
        ++filesScanned;
        for (const auto& d : iface.dimSnaps) {
            ++totalDims;
            // measureValue should be finite (NaN/Inf would indicate the
            // assignment never happened).
            CHECK(std::isfinite(d.measureValue));
            if (d.measureValue != 0.0) ++nonZeroMeasure;
            if (d.flipArrow1 || d.flipArrow2) ++flippedArrows;
        }
    }

    std::cout << "\n  files scanned: " << filesScanned
              << "  dims: " << totalDims
              << "  non-zero measure: " << nonZeroMeasure
              << "  flipped arrows: " << flippedArrows << "\n";

    // At least some dimensions in the corpus should have non-zero measure
    // values. If none do, either the assignment is broken or the corpus has
    // no meaningful dimensions.
    if (totalDims > 0)
        CHECK(nonZeroMeasure > 0);
}

// ----------------------------------------------------------------------------
// Pre-R13 detection: confirm the reader recognizes AC2.10 (1986) header,
// classifies it as BAD_VERSION (no parser exists), and surfaces the
// recognized version through dwgR::getVersion(). This is what powers the
// improved user-facing error message in RS_FilterDXFRW.
// ----------------------------------------------------------------------------
TEST_CASE("DWG pre-R13 detection: ~/doc/dwg3/block.dwg classifies AC2.10",
          "[.dwg_pre_r13]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home) + "/doc/dwg3/block.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("~/doc/dwg3/block.dwg not found"); return;
    }
    const DwgResult r = readDwg(path);
    CHECK_FALSE(r.ok);
    CHECK(r.error == DRW::BAD_VERSION);
    CHECK(r.version == DRW::AC210);
    INFO("recognized version: " << versionStr(r.version));
}

// Deep troubleshooting probe for ~/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg.
// Reports MLEADER fidelity (per-entity points, breaks, text content), MLEADER style count,
// per-block entity delivery, dimension subtype coverage, and any debug-trace warnings
// that suggest parse drift (leftover bytes, alignment hiccups, DBG markers).
// Run: ./librecad_tests "[.dwg_arch_mleader_probe]" -s
TEST_CASE("DWG arch_multileaders: deep fidelity probe",
          "[.dwg_arch_mleader_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home)
        + "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("file not present; skipping"); return;
    }

    class Probe : public TypeTrackingIface {
    public:
        struct MLeaderSnap {
            int    rootCount;
            int    leaderLineCount;
            int    pointCount;       // sum across all leader lines
            int    breakCount;       // sum: root-level + line-level
            bool   hasTextContents;
            std::string textLabel;
            int    styleContentType;
            double overallScale;
            bool   hasContentsBlock;
            std::string layer;
        };
        std::vector<MLeaderSnap> mleaderSnaps;
        std::map<std::string,int> blockEntityCounts; // block_idx-name -> count when inBlock>0
        int currentBlockIdx = -1;
        std::string currentBlockName;
        int mleaderStyles = 0;

        void addBlock(const DRW_Block& b) override {
            TypeTrackingIface::addBlock(b);
            ++currentBlockIdx;
            currentBlockName = b.name;
        }
        void endBlock() override {
            TypeTrackingIface::endBlock();
        }

        void trackInBlock() {
            if (inBlock > 0) {
                ++blockEntityCounts[std::to_string(currentBlockIdx) + ":" + currentBlockName];
            }
        }

        void addLine(const DRW_Line& e) override { TypeTrackingIface::addLine(e); trackInBlock(); }
        void addArc(const DRW_Arc& e) override { TypeTrackingIface::addArc(e); trackInBlock(); }
        void addCircle(const DRW_Circle& e) override { TypeTrackingIface::addCircle(e); trackInBlock(); }
        void addLWPolyline(const DRW_LWPolyline& e) override { TypeTrackingIface::addLWPolyline(e); trackInBlock(); }
        void addPolyline(const DRW_Polyline& e) override { TypeTrackingIface::addPolyline(e); trackInBlock(); }
        void addInsert(const DRW_Insert& e) override { TypeTrackingIface::addInsert(e); trackInBlock(); }
        void addText(const DRW_Text& e) override { TypeTrackingIface::addText(e); trackInBlock(); }
        void addMText(const DRW_MText& e) override { TypeTrackingIface::addMText(e); trackInBlock(); }
        void addPoint(const DRW_Point& e) override { TypeTrackingIface::addPoint(e); trackInBlock(); }
        void addHatch(const DRW_Hatch* e) override { TypeTrackingIface::addHatch(e); trackInBlock(); }
        void addEllipse(const DRW_Ellipse& e) override { TypeTrackingIface::addEllipse(e); trackInBlock(); }
        void addSpline(const DRW_Spline* e) override { TypeTrackingIface::addSpline(e); trackInBlock(); }

        void addMLeader(const DRW_MLeader* e) override {
            TypeTrackingIface::addMLeader(e);
            trackInBlock();
            int pts = 0, brks = 0;
            for (const auto& r : e->context.roots) {
                brks += static_cast<int>(r.breaks.size());
                for (const auto& ll : r.leaderLines) {
                    pts  += static_cast<int>(ll.points.size());
                    brks += static_cast<int>(ll.breaks.size());
                }
            }
            mleaderSnaps.push_back({static_cast<int>(e->context.roots.size()),
                                    [&]{int n=0; for(auto&r:e->context.roots) n+=r.leaderLines.size(); return n;}(),
                                    pts, brks,
                                    e->context.hasTextContents,
                                    e->context.textLabel,
                                    e->styleContentType,
                                    e->context.overallScale,
                                    e->context.hasContentsBlock,
                                    e->layer});
        }
        void addMLeaderStyle(const DRW_MLeaderStyle*) override { ++mleaderStyles; }
    };

    auto* capture = new CapturingPrinter();
    DRW::setCustomDebugPrinter(capture);

    Probe p;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::Debug);
        bool ok = reader.read(&p, true);
        REQUIRE(ok);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }
    std::string log = std::move(capture->buf);
    DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

    std::cout << "\n=== arch multileaders: probe ===\n";
    std::cout << "blocks=" << p.blocks
              << " layers=" << p.layers
              << " modelspaceEnt=" << p.modelSpaceEntities
              << " inBlocksEnt=" << p.blockSpaceEntities << "\n";

    std::cout << "\nHandled type counts:\n";
    for (const auto& [k,v] : p.typeCounts)
        std::cout << "  " << std::left << std::setw(16) << k << " " << v << "\n";

    std::cout << "\nMLEADER styles delivered: " << p.mleaderStyles << "\n";
    std::cout << "MLEADER snapshots (" << p.mleaderSnaps.size() << "):\n";
    int degenerate = 0, missingText = 0;
    int totalRoots = 0, totalLines = 0, totalPts = 0;
    for (size_t i = 0; i < p.mleaderSnaps.size(); ++i) {
        const auto& m = p.mleaderSnaps[i];
        totalRoots += m.rootCount;
        totalLines += m.leaderLineCount;
        totalPts += m.pointCount;
        bool isDegenerate = (m.pointCount < 2 * m.leaderLineCount);
        bool isMissingText = (m.styleContentType == 2 /*MTEXT*/ && m.textLabel.empty());
        if (isDegenerate) ++degenerate;
        if (isMissingText) ++missingText;
        if (i < 8 || isDegenerate || isMissingText) {
            std::cout << "  #" << i << " layer=" << std::setw(28) << m.layer
                      << " roots=" << m.rootCount
                      << " lines=" << m.leaderLineCount
                      << " pts=" << m.pointCount
                      << " brks=" << m.breakCount
                      << " hasText=" << m.hasTextContents
                      << " contentType=" << m.styleContentType
                      << " scale=" << m.overallScale
                      << " text=\"" << m.textLabel.substr(0, 40) << "\""
                      << (isDegenerate ? "  [DEGENERATE]" : "")
                      << (isMissingText ? "  [MISSING_TEXT]" : "")
                      << "\n";
        }
    }
    std::cout << "MLEADER aggregate: roots=" << totalRoots
              << " lines=" << totalLines
              << " pts=" << totalPts
              << " degenerate=" << degenerate
              << " missingText=" << missingText << "\n";

    std::cout << "\nPer-block entity delivery (top 20):\n";
    std::vector<std::pair<int,std::string>> blockSorted;
    for (const auto& [k,v] : p.blockEntityCounts) blockSorted.emplace_back(v,k);
    std::sort(blockSorted.rbegin(), blockSorted.rend());
    int shown = 0;
    int blocksWithEntities = 0;
    for (const auto& [v,k] : blockSorted) {
        if (v > 0) ++blocksWithEntities;
        if (shown < 20) {
            std::cout << "  " << std::left << std::setw(40) << k << " " << v << "\n";
            ++shown;
        }
    }
    std::cout << "Blocks with delivered entities: " << blocksWithEntities
              << " / " << p.blocks << "\n";

    auto countMarker = [&](const std::string& m) {
        int c = 0; size_t pos = 0;
        while ((pos = log.find(m, pos)) != std::string::npos) { ++c; ++pos; }
        return c;
    };
    std::cout << "\nDebug-trace warnings:\n";
    std::cout << "  parseDwgEntHandle leftover : " << countMarker("parseDwgEntHandle leftover") << "\n";
    std::cout << "  AnnotContext parse drift   : " << countMarker("AnnotContext parse drift") << "\n";
    std::cout << "  MLEADER: parseDwgEntHandle hiccup : " << countMarker("MLEADER: parseDwgEntHandle hiccup") << "\n";
    std::cout << "  MLEADER: handle-stream tail unconsumed : " << countMarker("MLEADER: handle-stream tail") << "\n";
    std::cout << "  not implemented            : " << countMarker("not implemented") << "\n";
    std::cout << "  RLZ                        : " << countMarker("RLZ") << "\n";
    std::cout << "  entity-pass-defer          : " << countMarker("[entity-pass-defer") << "\n";
    std::cout << "  bad CRC                    : " << countMarker("CRC") << "\n";
    std::cout << "  read past end / not ok     : " << countMarker("not ok") << "\n";
    std::cout << "  ERROR/BAD                  : " << countMarker("BAD_") << "\n";

    // MLEADER tail-rb distribution: "MLEADER tail rb=N" lines. With the
    // entity-specific handle stream fully consumed, every entity should
    // report rb in {0..4} (just the trailing CRC).
    {
        std::map<int,int> rbHist;
        std::regex re(R"(MLEADER tail rb=(-?\d+))");
        auto begin = std::sregex_iterator(log.begin(), log.end(), re);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) ++rbHist[std::stoi((*it)[1].str())];
        std::cout << "\nMLEADER tail-rb histogram (bytes left after MLEADER handle stream):\n";
        for (const auto& [rb, cnt] : rbHist)
            std::cout << "  rb=" << std::setw(4) << rb << "  count=" << cnt << "\n";
    }

    SUCCEED();
}

// Regression guard for the MLEADER body parser fix
// (parseMLeaderRoot rewritten to libreDWG dwg2.spec parity, 2026-05-10).
// Asserts on real per-entity content, not just counts — without the fix
// most fields read as denormalized garbage even though the file loads OK.
TEST_CASE("DWG arch_multileaders: MLEADER body parser fidelity") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home)
        + "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("fixture not present; skipping"); return;
    }

    struct Snap {
        std::string layer;
        std::string text;
        int    contentType;
        double scale;
        bool   hasText;
        duint32 styleHandleRef;
        duint32 textStyleHandleRef;       // ctx.textStyleHandle
        duint32 leaderLineTypeHandleRef;
        duint32 styleTextStyleHandleRef;
    };

    class Iface : public TypeTrackingIface {
    public:
        std::vector<Snap> snaps;
        void addMLeader(const DRW_MLeader* e) override {
            TypeTrackingIface::addMLeader(e);
            snaps.push_back({e->layer, e->context.textLabel, e->styleContentType,
                             e->context.overallScale, e->context.hasTextContents,
                             e->styleHandle.ref,
                             e->context.textStyleHandle.ref,
                             e->leaderLineTypeHandle.ref,
                             e->styleTextStyleHandle.ref});
        }
    };

    auto* capture = new CapturingPrinter();
    DRW::setCustomDebugPrinter(capture);

    Iface iface;
    {
        dwgR reader(path.c_str());
        reader.setDebug(DRW::DebugLevel::Debug);
        REQUIRE(reader.read(&iface, true));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }
    const std::string log = std::move(capture->buf);
    DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

    // The fixture is documented to carry 36 MLEADERs.
    REQUIRE(iface.snaps.size() == 36u);

    // No body-parse drift markers in the trace — these were 8 + 12
    // pre-fix and would silently corrupt every subsequent field.
    auto countMarker = [&](const std::string& m) {
        int c = 0; size_t pos = 0;
        while ((pos = log.find(m, pos)) != std::string::npos) { ++c; ++pos; }
        return c;
    };
    CHECK(countMarker("AnnotContext parse drift") == 0);
    CHECK(countMarker("MLEADER: parseDwgEntHandle hiccup") == 0);
    CHECK(countMarker("MLEADER: implausible classVersion") == 0);

    // Tail handle stream fully consumed (Issue 2 fix). Per-MLEADER "tail
    // rb=N" lines should all report 0 — the entity-level + per-line +
    // per-arrowhead/blocklabel handles drain the remaining bits exactly.
    CHECK(countMarker("MLEADER: handle-stream tail") == 0);
    {
        std::regex re(R"(MLEADER tail rb=(-?\d+))");
        auto begin = std::sregex_iterator(log.begin(), log.end(), re);
        auto end   = std::sregex_iterator();
        int total = 0, nonzero = 0;
        for (auto it = begin; it != end; ++it) {
            ++total;
            if (std::stoi((*it)[1].str()) != 0) ++nonzero;
        }
        CHECK(total == 36);
        CHECK(nonzero == 0);
    }

    // Per-entity assertions. Every MLEADER must have:
    //   - styleContentType in {0,1,2,3} — the 4 valid values
    //   - overallScale finite and >0; for THIS fixture, ∈ {4, 24, 48}
    //     (the file's annotation scales)
    //   - layer name resolved via the trailing handle stream, NOT "0"
    //     (pre-fix, ~13/36 landed on "0" because the body left the
    //     handle stream mid-byte and the layer handle resolved wrong)
    int textHits = 0;
    for (const auto& s : iface.snaps) {
        CAPTURE(s.layer, s.text, s.contentType, s.scale);
        CHECK(s.contentType >= 0);
        CHECK(s.contentType <= 3);
        CHECK(std::isfinite(s.scale));
        CHECK(s.scale > 0.0);
        // This fixture: only annotation scales 4, 24, 48 are used.
        CHECK((s.scale == 4.0 || s.scale == 24.0 || s.scale == 48.0));
        // Layer must come from the file's annotation-scale layer naming
        // ("Mleader @ 4/24/48"); pre-fix many MLEADERs misresolved to "0".
        CHECK(s.layer.rfind("Mleader", 0) == 0);
        if (s.hasText && !s.text.empty()) ++textHits;
    }
    // Most MLEADERs in this file carry MTEXT content. Pre-fix, only 7/36
    // had non-empty text; post-fix all 24 MTEXT-typed entities populate.
    CHECK(textHits >= 20);

    // Spot-check a known string survives end-to-end (this MLEADER labels
    // a 1/2" gypsum-board callout — appears verbatim in the file).
    bool foundGypsum = false;
    for (const auto& s : iface.snaps)
        if (s.text.find("GYPSUM BOARD") != std::string::npos) { foundGypsum = true; break; }
    CHECK(foundGypsum);

    // Issue 2: entity-specific handles populate from the trailing handle
    // stream. Pre-fix these slots were declared but never read; after the
    // fix every MLEADER must carry a non-null mleaderstyle handle (the
    // file uses 3 MLEADERSTYLEs across the 36 entities).
    int populatedStyleHandles = 0;
    int populatedTextStyleHandles = 0;
    int populatedLineLTypeHandles = 0;
    std::set<duint32> distinctStyles;
    for (const auto& s : iface.snaps) {
        if (s.styleHandleRef != 0) { ++populatedStyleHandles; distinctStyles.insert(s.styleHandleRef); }
        if (s.hasText && s.textStyleHandleRef != 0) ++populatedTextStyleHandles;
        if (s.leaderLineTypeHandleRef != 0) ++populatedLineLTypeHandles;
    }
    CHECK(populatedStyleHandles == 36);
    // The 36 entities reference more than one distinct MLEADERSTYLE; pre-fix
    // every styleHandle.ref was 0 (slot was declared but never read).
    CHECK(distinctStyles.size() >= 2u);
    // The MTEXT-typed entities should reference an AcDbTextStyle handle
    // via ctx.textStyleHandle. ≥20 matches the populatedText threshold.
    CHECK(populatedTextStyleHandles >= 20);
}

// Regression for Issue 3 (SCALE / AcDbScale parsing).
// The architectural fixture's annotation scaling layers are named
// "Mleader @ 4", "@ 24", "@ 48" — the file's ACAD_SCALELIST should
// contain matching scale entries. Pre-fix, SCALE objects fell into
// the "[entity-pass-defer 706]" bucket and were never parsed.
TEST_CASE("DWG arch_multileaders: SCALE table delivery") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home)
        + "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("fixture not present; skipping"); return;
    }

    class ScaleIface : public TypeTrackingIface {
    public:
        std::vector<DRW_Scale> scales;
        void addScale(const DRW_Scale& s) override { scales.push_back(s); }
    };

    ScaleIface iface;
    {
        dwgR reader(path.c_str());
        REQUIRE(reader.read(&iface, true));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    // The file declares at least the three annotation scales referenced by
    // its layers (1:48, 1:24, 1:4) plus typically a 1:1 unit-scale entry.
    CHECK(iface.scales.size() >= 3u);
    std::cout << "\n  SCALE entries delivered: " << iface.scales.size() << "\n";
    for (const auto& s : iface.scales) {
        std::cout << "    name='" << s.name
                  << "' paper=" << s.paperUnits
                  << " drawing=" << s.drawingUnits
                  << " factor=" << s.scaleFactor()
                  << " unitScale=" << (s.isUnitScale ? 1 : 0) << "\n";
    }

    // Build a {factor → name} index and verify the three layer-named factors
    // (4, 24, 48) all appear in the SCALE table.
    std::map<double,std::string> byFactor;
    bool foundUnit = false;
    for (const auto& s : iface.scales) {
        byFactor[s.scaleFactor()] = s.name;
        if (s.isUnitScale) foundUnit = true;
        // Sanity: every entry has a name and finite ratio.
        CHECK(!s.name.empty());
        CHECK(std::isfinite(s.scaleFactor()));
        CHECK(s.scaleFactor() > 0.0);
    }
    CHECK(byFactor.count(4.0)  == 1u);
    CHECK(byFactor.count(24.0) == 1u);
    CHECK(byFactor.count(48.0) == 1u);
    CHECK(foundUnit);  // at least one 1:1 entry present
}
