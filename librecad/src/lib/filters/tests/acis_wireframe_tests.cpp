/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
******************************************************************************/

/**
 * ACIS SAB wireframe decoder tests.
 *
 * Synthetic-vector port of the reference parser's oracle
 * (d:/data/dli/dwg-parser/tests/acisGeometry.test.ts): a byte-true SAB builder
 * (simpleSabBytes) plus a hand-built record graph (model()) exercising every
 * analytic extractor — straight/ellipse curves and plane/cone/torus surfaces,
 * a leading-pointer skip, a surface-less face, loop structure, the finite bbox,
 * and null-safety. These assert decoded values, not smoke checks. Tessellation
 * cases from the reference are intentionally excluded (out of scope for 4.8a).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <cmath>

#include "drw_acis.h"

#include "rs.h"
#include "rs_ellipse.h"
#include "rs_filterdxfrw.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_spline.h"
#include "rs_vector.h"
#include "rs_entitycontainer.h"

using Catch::Approx;

namespace {

// ── Record-graph builders (mirror the TS et/ptr/loc/dir/dbl/rec helpers) ────

DRW_SabToken et(const std::string& name) {
    DRW_SabToken t; t.tag = DRW_SabTag::EntityType; t.sval = name; return t;
}
DRW_SabToken ptr(int v) {
    DRW_SabToken t; t.tag = DRW_SabTag::Pointer; t.ival = v; return t;
}
DRW_SabToken loc(double x, double y, double z) {
    DRW_SabToken t; t.tag = DRW_SabTag::LocationVec; t.vec = DRW_Coord(x, y, z); return t;
}
DRW_SabToken dir(double x, double y, double z) {
    DRW_SabToken t; t.tag = DRW_SabTag::DirectionVec; t.vec = DRW_Coord(x, y, z); return t;
}
DRW_SabToken dbl(double x) {
    DRW_SabToken t; t.tag = DRW_SabTag::Double; t.dval = x; return t;
}
DRW_SabToken inum(long long v) {
    DRW_SabToken t; t.tag = DRW_SabTag::Int; t.ival = v; return t;
}
DRW_SabRecord rec(const std::string& type, std::vector<DRW_SabToken> tokens) {
    DRW_SabRecord r;
    r.type = type;
    r.tokens.push_back(et(type));
    for (auto& tk : tokens) r.tokens.push_back(tk);
    return r;
}

DRW_SabHeader testHeader() {
    DRW_SabHeader h;
    h.signature = "ACIS BinaryFile";
    h.version = 21200;
    h.productId = "test";
    h.acisVersion = "ASM";
    h.creationDate = "now";
    h.unitsInMm = 1;
    h.resTol = 1e-6;
    h.norTol = 1e-10;
    return h;
}

// A small but complete B-rep: two vertices joined by a straight edge, one plane
// face (loop -> coedge -> edge), plus an ellipse edge, a cone face and a torus
// face exercising every analytic extractor. Pointer values are record indices.
DRW_SabData model() {
    DRW_SabData d;
    d.header = testHeader();
    d.records = {
        /* 0 */ rec("asmheader", {}),
        /* 1 */ rec("point", { loc(1, 2, 3) }),
        /* 2 */ rec("point", { loc(4, 5, 6) }),
        /* 3 */ rec("vertex", { ptr(6), ptr(1) }),
        /* 4 */ rec("vertex", { ptr(6), ptr(2) }),
        // straight-curve with an adversarial LEADING pointer before the vecs:
        /* 5 */ rec("straight-curve", { ptr(0), loc(10, 20, 30), dir(2, 0, 0) }),
        /* 6 */ rec("edge", { ptr(3), ptr(4), ptr(5) }),
        /* 7 */ rec("plane-surface", { loc(0, 0, 0), dir(0, 0, 1), dir(1, 0, 0) }),
        /* 8 */ rec("coedge", { ptr(8), ptr(8), ptr(8), ptr(6), ptr(9) }),
        /* 9 */ rec("loop", { ptr(-1), ptr(8), ptr(10) }),
        /* 10 */ rec("face", { ptr(-1), ptr(9), ptr(-1), ptr(-1), ptr(7) }),
        // ellipse edge (no leading ptr; center/normal/major/ratio):
        /* 11 */ rec("point", { loc(0, 0, 5) }),
        /* 12 */ rec("vertex", { ptr(15), ptr(11) }),
        /* 13 */ rec("ellipse-curve", { loc(0, 0, 5), dir(0, 0, 1), dir(3, 0, 0), dbl(0.5) }),
        /* 14 */ rec("point", { loc(3, 0, 5) }),
        /* 15 */ rec("edge", { ptr(12), ptr(16), ptr(13) }),
        /* 16 */ rec("vertex", { ptr(15), ptr(14) }),
        // cone face (origin, axis, major, ratio, sin, cos):
        /* 17 */ rec("cone-surface", { loc(0, 0, 0), dir(0, 0, 1), dir(1, 0, 0), dbl(0.5), dbl(0.3), dbl(0.95) }),
        /* 18 */ rec("face", { ptr(-1), ptr(-1), ptr(-1), ptr(-1), ptr(17) }),
        // torus face (center, axis, x_axis, major, minor) — axis = 2nd vec, x_axis skipped:
        /* 19 */ rec("torus-surface", { loc(0, 0, 0), dir(0, 0, 1), dir(1, 0, 0), dbl(10), dbl(2) }),
        /* 20 */ rec("face", { ptr(-1), ptr(-1), ptr(-1), ptr(-1), ptr(19) }),
        // a face with no surface pointer -> 'unknown' + diagnostic:
        /* 21 */ rec("face", { ptr(-1), ptr(-1), ptr(-1), ptr(-1), ptr(-1) }),
        /* 22 */ rec("End-of-ACIS-data", {}),
    };
    return d;
}

DRW_AcisBrep extractModel() {
    DRW_AcisModel m = drw_buildAcisModel(model());
    DRW_AcisBrep brep;
    drw_extractAcisWireframe(m, brep);
    return brep;
}

// An intcurve-curve record whose subtype carries a `nubs` B-spline control
// polygon (mirrors geometry.ts intcurveControlPoints). Layout after the leading
// entity-type: `nubs`, an Int (numKnots=2), two knot doubles, then the flat
// control-point coordinate stream (2 points => 6 doubles).
DRW_SabRecord intcurveCurveRec() {
    return rec("intcurve-curve", {
        et("nubs"), inum(2),
        dbl(0.0), dbl(1.0),                     // 2 knots
        dbl(7), dbl(8), dbl(9),                 // control point 0
        dbl(10), dbl(11), dbl(12),              // control point 1
    });
}

// The base model() graph EXTENDED with an intcurve edge (two fresh vertices +
// points joined by an intcurve-curve). Kept separate from model() so the base
// count assertions above stay pinned; this exercises the intcurve extractor
// path (drw_acis.cpp intcurveControlPoints), unexercised by the 4.8a suite.
DRW_SabData modelWithIntcurve() {
    DRW_SabData d = model();
    d.records.pop_back();   // drop End-of-ACIS-data (index 22); reappended below
    // New indices start at 22.
    d.records.push_back(rec("point", { loc(0, 0, 0) }));         // 22
    d.records.push_back(rec("point", { loc(4, 0, 0) }));         // 23
    d.records.push_back(rec("vertex", { ptr(27), ptr(22) }));    // 24 (edge=27)
    d.records.push_back(rec("vertex", { ptr(27), ptr(23) }));    // 25
    d.records.push_back(intcurveCurveRec());                     // 26
    d.records.push_back(rec("edge", { ptr(24), ptr(25), ptr(26) })); // 27
    d.records.push_back(rec("End-of-ACIS-data", {}));            // 28
    return d;
}

bool coordEq(const DRW_Coord& c, double x, double y, double z) {
    return c.x == Approx(x) && c.y == Approx(y) && c.z == Approx(z);
}

// ── Byte-true SAB builder (mirrors TS simpleSabBytes) ───────────────────────

void pushAscii(std::vector<unsigned char>& out, const std::string& s) {
    for (char c : s) out.push_back(static_cast<std::uint8_t>(c));
}
void pushInt(std::vector<unsigned char>& out, std::int32_t v) {
    std::uint8_t b[4];
    std::memcpy(b, &v, 4);   // little-endian host
    out.insert(out.end(), b, b + 4);
}
void pushDouble(std::vector<unsigned char>& out, double v) {
    std::uint8_t b[8];
    std::memcpy(b, &v, 8);
    out.insert(out.end(), b, b + 8);
}
void pushSabString(std::vector<unsigned char>& out, const std::string& s) {
    out.push_back(DRW_SabTag::Str);
    out.push_back(static_cast<std::uint8_t>(s.size()));
    pushAscii(out, s);
}
void pushSabDouble(std::vector<unsigned char>& out, double v) {
    out.push_back(DRW_SabTag::Double);
    pushDouble(out, v);
}
void pushEntityType(std::vector<unsigned char>& out, const std::string& s, bool extended = false) {
    out.push_back(extended ? DRW_SabTag::EntityTypeEx : DRW_SabTag::EntityType);
    out.push_back(static_cast<std::uint8_t>(s.size()));
    pushAscii(out, s);
}
void pushSabVec(std::vector<unsigned char>& out, int tag, double x, double y, double z) {
    out.push_back(static_cast<std::uint8_t>(tag));
    pushDouble(out, x);
    pushDouble(out, y);
    pushDouble(out, z);
}

std::vector<unsigned char> simpleSabBytes() {
    std::vector<unsigned char> out;
    pushAscii(out, "ACIS BinaryFile");
    pushInt(out, 21200);
    pushInt(out, -1);
    pushInt(out, -1);
    pushInt(out, 0);
    pushSabString(out, "test");
    pushSabString(out, "ASM");
    pushSabString(out, "now");
    pushSabDouble(out, 1);
    pushSabDouble(out, 1e-6);
    pushSabDouble(out, 1e-10);

    pushEntityType(out, "point");
    pushSabVec(out, DRW_SabTag::LocationVec, 1, 2, 3);
    out.push_back(DRW_SabTag::RecordEnd);

    pushEntityType(out, "vertex");
    out.push_back(DRW_SabTag::Pointer);
    pushInt(out, 0);
    out.push_back(DRW_SabTag::RecordEnd);

    pushEntityType(out, "End", true);
    pushEntityType(out, "of", true);
    pushEntityType(out, "ACIS", true);
    pushEntityType(out, "data");
    return out;
}

const DRW_AcisEdge* findEdge(const DRW_AcisBrep& g, DRW_AcisCurve kind) {
    for (const auto& e : g.edges) if (e.curveType == kind) return &e;
    return nullptr;
}
const DRW_AcisFace* findFace(const DRW_AcisBrep& g, DRW_AcisSurface kind) {
    for (const auto& f : g.faces) if (f.surfaceType == kind) return &f;
    return nullptr;
}

} // namespace

TEST_CASE("extractAcisWireframe: vertex set + finite bbox", "[acis]") {
    DRW_AcisBrep g = extractModel();
    REQUIRE(g.vertices.size() == 4);
    for (const auto& v : g.vertices) REQUIRE(v.valid);
    REQUIRE(g.hasBBox);
    REQUIRE(coordEq(g.bboxMin, 0, 0, 3));
    REQUIRE(coordEq(g.bboxMax, 4, 5, 6));
}

TEST_CASE("extractAcisWireframe: straight edge endpoints + leading-pointer skip", "[acis]") {
    DRW_AcisBrep g = extractModel();
    const DRW_AcisEdge* straight = findEdge(g, DRW_AcisCurve::Straight);
    REQUIRE(straight != nullptr);
    REQUIRE(straight->hasStart);
    REQUIRE(straight->hasEnd);
    REQUIRE(coordEq(straight->start, 1, 2, 3));
    REQUIRE(coordEq(straight->end, 4, 5, 6));
    REQUIRE(straight->hasCurve);
    REQUIRE(coordEq(straight->p0, 10, 20, 30));   // origin (past the leading pointer)
    REQUIRE(coordEq(straight->p1, 2, 0, 0));       // direction
}

TEST_CASE("extractAcisWireframe: ellipse curve params", "[acis]") {
    DRW_AcisBrep g = extractModel();
    const DRW_AcisEdge* ell = findEdge(g, DRW_AcisCurve::Ellipse);
    REQUIRE(ell != nullptr);
    REQUIRE(ell->hasCurve);
    REQUIRE(coordEq(ell->p0, 0, 0, 5));   // center
    REQUIRE(coordEq(ell->p1, 0, 0, 1));   // normal
    REQUIRE(coordEq(ell->p2, 3, 0, 0));   // majorAxis
    REQUIRE(ell->ratio == Approx(0.5));
}

TEST_CASE("extractAcisWireframe: plane/cone/torus surface params + loops", "[acis]") {
    DRW_AcisBrep g = extractModel();

    const DRW_AcisFace* plane = findFace(g, DRW_AcisSurface::Plane);
    REQUIRE(plane != nullptr);
    REQUIRE(plane->hasSurface);
    REQUIRE(coordEq(plane->p0, 0, 0, 0));   // origin
    REQUIRE(coordEq(plane->p1, 0, 0, 1));   // normal
    REQUIRE(plane->hasUDir);
    REQUIRE(coordEq(plane->p2, 1, 0, 0));   // uDir
    REQUIRE(plane->loops.size() == 1);
    REQUIRE(plane->loops[0].nodeIndex == 9);
    REQUIRE(plane->loops[0].coedgeCount == 1);

    const DRW_AcisFace* cone = findFace(g, DRW_AcisSurface::Cone);
    REQUIRE(cone != nullptr);
    REQUIRE(cone->hasSurface);
    REQUIRE(coordEq(cone->p0, 0, 0, 0));    // origin
    REQUIRE(coordEq(cone->p1, 0, 0, 1));    // axis
    REQUIRE(coordEq(cone->p2, 1, 0, 0));    // majorAxis
    REQUIRE(cone->ratio == Approx(0.5));
    REQUIRE(cone->sineAngle == Approx(0.3));
    REQUIRE(cone->cosineAngle == Approx(0.95));

    const DRW_AcisFace* torus = findFace(g, DRW_AcisSurface::Torus);
    REQUIRE(torus != nullptr);
    REQUIRE(torus->hasSurface);
    REQUIRE(coordEq(torus->p0, 0, 0, 0));   // center
    REQUIRE(coordEq(torus->p1, 0, 0, 1));   // axis (x_axis vec skipped)
    REQUIRE(torus->majorRadius == Approx(10));
    REQUIRE(torus->minorRadius == Approx(2));
}

TEST_CASE("extractAcisWireframe: surface-less face -> unknown + diagnostic", "[acis]") {
    DRW_AcisBrep g = extractModel();
    int unknownCount = 0;
    const DRW_AcisFace* unknown = nullptr;
    for (const auto& f : g.faces) {
        if (f.surfaceType == DRW_AcisSurface::Unknown) { ++unknownCount; unknown = &f; }
    }
    REQUIRE(unknownCount == 1);
    REQUIRE(unknown != nullptr);
    REQUIRE_FALSE(unknown->hasSurface);
    bool sawDiag = false;
    for (const auto& d : g.diagnostics) if (d.find("no surface") != std::string::npos) sawDiag = true;
    REQUIRE(sawDiag);
}

TEST_CASE("extractAcisWireframe: exact element counts", "[acis]") {
    DRW_AcisBrep g = extractModel();
    REQUIRE(g.vertices.size() == 4);
    REQUIRE(g.edges.size() == 2);
    REQUIRE(g.faces.size() == 4);

    // Curve-type histogram: one straight, one ellipse.
    int straight = 0, ellipse = 0, intcurve = 0, unknownCurve = 0;
    for (const auto& e : g.edges) {
        switch (e.curveType) {
        case DRW_AcisCurve::Straight: ++straight; break;
        case DRW_AcisCurve::Ellipse: ++ellipse; break;
        case DRW_AcisCurve::Intcurve: ++intcurve; break;
        case DRW_AcisCurve::Unknown: ++unknownCurve; break;
        }
    }
    REQUIRE(straight == 1);
    REQUIRE(ellipse == 1);
    REQUIRE(intcurve == 0);
    REQUIRE(unknownCurve == 0);

    // Surface-type histogram: plane/cone/torus + one unknown; one loop total.
    int plane = 0, cone = 0, torus = 0, sphere = 0, spline = 0, unknownSurf = 0, loops = 0;
    for (const auto& f : g.faces) {
        switch (f.surfaceType) {
        case DRW_AcisSurface::Plane: ++plane; break;
        case DRW_AcisSurface::Cone: ++cone; break;
        case DRW_AcisSurface::Torus: ++torus; break;
        case DRW_AcisSurface::Sphere: ++sphere; break;
        case DRW_AcisSurface::Spline: ++spline; break;
        case DRW_AcisSurface::Unknown: ++unknownSurf; break;
        }
        loops += static_cast<int>(f.loops.size());
    }
    REQUIRE(plane == 1);
    REQUIRE(cone == 1);
    REQUIRE(torus == 1);
    REQUIRE(sphere == 0);
    REQUIRE(spline == 0);
    REQUIRE(unknownSurf == 1);
    REQUIRE(loops == 1);
}

TEST_CASE("extractAcisWireframe: intcurve edge control points", "[acis]") {
    DRW_AcisModel m = drw_buildAcisModel(modelWithIntcurve());
    DRW_AcisBrep g;
    REQUIRE(drw_extractAcisWireframe(m, g));

    const DRW_AcisEdge* ic = findEdge(g, DRW_AcisCurve::Intcurve);
    REQUIRE(ic != nullptr);
    REQUIRE(ic->hasCurve);
    REQUIRE(ic->curveType == DRW_AcisCurve::Intcurve);
    // Endpoints still resolve from the two fresh vertices.
    REQUIRE(ic->hasStart);
    REQUIRE(ic->hasEnd);
    REQUIRE(coordEq(ic->start, 0, 0, 0));
    REQUIRE(coordEq(ic->end, 4, 0, 0));
    // The nubs control polygon is recovered (drw_acis intcurveControlPoints).
    REQUIRE(ic->controlPoints.size() == 2);
    REQUIRE(coordEq(ic->controlPoints[0], 7, 8, 9));
    REQUIRE(coordEq(ic->controlPoints[1], 10, 11, 12));

    // The base straight + ellipse edges are still present alongside the intcurve.
    REQUIRE(findEdge(g, DRW_AcisCurve::Straight) != nullptr);
    REQUIRE(findEdge(g, DRW_AcisCurve::Ellipse) != nullptr);
    REQUIRE(g.edges.size() == 3);
}

TEST_CASE("drw_decodeAcisWireframe: byte-true SAB decode -> vertex [1,2,3]", "[acis]") {
    std::vector<unsigned char> bytes = simpleSabBytes();

    // Parse layers directly.
    DRW_SabData sab;
    REQUIRE(drw_parseSab(bytes.data(), bytes.size(), sab));
    DRW_AcisModel m = drw_buildAcisModel(sab);
    REQUIRE(m.nodesOfType("vertex").size() == 1);

    DRW_AcisBrep g;
    REQUIRE(drw_decodeAcisWireframe(bytes, g));
    REQUIRE(g.vertices.size() == 1);
    REQUIRE(g.vertices[0].valid);
    REQUIRE(coordEq(g.vertices[0].point, 1, 2, 3));
    REQUIRE(g.edges.empty());
    REQUIRE(g.faces.empty());
    REQUIRE(g.hasBBox);
    REQUIRE(coordEq(g.bboxMin, 1, 2, 3));
    REQUIRE(coordEq(g.bboxMax, 1, 2, 3));
}

TEST_CASE("drw_decodeAcisWireframe: prefixed SAB payload is located by signature", "[acis]") {
    // A real DXF/DWG blob may carry leading bytes before the ACIS signature.
    std::vector<unsigned char> prefixed = { 0x00, 0xff, 0x42 };
    std::vector<unsigned char> sab = simpleSabBytes();
    prefixed.insert(prefixed.end(), sab.begin(), sab.end());

    DRW_AcisBrep g;
    REQUIRE(drw_decodeAcisWireframe(prefixed, g));
    REQUIRE(g.vertices.size() == 1);
    REQUIRE(coordEq(g.vertices[0].point, 1, 2, 3));
}

TEST_CASE("drw_decodeAcisWireframe: null-safety on empty/garbage input", "[acis]") {
    DRW_AcisBrep g1;
    REQUIRE_FALSE(drw_decodeAcisWireframe(std::vector<unsigned char>{}, g1));
    REQUIRE(g1.empty());

    DRW_AcisBrep g2;
    REQUIRE_FALSE(drw_decodeAcisWireframe(std::vector<unsigned char>{ 1, 2, 3, 4, 5 }, g2));
    REQUIRE(g2.empty());

    // Signature present but truncated body -> parse fails, empty, no throw.
    DRW_AcisBrep g3;
    std::vector<unsigned char> truncated;
    pushAscii(truncated, "ACIS BinaryFile");
    truncated.push_back(0x01);   // one stray byte, not enough for the header ints
    REQUIRE_FALSE(drw_decodeAcisWireframe(truncated, g3));
    REQUIRE(g3.empty());
}

TEST_CASE("drw_parseSab: null-safety on null pointer", "[acis]") {
    DRW_SabData sab;
    REQUIRE_FALSE(drw_parseSab(nullptr, 0, sab));
}

// ── Render tests: DRW_AcisBrep -> RS_* entities (4.8b) ───────────────────────
// Exercise RS_FilterDXFRW::acisWireframeToEntities directly against a bare
// RS_EntityContainer (no DXF import / no m_graphic), asserting entity COUNT,
// TYPES and key geometry. Breps are hand-built so geometry is exact.

namespace {

DRW_AcisEdge straightEdge(const DRW_Coord& a, const DRW_Coord& b) {
    DRW_AcisEdge e;
    e.curveType = DRW_AcisCurve::Straight;
    e.hasStart = e.hasEnd = true;
    e.start = a; e.end = b;
    e.hasCurve = true;
    return e;
}

} // namespace

TEST_CASE("acisWireframeToEntities: straight edge -> RS_Line", "[acis][render]") {
    DRW_AcisBrep brep;
    brep.edges.push_back(straightEdge(DRW_Coord(1, 2, 7), DRW_Coord(4, 6, 9)));

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 1);
    REQUIRE(container.count() == 1);
    REQUIRE(ents[0]->rtti() == RS2::EntityLine);
    auto* line = static_cast<RS_Line*>(ents[0]);
    // Z is dropped (projection to 2D), like addMesh.
    REQUIRE(line->getStartpoint().x == Approx(1));
    REQUIRE(line->getStartpoint().y == Approx(2));
    REQUIRE(line->getEndpoint().x == Approx(4));
    REQUIRE(line->getEndpoint().y == Approx(6));
}

TEST_CASE("acisWireframeToEntities: ellipse edge -> RS_Ellipse with derived angles",
          "[acis][render]") {
    DRW_AcisBrep brep;
    DRW_AcisEdge e;
    e.curveType = DRW_AcisCurve::Ellipse;
    e.hasCurve = true;
    e.p0 = DRW_Coord(0, 0, 0);        // center
    e.p1 = DRW_Coord(0, 0, 1);        // normal
    e.p2 = DRW_Coord(4, 0, 0);        // major axis vector
    e.ratio = 0.5;                    // minor = (0, 2)
    e.hasStart = e.hasEnd = true;
    e.start = DRW_Coord(4, 0, 0);     // param 0 (tip of major axis)
    e.end = DRW_Coord(0, 2, 0);       // param pi/2 (tip of minor axis)
    brep.edges.push_back(e);

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 1);
    REQUIRE(ents[0]->rtti() == RS2::EntityEllipse);
    auto* ell = static_cast<RS_Ellipse*>(ents[0]);
    REQUIRE(ell->getCenter().x == Approx(0));
    REQUIRE(ell->getCenter().y == Approx(0));
    REQUIRE(ell->getMajorP().x == Approx(4));
    REQUIRE(ell->getMajorP().y == Approx(0));
    REQUIRE(ell->getRatio() == Approx(0.5));
    REQUIRE(ell->getAngle1() == Approx(0.0));
    REQUIRE(ell->getAngle2() == Approx(M_PI / 2.0));
}

TEST_CASE("acisWireframeToEntities: degenerate ellipse -> RS_Line fallback",
          "[acis][render]") {
    DRW_AcisBrep brep;
    DRW_AcisEdge e;
    e.curveType = DRW_AcisCurve::Ellipse;
    e.hasCurve = true;
    e.p0 = DRW_Coord(0, 0, 0);
    e.p2 = DRW_Coord(0, 0, 0);        // zero-length major axis -> degenerate
    e.ratio = 0.0;
    e.hasStart = e.hasEnd = true;
    e.start = DRW_Coord(1, 1, 0);
    e.end = DRW_Coord(5, 3, 0);
    brep.edges.push_back(e);

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 1);
    REQUIRE(ents[0]->rtti() == RS2::EntityLine);
    auto* line = static_cast<RS_Line*>(ents[0]);
    REQUIRE(line->getStartpoint().x == Approx(1));
    REQUIRE(line->getEndpoint().x == Approx(5));
}

TEST_CASE("acisWireframeToEntities: intcurve edge -> RS_Spline through control polygon",
          "[acis][render]") {
    DRW_AcisBrep brep;
    DRW_AcisEdge e;
    e.curveType = DRW_AcisCurve::Intcurve;
    e.hasCurve = true;
    e.hasStart = e.hasEnd = true;
    e.start = DRW_Coord(0, 0, 0);
    e.end = DRW_Coord(4, 0, 0);
    e.controlPoints = {
        DRW_Coord(0, 0, 0), DRW_Coord(1, 2, 0),
        DRW_Coord(3, 1, 0), DRW_Coord(4, 0, 0),
    };
    brep.edges.push_back(e);

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 1);
    REQUIRE(ents[0]->rtti() == RS2::EntitySpline);
    auto* spline = static_cast<RS_Spline*>(ents[0]);
    REQUIRE(spline->getNumberOfControlPoints() == 4);
    std::vector<RS_Vector> cps = spline->getControlPoints();
    REQUIRE(cps.size() == 4);
    REQUIRE(cps.front().x == Approx(0));
    REQUIRE(cps.back().x == Approx(4));
}

TEST_CASE("acisWireframeToEntities: intcurve without control polygon -> RS_Line fallback",
          "[acis][render]") {
    DRW_AcisBrep brep;
    DRW_AcisEdge e;
    e.curveType = DRW_AcisCurve::Intcurve;
    e.hasStart = e.hasEnd = true;
    e.start = DRW_Coord(2, 2, 0);
    e.end = DRW_Coord(7, 8, 0);
    // controlPoints intentionally empty
    brep.edges.push_back(e);

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 1);
    REQUIRE(ents[0]->rtti() == RS2::EntityLine);
}

TEST_CASE("acisWireframeToEntities: isolated vertex -> RS_Point; edge vertices skipped",
          "[acis][render]") {
    DRW_AcisBrep brep;
    brep.edges.push_back(straightEdge(DRW_Coord(1, 2, 0), DRW_Coord(4, 6, 0)));

    auto vtx = [](double x, double y, double z) {
        DRW_AcisVertex v; v.valid = true; v.point = DRW_Coord(x, y, z); return v;
    };
    brep.vertices.push_back(vtx(1, 2, 0));   // on the edge start -> no point
    brep.vertices.push_back(vtx(4, 6, 0));   // on the edge end   -> no point
    brep.vertices.push_back(vtx(9, 9, 0));   // isolated          -> RS_Point

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 2);   // one line + one isolated point
    REQUIRE(ents[0]->rtti() == RS2::EntityLine);
    REQUIRE(ents[1]->rtti() == RS2::EntityPoint);
    auto* pt = static_cast<RS_Point*>(ents[1]);
    REQUIRE(pt->getStartpoint().x == Approx(9));
    REQUIRE(pt->getStartpoint().y == Approx(9));
}

TEST_CASE("acisWireframeToEntities: mixed brep entity count + type histogram",
          "[acis][render]") {
    DRW_AcisBrep brep;
    // straight
    brep.edges.push_back(straightEdge(DRW_Coord(0, 0, 0), DRW_Coord(1, 0, 0)));
    // ellipse
    DRW_AcisEdge el;
    el.curveType = DRW_AcisCurve::Ellipse;
    el.p0 = DRW_Coord(0, 0, 0); el.p2 = DRW_Coord(2, 0, 0); el.ratio = 0.5;
    el.hasStart = el.hasEnd = true;
    el.start = DRW_Coord(2, 0, 0); el.end = DRW_Coord(0, 1, 0);
    brep.edges.push_back(el);
    // intcurve (spline)
    DRW_AcisEdge ic;
    ic.curveType = DRW_AcisCurve::Intcurve;
    ic.controlPoints = { DRW_Coord(0, 0, 0), DRW_Coord(1, 1, 0), DRW_Coord(2, 0, 0) };
    brep.edges.push_back(ic);
    // isolated vertex
    DRW_AcisVertex v; v.valid = true; v.point = DRW_Coord(10, 10, 0);
    brep.vertices.push_back(v);

    RS_EntityContainer container;
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, &container);

    REQUIRE(ents.size() == 4);
    REQUIRE(container.count() == 4);
    int lines = 0, ellipses = 0, splines = 0, points = 0;
    for (RS_Entity* e : ents) {
        switch (e->rtti()) {
        case RS2::EntityLine: ++lines; break;
        case RS2::EntityEllipse: ++ellipses; break;
        case RS2::EntitySpline: ++splines; break;
        case RS2::EntityPoint: ++points; break;
        default: break;
        }
    }
    REQUIRE(lines == 1);
    REQUIRE(ellipses == 1);
    REQUIRE(splines == 1);
    REQUIRE(points == 1);
}

TEST_CASE("acisWireframeToEntities: null container is a safe no-op",
          "[acis][render]") {
    DRW_AcisBrep brep;
    brep.edges.push_back(straightEdge(DRW_Coord(0, 0, 0), DRW_Coord(1, 1, 0)));
    std::vector<RS_Entity*> ents =
        RS_FilterDXFRW::acisWireframeToEntities(brep, nullptr);
    REQUIRE(ents.empty());
}
