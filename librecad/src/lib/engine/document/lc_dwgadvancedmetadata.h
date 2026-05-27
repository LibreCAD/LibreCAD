/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published by
** the Free Software Foundation either version 2 of the License, or (at your
** option) any later version.
**
**********************************************************************/

#ifndef LC_DWGADVANCEDMETADATA_H
#define LC_DWGADVANCEDMETADATA_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"

class LC_DwgAdvancedMetadata {
public:
    enum class ReplayState {
        ReplayAllowed,
        ReplayInvalidated,
        ReplayReplaced
    };

    enum class ReplayBlocker {
        None,
        Invalidated,
        Replaced,
        EntityReplayUnsupported,
        MissingRawBytes,
        MissingClassMetadata,
        WriterRejected,
        SemanticOnly
    };

    enum class AssociativeKind {
        Unknown,
        Network,
        Action,
        Dependency,
        GeometryDependency,
        PersistentSubentityManager,
        AlignedDimensionActionBody,
        VertexActionParam,
        OsnapPointRefActionParam
    };

    struct RawObjectRecord {
        int objectType = 0;
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 bodyBitSize = 0;
        duint64 objectOffset = 0;
        duint32 objectSize = 0;
        bool isEntity = false;
        bool isCustomClass = false;
        std::string recordName;
        std::string className;
        std::vector<duint8> rawBytes;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ViewRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string name;
        DRW_Coord size;
        DRW_Coord center;
        DRW_Coord viewDirectionFromTarget;
        DRW_Coord targetPoint;
        double lensLen = 0.0;
        double frontClippingPlaneOffset = 0.0;
        double backClippingPlaneOffset = 0.0;
        double twistAngle = 0.0;
        int viewMode = 0;
        unsigned int renderMode = 0;
        bool hasUcs = false;
        bool cameraPlottable = false;
        DRW_Coord ucsOrigin;
        DRW_Coord ucsXAxis;
        DRW_Coord ucsYAxis;
        int ucsOrthoType = 0;
        double ucsElevation = 0.0;
        duint32 namedUcsHandle = 0;
        duint32 baseUcsHandle = 0;
        duint32 backgroundHandle = 0;
        duint32 visualStyleHandle = 0;
        duint32 sunHandle = 0;
        duint32 liveSectionHandle = 0;
        bool hasUcsHandleRefs = false;
        bool hasVisualHandleRefs = false;
        bool sunResolved = false;
        bool useDefaultLights = true;
        duint8 defaultLightingType = 1;
        double brightness = 0.0;
        double contrast = 0.0;
        duint32 ambientColor = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct LightRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 classVersion = 0;
        std::string name;
        duint32 type = 0;
        bool status = false;
        duint32 color = 0;
        bool plotGlyph = false;
        double intensity = 0.0;
        DRW_Coord position;
        DRW_Coord target;
        duint32 attenuationType = 0;
        bool useAttenuationLimits = false;
        double attenuationStartLimit = 0.0;
        double attenuationEndLimit = 0.0;
        double hotspotAngle = 0.0;
        double falloffAngle = 0.0;
        bool castShadows = false;
        duint32 shadowType = 0;
        duint16 shadowMapSize = 0;
        duint8 shadowMapSoftness = 0;
        bool hasPhotometricData = false;
        bool hasWebFile = false;
        std::string webFile;
        duint16 physicalIntensityMethod = 0;
        double physicalIntensity = 0.0;
        double illuminanceDistance = 0.0;
        duint16 lampColorType = 0;
        double lampColorTemperature = 0.0;
        duint16 lampColorPreset = 0;
        DRW_Coord webRotation{1.0, 0.0, 0.0};
        duint16 extendedLightShape = 0;
        double extendedLightLength = 0.0;
        double extendedLightWidth = 0.0;
        double extendedLightRadius = 0.0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SunRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 classVersion = 0;
        bool isOn = false;
        duint32 color = 0;
        double intensity = 0.0;
        bool hasShadow = false;
        dint32 julianDay = 0;
        dint32 milliseconds = 0;
        bool isDaylightSavings = false;
        duint32 shadowType = 0;
        duint16 shadowMapSize = 0;
        duint8 shadowSoftness = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ModelerGeometryRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        DRW::ETYPE type = DRW::UNKNOWN;
        duint16 modelerVersion = 0;
        duint32 bodyBitSize = 0;
        duint32 objectSize = 0;
        bool isEmpty = false;
        bool hasModelerData = false;
        bool modelerDataUnknownBit = false;
        bool hasWireframe = false;
        bool hasRawPayload = false;
        duint32 historyHandle = 0;
        size_t rawByteCount = 0;
        std::vector<duint8> rawBytes;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableMergedRangeRecord {
        int topRow = 0;
        int leftColumn = 0;
        int bottomRow = 0;
        int rightColumn = 0;
    };

    struct TableRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        duint32 tableStyleHandle = 0;
        int rowCount = 0;
        int columnCount = 0;
        size_t cellCount = 0;
        size_t contentCount = 0;
        size_t textContentCount = 0;
        size_t fieldContentCount = 0;
        size_t blockContentCount = 0;
        size_t attributeCount = 0;
        size_t valueHandleCount = 0;
        size_t blockHandleCount = 0;
        size_t fieldHandleCount = 0;
        size_t mergedRangeCount = 0;
        size_t overrideCellCount = 0;
        size_t geometryCellCount = 0;
        size_t rowStyleCount = 0;
        size_t cellStyleCount = 0;
        size_t borderCount = 0;
        bool titleSuppressed = false;
        bool headerSuppressed = false;
        bool hasTextContent = false;
        bool hasBlockContent = false;
        bool rawReplayAvailable = false;
        bool semanticParsed = false;
        bool styleResolved = false;
        bool fallbackRendered = false;
        std::vector<double> columnWidths;
        std::vector<double> rowHeights;
        std::vector<int> cellStyleIds;
        std::vector<std::string> cellTexts;
        std::vector<std::string> attributeTexts;
        std::vector<duint32> valueHandles;
        std::vector<duint32> blockHandles;
        std::vector<duint32> fieldHandles;
        std::vector<duint32> geometryHandles;
        std::vector<TableMergedRangeRecord> mergedRanges;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativeRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        AssociativeKind kind = AssociativeKind::Unknown;
        duint16 classVersion = 0;
        dint32 geometryStatus = 0;
        duint32 owningNetworkHandle = 0;
        duint32 actionBodyHandle = 0;
        dint32 actionIndex = 0;
        dint32 maxDependencyIndex = 0;
        size_t dependencyCount = 0;
        size_t actionCount = 0;
        std::vector<DRW_AssociativeHandleRef> dependencyRefs;
        std::vector<DRW_AssociativeHandleRef> actionRefs;
        std::vector<duint32> ownedParamHandles;
        std::vector<duint32> ownedActionHandles;
        duint32 dependencyHandle = 0;
        duint32 readDependencyHandle = 0;
        duint32 writeDependencyHandle = 0;
        duint32 rNodeHandle = 0;
        duint32 dNodeHandle = 0;
        dint32 status = 0;
        duint8 osnapMode = 0;
        double parameter = 0.0;
        DRW_Coord point;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AcShRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        duint32 major = 0;
        duint32 minor = 0;
        duint32 ownerHandle = 0;
        duint32 historyNodeId = 0;
        bool showHistory = false;
        bool recordHistory = false;
        DRW_Coord direction;
        double draftAngle = 0.0;
        double startDraftDistance = 0.0;
        double endDraftDistance = 0.0;
        double scaleFactor = 1.0;
        double twistAngle = 0.0;
        double alignAngle = 0.0;
        size_t binaryBlob1Bytes = 0;
        size_t binaryBlob2Bytes = 0;
        size_t blobBytes = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct MLeaderRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint16 classVersion = 0;
        duint32 styleHandle = 0;
        dint32 overrideFlags = 0;
        duint16 leaderType = 0;
        duint16 styleContentType = 0;
        duint32 leaderLineTypeHandle = 0;
        duint32 arrowHeadHandle = 0;
        duint32 textStyleHandle = 0;
        duint32 blockHandle = 0;
        duint16 effectiveContentType = 0;
        duint16 effectiveLeaderType = 0;
        duint32 effectiveLeaderLineTypeHandle = 0;
        duint32 effectiveArrowHeadHandle = 0;
        duint32 effectiveTextStyleHandle = 0;
        duint32 effectiveBlockHandle = 0;
        size_t rootCount = 0;
        size_t leaderLineCount = 0;
        size_t pointCount = 0;
        size_t breakCount = 0;
        size_t columnCount = 0;
        size_t arrowHeadOverrideCount = 0;
        size_t blockLabelCount = 0;
        std::vector<duint32> arrowHeadOverrideHandles;
        std::vector<duint32> blockAttributeDefinitionHandles;
        std::vector<std::string> blockLabelTexts;
        double overallScale = 1.0;
        double landingDistance = 0.0;
        double defaultArrowHeadSize = 0.0;
        double textHeight = 0.0;
        bool styleResolved = false;
        bool landingEnabled = false;
        bool doglegEnabled = false;
        bool isAnnotative = false;
        bool hasTextLabel = false;
        bool hasTextContent = false;
        bool hasBlockContent = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct MLeaderStyleRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string name;
        duint16 styleVersion = 0;
        duint16 contentType = 0;
        duint16 leaderType = 0;
        duint32 leaderLineTypeHandle = 0;
        duint32 arrowHeadBlockHandle = 0;
        duint32 textStyleHandle = 0;
        duint32 blockHandle = 0;
        double arrowHeadSize = 0.0;
        double textHeight = 0.0;
        double scaleFactor = 1.0;
        bool isAnnotative = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct DetailViewStyleRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string name;
        std::string description;
        std::string displayName;
        duint16 classVersion = 0;
        duint32 flags = 0;
        duint32 identifierStyleHandle = 0;
        duint32 arrowSymbolHandle = 0;
        duint32 viewLabelTextStyleHandle = 0;
        duint32 boundaryLineTypeHandle = 0;
        duint32 connectionLineTypeHandle = 0;
        duint32 borderLineTypeHandle = 0;
        std::string viewLabelPattern;
        double identifierHeight = 0.0;
        double arrowSymbolSize = 0.0;
        double viewLabelTextHeight = 0.0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SectionViewStyleRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string name;
        std::string description;
        std::string displayName;
        duint16 classVersion = 0;
        duint32 flags = 0;
        duint32 identifierStyleHandle = 0;
        duint32 arrowStartSymbolHandle = 0;
        duint32 arrowEndSymbolHandle = 0;
        duint32 planeLineTypeHandle = 0;
        duint32 bendLineTypeHandle = 0;
        duint32 viewLabelTextStyleHandle = 0;
        std::string viewLabelPattern;
        std::string hatchPattern;
        double identifierHeight = 0.0;
        double arrowSymbolSize = 0.0;
        double viewLabelTextHeight = 0.0;
        double hatchScale = 1.0;
        size_t hatchAngleCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct BreakDataRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 dimensionHandle = 0;
        std::vector<duint32> pointRefHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct BreakPointRefRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct GroupRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string description;
        bool isUnnamed = false;
        bool selectable = true;
        std::vector<duint32> entityHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ImageDefinitionReactorRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        dint32 classVersion = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SpatialFilterRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        size_t boundaryPointCount = 0;
        bool displayBoundary = false;
        bool clipFrontPlane = false;
        bool clipBackPlane = false;
        double frontDistance = 0.0;
        double backDistance = 0.0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct GeoDataRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 hostBlockHandle = 0;
        dint32 version = 0;
        dint16 coordinatesType = 0;
        dint32 horizontalUnits = 0;
        dint32 verticalUnits = 0;
        double horizontalUnitScale = 1.0;
        double verticalUnitScale = 1.0;
        std::string coordinateSystemDefinition;
        std::string geoRssTag;
        size_t meshPointCount = 0;
        size_t meshFaceCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableGeometryRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        dint32 rowCount = 0;
        dint32 columnCount = 0;
        dint32 cellCount = 0;
        size_t contentCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct PlaceholderRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    void clear() {
        m_rawObjects.clear();
        m_views.clear();
        m_lights.clear();
        m_suns.clear();
        m_modelerGeometry.clear();
        m_tables.clear();
        m_associativeObjects.clear();
        m_acshObjects.clear();
        m_mleaders.clear();
        m_mleaderStyles.clear();
        m_detailViewStyles.clear();
        m_sectionViewStyles.clear();
        m_breakData.clear();
        m_breakPointRefs.clear();
        m_groups.clear();
        m_imageDefinitionReactors.clear();
        m_spatialFilters.clear();
        m_geoData.clear();
        m_tableGeometry.clear();
        m_placeholders.clear();
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& object) {
        RawObjectRecord record;
        record.objectType = object.m_objectType;
        record.handle = object.m_handle;
        record.bodyBitSize = object.m_bodyBitSize;
        record.objectOffset = object.m_objectOffset;
        record.objectSize = object.m_objectSize;
        record.isEntity = object.m_isEntity;
        record.isCustomClass = object.m_isCustomClass;
        record.recordName = object.m_recordName;
        record.className = object.m_className;
        record.rawBytes = object.m_rawBytes;
        m_rawObjects.push_back(std::move(record));
    }

    void addView(const DRW_View& view) {
        ViewRecord record;
        record.handle = view.handle;
        record.parentHandle = view.parentHandle;
        record.name = view.name;
        record.size = view.size;
        record.center = view.center;
        record.viewDirectionFromTarget = view.viewDirectionFromTarget;
        record.targetPoint = view.targetPoint;
        record.lensLen = view.lensLen;
        record.frontClippingPlaneOffset = view.frontClippingPlaneOffset;
        record.backClippingPlaneOffset = view.backClippingPlaneOffset;
        record.twistAngle = view.twistAngle;
        record.viewMode = view.viewMode;
        record.renderMode = view.renderMode;
        record.hasUcs = view.hasUCS;
        record.cameraPlottable = view.cameraPlottable;
        record.ucsOrigin = view.ucsOrigin;
        record.ucsXAxis = view.ucsXAxis;
        record.ucsYAxis = view.ucsYAxis;
        record.ucsOrthoType = view.ucsOrthoType;
        record.ucsElevation = view.ucsElevation;
        record.namedUcsHandle = view.namedUCS_ID;
        record.baseUcsHandle = view.baseUCS_ID;
        record.backgroundHandle = view.m_backgroundHandle;
        record.visualStyleHandle = view.m_visualStyleHandle;
        record.sunHandle = view.m_sunHandle;
        record.liveSectionHandle = view.m_liveSectionHandle;
        record.hasUcsHandleRefs = record.namedUcsHandle != 0 || record.baseUcsHandle != 0;
        record.hasVisualHandleRefs = record.backgroundHandle != 0
                                     || record.visualStyleHandle != 0
                                     || record.sunHandle != 0
                                     || record.liveSectionHandle != 0;
        record.sunResolved = record.sunHandle != 0 && findSunByHandle(record.sunHandle) != nullptr;
        record.useDefaultLights = view.m_useDefaultLights;
        record.defaultLightingType = view.m_defaultLightingType;
        record.brightness = view.m_brightness;
        record.contrast = view.m_contrast;
        record.ambientColor = view.m_ambientColor;
        m_views.push_back(record);
    }

    void addLight(const DRW_Light& light) {
        LightRecord record;
        record.handle = light.handle;
        record.parentHandle = light.parentHandle;
        record.classVersion = light.m_classVersion;
        record.name = light.m_name;
        record.type = light.m_type;
        record.status = light.m_status;
        record.color = light.m_color;
        record.plotGlyph = light.m_plotGlyph;
        record.intensity = light.m_intensity;
        record.position = light.m_position;
        record.target = light.m_target;
        record.attenuationType = light.m_attenuationType;
        record.useAttenuationLimits = light.m_useAttenuationLimits;
        record.attenuationStartLimit = light.m_attenuationStartLimit;
        record.attenuationEndLimit = light.m_attenuationEndLimit;
        record.hotspotAngle = light.m_hotspotAngle;
        record.falloffAngle = light.m_falloffAngle;
        record.castShadows = light.m_castShadows;
        record.shadowType = light.m_shadowType;
        record.shadowMapSize = light.m_shadowMapSize;
        record.shadowMapSoftness = light.m_shadowMapSoftness;
        record.hasPhotometricData = light.m_hasPhotometricData;
        record.hasWebFile = light.m_hasWebFile;
        record.webFile = light.m_webFile;
        record.physicalIntensityMethod = light.m_physicalIntensityMethod;
        record.physicalIntensity = light.m_physicalIntensity;
        record.illuminanceDistance = light.m_illuminanceDistance;
        record.lampColorType = light.m_lampColorType;
        record.lampColorTemperature = light.m_lampColorTemperature;
        record.lampColorPreset = light.m_lampColorPreset;
        record.webRotation = light.m_webRotation;
        record.extendedLightShape = light.m_extendedLightShape;
        record.extendedLightLength = light.m_extendedLightLength;
        record.extendedLightWidth = light.m_extendedLightWidth;
        record.extendedLightRadius = light.m_extendedLightRadius;
        m_lights.push_back(record);
    }

    void addSun(const DRW_Sun& sun) {
        SunRecord record;
        record.handle = sun.handle;
        record.parentHandle = sun.parentHandle;
        record.classVersion = sun.m_classVersion;
        record.isOn = sun.m_isOn;
        record.color = sun.m_color;
        record.intensity = sun.m_intensity;
        record.hasShadow = sun.m_hasShadow;
        record.julianDay = sun.m_julianDay;
        record.milliseconds = sun.m_milliseconds;
        record.isDaylightSavings = sun.m_isDaylightSavings;
        record.shadowType = sun.m_shadowType;
        record.shadowMapSize = sun.m_shadowMapSize;
        record.shadowSoftness = sun.m_shadowSoftness;
        m_suns.push_back(record);
        for (ViewRecord& view : m_views) {
            if (view.sunHandle == record.handle)
                view.sunResolved = true;
        }
    }

    void addModelerGeometry(const DRW_ModelerGeometry& geometry) {
        ModelerGeometryRecord record;
        record.handle = geometry.handle;
        record.parentHandle = geometry.parentHandle;
        record.type = geometry.eType;
        record.modelerVersion = geometry.m_modelerVersion;
        record.bodyBitSize = geometry.m_bodyBitSize;
        record.objectSize = geometry.m_objectSize;
        record.isEmpty = geometry.m_isEmpty;
        record.hasModelerData = geometry.m_hasModelerData;
        record.modelerDataUnknownBit = geometry.m_modelerDataUnknownBit;
        record.hasWireframe = geometry.m_hasWireframe;
        record.hasRawPayload = !geometry.m_rawBytes.empty();
        record.historyHandle = geometry.m_historyHandle;
        record.rawByteCount = geometry.m_rawBytes.size();
        record.rawBytes = geometry.m_rawBytes;
        m_modelerGeometry.push_back(std::move(record));
    }

    void addTableStyle(const DRW_TableStyle& style) {
        TableRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.recordName = style.m_name;
        record.rowStyleCount = style.m_rowStyles.size();
        record.cellStyleCount = style.m_cellStyles.size();
        record.borderCount = style.m_tableCellStyle.m_borders.size();
        for (const DRW_TableStyleRowStyle& rowStyle : style.m_rowStyles)
            record.borderCount += rowStyle.m_borders.size();
        for (const DRW_TableStyleCellStyle& cellStyle : style.m_cellStyles)
            record.borderCount += cellStyle.m_borders.size();
        record.titleSuppressed = style.m_titleSuppressed;
        record.headerSuppressed = style.m_headerSuppressed;
        record.semanticParsed = true;
        record.styleResolved = true;
        m_tables.push_back(record);
    }

    void addTable(const DRW_Table& table, bool fallbackRendered) {
        TableRecord record;
        record.handle = table.handle;
        record.parentHandle = table.parentHandle;
        record.recordName = "ACAD_TABLE";
        record.rowCount = static_cast<int>(table.m_content.m_rows.size());
        record.columnCount = static_cast<int>(table.m_content.m_columns.size());
        record.tableStyleHandle = table.m_tableStyleHandle != 0
                                      ? table.m_tableStyleHandle
                                      : table.m_content.m_tableStyleHandle;
        populateTableContentSummary(record, table.m_content);
        record.semanticParsed = table.m_hasSemanticContent && table.m_semanticContentComplete;
        record.fallbackRendered = fallbackRendered;
        record.replayState = fallbackRendered ? ReplayState::ReplayReplaced
                                              : ReplayState::ReplayAllowed;
        m_tables.push_back(record);
    }

    void addTableContent(const DRW_TableContentObject& table) {
        TableRecord record;
        record.handle = table.handle;
        record.parentHandle = table.parentHandle;
        record.recordName = "TABLECONTENT";
        record.rowCount = static_cast<int>(table.m_content.m_rows.size());
        record.columnCount = static_cast<int>(table.m_content.m_columns.size());
        record.tableStyleHandle = table.m_content.m_tableStyleHandle;
        populateTableContentSummary(record, table.m_content);
        record.semanticParsed = table.m_parseComplete;
        record.fallbackRendered = false;
        m_tables.push_back(record);
    }

    void addAssociativeObject(const DRW_AssociativeObject& object) {
        AssociativeRecord record;
        record.handle = object.handle;
        record.parentHandle = object.parentHandle;
        record.recordName = object.m_recordName;
        record.kind = associativeKindFromRecordName(record.recordName);
        record.classVersion = object.m_classVersion;
        record.geometryStatus = object.m_geometryStatus;
        record.owningNetworkHandle = object.m_owningNetworkHandle;
        record.actionBodyHandle = object.m_actionBodyHandle;
        record.actionIndex = object.m_actionIndex;
        record.maxDependencyIndex = object.m_maxDependencyIndex;
        record.dependencyCount = object.m_dependencies.size();
        record.actionCount = object.m_actions.size();
        record.dependencyRefs = object.m_dependencies;
        record.actionRefs = object.m_actions;
        record.ownedParamHandles = object.m_ownedParams;
        record.ownedActionHandles = object.m_ownedActions;
        record.dependencyHandle = object.m_dependencyHandle;
        record.readDependencyHandle = object.m_readDependencyHandle;
        record.writeDependencyHandle = object.m_writeDependencyHandle;
        record.rNodeHandle = object.m_rNodeHandle;
        record.dNodeHandle = object.m_dNodeHandle;
        record.status = object.m_status;
        record.osnapMode = object.m_osnapMode;
        record.parameter = object.m_parameter;
        record.point = object.m_point;
        m_associativeObjects.push_back(std::move(record));
    }

    void addAcShObject(const DRW_AcShHistoryObject& object) {
        AcShRecord record;
        record.handle = object.handle;
        record.parentHandle = object.parentHandle;
        record.recordName = object.m_recordName;
        record.major = object.m_major;
        record.minor = object.m_minor;
        record.ownerHandle = object.m_ownerHandle;
        record.historyNodeId = object.m_historyNodeId;
        record.showHistory = object.m_showHistory;
        record.recordHistory = object.m_recordHistory;
        record.direction = object.m_direction;
        record.draftAngle = object.m_draftAngle;
        record.startDraftDistance = object.m_startDraftDistance;
        record.endDraftDistance = object.m_endDraftDistance;
        record.scaleFactor = object.m_scaleFactor;
        record.twistAngle = object.m_twistAngle;
        record.alignAngle = object.m_alignAngle;
        record.binaryBlob1Bytes = object.m_binaryBlob1.size();
        record.binaryBlob2Bytes = object.m_binaryBlob2.size();
        record.blobBytes = object.m_binaryBlob1.size() + object.m_binaryBlob2.size();
        m_acshObjects.push_back(record);
    }

    void addMLeader(const DRW_MLeader& mleader) {
        MLeaderRecord record;
        record.handle = mleader.handle;
        record.parentHandle = mleader.parentHandle;
        record.classVersion = mleader.classVersion;
        record.styleHandle = mleader.styleHandle.ref;
        record.overrideFlags = mleader.overrideFlags;
        record.leaderType = mleader.leaderType;
        record.styleContentType = mleader.styleContentType;
        record.leaderLineTypeHandle = mleader.leaderLineTypeHandle.ref;
        record.arrowHeadHandle = mleader.arrowHeadHandle.ref;
        record.textStyleHandle = mleader.styleTextStyleHandle.ref != 0
                                     ? mleader.styleTextStyleHandle.ref
                                     : mleader.context.textStyleHandle.ref;
        record.blockHandle = mleader.styleBlockHandle.ref != 0
                                 ? mleader.styleBlockHandle.ref
                                 : mleader.context.blockTableRecordHandle.ref;
        record.effectiveContentType = record.styleContentType;
        record.effectiveLeaderType = record.leaderType;
        record.effectiveLeaderLineTypeHandle = record.leaderLineTypeHandle;
        record.effectiveArrowHeadHandle = record.arrowHeadHandle;
        record.effectiveTextStyleHandle = record.textStyleHandle;
        record.effectiveBlockHandle = record.blockHandle;
        resolveMLeaderStyle(record);
        record.rootCount = mleader.context.roots.size();
        for (const DRW_MLeaderRoot& root : mleader.context.roots) {
            record.breakCount += root.breaks.size();
            record.leaderLineCount += root.leaderLines.size();
            for (const DRW_MLeaderLeaderLine& leaderLine : root.leaderLines) {
                record.pointCount += leaderLine.points.size();
                record.breakCount += leaderLine.breaks.size();
            }
        }
        record.columnCount = mleader.context.columnSizes.size();
        record.arrowHeadOverrideCount = mleader.arrowHeads.size();
        record.blockLabelCount = mleader.blockLabels.size();
        record.arrowHeadOverrideHandles.reserve(mleader.arrowHeads.size());
        for (const DRW_MLeader::ArrowHeadEntry& arrowHead : mleader.arrowHeads)
            record.arrowHeadOverrideHandles.push_back(arrowHead.handle.ref);
        record.blockAttributeDefinitionHandles.reserve(mleader.blockLabels.size());
        record.blockLabelTexts.reserve(mleader.blockLabels.size());
        for (const DRW_MLeader::BlockLabelEntry& blockLabel : mleader.blockLabels) {
            record.blockAttributeDefinitionHandles.push_back(blockLabel.attDefHandle.ref);
            record.blockLabelTexts.push_back(blockLabel.labelText);
        }
        record.overallScale = mleader.context.overallScale;
        record.landingDistance = mleader.landingDistance;
        record.defaultArrowHeadSize = mleader.defaultArrowHeadSize;
        record.textHeight = mleader.context.textHeight;
        record.landingEnabled = mleader.landingEnabled;
        record.doglegEnabled = mleader.doglegEnabled;
        record.isAnnotative = mleader.isAnnotative;
        record.hasTextLabel = !mleader.context.textLabel.empty();
        record.hasTextContent = mleader.context.hasTextContents;
        record.hasBlockContent = mleader.context.hasContentsBlock;
        m_mleaders.push_back(record);
    }

    void addMLeaderStyle(const DRW_MLeaderStyle& style) {
        MLeaderStyleRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.name = style.name;
        record.styleVersion = style.styleVersion;
        record.contentType = style.contentType;
        record.leaderType = style.leaderType;
        record.leaderLineTypeHandle = style.leaderLineTypeHandle.ref;
        record.arrowHeadBlockHandle = style.arrowHeadBlockHandle.ref;
        record.textStyleHandle = style.textStyleHandle.ref;
        record.blockHandle = style.blockHandle.ref;
        record.arrowHeadSize = style.arrowHeadSize;
        record.textHeight = style.textHeight;
        record.scaleFactor = style.scaleFactor;
        record.isAnnotative = style.isAnnotative;
        m_mleaderStyles.push_back(record);
        for (MLeaderRecord& mleader : m_mleaders)
            resolveMLeaderStyle(mleader, record);
    }

    void addDetailViewStyle(const DRW_DetailViewStyle& style) {
        DetailViewStyleRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.name = style.name;
        record.description = style.m_modelDoc.m_description;
        record.displayName = style.m_modelDoc.m_displayName;
        record.classVersion = style.m_classVersion;
        record.flags = style.m_flags;
        record.identifierStyleHandle = style.m_identifierStyleHandle;
        record.arrowSymbolHandle = style.m_arrowSymbolHandle;
        record.viewLabelTextStyleHandle = style.m_viewLabelTextStyleHandle;
        record.boundaryLineTypeHandle = style.m_boundaryLineTypeHandle;
        record.connectionLineTypeHandle = style.m_connectionLineTypeHandle;
        record.borderLineTypeHandle = style.m_borderLineTypeHandle;
        record.viewLabelPattern = style.m_viewLabelPattern;
        record.identifierHeight = style.m_identifierHeight;
        record.arrowSymbolSize = style.m_arrowSymbolSize;
        record.viewLabelTextHeight = style.m_viewLabelTextHeight;
        m_detailViewStyles.push_back(record);
    }

    void addSectionViewStyle(const DRW_SectionViewStyle& style) {
        SectionViewStyleRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.name = style.name;
        record.description = style.m_modelDoc.m_description;
        record.displayName = style.m_modelDoc.m_displayName;
        record.classVersion = style.m_classVersion;
        record.flags = style.m_flags;
        record.identifierStyleHandle = style.m_identifierStyleHandle;
        record.arrowStartSymbolHandle = style.m_arrowStartSymbolHandle;
        record.arrowEndSymbolHandle = style.m_arrowEndSymbolHandle;
        record.planeLineTypeHandle = style.m_planeLineTypeHandle;
        record.bendLineTypeHandle = style.m_bendLineTypeHandle;
        record.viewLabelTextStyleHandle = style.m_viewLabelTextStyleHandle;
        record.viewLabelPattern = style.m_viewLabelPattern;
        record.hatchPattern = style.m_hatchPattern;
        record.identifierHeight = style.m_identifierHeight;
        record.arrowSymbolSize = style.m_arrowSymbolSize;
        record.viewLabelTextHeight = style.m_viewLabelTextHeight;
        record.hatchScale = style.m_hatchScale;
        record.hatchAngleCount = style.m_hatchAngles.size();
        m_sectionViewStyles.push_back(record);
    }

    void addBreakData(const DRW_BreakData& data) {
        BreakDataRecord record;
        record.handle = data.handle;
        record.parentHandle = data.parentHandle;
        record.dimensionHandle = data.m_dimensionHandle;
        record.pointRefHandles = data.m_pointRefHandles;
        m_breakData.push_back(std::move(record));
    }

    void addBreakPointRef(const DRW_BreakPointRef& data) {
        BreakPointRefRecord record;
        record.handle = data.handle;
        record.parentHandle = data.parentHandle;
        m_breakPointRefs.push_back(record);
    }

    void addGroup(const DRW_Group& group) {
        GroupRecord record;
        record.handle = group.handle;
        record.parentHandle = group.parentHandle;
        record.description = group.m_description;
        record.isUnnamed = group.m_isUnnamed;
        record.selectable = group.m_selectable;
        record.entityHandles = group.m_entityHandles;
        m_groups.push_back(std::move(record));
    }

    void addImageDefinitionReactor(const DRW_ImageDefinitionReactor& reactor) {
        ImageDefinitionReactorRecord record;
        record.handle = reactor.handle;
        record.parentHandle = reactor.parentHandle;
        record.classVersion = reactor.m_classVersion;
        m_imageDefinitionReactors.push_back(record);
    }

    void addSpatialFilter(const DRW_SpatialFilter& filter) {
        SpatialFilterRecord record;
        record.handle = filter.handle;
        record.parentHandle = filter.parentHandle;
        record.boundaryPointCount = filter.m_boundaryPoints.size();
        record.displayBoundary = filter.m_displayBoundary;
        record.clipFrontPlane = filter.m_clipFrontPlane;
        record.clipBackPlane = filter.m_clipBackPlane;
        record.frontDistance = filter.m_frontDistance;
        record.backDistance = filter.m_backDistance;
        m_spatialFilters.push_back(record);
    }

    void addGeoData(const DRW_GeoData& geoData) {
        GeoDataRecord record;
        record.handle = geoData.handle;
        record.parentHandle = geoData.parentHandle;
        record.hostBlockHandle = geoData.m_hostBlockHandle;
        record.version = geoData.m_version;
        record.coordinatesType = geoData.m_coordinatesType;
        record.horizontalUnits = geoData.m_horizontalUnits;
        record.verticalUnits = geoData.m_verticalUnits;
        record.horizontalUnitScale = geoData.m_horizontalUnitScale;
        record.verticalUnitScale = geoData.m_verticalUnitScale;
        record.coordinateSystemDefinition = geoData.m_coordinateSystemDefinition;
        record.geoRssTag = geoData.m_geoRssTag;
        record.meshPointCount = geoData.m_points.size();
        record.meshFaceCount = geoData.m_faces.size();
        m_geoData.push_back(std::move(record));
    }

    void addTableGeometry(const DRW_TableGeometry& geometry) {
        TableGeometryRecord record;
        record.handle = geometry.handle;
        record.parentHandle = geometry.parentHandle;
        record.rowCount = geometry.m_rowCount;
        record.columnCount = geometry.m_columnCount;
        record.cellCount = geometry.m_cellCount;
        for (const DRW_TableGeometryCell& cell : geometry.m_cells) {
            record.contentCount += cell.m_contents.size();
        }
        m_tableGeometry.push_back(record);
    }

    void addAcDbPlaceholder(const DRW_AcDbPlaceholder& placeholder) {
        PlaceholderRecord record;
        record.handle = placeholder.handle;
        record.parentHandle = placeholder.parentHandle;
        m_placeholders.push_back(record);
    }

    const std::vector<RawObjectRecord>& rawObjects() const { return m_rawObjects; }
    const std::vector<ViewRecord>& views() const { return m_views; }
    const std::vector<LightRecord>& lights() const { return m_lights; }
    const std::vector<SunRecord>& suns() const { return m_suns; }
    const std::vector<ModelerGeometryRecord>& modelerGeometry() const { return m_modelerGeometry; }
    const std::vector<TableRecord>& tables() const { return m_tables; }
    const std::vector<AssociativeRecord>& associativeObjects() const { return m_associativeObjects; }
    const std::vector<AcShRecord>& acshObjects() const { return m_acshObjects; }
    const std::vector<MLeaderRecord>& mleaders() const { return m_mleaders; }
    const std::vector<MLeaderStyleRecord>& mleaderStyles() const { return m_mleaderStyles; }
    const std::vector<DetailViewStyleRecord>& detailViewStyles() const { return m_detailViewStyles; }
    const std::vector<SectionViewStyleRecord>& sectionViewStyles() const { return m_sectionViewStyles; }
    const std::vector<BreakDataRecord>& breakData() const { return m_breakData; }
    const std::vector<BreakPointRefRecord>& breakPointRefs() const { return m_breakPointRefs; }
    const std::vector<GroupRecord>& groups() const { return m_groups; }
    const std::vector<ImageDefinitionReactorRecord>& imageDefinitionReactors() const {
        return m_imageDefinitionReactors;
    }
    const std::vector<SpatialFilterRecord>& spatialFilters() const { return m_spatialFilters; }
    const std::vector<GeoDataRecord>& geoData() const { return m_geoData; }
    const std::vector<TableGeometryRecord>& tableGeometry() const { return m_tableGeometry; }
    const std::vector<PlaceholderRecord>& placeholders() const { return m_placeholders; }

    const ViewRecord* findViewByName(const std::string& name) const {
        for (const ViewRecord& record : m_views) {
            if (record.name == name)
                return &record;
        }
        return nullptr;
    }
    const SunRecord* findSunByHandle(duint32 handle) const {
        for (const SunRecord& record : m_suns) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    static ReplayBlocker rawReplayBlocker(const RawObjectRecord& record) {
        if (record.replayState == ReplayState::ReplayInvalidated)
            return ReplayBlocker::Invalidated;
        if (record.replayState == ReplayState::ReplayReplaced)
            return ReplayBlocker::Replaced;
        if (record.isEntity)
            return ReplayBlocker::EntityReplayUnsupported;
        if (record.rawBytes.empty())
            return ReplayBlocker::MissingRawBytes;
        if (record.isCustomClass && record.recordName.empty() && record.className.empty())
            return ReplayBlocker::MissingClassMetadata;
        return ReplayBlocker::None;
    }

    static AssociativeKind associativeKindFromRecordName(const std::string& recordName) {
        if (recordName == "ACDBASSOCNETWORK")
            return AssociativeKind::Network;
        if (recordName == "ACDBASSOCACTION")
            return AssociativeKind::Action;
        if (recordName == "ACDBASSOCDEPENDENCY")
            return AssociativeKind::Dependency;
        if (recordName == "ACDBASSOCGEOMDEPENDENCY")
            return AssociativeKind::GeometryDependency;
        if (recordName == "ACDBASSOCPERSSUBENTMANAGER"
            || recordName == "ACDBPERSSUBENTMANAGER") {
            return AssociativeKind::PersistentSubentityManager;
        }
        if (recordName == "ACDBASSOCALIGNEDDIMACTIONBODY")
            return AssociativeKind::AlignedDimensionActionBody;
        if (recordName == "ACDBASSOCVERTEXACTIONPARAM")
            return AssociativeKind::VertexActionParam;
        if (recordName == "ACDBASSOCOSNAPPOINTREFACTIONPARAM")
            return AssociativeKind::OsnapPointRefActionParam;
        return AssociativeKind::Unknown;
    }

    static const char* replayBlockerName(ReplayBlocker blocker) {
        switch (blocker) {
            case ReplayBlocker::None:
                return "none";
            case ReplayBlocker::Invalidated:
                return "invalidated";
            case ReplayBlocker::Replaced:
                return "replaced";
            case ReplayBlocker::EntityReplayUnsupported:
                return "entity replay unsupported";
            case ReplayBlocker::MissingRawBytes:
                return "missing raw bytes";
            case ReplayBlocker::MissingClassMetadata:
                return "missing class metadata";
            case ReplayBlocker::WriterRejected:
                return "writer rejected";
            case ReplayBlocker::SemanticOnly:
                return "semantic-only metadata";
        }
        return "unknown";
    }

    bool hasBlockedRawReplay() const {
        for (const RawObjectRecord& record : m_rawObjects) {
            if (rawReplayBlocker(record) != ReplayBlocker::None)
                return true;
        }
        return false;
    }

    size_t semanticOnlyRecordCount() const {
        return m_lights.size() + m_suns.size() + m_modelerGeometry.size()
            + m_tables.size() + m_associativeObjects.size() + m_acshObjects.size()
            + m_mleaderStyles.size() + m_detailViewStyles.size()
            + m_sectionViewStyles.size() + m_breakData.size()
            + m_breakPointRefs.size() + m_groups.size()
            + m_imageDefinitionReactors.size() + m_spatialFilters.size()
            + m_geoData.size() + m_tableGeometry.size() + m_placeholders.size();
    }

    bool hasReplayableAdvancedObjects() const {
        return hasReplayable(m_rawObjects)
            || hasReplayable(m_lights)
            || hasReplayable(m_suns)
            || hasReplayable(m_modelerGeometry)
            || hasReplayable(m_tables)
            || hasReplayable(m_associativeObjects)
            || hasReplayable(m_acshObjects)
            || hasReplayable(m_mleaders)
            || hasReplayable(m_mleaderStyles)
            || hasReplayable(m_detailViewStyles)
            || hasReplayable(m_sectionViewStyles)
            || hasReplayable(m_breakData)
            || hasReplayable(m_breakPointRefs)
            || hasReplayable(m_groups)
            || hasReplayable(m_imageDefinitionReactors)
            || hasReplayable(m_spatialFilters)
            || hasReplayable(m_geoData)
            || hasReplayable(m_tableGeometry)
            || hasReplayable(m_placeholders);
    }

    void invalidateByHandle(duint32 handle) {
        invalidateMatching([handle](duint32 recordHandle, duint32) {
            return recordHandle == handle;
        });
    }

    void invalidateByOwner(duint32 ownerHandle) {
        invalidateMatching([ownerHandle](duint32, duint32 parentHandle) {
            return parentHandle == ownerHandle;
        });
    }

    void invalidateAssociativeGraphForHandle(duint32 dependentHandle) {
        for (AssociativeRecord& record : m_associativeObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (associativeRecordReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateRawAssociativeObject(record.handle);
            }
        }
    }

    void invalidateAllReplayable() {
        invalidateContainer(m_rawObjects);
        invalidateContainer(m_views);
        invalidateContainer(m_lights);
        invalidateContainer(m_suns);
        invalidateContainer(m_modelerGeometry);
        invalidateContainer(m_tables);
        invalidateContainer(m_associativeObjects);
        invalidateContainer(m_acshObjects);
        invalidateContainer(m_mleaders);
        invalidateContainer(m_mleaderStyles);
        invalidateContainer(m_detailViewStyles);
        invalidateContainer(m_sectionViewStyles);
        invalidateContainer(m_breakData);
        invalidateContainer(m_breakPointRefs);
        invalidateContainer(m_groups);
        invalidateContainer(m_imageDefinitionReactors);
        invalidateContainer(m_spatialFilters);
        invalidateContainer(m_geoData);
        invalidateContainer(m_tableGeometry);
        invalidateContainer(m_placeholders);
    }

private:
    static void populateTableContentSummary(TableRecord& record,
                                            const DRW_TableContent& content) {
        record.fieldHandleCount = content.m_fieldHandles.size();
        record.fieldHandles = content.m_fieldHandles;
        record.mergedRangeCount = content.m_mergedRanges.size();
        record.columnWidths.reserve(content.m_columns.size());
        for (const DRW_TableColumn& column : content.m_columns)
            record.columnWidths.push_back(column.m_width);
        record.mergedRanges.reserve(content.m_mergedRanges.size());
        for (const DRW_TableMergedRange& range : content.m_mergedRanges) {
            record.mergedRanges.push_back({range.m_topRow, range.m_leftColumn,
                                           range.m_bottomRow, range.m_rightColumn});
        }
        record.rowHeights.reserve(content.m_rows.size());
        for (const DRW_TableRow& row : content.m_rows) {
            record.rowHeights.push_back(row.m_height);
            record.cellCount += row.m_cells.size();
            for (const DRW_TableCell& cell : row.m_cells) {
                record.contentCount += cell.m_contents.size();
                record.attributeCount += cell.m_attributes.size();
                if (cell.m_styleId != 0)
                    record.cellStyleIds.push_back(cell.m_styleId);
                if (cell.m_valueHandle != 0) {
                    ++record.valueHandleCount;
                    record.valueHandles.push_back(cell.m_valueHandle);
                }
                if (cell.m_blockHandle != 0) {
                    ++record.blockHandleCount;
                    record.blockHandles.push_back(cell.m_blockHandle);
                }
                if (cell.m_blockHandle != 0)
                    record.hasBlockContent = true;
                if (cell.m_overrideFlags != 0)
                    ++record.overrideCellCount;
                if (cell.m_geometryFlags != 0 || cell.m_geometryHandle != 0) {
                    ++record.geometryCellCount;
                    if (cell.m_geometryHandle != 0)
                        record.geometryHandles.push_back(cell.m_geometryHandle);
                }
                for (const DRW_TableCellAttribute& attribute : cell.m_attributes) {
                    if (!attribute.m_text.empty())
                        record.attributeTexts.push_back(attribute.m_text);
                }
                for (const DRW_TableCellContent& cellContent : cell.m_contents) {
                    if (!cellContent.m_text.empty()) {
                        ++record.textContentCount;
                        record.hasTextContent = true;
                        record.cellTexts.push_back(cellContent.m_text);
                    }
                    if (cellContent.m_type == 2) {
                        ++record.fieldContentCount;
                        if (cellContent.m_handle != 0)
                            record.fieldHandles.push_back(cellContent.m_handle);
                    }
                    if (cellContent.m_type == 4) {
                        ++record.blockContentCount;
                        record.hasBlockContent = true;
                        if (cellContent.m_handle != 0)
                            record.blockHandles.push_back(cellContent.m_handle);
                    }
                }
            }
        }
        record.fieldHandleCount = record.fieldHandles.size();
        record.blockHandleCount = record.blockHandles.size();
    }

    template<typename Container>
    static bool hasReplayable(const Container& container) {
        for (const auto& record : container) {
            if (record.replayState == ReplayState::ReplayAllowed)
                return true;
        }
        return false;
    }

    template<typename Container>
    static void invalidateContainer(Container& container) {
        for (auto& record : container) {
            if (record.replayState == ReplayState::ReplayAllowed)
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    static bool associativeRecordReferences(const AssociativeRecord& record,
                                            duint32 handle) {
        if (handle == 0)
            return false;
        if (record.owningNetworkHandle == handle
            || record.actionBodyHandle == handle
            || record.dependencyHandle == handle
            || record.readDependencyHandle == handle
            || record.writeDependencyHandle == handle
            || record.rNodeHandle == handle
            || record.dNodeHandle == handle) {
            return true;
        }
        for (const DRW_AssociativeHandleRef& ref : record.dependencyRefs) {
            if (ref.m_handle == handle)
                return true;
        }
        for (const DRW_AssociativeHandleRef& ref : record.actionRefs) {
            if (ref.m_handle == handle)
                return true;
        }
        for (duint32 refHandle : record.ownedParamHandles) {
            if (refHandle == handle)
                return true;
        }
        for (duint32 refHandle : record.ownedActionHandles) {
            if (refHandle == handle)
                return true;
        }
        return false;
    }

    static bool isAssociativeRawObject(const RawObjectRecord& record) {
        if (associativeKindFromRecordName(record.recordName) != AssociativeKind::Unknown)
            return true;
        return record.className.find("Assoc") != std::string::npos
               || record.className.find("PersSubent") != std::string::npos;
    }

    void invalidateRawAssociativeObject(duint32 handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (record.handle == handle && isAssociativeRawObject(record))
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    void resolveMLeaderStyle(MLeaderRecord& record) const {
        for (const MLeaderStyleRecord& style : m_mleaderStyles) {
            if (resolveMLeaderStyle(record, style))
                return;
        }
    }

    static bool resolveMLeaderStyle(MLeaderRecord& record,
                                    const MLeaderStyleRecord& style) {
        if (style.handle == 0 || style.handle != record.styleHandle)
            return false;
        record.styleResolved = true;
        if (record.effectiveContentType == 0)
            record.effectiveContentType = style.contentType;
        if (record.effectiveLeaderType == 0)
            record.effectiveLeaderType = style.leaderType;
        if (record.effectiveLeaderLineTypeHandle == 0)
            record.effectiveLeaderLineTypeHandle = style.leaderLineTypeHandle;
        if (record.effectiveArrowHeadHandle == 0)
            record.effectiveArrowHeadHandle = style.arrowHeadBlockHandle;
        if (record.effectiveTextStyleHandle == 0)
            record.effectiveTextStyleHandle = style.textStyleHandle;
        if (record.effectiveBlockHandle == 0)
            record.effectiveBlockHandle = style.blockHandle;
        return true;
    }

    template<typename Predicate>
    void invalidateMatching(Predicate predicate) {
        invalidateMatching(m_rawObjects, predicate);
        invalidateMatching(m_views, predicate);
        invalidateMatching(m_lights, predicate);
        invalidateMatching(m_suns, predicate);
        invalidateMatching(m_modelerGeometry, predicate);
        invalidateMatching(m_tables, predicate);
        invalidateMatching(m_associativeObjects, predicate);
        invalidateMatching(m_acshObjects, predicate);
        invalidateMatching(m_mleaders, predicate);
        invalidateMatching(m_mleaderStyles, predicate);
        invalidateMatching(m_detailViewStyles, predicate);
        invalidateMatching(m_sectionViewStyles, predicate);
        invalidateMatching(m_breakData, predicate);
        invalidateMatching(m_breakPointRefs, predicate);
        invalidateMatching(m_groups, predicate);
        invalidateMatching(m_imageDefinitionReactors, predicate);
        invalidateMatching(m_spatialFilters, predicate);
        invalidateMatching(m_geoData, predicate);
        invalidateMatching(m_tableGeometry, predicate);
        invalidateMatching(m_placeholders, predicate);
    }

    template<typename Container, typename Predicate>
    static void invalidateMatching(Container& container, Predicate predicate) {
        for (auto& record : container) {
            if (record.replayState == ReplayState::ReplayAllowed
                && predicate(record.handle, record.parentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
            }
        }
    }

    std::vector<RawObjectRecord> m_rawObjects;
    std::vector<ViewRecord> m_views;
    std::vector<LightRecord> m_lights;
    std::vector<SunRecord> m_suns;
    std::vector<ModelerGeometryRecord> m_modelerGeometry;
    std::vector<TableRecord> m_tables;
    std::vector<AssociativeRecord> m_associativeObjects;
    std::vector<AcShRecord> m_acshObjects;
    std::vector<MLeaderRecord> m_mleaders;
    std::vector<MLeaderStyleRecord> m_mleaderStyles;
    std::vector<DetailViewStyleRecord> m_detailViewStyles;
    std::vector<SectionViewStyleRecord> m_sectionViewStyles;
    std::vector<BreakDataRecord> m_breakData;
    std::vector<BreakPointRefRecord> m_breakPointRefs;
    std::vector<GroupRecord> m_groups;
    std::vector<ImageDefinitionReactorRecord> m_imageDefinitionReactors;
    std::vector<SpatialFilterRecord> m_spatialFilters;
    std::vector<GeoDataRecord> m_geoData;
    std::vector<TableGeometryRecord> m_tableGeometry;
    std::vector<PlaceholderRecord> m_placeholders;
};

#endif
