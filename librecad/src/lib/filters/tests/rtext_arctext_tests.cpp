/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * RTEXT (RText / ODA 1159) and ARCALIGNEDTEXT (AcDbArcAlignedText / ODA 1163)
 * read tests, on both the DXF and DWG paths.
 *
 * Both are AutoCAD Express Tools entities that previously fell to the raw
 * DRW_UnsupportedObject path.  They are now decoded and mapped onto DRW_Text so
 * LibreCAD renders them, delivered via addText:
 *   - RTEXT           -> DRW_RText : DRW_Text (literal text or DIESEL expression)
 *   - ARCALIGNEDTEXT  -> DRW_ArcAlignedText : DRW_Text (text at the arc mid-point)
 *
 * Field layouts follow the read-only dwg-ts reference decoder (parseRText /
 * parseArcAlignedText, makeRTextParserDxf / makeArcAlignedTextParserDxf).
 *
 * DWG fixture provenance: an inline DXF holding one RTEXT and one
 * ARCALIGNEDTEXT (matching CLASS entries, the RText subclass marker being the
 * class C++ name "RText" not "AcDbRText") was converted DXF->DWG (AC1032) with
 * the ODA File Converter 27.1.0.  testdata/rtext_arctext.dwg was confirmed with
 * the read-only dwg-ts parser (cad-to-json): RTEXT text "RTEXT-DIESEL-TEST" at
 * (100,200), height 2.5, rotation 30 deg; ARCALIGNEDTEXT text "ARC-TEXT-TEST",
 * center (50,60), radius 25, start 0 / end 90 deg, style "Standard", font
 * "Arial".  This test asserts libdxfrw delivers both *typed* through addText
 * with those values.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"
#include "libdxfrw.h"

using Catch::Approx;

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

struct CapturedText {
  bool isArcText = false;
  bool isRText = false;
  std::string text, style, fontName, textSize;
  double bx = 0, by = 0, height = 0, angle = 0;
  double cx = 0, cy = 0, radius = 0, startAngle = 0, endAngle = 0;
  int rtextFlags = 0;
};

// Captures every text delivered via addText, recording which mapped subclass it
// came from (RText / ArcAlignedText) via dynamic_cast, plus the raw counts so a
// test can prove typed delivery (addText, not addUnsupportedObject).
class TextCapture : public StubInterface {
public:
  int m_textCount = 0;
  int m_mtextCount = 0;
  int m_unsupportedCount = 0;
  std::vector<CapturedText> texts;

  void addText(const DRW_Text &d) override {
    ++m_textCount;
    CapturedText c;
    c.text = d.text;
    c.style = d.style;
    c.bx = d.basePoint.x;
    c.by = d.basePoint.y;
    c.height = d.height;
    c.angle = d.angle;
    if (const auto *at = dynamic_cast<const DRW_ArcAlignedText *>(&d)) {
      c.isArcText = true;
      c.cx = at->m_center.x;
      c.cy = at->m_center.y;
      c.radius = at->m_radius;
      c.startAngle = at->m_startAngle;
      c.endAngle = at->m_endAngle;
      c.fontName = at->m_fontName;
      c.textSize = at->m_textSize;
    } else if (const auto *rt = dynamic_cast<const DRW_RText *>(&d)) {
      c.isRText = true;
      c.rtextFlags = rt->m_rTextFlags;
    }
    texts.push_back(c);
  }
  void addMText(const DRW_MText &) override { ++m_mtextCount; }
  void addUnsupportedObject(const DRW_UnsupportedObject &) override {
    ++m_unsupportedCount;
  }

  const CapturedText *findRText() const {
    for (const auto &c : texts)
      if (c.isRText)
        return &c;
    return nullptr;
  }
  const CapturedText *findArcText() const {
    for (const auto &c : texts)
      if (c.isArcText)
        return &c;
    return nullptr;
  }
};

void readDxf(const std::string &dxf, DRW_Interface &cap, const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW r(path.string().c_str());
  REQUIRE(r.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

// Inline DXF holding one RTEXT and one ARCALIGNEDTEXT with the same values as
// the committed DWG fixture (start/end angles in DXF degrees).
const char *kTextEntities =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nRTEXT\n5\nA0\n100\nAcDbEntity\n8\n0\n100\nRText\n"
    "7\nStandard\n10\n100.0\n20\n200.0\n30\n0.0\n40\n2.5\n50\n30.0\n70\n0\n"
    "1\nRTEXT-DIESEL-TEST\n210\n0.0\n220\n0.0\n230\n1.0\n"
    "0\nARCALIGNEDTEXT\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbArcAlignedText\n"
    "1\nARC-TEXT-TEST\n2\nArial\n7\nStandard\n"
    "10\n50.0\n20\n60.0\n40\n25.0\n42\n2.5\n50\n0.0\n51\n90.0\n72\n0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF RTEXT is read as text via processRText", "[dxf][rtext]") {
  TextCapture cap;
  readDxf(kTextEntities, cap, "lc_rtext.dxf");

  const CapturedText *rt = cap.findRText();
  REQUIRE(rt != nullptr);
  CHECK(rt->text == "RTEXT-DIESEL-TEST");
  CHECK(rt->bx == Approx(100.0));
  CHECK(rt->by == Approx(200.0));
  CHECK(rt->height == Approx(2.5));
  CHECK(rt->angle == Approx(30.0)); // DRW_Text angle is in degrees
  CHECK(rt->style == "Standard");
  CHECK(rt->rtextFlags == 0);
}

TEST_CASE("DXF ARCALIGNEDTEXT is read as text via processArcAlignedText",
          "[dxf][arcalignedtext]") {
  TextCapture cap;
  readDxf(kTextEntities, cap, "lc_arctext.dxf");

  const CapturedText *at = cap.findArcText();
  REQUIRE(at != nullptr);
  CHECK(at->text == "ARC-TEXT-TEST");
  CHECK(at->fontName == "Arial");
  CHECK(at->style == "Standard");
  CHECK(at->textSize == "2.5");
  CHECK(at->cx == Approx(50.0));
  CHECK(at->cy == Approx(60.0));
  CHECK(at->radius == Approx(25.0));
  CHECK(at->startAngle == Approx(0.0));
  CHECK(at->endAngle == Approx(M_PI / 2)); // 90 deg -> radians

  // 2D approximation: text at the arc mid-point (angle pi/4), baseline tangent.
  const double mid = M_PI / 4;
  CHECK(at->bx == Approx(50.0 + 25.0 * std::cos(mid)));
  CHECK(at->by == Approx(60.0 + 25.0 * std::sin(mid)));
  CHECK(at->angle == Approx((mid + M_PI / 2) * 57.29577951308232)); // deg
  CHECK(at->height == Approx(2.5)); // from the textSize D2T string
}

TEST_CASE("DWG RTEXT + ARCALIGNEDTEXT are read as text via parseDwg",
          "[dwg][rtext][arcalignedtext]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/rtext_arctext.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("rtext_arctext.dwg fixture not found; skipping");
    return;
  }

  TextCapture cap;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  REQUIRE(ok);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // Both must arrive typed through addText (never addUnsupportedObject for these
  // two — the DWG dispatch breaks straight after addText).
  const CapturedText *rt = cap.findRText();
  const CapturedText *at = cap.findArcText();
  REQUIRE(rt != nullptr);
  REQUIRE(at != nullptr);

  // RTEXT — cross-checked against dwg-ts cad-to-json of the same fixture.
  CHECK(rt->text == "RTEXT-DIESEL-TEST");
  CHECK(rt->bx == Approx(100.0));
  CHECK(rt->by == Approx(200.0));
  CHECK(rt->height == Approx(2.5));
  CHECK(rt->angle == Approx(30.0)); // 0.5236 rad * ARAD
  CHECK(rt->style == "Standard");   // resolved from the style handle

  // ARCALIGNEDTEXT — cross-checked against the same oracle.
  CHECK(at->text == "ARC-TEXT-TEST");
  CHECK(at->fontName == "Arial");
  CHECK(at->style == "Standard");
  CHECK(at->cx == Approx(50.0));
  CHECK(at->cy == Approx(60.0));
  CHECK(at->radius == Approx(25.0));
  CHECK(at->startAngle == Approx(0.0));
  CHECK(at->endAngle == Approx(M_PI / 2));
  const double mid = M_PI / 4;
  CHECK(at->bx == Approx(50.0 + 25.0 * std::cos(mid)));
  CHECK(at->by == Approx(60.0 + 25.0 * std::sin(mid)));
}

// Dev-local gated read of the large BYDD00301 sample.  NOTE: two independent
// oracles (dwg-ts cad-to-json AND the ODA File Converter DWG->DXF export) agree
// that this file's CLASSES section *declares* RTEXT + ARCALIGNEDTEXT but holds
// no live instances of either, so nothing is delivered — this only asserts the
// file still reads cleanly with the new dispatch in place.  Hidden tag; the
// 31 MB read is skipped in the default suite and when the file is absent.
TEST_CASE("DWG BYDD00301 sample reads cleanly with RTEXT/ARCALIGNEDTEXT dispatch",
          "[.rtext_bydd]") {
  const std::string dir = "d:/data/dli/doc/dwg4";
  if (!std::filesystem::is_directory(dir)) {
    SUCCEED("BYDD corpus directory not present; skipping");
    return;
  }
  std::string path;
  for (const auto &e : std::filesystem::directory_iterator(dir)) {
    const std::string name = e.path().filename().string();
    if (name.rfind("BYDD00301", 0) == 0 &&
        e.path().extension() == ".dwg") {
      path = e.path().string();
      break;
    }
  }
  if (path.empty()) {
    SUCCEED("BYDD00301*.dwg not found; skipping");
    return;
  }

  TextCapture cap;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  REQUIRE(ok);
  CHECK(reader.getError() == DRW::BAD_NONE);
  // Any RTEXT/ARCALIGNEDTEXT that *were* live would arrive typed, not raw.
  const int typed = (cap.findRText() ? 1 : 0) + (cap.findArcText() ? 1 : 0);
  INFO("live RTEXT/ARCALIGNEDTEXT instances delivered: " << typed);
  SUCCEED();
}
