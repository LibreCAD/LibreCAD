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
        duint32 namedUcsHandle = 0;
        duint32 baseUcsHandle = 0;
        duint32 backgroundHandle = 0;
        duint32 visualStyleHandle = 0;
        duint32 sunHandle = 0;
        duint32 liveSectionHandle = 0;
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
        std::string name;
        duint32 type = 0;
        bool status = false;
        bool hasPhotometricData = false;
        std::string webFile;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SunRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        bool isOn = false;
        duint32 color = 0;
        double intensity = 0.0;
        bool hasShadow = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ModelerGeometryRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        DRW::ETYPE type = DRW::UNKNOWN;
        duint16 modelerVersion = 0;
        bool isEmpty = false;
        bool hasWireframe = false;
        duint32 historyHandle = 0;
        std::vector<duint8> rawBytes;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        int rowCount = 0;
        int columnCount = 0;
        bool semanticParsed = false;
        bool styleResolved = false;
        bool fallbackRendered = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativeRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        duint16 classVersion = 0;
        duint32 owningNetworkHandle = 0;
        duint32 actionBodyHandle = 0;
        size_t dependencyCount = 0;
        size_t actionCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AcShRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        std::string recordName;
        duint32 ownerHandle = 0;
        duint32 historyNodeId = 0;
        size_t blobBytes = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct MLeaderRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        duint32 styleHandle = 0;
        size_t rootCount = 0;
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
        record.namedUcsHandle = view.namedUCS_ID;
        record.baseUcsHandle = view.baseUCS_ID;
        record.backgroundHandle = view.m_backgroundHandle;
        record.visualStyleHandle = view.m_visualStyleHandle;
        record.sunHandle = view.m_sunHandle;
        record.liveSectionHandle = view.m_liveSectionHandle;
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
        record.name = light.m_name;
        record.type = light.m_type;
        record.status = light.m_status;
        record.hasPhotometricData = light.m_hasPhotometricData;
        record.webFile = light.m_webFile;
        m_lights.push_back(record);
    }

    void addSun(const DRW_Sun& sun) {
        SunRecord record;
        record.handle = sun.handle;
        record.parentHandle = sun.parentHandle;
        record.isOn = sun.m_isOn;
        record.color = sun.m_color;
        record.intensity = sun.m_intensity;
        record.hasShadow = sun.m_hasShadow;
        m_suns.push_back(record);
    }

    void addModelerGeometry(const DRW_ModelerGeometry& geometry) {
        ModelerGeometryRecord record;
        record.handle = geometry.handle;
        record.parentHandle = geometry.parentHandle;
        record.type = geometry.eType;
        record.modelerVersion = geometry.m_modelerVersion;
        record.isEmpty = geometry.m_isEmpty;
        record.hasWireframe = geometry.m_hasWireframe;
        record.historyHandle = geometry.m_historyHandle;
        record.rawBytes = geometry.m_rawBytes;
        m_modelerGeometry.push_back(std::move(record));
    }

    void addTableStyle(const DRW_TableStyle& style) {
        TableRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.recordName = style.m_name;
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
        record.semanticParsed = table.m_parseComplete;
        record.fallbackRendered = false;
        m_tables.push_back(record);
    }

    void addAssociativeObject(const DRW_AssociativeObject& object) {
        AssociativeRecord record;
        record.handle = object.handle;
        record.parentHandle = object.parentHandle;
        record.recordName = object.m_recordName;
        record.classVersion = object.m_classVersion;
        record.owningNetworkHandle = object.m_owningNetworkHandle;
        record.actionBodyHandle = object.m_actionBodyHandle;
        record.dependencyCount = object.m_dependencies.size();
        record.actionCount = object.m_actions.size();
        m_associativeObjects.push_back(record);
    }

    void addAcShObject(const DRW_AcShHistoryObject& object) {
        AcShRecord record;
        record.handle = object.handle;
        record.parentHandle = object.parentHandle;
        record.recordName = object.m_recordName;
        record.ownerHandle = object.m_ownerHandle;
        record.historyNodeId = object.m_historyNodeId;
        record.blobBytes = object.m_binaryBlob1.size() + object.m_binaryBlob2.size();
        m_acshObjects.push_back(record);
    }

    void addMLeader(const DRW_MLeader& mleader) {
        MLeaderRecord record;
        record.handle = mleader.handle;
        record.parentHandle = mleader.parentHandle;
        record.styleHandle = mleader.styleHandle.ref;
        record.rootCount = mleader.context.roots.size();
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
