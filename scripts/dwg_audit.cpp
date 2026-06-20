// DWG read-coverage audit harness (build-and-run, not part of the test suite).
// Reads each .dwg argument through libdxfrw's dwgRW and prints one pipe-separated
// line per file with the read result and the "missing/incomplete feature"
// signals libdxfrw already tracks: skipped custom-class entities, skipped
// unsupported OBJECTS, entity/object parse failures, decoded proxy primitives.
//
// Build (from repo root):
//   clang++ -std=c++17 -O1 -I libraries/libdxfrw/src \
//     scripts/dwg_audit.cpp \
//     libraries/libdxfrw/src/*.cpp libraries/libdxfrw/src/intern/*.cpp \
//     -o /tmp/dwg_audit
//   /tmp/dwg_audit <files...>
#include <cstdio>
#include <map>
#include <string>

#include "drw_interface.h"
#include "libdwgr.h"

namespace {
class CountIface : public DRW_Interface {
public:
    long ents = 0;   // graphical entities delivered
    long objs = 0;   // non-graphical objects delivered
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override { ++ents; }
    void addLine(const DRW_Line&) override { ++ents; }
    void addRay(const DRW_Ray&) override { ++ents; }
    void addXline(const DRW_Xline&) override { ++ents; }
    void addCircle(const DRW_Circle&) override { ++ents; }
    void addArc(const DRW_Arc&) override { ++ents; }
    void addEllipse(const DRW_Ellipse&) override { ++ents; }
    void addLWPolyline(const DRW_LWPolyline&) override { ++ents; }
    void addPolyline(const DRW_Polyline&) override { ++ents; }
    void addSpline(const DRW_Spline*) override { ++ents; }
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override { ++ents; }
    void addTrace(const DRW_Trace&) override { ++ents; }
    void add3dFace(const DRW_3Dface&) override { ++ents; }
    void addSolid(const DRW_Solid&) override { ++ents; }
    void addMText(const DRW_MText&) override { ++ents; }
    void addText(const DRW_Text&) override { ++ents; }
    void addDimAlign(const DRW_DimAligned*) override { ++ents; }
    void addDimLinear(const DRW_DimLinear*) override { ++ents; }
    void addDimRadial(const DRW_DimRadial*) override { ++ents; }
    void addDimDiametric(const DRW_DimDiametric*) override { ++ents; }
    void addDimAngular(const DRW_DimAngular*) override { ++ents; }
    void addDimAngular3P(const DRW_DimAngular3p*) override { ++ents; }
    void addDimArc(const DRW_DimArc*) override { ++ents; }
    void addDimOrdinate(const DRW_DimOrdinate*) override { ++ents; }
    void addLeader(const DRW_Leader*) override { ++ents; }
    void addHatch(const DRW_Hatch*) override { ++ents; }
    void addViewport(const DRW_Viewport&) override { ++ents; }
    void addImage(const DRW_Image*) override { ++ents; }
    void linkImage(const DRW_ImageDef*) override { ++objs; }
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override { ++objs; }
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

std::string joinMap(const std::unordered_map<std::string, size_t>& m) {
    std::string s;
    for (const auto& kv : m) {
        if (!s.empty()) s += ",";
        s += kv.first + ":" + std::to_string(kv.second);
    }
    return s.empty() ? "-" : s;
}
} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const char* path = argv[i];
        dwgRW r(path);
        CountIface iface;
        bool ok = false;
        try { ok = r.read(&iface, /*ext=*/true); } catch (...) { ok = false; }
        std::printf("%s|ver=%d|ok=%d|err=0x%X|ents=%ld|objs=%ld|skipEnt=[%s]|skipObj=[%s]|entFail=%zu|objFail=%zu|proxyPrim=%zu\n",
                    path, static_cast<int>(r.getVersion()), ok ? 1 : 0,
                    static_cast<unsigned>(r.getError()), iface.ents, iface.objs,
                    joinMap(r.getSkippedCustomClasses()).c_str(),
                    joinMap(r.getSkippedUnsupportedObjects()).c_str(),
                    r.getEntityParseFailures(), r.getObjectParseFailures(),
                    r.getDecodedProxyPrimitives());
        std::fflush(stdout);
    }
    return 0;
}
