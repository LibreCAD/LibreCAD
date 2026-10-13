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
 * R2018 (AC1032) multi-line attributes in whole DWG files.
 *
 * A multi-line ATTRIB / ATTDEF holds an embedded MTEXT. AutoCAD stores every
 * MTEXT field in it, the R2018 "not annotative" block included, and puts the
 * attribute's own text style handle ahead of the handles the MTEXT refers to.
 * libdxfrw used to read and write a shorter form, which turned the fields
 * after the MTEXT - field length, flags - into leftovers of that block: 293
 * and 104 for an attribute AutoCAD's own DXF lists with 0 and no field length.
 *
 * Fixture: testdata/lc_multiline_attrib_ac1032.dwg, see testdata/PROVENANCE.md.
 * ezdxf wrote a DXF with block LC_ML_BLOCK - a multi-line and a single-line
 * attribute definition - and one reference to it; the ODA File Converter
 * 27.1.0 turned that into DWG. The converter writes the same form as AutoCAD.
 *
 * The object-level counterpart, with the bits of an attribute AutoCAD itself
 * wrote, is in dwg_entity_encode_round_trip_tests.cpp.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "drw_objects.h"
#include "libdwgr.h"

namespace {

// Keeps the attributes of a drawing and the names behind table handles; as a
// writer it emits one block with a multi-line and a single-line attribute.
class AttributeIface : public DRW_Interface {
public:
  dwgRW *m_writer = nullptr;
  std::uint32_t m_blockRecord = 0;
  std::map<std::uint32_t, std::string> m_layers;
  std::map<std::uint32_t, std::string> m_lineTypes;
  std::map<std::uint32_t, std::string> m_textStyles;
  std::vector<DRW_Attdef> m_attdefs;
  std::vector<DRW_Attrib> m_attribs;

  void addHeader(const DRW_Header *) override {}
  void addLType(const DRW_LType &data) override {
    m_lineTypes[data.handle] = data.name;
  }
  void addLayer(const DRW_Layer &data) override {
    m_layers[data.handle] = data.name;
  }
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addTextStyle(const DRW_Textstyle &data) override {
    m_textStyles[data.handle] = data.name;
  }
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
  void addInsert(const DRW_Insert &data) override {
    for (const auto &attrib : data.attlist) {
      if (attrib != nullptr)
        m_attribs.push_back(*attrib);
    }
  }
  void addAttDef(const DRW_Attdef &data) override { m_attdefs.push_back(data); }
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
  void linkImage(const DRW_ImageDef *) override {}
  void addComment(const char *) override {}
  void addPlotSettings(const DRW_PlotSettings *) override {}
  void writeHeader(DRW_Header &) override {}
  void writeBlockRecords() override {}
  void writeLTypes() override {}
  void writeLayers() override {}
  void writeTextstyles() override {}
  void writeVports() override {}
  void writeDimstyles() override {}
  void writeObjects() override {}
  void writeAppId() override {}

  // Not annotative, as AutoCAD writes a plain attribute, and with the handles
  // that came with the MTEXT from AutoCAD's drawing: there 0x10 is layer "0",
  // 0x15 linetype ByLayer and 0x11 text style Standard. In a drawing libdxfrw
  // writes, all three are linetypes or a dimension style.
  static std::unique_ptr<DRW_MText> foreignMText(const char *text,
                                                 const DRW_Coord &at) {
    auto mtext = std::make_unique<DRW_MText>();
    mtext->basePoint = at;
    mtext->secPoint = DRW_Coord{1.0, 0.0, 0.0};
    mtext->height = 2.5;
    mtext->widthscale = 30.0;
    mtext->textgen = 1;
    mtext->alignH = static_cast<DRW_Text::HAlign>(5);
    mtext->text = text;
    mtext->m_r2018IsNotAnnotative = true;
    mtext->m_r2018Version = 4;
    mtext->m_r2018DefaultFlag = true;
    mtext->setDwgLayerHandle(0x10);
    mtext->setDwgLinetypeHandle(0x15);
    mtext->lineType.clear();
    mtext->styleH.ref = 0x11;
    return mtext;
  }

  void writeBlocks() override {
    if (m_writer == nullptr)
      return;
    m_blockRecord =
        m_writer->defineBlock("LC_ML_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(m_blockRecord != 0);
    REQUIRE(m_writer->beginBlockContent(m_blockRecord));

    DRW_Attdef multiLine;
    multiLine.basePoint = DRW_Coord{2.0, 12.0, 0.0};
    multiLine.secPoint = multiLine.basePoint;
    multiLine.height = 2.5;
    multiLine.tag = "ML_TAG";
    multiLine.prompt = "Multi line prompt";
    multiLine.lockPosition = true;
    multiLine.m_attributeType = 4;
    // An MTEXT without the R2018 block: the form of an annotative one.
    multiLine.mtext = std::make_unique<DRW_MText>();
    multiLine.mtext->basePoint = multiLine.basePoint;
    multiLine.mtext->secPoint = DRW_Coord{1.0, 0.0, 0.0};
    multiLine.mtext->height = 2.5;
    multiLine.mtext->text = "default line 1\\Pdefault line 2";
    REQUIRE(m_writer->writeAttdef(&multiLine));

    DRW_Attdef singleLine;
    singleLine.basePoint = DRW_Coord{2.0, 2.0, 0.0};
    singleLine.secPoint = singleLine.basePoint;
    singleLine.height = 2.5;
    singleLine.text = "single default";
    singleLine.tag = "SL_TAG";
    singleLine.prompt = "Single line prompt";
    singleLine.attribFlags = 8;
    REQUIRE(m_writer->writeAttdef(&singleLine));
    REQUIRE(m_writer->endBlockContent());
  }

  void writeEntities() override {
    if (m_writer == nullptr || m_blockRecord == 0)
      return;
    DRW_Insert insert;
    insert.basePoint = DRW_Coord{100.0, 50.0, 0.0};
    insert.blockRecH.ref = m_blockRecord;

    auto multiLine = std::make_shared<DRW_Attrib>();
    multiLine->basePoint = DRW_Coord{102.0, 62.0, 0.0};
    multiLine->secPoint = multiLine->basePoint;
    multiLine->height = 2.5;
    multiLine->tag = "ML_TAG";
    multiLine->lockPosition = true;
    multiLine->m_attributeType = 2;
    multiLine->mtext =
        foreignMText("value line 1\\Pvalue line 2", multiLine->basePoint);
    insert.attlist.push_back(std::move(multiLine));

    auto singleLine = std::make_shared<DRW_Attrib>();
    singleLine->basePoint = DRW_Coord{102.0, 52.0, 0.0};
    singleLine->secPoint = singleLine->basePoint;
    singleLine->height = 2.5;
    singleLine->text = "single value";
    singleLine->tag = "SL_TAG";
    singleLine->attribFlags = 8;
    insert.attlist.push_back(std::move(singleLine));
    REQUIRE(m_writer->writeInsert(&insert));
  }

  const DRW_Attrib *attrib(const std::string &tag) const {
    for (const DRW_Attrib &candidate : m_attribs) {
      if (candidate.tag == tag)
        return &candidate;
    }
    return nullptr;
  }

  const DRW_Attdef *attdef(const std::string &tag) const {
    for (const DRW_Attdef &candidate : m_attdefs) {
      if (candidate.tag == tag)
        return &candidate;
    }
    return nullptr;
  }

  static std::string name(const std::map<std::uint32_t, std::string> &table,
                          std::uint32_t handle) {
    const auto found = table.find(handle);
    return found == table.end() ? std::string("<no such record>")
                                : found->second;
  }
};

// Layer "0", linetype ByLayer and text style Standard, whatever handles and
// spelling the drawing gives them.
void checkEmbeddedTableHandles(const AttributeIface &drawing,
                               const DRW_MText &mtext) {
  CHECK(AttributeIface::name(drawing.m_layers, mtext.dwgLayerHandle()) == "0");
  if (mtext.dwgLinetypeHandle() != 0) {
    const std::string lineType =
        AttributeIface::name(drawing.m_lineTypes, mtext.dwgLinetypeHandle());
    CHECK((lineType == "ByLayer" || lineType == "BYLAYER"));
  }
  const std::string style =
      AttributeIface::name(drawing.m_textStyles, mtext.styleH.ref);
  CHECK((style == "Standard" || style == "STANDARD"));
}

std::string autoCadSamplePath() {
  if (const char *root = std::getenv("LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR");
      root != nullptr && *root != '\0') {
    const auto candidate =
        std::filesystem::path(root) / "sample_AC1032.dwg";
    if (std::filesystem::is_regular_file(candidate))
      return candidate.string();
  }
  if (const char *home = std::getenv("HOME"); home != nullptr) {
    const auto candidate = std::filesystem::path(home) / "dev" / "ACadSharp" /
                           "samples" / "sample_AC1032.dwg";
    if (std::filesystem::is_regular_file(candidate))
      return candidate.string();
  }
  return {};
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG AC1032 multi-line attributes of the ODA File Converter",
          "[dwg][attrib][attdef][ac1032]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/lc_multiline_attrib_ac1032.dwg";
  REQUIRE(std::filesystem::is_regular_file(path));

  AttributeIface drawing;
  dwgRW reader(path.c_str());
  REQUIRE(reader.read(&drawing, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  CHECK(reader.getVersion() == DRW::AC1032);

  const DRW_Attrib *attrib = drawing.attrib("ML_TAG");
  REQUIRE(attrib != nullptr);
  CHECK(attrib->attribFlags == 0);
  CHECK(attrib->m_fieldLength == 0);
  CHECK(attrib->lockPosition);
  CHECK(attrib->m_attributeType == 2);
  CHECK(AttributeIface::name(drawing.m_textStyles, attrib->styleH.ref) ==
        "Standard");
  REQUIRE(attrib->mtext != nullptr);
  CHECK(attrib->mtext->text == "value line 1\\Pvalue line 2");
  CHECK(attrib->mtext->basePoint.x == Catch::Approx(102.0));
  CHECK(attrib->mtext->basePoint.y == Catch::Approx(62.0));
  CHECK(attrib->mtext->height == Catch::Approx(2.5));
  CHECK(attrib->mtext->m_r2018IsNotAnnotative);
  CHECK(attrib->mtext->m_r2018Attachment == attrib->mtext->textgen);
  CHECK(attrib->mtext->m_r2018InsertionPoint.x == Catch::Approx(102.0));
  CHECK(attrib->mtext->m_r2018InsertionPoint.y == Catch::Approx(62.0));
  CHECK(attrib->mtext->m_r2018XAxisDir.x == Catch::Approx(1.0));
  CHECK(attrib->mtext->parentHandle == DRW::NoHandle);
  CHECK(attrib->mtext->m_r2018AppIdHandle == 0u);
  checkEmbeddedTableHandles(drawing, *attrib->mtext);

  const DRW_Attdef *attdef = drawing.attdef("ML_TAG");
  REQUIRE(attdef != nullptr);
  CHECK(attdef->prompt == "Multi line prompt");
  CHECK(attdef->attribFlags == 0);
  CHECK(attdef->m_fieldLength == 0);
  CHECK(attdef->lockPosition);
  CHECK(attdef->m_attributeType == 4);
  REQUIRE(attdef->mtext != nullptr);
  CHECK(attdef->mtext->text == "default line 1\\Pdefault line 2");
  CHECK(attdef->mtext->m_r2018IsNotAnnotative);
  CHECK(attdef->mtext->m_r2018AppIdHandle == 0u);
  checkEmbeddedTableHandles(drawing, *attdef->mtext);

  // The single-line ones next to them never had the problem.
  const DRW_Attrib *singleLine = drawing.attrib("SL_TAG");
  REQUIRE(singleLine != nullptr);
  CHECK(singleLine->text == "single value");
  CHECK(singleLine->attribFlags == 8);
  CHECK(singleLine->mtext == nullptr);
  const DRW_Attdef *singleLineDef = drawing.attdef("SL_TAG");
  REQUIRE(singleLineDef != nullptr);
  CHECK(singleLineDef->prompt == "Single line prompt");
  CHECK(singleLineDef->attribFlags == 8);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes AC1032 multi-line attributes it reads back",
          "[dwg-write][dwg][attrib][attdef][ac1032]") {
  const std::string path =
      (std::filesystem::temp_directory_path() /
       "lc_attrib_embedded_mtext_written_ac1032.dwg")
          .string();
  {
    AttributeIface source;
    dwgRW writer(path.c_str());
    source.m_writer = &writer;
    REQUIRE(writer.write(&source, DRW::AC1032, /*bin=*/false));
  }

  AttributeIface drawing;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&drawing, /*ext=*/true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getEntityParseFailures() == 0);
  }
  std::remove(path.c_str());

  const DRW_Attrib *attrib = drawing.attrib("ML_TAG");
  REQUIRE(attrib != nullptr);
  CHECK(attrib->attribFlags == 0);
  CHECK(attrib->m_fieldLength == 0);
  CHECK(attrib->lockPosition);
  CHECK(attrib->m_attributeType == 2);
  REQUIRE(attrib->mtext != nullptr);
  const DRW_MText &mtext = *attrib->mtext;
  CHECK(mtext.text == "value line 1\\Pvalue line 2");
  CHECK(mtext.m_r2018IsNotAnnotative);
  CHECK(mtext.m_r2018Version == 4);
  CHECK(mtext.m_r2018DefaultFlag);
  CHECK(mtext.m_r2018Attachment == 1);
  CHECK(mtext.m_r2018InsertionPoint.x == Catch::Approx(102.0));
  CHECK(mtext.m_r2018InsertionPoint.y == Catch::Approx(62.0));
  CHECK(mtext.m_r2018XAxisDir.x == Catch::Approx(1.0));
  CHECK(mtext.m_r2018RectWidth == Catch::Approx(30.0));
  CHECK(mtext.m_r2018AppIdHandle == 0u);
  // The handles of the other drawing are gone: the MTEXT is on the
  // attribute's layer and in its text style, with the linetype of the layer.
  CHECK(mtext.dwgLayerHandle() == attrib->dwgLayerHandle());
  CHECK(mtext.styleH.ref == attrib->styleH.ref);
  CHECK(mtext.dwgLinetypeHandle() == 0u);
  CHECK(mtext.lineType == "BYLAYER");
  checkEmbeddedTableHandles(drawing, mtext);

  const DRW_Attdef *attdef = drawing.attdef("ML_TAG");
  REQUIRE(attdef != nullptr);
  CHECK(attdef->prompt == "Multi line prompt");
  CHECK(attdef->attribFlags == 0);
  CHECK(attdef->m_fieldLength == 0);
  CHECK(attdef->lockPosition);
  CHECK(attdef->m_attributeType == 4);
  REQUIRE(attdef->mtext != nullptr);
  CHECK(attdef->mtext->text == "default line 1\\Pdefault line 2");
  CHECK_FALSE(attdef->mtext->m_r2018IsNotAnnotative);
  checkEmbeddedTableHandles(drawing, *attdef->mtext);

  const DRW_Attrib *singleLine = drawing.attrib("SL_TAG");
  REQUIRE(singleLine != nullptr);
  CHECK(singleLine->text == "single value");
  CHECK(singleLine->attribFlags == 8);
  const DRW_Attdef *singleLineDef = drawing.attdef("SL_TAG");
  REQUIRE(singleLineDef != nullptr);
  CHECK(singleLineDef->text == "single default");
  CHECK(singleLineDef->prompt == "Single line prompt");
  CHECK(singleLineDef->attribFlags == 8);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG AC1032 multi-line attribute of an AutoCAD drawing",
          "[dwg][attrib][attdef][ac1032]") {
  const std::string path = autoCadSamplePath();
  if (path.empty())
    SKIP("ACadSharp's sample_AC1032.dwg is not on this machine; skipping");

  AttributeIface drawing;
  dwgRW reader(path.c_str());
  REQUIRE(reader.read(&drawing, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // AutoCAD's DXF of the same drawing has 70 = 0, 280 = 1 and no 73 here.
  const DRW_Attrib *attrib = drawing.attrib("MULTI_LINE_ATT");
  REQUIRE(attrib != nullptr);
  CHECK(attrib->attribFlags == 0);
  CHECK(attrib->m_fieldLength == 0);
  CHECK(attrib->lockPosition);
  CHECK(AttributeIface::name(drawing.m_textStyles, attrib->styleH.ref) ==
        "Standard");
  REQUIRE(attrib->mtext != nullptr);
  CHECK(attrib->mtext->text == "my multi line text for the attrrib");
  CHECK(attrib->mtext->m_r2018IsNotAnnotative);
  CHECK(attrib->mtext->m_r2018AppIdHandle == 0u);
  checkEmbeddedTableHandles(drawing, *attrib->mtext);

  const DRW_Attdef *attdef = drawing.attdef("MULTI_LINE_ATT");
  REQUIRE(attdef != nullptr);
  CHECK(attdef->prompt == "this is an example of prompt");
  CHECK(attdef->attribFlags == 0);
  CHECK(attdef->m_fieldLength == 0);
  CHECK(attdef->lockPosition);
  REQUIRE(attdef->mtext != nullptr);
  checkEmbeddedTableHandles(drawing, *attdef->mtext);
}
