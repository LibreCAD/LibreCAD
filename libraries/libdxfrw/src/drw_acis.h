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

//! In-library ACIS SAB (Standard ACIS Binary) wireframe decoder.
/*!
 *  Decodes the byte-true SAB stream captured from DXF/DWG modeler payloads into
 *  a lightweight B-rep wireframe: vertices (3D points), edges (segment endpoints
 *  + analytic curve params) and faces (surface type/params + loop structure).
 *
 *  This is a Qt-free, RS_*-free port of the TypeScript reference parser
 *  (d:/data/dli/dwg-parser/src/dwg/acis/: sab.ts, entities.ts, geometry.ts).
 *  Three layers:
 *    1. DRW_SabReader / drw_parseSab   — SAB tag byte stream -> record list.
 *    2. DRW_AcisModel / drw_buildAcisModel — resolve POINTER tokens into a graph.
 *    3. drw_extractAcisWireframe        — walk the graph into a DRW_AcisBrep.
 *  drw_decodeAcisWireframe() is the convenience entry: locate the SAB signature
 *  in a raw blob, then parse -> build -> extract. It NEVER throws; malformed or
 *  non-SAB input yields an empty DRW_AcisBrep and a false return.
 *
 *  Tessellation, SAT text and NURBS evaluation are intentionally out of scope.
 */

#ifndef DRW_ACIS_H
#define DRW_ACIS_H

#include <cstdint>
#include <string>
#include <vector>

#include "drw_base.h"

#if defined(__APPLE__) && defined(__MACH__)
#define LIBDXFRW_EXPORT __attribute__((visibility("default")))
#else
#define LIBDXFRW_EXPORT
#endif

// ── SAB low-level token/record model (sab.ts) ──────────────────────────────

//! SAB tag bytes (ezdxf `Tags`).
namespace DRW_SabTag {
    enum Tag {
        NoType = 0x00, Byte = 0x01, Char = 0x02, Short = 0x03, Int = 0x04,
        Float = 0x05, Double = 0x06, Str = 0x07, Str2 = 0x08, Str3 = 0x09,
        BoolTrue = 0x0a, BoolFalse = 0x0b, Pointer = 0x0c, EntityType = 0x0d,
        EntityTypeEx = 0x0e, SubtypeStart = 0x0f, SubtypeEnd = 0x10,
        RecordEnd = 0x11, LiteralStr = 0x12, LocationVec = 0x13,
        DirectionVec = 0x14, Enum = 0x15, Unknown0x17 = 0x17,
    };
}

//! A single decoded SAB field token: a tag plus its typed value.
/*!
 *  Only one of the value slots is meaningful, selected by `tag`:
 *  Double/Unknown0x17 -> dval; Int/Pointer/Enum/SubtypeStart/SubtypeEnd -> ival;
 *  BoolTrue/BoolFalse -> bval; Str/LiteralStr/EntityType/EntityTypeEx -> sval;
 *  LocationVec/DirectionVec -> vec.
 */
struct DRW_SabToken {
    int tag = DRW_SabTag::NoType;
    double dval = 0.0;
    long long ival = 0;
    bool bval = false;
    std::string sval;
    DRW_Coord vec;
};

//! A decoded SAB record: its entity-type identity plus all field tokens.
struct DRW_SabRecord {
    std::string type;                    //!< record identity = first entity-type
    std::vector<DRW_SabToken> tokens;    //!< all field tokens in wire order
};

//! SAB stream header (product id, version, tolerances).
struct DRW_SabHeader {
    std::string signature;
    int version = 0;
    int numRecords = 0;
    int numEntities = 0;
    int flags = 0;
    std::string productId;
    std::string acisVersion;
    std::string creationDate;
    double unitsInMm = 1.0;
    double resTol = 0.0;
    double norTol = 0.0;
};

//! A fully decoded SAB stream: header + record list.
struct DRW_SabData {
    DRW_SabHeader header;
    std::vector<DRW_SabRecord> records;
};

//! Decode a byte-true SAB stream into a header + record list.
/*!
 *  Returns false on a malformed/truncated stream or missing signature (out
 *  stays partially filled / empty). Never throws.
 */
LIBDXFRW_EXPORT bool drw_parseSab(const std::uint8_t* data, std::size_t length, DRW_SabData& out);

// ── B-rep record graph (entities.ts) ───────────────────────────────────────

struct DRW_AcisNode {
    int index = 0;                       //!< position in the record stream
    std::string type;                    //!< entity-type name
    DRW_SabRecord record;                //!< the decoded record (all tokens)
    std::vector<int> refs;               //!< resolved POINTER targets (-1 = null)
};

//! Resolved SAB record graph: nodes indexed by record position.
struct DRW_AcisModel {
    DRW_SabHeader header;
    std::vector<DRW_AcisNode> nodes;
    //! Node indices grouped by entity type (excludes end-of-data marker).
    std::vector<int> nodesOfType(const std::string& type) const;
};

//! Build the resolved B-rep graph from a decoded SAB stream.
DRW_AcisModel drw_buildAcisModel(const DRW_SabData& sab);

// ── Extracted wireframe geometry (geometry.ts) ─────────────────────────────

enum class DRW_AcisCurve { Straight, Ellipse, Intcurve, Unknown };
enum class DRW_AcisSurface { Plane, Cone, Torus, Sphere, Spline, Unknown };

//! A B-rep vertex; `valid` is false when its point could not be resolved.
struct DRW_AcisVertex {
    int nodeIndex = 0;
    DRW_Coord point;
    bool valid = false;
};

//! A B-rep edge with resolved endpoints and analytic curve params.
/*!
 *  Curve param slots by curveType:
 *   - Straight: p0 = origin, p1 = direction.
 *   - Ellipse:  p0 = center, p1 = normal, p2 = majorAxis, ratio.
 *   - Intcurve: controlPoints (B-spline control polygon; may be empty).
 */
struct DRW_AcisEdge {
    int nodeIndex = 0;
    DRW_Coord start;
    DRW_Coord end;
    bool hasStart = false;
    bool hasEnd = false;
    DRW_AcisCurve curveType = DRW_AcisCurve::Unknown;
    bool hasCurve = false;               //!< true when curve params were resolved
    DRW_Coord p0;
    DRW_Coord p1;
    DRW_Coord p2;
    double ratio = 0.0;
    std::vector<DRW_Coord> controlPoints;
};

//! One loop of a face: its node index and coedge-ring size.
struct DRW_AcisFaceLoop {
    int nodeIndex = 0;
    int coedgeCount = 0;
};

//! A B-rep face: surface type/params + loop structure.
/*!
 *  Surface param slots by surfaceType:
 *   - Plane: p0 = origin, p1 = normal, p2 = uDir (hasUDir).
 *   - Cone:  p0 = origin, p1 = axis, p2 = majorAxis, ratio, sineAngle, cosineAngle.
 *   - Torus: p0 = center, p1 = axis, majorRadius, minorRadius.
 */
struct DRW_AcisFace {
    int nodeIndex = 0;
    DRW_AcisSurface surfaceType = DRW_AcisSurface::Unknown;
    bool hasSurface = false;             //!< true when surface params were resolved
    DRW_Coord p0;
    DRW_Coord p1;
    DRW_Coord p2;
    bool hasUDir = false;                //!< plane: whether uDir (p2) was present
    double ratio = 0.0;
    double sineAngle = 0.0;
    double cosineAngle = 0.0;
    double majorRadius = 0.0;
    double minorRadius = 0.0;
    std::vector<DRW_AcisFaceLoop> loops;
};

//! Decoded ACIS wireframe: vertices/edges/faces + bbox + diagnostics.
struct DRW_AcisBrep {
    std::vector<DRW_AcisVertex> vertices;
    std::vector<DRW_AcisEdge> edges;
    std::vector<DRW_AcisFace> faces;
    bool hasBBox = false;
    DRW_Coord bboxMin;
    DRW_Coord bboxMax;
    std::vector<std::string> diagnostics;

    bool empty() const {
        return vertices.empty() && edges.empty() && faces.empty();
    }
};

//! Extract B-rep wireframe geometry from a resolved model. Never throws.
LIBDXFRW_EXPORT bool drw_extractAcisWireframe(const DRW_AcisModel& model, DRW_AcisBrep& out);

//! Convenience entry: locate the SAB signature in `raw`, parse -> build -> extract.
/*!
 *  Returns false (and leaves `out` empty) for non-SAB or malformed input.
 *  NEVER throws.
 */
LIBDXFRW_EXPORT bool drw_decodeAcisWireframe(const std::vector<unsigned char>& raw, DRW_AcisBrep& out);

#endif // DRW_ACIS_H
