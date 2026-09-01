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
**********************************************************************/

// Cross-read parity: metadata-only family exposure via REAL RS_FilterDXFRW
// entry points (PR-4a/4c + Navisworks). Not a reimplementation of counters.

#include <QCoreApplication>
#include <catch2/catch_test_macros.hpp>

#include "drw_datastorage.h"
#include "drw_entities.h"
#include "drw_objects.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_settings.h"

/// Test-only accessor: bind a live RS_Graphic to RS_FilterDXFRW::m_graphic so
/// add* overrides store into advanced metadata (same pattern as header tests).
class RsFilterDxfRwExposureTestAccess {
public:
  static void bindGraphic(RS_FilterDXFRW &filter, RS_Graphic &graphic) {
    filter.m_graphic = &graphic;
    filter.m_currentContainer = &graphic;
  }
};

namespace {

// RS_Graphic's ctor needs a QCoreApplication (same bootstrap as header tests).
void ensureQtContext() {
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp =
      QCoreApplication::instance()
          ? QCoreApplication::instance()
          : new QCoreApplication(qargc, qargv);
  (void)qapp;
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)settingsReady;
}

} // namespace

TEST_CASE("RS_FilterDXFRW point-cloud and PR-4c overrides store metadata-only",
          "[cross-read][exposure][filter]") {
  ensureQtContext();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  RsFilterDxfRwExposureTestAccess::bindGraphic(filter, graphic);

  DRW_PointCloud pc;
  pc.handle = 0x101;
  pc.parentHandle = 1;
  pc.savedFilename = "cloud.rcs";
  filter.addPointCloud(&pc);

  DRW_PointCloudEx pcex;
  pcex.handle = 0x102;
  pcex.parentHandle = 1;
  pcex.name = "ex";
  filter.addPointCloudEx(&pcex);

  DRW_PointCloudDef pcd;
  pcd.handle = 0x103;
  pcd.parentHandle = 1;
  pcd.m_sourceFilename = "src.rcs";
  filter.addPointCloudDef(pcd);

  DRW_Background bg;
  bg.handle = 0x104;
  filter.addBackground(bg);

  DRW_Material mat;
  mat.handle = 0x105;
  mat.m_name = "Steel";
  filter.addMaterial(mat);

  DRW_RenderSettings rs;
  rs.handle = 0x106;
  filter.addRenderSettings(rs);

  DRW_SunStudy sunStudy;
  sunStudy.handle = 0x107;
  filter.addSunStudy(sunStudy);

  DRW_DbColor color;
  color.handle = 0x108;
  filter.addDbColor(color);

  DRW_DimensionAssociation dim;
  dim.handle = 0x109;
  filter.addDimensionAssociation(dim);

  DRW_EvaluationGraph eval;
  eval.handle = 0x10A;
  filter.addEvaluationGraph(eval);

  DRW_Section sec;
  sec.handle = 0x10B;
  filter.addSection(sec);

  DRW_SectionObject so;
  so.handle = 0x10C;
  filter.addSectionObject(so);

  auto &meta = graphic.dwgAdvancedMetadata();
  REQUIRE(meta.familyExposureCount("POINTCLOUD") == 1);
  REQUIRE(meta.familyExposureCount("POINTCLOUDEX") == 1);
  REQUIRE(meta.familyExposureCount("POINTCLOUDDEF") == 1);
  REQUIRE(meta.familyExposureCount("BACKGROUND") == 1);
  REQUIRE(meta.familyExposureCount("MATERIAL") == 1);
  REQUIRE(meta.familyExposureCount("RENDERSETTINGS") == 1);
  REQUIRE(meta.familyExposureCount("SUNSTUDY") == 1);
  REQUIRE(meta.familyExposureCount("DBCOLOR") == 1);
  REQUIRE(meta.familyExposureCount("DIMASSOC") == 1);
  REQUIRE(meta.familyExposureCount("EVALUATION_GRAPH") == 1);
  REQUIRE(meta.familyExposureCount("SECTION") == 1);
  REQUIRE(meta.familyExposureCount("SECTIONOBJECT") == 1);

  // Metadata-only: no document geometry entities for these families.
  int entityCount = 0;
  for (RS_Entity *e = graphic.firstEntity(); e != nullptr;
       e = graphic.nextEntity()) {
    ++entityCount;
  }
  REQUIRE(entityCount == 0);
}

TEST_CASE("RS_FilterDXFRW::addDataStorage stores typed index in metadata",
          "[cross-read][exposure][datastorage][filter]") {
  ensureQtContext();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  RsFilterDxfRwExposureTestAccess::bindGraphic(filter, graphic);

  DRW_DataStorageSection section;
  section.m_name = "AcDb:AcDsPrototype_1b";
  DRW_DataStorageRecord rec;
  rec.handle = 0xABCDEF;
  rec.dataByteLength = 16;
  section.records.push_back(rec);
  filter.addDataStorage(section);

  auto &meta = graphic.dwgAdvancedMetadata();
  REQUIRE(meta.dataStorages().size() == 1);
  REQUIRE(meta.dataStorages().front().recordCount == 1);
  REQUIRE(meta.dataStorages().front().recordHandles.front() == 0xABCDEF);
}

TEST_CASE("RS_FilterDXFRW::addUnsupportedObject notes NAVISWORKS metadata",
          "[cross-read][exposure][navisworks][filter]") {
  ensureQtContext();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  RsFilterDxfRwExposureTestAccess::bindGraphic(filter, graphic);

  DRW_UnsupportedObject raw;
  raw.m_handle = 0x5150;
  raw.m_recordName = "NAVISWORKSMODEL";
  raw.m_className = "AcDbNavisworksModel";
  raw.m_isEntity = true;
  raw.m_isCustomClass = true;
  raw.m_rawBytes = {0x01, 0x02, 0x03};

  filter.addUnsupportedObject(raw);

  auto &meta = graphic.dwgAdvancedMetadata();
  REQUIRE(meta.familyExposureCount("NAVISWORKSMODEL") == 1);
  bool found = false;
  for (const auto &hit : meta.familyExposures()) {
    if (hit.family == "NAVISWORKSMODEL" && hit.handle == 0x5150) {
      found = true;
      REQUIRE(hit.detail.find("metadata-only") != std::string::npos);
    }
  }
  REQUIRE(found);

  // Def variant also classifies.
  DRW_UnsupportedObject def;
  def.m_handle = 0x5151;
  def.m_recordName = "NAVISWORKSMODELDEF";
  filter.addUnsupportedObject(def);
  REQUIRE(meta.familyExposureCount("NAVISWORKSMODELDEF") == 1);

  // Still no document geometry for Navisworks.
  int entityCount = 0;
  for (RS_Entity *e = graphic.firstEntity(); e != nullptr;
       e = graphic.nextEntity()) {
    ++entityCount;
  }
  REQUIRE(entityCount == 0);
}
