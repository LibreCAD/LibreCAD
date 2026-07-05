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
 * DXF OBJECTS-section read tests for newly-wired object types.
 *   - slice C3: GROUP (AcDbGroup) read dispatch + DRW_Group::parseCode.
 * The DXF parser previously dispatched only 6 OBJECTS types and silently
 * skipped the rest (incl. GROUP); RS_FilterDXFRW::addGroup already stores
 * the group into LC_DwgAdvancedMetadata.
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "drw_header.h"
#include "drw_objects.h"
#include "libdxfrw.h"

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

class GroupCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Group m_captured;
  void addGroup(const DRW_Group &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Dictionary m_captured;
  void addDictionary(const DRW_Dictionary &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class ScaleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Scale m_captured;
  void addScale(const DRW_Scale &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class MLineStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MLineStyle m_captured;
  void addMLineStyle(const DRW_MLineStyle &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryVarCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DictionaryVar m_captured;
  void addDictionaryVar(const DRW_DictionaryVar &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryWithDefaultCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DictionaryWithDefault m_captured;
  void addDictionaryWithDefault(const DRW_DictionaryWithDefault &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class RasterVariablesCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_RasterVariables m_captured;
  void addRasterVariables(const DRW_RasterVariables &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class SunCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Sun m_captured;
  void addSun(const DRW_Sun &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class LayoutCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Layout m_captured;
  void addLayout(const DRW_Layout &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class WipeoutVariablesCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_WipeoutVariables m_captured;
  void addWipeoutVariables(const DRW_WipeoutVariables &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class RawObjectCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfObject> m_objects;
  void addRawDxfObject(const DRW_RawDxfObject &d) override {
    m_objects.push_back(d);
  }
};

class RawEntityCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfObject> m_entities;
  void addRawDxfEntity(const DRW_RawDxfObject &d) override {
    m_entities.push_back(d);
  }
};

class RawObjectEmitter : public StubInterface {
public:
  DRW_RawDxfObject m_obj;
  dxfRW *m_rw = nullptr;
  void writeObjects() override { m_rw->writeRawDxfObject(&m_obj); }
};

// Emits nothing of its own; the codec emits the named-dict / group objects it
// was handed via setNamedDictObjects / setGroups. Used by the [objects] codec
// write units.
class NullObjectEmitter : public StubInterface {
public:
  void writeObjects() override {}
};

// Emits a set of points whose handle field is pre-seeded with a SOURCE handle
// (as RS_FilterDXFRW::getEntityAttributes does). Drives dxfRW::writePoint ->
// writeEntity so the codec's source->minted capture (F3) can be inspected.
class SeededPointEmitter : public StubInterface {
public:
  std::vector<std::uint32_t> m_sourceHandles;
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    for (std::uint32_t src : m_sourceHandles) {
      DRW_Point pt;
      pt.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      pt.handle = src;  // seed: source-handle key consumed by writeEntity
      m_rw->writePoint(&pt);
    }
  }
};

// Emits a POLYLINE (two vertices, seeded source 0xAA) followed by a POINT (seeded
// source 0xBB). The POLYLINE drives the VERTEX/SEQEND parent re-entries into
// dxfRW::writeEntity that must NOT pollute the source->minted map (A-3).
class SeededPolylineEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    DRW_Polyline pl;
    pl.handle = 0xAAu;  // seeded source-handle key
    auto v1 = std::make_shared<DRW_Vertex>();
    v1->basePoint = DRW_Coord(0.0, 0.0, 0.0);
    auto v2 = std::make_shared<DRW_Vertex>();
    v2->basePoint = DRW_Coord(1.0, 1.0, 0.0);
    pl.vertlist.push_back(v1);
    pl.vertlist.push_back(v2);
    m_rw->writePolyline(&pl);

    DRW_Point pt;
    pt.basePoint = DRW_Coord(2.0, 2.0, 0.0);
    pt.handle = 0xBBu;  // seeded source-handle key
    m_rw->writePoint(&pt);
  }
};

// Emits a POINT whose xAxisAngle is set in TRUE RADIANS (the DWG-path / field
// convention). The DXF writer must convert it to DEGREES for code 50 (C-2).
class XAxisPointEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    DRW_Point pt;
    pt.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    pt.xAxisAngle = 1.5707963267948966;  // pi/2 radians == 90 degrees
    m_rw->writePoint(&pt);
  }
};

class TableXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_TableEntry &entry, const char *payload, int intCode,
                       int intValue) {
    entry.extData.push_back(new DRW_Variant(1001, std::string{"TABLEAPP"}));
    entry.extData.push_back(new DRW_Variant(1000, std::string{payload}));
    entry.extData.push_back(
        new DRW_Variant(intCode, static_cast<std::int32_t>(intValue)));
  }

  void writeLTypes() override {
    DRW_LType lt;
    lt.name = "XDASH";
    lt.desc = "xdata ltype";
    addXData(lt, "ltype-xdata", 1071, 42);
    m_rw->writeLineType(&lt);
  }

  void writeTextstyles() override {
    DRW_Textstyle style;
    style.name = "XDATASTYLE";
    addXData(style, "style-xdata", 1070, 7);
    m_rw->writeTextstyle(&style);
  }

  void writeVports() override {
    DRW_Vport vp;
    vp.name = "XVPORT";
    addXData(vp, "vport-xdata", 1070, 8);
    m_rw->writeVport(&vp);
  }

  void writeViews() override {
    DRW_View view;
    view.name = "XVIEW";
    addXData(view, "view-xdata", 1070, 9);
    m_rw->writeView(&view);
  }

  void writeUCSs() override {
    DRW_UCS ucs;
    ucs.name = "XUCS";
    addXData(ucs, "ucs-xdata", 1070, 10);
    m_rw->writeUCS(&ucs);
  }

  void writeAppId() override {
    DRW_AppId tableApp;
    tableApp.name = "TABLEAPP";
    m_rw->writeAppId(&tableApp);

    DRW_AppId xapp;
    xapp.name = "XAPPID";
    addXData(xapp, "appid-xdata", 1070, 11);
    m_rw->writeAppId(&xapp);
  }

  void writeDimstyles() override {
    DRW_Dimstyle dim;
    dim.name = "XDIM";
    addXData(dim, "dimstyle-xdata", 1070, 12);
    m_rw->writeDimstyle(&dim);
  }
};

class EntityXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_Entity &entity, const char *payload, int intCode,
                       int intValue) {
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(1001, std::string{"ENTITYAPP"}));
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(1000, std::string{payload}));
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(intCode, static_cast<std::int32_t>(intValue)));
  }

  void writeEntities() override {
    DRW_Line line;
    line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
    addXData(line, "line-xdata", 1070, 17);
    m_rw->writeLine(&line);

    DRW_LWPolyline lw;
    auto v1 = lw.addVertex();
    v1->x = 0.0;
    v1->y = 0.0;
    auto v2 = lw.addVertex();
    v2->x = 2.0;
    v2->y = 0.0;
    addXData(lw, "lwpolyline-xdata", 1071, 18);
    m_rw->writeLWPolyline(&lw);

    DRW_Text text;
    text.basePoint = DRW_Coord(3.0, 4.0, 0.0);
    text.height = 0.5;
    text.text = "XDATA text";
    addXData(text, "text-xdata", 1070, 19);
    m_rw->writeText(&text);

    DRW_Point point;
    point.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    addXData(point, "point-xdata", 1070, 20);
    m_rw->writePoint(&point);

    DRW_Ray ray;
    ray.basePoint = DRW_Coord(0.0, 1.0, 0.0);
    ray.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    addXData(ray, "ray-xdata", 1070, 21);
    m_rw->writeRay(&ray);

    DRW_Xline xline;
    xline.basePoint = DRW_Coord(0.0, 2.0, 0.0);
    xline.secPoint = DRW_Coord(0.0, 1.0, 0.0);
    addXData(xline, "xline-xdata", 1070, 22);
    m_rw->writeXline(&xline);

    DRW_Circle circle;
    circle.basePoint = DRW_Coord(4.0, 4.0, 0.0);
    circle.radious = 2.0;
    addXData(circle, "circle-xdata", 1070, 23);
    m_rw->writeCircle(&circle);

    DRW_Arc arc;
    arc.basePoint = DRW_Coord(5.0, 5.0, 0.0);
    arc.radious = 1.5;
    arc.endangle = 1.0;
    addXData(arc, "arc-xdata", 1070, 24);
    m_rw->writeArc(&arc);

    DRW_Ellipse ellipse;
    ellipse.basePoint = DRW_Coord(6.0, 6.0, 0.0);
    ellipse.secPoint = DRW_Coord(2.0, 0.0, 0.0);
    ellipse.ratio = 0.5;
    ellipse.endparam = 6.283185307179586;
    addXData(ellipse, "ellipse-xdata", 1070, 25);
    m_rw->writeEllipse(&ellipse);

    DRW_Trace trace;
    trace.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    trace.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    trace.thirdPoint = DRW_Coord(1.0, 1.0, 0.0);
    trace.fourPoint = DRW_Coord(0.0, 1.0, 0.0);
    addXData(trace, "trace-xdata", 1070, 26);
    m_rw->writeTrace(&trace);

    DRW_Solid solid;
    solid.basePoint = DRW_Coord(2.0, 0.0, 0.0);
    solid.secPoint = DRW_Coord(3.0, 0.0, 0.0);
    solid.thirdPoint = DRW_Coord(3.0, 1.0, 0.0);
    solid.fourPoint = DRW_Coord(2.0, 1.0, 0.0);
    addXData(solid, "solid-xdata", 1070, 27);
    m_rw->writeSolid(&solid);

    DRW_3Dface face;
    face.basePoint = DRW_Coord(4.0, 0.0, 0.0);
    face.secPoint = DRW_Coord(5.0, 0.0, 0.0);
    face.thirdPoint = DRW_Coord(5.0, 1.0, 0.0);
    face.fourPoint = DRW_Coord(4.0, 1.0, 0.0);
    addXData(face, "3dface-xdata", 1070, 28);
    m_rw->write3dface(&face);

    DRW_Polyline polyline;
    DRW_Vertex pv1;
    pv1.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    polyline.addVertex(pv1);
    DRW_Vertex pv2;
    pv2.basePoint = DRW_Coord(1.0, 0.0, 0.0);
    polyline.addVertex(pv2);
    addXData(polyline, "polyline-xdata", 1070, 29);
    m_rw->writePolyline(&polyline);

    DRW_Spline spline;
    spline.normalVec = DRW_Coord(0.0, 0.0, 1.0);
    spline.degree = 1;
    spline.knotslist = {0.0, 0.0, 1.0, 1.0};
    spline.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    spline.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    addXData(spline, "spline-xdata", 1070, 30);
    m_rw->writeSpline(&spline);

    DRW_Helix helix;
    helix.normalVec = DRW_Coord(0.0, 0.0, 1.0);
    helix.degree = 1;
    helix.knotslist = {0.0, 0.0, 1.0, 1.0};
    helix.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    helix.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 0.5, 0.0));
    helix.radius = 0.5;
    helix.turns = 1.0;
    helix.turnHeight = 1.0;
    addXData(helix, "helix-xdata", 1070, 31);
    m_rw->writeHelix(&helix);

    DRW_Hatch hatch;
    hatch.name = "SOLID";
    addXData(hatch, "hatch-xdata", 1070, 32);
    m_rw->writeHatch(&hatch);

    DRW_MPolygon mpolygon;
    mpolygon.name = "SOLID";
    addXData(mpolygon, "mpolygon-xdata", 1070, 33);
    m_rw->writeMPolygon(&mpolygon);

    DRW_Leader leader;
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    addXData(leader, "leader-xdata", 1070, 34);
    m_rw->writeLeader(&leader);

    DRW_Insert insert;
    insert.name = "XDATA_BLOCK";
    insert.basePoint = DRW_Coord(1.0, 1.0, 0.0);
    addXData(insert, "insert-xdata", 1070, 35);
    m_rw->writeInsert(&insert);

    DRW_Table table;
    table.name = "XDATA_TABLE";
    table.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    addXData(table, "table-xdata", 1070, 36);
    m_rw->writeTable(&table);

    DRW_Attrib attrib;
    attrib.basePoint = DRW_Coord(2.0, 2.0, 0.0);
    attrib.height = 0.25;
    attrib.text = "value";
    attrib.tag = "TAG";
    addXData(attrib, "attrib-xdata", 1070, 37);
    m_rw->writeAttrib(&attrib);

    DRW_Attdef attdef;
    attdef.basePoint = DRW_Coord(2.0, 3.0, 0.0);
    attdef.height = 0.25;
    attdef.text = "default";
    attdef.tag = "TAGDEF";
    attdef.prompt = "Prompt";
    addXData(attdef, "attdef-xdata", 1070, 38);
    m_rw->writeAttdef(&attdef);

    DRW_RText rtext;
    rtext.basePoint = DRW_Coord(3.0, 3.0, 0.0);
    rtext.height = 0.5;
    rtext.text = "RText";
    addXData(rtext, "rtext-xdata", 1070, 39);
    m_rw->writeRText(&rtext);

    DRW_ArcAlignedText arcText;
    arcText.text = "Arc text";
    arcText.m_center = DRW_Coord(4.0, 4.0, 0.0);
    arcText.m_radius = 2.0;
    arcText.m_endAngle = 1.0;
    arcText.height = 0.25;
    addXData(arcText, "arctext-xdata", 1070, 40);
    m_rw->writeArcAlignedText(&arcText);

    DRW_Tolerance tolerance;
    tolerance.insertionPoint = DRW_Coord(5.0, 5.0, 0.0);
    tolerance.text = "tol";
    tolerance.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);
    addXData(tolerance, "tolerance-xdata", 1070, 41);
    m_rw->writeTolerance(&tolerance);

    DRW_DimLinear dim;
    dim.setDimPoint(DRW_Coord(0.0, 0.0, 0.0));
    dim.setTextPoint(DRW_Coord(0.5, 0.5, 0.0));
    dim.setDef1Point(DRW_Coord(0.0, 0.0, 0.0));
    dim.setDef2Point(DRW_Coord(1.0, 0.0, 0.0));
    addXData(dim, "dimension-xdata", 1070, 42);
    m_rw->writeDimension(&dim);

    DRW_DimArc arcDim;
    arcDim.setArcDefPoint(DRW_Coord(0.0, 0.0, 0.0));
    arcDim.setTextPoint(DRW_Coord(0.5, 0.5, 0.0));
    arcDim.setExtLine1(DRW_Coord(1.0, 0.0, 0.0));
    arcDim.setExtLine2(DRW_Coord(0.0, 1.0, 0.0));
    arcDim.setArcCenter(DRW_Coord(0.0, 0.0, 0.0));
    addXData(arcDim, "arcdim-xdata", 1070, 43);
    m_rw->writeDimension(&arcDim);

    DRW_DimLargeRadial largeRadial;
    largeRadial.setCenterPoint(DRW_Coord(0.0, 0.0, 0.0));
    largeRadial.setTextPoint(DRW_Coord(1.0, 1.0, 0.0));
    largeRadial.setChordPoint(DRW_Coord(2.0, 0.0, 0.0));
    largeRadial.overrideCenterPoint = DRW_Coord(0.1, 0.2, 0.0);
    largeRadial.jogPoint = DRW_Coord(1.0, 0.5, 0.0);
    largeRadial.jogAngle = 0.25;
    addXData(largeRadial, "largeradial-xdata", 1070, 44);
    m_rw->writeDimension(&largeRadial);

    DRW_MLeader mleader;
    addXData(mleader, "mleader-xdata", 1070, 45);
    m_rw->writeMultiLeader(&mleader);

    DRW_Light light;
    light.m_name = "XDATA_LIGHT";
    addXData(light, "light-xdata", 1070, 46);
    m_rw->writeLight(&light);

    DRW_Mesh mesh;
    addXData(mesh, "mesh-xdata", 1070, 47);
    m_rw->writeMesh(&mesh);

    DRW_Shape shape;
    shape.m_insertionPoint = DRW_Coord(6.0, 6.0, 0.0);
    shape.m_styleName = "STANDARD";
    addXData(shape, "shape-xdata", 1070, 48);
    m_rw->writeShape(&shape);

    DRW_Ole2Frame ole;
    addXData(ole, "ole2frame-xdata", 1070, 49);
    m_rw->writeOle2Frame(&ole);

    DRW_Viewport viewport;
    viewport.basePoint = DRW_Coord(7.0, 7.0, 0.0);
    viewport.pswidth = 2.0;
    viewport.psheight = 1.0;
    addXData(viewport, "viewport-xdata", 1070, 50);
    m_rw->writeViewport(&viewport);

    DRW_Image image;
    image.basePoint = DRW_Coord(8.0, 8.0, 0.0);
    image.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    image.vVector = DRW_Coord(0.0, 1.0, 0.0);
    image.sizeu = 10.0;
    image.sizev = 10.0;
    addXData(image, "image-xdata", 1070, 51);
    m_rw->writeImage(&image, "xdata.png");

    DRW_Wipeout wipeout;
    wipeout.basePoint = DRW_Coord(9.0, 9.0, 0.0);
    wipeout.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    wipeout.vVector = DRW_Coord(0.0, 1.0, 0.0);
    wipeout.sizeu = 2.0;
    wipeout.sizev = 2.0;
    addXData(wipeout, "wipeout-xdata", 1070, 52);
    m_rw->writeWipeout(&wipeout);

    DRW_PointCloud pointCloud;
    pointCloud.savedFilename = "cloud.rcp";
    addXData(pointCloud, "pointcloud-xdata", 1070, 53);
    m_rw->writePointCloud(&pointCloud);

    DRW_PlaneSurface surface;
    surface.modelerFormatVersion = 1;
    addXData(surface, "surface-xdata", 1070, 54);
    m_rw->writeSurface(&surface);
  }
};

class ObjectXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_TableEntry &entry, const char *payload, int value) {
    entry.extData.push_back(new DRW_Variant(1001, std::string{"OBJECTAPP"}));
    entry.extData.push_back(new DRW_Variant(1000, std::string{payload}));
    entry.extData.push_back(new DRW_Variant(1070, static_cast<std::int32_t>(value)));
  }

  void writeObjects() override {
    DRW_Scale scale;
    scale.handle = 0x510u;
    scale.parentHandle = 0xCu;
    scale.name = "XOBJ_SCALE";
    scale.paperUnits = 1.0;
    scale.drawingUnits = 48.0;
    addXData(scale, "scale-xdata", 21);
    m_rw->writeScale(&scale);

    DRW_MLineStyle mlineStyle;
    mlineStyle.handle = 0x511u;
    mlineStyle.parentHandle = 0xCu;
    mlineStyle.name = "XOBJ_MLINESTYLE";
    mlineStyle.description = "object xdata";
    addXData(mlineStyle, "mlinestyle-xdata", 22);
    m_rw->writeMLineStyle(&mlineStyle);

    DRW_Field field;
    field.handle = 0x512u;
    field.parentHandle = 0xCu;
    field.m_evaluatorId = "AcExpr";
    field.m_fieldCode = "1+1";
    addXData(field, "field-xdata", 23);
    m_rw->writeField(&field);

    DRW_WipeoutVariables vars;
    vars.handle = 0x513u;
    vars.parentHandle = 0xCu;
    vars.m_displayFrame = 1;
    addXData(vars, "wipeoutvars-xdata", 24);
    m_rw->writeWipeoutVariables(&vars);
  }
};

// Read a written DXF file back into a string for structural assertions.
std::string slurp(const std::filesystem::path &path) {
  std::ifstream in(path);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// Format a handle the way the codec emits code-5/340/350: uppercase hex, no
// leading zeros (matches dxfRW::toHexStr's "%X").
std::string toHexUpper(std::uint32_t h) {
  char buf[9] = {'\0'};
  std::snprintf(buf, sizeof(buf), "%X", h);
  return std::string(buf);
}

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

} // namespace

TEST_CASE("DXF GROUP object is read into a DRW_Group (slice C3)", "[dxf][group]") {
  GroupCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGROUP\n5\n2F\n330\nC\n100\nAcDbGroup\n"
      "300\nFasteners\n70\n0\n71\n1\n"
      "340\n30\n340\n31\n340\n32\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_group_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_description == "Fasteners");
  CHECK(cap.m_captured.m_isUnnamed == false);
  CHECK(cap.m_captured.m_selectable == true);
  REQUIRE(cap.m_captured.m_entityHandles.size() == 3);
  CHECK(cap.m_captured.m_entityHandles[0] == 0x30u);
  CHECK(cap.m_captured.m_entityHandles[2] == 0x32u);
}

TEST_CASE("DXF unnamed GROUP sets the unnamed flag (slice C3)", "[dxf][group]") {
  GroupCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGROUP\n5\n3A\n330\nC\n100\nAcDbGroup\n"
      "300\n\n70\n1\n71\n0\n340\n40\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_group_unnamed.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_isUnnamed == true);
  CHECK(cap.m_captured.m_selectable == false);
  REQUIRE(cap.m_captured.m_entityHandles.size() == 1);
}

TEST_CASE("DXF DICTIONARY entries are read (name->handle) (slice C1)", "[dxf][dictionary]") {
  DictionaryCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n"
      "3\nACAD_LAYOUT\n350\n1A\n"
      "3\nACAD_MLINESTYLE\n350\n17\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictionary_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.cloning == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 3);
  CHECK(cap.m_captured.m_entries[0].m_name == "ACAD_GROUP");
  CHECK(cap.m_captured.m_entries[0].m_handle == 0xDu);
  CHECK(cap.m_captured.m_entries[1].m_name == "ACAD_LAYOUT");
  CHECK(cap.m_captured.m_entries[1].m_handle == 0x1Au);
  CHECK(cap.m_captured.m_entries[2].m_name == "ACAD_MLINESTYLE");
  CHECK(cap.m_captured.m_entries[2].m_handle == 0x17u);
}

TEST_CASE("DXF SCALE object is read (label + numerator/denominator) (slice C6)", "[dxf][scale]") {
  ScaleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSCALE\n5\nB1\n330\nB0\n100\nAcDbScale\n"
      "70\n0\n300\n1:2\n140\n1.0\n141\n2.0\n290\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_scale_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "1:2");
  CHECK(cap.m_captured.paperUnits == 1.0);
  CHECK(cap.m_captured.drawingUnits == 2.0);
  CHECK(cap.m_captured.isUnitScale == false);
  CHECK(cap.m_captured.scaleFactor() == 2.0);
}

TEST_CASE("DXF MLINESTYLE object is read with elements (slice C5)", "[dxf][mlinestyle]") {
  MLineStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMLINESTYLE\n5\n18\n330\n17\n100\nAcDbMlineStyle\n"
      "2\nSTANDARD\n70\n0\n3\nstd desc\n62\n256\n51\n90.0\n52\n90.0\n"
      "71\n2\n"
      "49\n0.5\n62\n1\n6\nBYLAYER\n"
      "49\n-0.5\n62\n2\n6\nCONTINUOUS\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_mlinestyle_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "STANDARD");
  CHECK(cap.m_captured.description == "std desc");
  CHECK(cap.m_captured.fillColor == 256);  // the pre-element 62
  CHECK(cap.m_captured.startAngle == 90.0);
  REQUIRE(cap.m_captured.elements.size() == 2);
  CHECK(cap.m_captured.elements[0].offset == 0.5);
  CHECK(cap.m_captured.elements[0].color == 1);
  CHECK(cap.m_captured.elements[0].linetype == "BYLAYER");
  CHECK(cap.m_captured.elements[1].offset == -0.5);
  CHECK(cap.m_captured.elements[1].color == 2);
  CHECK(cap.m_captured.elements[1].linetype == "CONTINUOUS");
}

TEST_CASE("DXF DICTIONARYVAR object is read (schema + value)", "[dxf][dictionaryvar]") {
  DictionaryVarCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARYVAR\n5\n2A\n330\n29\n100\nDictionaryVariables\n"
      "280\n0\n1\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictvar_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_schema == 0);
  CHECK(cap.m_captured.m_value == "2");
}

TEST_CASE("DXF ACDBDICTIONARYWDFLT reads entries + default handle", "[dxf][dictionary]") {
  DictionaryWithDefaultCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBDICTIONARYWDFLT\n5\n2B\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nNormal\n350\n2C\n"
      "100\nAcDbDictionaryWithDefault\n340\n2C\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictwdflt_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 1);
  CHECK(cap.m_captured.m_entries[0].m_name == "Normal");
  CHECK(cap.m_captured.m_defaultEntryHandle == 0x2Cu);
}

TEST_CASE("DXF RASTERVARIABLES object is read (frame/quality/units)", "[dxf][rastervariables]") {
  RasterVariablesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nRASTERVARIABLES\n5\n2D\n330\n29\n100\nAcDbRasterVariables\n"
      "90\n0\n70\n1\n71\n1\n72\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rastervars_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_imageFrame == 1);
  CHECK(cap.m_captured.m_imageQuality == 1);
  CHECK(cap.m_captured.m_units == 0);
}

TEST_CASE("DXF SUN object is read (status/intensity/shadows/date)", "[dxf][sun]") {
  SunCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUN\n5\n2E\n330\n29\n100\nAcDbSun\n"
      "90\n1\n290\n1\n63\n7\n421\n16711680\n40\n1.0\n291\n1\n"
      "91\n2455563\n92\n43200000\n292\n0\n70\n1\n71\n256\n280\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sun_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_isOn == true);
  CHECK(cap.m_captured.m_color == 7u);
  CHECK(cap.m_captured.m_color24 == 16711680);  // code 421 true-color preserved (G-1)
  CHECK(cap.m_captured.m_intensity == 1.0);
  CHECK(cap.m_captured.m_hasShadow == true);
  CHECK(cap.m_captured.m_julianDay == 2455563);
  CHECK(cap.m_captured.m_milliseconds == 43200000);
  CHECK(cap.m_captured.m_shadowMapSize == 256);
}

TEST_CASE("DXF LAYOUT object disambiguates plot vs layout subclasses (slice C2)", "[dxf][layout]") {
  LayoutCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nLAYOUT\n5\n4F\n330\n1A\n"
      "100\nAcDbPlotSettings\n"
      "1\nMy Page Setup\n2\nDWG To PDF\n4\nANSI_A\n"
      "40\n5.8\n41\n5.8\n42\n5.8\n43\n5.8\n44\n215.9\n45\n279.4\n"
      "70\n688\n72\n0\n73\n1\n74\n5\n75\n16\n"
      "100\nAcDbLayout\n"
      "1\nLayout1\n70\n1\n71\n2\n"
      "10\n0.0\n20\n0.0\n11\n12.0\n21\n9.0\n"
      "12\n0.0\n22\n0.0\n32\n0.0\n"
      "76\n0\n146\n0.0\n330\n50\n331\n51\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_layout_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  // AcDbPlotSettings prefix
  CHECK(cap.m_captured.pageSetupName == "My Page Setup");
  CHECK(cap.m_captured.printerConfig == "DWG To PDF");
  CHECK(cap.m_captured.paperSize == "ANSI_A");
  CHECK(cap.m_captured.paperWidth == 215.9);
  CHECK(cap.m_captured.plotLayoutFlags == 688);
  // AcDbLayout body — code 1/70/76/330 must NOT be confused with the prefix
  CHECK(cap.m_captured.name == "Layout1");
  CHECK(cap.m_captured.layoutFlags == 1);
  CHECK(cap.m_captured.tabOrder == 2);
  CHECK(cap.m_captured.limMaxX == 12.0);
  CHECK(cap.m_captured.limMaxY == 9.0);
  CHECK(cap.m_captured.orthoViewType == 0);
  CHECK(cap.m_captured.paperSpaceBlockRecordHandle.ref == 0x50u);
  CHECK(cap.m_captured.lastActiveViewportHandle.ref == 0x51u);
}

TEST_CASE("DXF WIPEOUTVARIABLES object is read (display-frame flag)", "[dxf][wipeoutvars]") {
  WipeoutVariablesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nWIPEOUTVARIABLES\n5\n30\n330\n29\n100\nAcDbWipeoutVariables\n70\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_wipeoutvars_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_displayFrame == 1);
}

TEST_CASE("DXF unmodeled OBJECT is captured verbatim, not dropped (slice A1)", "[dxf][rawobject]") {
  RawObjectCapture cap;
  // MATERIAL is a real object libdxfrw does not (yet) type for DXF; plus a
  // genuinely unknown object name. Both must be preserved, none dropped.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\n3B\n330\n29\n100\nAcDbMaterial\n"
      "1\nMyMaterial\n94\n63\n"
      "0\nACDBWEIRDOBJECT\n5\n3C\n330\n29\n70\n5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawobject.dxf");

  REQUIRE(cap.m_objects.size() == 2);

  const DRW_RawDxfObject &mat = cap.m_objects[0];
  CHECK(mat.name == "MATERIAL");
  CHECK(mat.handle == 0x3Bu);
  CHECK(mat.parentHandle == 0x29u);
  // groups captured verbatim: 5, 330, 100, 1, 94
  REQUIRE(mat.groups.size() == 5);
  CHECK(mat.groups[0].code() == 5);
  CHECK(mat.groups[2].code() == 100);
  CHECK(mat.groups[3].code() == 1);

  const DRW_RawDxfObject &weird = cap.m_objects[1];
  CHECK(weird.name == "ACDBWEIRDOBJECT");
  CHECK(weird.handle == 0x3Cu);
  REQUIRE(weird.groups.size() == 3);  // 5, 330, 70
}

TEST_CASE("DXF BREAKDATA/BREAKPOINTREF are raw-captured, not dropped (write-review 7.2)",
          "[dxf][rawobject][breakdata]") {
  // processBreakData/processBreakPointRef previously called only their typed
  // add* callbacks (no DXF writer for either), so both were silently dropped on
  // DXF->DXF. They now also raw-capture (mirroring processScale), and the
  // raw-net replay loop re-emits them.
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nBREAKDATA\n5\nA1\n330\nA0\n100\nAcDbBreakData\n90\n2\n331\nB5\n"
      "0\nBREAKPOINTREF\n5\nA2\n330\nA1\n100\nAcDbBreakPointRef\n90\n0\n10\n1.0\n20\n2.0\n30\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_breakdata.dxf");

  REQUIRE(cap.m_objects.size() == 2);
  const DRW_RawDxfObject &bd = cap.m_objects[0];
  CHECK(bd.name == "BREAKDATA");
  CHECK(bd.handle == 0xA1u);
  REQUIRE(bd.groups.size() == 5);  // 5, 330, 100, 90, 331
  CHECK(bd.groups[0].code() == 5);
  CHECK(bd.groups[2].code() == 100);
  const DRW_RawDxfObject &bp = cap.m_objects[1];
  CHECK(bp.name == "BREAKPOINTREF");
  CHECK(bp.handle == 0xA2u);
  CHECK(bp.groups.size() >= 5);
}

TEST_CASE("DXF unmodeled ENTITY is captured verbatim, not dropped (slice A4)", "[dxf][rawentity]") {
  RawEntityCapture cap;
  // An unknown entity and a standalone ATTDEF (no typed DXF dispatch) — both
  // must be preserved rather than silently skipped.
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n8\n0\n5\n4A\n62\n3\n10\n1.0\n20\n2.0\n"
      "0\nATTDEF\n8\n0\n5\n4B\n10\n0.0\n20\n0.0\n40\n0.5\n1\ndef\n2\nTAG\n3\nPrompt?\n70\n0\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n5.0\n21\n5.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawentity.dxf");

  // WEIRDENT + ATTDEF captured; LINE is typed (not raw).
  REQUIRE(cap.m_entities.size() == 2);
  CHECK(cap.m_entities[0].name == "WEIRDENT");
  CHECK(cap.m_entities[0].handle == 0x4Au);
  CHECK(cap.m_entities[1].name == "ATTDEF");
  CHECK(cap.m_entities[1].handle == 0x4Bu);
  // ATTDEF groups preserved verbatim (8,5,10,20,40,1,2,3,70)
  CHECK(cap.m_entities[1].groups.size() == 9);
}

TEST_CASE("DXF raw object round-trips through write+read (slice A2)",
          "[dxf][rawobject][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_rawobject_rt.dxf";
  std::filesystem::remove(path);

  RawObjectEmitter em;
  em.m_obj.name = "ACDBWEIRDOBJECT";
  em.m_obj.groups.emplace_back(5, std::string("3C"));
  em.m_obj.groups.emplace_back(330, std::string("29"));
  em.m_obj.groups.emplace_back(100, std::string("AcDbWeird"));
  em.m_obj.groups.emplace_back(1, std::string("payload text"));
  em.m_obj.groups.emplace_back(70, std::string("5"));
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  RawObjectCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_objects.size() == 1);
  CHECK(cap.m_objects[0].name == "ACDBWEIRDOBJECT");
  REQUIRE(cap.m_objects[0].groups.size() == 5);
  CHECK(cap.m_objects[0].groups[3].code() == 1);
  CHECK(std::string(cap.m_objects[0].groups[3].c_str()) == "payload text");

  std::filesystem::remove(path);
}

// Regression for the A1/A4 capture bug: the reader leaves strData stale for
// numeric codes (readInt16/32/64/Double/Bool parse into a local string), so the
// old all-getString() capture stored the PREVIOUS string value for every
// numeric group and mistyped it STRING. captureRawGroup must instead store a
// correctly-typed DRW_Variant per reader->type. Asserts VALUES, not just codes.
TEST_CASE("DXF raw-net captures numeric group VALUES, not stale strings "
          "(capture-bug fix)",
          "[dxf][rawobject][rawcapture]") {
  RawObjectCapture cap;
  // A string group (code 1) precedes every numeric group; under the old bug all
  // numerics would re-capture "STRINGVAL". One group of each reader type:
  // 1=string, 70=int16, 90=int32, 160=int64, 40=double, 290=bool.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWFDIAG\n5\n3B\n330\n29\n100\nAcDbWfDiag\n"
      "1\nSTRINGVAL\n70\n7\n90\n123456\n160\n40\n40\n2.5\n290\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawcapture.dxf");

  REQUIRE(cap.m_objects.size() == 1);
  const DRW_RawDxfObject &o = cap.m_objects[0];
  CHECK(o.handle == 0x3Bu);
  CHECK(o.parentHandle == 0x29u);
  // Index map: 0=5(str) 1=330(str) 2=100(str) 3=1(str) 4=70 5=90 6=160 7=40 8=290
  REQUIRE(o.groups.size() == 9);
  CHECK(o.groups[3].type() == DRW_Variant::STRING);
  CHECK(std::string(o.groups[3].c_str()) == "STRINGVAL");

  CHECK(o.groups[4].code() == 70);
  CHECK(o.groups[4].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[4].i_val() == 7);

  CHECK(o.groups[5].code() == 90);
  CHECK(o.groups[5].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[5].i_val() == 123456);

  CHECK(o.groups[6].code() == 160);
  CHECK(o.groups[6].type() == DRW_Variant::INTEGER64);
  CHECK(o.groups[6].i64_val() == 40);

  CHECK(o.groups[7].code() == 40);
  CHECK(o.groups[7].type() == DRW_Variant::DOUBLE);
  CHECK(o.groups[7].d_val() == 2.5);

  CHECK(o.groups[8].code() == 290);
  CHECK(o.groups[8].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[8].i_val() == 1);

  // End-to-end: re-emit the captured object and read it back; numeric values
  // must survive (writeRawDxfObject keys off variant type()).
  const auto rtPath =
      std::filesystem::temp_directory_path() / "lc_rawcapture_rt.dxf";
  std::filesystem::remove(rtPath);
  RawObjectEmitter em;
  em.m_obj = o;
  {
    dxfRW w(rtPath.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  RawObjectCapture rt;
  {
    dxfRW r(rtPath.string().c_str());
    REQUIRE(r.read(&rt, /*ext=*/true));
  }
  REQUIRE(rt.m_objects.size() == 1);
  const DRW_RawDxfObject &b = rt.m_objects[0];
  REQUIRE(b.groups.size() == 9);
  CHECK(b.groups[4].i_val() == 7);
  CHECK(b.groups[5].i_val() == 123456);
  CHECK(b.groups[6].i64_val() == 40);
  CHECK(b.groups[7].d_val() == 2.5);
  CHECK(b.groups[8].i_val() == 1);
  std::filesystem::remove(rtPath);
}

namespace {
// Parse a DXF file's lines into trimmed (code-line, value-line) text rows so a
// codec-write test can assert ordered group sequences without a full reader.
std::vector<std::pair<std::string, std::string>> readGroups(
    const std::filesystem::path &path) {
  std::vector<std::pair<std::string, std::string>> out;
  std::ifstream in(path);
  std::string codeLine;
  std::string valLine;
  auto trim = [](std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
      s.pop_back();
    std::size_t b = s.find_first_not_of(" \t");
    return b == std::string::npos ? std::string() : s.substr(b);
  };
  while (std::getline(in, codeLine) && std::getline(in, valLine))
    out.emplace_back(trim(codeLine), trim(valLine));
  return out;
}

// True if the group sequence `groups` contains, starting at any position, the
// ordered subsequence `seq` (each element a (code,value) pair), allowing other
// groups in between is NOT permitted — strictly consecutive.
bool hasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  if (seq.empty() || groups.size() < seq.size())
    return false;
  for (std::size_t i = 0; i + seq.size() <= groups.size(); ++i) {
    bool ok = true;
    for (std::size_t j = 0; j < seq.size(); ++j)
      if (groups[i + j] != seq[j]) { ok = false; break; }
    if (ok)
      return true;
  }
  return false;
}

bool recordHasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::string &recordType, const std::string &recordName,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  std::vector<std::pair<std::string, std::string>> record;
  auto matches = [&]() {
    if (record.empty())
      return false;
    bool hasName = false;
    for (const auto &kv : record) {
      if (kv.first == "2" && kv.second == recordName) {
        hasName = true;
        break;
      }
    }
    return hasName && hasConsecutive(record, seq);
  };

  bool inRecord = false;
  for (const auto &kv : groups) {
    if (kv.first == "0") {
      if (inRecord && matches())
        return true;
      inRecord = (kv.second == recordType);
      record.clear();
    }
    if (inRecord)
      record.push_back(kv);
  }
  return inRecord && matches();
}

bool recordTypeHasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::string &recordType,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  std::vector<std::pair<std::string, std::string>> record;
  bool inRecord = false;
  for (const auto &kv : groups) {
    if (kv.first == "0") {
      if (inRecord && hasConsecutive(record, seq))
        return true;
      inRecord = (kv.second == recordType);
      record.clear();
    }
    if (inRecord)
      record.push_back(kv);
  }
  return inRecord && hasConsecutive(record, seq);
}
} // namespace

TEST_CASE("DXF LAYOUT object writes plot prefix and layout body",
          "[dxf][layout][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_layout_write.dxf";
  std::filesystem::remove(path);

  class LayoutEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Layout m_layout;

    void writeObjects() override { m_rw->writeLayout(&m_layout); }
  };

  LayoutEmitter em;
  em.m_layout.handle = 0x4Fu;
  em.m_layout.parentHandle = 0x1Au;
  em.m_layout.pageSetupName = "My Page Setup";
  em.m_layout.printerConfig = "DWG To PDF.pc3";
  em.m_layout.paperSize = "ANSI_A";
  em.m_layout.plotLayoutFlags = 688;
  em.m_layout.marginLeft = 5.8;
  em.m_layout.marginBottom = 6.1;
  em.m_layout.marginRight = 5.9;
  em.m_layout.marginTop = 6.2;
  em.m_layout.paperWidth = 215.9;
  em.m_layout.paperHeight = 279.4;
  em.m_layout.plotOriginX = 1.0;
  em.m_layout.plotOriginY = 2.0;
  em.m_layout.paperUnits = 0;
  em.m_layout.plotRotation = 1;
  em.m_layout.plotType = 5;
  em.m_layout.windowMinX = 0.5;
  em.m_layout.windowMinY = 0.25;
  em.m_layout.windowMaxX = 12.0;
  em.m_layout.windowMaxY = 9.0;
  em.m_layout.currentStyleSheet = "monochrome.ctb";
  em.m_layout.scaleType = 16;
  em.m_layout.scaleFactor = 1.0;
  em.m_layout.paperImageOriginX = 0.0;
  em.m_layout.paperImageOriginY = 0.0;
  em.m_layout.shadePlotMode = 1;
  em.m_layout.shadePlotResLevel = 2;
  em.m_layout.shadePlotCustomDPI = 300;
  em.m_layout.name = "Layout1";
  em.m_layout.layoutFlags = 1;
  em.m_layout.tabOrder = 2;
  em.m_layout.limMinX = 0.0;
  em.m_layout.limMinY = 0.0;
  em.m_layout.limMaxX = 12.0;
  em.m_layout.limMaxY = 9.0;
  em.m_layout.insPoint = DRW_Coord(0.0, 0.0, 0.0);
  em.m_layout.extMin = DRW_Coord(-1.0, -2.0, 0.0);
  em.m_layout.extMax = DRW_Coord(13.0, 10.0, 0.0);
  em.m_layout.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
  em.m_layout.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
  em.m_layout.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
  em.m_layout.elevation = 0.0;
  em.m_layout.orthoViewType = 0;
  em.m_layout.paperSpaceBlockRecordHandle.ref = 0x50u;
  em.m_layout.lastActiveViewportHandle.ref = 0x51u;
  em.m_layout.namedUcsHandle.ref = 0x52u;
  em.m_layout.baseUcsHandle.ref = 0x53u;

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Dictionary layoutDict;
    layoutDict.handle = 0x1Au;
    layoutDict.parentHandle = 0;
    layoutDict.cloning = 1;
    layoutDict.m_entries.push_back({"Layout1", 0x4Fu});
    w.setNamedDictObjects({layoutDict});
    w.setRootDictEntries({{"ACAD_LAYOUT", "1A"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups, {{"3", "ACAD_LAYOUT"}, {"350", "1A"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "1A"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "1"},
                        {"3", "Layout1"}, {"350", "4F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "LAYOUT"}, {"5", "4F"}, {"330", "1A"},
                        {"100", "AcDbPlotSettings"}}));
  CHECK(hasConsecutive(groups,
                       {{"100", "AcDbLayout"}, {"1", "Layout1"},
                        {"70", "1"}, {"71", "2"}}));

  LayoutCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x4Fu);
  CHECK(cap.m_captured.parentHandle == 0x1A);
  CHECK(cap.m_captured.pageSetupName == "My Page Setup");
  CHECK(cap.m_captured.plotLayoutFlags == 688);
  CHECK(cap.m_captured.shadePlotMode == 1);
  CHECK(cap.m_captured.name == "Layout1");
  CHECK(cap.m_captured.layoutFlags == 1);
  CHECK(cap.m_captured.tabOrder == 2);
  CHECK(cap.m_captured.limMaxX == 12.0);
  CHECK(cap.m_captured.extMin.x == -1.0);
  CHECK(cap.m_captured.ucsOrigin.z == 3.0);
  CHECK(cap.m_captured.paperSpaceBlockRecordHandle.ref == 0x50u);
  CHECK(cap.m_captured.lastActiveViewportHandle.ref == 0x51u);
  CHECK(cap.m_captured.namedUcsHandle.ref == 0x52u);
  CHECK(cap.m_captured.baseUcsHandle.ref == 0x53u);
}

// F4f-1: dxfRW::setNamedDictObjects emits a named DICTIONARY object in the
// OBJECTS section, owned by C (parentHandle 0 -> 330 "C"), with its entry list,
// while setRootDictEntries re-attaches it under the root C dict. This is the
// emit that gives the previously-dangling 350 references a real, valid-owner
// target (clearing ezdxf INVALID_OWNER_HANDLE on the DWG->DXF path).
TEST_CASE("DXF setNamedDictObjects emits an owned named dictionary (F4f-1)",
          "[dxf][objects][dictionary]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_nameddict_emit.dxf";
  std::filesystem::remove(path);

  NullObjectEmitter em;
  {
    dxfRW w(path.string().c_str());
    DRW_Dictionary dict;
    dict.handle = 0x50u;
    dict.parentHandle = 0;  // -> owner "C"
    dict.cloning = 1;
    DRW_Dictionary::Entry e;
    e.m_name = "SCALE";
    e.m_handle = 0x51u;
    dict.m_entries.push_back(e);
    w.setNamedDictObjects({dict});
    w.setRootDictEntries({{"ACAD_SCALELIST", "50"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Root C dict carries the ACAD_SCALELIST -> 50 entry.
  CHECK(hasConsecutive(groups, {{"3", "ACAD_SCALELIST"}, {"350", "50"}}));
  // A DICTIONARY object at handle 50, owned by C, with the SCALE -> 51 entry.
  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "50"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "1"},
                        {"3", "SCALE"}, {"350", "51"}}));
}

// C-1: the named-dictionary emit preserves the FULL duplicate-record cloning
// policy (code 281), not a 0/1 collapse. A dict with cloning=12 (keep + sort)
// must round-trip as 281=12, since parseCode reads the full int.
TEST_CASE("DXF setNamedDictObjects preserves the cloning policy (code 281)",
          "[dxf][objects][dictionary]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_nameddict_cloning.dxf";
  std::filesystem::remove(path);

  NullObjectEmitter em;
  {
    dxfRW w(path.string().c_str());
    DRW_Dictionary dict;
    dict.handle = 0x50u;
    dict.parentHandle = 0;
    dict.cloning = 12;  // keep + sort — must NOT collapse to 1
    DRW_Dictionary::Entry e;
    e.m_name = "SCALE";
    e.m_handle = 0x51u;
    dict.m_entries.push_back(e);
    w.setNamedDictObjects({dict});
    w.setRootDictEntries({{"ACAD_SCALELIST", "50"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "50"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "12"}}));
}

// C-2: DRW_Point::xAxisAngle is true radians (matching the DWG path); the DXF
// writer must emit code 50 in DEGREES (radians * ARAD). A point with pi/2 rad
// must write 90, not pi/2 / ARAD (~0.0274, the pre-fix bug).
TEST_CASE("DXF writePoint emits xAxisAngle (code 50) in degrees from radians (C-2)",
          "[dxf][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_point_xaxis.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    XAxisPointEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Find the POINT record's code-50 value and assert ~90 degrees.
  bool inPoint = false;
  bool found = false;
  double angle = -1.0;
  for (const auto &kv : groups) {
    if (kv.first == "0")
      inPoint = (kv.second == "POINT");
    else if (inPoint && kv.first == "50") {
      angle = std::stod(kv.second);
      found = true;
    }
  }
  REQUIRE(found);
  CHECK(std::fabs(angle - 90.0) < 1e-6);  // 90 deg, not ~0.0274 (the /ARAD bug)
}

TEST_CASE("DXF table record writers preserve XDATA",
          "[dxf][table][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_table_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    TableXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordHasConsecutive(groups, "LTYPE", "XDASH",
                             {{"1001", "TABLEAPP"}, {"1000", "ltype-xdata"},
                              {"1071", "42"}}));
  CHECK(recordHasConsecutive(groups, "STYLE", "XDATASTYLE",
                             {{"1001", "TABLEAPP"}, {"1000", "style-xdata"},
                              {"1070", "7"}}));
  CHECK(recordHasConsecutive(groups, "VPORT", "*ACTIVE",
                             {{"1001", "TABLEAPP"}, {"1000", "vport-xdata"},
                              {"1070", "8"}}));
  CHECK(recordHasConsecutive(groups, "VIEW", "XVIEW",
                             {{"1001", "TABLEAPP"}, {"1000", "view-xdata"},
                              {"1070", "9"}}));
  CHECK(recordHasConsecutive(groups, "UCS", "XUCS",
                             {{"1001", "TABLEAPP"}, {"1000", "ucs-xdata"},
                              {"1070", "10"}}));
  CHECK(recordHasConsecutive(groups, "APPID", "XAPPID",
                             {{"1001", "TABLEAPP"}, {"1000", "appid-xdata"},
                              {"1070", "11"}}));
  CHECK(recordHasConsecutive(groups, "DIMSTYLE", "XDIM",
                             {{"1001", "TABLEAPP"}, {"1000", "dimstyle-xdata"},
                              {"1070", "12"}}));
}

TEST_CASE("DXF selected entity writers preserve XDATA",
          "[dxf][entity][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_entity_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    EntityXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordTypeHasConsecutive(
      groups, "LINE", {{"1001", "ENTITYAPP"}, {"1000", "line-xdata"},
                       {"1070", "17"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "LWPOLYLINE",
      {{"1001", "ENTITYAPP"}, {"1000", "lwpolyline-xdata"}, {"1071", "18"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "TEXT", {{"1001", "ENTITYAPP"}, {"1000", "text-xdata"},
                       {"1070", "19"}}));

  struct EntityXDataExpectation {
    const char *recordType;
    const char *payload;
    int value;
  };
  const EntityXDataExpectation expectations[] = {
      {"POINT", "point-xdata", 20},
      {"RAY", "ray-xdata", 21},
      {"XLINE", "xline-xdata", 22},
      {"CIRCLE", "circle-xdata", 23},
      {"ARC", "arc-xdata", 24},
      {"ELLIPSE", "ellipse-xdata", 25},
      {"TRACE", "trace-xdata", 26},
      {"SOLID", "solid-xdata", 27},
      {"3DFACE", "3dface-xdata", 28},
      {"POLYLINE", "polyline-xdata", 29},
      {"SPLINE", "spline-xdata", 30},
      {"HELIX", "helix-xdata", 31},
      {"HATCH", "hatch-xdata", 32},
      {"MPOLYGON", "mpolygon-xdata", 33},
      {"LEADER", "leader-xdata", 34},
      {"INSERT", "insert-xdata", 35},
      {"ACAD_TABLE", "table-xdata", 36},
      {"ATTRIB", "attrib-xdata", 37},
      {"ATTDEF", "attdef-xdata", 38},
      {"RTEXT", "rtext-xdata", 39},
      {"ARCALIGNEDTEXT", "arctext-xdata", 40},
      {"TOLERANCE", "tolerance-xdata", 41},
      {"DIMENSION", "dimension-xdata", 42},
      {"ARC_DIMENSION", "arcdim-xdata", 43},
      {"LARGE_RADIAL_DIMENSION", "largeradial-xdata", 44},
      {"MULTILEADER", "mleader-xdata", 45},
      {"LIGHT", "light-xdata", 46},
      {"MESH", "mesh-xdata", 47},
      {"SHAPE", "shape-xdata", 48},
      {"OLE2FRAME", "ole2frame-xdata", 49},
      {"VIEWPORT", "viewport-xdata", 50},
      {"IMAGE", "image-xdata", 51},
      {"WIPEOUT", "wipeout-xdata", 52},
      {"POINTCLOUD", "pointcloud-xdata", 53},
      {"PLANESURFACE", "surface-xdata", 54},
  };
  for (const auto &expected : expectations) {
    CAPTURE(expected.recordType);
    CHECK(recordTypeHasConsecutive(
        groups, expected.recordType,
        {{"1001", "ENTITYAPP"}, {"1000", expected.payload},
         {"1070", std::to_string(expected.value)}}));
  }
}

TEST_CASE("DXF selected object writers preserve XDATA",
          "[dxf][object][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_object_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    ObjectXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordTypeHasConsecutive(
      groups, "SCALE", {{"1001", "OBJECTAPP"}, {"1000", "scale-xdata"},
                        {"1070", "21"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "MLINESTYLE",
      {{"1001", "OBJECTAPP"}, {"1000", "mlinestyle-xdata"}, {"1070", "22"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "FIELD", {{"1001", "OBJECTAPP"}, {"1000", "field-xdata"},
                        {"1070", "23"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "WIPEOUTVARIABLES",
      {{"1001", "OBJECTAPP"}, {"1000", "wipeoutvars-xdata"}, {"1070", "24"}}));
}

// F3-1: dxfRW::writeEntity captures source-handle -> minted-handle in the
// writing context. Two entities seeded with distinct source handles map to two
// distinct minted handles (>= the first minted handle FIRSTHANDLE 0x30). The map
// is written here but consumed only by GROUP-emit (F3-2/F3-3).
TEST_CASE("DXF writeEntity captures source->minted handles (F3-1)",
          "[dxf][objects][handles]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_srcminted_capture.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu, 0xBBu};
  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }
  std::filesystem::remove(path);

  REQUIRE(captured.count(0xAAu) == 1);
  REQUIRE(captured.count(0xBBu) == 1);
  const std::uint32_t mintedA = captured[0xAAu];
  const std::uint32_t mintedB = captured[0xBBu];
  CHECK(mintedA >= 0x30u);
  CHECK(mintedB >= 0x30u);
  CHECK(mintedA != mintedB);
}

// B3: writeEntity must emit a code-330 owner (soft-pointer to the owning
// BLOCK_RECORD) on every R2000+ entity. Pre-fix the entity carried no 330 at
// all, so ezdxf/AutoCAD treated it as an orphan. A model-space POINT must own
// to the Model_Space BLOCK_RECORD (handle 1F), emitted right after its (5)
// handle.
TEST_CASE("DXF writeEntity emits a 330 owner handle on every entity (B3)",
          "[dxf][objects][owner]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_entity_owner.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu};
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  bool foundPoint = false;
  for (std::size_t i = 0; i + 2 < groups.size(); ++i) {
    if (groups[i].first == "0" && groups[i].second == "POINT") {
      REQUIRE(groups[i + 1].first == "5");          // minted entity handle
      REQUIRE(groups[i + 2].first == "330");         // owner handle follows
      REQUIRE(groups[i + 2].second == "1F");         // Model_Space BLOCK_RECORD
      foundPoint = true;
    }
  }
  REQUIRE(foundPoint);
}

// P1 (circle/arc extrusion+thickness): writeCircle/writeArc previously dropped
// the AcDbCircle thickness (39) and extrusion (210/220/230), so non-Z-up or
// thick circles/arcs flattened on DXF export. The reader (DRW_Point::parseCode)
// already consumed them; this confirms the writer now emits them.
TEST_CASE("DXF writeCircle/writeArc emit thickness + extrusion (P1)",
          "[dxf][objects][circle]") {
  class CurveEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Circle c;
      c.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      c.radious = 5.0;
      c.thickness = 2.5;
      c.extPoint = DRW_Coord(0.0, 0.0, -1.0);  // flipped normal
      m_rw->writeCircle(&c);
      DRW_Arc a;
      a.basePoint = DRW_Coord(3.0, 4.0, 0.0);
      a.radious = 1.5;
      a.thickness = 0.0;
      a.extPoint = DRW_Coord(0.0, 1.0, 0.0);  // sideways extrusion
      a.staangle = 0.0;
      a.endangle = 1.0;
      m_rw->writeArc(&a);
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_curve_extrusion.dxf";
  std::filesystem::remove(path);
  CurveEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Find the first (code) value within a given entity's group block (from its
  // (0,name) marker up to the next (0,*)); returns true and sets out if found.
  auto entityVal = [&](const std::string &name, const std::string &code,
                       double &out) -> bool {
    bool in = false;
    for (const auto &g : groups) {
      if (g.first == "0") { in = (g.second == name); continue; }
      if (in && g.first == code) { out = std::stod(g.second); return true; }
    }
    return false;
  };
  auto close = [](double a, double b) { double d = a - b; return (d < 0 ? -d : d) < 1e-9; };

  double v = 0.0;
  // CIRCLE: thickness 2.5, extrusion z = -1.0 (flipped).
  REQUIRE(entityVal("CIRCLE", "39", v));
  CHECK(close(v, 2.5));
  REQUIRE(entityVal("CIRCLE", "230", v));
  CHECK(close(v, -1.0));
  // ARC: no thickness (0 -> omitted), extrusion y = 1.0.
  CHECK_FALSE(entityVal("ARC", "39", v));
  REQUIRE(entityVal("ARC", "220", v));
  CHECK(close(v, 1.0));
}

// Batch A (vertex subclass): a 3D POLYLINE's VERTEX must carry the type-specific
// second subclass marker (AcDb3dPolylineVertex) after AcDbVertex. Pre-fix only
// AcDbVertex was emitted, so ezdxf/AutoCAD mis-typed 3D/mesh vertices.
TEST_CASE("DXF writePolyline emits VERTEX type subclass marker (Batch A)",
          "[dxf][objects][vertex]") {
  class Poly3dEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Polyline pl;
      pl.flags = 8;  // 3D polyline
      auto v1 = std::make_shared<DRW_Vertex>();
      v1->basePoint = DRW_Coord(0.0, 0.0, 0.0);
      auto v2 = std::make_shared<DRW_Vertex>();
      v2->basePoint = DRW_Coord(1.0, 1.0, 2.0);
      pl.vertlist.push_back(v1);
      pl.vertlist.push_back(v2);
      m_rw->writePolyline(&pl);
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_vertex_subclass.dxf";
  std::filesystem::remove(path);
  Poly3dEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(hasConsecutive(groups,
        {{"100", "AcDbVertex"}, {"100", "AcDb3dPolylineVertex"}}));
}

// Batch A (image mandatory fields): writeImage must emit class_version (90)
// and the clip boundary (71 type, 91 count, 14/24 vertices). Pre-fix these
// were absent and ezdxf flagged the IMAGE as malformed.
TEST_CASE("DXF writeImage emits class_version + clip boundary (Batch A)",
          "[dxf][objects][image]") {
  class ImgEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Image img;
      img.basePoint = DRW_Coord(0.0, 0.0, 0.0);
      img.secPoint = DRW_Coord(1.0, 0.0, 0.0);
      img.vVector = DRW_Coord(0.0, 1.0, 0.0);
      img.sizeu = 640.0;
      img.sizev = 480.0;
      img.clipPath.push_back(DRW_Coord(0.0, 0.0, 0.0));
      img.clipPath.push_back(DRW_Coord(100.0, 0.0, 0.0));
      img.clipPath.push_back(DRW_Coord(100.0, 80.0, 0.0));
      img.clipPath.push_back(DRW_Coord(0.0, 80.0, 0.0));
      m_rw->writeImage(&img, "ref.png");
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_image_fields.dxf";
  std::filesystem::remove(path);
  ImgEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  auto inImage = [&](const std::string &code, const std::string &val) {
    bool in = false;
    for (const auto &g : groups) {
      if (g.first == "0") { in = (g.second == "IMAGE"); continue; }
      if (in && g.first == code && g.second == val) return true;
    }
    return false;
  };
  CHECK(inImage("90", "0"));    // class_version
  CHECK(inImage("71", "2"));    // polygonal clip boundary type
  CHECK(inImage("91", "4"));    // 4 boundary vertices
}

// A-3: the VERTEX/SEQEND parent re-entries from writePolyline call
// writeEntity(ent) again on the already-minted parent. Those re-entries must NOT
// emplace into sourceHandleToMintedMap. After writing a POLYLINE (2 vertices,
// source 0xAA) and a POINT (source 0xBB), the map's keys must be EXACTLY the two
// genuine seeded source handles -- no extra minted-range keys. A polluting key
// (a stale minted handle) can numerically equal a real source handle and, via
// emplace's keep-first-seen, SHADOW the genuine mapping, mis-resolving GROUP 340.
TEST_CASE("DXF writeEntity does not pollute source->minted map on VERTEX/SEQEND re-entry (A-3)",
          "[dxf][objects][handles]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_srcminted_poly.dxf";
  std::filesystem::remove(path);

  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    SeededPolylineEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }
  std::filesystem::remove(path);

  // Exactly the two genuine source handles, nothing else (no re-entry pollution).
  REQUIRE(captured.size() == 2u);
  CHECK(captured.count(0xAAu) == 1);
  CHECK(captured.count(0xBBu) == 1);
  CHECK(captured[0xAAu] >= 0x30u);
  CHECK(captured[0xBBu] >= 0x30u);
  CHECK(captured[0xAAu] != captured[0xBBu]);
}

// F3-2: setGroups injects each group into the ACAD_GROUP D dict (name -> minted
// group handle) and emits a GROUP object owned by D. Members are resolved
// through the source->minted map captured by writeEntity; a member whose source
// handle was never written (0xDEAD) is dropped, never emitted as a dangling 340.
TEST_CASE("DXF setGroups emits a D-owned GROUP with resolved members (F3-2)",
          "[dxf][objects][group]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_group_emit.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu, 0xBBu};
  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Group g;
    g.name = "*A1";
    g.m_description = "test group";
    g.m_isUnnamed = true;
    g.m_selectable = true;
    g.m_entityHandles = {0xAAu, 0xBBu, 0xDEADu};  // 0xDEAD never written
    w.setGroups({g});
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  const std::string mintedA = toHexUpper(captured[0xAAu]);
  const std::string mintedB = toHexUpper(captured[0xBBu]);

  // The GROUP object: owned by D, the unnamed/selectable flags, then exactly the
  // two resolvable members as 340 (0xDEAD dropped).
  CHECK(hasConsecutive(groups,
                       {{"0", "GROUP"}}));
  CHECK(hasConsecutive(
      groups, {{"100", "AcDbGroup"}, {"300", "test group"}, {"70", "1"},
               {"71", "1"}, {"340", mintedA}, {"340", mintedB}}));
  // The GROUP is owned by D and has exactly two 340 members (0xDEAD skipped).
  // Count 340s ONLY inside the GROUP record (from its 0/GROUP marker to the next
  // 0/...), so unrelated table 340s (e.g. STYLE font handle) are not counted.
  int groupCount = 0;
  int memberCount = 0;
  std::string groupHandle;
  for (std::size_t i = 0; i < groups.size(); ++i) {
    if (groups[i].first == "0" && groups[i].second == "GROUP") {
      ++groupCount;
      if (i + 1 < groups.size())
        groupHandle = groups[i + 1].second;  // the code-5 line
      for (std::size_t j = i + 1; j < groups.size(); ++j) {
        if (groups[j].first == "0")
          break;  // end of GROUP record
        if (groups[j].first == "340")
          ++memberCount;
      }
    }
  }
  CHECK(groupCount == 1);
  CHECK(memberCount == 2);
  // The ACAD_GROUP D dict lists the group: 3 *A1 / 350 <minted group handle>.
  CHECK(hasConsecutive(groups, {{"3", "*A1"}, {"350", groupHandle}}));
}

// dxf-struct-003 (getstr-prefix): DRW_Header::getStr/getInt/getDouble/getCoord
// now try the alternate $-convention when the exact key is not found.  This
// bridges the DWG->DXF path where DWG parseDwg stores bare keys ("LTSCALE")
// but encodeDxf (DRW_Header::write) queries with "$" prefix ("$LTSCALE").
// Without the fix, every header var would fall through to the codec-internal
// default, silently discarding what was read from the DWG file.
//
// Also verifies that $FINGERPRINTGUID/$VERSIONGUID are now emitted (R2000+)
// when stored under their DWG bare-key names.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF header write tolerates bare vs $-prefixed key convention (dxf-struct-003)",
          "[dxf][header][getstr-prefix]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_header_getstr.dxf";
  std::filesystem::remove(path);

  class BareKeyHeaderEmitter : public StubInterface {
  public:
    void writeHeader(DRW_Header &h) override {
      // DWG-parse convention: bare keys, no '$'.
      h.addDouble("LTSCALE", 4.25, 40);  // default would be 1.0 -> emits "1"
      h.addInt("LUNITS", 3, 70);          // default would be 2
      h.addDouble("LATITUDE", 11.0, 40);
      h.addDouble("LONGITUDE", 22.0, 40);
      h.addDouble("NORTHDIRECTION", 33.0, 40);
      h.addInt("PELLIPSE", 1, 70);
      h.addInt("PROXIGRAPHICS", 0, 70);
      h.addInt("ISOLINES", 9, 70);
      h.addDouble("FACETRES", 0.75, 40);
      h.addInt("TEXTQLTY", 42, 70);
      h.addDouble("TDCREATE", 2450000.25, 40);
      h.addDouble("TDUPDATE", 2450001.5, 40);
      h.addDouble("TDINDWG", 0.125, 40);
      h.addDouble("TDUSRTIMER", 0.75, 40);
      h.addInt("TSTACKALIGN", 2, 70);
      h.addInt("TSTACKSIZE", 80, 70);
      h.addInt("OBSCUREDCOLOR", 123, 70);
      h.addInt("OBSCUREDLTYPE", 4, 70);
      h.addDouble("DIMALTMZF", 1.25, 40);
      h.addStr("DIMALTMZS", "alt-suffix", 1);
      h.addDouble("DIMMZF", 2.5, 40);
      h.addStr("DIMMZS", "main-suffix", 1);
      h.addStr("FINGERPRINTGUID",
               "{AABBCCDD-0000-0000-0000-001122334455}", 2);
      h.addStr("VERSIONGUID",
               "{FFEEDDCC-0000-0000-0000-AABBCCDDEEFF}", 2);
    }
  } em;

  {
    dxfRW w(path.string().c_str());
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // LTSCALE: bare key must propagate 4.25, not the default 1.
  CHECK(hasConsecutive(groups, {{"9", "$LTSCALE"}, {"40", "4.25"}}));
  // LUNITS: bare key must propagate 3, not the default 2.
  CHECK(hasConsecutive(groups, {{"9", "$LUNITS"}, {"70", "3"}}));
  CHECK(hasConsecutive(groups, {{"9", "$LATITUDE"}, {"40", "11"}}));
  CHECK(hasConsecutive(groups, {{"9", "$LONGITUDE"}, {"40", "22"}}));
  // NORTHDIRECTION must use its own value, not the longitude value.
  CHECK(hasConsecutive(groups, {{"9", "$NORTHDIRECTION"}, {"40", "33"}}));
  CHECK(hasConsecutive(groups, {{"9", "$PELLIPSE"}, {"70", "1"}}));
  CHECK(hasConsecutive(groups, {{"9", "$PROXYGRAPHICS"}, {"70", "0"}}));
  CHECK(hasConsecutive(groups, {{"9", "$ISOLINES"}, {"70", "9"}}));
  CHECK(hasConsecutive(groups, {{"9", "$FACETRES"}, {"40", "0.75"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TEXTQLTY"}, {"70", "42"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDCREATE"}, {"40", "2450000.25"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDUPDATE"}, {"40", "2450001.5"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDINDWG"}, {"40", "0.125"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDUSRTIMER"}, {"40", "0.75"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TSTACKALIGN"}, {"70", "2"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TSTACKSIZE"}, {"70", "80"}}));
  CHECK(hasConsecutive(groups, {{"9", "$OBSCOLOR"}, {"70", "123"}}));
  CHECK(hasConsecutive(groups, {{"9", "$OBSLTYPE"}, {"280", "4"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMALTMZF"}, {"40", "1.25"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMALTMZS"}, {"1", "alt-suffix"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMMZF"}, {"40", "2.5"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMMZS"}, {"1", "main-suffix"}}));
  // GUIDs must appear (not silently skipped) when stored as bare keys.
  CHECK(hasConsecutive(groups,
      {{"9", "$FINGERPRINTGUID"},
       {"2", "{AABBCCDD-0000-0000-0000-001122334455}"}}));
  CHECK(hasConsecutive(groups,
      {{"9", "$VERSIONGUID"},
       {"2", "{FFEEDDCC-0000-0000-0000-AABBCCDDEEFF}"}}));
}

// ── Preservation parity: structured DXF read of objects previously raw-only ──
namespace {
class MaterialCapture : public StubInterface {
public:
  int m_callCount = 0;
  int m_rawCount = 0;
  DRW_Material m_captured;
  void addMaterial(const DRW_Material &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
  void addRawDxfObject(const DRW_RawDxfObject &) override { ++m_rawCount; }
};

class GeoDataCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_GeoData m_captured;
  void addGeoData(const DRW_GeoData &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF MATERIAL is read into a DRW_Material (name + description)",
          "[dxf][material][preservation]") {
  MaterialCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\nEF\n330\nC\n100\nAcDbMaterial\n"
      "1\nBrass\n2\nPolished brass material\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_material_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_name == "Brass");
  CHECK(cap.m_captured.m_description == "Polished brass material");
  // Dual-mode: also preserved raw for lossless DXF re-emit.
  CHECK(cap.m_rawCount == 1);
}

TEST_CASE("DXF GEODATA is read into a DRW_GeoData (scalar geolocation fields)",
          "[dxf][geodata][preservation]") {
  GeoDataCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGEODATA\n5\nF0\n330\n1F\n100\nAcDbGeoData\n"
      "90\n3\n70\n2\n"
      "10\n100.0\n20\n200.0\n30\n0.0\n"
      "11\n12.5\n21\n55.25\n31\n0.0\n"
      "40\n1.0\n41\n1.0\n"
      "95\n1\n141\n2.5\n294\n1\n142\n123.0\n"
      "302\ngeo-rss-tag\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_geodata_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  const DRW_GeoData &g = cap.m_captured;
  CHECK(g.m_version == 3);
  CHECK(g.m_coordinatesType == 2);
  CHECK(g.m_designPoint.x == 100.0);
  CHECK(g.m_designPoint.y == 200.0);
  CHECK(g.m_referencePoint.x == 12.5);
  CHECK(g.m_referencePoint.y == 55.25);
  CHECK(g.m_scaleEstimationMethod == 1);
  CHECK(g.m_userSpecifiedScaleFactor == 2.5);
  CHECK(g.m_enableSeaLevelCorrection == true);
  CHECK(g.m_seaLevelElevation == 123.0);
  CHECK(g.m_geoRssTag == "geo-rss-tag");
}

TEST_CASE("DXF GEODATA object writes class and geolocation fields",
          "[dxf][geodata][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_geodata_write.dxf";
  std::filesystem::remove(path);

  class GeoDataEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_GeoData m_geo;

    void writeObjects() override { m_rw->writeGeoData(&m_geo); }
  };

  GeoDataEmitter em;
  em.m_geo.handle = 0xF0u;
  em.m_geo.parentHandle = 0x1Fu;
  em.m_geo.m_hostBlockHandle = 0x70u;
  em.m_geo.m_version = 3;
  em.m_geo.m_coordinatesType = 2;
  em.m_geo.m_designPoint = DRW_Coord(100.0, 200.0, 0.0);
  em.m_geo.m_referencePoint = DRW_Coord(12.5, 55.25, 3.0);
  em.m_geo.m_horizontalUnitScale = 1.0;
  em.m_geo.m_verticalUnitScale = 1.0;
  em.m_geo.m_horizontalUnits = 6;
  em.m_geo.m_verticalUnits = 6;
  em.m_geo.m_upDirection = DRW_Coord(0.0, 0.0, 1.0);
  em.m_geo.m_northDirection = DRW_Coord(0.0, 1.0, 0.0);
  em.m_geo.m_scaleEstimationMethod = 1;
  em.m_geo.m_userSpecifiedScaleFactor = 2.5;
  em.m_geo.m_enableSeaLevelCorrection = true;
  em.m_geo.m_seaLevelElevation = 123.0;
  em.m_geo.m_coordinateProjectionRadius = 6378137.0;
  em.m_geo.m_coordinateSystemDefinition = "EPSG:4326";
  em.m_geo.m_geoRssTag = "geo-rss";
  em.m_geo.m_observationFromTag = "from-tag";
  em.m_geo.m_observationToTag = "to-tag";
  em.m_geo.m_observationCoverageTag = "coverage-tag";
  DRW_GeoMeshPoint point;
  point.m_source = DRW_Coord(0.0, 0.0, 0.0);
  point.m_destination = DRW_Coord(100.0, 200.0, 0.0);
  em.m_geo.m_points.push_back(point);
  point.m_source = DRW_Coord(1.0, 2.0, 0.0);
  point.m_destination = DRW_Coord(101.0, 202.0, 0.0);
  em.m_geo.m_points.push_back(point);
  DRW_GeoMeshFace face;
  face.m_index1 = 0;
  face.m_index2 = 1;
  face.m_index3 = 0;
  em.m_geo.m_faces.push_back(face);

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("GEODATA", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary geoDict;
    geoDict.handle = 0x1Fu;
    geoDict.parentHandle = 0;
    geoDict.cloning = 1;
    geoDict.m_entries.push_back({"GeoData", 0xF0u});
    w.setNamedDictObjects({geoDict});
    w.setRootDictEntries({{"ACAD_GEOGRAPHICDATA", "1F"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "GEODATA"},
                        {"2", "AcDbGeoData"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_GEOGRAPHICDATA"}, {"350", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "GEODATA"}, {"5", "F0"}, {"330", "1F"},
                        {"100", "AcDbGeoData"}, {"90", "3"}, {"330", "70"},
                        {"70", "2"}}));
  CHECK(hasConsecutive(groups, {{"301", "EPSG:4326"}, {"302", "geo-rss"}}));
  CHECK(hasConsecutive(groups, {{"93", "2"}}));
  CHECK(hasConsecutive(groups, {{"96", "1"}, {"97", "0"}, {"98", "1"},
                                {"99", "0"}}));

  GeoDataCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_GeoData &g = cap.m_captured;
  CHECK(g.handle == 0xF0u);
  CHECK(g.parentHandle == 0x1F);
  CHECK(g.m_hostBlockHandle == 0x70u);
  CHECK(g.m_version == 3);
  CHECK(g.m_coordinatesType == 2);
  CHECK(g.m_designPoint.x == 100.0);
  CHECK(g.m_referencePoint.z == 3.0);
  CHECK(g.m_horizontalUnits == 6);
  CHECK(g.m_verticalUnits == 6);
  CHECK(g.m_upDirection.z == 1.0);
  CHECK(g.m_northDirection.y == 1.0);
  CHECK(g.m_scaleEstimationMethod == 1);
  CHECK(g.m_userSpecifiedScaleFactor == 2.5);
  CHECK(g.m_enableSeaLevelCorrection == true);
  CHECK(g.m_seaLevelElevation == 123.0);
  CHECK(g.m_coordinateProjectionRadius == 6378137.0);
  CHECK(g.m_coordinateSystemDefinition == "EPSG:4326");
  CHECK(g.m_geoRssTag == "geo-rss");
  CHECK(g.m_observationFromTag == "from-tag");
  CHECK(g.m_observationToTag == "to-tag");
  CHECK(g.m_observationCoverageTag == "coverage-tag");
  REQUIRE(g.m_points.size() == 2);
  CHECK(g.m_points[0].m_source.x == 0.0);
  CHECK(g.m_points[0].m_destination.y == 200.0);
  CHECK(g.m_points[1].m_source.y == 2.0);
  CHECK(g.m_points[1].m_destination.x == 101.0);
  REQUIRE(g.m_faces.size() == 1);
  CHECK(g.m_faces[0].m_index1 == 0);
  CHECK(g.m_faces[0].m_index2 == 1);
  CHECK(g.m_faces[0].m_index3 == 0);
}

namespace {
class VisualStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_VisualStyle m_captured;
  void addVisualStyle(const DRW_VisualStyle &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF VISUALSTYLE is read into a DRW_VisualStyle (desc + type)",
          "[dxf][visualstyle][preservation]") {
  VisualStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nVISUALSTYLE\n5\nF1\n330\nC\n100\nAcDbVisualStyle\n"
      "2\nConceptual\n70\n5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_visualstyle_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.desc == "Conceptual");
  CHECK(cap.m_captured.type == 5);
}

namespace {
class TableStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_TableStyle m_captured;
  void addTableStyle(const DRW_TableStyle &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
class MLeaderStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MLeaderStyle m_captured;
  void addMLeaderStyle(const DRW_MLeaderStyle *d) override {
    if (m_callCount == 0 && d) m_captured = *d;
    ++m_callCount;
  }
};
class SpatialFilterCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SpatialFilter m_captured;
  void addSpatialFilter(const DRW_SpatialFilter &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
class ImageDefReactorCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_ImageDefinitionReactor m_captured;
  void addImageDefinitionReactor(const DRW_ImageDefinitionReactor &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF TABLESTYLE is read into a DRW_TableStyle (top-level fields)",
          "[dxf][tablestyle][preservation]") {
  TableStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nTABLESTYLE\n5\n1C0\n330\nC\n100\nAcDbTableStyle\n"
      "3\nMyStyle\n70\n1\n71\n2\n40\n0.06\n41\n0.07\n280\n1\n281\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_tablestyle_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_name == "MyStyle");
  CHECK(cap.m_captured.m_flowDirection == 1);
  CHECK(cap.m_captured.m_flags == 2);
  CHECK(cap.m_captured.m_horizontalCellMargin == 0.06);
  CHECK(cap.m_captured.m_verticalCellMargin == 0.07);
  CHECK(cap.m_captured.m_titleSuppressed == true);
  CHECK(cap.m_captured.m_headerSuppressed == false);
}

TEST_CASE("DXF MLEADERSTYLE is read into a DRW_MLeaderStyle",
          "[dxf][mleaderstyle][preservation]") {
  MLeaderStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMLEADERSTYLE\n5\n1D0\n330\nC\n100\nAcDbMLeaderStyle\n"
      "3\nStandard\n300\nDefault Text\n170\n2\n90\n2\n"
      "40\n0.5\n41\n0.75\n173\n1\n44\n2.5\n45\n3.0\n"
      "290\n1\n291\n0\n142\n1.5\n296\n1\n"
      "340\n1E\n342\n20\n343\n21\n271\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_mleaderstyle_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_MLeaderStyle &s = cap.m_captured;
  CHECK(s.description == "Standard");
  CHECK(s.textDefault == "Default Text");
  CHECK(s.contentType == 2);
  CHECK(s.maxLeaderPoints == 2);
  CHECK(s.firstSegmentAngle == 0.5);
  CHECK(s.secondSegmentAngle == 0.75);
  CHECK(s.leaderType == 1);
  CHECK(s.arrowHeadSize == 2.5);
  CHECK(s.textHeight == 3.0);
  CHECK(s.landingEnabled == true);
  CHECK(s.autoIncludeLanding == false);
  CHECK(s.scaleFactor == 1.5);
  CHECK(s.isAnnotative == true);
  CHECK(s.attachmentDirection == 2);
  CHECK(s.leaderLineTypeHandle.ref == 0x1Eu);
  CHECK(s.textStyleHandle.ref == 0x20u);
  CHECK(s.blockHandle.ref == 0x21u);
}

TEST_CASE("DXF MLEADERSTYLE object writes class and style fields",
          "[dxf][mleaderstyle][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_mleaderstyle_write.dxf";
  std::filesystem::remove(path);

  class MLeaderStyleEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_MLeaderStyle m_style;

    void writeObjects() override { m_rw->writeMLeaderStyle(&m_style); }
  };

  MLeaderStyleEmitter em;
  em.m_style.handle = 0x1D0u;
  em.m_style.parentHandle = 0x1CFu;
  em.m_style.name = "Standard";
  em.m_style.flags = 4;
  em.m_style.styleVersion = 2;
  em.m_style.contentType = 2;
  em.m_style.drawMLeaderOrder = 1;
  em.m_style.drawLeaderOrder = 0;
  em.m_style.maxLeaderPoints = 7;
  em.m_style.firstSegmentAngle = 0.5;
  em.m_style.secondSegmentAngle = 0.75;
  em.m_style.leaderType = 1;
  em.m_style.leaderColor = 3;
  em.m_style.leaderLineTypeHandle.ref = 0x1E0u;
  em.m_style.leaderLineWeight = 29;
  em.m_style.landingEnabled = true;
  em.m_style.landingGap = 1.25;
  em.m_style.autoIncludeLanding = false;
  em.m_style.landingDistance = 2.25;
  em.m_style.description = "Round-trip MLeader style";
  em.m_style.arrowHeadBlockHandle.ref = 0x1E1u;
  em.m_style.arrowHeadSize = 0.75;
  em.m_style.textDefault = "Default leader text";
  em.m_style.textStyleHandle.ref = 0x1E2u;
  em.m_style.leftAttachment = 1;
  em.m_style.rightAttachment = 2;
  em.m_style.textAngleType = 1;
  em.m_style.textAlignmentType = 2;
  em.m_style.textColor = 5;
  em.m_style.textHeight = 2.5;
  em.m_style.textFrameEnabled = true;
  em.m_style.alwaysAlignTextLeft = true;
  em.m_style.alignSpace = 0.625;
  em.m_style.blockHandle.ref = 0x1E3u;
  em.m_style.blockColor = 6;
  em.m_style.blockScale = DRW_Coord(1.0, 2.0, 3.0);
  em.m_style.blockScaleEnabled = true;
  em.m_style.blockRotation = 0.25;
  em.m_style.blockRotationEnabled = true;
  em.m_style.blockConnectionType = 1;
  em.m_style.scaleFactor = 1.5;
  em.m_style.propertyChanged = true;
  em.m_style.isAnnotative = true;
  em.m_style.breakSize = 0.125;
  em.m_style.attachmentDirection = 2;
  em.m_style.topAttachment = 9;
  em.m_style.bottomAttachment = 10;
  em.m_style.textExtended = true;

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("MLEADERSTYLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary styleDict;
    styleDict.handle = 0x1CFu;
    styleDict.parentHandle = 0;
    styleDict.cloning = 1;
    styleDict.m_entries.push_back({"Standard", 0x1D0u});
    w.setNamedDictObjects({styleDict});
    w.setRootDictEntries({{"ACAD_MLEADERSTYLE", "1CF"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "MLEADERSTYLE"},
                        {"2", "AcDbMLeaderStyle"},
                        {"3", "ACDB_MLEADERSTYLE_CLASS"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_MLEADERSTYLE"}, {"350", "1CF"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "MLEADERSTYLE"}, {"5", "1D0"}, {"330", "1CF"},
                        {"100", "AcDbMLeaderStyle"}, {"2", "Standard"}}));
  CHECK(hasConsecutive(groups,
                       {{"340", "1E0"}, {"92", "29"}, {"290", "1"}}));
  CHECK(hasConsecutive(groups,
                       {{"300", "Default leader text"}, {"342", "1E2"}}));
  CHECK(hasConsecutive(groups,
                       {{"343", "1E3"}, {"94", "6"}}));

  MLeaderStyleCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_MLeaderStyle &s = cap.m_captured;
  CHECK(s.handle == 0x1D0u);
  CHECK(s.parentHandle == 0x1CF);
  CHECK(s.name == "Standard");
  CHECK(s.flags == 4);
  CHECK(s.styleVersion == 2);
  CHECK(s.drawMLeaderOrder == 1);
  CHECK(s.maxLeaderPoints == 7);
  CHECK(s.leaderColor == 3);
  CHECK(s.leaderLineTypeHandle.ref == 0x1E0u);
  CHECK(s.landingGap == 1.25);
  CHECK(s.autoIncludeLanding == false);
  CHECK(s.description == "Round-trip MLeader style");
  CHECK(s.arrowHeadBlockHandle.ref == 0x1E1u);
  CHECK(s.textDefault == "Default leader text");
  CHECK(s.textStyleHandle.ref == 0x1E2u);
  CHECK(s.textFrameEnabled == true);
  CHECK(s.blockHandle.ref == 0x1E3u);
  CHECK(s.blockScale.z == 3.0);
  CHECK(s.isAnnotative == true);
  CHECK(s.bottomAttachment == 10);
  CHECK(s.textExtended == true);
}

TEST_CASE("DXF SPATIAL_FILTER is read into a DRW_SpatialFilter",
          "[dxf][spatialfilter][preservation]") {
  SpatialFilterCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSPATIAL_FILTER\n5\n1B0\n330\nC\n100\nAcDbFilter\n"
      "100\nAcDbSpatialFilter\n"
      "70\n3\n10\n0.0\n20\n0.0\n10\n10.0\n20\n0.0\n10\n10.0\n20\n10.0\n"
      "210\n0.0\n220\n0.0\n230\n1.0\n11\n1.0\n21\n2.0\n31\n0.0\n"
      "71\n1\n72\n0\n73\n1\n41\n5.5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_spatialfilter_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SpatialFilter &f = cap.m_captured;
  REQUIRE(f.m_boundaryPoints.size() == 3);
  CHECK(f.m_boundaryPoints[1].x == 10.0);
  CHECK(f.m_boundaryPoints[2].y == 10.0);
  CHECK(f.m_normal.z == 1.0);
  CHECK(f.m_origin.x == 1.0);
  CHECK(f.m_origin.y == 2.0);
  CHECK(f.m_displayBoundary == true);
  CHECK(f.m_clipFrontPlane == false);
  CHECK(f.m_clipBackPlane == true);
  CHECK(f.m_backDistance == 5.5);
}

TEST_CASE("DXF SPATIAL_FILTER object writes class and clip fields",
          "[dxf][spatialfilter][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_spatialfilter_write.dxf";
  std::filesystem::remove(path);

  class SpatialFilterEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SpatialFilter m_filter;

    void writeObjects() override { m_rw->writeSpatialFilter(&m_filter); }
  };

  SpatialFilterEmitter em;
  em.m_filter.handle = 0x1B0u;
  em.m_filter.parentHandle = 0x1AFu;
  em.m_filter.m_boundaryPoints = {
      DRW_Coord(0.0, 0.0, 0.0),
      DRW_Coord(10.0, 0.0, 0.0),
      DRW_Coord(10.0, 10.0, 0.0)};
  em.m_filter.m_normal = DRW_Coord(0.0, 0.0, 1.0);
  em.m_filter.m_origin = DRW_Coord(1.0, 2.0, 0.0);
  em.m_filter.m_displayBoundary = true;
  em.m_filter.m_clipFrontPlane = true;
  em.m_filter.m_frontDistance = 2.25;
  em.m_filter.m_clipBackPlane = true;
  em.m_filter.m_backDistance = 5.5;
  em.m_filter.m_inverseInsertTransform = {
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0,
      3.0, 4.0, 5.0};
  em.m_filter.m_insertTransform = {
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0,
      6.0, 7.0, 8.0};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SPATIAL_FILTER", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary filterDict;
    filterDict.handle = 0x1AFu;
    filterDict.parentHandle = 0;
    filterDict.cloning = 1;
    filterDict.m_entries.push_back({"SPATIAL", 0x1B0u});
    w.setNamedDictObjects({filterDict});
    w.setRootDictEntries({{"ACAD_SPATIALFILTERS", "1AF"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "SPATIAL_FILTER"},
                        {"2", "AcDbSpatialFilter"},
                        {"3", "ObjectDBX Classes"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_SPATIALFILTERS"}, {"350", "1AF"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SPATIAL_FILTER"}, {"5", "1B0"},
                        {"330", "1AF"}, {"100", "AcDbFilter"},
                        {"100", "AcDbSpatialFilter"}, {"70", "3"}}));
  CHECK(hasConsecutive(groups,
                       {{"71", "1"}, {"72", "1"}, {"40", "2.25"},
                        {"73", "1"}, {"41", "5.5"}}));
  CHECK(hasConsecutive(groups, {{"40", "6"}, {"40", "7"}, {"40", "8"}}));

  SpatialFilterCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_SpatialFilter &f = cap.m_captured;
  CHECK(f.handle == 0x1B0u);
  CHECK(f.parentHandle == 0x1AF);
  REQUIRE(f.m_boundaryPoints.size() == 3);
  CHECK(f.m_boundaryPoints[1].x == 10.0);
  CHECK(f.m_boundaryPoints[2].y == 10.0);
  CHECK(f.m_normal.z == 1.0);
  CHECK(f.m_origin.x == 1.0);
  CHECK(f.m_origin.y == 2.0);
  CHECK(f.m_displayBoundary == true);
  CHECK(f.m_clipFrontPlane == true);
  CHECK(f.m_frontDistance == 2.25);
  CHECK(f.m_clipBackPlane == true);
  CHECK(f.m_backDistance == 5.5);
  REQUIRE(f.m_inverseInsertTransform.size() == 12);
  REQUIRE(f.m_insertTransform.size() == 12);
  CHECK(f.m_inverseInsertTransform[9] == 3.0);
  CHECK(f.m_inverseInsertTransform[10] == 4.0);
  CHECK(f.m_inverseInsertTransform[11] == 5.0);
  CHECK(f.m_insertTransform[9] == 6.0);
  CHECK(f.m_insertTransform[10] == 7.0);
  CHECK(f.m_insertTransform[11] == 8.0);
}

TEST_CASE("DXF IMAGEDEF_REACTOR is read into a DRW_ImageDefinitionReactor",
          "[dxf][imagedefreactor][preservation]") {
  ImageDefReactorCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIMAGEDEF_REACTOR\n5\n1A0\n330\n1A\n100\nAcDbRasterImageDefReactor\n"
      "90\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_imagedefreactor_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_classVersion == 2);
}

namespace {
class SortEntsTableCapture : public StubInterface {
public:
  int m_callCount = 0;
  std::vector<DRW_RawDxfObject> m_rawObjects;
  DRW_SortEntsTable m_captured;
  void addSortEntsTable(const DRW_SortEntsTable &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
  void addRawDxfObject(const DRW_RawDxfObject &d) override {
    m_rawObjects.push_back(d);
  }
};
class DimAssocCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DimensionAssociation m_captured;
  void addDimensionAssociation(const DRW_DimensionAssociation &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF SORTENTSTABLE is read into a DRW_SortEntsTable (draw order)",
          "[dxf][sortents][preservation]") {
  SortEntsTableCapture cap;
  // 5/330 before the 100 marker are the object's own handle/owner; after it,
  // 330=block owner and 331/5 are the entity/sort pairs.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSORTENTSTABLE\n5\n355\n330\n1A\n100\nAcDbSortentsTable\n"
      "330\n1F\n331\n35A\n5\n354\n331\n35B\n5\n356\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sortents_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SortEntsTable &s = cap.m_captured;
  CHECK(s.m_blockOwnerHandle == 0x1Fu);
  REQUIRE(s.m_entityHandles.size() == 2);
  REQUIRE(s.m_sortHandles.size() == 2);
  CHECK(s.m_entityHandles[0] == 0x35Au);
  CHECK(s.m_entityHandles[1] == 0x35Bu);
  CHECK(s.m_sortHandles[0] == 0x354u);
  CHECK(s.m_sortHandles[1] == 0x356u);
  const auto rawSort = std::find_if(
      cap.m_rawObjects.begin(), cap.m_rawObjects.end(),
      [](const DRW_RawDxfObject &o) { return o.name == "SORTENTSTABLE"; });
  REQUIRE(rawSort != cap.m_rawObjects.end());
  CHECK(rawSort->handle == 0x355u);
  CHECK(rawSort->parentHandle == 0x1Au);
}

TEST_CASE("DXF SORTENTSTABLE object writes class and draw order",
          "[dxf][sortents][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_sortents_write.dxf";
  std::filesystem::remove(path);

  class SortEntsTableEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SortEntsTable m_table;

    void writeObjects() override { m_rw->writeSortEntsTable(&m_table); }
  };

  SortEntsTableEmitter em;
  em.m_table.handle = 0x355u;
  em.m_table.parentHandle = 0x1Au;
  em.m_table.m_blockOwnerHandle = 0x1Fu;
  em.m_table.m_entityHandles = {0x35Au, 0x35Bu};
  em.m_table.m_sortHandles = {0x354u, 0x356u};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SORTENTSTABLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary sortDict;
    sortDict.handle = 0x1Au;
    sortDict.parentHandle = 0;
    sortDict.cloning = 1;
    sortDict.m_entries.push_back({"DrawOrder", 0x355u});
    w.setNamedDictObjects({sortDict});
    w.setRootDictEntries({{"ACAD_SORTENTS", "1A"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "SORTENTSTABLE"},
                        {"2", "AcDbSortentsTable"},
                        {"3", "ObjectDBX Classes"}}));
  CHECK(hasConsecutive(groups, {{"3", "ACAD_SORTENTS"}, {"350", "1A"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SORTENTSTABLE"}, {"5", "355"},
                        {"330", "1A"}, {"100", "AcDbSortentsTable"},
                        {"330", "1F"}, {"331", "35A"}, {"5", "354"},
                        {"331", "35B"}, {"5", "356"}}));

  SortEntsTableCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_SortEntsTable &s = cap.m_captured;
  CHECK(s.handle == 0x355u);
  CHECK(s.parentHandle == 0x1A);
  CHECK(s.m_blockOwnerHandle == 0x1Fu);
  REQUIRE(s.m_entityHandles.size() == 2);
  REQUIRE(s.m_sortHandles.size() == 2);
  CHECK(s.m_entityHandles[0] == 0x35Au);
  CHECK(s.m_entityHandles[1] == 0x35Bu);
  CHECK(s.m_sortHandles[0] == 0x354u);
  CHECK(s.m_sortHandles[1] == 0x356u);
  const auto rawSort = std::find_if(
      cap.m_rawObjects.begin(), cap.m_rawObjects.end(),
      [](const DRW_RawDxfObject &o) { return o.name == "SORTENTSTABLE"; });
  REQUIRE(rawSort != cap.m_rawObjects.end());
  CHECK(rawSort->handle == 0x355u);
}

TEST_CASE("DXF SORTENTSTABLE remaps source entity handles on write",
          "[dxf][sortents][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_sortents_remap.dxf";
  std::filesystem::remove(path);

  class SortEntsRemapEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SortEntsTable m_table;

    void writeEntities() override {
      DRW_Point first;
      first.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      first.handle = 0x35Au;
      m_rw->writePoint(&first);
      DRW_Point second;
      second.basePoint = DRW_Coord(3.0, 4.0, 0.0);
      second.handle = 0x35Bu;
      m_rw->writePoint(&second);
    }
    void writeObjects() override { m_rw->writeSortEntsTable(&m_table); }
  };

  SortEntsRemapEmitter em;
  em.m_table.handle = 0x355u;
  em.m_table.parentHandle = 0;
  em.m_table.m_blockOwnerHandle = 0x1Fu;
  em.m_table.m_entityHandles = {0x35Au, 0x35Bu};
  em.m_table.m_sortHandles = {0x35Bu, 0x35Au};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SORTENTSTABLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);
  auto handlesForType = [](const auto &groupList, const std::string &typeName) {
    std::vector<std::string> handles;
    for (std::size_t i = 0; i < groupList.size(); ++i) {
      if (groupList[i].first != "0" || groupList[i].second != typeName)
        continue;
      for (std::size_t j = i + 1; j < groupList.size()
           && groupList[j].first != "0"; ++j) {
        if (groupList[j].first == "5") {
          handles.push_back(groupList[j].second);
          break;
        }
      }
    }
    return handles;
  };
  const auto pointHandles = handlesForType(groups, "POINT");
  REQUIRE(pointHandles.size() >= 2);
  CHECK(hasConsecutive(groups,
                       {{"0", "POINT"}, {"5", pointHandles[0]},
                        {"330", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "POINT"}, {"5", pointHandles[1]},
                        {"330", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SORTENTSTABLE"}, {"5", "355"},
                        {"330", "C"}, {"100", "AcDbSortentsTable"},
                        {"330", "1F"}, {"331", pointHandles[0]},
                        {"5", pointHandles[1]}, {"331", pointHandles[1]},
                        {"5", pointHandles[0]}}));
}

TEST_CASE("DXF DIMASSOC is read into a DRW_DimensionAssociation",
          "[dxf][dimassoc][preservation]") {
  DimAssocCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDIMASSOC\n5\n521\n330\n1A\n100\nAcDbDimAssoc\n"
      "330\n500\n90\n3\n70\n0\n71\n0\n"
      "1\nAcDbOsnapPointRef\n72\n1\n331\n510\n"
      "1\nAcDbOsnapPointRef\n72\n7\n331\n511\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dimassoc_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_DimensionAssociation &a = cap.m_captured;
  CHECK(a.m_dimensionHandle == 0x500u);
  CHECK(a.m_associativityFlags == 3u);
  CHECK(a.m_isTransSpace == false);
  REQUIRE(a.m_osnapRefs.size() == 2);
  CHECK(a.m_osnapRefs[0].m_className == "AcDbOsnapPointRef");
  CHECK(a.m_osnapRefs[0].m_objectOsnapType == 1);
  CHECK(a.m_osnapRefs[0].m_objectHandle == 0x510u);
  CHECK(a.m_osnapRefs[1].m_objectOsnapType == 7);
  CHECK(a.m_osnapRefs[1].m_objectHandle == 0x511u);
}

namespace {
class BackgroundCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Background m_captured;
  void addBackground(const DRW_Background &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF GRADIENTBACKGROUND is read into a DRW_Background",
          "[dxf][background][preservation]") {
  BackgroundCapture cap;
  // 90 appears twice: class version, then color_top.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGRADIENTBACKGROUND\n5\n2A0\n330\nC\n100\nAcDbGradientBackground\n"
      "90\n1\n90\n100\n91\n200\n92\n300\n140\n0.5\n141\n0.25\n142\n1.57\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_gradientbg_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Background &b = cap.m_captured;
  CHECK(b.m_kind == DRW_Background::Gradient);
  CHECK(b.m_classVersion == 1);
  CHECK(b.m_colorTop == 100);
  CHECK(b.m_colorMiddle == 200);
  CHECK(b.m_colorBottom == 300);
  CHECK(b.m_horizon == 0.5);
  CHECK(b.m_height == 0.25);
  CHECK(b.m_rotation == 1.57);
}

TEST_CASE("DXF IMAGEBACKGROUND is read into a DRW_Background",
          "[dxf][background][preservation]") {
  BackgroundCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIMAGEBACKGROUND\n5\n2A1\n330\nC\n100\nAcDbImageBackground\n"
      "90\n1\n300\nsky.jpg\n290\n1\n291\n0\n292\n1\n"
      "140\n2.0\n142\n1.5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_imagebg_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Background &b = cap.m_captured;
  CHECK(b.m_kind == DRW_Background::Image);
  CHECK(b.m_fileName == "sky.jpg");
  CHECK(b.m_fitToScreen == true);
  CHECK(b.m_maintainAspect == false);
  CHECK(b.m_useTiling == true);
  CHECK(b.m_offset.x == 2.0);
  CHECK(b.m_scale.x == 1.5);
  // offset.y/scale.y (DXF 240/242) are unreadable by libdxfrw's dxfReader (the
  // 240-269 code range is an unhandled gap) — preserved raw, not decoded here.
}

TEST_CASE("DXF SKYLIGHTBACKGROUND + SOLIDBACKGROUND read into DRW_Background",
          "[dxf][background][preservation]") {
  {
    BackgroundCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nSKYLIGHTBACKGROUND\n5\n2A2\n330\nC\n100\nAcDbSkyBackground\n"
        "90\n2\n340\n2BC\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_skybg_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_Background::Skylight);
    CHECK(cap.m_captured.m_classVersion == 2);
    CHECK(cap.m_captured.m_sunHandle == 0x2BCu);
  }
  {
    BackgroundCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nSOLIDBACKGROUND\n5\n2A3\n330\nC\n100\nAcDbSolidBackground\n"
        "90\n1\n90\n255\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_solidbg_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_Background::Solid);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_solidColor == 255);
  }
}

namespace {
class PointCloudDefCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_PointCloudDef m_captured;
  void addPointCloudDef(const DRW_PointCloudDef &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF POINTCLOUDDEFINITION is read into a DRW_PointCloudDef",
          "[dxf][pointcloud][preservation]") {
  PointCloudDefCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPOINTCLOUDDEFINITION\n5\n2D0\n330\nC\n100\nAcDbPointCloudDef\n"
      "90\n1\n1\nscan.rcp\n280\n1\n"
      "10\n-5.0\n20\n-6.0\n30\n0.0\n11\n5.0\n21\n6.0\n31\n2.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointclouddef_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_PointCloudDef &p = cap.m_captured;
  CHECK(p.m_kind == DRW_PointCloudDef::Definition);
  CHECK(p.m_classVersion == 1);
  CHECK(p.m_sourceFilename == "scan.rcp");
  CHECK(p.m_isLoaded == true);
  CHECK(p.m_extentsMin.x == -5.0);
  CHECK(p.m_extentsMin.y == -6.0);
  CHECK(p.m_extentsMax.x == 5.0);
  CHECK(p.m_extentsMax.z == 2.0);
}

TEST_CASE("DXF POINTCLOUDDEFREACTOR is read (class version only)",
          "[dxf][pointcloud][preservation]") {
  PointCloudDefCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPOINTCLOUDDEFREACTOR\n5\n2D1\n330\nC\n100\nAcDbPointCloudDefReactor\n"
      "90\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloudreactor_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_kind == DRW_PointCloudDef::Reactor);
  CHECK(cap.m_captured.m_classVersion == 2);
}

namespace {
class SunStudyCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SunStudy m_captured;
  void addSunStudy(const DRW_SunStudy &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF SUNSTUDY is read into a DRW_SunStudy (scalar config)",
          "[dxf][sunstudy][preservation]") {
  SunStudyCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUNSTUDY\n5\n2E0\n330\nC\n100\nAcDbSunStudy\n"
      "90\n1\n1\nStudy1\n2\nMy study\n3\nSet\n4\nSubset\n70\n2\n"
      "290\n1\n291\n0\n292\n1\n293\n1\n294\n0\n"
      "93\n100\n94\n200\n95\n10\n74\n5\n75\n4\n76\n2\n77\n2\n40\n0.5\n"
      "341\n2E1\n342\n2E2\n343\n2E3\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sunstudy_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SunStudy &s = cap.m_captured;
  CHECK(s.m_classVersion == 1);
  CHECK(s.m_setupName == "Study1");
  CHECK(s.m_description == "My study");
  CHECK(s.m_sheetSetName == "Set");
  CHECK(s.m_sheetSubsetName == "Subset");
  CHECK(s.m_outputType == 2);
  CHECK(s.m_useSubset == true);
  CHECK(s.m_selectDatesFromCalendar == false);
  CHECK(s.m_selectRangeOfDates == true);
  CHECK(s.m_lockViewports == true);
  CHECK(s.m_labelViewports == false);
  CHECK(s.m_startTime == 100);
  CHECK(s.m_endTime == 200);
  CHECK(s.m_interval == 10);
  CHECK(s.m_viewportCount == 4);
  CHECK(s.m_rowCount == 2);
  CHECK(s.m_columnCount == 2);
  CHECK(s.m_spacing == 0.5);
  CHECK(s.m_viewHandle == 0x2E1u);
  CHECK(s.m_visualStyleHandle == 0x2E2u);
  CHECK(s.m_textStyleHandle == 0x2E3u);
}

namespace {
class RenderSettingsCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_RenderSettings m_captured;
  void addRenderSettings(const DRW_RenderSettings &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF RENDERENVIRONMENT decodes named fog fields (positional)",
          "[dxf][rendersettings][preservation]") {
  RenderSettingsCapture cap;
  // 90 classVersion, 290×3 (fogEnabled, fogBgEnabled, envImgEnabled),
  // 280×3 RC fogColor, 40×4 fog densities/distances, 1 filename.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nRENDERENVIRONMENT\n5\n2F0\n330\nC\n100\nAcDbRenderEnvironment\n"
      "90\n1\n290\n1\n290\n0\n290\n1\n280\n10\n280\n20\n280\n30\n"
      "40\n0.1\n40\n0.9\n40\n5.0\n40\n50.0\n1\nbg.hdr\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_renderenv_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_RenderSettings &r = cap.m_captured;
  CHECK(r.m_kind == DRW_RenderSettings::Environment);
  CHECK(r.m_classVersion == 1);
  CHECK(r.m_fogEnabled == true);
  CHECK(r.m_fogBackgroundEnabled == false);
  CHECK(r.m_environmentImageEnabled == true);
  CHECK(r.m_fogColorR == 10);
  CHECK(r.m_fogColorG == 20);
  CHECK(r.m_fogColorB == 30);
  CHECK(r.m_fogDensityNear == 0.1);
  CHECK(r.m_fogDensityFar == 0.9);
  CHECK(r.m_fogDistanceNear == 5.0);
  CHECK(r.m_fogDistanceFar == 50.0);
  CHECK(r.m_name == "bg.hdr");
}

TEST_CASE("DXF RENDERGLOBAL + RENDERSETTINGS capture class version + vectors",
          "[dxf][rendersettings][preservation]") {
  {
    RenderSettingsCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nRENDERGLOBAL\n5\n2F1\n330\nC\n100\nAcDbRenderGlobal\n"
        "90\n1\n90\n2\n90\n3\n290\n1\n290\n0\n1\nout.png\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_renderglobal_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_RenderSettings::Global);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_procedure == 2);
    CHECK(cap.m_captured.m_destination == 3);
    CHECK(cap.m_captured.m_name == "out.png");
  }
  {
    RenderSettingsCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nRENDERSETTINGS\n5\n2F2\n330\nC\n100\nAcDbRenderSettings\n"
        "90\n1\n1\npreset\n70\n2\n40\n12.5\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_rendersettings_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_RenderSettings::Settings);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_name == "preset");
    REQUIRE(cap.m_captured.m_doubles.size() == 1);
    CHECK(cap.m_captured.m_doubles[0] == 12.5);
  }
}

namespace {
class SectionCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Section m_captured;
  void addSection(const DRW_Section &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF SECTIONMANAGER is read into a DRW_Section (live + handles)",
          "[dxf][section][preservation]") {
  SectionCapture cap;
  // 330 before the 100 marker is the owner; 330s after are section handles.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSECTIONMANAGER\n5\n300\n330\nC\n100\nAcDbSectionManager\n"
      "70\n1\n90\n2\n330\n3A0\n330\n3A1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sectionmanager_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Section &s = cap.m_captured;
  CHECK(s.m_kind == DRW_Section::Manager);
  CHECK(s.m_isLive == true);
  CHECK(s.m_sectionCount == 2);
  REQUIRE(s.m_sectionHandles.size() == 2);
  CHECK(s.m_sectionHandles[0] == 0x3A0u);
  CHECK(s.m_sectionHandles[1] == 0x3A1u);
}

TEST_CASE("DXF SECTIONSETTINGS is read into a DRW_Section (type triple)",
          "[dxf][section][preservation]") {
  SectionCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSECTIONSETTINGS\n5\n301\n330\nC\n100\nAcDbSectionSettings\n"
      "90\n1\n91\n2\n90\n4\n331\n3B0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sectionsettings_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Section &s = cap.m_captured;
  CHECK(s.m_kind == DRW_Section::Settings);
  CHECK(s.m_classVersion == 1);
  CHECK(s.m_sectionType == 2);
  CHECK(s.m_generationOptions == 4);
  CHECK(s.m_destinationBlockHandle == 0x3B0u);
}
