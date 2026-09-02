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

  DRW_NavisworksModel model;
  model.handle = 0x10F;
  model.parentHandle = 0x1F;
  model.flags = 0x8001;
  model.definitionHandle = 0x110;
  for (std::size_t i = 0; i < model.transform.size(); ++i)
    model.transform[i] = static_cast<double>(i + 1);
  model.unitFactor = 0.001;
  filter.addNavisworksModel(&model);

  DRW_PointCloudDef pcd;
  pcd.handle = 0x103;
  pcd.parentHandle = 1;
  pcd.m_sourceFilename = "src.rcs";
  filter.addPointCloudDef(pcd);

  DRW_NavisworksModelDef navisworks;
  navisworks.handle = 0x10D;
  navisworks.parentHandle = 1;
  navisworks.m_path = "coordination/model.nwd";
  navisworks.m_flags = 5;
  navisworks.m_status = true;
  filter.addNavisworksModelDef(navisworks);

  DRW_PointCloudColorMap colorMap;
  colorMap.handle = 0x10E;
  colorMap.parentHandle = 1;
  colorMap.m_classVersion = 2;
  colorMap.m_defaultIntensityColorScheme = "intensity";
  colorMap.m_defaultElevationColorScheme = "elevation";
  colorMap.m_defaultClassificationColorScheme = "classification";
  colorMap.m_colorRampCount = 1;
  colorMap.m_colorRamps.push_back({3, 2, {"red", "blue"}});
  colorMap.m_classificationColorRampCount = 1;
  colorMap.m_classificationColorRamps.push_back({4, 1, {"green"}});
  filter.addPointCloudColorMap(colorMap);

  DRW_Background bg;
  bg.handle = 0x104;
  bg.parentHandle = 0x20;
  bg.m_kind = DRW_Background::Image;
  bg.m_classVersion = 7;
  bg.m_solidColor = 0x123456;
  bg.m_colorTop = 11;
  bg.m_horizon = 1.25;
  bg.m_fileName = "background.png";
  bg.m_fitToScreen = true;
  bg.m_maintainAspect = true;
  bg.m_offset = {2.0, 3.0, 0.0};
  bg.m_scale = {4.0, 5.0, 0.0};
  filter.addBackground(bg);

  DRW_Material mat;
  mat.handle = 0x105;
  mat.parentHandle = 0x21;
  mat.m_name = "Steel";
  mat.m_description = "Brushed steel";
  filter.addMaterial(mat);

  DRW_RenderSettings rs;
  rs.handle = 0x106;
  rs.parentHandle = 0x22;
  rs.m_kind = DRW_RenderSettings::Environment;
  rs.m_classVersion = 8;
  rs.m_name = "Studio";
  rs.m_longs = {1, 2, 3};
  rs.m_strings = {"one", "two"};
  rs.m_bools = {true, false};
  rs.m_doubles = {0.5, 1.5};
  rs.m_fogEnabled = true;
  rs.m_fogColorG = 64;
  rs.m_procedure = 9;
  rs.m_destination = 10;
  filter.addRenderSettings(rs);

  DRW_SunStudy sun;
  sun.handle = 0x107;
  sun.parentHandle = 0x23;
  sun.m_classVersion = 9;
  sun.m_setupName = "Summer";
  sun.m_description = "South facade";
  sun.m_outputType = 2;
  sun.m_useSubset = true;
  sun.m_selectDatesFromCalendar = true;
  sun.m_startTime = 100;
  sun.m_endTime = 200;
  sun.m_interval = 15;
  sun.m_spacing = 0.75;
  sun.m_viewHandle = 0x301;
  sun.m_visualStyleHandle = 0x302;
  sun.m_textStyleHandle = 0x303;
  filter.addSunStudy(sun);

  DRW_DbColor color;
  color.handle = 0x108;
  color.parentHandle = 0x24;
  color.name = "SteelColor";
  color.rgb = 0x102030;
  color.colorIndex = 42;
  color.colorMethod = 0xC2;
  color.bookName = "Company Colors";
  filter.addDbColor(color);

  DRW_DimensionAssociation dim;
  dim.handle = 0x109;
  dim.parentHandle = 0x25;
  dim.m_dimensionHandle = 0x401;
  dim.m_associativityFlags = 0x55;
  dim.m_isTransSpace = true;
  dim.m_rotatedDimensionType = 3;
  dim.m_osnapRefs.push_back({"AcDbLine", 4, 0x402});
  filter.addDimensionAssociation(dim);

  DRW_EvaluationGraph eval;
  eval.handle = 0x10A;
  eval.parentHandle = 0x26;
  eval.m_value96 = 96;
  eval.m_value97 = 97;
  eval.m_nodes.push_back({1, 2, 3, 0x501, 4, 5, 6, 7});
  eval.m_edges.push_back({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  filter.addEvaluationGraph(eval);

  DRW_Section sec;
  sec.handle = 0x10B;
  sec.parentHandle = 0x27;
  sec.m_kind = DRW_Section::Settings;
  sec.m_isLive = true;
  sec.m_sectionCount = 2;
  sec.m_sectionHandles = {0x601, 0x602};
  sec.m_classVersion = 12;
  sec.m_sectionType = 13;
  sec.m_generationOptions = 14;
  sec.m_destinationBlockHandle = 0x603;
  filter.addSection(sec);

  DRW_SectionObject so;
  so.handle = 0x10C;
  filter.addSectionObject(so);

  auto &meta = graphic.dwgAdvancedMetadata();
  REQUIRE(meta.familyExposureCount("POINTCLOUD") == 1);
  REQUIRE(meta.familyExposureCount("POINTCLOUDEX") == 1);
  REQUIRE(meta.familyExposureCount("NAVISWORKSMODEL") == 1);
  REQUIRE(meta.familyExposureCount("POINTCLOUDDEF") == 1);
  REQUIRE(meta.familyExposureCount("NAVISWORKSMODELDEF") == 1);
  REQUIRE(meta.familyExposureCount("POINTCLOUDCOLORMAP") == 1);
  REQUIRE(meta.pointCloudDefinitions().size() == 1);
  CHECK(meta.pointCloudDefinitions()[0].sourceFilename == "src.rcs");
  REQUIRE(meta.navisworksModelDefinitions().size() == 1);
  CHECK(meta.navisworksModelDefinitions()[0].path
        == "coordination/model.nwd");
  REQUIRE(meta.navisworksModels().size() == 1);
  const auto& storedModel = meta.navisworksModels()[0];
  CHECK(storedModel.handle == model.handle);
  CHECK(storedModel.parentHandle == model.parentHandle);
  CHECK(storedModel.flags == model.flags);
  CHECK(storedModel.definitionHandle == model.definitionHandle);
  CHECK(storedModel.transform == model.transform);
  CHECK(storedModel.unitFactor == model.unitFactor);
  REQUIRE(meta.pointCloudColorMaps().size() == 1);
  const auto& storedColorMap = meta.pointCloudColorMaps()[0];
  CHECK(storedColorMap.defaultIntensityColorScheme == "intensity");
  REQUIRE(storedColorMap.colorRamps.size() == 1);
  CHECK(storedColorMap.colorRamps[0].declaredColorCount == 2);
  CHECK(storedColorMap.colorRamps[0].colorSchemes
        == std::vector<std::string>{"red", "blue"});
  REQUIRE(storedColorMap.classificationColorRamps.size() == 1);
  CHECK(storedColorMap.classificationColorRamps[0].declaredColorCount == 1);
  REQUIRE(meta.familyExposureCount("BACKGROUND") == 1);
  REQUIRE(meta.familyExposureCount("MATERIAL") == 1);
  REQUIRE(meta.familyExposureCount("RENDERSETTINGS") == 1);
  REQUIRE(meta.familyExposureCount("SUNSTUDY") == 1);
  REQUIRE(meta.familyExposureCount("DBCOLOR") == 1);
  REQUIRE(meta.familyExposureCount("DIMASSOC") == 1);
  REQUIRE(meta.familyExposureCount("EVALUATION_GRAPH") == 1);
  REQUIRE(meta.familyExposureCount("SECTION") == 1);
  REQUIRE(meta.familyExposureCount("SECTIONOBJECT") == 1);
  REQUIRE(meta.backgrounds().size() == 1);
  CHECK(meta.backgrounds()[0].kind == DRW_Background::Image);
  CHECK(meta.backgrounds()[0].fileName == "background.png");
  CHECK(meta.backgrounds()[0].offset.x == 2.0);
  CHECK(meta.backgrounds()[0].scale.y == 5.0);
  REQUIRE(meta.materials().size() == 1);
  CHECK(meta.materials()[0].description == "Brushed steel");
  REQUIRE(meta.renderSettings().size() == 1);
  CHECK(meta.renderSettings()[0].kind == DRW_RenderSettings::Environment);
  CHECK(meta.renderSettings()[0].longs == std::vector<std::int32_t>{1, 2, 3});
  CHECK(meta.renderSettings()[0].fogEnabled);
  CHECK(meta.renderSettings()[0].fogColorG == 64);
  CHECK(meta.renderSettings()[0].destination == 10);
  REQUIRE(meta.sunStudies().size() == 1);
  CHECK(meta.sunStudies()[0].setupName == "Summer");
  CHECK(meta.sunStudies()[0].selectDatesFromCalendar);
  CHECK(meta.sunStudies()[0].viewHandle == 0x301);
  REQUIRE(meta.dbColors().size() == 1);
  CHECK(meta.dbColors()[0].name == "SteelColor");
  CHECK(meta.dbColors()[0].rgb == 0x102030);
  CHECK(meta.dbColors()[0].bookName == "Company Colors");
  REQUIRE(meta.dimensionAssociations().size() == 1);
  CHECK(meta.dimensionAssociations()[0].dimensionHandle == 0x401);
  REQUIRE(meta.dimensionAssociations()[0].osnapRefs.size() == 1);
  CHECK(meta.dimensionAssociations()[0].osnapRefs[0].m_className == "AcDbLine");
  REQUIRE(meta.evaluationGraphs().size() == 1);
  CHECK(meta.evaluationGraphs()[0].value97 == 97);
  REQUIRE(meta.evaluationGraphs()[0].nodes.size() == 1);
  CHECK(meta.evaluationGraphs()[0].nodes[0].m_expressionHandle == 0x501);
  REQUIRE(meta.evaluationGraphs()[0].edges.size() == 1);
  CHECK(meta.evaluationGraphs()[0].edges[0].m_value92e == 10);
  REQUIRE(meta.sections().size() == 1);
  CHECK(meta.sections()[0].kind == DRW_Section::Settings);
  CHECK(meta.sections()[0].sectionHandles == std::vector<std::uint32_t>{0x601, 0x602});
  CHECK(meta.sections()[0].destinationBlockHandle == 0x603);

  meta.invalidateByHandle(bg.handle);
  CHECK(meta.backgrounds()[0].replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);

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

TEST_CASE("RS_FilterDXFRW dictionary callbacks preserve count diagnostics",
          "[cross-read][exposure][dictionary][filter]") {
  ensureQtContext();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  RsFilterDxfRwExposureTestAccess::bindGraphic(filter, graphic);

  DRW_Dictionary dictionary;
  dictionary.handle = 0x200;
  dictionary.m_entries.push_back({"VALID", 0x201});
  dictionary.countCap = DRW_DictionaryCountCap{
      2u, 1u, 128u, 4u, 32u};
  filter.addDictionary(dictionary);

  REQUIRE(graphic.dwgAdvancedMetadata().dictionaries().size() == 1u);
  const auto &stored = graphic.dwgAdvancedMetadata().dictionaries().front();
  REQUIRE(stored.countCap.has_value());
  CHECK(stored.countCap->declaredCount == 2u);
  CHECK(stored.countCap->clampedCount == 1u);
  CHECK(stored.countCap->bitOffset == 128u);
  CHECK(stored.countCap->remainingBytes == 4u);
  CHECK(stored.countCap->remainingBits == 32u);

  DRW_DictionaryWithDefault dictionaryWithDefault;
  dictionaryWithDefault.handle = 0x210;
  dictionaryWithDefault.m_defaultEntryHandle = 0x211;
  dictionaryWithDefault.countCap = DRW_DictionaryCountCap{
      3u, 2u, 256u, 8u, 64u};
  filter.addDictionaryWithDefault(dictionaryWithDefault);

  REQUIRE(graphic.dwgAdvancedMetadata().dictionariesWithDefault().size()
          == 1u);
  const auto &storedWithDefault =
      graphic.dwgAdvancedMetadata().dictionariesWithDefault().front();
  REQUIRE(storedWithDefault.countCap.has_value());
  CHECK(storedWithDefault.countCap->declaredCount == 3u);
  CHECK(storedWithDefault.countCap->clampedCount == 2u);
  CHECK(storedWithDefault.countCap->bitOffset == 256u);
  CHECK(storedWithDefault.countCap->remainingBytes == 8u);
  CHECK(storedWithDefault.countCap->remainingBits == 64u);
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

TEST_CASE("RS_FilterDXFRW retains modern typed callback carriers",
          "[cross-read][exposure][typed-callbacks][filter]") {
  ensureQtContext();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  RsFilterDxfRwExposureTestAccess::bindGraphic(filter, graphic);

  DRW_CsacDocumentOptions csac;
  csac.handle = 0x701;
  csac.parentHandle = 0x70;
  csac.classVersion = 3;
  csac.flags = 5;
  filter.addCsacDocumentOptions(csac);

  DRW_ContextDataManager context;
  context.handle = 0x702;
  context.parentHandle = 0x70;
  context.objectContextHandle = 0x703;
  context.subManagerCount = 1;
  context.subManagers.push_back({0x704, 1, {{"Annotative", 0x705}}});
  filter.addContextDataManager(context);

  DRW_ProxyEntity proxyEntity;
  proxyEntity.handle = 0x706;
  proxyEntity.parentHandle = 0x70;
  proxyEntity.m_hasProxyClassId = true;
  proxyEntity.m_proxyClassId = 42;
  proxyEntity.m_entityData = {0x01, 0x02, 0x03};
  proxyEntity.m_objectIdRefs.push_back({330, 2, 0x707, 0x707});
  filter.addProxyEntity(proxyEntity);

  DRW_VbaProject vba;
  vba.handle = 0x708;
  vba.parentHandle = 0x70;
  vba.m_data = {0x10, 0x20, 0x30, 0x40};
  vba.m_dataSize = static_cast<std::uint32_t>(vba.m_data.size());
  filter.addVbaProject(vba);

  DRW_ProxyObject proxyObject;
  proxyObject.handle = 0x709;
  proxyObject.parentHandle = 0x70;
  proxyObject.m_hasProxyCarrierId = true;
  proxyObject.m_proxyCarrierId = 9;
  proxyObject.m_objectData = {0xAA, 0xBB};
  proxyObject.m_objectIdRefs.push_back({340, 3, 0x70A, 0x70A});
  filter.addProxyObject(proxyObject);

  const auto &metadata = graphic.dwgAdvancedMetadata();
  REQUIRE(metadata.csacDocumentOptions().size() == 1);
  CHECK(metadata.csacDocumentOptions()[0].flags == 5);
  REQUIRE(metadata.contextDataManagers().size() == 1);
  REQUIRE(metadata.contextDataManagers()[0].subManagers.size() == 1);
  CHECK(metadata.contextDataManagers()[0].subManagers[0].entries[0].text
        == "Annotative");
  REQUIRE(metadata.proxyEntities().size() == 1);
  CHECK(metadata.proxyEntities()[0].m_entityData
        == std::vector<std::uint8_t>{0x01, 0x02, 0x03});
  REQUIRE(metadata.vbaProjects().size() == 1);
  CHECK(metadata.vbaProjects()[0].m_data.size() == 4);
  REQUIRE(metadata.proxyObjects().size() == 1);
  CHECK(metadata.proxyObjects()[0].m_objectData
        == std::vector<std::uint8_t>{0xAA, 0xBB});
  CHECK(metadata.familyExposureCount("CSACDOCUMENTOPTIONS") == 1);
  CHECK(metadata.familyExposureCount("CONTEXTDATAMANAGER") == 1);
  CHECK(metadata.familyExposureCount("ACAD_PROXY_ENTITY") == 1);
  CHECK(metadata.familyExposureCount("VBA_PROJECT") == 1);
  CHECK(metadata.familyExposureCount("ACAD_PROXY_OBJECT") == 1);

  int entityCount = 0;
  for (RS_Entity *entity = graphic.firstEntity(); entity != nullptr;
       entity = graphic.nextEntity()) {
    ++entityCount;
  }
  CHECK(entityCount == 0);
}
