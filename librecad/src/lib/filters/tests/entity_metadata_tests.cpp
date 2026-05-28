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
 * Entity-metadata sidecar tests — covers transparency / material /
 * plotStyle / shadow / visualstyle round-trip preservation through
 * RS_Entity. The libdxfrw read+write paths (parseCode + writeEntity)
 * are tested via in-memory DXF round-trip below.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "lc_dwgadvancedmetadata.h"
#include "rs_line.h"
#include "rs_vector.h"

#include "drw_entities.h"
#include "drw_objects.h"
#include "libdxfrw.h"

TEST_CASE("RS_Entity: sidecar defaults are zero", "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  REQUIRE(line.materialHandle() == 0u);
  REQUIRE(line.plotStyleHandle() == 0u);
  REQUIRE(line.shadowMode() == 0);
  REQUIRE(line.fullVisualStyleHandle() == 0u);
  REQUIRE(line.faceVisualStyleHandle() == 0u);
  REQUIRE(line.edgeVisualStyleHandle() == 0u);
}

TEST_CASE("RS_Entity: material/plotStyle/shadow round-trip",
          "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});

  line.setMaterialHandle(0xABCDu);
  line.setPlotStyleHandle(0x1234u);
  line.setShadowMode(2);

  REQUIRE(line.materialHandle() == 0xABCDu);
  REQUIRE(line.plotStyleHandle() == 0x1234u);
  REQUIRE(line.shadowMode() == 2);
}

TEST_CASE("RS_Entity: visual-style handles round-trip", "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  line.setVisualStyleHandles(0xAAAAu, 0xBBBBu, 0xCCCCu);
  REQUIRE(line.fullVisualStyleHandle() == 0xAAAAu);
  REQUIRE(line.faceVisualStyleHandle() == 0xBBBBu);
  REQUIRE(line.edgeVisualStyleHandle() == 0xCCCCu);
}

TEST_CASE("RS_Entity: copy ctor preserves sidecars", "[entity_metadata]") {
  RS_Line src(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  src.setMaterialHandle(0x77u);
  src.setPlotStyleHandle(0x88u);
  src.setShadowMode(3);
  src.setVisualStyleHandles(0x11u, 0x22u, 0x33u);

  RS_Line copy(src);
  REQUIRE(copy.materialHandle() == 0x77u);
  REQUIRE(copy.plotStyleHandle() == 0x88u);
  REQUIRE(copy.shadowMode() == 3);
  REQUIRE(copy.fullVisualStyleHandle() == 0x11u);
  REQUIRE(copy.faceVisualStyleHandle() == 0x22u);
  REQUIRE(copy.edgeVisualStyleHandle() == 0x33u);

  // Mutating the source must not affect the copy.
  src.setMaterialHandle(0u);
  REQUIRE(src.materialHandle() == 0u);
  REQUIRE(copy.materialHandle() == 0x77u);
}

TEST_CASE("RS_Entity: setting sidecars to zero clears them",
          "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  line.setMaterialHandle(0xFFu);
  line.setMaterialHandle(0u);
  REQUIRE(line.materialHandle() == 0u);
}

TEST_CASE("DWG advanced metadata caches raw and semantic sidecars",
          "[entity_metadata][dwg_metadata]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject raw;
  raw.m_objectType = 501;
  raw.m_handle = 0x77u;
  raw.m_bodyBitSize = 128u;
  raw.m_objectOffset = 4096u;
  raw.m_objectSize = 32u;
  raw.m_isCustomClass = true;
  raw.m_recordName = "ACDBASSOCNETWORK";
  raw.m_className = "AcDbAssocNetwork";
  raw.m_rawBytes = {0x01u, 0x02u, 0x03u};
  metadata.addUnsupportedObject(raw);

  DRW_UnsupportedObject rawEntity;
  rawEntity.m_objectType = 502;
  rawEntity.m_handle = 0x78u;
  rawEntity.m_isEntity = true;
  rawEntity.m_isCustomClass = true;
  rawEntity.m_recordName = "RAW_ENTITY";
  rawEntity.m_className = "AcDbRawEntity";
  rawEntity.m_rawBytes = {0x04u};
  metadata.addUnsupportedObject(rawEntity);

  DRW_UnsupportedObject rawWithoutBytes;
  rawWithoutBytes.m_objectType = 79;
  rawWithoutBytes.m_handle = 0x79u;
  metadata.addUnsupportedObject(rawWithoutBytes);

  DRW_View view;
  view.name = "Camera A";
  view.handle = 0x90u;
  view.parentHandle = 0x8Fu;
  view.size = DRW_Coord{12.0, 6.0, 0.0};
  view.center = DRW_Coord{3.0, 4.0, 0.0};
  view.viewDirectionFromTarget = DRW_Coord{0.0, 0.0, 1.0};
  view.targetPoint = DRW_Coord{10.0, 11.0, 12.0};
  view.lensLen = 50.0;
  view.frontClippingPlaneOffset = 0.5;
  view.backClippingPlaneOffset = 500.0;
  view.twistAngle = 0.125;
  view.viewMode = 7;
  view.renderMode = 4u;
  view.hasUCS = true;
  view.cameraPlottable = true;
  view.ucsOrigin = DRW_Coord{1.0, 2.0, 3.0};
  view.ucsXAxis = DRW_Coord{1.0, 0.0, 0.0};
  view.ucsYAxis = DRW_Coord{0.0, 1.0, 0.0};
  view.ucsOrthoType = 2;
  view.ucsElevation = 14.0;
  view.namedUCS_ID = 0x91u;
  view.baseUCS_ID = 0x92u;
  view.m_backgroundHandle = 0x93u;
  view.m_visualStyleHandle = 0x94u;
  view.m_sunHandle = 0x9Au;
  view.m_liveSectionHandle = 0x96u;
  view.m_useDefaultLights = false;
  view.m_defaultLightingType = 2u;
  view.m_brightness = 0.25;
  view.m_contrast = 0.75;
  view.m_ambientColor = 7u;
  metadata.addView(view);

  DRW_Light light;
  light.handle = 0x98u;
  light.parentHandle = 0x99u;
  light.m_classVersion = 1u;
  light.m_name = "Spot A";
  light.m_type = 3u;
  light.m_status = true;
  light.m_color = 0x112233u;
  light.m_plotGlyph = true;
  light.m_intensity = 4.5;
  light.m_position = DRW_Coord{1.0, 2.0, 3.0};
  light.m_target = DRW_Coord{4.0, 5.0, 6.0};
  light.m_attenuationType = 2u;
  light.m_useAttenuationLimits = true;
  light.m_attenuationStartLimit = 7.0;
  light.m_attenuationEndLimit = 8.0;
  light.m_hotspotAngle = 0.25;
  light.m_falloffAngle = 0.5;
  light.m_castShadows = true;
  light.m_shadowType = 2u;
  light.m_shadowMapSize = 256u;
  light.m_shadowMapSoftness = 3u;
  light.m_hasPhotometricData = true;
  light.m_hasWebFile = true;
  light.m_webFile = "lamp.ies";
  light.m_physicalIntensityMethod = 1u;
  light.m_physicalIntensity = 9.0;
  light.m_illuminanceDistance = 10.0;
  light.m_lampColorType = 2u;
  light.m_lampColorTemperature = 6500.0;
  light.m_lampColorPreset = 5u;
  light.m_webRotation = DRW_Coord{0.0, 1.0, 0.0};
  light.m_extendedLightShape = 4u;
  light.m_extendedLightLength = 11.0;
  light.m_extendedLightWidth = 12.0;
  light.m_extendedLightRadius = 13.0;
  metadata.addLight(light);

  DRW_Sun sun;
  sun.handle = 0x9Au;
  sun.parentHandle = 0x9Bu;
  sun.m_classVersion = 2u;
  sun.m_isOn = true;
  sun.m_color = 0x445566u;
  sun.m_intensity = 1.25;
  sun.m_hasShadow = true;
  sun.m_julianDay = 2460000;
  sun.m_milliseconds = 43200000;
  sun.m_isDaylightSavings = true;
  sun.m_shadowType = 1u;
  sun.m_shadowMapSize = 512u;
  sun.m_shadowSoftness = 6u;
  metadata.addSun(sun);

  DRW_MLeaderStyle style;
  style.handle = 0xA0u;
  style.parentHandle = 0xA1u;
  style.name = "CalloutStyle";
  style.styleVersion = 2u;
  style.contentType = 2u;
  style.drawMLeaderOrder = 1u;
  style.drawLeaderOrder = 2u;
  style.maxLeaderPoints = 9;
  style.firstSegmentAngle = 0.25;
  style.secondSegmentAngle = 0.5;
  style.leaderType = 1u;
  style.leaderColor = 3;
  style.leaderLineTypeHandle.ref = 0xA2u;
  style.leaderLineWeight = 29;
  style.landingEnabled = true;
  style.landingGap = 0.125;
  style.autoIncludeLanding = true;
  style.landingDistance = 2.25;
  style.description = "Callout description";
  style.arrowHeadBlockHandle.ref = 0xA3u;
  style.textStyleHandle.ref = 0xA4u;
  style.blockHandle.ref = 0xA5u;
  style.arrowHeadSize = 0.75;
  style.textDefault = "Default text";
  style.leftAttachment = 1u;
  style.rightAttachment = 2u;
  style.textAngleType = 3u;
  style.textAlignmentType = 4u;
  style.textColor = 5;
  style.textHeight = 2.5;
  style.textFrameEnabled = true;
  style.alwaysAlignTextLeft = true;
  style.alignSpace = 0.2;
  style.blockColor = 6;
  style.blockScale = DRW_Coord{1.0, 2.0, 3.0};
  style.blockScaleEnabled = true;
  style.blockRotation = 0.75;
  style.blockRotationEnabled = true;
  style.blockConnectionType = 7u;
  style.scaleFactor = 3.0;
  style.propertyChanged = true;
  style.isAnnotative = true;
  style.breakSize = 0.375;
  style.attachmentDirection = 8u;
  style.topAttachment = 9u;
  style.bottomAttachment = 10u;
  style.textExtended = true;
  metadata.addMLeaderStyle(style);

  DRW_MLeader mleader;
  mleader.handle = 0xA6u;
  mleader.parentHandle = 0xA7u;
  mleader.classVersion = 2u;
  mleader.styleHandle.ref = 0xA0u;
  mleader.overrideFlags = 0x1234;
  mleader.leaderType = 2u;
  mleader.styleContentType = 2u;
  mleader.leaderLineTypeHandle.ref = 0xA8u;
  mleader.arrowHeadHandle.ref = 0xA9u;
  mleader.styleTextStyleHandle.ref = 0xAAu;
  mleader.styleBlockHandle.ref = 0xABu;
  mleader.landingDistance = 1.75;
  mleader.defaultArrowHeadSize = 0.625;
  mleader.landingEnabled = true;
  mleader.doglegEnabled = false;
  mleader.isAnnotative = true;
  mleader.context.overallScale = 2.0;
  mleader.context.textHeight = 3.5;
  mleader.context.hasTextContents = true;
  mleader.context.textLabel = "Leader note";
  mleader.context.hasContentsBlock = true;
  mleader.context.columnSizes = {4.0, 5.0};
  DRW_MLeaderLeaderLine leaderLine;
  leaderLine.points = {DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 1.0, 0.0}};
  leaderLine.breaks.push_back({DRW_Coord{0.25, 0.25, 0.0},
                               DRW_Coord{0.5, 0.5, 0.0}});
  DRW_MLeaderRoot leaderRoot;
  leaderRoot.breaks.push_back({DRW_Coord{0.0, 0.0, 0.0},
                               DRW_Coord{0.1, 0.1, 0.0}});
  leaderRoot.leaderLines.push_back(leaderLine);
  mleader.context.roots.push_back(leaderRoot);
  DRW_MLeader::ArrowHeadEntry arrowEntry;
  arrowEntry.handle.ref = 0xACu;
  mleader.arrowHeads.push_back(arrowEntry);
  DRW_MLeader::BlockLabelEntry blockLabel;
  blockLabel.attDefHandle.ref = 0xADu;
  blockLabel.labelText = "TAG";
  mleader.blockLabels.push_back(blockLabel);
  metadata.addMLeader(mleader);

  DRW_DetailViewStyle detailStyle;
  detailStyle.handle = 0xB0u;
  detailStyle.parentHandle = 0xB1u;
  detailStyle.name = "DetailStyle";
  detailStyle.m_modelDoc.m_description = "detail description";
  detailStyle.m_modelDoc.m_displayName = "Detail Display";
  detailStyle.m_classVersion = 3u;
  detailStyle.m_flags = 0x12u;
  detailStyle.m_identifierStyleHandle = 0xB2u;
  detailStyle.m_arrowSymbolHandle = 0xB3u;
  detailStyle.m_viewLabelTextStyleHandle = 0xB4u;
  detailStyle.m_boundaryLineTypeHandle = 0xB5u;
  detailStyle.m_connectionLineTypeHandle = 0xB6u;
  detailStyle.m_borderLineTypeHandle = 0xB7u;
  detailStyle.m_viewLabelPattern = "DETAIL %s";
  detailStyle.m_identifierHeight = 1.25;
  detailStyle.m_arrowSymbolSize = 0.5;
  detailStyle.m_viewLabelTextHeight = 2.0;
  metadata.addDetailViewStyle(detailStyle);

  DRW_SectionViewStyle sectionStyle;
  sectionStyle.handle = 0xC0u;
  sectionStyle.parentHandle = 0xC1u;
  sectionStyle.name = "SectionStyle";
  sectionStyle.m_modelDoc.m_description = "section description";
  sectionStyle.m_modelDoc.m_displayName = "Section Display";
  sectionStyle.m_classVersion = 4u;
  sectionStyle.m_flags = 0x34u;
  sectionStyle.m_identifierStyleHandle = 0xC2u;
  sectionStyle.m_arrowStartSymbolHandle = 0xC3u;
  sectionStyle.m_arrowEndSymbolHandle = 0xC4u;
  sectionStyle.m_planeLineTypeHandle = 0xC5u;
  sectionStyle.m_bendLineTypeHandle = 0xC6u;
  sectionStyle.m_viewLabelTextStyleHandle = 0xC7u;
  sectionStyle.m_viewLabelPattern = "SECTION %s";
  sectionStyle.m_hatchPattern = "ANSI31";
  sectionStyle.m_identifierHeight = 1.5;
  sectionStyle.m_arrowSymbolSize = 0.75;
  sectionStyle.m_viewLabelTextHeight = 2.25;
  sectionStyle.m_hatchScale = 0.5;
  sectionStyle.m_hatchAngles = {0.0, 1.5707963267948966};
  metadata.addSectionViewStyle(sectionStyle);

  DRW_BreakData breakData;
  breakData.handle = 0xD0u;
  breakData.parentHandle = 0xD1u;
  breakData.m_dimensionHandle = 0xD2u;
  breakData.m_pointRefHandles = {0xD3u, 0xD4u};
  metadata.addBreakData(breakData);

  DRW_BreakPointRef breakPointRef;
  breakPointRef.handle = 0xD5u;
  breakPointRef.parentHandle = 0xD6u;
  metadata.addBreakPointRef(breakPointRef);

  DRW_Group group;
  group.handle = 0xD7u;
  group.parentHandle = 0xD8u;
  group.m_description = "fixture group";
  group.m_isUnnamed = true;
  group.m_selectable = false;
  group.m_entityHandles = {0xD9u, 0xDAu};
  metadata.addGroup(group);

  DRW_ImageDefinitionReactor reactor;
  reactor.handle = 0xDBu;
  reactor.parentHandle = 0xDCu;
  reactor.m_classVersion = 7;
  metadata.addImageDefinitionReactor(reactor);

  DRW_SpatialFilter spatialFilter;
  spatialFilter.handle = 0xDDu;
  spatialFilter.parentHandle = 0xDEu;
  spatialFilter.m_boundaryPoints = {
      DRW_Coord{0.0, 0.0, 0.0},
      DRW_Coord{1.0, 0.0, 0.0},
      DRW_Coord{1.0, 1.0, 0.0}};
  spatialFilter.m_displayBoundary = true;
  spatialFilter.m_clipFrontPlane = true;
  spatialFilter.m_frontDistance = 2.5;
  metadata.addSpatialFilter(spatialFilter);

  DRW_GeoData geoData;
  geoData.handle = 0xDFu;
  geoData.parentHandle = 0xE0u;
  geoData.m_hostBlockHandle = 0xE1u;
  geoData.m_version = 2;
  geoData.m_coordinatesType = 1;
  geoData.m_horizontalUnits = 6;
  geoData.m_verticalUnits = 6;
  geoData.m_horizontalUnitScale = 0.3048;
  geoData.m_verticalUnitScale = 1.0;
  geoData.m_coordinateSystemDefinition = "EPSG:3857";
  geoData.m_geoRssTag = "rss";
  geoData.m_points.push_back({DRW_Coord{0.0, 0.0, 0.0},
                              DRW_Coord{1.0, 1.0, 0.0}});
  geoData.m_faces.push_back({1, 2, 3});
  metadata.addGeoData(geoData);

  DRW_TableGeometry tableGeometry;
  tableGeometry.handle = 0xE2u;
  tableGeometry.parentHandle = 0xE3u;
  tableGeometry.m_rowCount = 2;
  tableGeometry.m_columnCount = 3;
  tableGeometry.m_cellCount = 6;
  DRW_TableGeometryCell geometryCell;
  geometryCell.m_contents.resize(2);
  tableGeometry.m_cells.push_back(geometryCell);
  metadata.addTableGeometry(tableGeometry);

  DRW_AcDbPlaceholder placeholder;
  placeholder.handle = 0xE4u;
  placeholder.parentHandle = 0xE5u;
  metadata.addAcDbPlaceholder(placeholder);

  DRW_TableStyle tableStyle;
  tableStyle.handle = 0xFCu;
  tableStyle.parentHandle = 0xFDu;
  tableStyle.m_name = "SemanticTableStyle";
  tableStyle.m_titleSuppressed = true;
  tableStyle.m_headerSuppressed = false;
  tableStyle.m_tableCellStyle.m_borders.resize(1);
  tableStyle.m_tableCellStyle.m_contentFormat.m_textStyleHandle = 0x108u;
  tableStyle.m_tableCellStyle.m_borders[0].m_lineTypeHandle = 0x109u;
  DRW_TableStyleRowStyle rowStyle;
  rowStyle.m_borders.resize(2);
  rowStyle.m_textStyleHandle = 0x10Au;
  rowStyle.m_borders[0].m_lineTypeHandle = 0x10Bu;
  tableStyle.m_rowStyles.push_back(rowStyle);
  DRW_TableStyleCellStyle cellStyle;
  cellStyle.m_borders.resize(3);
  cellStyle.m_contentFormat.m_textStyleHandle = 0x10Cu;
  cellStyle.m_borders[0].m_lineTypeHandle = 0x10Du;
  tableStyle.m_cellStyles.push_back(cellStyle);
  metadata.addTableStyle(tableStyle);

  DRW_TableContentObject tableContent;
  tableContent.handle = 0xFEu;
  tableContent.parentHandle = 0xFFu;
  tableContent.m_parseComplete = true;
  tableContent.m_content.m_tableStyleHandle = 0xFCu;
  tableContent.m_content.m_columns.resize(2);
  tableContent.m_content.m_columns[0].m_width = 12.5;
  tableContent.m_content.m_columns[1].m_width = 15.0;
  tableContent.m_content.m_fieldHandles = {0x100u, 0x101u};
  tableContent.m_content.m_mergedRanges.push_back({0, 0, 0, 1});
  DRW_TableRow semanticRow;
  semanticRow.m_height = 4.25;
  semanticRow.m_cells.resize(2);
  semanticRow.m_cells[0].m_overrideFlags = 0x1u;
  semanticRow.m_cells[0].m_valueHandle = 0x102u;
  semanticRow.m_cells[0].m_geometryHandle = 0x103u;
  semanticRow.m_cells[0].m_styleId = 9;
  semanticRow.m_cells[0].m_attributes.push_back({0x104u, 1, "A1"});
  DRW_TableCellContent textContent;
  textContent.m_type = 1;
  textContent.m_text = "Cell text";
  semanticRow.m_cells[0].m_contents.push_back(textContent);
  semanticRow.m_cells[1].m_blockHandle = 0x105u;
  DRW_TableCellContent blockContent;
  blockContent.m_type = 4;
  blockContent.m_handle = 0x106u;
  semanticRow.m_cells[1].m_contents.push_back(blockContent);
  DRW_TableCellContent fieldContent;
  fieldContent.m_type = 2;
  fieldContent.m_handle = 0x107u;
  semanticRow.m_cells[1].m_contents.push_back(fieldContent);
  tableContent.m_content.m_rows.push_back(semanticRow);
  metadata.addTableContent(tableContent);

  DRW_ModelerGeometry modelerGeometry(DRW::E3DSOLID);
  modelerGeometry.handle = 0xF9u;
  modelerGeometry.parentHandle = 0xFAu;
  modelerGeometry.m_modelerVersion = 0x21u;
  modelerGeometry.m_bodyBitSize = 456u;
  modelerGeometry.m_objectSize = 64u;
  modelerGeometry.m_isEmpty = false;
  modelerGeometry.m_hasModelerData = true;
  modelerGeometry.m_modelerDataUnknownBit = true;
  modelerGeometry.m_hasWireframe = true;
  modelerGeometry.m_historyHandle = 0xFBu;
  modelerGeometry.m_rawBytes = {'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n',
                                'a', 'r', 'y', 'F', 'i', 'l', 'e'};
  metadata.addModelerGeometry(modelerGeometry);

  DRW_AssociativeObject associativeObject("ACDBASSOCVERTEXACTIONPARAM");
  associativeObject.handle = 0xE6u;
  associativeObject.parentHandle = 0xE7u;
  associativeObject.m_classVersion = 3u;
  associativeObject.m_geometryStatus = 4;
  associativeObject.m_owningNetworkHandle = 0xE8u;
  associativeObject.m_actionBodyHandle = 0xE9u;
  associativeObject.m_actionIndex = 11;
  associativeObject.m_maxDependencyIndex = 12;
  associativeObject.m_dependencies = {{true, 0xEAu}, {false, 0xEBu}};
  associativeObject.m_actions = {{true, 0xECu}};
  associativeObject.m_ownedParams = {0xEDu, 0xEEu};
  associativeObject.m_ownedActions = {0xEFu};
  associativeObject.m_dependencyHandle = 0xF0u;
  associativeObject.m_readDependencyHandle = 0xF1u;
  associativeObject.m_writeDependencyHandle = 0xF2u;
  associativeObject.m_rNodeHandle = 0xF3u;
  associativeObject.m_dNodeHandle = 0xF4u;
  associativeObject.m_status = 5;
  associativeObject.m_osnapMode = 6u;
  associativeObject.m_parameter = 0.875;
  associativeObject.m_point = DRW_Coord{7.0, 8.0, 9.0};
  metadata.addAssociativeObject(associativeObject);

  DRW_AcShHistoryObject acshObject("ACSH_SWEEP_CLASS");
  acshObject.handle = 0xF5u;
  acshObject.parentHandle = 0xF6u;
  acshObject.m_major = 24u;
  acshObject.m_minor = 3u;
  acshObject.m_ownerHandle = 0xF7u;
  acshObject.m_historyNodeId = 0xF8u;
  acshObject.m_showHistory = true;
  acshObject.m_recordHistory = true;
  acshObject.m_direction = DRW_Coord{0.0, 0.0, 1.0};
  acshObject.m_draftAngle = 0.125;
  acshObject.m_startDraftDistance = 1.25;
  acshObject.m_endDraftDistance = 2.5;
  acshObject.m_scaleFactor = 1.5;
  acshObject.m_twistAngle = 0.25;
  acshObject.m_alignAngle = 0.5;
  acshObject.m_binaryBlob1 = {0x01u, 0x02u};
  acshObject.m_binaryBlob2 = {0x03u, 0x04u, 0x05u};
  metadata.addAcShObject(acshObject);

  REQUIRE(metadata.rawObjects().size() == 3);
  CHECK(metadata.rawObjects().front().handle == 0x77u);
  CHECK(metadata.rawObjects().front().bodyBitSize == 128u);
  CHECK(metadata.rawObjects().front().family ==
        LC_DwgAdvancedMetadata::RawObjectFamily::Associative);
  CHECK(metadata.rawObjects().front().rawBytes.size() == 3);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[0]) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::None);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[1]) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::EntityReplayUnsupported);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[2]) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::MissingRawBytes);
  CHECK(metadata.hasBlockedRawReplay());
  CHECK(std::string(LC_DwgAdvancedMetadata::replayBlockerName(
            LC_DwgAdvancedMetadata::ReplayBlocker::EntityReplayUnsupported)) ==
        "entity replay unsupported");

  const auto* foundView = metadata.findViewByName("Camera A");
  REQUIRE(foundView != nullptr);
  CHECK(foundView->handle == 0x90u);
  CHECK(foundView->parentHandle == 0x8Fu);
  CHECK(foundView->size.x == 12.0);
  CHECK(foundView->center.y == 4.0);
  CHECK(foundView->viewDirectionFromTarget.z == 1.0);
  CHECK(foundView->targetPoint.z == 12.0);
  CHECK(foundView->lensLen == 50.0);
  CHECK(foundView->frontClippingPlaneOffset == 0.5);
  CHECK(foundView->backClippingPlaneOffset == 500.0);
  CHECK(foundView->twistAngle == 0.125);
  CHECK(foundView->viewMode == 7);
  CHECK(foundView->renderMode == 4u);
  CHECK(foundView->hasUcs);
  CHECK(foundView->cameraPlottable);
  CHECK(foundView->ucsOrigin.z == 3.0);
  CHECK(foundView->ucsXAxis.x == 1.0);
  CHECK(foundView->ucsYAxis.y == 1.0);
  CHECK(foundView->ucsOrthoType == 2);
  CHECK(foundView->ucsElevation == 14.0);
  CHECK(foundView->namedUcsHandle == 0x91u);
  CHECK(foundView->visualStyleHandle == 0x94u);
  CHECK(foundView->sunHandle == 0x9Au);
  CHECK(foundView->hasUcsHandleRefs);
  CHECK(foundView->hasVisualHandleRefs);
  CHECK(foundView->sunResolved);
  CHECK(foundView->useDefaultLights == false);
  CHECK(foundView->defaultLightingType == 2u);
  CHECK(foundView->ambientColor == 7u);
  CHECK(metadata.findViewByHandle(0x90u) == foundView);
  CHECK(metadata.findViewByHandle(0u) == nullptr);

  REQUIRE(metadata.lights().size() == 1);
  const auto& capturedLight = metadata.lights().front();
  CHECK(capturedLight.handle == 0x98u);
  CHECK(capturedLight.name == "Spot A");
  CHECK(capturedLight.type == 3u);
  CHECK(capturedLight.color == 0x112233u);
  CHECK(capturedLight.position.x == 1.0);
  CHECK(capturedLight.target.z == 6.0);
  CHECK(capturedLight.attenuationEndLimit == 8.0);
  CHECK(capturedLight.shadowMapSize == 256u);
  CHECK(capturedLight.hasPhotometricData);
  CHECK(capturedLight.hasWebFile);
  CHECK(capturedLight.webFile == "lamp.ies");
  CHECK(capturedLight.lampColorTemperature == 6500.0);
  CHECK(capturedLight.webRotation.y == 1.0);
  CHECK(capturedLight.extendedLightRadius == 13.0);
  CHECK(metadata.findLightByHandle(0x98u) == &capturedLight);
  const auto lightsForOwner = metadata.findLightsByParentHandle(0x99u);
  REQUIRE(lightsForOwner.size() == 1);
  CHECK(lightsForOwner.front() == &capturedLight);

  REQUIRE(metadata.suns().size() == 1);
  const auto& capturedSun = metadata.suns().front();
  CHECK(capturedSun.handle == 0x9Au);
  CHECK(capturedSun.isOn);
  CHECK(capturedSun.color == 0x445566u);
  CHECK(capturedSun.intensity == 1.25);
  CHECK(capturedSun.hasShadow);
  CHECK(capturedSun.julianDay == 2460000);
  CHECK(capturedSun.milliseconds == 43200000);
  CHECK(capturedSun.isDaylightSavings);
  CHECK(capturedSun.shadowMapSize == 512u);
  CHECK(capturedSun.shadowSoftness == 6u);
  REQUIRE(metadata.findSunByHandle(0x9Au) != nullptr);
  CHECK(metadata.findSunForViewName("Camera A") == &capturedSun);
  CHECK(metadata.findSunForViewHandle(0x90u) == &capturedSun);

  REQUIRE(metadata.mleaderStyles().size() == 1);
  const auto& capturedStyle = metadata.mleaderStyles().front();
  CHECK(capturedStyle.handle == 0xA0u);
  CHECK(capturedStyle.parentHandle == 0xA1u);
  CHECK(capturedStyle.name == "CalloutStyle");
  CHECK(capturedStyle.contentType == 2u);
  CHECK(capturedStyle.drawMLeaderOrder == 1u);
  CHECK(capturedStyle.drawLeaderOrder == 2u);
  CHECK(capturedStyle.maxLeaderPoints == 9);
  CHECK(capturedStyle.leaderColor == 3);
  CHECK(capturedStyle.textStyleHandle == 0xA4u);
  CHECK(capturedStyle.blockHandle == 0xA5u);
  CHECK(capturedStyle.leaderLineWeight == 29);
  CHECK(capturedStyle.landingEnabled);
  CHECK(capturedStyle.description == "Callout description");
  CHECK(capturedStyle.arrowHeadSize == 0.75);
  CHECK(capturedStyle.textDefault == "Default text");
  CHECK(capturedStyle.textColor == 5);
  CHECK(capturedStyle.textHeight == 2.5);
  CHECK(capturedStyle.textFrameEnabled);
  CHECK(capturedStyle.blockColor == 6);
  CHECK(capturedStyle.blockScale.y == 2.0);
  CHECK(capturedStyle.scaleFactor == 3.0);
  CHECK(capturedStyle.propertyChanged);
  CHECK(capturedStyle.isAnnotative);
  CHECK(capturedStyle.breakSize == 0.375);
  CHECK(capturedStyle.textExtended);
  CHECK(metadata.findMLeaderStyleByHandle(0xA0u) == &capturedStyle);
  const auto mleaderStylesByArrow =
      metadata.findMLeaderStylesReferencingHandle(0xA3u);
  REQUIRE(mleaderStylesByArrow.size() == 1u);
  CHECK(mleaderStylesByArrow.front() == &capturedStyle);

  REQUIRE(metadata.mleaders().size() == 1);
  const auto& capturedMLeader = metadata.mleaders().front();
  CHECK(capturedMLeader.handle == 0xA6u);
  CHECK(capturedMLeader.parentHandle == 0xA7u);
  CHECK(capturedMLeader.classVersion == 2u);
  CHECK(capturedMLeader.styleHandle == 0xA0u);
  CHECK(capturedMLeader.overrideFlags == 0x1234);
  CHECK(capturedMLeader.leaderType == 2u);
  CHECK(capturedMLeader.styleContentType == 2u);
  CHECK(capturedMLeader.leaderLineTypeHandle == 0xA8u);
  CHECK(capturedMLeader.arrowHeadHandle == 0xA9u);
  CHECK(capturedMLeader.textStyleHandle == 0xAAu);
  CHECK(capturedMLeader.blockHandle == 0xABu);
  CHECK(capturedMLeader.styleResolved);
  CHECK(capturedMLeader.effectiveContentType == 2u);
  CHECK(capturedMLeader.effectiveLeaderType == 2u);
  CHECK(capturedMLeader.effectiveLeaderLineTypeHandle == 0xA8u);
  CHECK(capturedMLeader.effectiveArrowHeadHandle == 0xA9u);
  CHECK(capturedMLeader.effectiveTextStyleHandle == 0xAAu);
  CHECK(capturedMLeader.effectiveBlockHandle == 0xABu);
  CHECK(capturedMLeader.rootCount == 1u);
  CHECK(capturedMLeader.leaderLineCount == 1u);
  CHECK(capturedMLeader.pointCount == 2u);
  CHECK(capturedMLeader.breakCount == 2u);
  CHECK(capturedMLeader.columnCount == 2u);
  CHECK(capturedMLeader.arrowHeadOverrideCount == 1u);
  CHECK(capturedMLeader.blockLabelCount == 1u);
  REQUIRE(capturedMLeader.arrowHeadOverrideHandles.size() == 1u);
  CHECK(capturedMLeader.arrowHeadOverrideHandles.front() == 0xACu);
  REQUIRE(capturedMLeader.blockAttributeDefinitionHandles.size() == 1u);
  CHECK(capturedMLeader.blockAttributeDefinitionHandles.front() == 0xADu);
  REQUIRE(capturedMLeader.blockLabelTexts.size() == 1u);
  CHECK(capturedMLeader.blockLabelTexts.front() == "TAG");
  CHECK(capturedMLeader.overallScale == 2.0);
  CHECK(capturedMLeader.landingDistance == 1.75);
  CHECK(capturedMLeader.defaultArrowHeadSize == 0.625);
  CHECK(capturedMLeader.textHeight == 3.5);
  CHECK(capturedMLeader.landingEnabled);
  CHECK_FALSE(capturedMLeader.doglegEnabled);
  CHECK(capturedMLeader.isAnnotative);
  CHECK(capturedMLeader.hasTextLabel);
  CHECK(capturedMLeader.hasTextContent);
  CHECK(capturedMLeader.hasBlockContent);
  CHECK(metadata.findMLeaderByHandle(0xA6u) == &capturedMLeader);
  const auto mleadersUsingStyle = metadata.findMLeadersUsingStyle(0xA0u);
  REQUIRE(mleadersUsingStyle.size() == 1u);
  CHECK(mleadersUsingStyle.front() == &capturedMLeader);
  const auto mleadersByStyle = metadata.findMLeadersReferencingHandle(0xA0u);
  REQUIRE(mleadersByStyle.size() == 1u);
  CHECK(mleadersByStyle.front() == &capturedMLeader);
  const auto mleadersByArrowOverride =
      metadata.findMLeadersReferencingHandle(0xACu);
  REQUIRE(mleadersByArrowOverride.size() == 1u);
  CHECK(mleadersByArrowOverride.front() == &capturedMLeader);
  CHECK(metadata.findMLeadersReferencingHandle(0xDEADBEEFu).empty());
  metadata.invalidateMLeaderGraphForHandle(0xACu);
  CHECK(capturedMLeader.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateMLeaderGraphForHandle(0xA3u);
  CHECK(capturedStyle.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);

  REQUIRE(metadata.detailViewStyles().size() == 1);
  const auto& capturedDetail = metadata.detailViewStyles().front();
  CHECK(capturedDetail.handle == 0xB0u);
  CHECK(capturedDetail.parentHandle == 0xB1u);
  CHECK(capturedDetail.name == "DetailStyle");
  CHECK(capturedDetail.displayName == "Detail Display");
  CHECK(capturedDetail.identifierStyleHandle == 0xB2u);
  CHECK(capturedDetail.arrowSymbolHandle == 0xB3u);
  CHECK(capturedDetail.viewLabelTextStyleHandle == 0xB4u);
  CHECK(capturedDetail.viewLabelPattern == "DETAIL %s");
  CHECK(capturedDetail.viewLabelTextHeight == 2.0);

  REQUIRE(metadata.sectionViewStyles().size() == 1);
  const auto& capturedSection = metadata.sectionViewStyles().front();
  CHECK(capturedSection.handle == 0xC0u);
  CHECK(capturedSection.parentHandle == 0xC1u);
  CHECK(capturedSection.name == "SectionStyle");
  CHECK(capturedSection.displayName == "Section Display");
  CHECK(capturedSection.arrowStartSymbolHandle == 0xC3u);
  CHECK(capturedSection.arrowEndSymbolHandle == 0xC4u);
  CHECK(capturedSection.viewLabelTextStyleHandle == 0xC7u);
  CHECK(capturedSection.hatchPattern == "ANSI31");
  CHECK(capturedSection.hatchScale == 0.5);
  CHECK(capturedSection.hatchAngleCount == 2u);

  REQUIRE(metadata.breakData().size() == 1);
  const auto& capturedBreakData = metadata.breakData().front();
  CHECK(capturedBreakData.handle == 0xD0u);
  CHECK(capturedBreakData.dimensionHandle == 0xD2u);
  CHECK(capturedBreakData.pointRefHandles.size() == 2u);

  REQUIRE(metadata.breakPointRefs().size() == 1);
  CHECK(metadata.breakPointRefs().front().handle == 0xD5u);

  REQUIRE(metadata.groups().size() == 1);
  const auto& capturedGroup = metadata.groups().front();
  CHECK(capturedGroup.handle == 0xD7u);
  CHECK(capturedGroup.description == "fixture group");
  CHECK(capturedGroup.isUnnamed);
  CHECK(capturedGroup.selectable == false);
  CHECK(capturedGroup.entityHandles.size() == 2u);

  REQUIRE(metadata.imageDefinitionReactors().size() == 1);
  CHECK(metadata.imageDefinitionReactors().front().classVersion == 7);

  REQUIRE(metadata.spatialFilters().size() == 1);
  const auto& capturedSpatialFilter = metadata.spatialFilters().front();
  CHECK(capturedSpatialFilter.boundaryPointCount == 3u);
  CHECK(capturedSpatialFilter.displayBoundary);
  CHECK(capturedSpatialFilter.clipFrontPlane);
  CHECK(capturedSpatialFilter.frontDistance == 2.5);

  REQUIRE(metadata.geoData().size() == 1);
  const auto& capturedGeoData = metadata.geoData().front();
  CHECK(capturedGeoData.hostBlockHandle == 0xE1u);
  CHECK(capturedGeoData.coordinateSystemDefinition == "EPSG:3857");
  CHECK(capturedGeoData.meshPointCount == 1u);
  CHECK(capturedGeoData.meshFaceCount == 1u);

  REQUIRE(metadata.tableGeometry().size() == 1);
  const auto& capturedTableGeometry = metadata.tableGeometry().front();
  CHECK(capturedTableGeometry.rowCount == 2);
  CHECK(capturedTableGeometry.columnCount == 3);
  CHECK(capturedTableGeometry.contentCount == 2u);

  REQUIRE(metadata.placeholders().size() == 1);
  CHECK(metadata.placeholders().front().handle == 0xE4u);

  REQUIRE(metadata.tables().size() == 2);
  const auto& capturedTableStyle = metadata.tables().front();
  CHECK(capturedTableStyle.handle == 0xFCu);
  CHECK(capturedTableStyle.recordName == "SemanticTableStyle");
  CHECK(capturedTableStyle.rowStyleCount == 1u);
  CHECK(capturedTableStyle.cellStyleCount == 1u);
  CHECK(capturedTableStyle.borderCount == 6u);
  CHECK(capturedTableStyle.textStyleHandleCount == 3u);
  CHECK(capturedTableStyle.lineTypeHandleCount == 3u);
  CHECK(capturedTableStyle.titleSuppressed);
  CHECK_FALSE(capturedTableStyle.headerSuppressed);
  CHECK(capturedTableStyle.styleResolved);
  CHECK(metadata.findTableStyleByHandle(0xFCu) == &capturedTableStyle);
  const auto tableStylesByTextStyle =
      metadata.findTableStylesReferencingHandle(0x10Cu);
  REQUIRE(tableStylesByTextStyle.size() == 1u);
  CHECK(tableStylesByTextStyle.front() == &capturedTableStyle);
  const auto tableStylesByLineType =
      metadata.findTableStylesReferencingHandle(0x10Du);
  REQUIRE(tableStylesByLineType.size() == 1u);
  CHECK(tableStylesByLineType.front() == &capturedTableStyle);

  const auto& capturedTableContent = metadata.tables().back();
  CHECK(capturedTableContent.handle == 0xFEu);
  CHECK(capturedTableContent.tableStyleHandle == 0xFCu);
  CHECK(capturedTableContent.rowCount == 1);
  CHECK(capturedTableContent.columnCount == 2);
  CHECK(capturedTableContent.cellCount == 2u);
  CHECK(capturedTableContent.contentCount == 3u);
  CHECK(capturedTableContent.textContentCount == 1u);
  CHECK(capturedTableContent.fieldContentCount == 1u);
  CHECK(capturedTableContent.blockContentCount == 1u);
  CHECK(capturedTableContent.attributeCount == 1u);
  CHECK(capturedTableContent.valueHandleCount == 1u);
  CHECK(capturedTableContent.blockHandleCount == 2u);
  CHECK(capturedTableContent.fieldHandleCount == 3u);
  CHECK(capturedTableContent.attributeHandleCount == 1u);
  CHECK(capturedTableContent.mergedRangeCount == 1u);
  CHECK(capturedTableContent.overrideCellCount == 1u);
  CHECK(capturedTableContent.geometryCellCount == 1u);
  CHECK(capturedTableContent.hasTextContent);
  CHECK(capturedTableContent.hasBlockContent);
  CHECK(capturedTableContent.semanticParsed);
  CHECK(capturedTableContent.styleResolved);
  CHECK(metadata.findTableByHandle(0xFEu) == &capturedTableContent);
  const auto tablesUsingStyle = metadata.findTablesUsingStyle(0xFCu);
  REQUIRE(tablesUsingStyle.size() == 1u);
  CHECK(tablesUsingStyle.front() == &capturedTableContent);
  const auto tablesByStyleHandle = metadata.findTablesReferencingHandle(0xFCu);
  REQUIRE(tablesByStyleHandle.size() == 1u);
  CHECK(tablesByStyleHandle.front() == &capturedTableContent);
  REQUIRE(capturedTableContent.columnWidths.size() == 2u);
  CHECK(capturedTableContent.columnWidths[0] == 12.5);
  REQUIRE(capturedTableContent.rowHeights.size() == 1u);
  CHECK(capturedTableContent.rowHeights[0] == 4.25);
  REQUIRE(capturedTableContent.cellTexts.size() == 1u);
  CHECK(capturedTableContent.cellTexts.front() == "Cell text");
  REQUIRE(capturedTableContent.attributeTexts.size() == 1u);
  CHECK(capturedTableContent.attributeTexts.front() == "A1");
  REQUIRE(capturedTableContent.attributeHandles.size() == 1u);
  CHECK(capturedTableContent.attributeHandles.front() == 0x104u);
  REQUIRE(capturedTableContent.valueHandles.size() == 1u);
  CHECK(capturedTableContent.valueHandles.front() == 0x102u);
  REQUIRE(capturedTableContent.geometryHandles.size() == 1u);
  CHECK(capturedTableContent.geometryHandles.front() == 0x103u);
  REQUIRE(capturedTableContent.cellStyleIds.size() == 1u);
  CHECK(capturedTableContent.cellStyleIds.front() == 9);
  REQUIRE(capturedTableContent.mergedRanges.size() == 1u);
  CHECK(capturedTableContent.mergedRanges.front().rightColumn == 1);
  CHECK(capturedTableContent.fieldHandles.back() == 0x107u);
  CHECK(capturedTableContent.blockHandles.back() == 0x106u);
  REQUIRE(capturedTableContent.cells.size() == 2u);
  const auto& firstCell = capturedTableContent.cells.front();
  CHECK(firstCell.row == 0);
  CHECK(firstCell.column == 0);
  CHECK(firstCell.styleId == 9);
  CHECK(firstCell.overrideFlags == 0x1u);
  CHECK(firstCell.valueHandle == 0x102u);
  CHECK(firstCell.geometryHandle == 0x103u);
  CHECK(firstCell.contentCount == 1u);
  CHECK(firstCell.textContentCount == 1u);
  CHECK(firstCell.attributeCount == 1u);
  CHECK(firstCell.attributeHandleCount == 1u);
  REQUIRE(firstCell.texts.size() == 1u);
  CHECK(firstCell.texts.front() == "Cell text");
  REQUIRE(firstCell.attributeTexts.size() == 1u);
  CHECK(firstCell.attributeTexts.front() == "A1");
  REQUIRE(firstCell.attributeHandles.size() == 1u);
  CHECK(firstCell.attributeHandles.front() == 0x104u);
  const auto& secondCell = capturedTableContent.cells.back();
  CHECK(secondCell.row == 0);
  CHECK(secondCell.column == 1);
  CHECK(secondCell.blockHandle == 0x105u);
  CHECK(secondCell.fieldContentCount == 1u);
  CHECK(secondCell.blockContentCount == 1u);
  REQUIRE(secondCell.contentHandles.size() == 2u);
  CHECK(secondCell.contentHandles.front() == 0x106u);
  CHECK(secondCell.contentHandles.back() == 0x107u);
  CHECK(metadata.findTableCell(0xFEu, 0, 0) == &firstCell);
  CHECK(metadata.findTableCell(0xFEu, 0, 1) == &secondCell);
  CHECK(metadata.findTableCell(0xFEu, 1, 0) == nullptr);
  const auto cellsByValue = metadata.findTableCellsReferencingHandle(0x102u);
  REQUIRE(cellsByValue.size() == 1u);
  CHECK(cellsByValue.front() == &firstCell);
  const auto cellsByAttribute = metadata.findTableCellsReferencingHandle(0x104u);
  REQUIRE(cellsByAttribute.size() == 1u);
  CHECK(cellsByAttribute.front() == &firstCell);
  const auto cellsByBlockContent = metadata.findTableCellsReferencingHandle(0x106u);
  REQUIRE(cellsByBlockContent.size() == 1u);
  CHECK(cellsByBlockContent.front() == &secondCell);
  const auto tablesByBlockContent = metadata.findTablesReferencingHandle(0x106u);
  REQUIRE(tablesByBlockContent.size() == 1u);
  CHECK(tablesByBlockContent.front() == &capturedTableContent);
  const auto cellsByMissingHandle = metadata.findTableCellsReferencingHandle(0xDEADBEEFu);
  CHECK(cellsByMissingHandle.empty());
  CHECK(metadata.findTablesReferencingHandle(0xDEADBEEFu).empty());
  const LC_DwgAdvancedMetadata::TableWriterBlockerCounts tableBlockers =
      metadata.tableWriterBlockerCounts();
  CHECK(tableBlockers.tableCount == 1u);
  CHECK(tableBlockers.fieldContent == 1u);
  CHECK(tableBlockers.blockContent == 1u);
  CHECK(tableBlockers.attributeContent == 1u);
  CHECK(tableBlockers.overrideCells == 1u);
  CHECK(tableBlockers.geometryCells == 1u);
  CHECK(tableBlockers.totalBlockers() == 5u);
  CHECK(capturedTableContent.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);
  metadata.invalidateTableGraphForHandle(0x106u);
  CHECK(capturedTableContent.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);

  REQUIRE(metadata.modelerGeometry().size() == 1);
  const auto& capturedModeler = metadata.modelerGeometry().front();
  CHECK(capturedModeler.handle == 0xF9u);
  CHECK(capturedModeler.parentHandle == 0xFAu);
  CHECK(capturedModeler.type == DRW::E3DSOLID);
  CHECK(capturedModeler.modelerVersion == 0x21u);
  CHECK(capturedModeler.bodyBitSize == 456u);
  CHECK(capturedModeler.objectSize == 64u);
  CHECK_FALSE(capturedModeler.isEmpty);
  CHECK(capturedModeler.hasModelerData);
  CHECK(capturedModeler.modelerDataUnknownBit);
  CHECK(capturedModeler.hasWireframe);
  CHECK(capturedModeler.hasRawPayload);
  CHECK(capturedModeler.payloadKind ==
        LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(std::string(LC_DwgAdvancedMetadata::modelerPayloadKindName(
            capturedModeler.payloadKind)) == "SAB");
  CHECK(capturedModeler.historyHandle == 0xFBu);
  CHECK(capturedModeler.rawByteCount == 15u);
  CHECK(capturedModeler.rawBytes.back() == 'e');
  CHECK(metadata.findModelerGeometryByHandle(0xF9u) == &capturedModeler);
  const auto modelerByHistory = metadata.findModelerGeometryByHistoryHandle(0xFBu);
  REQUIRE(modelerByHistory.size() == 1u);
  CHECK(modelerByHistory.front() == &capturedModeler);

  REQUIRE(metadata.associativeObjects().size() == 1);
  const auto& capturedAssoc = metadata.associativeObjects().front();
  CHECK(capturedAssoc.handle == 0xE6u);
  CHECK(capturedAssoc.parentHandle == 0xE7u);
  CHECK(capturedAssoc.recordName == "ACDBASSOCVERTEXACTIONPARAM");
  CHECK(capturedAssoc.kind ==
        LC_DwgAdvancedMetadata::AssociativeKind::VertexActionParam);
  CHECK(std::string(LC_DwgAdvancedMetadata::associativeKindName(
            capturedAssoc.kind)) == "ACDBASSOCVERTEXACTIONPARAM");
  CHECK(capturedAssoc.geometryStatus == 4);
  CHECK(capturedAssoc.owningNetworkHandle == 0xE8u);
  CHECK(capturedAssoc.actionBodyHandle == 0xE9u);
  CHECK(capturedAssoc.actionIndex == 11);
  CHECK(capturedAssoc.maxDependencyIndex == 12);
  CHECK(capturedAssoc.dependencyCount == 2u);
  CHECK(capturedAssoc.actionCount == 1u);
  CHECK(capturedAssoc.dependencyRefs.front().m_isOwned);
  CHECK(capturedAssoc.dependencyRefs.back().m_handle == 0xEBu);
  CHECK(capturedAssoc.actionRefs.front().m_handle == 0xECu);
  CHECK(capturedAssoc.ownedParamHandles.size() == 2u);
  CHECK(capturedAssoc.ownedActionHandles.front() == 0xEFu);
  CHECK(capturedAssoc.dependencyHandle == 0xF0u);
  CHECK(capturedAssoc.readDependencyHandle == 0xF1u);
  CHECK(capturedAssoc.writeDependencyHandle == 0xF2u);
  CHECK(capturedAssoc.rNodeHandle == 0xF3u);
  CHECK(capturedAssoc.dNodeHandle == 0xF4u);
  CHECK(capturedAssoc.status == 5);
  CHECK(capturedAssoc.osnapMode == 6u);
  CHECK(capturedAssoc.parameter == 0.875);
  CHECK(capturedAssoc.point.z == 9.0);
  CHECK(capturedAssoc.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);
  CHECK(metadata.findAssociativeObjectByHandle(0xE6u) == &capturedAssoc);
  const auto assocByKind = metadata.findAssociativeObjectsByKind(
      LC_DwgAdvancedMetadata::AssociativeKind::VertexActionParam);
  REQUIRE(assocByKind.size() == 1u);
  CHECK(assocByKind.front() == &capturedAssoc);
  CHECK(metadata.findAssociativeObjectsByKind(
            LC_DwgAdvancedMetadata::AssociativeKind::Action)
            .empty());
  const auto assocByDependency = metadata.findAssociativeObjectsReferencingHandle(0xEBu);
  REQUIRE(assocByDependency.size() == 1u);
  CHECK(assocByDependency.front() == &capturedAssoc);
  metadata.invalidateAssociativeGraphForHandle(0xEBu);
  CHECK(capturedAssoc.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);

  REQUIRE(metadata.acshObjects().size() == 1);
  const auto& capturedAcSh = metadata.acshObjects().front();
  CHECK(capturedAcSh.handle == 0xF5u);
  CHECK(capturedAcSh.parentHandle == 0xF6u);
  CHECK(capturedAcSh.recordName == "ACSH_SWEEP_CLASS");
  CHECK(capturedAcSh.major == 24u);
  CHECK(capturedAcSh.minor == 3u);
  CHECK(capturedAcSh.ownerHandle == 0xF7u);
  CHECK(capturedAcSh.historyNodeId == 0xF8u);
  CHECK(capturedAcSh.showHistory);
  CHECK(capturedAcSh.recordHistory);
  CHECK(capturedAcSh.direction.z == 1.0);
  CHECK(capturedAcSh.draftAngle == 0.125);
  CHECK(capturedAcSh.startDraftDistance == 1.25);
  CHECK(capturedAcSh.endDraftDistance == 2.5);
  CHECK(capturedAcSh.scaleFactor == 1.5);
  CHECK(capturedAcSh.twistAngle == 0.25);
  CHECK(capturedAcSh.alignAngle == 0.5);
  CHECK(capturedAcSh.binaryBlob1Bytes == 2u);
  CHECK(capturedAcSh.binaryBlob2Bytes == 3u);
  CHECK(capturedAcSh.blobBytes == 5u);
  CHECK(metadata.findAcShObjectByHandle(0xF5u) == &capturedAcSh);
  const auto acshByOwner = metadata.findAcShObjectsByOwnerHandle(0xF7u);
  REQUIRE(acshByOwner.size() == 1u);
  CHECK(acshByOwner.front() == &capturedAcSh);

  REQUIRE(metadata.hasReplayableAdvancedObjects());
  CHECK(metadata.semanticOnlyRecordCount() >= 18u);
  metadata.invalidateByOwner(0xA1u);
  CHECK(capturedStyle.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateByOwner(0xB1u);
  CHECK(capturedDetail.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateByOwner(0xC1u);
  CHECK(capturedSection.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateByOwner(0xD1u);
  CHECK(capturedBreakData.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateByHandle(0x77u);
  CHECK(metadata.rawObjects().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  CHECK(foundView->replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);
}

TEST_CASE("DWG advanced metadata resolves TABLESTYLE after TABLECONTENT import",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_TableContentObject tableContent;
  tableContent.handle = 0x410u;
  tableContent.m_parseComplete = true;
  tableContent.m_content.m_tableStyleHandle = 0x420u;
  metadata.addTableContent(tableContent);

  REQUIRE(metadata.tables().size() == 1u);
  CHECK_FALSE(metadata.tables().front().styleResolved);

  DRW_TableStyle tableStyle;
  tableStyle.handle = 0x420u;
  tableStyle.m_name = "LateTableStyle";
  tableStyle.m_tableCellStyle.m_borders.resize(1);
  metadata.addTableStyle(tableStyle);

  REQUIRE(metadata.tables().size() == 2u);
  const auto* resolvedContent = metadata.findTableByHandle(0x410u);
  REQUIRE(resolvedContent != nullptr);
  CHECK(resolvedContent->styleResolved);
  const auto* resolvedStyle = metadata.findTableStyleByHandle(0x420u);
  REQUIRE(resolvedStyle != nullptr);
  CHECK(resolvedStyle->recordName == "LateTableStyle");
  const auto tablesUsingLateStyle = metadata.findTablesUsingStyle(0x420u);
  REQUIRE(tablesUsingLateStyle.size() == 1u);
  CHECK(tablesUsingLateStyle.front() == resolvedContent);
}

TEST_CASE("DWG advanced metadata invalidates TABLECONTENT raw replay",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject rawTableContent;
  rawTableContent.m_objectType = 512;
  rawTableContent.m_handle = 0x440u;
  rawTableContent.m_isCustomClass = true;
  rawTableContent.m_recordName = "TABLECONTENT";
  rawTableContent.m_className = "AcDbTableContent";
  rawTableContent.m_rawBytes = {0x01u, 0x02u, 0x03u};
  metadata.addUnsupportedObject(rawTableContent);

  DRW_UnsupportedObject unrelatedRawObject;
  unrelatedRawObject.m_objectType = 513;
  unrelatedRawObject.m_handle = 0x441u;
  unrelatedRawObject.m_isCustomClass = true;
  unrelatedRawObject.m_recordName = "RAW_REPLAY_TEST";
  unrelatedRawObject.m_className = "AcDbRawReplayTest";
  unrelatedRawObject.m_rawBytes = {0x04u};
  metadata.addUnsupportedObject(unrelatedRawObject);

  DRW_TableContentObject tableContent;
  tableContent.handle = 0x440u;
  tableContent.m_parseComplete = true;
  DRW_TableColumn column;
  column.m_width = 2.0;
  tableContent.m_content.m_columns.push_back(column);
  DRW_TableRow row;
  row.m_height = 1.0;
  DRW_TableCell cell;
  cell.m_attributes.push_back({0x442u, 1, "Attribute text"});
  row.m_cells.push_back(cell);
  tableContent.m_content.m_rows.push_back(row);
  metadata.addTableContent(tableContent);

  metadata.invalidateTableGraphForHandle(0x442u);

  REQUIRE(metadata.tables().size() == 1u);
  CHECK(metadata.tables().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 2u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().front()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().back()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::None);
}

TEST_CASE("DWG advanced metadata invalidates TABLESTYLE raw replay",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject rawTableStyle;
  rawTableStyle.m_objectType = 514;
  rawTableStyle.m_handle = 0x450u;
  rawTableStyle.m_isCustomClass = true;
  rawTableStyle.m_recordName = "TABLESTYLE";
  rawTableStyle.m_className = "AcDbTableStyle";
  rawTableStyle.m_rawBytes = {0x01u, 0x02u};
  metadata.addUnsupportedObject(rawTableStyle);

  DRW_TableStyle tableStyle;
  tableStyle.handle = 0x450u;
  tableStyle.m_name = "HandleStyle";
  tableStyle.m_tableCellStyle.m_contentFormat.m_textStyleHandle = 0x451u;
  tableStyle.m_tableCellStyle.m_borders.resize(1);
  tableStyle.m_tableCellStyle.m_borders[0].m_lineTypeHandle = 0x452u;
  metadata.addTableStyle(tableStyle);

  const auto stylesByLineType = metadata.findTableStylesReferencingHandle(0x452u);
  REQUIRE(stylesByLineType.size() == 1u);
  CHECK(stylesByLineType.front()->handle == 0x450u);

  metadata.invalidateTableGraphForHandle(0x452u);

  REQUIRE(metadata.tables().size() == 1u);
  CHECK(metadata.tables().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 1u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().front()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
}

TEST_CASE("DWG advanced metadata classifies modeler payload markers",
          "[entity_metadata][dwg_metadata][modeler]") {
  CHECK(LC_DwgAdvancedMetadata::classifyModelerPayload(
            {'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F',
             'i', 'l', 'e'})
        == LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(LC_DwgAdvancedMetadata::classifyModelerPayload(
            {'7', ' ', '0', ' ', '0', '\n', 'B', 'e', 'g', 'i', 'n', '-',
             'o', 'f', '-', 'A', 'C', 'I', 'S', '-', 'H', 'i', 's', 't',
             'o', 'r', 'y'})
        == LC_DwgAdvancedMetadata::ModelerPayloadKind::Sat);
  CHECK(LC_DwgAdvancedMetadata::classifyModelerPayload({0x01u, 0x02u})
        == LC_DwgAdvancedMetadata::ModelerPayloadKind::Unknown);
}

TEST_CASE("DWG advanced metadata classifies associative object names",
          "[entity_metadata][dwg_metadata][assoc]") {
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName("ACDBASSOCNETWORK")
        == LC_DwgAdvancedMetadata::AssociativeKind::Network);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName("ACDBASSOCACTION")
        == LC_DwgAdvancedMetadata::AssociativeKind::Action);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCDEPENDENCY")
        == LC_DwgAdvancedMetadata::AssociativeKind::Dependency);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCGEOMDEPENDENCY")
        == LC_DwgAdvancedMetadata::AssociativeKind::GeometryDependency);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCPERSSUBENTMANAGER")
        == LC_DwgAdvancedMetadata::AssociativeKind::PersistentSubentityManager);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBPERSSUBENTMANAGER")
        == LC_DwgAdvancedMetadata::AssociativeKind::PersistentSubentityManager);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCALIGNEDDIMACTIONBODY")
        == LC_DwgAdvancedMetadata::AssociativeKind::AlignedDimensionActionBody);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCVERTEXACTIONPARAM")
        == LC_DwgAdvancedMetadata::AssociativeKind::VertexActionParam);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName(
            "ACDBASSOCOSNAPPOINTREFACTIONPARAM")
        == LC_DwgAdvancedMetadata::AssociativeKind::OsnapPointRefActionParam);
  CHECK(LC_DwgAdvancedMetadata::associativeKindFromRecordName("ACDBUNKNOWN")
        == LC_DwgAdvancedMetadata::AssociativeKind::Unknown);
  CHECK(std::string(LC_DwgAdvancedMetadata::associativeKindName(
            LC_DwgAdvancedMetadata::AssociativeKind::Unknown)) == "unknown");
}

TEST_CASE("DWG advanced metadata classifies raw object families",
          "[entity_metadata][dwg_metadata][raw-replay]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject evaluationGraph;
  evaluationGraph.m_handle = 0x510u;
  evaluationGraph.m_recordName = "ACAD_EVALUATION_GRAPH";
  evaluationGraph.m_className = "AcDbEvalGraph";
  evaluationGraph.m_rawBytes = {0x01u};
  metadata.addUnsupportedObject(evaluationGraph);

  DRW_UnsupportedObject dynamicBlock;
  dynamicBlock.m_handle = 0x511u;
  dynamicBlock.m_recordName = "BLOCKROTATIONPARAMETER";
  dynamicBlock.m_className = "AcDbBlockRotationParameter";
  dynamicBlock.m_rawBytes = {0x02u};
  metadata.addUnsupportedObject(dynamicBlock);

  DRW_UnsupportedObject objectContext;
  objectContext.m_handle = 0x512u;
  objectContext.m_recordName = "ACDB_MLEADEROBJECTCONTEXTDATA_CLASS";
  objectContext.m_className = "AcDbMLeaderObjectContextData";
  objectContext.m_rawBytes = {0x03u};
  metadata.addUnsupportedObject(objectContext);

  DRW_UnsupportedObject unknown;
  unknown.m_handle = 0x513u;
  unknown.m_recordName = "RAW_REPLAY_TEST";
  unknown.m_className = "AcDbRawReplayTest";
  unknown.m_rawBytes = {0x04u};
  metadata.addUnsupportedObject(unknown);

  CHECK(LC_DwgAdvancedMetadata::rawObjectFamilyFromNames(
            "ACDBASSOCDEPENDENCY", "AcDbAssocDependency")
        == LC_DwgAdvancedMetadata::RawObjectFamily::Associative);
  CHECK(LC_DwgAdvancedMetadata::rawObjectFamilyFromNames(
            "BLOCKFLIPPARAMETER", "AcDbBlockFlipParameter")
        == LC_DwgAdvancedMetadata::RawObjectFamily::DynamicBlock);
  CHECK(LC_DwgAdvancedMetadata::rawObjectFamilyFromNames(
            "ACDB_BLKREFOBJECTCONTEXTDATA_CLASS", "AcDbBlkRefObjectContextData")
        == LC_DwgAdvancedMetadata::RawObjectFamily::ObjectContext);
  CHECK(std::string(LC_DwgAdvancedMetadata::rawObjectFamilyName(
            LC_DwgAdvancedMetadata::RawObjectFamily::EvaluationGraph))
        == "evaluation graph");

  const auto evalGraphs = metadata.findRawObjectsByFamily(
      LC_DwgAdvancedMetadata::RawObjectFamily::EvaluationGraph);
  REQUIRE(evalGraphs.size() == 1u);
  CHECK(evalGraphs.front()->handle == 0x510u);
  const auto dynamicBlocks = metadata.findRawObjectsByFamily(
      LC_DwgAdvancedMetadata::RawObjectFamily::DynamicBlock);
  REQUIRE(dynamicBlocks.size() == 1u);
  CHECK(dynamicBlocks.front()->handle == 0x511u);
  const auto contexts = metadata.findRawObjectsByFamily(
      LC_DwgAdvancedMetadata::RawObjectFamily::ObjectContext);
  REQUIRE(contexts.size() == 1u);
  CHECK(contexts.front()->handle == 0x512u);
  const auto unknowns = metadata.findRawObjectsByFamily(
      LC_DwgAdvancedMetadata::RawObjectFamily::Unknown);
  REQUIRE(unknowns.size() == 1u);
  CHECK(unknowns.front()->handle == 0x513u);

  const LC_DwgAdvancedMetadata::RawObjectFamilyCounts counts =
      metadata.rawObjectFamilyCounts();
  CHECK(counts.total() == 4u);
  CHECK(counts.associative == 0u);
  CHECK(counts.evaluationGraph == 1u);
  CHECK(counts.dynamicBlock == 1u);
  CHECK(counts.objectContext == 1u);
  CHECK(counts.unknown == 1u);
  CHECK(LC_DwgAdvancedMetadata::rawObjectFamilyCount(
            counts, LC_DwgAdvancedMetadata::RawObjectFamily::DynamicBlock)
        == 1u);
}

TEST_CASE("DWG advanced metadata invalidates associative raw replay",
          "[entity_metadata][dwg_metadata][raw-replay]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_AssociativeObject associativeObject("ACDBASSOCDEPENDENCY");
  associativeObject.handle = 0x210u;
  associativeObject.m_dependencies = {{false, 0x220u}};
  metadata.addAssociativeObject(associativeObject);

  DRW_UnsupportedObject rawAssociativeObject;
  rawAssociativeObject.m_objectType = 509;
  rawAssociativeObject.m_handle = 0x210u;
  rawAssociativeObject.m_isCustomClass = true;
  rawAssociativeObject.m_recordName = "ACDBASSOCDEPENDENCY";
  rawAssociativeObject.m_className = "AcDbAssocDependency";
  rawAssociativeObject.m_rawBytes = {0x01u, 0x02u};
  metadata.addUnsupportedObject(rawAssociativeObject);

  DRW_UnsupportedObject unrelatedRawObject;
  unrelatedRawObject.m_objectType = 510;
  unrelatedRawObject.m_handle = 0x211u;
  unrelatedRawObject.m_isCustomClass = true;
  unrelatedRawObject.m_recordName = "RAW_REPLAY_TEST";
  unrelatedRawObject.m_className = "AcDbRawReplayTest";
  unrelatedRawObject.m_rawBytes = {0x03u};
  metadata.addUnsupportedObject(unrelatedRawObject);

  metadata.invalidateAssociativeGraphForHandle(0x220u);

  REQUIRE(metadata.associativeObjects().size() == 1u);
  CHECK(metadata.associativeObjects().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 2u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().front()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().back()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::None);
  CHECK(std::string(LC_DwgAdvancedMetadata::replayBlockerName(
            LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated)) ==
        "invalidated");
  CHECK(std::string(LC_DwgAdvancedMetadata::replayBlockerName(
            LC_DwgAdvancedMetadata::ReplayBlocker::Replaced)) == "replaced");
}

TEST_CASE("DWG advanced metadata resolves MLEADERSTYLE after MLEADER import",
          "[entity_metadata][dwg_metadata][mleader]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_MLeader mleader;
  mleader.handle = 0x310u;
  mleader.styleHandle.ref = 0x320u;
  metadata.addMLeader(mleader);

  REQUIRE(metadata.mleaders().size() == 1u);
  CHECK_FALSE(metadata.mleaders().front().styleResolved);

  DRW_MLeaderStyle style;
  style.handle = 0x320u;
  style.contentType = 2u;
  style.leaderType = 1u;
  style.leaderLineTypeHandle.ref = 0x321u;
  style.arrowHeadBlockHandle.ref = 0x322u;
  style.textStyleHandle.ref = 0x323u;
  style.blockHandle.ref = 0x324u;
  metadata.addMLeaderStyle(style);

  const auto& capturedMLeader = metadata.mleaders().front();
  CHECK(capturedMLeader.styleResolved);
  CHECK(capturedMLeader.effectiveContentType == 2u);
  CHECK(capturedMLeader.effectiveLeaderType == 1u);
  CHECK(capturedMLeader.effectiveLeaderLineTypeHandle == 0x321u);
  CHECK(capturedMLeader.effectiveArrowHeadHandle == 0x322u);
  CHECK(capturedMLeader.effectiveTextStyleHandle == 0x323u);
  CHECK(capturedMLeader.effectiveBlockHandle == 0x324u);
}

TEST_CASE("DWG advanced metadata invalidates MLEADERSTYLE raw replay",
          "[entity_metadata][dwg_metadata][mleader]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject rawStyle;
  rawStyle.m_objectType = 511;
  rawStyle.m_handle = 0x330u;
  rawStyle.m_isCustomClass = true;
  rawStyle.m_recordName = "MLEADERSTYLE";
  rawStyle.m_className = "AcDbMLeaderStyle";
  rawStyle.m_rawBytes = {0x01u, 0x02u, 0x03u};
  metadata.addUnsupportedObject(rawStyle);

  DRW_MLeaderStyle style;
  style.handle = 0x330u;
  style.arrowHeadBlockHandle.ref = 0x331u;
  metadata.addMLeaderStyle(style);

  metadata.invalidateMLeaderGraphForHandle(0x331u);

  REQUIRE(metadata.mleaderStyles().size() == 1u);
  CHECK(metadata.mleaderStyles().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 1u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().front()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
}

TEST_CASE("DWG fixture manifest is valid JSON and optional by default",
          "[entity_metadata][dwg_fixtures]") {
  QFile manifest("libraries/libdxfrw/testdata/dwg-fixtures.json");
  REQUIRE(manifest.open(QIODevice::ReadOnly));
  QJsonParseError error;
  const QJsonDocument document =
      QJsonDocument::fromJson(manifest.readAll(), &error);
  REQUIRE(error.error == QJsonParseError::NoError);
  REQUIRE(document.isObject());

  const QJsonObject root = document.object();
  CHECK(root.value("version").toInt() == 1);
  const QJsonArray fixtures = root.value("fixtures").toArray();
  REQUIRE(!fixtures.isEmpty());
  const QJsonObject first = fixtures.first().toObject();
  CHECK(first.value("optional").toBool());
  CHECK(first.value("path").toString().contains("${HOME}"));
  CHECK(!first.value("targetVersion").toString().isEmpty());
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("table"))));
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("mleader"))));
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("acis"))));
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("dynamic-block"))));
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("r2018-text"))));
  CHECK(first.value("tags").toArray().contains(QJsonValue(QStringLiteral("advanced-entities"))));
  CHECK(first.value("expectedCallbacks").toArray().contains(
      QJsonValue(QStringLiteral("addTableContent"))));
  CHECK(first.value("expectedUnsupportedDiagnostics").isArray());
  const QJsonArray rawFamilies = first.value("expectedRawFamilies").toArray();
  CHECK(rawFamilies.contains(QJsonValue(QStringLiteral("associative"))));
  CHECK(rawFamilies.contains(QJsonValue(QStringLiteral("evaluation graph"))));
  CHECK(rawFamilies.contains(QJsonValue(QStringLiteral("dynamic block"))));
  CHECK(rawFamilies.contains(QJsonValue(QStringLiteral("object context"))));
  CHECK(first.value("expectedRawReplayCount").toInt(-1) >= 0);
  CHECK(first.value("references").toObject().contains("acadSharp"));
  CHECK(first.value("references").toObject().contains("libreDwg"));
  CHECK(first.value("expect").toObject().value("preserveRawUnsupported").toBool());
}

namespace {

// Stub base that satisfies every DRW_Interface pure virtual with an
// empty body. Test-specific interfaces below override only what they
// need.
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
  void addWipeout(const DRW_Image *) override {}
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

class SingleLineCapture : public StubInterface {
public:
  bool m_gotLine = false;
  DRW_Line m_captured;
  void addLine(const DRW_Line &d) override {
    if (!m_gotLine) {
      m_captured = d;
      m_gotLine = true;
    }
  }
};

class SingleLineEmitter : public StubInterface {
public:
  DRW_Line m_line;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeLine(&m_line); }
};

class SingleSplineEmitter : public StubInterface {
public:
  DRW_Spline m_spline;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeSpline(&m_spline); }
};

class SingleSplineCapture : public StubInterface {
public:
  bool m_gotSpline = false;
  DRW_Spline m_captured;
  void addSpline(const DRW_Spline *spline) override {
    if (!m_gotSpline && spline != nullptr) {
      m_captured = *spline;
      m_gotSpline = true;
    }
  }
};

class SinglePolylineEmitter : public StubInterface {
public:
  DRW_Polyline m_polyline;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writePolyline(&m_polyline); }
};

class SinglePolylineCapture : public StubInterface {
public:
  bool m_gotPolyline = false;
  DRW_Polyline m_captured;
  void addPolyline(const DRW_Polyline &polyline) override {
    if (!m_gotPolyline) {
      m_captured = polyline;
      m_gotPolyline = true;
    }
  }
};

} // namespace

TEST_CASE("DXF round-trip: 284/347/390/430/440 survive write+read",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_metadata_roundtrip.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;                        // ByLayer
  emitter.m_line.transparency = (0x03 << 24) | 0x80; // 50% alpha
  emitter.m_line.material = 0xABCDu;
  emitter.m_line.plotStyle = 0x1234;
  emitter.m_line.shadow = static_cast<DRW::ShadowMode>(2); // ReceiveOnly
  emitter.m_line.colorName = "RAL$RAL 1003";

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false)); // R2007 ASCII
  }

  SingleLineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotLine);
  CHECK(capture.m_captured.transparency == ((0x03 << 24) | 0x80));
  CHECK(capture.m_captured.material == 0xABCDu);
  CHECK(capture.m_captured.plotStyle == 0x1234);
  CHECK(static_cast<int>(capture.m_captured.shadow) == 2);
  CHECK(capture.m_captured.colorName == "RAL$RAL 1003");

  std::filesystem::remove(path);
}

TEST_CASE("DXF round-trip: default-valued entity emits no metadata codes",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_metadata_default.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;
  // All metadata fields left at default sentinels.

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  // Read the file as text and assert the metadata group codes are
  // absent — confirms the skip-on-default write guards work.
  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();

  // Each appears as "\n<code>\n..." in DXF ASCII. Only check codes
  // that are entity-unique — 284 collides with DIMSTYLE::dimtzin
  // and 390 is also emitted by LTYPE writes, so they appear in the
  // file regardless of entity content. 347/430/440 are entity-only
  // and prove the skip-on-default guards work.
  CHECK(content.find("\n347\n") == std::string::npos);
  CHECK(content.find("\n430\n") == std::string::npos);
  CHECK(content.find("\n440\n") == std::string::npos);

  std::filesystem::remove(path);
}

TEST_CASE("DXF write: app-data doubles preserve fractional values",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_appdata_double.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;

  std::list<DRW_Variant> appData;
  appData.emplace_back(102, std::string{"APPDATA"});
  appData.emplace_back(40, 12.75);
  emitter.m_line.appData.push_back(appData);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();

  CHECK(content.find("\n102\n{APPDATA\n") != std::string::npos);
  CHECK(content.find("\n 40\n12.75\n") != std::string::npos);
  CHECK(content.find("\n102\n}\n") != std::string::npos);

  std::filesystem::remove(path);
}

TEST_CASE("DXF write: spline weights force rational flag",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_spline_rational_flag.dxf";
  std::filesystem::remove(path);

  SingleSplineEmitter emitter;
  emitter.m_spline.layer = "0";
  emitter.m_spline.color = 256;
  emitter.m_spline.flags = 8; // planar only; writer should add rational bit
  emitter.m_spline.degree = 2;
  emitter.m_spline.knotslist = {0, 0, 0, 1, 1, 1};
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(2.0, 0.0, 0.0));
  emitter.m_spline.weightlist = {1.0, 0.5, 1.0};

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  SingleSplineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotSpline);
  CHECK((capture.m_captured.flags & 0x04) == 0x04);
  REQUIRE(capture.m_captured.weightlist.size() == 3);
  CHECK(capture.m_captured.weightlist[1] == 0.5);

  std::filesystem::remove(path);
}

TEST_CASE("DXF round-trip: polyface vertex flags survive write+read",
          "[entity_metadata][dxf_roundtrip][polyline]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_polyface_vertex_flags.dxf";
  std::filesystem::remove(path);

  SinglePolylineEmitter emitter;
  emitter.m_polyline.layer = "0";
  emitter.m_polyline.color = 256;
  emitter.m_polyline.flags = 64;
  emitter.m_polyline.vertexcount = 3;
  emitter.m_polyline.facecount = 1;

  DRW_Vertex v1(0.0, 0.0, 0.0, 0.0);
  v1.flags = 64 | 128;
  DRW_Vertex v2(10.0, 0.0, 0.0, 0.0);
  v2.flags = 64 | 128;
  DRW_Vertex v3(0.0, 10.0, 0.0, 0.0);
  v3.flags = 64 | 128;
  DRW_Vertex face;
  face.flags = 128;
  face.vindex1 = 1;
  face.vindex2 = -2;
  face.vindex3 = 3;

  emitter.m_polyline.addVertex(v1);
  emitter.m_polyline.addVertex(v2);
  emitter.m_polyline.addVertex(v3);
  emitter.m_polyline.addVertex(face);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  SinglePolylineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotPolyline);
  CHECK(capture.m_captured.flags == 64);
  REQUIRE(capture.m_captured.vertlist.size() == 4);
  CHECK(capture.m_captured.vertlist[0]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[1]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[2]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[3]->flags == 128);
  CHECK(capture.m_captured.vertlist[3]->vindex1 == 1);
  CHECK(capture.m_captured.vertlist[3]->vindex2 == -2);
  CHECK(capture.m_captured.vertlist[3]->vindex3 == 3);

  std::filesystem::remove(path);
}
