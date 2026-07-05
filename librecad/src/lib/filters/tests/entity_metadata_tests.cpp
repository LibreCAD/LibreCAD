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

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

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

  DRW_UnsupportedObject rawModelerEntity;
  rawModelerEntity.m_objectType = 37;
  rawModelerEntity.m_handle = 0x7Au;
  rawModelerEntity.m_isEntity = true;
  rawModelerEntity.m_rawBytes = {0x25u, 0x01u, 0x02u};
  metadata.addUnsupportedObject(rawModelerEntity);

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
  tableStyle.m_flowDirection = 1;
  tableStyle.m_flags = 0x24;
  tableStyle.m_horizontalCellMargin = 0.75;
  tableStyle.m_verticalCellMargin = 0.5;
  tableStyle.m_titleSuppressed = true;
  tableStyle.m_headerSuppressed = false;
  tableStyle.m_tableCellStyle.m_borders.resize(1);
  tableStyle.m_tableCellStyle.m_contentFormat.m_textStyleHandle = 0x108u;
  tableStyle.m_tableCellStyle.m_contentFormat.m_textHeight = 2.5;
  tableStyle.m_tableCellStyle.m_contentFormat.m_cellAlignment = 5;
  tableStyle.m_tableCellStyle.m_contentFormat.m_contentColor = 7;
  tableStyle.m_tableCellStyle.m_verticalMargin = 0.25;
  tableStyle.m_tableCellStyle.m_borders[0].m_lineTypeHandle = 0x109u;
  tableStyle.m_tableCellStyle.m_borders[0].m_visible = 1;
  tableStyle.m_tableCellStyle.m_borders[0].m_color = 11;
  DRW_TableStyleRowStyle rowStyle;
  rowStyle.m_borders.resize(2);
  rowStyle.m_textStyleHandle = 0x10Au;
  rowStyle.m_textHeight = 3.0;
  rowStyle.m_textAlignment = 4;
  rowStyle.m_textColor = 12;
  rowStyle.m_fillColor = 13;
  rowStyle.m_borders[0].m_lineTypeHandle = 0x10Bu;
  rowStyle.m_borders[0].m_visible = 1;
  rowStyle.m_borders[0].m_color = 14;
  tableStyle.m_rowStyles.push_back(rowStyle);
  DRW_TableStyleCellStyle cellStyle;
  cellStyle.m_borders.resize(3);
  cellStyle.m_id = 42;
  cellStyle.m_name = "DataStyle";
  cellStyle.m_backgroundColor = 9;
  cellStyle.m_contentFormat.m_textStyleHandle = 0x10Cu;
  cellStyle.m_contentFormat.m_textHeight = 1.75;
  cellStyle.m_contentFormat.m_cellAlignment = 6;
  cellStyle.m_contentFormat.m_contentColor = 10;
  cellStyle.m_horizontalMargin = 0.35;
  cellStyle.m_borders[0].m_lineTypeHandle = 0x10Du;
  cellStyle.m_borders[0].m_visible = 1;
  cellStyle.m_borders[0].m_color = 15;
  tableStyle.m_cellStyles.push_back(cellStyle);
  DRW_DwgSubrecordRange tableStyleRange;
  tableStyleRange.m_name = "table-style-cell-style";
  tableStyleRange.m_startBit = 0;
  tableStyleRange.m_bitSize = 32;
  tableStyleRange.m_version = DRW::AC1027;
  tableStyleRange.m_count = 1;
  tableStyle.m_subrecordRanges.push_back(tableStyleRange);
  metadata.addTableStyle(tableStyle);

  DRW_CellStyleMap cellStyleMap;
  cellStyleMap.handle = 0xF6u;
  cellStyleMap.parentHandle = 0xF7u;
  DRW_TableStyleCellStyle mapStyle;
  mapStyle.m_id = 77;
  mapStyle.m_styleClass = 3;
  mapStyle.m_name = "MappedDataStyle";
  mapStyle.m_backgroundColor = 21;
  mapStyle.m_horizontalMargin = 0.45;
  mapStyle.m_contentFormat.m_textStyleHandle = 0x110u;
  mapStyle.m_contentFormat.m_textHeight = 2.25;
  mapStyle.m_contentFormat.m_cellAlignment = 7;
  mapStyle.m_contentFormat.m_contentColor = 22;
  mapStyle.m_borders.resize(1);
  mapStyle.m_borders[0].m_lineTypeHandle = 0x111u;
  mapStyle.m_borders[0].m_visible = 1;
  mapStyle.m_borders[0].m_color = 23;
  cellStyleMap.m_cellStyles.push_back(mapStyle);
  DRW_DwgSubrecordRange cellStyleMapRange;
  cellStyleMapRange.m_name = "table-style-cell-style";
  cellStyleMapRange.m_startBit = 0;
  cellStyleMapRange.m_bitSize = 48;
  cellStyleMapRange.m_version = DRW::AC1027;
  cellStyleMapRange.m_count = 1;
  cellStyleMap.m_subrecordRanges.push_back(cellStyleMapRange);
  metadata.addCellStyleMap(cellStyleMap);

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
  DRW_DwgSubrecordRange tableContentFormatRange;
  tableContentFormatRange.m_name = "table-content-format";
  tableContentFormatRange.m_startBit = 0;
  tableContentFormatRange.m_bitSize = 64;
  tableContentFormatRange.m_version = DRW::AC1027;
  tableContentFormatRange.m_count = 1;
  tableContent.m_content.m_subrecordRanges.push_back(tableContentFormatRange);
  DRW_DwgSubrecordRange tableOverrideRange;
  tableOverrideRange.m_name = "table-cell-overrides";
  tableOverrideRange.m_startBit = 64;
  tableOverrideRange.m_bitSize = 24;
  tableOverrideRange.m_version = DRW::AC1027;
  tableOverrideRange.m_count = 1;
  tableContent.m_content.m_subrecordRanges.push_back(tableOverrideRange);
  DRW_DwgSubrecordRange tableGeometryRange;
  tableGeometryRange.m_name = "table-cell-geometry-tail";
  tableGeometryRange.m_startBit = 88;
  tableGeometryRange.m_bitSize = 40;
  tableGeometryRange.m_version = DRW::AC1027;
  tableGeometryRange.m_count = 1;
  tableContent.m_content.m_subrecordRanges.push_back(tableGeometryRange);
  DRW_DwgSubrecordRange tableBreakRange;
  tableBreakRange.m_name = "table-break-data";
  tableBreakRange.m_startBit = 128;
  tableBreakRange.m_bitSize = 16;
  tableBreakRange.m_version = DRW::AC1027;
  tableBreakRange.m_count = 1;
  tableBreakRange.m_parseComplete = false;
  tableContent.m_content.m_subrecordRanges.push_back(tableBreakRange);
  metadata.addTableContent(tableContent);

  DRW_Table fallbackTable;
  fallbackTable.handle = 0x112u;
  fallbackTable.parentHandle = 0x113u;
  fallbackTable.m_hasSemanticContent = true;
  fallbackTable.m_semanticContentComplete = true;
  fallbackTable.m_tableStyleHandle = 0xFCu;
  fallbackTable.m_content.m_tableStyleHandle = 0xFCu;
  fallbackTable.m_content.m_columns.resize(1);
  fallbackTable.m_content.m_columns[0].m_width = 8.0;
  fallbackTable.m_content.m_rows.resize(1);
  fallbackTable.m_content.m_rows[0].m_height = 3.0;
  fallbackTable.m_content.m_rows[0].m_cells.resize(1);
  DRW_TableCellContent fallbackTextContent;
  fallbackTextContent.m_type = 1;
  fallbackTextContent.m_text = "Fallback text";
  fallbackTable.m_content.m_rows[0].m_cells[0].m_contents.push_back(
      fallbackTextContent);
  metadata.addTable(fallbackTable, true);
  LC_DwgAdvancedMetadata::TableFallbackRenderSummary fallbackSummary;
  fallbackSummary.tableHandle = fallbackTable.handle;
  fallbackSummary.gridEntityCount = 1;
  fallbackSummary.textEntityCount = 1;
  fallbackSummary.placeholderEntityCount = 0;
  fallbackSummary.unresolvedTextStyleCount = 1;
  fallbackSummary.clampedDimensionCount = 2;
  metadata.updateTableFallbackRenderSummary(fallbackSummary);
  LC_DwgAdvancedMetadata::TableFallbackEntityRecord fallbackGridRecord;
  fallbackGridRecord.tableHandle = fallbackTable.handle;
  fallbackGridRecord.sourceHandle = fallbackTable.handle;
  fallbackGridRecord.column = 0;
  fallbackGridRecord.role =
      LC_DwgAdvancedMetadata::TableFallbackRole::GridLine;
  fallbackGridRecord.entityId = 9001u;
  metadata.addTableFallbackEntity(fallbackGridRecord);
  LC_DwgAdvancedMetadata::TableFallbackEntityRecord fallbackTextRecord;
  fallbackTextRecord.tableHandle = fallbackTable.handle;
  fallbackTextRecord.sourceHandle = fallbackTable.handle;
  fallbackTextRecord.row = 0;
  fallbackTextRecord.column = 0;
  fallbackTextRecord.role =
      LC_DwgAdvancedMetadata::TableFallbackRole::CellText;
  fallbackTextRecord.entityId = 9002u;
  metadata.addTableFallbackEntity(fallbackTextRecord);

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
  associativeObject.m_valueParamCount = 3u;
  associativeObject.m_ownedParamPrefixCount = 2u;
  associativeObject.m_valueParamsParsed = true;
  associativeObject.m_actionParamPrefixParsed = true;
  associativeObject.m_singleDependencyActionParamParsed = true;
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

  REQUIRE(metadata.rawObjects().size() == 4);
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
        LC_DwgAdvancedMetadata::ReplayBlocker::None);
  CHECK(LC_DwgAdvancedMetadata::isReplayableFixedModelerRawEntity(
            metadata.rawObjects()[2]));
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[3]) ==
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
  const auto viewsByVisualStyle = metadata.findViewsReferencingHandle(0x94u);
  REQUIRE(viewsByVisualStyle.size() == 1u);
  CHECK(viewsByVisualStyle.front() == foundView);

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

  REQUIRE(metadata.tables().size() == 3);
  const auto& capturedTableStyle = metadata.tables().front();
  CHECK(capturedTableStyle.handle == 0xFCu);
  CHECK(capturedTableStyle.recordName == "SemanticTableStyle");
  CHECK(capturedTableStyle.rowStyleCount == 1u);
  CHECK(capturedTableStyle.cellStyleCount == 1u);
  CHECK(capturedTableStyle.borderCount == 6u);
  CHECK(capturedTableStyle.m_tableFlowDirection == 1);
  CHECK(capturedTableStyle.m_tableFlags == 0x24);
  CHECK(capturedTableStyle.m_tableHorizontalCellMargin == 0.75);
  CHECK(capturedTableStyle.m_tableVerticalCellMargin == 0.5);
  CHECK(capturedTableStyle.m_tableContentFormatCount == 2u);
  CHECK(capturedTableStyle.m_tableNamedCellStyleCount == 1u);
  CHECK(capturedTableStyle.m_tableVisibleBorderCount == 3u);
  CHECK(capturedTableStyle.m_tableMarginStyleCount == 2u);
  CHECK(capturedTableStyle.m_unknownRangeCount == 1u);
  CHECK(capturedTableStyle.m_incompleteRangeCount == 0u);
  CHECK(capturedTableStyle.textStyleHandleCount == 3u);
  CHECK(capturedTableStyle.lineTypeHandleCount == 3u);
  REQUIRE(capturedTableStyle.m_tableStyleIds.size() == 1u);
  CHECK(capturedTableStyle.m_tableStyleIds.front() == 42);
  REQUIRE(capturedTableStyle.m_tableStyleNames.size() == 1u);
  CHECK(capturedTableStyle.m_tableStyleNames.front() == "DataStyle");
  REQUIRE(capturedTableStyle.m_tableTextHeights.size() == 3u);
  CHECK(capturedTableStyle.m_tableTextHeights[0] == 2.5);
  CHECK(capturedTableStyle.m_tableTextHeights[1] == 3.0);
  CHECK(capturedTableStyle.m_tableTextHeights[2] == 1.75);
  REQUIRE(capturedTableStyle.m_tableAlignments.size() == 3u);
  CHECK(capturedTableStyle.m_tableAlignments[0] == 5);
  CHECK(capturedTableStyle.m_tableAlignments[1] == 4);
  CHECK(capturedTableStyle.m_tableAlignments[2] == 6);
  CHECK(capturedTableStyle.m_tableColors.size() >= 7u);
  CHECK(capturedTableStyle.titleSuppressed);
  CHECK_FALSE(capturedTableStyle.headerSuppressed);
  CHECK(capturedTableStyle.styleResolved);
  CHECK(metadata.findTableStyleByHandle(0xFCu) == &capturedTableStyle);
  const auto tableStylesByCellStyle =
      metadata.findTableStylesReferencingCellStyle(42);
  REQUIRE(tableStylesByCellStyle.size() == 1u);
  CHECK(tableStylesByCellStyle.front() == &capturedTableStyle);
  const auto tableStylesByTextStyle =
      metadata.findTableStylesReferencingHandle(0x10Cu);
  REQUIRE(tableStylesByTextStyle.size() == 1u);
  CHECK(tableStylesByTextStyle.front() == &capturedTableStyle);
  const auto tableStylesByLineType =
      metadata.findTableStylesReferencingHandle(0x10Du);
  REQUIRE(tableStylesByLineType.size() == 1u);
  CHECK(tableStylesByLineType.front() == &capturedTableStyle);

  const auto& capturedTableContent = metadata.tables().at(1);
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
  CHECK(capturedTableContent.m_unknownRangeCount == 4u);
  CHECK(capturedTableContent.m_incompleteRangeCount == 1u);
  CHECK(capturedTableContent.m_overrideMaskCount == 1u);
  CHECK(capturedTableContent.m_breakRangeCount == 1u);
  CHECK(capturedTableContent.m_tableGeometryTailRangeCount == 1u);
  CHECK(capturedTableContent.hasTextContent);
  CHECK(capturedTableContent.hasBlockContent);
  CHECK(capturedTableContent.semanticParsed);
  CHECK(capturedTableContent.styleResolved);
  CHECK(metadata.findTableByHandle(0xFEu) == &capturedTableContent);
  const auto tablesUsingStyle = metadata.findTablesUsingStyle(0xFCu);
  REQUIRE(tablesUsingStyle.size() == 2u);
  CHECK(tablesUsingStyle[0] == &capturedTableContent);
  const auto tablesByStyleHandle = metadata.findTablesReferencingHandle(0xFCu);
  REQUIRE(tablesByStyleHandle.size() == 2u);
  CHECK(tablesByStyleHandle[0] == &capturedTableContent);
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

  REQUIRE(metadata.cellStyleMaps().size() == 1u);
  const auto& capturedCellStyleMap = metadata.cellStyleMaps().front();
  CHECK(capturedCellStyleMap.handle == 0xF6u);
  CHECK(capturedCellStyleMap.parentHandle == 0xF7u);
  CHECK(capturedCellStyleMap.m_cellStyleCount == 1u);
  CHECK(capturedCellStyleMap.m_borderCount == 1u);
  CHECK(capturedCellStyleMap.m_contentFormatCount == 1u);
  CHECK(capturedCellStyleMap.m_namedCellStyleCount == 1u);
  CHECK(capturedCellStyleMap.m_visibleBorderCount == 1u);
  CHECK(capturedCellStyleMap.m_marginStyleCount == 1u);
  CHECK(capturedCellStyleMap.m_unknownRangeCount == 1u);
  CHECK(capturedCellStyleMap.m_incompleteRangeCount == 0u);
  REQUIRE(capturedCellStyleMap.m_styleIds.size() == 1u);
  CHECK(capturedCellStyleMap.m_styleIds.front() == 77);
  REQUIRE(capturedCellStyleMap.m_styleClasses.size() == 1u);
  CHECK(capturedCellStyleMap.m_styleClasses.front() == 3);
  REQUIRE(capturedCellStyleMap.m_styleNames.size() == 1u);
  CHECK(capturedCellStyleMap.m_styleNames.front() == "MappedDataStyle");
  REQUIRE(capturedCellStyleMap.m_textStyleHandles.size() == 1u);
  CHECK(capturedCellStyleMap.m_textStyleHandles.front() == 0x110u);
  REQUIRE(capturedCellStyleMap.m_lineTypeHandles.size() == 1u);
  CHECK(capturedCellStyleMap.m_lineTypeHandles.front() == 0x111u);
  CHECK(metadata.findCellStyleMapByHandle(0xF6u) == &capturedCellStyleMap);
  const auto mapsByStyleId = metadata.findCellStylesById(77);
  REQUIRE(mapsByStyleId.size() == 1u);
  CHECK(mapsByStyleId.front() == &capturedCellStyleMap);
  const auto mapsByTextStyle =
      metadata.findCellStyleMapsReferencingHandle(0x110u);
  REQUIRE(mapsByTextStyle.size() == 1u);
  CHECK(mapsByTextStyle.front() == &capturedCellStyleMap);

  const auto& capturedFallbackTable = metadata.tables().at(2);
  CHECK(capturedFallbackTable.handle == 0x112u);
  CHECK(capturedFallbackTable.fallbackRendered);
  CHECK(capturedFallbackTable.m_fallbackGridEntityCount == 1u);
  CHECK(capturedFallbackTable.m_fallbackTextEntityCount == 1u);
  CHECK(capturedFallbackTable.m_fallbackPlaceholderEntityCount == 0u);
  CHECK(capturedFallbackTable.m_fallbackUnresolvedTextStyleCount == 1u);
  CHECK(capturedFallbackTable.m_fallbackClampedDimensionCount == 2u);
  CHECK(capturedFallbackTable.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayReplaced);
  const auto fallbackRecords =
      metadata.findTableFallbackEntities(capturedFallbackTable.handle);
  REQUIRE(fallbackRecords.size() == 2u);
  CHECK(fallbackRecords[0]->role ==
        LC_DwgAdvancedMetadata::TableFallbackRole::GridLine);
  CHECK(fallbackRecords[1]->role ==
        LC_DwgAdvancedMetadata::TableFallbackRole::CellText);
  CHECK(metadata.findTableByFallbackEntityId(9002u) == &capturedFallbackTable);

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
  CHECK(tableBlockers.tableCount == 2u);
  CHECK(tableBlockers.fallbackRendered == 1u);
  CHECK(tableBlockers.fieldContent == 1u);
  CHECK(tableBlockers.blockContent == 1u);
  CHECK(tableBlockers.attributeContent == 1u);
  CHECK(tableBlockers.overrideCells == 1u);
  CHECK(tableBlockers.geometryCells == 1u);
  CHECK(tableBlockers.unknownRanges == 1u);
  CHECK(tableBlockers.incompleteRanges == 1u);
  CHECK(tableBlockers.overrideMasks == 1u);
  CHECK(tableBlockers.breakRanges == 1u);
  CHECK(tableBlockers.tableGeometryTailRanges == 1u);
  CHECK(tableBlockers.editedFallbackEntities == 0u);
  CHECK(tableBlockers.missingFallbackAttachments == 0u);
  CHECK(tableBlockers.totalBlockers() == 11u);
  CHECK(capturedTableContent.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);
  REQUIRE(metadata.invalidateTableForFallbackEntity(9002u));
  CHECK(capturedFallbackTable.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  CHECK(fallbackRecords[1]->replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  const LC_DwgAdvancedMetadata::TableWriterBlockerCounts editedTableBlockers =
      metadata.tableWriterBlockerCounts();
  CHECK(editedTableBlockers.editedFallbackEntities == 1u);
  CHECK(editedTableBlockers.totalBlockers() == 12u);
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
  CHECK(capturedModeler.markerOffset == 0u);
  CHECK(capturedModeler.markerLength == 15u);
  CHECK(capturedModeler.markerText == "ACIS BinaryFile");
  CHECK(capturedModeler.rawByteSplitKnown);
  CHECK_FALSE(capturedModeler.rawByteSplitConsistent);
  CHECK(capturedModeler.rawBodyByteCount == 15u);
  CHECK(capturedModeler.rawHandleByteCount == 0u);
  CHECK(capturedModeler.markerSection ==
        LC_DwgAdvancedMetadata::ModelerPayloadSection::Body);
  CHECK(std::string(LC_DwgAdvancedMetadata::modelerPayloadKindName(
            capturedModeler.payloadKind)) == "SAB");
  CHECK(std::string(LC_DwgAdvancedMetadata::modelerPayloadSectionName(
            capturedModeler.markerSection)) == "body");
  CHECK(capturedModeler.historyHandle == 0xFBu);
  CHECK(capturedModeler.rawByteCount == 15u);
  CHECK(capturedModeler.rawBytes.back() == 'e');
  CHECK(metadata.findModelerGeometryByHandle(0xF9u) == &capturedModeler);
  const auto modelerByHistory = metadata.findModelerGeometryByHistoryHandle(0xFBu);
  REQUIRE(modelerByHistory.size() == 1u);
  CHECK(modelerByHistory.front() == &capturedModeler);
  const LC_DwgAdvancedMetadata::ModelerPayloadCounts modelerPayloads =
      metadata.modelerPayloadCounts();
  CHECK(modelerPayloads.recordCount == 1u);
  CHECK(modelerPayloads.sab == 1u);
  CHECK(modelerPayloads.sat == 0u);
  CHECK(modelerPayloads.unknown == 0u);
  CHECK(modelerPayloads.inconsistentSplit == 1u);
  CHECK(modelerPayloads.markerInBody == 1u);
  CHECK(modelerPayloads.markerInHandleStream == 0u);

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
  CHECK(capturedAssoc.valueParamCount == 3u);
  CHECK(capturedAssoc.ownedParamPrefixCount == 2u);
  CHECK(capturedAssoc.valueParamsParsed);
  CHECK(capturedAssoc.actionParamPrefixParsed);
  CHECK(capturedAssoc.singleDependencyActionParamParsed);
  CHECK_FALSE(capturedAssoc.compoundActionParamParsed);
  const LC_DwgAdvancedMetadata::AssociativeShellCounts associativeShells =
      metadata.associativeShellCounts();
  CHECK(associativeShells.recordCount == 1u);
  CHECK(associativeShells.vertexActionParam == 1u);
  CHECK(associativeShells.valueParamRecords == 1u);
  CHECK(associativeShells.parsedValueParamRecords == 1u);
  CHECK(associativeShells.actionParamRecords == 1u);
  CHECK(associativeShells.parsedActionParamPrefixes == 1u);
  CHECK(associativeShells.singleDependencyActionParamPrefixes == 1u);
  CHECK(associativeShells.compoundActionParamPrefixes == 0u);
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

TEST_CASE("DWG table native writer eligibility follows ODA layout gates",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_TableStyle tableStyle;
  tableStyle.handle = 0x200u;
  tableStyle.parentHandle = 0x201u;
  tableStyle.m_name = "TableStyle";
  metadata.addTableStyle(tableStyle);

  DRW_Table cleanTable;
  cleanTable.handle = 0x210u;
  cleanTable.parentHandle = 0x211u;
  cleanTable.m_hasSemanticContent = true;
  cleanTable.m_semanticContentComplete = true;
  cleanTable.m_tableStyleHandle = tableStyle.handle;
  cleanTable.m_content.m_columns.resize(1);
  cleanTable.m_content.m_columns[0].m_width = 6.0;
  cleanTable.m_content.m_rows.resize(1);
  cleanTable.m_content.m_rows[0].m_height = 2.0;
  cleanTable.m_content.m_rows[0].m_cells.resize(1);
  DRW_TableCellContent cleanText;
  cleanText.m_type = 1;
  cleanText.m_text = "plain";
  cleanTable.m_content.m_rows[0].m_cells[0].m_contents.push_back(cleanText);
  metadata.addTable(cleanTable, false);

  const auto embeddedEligibility =
      metadata.tableNativeWriterEligibility(cleanTable.handle, DRW::AC1024);
  CHECK(embeddedEligibility.tableHandle == cleanTable.handle);
  CHECK(embeddedEligibility.recordName == "ACAD_TABLE");
  CHECK(embeddedEligibility.writerVersion == DRW::AC1024);
  CHECK(embeddedEligibility.storageMode ==
        LC_DwgAdvancedMetadata::TableContentStorageMode::EmbeddedTableContent);
  CHECK(embeddedEligibility.eligibleTextOnly);
  CHECK(embeddedEligibility.blockers.empty());

  const auto embeddedCounts =
      metadata.tableNativeWriterBlockerCounts(DRW::AC1024);
  CHECK(embeddedCounts.tableCount == 1u);
  CHECK(embeddedCounts.eligibleTextOnly == 1u);
  CHECK(embeddedCounts.embeddedTableContentLayout == 1u);
  CHECK(embeddedCounts.totalBlockers() == 0u);

  const auto separateEligibility =
      metadata.tableNativeWriterEligibility(cleanTable.handle, DRW::AC1021);
  CHECK(separateEligibility.storageMode ==
        LC_DwgAdvancedMetadata::TableContentStorageMode::SeparateTableContent);
  CHECK_FALSE(separateEligibility.eligibleTextOnly);
  CHECK(separateEligibility.hasBlocker(
      LC_DwgAdvancedMetadata::TableNativeWriterBlocker::
          AmbiguousTableContentStorage));

  const auto legacyEligibility =
      metadata.tableNativeWriterEligibility(cleanTable.handle, DRW::AC1018);
  CHECK(legacyEligibility.storageMode ==
        LC_DwgAdvancedMetadata::TableContentStorageMode::LegacyDirectTable);
  CHECK_FALSE(legacyEligibility.eligibleTextOnly);
  CHECK(legacyEligibility.hasBlocker(
      LC_DwgAdvancedMetadata::TableNativeWriterBlocker::
          UnsupportedTableVersion));
}

TEST_CASE("DWG table native writer eligibility reports blocker buckets",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_Table blockedTable;
  blockedTable.handle = 0x310u;
  blockedTable.parentHandle = 0;
  blockedTable.m_hasSemanticContent = true;
  blockedTable.m_semanticContentComplete = true;
  blockedTable.m_tableStyleHandle = 0xDEADu;
  blockedTable.m_content.m_columns.resize(1);
  blockedTable.m_content.m_columns[0].m_width = 0.0;
  blockedTable.m_content.m_rows.resize(1);
  blockedTable.m_content.m_rows[0].m_height = -1.0;
  blockedTable.m_content.m_rows[0].m_cells.resize(1);
  blockedTable.m_content.m_mergedRanges.push_back({0, 0, 0, 0});
  DRW_TableCell& blockedCell = blockedTable.m_content.m_rows[0].m_cells[0];
  blockedCell.m_styleId = 99;
  blockedCell.m_overrideFlags = 0x1u;
  blockedCell.m_valueHandle = 0x320u;
  blockedCell.m_textStyleHandle = 0x321u;
  blockedCell.m_geometryFlags = 0x1u;
  blockedCell.m_geometryHandle = 0x322u;
  blockedCell.m_isMerged = true;
  blockedCell.m_attributes.push_back({0x323u, 1, "A"});
  DRW_TableCellContent fieldContent;
  fieldContent.m_type = 2;
  fieldContent.m_handle = 0x324u;
  blockedCell.m_contents.push_back(fieldContent);
  DRW_TableCellContent blockContent;
  blockContent.m_type = 4;
  blockContent.m_handle = 0x325u;
  blockedCell.m_contents.push_back(blockContent);
  DRW_TableCellContent unknownContent;
  unknownContent.m_type = 77;
  blockedCell.m_contents.push_back(unknownContent);
  DRW_DwgSubrecordRange overrideRange;
  overrideRange.m_name = "table-cell-overrides";
  overrideRange.m_count = 1;
  blockedTable.m_content.m_subrecordRanges.push_back(overrideRange);
  DRW_DwgSubrecordRange breakRange;
  breakRange.m_name = "table-break-data";
  breakRange.m_count = 1;
  breakRange.m_parseComplete = false;
  blockedTable.m_content.m_subrecordRanges.push_back(breakRange);
  DRW_DwgSubrecordRange geometryRange;
  geometryRange.m_name = "table-cell-geometry-tail";
  geometryRange.m_count = 1;
  blockedTable.m_content.m_subrecordRanges.push_back(geometryRange);
  metadata.addTable(blockedTable, true);

  DRW_TableStyle resolvedStyle;
  resolvedStyle.handle = 0x400u;
  resolvedStyle.parentHandle = 0x401u;
  resolvedStyle.m_name = "ResolvedStyle";
  metadata.addTableStyle(resolvedStyle);

  DRW_Table editedFallbackTable;
  editedFallbackTable.handle = 0x410u;
  editedFallbackTable.parentHandle = 0x411u;
  editedFallbackTable.m_hasSemanticContent = true;
  editedFallbackTable.m_semanticContentComplete = true;
  editedFallbackTable.m_tableStyleHandle = resolvedStyle.handle;
  editedFallbackTable.m_content.m_columns.resize(1);
  editedFallbackTable.m_content.m_columns[0].m_width = 3.0;
  editedFallbackTable.m_content.m_rows.resize(1);
  editedFallbackTable.m_content.m_rows[0].m_height = 1.0;
  editedFallbackTable.m_content.m_rows[0].m_cells.resize(1);
  DRW_TableCellContent editedText;
  editedText.m_type = 1;
  editedText.m_text = "fallback";
  editedFallbackTable.m_content.m_rows[0].m_cells[0].m_contents.push_back(
      editedText);
  metadata.addTable(editedFallbackTable, true);
  LC_DwgAdvancedMetadata::TableFallbackEntityRecord fallbackRecord;
  fallbackRecord.tableHandle = editedFallbackTable.handle;
  fallbackRecord.sourceHandle = editedFallbackTable.handle;
  fallbackRecord.entityId = 0xFEEDu;
  fallbackRecord.role = LC_DwgAdvancedMetadata::TableFallbackRole::CellText;
  metadata.addTableFallbackEntity(fallbackRecord);
  REQUIRE(metadata.invalidateTableForFallbackEntity(0xFEEDu));

  const auto blockers =
      metadata.tableNativeWriterBlockerCounts(DRW::AC1024);
  CHECK(blockers.tableCount == 2u);
  CHECK(blockers.embeddedTableContentLayout == 2u);
  CHECK(blockers.eligibleTextOnly == 0u);
  CHECK(blockers.missingOwnerHandle == 1u);
  CHECK(blockers.unresolvedTableStyle == 1u);
  CHECK(blockers.unresolvedCellStyleMap == 1u);
  CHECK(blockers.unknownSubrecordRange == 1u);
  CHECK(blockers.incompleteSubrecordRange == 1u);
  CHECK(blockers.overrideMask == 1u);
  CHECK(blockers.breakData == 1u);
  CHECK(blockers.geometryTail == 1u);
  CHECK(blockers.mergedCell == 1u);
  CHECK(blockers.fieldContent == 1u);
  CHECK(blockers.blockContent == 1u);
  CHECK(blockers.attributeContent == 1u);
  CHECK(blockers.unknownCellContent == 1u);
  CHECK(blockers.incompleteValuePayload == 1u);
  CHECK(blockers.missingFallbackAttachment == 1u);
  CHECK(blockers.anonymousBlockPolicyUnresolved == 2u);
  CHECK(blockers.unresolvedTextStyle == 1u);
  CHECK(blockers.rawReplayInvalidated == 1u);
  CHECK(blockers.rawReplayReplaced == 1u);
  CHECK(blockers.nonPositiveDimension == 1u);
  CHECK(blockers.countFor(
            LC_DwgAdvancedMetadata::TableNativeWriterBlocker::EditedFallback)
        == 1u);

  const auto blockedEligibility =
      metadata.tableNativeWriterEligibility(blockedTable.handle, DRW::AC1024);
  CHECK_FALSE(blockedEligibility.eligibleTextOnly);
  CHECK(blockedEligibility.hasBlocker(
      LC_DwgAdvancedMetadata::TableNativeWriterBlocker::
          MissingFallbackAttachment));
  CHECK(blockedEligibility.hasBlocker(
      LC_DwgAdvancedMetadata::TableNativeWriterBlocker::UnknownCellContent));
  CHECK(blockedEligibility.hasBlocker(
      LC_DwgAdvancedMetadata::TableNativeWriterBlocker::
          IncompleteValuePayload));
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

TEST_CASE("DWG advanced metadata invalidates VIEW dependencies",
          "[entity_metadata][dwg_metadata][view]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_View view;
  view.handle = 0x480u;
  view.namedUCS_ID = 0x481u;
  view.baseUCS_ID = 0x482u;
  view.m_backgroundHandle = 0x483u;
  view.m_visualStyleHandle = 0x484u;
  view.m_sunHandle = 0x485u;
  view.m_liveSectionHandle = 0x486u;
  metadata.addView(view);

  CHECK(metadata.findViewsReferencingHandle(0x481u).size() == 1u);
  CHECK(metadata.findViewsReferencingHandle(0x482u).size() == 1u);
  CHECK(metadata.findViewsReferencingHandle(0x484u).size() == 1u);
  CHECK(metadata.findViewsReferencingHandle(0x485u).size() == 1u);
  CHECK(metadata.findViewsReferencingHandle(0xDEADBEEFu).empty());

  metadata.invalidateViewGraphForHandle(0x484u);

  REQUIRE(metadata.views().size() == 1u);
  CHECK(metadata.views().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
}

TEST_CASE("DWG advanced metadata maps VIEW UCS and VPORT document items",
          "[entity_metadata][dwg_metadata][view]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_View view;
  view.name = "Plan View";
  view.handle = 0x510u;
  view.parentHandle = 0x501u;
  view.hasUCS = true;
  view.namedUCS_ID = 0x520u;
  view.baseUCS_ID = 0x521u;
  view.m_backgroundHandle = 0x530u;
  view.m_visualStyleHandle = 0x531u;
  view.m_sunHandle = 0x540u;
  view.m_liveSectionHandle = 0x532u;
  metadata.addView(view);

  DRW_UCS ucs;
  ucs.name = "Plan UCS";
  ucs.handle = 0x520u;
  ucs.parentHandle = 0x502u;
  ucs.origin = DRW_Coord{1.0, 2.0, 0.0};
  ucs.xAxisDirection = DRW_Coord{1.0, 0.0, 0.0};
  ucs.yAxisDirection = DRW_Coord{0.0, 1.0, 0.0};
  metadata.addUcs(ucs);

  DRW_Sun sun;
  sun.handle = 0x540u;
  sun.parentHandle = 0x503u;
  metadata.addSun(sun);

  DRW_Vport vport;
  vport.name = "*ACTIVE";
  vport.handle = 0x550u;
  vport.parentHandle = 0x504u;
  vport.backgroundHandle = 0x530u;
  vport.visualStyleHandle = 0x531u;
  vport.m_sunHandle = 0x540u;
  vport.namedUcsHandle = 0x520u;
  vport.baseUcsHandle = 0x521u;
  metadata.addVport(vport);

  metadata.mapViewToDocumentItem(view.handle, view.name, 2);
  metadata.mapUcsToDocumentItem(ucs.handle, ucs.name, 1);

  REQUIRE(metadata.ucsRecords().size() == 1u);
  CHECK(metadata.findUcsByHandle(0x520u)->name == "Plan UCS");
  REQUIRE(metadata.vports().size() == 1u);
  CHECK(metadata.findVportByHandle(0x550u)->visualStyleHandle == 0x531u);

  const auto* viewMapping =
      metadata.findDocumentMappingBySourceHandle(view.handle);
  REQUIRE(viewMapping != nullptr);
  CHECK(viewMapping->sourceType ==
        LC_DwgAdvancedMetadata::DocumentMappingSource::View);
  CHECK(viewMapping->documentItemIndex == 2);
  CHECK(viewMapping->documentItemName == "Plan View");
  CHECK(viewMapping->namedUcsHandle == 0x520u);
  CHECK(viewMapping->sunHandle == 0x540u);
  CHECK(viewMapping->unresolvedReferenceCount == 4u);

  const auto ucsMappings = metadata.findDocumentMappingsByName(
      LC_DwgAdvancedMetadata::DocumentMappingSource::Ucs, "Plan UCS");
  REQUIRE(ucsMappings.size() == 1u);
  CHECK(ucsMappings.front()->documentItemIndex == 1);

  CHECK(metadata.findDocumentMappingsByOwner(0x501u).size() == 1u);
  const auto counts = metadata.documentMappingCounts();
  CHECK(counts.viewMappings == 1u);
  CHECK(counts.ucsMappings == 1u);
  CHECK(counts.vportMappings == 1u);
  CHECK(counts.mappedDocumentItems == 2u);
  CHECK(counts.unresolvedReferences == 7u);

  CHECK(metadata.invalidateDocumentMappingByItem(
      LC_DwgAdvancedMetadata::DocumentMappingSource::View, "Plan View"));
  REQUIRE(metadata.findViewByHandle(0x510u) != nullptr);
  CHECK(metadata.findViewByHandle(0x510u)->replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.findDocumentMappingBySourceHandle(0x510u) != nullptr);
  CHECK(metadata.findDocumentMappingBySourceHandle(0x510u)->replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
}

TEST_CASE("DWG advanced metadata summarizes visual and light records",
          "[entity_metadata][dwg_metadata][view]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_View view;
  view.name = "Camera Summary";
  view.handle = 0x610u;
  view.parentHandle = 0x601u;
  view.m_backgroundHandle = 0x630u;
  view.m_visualStyleHandle = 0x620u;
  view.m_sunHandle = 0x640u;
  view.m_liveSectionHandle = 0x631u;
  metadata.addView(view);

  DRW_Vport vport;
  vport.name = "*ACTIVE";
  vport.handle = 0x611u;
  vport.parentHandle = 0x602u;
  vport.backgroundHandle = 0x632u;
  vport.visualStyleHandle = 0x620u;
  vport.m_sunHandle = 0x640u;
  metadata.addVport(vport);

  DRW_VisualStyle visualStyle;
  visualStyle.name = "Conceptual";
  visualStyle.desc = "Stub visual style";
  visualStyle.handle = 0x620u;
  visualStyle.parentHandle = 0x603u;
  visualStyle.type = 4u;
  metadata.addVisualStyle(visualStyle);

  DRW_Light light;
  light.handle = 0x650u;
  light.parentHandle = 0x604u;
  light.m_name = "Key";
  light.m_type = 2u;
  light.m_status = true;
  light.m_intensity = 3.25;
  light.m_color = 0x0A0B0Cu;
  metadata.addLight(light);

  DRW_Sun sun;
  sun.handle = 0x640u;
  sun.parentHandle = 0x605u;
  sun.m_isOn = true;
  sun.m_intensity = 1.5;
  sun.m_color = 0x00FFFFu;
  sun.m_julianDay = 2451545;
  sun.m_milliseconds = 43200000;
  metadata.addSun(sun);

  const auto summaries = metadata.visualMetadataSummaries();
  REQUIRE(summaries.size() == 5u);

  const auto ownerSummaries =
      metadata.findVisualSummariesByOwner(summaries, 0x601u);
  REQUIRE(ownerSummaries.size() == 1u);
  CHECK(ownerSummaries.front()->sourceType ==
        LC_DwgAdvancedMetadata::VisualMetadataSource::View);
  CHECK(ownerSummaries.front()->referencedVisualStyleHandle == 0x620u);
  CHECK(ownerSummaries.front()->referencedSunHandle == 0x640u);
  CHECK(ownerSummaries.front()->unresolvedReferenceCount == 2u);

  const auto styleSummary =
      std::find_if(summaries.begin(), summaries.end(), [](const auto& record) {
        return record.sourceType ==
               LC_DwgAdvancedMetadata::VisualMetadataSource::VisualStyle;
      });
  REQUIRE(styleSummary != summaries.end());
  CHECK(styleSummary->displayName == "Conceptual");
  CHECK(styleSummary->lightOrSunType == 4u);
  CHECK(styleSummary->specCoverage ==
        LC_DwgAdvancedMetadata::VisualMetadataSpecCoverage::RawOnly);

  const auto sunSummary =
      std::find_if(summaries.begin(), summaries.end(), [](const auto& record) {
        return record.sourceType ==
               LC_DwgAdvancedMetadata::VisualMetadataSource::Sun;
      });
  REQUIRE(sunSummary != summaries.end());
  CHECK(sunSummary->lightOrSunEnabled);
  CHECK(sunSummary->julianDay == 2451545);
  CHECK(sunSummary->milliseconds == 43200000);

  const auto counts = metadata.visualMetadataSummaryCounts();
  CHECK(counts.view == 1u);
  CHECK(counts.vport == 1u);
  CHECK(counts.visualStyle == 1u);
  CHECK(counts.light == 1u);
  CHECK(counts.sun == 1u);
  CHECK(counts.odaCovered == 2u);
  CHECK(counts.crossReferenceSourced == 2u);
  CHECK(counts.rawOnly == 1u);
  CHECK(counts.ownerMapped == 5u);
  CHECK(counts.unresolvedReferences == 3u);

  metadata.invalidateViewGraphForHandle(0x620u);
  const auto invalidatedCounts = metadata.visualMetadataSummaryCounts();
  CHECK(invalidatedCounts.invalidated == 2u);
}

TEST_CASE("DWG visual metadata reports export policy blockers",
          "[entity_metadata][dwg_metadata][view][dwg-write]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject rawView;
  rawView.m_objectType = 300;
  rawView.m_handle = 0x710u;
  rawView.m_recordName = "VIEW";
  rawView.m_className = "AcDbViewTableRecord";
  rawView.m_rawBytes = {0x01u, 0x02u, 0x03u};
  metadata.addUnsupportedObject(rawView);

  DRW_View view;
  view.name = "Visual Policy";
  view.handle = 0x710u;
  view.parentHandle = 0x701u;
  view.hasUCS = true;
  view.namedUCS_ID = 0x711u;
  view.baseUCS_ID = 0x712u;
  view.m_visualStyleHandle = 0x720u;
  view.m_sunHandle = 0x730u;
  view.m_backgroundHandle = 0x740u;
  view.m_liveSectionHandle = 0x741u;
  metadata.addView(view);

  DRW_VisualStyle visualStyle;
  visualStyle.name = "Unwritten Style";
  visualStyle.handle = 0x750u;
  metadata.addVisualStyle(visualStyle);

  auto eligibility =
      metadata.visualMetadataReplayEligibility(0x710u, DRW::AC1027);
  CHECK(eligibility.hasSemanticRecord);
  CHECK(eligibility.hasRawPayload);
  CHECK(eligibility.rawReplayable);
  CHECK(eligibility.rawBlocker == LC_DwgAdvancedMetadata::ReplayBlocker::None);

  const auto styleEligibility =
      metadata.visualMetadataReplayEligibility(0x750u, DRW::AC1027);
  CHECK(styleEligibility.hasSemanticRecord);
  CHECK_FALSE(styleEligibility.hasRawPayload);
  CHECK(styleEligibility.unsupportedNativeWriter);

  auto counts = metadata.visualMetadataWriterBlockerCounts(DRW::AC1027);
  CHECK(counts.recordCount == 2u);
  CHECK(counts.rawPayloads == 1u);
  CHECK(counts.replayableRawPayloads == 1u);
  CHECK(counts.unresolvedUcs == 1u);
  CHECK(counts.unresolvedBaseUcs == 1u);
  CHECK(counts.unresolvedVisualStyle == 1u);
  CHECK(counts.unresolvedSun == 1u);
  CHECK(counts.unresolvedBackground == 1u);
  CHECK(counts.unresolvedLiveSection == 1u);
  CHECK(counts.missingOwnerOrLayout == 1u);
  CHECK(counts.unsupportedVisualStyleWriter == 1u);

  metadata.invalidateViewGraphForHandle(0x720u);
  eligibility = metadata.visualMetadataReplayEligibility(0x710u, DRW::AC1027);
  CHECK_FALSE(eligibility.rawReplayable);
  CHECK(eligibility.rawBlocker ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);

  counts = metadata.visualMetadataWriterBlockerCounts(DRW::AC1027);
  CHECK(counts.replayableRawPayloads == 0u);
  CHECK(counts.suppressedRawPayloads == 1u);
  CHECK(counts.invalidatedRawPayload == 1u);
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

TEST_CASE("DWG advanced metadata invalidates CELLSTYLEMAP raw replay",
          "[entity_metadata][dwg_metadata][table]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject rawCellStyleMap;
  rawCellStyleMap.m_objectType = 515;
  rawCellStyleMap.m_handle = 0x460u;
  rawCellStyleMap.m_isCustomClass = true;
  rawCellStyleMap.m_recordName = "CELLSTYLEMAP";
  rawCellStyleMap.m_className = "AcDbCellStyleMap";
  rawCellStyleMap.m_rawBytes = {0x01u, 0x02u};
  metadata.addUnsupportedObject(rawCellStyleMap);

  DRW_CellStyleMap cellStyleMap;
  cellStyleMap.handle = 0x460u;
  DRW_TableStyleCellStyle style;
  style.m_id = 11;
  style.m_name = "MappedStyle";
  style.m_contentFormat.m_textStyleHandle = 0x461u;
  style.m_borders.resize(1);
  style.m_borders[0].m_lineTypeHandle = 0x462u;
  cellStyleMap.m_cellStyles.push_back(style);
  metadata.addCellStyleMap(cellStyleMap);

  const auto mapsByLineType = metadata.findCellStyleMapsReferencingHandle(0x462u);
  REQUIRE(mapsByLineType.size() == 1u);
  CHECK(mapsByLineType.front()->handle == 0x460u);

  metadata.invalidateTableGraphForHandle(0x462u);

  REQUIRE(metadata.cellStyleMaps().size() == 1u);
  CHECK(metadata.cellStyleMaps().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 1u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects().front()) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
}

TEST_CASE("DWG advanced metadata classifies modeler payload markers",
          "[entity_metadata][dwg_metadata][modeler]") {
  const LC_DwgAdvancedMetadata::ModelerPayloadMarker sabMarker =
      LC_DwgAdvancedMetadata::scanModelerPayloadMarker(
          {'x', 'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F',
           'i', 'l', 'e'});
  CHECK(sabMarker.kind == LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(sabMarker.offset == 1u);
  CHECK(sabMarker.length == 15u);
  CHECK(sabMarker.text == "ACIS BinaryFile");

  const LC_DwgAdvancedMetadata::ModelerPayloadMarker satMarker =
      LC_DwgAdvancedMetadata::scanModelerPayloadMarker(
          {'7', ' ', '0', ' ', '0', '\n', 'B', 'e', 'g', 'i', 'n', '-',
           'o', 'f', '-', 'A', 'C', 'I', 'S', '-', 'H', 'i', 's', 't',
           'o', 'r', 'y'});
  CHECK(satMarker.kind == LC_DwgAdvancedMetadata::ModelerPayloadKind::Sat);
  CHECK(satMarker.offset == 6u);
  CHECK(satMarker.length == 21u);
  CHECK(satMarker.text == "Begin-of-ACIS-History");

  const LC_DwgAdvancedMetadata::ModelerPayloadMarker unknownMarker =
      LC_DwgAdvancedMetadata::scanModelerPayloadMarker({0x01u, 0x02u});
  CHECK(unknownMarker.kind == LC_DwgAdvancedMetadata::ModelerPayloadKind::Unknown);
  CHECK(unknownMarker.offset == 0u);
  CHECK(unknownMarker.length == 0u);
  CHECK(unknownMarker.text.empty());
}

TEST_CASE("DWG advanced metadata indexes modeler raw byte splits",
          "[entity_metadata][dwg_metadata][modeler]") {
  const LC_DwgAdvancedMetadata::ModelerRawByteSplit unknownSplit =
      LC_DwgAdvancedMetadata::splitModelerRawBytes(0u, 12u);
  CHECK_FALSE(unknownSplit.known);
  CHECK(unknownSplit.consistent);
  CHECK(unknownSplit.bodyByteCount == 12u);
  CHECK(unknownSplit.handleByteCount == 0u);

  const LC_DwgAdvancedMetadata::ModelerRawByteSplit split =
      LC_DwgAdvancedMetadata::splitModelerRawBytes(64u, 12u);
  CHECK(split.known);
  CHECK(split.consistent);
  CHECK(split.bodyByteCount == 8u);
  CHECK(split.handleByteCount == 4u);

  const LC_DwgAdvancedMetadata::ModelerRawByteSplit inconsistentSplit =
      LC_DwgAdvancedMetadata::splitModelerRawBytes(128u, 12u);
  CHECK(inconsistentSplit.known);
  CHECK_FALSE(inconsistentSplit.consistent);
  CHECK(inconsistentSplit.bodyByteCount == 12u);
  CHECK(inconsistentSplit.handleByteCount == 0u);

  LC_DwgAdvancedMetadata metadata;
  DRW_ModelerGeometry modelerGeometry(DRW::BODY);
  modelerGeometry.handle = 0x4A0u;
  modelerGeometry.m_bodyBitSize = 64u;
  modelerGeometry.m_rawBytes = {'b', 'o', 'd', 'y', 'd', 'a',
                                't', 'a', 'A', 'C', 'I', 'S'};
  metadata.addModelerGeometry(modelerGeometry);

  REQUIRE(metadata.modelerGeometry().size() == 1u);
  const auto& record = metadata.modelerGeometry().front();
  CHECK(record.rawByteSplitKnown);
  CHECK(record.rawByteSplitConsistent);
  CHECK(record.rawBodyByteCount == 8u);
  CHECK(record.rawHandleByteCount == 4u);
  CHECK(record.markerSection ==
        LC_DwgAdvancedMetadata::ModelerPayloadSection::HandleStream);
  CHECK(std::string(LC_DwgAdvancedMetadata::modelerPayloadSectionName(
            record.markerSection)) == "handle-stream");

  const LC_DwgAdvancedMetadata::ModelerPayloadCounts counts =
      metadata.modelerPayloadCounts();
  CHECK(counts.recordCount == 1u);
  CHECK(counts.sat == 1u);
  CHECK(counts.sab == 0u);
  CHECK(counts.unknown == 0u);
  CHECK(counts.inconsistentSplit == 0u);
  CHECK(counts.markerInBody == 0u);
  CHECK(counts.markerInHandleStream == 1u);
  CHECK(counts.rangeCount == 2u);
  CHECK(counts.unknownTailRanges == 1u);
  CHECK(counts.handleStreamRanges == 1u);
}

TEST_CASE("DWG advanced metadata indexes modeler payload ranges",
          "[entity_metadata][dwg_metadata][modeler]") {
  const std::vector<std::uint8_t> sabTerminated = {
      'x', 'x', 'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y',
      'F', 'i', 'l', 'e', 0x01u, 'E', 'n', 'd', 0x0Eu, 0x02u, 'o',
      'f', 0x0Eu, 0x04u, 'A', 'C', 'I', 'S', 0x0Du, 0x04u, 'd',
      'a', 't', 'a', 't', 'a', 'i', 'l'};
  const std::vector<LC_DwgAdvancedMetadata::ModelerPayloadRangeRecord>
      sabRanges = LC_DwgAdvancedMetadata::scanModelerPayloadRanges(
          sabTerminated, 328u);
  REQUIRE(sabRanges.size() == 3u);
  CHECK(sabRanges[0].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::UnknownTail);
  CHECK(sabRanges[0].offset == 0u);
  CHECK(sabRanges[0].length == 2u);
  CHECK(sabRanges[1].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::Sab);
  CHECK(sabRanges[1].offset == 2u);
  CHECK(sabRanges[1].length == 35u);
  CHECK(sabRanges[1].markerText == "ACIS BinaryFile");
  CHECK(sabRanges[1].consistency ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeConsistency::Exact);
  CHECK(sabRanges[1].confidence ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeConfidence::Marker);
  CHECK(sabRanges[2].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::UnknownTail);
  CHECK(sabRanges[2].offset == 37u);
  CHECK(sabRanges[2].length == 4u);
  CHECK(std::string(LC_DwgAdvancedMetadata::modelerPayloadRangeKindName(
            sabRanges[1].kind)) == "SAB");
  CHECK(std::string(
            LC_DwgAdvancedMetadata::modelerPayloadRangeConsistencyName(
                sabRanges[1].consistency)) == "exact");

  const std::vector<LC_DwgAdvancedMetadata::ModelerPayloadRangeRecord>
      historyRanges = LC_DwgAdvancedMetadata::scanModelerPayloadRanges(
          {'h', 'd', 'r', 'B', 'e', 'g', 'i', 'n', '-', 'o', 'f', '-',
           'A', 'C', 'I', 'S', '-', 'H', 'i', 's', 't', 'o', 'r', 'y',
           '\n', 'd', 'a', 't', 'a'},
          0u);
  REQUIRE(historyRanges.size() == 2u);
  CHECK(historyRanges[0].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::UnknownTail);
  CHECK(historyRanges[1].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::History);
  CHECK(historyRanges[1].offset == 3u);
  CHECK(historyRanges[1].markerText == "Begin-of-ACIS-History");

  const std::vector<LC_DwgAdvancedMetadata::ModelerPayloadRangeRecord>
      truncatedRanges = LC_DwgAdvancedMetadata::scanModelerPayloadRanges(
          {'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F',
           'i', 'l', 'e'},
          256u);
  REQUIRE(truncatedRanges.size() == 1u);
  CHECK(truncatedRanges[0].kind ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeKind::Sab);
  CHECK(truncatedRanges[0].declaredByteSize == 32u);
  CHECK(truncatedRanges[0].length == 15u);
  CHECK(truncatedRanges[0].consistency ==
        LC_DwgAdvancedMetadata::ModelerPayloadRangeConsistency::Truncated);

  LC_DwgAdvancedMetadata metadata;
  DRW_ModelerGeometry modelerGeometry(DRW::E3DSOLID);
  modelerGeometry.handle = 0x5B0u;
  modelerGeometry.m_bodyBitSize = 328u;
  modelerGeometry.m_rawBytes = sabTerminated;
  metadata.addModelerGeometry(modelerGeometry);
  const LC_DwgAdvancedMetadata::ModelerPayloadCounts counts =
      metadata.modelerPayloadCounts();
  CHECK(counts.rangeCount == 3u);
  CHECK(counts.sabRanges == 1u);
  CHECK(counts.unknownTailRanges == 2u);
  CHECK(counts.exactRanges == 3u);
  CHECK(counts.truncatedRanges == 0u);
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

TEST_CASE("DWG advanced metadata stores associative prefix accounting",
          "[entity_metadata][dwg_metadata][assoc]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_AssociativeObject action("ACDBASSOCACTION");
  action.handle = 0x470u;
  action.m_valueParamCount = 4u;
  action.m_ownedParamPrefixCount = 2u;
  action.m_valueParamsParsed = true;
  DRW_AssociativePrefixStatus actionPrefix;
  actionPrefix.m_kind =
      DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
  actionPrefix.m_status =
      DRW_AssociativePrefixStatus::ParseStatus::Complete;
  actionPrefix.m_startBit = 16u;
  actionPrefix.m_bitSize = 80u;
  actionPrefix.m_classVersion = 2u;
  actionPrefix.m_decodedHandleCount = 3u;
  actionPrefix.m_decodedValueCount = 4u;
  actionPrefix.m_decodedCountValue = 4;
  actionPrefix.m_sourceAssumption = "ACadSharp/libreDWG";
  DRW_AssociativePrefixStatus networkPrefix;
  networkPrefix.m_kind =
      DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork;
  networkPrefix.m_status =
      DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow;
  networkPrefix.m_decodedCountValue = 100001;
  action.m_prefixStatuses = {actionPrefix, networkPrefix};
  metadata.addAssociativeObject(action);

  DRW_AssociativeObject osnap("ACDBASSOCOSNAPPOINTREFACTIONPARAM");
  osnap.handle = 0x471u;
  osnap.m_actionParamPrefixParsed = true;
  osnap.m_compoundActionParamParsed = true;
  DRW_AssociativePrefixStatus paramPrefix;
  paramPrefix.m_kind =
      DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam;
  paramPrefix.m_status =
      DRW_AssociativePrefixStatus::ParseStatus::Partial;
  paramPrefix.m_startBit = 24u;
  paramPrefix.m_bitSize = 12u;
  osnap.m_prefixStatuses = {paramPrefix};
  metadata.addAssociativeObject(osnap);

  DRW_AcShHistoryObject sweep("ACSH_SWEEP_CLASS");
  sweep.handle = 0x472u;
  DRW_AssociativePrefixStatus evalPrefix;
  evalPrefix.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr;
  evalPrefix.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
  evalPrefix.m_decodedHandleCount = 1u;
  evalPrefix.m_decodedValueCount = 1u;
  DRW_AssociativePrefixStatus shPrefix;
  shPrefix.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode;
  shPrefix.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
  DRW_AssociativePrefixStatus bodyPrefix;
  bodyPrefix.m_kind = DRW_AssociativePrefixStatus::Kind::AcShActionBody;
  bodyPrefix.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
  sweep.m_prefixStatuses = {evalPrefix, shPrefix, bodyPrefix};
  metadata.addAcShObject(sweep);

  REQUIRE(metadata.associativeObjects().size() == 2u);
  const auto& capturedAction = metadata.associativeObjects().front();
  CHECK(capturedAction.valueParamCount == 4u);
  CHECK(capturedAction.ownedParamPrefixCount == 2u);
  CHECK(capturedAction.valueParamsParsed);
  CHECK_FALSE(capturedAction.actionParamPrefixParsed);
  REQUIRE(capturedAction.prefixStatuses.size() == 2u);
  CHECK(capturedAction.prefixStatuses.front().kind ==
        LC_DwgAdvancedMetadata::AssociativePrefixKind::AcDbAssocAction);
  CHECK(capturedAction.prefixStatuses.front().status ==
        LC_DwgAdvancedMetadata::AssociativePrefixParseStatus::Complete);
  CHECK(capturedAction.prefixStatuses.front().startBit == 16u);
  CHECK(capturedAction.prefixStatuses.front().bitSize == 80u);
  CHECK(capturedAction.prefixStatuses.front().decodedValueCount == 4u);
  CHECK(capturedAction.prefixStatuses.front().sourceAssumption ==
        "ACadSharp/libreDWG");

  const auto& capturedOsnap = metadata.associativeObjects().back();
  CHECK(capturedOsnap.actionParamPrefixParsed);
  CHECK_FALSE(capturedOsnap.singleDependencyActionParamParsed);
  CHECK(capturedOsnap.compoundActionParamParsed);
  REQUIRE(capturedOsnap.prefixStatuses.size() == 1u);
  CHECK(capturedOsnap.prefixStatuses.front().status ==
        LC_DwgAdvancedMetadata::AssociativePrefixParseStatus::Partial);

  const LC_DwgAdvancedMetadata::AssociativeShellCounts counts =
      metadata.associativeShellCounts();
  CHECK(counts.recordCount == 2u);
  CHECK(counts.action == 1u);
  CHECK(counts.osnapPointRefActionParam == 1u);
  CHECK(LC_DwgAdvancedMetadata::associativeShellKindCount(
            counts, LC_DwgAdvancedMetadata::AssociativeKind::Action) == 1u);
  CHECK(LC_DwgAdvancedMetadata::associativeShellKindCount(
            counts,
            LC_DwgAdvancedMetadata::AssociativeKind::OsnapPointRefActionParam)
        == 1u);
  CHECK(counts.valueParamRecords == 1u);
  CHECK(counts.parsedValueParamRecords == 1u);
  CHECK(counts.actionParamRecords == 1u);
  CHECK(counts.parsedActionParamPrefixes == 1u);
  CHECK(counts.singleDependencyActionParamPrefixes == 0u);
  CHECK(counts.compoundActionParamPrefixes == 1u);

  const LC_DwgAdvancedMetadata::AssociativePrefixCounts prefixCounts =
      metadata.associativePrefixCounts();
  CHECK(prefixCounts.prefixCount == 6u);
  CHECK(prefixCounts.assocAction == 1u);
  CHECK(prefixCounts.assocNetwork == 1u);
  CHECK(prefixCounts.assocActionParam == 1u);
  CHECK(prefixCounts.evalExpr == 1u);
  CHECK(prefixCounts.shHistoryNode == 1u);
  CHECK(prefixCounts.shActionBody == 1u);
  CHECK(prefixCounts.complete == 4u);
  CHECK(prefixCounts.partial == 1u);
  CHECK(prefixCounts.boundedCountOverflow == 1u);
  CHECK(prefixCounts.decodedHandleCount == 4u);
  CHECK(prefixCounts.decodedValueCount == 5u);
  CHECK(std::string(LC_DwgAdvancedMetadata::associativePrefixKindName(
            LC_DwgAdvancedMetadata::AssociativePrefixKind::AcDbEvalExpr))
        == "AcDbEvalExpr");
  CHECK(std::string(
            LC_DwgAdvancedMetadata::associativePrefixParseStatusName(
                LC_DwgAdvancedMetadata::AssociativePrefixParseStatus::
                    BoundedCountOverflow)) == "bounded count overflow");
}

TEST_CASE("DWG advanced metadata indexes associative graph edges",
          "[entity_metadata][dwg_metadata][assoc]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_AssociativeObject dependency("ACDBASSOCDEPENDENCY");
  dependency.handle = 0x600u;
  dependency.m_readDependencyHandle = 0x610u;
  dependency.m_writeDependencyHandle = 0x611u;
  metadata.addAssociativeObject(dependency);

  DRW_AssociativeObject action("ACDBASSOCACTION");
  action.handle = 0x601u;
  action.m_dependencies = {{false, 0x600u}};
  action.m_actionBodyHandle = 0x612u;
  action.m_ownedParams = {0x613u};
  metadata.addAssociativeObject(action);

  DRW_AssociativeObject network("ACDBASSOCNETWORK");
  network.handle = 0x602u;
  network.m_actions = {{true, 0x601u}};
  network.m_ownedActions = {0x614u};
  metadata.addAssociativeObject(network);

  DRW_AcShHistoryObject history("ACSH_SWEEP_CLASS");
  history.handle = 0x603u;
  history.m_ownerHandle = 0x601u;
  metadata.addAcShObject(history);

  CHECK(metadata.findAssociativeEdgesTo(0x610u).size() == 1u);
  CHECK(metadata.findAssociativeEdgesTo(0x601u).size() == 2u);
  const auto actionEdges = metadata.findAssociativeEdgesFrom(0x601u);
  REQUIRE(actionEdges.size() == 3u);
  CHECK(actionEdges.front()->sourceRecordName == "ACDBASSOCACTION");

  const std::vector<std::uint32_t> closure =
      metadata.findAssociativeClosureFrom(0x610u, 8u);
  REQUIRE(closure.size() == 4u);
  CHECK(closure[0] == 0x600u);
  CHECK(closure[1] == 0x601u);
  CHECK(closure[2] == 0x602u);
  CHECK(closure[3] == 0x603u);

  const auto affected = metadata.findAssociativeRecordsAffectedBy(0x610u);
  REQUIRE(affected.size() == 3u);
  CHECK(affected[0]->handle == 0x600u);
  CHECK(affected[1]->handle == 0x601u);
  CHECK(affected[2]->handle == 0x602u);

  const LC_DwgAdvancedMetadata::AssociativeEdgeCounts counts =
      metadata.associativeEdgeCounts();
  CHECK(counts.edgeCount == 8u);
  CHECK(counts.ownsAction == 2u);
  CHECK(counts.ownsParameter == 1u);
  CHECK(counts.dependsOn == 1u);
  CHECK(counts.readDependency == 1u);
  CHECK(counts.writeDependency == 1u);
  CHECK(counts.actionBody == 1u);
  CHECK(counts.historyNode == 1u);
  CHECK(counts.explicitHandle == 7u);
  CHECK(counts.inferredFromClassLayout == 1u);
}

TEST_CASE("DWG advanced metadata invalidates associative graph closure",
          "[entity_metadata][dwg_metadata][raw-replay]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_AssociativeObject dependency("ACDBASSOCDEPENDENCY");
  dependency.handle = 0x620u;
  dependency.m_readDependencyHandle = 0x630u;
  metadata.addAssociativeObject(dependency);

  DRW_AssociativeObject action("ACDBASSOCACTION");
  action.handle = 0x621u;
  action.m_dependencies = {{false, 0x620u}};
  metadata.addAssociativeObject(action);

  DRW_UnsupportedObject rawDependency;
  rawDependency.m_handle = 0x620u;
  rawDependency.m_recordName = "ACDBASSOCDEPENDENCY";
  rawDependency.m_className = "AcDbAssocDependency";
  rawDependency.m_rawBytes = {0x01u};
  metadata.addUnsupportedObject(rawDependency);

  DRW_UnsupportedObject rawAction;
  rawAction.m_handle = 0x621u;
  rawAction.m_recordName = "ACDBASSOCACTION";
  rawAction.m_className = "AcDbAssocAction";
  rawAction.m_rawBytes = {0x02u};
  metadata.addUnsupportedObject(rawAction);

  metadata.invalidateAssociativeGraphForHandle(0x630u);

  REQUIRE(metadata.associativeObjects().size() == 2u);
  CHECK(metadata.associativeObjects()[0].replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  CHECK(metadata.associativeObjects()[1].replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  REQUIRE(metadata.rawObjects().size() == 2u);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[0]) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[1]) ==
        LC_DwgAdvancedMetadata::ReplayBlocker::Invalidated);

  const LC_DwgAdvancedMetadata::AssociativeEdgeCounts counts =
      metadata.associativeEdgeCounts();
  CHECK(counts.invalidated == 2u);
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

TEST_CASE("DWG advanced metadata stores typed object-context shells",
          "[entity_metadata][dwg_metadata][object-context]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_ObjectContextData textContext(
      "TEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Text);
  textContext.handle = 0x910u;
  textContext.parentHandle = 0x700u;
  textContext.m_classVersion = 4u;
  textContext.m_defaultFlag = true;
  textContext.m_scaleHandle = 0x701u;
  textContext.m_annotatedHandle = 0x700u;
  textContext.m_horizontalMode = 2u;
  textContext.m_rotation = 0.25;
  textContext.m_insertionPoint = DRW_Coord{1.0, 2.0, 0.0};
  textContext.m_alignmentPoint = DRW_Coord{3.0, 4.0, 0.0};
  metadata.addObjectContextData(textContext);

  DRW_ObjectContextData dimContext(
      "ALDIMOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::AlignedDimension);
  dimContext.handle = 0x920u;
  dimContext.parentHandle = 0x710u;
  dimContext.m_classVersion = 5u;
  dimContext.m_scaleHandle = 0x711u;
  dimContext.m_blockHandle = 0x712u;
  dimContext.m_definitionPoint = DRW_Coord{5.0, 6.0, 0.0};
  dimContext.m_isDefaultTextLocation = true;
  dimContext.m_dimTofl = true;
  dimContext.m_hasArrow2 = true;
  dimContext.m_flipArrow1 = true;
  dimContext.m_textRotation = 0.5;
  dimContext.m_overrideCode = 7u;
  metadata.addObjectContextData(dimContext);

  REQUIRE(metadata.objectContextData().size() == 2u);
  const auto &textRecord = metadata.objectContextData()[0];
  CHECK(textRecord.handle == 0x910u);
  CHECK(textRecord.parentHandle == 0x700u);
  CHECK(textRecord.recordName == "TEXTOBJECTCONTEXTDATA");
  CHECK(textRecord.kind == DRW_ObjectContextData::Kind::Text);
  CHECK(textRecord.classVersion == 4u);
  CHECK(textRecord.defaultFlag);
  CHECK(textRecord.scaleHandle == 0x701u);
  CHECK(textRecord.annotatedHandle == 0x700u);
  CHECK(textRecord.horizontalMode == 2);
  CHECK(textRecord.rotation == 0.25);
  CHECK(textRecord.insertionPoint.x == 1.0);
  CHECK(textRecord.alignmentPoint.y == 4.0);

  const auto &dimRecord = metadata.objectContextData()[1];
  CHECK(dimRecord.handle == 0x920u);
  CHECK(dimRecord.kind == DRW_ObjectContextData::Kind::AlignedDimension);
  CHECK(dimRecord.scaleHandle == 0x711u);
  CHECK(dimRecord.blockHandle == 0x712u);
  CHECK(dimRecord.definitionPoint.x == 5.0);
  CHECK(dimRecord.isDefaultTextLocation);
  CHECK(dimRecord.dimTofl);
  CHECK(dimRecord.hasArrow2);
  CHECK(dimRecord.flipArrow1);
  CHECK(dimRecord.textRotation == 0.5);
  CHECK(dimRecord.overrideCode == 7);

  metadata.invalidateByHandle(0x910u);
  REQUIRE(metadata.objectContextData().size() == 2u);
  CHECK(metadata.objectContextData()[0].replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  CHECK(metadata.objectContextData()[1].replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);

  metadata.clear();
  CHECK(metadata.objectContextData().empty());
}

TEST_CASE("DWG advanced metadata reports graph replay policy by family",
          "[entity_metadata][dwg_metadata][raw-replay][assoc]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject dimAssoc;
  dimAssoc.m_handle = 0x700u;
  dimAssoc.m_recordName = "DIMASSOC";
  dimAssoc.m_className = "AcDbDimAssoc";
  dimAssoc.m_rawBytes = {0x01u};
  metadata.addUnsupportedObject(dimAssoc);

  DRW_UnsupportedObject evaluationGraph;
  evaluationGraph.m_handle = 0x701u;
  evaluationGraph.m_recordName = "ACAD_EVALUATION_GRAPH";
  evaluationGraph.m_className = "AcDbEvalGraph";
  metadata.addUnsupportedObject(evaluationGraph);

  DRW_AssociativeObject dependency("ACDBASSOCDEPENDENCY");
  dependency.handle = 0x702u;
  dependency.m_readDependencyHandle = 0x730u;
  metadata.addAssociativeObject(dependency);

  DRW_UnsupportedObject rawDependency;
  rawDependency.m_handle = 0x702u;
  rawDependency.m_recordName = "ACDBASSOCDEPENDENCY";
  rawDependency.m_className = "AcDbAssocDependency";
  rawDependency.m_rawBytes = {0x02u};
  metadata.addUnsupportedObject(rawDependency);

  DRW_AssociativeObject semanticOnlyAction("ACDBASSOCACTION");
  semanticOnlyAction.handle = 0x703u;
  DRW_AssociativePrefixStatus partialPrefix;
  partialPrefix.m_kind =
      DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
  partialPrefix.m_status =
      DRW_AssociativePrefixStatus::ParseStatus::Partial;
  semanticOnlyAction.m_prefixStatuses = {partialPrefix};
  metadata.addAssociativeObject(semanticOnlyAction);

  DRW_UnsupportedObject dynamicBlock;
  dynamicBlock.m_handle = 0x704u;
  dynamicBlock.m_recordName = "BLOCKVISIBILITYPARAMETER";
  dynamicBlock.m_className = "AcDbBlockVisibilityParameter";
  dynamicBlock.m_rawBytes = {0x04u};
  metadata.addUnsupportedObject(dynamicBlock);

  DRW_UnsupportedObject objectContext;
  objectContext.m_handle = 0x705u;
  objectContext.m_recordName = "ACDB_BLKREFOBJECTCONTEXTDATA_CLASS";
  objectContext.m_className = "AcDbBlkRefObjectContextData";
  objectContext.m_rawBytes = {0x05u};
  metadata.addUnsupportedObject(objectContext);

  DRW_UnsupportedObject acshRaw;
  acshRaw.m_handle = 0x706u;
  acshRaw.m_recordName = "ACSH_SWEEP_CLASS";
  acshRaw.m_className = "AcShSweepClass";
  acshRaw.m_rawBytes = {0x06u};
  metadata.addUnsupportedObject(acshRaw);

  DRW_AcShHistoryObject semanticOnlyAcSh("ACSH_REVOLVE_CLASS");
  semanticOnlyAcSh.handle = 0x707u;
  metadata.addAcShObject(semanticOnlyAcSh);

  metadata.invalidateAssociativeGraphForHandle(0x730u);
  metadata.markRawReplayReplacedForHandle(0x706u);

  const LC_DwgAdvancedMetadata::GraphReplayPolicyCounts policy =
      metadata.graphReplayPolicyCounts();
  CHECK(policy.preserved.dimensionAssociation == 1u);
  CHECK(policy.preserved.dynamicBlock == 1u);
  CHECK(policy.preserved.objectContext == 1u);
  CHECK(policy.preserved.total() == 3u);
  CHECK(policy.suppressed.evaluationGraph == 1u);
  CHECK(policy.suppressed.acDbAssoc == 1u);
  CHECK(policy.suppressed.acShHistory == 1u);
  CHECK(policy.suppressed.total() == 3u);
  CHECK(policy.semanticOnlyAssociative == 1u);
  CHECK(policy.semanticOnlyAcSh == 1u);
  CHECK(policy.totalSemanticOnly() == 2u);
  CHECK(policy.invalidated == 1u);
  CHECK(policy.cyclePathInvalidated == 1u);
  CHECK(policy.replaced == 1u);
  CHECK(policy.nativeReplacement == 1u);
  CHECK(policy.missingRawBytes == 1u);
  CHECK(policy.missingTarget == 1u);
  CHECK(policy.parserPartial == 1u);
  CHECK(policy.totalReasons() == 4u);
  CHECK(LC_DwgAdvancedMetadata::graphReplayFamilyFromNames(
            "DIMASSOC", "AcDbDimAssoc")
        == LC_DwgAdvancedMetadata::GraphReplayFamily::DimensionAssociation);
  CHECK(std::string(LC_DwgAdvancedMetadata::graphReplayFamilyName(
            LC_DwgAdvancedMetadata::GraphReplayFamily::AcShHistory)) == "ACSH");
  CHECK(LC_DwgAdvancedMetadata::graphReplayFamilyCount(
            policy.suppressed,
            LC_DwgAdvancedMetadata::GraphReplayFamily::AcDbAssoc) == 1u);
}

TEST_CASE("DWG advanced metadata reports advanced entity writer readiness",
          "[entity_metadata][dwg_metadata][raw-replay]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject mesh;
  mesh.m_handle = 0x740u;
  mesh.m_isEntity = true;
  mesh.m_recordName = "MESH";
  mesh.m_className = "AcDbSubDMesh";
  mesh.m_rawBytes = {0x01u};
  metadata.addUnsupportedObject(mesh);

  DRW_UnsupportedObject shape;
  shape.m_handle = 0x741u;
  shape.m_isEntity = true;
  shape.m_recordName = "SHAPE";
  shape.m_className = "AcDbShape";
  metadata.addUnsupportedObject(shape);

  DRW_UnsupportedObject underlay;
  underlay.m_handle = 0x742u;
  underlay.m_isEntity = true;
  underlay.m_recordName = "PDFUNDERLAY";
  underlay.m_className = "AcDbPdfReference";
  underlay.m_rawBytes = {0x03u};
  metadata.addUnsupportedObject(underlay);

  DRW_UnsupportedObject unknownEntity;
  unknownEntity.m_handle = 0x743u;
  unknownEntity.m_isEntity = true;
  unknownEntity.m_isCustomClass = true;
  metadata.addUnsupportedObject(unknownEntity);

  DRW_MLeader blockLeader;
  blockLeader.handle = 0x744u;
  blockLeader.styleContentType = 1u;
  blockLeader.context.hasContentsBlock = true;
  DRW_MLeaderLeaderLine leaderLine;
  leaderLine.points = {DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 0.0, 0.0}};
  DRW_MLeaderRoot root;
  root.leaderLines.push_back(leaderLine);
  blockLeader.context.roots.push_back(root);
  metadata.addMLeader(blockLeader);

  const std::vector<LC_DwgAdvancedMetadata::AdvancedEntityWriterReadiness>
      ledger = metadata.advancedEntityWriterLedger(DRW::AC1024);
  REQUIRE(ledger.size() == 5u);
  CHECK(ledger.front().family ==
        LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::Mesh);
  CHECK(ledger.front().fallbackAvailable);
  CHECK(ledger.front().editedFallbackInvalidated);
  CHECK_FALSE(ledger.front().rawReplayAvailable);

  const LC_DwgAdvancedMetadata::AdvancedEntityWriterBlockerCounts counts =
      metadata.advancedEntityWriterBlockerCounts(DRW::AC1024);
  CHECK(counts.recordCount == 5u);
  CHECK(counts.mesh == 1u);
  CHECK(counts.shape == 1u);
  CHECK(counts.underlay == 1u);
  CHECK(counts.mleader == 1u);
  CHECK(counts.unknown == 1u);
  CHECK(counts.nativeWriterAvailable == 1u);
  CHECK(counts.rawReplayAvailable == 0u);
  CHECK(counts.fallbackAvailable == 3u);
  CHECK(counts.editedFallbackInvalidated == 4u);
  CHECK(counts.missingRequiredMetadata == 1u);
  CHECK(counts.missingPayloadBytes == 2u);
  CHECK(counts.unsupportedAdvancedContent == 4u);
  CHECK(counts.odaComplete == 2u);
  CHECK(counts.odaPartial == 2u);
  CHECK(counts.odaAbsent == 1u);
  CHECK(counts.totalBlockers() == 11u);
  CHECK(LC_DwgAdvancedMetadata::advancedEntityWriterFamilyFromNames(
            "ARC_DIMENSION", "AcDbArcDimension")
        == LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::ArcDimension);
  CHECK(std::string(LC_DwgAdvancedMetadata::advancedEntityWriterFamilyName(
            LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::Wipeout))
        == "WIPEOUT");
  CHECK(std::string(
            LC_DwgAdvancedMetadata::advancedEntityWriterOdaCoverageName(
                LC_DwgAdvancedMetadata::AdvancedEntityWriterOdaCoverage::
                    Partial)) == "partial");
  CHECK(LC_DwgAdvancedMetadata::advancedEntityWriterFamilyCount(
            counts,
            LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::MLeader) == 1u);
}

TEST_CASE("DWG mesh metadata preserves counts and sidecar lookup",
          "[entity_metadata][dwg_metadata][mesh]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_Polyline mesh;
  mesh.handle = 0x781u;
  mesh.parentHandle = 0x11u;
  mesh.flags = 0x10;
  mesh.vertexcount = 2;
  mesh.facecount = 3;
  mesh.smoothM = 4;
  mesh.smoothN = 5;
  mesh.curvetype = 0;
  for (int i = 0; i < mesh.vertexcount * mesh.facecount; ++i) {
    DRW_Vertex vertex(i, i + 1.0, i + 2.0, 0.0);
    vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Mesh);
    mesh.addVertex(vertex);
  }
  metadata.addMeshPolyline(mesh, true);

  LC_DwgAdvancedMetadata::MeshSidecarRecord rowSidecar;
  rowSidecar.sourceHandle = mesh.handle;
  rowSidecar.fallbackEntityId = 9001u;
  rowSidecar.meshId = "polyline_mesh_1921";
  rowSidecar.role = "row";
  rowSidecar.elementIndex = 0;
  rowSidecar.elementCount = 5;
  rowSidecar.roleIndex = 0;
  rowSidecar.flags = mesh.flags;
  rowSidecar.mCount = mesh.vertexcount;
  rowSidecar.nCount = mesh.facecount;
  rowSidecar.smoothM = mesh.smoothM;
  rowSidecar.smoothN = mesh.smoothN;
  rowSidecar.curveType = mesh.curvetype;
  rowSidecar.sourceVertexCount = mesh.vertlist.size();
  rowSidecar.anchor = true;
  metadata.addMeshSidecar(rowSidecar);

  LC_DwgAdvancedMetadata::MeshSidecarRecord columnSidecar = rowSidecar;
  columnSidecar.fallbackEntityId = 9002u;
  columnSidecar.role = "column";
  columnSidecar.elementIndex = 2;
  columnSidecar.roleIndex = 0;
  columnSidecar.sourceVertexCount = 0u;
  columnSidecar.anchor = false;
  metadata.addMeshSidecar(columnSidecar);

  REQUIRE(metadata.meshes().size() == 1u);
  const LC_DwgAdvancedMetadata::MeshRecord* record =
      metadata.findMeshByHandle(mesh.handle);
  REQUIRE(record != nullptr);
  CHECK(record->handle == mesh.handle);
  CHECK(record->parentHandle == mesh.parentHandle);
  CHECK(record->vertexCount == 2);
  CHECK(record->faceCount == 3);
  CHECK(record->preservedVertexCount == 6u);
  CHECK(record->smoothM == 4);
  CHECK(record->smoothN == 5);
  CHECK(record->rawRangeStatus ==
        LC_DwgAdvancedMetadata::MeshRawRangeStatus::Complete);
  CHECK(record->fallbackPreviewGenerated);

  const std::vector<const LC_DwgAdvancedMetadata::MeshSidecarRecord*>
      sourceSidecars = metadata.findMeshSidecarsBySourceHandle(mesh.handle);
  REQUIRE(sourceSidecars.size() == 2u);
  CHECK(sourceSidecars.front()->anchor);
  CHECK(sourceSidecars.front()->sourceVertexCount == 6u);
  const LC_DwgAdvancedMetadata::MeshSidecarRecord* fallbackSidecar =
      metadata.findMeshSidecarByFallbackEntityId(9002u);
  REQUIRE(fallbackSidecar != nullptr);
  CHECK(fallbackSidecar->role == "column");

  const LC_DwgAdvancedMetadata::MeshWriterBlockerCounts blockers =
      metadata.meshWriterBlockerCounts();
  CHECK(blockers.meshCount == 1u);
  CHECK(blockers.sidecarCount == 2u);
  CHECK(blockers.completeRawRange == 1u);
  CHECK(blockers.fallbackOnlyPreview == 1u);
  CHECK(blockers.malformedCountRelationships == 0u);
  CHECK(blockers.missingOwnerOrClassHandle == 0u);
}

TEST_CASE("DWG mesh writer blockers diagnose incomplete SubDMesh metadata",
          "[entity_metadata][dwg_metadata][mesh]") {
  LC_DwgAdvancedMetadata metadata;

  LC_DwgAdvancedMetadata::MeshRecord subDMesh;
  subDMesh.handle = 0x782u;
  subDMesh.recordName = "MESH";
  subDMesh.isSubDMesh = true;
  subDMesh.subdivisionLevel = 2;
  subDMesh.vertexCount = 4;
  subDMesh.faceCount = 2;
  subDMesh.edgeCount = 3;
  subDMesh.creaseCount = 1;
  subDMesh.preservedVertexCount = 3u;
  subDMesh.hasCreaseData = false;
  metadata.addMeshRecord(subDMesh);

  const LC_DwgAdvancedMetadata::MeshWriterBlockerCounts blockers =
      metadata.meshWriterBlockerCounts();
  CHECK(blockers.meshCount == 1u);
  CHECK(blockers.incompleteRawRange == 1u);
  CHECK(blockers.missingCreaseData == 1u);
  CHECK(blockers.unsupportedSubdivisionData == 1u);
  CHECK(blockers.missingOwnerOrClassHandle == 1u);
  CHECK(blockers.malformedCountRelationships == 1u);

  const std::vector<LC_DwgAdvancedMetadata::AdvancedEntityWriterReadiness>
      ledger = metadata.advancedEntityWriterLedger(DRW::AC1027);
  REQUIRE(ledger.size() == 1u);
  CHECK(ledger.front().family ==
        LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::Mesh);
  CHECK_FALSE(ledger.front().nativeWriterAvailable);
  CHECK(ledger.front().missingRequiredMetadata);
  CHECK(ledger.front().unsupportedAdvancedContent);
}

TEST_CASE("DWG external reference metadata tracks image and underlay links",
          "[entity_metadata][dwg_metadata][external-ref]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_RasterVariables rasterVariables;
  rasterVariables.handle = 0x790u;
  rasterVariables.m_classVersion = 1;
  rasterVariables.m_imageFrame = 2;
  rasterVariables.m_imageQuality = 1;
  rasterVariables.m_units = 5;
  metadata.addRasterVariables(rasterVariables);

  DRW_ImageDef imageDefinition;
  imageDefinition.handle = 0x791u;
  imageDefinition.parentHandle = 0x20u;
  imageDefinition.name = "textures/example.png";
  imageDefinition.imgVersion = 0;
  imageDefinition.u = 640.0;
  imageDefinition.v = 480.0;
  imageDefinition.up = 0.25;
  imageDefinition.vp = 0.25;
  imageDefinition.loaded = 1;
  imageDefinition.resolution = 5;
  metadata.addImageDefinition(imageDefinition);

  DRW_ImageDefinitionReactor reactor;
  reactor.handle = 0x792u;
  reactor.parentHandle = imageDefinition.handle;
  reactor.m_classVersion = 2;
  metadata.addImageDefinitionReactor(reactor);

  DRW_Image image;
  image.handle = 0x793u;
  image.parentHandle = 0x21u;
  image.ref = imageDefinition.handle;
  image.sizeu = 640.0;
  image.sizev = 480.0;
  image.clip = 1;
  image.clipMode = true;
  image.clipPath = {DRW_Coord{0.0, 0.0, 0.0},
                    DRW_Coord{1.0, 1.0, 0.0}};
  metadata.addRasterImage(image, false);

  DRW_Image malformedWipeout;
  malformedWipeout.handle = 0x794u;
  malformedWipeout.parentHandle = 0x21u;
  malformedWipeout.ref = 0;
  malformedWipeout.clip = 1;
  malformedWipeout.clipPath = {DRW_Coord{0.0, 0.0, 0.0}};
  metadata.addRasterImage(malformedWipeout, true);

  DRW_UnderlayDefinition underlayDefinition;
  underlayDefinition.handle = 0x795u;
  underlayDefinition.parentHandle = 0x20u;
  underlayDefinition.kind = DRW_UnderlayDefinition::PDF;
  underlayDefinition.filename = "/definitely/missing/librecad-test.pdf";
  underlayDefinition.sheetName = "Sheet";
  metadata.addUnderlayDefinition(underlayDefinition);

  DRW_Underlay underlay;
  underlay.handle = 0x796u;
  underlay.parentHandle = 0x21u;
  underlay.definitionHandle = underlayDefinition.handle;
  underlay.kind = DRW_Underlay::PDF;
  underlay.flags = 0;
  underlay.clipBoundary = {DRW_Coord{0.0, 0.0, 0.0},
                           DRW_Coord{1.0, 0.0, 0.0},
                           DRW_Coord{1.0, 1.0, 0.0}};
  metadata.addUnderlay(underlay, true);

  const LC_DwgAdvancedMetadata::ExternalReferenceCounts counts =
      metadata.externalReferenceCounts();
  CHECK(counts.imageEntities == 1u);
  CHECK(counts.wipeouts == 1u);
  CHECK(counts.imageDefinitions == 1u);
  CHECK(counts.underlays == 1u);
  CHECK(counts.underlayDefinitions == 1u);
  CHECK(counts.rasterVariables == 1u);
  CHECK(counts.relativePaths == 1u);
  CHECK(counts.absoluteMissingPaths == 1u);
  CHECK(counts.missingDefinitionHandles == 1u);
  CHECK(counts.rectangularClips == 1u);
  CHECK(counts.polygonalClips == 1u);
  CHECK(counts.malformedClips == 1u);
  CHECK(counts.invertedClips == 1u);
  CHECK(counts.hiddenFrames == 1u);

  REQUIRE(metadata.findImageDefinitionByHandle(imageDefinition.handle) != nullptr);
  CHECK(metadata.findImageDefinitionsByPath(imageDefinition.name).size() == 1u);
  CHECK(metadata.findRasterImagesByDefinitionHandle(imageDefinition.handle).size()
        == 1u);
  CHECK(metadata.findImageDefinitionReactorsByDefinitionHandle(
            imageDefinition.handle).size() == 1u);
  REQUIRE(metadata.findUnderlayDefinitionByHandle(underlayDefinition.handle)
          != nullptr);
  CHECK(metadata.findUnderlayDefinitionsByPath(underlayDefinition.filename)
        .size() == 1u);
  CHECK(metadata.findUnderlaysByDefinitionHandle(underlayDefinition.handle)
        .size() == 1u);
}

TEST_CASE("DWG shape and OLE metadata reports writer blockers",
          "[entity_metadata][dwg_metadata][shape-ole]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_Shape shape;
  shape.handle = 0x7A0u;
  shape.parentHandle = 0x20u;
  shape.m_shapeFileHandle = 0x301u;
  shape.m_shapeIndex = 7u;
  shape.m_scale = 2.0;
  shape.m_rotation = 0.25;
  shape.m_widthFactor = 0.8;
  shape.m_oblique = 0.1;
  shape.m_thickness = 0.0;
  shape.m_insertionPoint = DRW_Coord{3.0, 4.0, 0.0};
  shape.m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
  shape.m_rawBytes = {1u, 2u, 3u};
  metadata.addShape(shape);

  DRW_Shape missingStyleShape;
  missingStyleShape.handle = 0x7A1u;
  missingStyleShape.m_shapeIndex = 3u;
  metadata.addShape(missingStyleShape);

  DRW_Ole2Frame ole2Frame;
  ole2Frame.handle = 0x7A2u;
  ole2Frame.parentHandle = 0x20u;
  ole2Frame.m_flags = 2u;
  ole2Frame.m_mode = 1u;
  ole2Frame.m_declaredPayloadLength = 16u;
  ole2Frame.m_payloadByteCount = 16u;
  ole2Frame.m_payloadPresent = true;
  ole2Frame.m_hasR2000TrailingByte = true;
  ole2Frame.m_r2000TrailingByte = 0u;
  ole2Frame.m_rawBytes = {4u, 5u, 6u};
  metadata.addOle2Frame(ole2Frame);

  DRW_Ole2Frame truncatedOle2Frame;
  truncatedOle2Frame.handle = 0x7A3u;
  truncatedOle2Frame.m_declaredPayloadLength =
      DRW_Ole2Frame::kMaxOlePayloadBytes + 1u;
  truncatedOle2Frame.m_payloadTooLarge = true;
  truncatedOle2Frame.m_payloadTruncated = true;
  metadata.addOle2Frame(truncatedOle2Frame);

  const LC_DwgAdvancedMetadata::ShapeOleWriterBlockerCounts blockers =
      metadata.shapeOleWriterBlockerCounts();
  CHECK(blockers.shapeCount == 2u);
  CHECK(blockers.ole2FrameCount == 2u);
  CHECK(blockers.missingStyleHandle == 1u);
  CHECK(blockers.missingOlePayload == 1u);
  CHECK(blockers.truncatedOlePayload == 1u);
  CHECK(blockers.oversizedOlePayload == 1u);
  CHECK(blockers.unsupportedOlePayloadRegeneration == 2u);
  CHECK(blockers.missingRawRange == 2u);

  REQUIRE(metadata.findShapeByHandle(shape.handle) != nullptr);
  CHECK(metadata.findShapesByShapeFileHandle(shape.m_shapeFileHandle).size()
        == 1u);
  REQUIRE(metadata.findOle2FrameByHandle(ole2Frame.handle) != nullptr);

  const std::vector<LC_DwgAdvancedMetadata::AdvancedEntityWriterReadiness>
      ledger = metadata.advancedEntityWriterLedger(DRW::AC1027);
  CHECK(std::count_if(
            ledger.begin(), ledger.end(),
            [](const LC_DwgAdvancedMetadata::AdvancedEntityWriterReadiness& r) {
              return r.family
                  == LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::Shape;
            }) == 2);
  CHECK(std::count_if(
            ledger.begin(), ledger.end(),
            [](const LC_DwgAdvancedMetadata::AdvancedEntityWriterReadiness& r) {
              return r.family
                  == LC_DwgAdvancedMetadata::AdvancedEntityWriterFamily::Ole2Frame;
            }) == 2);
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

TEST_CASE("DWG advanced metadata reports MLEADER writer blockers",
          "[entity_metadata][dwg_metadata][mleader]") {
  LC_DwgAdvancedMetadata metadata;

  auto addLeaderGeometry = [](DRW_MLeader& mleader) {
    DRW_MLeaderLeaderLine leaderLine;
    leaderLine.points = {DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 1.0, 0.0}};
    DRW_MLeaderRoot root;
    root.leaderLines.push_back(leaderLine);
    mleader.context.roots.push_back(root);
  };

  DRW_MLeader textLeader;
  textLeader.handle = 0x360u;
  textLeader.styleContentType = 2u;
  textLeader.context.hasTextContents = true;
  addLeaderGeometry(textLeader);
  metadata.addMLeader(textLeader);

  DRW_MLeader unresolvedStyleLeader;
  unresolvedStyleLeader.handle = 0x361u;
  unresolvedStyleLeader.styleHandle.ref = 0x460u;
  unresolvedStyleLeader.styleContentType = 2u;
  unresolvedStyleLeader.context.hasTextContents = true;
  addLeaderGeometry(unresolvedStyleLeader);
  metadata.addMLeader(unresolvedStyleLeader);

  DRW_MLeader missingTextLeader;
  missingTextLeader.handle = 0x362u;
  missingTextLeader.styleContentType = 2u;
  addLeaderGeometry(missingTextLeader);
  metadata.addMLeader(missingTextLeader);

  DRW_MLeader blockLeader;
  blockLeader.handle = 0x363u;
  blockLeader.styleContentType = 1u;
  blockLeader.context.hasContentsBlock = true;
  addLeaderGeometry(blockLeader);
  metadata.addMLeader(blockLeader);

  DRW_MLeader toleranceLeader;
  toleranceLeader.handle = 0x364u;
  toleranceLeader.styleContentType = 3u;
  addLeaderGeometry(toleranceLeader);
  metadata.addMLeader(toleranceLeader);

  DRW_MLeader overrideLeader;
  overrideLeader.handle = 0x365u;
  overrideLeader.styleContentType = 2u;
  overrideLeader.overrideFlags = 0x10;
  overrideLeader.context.hasTextContents = true;
  addLeaderGeometry(overrideLeader);
  metadata.addMLeader(overrideLeader);

  DRW_MLeader missingGeometryLeader;
  missingGeometryLeader.handle = 0x366u;
  missingGeometryLeader.styleContentType = 2u;
  missingGeometryLeader.context.hasTextContents = true;
  metadata.addMLeader(missingGeometryLeader);

  DRW_MLeader invalidatedLeader;
  invalidatedLeader.handle = 0x367u;
  invalidatedLeader.styleContentType = 2u;
  invalidatedLeader.context.hasTextContents = true;
  invalidatedLeader.arrowHeadHandle.ref = 0x461u;
  addLeaderGeometry(invalidatedLeader);
  metadata.addMLeader(invalidatedLeader);
  metadata.invalidateMLeaderGraphForHandle(0x461u);

  DRW_MLeader replacedLeader;
  replacedLeader.handle = 0x368u;
  replacedLeader.styleContentType = 2u;
  replacedLeader.context.hasTextContents = true;
  addLeaderGeometry(replacedLeader);
  metadata.addMLeader(replacedLeader);
  metadata.markMLeaderReplayReplacedForHandle(0x368u);

  const LC_DwgAdvancedMetadata::MLeaderWriterBlockerCounts blockers =
      metadata.mleaderWriterBlockerCounts();
  CHECK(blockers.mleaderCount == 9u);
  CHECK(blockers.unresolvedStyle == 1u);
  CHECK(blockers.missingTextContent == 1u);
  CHECK(blockers.blockContent == 1u);
  CHECK(blockers.toleranceContent == 1u);
  CHECK(blockers.overrideFlags == 1u);
  CHECK(blockers.missingLeaderGeometry == 1u);
  CHECK(blockers.invalidated == 1u);
  CHECK(blockers.replaced == 1u);
  CHECK(blockers.totalBlockers() == 8u);
}

struct DwgFixtureManifestEntry {
  QString name;
  QString path;
  QString targetVersion;
  bool optional = true;
  std::vector<QString> tags;
  std::vector<QString> expectedCallbacks;
  std::vector<QString> expectedRawFamilies;
  int expectedRawReplayCount = 0;
  QString acadSharpReference;
  QString libreDwgReference;
  bool expectedLoad = false;
  bool preserveRawUnsupported = false;
};

static std::vector<QString> fixtureStringArray(const QJsonObject& object,
                                               const QString& key) {
  std::vector<QString> values;
  const QJsonArray array = object.value(key).toArray();
  values.reserve(static_cast<size_t>(array.size()));
  for (const QJsonValue& value : array)
    values.push_back(value.toString());
  return values;
}

static bool fixtureVectorContains(const std::vector<QString>& values,
                                  const QString& expected) {
  for (const QString& value : values) {
    if (value == expected)
      return true;
  }
  return false;
}

static DwgFixtureManifestEntry fixtureManifestEntryFromJson(
    const QJsonObject& object) {
  DwgFixtureManifestEntry entry;
  entry.name = object.value("name").toString();
  entry.path = object.value("path").toString();
  entry.targetVersion = object.value("targetVersion").toString();
  entry.optional = object.value("optional").toBool(true);
  entry.tags = fixtureStringArray(object, "tags");
  entry.expectedCallbacks = fixtureStringArray(object, "expectedCallbacks");
  entry.expectedRawFamilies = fixtureStringArray(object, "expectedRawFamilies");
  entry.expectedRawReplayCount = object.value("expectedRawReplayCount").toInt();
  const QJsonObject references = object.value("references").toObject();
  entry.acadSharpReference = references.value("acadSharp").toString();
  entry.libreDwgReference = references.value("libreDwg").toString();
  const QJsonObject expect = object.value("expect").toObject();
  entry.expectedLoad = expect.value("load").toBool();
  entry.preserveRawUnsupported =
      expect.value("preserveRawUnsupported").toBool();
  return entry;
}

TEST_CASE("DWG fixture manifest is valid JSON and optional by default",
          "[.dwg_fixtures]") {
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
  const DwgFixtureManifestEntry entry = fixtureManifestEntryFromJson(first);
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
  CHECK(entry.optional);
  CHECK(!entry.name.isEmpty());
  CHECK(entry.path.contains("${HOME}"));
  CHECK(!entry.targetVersion.isEmpty());
  CHECK(fixtureVectorContains(entry.tags, QStringLiteral("mleader")));
  CHECK(fixtureVectorContains(entry.expectedCallbacks,
                              QStringLiteral("addUnsupportedObject")));
  CHECK(fixtureVectorContains(entry.expectedRawFamilies,
                              QStringLiteral("associative")));
  CHECK(entry.expectedRawReplayCount >= 0);
  CHECK(entry.acadSharpReference.isEmpty());
  CHECK(entry.libreDwgReference.isEmpty());
  CHECK(entry.expectedLoad);
  CHECK(entry.preserveRawUnsupported);
  INFO("optional DWG fixture path: " << entry.path.toStdString());
  INFO("enabled DWG fixture tags: " << static_cast<int>(entry.tags.size()));
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

  in.close();
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

  in.close();
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

// Phase 2b.0 — source DWG version accessor + raw-replay version guard.
namespace {
// Mirrors the version guard added to RS_FilterDXFRW::writeObjects /
// writeDwgClasses raw-replay loops: a same-version (or UNKNOWNV-source) object
// is eligible; a cross-version object is dropped (replayed==0, blocked==1).
struct RawReplayGuardResult {
  int replayed = 0;
  int blockedVersionMismatch = 0;
};

RawReplayGuardResult simulateRawReplay(const LC_DwgAdvancedMetadata& metadata,
                                       DRW::Version targetVersion) {
  RawReplayGuardResult result;
  for (const auto& record : metadata.rawObjects()) {
    if (metadata.sourceDwgVersion() != DRW::UNKNOWNV
        && metadata.sourceDwgVersion() != targetVersion) {
      ++result.blockedVersionMismatch;
      continue;
    }
    if (LC_DwgAdvancedMetadata::rawReplayBlocker(record)
        != LC_DwgAdvancedMetadata::ReplayBlocker::None) {
      continue;
    }
    ++result.replayed;
  }
  return result;
}
}  // namespace

TEST_CASE("DWG advanced metadata source version accessor round-trips",
          "[entity_metadata][dwg_metadata][replay_version]") {
  LC_DwgAdvancedMetadata metadata;
  // Default is UNKNOWNV (no source recorded).
  CHECK(metadata.sourceDwgVersion() == DRW::UNKNOWNV);

  metadata.setSourceDwgVersion(DRW::AC1015);
  CHECK(metadata.sourceDwgVersion() == DRW::AC1015);

  metadata.setSourceDwgVersion(DRW::AC1032);
  CHECK(metadata.sourceDwgVersion() == DRW::AC1032);

  metadata.clear();
  CHECK(metadata.sourceDwgVersion() == DRW::UNKNOWNV);

  CHECK(std::string(LC_DwgAdvancedMetadata::replayBlockerName(
            LC_DwgAdvancedMetadata::ReplayBlocker::VersionMismatch))
        == "source/target version mismatch");
}

TEST_CASE("DWG raw-replay version guard drops cross-version custom objects",
          "[entity_metadata][dwg_metadata][replay_version]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject raw;
  raw.m_objectType = 510;  // >=500 custom class.
  raw.m_handle = 0x123u;
  raw.m_bodyBitSize = 64u;
  raw.m_isCustomClass = true;
  raw.m_recordName = "ACDBMATERIAL";
  raw.m_className = "AcDbMaterial";
  raw.m_rawBytes = {0x10u, 0x20u, 0x30u};
  metadata.addUnsupportedObject(raw);

  // Same-version (AC1015 source -> AC1015 target): object is eligible.
  metadata.setSourceDwgVersion(DRW::AC1015);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[0])
        == LC_DwgAdvancedMetadata::ReplayBlocker::None);
  RawReplayGuardResult sameVersion =
      simulateRawReplay(metadata, DRW::AC1015);
  CHECK(sameVersion.replayed == 1);
  CHECK(sameVersion.blockedVersionMismatch == 0);

  // Cross-version (AC1032 source -> AC1015 target): object is dropped, no
  // malformed bytes emitted, counted as a version mismatch.
  metadata.setSourceDwgVersion(DRW::AC1032);
  RawReplayGuardResult crossVersion =
      simulateRawReplay(metadata, DRW::AC1015);
  CHECK(crossVersion.replayed == 0);
  CHECK(crossVersion.blockedVersionMismatch == 1);

  // UNKNOWNV source (no version recorded): guard is a no-op, object eligible.
  metadata.setSourceDwgVersion(DRW::UNKNOWNV);
  RawReplayGuardResult unknownSource =
      simulateRawReplay(metadata, DRW::AC1015);
  CHECK(unknownSource.replayed == 1);
  CHECK(unknownSource.blockedVersionMismatch == 0);
}

// Phase 2b.1 — MATERIAL + VISUALSTYLE raw-replay rescue. The reader arms now
// capture the full byte image as a custom-class raw object, eligible for
// byte-for-byte replay (same source/target version).
TEST_CASE("DWG MATERIAL and VISUALSTYLE raw objects are replay-eligible",
          "[entity_metadata][dwg_metadata][replay_rescue]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject material;
  material.m_objectType = 600;  // >=500 custom class.
  material.m_handle = 0x301u;
  material.m_isCustomClass = true;
  material.m_recordName = "MATERIAL";
  material.m_className = "AcDbMaterial";
  material.m_rawBytes = {0xAAu, 0xBBu, 0xCCu, 0xDDu};
  metadata.addUnsupportedObject(material);

  DRW_UnsupportedObject visualStyle;
  visualStyle.m_objectType = 601;
  visualStyle.m_handle = 0x302u;
  visualStyle.m_isCustomClass = true;
  visualStyle.m_recordName = "ACDB_VISUALSTYLE_CLASS";
  visualStyle.m_className = "AcDbVisualStyle";
  visualStyle.m_rawBytes = {0x11u, 0x22u};
  metadata.addUnsupportedObject(visualStyle);

  REQUIRE(metadata.rawObjects().size() == 2);
  CHECK(metadata.rawObjects()[0].recordName == "MATERIAL");
  CHECK(metadata.rawObjects()[0].rawBytes.size() == 4);
  CHECK(metadata.rawObjects()[1].recordName == "ACDB_VISUALSTYLE_CLASS");
  CHECK(metadata.rawObjects()[1].rawBytes.size() == 2);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[0])
        == LC_DwgAdvancedMetadata::ReplayBlocker::None);
  CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(metadata.rawObjects()[1])
        == LC_DwgAdvancedMetadata::ReplayBlocker::None);
}

// Phase 2b.2 — TABLESTYLE + CELLSTYLEMAP + TABLECONTENT raw-replay rescue.
// When not invalidated by the table graph, all three are replay-eligible.
TEST_CASE("DWG TABLESTYLE/CELLSTYLEMAP/TABLECONTENT raw objects replay-eligible",
          "[entity_metadata][dwg_metadata][replay_rescue][table]") {
  LC_DwgAdvancedMetadata metadata;

  const struct {
    int objectType;
    std::uint32_t handle;
    const char* recName;
    const char* className;
  } kCases[] = {
      {620, 0x500u, "TABLESTYLE", "AcDbTableStyle"},
      {621, 0x501u, "CELLSTYLEMAP", "AcDbCellStyleMap"},
      {622, 0x502u, "TABLECONTENT", "AcDbTableContent"},
  };
  for (const auto& c : kCases) {
    DRW_UnsupportedObject raw;
    raw.m_objectType = c.objectType;
    raw.m_handle = c.handle;
    raw.m_isCustomClass = true;
    raw.m_recordName = c.recName;
    raw.m_className = c.className;
    raw.m_rawBytes = {0x01u, 0x02u, 0x03u};
    metadata.addUnsupportedObject(raw);
  }

  REQUIRE(metadata.rawObjects().size() == 3);
  for (const auto& record : metadata.rawObjects()) {
    CHECK(record.rawBytes.size() == 3);
    CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(record)
          == LC_DwgAdvancedMetadata::ReplayBlocker::None);
  }
}

// Phase 2b.3 — DETAILVIEWSTYLE + SECTIONVIEWSTYLE raw-replay rescue.
TEST_CASE("DWG DETAILVIEWSTYLE/SECTIONVIEWSTYLE raw objects replay-eligible",
          "[entity_metadata][dwg_metadata][replay_rescue]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject detail;
  detail.m_objectType = 630;
  detail.m_handle = 0x510u;
  detail.m_isCustomClass = true;
  detail.m_recordName = "ACDBDETAILVIEWSTYLE";
  detail.m_className = "AcDbDetailViewStyle";
  detail.m_rawBytes = {0x01u, 0x02u, 0x03u, 0x04u};
  metadata.addUnsupportedObject(detail);

  DRW_UnsupportedObject section;
  section.m_objectType = 631;
  section.m_handle = 0x511u;
  section.m_isCustomClass = true;
  section.m_recordName = "ACDBSECTIONVIEWSTYLE";
  section.m_className = "AcDbSectionViewStyle";
  section.m_rawBytes = {0x05u, 0x06u};
  metadata.addUnsupportedObject(section);

  REQUIRE(metadata.rawObjects().size() == 2);
  CHECK(metadata.rawObjects()[0].recordName == "ACDBDETAILVIEWSTYLE");
  CHECK(metadata.rawObjects()[1].recordName == "ACDBSECTIONVIEWSTYLE");
  for (const auto& record : metadata.rawObjects()) {
    CHECK(!record.rawBytes.empty());
    CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(record)
          == LC_DwgAdvancedMetadata::ReplayBlocker::None);
  }
}

// Phase 2b.4 — IMAGEDEF_REACTOR + TABLEGEOMETRY + ACAD_EVALUATION_GRAPH rescue.
TEST_CASE("DWG IMAGEDEF_REACTOR/TABLEGEOMETRY/EVALGRAPH raw objects eligible",
          "[entity_metadata][dwg_metadata][replay_rescue]") {
  LC_DwgAdvancedMetadata metadata;

  const struct {
    int objectType;
    std::uint32_t handle;
    const char* recName;
    const char* className;
  } kCases[] = {
      {640, 0x520u, "IMAGEDEF_REACTOR", "AcDbRasterImageDefReactor"},
      {641, 0x521u, "TABLEGEOMETRY", "AcDbTableGeometry"},
      {642, 0x522u, "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph"},
  };
  for (const auto& c : kCases) {
    DRW_UnsupportedObject raw;
    raw.m_objectType = c.objectType;
    raw.m_handle = c.handle;
    raw.m_isCustomClass = true;
    raw.m_recordName = c.recName;
    raw.m_className = c.className;
    raw.m_rawBytes = {0x07u, 0x08u};
    metadata.addUnsupportedObject(raw);
  }

  REQUIRE(metadata.rawObjects().size() == 3);
  for (const auto& record : metadata.rawObjects()) {
    CHECK(!record.rawBytes.empty());
    CHECK(LC_DwgAdvancedMetadata::rawReplayBlocker(record)
          == LC_DwgAdvancedMetadata::ReplayBlocker::None);
  }
}
