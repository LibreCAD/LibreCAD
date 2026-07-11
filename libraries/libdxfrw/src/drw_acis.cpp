/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

// Port of d:/data/dli/dwg-parser/src/dwg/acis/{sab,entities,geometry}.ts.
// Three layers: SAB byte decode -> record graph -> wireframe extraction.

#include "drw_acis.h"

#include <cstring>
#include <exception>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace {

// ── Layer 1: SAB byte reader (sab.ts) ──────────────────────────────────────

const char* const kSignatures[] = { "ACIS BinaryFile", "ASM BinaryFile4" };

bool isEndMarker(const std::string& t) {
    return t == "End-of-ACIS-data" || t == "End-of-ASM-data"
        || t == "End-of-ACIS-History-data";
}

// Thrown internally on any malformed/truncated read; caught at drw_parseSab.
struct SabError : std::exception {};

bool startsWithAscii(const std::uint8_t* data, std::size_t len, const char* prefix) {
    std::size_t plen = std::strlen(prefix);
    if (len < plen) return false;
    for (std::size_t i = 0; i < plen; ++i) {
        if (data[i] != static_cast<std::uint8_t>(prefix[i])) return false;
    }
    return true;
}

class SabByteReader {
public:
    SabByteReader(const std::uint8_t* data, std::size_t len)
        : m_data(data), m_len(len) {}

    std::size_t index = 0;

    bool hasData() const { return index < m_len; }

    std::uint8_t readByte() {
        require(1);
        return m_data[index++];
    }
    int readInt() {
        require(4);
        std::int32_t v;
        std::memcpy(&v, m_data + index, 4);   // little-endian host (MSVC/x64)
        index += 4;
        return static_cast<int>(v);
    }
    double readFloat() {
        require(8);
        double v;
        std::memcpy(&v, m_data + index, 8);
        index += 8;
        return v;
    }
    std::string readStr(std::size_t length) {
        require(length);
        std::string s(reinterpret_cast<const char*>(m_data + index), length);
        index += length;
        return s;
    }
    DRW_Coord readVec() {
        double x = readFloat();
        double y = readFloat();
        double z = readFloat();
        return DRW_Coord(x, y, z);
    }

private:
    const std::uint8_t* m_data;
    std::size_t m_len;
    void require(std::size_t n) {
        if (index + n > m_len) throw SabError();
    }
};

std::string readStrTag(SabByteReader& r) {
    if (r.readByte() != DRW_SabTag::Str) throw SabError();
    return r.readStr(r.readByte());
}
double readDoubleTag(SabByteReader& r) {
    if (r.readByte() != DRW_SabTag::Double) throw SabError();
    return r.readFloat();
}

DRW_SabHeader readHeader(SabByteReader& r, const std::uint8_t* data, std::size_t len) {
    DRW_SabHeader h;
    for (const char* sig : kSignatures) {
        if (startsWithAscii(data, len, sig)) {
            h.signature = sig;
            r.index = std::strlen(sig);
            break;
        }
    }
    if (h.signature.empty()) throw SabError();
    h.version = r.readInt();
    h.numRecords = r.readInt();
    h.numEntities = r.readInt();
    h.flags = r.readInt();
    h.productId = readStrTag(r);
    h.acisVersion = readStrTag(r);
    h.creationDate = readStrTag(r);
    h.unitsInMm = readDoubleTag(r);
    h.resTol = readDoubleTag(r);
    h.norTol = readDoubleTag(r);
    return h;
}

DRW_SabToken tok(int tag) { DRW_SabToken t; t.tag = tag; return t; }

// Read one record up to RECORD_END (or a data-end marker at stream end).
DRW_SabRecord readRecord(SabByteReader& r) {
    DRW_SabRecord out;
    std::vector<std::string> typeParts;
    std::string type;
    int subtypeLevel = 0;

    for (;;) {
        if (!r.hasData()) {
            if (isEndMarker(type)) { out.type = type; return out; }
            throw SabError();
        }
        int tag = r.readByte();
        switch (tag) {
        case DRW_SabTag::Int: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::Double: {
            DRW_SabToken t = tok(tag); t.dval = r.readFloat(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::Str: {
            DRW_SabToken t = tok(tag); t.sval = r.readStr(r.readByte()); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::Pointer: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::BoolTrue: {
            DRW_SabToken t = tok(tag); t.bval = true; out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::BoolFalse: {
            DRW_SabToken t = tok(tag); t.bval = false; out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::LiteralStr: {
            DRW_SabToken t = tok(tag); t.sval = r.readStr(r.readInt()); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::EntityTypeEx: {
            typeParts.push_back(r.readStr(r.readByte()));
            break;
        }
        case DRW_SabTag::EntityType: {
            typeParts.push_back(r.readStr(r.readByte()));
            std::string name;
            for (std::size_t i = 0; i < typeParts.size(); ++i) {
                if (i) name += '-';
                name += typeParts[i];
            }
            typeParts.clear();
            if (type.empty()) type = name;
            DRW_SabToken t = tok(tag); t.sval = name; out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::LocationVec: {
            DRW_SabToken t = tok(tag); t.vec = r.readVec(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::DirectionVec: {
            DRW_SabToken t = tok(tag); t.vec = r.readVec(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::Enum: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::Unknown0x17: {
            DRW_SabToken t = tok(tag); t.dval = r.readFloat(); out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::SubtypeStart: {
            ++subtypeLevel;
            DRW_SabToken t = tok(tag); t.ival = subtypeLevel; out.tokens.push_back(t);
            break;
        }
        case DRW_SabTag::SubtypeEnd: {
            DRW_SabToken t = tok(tag); t.ival = subtypeLevel; out.tokens.push_back(t);
            --subtypeLevel;
            break;
        }
        case DRW_SabTag::RecordEnd:
            out.type = type;
            return out;
        default:
            throw SabError();
        }
    }
}

// ── Layer 3 helpers: graph navigation (geometry.ts) ────────────────────────

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size()
        && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const DRW_AcisNode* nodeAt(const DRW_AcisModel& m, int idx) {
    if (idx < 0 || idx >= static_cast<int>(m.nodes.size())) return nullptr;
    return &m.nodes[idx];
}

// First non-null ref whose type == suffix or endsWith(suffix); -1 if none.
int refOfType(const DRW_AcisModel& m, const DRW_AcisNode& node, const std::string& suffix) {
    for (int ref : node.refs) {
        const DRW_AcisNode* n = nodeAt(m, ref);
        if (n && (n->type == suffix || endsWith(n->type, suffix))) return ref;
    }
    return -1;
}

// All non-null refs whose type == `type` exactly.
std::vector<int> refsOfType(const DRW_AcisModel& m, const DRW_AcisNode& node, const std::string& type) {
    std::vector<int> out;
    for (int ref : node.refs) {
        const DRW_AcisNode* n = nodeAt(m, ref);
        if (n && n->type == type) out.push_back(ref);
    }
    return out;
}

bool vertexPoint(const DRW_AcisModel& m, int vertexIdx, DRW_Coord& out) {
    const DRW_AcisNode* vertex = nodeAt(m, vertexIdx);
    if (!vertex) return false;
    int pointIdx = refOfType(m, *vertex, "point");
    const DRW_AcisNode* point = nodeAt(m, pointIdx);
    if (!point) return false;
    for (const DRW_SabToken& t : point->record.tokens) {
        if (t.tag == DRW_SabTag::LocationVec) { out = t.vec; return true; }
    }
    return false;
}

// Walks a record's tokens in wire order, yielding the next token of a wanted tag.
class TokenCursor {
public:
    explicit TokenCursor(const std::vector<DRW_SabToken>& tokens) : m_tokens(tokens) {}

    bool nextVec(DRW_Coord& out) {
        const DRW_SabToken* t = next2(DRW_SabTag::LocationVec, DRW_SabTag::DirectionVec);
        if (!t) return false;
        out = t->vec;
        return true;
    }
    bool nextDouble(double& out) {
        const DRW_SabToken* t = next1(DRW_SabTag::Double);
        if (!t) return false;
        out = t->dval;
        return true;
    }

private:
    const std::vector<DRW_SabToken>& m_tokens;
    std::size_t m_i = 0;
    const DRW_SabToken* next1(int tag) {
        while (m_i < m_tokens.size()) {
            const DRW_SabToken& t = m_tokens[m_i++];
            if (t.tag == tag) return &t;
        }
        return nullptr;
    }
    const DRW_SabToken* next2(int a, int b) {
        while (m_i < m_tokens.size()) {
            const DRW_SabToken& t = m_tokens[m_i++];
            if (t.tag == a || t.tag == b) return &t;
        }
        return nullptr;
    }
};

// Extract the nubs/nurbs B-spline control polygon from an intcurve's subtype
// tokens (see geometry.ts intcurveControlPoints).
std::vector<DRW_Coord> intcurveControlPoints(const std::vector<DRW_SabToken>& tokens) {
    std::vector<DRW_Coord> out;
    int nubs = -1;
    for (std::size_t k = 0; k < tokens.size(); ++k) {
        if (tokens[k].tag == DRW_SabTag::EntityType
            && (tokens[k].sval == "nubs" || tokens[k].sval == "nurbs")) {
            nubs = static_cast<int>(k);
            break;
        }
    }
    if (nubs < 0) return out;
    std::size_t i = static_cast<std::size_t>(nubs) + 1;
    long long numKnots = -1;
    for (; i < tokens.size(); ++i) {
        const DRW_SabToken& t = tokens[i];
        if (t.tag == DRW_SabTag::Double) break;
        if (t.tag == DRW_SabTag::Int) numKnots = t.ival;
        if (t.tag == DRW_SabTag::EntityType || t.tag == DRW_SabTag::SubtypeEnd) return out;
    }
    if (numKnots < 0) return out;
    for (long long k = 0; k < numKnots; ++k) {
        if (i >= tokens.size() || tokens[i].tag != DRW_SabTag::Double) return out;
        i += 1;
        if (i < tokens.size() && tokens[i].tag == DRW_SabTag::Int) i += 1;
    }
    std::vector<double> coords;
    while (i < tokens.size() && tokens[i].tag == DRW_SabTag::Double) {
        coords.push_back(tokens[i].dval);
        i += 1;
    }
    for (std::size_t k = 0; k + 3 <= coords.size(); k += 3) {
        out.push_back(DRW_Coord(coords[k], coords[k + 1], coords[k + 2]));
    }
    return out;
}

// Fill an edge's analytic curve params from its curve node.
void curveParams(const DRW_AcisNode& curve, DRW_AcisEdge& edge) {
    TokenCursor c(curve.record.tokens);
    if (curve.type == "straight-curve") {
        DRW_Coord origin, direction;
        if (c.nextVec(origin) && c.nextVec(direction)) {
            edge.curveType = DRW_AcisCurve::Straight;
            edge.p0 = origin;
            edge.p1 = direction;
            edge.hasCurve = true;
        }
    } else if (curve.type == "ellipse-curve") {
        DRW_Coord center, normal, majorAxis;
        double ratio;
        if (c.nextVec(center) && c.nextVec(normal) && c.nextVec(majorAxis) && c.nextDouble(ratio)) {
            edge.curveType = DRW_AcisCurve::Ellipse;
            edge.p0 = center;
            edge.p1 = normal;
            edge.p2 = majorAxis;
            edge.ratio = ratio;
            edge.hasCurve = true;
        }
    } else if (curve.type == "intcurve-curve") {
        edge.curveType = DRW_AcisCurve::Intcurve;
        edge.controlPoints = intcurveControlPoints(curve.record.tokens);
        edge.hasCurve = true;
    }
}

// Fill a face's analytic surface params from its surface node.
void surfaceParams(const DRW_AcisNode& surface, DRW_AcisFace& face) {
    TokenCursor c(surface.record.tokens);
    if (surface.type == "plane-surface") {
        DRW_Coord origin, normal, uDir;
        if (c.nextVec(origin) && c.nextVec(normal)) {
            face.p0 = origin;
            face.p1 = normal;
            if (c.nextVec(uDir)) { face.p2 = uDir; face.hasUDir = true; }
            face.hasSurface = true;
        }
    } else if (surface.type == "cone-surface") {
        DRW_Coord origin, axis, majorAxis;
        double ratio, sineAngle, cosineAngle;
        if (c.nextVec(origin) && c.nextVec(axis) && c.nextVec(majorAxis)
            && c.nextDouble(ratio) && c.nextDouble(sineAngle) && c.nextDouble(cosineAngle)) {
            face.p0 = origin;
            face.p1 = axis;
            face.p2 = majorAxis;
            face.ratio = ratio;
            face.sineAngle = sineAngle;
            face.cosineAngle = cosineAngle;
            face.hasSurface = true;
        }
    } else if (surface.type == "torus-surface") {
        DRW_Coord center, axis;
        double majorRadius, minorRadius;
        if (c.nextVec(center) && c.nextVec(axis)
            && c.nextDouble(majorRadius) && c.nextDouble(minorRadius)) {
            face.p0 = center;
            face.p1 = axis;
            face.majorRadius = majorRadius;
            face.minorRadius = minorRadius;
            face.hasSurface = true;
        }
    }
}

DRW_AcisSurface surfaceKind(const std::string& type) {
    if (type == "plane-surface") return DRW_AcisSurface::Plane;
    if (type == "cone-surface") return DRW_AcisSurface::Cone;
    if (type == "torus-surface") return DRW_AcisSurface::Torus;
    if (type == "sphere-surface") return DRW_AcisSurface::Sphere;
    if (type == "spline-surface") return DRW_AcisSurface::Spline;
    return DRW_AcisSurface::Unknown;
}

DRW_AcisCurve curveKind(const std::string& type) {
    if (type == "straight-curve") return DRW_AcisCurve::Straight;
    if (type == "ellipse-curve") return DRW_AcisCurve::Ellipse;
    if (type == "intcurve-curve") return DRW_AcisCurve::Intcurve;
    return DRW_AcisCurve::Unknown;
}

// Count coedges in a loop's ring (next-coedge in wire order), cycle-guarded.
int loopCoedgeCount(const DRW_AcisModel& m, const DRW_AcisNode& loop) {
    int start = refOfType(m, loop, "coedge");
    if (start < 0) return 0;
    std::set<int> seen;
    int coedge = start;
    while (coedge >= 0 && seen.find(coedge) == seen.end()) {
        seen.insert(coedge);
        const DRW_AcisNode* c = nodeAt(m, coedge);
        if (!c) break;
        std::vector<int> nexts = refsOfType(m, *c, "coedge");
        coedge = nexts.empty() ? -1 : nexts[0];
    }
    return static_cast<int>(seen.size());
}

std::string toStr(int v) { return std::to_string(v); }

} // namespace

// ── Layer 1 entry: drw_parseSab ────────────────────────────────────────────

bool drw_parseSab(const std::uint8_t* data, std::size_t length, DRW_SabData& out) {
    if (!data) return false;
    try {
        SabByteReader r(data, length);
        out.header = readHeader(r, data, length);
        out.records.clear();
        while (r.hasData()) {
            DRW_SabRecord record = readRecord(r);
            bool marker = isEndMarker(record.type);
            out.records.push_back(std::move(record));
            if (marker) break;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ── Layer 2 entry: drw_buildAcisModel ──────────────────────────────────────

std::vector<int> DRW_AcisModel::nodesOfType(const std::string& type) const {
    std::vector<int> out;
    for (const DRW_AcisNode& n : nodes) {
        if (isEndMarker(n.type)) continue;
        if (n.type == type) out.push_back(n.index);
    }
    return out;
}

DRW_AcisModel drw_buildAcisModel(const DRW_SabData& sab) {
    DRW_AcisModel model;
    model.header = sab.header;
    model.nodes.resize(sab.records.size());
    for (std::size_t i = 0; i < sab.records.size(); ++i) {
        DRW_AcisNode& node = model.nodes[i];
        node.index = static_cast<int>(i);
        node.type = sab.records[i].type;
        node.record = sab.records[i];
    }
    // Resolve POINTER tokens -> node indices (-1 => null / out of range).
    const int n = static_cast<int>(model.nodes.size());
    for (DRW_AcisNode& node : model.nodes) {
        for (const DRW_SabToken& t : node.record.tokens) {
            if (t.tag != DRW_SabTag::Pointer) continue;
            int target = static_cast<int>(t.ival);
            node.refs.push_back((target >= 0 && target < n) ? target : -1);
        }
    }
    return model;
}

// ── Layer 3 entry: drw_extractAcisWireframe ────────────────────────────────

bool drw_extractAcisWireframe(const DRW_AcisModel& model, DRW_AcisBrep& out) {
    out = DRW_AcisBrep();

    double minX = std::numeric_limits<double>::infinity();
    double minY = minX, minZ = minX;
    double maxX = -minX, maxY = -minX, maxZ = -minX;
    bool grown = false;
    auto grow = [&](const DRW_Coord& p) {
        grown = true;
        if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
        if (p.z < minZ) minZ = p.z; if (p.z > maxZ) maxZ = p.z;
    };

    // Vertices.
    for (int vi : model.nodesOfType("vertex")) {
        const DRW_AcisNode* v = nodeAt(model, vi);
        DRW_AcisVertex vert;
        vert.nodeIndex = vi;
        DRW_Coord p;
        if (v && vertexPoint(model, vi, p)) {
            vert.point = p;
            vert.valid = true;
            grow(p);
        } else {
            out.diagnostics.push_back("vertex#" + toStr(vi) + ": no resolvable point");
        }
        out.vertices.push_back(vert);
    }

    // Edges.
    for (int ei : model.nodesOfType("edge")) {
        const DRW_AcisNode* e = nodeAt(model, ei);
        DRW_AcisEdge edge;
        edge.nodeIndex = ei;
        if (!e) { out.edges.push_back(edge); continue; }
        std::vector<int> verts = refsOfType(model, *e, "vertex");
        DRW_Coord sp, ep;
        if (!verts.empty() && vertexPoint(model, verts[0], sp)) { edge.start = sp; edge.hasStart = true; }
        if (verts.size() > 1 && vertexPoint(model, verts[1], ep)) { edge.end = ep; edge.hasEnd = true; }
        int curveIdx = refOfType(model, *e, "-curve");
        const DRW_AcisNode* curveNode = nodeAt(model, curveIdx);
        if (curveNode) {
            edge.curveType = curveKind(curveNode->type);
            curveParams(*curveNode, edge);
        }
        if (edge.hasStart && edge.hasEnd
            && edge.start.x == edge.end.x && edge.start.y == edge.end.y && edge.start.z == edge.end.z) {
            out.diagnostics.push_back("edge#" + toStr(ei) + ": degenerate (coincident endpoints)");
        }
        out.edges.push_back(std::move(edge));
    }

    // Faces.
    for (int fi : model.nodesOfType("face")) {
        const DRW_AcisNode* f = nodeAt(model, fi);
        DRW_AcisFace face;
        face.nodeIndex = fi;
        if (!f) { out.faces.push_back(face); continue; }
        int surfaceIdx = refOfType(model, *f, "-surface");
        const DRW_AcisNode* surfaceNode = nodeAt(model, surfaceIdx);
        if (surfaceNode) {
            face.surfaceType = surfaceKind(surfaceNode->type);
            surfaceParams(*surfaceNode, face);
        }
        // Walk the next-loop chain (loop-typed ref), cycle-guarded.
        std::set<int> seen;
        int loopIdx = refOfType(model, *f, "loop");
        while (loopIdx >= 0 && seen.find(loopIdx) == seen.end()) {
            seen.insert(loopIdx);
            const DRW_AcisNode* loop = nodeAt(model, loopIdx);
            if (!loop) break;
            DRW_AcisFaceLoop fl;
            fl.nodeIndex = loopIdx;
            fl.coedgeCount = loopCoedgeCount(model, *loop);
            face.loops.push_back(fl);
            loopIdx = refOfType(model, *loop, "loop");
        }
        if (!surfaceNode) {
            out.diagnostics.push_back("face#" + toStr(fi) + ": no surface");
        }
        out.faces.push_back(std::move(face));
    }

    if (grown) {
        out.hasBBox = true;
        out.bboxMin = DRW_Coord(minX, minY, minZ);
        out.bboxMax = DRW_Coord(maxX, maxY, maxZ);
    }
    return true;
}

// ── Convenience entry: drw_decodeAcisWireframe ─────────────────────────────

namespace {

const char* const kSignaturesLocal[] = { "ACIS BinaryFile", "ASM BinaryFile4" };

long long signatureOffset(const std::vector<unsigned char>& data) {
    for (const char* sig : kSignaturesLocal) {
        std::size_t plen = std::strlen(sig);
        if (data.size() < plen) continue;
        for (std::size_t i = 0; i + plen <= data.size(); ++i) {
            bool match = true;
            for (std::size_t j = 0; j < plen; ++j) {
                if (data[i + j] != static_cast<std::uint8_t>(sig[j])) { match = false; break; }
            }
            if (match) return static_cast<long long>(i);
        }
    }
    return -1;
}

} // namespace

LIBDXFRW_EXPORT bool drw_decodeAcisWireframe(const std::vector<unsigned char>& raw, DRW_AcisBrep& out) {
    out = DRW_AcisBrep();
    try {
        long long off = signatureOffset(raw);
        if (off < 0) return false;
        DRW_SabData sab;
        if (!drw_parseSab(raw.data() + off, raw.size() - static_cast<std::size_t>(off), sab)) {
            return false;
        }
        DRW_AcisModel model = drw_buildAcisModel(sab);
        return drw_extractAcisWireframe(model, out);
    } catch (...) {
        out = DRW_AcisBrep();
        return false;
    }
}
