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

// Closed and periodic SPLINE entities read through RS_FilterDXFRW.
//
// A SPLINE is the curve its control points, weights and knots draw over
// [knots[degree], knots[count]], whatever its closed (group 70 bit 0) and
// periodic (bit 1) flags say. AutoCAD writes closed and periodic splines with a
// clamped knot vector and the last control point on the first. addSpline()
// made every flagged spline Standard and closed it with setClosed(), which
// wrapped those clamped control points into a periodic spline: a different
// curve, and whenever the first inner knot is repeated (as it is for AutoCAD's
// fit-point splines) one that RS_Spline::validate() rejects, so that nothing
// was drawn. 888 of the 1044 splines of one DWG sample were invisible.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <QCoreApplication>

#include "drw_entities.h"
#include "lc_splinepoints.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_spline.h"

namespace {

void ensureSettings() {
  static int argc = 1;
  static char arg0[] = "librecad_tests";
  static char *argv[] = {arg0, nullptr};
  static QCoreApplication *app = QCoreApplication::instance()
                                     ? QCoreApplication::instance()
                                     : new QCoreApplication(argc, argv);
  static bool ready = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)app;
  (void)ready;
}

std::string tempPath(const std::string &name) {
  return (std::filesystem::temp_directory_path() / ("closed_spline_" + name)).string();
}

/** What a SPLINE record carries. */
struct SplineRecord {
  int m_flags = 0;
  int m_degree = 3;
  std::vector<RS_Vector> m_controlPoints;
  std::vector<double> m_knots;
};

/** A DXF holding one SPLINE; no HEADER, so no $ACADVER. */
std::string splineDxf(const SplineRecord &spline) {
  std::ostringstream out;
  out.precision(17);
  out << "0\nSECTION\n2\nENTITIES\n0\nSPLINE\n8\n0\n100\nAcDbEntity\n100\nAcDbSpline\n"
      << "70\n" << spline.m_flags << "\n71\n" << spline.m_degree << "\n72\n" << spline.m_knots.size()
      << "\n73\n" << spline.m_controlPoints.size() << "\n74\n0\n42\n1e-10\n43\n1e-10\n";
  for (const double k : spline.m_knots)
    out << "40\n" << k << "\n";
  for (const RS_Vector &p : spline.m_controlPoints)
    out << "10\n" << p.x << "\n20\n" << p.y << "\n30\n0\n";
  out << "0\nENDSEC\n0\nEOF\n";
  return out.str();
}

/** Keeps the SPLINE records a file hands the filter. */
class SplineCaptureFilter : public RS_FilterDXFRW {
public:
  std::vector<SplineRecord> m_records;

  void addSpline(const DRW_Spline *data) override {
    SplineRecord record;
    record.m_flags = data->flags;
    record.m_degree = data->degree;
    for (const auto &p : data->controllist)
      record.m_controlPoints.emplace_back(p->x, p->y);
    record.m_knots = data->knotslist;
    m_records.push_back(record);
    RS_FilterDXFRW::addSpline(data);
  }
};

RS_Spline *onlySpline(RS_Graphic &graphic) {
  std::vector<RS_Spline *> splines;
  for (RS_Entity *e : graphic)
    if (e->rtti() == RS2::EntitySpline)
      splines.push_back(static_cast<RS_Spline *>(e));
  REQUIRE(splines.size() == 1);
  REQUIRE(graphic.count() == 1);
  return splines.front();
}

void importFile(RS_Graphic &graphic, const std::string &path, RS2::FormatType format,
                std::vector<SplineRecord> *records = nullptr) {
  graphic.initForNewDocument();
  SplineCaptureFilter filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path), format));
  if (records != nullptr)
    *records = filter.m_records;
}

void importDxf(RS_Graphic &graphic, const SplineRecord &spline, const std::string &name) {
  const std::string path = tempPath(name + ".dxf");
  {
    std::ofstream out(path);
    out << splineDxf(spline);
  }
  importFile(graphic, path, RS2::FormatDXFRW);
  std::filesystem::remove(path);
}

/** The curve of the file's data, evaluated as a plain B-spline. */
std::vector<RS_Vector> exactCurve(const SplineRecord &spline, int samples) {
  RS_SplineData data(spline.m_degree, false);
  data.controlPoints = spline.m_controlPoints;
  data.weights.assign(spline.m_controlPoints.size(), 1.0);
  data.knotslist = spline.m_knots;
  const double start = spline.m_knots[spline.m_degree];
  const double end = spline.m_knots[spline.m_controlPoints.size()];
  std::vector<RS_Vector> points;
  for (int i = 0; i <= samples; ++i)
    points.push_back(RS_Spline::evaluateNURBS(data, start + (end - start) * i / samples));
  return points;
}

/** The curve of an RS_Spline over its own parameter range. */
std::vector<RS_Vector> splineCurve(const RS_Spline &spline, int samples) {
  const RS_SplineData &data = spline.getData();
  const double start = data.knotslist[data.degree];
  const double end = data.knotslist[data.knotslist.size() - data.degree - 1];
  std::vector<RS_Vector> points;
  for (int i = 0; i <= samples; ++i)
    points.push_back(spline.getPointAt(start + (end - start) * i / samples));
  return points;
}

double distanceToPolyline(const RS_Vector &p, const std::vector<RS_Vector> &polyline) {
  double best = RS_MAXDOUBLE;
  for (size_t i = 0; i + 1 < polyline.size(); ++i) {
    const RS_Vector a = polyline[i];
    const RS_Vector ab = polyline[i + 1] - a;
    const double t = std::clamp(RS_Vector::dotP(p - a, ab) / std::max(ab.squared(), RS_TOLERANCE2), 0., 1.);
    best = std::min(best, (a + ab * t).distanceTo(p));
  }
  return best;
}

/** The largest distance between the two curves, sampled densely in both directions. */
double curveDistance(const std::vector<RS_Vector> &a, const std::vector<RS_Vector> &b) {
  double worst = 0.;
  for (const RS_Vector &p : a)
    worst = std::max(worst, distanceToPolyline(p, b));
  for (const RS_Vector &p : b)
    worst = std::max(worst, distanceToPolyline(p, a));
  return worst;
}

void checkSameVectors(const std::vector<RS_Vector> &actual, const std::vector<RS_Vector> &expected) {
  REQUIRE(actual.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    INFO("control point " << i);
    CHECK(actual[i].distanceTo(expected[i]) < 1e-12);
  }
}

void checkSameKnots(const std::vector<double> &actual, const std::vector<double> &expected) {
  REQUIRE(actual.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    INFO("knot " << i);
    CHECK(std::abs(actual[i] - expected[i]) < 1e-12);
  }
}

/** A closed loop of 8 control points, the last on the first. */
const std::vector<RS_Vector> kClampedLoop = {{0., 0.},   {10., -5.}, {20., 0.},  {25., 10.},
                                             {20., 20.}, {10., 25.}, {-5., 10.}, {0., 0.}};

/**
 * AutoCAD's closed spline: clamped knots, the first inner knot repeated as for
 * a spline AutoCAD fits through points.
 */
SplineRecord autocadClosed(int flags) {
  return {flags, 3, kClampedLoop, {0., 0., 0., 0., 0.25, 0.25, 0.5, 0.75, 1., 1., 1., 1.}};
}

/** 5 distinct control points of a periodic cubic. */
const std::vector<RS_Vector> kPeriodicPoints = {
    {0., 0.}, {20., -5.}, {30., 15.}, {15., 30.}, {-5., 15.}};

/**
 * A truly periodic spline: the first 3 control points repeated at the end,
 * uniform knots.
 */
SplineRecord periodicWrapped() {
  std::vector<RS_Vector> points = kPeriodicPoints;
  points.insert(points.end(), kPeriodicPoints.begin(), kPeriodicPoints.begin() + 3);
  return {11, 3, points, {0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11.}};
}

/**
 * The same spline as LibreCAD writes it since 75c48f9ac (unreleased master):
 * without the repeated control points, and with the first knots only.
 */
SplineRecord periodicAsLibreCadWrites() {
  return {11, 3, kPeriodicPoints, {0., 1., 2., 3., 4., 5., 6., 7., 8.}};
}

/** 6 control points of a closed spline drawn in LibreCAD, its first not on its last. */
const std::vector<RS_Vector> kOlderLibreCadPoints = {
    {55.25, 72.5}, {90., 110.}, {140., 95.}, {150., 40.}, {110., 5.}, {124.25, 16.}};

/**
 * A closed spline as LibreCAD 2.0 to 2.2.0 wrote it: group 70 = 11, the
 * control points unwrapped, and clamped knots: uniform ones for a spline drawn
 * in LibreCAD, the file's own for an imported spline closed with the Closed
 * check box. LibreCAD ignored those knots for a closed spline and drew the
 * uniform periodic spline of its control points.
 */
SplineRecord closedAsLibreCad220Wrote(std::vector<RS_Vector> points, std::vector<double> knots) {
  return {11, 3, std::move(points), std::move(knots)};
}

/** The uniform periodic cubic of @p points, wrapped: the curve LibreCAD 2.2.0 drew. */
SplineRecord uniformPeriodic(const std::vector<RS_Vector> &points, int flags = 11) {
  std::vector<RS_Vector> wrapped = points;
  wrapped.insert(wrapped.end(), points.begin(), points.begin() + 3);
  std::vector<double> knots(wrapped.size() + 4);
  for (size_t i = 0; i < knots.size(); ++i)
    knots[i] = static_cast<double>(i);
  return {flags, 3, wrapped, knots};
}

/** A hatch boundary's spline edge with the data of @p record. */
DRW_Spline hatchEdge(const SplineRecord &record) {
  DRW_Spline edge;
  edge.flags = record.m_flags;
  edge.degree = record.m_degree;
  edge.knotslist = record.m_knots;
  edge.nknots = static_cast<std::int32_t>(record.m_knots.size());
  for (const RS_Vector &p : record.m_controlPoints)
    edge.controllist.push_back(std::make_shared<DRW_Coord>(p.x, p.y, 0.));
  edge.ncontrol = static_cast<std::int32_t>(record.m_controlPoints.size());
  return edge;
}

} // namespace

TEST_CASE("A closed spline AutoCAD writes clamped keeps its curve",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  for (const int flags : {1, 9, 3, 11}) {
    INFO("group 70 flags " << flags);
    const SplineRecord source = autocadClosed(flags);
    RS_Graphic graphic;
    importDxf(graphic, source, "clamped_" + std::to_string(flags));
    RS_Spline *spline = onlySpline(graphic);

    // drawn at all
    CHECK(spline->validate());
    CHECK(spline->count() > 0);

    // as the clamped spline the data is, its ends meeting
    CHECK(spline->getData().type == RS_SplineData::SplineType::ClampedOpen);
    CHECK_FALSE(spline->isClosed());
    checkSameVectors(spline->getControlPoints(), source.m_controlPoints);
    checkSameKnots(spline->getKnotVector(), source.m_knots);
    CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(source, 2000)) < 1e-9);
    CHECK(spline->getData().m_closedFlag == ((flags & 0x1) != 0));
    CHECK(spline->getData().m_periodicFlag == ((flags & 0x2) != 0));
  }
}

TEST_CASE("A closed spline AutoCAD writes with uniform clamped knots keeps its curve",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  // validate() accepted what setClosed() made of this one, but that periodic
  // spline was another curve
  const SplineRecord source{9, 3, kClampedLoop, {0., 0., 0., 0., 0.2, 0.4, 0.6, 0.8, 1., 1., 1., 1.}};
  RS_Graphic graphic;
  importDxf(graphic, source, "clamped_uniform");
  RS_Spline *spline = onlySpline(graphic);

  CHECK(spline->validate());
  CHECK(spline->count() > 0);
  checkSameVectors(spline->getControlPoints(), source.m_controlPoints);
  checkSameKnots(spline->getKnotVector(), source.m_knots);
  CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(source, 2000)) < 1e-9);
}

TEST_CASE("A periodic spline with wrapped control points is read as a closed spline",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  const SplineRecord source = periodicWrapped();
  RS_Graphic graphic;
  importDxf(graphic, source, "periodic_wrapped");
  RS_Spline *spline = onlySpline(graphic);

  CHECK(spline->validate());
  CHECK(spline->count() > 0);
  CHECK(spline->isClosed());
  // the data is already what WrappedClosed keeps: not wrapped a second time
  checkSameVectors(spline->getControlPoints(), kPeriodicPoints);
  checkSameVectors(spline->getData().controlPoints, source.m_controlPoints);
  checkSameKnots(spline->getData().knotslist, source.m_knots);
  CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(source, 2000)) < 1e-9);
}

TEST_CASE("A closed spline LibreCAD wrote is still read as a closed spline",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  RS_Graphic graphic;
  importDxf(graphic, periodicAsLibreCadWrites(), "librecad_closed");
  RS_Spline *spline = onlySpline(graphic);

  CHECK(spline->validate());
  CHECK(spline->count() > 0);
  CHECK(spline->isClosed());
  const SplineRecord wrapped = periodicWrapped();
  checkSameVectors(spline->getData().controlPoints, wrapped.m_controlPoints);
  checkSameKnots(spline->getData().knotslist, wrapped.m_knots);
  CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(wrapped, 2000)) < 1e-9);
}

TEST_CASE("A closed spline LibreCAD 2.2.0 wrote is read as the curve it drew",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  const std::vector<RS_Vector> four(kOlderLibreCadPoints.begin(), kOlderLibreCadPoints.begin() + 4);
  struct Case {
    std::string name;
    SplineRecord source;
    std::vector<RS_Vector> points;
  };
  const std::vector<Case> cases{
      {"drawn", closedAsLibreCad220Wrote(kOlderLibreCadPoints, {0., 0., 0., 0., 1., 2., 3., 3., 3., 3.}),
       kOlderLibreCadPoints},
      {"drawn_4_points", closedAsLibreCad220Wrote(four, {0., 0., 0., 0., 1., 1., 1., 1.}), four},
      {"imported_then_closed",
       closedAsLibreCad220Wrote(kOlderLibreCadPoints, {0., 0., 0., 0., 10., 50., 60., 60., 60., 60.}),
       kOlderLibreCadPoints},
  };
  for (const Case &c : cases) {
    INFO(c.name);
    RS_Graphic graphic;
    importDxf(graphic, c.source, "librecad_2_2_0_" + c.name);
    RS_Spline *spline = onlySpline(graphic);

    CHECK(spline->validate());
    CHECK(spline->count() > 0);
    CHECK(spline->isClosed());
    CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(uniformPeriodic(c.points), 2000)) < 1e-9);
  }
}

TEST_CASE("A closed spline LibreCAD 2.2.1 wrote is read as a closed spline",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  // released 2.2.1 writes the control points wrapped, with uniform knots and
  // flags 0x1011
  const SplineRecord source = uniformPeriodic(kOlderLibreCadPoints, 4113);
  RS_Graphic graphic;
  importDxf(graphic, source, "librecad_2_2_1");
  RS_Spline *spline = onlySpline(graphic);

  CHECK(spline->validate());
  CHECK(spline->count() > 0);
  CHECK(spline->isClosed());
  checkSameVectors(spline->getData().controlPoints, source.m_controlPoints);
  CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(source, 2000)) < 1e-9);
}

TEST_CASE("An open spline with unclamped knots is drawn as its data",
          "[dxf][filter][spline][regression]") {
  ensureSettings();

  // Master read every open spline as ClampedOpen, which validate() rejects
  // for these knots, so that nothing was drawn.
  struct Case {
    std::string name;
    SplineRecord source;
  };
  const std::vector<RS_Vector> points = {{0., 0.}, {10., 15.}, {20., -5.}, {30., 10.}, {40., 0.}, {50., 20.}};
  const std::vector<Case> cases{
      {"unclamped", {8, 3, points, {0., 1., 2., 3., 4., 5., 6., 7., 8., 9.}}},
      {"clamped_at_start", {8, 3, points, {0., 0., 0., 0., 1., 3., 4., 6., 7., 9.}}},
  };
  for (const Case &c : cases) {
    INFO(c.name);
    RS_Graphic graphic;
    importDxf(graphic, c.source, "open_" + c.name);
    RS_Spline *spline = onlySpline(graphic);

    CHECK(spline->validate());
    CHECK(spline->count() > 0);
    CHECK_FALSE(spline->isClosed());
    CHECK(curveDistance(splineCurve(*spline, 2000), exactCurve(c.source, 2000)) < 1e-9);
  }
}

TEST_CASE("A closed hatch boundary spline edge is sampled on its curve",
          "[dxf][filter][spline][hatch][regression]") {
  ensureSettings();

  // DXF sets bit 0 for a closed edge, DWG bit 1 for a periodic one
  for (const int flags : {1, 2}) {
    INFO("flags " << flags);
    const SplineRecord source = autocadClosed(flags);
    const DRW_Spline data = hatchEdge(source);
    std::unique_ptr<LC_SplinePoints> edge{RS_FilterDXFRW::buildHatchSplineEdge(nullptr, &data)};
    REQUIRE(edge != nullptr);
    CHECK(edge->isClosed());
    const std::vector<RS_Vector> curve = exactCurve(source, 20000);
    REQUIRE(edge->getPoints().size() > 16);
    for (const RS_Vector &p : edge->getPoints()) {
      INFO("sample " << p);
      CHECK(distanceToPolyline(p, curve) < 1e-6);
    }
  }
}

TEST_CASE("A cubic hatch boundary spline edge with too few control points passes through them",
          "[dxf][filter][spline][hatch]") {
  ensureSettings();

  // a cubic needs 4 control points; evaluating this one read outside its
  // arrays. With 2 points the edge is their segment.
  const SplineRecord source{0, 3, {{0., 0.}, {10., 5.}}, {0., 0., 0., 0., 1., 1.}};
  const DRW_Spline data = hatchEdge(source);
  std::unique_ptr<LC_SplinePoints> edge{RS_FilterDXFRW::buildHatchSplineEdge(nullptr, &data)};
  REQUIRE(edge != nullptr);
  checkSameVectors(edge->getPoints(), source.m_controlPoints);
}

TEST_CASE("Closed splines survive a DXF and DWG round trip",
          "[dxf][dwg][filter][spline][roundtrip]") {
  ensureSettings();

  struct Case {
    std::string name;
    SplineRecord source;
  };
  const std::vector<Case> cases{{"closed", autocadClosed(9)},
                                {"periodic_clamped", autocadClosed(11)},
                                {"open_meeting_ends", autocadClosed(8)},
                                {"periodic_wrapped", periodicWrapped()},
                                {"librecad_closed", periodicAsLibreCadWrites()},
                                {"librecad_2_2_0_closed",
                                 closedAsLibreCad220Wrote(kOlderLibreCadPoints,
                                                          {0., 0., 0., 0., 1., 2., 3., 3., 3., 3.})},
                                {"librecad_2_2_1_closed", uniformPeriodic(kOlderLibreCadPoints, 4113)}};
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, ".dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, ".dwg"},
      {RS2::FormatDWG2013, "_2013.dwg"},
#endif
  };

  for (const Case &c : cases) {
    RS_Graphic first;
    importDxf(first, c.source, "rt_" + c.name);
    const RS_SplineData imported = onlySpline(first)->getData();

    for (const auto &[format, suffix] : formats) {
      INFO(c.name << suffix);
      const std::string path = tempPath("rt_" + c.name + suffix);
      std::remove(path.c_str());
      {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(first, QString::fromStdString(path), format));
      }
      RS_Graphic second;
      std::vector<SplineRecord> written;
      importFile(second, path, format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG,
                 &written);
      std::remove(path.c_str());

      // what the file says: a spline LibreCAD keeps WrappedClosed is written
      // closed and periodic, any other with the flags it was read with
      REQUIRE(written.size() == 1);
      const int flags = imported.type == RS_SplineData::SplineType::WrappedClosed
                            ? 0x3
                            : (c.source.m_flags & 0x3);
      CHECK((written.front().m_flags & 0x3) == flags);
      if (imported.type != RS_SplineData::SplineType::WrappedClosed) {
        checkSameVectors(written.front().m_controlPoints, c.source.m_controlPoints);
        checkSameKnots(written.front().m_knots, c.source.m_knots);
      }

      // and what LibreCAD makes of it again
      RS_Spline *spline = onlySpline(second);
      CHECK(spline->validate());
      CHECK(spline->count() > 0);
      CHECK(spline->getData().type == imported.type);
      checkSameVectors(spline->getData().controlPoints, imported.controlPoints);
      checkSameKnots(spline->getData().knotslist, imported.knotslist);
      CHECK(spline->getData().m_closedFlag == imported.m_closedFlag);
      CHECK(spline->getData().m_periodicFlag == imported.m_periodicFlag);
    }
  }
}

TEST_CASE("A spline read closed whose ends are moved apart is written open",
          "[dxf][filter][spline][roundtrip]") {
  ensureSettings();

  RS_Graphic first;
  importDxf(first, autocadClosed(11), "opened");
  RS_Spline *spline = onlySpline(first);
  spline->setControlPoint(7, RS_Vector(1., 1.));

  const std::string path = tempPath("opened_out.dxf");
  std::remove(path.c_str());
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(first, QString::fromStdString(path), RS2::FormatDXFRW));
  }
  RS_Graphic second;
  std::vector<SplineRecord> written;
  importFile(second, path, RS2::FormatDXFRW, &written);
  std::remove(path.c_str());
  REQUIRE(written.size() == 1);
  CHECK((written.front().m_flags & 0x3) == 0);
}

TEST_CASE("A closed spline written to R12 is a polyline on its curve",
          "[dxf][filter][spline][r12]") {
  ensureSettings();

  // away from the origin, where the polyline of an open spline used to end
  SplineRecord source = autocadClosed(9);
  for (RS_Vector &p : source.m_controlPoints)
    p += RS_Vector(100., 50.);
  RS_Graphic first;
  importDxf(first, source, "r12_source");

  const std::string path = tempPath("r12_out.dxf");
  std::remove(path.c_str());
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(first, QString::fromStdString(path), RS2::FormatDXFRW12));
  }
  RS_Graphic second;
  importFile(second, path, RS2::FormatDXFRW);
  std::remove(path.c_str());

  const std::vector<RS_Vector> curve = exactCurve(source, 20000);
  std::vector<RS_Vector> vertices;
  for (RS_Entity *e : second) {
    REQUIRE(e->rtti() == RS2::EntityPolyline);
    // closed, as the R2000 and later SPLINE is
    CHECK(static_cast<RS_Polyline *>(e)->isClosed());
    for (RS_Entity *segment : *static_cast<RS_EntityContainer *>(e)) {
      vertices.push_back(segment->getStartpoint());
      vertices.push_back(segment->getEndpoint());
    }
  }
  REQUIRE(vertices.size() > 2);
  for (const RS_Vector &v : vertices) {
    INFO("vertex " << v);
    CHECK(distanceToPolyline(v, curve) < 1e-6);
  }
  CHECK(vertices.front().distanceTo(curve.front()) < 1e-9);
  CHECK(vertices.back().distanceTo(curve.back()) < 1e-9);
}
