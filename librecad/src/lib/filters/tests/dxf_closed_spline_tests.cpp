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
#include <cstdlib>
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
  return (std::filesystem::temp_directory_path() / ("closed_spline_" + name))
      .string();
}

void preserveSplineDwgFixture(const std::string &source,
                              const std::string &fileName) {
  const char *directory = std::getenv("LIBRECAD_TANGENT_SPLINE_DWG_DIR");
  if (directory == nullptr || *directory == '\0') return;
  std::error_code error;
  std::filesystem::create_directories(directory, error);
  REQUIRE_FALSE(error);
  REQUIRE(std::filesystem::copy_file(
      source, std::filesystem::path(directory) / fileName,
      std::filesystem::copy_options::overwrite_existing, error));
  REQUIRE_FALSE(error);
}

const std::vector<std::vector<RS_Vector>> &closedSplinePointControlSets() {
  static const std::vector<std::vector<RS_Vector>> controlSets{
      {{0.0, 0.0}, {4.0, 0.0}, {2.0, 3.5}},
      {{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0}, {0.0, 3.0}},
      {{0.0, 0.0}, {4.0, 0.0}, {5.0, 3.0}, {2.0, 5.0}, {-1.0, 2.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {5.0, 2.0},
       {4.0, 4.0},
       {2.0, 5.0},
       {0.0, 4.0},
       {-1.0, 2.0}},
      {{0.0, 0.0}, {2.0, -1.0}, {4.0, 0.0}, {5.0, 2.0}, {4.0, 4.0}, {1.0, 3.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {5.0, 2.0},
       {4.0, 4.0},
       {2.0, 5.0},
       {0.0, 4.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {5.0, 2.0},
       {6.0, 4.0},
       {4.0, 6.0},
       {2.0, 5.0},
       {0.0, 4.0},
       {-1.0, 2.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {5.0, 2.0},
       {6.0, 4.0},
       {4.0, 6.0},
       {2.0, 5.0},
       {0.0, 4.0},
       {-1.0, 2.0},
       {-2.0, 1.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {6.0, 1.0},
       {7.0, 3.0},
       {6.0, 5.0},
       {4.0, 6.0},
       {2.0, 5.0},
       {0.0, 4.0},
       {-1.0, 3.0},
       {-2.0, 1.0}},
      {{0.0, 0.0},
       {2.0, -1.0},
       {4.0, 0.0},
       {6.0, 1.0},
       {7.0, 3.0},
       {6.0, 5.0},
       {4.0, 6.0},
       {2.0, 5.0},
       {0.0, 4.0},
       {-1.0, 3.0},
       {-2.0, 2.0},
       {-1.0, 1.0}},
  };
  return controlSets;
}

/** What a SPLINE record carries. */
struct SplineRecord {
  int m_flags = 0;
  int m_degree = 3;
  std::vector<RS_Vector> m_controlPoints;
  std::vector<double> m_knots;
  std::vector<RS_Vector> m_fitPoints;
  int m_scenario = 0;
  double m_tolFit = 0.0;
  RS_Vector m_tangentStart;
  RS_Vector m_tangentEnd;
};

/** A DXF holding one SPLINE; no HEADER, so no $ACADVER. */
std::string splineDxf(const SplineRecord &spline) {
  std::ostringstream out;
  out.precision(17);
  out << "0\nSECTION\n2\nENTITIES\n0\nSPLINE\n8\n0\n100\nAcDbEntity\n100\nAcDbS"
         "pline\n"
      << "70\n"
      << spline.m_flags << "\n71\n"
      << spline.m_degree << "\n72\n"
      << spline.m_knots.size() << "\n73\n"
      << spline.m_controlPoints.size() << "\n74\n"
      << spline.m_fitPoints.size() << "\n42\n1e-10\n43\n1e-10\n44\n1e-10\n";
  for (const double k : spline.m_knots)
    out << "40\n" << k << "\n";
  for (const RS_Vector &p : spline.m_controlPoints)
    out << "10\n" << p.x << "\n20\n" << p.y << "\n30\n0\n";
  for (const RS_Vector &p : spline.m_fitPoints)
    out << "11\n" << p.x << "\n21\n" << p.y << "\n31\n0\n";
  if (spline.m_tangentStart.magnitude() > 0.0)
    out << "12\n"
        << spline.m_tangentStart.x << "\n22\n"
        << spline.m_tangentStart.y << "\n32\n"
        << spline.m_tangentStart.z << "\n";
  if (spline.m_tangentEnd.magnitude() > 0.0)
    out << "13\n"
        << spline.m_tangentEnd.x << "\n23\n"
        << spline.m_tangentEnd.y << "\n33\n"
        << spline.m_tangentEnd.z << "\n";
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
    record.m_scenario = data->m_scenario;
    record.m_tolFit = data->tolfit;
    record.m_tangentStart = {data->tgStart.x, data->tgStart.y, data->tgStart.z};
    record.m_tangentEnd = {data->tgEnd.x, data->tgEnd.y, data->tgEnd.z};
    for (const auto &p : data->controllist)
      record.m_controlPoints.emplace_back(p->x, p->y);
    for (const auto &p : data->fitlist)
      record.m_fitPoints.emplace_back(p->x, p->y);
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

LC_SplinePoints *onlySplinePoints(RS_Graphic &graphic) {
  std::vector<LC_SplinePoints *> splines;
  for (RS_Entity *e : graphic)
    if (e->rtti() == RS2::EntitySplinePoints)
      splines.push_back(static_cast<LC_SplinePoints *>(e));
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

void importDxf(RS_Graphic &graphic, const SplineRecord &spline,
               const std::string &name,
               std::vector<SplineRecord> *records = nullptr) {
  const std::string path = tempPath(name + ".dxf");
  {
    std::ofstream out(path);
    out << splineDxf(spline);
  }
  importFile(graphic, path, RS2::FormatDXFRW, records);
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

TEST_CASE(
    "Nonuniform rational periodic RS_Spline survives DXF and DWG round trips",
    "[dxf][dwg][filter][spline][periodic][roundtrip]") {
  ensureSettings();
  struct Layout {
    std::string name;
    int degree;
    std::vector<RS_Vector> controls;
    std::vector<double> knots;
    std::vector<double> weights;
  };
  const std::vector<Layout> layouts{
      {"linear-5ctrl",
       1,
       {{0, 0}, {20, -10}, {45, 5}, {30, 35}, {-10, 20}},
       {0, 0, 0.5, 1.1, 2.5, 4, 4}},
      {"quadratic-5ctrl",
       2,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}},
       {0, 0, 0, 0.5, 1.5, 3, 3, 3}},
      {"cubic-5ctrl",
       3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}},
       {0, 0, 0, 0, 0.4, 1, 1, 1, 1}},
      {"cubic-6ctrl",
       3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}},
       {0, 0, 0, 0, 0.5, 2, 3, 3, 3, 3}},
      {"cubic-6ctrl-wide-weights",
       3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}},
       {0, 0, 0, 0, 0.5, 2, 3, 3, 3, 3},
       {0.01, 100.0, 0.1, 10.0, 0.5, 2.0}},
      {"cubic-6ctrl-conditioned-weights",
       3,
       {{0, 0}, {30, -15}, {60, 0}, {55, 35}, {20, 50}, {-15, 25}},
       {0, 0, 0, 0, 0.5, 2, 3, 3, 3, 3},
       {1.0e-6, 1.0e6, 1.0e-4, 1.0e4, 1.0e-2, 1.0e2}},
      {"cubic-8ctrl",
       3,
       {{0, 0},
        {30, -15},
        {60, 0},
        {55, 35},
        {35, 55},
        {5, 60},
        {-15, 25},
        {-10, 5}},
       {0, 0, 0, 0, 0.25, 0.75, 2.5, 3.5, 4, 4, 4, 4}},
  };
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},     {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"}, {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"}, {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const Layout &layout : layouts) {
    INFO(layout.name);
    RS_Spline open{nullptr, RS_SplineData{layout.degree, false}};
    for (const RS_Vector &point : layout.controls)
      open.addControlPoint(point);
    open.setKnotVector(layout.knots);
    REQUIRE(open.validate());
    open.setClosed(true);
    RS_SplineData sourceData = open.getData();
    std::vector<double> baseWeights = layout.weights;
    if (baseWeights.empty()) {
      for (size_t i = 0; i < layout.controls.size(); ++i)
        baseWeights.push_back(0.85 + 0.05 * static_cast<double>(i % 6));
    }
    REQUIRE(baseWeights.size() == layout.controls.size());
    sourceData.weights = baseWeights;
    sourceData.weights.insert(sourceData.weights.end(), baseWeights.begin(),
                              baseWeights.begin() + sourceData.degree);
    RS_Spline source{nullptr, sourceData};
    REQUIRE(source.validate());
    REQUIRE(source.hasWrappedControlPoints());
    bool hasNonuniformIntervals = false;
    for (size_t i = 2; i < sourceData.knotslist.size(); ++i) {
      if (std::abs((sourceData.knotslist[i] - sourceData.knotslist[i - 1]) -
                   (sourceData.knotslist[1] - sourceData.knotslist[0])) >
          1.0e-12) {
        hasNonuniformIntervals = true;
        break;
      }
    }
    CHECK(hasNonuniformIntervals);
    const auto expected = splineCurve(source, 2000);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Spline(&graphic, sourceData));
    for (const auto &[format, suffix] : formats) {
      INFO(suffix);
      const std::string fixture =
          "nonuniform-periodic-rational-" + layout.name + "-" + suffix;
      const std::string path = tempPath(fixture);
      std::remove(path.c_str());
      {
        RS_FilterDXFRW writer;
        REQUIRE(
            writer.fileExport(graphic, QString::fromStdString(path), format));
      }
#ifdef DWGSUPPORT
      if (format != RS2::FormatDXFRW)
        preserveSplineDwgFixture(path, fixture + ".dwg");
#endif
      RS_Graphic reopened;
      std::vector<SplineRecord> written;
      importFile(reopened, path,
                 format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG,
                 &written);
      std::filesystem::remove(path);
      REQUIRE(written.size() == 1);
      CHECK((written.front().m_flags & 0x3) == 0x3);
      CHECK((written.front().m_flags & 0x4) != 0);
      checkSameVectors(written.front().m_controlPoints,
                       sourceData.controlPoints);
      checkSameKnots(written.front().m_knots, sourceData.knotslist);
      RS_Spline *received = onlySpline(reopened);
      REQUIRE(received->validate());
      CHECK(received->isClosed());
      CHECK(received->hasWrappedControlPoints());
      CHECK(curveDistance(expected, splineCurve(*received, 2000)) < 1.0e-9);

      const char *odaDirectory =
          std::getenv("LIBRECAD_NONUNIFORM_PERIODIC_ODA_DXF_DIR");
      if (format != RS2::FormatDXFRW && odaDirectory != nullptr &&
          *odaDirectory != '\0') {
        const auto converted =
            std::filesystem::path(odaDirectory) / (fixture + ".dxf");
        REQUIRE(std::filesystem::exists(converted));
        RS_Graphic external;
        importFile(external, converted.string(), RS2::FormatDXFRW);
        RS_Spline *externalSpline = onlySpline(external);
        REQUIRE(externalSpline->validate());
        CHECK(curveDistance(expected, splineCurve(*externalSpline, 2000)) <
              1.0e-5);
      }
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
    REQUIRE(filter.fileExport(first, QString::fromStdString(path),
                              RS2::FormatDXFRW));
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

TEST_CASE("Fit-point SPLINE intent survives DXF and DWG file round trips",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();

  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 2;
  source.m_fitPoints = {{0., 0.}, {2., 3.}, {5., 4.}, {8., 0.}};
  const std::string inputPath = tempPath("fit_point_source.dxf");
  {
    std::ofstream out(inputPath);
    out << splineDxf(source);
  }

  RS_Graphic first;
  importFile(first, inputPath, RS2::FormatDXFRW);
  std::filesystem::remove(inputPath);
  LC_SplinePoints *sourcePoints = onlySplinePoints(first);
  REQUIRE_FALSE(sourcePoints->getData().useControlPoints);
  checkSameVectors(sourcePoints->getData().splinePoints, source.m_fitPoints);
  CHECK(std::abs(sourcePoints->getData().fitTolerance - 1.0e-10) < 1.0e-20);

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("fit_point_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(first, QString::fromStdString(path), format));
    }

    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);

    REQUIRE(written.size() == 1);
    CHECK(written.front().m_controlPoints.empty());
    checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
    CHECK(std::abs(written.front().m_tolFit - 1.0e-10) < 1.0e-20);
    if (format != RS2::FormatDXFRW)
      CHECK(written.front().m_scenario == 2);

    LC_SplinePoints *reopenedPoints = onlySplinePoints(reopened);
    CHECK_FALSE(reopenedPoints->getData().useControlPoints);
    CHECK(std::abs(reopenedPoints->getData().fitTolerance - 1.0e-10) < 1.0e-20);
    checkSameVectors(reopenedPoints->getData().splinePoints,
                     sourcePoints->getData().splinePoints);
  }
}

TEST_CASE("Tangent-constrained quadratic fit SPLINE falls back to its control net",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 2;
  source.m_fitPoints = {{0., 0.}, {2., 3.}, {5., 4.}, {8., 0.}};
  source.m_tangentStart = {1., 0., 0.};
  source.m_tangentEnd = {0., 1., 0.};
  RS_Graphic graphic;
  importDxf(graphic, source, "tangent_quadratic_fit_point_source");

  LC_SplinePoints *sourceSpline = onlySplinePoints(graphic);
  REQUIRE_FALSE(sourceSpline->getData().useControlPoints);
  REQUIRE(sourceSpline->getData().fitTangentsConstrained);
  CHECK(sourceSpline->getData().fitStartTangent.x == source.m_tangentStart.x);
  CHECK(sourceSpline->getData().fitStartTangent.y == source.m_tangentStart.y);
  CHECK(sourceSpline->getData().fitEndTangent.x == source.m_tangentEnd.x);
  CHECK(sourceSpline->getData().fitEndTangent.y == source.m_tangentEnd.y);
  CHECK_FALSE(sourceSpline->fitTangentsMatchCurve());
  const auto sourceControls = sourceSpline->getControlPoints();
  REQUIRE(sourceControls.size() >= 3);
  std::vector<RS_Vector> sourceCurve;
  for (int i = 0; i <= 200; ++i) {
    const double parameter =
        static_cast<double>(sourceSpline->getSegmentCount()) * i / 200.0;
    const auto side = i == 200 ? LC_CurveEvaluationSide::Left
                               : LC_CurveEvaluationSide::Right;
    LC_CurveJet jet;
    REQUIRE(sourceSpline->tryEvaluateJet(parameter, side, jet));
    sourceCurve.push_back(jet.point);
  }

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("tangent_quadratic_fit_point_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
    }

    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);

    REQUIRE(written.size() == 1);
    CHECK_FALSE(written.front().m_controlPoints.empty());
    if (format == RS2::FormatDXFRW) {
      checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
      CHECK(written.front().m_tangentStart.x == source.m_tangentStart.x);
      CHECK(written.front().m_tangentEnd.y == source.m_tangentEnd.y);
      RS_Spline *reopenedSpline = onlySpline(reopened);
      CHECK(reopenedSpline->getData().fitTangentsConstrained);
      checkSameVectors(reopenedSpline->getData().fitPoints, source.m_fitPoints);
    } else {
      CHECK(written.front().m_fitPoints.empty());
      CHECK(written.front().m_tangentStart.magnitude() == 0.0);
      CHECK(written.front().m_tangentEnd.magnitude() == 0.0);
      CHECK(written.front().m_scenario == 1);
      LC_SplinePoints *reopenedSpline = onlySplinePoints(reopened);
      checkSameVectors(reopenedSpline->getControlPoints(), sourceControls);
      CHECK_FALSE(reopenedSpline->getData().fitTangentsConstrained);
      std::vector<RS_Vector> reopenedCurve;
      for (int i = 0; i <= 200; ++i) {
        const double parameter =
            static_cast<double>(reopenedSpline->getSegmentCount()) * i / 200.0;
        const auto side = i == 200 ? LC_CurveEvaluationSide::Left
                                   : LC_CurveEvaluationSide::Right;
        LC_CurveJet jet;
        REQUIRE(reopenedSpline->tryEvaluateJet(parameter, side, jet));
        reopenedCurve.push_back(jet.point);
      }
      REQUIRE(reopenedCurve.size() == sourceCurve.size());
      for (size_t i = 0; i < sourceCurve.size(); ++i)
        CHECK(reopenedCurve[i].distanceTo(sourceCurve[i]) < 1.0e-10);
    }
  }
}

#ifdef DWGSUPPORT
TEST_CASE("DWG preserves tangent-constrained spline-points as exact control splines",
          "[dwg][filter][spline-points][fit-points][roundtrip]") {
  ensureSettings();
  const std::vector<std::vector<RS_Vector>> fitPointSets{
      {{0.0, 0.0}, {2.0, 3.0}, {5.0, 0.0}},
      {{0.0, 0.0}, {2.0, 3.0}, {5.0, 4.0}, {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 4.0}, {3.0, 5.0}, {6.0, 2.0}, {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {2.0, 4.0}, {4.0, 4.0}, {6.0, 2.0},
       {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {2.0, 4.0}, {3.0, 2.0}, {4.0, 1.0},
       {5.0, 4.0}, {6.0, 3.0}, {8.0, 0.0}},
  };

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  for (const int tangentMode : {0, 1, 2}) {
    const bool constrainStart = tangentMode != 2;
    const bool constrainEnd = tangentMode != 1;
    for (const auto& fitPoints : fitPointSets) {
      CAPTURE(fitPoints.size());
      LC_SplinePointsData initialData{false, false};
      initialData.splinePoints = fitPoints;
      initialData.fitTolerance = 1.0e-8;
      LC_SplinePoints initial{nullptr, initialData};
      LC_CurveJet startJet;
      LC_CurveJet endJet;
      REQUIRE(initial.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right,
                                     startJet));
      const double endParameter = static_cast<double>(initial.getSegmentCount());
      REQUIRE(initial.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left,
                                     endJet));

      LC_SplinePointsData sourceData = initial.getData();
      sourceData.fitTangentsConstrained = true;
      sourceData.fitStartTangent = constrainStart
          ? startJet.first : RS_Vector{0.0, 0.0, 0.0};
      sourceData.fitEndTangent = constrainEnd
          ? endJet.first : RS_Vector{0.0, 0.0, 0.0};
      LC_SplinePoints source{nullptr, sourceData};
      REQUIRE(source.fitTangentsMatchCurve());
      RS_Graphic graphic;
      graphic.initForNewDocument();
      auto* stored = new LC_SplinePoints(&graphic, source.getData());
      graphic.addEntity(stored);

      for (const auto& [format, suffix] : formats) {
        INFO(suffix);
        INFO(tangentMode);
        INFO(fitPoints.size());
        const std::string pointSuffix = std::to_string(fitPoints.size()) + "pt";
        const std::string path = tempPath("tangent_spline_points_" + suffix +
            "_mode" + std::to_string(tangentMode) + "_" + pointSuffix);
        std::remove(path.c_str());
        {
          RS_FilterDXFRW writer;
          REQUIRE(writer.fileExport(graphic, QString::fromStdString(path),
                                    format));
        }
        preserveSplineDwgFixture(
            path, "control-net-mode" + std::to_string(tangentMode) + "-" +
                      pointSuffix + "-" + suffix + ".dwg");
        RS_Graphic reopened;
        std::vector<SplineRecord> written;
        importFile(reopened, path, RS2::FormatDWG, &written);
        std::filesystem::remove(path);

        REQUIRE(written.size() == 1);
        CHECK(written.front().m_scenario == 1);
        CHECK(written.front().m_degree == 2);
        CHECK(written.front().m_fitPoints.empty());
        checkSameVectors(written.front().m_controlPoints,
                         source.getControlPoints());
        CHECK_FALSE(written.front().m_tangentStart.magnitude() > 0.0);
        CHECK_FALSE(written.front().m_tangentEnd.magnitude() > 0.0);

        REQUIRE(reopened.count() == 1);
        auto* reopenedSpline = dynamic_cast<LC_SplinePoints*>(*reopened.begin());
        REQUIRE(reopenedSpline != nullptr);
        CHECK_FALSE(reopenedSpline->getData().fitTangentsConstrained);
        CHECK(stored->fitTangentsMatchCurve());
        REQUIRE(reopenedSpline->getSegmentCount() == source.getSegmentCount());
        for (int i = 0; i <= 100; ++i) {
          const double parameter = static_cast<double>(source.getSegmentCount()) *
                                   i / 100.0;
          const auto side = i == 100 ? LC_CurveEvaluationSide::Left
                                     : LC_CurveEvaluationSide::Right;
          LC_CurveJet sourceJet;
          LC_CurveJet reopenedJet;
          REQUIRE(source.tryEvaluateJet(parameter, side, sourceJet));
          REQUIRE(reopenedSpline->tryEvaluateJet(parameter, side, reopenedJet));
          CHECK(reopenedJet.point.distanceTo(sourceJet.point) < 1.0e-10);
        }
      }
    }
  }
}

TEST_CASE("DWG round-trips closed spline-points control geometry",
          "[dwg][filter][spline-points][closed][roundtrip]") {
  ensureSettings();
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  for (const auto& controlPoints : closedSplinePointControlSets()) {
    INFO(controlPoints.size());
    LC_SplinePointsData data{true, true};
    data.useControlPoints = true;
    data.controlPoints = controlPoints;
    LC_SplinePoints source{nullptr, data};
    REQUIRE(source.getSegmentCount() == data.controlPoints.size());
    std::vector<RS_Vector> sourceCurve;
    for (int i = 0; i <= 500; ++i) {
      const double parameter =
          static_cast<double>(source.getSegmentCount()) * i / 500.0;
      const auto side = i == 500 ? LC_CurveEvaluationSide::Left
                                 : LC_CurveEvaluationSide::Right;
      LC_CurveJet jet;
      REQUIRE(source.tryEvaluateJet(parameter, side, jet));
      sourceCurve.push_back(jet.point);
    }

    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* stored = new LC_SplinePoints(&graphic, data);
    graphic.addEntity(stored);
    for (const auto& [format, version] : formats) {
      INFO(version);
      const std::string pointCount = std::to_string(controlPoints.size()) + "pt";
      const std::string path = tempPath("closed-control-net-" + pointCount +
                                        "-" + version);
      std::remove(path.c_str());
      {
        RS_FilterDXFRW writer;
        REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
      }
      preserveSplineDwgFixture(path, "closed-control-" + pointCount + "-" +
                                        version + ".dwg");
      RS_Graphic reopened;
      std::vector<SplineRecord> written;
      importFile(reopened, path, RS2::FormatDWG, &written);
      std::filesystem::remove(path);

      REQUIRE(written.size() == 1);
      CHECK(written.front().m_scenario == 1);
      CHECK(written.front().m_degree == 2);
      CHECK((written.front().m_flags & 0x1) != 0);
      CHECK((written.front().m_flags & 0x2) != 0);
      auto periodicControls = controlPoints;
      periodicControls.push_back(controlPoints[0]);
      periodicControls.push_back(controlPoints[1]);
      checkSameVectors(written.front().m_controlPoints, periodicControls);
      REQUIRE(written.front().m_knots.size() == periodicControls.size() + 3);
      for (size_t i = 0; i < written.front().m_knots.size(); ++i)
        CHECK(written.front().m_knots[i] == static_cast<double>(i));
      REQUIRE(reopened.count() == 1);
      auto* reopenedSpline = dynamic_cast<LC_SplinePoints*>(*reopened.begin());
      REQUIRE(reopenedSpline != nullptr);
      CHECK(reopenedSpline->isClosed());
      CHECK(reopenedSpline->getData().cut);
      std::vector<RS_Vector> reopenedCurve;
      for (int i = 0; i <= 500; ++i) {
        const double parameter =
            static_cast<double>(reopenedSpline->getSegmentCount()) * i / 500.0;
        const auto side = i == 500 ? LC_CurveEvaluationSide::Left
                                   : LC_CurveEvaluationSide::Right;
        LC_CurveJet jet;
        REQUIRE(reopenedSpline->tryEvaluateJet(parameter, side, jet));
        reopenedCurve.push_back(jet.point);
      }
      REQUIRE(reopenedCurve.size() == sourceCurve.size());
      for (size_t i = 0; i < sourceCurve.size(); ++i)
        CHECK(reopenedCurve[i].distanceTo(sourceCurve[i]) < 1.0e-10);
    }
  }
}

TEST_CASE("DWG round-trips both pieces of a cut spline-points curve",
          "[dwg][filter][spline-points][cut][roundtrip]") {
  ensureSettings();
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  const auto sample = [](const LC_SplinePoints& spline) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= 400; ++i) {
      const bool atEnd = i == 400;
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / 400.0,
          atEnd ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Right,
          jet));
      points.push_back(jet.point);
    }
    return points;
  };
  const std::vector<std::vector<RS_Vector>> controlSets{
      {{0.0, 0.0}, {3.0, 4.0}, {7.0, 0.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {6.0, -1.0}, {10.0, 1.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {5.0, -1.0}, {8.0, 3.0}, {11.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {3.0, 5.0}, {5.0, 1.0},
       {7.0, -2.0}, {9.0, 3.0}, {12.0, 0.0}},
  };
  const std::array<double, 3> cutFractions{0.25, 0.5, 0.75};
  for (size_t caseIndex = 0; caseIndex < controlSets.size(); ++caseIndex) {
    CAPTURE(caseIndex, controlSets[caseIndex].size());
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controlSets[caseIndex];
    LC_SplinePoints original{nullptr, data};

    const double domain = static_cast<double>(original.getSegmentCount());
    for (const double usedFraction : cutFractions) {
      LC_SplinePoints candidate{nullptr, data};
      LC_CurveJet cutJet;
      REQUIRE(candidate.tryEvaluateJet(domain * usedFraction,
                                       LC_CurveEvaluationSide::Right,
                                       cutJet));
      std::unique_ptr<LC_SplinePoints> candidateRight(candidate.cut(cutJet.point));
      REQUIRE(candidateRight);
      LC_SplinePoints left{nullptr, candidate.getData()};
      CHECK(left.getData().cut);
      CHECK(candidateRight->getData().cut);
      const std::array<std::vector<RS_Vector>, 2> expected{
          sample(left), sample(*candidateRight)};

      RS_Graphic graphic;
      graphic.initForNewDocument();
      graphic.addEntity(new LC_SplinePoints(&graphic, left.getData()));
      graphic.addEntity(new LC_SplinePoints(&graphic, candidateRight->getData()));
      for (const auto& [format, version] : formats) {
        INFO(version);
        INFO(usedFraction);
        const std::string fractionLabel = std::to_string(static_cast<int>(usedFraction * 100));
        const std::string pointCount = std::to_string(data.controlPoints.size()) + "pt";
        const std::string path = tempPath("cut-control-net-" + pointCount + "-q" +
                                           fractionLabel + "-" + version);
        std::remove(path.c_str());
        {
          RS_FilterDXFRW writer;
          REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
        }
        preserveSplineDwgFixture(path, "cut-control-" + pointCount + "-q" +
                                          fractionLabel + "-" + version +
                                          ".dwg");
        RS_Graphic reopened;
        std::vector<SplineRecord> written;
        importFile(reopened, path, RS2::FormatDWG, &written);
        std::filesystem::remove(path);

        REQUIRE(written.size() == 2);
        REQUIRE(reopened.count() == 2);
        std::array<bool, 2> matched{false, false};
        for (RS_Entity* entity : reopened) {
          auto* spline = dynamic_cast<LC_SplinePoints*>(entity);
          REQUIRE(spline != nullptr);
          const auto actual = sample(*spline);
          const double d0 = curveDistance(expected[0], actual);
          const double d1 = curveDistance(expected[1], actual);
          const size_t match = d0 <= d1 ? 0 : 1;
          CHECK_FALSE(matched[match]);
          matched[match] = true;
          CHECK(std::min(d0, d1) < 1.0e-10);
        }
        CHECK(matched[0]);
        CHECK(matched[1]);
      }
    }
  }

}

TEST_CASE("DWG round-trips a closed spline-points curve cut at its seam",
          "[dwg][filter][spline-points][closed][cut][roundtrip]") {
  ensureSettings();
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  const std::array<double, 3> cutFractions{0.25, 0.5, 0.75};
  const auto sample = [](const LC_SplinePoints& spline) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= 600; ++i) {
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / 600.0,
          i == 600 ? LC_CurveEvaluationSide::Left
                   : LC_CurveEvaluationSide::Right, jet));
      points.push_back(jet.point);
    }
    return points;
  };

  for (const auto& controlPoints : closedSplinePointControlSets()) {
    CAPTURE(controlPoints.size());
    LC_SplinePointsData data{true, false};
    data.useControlPoints = true;
    data.controlPoints = controlPoints;
    LC_SplinePoints original{nullptr, data};
    const double domain = static_cast<double>(original.getSegmentCount());
    for (const double usedFraction : cutFractions) {
      auto candidate = std::make_unique<LC_SplinePoints>(nullptr, data);
      LC_CurveJet cutJet;
      REQUIRE(candidate->tryEvaluateJet(domain * usedFraction,
                                       LC_CurveEvaluationSide::Right, cutJet));
      const auto* right = candidate->cut(cutJet.point);
      CHECK(right == nullptr);
      delete right;
      REQUIRE_FALSE(candidate->isClosed());
      CHECK(candidate->getData().cut);
      INFO(usedFraction);
      const auto expected = sample(*candidate);

      RS_Graphic graphic;
      graphic.initForNewDocument();
      graphic.addEntity(new LC_SplinePoints(&graphic, candidate->getData()));
      for (const auto& [format, version] : formats) {
        INFO(version);
        const std::string pointCount = std::to_string(controlPoints.size()) + "pt";
        const std::string fractionLabel = std::to_string(static_cast<int>(usedFraction * 100));
        const std::string path = tempPath("closed-cut-control-" + pointCount + "-q" +
                                           fractionLabel + "-" + version);
        std::remove(path.c_str());
        {
          RS_FilterDXFRW writer;
          REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
        }
        preserveSplineDwgFixture(
            path, "closed-cut-control-" + pointCount + "-q" + fractionLabel +
                      "-" + version + ".dwg");
        RS_Graphic reopened;
        std::vector<SplineRecord> written;
        importFile(reopened, path, RS2::FormatDWG, &written);
        std::filesystem::remove(path);

        REQUIRE(written.size() == 1);
        CHECK((written.front().m_flags & 0x3) == 0);
        REQUIRE(reopened.count() == 1);
        auto* spline = dynamic_cast<LC_SplinePoints*>(*reopened.begin());
        REQUIRE(spline != nullptr);
        CHECK_FALSE(spline->isClosed());
        CHECK(curveDistance(expected, sample(*spline)) < 1.0e-10);
      }
    }
  }
}

TEST_CASE("DWG writes cut fit-point splines from their authoritative control nets",
          "[dwg][filter][spline-points][fit-points][cut][roundtrip]") {
  ensureSettings();
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  const std::vector<std::vector<RS_Vector>> fitPointSets{
      {{0.0, 0.0}, {2.0, 4.0}, {6.0, -1.0}, {10.0, 1.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {5.0, -1.0}, {8.0, 3.0}, {11.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {3.0, 5.0}, {5.0, 1.0},
       {7.0, -2.0}, {9.0, 3.0}, {12.0, 0.0}},
      {{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}, {3.5, 3.0}, {5.0, 1.0},
       {6.5, -1.0}, {8.0, 0.0}, {10.0, 2.0}, {12.0, 0.0}},
      {{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}, {3.5, 3.0}, {5.0, 1.0},
       {6.5, -1.0}, {8.0, 0.0}, {9.0, 2.5}, {10.0, 4.0}, {11.0, 2.0},
       {13.0, 0.0}},
  };
  const std::array<double, 3> cutFractions{0.25, 0.5, 0.75};
  const auto sample = [](const LC_SplinePoints& spline) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= 400; ++i) {
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / 400.0,
          i == 400 ? LC_CurveEvaluationSide::Left
                   : LC_CurveEvaluationSide::Right, jet));
      points.push_back(jet.point);
    }
    return points;
  };

  for (const auto& fitPoints : fitPointSets) {
    CAPTURE(fitPoints.size());
    LC_SplinePointsData data{false, false};
    data.splinePoints = fitPoints;
    LC_SplinePoints source{nullptr, data};
    const double domain = static_cast<double>(source.getSegmentCount());
    for (const double fraction : cutFractions) {
      INFO(fraction);
      LC_SplinePoints candidate{nullptr, data};
      LC_CurveJet cutJet;
      REQUIRE(candidate.tryEvaluateJet(domain * fraction,
                                       LC_CurveEvaluationSide::Right, cutJet));
      std::unique_ptr<LC_SplinePoints> right(candidate.cut(cutJet.point));
      REQUIRE(right);
      LC_SplinePoints left{nullptr, candidate.getData()};
      CHECK(left.getData().cut);
      CHECK(right->getData().cut);
      const std::array<std::vector<RS_Vector>, 2> expected{sample(left), sample(*right)};

      RS_Graphic graphic;
      graphic.initForNewDocument();
      graphic.addEntity(new LC_SplinePoints(&graphic, left.getData()));
      graphic.addEntity(new LC_SplinePoints(&graphic, right->getData()));
      for (const auto& [format, version] : formats) {
        INFO(version);
        const std::string pointCount = std::to_string(fitPoints.size()) + "pt";
        const std::string fractionLabel = std::to_string(static_cast<int>(fraction * 100));
        const std::string path = tempPath("cut-fit-control-net-" + pointCount + "-q" +
                                           fractionLabel + "-" + version);
        std::remove(path.c_str());
        {
          RS_FilterDXFRW writer;
          REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
        }
        preserveSplineDwgFixture(path, "cut-fit-control-" + pointCount + "-q" +
                                          fractionLabel + "-" + version +
                                          ".dwg");
        RS_Graphic reopened;
        std::vector<SplineRecord> written;
        importFile(reopened, path, RS2::FormatDWG, &written);
        std::filesystem::remove(path);

        REQUIRE(written.size() == 2);
        for (const auto& record : written) {
          CHECK(record.m_scenario == 1);
          CHECK(record.m_fitPoints.empty());
        }
        REQUIRE(reopened.count() == 2);
        std::array<bool, 2> matched{false, false};
        for (RS_Entity* entity : reopened) {
          auto* spline = dynamic_cast<LC_SplinePoints*>(entity);
          REQUIRE(spline != nullptr);
          const auto actual = sample(*spline);
          const double d0 = curveDistance(expected[0], actual);
          const double d1 = curveDistance(expected[1], actual);
          const size_t match = d0 <= d1 ? 0 : 1;
          CHECK_FALSE(matched[match]);
          matched[match] = true;
          CHECK(std::min(d0, d1) < 1.0e-10);
        }
        CHECK(matched[0]);
        CHECK(matched[1]);
      }
    }
  }
}

TEST_CASE("ODA-converted spline-point control DWGs preserve evaluated geometry",
          "[external][oda][dwg][spline-points][geometry]") {
  const char* outputDirectory =
      std::getenv("LIBRECAD_TANGENT_SPLINE_ODA_DXF_DIR");
  if (outputDirectory == nullptr || *outputDirectory == '\0')
    SKIP("Set LIBRECAD_TANGENT_SPLINE_ODA_DXF_DIR to converted ODA DXFs");
  ensureSettings();

  const std::vector<std::vector<RS_Vector>> fitPointSets{
      {{0.0, 0.0}, {2.0, 3.0}, {5.0, 0.0}},
      {{0.0, 0.0}, {2.0, 3.0}, {5.0, 4.0}, {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 4.0}, {3.0, 5.0}, {6.0, 2.0}, {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {2.0, 4.0}, {4.0, 4.0}, {6.0, 2.0},
       {8.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {2.0, 4.0}, {3.0, 2.0}, {4.0, 1.0},
       {5.0, 4.0}, {6.0, 3.0}, {8.0, 0.0}},
  };
  const std::vector<std::pair<std::string, std::string>> versions{
      {"dwg-r2000", "R2000"}, {"dwg-r2004", "R2004"},
      {"dwg-r2007", "R2007"}, {"dwg-r2010", "R2010"},
      {"dwg-r2013", "R2013"}, {"dwg-r2018", "R2018"},
  };

  for (const int tangentMode : {0, 1, 2}) {
    const bool constrainStart = tangentMode != 2;
    const bool constrainEnd = tangentMode != 1;
    for (const auto& fitPoints : fitPointSets) {
      LC_SplinePointsData initialData{false, false};
      initialData.splinePoints = fitPoints;
      initialData.fitTolerance = 1.0e-8;
      LC_SplinePoints initial{nullptr, initialData};
      LC_CurveJet startJet;
      LC_CurveJet endJet;
      REQUIRE(initial.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right,
                                     startJet));
      const double endParameter = static_cast<double>(initial.getSegmentCount());
      REQUIRE(initial.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left,
                                     endJet));
      LC_SplinePointsData sourceData = initial.getData();
      sourceData.fitTangentsConstrained = true;
      sourceData.fitStartTangent = constrainStart
          ? startJet.first : RS_Vector{0.0, 0.0, 0.0};
      sourceData.fitEndTangent = constrainEnd
          ? endJet.first : RS_Vector{0.0, 0.0, 0.0};
      LC_SplinePoints source{nullptr, sourceData};
      REQUIRE(source.fitTangentsMatchCurve());

      std::vector<RS_Vector> expected;
      for (int i = 0; i <= 2000; ++i) {
        const double parameter =
            static_cast<double>(source.getSegmentCount()) * i / 2000.0;
        const auto side = i == 2000 ? LC_CurveEvaluationSide::Left
                                    : LC_CurveEvaluationSide::Right;
        LC_CurveJet jet;
        REQUIRE(source.tryEvaluateJet(parameter, side, jet));
        expected.push_back(jet.point);
      }

      for (const auto& [version, label] : versions) {
        INFO(label);
        INFO(tangentMode);
        INFO(fitPoints.size());
        const std::string name = "control-net-mode" +
            std::to_string(tangentMode) + "-" +
            std::to_string(fitPoints.size()) + "pt-" + version + ".dxf";
        const auto path = std::filesystem::path(outputDirectory) / name;
        REQUIRE(std::filesystem::exists(path));
        RS_Graphic converted;
        std::vector<SplineRecord> records;
        importFile(converted, path.string(), RS2::FormatDXFRW, &records);
        REQUIRE(records.size() == 1);
        REQUIRE(converted.count() == 1);

        std::vector<RS_Vector> actual;
        auto* entity = *converted.begin();
        if (auto* splinePoints = dynamic_cast<LC_SplinePoints*>(entity)) {
          for (int i = 0; i <= 2000; ++i) {
            const double parameter =
                static_cast<double>(splinePoints->getSegmentCount()) * i / 2000.0;
            const auto side = i == 2000 ? LC_CurveEvaluationSide::Left
                                        : LC_CurveEvaluationSide::Right;
            LC_CurveJet jet;
            REQUIRE(splinePoints->tryEvaluateJet(parameter, side, jet));
            actual.push_back(jet.point);
          }
        } else if (auto* spline = dynamic_cast<RS_Spline*>(entity)) {
          actual = splineCurve(*spline, 2000);
        } else {
          FAIL("ODA-converted SPLINE imported as an unexpected entity type");
        }
        CHECK(curveDistance(expected, actual) < 1.0e-6);
      }
    }
  }

  for (const auto& controlPoints : closedSplinePointControlSets()) {
    INFO(controlPoints.size());
    LC_SplinePointsData closedData{true, true};
    closedData.useControlPoints = true;
    closedData.controlPoints = controlPoints;
    LC_SplinePoints closedSource{nullptr, closedData};
    std::vector<RS_Vector> closedExpected;
    for (int i = 0; i <= 2000; ++i) {
      const double parameter =
          static_cast<double>(closedSource.getSegmentCount()) * i / 2000.0;
      const auto side = i == 2000 ? LC_CurveEvaluationSide::Left
                                  : LC_CurveEvaluationSide::Right;
      LC_CurveJet jet;
      REQUIRE(closedSource.tryEvaluateJet(parameter, side, jet));
      closedExpected.push_back(jet.point);
    }
    const std::string pointCount = std::to_string(controlPoints.size()) + "pt";
    for (const auto& [version, label] : versions) {
      INFO(label);
      const auto path = std::filesystem::path(outputDirectory) /
                        ("closed-control-" + pointCount + "-" + version +
                         ".dxf");
      REQUIRE(std::filesystem::exists(path));
      RS_Graphic converted;
      std::vector<SplineRecord> records;
      importFile(converted, path.string(), RS2::FormatDXFRW, &records);
      REQUIRE(records.size() == 1);
      CHECK((records.front().m_flags & 0x1) != 0);
      CHECK((records.front().m_flags & 0x2) != 0);
      REQUIRE(converted.count() == 1);
      std::vector<RS_Vector> actual;
      auto* entity = *converted.begin();
      if (auto* splinePoints = dynamic_cast<LC_SplinePoints*>(entity)) {
        REQUIRE(splinePoints->isClosed());
        for (int i = 0; i <= 2000; ++i) {
          const double parameter =
              static_cast<double>(splinePoints->getSegmentCount()) * i / 2000.0;
          const auto side = i == 2000 ? LC_CurveEvaluationSide::Left
                                      : LC_CurveEvaluationSide::Right;
          LC_CurveJet jet;
          REQUIRE(splinePoints->tryEvaluateJet(parameter, side, jet));
          actual.push_back(jet.point);
        }
      } else if (auto* spline = dynamic_cast<RS_Spline*>(entity)) {
        actual = splineCurve(*spline, 2000);
      } else {
        FAIL("ODA-converted closed SPLINE imported as an unexpected entity type");
      }
      CHECK(curveDistance(closedExpected, actual) < 1.0e-5);
    }
  }
}

TEST_CASE("ODA-converted cut spline-point DWGs preserve both piece geometries",
          "[external][oda][dwg][spline-points][cut][geometry]") {
  const char* outputDirectory = std::getenv("LIBRECAD_OFFSET_CUT_ODA_DXF_DIR");
  if (outputDirectory == nullptr || *outputDirectory == '\0')
    SKIP("Set LIBRECAD_OFFSET_CUT_ODA_DXF_DIR to converted cut-spline DXFs");
  ensureSettings();

  const std::vector<std::vector<RS_Vector>> controlSets{
      {{0.0, 0.0}, {3.0, 4.0}, {7.0, 0.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {6.0, -1.0}, {10.0, 1.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {5.0, -1.0}, {8.0, 3.0}, {11.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {3.0, 5.0}, {5.0, 1.0},
       {7.0, -2.0}, {9.0, 3.0}, {12.0, 0.0}},
  };
  const std::array<double, 3> cutFractions{0.25, 0.5, 0.75};
  const std::vector<std::pair<std::string, std::string>> versions{
      {"dwg-r2000", "R2000"}, {"dwg-r2004", "R2004"},
      {"dwg-r2007", "R2007"}, {"dwg-r2010", "R2010"},
      {"dwg-r2013", "R2013"}, {"dwg-r2018", "R2018"},
  };
  const auto sampleSplinePoints = [](const LC_SplinePoints& spline) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= 1000; ++i) {
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / 1000.0,
          i == 1000 ? LC_CurveEvaluationSide::Left
                    : LC_CurveEvaluationSide::Right, jet));
      points.push_back(jet.point);
    }
    return points;
  };

  for (size_t caseIndex = 0; caseIndex < controlSets.size(); ++caseIndex) {
    CAPTURE(caseIndex, controlSets[caseIndex].size());
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controlSets[caseIndex];
    LC_SplinePoints source{nullptr, data};
    const double domain = static_cast<double>(source.getSegmentCount());
    for (const double fraction : cutFractions) {
      LC_SplinePoints candidate{nullptr, data};
      LC_CurveJet cutJet;
      REQUIRE(candidate.tryEvaluateJet(domain * fraction,
                                       LC_CurveEvaluationSide::Right, cutJet));
      std::unique_ptr<LC_SplinePoints> candidateRight(candidate.cut(cutJet.point));
      REQUIRE(candidateRight);
      LC_SplinePoints left{nullptr, candidate.getData()};
      const std::array<std::vector<RS_Vector>, 2> expected{
          sampleSplinePoints(left), sampleSplinePoints(*candidateRight)};
      const std::string pointCount = std::to_string(data.controlPoints.size()) + "pt";
      const std::string fractionLabel = std::to_string(static_cast<int>(fraction * 100));

      for (const auto& [version, label] : versions) {
        INFO(label);
        INFO(fraction);
        const auto path = std::filesystem::path(outputDirectory) /
            ("cut-control-" + pointCount + "-q" + fractionLabel + "-" + version + ".dxf");
        REQUIRE(std::filesystem::exists(path));
        RS_Graphic converted;
        std::vector<SplineRecord> records;
        importFile(converted, path.string(), RS2::FormatDXFRW, &records);
        REQUIRE(records.size() == 2);
        REQUIRE(converted.count() == 2);
        std::array<bool, 2> matched{false, false};
        for (RS_Entity* entity : converted) {
          std::vector<RS_Vector> actual;
          if (auto* splinePoints = dynamic_cast<LC_SplinePoints*>(entity)) {
            actual = sampleSplinePoints(*splinePoints);
          } else if (auto* spline = dynamic_cast<RS_Spline*>(entity)) {
            actual = splineCurve(*spline, 1000);
          } else {
            FAIL("ODA-converted cut SPLINE imported as an unexpected entity type");
          }
          const double d0 = curveDistance(expected[0], actual);
          const double d1 = curveDistance(expected[1], actual);
          const size_t match = d0 <= d1 ? 0 : 1;
          CHECK_FALSE(matched[match]);
          matched[match] = true;
          CHECK(std::min(d0, d1) < 5.0e-6);
        }
        CHECK(matched[0]);
        CHECK(matched[1]);
      }
    }
  }

  for (const auto& controlPoints : closedSplinePointControlSets()) {
    CAPTURE(controlPoints.size());
    LC_SplinePointsData data{true, false};
    data.useControlPoints = true;
    data.controlPoints = controlPoints;
    LC_SplinePoints source{nullptr, data};
    const double domain = static_cast<double>(source.getSegmentCount());
    for (const double fraction : cutFractions) {
      auto candidate = std::make_unique<LC_SplinePoints>(nullptr, data);
      LC_CurveJet cutJet;
      REQUIRE(candidate->tryEvaluateJet(domain * fraction,
                                       LC_CurveEvaluationSide::Right, cutJet));
      const auto* right = candidate->cut(cutJet.point);
      CHECK(right == nullptr);
      delete right;
      REQUIRE_FALSE(candidate->isClosed());
      CHECK(candidate->getData().cut);
      const auto expected = sampleSplinePoints(*candidate);
      const std::string pointCount = std::to_string(controlPoints.size()) + "pt";
      const std::string fractionLabel = std::to_string(static_cast<int>(fraction * 100));

      for (const auto& [version, label] : versions) {
        INFO(label);
        INFO(fraction);
        const auto path = std::filesystem::path(outputDirectory) /
            ("closed-cut-control-" + pointCount + "-q" + fractionLabel + "-" + version + ".dxf");
        REQUIRE(std::filesystem::exists(path));
        RS_Graphic converted;
        std::vector<SplineRecord> records;
        importFile(converted, path.string(), RS2::FormatDXFRW, &records);
        REQUIRE(records.size() == 1);
        CHECK((records.front().m_flags & 0x3) == 0x3);
        REQUIRE(converted.count() == 1);
        std::vector<RS_Vector> actual;
        if (auto* splinePoints = dynamic_cast<LC_SplinePoints*>(*converted.begin())) {
          CHECK_FALSE(splinePoints->isClosed());
          actual = sampleSplinePoints(*splinePoints);
        } else if (auto* spline = dynamic_cast<RS_Spline*>(*converted.begin())) {
          actual = splineCurve(*spline, 1000);
        } else {
          FAIL("ODA-converted closed-cut SPLINE imported as an unexpected entity type");
        }
        CHECK(curveDistance(expected, actual) < 1.0e-5);
      }
    }
  }
}

TEST_CASE(
    "Closed fit-point spline-points preserve geometry in file round trips",
    "[dxf][dwg][filter][spline-points][fit-points][closed][roundtrip]") {
  ensureSettings();
  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},         {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"}, {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"}, {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
  };
  LC_SplinePointsData data{true, false};
  data.splinePoints = {
      {0.0, 0.0}, {3.0, -1.0}, {5.0, 2.0}, {3.0, 5.0}, {-1.0, 3.0}};
  LC_SplinePoints source{nullptr, data};
  REQUIRE(source.isClosed());
  const auto sampleFit = [](const LC_SplinePoints &spline, int count) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= count; ++i) {
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / count,
                                    i == count ? LC_CurveEvaluationSide::Left
                                               : LC_CurveEvaluationSide::Right,
                                    jet));
      points.push_back(jet.point);
    }
    return points;
  };
  const auto expected = sampleFit(source, 1200);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  graphic.addEntity(new LC_SplinePoints(&graphic, data));
  for (const auto &[format, version] : formats) {
    INFO(version);
    const std::string path = tempPath("closed_fit_spline_points_" + version);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
    }
    if (format != RS2::FormatDXFRW)
      preserveSplineDwgFixture(path,
                               "closed-fit-spline-points-" + version + ".dwg");
    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    importFile(reopened, path,
               format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG,
               &written);
    std::filesystem::remove(path);
    REQUIRE(written.size() == 1);
    if (format != RS2::FormatDXFRW)
      CHECK(written.front().m_scenario == 1);
    CHECK((written.front().m_flags & 0x3) == 0x3);
    CHECK(written.front().m_fitPoints.empty());
    REQUIRE(reopened.count() == 1);
    auto *received = dynamic_cast<LC_SplinePoints *>(*reopened.begin());
    REQUIRE(received);
    CHECK(received->isClosed());
    CHECK(curveDistance(expected, sampleFit(*received, 1200)) < 1.0e-9);

    const char *odaDirectory =
        std::getenv("LIBRECAD_CLOSED_SPLINEPOINTS_ODA_DXF_DIR");
    if (format != RS2::FormatDXFRW && odaDirectory != nullptr &&
        *odaDirectory != '\0') {
      const auto converted = std::filesystem::path(odaDirectory) /
                             ("closed-fit-spline-points-" + version + ".dxf");
      REQUIRE(std::filesystem::exists(converted));
      RS_Graphic external;
      importFile(external, converted.string(), RS2::FormatDXFRW);
      REQUIRE(external.count() == 1);
      LC_SplinePoints *externalFit = nullptr;
      RS_Spline *externalSpline = nullptr;
      for (RS_Entity *entity : external) {
        externalFit = dynamic_cast<LC_SplinePoints *>(entity);
        externalSpline = dynamic_cast<RS_Spline *>(entity);
      }
      REQUIRE((externalFit != nullptr || externalSpline != nullptr));
      const auto actual = externalFit != nullptr
                              ? sampleFit(*externalFit, 1200)
                              : splineCurve(*externalSpline, 1200);
      CHECK(curveDistance(expected, actual) < 1.0e-5);
    }
  }
}

TEST_CASE("ODA-converted cut fit-point DWGs preserve both piece geometries",
          "[external][oda][dwg][spline-points][fit-points][cut][geometry]") {
  const char* outputDirectory = std::getenv("LIBRECAD_OFFSET_CUT_ODA_DXF_DIR");
  if (outputDirectory == nullptr || *outputDirectory == '\0')
    SKIP("Set LIBRECAD_OFFSET_CUT_ODA_DXF_DIR to converted cut-spline DXFs");
  ensureSettings();
  const std::vector<std::vector<RS_Vector>> fitPointSets{
      {{0.0, 0.0}, {2.0, 4.0}, {6.0, -1.0}, {10.0, 1.0}},
      {{0.0, 0.0}, {2.0, 4.0}, {5.0, -1.0}, {8.0, 3.0}, {11.0, 0.0}},
      {{0.0, 0.0}, {1.0, 3.0}, {3.0, 5.0}, {5.0, 1.0},
       {7.0, -2.0}, {9.0, 3.0}, {12.0, 0.0}},
      {{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}, {3.5, 3.0}, {5.0, 1.0},
       {6.5, -1.0}, {8.0, 0.0}, {10.0, 2.0}, {12.0, 0.0}},
      {{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}, {3.5, 3.0}, {5.0, 1.0},
       {6.5, -1.0}, {8.0, 0.0}, {9.0, 2.5}, {10.0, 4.0}, {11.0, 2.0},
       {13.0, 0.0}},
  };
  const std::array<double, 3> cutFractions{0.25, 0.5, 0.75};
  const std::vector<std::pair<std::string, std::string>> versions{
      {"dwg-r2000", "R2000"}, {"dwg-r2004", "R2004"},
      {"dwg-r2007", "R2007"}, {"dwg-r2010", "R2010"},
      {"dwg-r2013", "R2013"}, {"dwg-r2018", "R2018"},
  };
  const auto sample = [](const LC_SplinePoints& spline) {
    std::vector<RS_Vector> points;
    const double end = static_cast<double>(spline.getSegmentCount());
    for (int i = 0; i <= 1000; ++i) {
      LC_CurveJet jet;
      REQUIRE(spline.tryEvaluateJet(end * i / 1000.0,
          i == 1000 ? LC_CurveEvaluationSide::Left
                    : LC_CurveEvaluationSide::Right, jet));
      points.push_back(jet.point);
    }
    return points;
  };

  for (const auto& fitPoints : fitPointSets) {
    CAPTURE(fitPoints.size());
    LC_SplinePointsData data{false, false};
    data.splinePoints = fitPoints;
    LC_SplinePoints source{nullptr, data};
    const double domain = static_cast<double>(source.getSegmentCount());
    for (const double fraction : cutFractions) {
      INFO(fraction);
      LC_SplinePoints candidate{nullptr, data};
      LC_CurveJet cutJet;
      REQUIRE(candidate.tryEvaluateJet(domain * fraction,
                                       LC_CurveEvaluationSide::Right, cutJet));
      std::unique_ptr<LC_SplinePoints> right(candidate.cut(cutJet.point));
      REQUIRE(right);
      LC_SplinePoints left{nullptr, candidate.getData()};
      const std::array<std::vector<RS_Vector>, 2> expected{sample(left), sample(*right)};
      const std::string pointCount = std::to_string(fitPoints.size()) + "pt";
      const std::string fractionLabel = std::to_string(static_cast<int>(fraction * 100));

      for (const auto& [version, label] : versions) {
        INFO(label);
        const auto path = std::filesystem::path(outputDirectory) /
            ("cut-fit-control-" + pointCount + "-q" + fractionLabel + "-" + version + ".dxf");
        REQUIRE(std::filesystem::exists(path));
        RS_Graphic converted;
        std::vector<SplineRecord> records;
        importFile(converted, path.string(), RS2::FormatDXFRW, &records);
        REQUIRE(records.size() == 2);
        REQUIRE(converted.count() == 2);
        std::array<bool, 2> matched{false, false};
        for (RS_Entity* entity : converted) {
          std::vector<RS_Vector> actual;
          if (auto* splinePoints = dynamic_cast<LC_SplinePoints*>(entity)) {
            actual = sample(*splinePoints);
          } else if (auto* spline = dynamic_cast<RS_Spline*>(entity)) {
            actual = splineCurve(*spline, 1000);
          } else {
            FAIL("ODA-converted cut fit SPLINE imported as an unexpected entity type");
          }
          const double d0 = curveDistance(expected[0], actual);
          const double d1 = curveDistance(expected[1], actual);
          const size_t match = d0 <= d1 ? 0 : 1;
          CHECK_FALSE(matched[match]);
          matched[match] = true;
          CHECK(std::min(d0, d1) < 5.0e-6);
        }
        CHECK(matched[0]);
        CHECK(matched[1]);
      }
    }
  }
}

#endif

TEST_CASE("Tangent-constrained quadratic SPLINE keeps its explicit control net",
          "[dxf][filter][spline][fit-points][control-net]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 2;
  source.m_controlPoints = {{0., 0.}, {2., 8.}, {8., 8.}, {10., 0.}};
  source.m_knots = {0., 0., 0., 0.5, 1., 1., 1.};
  source.m_fitPoints = {{0., 0.}, {3., 4.}, {7., 4.}, {10., 0.}};
  source.m_tangentStart = {1., 0., 0.};
  source.m_tangentEnd = {0., -1., 0.};

  RS_Graphic graphic;
  std::vector<SplineRecord> parsed;
  importDxf(graphic, source, "tangent_quadratic_explicit_control_net", &parsed);
  REQUIRE(parsed.size() == 1);
  CHECK(parsed.front().m_controlPoints.size() == source.m_controlPoints.size());

  RS_Spline *spline = onlySpline(graphic);
  const RS_SplineData &data = spline->getData();
  CHECK(data.degree == 2);
  CHECK(data.controlPoints.size() == source.m_controlPoints.size());
  CHECK(data.knotslist == source.m_knots);
  CHECK(data.fitPoints.size() == source.m_fitPoints.size());
  CHECK(data.fitTangentsConstrained);
  CHECK(data.fitStartTangent.x == source.m_tangentStart.x);
  CHECK(data.fitStartTangent.y == source.m_tangentStart.y);
  CHECK(data.fitEndTangent.x == source.m_tangentEnd.x);
  CHECK(data.fitEndTangent.y == source.m_tangentEnd.y);

  RS_SplineData expected(2, false);
  expected.degree = 2;
  expected.controlPoints = source.m_controlPoints;
  expected.knotslist = source.m_knots;
  expected.weights.assign(source.m_controlPoints.size(), 1.0);
  for (int i = 0; i <= 32; ++i) {
    const double t = static_cast<double>(i) / 32.0;
    const RS_Vector actual = RS_Spline::evaluateNURBS(data, t);
    const RS_Vector reference = RS_Spline::evaluateNURBS(expected, t);
    REQUIRE(actual.valid);
    REQUIRE(reference.valid);
    CHECK(actual.distanceTo(reference) <= 1.0e-12);
  }

  const std::string path = tempPath("tangent_quadratic_explicit_control_net_roundtrip.dxf");
  {
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), RS2::FormatDXFRW));
  }
  RS_Graphic reopened;
  std::vector<SplineRecord> written;
  importFile(reopened, path, RS2::FormatDXFRW, &written);
  std::filesystem::remove(path);
  REQUIRE(written.size() == 1);
  CHECK(written.front().m_tangentStart.x == source.m_tangentStart.x);
  CHECK(written.front().m_tangentStart.y == source.m_tangentStart.y);
  CHECK(written.front().m_tangentEnd.x == source.m_tangentEnd.x);
  CHECK(written.front().m_tangentEnd.y == source.m_tangentEnd.y);
  const RS_SplineData &reopenedData = onlySpline(reopened)->getData();
  CHECK(reopenedData.fitStartTangent.x == source.m_tangentStart.x);
  CHECK(reopenedData.fitStartTangent.y == source.m_tangentStart.y);
  CHECK(reopenedData.fitEndTangent.x == source.m_tangentEnd.x);
  CHECK(reopenedData.fitEndTangent.y == source.m_tangentEnd.y);
}

TEST_CASE("Cubic fit-point SPLINE intent survives file round trips",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();

  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 3;
  source.m_fitPoints = {{0., 0.}, {2., 4.}, {5., 3.}, {8., 0.}};
  const std::string inputPath = tempPath("cubic_fit_point_source.dxf");
  {
    std::ofstream out(inputPath);
    out << splineDxf(source);
  }

  RS_Graphic first;
  importFile(first, inputPath, RS2::FormatDXFRW);
  std::filesystem::remove(inputPath);
  RS_Spline *sourceSpline = onlySpline(first);
  REQUIRE(sourceSpline->getData().degree == 3);
  checkSameVectors(sourceSpline->getData().fitPoints, source.m_fitPoints);
  CHECK(std::abs(sourceSpline->getData().fitTolerance - 1.0e-10) < 1.0e-20);

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("cubic_fit_point_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(first, QString::fromStdString(path), format));
    }

    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);

    REQUIRE(written.size() == 1);
    CHECK(written.front().m_controlPoints.empty());
    CHECK(written.front().m_fitPoints.size() == source.m_fitPoints.size());
    checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
    CHECK(std::abs(written.front().m_tolFit - 1.0e-10) < 1.0e-20);
    if (format != RS2::FormatDXFRW)
      CHECK(written.front().m_scenario == 2);

    RS_Spline *reopenedSpline = onlySpline(reopened);
    CHECK(reopenedSpline->getData().degree == 3);
    checkSameVectors(reopenedSpline->getData().fitPoints, source.m_fitPoints);
    const auto before = splineCurve(*sourceSpline, 100);
    const auto after = splineCurve(*reopenedSpline, 100);
    REQUIRE(before.size() == after.size());
    for (std::size_t i = 0; i < before.size(); ++i)
      CHECK(before[i].distanceTo(after[i]) < 1.0e-8);
  }
}

TEST_CASE("Coherent cubic SPLINE control and fit lists retain fit intent",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 3;
  source.m_fitPoints = {{0., 0.}, {2., 4.}, {5., 3.}, {8., 0.}};
  RS_SplineData generatedData(3, false);
  RS_Spline generated(nullptr, generatedData);
  generated.setFitPoints(source.m_fitPoints);
  source.m_controlPoints = generated.getData().controlPoints;
  source.m_knots = generated.getData().knotslist;

  RS_Graphic graphic;
  importDxf(graphic, source, "cubic_fit_point_both_lists");
  RS_Spline *imported = onlySpline(graphic);
  checkSameVectors(imported->getData().fitPoints, source.m_fitPoints);

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("cubic_fit_point_both_lists_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
    }
    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);
    REQUIRE(written.size() == 1);
    CHECK(written.front().m_controlPoints.empty());
    checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
    if (format != RS2::FormatDXFRW)
      CHECK(written.front().m_scenario == 2);
    checkSameVectors(onlySpline(reopened)->getData().fitPoints,
                     source.m_fitPoints);
  }
}

TEST_CASE("Closed cubic fit SPLINE safely round-trips as a control net",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 3;
  source.m_degree = 3;
  source.m_fitPoints = {{0., 0.}, {2., 0.}, {3., 2.}, {1., 4.}, {-1., 2.}, {0., 0.}};
  RS_Graphic graphic;
  importDxf(graphic, source, "closed_cubic_fit_point_source");
  RS_Spline *imported = onlySpline(graphic);
  REQUIRE(imported->validate());
  REQUIRE(imported->isClosed());
  const auto sourceCurve = splineCurve(*imported, 100);

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("closed_cubic_fit_point_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
    }
    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);
    REQUIRE(written.size() == 1);
    CHECK(written.front().m_fitPoints.empty());
    CHECK_FALSE(written.front().m_controlPoints.empty());
    if (format != RS2::FormatDXFRW)
      CHECK(written.front().m_scenario == 1);
    RS_Spline *reopenedSpline = onlySpline(reopened);
    REQUIRE(reopenedSpline->validate());
    const auto reopenedCurve = splineCurve(*reopenedSpline, 100);
    REQUIRE(sourceCurve.size() == reopenedCurve.size());
    for (std::size_t i = 0; i < sourceCurve.size(); ++i)
      CHECK(sourceCurve[i].distanceTo(reopenedCurve[i]) < 2.5e-7);
  }
}

TEST_CASE("Edited cubic fit-point SPLINE exports its current control net",
          "[dxf][filter][spline][fit-points][roundtrip]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 3;
  source.m_fitPoints = {{0., 0.}, {2., 4.}, {5., 3.}, {8., 0.}};
  RS_Graphic graphic;
  importDxf(graphic, source, "edited_cubic_fit_point_source");
  RS_Spline *spline = onlySpline(graphic);
  REQUIRE(spline->getData().fitPoints.size() == 4);
  const RS_Vector edited =
      spline->getData().controlPoints[1] + RS_Vector{0.25, 0.5};
  spline->getData().controlPoints[1] = edited;
  spline->update();

  const std::string path = tempPath("edited_cubic_fit_point_export.dxf");
  std::remove(path.c_str());
  {
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, QString::fromStdString(path),
                              RS2::FormatDXFRW));
  }
  RS_Graphic reopened;
  std::vector<SplineRecord> written;
  importFile(reopened, path, RS2::FormatDXFRW, &written);
  std::filesystem::remove(path);
  REQUIRE(written.size() == 1);
  CHECK(written.front().m_fitPoints.empty());
  CHECK(written.front().m_controlPoints.size() ==
        spline->getUnwrappedControlPoints().size());
  RS_Spline *reopenedSpline = onlySpline(reopened);
  CHECK(reopenedSpline->getData().fitPoints.empty());
  CHECK(reopenedSpline->getData().controlPoints[1].distanceTo(edited) <
        1.0e-12);
}

TEST_CASE("Tangent-constrained cubic fit SPLINE preserves its verified DWG fit representation",
          "[dxf][dwg][filter][spline][fit-points][roundtrip]") {
  ensureSettings();
  SplineRecord source;
  source.m_flags = 8;
  source.m_degree = 3;
  source.m_fitPoints = {{0., 0.}, {2., 4.}, {5., 3.}, {8., 0.}};
  source.m_tangentStart = {1., 0., 0.};
  source.m_tangentEnd = {0., 1., 0.};
  RS_Graphic graphic;
  importDxf(graphic, source, "tangent_cubic_fit_point_source");
  RS_Spline *sourceSpline = onlySpline(graphic);
  REQUIRE(sourceSpline->getData().fitTangentsConstrained);
  const auto sourceCurve = splineCurve(*sourceSpline, 100);

  const std::vector<std::pair<RS2::FormatType, std::string>> formats{
      {RS2::FormatDXFRW, "dxf"},
#ifdef DWGSUPPORT
      {RS2::FormatDWG, "dwg-r2000"},
      {RS2::FormatDWG2004, "dwg-r2004"},
      {RS2::FormatDWG2007, "dwg-r2007"},
      {RS2::FormatDWG2010, "dwg-r2010"},
      {RS2::FormatDWG2013, "dwg-r2013"},
      {RS2::FormatDWG2018, "dwg-r2018"},
#endif
  };
  for (const auto &[format, suffix] : formats) {
    INFO(suffix);
    const std::string path = tempPath("tangent_cubic_fit_point_" + suffix);
    std::remove(path.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(path), format));
    }
    if (format != RS2::FormatDXFRW)
      preserveSplineDwgFixture(path, "both-tangent-" + suffix + ".dwg");
    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    const RS2::FormatType readFormat =
        format == RS2::FormatDXFRW ? RS2::FormatDXFRW : RS2::FormatDWG;
    importFile(reopened, path, readFormat, &written);
    std::filesystem::remove(path);

    REQUIRE(written.size() == 1);
    if (format == RS2::FormatDXFRW) {
      checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
      CHECK(written.front().m_tangentStart.x == source.m_tangentStart.x);
      CHECK(written.front().m_tangentEnd.y == source.m_tangentEnd.y);
    } else {
      CHECK(written.front().m_scenario == 2);
      checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
      CHECK(written.front().m_tangentStart.x == source.m_tangentStart.x);
      CHECK(written.front().m_tangentEnd.y == source.m_tangentEnd.y);
      CHECK(written.front().m_controlPoints.empty());
    }
    if (format == RS2::FormatDXFRW)
      CHECK_FALSE(written.front().m_controlPoints.empty());
    RS_Spline *reopenedSpline = onlySpline(reopened);
    const auto reopenedCurve = splineCurve(*reopenedSpline, 100);
    REQUIRE(sourceCurve.size() == reopenedCurve.size());
    for (std::size_t i = 0; i < sourceCurve.size(); ++i)
      CHECK(sourceCurve[i].distanceTo(reopenedCurve[i]) < 1.0e-8);
  }
}

TEST_CASE("Cubic fit SPLINE import honors one tangent and naturally bounds the other",
          "[dxf][filter][spline][fit-points][tangents]") {
  ensureSettings();
  const std::vector<RS_Vector> fitPoints{{0., 0.}, {2., 3.}, {6., 4.}, {9., 0.}};
  for (const bool constrainStart : {true, false}) {
    SplineRecord source;
    source.m_flags = 8;
    source.m_degree = 3;
    source.m_fitPoints = fitPoints;
    const RS_Vector requested{2., -1.};
    source.m_tangentStart = constrainStart ? requested : RS_Vector{0., 0., 0.};
    source.m_tangentEnd = constrainStart ? RS_Vector{0., 0., 0.} : requested;

    RS_Graphic graphic;
    importDxf(graphic, source, constrainStart ? "cubic_fit_start_tangent" : "cubic_fit_end_tangent");
    RS_Spline* spline = onlySpline(graphic);
    REQUIRE(spline->validate());
    CHECK(spline->getData().fitTangentsConstrained);
    CHECK(spline->getData().fitStartTangent == source.m_tangentStart);
    CHECK(spline->getData().fitEndTangent == source.m_tangentEnd);
    CHECK(spline->getData().fitPoints == fitPoints);

    LC_CurveJet startJet;
    LC_CurveJet endJet;
    REQUIRE(spline->tryEvaluateJet(0., LC_CurveEvaluationSide::Right, startJet));
    REQUIRE(spline->tryEvaluateJet(1., LC_CurveEvaluationSide::Left, endJet));
    const RS_Vector unit = requested / requested.magnitude();
    if (constrainStart) {
      CHECK(startJet.first.distanceTo(unit) < 1e-8);
      CHECK(endJet.second.magnitude() < 1e-8);
    } else {
      CHECK(startJet.second.magnitude() < 1e-8);
      CHECK(endJet.first.distanceTo(unit) < 1e-8);
    }
    for (const auto& point : fitPoints)
      CHECK(spline->getNearestPointOnEntity(point, true).distanceTo(point) < 1e-7);

    const auto expectedCurve = splineCurve(*spline, 100);
    const std::string dxfPath = tempPath(constrainStart
        ? "cubic_fit_start_tangent_roundtrip" : "cubic_fit_end_tangent_roundtrip");
    std::remove(dxfPath.c_str());
    {
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(graphic, QString::fromStdString(dxfPath), RS2::FormatDXFRW));
    }
    RS_Graphic reopened;
    std::vector<SplineRecord> written;
    importFile(reopened, dxfPath, RS2::FormatDXFRW, &written);
    std::filesystem::remove(dxfPath);
    REQUIRE(written.size() == 1);
    CHECK(written.front().m_tangentStart.x == source.m_tangentStart.x);
    CHECK(written.front().m_tangentStart.y == source.m_tangentStart.y);
    CHECK(written.front().m_tangentEnd.x == source.m_tangentEnd.x);
    CHECK(written.front().m_tangentEnd.y == source.m_tangentEnd.y);
    checkSameVectors(written.front().m_fitPoints, source.m_fitPoints);
    RS_Spline* reopenedSpline = onlySpline(reopened);
    REQUIRE(reopenedSpline->validate());
    CHECK(reopenedSpline->getData().fitStartTangent == spline->getData().fitStartTangent);
    CHECK(reopenedSpline->getData().fitEndTangent == spline->getData().fitEndTangent);
    const auto reopenedCurve = splineCurve(*reopenedSpline, 100);
    REQUIRE(expectedCurve.size() == reopenedCurve.size());
    for (std::size_t i = 0; i < expectedCurve.size(); ++i)
      CHECK(expectedCurve[i].distanceTo(reopenedCurve[i]) < 1e-8);

#ifdef DWGSUPPORT
    const std::string dwgPath = tempPath(constrainStart
        ? "cubic_fit_start_tangent_refusal" : "cubic_fit_end_tangent_refusal");
    std::remove(dwgPath.c_str());
    RS_FilterDXFRW dwgWriter;
    CHECK_FALSE(dwgWriter.fileExport(graphic, QString::fromStdString(dwgPath), RS2::FormatDWG2018));
    CHECK_FALSE(std::filesystem::exists(dwgPath));
#endif
  }
}
