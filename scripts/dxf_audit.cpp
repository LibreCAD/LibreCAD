// DXF read-coverage audit harness (build-and-run, not part of the test suite).
// Reads each .dxf argument through libdxfrw's dxfRW (ASCII + binary) and prints
// one pipe-separated line per file: read result + the "unmodeled type" signal
// (the raw-net: addRawDxfEntity / addRawDxfObject deliver any entity/object type
// libdxfrw has no typed parser for — that IS the DXF coverage-gap signal).
//
// Build (from repo root):
//   clang++ -std=c++17 -O1 -I libraries/libdxfrw/src \
//     scripts/dxf_audit.cpp \
//     libraries/libdxfrw/src/*.cpp libraries/libdxfrw/src/intern/*.cpp \
//     -o /tmp/dxf_audit
//   /tmp/dxf_audit <files...>
#include <cstdio>
#include <map>
#include <string>

#include "drw_interface.h"
#include "drw_objects.h"
#include "libdxfrw.h"

namespace {
class CountIface : public DRW_Interface {
public:
    long m_ents = 0, m_objs = 0;
    std::map<std::string, int> m_rawEnt;  // unmodeled ENTITIES (by type name)
    std::map<std::string, int> m_rawObj;  // unmodeled OBJECTS (by type name)
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
    void addPoint(const DRW_Point&) override { ++m_ents; }
    void addLine(const DRW_Line&) override { ++m_ents; }
    void addRay(const DRW_Ray&) override { ++m_ents; }
    void addXline(const DRW_Xline&) override { ++m_ents; }
    void addCircle(const DRW_Circle&) override { ++m_ents; }
    void addArc(const DRW_Arc&) override { ++m_ents; }
    void addEllipse(const DRW_Ellipse&) override { ++m_ents; }
    void addLWPolyline(const DRW_LWPolyline&) override { ++m_ents; }
    void addPolyline(const DRW_Polyline&) override { ++m_ents; }
    void addSpline(const DRW_Spline*) override { ++m_ents; }
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override { ++m_ents; }
    void addTrace(const DRW_Trace&) override { ++m_ents; }
    void add3dFace(const DRW_3Dface&) override { ++m_ents; }
    void addSolid(const DRW_Solid&) override { ++m_ents; }
    void addMText(const DRW_MText&) override { ++m_ents; }
    void addText(const DRW_Text&) override { ++m_ents; }
    void addDimAlign(const DRW_DimAligned*) override { ++m_ents; }
    void addDimLinear(const DRW_DimLinear*) override { ++m_ents; }
    void addDimRadial(const DRW_DimRadial*) override { ++m_ents; }
    void addDimDiametric(const DRW_DimDiametric*) override { ++m_ents; }
    void addDimAngular(const DRW_DimAngular*) override { ++m_ents; }
    void addDimAngular3P(const DRW_DimAngular3p*) override { ++m_ents; }
    void addDimArc(const DRW_DimArc*) override { ++m_ents; }
    void addDimOrdinate(const DRW_DimOrdinate*) override { ++m_ents; }
    void addLeader(const DRW_Leader*) override { ++m_ents; }
    void addHatch(const DRW_Hatch*) override { ++m_ents; }
    void addViewport(const DRW_Viewport&) override { ++m_ents; }
    void addImage(const DRW_Image*) override { ++m_ents; }
    void linkImage(const DRW_ImageDef*) override { ++m_objs; }
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override { ++m_objs; }
    void addRawDxfEntity(const DRW_RawDxfObject& d) override { ++m_rawEnt[d.name]; }
    void addRawDxfObject(const DRW_RawDxfObject& d) override { ++m_rawObj[d.name]; }
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

std::string joinMap(const std::map<std::string, int>& m) {
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
        dxfRW r(path);
        CountIface iface;
        bool ok = false;
        try { ok = r.read(&iface, /*ext=*/true); } catch (...) { ok = false; }
        std::printf("%s|ok=%d|err=0x%X|ents=%ld|objs=%ld|rawEnt=[%s]|rawObj=[%s]\n",
                    path, ok ? 1 : 0, static_cast<unsigned>(r.getError()),
                    iface.m_ents, iface.m_objs,
                    joinMap(iface.m_rawEnt).c_str(), joinMap(iface.m_rawObj).c_str());
        std::fflush(stdout);
    }
    return 0;
}
