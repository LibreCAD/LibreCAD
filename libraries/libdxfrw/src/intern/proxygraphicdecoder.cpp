#include "proxygraphicdecoder.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../drw_entities.h"
#include "../drw_interface.h"
#include "drw_dbg.h"
#include "dwgbuffer.h"

namespace {

constexpr double DEG_PER_RAD = 57.29577951308232087680; // 180/pi

// ezdxf ProxyGraphicTypes (proxygraphic.py:208). Only the opcodes we decode
// are named; every other opcode (matrices, mesh, shell, extents, clip, …) is
// skipped uniformly by the framing loop via its self-describing chunk size.
enum OpCode {
    OP_CIRCLE               = 2,
    OP_CIRCULAR_ARC         = 4,
    OP_POLYLINE             = 6,
    OP_POLYGON              = 7,
    OP_TEXT                 = 10,
    OP_ATTRIBUTE_COLOR      = 14,
    OP_ATTRIBUTE_TRUE_COLOR = 22,
    OP_POLYLINE_NORMALS     = 32,
    OP_LWPOLYLINE           = 33,
    OP_ELLIPTIC_ARC         = 44,
};

// Byte-aligned little-endian cursor with 4-byte struct alignment, mirroring
// ezdxf ByteStream (tools/binarydata.py).  Bounds-checked: any over-read sets
// `bad` and yields zeros so the caller aborts the current opcode.  Host is
// assumed little-endian (x86_64 / arm64 — the only libdxfrw targets).
class ByteStream {
public:
    ByteStream(const std::uint8_t *p, std::size_t n) : m_p(p), m_n(n) {}

    bool bad = false;
    bool hasData() const { return m_i < m_n; }

    std::uint32_t readLong() {
        std::uint32_t v = 0;
        take(4, &v);
        return v;
    }
    double readDouble() {
        double v = 0.0;
        take(8, &v);
        return v;
    }
    DRW_Coord readVertex() {
        DRW_Coord c;
        c.x = readDouble();
        c.y = readDouble();
        c.z = readDouble();
        return c;
    }
    // PS: a NUL-terminated string, then align the cursor to 4 bytes.
    std::string readPaddedString() {
        std::string s;
        std::size_t e = m_i;
        while (e < m_n && m_p[e] != 0) ++e;
        if (e >= m_n) { bad = true; return s; }
        s.assign(reinterpret_cast<const char *>(m_p + m_i), e - m_i);
        m_i = align4(e + 1);
        return s;
    }

private:
    bool take(std::size_t bytes, void *out) {
        if (m_i + bytes > m_n) {
            bad = true;
            std::memset(out, 0, bytes);
            return false;
        }
        std::memcpy(out, m_p + m_i, bytes);
        m_i = align4(m_i + bytes);
        return true;
    }
    std::size_t align4(std::size_t i) const {
        std::size_t r = i % 4;
        return r ? i + 4 - r : i;
    }

    const std::uint8_t *m_p;
    std::size_t m_n;
    std::size_t m_i = 0;
};

// Accumulated draw-state mutated by the ATTRIBUTE_* opcodes
// (proxygraphic.py:353-406).  Only colour is rendered; layer / linetype index
// opcodes are ignored (we have no per-doc index→name table here).
struct DrawState {
    int color = DRW::ColorByLayer; // 256 == BYLAYER
    int trueColor = -1;            // 24-bit RGB, -1 == unset
};

// Apply the owning entity's identity plus the accumulated colour state onto a
// freshly decoded primitive so it lands in the right container and renders with
// the proxy's colour.  Coordinates are emitted verbatim (WCS) with a Z
// extrusion: proxy graphics is overwhelmingly planar, and forcing extPoint=Z
// avoids a double OCS transform downstream.
void applyAttribs(DRW_Entity &e, const DRW_Entity &parent, const DrawState &st) {
    e.space = parent.space;
    e.layer = parent.layer;
    e.lineType = parent.lineType;
    e.handle = parent.handle;
    e.parentHandle = parent.parentHandle;
    e.lWeight = parent.lWeight;
    e.color = (st.color >= 0 && st.color != DRW::ColorByLayer) ? st.color : parent.color;
    if (st.trueColor >= 0)
        e.color24 = st.trueColor;
}

// --- opcode 2: CIRCLE (proxygraphic.py:408) ---
void doCircle(const std::uint8_t *p, std::size_t n, DRW_Interface &iface,
              const DRW_Entity &parent, const DrawState &st, int &count) {
    ByteStream bs(p, n);
    DRW_Circle e;
    e.basePoint = bs.readVertex();
    e.radious = bs.readDouble();
    // normal vector follows (3 doubles) — read to advance but ignore (planar).
    bs.readVertex();
    if (bs.bad) return;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    iface.addCircle(e);
    ++count;
}

// --- opcode 4: CIRCULAR_ARC (proxygraphic.py:436) ---
void doArc(const std::uint8_t *p, std::size_t n, DRW_Interface &iface,
           const DRW_Entity &parent, const DrawState &st, int &count) {
    ByteStream bs(p, n);
    DRW_Arc e;
    e.basePoint = bs.readVertex();      // center (WCS)
    e.radious = bs.readDouble();
    bs.readVertex();                    // normal (ignored — planar)
    DRW_Coord start = bs.readVertex();  // UCS x-axis
    double sweep = bs.readDouble();     // radians
    if (bs.bad) return;
    // Planar case: start angle from the start vector, end = start + sweep.
    e.staangle = std::atan2(start.y, start.x); // libdxfrw stores radians
    e.endangle = e.staangle + sweep;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    iface.addArc(e);
    ++count;
}

// --- opcode 44: ELLIPTIC_ARC (proxygraphic.py:484) ---
void doEllipse(const std::uint8_t *p, std::size_t n, DRW_Interface &iface,
               const DRW_Entity &parent, const DrawState &st, int &count) {
    ByteStream bs(p, n);
    DRW_Ellipse e;
    e.basePoint = bs.readVertex();   // center
    bs.readVertex();                 // extrusion (ignored — planar)
    double majorLen = bs.readDouble();
    double minorLen = bs.readDouble();
    double startParam = bs.readDouble();
    double endParam = bs.readDouble();
    double majorAngle = bs.readDouble();
    if (bs.bad || majorLen == 0.0) return;
    e.ratio = minorLen / majorLen;
    e.secPoint = DRW_Coord(std::cos(majorAngle) * majorLen,
                           std::sin(majorAngle) * majorLen, 0.0); // major axis vector
    e.staparam = startParam;
    e.endparam = endParam;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    iface.addEllipse(e);
    ++count;
}

// --- opcodes 6 / 7 / 32: POLYLINE / POLYGON (proxygraphic.py:536) ---
// _load_vertices (proxygraphic.py:849): L count [+1 if load_normal] then
// count×3d.  A POLYGON (7) is a closed POLYLINE.
void doPolyline(const std::uint8_t *p, std::size_t n, bool closed, bool loadNormal,
                DRW_Interface &iface, const DRW_Entity &parent, const DrawState &st,
                int &count) {
    ByteStream bs(p, n);
    std::uint32_t vcount = bs.readLong();
    if (loadNormal) vcount += 1; // last vertex is the normal
    if (vcount == 0 || vcount > 1000000u) return;
    std::vector<DRW_Coord> verts;
    verts.reserve(vcount);
    for (std::uint32_t i = 0; i < vcount && !bs.bad; ++i)
        verts.push_back(bs.readVertex());
    if (bs.bad) return;
    if (loadNormal && !verts.empty()) verts.pop_back(); // drop the normal
    if (verts.size() < 2) return;

    DRW_Polyline e;
    bool is3d = false;
    for (const DRW_Coord &v : verts)
        if (std::fabs(v.z) > 1e-9) { is3d = true; break; }
    e.flags = (closed ? 1 : 0) | (is3d ? 8 : 0);
    for (const DRW_Coord &v : verts)
        e.addVertex(DRW_Vertex(v.x, v.y, v.z, 0.0));
    applyAttribs(e, parent, st);
    iface.addPolyline(e);
    ++count;
}

// --- opcode 10: TEXT (proxygraphic.py:694, non-unicode) ---
void doText(const std::uint8_t *p, std::size_t n, DRW_Interface &iface,
            const DRW_Entity &parent, const DrawState &st, int &count) {
    ByteStream bs(p, n);
    DRW_Text e;
    e.basePoint = bs.readVertex();          // start point
    bs.readVertex();                        // normal (ignored)
    DRW_Coord dir = bs.readVertex();        // text direction
    e.height = bs.readDouble();
    e.widthscale = bs.readDouble();
    e.oblique = bs.readDouble() * DEG_PER_RAD;
    std::string text = bs.readPaddedString();
    if (bs.bad || text.empty()) return;
    e.text = text;
    e.angle = std::atan2(dir.y, dir.x) * DEG_PER_RAD;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    iface.addText(e);
    ++count;
}

// --- opcode 33: LWPOLYLINE (proxygraphic.py:549) — an ODA *bit* stream ---
void doLwpolyline(const std::uint8_t *p, std::size_t n, DRW::Version version,
                  DRW_Interface &iface, const DRW_Entity &parent,
                  const DrawState &st, int &count) {
    std::vector<std::uint8_t> buf(p, p + n);
    dwgBuffer bs(buf.data(), buf.size(), nullptr); // no text → no decoder needed
    bs.getRawLong32();                              // num_data_bytes (RL)
    std::int32_t flag = bs.getBitShort();           // BS
    DRW_LWPolyline e;
    if (flag & 4) e.width = bs.getBitDouble();          // const width
    if (flag & 8) e.elevation = bs.getBitDouble();
    if (flag & 2) e.thickness = bs.getBitDouble();
    if (flag & 1) { bs.getBitDouble(); bs.getBitDouble(); bs.getBitDouble(); } // extrusion 3BD
    bool isClosed = (flag & 512) != 0;
    std::int32_t numPoints = bs.getBitLong();
    if (numPoints <= 0 || numPoints > 1000000) return;
    std::int32_t numBulges = 0, numVertexIds = 0, numWidth = 0;
    if (flag & 16) numBulges = bs.getBitLong();
    if (version >= DRW::AC1024) {
        if (flag & 1024) numVertexIds = bs.getBitLong();
        if (flag & 32)   numWidth     = bs.getBitLong();
    }
    (void)numBulges; (void)numVertexIds; (void)numWidth;

    double px = bs.getRawDouble();
    double py = bs.getRawDouble();
    e.addVertex(DRW_Vertex2D(px, py, 0.0));
    for (std::int32_t i = 1; i < numPoints; ++i) {
        px = bs.getDefaultDouble(px);
        py = bs.getDefaultDouble(py);
        e.addVertex(DRW_Vertex2D(px, py, 0.0));
    }
    if (!bs.isGood()) return; // bit stream overran — drop
    e.vertexnum = numPoints;
    e.flags = isClosed ? 1 : 0;
    applyAttribs(e, parent, st);
    iface.addLWPolyline(e);
    ++count;
}

} // namespace

int DRW_ProxyGraphicDecoder::decode(const std::string &bytes, DRW::Version version,
                                  DRW_Interface &iface, const DRW_Entity &parent) {
    const std::size_t len = bytes.size();
    if (len < 16) return 0; // 8-byte header + at least one 8-byte chunk header
    const std::uint8_t *data = reinterpret_cast<const std::uint8_t *>(bytes.data());

    // Framing (proxygraphic.py:315): the first 8 bytes are a header; chunks are
    // [<u32 size><u32 type><payload>] with payload = data[index+8 : index+size].
    std::size_t index = 8;
    int count = 0;
    DrawState st;
    int guard = 0;
    while (index + 8 <= len && guard++ < 1000000) {
        std::uint32_t size, type;
        std::memcpy(&size, data + index, 4);
        std::memcpy(&type, data + index + 4, 4);
        if (size < 8) break;                       // cannot advance
        std::size_t payEnd = index + size;
        if (payEnd > len) payEnd = len;            // clamp a ragged tail
        const std::uint8_t *pay = data + index + 8;
        std::size_t payLen = (payEnd > index + 8) ? payEnd - (index + 8) : 0;

        switch (type) {
        case OP_ATTRIBUTE_COLOR:
            if (payLen >= 4) {
                std::uint32_t c;
                std::memcpy(&c, pay, 4);
                st.trueColor = -1;
                st.color = (c <= 256u) ? static_cast<int>(c) : DRW::ColorByLayer;
            }
            break;
        case OP_ATTRIBUTE_TRUE_COLOR:
            if (payLen >= 4) {
                std::uint32_t raw;
                std::memcpy(&raw, pay, 4);
                // low 24 bits = RGB when the high byte marks an RGB colour
                st.trueColor = static_cast<int>(raw & 0x00FFFFFFu);
            }
            break;
        case OP_CIRCLE:        doCircle(pay, payLen, iface, parent, st, count); break;
        case OP_CIRCULAR_ARC:  doArc(pay, payLen, iface, parent, st, count); break;
        case OP_ELLIPTIC_ARC:  doEllipse(pay, payLen, iface, parent, st, count); break;
        case OP_POLYLINE:      doPolyline(pay, payLen, /*closed=*/false, /*normal=*/false, iface, parent, st, count); break;
        case OP_POLYLINE_NORMALS: doPolyline(pay, payLen, /*closed=*/false, /*normal=*/true, iface, parent, st, count); break;
        case OP_POLYGON:       doPolyline(pay, payLen, /*closed=*/true, /*normal=*/false, iface, parent, st, count); break;
        case OP_TEXT:          doText(pay, payLen, iface, parent, st, count); break;
        case OP_LWPOLYLINE:    doLwpolyline(pay, payLen, version, iface, parent, st, count); break;
        default:
            break; // unsupported opcode — skip by size
        }
        index += size;
    }
    if (count > 0) {
        DRW_DBG("proxy graphics: decoded "); DRW_DBG(count); DRW_DBG(" primitive(s)\n");
    }
    return count;
}
