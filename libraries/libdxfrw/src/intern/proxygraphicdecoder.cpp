#include "proxygraphicdecoder.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "../drw_entities.h"
#include "../drw_interface.h"
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "drw_textcodec.h"
#include "dwgbuffer.h"

namespace {

constexpr double DEG_PER_RAD = 57.29577951308232087680; // 180/pi

// ezdxf ProxyGraphicTypes (proxygraphic.py:208). Only the opcodes we decode
// are named; every other opcode (mesh, shell, extents, clip, …) is skipped
// uniformly by the framing loop via its self-describing chunk size.
enum OpCode {
    OP_CIRCLE                = 2,
    OP_CIRCLE_3P             = 3,
    OP_CIRCULAR_ARC          = 4,
    OP_CIRCULAR_ARC_3P       = 5,
    OP_POLYLINE              = 6,
    OP_POLYGON               = 7,
    OP_SHELL                 = 9,
    OP_TEXT                  = 10,
    OP_TEXT2                 = 11,
    OP_ATTRIBUTE_COLOR       = 14,
    OP_ATTRIBUTE_LAYER       = 16,
    OP_ATTRIBUTE_LINETYPE    = 18,
    OP_ATTRIBUTE_TRUE_COLOR  = 22,
    OP_ATTRIBUTE_LINEWEIGHT  = 23,
    OP_PUSH_MATRIX           = 29,
    OP_PUSH_MATRIX2          = 30,
    OP_POP_MATRIX            = 31,
    OP_POLYLINE_NORMALS      = 32,
    OP_LWPOLYLINE            = 33,
    OP_UNICODE_TEXT2         = 38,
    OP_ELLIPTIC_ARC          = 44,
};

// Affine transform carried by PUSH_MATRIX (29/30) and unwound by POP_MATRIX
// (31).  Stored row-major with the translation in the 4th column (the ODA
// transform convention; equivalently ezdxf's Matrix44 after its transpose at
// proxygraphic.py:343-347).  Without this, every primitive nested inside a
// matrix is emitted in the block's local frame and renders at the wrong
// position — a silent error, since the primitive still decodes and counts.
struct Matrix {
    // rows 0..2, each [xx xy xz tx]
    double m[12] = {1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0};

    DRW_Coord point(const DRW_Coord &p) const {
        return DRW_Coord(m[0] * p.x + m[1] * p.y + m[2]  * p.z + m[3],
                         m[4] * p.x + m[5] * p.y + m[6]  * p.z + m[7],
                         m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]);
    }
    // Direction vectors ignore the translation column.
    DRW_Coord dir(const DRW_Coord &v) const {
        return DRW_Coord(m[0] * v.x + m[1] * v.y + m[2]  * v.z,
                         m[4] * v.x + m[5] * v.y + m[6]  * v.z,
                         m[8] * v.x + m[9] * v.y + m[10] * v.z);
    }
    // Similarity scale from the planar linear part.  Proxy graphics is
    // overwhelmingly planar; a non-uniform scale would turn a circle into an
    // ellipse, which no corpus file exercises (no circle/arc/ellipse occurs
    // under an active matrix).
    double scale() const {
        const double det = m[0] * m[5] - m[1] * m[4];
        const double s = std::sqrt(std::fabs(det));
        return (s > 1e-12) ? s : 1.0;
    }
};

inline DRW_Coord xfPoint(const Matrix *xf, const DRW_Coord &p) {
    return xf ? xf->point(p) : p;
}
inline DRW_Coord xfDir(const Matrix *xf, const DRW_Coord &v) {
    return xf ? xf->dir(v) : v;
}
inline double xfScale(const Matrix *xf) { return xf ? xf->scale() : 1.0; }

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
    // PUS: UTF-16LE, double-NUL (0x0000) terminated, then align to 4.  Returns
    // the RAW UTF-16LE bytes (caller transcodes to UTF-8).  Mirrors ezdxf
    // ByteStream.read_padded_unicode_string (binarydata.py).
    std::string readPaddedUnicodeString() {
        std::string s;
        std::size_t e = m_i;
        while (e + 1 < m_n && !(m_p[e] == 0 && m_p[e + 1] == 0)) e += 2;
        if (e + 1 >= m_n) { bad = true; return s; }
        s.assign(reinterpret_cast<const char *>(m_p + m_i), e - m_i);
        m_i = align4(e + 2);
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
// (proxygraphic.py:353-406).  layer/lineType are resolved from the per-proxy
// index→name tables passed into decode(); empty == inherit from the parent.
struct DrawState {
    int color = DRW::ColorByLayer; // 256 == BYLAYER
    int trueColor = -1;            // 24-bit RGB, -1 == unset
    int lWeight = -1;              // DXF-scale lineweight, -1 == unset
    std::string layer;             // empty == inherit parent
    std::string lineType;          // empty == inherit parent
};

class DecodeContext {
public:
    DecodeContext(DRW_ProxyGraphicSink& sink,
                  const DRW_ProxyGraphicLimits& limits,
                  DRW_ProxyGraphicDecodeResult& result) noexcept
        : m_sink(sink), m_limits(limits), m_result(result) {}

    template<typename T>
    bool emit(const T& value,
              bool (DRW_ProxyGraphicSink::*callback)(const T&)) {
        if (!m_result.completed())
            return false;
        if (m_result.emittedPrimitiveCount >= m_limits.maxPrimitiveCount) {
            fail(DRW_ProxyGraphicStopReason::PrimitiveLimit);
            return false;
        }
        if (!(m_sink.*callback)(value)) {
            fail(DRW_ProxyGraphicStopReason::SinkRefused);
            return false;
        }
        ++m_result.emittedPrimitiveCount;
        return true;
    }

    void fail(DRW_ProxyGraphicStopReason reason) noexcept {
        if (m_result.completed())
            m_result.stopReason = reason;
    }

    [[nodiscard]] bool stopped() const noexcept {
        return !m_result.completed();
    }

    [[nodiscard]] const DRW_ProxyGraphicLimits& limits() const noexcept {
        return m_limits;
    }

private:
    DRW_ProxyGraphicSink& m_sink;
    const DRW_ProxyGraphicLimits& m_limits;
    DRW_ProxyGraphicDecodeResult& m_result;
};

// Apply the owning entity's identity plus the accumulated colour state onto a
// freshly decoded primitive so it lands in the right container and renders with
// the proxy's colour.  Coordinates are emitted verbatim (WCS) with a Z
// extrusion: proxy graphics is overwhelmingly planar, and forcing extPoint=Z
// avoids a double OCS transform downstream.
void applyAttribs(DRW_Entity &e, const DRW_Entity &parent, const DrawState &st) {
    e.space = parent.space;
    e.layer = st.layer.empty() ? parent.layer : st.layer;
    e.lineType = st.lineType.empty() ? parent.lineType : st.lineType;
    e.handle = parent.handle;
    e.parentHandle = parent.parentHandle;
    e.lWeight = (st.lWeight != -1) ? DRW_LW_Conv::dxfInt2lineWidth(st.lWeight) : parent.lWeight;
    e.color = (st.color >= 0 && st.color != DRW::ColorByLayer) ? st.color : parent.color;
    if (st.trueColor >= 0)
        e.color24 = st.trueColor;
}

// 2-D circumcenter of three points via the perpendicular-bisector intersection
// (ezdxf ConstructionCircle.from_3p, circle.py:39-52).  Returns false on a
// near-zero determinant (collinear) so the caller emits nothing.
bool circumcenter2d(const DRW_Coord &a, const DRW_Coord &b, const DRW_Coord &c,
                    double &cx, double &cy) {
    const double d = 2.0 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::fabs(d) < 1e-12) return false;
    const double a2 = a.x * a.x + a.y * a.y;
    const double b2 = b.x * b.x + b.y * b.y;
    const double c2 = c.x * c.x + c.y * c.y;
    cx = (a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d;
    cy = (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d;
    return true;
}

// --- opcode 5: CIRCULAR_ARC_3P (proxygraphic.py:470) ---
// Wire order is p1,p2,p3; ezdxf calls ConstructionArc.from_3p(p1, p3, p2)
// (arc.py:319) → start angle from p1, end angle from p3 (CCW), p2 only
// constrains the circle.  The trailing arc_type long is NOT read (commented out
// in ezdxf).
void doArc3p(const std::uint8_t *p, std::size_t n, DecodeContext& context,
             const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Coord p1 = bs.readVertex();  // start
    DRW_Coord p2 = bs.readVertex();  // definition point (only constrains the circle)
    DRW_Coord p3 = bs.readVertex();  // end
    if (bs.bad) return;
    // Transform the defining points; centre/radius/angles derived below then
    // follow automatically.
    p1 = xfPoint(xf, p1);
    p2 = xfPoint(xf, p2);
    p3 = xfPoint(xf, p3);
    double cx, cy;
    if (!circumcenter2d(p1, p2, p3, cx, cy)) return; // collinear → emit nothing
    DRW_Arc e;
    e.basePoint = DRW_Coord(cx, cy, 0.0);
    e.radious = std::hypot(p1.x - cx, p1.y - cy);
    e.staangle = std::atan2(p1.y - cy, p1.x - cx); // RADIANS (DRW_Arc native), start = p1
    e.endangle = std::atan2(p3.y - cy, p3.x - cx); // end = p3 (CCW, no swap)
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addArc);
}

// --- opcode 3: CIRCLE_3P (proxygraphic.py:425) ---
void doCircle3p(const std::uint8_t *p, std::size_t n, DecodeContext& context,
                const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Coord p1 = bs.readVertex();
    DRW_Coord p2 = bs.readVertex();
    DRW_Coord p3 = bs.readVertex();
    if (bs.bad) return;
    p1 = xfPoint(xf, p1);
    p2 = xfPoint(xf, p2);
    p3 = xfPoint(xf, p3);
    double cx, cy;
    if (!circumcenter2d(p1, p2, p3, cx, cy)) return;
    DRW_Circle e;
    e.basePoint = DRW_Coord(cx, cy, 0.0);
    e.radious = std::hypot(p1.x - cx, p1.y - cy);
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addCircle);
}

// --- opcode 2: CIRCLE (proxygraphic.py:408) ---
void doCircle(const std::uint8_t *p, std::size_t n, DecodeContext& context,
              const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Circle e;
    const DRW_Coord centre = bs.readVertex();
    const double radius = bs.readDouble();
    // normal vector follows (3 doubles) — read to advance but ignore (planar).
    bs.readVertex();
    if (bs.bad) return;
    e.basePoint = xfPoint(xf, centre);
    e.radious = radius * xfScale(xf);
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addCircle);
}

// --- opcode 4: CIRCULAR_ARC (proxygraphic.py:436) ---
void doArc(const std::uint8_t *p, std::size_t n, DecodeContext& context,
           const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Arc e;
    const DRW_Coord centre = bs.readVertex();  // center (WCS)
    const double radius = bs.readDouble();
    bs.readVertex();                    // normal (ignored — planar)
    DRW_Coord start = bs.readVertex();  // UCS x-axis
    double sweep = bs.readDouble();     // radians
    if (bs.bad) return;
    e.basePoint = xfPoint(xf, centre);
    e.radious = radius * xfScale(xf);
    // Planar case: start angle from the start vector, end = start + sweep.
    // The sweep is preserved under a rotation/uniform scale (the only kinds the
    // corpus carries); a mirroring matrix would additionally flip its sign.
    start = xfDir(xf, start);
    e.staangle = std::atan2(start.y, start.x); // libdxfrw stores radians
    e.endangle = e.staangle + sweep;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addArc);
}

// --- opcode 44: ELLIPTIC_ARC (proxygraphic.py:484) ---
void doEllipse(const std::uint8_t *p, std::size_t n, DecodeContext& context,
               const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Ellipse e;
    const DRW_Coord centre = bs.readVertex();   // center
    bs.readVertex();                 // extrusion (ignored — planar)
    double majorLen = bs.readDouble();
    double minorLen = bs.readDouble();
    double startParam = bs.readDouble();
    double endParam = bs.readDouble();
    double majorAngle = bs.readDouble();
    if (bs.bad || majorLen == 0.0) return;
    e.basePoint = xfPoint(xf, centre);
    e.ratio = minorLen / majorLen;   // preserved under a similarity transform
    // Major axis is a direction+length, so it rotates and scales with the
    // linear part alone.
    e.secPoint = xfDir(xf, DRW_Coord(std::cos(majorAngle) * majorLen,
                                     std::sin(majorAngle) * majorLen, 0.0));
    e.staparam = startParam;
    e.endparam = endParam;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addEllipse);
}

// --- opcodes 6 / 7 / 32: POLYLINE / POLYGON (proxygraphic.py:536) ---
// _load_vertices (proxygraphic.py:849): L count [+1 if load_normal] then
// count×3d.  A POLYGON (7) is a closed POLYLINE.
void doPolyline(const std::uint8_t *p, std::size_t n, bool closed, bool loadNormal,
                DecodeContext& context, const DRW_Entity &parent,
                const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    std::uint32_t vcount = bs.readLong();
    if (loadNormal) {
        if (vcount == std::numeric_limits<std::uint32_t>::max())
            return;
        vcount += 1; // last vertex is the normal
    }
    if (vcount == 0) return;
    if (vcount > context.limits().maxPolylineVertices) {
        context.fail(DRW_ProxyGraphicStopReason::ResourceLimit);
        return;
    }
    std::vector<DRW_Coord> verts;
    if (!DRW::reserve(verts, static_cast<int>(vcount))) {
        context.fail(DRW_ProxyGraphicStopReason::AllocationFailure);
        return;
    }
    for (std::uint32_t i = 0; i < vcount && !bs.bad; ++i)
        verts.push_back(bs.readVertex());
    if (bs.bad) return;
    if (loadNormal && !verts.empty()) verts.pop_back(); // drop the normal
    if (verts.size() < 2) return;

    // Transform before the planarity probe so `is3d` reflects the final coords.
    if (xf)
        for (DRW_Coord &v : verts) v = xf->point(v);

    DRW_Polyline e;
    bool is3d = false;
    for (const DRW_Coord &v : verts)
        if (std::fabs(v.z) > 1e-9) { is3d = true; break; }
    e.flags = (closed ? 1 : 0) | (is3d ? 8 : 0);
    for (const DRW_Coord &v : verts)
        e.addVertex(DRW_Vertex(v.x, v.y, v.z, 0.0));
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addPolyline);
}

// --- opcode 9: SHELL (proxygraphic.py shell) → polyface mesh ---
// Layout: RL total_vertex_count, count×3d vertices, RL face_entry_count (the
// number of longs in the face stream), then a flat stream of
// [signed edge_count, idx0, idx1, …] entries (edge_count<0 marks a wrap/hole,
// its magnitude is the vertex count; indices are 0-based).  ezdxf builds a
// POLYLINE_POLYFACE; we deliver a DRW_Mesh (base-cage vertices + face index
// lists) which LibreCAD renders by decomposing faces to polylines.  Trailing
// mesh traits (per-face colours/normals) are ignored.
void doShell(const std::uint8_t *p, std::size_t n, DecodeContext& context,
             const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    std::uint32_t vcount = bs.readLong();
    if (vcount == 0) return;
    if (vcount > context.limits().maxMeshVertices) {
        context.fail(DRW_ProxyGraphicStopReason::ResourceLimit);
        return;
    }
    DRW_Mesh e;
    if (!DRW::reserve(e.vertices, static_cast<int>(vcount))) {
        context.fail(DRW_ProxyGraphicStopReason::AllocationFailure);
        return;
    }
    for (std::uint32_t i = 0; i < vcount && !bs.bad; ++i)
        e.vertices.push_back(xfPoint(xf, bs.readVertex()));
    if (bs.bad) return;

    std::uint32_t faceEntries = bs.readLong();   // total longs in the face stream
    if (faceEntries > context.limits().maxMeshFaceEntries) {
        context.fail(DRW_ProxyGraphicStopReason::ResourceLimit);
        return;
    }
    std::uint32_t read = 0;
    while (read < faceEntries && !bs.bad) {
        const std::uint32_t rawEdgeCount = bs.readLong();
        const bool isHole = (rawEdgeCount & 0x80000000u) != 0;
        const std::uint32_t edgeCount = isHole
            ? 0u - rawEdgeCount : rawEdgeCount;
        if (edgeCount > faceEntries - read - 1u)
            return;
        read += 1u + edgeCount;
        std::vector<std::int32_t> face;
        if (!DRW::reserve(face, static_cast<int>(edgeCount))) {
            context.fail(DRW_ProxyGraphicStopReason::AllocationFailure);
            return;
        }
        for (std::uint32_t j = 0; j < edgeCount && !bs.bad; ++j) {
            std::uint32_t idx = bs.readLong();
            if (idx < e.vertices.size())
                face.push_back(static_cast<std::int32_t>(idx));
        }
        if (face.size() >= 2) e.faces.push_back(std::move(face));
    }
    if (e.vertices.empty() || e.faces.empty()) return;
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addMesh);
}

// --- opcode 10: TEXT (proxygraphic.py:694, non-unicode) ---
void doText(const std::uint8_t *p, std::size_t n, DecodeContext& context,
            const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Text e;
    const DRW_Coord start = bs.readVertex();  // start point
    bs.readVertex();                        // normal (ignored)
    DRW_Coord dir = bs.readVertex();        // text direction
    const double height = bs.readDouble();
    e.widthscale = bs.readDouble();
    e.oblique = bs.readDouble() * DEG_PER_RAD;
    std::string text = bs.readPaddedString();
    if (bs.bad || text.empty()) return;
    e.text = text;
    e.basePoint = xfPoint(xf, start);
    e.height = height * xfScale(xf);
    dir = xfDir(xf, dir);
    e.angle = std::atan2(dir.y, dir.x) * DEG_PER_RAD;
    e.extPoint = DRW_Coord(0, 0, 1);
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addText);
}

// Shared tail of op11/op38: after the (already-read) string the layout is
// <2l>(ignore_len, raw) <4d>(height, width, oblique, tracking) <5L>(backwards,
// upsidedown, vertical, underline, overline).  We stop before the trailing font
// strings (style resolution is out of scope).  Fills the metadata onto `e`.
void readText2Metadata(ByteStream &bs, DRW_Text &e, const DRW_Coord &dir,
                       double scale = 1.0) {
    bs.readLong(); bs.readLong();            // <2l ignore_length_of_string, raw
    double h = bs.readDouble();
    double w = bs.readDouble();
    double oblique = bs.readDouble();
    bs.readDouble();                         // tracking_percentage — discard
    std::uint32_t isBackwards = bs.readLong();
    std::uint32_t isUpsideDown = bs.readLong();
    bs.readLong(); bs.readLong(); bs.readLong(); // is_vertical/underline/overline — discard
    e.height = h * scale;
    e.widthscale = w;
    e.oblique = oblique * DEG_PER_RAD;
    e.angle = std::atan2(dir.y, dir.x) * DEG_PER_RAD;
    e.textgen = 2 * (isBackwards ? 1 : 0) + 4 * (isUpsideDown ? 1 : 0);
    e.extPoint = DRW_Coord(0, 0, 1);
}

// --- opcode 11: TEXT2 (proxygraphic.py:723) — string FIRST, then metadata ---
void doText2(const std::uint8_t *p, std::size_t n, DecodeContext& context,
             const DRW_Entity &parent, const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Text e;
    const DRW_Coord start = bs.readVertex();  // start_point
    bs.readVertex();                        // normal (ignored — planar)
    DRW_Coord dir = bs.readVertex();        // text_direction
    std::string text = bs.readPaddedString(); // self.encoding bytes (kept raw)
    readText2Metadata(bs, e, xfDir(xf, dir), xfScale(xf));
    if (bs.bad || text.empty()) return;
    e.basePoint = xfPoint(xf, start);
    e.text = text;
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addText);
}

// --- opcode 38: UNICODE_TEXT2 (proxygraphic.py:778) — UTF-16LE text ---
void doUnicodeText2(const std::uint8_t *p, std::size_t n,
                    DecodeContext& context, const DRW_Entity &parent,
                    const DrawState &st, const Matrix *xf) {
    ByteStream bs(p, n);
    DRW_Text e;
    const DRW_Coord start = bs.readVertex();
    bs.readVertex();                        // normal (ignored)
    DRW_Coord dir = bs.readVertex();
    std::string u16 = bs.readPaddedUnicodeString(); // raw UTF-16LE bytes
    readText2Metadata(bs, e, xfDir(xf, dir), xfScale(xf)); // same tail as op11
    if (bs.bad || u16.empty()) return;
    std::string text = DRW_ConvUTF16().toUtf8(u16); // surrogate-aware
    if (text.empty()) return;
    e.basePoint = xfPoint(xf, start);
    e.text = text;
    applyAttribs(e, parent, st);
    (void)context.emit(e, &DRW_ProxyGraphicSink::addText);
}

// --- opcode 33: LWPOLYLINE (proxygraphic.py:549) — an ODA *bit* stream ---
void doLwpolyline(const std::uint8_t *p, std::size_t n, DRW::Version version,
                  DecodeContext& context, const DRW_Entity &parent,
                  const DrawState &st, const Matrix *xf) {
    DRW_LWPolyline e;
    try {
        std::vector<std::uint8_t> buf(p, p + n);
        dwgBuffer bs(buf.data(), buf.size(), nullptr); // no text -> no decoder needed
        bs.getRawLong32();                              // num_data_bytes (RL)
        std::int32_t flag = bs.getBitShort();           // BS
        if (flag & 4) e.width = bs.getBitDouble();          // const width
        if (flag & 8) e.elevation = bs.getBitDouble();
        if (flag & 2) e.thickness = bs.getBitDouble();
        if (flag & 1) { bs.getBitDouble(); bs.getBitDouble(); bs.getBitDouble(); } // extrusion 3BD
        bool isClosed = (flag & 512) != 0;
        std::int32_t numPoints = bs.getBitLong();
        if (numPoints <= 0) return;
        if (static_cast<std::size_t>(numPoints)
            > context.limits().maxPolylineVertices) {
            context.fail(DRW_ProxyGraphicStopReason::ResourceLimit);
            return;
        }
        std::int32_t numBulges = 0, numVertexIds = 0, numWidth = 0;
        if (flag & 16) numBulges = bs.getBitLong();
        if (version >= DRW::AC1024) {
            if (flag & 1024) numVertexIds = bs.getBitLong();
            if (flag & 32)   numWidth     = bs.getBitLong();
        }
        (void)numBulges; (void)numVertexIds; (void)numWidth;

        // Vertices are decoded as deltas off the previous raw pair, so transform a
        // copy when emitting and keep px/py in the stream's own frame.
        auto emit2d = [&](double x, double y) {
            const DRW_Coord t = xfPoint(xf, DRW_Coord(x, y, 0.0));
            e.addVertex(DRW_Vertex2D(t.x, t.y, 0.0));
        };
        double px = bs.getRawDouble();
        double py = bs.getRawDouble();
        emit2d(px, py);
        for (std::int32_t i = 1; i < numPoints; ++i) {
            px = bs.getDefaultDouble(px);
            py = bs.getDefaultDouble(py);
            emit2d(px, py);
        }
        if (!bs.isGood()) return; // bit stream overran - drop
        e.vertexnum = numPoints;
        e.flags = isClosed ? 1 : 0;
        applyAttribs(e, parent, st);
    } catch (...) {
        context.fail(DRW_ProxyGraphicStopReason::AllocationFailure);
        return;
    }
    (void)context.emit(e, &DRW_ProxyGraphicSink::addLWPolyline);
}

bool isPrimitiveOpcode(std::uint32_t type) noexcept {
    switch (type) {
    case OP_CIRCLE:
    case OP_CIRCLE_3P:
    case OP_CIRCULAR_ARC:
    case OP_CIRCULAR_ARC_3P:
    case OP_POLYLINE:
    case OP_POLYGON:
    case OP_SHELL:
    case OP_TEXT:
    case OP_TEXT2:
    case OP_POLYLINE_NORMALS:
    case OP_LWPOLYLINE:
    case OP_UNICODE_TEXT2:
    case OP_ELLIPTIC_ARC:
        return true;
    default:
        return false;
    }
}

class InterfaceSink final : public DRW_ProxyGraphicSink {
public:
    explicit InterfaceSink(DRW_Interface& target) noexcept : m_target(target) {}

    bool addArc(const DRW_Arc& value) override { m_target.addArc(value); return true; }
    bool addCircle(const DRW_Circle& value) override { m_target.addCircle(value); return true; }
    bool addEllipse(const DRW_Ellipse& value) override { m_target.addEllipse(value); return true; }
    bool addLWPolyline(const DRW_LWPolyline& value) override { m_target.addLWPolyline(value); return true; }
    bool addMesh(const DRW_Mesh& value) override { m_target.addMesh(value); return true; }
    bool addPolyline(const DRW_Polyline& value) override { m_target.addPolyline(value); return true; }
    bool addText(const DRW_Text& value) override { m_target.addText(value); return true; }

private:
    DRW_Interface& m_target;
};

} // namespace

DRW_ProxyGraphicDecodeResult DRW_ProxyGraphicDecoder::inspect(
    const std::string& bytes, const DRW_ProxyGraphicLimits& limits) {
    DRW_ProxyGraphicDecodeResult result;
    const std::size_t length = bytes.size();
    if (length == 0)
        return result;
    if (length < 8) {
        result.stopReason = DRW_ProxyGraphicStopReason::ShortHeader;
        return result;
    }

    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    std::size_t index = 8;
    std::size_t chunkCount = 0;
    result.consumedByteCount = index;
    while (index < length) {
        if (length - index < 8) {
            result.stopReason = DRW_ProxyGraphicStopReason::ShortHeader;
            break;
        }
        if (chunkCount >= limits.maxChunkCount) {
            result.stopReason = DRW_ProxyGraphicStopReason::ChunkLimit;
            break;
        }

        std::uint32_t size = 0;
        std::uint32_t type = 0;
        std::memcpy(&size, data + index, sizeof(size));
        std::memcpy(&type, data + index + sizeof(size), sizeof(type));
        if (size < 8) {
            result.stopReason = DRW_ProxyGraphicStopReason::InvalidChunkSize;
            break;
        }
        if (size > length - index) {
            result.stopReason = DRW_ProxyGraphicStopReason::TruncatedChunk;
            break;
        }

        ++chunkCount;
        if (isPrimitiveOpcode(type)) {
            if (result.recognizedPrimitiveChunkCount
                >= limits.maxPrimitiveCount) {
                result.stopReason = DRW_ProxyGraphicStopReason::PrimitiveLimit;
                break;
            }
            ++result.recognizedPrimitiveChunkCount;
        } else {
            ++result.skippedUnsupportedChunkCount;
        }
        index += size;
        result.consumedByteCount = index;
    }
    return result;
}

DRW_ProxyGraphicDecodeResult DRW_ProxyGraphicDecoder::decode(
    const std::string& bytes, DRW::Version version, DRW_ProxyGraphicSink& sink,
    const DRW_Entity& parent, const std::vector<std::string>& layerNames,
    const std::vector<std::string>& ltypeNames,
    const DRW_ProxyGraphicLimits& limits) {
    DRW_ProxyGraphicDecodeResult result;
    const std::size_t length = bytes.size();
    if (length == 0)
        return result;
    if (length < 8) {
        result.stopReason = DRW_ProxyGraphicStopReason::ShortHeader;
        return result;
    }

    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    std::size_t index = 8;
    std::size_t chunkCount = 0;
    result.consumedByteCount = index;
    DrawState state;
    DecodeContext context(sink, limits, result);
    std::vector<Matrix> matrixStack;
    try {
        matrixStack.reserve(limits.maxMatrixDepth);
    } catch (...) {
        result.stopReason = DRW_ProxyGraphicStopReason::AllocationFailure;
        return result;
    }

    while (index < length) {
        if (length - index < 8) {
            result.stopReason = DRW_ProxyGraphicStopReason::ShortHeader;
            break;
        }
        if (chunkCount >= limits.maxChunkCount) {
            result.stopReason = DRW_ProxyGraphicStopReason::ChunkLimit;
            break;
        }

        std::uint32_t size = 0;
        std::uint32_t type = 0;
        std::memcpy(&size, data + index, sizeof(size));
        std::memcpy(&type, data + index + sizeof(size), sizeof(type));
        if (size < 8) {
            result.stopReason = DRW_ProxyGraphicStopReason::InvalidChunkSize;
            break;
        }
        if (size > length - index) {
            result.stopReason = DRW_ProxyGraphicStopReason::TruncatedChunk;
            break;
        }

        ++chunkCount;
        const std::uint8_t* const payload = data + index + 8;
        const std::size_t payloadLength = size - 8;
        const Matrix* const matrix = matrixStack.empty()
            ? nullptr : &matrixStack.back();
        if (isPrimitiveOpcode(type)) {
            if (result.recognizedPrimitiveChunkCount
                >= limits.maxPrimitiveCount) {
                result.stopReason = DRW_ProxyGraphicStopReason::PrimitiveLimit;
                break;
            }
            ++result.recognizedPrimitiveChunkCount;
        } else {
            ++result.skippedUnsupportedChunkCount;
        }

        switch (type) {
        case OP_ATTRIBUTE_COLOR:
            if (payloadLength >= 4) {
                std::uint32_t color = 0;
                std::memcpy(&color, payload, sizeof(color));
                state.trueColor = -1;
                state.color = color <= 256u ? static_cast<int>(color)
                                             : DRW::ColorByLayer;
            }
            break;
        case OP_ATTRIBUTE_TRUE_COLOR:
            if (payloadLength >= 4) {
                std::uint32_t raw = 0;
                std::memcpy(&raw, payload, sizeof(raw));
                state.color = DRW::ColorByLayer;
                state.trueColor = -1;
                switch ((raw >> 24) & 0xFFu) {
                case dwgColor::RGB: state.trueColor = static_cast<int>(raw & 0x00FFFFFFu); break;
                case dwgColor::ACIS: state.color = static_cast<int>(raw & 0xFFu); break;
                case dwgColor::BYLAYER: state.color = DRW::ColorByLayer; break;
                case dwgColor::BYBLOCK: state.color = DRW::ColorByBlock; break;
                default: break;
                }
            }
            break;
        case OP_ATTRIBUTE_LINEWEIGHT:
            if (payloadLength >= 4) {
                std::uint32_t lineweight = 0;
                std::memcpy(&lineweight, payload, sizeof(lineweight));
                const long value = lineweight > 211u
                    ? std::max(-3L, static_cast<long>(lineweight) - 0x100000000L)
                    : static_cast<long>(lineweight);
                state.lWeight = static_cast<int>(value);
            }
            break;
        case OP_ATTRIBUTE_LAYER:
            if (payloadLength >= 4) {
                std::uint32_t layer = 0;
                std::memcpy(&layer, payload, sizeof(layer));
                if (layer < layerNames.size() && !layerNames[layer].empty())
                    state.layer = layerNames[layer];
            }
            break;
        case OP_ATTRIBUTE_LINETYPE:
            if (payloadLength >= 4) {
                std::uint32_t lineType = 0;
                std::memcpy(&lineType, payload, sizeof(lineType));
                if (lineType == 32766u) state.lineType = "BYBLOCK";
                else if (lineType == 32767u) state.lineType = "BYLAYER";
                else if (lineType < ltypeNames.size() && !ltypeNames[lineType].empty())
                    state.lineType = ltypeNames[lineType];
                else state.lineType = "BYLAYER";
            }
            break;
        case OP_PUSH_MATRIX:
        case OP_PUSH_MATRIX2:
            if (payloadLength >= 16 * sizeof(double)) {
                if (matrixStack.size() >= limits.maxMatrixDepth) {
                    result.stopReason = DRW_ProxyGraphicStopReason::MatrixDepthLimit;
                    break;
                }
                double values[16];
                std::memcpy(values, payload, sizeof(values));
                Matrix next;
                for (int row = 0; row < 3; ++row)
                    for (int column = 0; column < 4; ++column)
                        next.m[row * 4 + column] = values[row * 4 + column];
                try {
                    matrixStack.push_back(next);
                } catch (...) {
                    result.stopReason = DRW_ProxyGraphicStopReason::AllocationFailure;
                }
            }
            break;
        case OP_POP_MATRIX:
            if (!matrixStack.empty()) matrixStack.pop_back();
            break;
        case OP_CIRCLE: doCircle(payload, payloadLength, context, parent, state, matrix); break;
        case OP_CIRCLE_3P: doCircle3p(payload, payloadLength, context, parent, state, matrix); break;
        case OP_CIRCULAR_ARC: doArc(payload, payloadLength, context, parent, state, matrix); break;
        case OP_CIRCULAR_ARC_3P: doArc3p(payload, payloadLength, context, parent, state, matrix); break;
        case OP_ELLIPTIC_ARC: doEllipse(payload, payloadLength, context, parent, state, matrix); break;
        case OP_POLYLINE: doPolyline(payload, payloadLength, false, false, context, parent, state, matrix); break;
        case OP_POLYLINE_NORMALS: doPolyline(payload, payloadLength, false, true, context, parent, state, matrix); break;
        case OP_POLYGON: doPolyline(payload, payloadLength, true, false, context, parent, state, matrix); break;
        case OP_SHELL: doShell(payload, payloadLength, context, parent, state, matrix); break;
        case OP_TEXT: doText(payload, payloadLength, context, parent, state, matrix); break;
        case OP_TEXT2: doText2(payload, payloadLength, context, parent, state, matrix); break;
        case OP_UNICODE_TEXT2: doUnicodeText2(payload, payloadLength, context, parent, state, matrix); break;
        case OP_LWPOLYLINE: doLwpolyline(payload, payloadLength, version, context, parent, state, matrix); break;
        default: break;
        }
        if (context.stopped())
            break;
        index += size;
        result.consumedByteCount = index;
    }
    if (result.emittedPrimitiveCount > 0) {
        DRW_DBG("proxy graphics: decoded ");
        DRW_DBG(result.emittedPrimitiveCount);
        DRW_DBG(" primitive(s)\n");
    }
    return result;
}

int DRW_ProxyGraphicDecoder::decode(const std::string& bytes,
                                    DRW::Version version,
                                    DRW_Interface& iface,
                                    const DRW_Entity& parent,
                                    const std::vector<std::string>& layerNames,
                                    const std::vector<std::string>& ltypeNames) {
    InterfaceSink sink(iface);
    const DRW_ProxyGraphicDecodeResult result = decode(
        bytes, version, sink, parent, layerNames, ltypeNames);
    return result.emittedPrimitiveCount
        > static_cast<std::size_t>(std::numeric_limits<int>::max())
        ? std::numeric_limits<int>::max()
        : static_cast<int>(result.emittedPrimitiveCount);
}
