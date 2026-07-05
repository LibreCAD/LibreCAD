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

#include "drw_acis.h"

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

bool coordEq(const DRW_Coord& c, double x, double y, double z) {
    return c.x == Approx(x) && c.y == Approx(y) && c.z == Approx(z);
}

// ── Byte-true SAB builder (mirrors TS simpleSabBytes) ───────────────────────

void pushAscii(std::vector<std::uint8_t>& out, const std::string& s) {
    for (char c : s) out.push_back(static_cast<std::uint8_t>(c));
}
void pushInt(std::vector<std::uint8_t>& out, std::int32_t v) {
    std::uint8_t b[4];
    std::memcpy(b, &v, 4);   // little-endian host
    out.insert(out.end(), b, b + 4);
}
void pushDouble(std::vector<std::uint8_t>& out, double v) {
    std::uint8_t b[8];
    std::memcpy(b, &v, 8);
    out.insert(out.end(), b, b + 8);
}
void pushSabString(std::vector<std::uint8_t>& out, const std::string& s) {
    out.push_back(DRW_SabTag::Str);
    out.push_back(static_cast<std::uint8_t>(s.size()));
    pushAscii(out, s);
}
void pushSabDouble(std::vector<std::uint8_t>& out, double v) {
    out.push_back(DRW_SabTag::Double);
    pushDouble(out, v);
}
void pushEntityType(std::vector<std::uint8_t>& out, const std::string& s, bool extended = false) {
    out.push_back(extended ? DRW_SabTag::EntityTypeEx : DRW_SabTag::EntityType);
    out.push_back(static_cast<std::uint8_t>(s.size()));
    pushAscii(out, s);
}
void pushSabVec(std::vector<std::uint8_t>& out, int tag, double x, double y, double z) {
    out.push_back(static_cast<std::uint8_t>(tag));
    pushDouble(out, x);
    pushDouble(out, y);
    pushDouble(out, z);
}

std::vector<std::uint8_t> simpleSabBytes() {
    std::vector<std::uint8_t> out;
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

TEST_CASE("drw_decodeAcisWireframe: byte-true SAB decode -> vertex [1,2,3]", "[acis]") {
    std::vector<std::uint8_t> bytes = simpleSabBytes();

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
    std::vector<std::uint8_t> prefixed = { 0x00, 0xff, 0x42 };
    std::vector<std::uint8_t> sab = simpleSabBytes();
    prefixed.insert(prefixed.end(), sab.begin(), sab.end());

    DRW_AcisBrep g;
    REQUIRE(drw_decodeAcisWireframe(prefixed, g));
    REQUIRE(g.vertices.size() == 1);
    REQUIRE(coordEq(g.vertices[0].point, 1, 2, 3));
}

TEST_CASE("drw_decodeAcisWireframe: null-safety on empty/garbage input", "[acis]") {
    DRW_AcisBrep g1;
    REQUIRE_FALSE(drw_decodeAcisWireframe(std::vector<std::uint8_t>{}, g1));
    REQUIRE(g1.empty());

    DRW_AcisBrep g2;
    REQUIRE_FALSE(drw_decodeAcisWireframe(std::vector<std::uint8_t>{ 1, 2, 3, 4, 5 }, g2));
    REQUIRE(g2.empty());

    // Signature present but truncated body -> parse fails, empty, no throw.
    DRW_AcisBrep g3;
    std::vector<std::uint8_t> truncated;
    pushAscii(truncated, "ACIS BinaryFile");
    truncated.push_back(0x01);   // one stray byte, not enough for the header ints
    REQUIRE_FALSE(drw_decodeAcisWireframe(truncated, g3));
    REQUIRE(g3.empty());
}

TEST_CASE("drw_parseSab: null-safety on null pointer", "[acis]") {
    DRW_SabData sab;
    REQUIRE_FALSE(drw_parseSab(nullptr, 0, sab));
}
