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
**********************************************************************/

/**
 * DWG ACDBASSOC* associativity-object read test (suffix-inferred decode).
 *
 * Exercises DRW_AssociativeObject::parseDwg via the widened OBJECTS-section
 * dispatch (dwgreader.cpp: rn.rfind("ACDBASSOC", 0) == 0 ||
 * rn == "ACDBPERSSUBENTMANAGER"). Every ACDBASSOC* subclass now routes to the
 * shell parser and is delivered via addAssociativeObject with m_recordName (the
 * associativity class name) set, mirroring dwgTs readAssociativityBody:
 *   *ACTIONPARAM                     -> AcDbAssocActionParam prefix decoded
 *   *DEPENDENCY / *VALUEDEPENDENCY   -> AcDbAssocDependency body decoded
 *   *ACTION / *NETWORK / *VARIABLE /
 *   *2DCONSTRAINTGROUP               -> AcDbAssocAction prefix decoded
 *   any other (surface/array action
 *   bodies, ...)                     -> common prefix + Missing marker, body
 *                                       preserved verbatim by the raw shelf
 * The exact-name branches (NETWORK / GEOMDEPENDENCY / ALIGNEDDIMACTIONBODY /
 * VERTEX & OSNAP action params / PERSSUBENTMANAGER) keep their extended-field
 * decoders. parseDwg never returns false on a short read, so the object is
 * always delivered (typed + raw-shelved).
 *
 * Fixtures (in testdata):
 *   assoc_surface_r2004.dwg     <- ~/dev/libredwg/.../2004/Surface.dwg (AC1018)
 *       ACDBASSOCEDGEACTIONPARAM, ACDBASSOCPATHACTIONPARAM (generic action
 *       params), ACDBASSOC{EXTRUDED,LOFTED,PLANE,REVOLVED,SWEPT}SURFACEACTIONBODY
 *       (raw-shelf fallback), ACDBASSOCACTION/DEPENDENCY/NETWORK/PERSSUBENTMANAGER.
 *   assoc_constraints_r2013.dwg <- ~/dev/libredwg/.../2013/Constraints.dwg (AC1027)
 *       ACDBASSOC2DCONSTRAINTGROUP (generic action body), ACDBASSOCNETWORK,
 *       ACDBASSOCGEOMDEPENDENCY.
 *
 * Oracle: LibreDWG dwgread -O JSON CLASSES/OBJECTS enumeration. The key parity
 * assertion is reachability (all subclasses typed-captured, not raw-only) with
 * no stream desync: getError()==BAD_NONE and 0 entity/object parse failures.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual.
class StubInterface : public DRW_Interface {
public:
  void addHeader(const DRW_Header *) override {}
  void addLType(const DRW_LType &) override {}
  void addLayer(const DRW_Layer &) override {}
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addTextStyle(const DRW_Textstyle &) override {}
  void addAppId(const DRW_AppId &) override {}
  void addBlock(const DRW_Block &) override {}
  void setBlock(const int) override {}
  void endBlock() override {}
  void addPoint(const DRW_Point &) override {}
  void addLine(const DRW_Line &) override {}
  void addRay(const DRW_Ray &) override {}
  void addXline(const DRW_Xline &) override {}
  void addArc(const DRW_Arc &) override {}
  void addCircle(const DRW_Circle &) override {}
  void addEllipse(const DRW_Ellipse &) override {}
  void addLWPolyline(const DRW_LWPolyline &) override {}
  void addPolyline(const DRW_Polyline &) override {}
  void addSpline(const DRW_Spline *) override {}
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &) override {}
  void addTrace(const DRW_Trace &) override {}
  void add3dFace(const DRW_3Dface &) override {}
  void addSolid(const DRW_Solid &) override {}
  void addMText(const DRW_MText &) override {}
  void addText(const DRW_Text &) override {}
  void addDimAlign(const DRW_DimAligned *) override {}
  void addDimLinear(const DRW_DimLinear *) override {}
  void addDimRadial(const DRW_DimRadial *) override {}
  void addDimDiametric(const DRW_DimDiametric *) override {}
  void addDimAngular(const DRW_DimAngular *) override {}
  void addDimAngular3P(const DRW_DimAngular3p *) override {}
  void addDimArc(const DRW_DimArc *) override {}
  void addDimOrdinate(const DRW_DimOrdinate *) override {}
  void addLeader(const DRW_Leader *) override {}
  void addHatch(const DRW_Hatch *) override {}
  void addViewport(const DRW_Viewport &) override {}
  void addImage(const DRW_Image *) override {}
  void addWipeout(const DRW_Wipeout *) override {}
  void addMLeader(const DRW_MLeader *) override {}
  void addMLeaderStyle(const DRW_MLeaderStyle *) override {}
  void linkImage(const DRW_ImageDef *) override {}
  void addComment(const char *) override {}
  void addPlotSettings(const DRW_PlotSettings *) override {}
  void writeHeader(DRW_Header &) override {}
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

// Collects every ACDBASSOC* object delivered by the OBJECTS-section dispatch.
class AssocCapture : public StubInterface {
public:
  std::vector<DRW_AssociativeObject> m_objs;
  void addAssociativeObject(const DRW_AssociativeObject &o) override {
    m_objs.push_back(o);
  }

  size_t countRecord(const std::string &rn) const {
    size_t n = 0;
    for (const auto &o : m_objs)
      if (o.m_recordName == rn)
        ++n;
    return n;
  }

  const DRW_AssociativeObject *firstRecord(const std::string &rn) const {
    for (const auto &o : m_objs)
      if (o.m_recordName == rn)
        return &o;
    return nullptr;
  }

  std::set<std::string> recordNames() const {
    std::set<std::string> s;
    for (const auto &o : m_objs)
      s.insert(o.m_recordName);
    return s;
  }

  bool hasPrefixKind(const DRW_AssociativeObject &o,
                     DRW_AssociativePrefixStatus::Kind kind) const {
    for (const auto &p : o.m_prefixStatuses)
      if (p.m_kind == kind)
        return true;
    return false;
  }
};

// Reads a fixture; returns false (with SUCCEED-skip) if it is missing or the
// read fails outright. On success, asserts a clean read with no desync.
bool tryReadAssoc(const std::string &path, AssocCapture &cap) {
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("fixture not found; skipping: " << path);
    return false;
  }
  dwgR reader(path.c_str());
  // Fixture is present (passed the is_regular_file gate above), so a read
  // failure is a real regression -- REQUIRE it, do not SUCCEED-skip.
  REQUIRE(reader.read(&cap, /*ext=*/true));
  // No stream desync: the OBJECTS section fully parsed and every entity /
  // object record was decoded (the widened ACDBASSOC* routing must not
  // introduce failures — bodies that lack a typed decoder are raw-shelved,
  // never counted as failures).
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  CHECK(reader.getEntityParseFailures() == 0u);
  CHECK(reader.getObjectParseFailures() == 0u);
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG ACDBASSOC* surface subclasses typed-captured (Surface / AC1018)",
          "[dwg][assoc][parity]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/assoc_surface_r2004.dwg";
  AssocCapture cap;
  if (!tryReadAssoc(path, cap))
    return;

  REQUIRE(cap.m_objs.size() >= 1);

  // Every captured associative object carries its class name (queryable kind).
  for (const auto &o : cap.m_objs)
    CHECK_FALSE(o.m_recordName.empty());

  // Generic action-param subclasses now decode the shared AcDbAssocActionParam
  // prefix instead of falling to a bare raw shell.
  CHECK(cap.countRecord("ACDBASSOCEDGEACTIONPARAM") >= 1);
  CHECK(cap.countRecord("ACDBASSOCPATHACTIONPARAM") >= 1);
  const DRW_AssociativeObject *edge =
      cap.firstRecord("ACDBASSOCEDGEACTIONPARAM");
  REQUIRE(edge != nullptr);
  CHECK(edge->m_actionParamPrefixParsed);
  CHECK(cap.hasPrefixKind(
      *edge, DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam));

  // Surface action bodies have no typed decoder: common prefix + Missing
  // marker, body preserved by the raw shelf. Reachable (typed-captured), not
  // dropped.
  static const char *kSurfaceBodies[] = {
      "ACDBASSOCEXTRUDEDSURFACEACTIONBODY",
      "ACDBASSOCLOFTEDSURFACEACTIONBODY",
      "ACDBASSOCPLANESURFACEACTIONBODY",
      "ACDBASSOCREVOLVEDSURFACEACTIONBODY",
      "ACDBASSOCSWEPTSURFACEACTIONBODY",
  };
  for (const char *rn : kSurfaceBodies) {
    CHECK(cap.countRecord(rn) >= 1);
    const DRW_AssociativeObject *body = cap.firstRecord(rn);
    REQUIRE(body != nullptr);
    CHECK(cap.hasPrefixKind(
        *body, DRW_AssociativePrefixStatus::Kind::AcDbAssocActionBody));
  }

  // Generic dependency + action + kept exact-name branches still deliver.
  CHECK(cap.countRecord("ACDBASSOCDEPENDENCY") >= 1);
  CHECK(cap.countRecord("ACDBASSOCACTION") >= 1);
  CHECK(cap.countRecord("ACDBASSOCNETWORK") >= 1);
  CHECK(cap.countRecord("ACDBASSOCPERSSUBENTMANAGER") >= 1);

  // The dependency body actually decoded (AcDbAssocDependency prefix present).
  const DRW_AssociativeObject *dep = cap.firstRecord("ACDBASSOCDEPENDENCY");
  REQUIRE(dep != nullptr);
  CHECK(cap.hasPrefixKind(
      *dep, DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG ACDBASSOC2DCONSTRAINTGROUP typed-captured (Constraints / AC1027)",
          "[dwg][assoc][parity]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/assoc_constraints_r2013.dwg";
  AssocCapture cap;
  if (!tryReadAssoc(path, cap))
    return;

  REQUIRE(cap.m_objs.size() >= 1);

  // The 2d-constraint group is action-bodied: routed via the ACTION-suffix /
  // 2DCONSTRAINTGROUP arm, decoding the AcDbAssocAction prefix.
  CHECK(cap.countRecord("ACDBASSOC2DCONSTRAINTGROUP") >= 1);
  const DRW_AssociativeObject *grp =
      cap.firstRecord("ACDBASSOC2DCONSTRAINTGROUP");
  REQUIRE(grp != nullptr);
  CHECK_FALSE(grp->m_recordName.empty());
  CHECK(cap.hasPrefixKind(
      *grp, DRW_AssociativePrefixStatus::Kind::AcDbAssocAction));

  // Exact-name branches still deliver on this fixture.
  CHECK(cap.countRecord("ACDBASSOCNETWORK") >= 1);
  CHECK(cap.countRecord("ACDBASSOCGEOMDEPENDENCY") >= 1);
  const DRW_AssociativeObject *geom =
      cap.firstRecord("ACDBASSOCGEOMDEPENDENCY");
  REQUIRE(geom != nullptr);
  CHECK(cap.hasPrefixKind(
      *geom, DRW_AssociativePrefixStatus::Kind::AcDbAssocGeomDependency));
}
