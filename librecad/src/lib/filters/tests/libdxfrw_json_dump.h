#ifndef LIBDXFRW_JSON_DUMP_H
#define LIBDXFRW_JSON_DUMP_H

// P0 read-level canonical JSON dumper for libdxfrw.
// Header-only so BOTH the standalone `libdxfrw_json_dump` binary and the
// Catch2 self-test in dxf_corpus_tests.cpp share one definition.
// Drives dwgRW::read / dxfRW::read straight into JsonDumpIface : DRW_Interface,
// BEFORE any RS_Graphic / RS_FilterDXFRW stage -> isolates READ coverage.
//
// Output is the CANONICAL schema (neutral field names) so the Python differ is
// a plain deep-compare vs the dwgTs adapter (scripts/dwgts_canon.py). Doubles
// use %.17g (round-trip exact); the differ compares floats with --atol.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_interface.h"
#include "libdwgr.h"
#include "libdxfrw.h"

namespace jsondump {

// ----------------------------- minimal JSON writer -------------------------
class JW {
    std::ostream& m_os;
    std::vector<bool> m_first;   // is the next entry the first in the current container?
    bool m_afterKey = false;     // last op was key(): the value must not emit a separator
    void pre() {
        if (m_afterKey) { m_afterKey = false; return; }
        if (!m_first.empty()) { if (!m_first.back()) m_os << ','; m_first.back() = false; }
    }
public:
    explicit JW(std::ostream& os) : m_os(os) {}
    static void writeString(std::ostream& os, const std::string& s) {
        os << '"';
        for (unsigned char c : s) {
            switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n";  break;
            case '\r': os << "\\r";  break;
            case '\t': os << "\\t";  break;
            case '\b': os << "\\b";  break;
            case '\f': os << "\\f";  break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); os << b; }
                else os << static_cast<char>(c);
            }
        }
        os << '"';
    }
    void beginObj() { pre(); m_os << '{'; m_first.push_back(true); }
    void endObj()   { m_os << '}'; if (!m_first.empty()) m_first.pop_back(); }
    void beginArr() { pre(); m_os << '['; m_first.push_back(true); }
    void endArr()   { m_os << ']'; if (!m_first.empty()) m_first.pop_back(); }
    void key(const std::string& k) { pre(); JW::writeString(m_os, k); m_os << ':'; m_afterKey = true; }
    void val(const std::string& s) { pre(); JW::writeString(m_os, s); }
    void val(const char* s)        { val(std::string(s ? s : "")); }
    void val(double d) { pre(); if (std::isfinite(d)) { char b[32]; std::snprintf(b, sizeof b, "%.17g", d); m_os << b; } else m_os << "null"; }
    void val(int i)        { pre(); m_os << i; }
    void val(unsigned u)   { pre(); m_os << u; }
    void val(long long i)  { pre(); m_os << i; }
    void val(bool b)       { pre(); m_os << (b ? "true" : "false"); }
    template <class T> void kv(const std::string& k, T v) { key(k); val(v); }
    void kv(const std::string& k, const char* v) { key(k); val(v); }
    void kcoord(const std::string& k, const DRW_Coord& c) { key(k); beginArr(); val(c.x); val(c.y); val(c.z); endArr(); }
};

inline std::string hexHandle(std::uint32_t h) { char b[16]; std::snprintf(b, sizeof b, "%X", h); return std::string(b); }

inline const char* versionStr(DRW::Version v) {
    switch (v) {
    case DRW::AC1006: return "AC1006"; case DRW::AC1009: return "AC1009";
    case DRW::AC1012: return "AC1012"; case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015"; case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021"; case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027"; case DRW::AC1032: return "AC1032";
    default: return "UNKNOWN";
    }
}

// ------------------------------- the dumper --------------------------------
class JsonDumpIface : public DRW_Interface {
    std::ostringstream m_entOs, m_objOs;
    JW m_ent{m_entOs};
    JW m_obj{m_objOs};
    std::string m_curBlock;

    void dumpCommon(const DRW_Entity& e, const char* type) {
        m_ent.kv("handle", hexHandle(e.handle));
        m_ent.kv("type", type);
        m_ent.kv("ownerHandle", hexHandle(e.parentHandle));
        m_ent.kv("layer", e.layer);
        m_ent.kv("space", static_cast<int>(e.space));
        m_ent.kv("linetype", e.lineType);
        m_ent.kv("color", e.color);
        m_ent.kv("color24", e.color24);
        m_ent.kv("ltypeScale", e.ltypeScale);
        m_ent.kv("visible", e.visible);
        if (!m_curBlock.empty()) m_ent.kv("block", m_curBlock);
    }
    void dumpDim(const DRW_Dimension& d, const char* subtype) {
        dumpCommon(d, "DIMENSION");
        m_ent.kv("dimType", subtype);
        m_ent.kcoord("defPoint", d.getDefPoint());
        m_ent.kcoord("textMidPt", d.getTextPoint());
        m_ent.kv("dimStyle", d.getStyle());
        m_ent.kv("dimText", d.getText());
    }
public:
    std::string m_file, m_sourceFormat = "dwg", m_version = "UNKNOWN";
    long m_entFail = 0, m_objFail = 0, m_proxyPrims = 0;

    JsonDumpIface() { m_ent.beginArr(); m_obj.beginArr(); }

    void writeTo(std::ostream& out) {
        m_ent.endArr(); m_obj.endArr();
        out << '{';
        out << "\"file\":";         JW::writeString(out, m_file);         out << ',';
        out << "\"sourceFormat\":"; JW::writeString(out, m_sourceFormat); out << ',';
        out << "\"version\":";      JW::writeString(out, m_version);      out << ',';
        out << "\"diagnostics\":{"
            << "\"entityParseFailures\":"    << m_entFail    << ','
            << "\"objectParseFailures\":"    << m_objFail    << ','
            << "\"decodedProxyPrimitives\":" << m_proxyPrims << "},";
        out << "\"entities\":" << m_entOs.str() << ',';
        out << "\"objects\":"  << m_objOs.str();
        out << "}\n";
    }

    // ---- block context ----
    // MUST-FIX #4: setBlock signals a block switch; clear the tracked name so
    // subsequent entities are not stamped with a stale block until the next
    // addBlock. (Model space -> handle 0 sends setBlock(-1); block boundaries
    // send setBlock(handle) between addBlock/endBlock pairs.)
    void addBlock(const DRW_Block& b) override { m_curBlock = b.name; }
    void setBlock(const int) override { m_curBlock.clear(); }
    void endBlock() override { m_curBlock.clear(); }

    // ---- L1 core entity types ----
    void addPoint(const DRW_Point& e) override {
        m_ent.beginObj(); dumpCommon(e, "POINT");
        m_ent.kcoord("position", e.basePoint); m_ent.kv("thickness", e.thickness);
        m_ent.kcoord("extrusion", e.extPoint); m_ent.endObj();
    }
    void addLine(const DRW_Line& e) override {
        m_ent.beginObj(); dumpCommon(e, "LINE");
        m_ent.kcoord("start", e.basePoint); m_ent.kcoord("end", e.secPoint);
        m_ent.kv("thickness", e.thickness); m_ent.kcoord("extrusion", e.extPoint); m_ent.endObj();
    }
    void addRay(const DRW_Ray& e) override {
        m_ent.beginObj(); dumpCommon(e, "RAY");
        m_ent.kcoord("start", e.basePoint); m_ent.kcoord("dir", e.secPoint); m_ent.endObj();
    }
    void addXline(const DRW_Xline& e) override {
        m_ent.beginObj(); dumpCommon(e, "XLINE");
        m_ent.kcoord("start", e.basePoint); m_ent.kcoord("dir", e.secPoint); m_ent.endObj();
    }
    void addArc(const DRW_Arc& e) override {
        m_ent.beginObj(); dumpCommon(e, "ARC");
        m_ent.kcoord("center", e.basePoint); m_ent.kv("radius", e.radious);
        m_ent.kv("startAngle", e.staangle); m_ent.kv("endAngle", e.endangle);
        m_ent.kcoord("extrusion", e.extPoint); m_ent.endObj();
    }
    void addCircle(const DRW_Circle& e) override {
        m_ent.beginObj(); dumpCommon(e, "CIRCLE");
        m_ent.kcoord("center", e.basePoint); m_ent.kv("radius", e.radious);
        m_ent.kcoord("extrusion", e.extPoint); m_ent.endObj();
    }
    void addEllipse(const DRW_Ellipse& e) override {
        m_ent.beginObj(); dumpCommon(e, "ELLIPSE");
        m_ent.kcoord("center", e.basePoint); m_ent.kcoord("majorAxis", e.secPoint);
        m_ent.kv("ratio", e.ratio); m_ent.kv("startParam", e.staparam); m_ent.kv("endParam", e.endparam);
        m_ent.endObj();
    }
    void addText(const DRW_Text& e) override {
        m_ent.beginObj(); dumpCommon(e, "TEXT");
        m_ent.kcoord("insertionPt", e.basePoint); m_ent.kcoord("alignPt", e.secPoint);
        m_ent.kv("height", e.height); m_ent.kv("text", e.text); m_ent.kv("rotation", e.angle);
        m_ent.kv("style", e.style); m_ent.kv("widthScale", e.widthscale); m_ent.kv("oblique", e.oblique);
        m_ent.kv("textgen", e.textgen); m_ent.kv("alignH", static_cast<int>(e.alignH));
        m_ent.kv("alignV", static_cast<int>(e.alignV)); m_ent.endObj();
    }
    void addMText(const DRW_MText& e) override {
        m_ent.beginObj(); dumpCommon(e, "MTEXT");
        m_ent.kcoord("insertionPt", e.basePoint); m_ent.kv("height", e.height); m_ent.kv("text", e.text);
        m_ent.kv("rotation", e.angle); m_ent.kv("width", e.widthscale);
        m_ent.kv("attachmentPoint", e.textgen); m_ent.kv("style", e.style);
        m_ent.kv("lineSpacingFactor", e.interlin); m_ent.endObj();
    }
    void addInsert(const DRW_Insert& e) override {
        m_ent.beginObj(); dumpCommon(e, "INSERT");
        m_ent.kv("name", e.name); m_ent.kcoord("insertionPt", e.basePoint);
        m_ent.kv("xScale", e.xscale); m_ent.kv("yScale", e.yscale); m_ent.kv("zScale", e.zscale);
        m_ent.kv("rotation", e.angle); m_ent.kv("colCount", e.colcount); m_ent.kv("rowCount", e.rowcount);
        m_ent.kv("colSpacing", e.colspace); m_ent.kv("rowSpacing", e.rowspace); m_ent.endObj();
    }
    void addLWPolyline(const DRW_LWPolyline& e) override {
        m_ent.beginObj(); dumpCommon(e, "LWPOLYLINE");
        m_ent.kv("flags", e.flags); m_ent.kv("constWidth", e.width);
        m_ent.kv("elevation", e.elevation); m_ent.kv("thickness", e.thickness);
        m_ent.kcoord("extrusion", e.extPoint);
        m_ent.key("vertices"); m_ent.beginArr();
        for (const auto& v : e.vertlist) { if (!v) continue;
            m_ent.beginObj(); m_ent.kv("x", v->x); m_ent.kv("y", v->y);
            m_ent.kv("startWidth", v->stawidth); m_ent.kv("endWidth", v->endwidth);
            m_ent.kv("bulge", v->bulge); m_ent.endObj(); }
        m_ent.endArr(); m_ent.endObj();
    }
    void addPolyline(const DRW_Polyline& e) override {
        m_ent.beginObj(); dumpCommon(e, "POLYLINE");
        m_ent.kv("flags", e.flags); m_ent.kv("elevation", e.basePoint.z);
        m_ent.key("vertices"); m_ent.beginArr();
        for (const auto& v : e.vertlist) { if (!v) continue;
            m_ent.beginObj(); m_ent.kcoord("position", v->basePoint);
            m_ent.kv("startWidth", v->stawidth); m_ent.kv("endWidth", v->endwidth);
            m_ent.kv("bulge", v->bulge); m_ent.endObj(); }
        m_ent.endArr(); m_ent.endObj();
    }
    void addSpline(const DRW_Spline* e) override {
        if (!e) return;
        m_ent.beginObj(); dumpCommon(*e, "SPLINE");
        m_ent.kv("degree", e->degree); m_ent.kv("flags", e->flags);
        m_ent.kv("nControl", e->ncontrol); m_ent.kv("nFit", e->nfit); m_ent.kv("nKnot", e->nknots);
        m_ent.key("controlPoints"); m_ent.beginArr();
        for (const auto& p : e->controllist) { if (p) { m_ent.beginArr(); m_ent.val(p->x); m_ent.val(p->y); m_ent.val(p->z); m_ent.endArr(); } }
        m_ent.endArr();
        m_ent.key("fitPoints"); m_ent.beginArr();
        for (const auto& p : e->fitlist) { if (p) { m_ent.beginArr(); m_ent.val(p->x); m_ent.val(p->y); m_ent.val(p->z); m_ent.endArr(); } }
        m_ent.endArr();
        m_ent.key("knots"); m_ent.beginArr();
        for (double k : e->knotslist) m_ent.val(k);
        m_ent.endArr(); m_ent.endObj();
    }
    void addKnot(const DRW_Entity&) override {}
    void addTrace(const DRW_Trace& e) override {
        m_ent.beginObj(); dumpCommon(e, "TRACE");
        m_ent.kcoord("p1", e.basePoint); m_ent.kcoord("p2", e.secPoint);
        m_ent.kcoord("p3", e.thirdPoint); m_ent.kcoord("p4", e.fourPoint); m_ent.endObj();
    }
    void add3dFace(const DRW_3Dface& e) override {
        m_ent.beginObj(); dumpCommon(e, "3DFACE");
        m_ent.kcoord("p1", e.basePoint); m_ent.kcoord("p2", e.secPoint);
        m_ent.kcoord("p3", e.thirdPoint); m_ent.kcoord("p4", e.fourPoint); m_ent.endObj();
    }
    void addSolid(const DRW_Solid& e) override {
        m_ent.beginObj(); dumpCommon(e, "SOLID");
        m_ent.kcoord("p1", e.basePoint); m_ent.kcoord("p2", e.secPoint);
        m_ent.kcoord("p3", e.thirdPoint); m_ent.kcoord("p4", e.fourPoint); m_ent.endObj();
    }
    void addHatch(const DRW_Hatch* e) override {
        if (!e) return;
        m_ent.beginObj(); dumpCommon(*e, "HATCH");
        m_ent.kv("name", e->name); m_ent.kv("solid", e->solid); m_ent.kv("associative", e->associative);
        m_ent.kv("hstyle", e->hstyle); m_ent.kv("hpattern", e->hpattern);
        m_ent.kv("angle", e->angle); m_ent.kv("scale", e->scale); m_ent.kv("loopsnum", e->loopsnum);
        m_ent.endObj();
    }
    void addViewport(const DRW_Viewport& e) override {   // acid-test type (filter no-op today)
        m_ent.beginObj(); dumpCommon(e, "VIEWPORT");
        m_ent.kcoord("center", e.basePoint); m_ent.kv("width", e.pswidth); m_ent.kv("height", e.psheight);
        m_ent.kv("centerPX", e.centerPX); m_ent.kv("centerPY", e.centerPY);
        m_ent.kcoord("viewTarget", e.viewTarget); m_ent.kv("viewHeight", e.viewHeight);
        m_ent.kv("status", e.vpstatus); m_ent.kv("id", e.vpID); m_ent.endObj();
    }
    void addImage(const DRW_Image* e) override {
        if (!e) return;
        m_ent.beginObj(); dumpCommon(*e, "IMAGE");
        m_ent.kv("defHandle", hexHandle(e->ref)); m_ent.kcoord("insertionPt", e->basePoint);
        m_ent.kcoord("uVector", e->secPoint); m_ent.kcoord("vVector", e->vVector);
        m_ent.kv("sizeU", e->sizeu); m_ent.kv("sizeV", e->sizev); m_ent.kv("clip", e->clip); m_ent.endObj();
    }
    void addWipeout(const DRW_Wipeout* e) override {
        if (!e) return;
        m_ent.beginObj(); dumpCommon(*e, "WIPEOUT");
        m_ent.kv("defHandle", hexHandle(e->ref)); m_ent.kcoord("insertionPt", e->basePoint);
        m_ent.kv("clip", e->clip); m_ent.endObj();
    }
    void linkImage(const DRW_ImageDef*) override {}

    // ---- dimensions (8 subtypes -> one canonical "DIMENSION") ----
    void addDimAlign(const DRW_DimAligned* e) override      { if (e) { m_ent.beginObj(); dumpDim(*e, "ALIGNED");   m_ent.endObj(); } }
    void addDimLinear(const DRW_DimLinear* e) override      { if (e) { m_ent.beginObj(); dumpDim(*e, "LINEAR");    m_ent.endObj(); } }
    void addDimRadial(const DRW_DimRadial* e) override      { if (e) { m_ent.beginObj(); dumpDim(*e, "RADIAL");    m_ent.endObj(); } }
    void addDimDiametric(const DRW_DimDiametric* e) override{ if (e) { m_ent.beginObj(); dumpDim(*e, "DIAMETRIC"); m_ent.endObj(); } }
    void addDimAngular(const DRW_DimAngular* e) override    { if (e) { m_ent.beginObj(); dumpDim(*e, "ANGULAR");   m_ent.endObj(); } }
    void addDimAngular3P(const DRW_DimAngular3p* e) override{ if (e) { m_ent.beginObj(); dumpDim(*e, "ANGULAR3P"); m_ent.endObj(); } }
    void addDimOrdinate(const DRW_DimOrdinate* e) override  { if (e) { m_ent.beginObj(); dumpDim(*e, "ORDINATE");  m_ent.endObj(); } }
    void addDimArc(const DRW_DimArc* e) override            { if (e) { m_ent.beginObj(); dumpDim(*e, "ARC");       m_ent.endObj(); } }
    void addLeader(const DRW_Leader* e) override {
        if (!e) return; m_ent.beginObj(); dumpCommon(*e, "LEADER"); m_ent.endObj();
    }

    // ---- optional starter overrides (types with default no-op) ----
    void addTable(const DRW_Table& e) override { addInsert(e); }   // renders as INSERT; label kept for later phase
    void addMesh(const DRW_Mesh& e) override   { m_ent.beginObj(); dumpCommon(e, "MESH"); m_ent.endObj(); }
    // MUST-FIX #1: DRW_Shape derives from DRW_Entity (NOT DRW_Point), so it has
    // no `basePoint`. Use m_insertionPoint (drw_entities.h:664).
    void addShape(const DRW_Shape& e) override {
        m_ent.beginObj(); dumpCommon(e, "SHAPE");
        m_ent.kcoord("insertionPt", e.m_insertionPoint);
        m_ent.kv("shapeIndex", e.m_shapeIndex);
        m_ent.kv("styleName", e.m_styleName);
        m_ent.endObj();
    }
    void addOle2Frame(const DRW_Ole2Frame& e) override { m_ent.beginObj(); dumpCommon(e, "OLE2FRAME"); m_ent.endObj(); }
    void addLight(const DRW_Light& e) override { m_ent.beginObj(); dumpCommon(e, "LIGHT"); m_ent.endObj(); }
    void addMLine(const DRW_MLine* e) override { if (e) { m_ent.beginObj(); dumpCommon(*e, "MLINE"); m_ent.endObj(); } }
    // MUST-FIX #3: emit the common header (handle/type/layer/...) not a bare
    // {"type":"MLEADER"} that would fail every differ handle-match.
    void addMLeader(const DRW_MLeader* e) override {
        if (!e) return; m_ent.beginObj(); dumpCommon(*e, "MLEADER"); m_ent.endObj();
    }

    // ---- objects: the raw-shelf acid test (VIEWPORT-has-neither vs LIGHT-has-both) ----
    void addUnsupportedObject(const DRW_UnsupportedObject& e) override {
        m_obj.beginObj();
        m_obj.kv("handle", hexHandle(e.m_handle));
        m_obj.kv("type", "RAW");
        m_obj.kv("objectType", e.m_objectType);
        m_obj.kv("recordName", e.m_recordName);
        m_obj.kv("className", e.m_className);
        m_obj.kv("isEntity", e.m_isEntity);
        m_obj.kv("isCustomClass", e.m_isCustomClass);
        m_obj.kv("byteLen", static_cast<long long>(e.m_rawBytes.size()));
        m_obj.endObj();
    }

    // ---- remaining pure-virtuals: no-op (tables/header not in the P0 diff schema) ----
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}

    // ---- write-side pure-virtuals: never invoked on read; empty ----
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

// ------------------------------ drivers ------------------------------------
// Branch on MAGIC BYTES, not extension: some corpora mislabel DXF as .dwg
// (dwg_tarch_standalone.cpp:436 sidesteps this by skipping '#'-prefixed files).
inline int dumpCadFile(const std::string& path, std::ostream& out) {
    char magic[8] = {0};
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) { out << "{\"file\":"; JW::writeString(out, path); out << ",\"error\":\"BAD_OPEN\"}\n"; return 2; }
        f.read(magic, 6);
    }
    const bool isDwg = (magic[0] == 'A' && magic[1] == 'C') || (magic[0] == 'M' && magic[1] == 'C');
    JsonDumpIface iface; iface.m_file = path;
    if (isDwg) {
        iface.m_sourceFormat = "dwg";
        dwgRW r(path.c_str());
        const bool ok = r.read(&iface, true); (void)ok;
        iface.m_version    = versionStr(r.getVersion());
        iface.m_entFail    = static_cast<long>(r.getEntityParseFailures());
        iface.m_objFail    = static_cast<long>(r.getObjectParseFailures());
        iface.m_proxyPrims = static_cast<long>(r.getDecodedProxyPrimitives());
    } else {
        iface.m_sourceFormat = "dxf";
        dxfRW r(path.c_str());
        const bool ok = r.read(&iface, true); (void)ok;
        iface.m_version = versionStr(r.getVersion());
    }
    iface.writeTo(out);
    return 0;
}

} // namespace jsondump

#endif // LIBDXFRW_JSON_DUMP_H
