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

#include <algorithm>
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

    enum class AssociativeEdgeKind {
        OwnsAction,
        OwnsParameter,
        DependsOn,
        ReadDependency,
        WriteDependency,
        ActionBody,
        EvaluationExpression,
        HistoryNode,
        GeometryReference,
        UnknownHandleReference
    };

    enum class AssociativeEdgeConfidence {
        Unknown,
        ExplicitHandle,
        InferredFromClassLayout
    };

    enum class AssociativePrefixKind {
        AcDbAssocAction,
        AcDbAssocActionParam,
        AcDbAssocDependency,
        AcDbAssocGeomDependency,
        AcDbAssocNetwork,
        AcDbAssocActionBody,
        AcDbEvalExpr,
        AcDbShHistoryNode,
        AcShActionBody
    };

    enum class AssociativePrefixParseStatus {
        Complete,
        Partial,
        Missing,
        UnsupportedVersion,
        BoundedCountOverflow
    };

    enum class ModelerPayloadKind {
        Unknown,
        Sat,
        Sab
    };

    enum class ModelerPayloadSection {
        Unknown,
        Body,
        HandleStream
    };

    enum class ModelerPayloadRangeKind {
        Sat,
        Sab,
        History,
        Wire,
        Silhouette,
        UnknownTail,
        HandleStream
    };

    enum class ModelerPayloadRangeConsistency {
        Unknown,
        Exact,
        Truncated,
        Overrun
    };

    enum class ModelerPayloadRangeConfidence {
        Unknown,
        Marker,
        DeclaredSize,
        Inferred
    };

    enum class RawObjectFamily {
        Unknown,
        Associative,
        EvaluationGraph,
        DynamicBlock,
        ObjectContext
    };

    enum class TableFallbackRole {
        GridLine,
        CellText,
        Placeholder,
        Boundary
    };

    enum class TableContentStorageMode {
        LegacyDirectTable,
        SeparateTableContent,
        EmbeddedTableContent,
        Unsupported
    };

    enum class TableNativeWriterBlocker {
        NoSemanticTableContent,
        MissingFallbackAttachment,
        EditedFallback,
        UnresolvedTableStyle,
        UnresolvedCellStyleMap,
        UnknownSubrecordRange,
        IncompleteSubrecordRange,
        OverrideMask,
        BreakData,
        GeometryTail,
        MergedCell,
        FieldContent,
        BlockContent,
        AttributeContent,
        UnknownCellContent,
        IncompleteValuePayload,
        MissingOwnerHandle,
        UnsupportedTableVersion,
        AmbiguousTableContentStorage,
        AnonymousBlockPolicyUnresolved,
        UnresolvedTextStyle,
        UnresolvedLineType,
        RawReplayInvalidated,
        RawReplayReplaced,
        NonPositiveDimension
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
        RawObjectFamily family = RawObjectFamily::Unknown;
        std::string recordName;
        std::string className;
        std::vector<duint8> rawBytes;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct RawObjectFamilyCounts {
        size_t unknown = 0;
        size_t associative = 0;
        size_t evaluationGraph = 0;
        size_t dynamicBlock = 0;
        size_t objectContext = 0;

        size_t total() const {
            return unknown + associative + evaluationGraph + dynamicBlock + objectContext;
        }
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

    struct ModelerPayloadRangeRecord {
        ModelerPayloadRangeKind kind = ModelerPayloadRangeKind::UnknownTail;
        ModelerPayloadSection section = ModelerPayloadSection::Unknown;
        size_t offset = 0;
        size_t length = 0;
        size_t declaredByteSize = 0;
        ModelerPayloadRangeConsistency consistency =
            ModelerPayloadRangeConsistency::Unknown;
        ModelerPayloadRangeConfidence confidence =
            ModelerPayloadRangeConfidence::Unknown;
        std::string markerText;
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
        ModelerPayloadKind payloadKind = ModelerPayloadKind::Unknown;
        size_t markerOffset = 0;
        size_t markerLength = 0;
        std::string markerText;
        bool rawByteSplitKnown = false;
        bool rawByteSplitConsistent = true;
        size_t rawBodyByteCount = 0;
        size_t rawHandleByteCount = 0;
        ModelerPayloadSection markerSection = ModelerPayloadSection::Unknown;
        duint32 historyHandle = 0;
        size_t rawByteCount = 0;
        std::vector<duint8> rawBytes;
        std::vector<ModelerPayloadRangeRecord> payloadRanges;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableMergedRangeRecord {
        int topRow = 0;
        int leftColumn = 0;
        int bottomRow = 0;
        int rightColumn = 0;
    };

    struct TableCellRecord {
        int row = 0;
        int column = 0;
        int flags = 0;
        int type = 0;
        int styleId = 0;
        duint32 overrideFlags = 0;
        duint32 valueHandle = 0;
        duint32 textStyleHandle = 0;
        duint32 textStyleOverrideHandle = 0;
        duint32 blockHandle = 0;
        duint32 geometryHandle = 0;
        bool isMerged = false;
        bool autoFit = false;
        size_t contentCount = 0;
        size_t textContentCount = 0;
        size_t fieldContentCount = 0;
        size_t blockContentCount = 0;
        size_t attributeCount = 0;
        size_t attributeHandleCount = 0;
        size_t unknownContentCount = 0;
        std::vector<std::string> texts;
        std::vector<std::string> attributeTexts;
        std::vector<duint32> contentHandles;
        std::vector<duint32> attributeHandles;
    };

    struct TableWriterBlockerCounts {
        size_t tableCount = 0;
        size_t fallbackRendered = 0;
        size_t incompleteSemanticParse = 0;
        size_t unresolvedStyle = 0;
        size_t fieldContent = 0;
        size_t blockContent = 0;
        size_t attributeContent = 0;
        size_t overrideCells = 0;
        size_t geometryCells = 0;
        size_t unknownRanges = 0;
        size_t incompleteRanges = 0;
        size_t overrideMasks = 0;
        size_t breakRanges = 0;
        size_t tableGeometryTailRanges = 0;
        size_t editedFallbackEntities = 0;
        size_t missingFallbackAttachments = 0;

        size_t totalBlockers() const {
            return fallbackRendered + incompleteSemanticParse + unresolvedStyle
                   + fieldContent + blockContent + attributeContent
                   + overrideCells + geometryCells + unknownRanges
                   + incompleteRanges + overrideMasks + breakRanges
                   + tableGeometryTailRanges + editedFallbackEntities
                   + missingFallbackAttachments;
        }
    };

    struct TableNativeWriterEligibility {
        duint32 tableHandle = 0;
        std::string recordName;
        DRW::Version writerVersion = DRW::UNKNOWNV;
        TableContentStorageMode storageMode =
            TableContentStorageMode::Unsupported;
        bool eligibleTextOnly = false;
        std::vector<TableNativeWriterBlocker> blockers;

        bool hasBlocker(TableNativeWriterBlocker blocker) const {
            return std::find(blockers.begin(), blockers.end(), blocker)
                   != blockers.end();
        }
    };

    struct TableNativeWriterBlockerCounts {
        size_t tableCount = 0;
        size_t eligibleTextOnly = 0;
        size_t legacyDirectLayout = 0;
        size_t separateTableContentLayout = 0;
        size_t embeddedTableContentLayout = 0;
        size_t unsupportedLayout = 0;
        size_t noSemanticTableContent = 0;
        size_t missingFallbackAttachment = 0;
        size_t editedFallback = 0;
        size_t unresolvedTableStyle = 0;
        size_t unresolvedCellStyleMap = 0;
        size_t unknownSubrecordRange = 0;
        size_t incompleteSubrecordRange = 0;
        size_t overrideMask = 0;
        size_t breakData = 0;
        size_t geometryTail = 0;
        size_t mergedCell = 0;
        size_t fieldContent = 0;
        size_t blockContent = 0;
        size_t attributeContent = 0;
        size_t unknownCellContent = 0;
        size_t incompleteValuePayload = 0;
        size_t missingOwnerHandle = 0;
        size_t unsupportedTableVersion = 0;
        size_t ambiguousTableContentStorage = 0;
        size_t anonymousBlockPolicyUnresolved = 0;
        size_t unresolvedTextStyle = 0;
        size_t unresolvedLineType = 0;
        size_t rawReplayInvalidated = 0;
        size_t rawReplayReplaced = 0;
        size_t nonPositiveDimension = 0;

        size_t totalBlockers() const {
            return noSemanticTableContent + missingFallbackAttachment
                   + editedFallback + unresolvedTableStyle
                   + unresolvedCellStyleMap + unknownSubrecordRange
                   + incompleteSubrecordRange + overrideMask + breakData
                   + geometryTail + mergedCell + fieldContent + blockContent
                   + attributeContent + unknownCellContent
                   + incompleteValuePayload + missingOwnerHandle
                   + unsupportedTableVersion + ambiguousTableContentStorage
                   + anonymousBlockPolicyUnresolved + unresolvedTextStyle
                   + unresolvedLineType + rawReplayInvalidated
                   + rawReplayReplaced + nonPositiveDimension;
        }

        size_t countFor(TableNativeWriterBlocker blocker) const {
            switch (blocker) {
            case TableNativeWriterBlocker::NoSemanticTableContent:
                return noSemanticTableContent;
            case TableNativeWriterBlocker::MissingFallbackAttachment:
                return missingFallbackAttachment;
            case TableNativeWriterBlocker::EditedFallback:
                return editedFallback;
            case TableNativeWriterBlocker::UnresolvedTableStyle:
                return unresolvedTableStyle;
            case TableNativeWriterBlocker::UnresolvedCellStyleMap:
                return unresolvedCellStyleMap;
            case TableNativeWriterBlocker::UnknownSubrecordRange:
                return unknownSubrecordRange;
            case TableNativeWriterBlocker::IncompleteSubrecordRange:
                return incompleteSubrecordRange;
            case TableNativeWriterBlocker::OverrideMask:
                return overrideMask;
            case TableNativeWriterBlocker::BreakData:
                return breakData;
            case TableNativeWriterBlocker::GeometryTail:
                return geometryTail;
            case TableNativeWriterBlocker::MergedCell:
                return mergedCell;
            case TableNativeWriterBlocker::FieldContent:
                return fieldContent;
            case TableNativeWriterBlocker::BlockContent:
                return blockContent;
            case TableNativeWriterBlocker::AttributeContent:
                return attributeContent;
            case TableNativeWriterBlocker::UnknownCellContent:
                return unknownCellContent;
            case TableNativeWriterBlocker::IncompleteValuePayload:
                return incompleteValuePayload;
            case TableNativeWriterBlocker::MissingOwnerHandle:
                return missingOwnerHandle;
            case TableNativeWriterBlocker::UnsupportedTableVersion:
                return unsupportedTableVersion;
            case TableNativeWriterBlocker::AmbiguousTableContentStorage:
                return ambiguousTableContentStorage;
            case TableNativeWriterBlocker::AnonymousBlockPolicyUnresolved:
                return anonymousBlockPolicyUnresolved;
            case TableNativeWriterBlocker::UnresolvedTextStyle:
                return unresolvedTextStyle;
            case TableNativeWriterBlocker::UnresolvedLineType:
                return unresolvedLineType;
            case TableNativeWriterBlocker::RawReplayInvalidated:
                return rawReplayInvalidated;
            case TableNativeWriterBlocker::RawReplayReplaced:
                return rawReplayReplaced;
            case TableNativeWriterBlocker::NonPositiveDimension:
                return nonPositiveDimension;
            }
            return 0;
        }
    };

    struct MLeaderWriterBlockerCounts {
        size_t mleaderCount = 0;
        size_t unresolvedStyle = 0;
        size_t missingTextContent = 0;
        size_t blockContent = 0;
        size_t toleranceContent = 0;
        size_t overrideFlags = 0;
        size_t missingLeaderGeometry = 0;
        size_t invalidated = 0;
        size_t replaced = 0;

        size_t totalBlockers() const {
            return unresolvedStyle + missingTextContent + blockContent
                   + toleranceContent + overrideFlags + missingLeaderGeometry
                   + invalidated + replaced;
        }
    };

    struct ModelerPayloadCounts {
        size_t recordCount = 0;
        size_t sat = 0;
        size_t sab = 0;
        size_t unknown = 0;
        size_t rangeCount = 0;
        size_t satRanges = 0;
        size_t sabRanges = 0;
        size_t historyRanges = 0;
        size_t wireRanges = 0;
        size_t silhouetteRanges = 0;
        size_t unknownTailRanges = 0;
        size_t handleStreamRanges = 0;
        size_t exactRanges = 0;
        size_t truncatedRanges = 0;
        size_t overrunRanges = 0;
        size_t inconsistentSplit = 0;
        size_t markerInBody = 0;
        size_t markerInHandleStream = 0;
    };

    struct AssociativeShellCounts {
        size_t recordCount = 0;
        size_t unknown = 0;
        size_t network = 0;
        size_t action = 0;
        size_t dependency = 0;
        size_t geometryDependency = 0;
        size_t persistentSubentityManager = 0;
        size_t alignedDimensionActionBody = 0;
        size_t vertexActionParam = 0;
        size_t osnapPointRefActionParam = 0;
        size_t valueParamRecords = 0;
        size_t parsedValueParamRecords = 0;
        size_t actionParamRecords = 0;
        size_t parsedActionParamPrefixes = 0;
        size_t singleDependencyActionParamPrefixes = 0;
        size_t compoundActionParamPrefixes = 0;
    };

    struct AssociativeEdgeCounts {
        size_t edgeCount = 0;
        size_t ownsAction = 0;
        size_t ownsParameter = 0;
        size_t dependsOn = 0;
        size_t readDependency = 0;
        size_t writeDependency = 0;
        size_t actionBody = 0;
        size_t evaluationExpression = 0;
        size_t historyNode = 0;
        size_t geometryReference = 0;
        size_t unknownHandleReference = 0;
        size_t explicitHandle = 0;
        size_t inferredFromClassLayout = 0;
        size_t invalidated = 0;
    };

    struct AssociativePrefixCounts {
        size_t prefixCount = 0;
        size_t assocAction = 0;
        size_t assocActionParam = 0;
        size_t assocDependency = 0;
        size_t assocGeomDependency = 0;
        size_t assocNetwork = 0;
        size_t assocActionBody = 0;
        size_t evalExpr = 0;
        size_t shHistoryNode = 0;
        size_t shActionBody = 0;
        size_t complete = 0;
        size_t partial = 0;
        size_t missing = 0;
        size_t unsupportedVersion = 0;
        size_t boundedCountOverflow = 0;
        size_t decodedHandleCount = 0;
        size_t decodedValueCount = 0;
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
        size_t unknownContentCount = 0;
        size_t valueHandleCount = 0;
        size_t blockHandleCount = 0;
        size_t fieldHandleCount = 0;
        size_t attributeHandleCount = 0;
        size_t textStyleHandleCount = 0;
        size_t lineTypeHandleCount = 0;
        size_t mergedRangeCount = 0;
        size_t overrideCellCount = 0;
        size_t geometryCellCount = 0;
        size_t rowStyleCount = 0;
        size_t cellStyleCount = 0;
        size_t borderCount = 0;
        size_t m_tableContentFormatCount = 0;
        size_t m_tableNamedCellStyleCount = 0;
        size_t m_tableVisibleBorderCount = 0;
        size_t m_tableMarginStyleCount = 0;
        size_t m_unknownRangeCount = 0;
        size_t m_incompleteRangeCount = 0;
        size_t m_overrideMaskCount = 0;
        size_t m_breakRangeCount = 0;
        size_t m_tableGeometryTailRangeCount = 0;
        size_t m_fallbackGridEntityCount = 0;
        size_t m_fallbackTextEntityCount = 0;
        size_t m_fallbackPlaceholderEntityCount = 0;
        size_t m_fallbackUnresolvedTextStyleCount = 0;
        size_t m_fallbackClampedDimensionCount = 0;
        int m_tableFlowDirection = 0;
        int m_tableFlags = 0;
        double m_tableHorizontalCellMargin = 0.0;
        double m_tableVerticalCellMargin = 0.0;
        bool titleSuppressed = false;
        bool headerSuppressed = false;
        bool hasTextContent = false;
        bool hasBlockContent = false;
        bool rawReplayAvailable = false;
        bool semanticParsed = false;
        bool styleResolved = false;
        bool fallbackRendered = false;
        bool fallbackInvalidated = false;
        std::vector<double> columnWidths;
        std::vector<double> rowHeights;
        std::vector<int> cellStyleIds;
        std::vector<std::string> cellTexts;
        std::vector<std::string> attributeTexts;
        std::vector<duint32> valueHandles;
        std::vector<duint32> blockHandles;
        std::vector<duint32> fieldHandles;
        std::vector<duint32> attributeHandles;
        std::vector<duint32> textStyleHandles;
        std::vector<duint32> lineTypeHandles;
        std::vector<duint32> geometryHandles;
        std::vector<int> m_tableStyleIds;
        std::vector<std::string> m_tableStyleNames;
        std::vector<double> m_tableTextHeights;
        std::vector<int> m_tableAlignments;
        std::vector<int> m_tableColors;
        std::vector<TableMergedRangeRecord> mergedRanges;
        std::vector<TableCellRecord> cells;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableFallbackRenderSummary {
        duint32 tableHandle = 0;
        size_t gridEntityCount = 0;
        size_t textEntityCount = 0;
        size_t placeholderEntityCount = 0;
        size_t unresolvedTextStyleCount = 0;
        size_t clampedDimensionCount = 0;
    };

    struct TableFallbackEntityRecord {
        duint32 tableHandle = 0;
        duint32 sourceHandle = 0;
        int row = -1;
        int column = -1;
        TableFallbackRole role = TableFallbackRole::Placeholder;
        unsigned long long entityId = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct CellStyleMapRecord {
        duint32 handle = 0;
        duint32 parentHandle = 0;
        size_t m_cellStyleCount = 0;
        size_t m_borderCount = 0;
        size_t m_contentFormatCount = 0;
        size_t m_namedCellStyleCount = 0;
        size_t m_visibleBorderCount = 0;
        size_t m_marginStyleCount = 0;
        size_t m_unknownRangeCount = 0;
        size_t m_incompleteRangeCount = 0;
        std::vector<int> m_styleIds;
        std::vector<int> m_styleClasses;
        std::vector<std::string> m_styleNames;
        std::vector<duint32> m_textStyleHandles;
        std::vector<duint32> m_lineTypeHandles;
        std::vector<double> m_textHeights;
        std::vector<int> m_alignments;
        std::vector<int> m_colors;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativePrefixStatusRecord {
        AssociativePrefixKind kind = AssociativePrefixKind::AcDbAssocAction;
        AssociativePrefixParseStatus status =
            AssociativePrefixParseStatus::Missing;
        duint64 startBit = 0;
        duint64 bitSize = 0;
        duint16 classVersion = 0;
        size_t decodedHandleCount = 0;
        size_t decodedValueCount = 0;
        dint32 decodedCountValue = 0;
        std::string sourceAssumption;
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
        size_t valueParamCount = 0;
        size_t ownedParamPrefixCount = 0;
        bool valueParamsParsed = false;
        bool actionParamPrefixParsed = false;
        bool singleDependencyActionParamParsed = false;
        bool compoundActionParamParsed = false;
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
        std::vector<AssociativePrefixStatusRecord> prefixStatuses;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativeEdgeRecord {
        duint32 sourceHandle = 0;
        duint32 targetHandle = 0;
        std::string sourceRecordName;
        AssociativeKind sourceKind = AssociativeKind::Unknown;
        AssociativeEdgeKind edgeKind =
            AssociativeEdgeKind::UnknownHandleReference;
        int rawRangeIndex = -1;
        AssociativeEdgeConfidence confidence =
            AssociativeEdgeConfidence::Unknown;
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
        std::vector<AssociativePrefixStatusRecord> prefixStatuses;
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
        duint16 drawMLeaderOrder = 0;
        duint16 drawLeaderOrder = 0;
        dint32 maxLeaderPoints = 0;
        double firstSegmentAngle = 0.0;
        double secondSegmentAngle = 0.0;
        duint16 leaderType = 0;
        int leaderColor = 0;
        duint32 leaderLineTypeHandle = 0;
        dint32 leaderLineWeight = 0;
        bool landingEnabled = false;
        double landingGap = 0.0;
        bool autoIncludeLanding = false;
        double landingDistance = 0.0;
        std::string description;
        duint32 arrowHeadBlockHandle = 0;
        duint32 textStyleHandle = 0;
        duint32 blockHandle = 0;
        double arrowHeadSize = 0.0;
        std::string textDefault;
        duint16 leftAttachment = 0;
        duint16 rightAttachment = 0;
        duint16 textAngleType = 0;
        duint16 textAlignmentType = 0;
        int textColor = 0;
        double textHeight = 0.0;
        bool textFrameEnabled = false;
        bool alwaysAlignTextLeft = false;
        double alignSpace = 0.0;
        int blockColor = 0;
        DRW_Coord blockScale{1.0, 1.0, 1.0};
        bool blockScaleEnabled = false;
        double blockRotation = 0.0;
        bool blockRotationEnabled = false;
        duint16 blockConnectionType = 0;
        double scaleFactor = 1.0;
        bool propertyChanged = false;
        bool isAnnotative = false;
        double breakSize = 0.0;
        duint16 attachmentDirection = 0;
        duint16 topAttachment = 0;
        duint16 bottomAttachment = 0;
        bool textExtended = false;
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
        m_tableFallbackEntities.clear();
        m_associativeObjects.clear();
        m_associativeEdges.clear();
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
        record.family = rawObjectFamilyFromNames(record.recordName, record.className);
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
        const ModelerPayloadMarker marker = scanModelerPayloadMarker(
            geometry.m_rawBytes);
        record.payloadKind = marker.kind;
        record.markerOffset = marker.offset;
        record.markerLength = marker.length;
        record.markerText = marker.text;
        record.historyHandle = geometry.m_historyHandle;
        record.rawByteCount = geometry.m_rawBytes.size();
        const ModelerRawByteSplit split = splitModelerRawBytes(
            geometry.m_bodyBitSize, record.rawByteCount);
        record.rawByteSplitKnown = split.known;
        record.rawByteSplitConsistent = split.consistent;
        record.rawBodyByteCount = split.bodyByteCount;
        record.rawHandleByteCount = split.handleByteCount;
        record.markerSection = modelerPayloadSectionForMarker(
            marker, split, record.rawByteCount);
        record.payloadRanges = scanModelerPayloadRanges(
            geometry.m_rawBytes, geometry.m_bodyBitSize,
            geometry.m_hasWireframe);
        record.rawBytes = geometry.m_rawBytes;
        m_modelerGeometry.push_back(std::move(record));
    }

    void addTableStyle(const DRW_TableStyle& style) {
        TableRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.recordName = style.m_name;
        record.m_tableFlowDirection = style.m_flowDirection;
        record.m_tableFlags = style.m_flags;
        record.m_tableHorizontalCellMargin = style.m_horizontalCellMargin;
        record.m_tableVerticalCellMargin = style.m_verticalCellMargin;
        collectTableSubrecordRangeSummary(record, style.m_subrecordRanges);
        record.rowStyleCount = style.m_rowStyles.size();
        record.cellStyleCount = style.m_cellStyles.size();
        record.borderCount = style.m_tableCellStyle.m_borders.size();
        collectTableStyleCellHandles(record, style.m_tableCellStyle);
        collectTableStyleCellSummary(record, style.m_tableCellStyle);
        for (const DRW_TableStyleRowStyle& rowStyle : style.m_rowStyles) {
            record.borderCount += rowStyle.m_borders.size();
            collectTableStyleRowHandles(record, rowStyle);
            collectTableStyleRowSummary(record, rowStyle);
        }
        for (const DRW_TableStyleCellStyle& cellStyle : style.m_cellStyles) {
            record.borderCount += cellStyle.m_borders.size();
            collectTableStyleCellHandles(record, cellStyle);
            collectTableStyleCellSummary(record, cellStyle);
        }
        record.textStyleHandleCount = record.textStyleHandles.size();
        record.lineTypeHandleCount = record.lineTypeHandles.size();
        record.m_tableNamedCellStyleCount = record.m_tableStyleNames.size();
        record.titleSuppressed = style.m_titleSuppressed;
        record.headerSuppressed = style.m_headerSuppressed;
        record.semanticParsed = true;
        record.styleResolved = true;
        m_tables.push_back(record);
        for (TableRecord& table : m_tables)
            resolveTableStyle(table, record);
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
        resolveTableStyle(record);
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
        resolveTableStyle(record);
        record.fallbackRendered = false;
        m_tables.push_back(record);
    }

    void addTableFallbackEntity(const TableFallbackEntityRecord& record) {
        if (record.tableHandle == 0 || record.entityId == 0)
            return;
        m_tableFallbackEntities.push_back(record);
    }

    void updateTableFallbackRenderSummary(
        const TableFallbackRenderSummary& summary) {
        if (summary.tableHandle == 0)
            return;
        for (TableRecord& record : m_tables) {
            if (record.handle != summary.tableHandle)
                continue;
            record.m_fallbackGridEntityCount = summary.gridEntityCount;
            record.m_fallbackTextEntityCount = summary.textEntityCount;
            record.m_fallbackPlaceholderEntityCount =
                summary.placeholderEntityCount;
            record.m_fallbackUnresolvedTextStyleCount =
                summary.unresolvedTextStyleCount;
            record.m_fallbackClampedDimensionCount =
                summary.clampedDimensionCount;
            return;
        }
    }

    void addCellStyleMap(const DRW_CellStyleMap& map) {
        CellStyleMapRecord record;
        record.handle = map.handle;
        record.parentHandle = map.parentHandle;
        record.m_cellStyleCount = map.m_cellStyles.size();
        for (const DRW_TableStyleCellStyle& cellStyle : map.m_cellStyles)
            collectCellStyleMapSummary(record, cellStyle);
        collectCellStyleMapSubrecordRangeSummary(record, map.m_subrecordRanges);
        record.m_namedCellStyleCount = record.m_styleNames.size();
        m_cellStyleMaps.push_back(std::move(record));
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
        record.valueParamCount = object.m_valueParamCount;
        record.ownedParamPrefixCount = object.m_ownedParamPrefixCount;
        record.valueParamsParsed = object.m_valueParamsParsed;
        record.actionParamPrefixParsed = object.m_actionParamPrefixParsed;
        record.singleDependencyActionParamParsed =
            object.m_singleDependencyActionParamParsed;
        record.compoundActionParamParsed = object.m_compoundActionParamParsed;
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
        for (const DRW_AssociativePrefixStatus& prefix :
             object.m_prefixStatuses) {
            record.prefixStatuses.push_back(makeAssociativePrefixStatus(prefix));
        }
        m_associativeObjects.push_back(std::move(record));
        appendAssociativeEdges(m_associativeObjects.back());
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
        for (const DRW_AssociativePrefixStatus& prefix :
             object.m_prefixStatuses) {
            record.prefixStatuses.push_back(makeAssociativePrefixStatus(prefix));
        }
        m_acshObjects.push_back(record);
        appendAssociativeEdges(m_acshObjects.back());
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
        record.drawMLeaderOrder = style.drawMLeaderOrder;
        record.drawLeaderOrder = style.drawLeaderOrder;
        record.maxLeaderPoints = style.maxLeaderPoints;
        record.firstSegmentAngle = style.firstSegmentAngle;
        record.secondSegmentAngle = style.secondSegmentAngle;
        record.leaderType = style.leaderType;
        record.leaderColor = style.leaderColor;
        record.leaderLineTypeHandle = style.leaderLineTypeHandle.ref;
        record.leaderLineWeight = style.leaderLineWeight;
        record.landingEnabled = style.landingEnabled;
        record.landingGap = style.landingGap;
        record.autoIncludeLanding = style.autoIncludeLanding;
        record.landingDistance = style.landingDistance;
        record.description = style.description;
        record.arrowHeadBlockHandle = style.arrowHeadBlockHandle.ref;
        record.textStyleHandle = style.textStyleHandle.ref;
        record.blockHandle = style.blockHandle.ref;
        record.arrowHeadSize = style.arrowHeadSize;
        record.textDefault = style.textDefault;
        record.leftAttachment = style.leftAttachment;
        record.rightAttachment = style.rightAttachment;
        record.textAngleType = style.textAngleType;
        record.textAlignmentType = style.textAlignmentType;
        record.textColor = style.textColor;
        record.textHeight = style.textHeight;
        record.textFrameEnabled = style.textFrameEnabled;
        record.alwaysAlignTextLeft = style.alwaysAlignTextLeft;
        record.alignSpace = style.alignSpace;
        record.blockColor = style.blockColor;
        record.blockScale = style.blockScale;
        record.blockScaleEnabled = style.blockScaleEnabled;
        record.blockRotation = style.blockRotation;
        record.blockRotationEnabled = style.blockRotationEnabled;
        record.blockConnectionType = style.blockConnectionType;
        record.scaleFactor = style.scaleFactor;
        record.propertyChanged = style.propertyChanged;
        record.isAnnotative = style.isAnnotative;
        record.breakSize = style.breakSize;
        record.attachmentDirection = style.attachmentDirection;
        record.topAttachment = style.topAttachment;
        record.bottomAttachment = style.bottomAttachment;
        record.textExtended = style.textExtended;
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
    const std::vector<TableFallbackEntityRecord>& tableFallbackEntities() const {
        return m_tableFallbackEntities;
    }
    const std::vector<CellStyleMapRecord>& cellStyleMaps() const { return m_cellStyleMaps; }
    const std::vector<AssociativeRecord>& associativeObjects() const { return m_associativeObjects; }
    const std::vector<AssociativeEdgeRecord>& associativeEdges() const {
        return m_associativeEdges;
    }
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

    RawObjectFamilyCounts rawObjectFamilyCounts() const {
        RawObjectFamilyCounts counts;
        for (const RawObjectRecord& record : m_rawObjects)
            incrementRawObjectFamilyCount(counts, record.family);
        return counts;
    }

    std::vector<const RawObjectRecord*> findRawObjectsByFamily(RawObjectFamily family) const {
        std::vector<const RawObjectRecord*> result;
        for (const RawObjectRecord& record : m_rawObjects) {
            if (record.family == family)
                result.push_back(&record);
        }
        return result;
    }

    const TableRecord* findTableByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const TableRecord& record : m_tables) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const TableRecord* findTableStyleByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record) && record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const TableRecord*> findTableStylesReferencingHandle(
        duint32 handle) const {
        std::vector<const TableRecord*> result;
        if (handle == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record) && tableRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableRecord*> findTablesUsingStyle(duint32 styleHandle) const {
        std::vector<const TableRecord*> result;
        if (styleHandle == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (!isTableStyleRecord(record) && record.tableStyleHandle == styleHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableRecord*> findTablesReferencingHandle(duint32 handle) const {
        std::vector<const TableRecord*> result;
        if (handle == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (!isTableStyleRecord(record) && tableRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableFallbackEntityRecord*> findTableFallbackEntities(
        duint32 tableHandle) const {
        std::vector<const TableFallbackEntityRecord*> result;
        if (tableHandle == 0)
            return result;
        for (const TableFallbackEntityRecord& record : m_tableFallbackEntities) {
            if (record.tableHandle == tableHandle)
                result.push_back(&record);
        }
        return result;
    }
    const TableRecord* findTableByFallbackEntityId(
        unsigned long long entityId) const {
        if (entityId == 0)
            return nullptr;
        for (const TableFallbackEntityRecord& fallback : m_tableFallbackEntities) {
            if (fallback.entityId == entityId)
                return findTableByHandle(fallback.tableHandle);
        }
        return nullptr;
    }
    const CellStyleMapRecord* findCellStyleMapByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const CellStyleMapRecord& record : m_cellStyleMaps) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const CellStyleMapRecord*> findCellStylesById(int styleId) const {
        std::vector<const CellStyleMapRecord*> result;
        if (styleId == 0)
            return result;
        for (const CellStyleMapRecord& record : m_cellStyleMaps) {
            if (containsValue(record.m_styleIds, styleId))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const CellStyleMapRecord*> findCellStyleMapsReferencingHandle(
        duint32 handle) const {
        std::vector<const CellStyleMapRecord*> result;
        if (handle == 0)
            return result;
        for (const CellStyleMapRecord& record : m_cellStyleMaps) {
            if (cellStyleMapReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableRecord*> findTableStylesReferencingCellStyle(
        int styleId) const {
        std::vector<const TableRecord*> result;
        if (styleId == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record)
                && containsValue(record.m_tableStyleIds, styleId)) {
                result.push_back(&record);
            }
        }
        return result;
    }
    TableNativeWriterEligibility tableNativeWriterEligibility(
        duint32 handle, DRW::Version version) const {
        TableNativeWriterEligibility eligibility;
        eligibility.tableHandle = handle;
        eligibility.writerVersion = version;
        eligibility.storageMode = tableContentStorageModeForVersion(version);
        const TableRecord* record = findTableByHandle(handle);
        if (record == nullptr || isTableStyleRecord(*record)) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::NoSemanticTableContent);
            return eligibility;
        }
        populateTableNativeWriterEligibility(*record, version, eligibility);
        return eligibility;
    }
    std::vector<TableNativeWriterEligibility> tableNativeWriterEligibilityForAll(
        DRW::Version version) const {
        std::vector<TableNativeWriterEligibility> result;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record))
                continue;
            TableNativeWriterEligibility eligibility;
            eligibility.tableHandle = record.handle;
            populateTableNativeWriterEligibility(record, version, eligibility);
            result.push_back(std::move(eligibility));
        }
        return result;
    }
    TableNativeWriterBlockerCounts tableNativeWriterBlockerCounts(
        DRW::Version version) const {
        TableNativeWriterBlockerCounts counts;
        for (const TableNativeWriterEligibility& eligibility :
             tableNativeWriterEligibilityForAll(version)) {
            ++counts.tableCount;
            if (eligibility.eligibleTextOnly)
                ++counts.eligibleTextOnly;
            incrementTableStorageModeCount(counts, eligibility.storageMode);
            for (TableNativeWriterBlocker blocker : eligibility.blockers)
                incrementTableNativeWriterBlockerCount(counts, blocker);
        }
        return counts;
    }
    TableWriterBlockerCounts tableWriterBlockerCounts() const {
        TableWriterBlockerCounts counts;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record))
                continue;
            ++counts.tableCount;
            if (record.fallbackRendered)
                ++counts.fallbackRendered;
            if (!record.semanticParsed)
                ++counts.incompleteSemanticParse;
            if (record.tableStyleHandle != 0 && !record.styleResolved)
                ++counts.unresolvedStyle;
            if (record.fieldContentCount != 0)
                ++counts.fieldContent;
            if (record.blockContentCount != 0 || record.hasBlockContent)
                ++counts.blockContent;
            if (record.attributeCount != 0 || record.attributeHandleCount != 0)
                ++counts.attributeContent;
            if (record.overrideCellCount != 0)
                ++counts.overrideCells;
            if (record.geometryCellCount != 0)
                ++counts.geometryCells;
            if (record.m_unknownRangeCount != 0)
                ++counts.unknownRanges;
            if (record.m_incompleteRangeCount != 0)
                ++counts.incompleteRanges;
            if (record.m_overrideMaskCount != 0)
                ++counts.overrideMasks;
            if (record.m_breakRangeCount != 0)
                ++counts.breakRanges;
            if (record.m_tableGeometryTailRangeCount != 0)
                ++counts.tableGeometryTailRanges;
            if (record.fallbackInvalidated)
                ++counts.editedFallbackEntities;
            if (record.fallbackRendered
                && findTableFallbackEntities(record.handle).empty()) {
                ++counts.missingFallbackAttachments;
            }
        }
        return counts;
    }
    MLeaderWriterBlockerCounts mleaderWriterBlockerCounts() const {
        MLeaderWriterBlockerCounts counts;
        for (const MLeaderRecord& record : m_mleaders) {
            ++counts.mleaderCount;
            if (record.styleHandle != 0 && !record.styleResolved)
                ++counts.unresolvedStyle;
            if (record.effectiveContentType == 2 && !record.hasTextContent)
                ++counts.missingTextContent;
            if (record.effectiveContentType == 1 || record.hasBlockContent)
                ++counts.blockContent;
            if (record.effectiveContentType == 3)
                ++counts.toleranceContent;
            if (record.overrideFlags != 0)
                ++counts.overrideFlags;
            if (record.rootCount == 0 || record.leaderLineCount == 0)
                ++counts.missingLeaderGeometry;
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
        }
        return counts;
    }
    ModelerPayloadCounts modelerPayloadCounts() const {
        ModelerPayloadCounts counts;
        for (const ModelerGeometryRecord& record : m_modelerGeometry) {
            ++counts.recordCount;
            if (record.payloadKind == ModelerPayloadKind::Sat)
                ++counts.sat;
            else if (record.payloadKind == ModelerPayloadKind::Sab)
                ++counts.sab;
            else
                ++counts.unknown;
            if (!record.rawByteSplitConsistent)
                ++counts.inconsistentSplit;
            if (record.markerSection == ModelerPayloadSection::Body)
                ++counts.markerInBody;
            else if (record.markerSection == ModelerPayloadSection::HandleStream)
                ++counts.markerInHandleStream;
            for (const ModelerPayloadRangeRecord& range :
                 record.payloadRanges) {
                ++counts.rangeCount;
                incrementModelerPayloadRangeKindCount(counts, range.kind);
                incrementModelerPayloadRangeConsistencyCount(
                    counts, range.consistency);
            }
        }
        return counts;
    }
    AssociativeShellCounts associativeShellCounts() const {
        AssociativeShellCounts counts;
        for (const AssociativeRecord& record : m_associativeObjects) {
            ++counts.recordCount;
            incrementAssociativeKindCount(counts, record.kind);
            if (record.valueParamCount != 0) {
                ++counts.valueParamRecords;
                if (record.valueParamsParsed)
                    ++counts.parsedValueParamRecords;
            }
            if (record.kind == AssociativeKind::VertexActionParam
                || record.kind == AssociativeKind::OsnapPointRefActionParam) {
                ++counts.actionParamRecords;
                if (record.actionParamPrefixParsed)
                    ++counts.parsedActionParamPrefixes;
            }
            if (record.singleDependencyActionParamParsed)
                ++counts.singleDependencyActionParamPrefixes;
            if (record.compoundActionParamParsed)
                ++counts.compoundActionParamPrefixes;
        }
        return counts;
    }
    AssociativeEdgeCounts associativeEdgeCounts() const {
        AssociativeEdgeCounts counts;
        for (const AssociativeEdgeRecord& edge : m_associativeEdges) {
            ++counts.edgeCount;
            incrementAssociativeEdgeKindCount(counts, edge.edgeKind);
            incrementAssociativeEdgeConfidenceCount(counts, edge.confidence);
            if (edge.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
        }
        return counts;
    }
    AssociativePrefixCounts associativePrefixCounts() const {
        AssociativePrefixCounts counts;
        for (const AssociativeRecord& record : m_associativeObjects) {
            for (const AssociativePrefixStatusRecord& prefix :
                 record.prefixStatuses) {
                incrementAssociativePrefixCounts(counts, prefix);
            }
        }
        for (const AcShRecord& record : m_acshObjects) {
            for (const AssociativePrefixStatusRecord& prefix :
                 record.prefixStatuses) {
                incrementAssociativePrefixCounts(counts, prefix);
            }
        }
        return counts;
    }
    const TableCellRecord* findTableCell(duint32 tableHandle, int row, int column) const {
        const TableRecord* table = findTableByHandle(tableHandle);
        if (table == nullptr || row < 0 || column < 0)
            return nullptr;
        for (const TableCellRecord& cell : table->cells) {
            if (cell.row == row && cell.column == column)
                return &cell;
        }
        return nullptr;
    }
    std::vector<const TableCellRecord*> findTableCellsReferencingHandle(duint32 handle) const {
        std::vector<const TableCellRecord*> result;
        if (handle == 0)
            return result;
        for (const TableRecord& table : m_tables) {
            if (isTableStyleRecord(table))
                continue;
            for (const TableCellRecord& cell : table.cells) {
                if (tableCellReferences(cell, handle))
                    result.push_back(&cell);
            }
        }
        return result;
    }

    const MLeaderRecord* findMLeaderByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const MLeaderRecord& record : m_mleaders) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const MLeaderStyleRecord* findMLeaderStyleByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const MLeaderStyleRecord& record : m_mleaderStyles) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const MLeaderRecord*> findMLeadersUsingStyle(duint32 styleHandle) const {
        std::vector<const MLeaderRecord*> result;
        if (styleHandle == 0)
            return result;
        for (const MLeaderRecord& record : m_mleaders) {
            if (record.styleHandle == styleHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const MLeaderRecord*> findMLeadersReferencingHandle(duint32 handle) const {
        std::vector<const MLeaderRecord*> result;
        if (handle == 0)
            return result;
        for (const MLeaderRecord& record : m_mleaders) {
            if (mleaderRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const MLeaderStyleRecord*> findMLeaderStylesReferencingHandle(
        duint32 handle) const {
        std::vector<const MLeaderStyleRecord*> result;
        if (handle == 0)
            return result;
        for (const MLeaderStyleRecord& record : m_mleaderStyles) {
            if (mleaderStyleRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }

    const ModelerGeometryRecord* findModelerGeometryByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const ModelerGeometryRecord& record : m_modelerGeometry) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const ModelerGeometryRecord*> findModelerGeometryByHistoryHandle(
        duint32 historyHandle) const {
        std::vector<const ModelerGeometryRecord*> result;
        if (historyHandle == 0)
            return result;
        for (const ModelerGeometryRecord& record : m_modelerGeometry) {
            if (record.historyHandle == historyHandle)
                result.push_back(&record);
        }
        return result;
    }

    const AssociativeRecord* findAssociativeObjectByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const AssociativeRecord& record : m_associativeObjects) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const AssociativeRecord*> findAssociativeObjectsByKind(
        AssociativeKind kind) const {
        std::vector<const AssociativeRecord*> result;
        for (const AssociativeRecord& record : m_associativeObjects) {
            if (record.kind == kind)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const AssociativeRecord*> findAssociativeObjectsReferencingHandle(
        duint32 handle) const {
        std::vector<const AssociativeRecord*> result;
        if (handle == 0)
            return result;
        for (const AssociativeRecord& record : m_associativeObjects) {
            if (associativeRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const AssociativeEdgeRecord*> findAssociativeEdgesFrom(
        duint32 handle) const {
        std::vector<const AssociativeEdgeRecord*> result;
        if (handle == 0)
            return result;
        for (const AssociativeEdgeRecord& edge : m_associativeEdges) {
            if (edge.sourceHandle == handle)
                result.push_back(&edge);
        }
        return result;
    }
    std::vector<const AssociativeEdgeRecord*> findAssociativeEdgesTo(
        duint32 handle) const {
        std::vector<const AssociativeEdgeRecord*> result;
        if (handle == 0)
            return result;
        for (const AssociativeEdgeRecord& edge : m_associativeEdges) {
            if (edge.targetHandle == handle)
                result.push_back(&edge);
        }
        return result;
    }
    std::vector<const AssociativeRecord*> findAssociativeRecordsAffectedBy(
        duint32 handle) const {
        std::vector<const AssociativeRecord*> result;
        for (duint32 sourceHandle : findAssociativeClosureFrom(handle, 32u)) {
            if (const AssociativeRecord* record =
                    findAssociativeObjectByHandle(sourceHandle)) {
                result.push_back(record);
            }
        }
        return result;
    }
    std::vector<duint32> findAssociativeClosureFrom(
        duint32 handle, size_t maxDepth) const {
        std::vector<duint32> result;
        if (handle == 0)
            return result;
        std::vector<duint32> visited;
        std::vector<std::pair<duint32, size_t>> queue;
        visited.push_back(handle);
        queue.push_back({handle, 0u});
        for (size_t index = 0; index < queue.size(); ++index) {
            const duint32 targetHandle = queue[index].first;
            const size_t depth = queue[index].second;
            for (const AssociativeEdgeRecord& edge : m_associativeEdges) {
                if (edge.targetHandle != targetHandle
                    || containsHandle(visited, edge.sourceHandle)) {
                    continue;
                }
                visited.push_back(edge.sourceHandle);
                result.push_back(edge.sourceHandle);
                if (depth < maxDepth)
                    queue.push_back({edge.sourceHandle, depth + 1u});
            }
        }
        return result;
    }

    const AcShRecord* findAcShObjectByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const AcShRecord& record : m_acshObjects) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const AcShRecord*> findAcShObjectsByOwnerHandle(duint32 ownerHandle) const {
        std::vector<const AcShRecord*> result;
        if (ownerHandle == 0)
            return result;
        for (const AcShRecord& record : m_acshObjects) {
            if (record.ownerHandle == ownerHandle)
                result.push_back(&record);
        }
        return result;
    }

    static const char* modelerPayloadKindName(ModelerPayloadKind kind) {
        switch (kind) {
            case ModelerPayloadKind::Sat:
                return "SAT";
            case ModelerPayloadKind::Sab:
                return "SAB";
            case ModelerPayloadKind::Unknown:
            default:
                return "unknown";
        }
    }

    static const char* modelerPayloadSectionName(ModelerPayloadSection section) {
        switch (section) {
            case ModelerPayloadSection::Body:
                return "body";
            case ModelerPayloadSection::HandleStream:
                return "handle-stream";
            case ModelerPayloadSection::Unknown:
            default:
                return "unknown";
        }
    }

    static const char* modelerPayloadRangeKindName(ModelerPayloadRangeKind kind) {
        switch (kind) {
            case ModelerPayloadRangeKind::Sat:
                return "SAT";
            case ModelerPayloadRangeKind::Sab:
                return "SAB";
            case ModelerPayloadRangeKind::History:
                return "history";
            case ModelerPayloadRangeKind::Wire:
                return "wire";
            case ModelerPayloadRangeKind::Silhouette:
                return "silhouette";
            case ModelerPayloadRangeKind::HandleStream:
                return "handle-stream";
            case ModelerPayloadRangeKind::UnknownTail:
            default:
                return "unknown-tail";
        }
    }

    static const char* modelerPayloadRangeConsistencyName(
        ModelerPayloadRangeConsistency consistency) {
        switch (consistency) {
            case ModelerPayloadRangeConsistency::Exact:
                return "exact";
            case ModelerPayloadRangeConsistency::Truncated:
                return "truncated";
            case ModelerPayloadRangeConsistency::Overrun:
                return "overrun";
            case ModelerPayloadRangeConsistency::Unknown:
            default:
                return "unknown";
        }
    }

    static const char* modelerPayloadRangeConfidenceName(
        ModelerPayloadRangeConfidence confidence) {
        switch (confidence) {
            case ModelerPayloadRangeConfidence::Marker:
                return "marker";
            case ModelerPayloadRangeConfidence::DeclaredSize:
                return "declared-size";
            case ModelerPayloadRangeConfidence::Inferred:
                return "inferred";
            case ModelerPayloadRangeConfidence::Unknown:
            default:
                return "unknown";
        }
    }

    struct ModelerPayloadMarker {
        ModelerPayloadKind kind = ModelerPayloadKind::Unknown;
        size_t offset = 0;
        size_t length = 0;
        std::string text;
    };

    struct ModelerRawByteSplit {
        bool known = false;
        bool consistent = true;
        size_t bodyByteCount = 0;
        size_t handleByteCount = 0;
    };

    static ModelerRawByteSplit splitModelerRawBytes(
        duint32 bodyBitSize, size_t rawByteCount) {
        ModelerRawByteSplit split;
        if (bodyBitSize == 0) {
            split.bodyByteCount = rawByteCount;
            return split;
        }
        split.known = true;
        const size_t bodyByteCount =
            (static_cast<size_t>(bodyBitSize) + 7u) / 8u;
        if (bodyByteCount > rawByteCount) {
            split.consistent = false;
            split.bodyByteCount = rawByteCount;
            return split;
        }
        split.bodyByteCount = bodyByteCount;
        split.handleByteCount = rawByteCount - bodyByteCount;
        return split;
    }

    static ModelerPayloadMarker scanModelerPayloadMarker(
        const std::vector<duint8>& bytes) {
        const ModelerPayloadMarker sab = findModelerPayloadMarker(
            bytes, "ACIS BinaryFile", ModelerPayloadKind::Sab);
        if (sab.kind != ModelerPayloadKind::Unknown)
            return sab;
        const ModelerPayloadMarker satHistory = findModelerPayloadMarker(
            bytes, "Begin-of-ACIS-History", ModelerPayloadKind::Sat);
        if (satHistory.kind != ModelerPayloadKind::Unknown)
            return satHistory;
        return findModelerPayloadMarker(bytes, "ACIS", ModelerPayloadKind::Sat);
    }

    static ModelerPayloadKind classifyModelerPayload(const std::vector<duint8>& bytes) {
        return scanModelerPayloadMarker(bytes).kind;
    }

    static std::vector<ModelerPayloadRangeRecord> scanModelerPayloadRanges(
        const std::vector<duint8>& bytes, duint32 bodyBitSize,
        bool hasWireframe = false) {
        std::vector<ModelerPayloadRangeRecord> ranges;
        if (bytes.empty())
            return ranges;

        const ModelerRawByteSplit split =
            splitModelerRawBytes(bodyBitSize, bytes.size());
        const size_t bodyByteCount =
            split.known ? split.bodyByteCount : bytes.size();
        const size_t declaredBodyByteCount =
            bodyBitSize == 0u ? bodyByteCount :
                                (static_cast<size_t>(bodyBitSize) + 7u) / 8u;
        const ModelerPayloadRangeConsistency bodyConsistency =
            split.known && !split.consistent ?
                ModelerPayloadRangeConsistency::Truncated :
                ModelerPayloadRangeConsistency::Exact;
        const ModelerPayloadMarker marker =
            scanModelerPayloadMarkerInRange(bytes, 0u, bodyByteCount);

        if (marker.kind == ModelerPayloadKind::Sab) {
            appendUnknownModelerRange(ranges, 0u, marker.offset,
                                      declaredBodyByteCount, bodyConsistency);
            const std::vector<duint8> terminator =
                sabTerminatorBytes();
            const size_t terminatorOffset = findByteSequence(
                bytes, terminator, marker.offset + marker.length,
                bodyByteCount);
            size_t rangeEnd = bodyByteCount;
            ModelerPayloadRangeConsistency consistency =
                bodyConsistency;
            ModelerPayloadRangeConfidence confidence =
                ModelerPayloadRangeConfidence::Marker;
            if (terminatorOffset != npos()) {
                rangeEnd = terminatorOffset + terminator.size();
                consistency = ModelerPayloadRangeConsistency::Exact;
            } else if (bodyConsistency == ModelerPayloadRangeConsistency::Exact) {
                confidence = ModelerPayloadRangeConfidence::Inferred;
            }
            appendModelerRange(ranges, ModelerPayloadRangeKind::Sab,
                               ModelerPayloadSection::Body, marker.offset,
                               rangeEnd - marker.offset,
                               declaredBodyByteCount, consistency, confidence,
                               marker.text);
            if (rangeEnd < bodyByteCount) {
                appendModelerRange(
                    ranges, ModelerPayloadRangeKind::UnknownTail,
                    ModelerPayloadSection::Body, rangeEnd,
                    bodyByteCount - rangeEnd, declaredBodyByteCount,
                    ModelerPayloadRangeConsistency::Exact,
                    ModelerPayloadRangeConfidence::Inferred, std::string());
            }
        } else if (marker.kind == ModelerPayloadKind::Sat) {
            appendUnknownModelerRange(ranges, 0u, marker.offset,
                                      declaredBodyByteCount, bodyConsistency);
            const ModelerPayloadRangeKind kind =
                marker.text == "Begin-of-ACIS-History" ?
                    ModelerPayloadRangeKind::History :
                    ModelerPayloadRangeKind::Sat;
            appendModelerRange(ranges, kind, ModelerPayloadSection::Body,
                               marker.offset, bodyByteCount - marker.offset,
                               declaredBodyByteCount, bodyConsistency,
                               ModelerPayloadRangeConfidence::Marker,
                               marker.text);
        } else if (bodyByteCount != 0u) {
            appendModelerRange(
                ranges,
                hasWireframe ? ModelerPayloadRangeKind::Wire :
                               ModelerPayloadRangeKind::UnknownTail,
                ModelerPayloadSection::Body, 0u, bodyByteCount,
                declaredBodyByteCount, bodyConsistency,
                hasWireframe ? ModelerPayloadRangeConfidence::Inferred :
                               ModelerPayloadRangeConfidence::Unknown,
                std::string());
        }

        if (split.known && split.handleByteCount != 0u) {
            appendModelerRange(ranges, ModelerPayloadRangeKind::HandleStream,
                               ModelerPayloadSection::HandleStream,
                               split.bodyByteCount, split.handleByteCount,
                               split.handleByteCount,
                               ModelerPayloadRangeConsistency::Exact,
                               ModelerPayloadRangeConfidence::DeclaredSize,
                               std::string());
        }
        return ranges;
    }

private:
    static ModelerPayloadSection modelerPayloadSectionForMarker(
        const ModelerPayloadMarker& marker, const ModelerRawByteSplit& split,
        size_t rawByteCount) {
        if (marker.kind == ModelerPayloadKind::Unknown || marker.length == 0)
            return ModelerPayloadSection::Unknown;
        const size_t markerEnd = marker.offset + marker.length;
        if (markerEnd > rawByteCount || markerEnd < marker.offset)
            return ModelerPayloadSection::Unknown;
        if (!split.known)
            return ModelerPayloadSection::Body;
        if (markerEnd <= split.bodyByteCount)
            return ModelerPayloadSection::Body;
        if (marker.offset >= split.bodyByteCount)
            return ModelerPayloadSection::HandleStream;
        return ModelerPayloadSection::Unknown;
    }

    static ModelerPayloadMarker findModelerPayloadMarker(
        const std::vector<duint8>& bytes, const char* marker,
        ModelerPayloadKind kind) {
        ModelerPayloadMarker result;
        if (marker == nullptr || marker[0] == '\0')
            return result;
        size_t markerSize = 0;
        while (marker[markerSize] != '\0')
            ++markerSize;
        if (bytes.size() < markerSize)
            return result;
        for (size_t offset = 0; offset + markerSize <= bytes.size(); ++offset) {
            bool matched = true;
            for (size_t index = 0; index < markerSize; ++index) {
                if (bytes[offset + index] != static_cast<duint8>(marker[index])) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                result.kind = kind;
                result.offset = offset;
                result.length = markerSize;
                result.text.assign(marker, markerSize);
                return result;
            }
        }
        return result;
    }

    static constexpr size_t npos() {
        return static_cast<size_t>(-1);
    }

    static ModelerPayloadMarker scanModelerPayloadMarkerInRange(
        const std::vector<duint8>& bytes, size_t start, size_t end) {
        const ModelerPayloadMarker sab = findModelerPayloadMarkerInRange(
            bytes, "ACIS BinaryFile", ModelerPayloadKind::Sab, start, end);
        if (sab.kind != ModelerPayloadKind::Unknown)
            return sab;
        const ModelerPayloadMarker satHistory =
            findModelerPayloadMarkerInRange(
                bytes, "Begin-of-ACIS-History", ModelerPayloadKind::Sat,
                start, end);
        if (satHistory.kind != ModelerPayloadKind::Unknown)
            return satHistory;
        return findModelerPayloadMarkerInRange(
            bytes, "ACIS", ModelerPayloadKind::Sat, start, end);
    }

    static ModelerPayloadMarker findModelerPayloadMarkerInRange(
        const std::vector<duint8>& bytes, const char* marker,
        ModelerPayloadKind kind, size_t start, size_t end) {
        ModelerPayloadMarker result;
        if (marker == nullptr || marker[0] == '\0' || start >= bytes.size())
            return result;
        end = std::min(end, bytes.size());
        if (start >= end)
            return result;
        size_t markerSize = 0;
        while (marker[markerSize] != '\0')
            ++markerSize;
        if (markerSize == 0 || end - start < markerSize)
            return result;
        for (size_t offset = start; offset + markerSize <= end; ++offset) {
            bool matched = true;
            for (size_t index = 0; index < markerSize; ++index) {
                if (bytes[offset + index] !=
                    static_cast<duint8>(marker[index])) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                result.kind = kind;
                result.offset = offset;
                result.length = markerSize;
                result.text.assign(marker, markerSize);
                return result;
            }
        }
        return result;
    }

    static std::vector<duint8> sabTerminatorBytes() {
        return {'E', 'n', 'd', 0x0Eu, 0x02u, 'o', 'f', 0x0Eu, 0x04u,
                'A', 'C', 'I', 'S', 0x0Du, 0x04u, 'd', 'a', 't', 'a'};
    }

    static size_t findByteSequence(const std::vector<duint8>& bytes,
                                   const std::vector<duint8>& marker,
                                   size_t start, size_t end) {
        if (marker.empty() || start >= bytes.size())
            return npos();
        end = std::min(end, bytes.size());
        if (start >= end || end - start < marker.size())
            return npos();
        for (size_t offset = start; offset + marker.size() <= end; ++offset) {
            bool matched = true;
            for (size_t index = 0; index < marker.size(); ++index) {
                if (bytes[offset + index] != marker[index]) {
                    matched = false;
                    break;
                }
            }
            if (matched)
                return offset;
        }
        return npos();
    }

    static void appendModelerRange(
        std::vector<ModelerPayloadRangeRecord>& ranges,
        ModelerPayloadRangeKind kind, ModelerPayloadSection section,
        size_t offset, size_t length, size_t declaredByteSize,
        ModelerPayloadRangeConsistency consistency,
        ModelerPayloadRangeConfidence confidence,
        const std::string& markerText) {
        if (length == 0u)
            return;
        ModelerPayloadRangeRecord range;
        range.kind = kind;
        range.section = section;
        range.offset = offset;
        range.length = length;
        range.declaredByteSize = declaredByteSize;
        range.consistency = consistency;
        range.confidence = confidence;
        range.markerText = markerText;
        ranges.push_back(std::move(range));
    }

    static void appendUnknownModelerRange(
        std::vector<ModelerPayloadRangeRecord>& ranges, size_t offset,
        size_t length, size_t declaredByteSize,
        ModelerPayloadRangeConsistency consistency) {
        appendModelerRange(ranges, ModelerPayloadRangeKind::UnknownTail,
                           ModelerPayloadSection::Body, offset, length,
                           declaredByteSize, consistency,
                           ModelerPayloadRangeConfidence::Inferred,
                           std::string());
    }

public:
    const ViewRecord* findViewByName(const std::string& name) const {
        for (const ViewRecord& record : m_views) {
            if (record.name == name)
                return &record;
        }
        return nullptr;
    }
    const ViewRecord* findViewByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const ViewRecord& record : m_views) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const ViewRecord*> findViewsReferencingHandle(duint32 handle) const {
        std::vector<const ViewRecord*> result;
        if (handle == 0)
            return result;
        for (const ViewRecord& record : m_views) {
            if (viewRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    const LightRecord* findLightByHandle(duint32 handle) const {
        if (handle == 0)
            return nullptr;
        for (const LightRecord& record : m_lights) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const LightRecord*> findLightsByParentHandle(duint32 parentHandle) const {
        std::vector<const LightRecord*> result;
        if (parentHandle == 0)
            return result;
        for (const LightRecord& record : m_lights) {
            if (record.parentHandle == parentHandle)
                result.push_back(&record);
        }
        return result;
    }
    const SunRecord* findSunByHandle(duint32 handle) const {
        for (const SunRecord& record : m_suns) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const SunRecord* findSunForViewName(const std::string& name) const {
        const ViewRecord* view = findViewByName(name);
        return view == nullptr ? nullptr : findSunByHandle(view->sunHandle);
    }
    const SunRecord* findSunForViewHandle(duint32 handle) const {
        const ViewRecord* view = findViewByHandle(handle);
        return view == nullptr ? nullptr : findSunByHandle(view->sunHandle);
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

    static RawObjectFamily rawObjectFamilyFromNames(
        const std::string& recordName, const std::string& className) {
        if (associativeKindFromRecordName(recordName) != AssociativeKind::Unknown
            || containsSubstring(className, "Assoc")
            || containsSubstring(className, "PersSubent")) {
            return RawObjectFamily::Associative;
        }
        if (recordName == "ACAD_EVALUATION_GRAPH"
            || containsSubstring(className, "EvalGraph")
            || containsSubstring(className, "EvalExpr")) {
            return RawObjectFamily::EvaluationGraph;
        }
        if (containsSubstring(recordName, "OBJECTCONTEXTDATA")
            || containsSubstring(className, "ObjectContextData")) {
            return RawObjectFamily::ObjectContext;
        }
        const bool isBlockRecord = containsSubstring(recordName, "BLOCK")
                                   || containsSubstring(className, "Block");
        const bool isDynamicBlockPart = containsSubstring(recordName, "PARAMETER")
                                        || containsSubstring(recordName, "ACTION")
                                        || containsSubstring(recordName, "GRIP")
                                        || containsSubstring(className, "Parameter")
                                        || containsSubstring(className, "Action")
                                        || containsSubstring(className, "Grip");
        if (isBlockRecord && isDynamicBlockPart)
            return RawObjectFamily::DynamicBlock;
        return RawObjectFamily::Unknown;
    }

    static const char* rawObjectFamilyName(RawObjectFamily family) {
        switch (family) {
            case RawObjectFamily::Associative:
                return "associative";
            case RawObjectFamily::EvaluationGraph:
                return "evaluation graph";
            case RawObjectFamily::DynamicBlock:
                return "dynamic block";
            case RawObjectFamily::ObjectContext:
                return "object context";
            case RawObjectFamily::Unknown:
            default:
                return "unknown";
        }
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

    static const char* associativeKindName(AssociativeKind kind) {
        switch (kind) {
            case AssociativeKind::Network:
                return "ACDBASSOCNETWORK";
            case AssociativeKind::Action:
                return "ACDBASSOCACTION";
            case AssociativeKind::Dependency:
                return "ACDBASSOCDEPENDENCY";
            case AssociativeKind::GeometryDependency:
                return "ACDBASSOCGEOMDEPENDENCY";
            case AssociativeKind::PersistentSubentityManager:
                return "ACDBASSOCPERSSUBENTMANAGER";
            case AssociativeKind::AlignedDimensionActionBody:
                return "ACDBASSOCALIGNEDDIMACTIONBODY";
            case AssociativeKind::VertexActionParam:
                return "ACDBASSOCVERTEXACTIONPARAM";
            case AssociativeKind::OsnapPointRefActionParam:
                return "ACDBASSOCOSNAPPOINTREFACTIONPARAM";
            case AssociativeKind::Unknown:
            default:
                return "unknown";
        }
    }

    static const char* associativePrefixKindName(AssociativePrefixKind kind) {
        switch (kind) {
            case AssociativePrefixKind::AcDbAssocAction:
                return "AcDbAssocAction";
            case AssociativePrefixKind::AcDbAssocActionParam:
                return "AcDbAssocActionParam";
            case AssociativePrefixKind::AcDbAssocDependency:
                return "AcDbAssocDependency";
            case AssociativePrefixKind::AcDbAssocGeomDependency:
                return "AcDbAssocGeomDependency";
            case AssociativePrefixKind::AcDbAssocNetwork:
                return "AcDbAssocNetwork";
            case AssociativePrefixKind::AcDbAssocActionBody:
                return "AcDbAssocActionBody";
            case AssociativePrefixKind::AcDbEvalExpr:
                return "AcDbEvalExpr";
            case AssociativePrefixKind::AcDbShHistoryNode:
                return "AcDbShHistoryNode";
            case AssociativePrefixKind::AcShActionBody:
                return "AcShActionBody";
        }
        return "unknown";
    }

    static const char* associativePrefixParseStatusName(
        AssociativePrefixParseStatus status) {
        switch (status) {
            case AssociativePrefixParseStatus::Complete:
                return "complete";
            case AssociativePrefixParseStatus::Partial:
                return "partial";
            case AssociativePrefixParseStatus::Missing:
                return "missing";
            case AssociativePrefixParseStatus::UnsupportedVersion:
                return "unsupported version";
            case AssociativePrefixParseStatus::BoundedCountOverflow:
                return "bounded count overflow";
        }
        return "unknown";
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

    static size_t rawObjectFamilyCount(
        const RawObjectFamilyCounts& counts, RawObjectFamily family) {
        switch (family) {
            case RawObjectFamily::Associative:
                return counts.associative;
            case RawObjectFamily::EvaluationGraph:
                return counts.evaluationGraph;
            case RawObjectFamily::DynamicBlock:
                return counts.dynamicBlock;
            case RawObjectFamily::ObjectContext:
                return counts.objectContext;
            case RawObjectFamily::Unknown:
            default:
                return counts.unknown;
        }
    }

    static size_t associativeShellKindCount(
        const AssociativeShellCounts& counts, AssociativeKind kind) {
        switch (kind) {
            case AssociativeKind::Network:
                return counts.network;
            case AssociativeKind::Action:
                return counts.action;
            case AssociativeKind::Dependency:
                return counts.dependency;
            case AssociativeKind::GeometryDependency:
                return counts.geometryDependency;
            case AssociativeKind::PersistentSubentityManager:
                return counts.persistentSubentityManager;
            case AssociativeKind::AlignedDimensionActionBody:
                return counts.alignedDimensionActionBody;
            case AssociativeKind::VertexActionParam:
                return counts.vertexActionParam;
            case AssociativeKind::OsnapPointRefActionParam:
                return counts.osnapPointRefActionParam;
            case AssociativeKind::Unknown:
            default:
                return counts.unknown;
        }
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
            + m_tables.size() + m_cellStyleMaps.size()
            + m_associativeObjects.size() + m_acshObjects.size()
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
            || hasReplayable(m_cellStyleMaps)
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
        if (dependentHandle == 0)
            return;
        const std::vector<duint32> affectedHandles =
            findAssociativeClosureFrom(dependentHandle, 32u);
        for (duint32 affectedHandle : affectedHandles) {
            invalidateAssociativeSemanticRecord(affectedHandle);
            invalidateRawAssociativeObject(affectedHandle);
            for (AssociativeEdgeRecord& edge : m_associativeEdges) {
                if (edge.sourceHandle == affectedHandle
                    && edge.replayState == ReplayState::ReplayAllowed) {
                    edge.replayState = ReplayState::ReplayInvalidated;
                }
            }
        }
    }

    void invalidateTableGraphForHandle(duint32 dependentHandle) {
        if (dependentHandle == 0)
            return;
        for (TableRecord& record : m_tables) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (tableRecordReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateRawTableObject(record.handle);
            }
        }
        for (CellStyleMapRecord& record : m_cellStyleMaps) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (cellStyleMapReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateRawTableObject(record.handle);
            }
        }
    }

    bool invalidateTableForFallbackEntity(unsigned long long entityId) {
        if (entityId == 0)
            return false;
        bool invalidated = false;
        for (TableFallbackEntityRecord& fallback : m_tableFallbackEntities) {
            if (fallback.entityId != entityId)
                continue;
            fallback.replayState = ReplayState::ReplayInvalidated;
            for (TableRecord& table : m_tables) {
                if (table.handle != fallback.tableHandle)
                    continue;
                table.replayState = ReplayState::ReplayInvalidated;
                table.fallbackInvalidated = true;
                invalidateRawTableObject(table.handle);
                invalidated = true;
            }
        }
        return invalidated;
    }

    void invalidateMLeaderGraphForHandle(duint32 dependentHandle) {
        if (dependentHandle == 0)
            return;
        for (MLeaderRecord& record : m_mleaders) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (mleaderRecordReferences(record, dependentHandle))
                record.replayState = ReplayState::ReplayInvalidated;
        }
        for (MLeaderStyleRecord& record : m_mleaderStyles) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (mleaderStyleRecordReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateRawMLeaderStyle(record.handle);
            }
        }
    }

    void invalidateViewGraphForHandle(duint32 dependentHandle) {
        if (dependentHandle == 0)
            return;
        for (ViewRecord& record : m_views) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (viewRecordReferences(record, dependentHandle))
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    void markMLeaderReplayReplacedForHandle(duint32 handle) {
        if (handle == 0)
            return;
        for (MLeaderRecord& record : m_mleaders) {
            if (record.handle == handle)
                record.replayState = ReplayState::ReplayReplaced;
        }
    }

    void invalidateAllReplayable() {
        invalidateContainer(m_rawObjects);
        invalidateContainer(m_views);
        invalidateContainer(m_lights);
        invalidateContainer(m_suns);
        invalidateContainer(m_modelerGeometry);
        invalidateContainer(m_tables);
        invalidateContainer(m_cellStyleMaps);
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
        collectTableSubrecordRangeSummary(record, content.m_subrecordRanges);
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
            const int rowIndex = static_cast<int>(record.rowHeights.size() - 1u);
            record.cellCount += row.m_cells.size();
            int columnIndex = 0;
            for (const DRW_TableCell& cell : row.m_cells) {
                TableCellRecord cellRecord;
                cellRecord.row = rowIndex;
                cellRecord.column = columnIndex;
                cellRecord.flags = cell.m_flags;
                cellRecord.type = cell.m_type;
                cellRecord.styleId = cell.m_styleId;
                cellRecord.overrideFlags = cell.m_overrideFlags;
                cellRecord.valueHandle = cell.m_valueHandle;
                cellRecord.textStyleHandle = cell.m_textStyleHandle;
                cellRecord.textStyleOverrideHandle = cell.m_textStyleOverrideHandle;
                cellRecord.blockHandle = cell.m_blockHandle;
                cellRecord.geometryHandle = cell.m_geometryHandle;
                cellRecord.isMerged = cell.m_isMerged;
                cellRecord.autoFit = cell.m_autoFit;
                cellRecord.contentCount = cell.m_contents.size();
                cellRecord.attributeCount = cell.m_attributes.size();
                record.contentCount += cell.m_contents.size();
                record.attributeCount += cell.m_attributes.size();
                if (cell.m_styleId != 0)
                    record.cellStyleIds.push_back(cell.m_styleId);
                if (cell.m_textStyleHandle != 0)
                    record.textStyleHandles.push_back(cell.m_textStyleHandle);
                if (cell.m_textStyleOverrideHandle != 0)
                    record.textStyleHandles.push_back(cell.m_textStyleOverrideHandle);
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
                    if (attribute.m_attdefHandle != 0) {
                        record.attributeHandles.push_back(attribute.m_attdefHandle);
                        cellRecord.attributeHandles.push_back(attribute.m_attdefHandle);
                    }
                    if (!attribute.m_text.empty()) {
                        record.attributeTexts.push_back(attribute.m_text);
                        cellRecord.attributeTexts.push_back(attribute.m_text);
                    }
                }
                cellRecord.attributeHandleCount = cellRecord.attributeHandles.size();
                for (const DRW_TableCellContent& cellContent : cell.m_contents) {
                    if (!cellContent.m_text.empty()) {
                        ++record.textContentCount;
                        ++cellRecord.textContentCount;
                        record.hasTextContent = true;
                        record.cellTexts.push_back(cellContent.m_text);
                        cellRecord.texts.push_back(cellContent.m_text);
                    }
                    if (cellContent.m_type == 2) {
                        ++record.fieldContentCount;
                        ++cellRecord.fieldContentCount;
                        if (cellContent.m_handle != 0) {
                            record.fieldHandles.push_back(cellContent.m_handle);
                            cellRecord.contentHandles.push_back(cellContent.m_handle);
                        }
                    }
                    if (cellContent.m_type == 4) {
                        ++record.blockContentCount;
                        ++cellRecord.blockContentCount;
                        record.hasBlockContent = true;
                        if (cellContent.m_handle != 0) {
                            record.blockHandles.push_back(cellContent.m_handle);
                            cellRecord.contentHandles.push_back(cellContent.m_handle);
                        }
                    }
                    if (cellContent.m_type != 0 && cellContent.m_type != 1
                        && cellContent.m_type != 2 && cellContent.m_type != 4) {
                        ++record.unknownContentCount;
                        ++cellRecord.unknownContentCount;
                    }
                }
                record.cells.push_back(std::move(cellRecord));
                ++columnIndex;
            }
        }
        record.fieldHandleCount = record.fieldHandles.size();
        record.blockHandleCount = record.blockHandles.size();
        record.attributeHandleCount = record.attributeHandles.size();
        record.textStyleHandleCount = record.textStyleHandles.size();
    }

    static bool isTableStyleRecord(const TableRecord& record) {
        return record.tableStyleHandle == 0
               && record.recordName != "ACAD_TABLE"
               && record.recordName != "TABLECONTENT"
               && record.semanticParsed
               && record.styleResolved;
    }

    static void collectTableStyleBorderHandle(TableRecord& record,
                                              const DRW_TableStyleBorder& border) {
        if (border.m_lineTypeHandle != 0)
            record.lineTypeHandles.push_back(border.m_lineTypeHandle);
    }

    static void collectTableStyleBorderSummary(TableRecord& record,
                                               const DRW_TableStyleBorder& border) {
        if (border.m_visible != 0)
            ++record.m_tableVisibleBorderCount;
        if (border.m_color != 0)
            record.m_tableColors.push_back(border.m_color);
    }

    static void collectTableStyleContentSummary(
        TableRecord& record, const DRW_TableStyleContentFormat& format) {
        ++record.m_tableContentFormatCount;
        if (format.m_textHeight > 0.0)
            record.m_tableTextHeights.push_back(format.m_textHeight);
        if (format.m_cellAlignment != 0)
            record.m_tableAlignments.push_back(format.m_cellAlignment);
        if (format.m_contentColor != 0)
            record.m_tableColors.push_back(format.m_contentColor);
    }

    static void collectTableStyleCellHandles(TableRecord& record,
                                             const DRW_TableStyleCellStyle& cellStyle) {
        if (cellStyle.m_contentFormat.m_textStyleHandle != 0)
            record.textStyleHandles.push_back(
                cellStyle.m_contentFormat.m_textStyleHandle);
        for (const DRW_TableStyleBorder& border : cellStyle.m_borders)
            collectTableStyleBorderHandle(record, border);
    }

    static void collectTableStyleCellSummary(TableRecord& record,
                                             const DRW_TableStyleCellStyle& cellStyle) {
        if (cellStyle.m_id != 0)
            record.m_tableStyleIds.push_back(cellStyle.m_id);
        if (!cellStyle.m_name.empty())
            record.m_tableStyleNames.push_back(cellStyle.m_name);
        if (cellStyle.m_backgroundColor != 0)
            record.m_tableColors.push_back(cellStyle.m_backgroundColor);
        if (cellStyle.m_verticalMargin != 0.0
            || cellStyle.m_horizontalMargin != 0.0
            || cellStyle.m_bottomMargin != 0.0
            || cellStyle.m_rightMargin != 0.0
            || cellStyle.m_marginHorizontalSpacing != 0.0
            || cellStyle.m_marginVerticalSpacing != 0.0) {
            ++record.m_tableMarginStyleCount;
        }
        collectTableStyleContentSummary(record, cellStyle.m_contentFormat);
        for (const DRW_TableStyleBorder& border : cellStyle.m_borders)
            collectTableStyleBorderSummary(record, border);
    }

    static void collectTableStyleRowHandles(TableRecord& record,
                                            const DRW_TableStyleRowStyle& rowStyle) {
        if (rowStyle.m_textStyleHandle != 0)
            record.textStyleHandles.push_back(rowStyle.m_textStyleHandle);
        for (const DRW_TableStyleBorder& border : rowStyle.m_borders)
            collectTableStyleBorderHandle(record, border);
    }

    static void collectTableStyleRowSummary(TableRecord& record,
                                            const DRW_TableStyleRowStyle& rowStyle) {
        if (rowStyle.m_textHeight > 0.0)
            record.m_tableTextHeights.push_back(rowStyle.m_textHeight);
        if (rowStyle.m_textAlignment != 0)
            record.m_tableAlignments.push_back(rowStyle.m_textAlignment);
        if (rowStyle.m_textColor != 0)
            record.m_tableColors.push_back(rowStyle.m_textColor);
        if (rowStyle.m_fillColor != 0)
            record.m_tableColors.push_back(rowStyle.m_fillColor);
        for (const DRW_TableStyleBorder& border : rowStyle.m_borders)
            collectTableStyleBorderSummary(record, border);
    }

    static void collectCellStyleMapSummary(CellStyleMapRecord& record,
                                           const DRW_TableStyleCellStyle& cellStyle) {
        if (cellStyle.m_id != 0)
            record.m_styleIds.push_back(cellStyle.m_id);
        if (cellStyle.m_styleClass != 0)
            record.m_styleClasses.push_back(cellStyle.m_styleClass);
        if (!cellStyle.m_name.empty())
            record.m_styleNames.push_back(cellStyle.m_name);
        if (cellStyle.m_contentFormat.m_textStyleHandle != 0)
            record.m_textStyleHandles.push_back(
                cellStyle.m_contentFormat.m_textStyleHandle);
        if (cellStyle.m_contentFormat.m_textHeight > 0.0)
            record.m_textHeights.push_back(cellStyle.m_contentFormat.m_textHeight);
        if (cellStyle.m_contentFormat.m_cellAlignment != 0)
            record.m_alignments.push_back(cellStyle.m_contentFormat.m_cellAlignment);
        if (cellStyle.m_contentFormat.m_contentColor != 0)
            record.m_colors.push_back(cellStyle.m_contentFormat.m_contentColor);
        if (cellStyle.m_backgroundColor != 0)
            record.m_colors.push_back(cellStyle.m_backgroundColor);
        if (cellStyle.m_verticalMargin != 0.0
            || cellStyle.m_horizontalMargin != 0.0
            || cellStyle.m_bottomMargin != 0.0
            || cellStyle.m_rightMargin != 0.0
            || cellStyle.m_marginHorizontalSpacing != 0.0
            || cellStyle.m_marginVerticalSpacing != 0.0) {
            ++record.m_marginStyleCount;
        }
        ++record.m_contentFormatCount;
        record.m_borderCount += cellStyle.m_borders.size();
        for (const DRW_TableStyleBorder& border : cellStyle.m_borders) {
            if (border.m_lineTypeHandle != 0)
                record.m_lineTypeHandles.push_back(border.m_lineTypeHandle);
            if (border.m_visible != 0)
                ++record.m_visibleBorderCount;
            if (border.m_color != 0)
                record.m_colors.push_back(border.m_color);
        }
    }

    static void collectTableSubrecordRangeSummary(
        TableRecord& record, const std::vector<DRW_DwgSubrecordRange>& ranges) {
        for (const DRW_DwgSubrecordRange& range : ranges) {
            ++record.m_unknownRangeCount;
            if (!range.m_parseComplete)
                ++record.m_incompleteRangeCount;
            if (rangeNameContains(range, "override"))
                ++record.m_overrideMaskCount;
            if (rangeNameContains(range, "break"))
                ++record.m_breakRangeCount;
            if (rangeNameContains(range, "geometry"))
                ++record.m_tableGeometryTailRangeCount;
        }
    }

    static void collectCellStyleMapSubrecordRangeSummary(
        CellStyleMapRecord& record,
        const std::vector<DRW_DwgSubrecordRange>& ranges) {
        record.m_unknownRangeCount += ranges.size();
        for (const DRW_DwgSubrecordRange& range : ranges) {
            if (!range.m_parseComplete)
                ++record.m_incompleteRangeCount;
        }
    }

    void populateTableNativeWriterEligibility(
        const TableRecord& record, DRW::Version version,
        TableNativeWriterEligibility& eligibility) const {
        eligibility.tableHandle = record.handle;
        eligibility.recordName = record.recordName;
        eligibility.writerVersion = version;
        eligibility.storageMode = tableContentStorageModeForVersion(version);

        if (eligibility.storageMode == TableContentStorageMode::Unsupported
            || eligibility.storageMode
                   == TableContentStorageMode::LegacyDirectTable) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::UnsupportedTableVersion);
        }
        if (version == DRW::AC1021) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::AmbiguousTableContentStorage);
        }
        if (!record.semanticParsed || record.rowCount <= 0
            || record.columnCount <= 0 || record.cellCount == 0) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::NoSemanticTableContent);
        }
        if (tableHasNonPositiveDimensions(record)) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::NonPositiveDimension);
        }
        if (record.parentHandle == 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::MissingOwnerHandle);
        }
        if (record.fallbackRendered
            && findTableFallbackEntities(record.handle).empty()) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::MissingFallbackAttachment);
        }
        if (record.fallbackInvalidated) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::EditedFallback);
        }
        if (record.tableStyleHandle != 0 && !record.styleResolved) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::UnresolvedTableStyle);
        }
        if (tableHasUnresolvedCellStyleMap(record)) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::UnresolvedCellStyleMap);
        }
        if (record.m_unknownRangeCount != 0) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::UnknownSubrecordRange);
        }
        if (record.m_incompleteRangeCount != 0) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::IncompleteSubrecordRange);
        }
        if (record.m_overrideMaskCount != 0 || record.overrideCellCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::OverrideMask);
        }
        if (record.m_breakRangeCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::BreakData);
        }
        if (record.m_tableGeometryTailRangeCount != 0
            || record.geometryCellCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::GeometryTail);
        }
        if (record.mergedRangeCount != 0 || tableHasMergedCell(record)) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::MergedCell);
        }
        if (record.fieldContentCount != 0 || record.fieldHandleCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::FieldContent);
        }
        if (record.blockContentCount != 0 || record.blockHandleCount != 0
            || record.hasBlockContent) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::BlockContent);
        }
        if (record.attributeCount != 0 || record.attributeHandleCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::AttributeContent);
        }
        if (record.unknownContentCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::UnknownCellContent);
        }
        if (record.valueHandleCount != 0) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::IncompleteValuePayload);
        }
        if (record.fallbackRendered) {
            addTableNativeWriterBlocker(
                eligibility,
                TableNativeWriterBlocker::AnonymousBlockPolicyUnresolved);
        }
        if (record.textStyleHandleCount != 0
            || record.m_fallbackUnresolvedTextStyleCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::UnresolvedTextStyle);
        }
        if (record.lineTypeHandleCount != 0) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::UnresolvedLineType);
        }
        if (record.replayState == ReplayState::ReplayInvalidated) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::RawReplayInvalidated);
        }
        if (record.replayState == ReplayState::ReplayReplaced) {
            addTableNativeWriterBlocker(
                eligibility, TableNativeWriterBlocker::RawReplayReplaced);
        }
        eligibility.eligibleTextOnly = eligibility.blockers.empty();
    }

    static TableContentStorageMode tableContentStorageModeForVersion(
        DRW::Version version) {
        if (version == DRW::UNKNOWNV)
            return TableContentStorageMode::Unsupported;
        if (version < DRW::AC1021)
            return TableContentStorageMode::LegacyDirectTable;
        if (version == DRW::AC1021)
            return TableContentStorageMode::SeparateTableContent;
        return TableContentStorageMode::EmbeddedTableContent;
    }

    static void addTableNativeWriterBlocker(
        TableNativeWriterEligibility& eligibility,
        TableNativeWriterBlocker blocker) {
        if (!eligibility.hasBlocker(blocker))
            eligibility.blockers.push_back(blocker);
    }

    static bool tableHasNonPositiveDimensions(const TableRecord& record) {
        if (record.columnWidths.empty() || record.rowHeights.empty())
            return true;
        for (double width : record.columnWidths) {
            if (width <= 0.0)
                return true;
        }
        for (double height : record.rowHeights) {
            if (height <= 0.0)
                return true;
        }
        return false;
    }

    static bool tableHasMergedCell(const TableRecord& record) {
        for (const TableCellRecord& cell : record.cells) {
            if (cell.isMerged)
                return true;
        }
        return false;
    }

    bool tableHasUnresolvedCellStyleMap(const TableRecord& record) const {
        for (int styleId : record.cellStyleIds) {
            if (styleId >= 1 && styleId <= 4)
                continue;
            if (findCellStylesById(styleId).empty()
                && findTableStylesReferencingCellStyle(styleId).empty()) {
                return true;
            }
        }
        return false;
    }

    static void incrementTableStorageModeCount(
        TableNativeWriterBlockerCounts& counts,
        TableContentStorageMode storageMode) {
        switch (storageMode) {
        case TableContentStorageMode::LegacyDirectTable:
            ++counts.legacyDirectLayout;
            break;
        case TableContentStorageMode::SeparateTableContent:
            ++counts.separateTableContentLayout;
            break;
        case TableContentStorageMode::EmbeddedTableContent:
            ++counts.embeddedTableContentLayout;
            break;
        case TableContentStorageMode::Unsupported:
            ++counts.unsupportedLayout;
            break;
        }
    }

    static void incrementTableNativeWriterBlockerCount(
        TableNativeWriterBlockerCounts& counts,
        TableNativeWriterBlocker blocker) {
        switch (blocker) {
        case TableNativeWriterBlocker::NoSemanticTableContent:
            ++counts.noSemanticTableContent;
            break;
        case TableNativeWriterBlocker::MissingFallbackAttachment:
            ++counts.missingFallbackAttachment;
            break;
        case TableNativeWriterBlocker::EditedFallback:
            ++counts.editedFallback;
            break;
        case TableNativeWriterBlocker::UnresolvedTableStyle:
            ++counts.unresolvedTableStyle;
            break;
        case TableNativeWriterBlocker::UnresolvedCellStyleMap:
            ++counts.unresolvedCellStyleMap;
            break;
        case TableNativeWriterBlocker::UnknownSubrecordRange:
            ++counts.unknownSubrecordRange;
            break;
        case TableNativeWriterBlocker::IncompleteSubrecordRange:
            ++counts.incompleteSubrecordRange;
            break;
        case TableNativeWriterBlocker::OverrideMask:
            ++counts.overrideMask;
            break;
        case TableNativeWriterBlocker::BreakData:
            ++counts.breakData;
            break;
        case TableNativeWriterBlocker::GeometryTail:
            ++counts.geometryTail;
            break;
        case TableNativeWriterBlocker::MergedCell:
            ++counts.mergedCell;
            break;
        case TableNativeWriterBlocker::FieldContent:
            ++counts.fieldContent;
            break;
        case TableNativeWriterBlocker::BlockContent:
            ++counts.blockContent;
            break;
        case TableNativeWriterBlocker::AttributeContent:
            ++counts.attributeContent;
            break;
        case TableNativeWriterBlocker::UnknownCellContent:
            ++counts.unknownCellContent;
            break;
        case TableNativeWriterBlocker::IncompleteValuePayload:
            ++counts.incompleteValuePayload;
            break;
        case TableNativeWriterBlocker::MissingOwnerHandle:
            ++counts.missingOwnerHandle;
            break;
        case TableNativeWriterBlocker::UnsupportedTableVersion:
            ++counts.unsupportedTableVersion;
            break;
        case TableNativeWriterBlocker::AmbiguousTableContentStorage:
            ++counts.ambiguousTableContentStorage;
            break;
        case TableNativeWriterBlocker::AnonymousBlockPolicyUnresolved:
            ++counts.anonymousBlockPolicyUnresolved;
            break;
        case TableNativeWriterBlocker::UnresolvedTextStyle:
            ++counts.unresolvedTextStyle;
            break;
        case TableNativeWriterBlocker::UnresolvedLineType:
            ++counts.unresolvedLineType;
            break;
        case TableNativeWriterBlocker::RawReplayInvalidated:
            ++counts.rawReplayInvalidated;
            break;
        case TableNativeWriterBlocker::RawReplayReplaced:
            ++counts.rawReplayReplaced;
            break;
        case TableNativeWriterBlocker::NonPositiveDimension:
            ++counts.nonPositiveDimension;
            break;
        }
    }

    static bool rangeNameContains(const DRW_DwgSubrecordRange& range,
                                  const char *needle) {
        return range.m_name.find(needle) != UTF8STRING::npos;
    }

    void resolveTableStyle(TableRecord& record) const {
        for (const TableRecord& style : m_tables) {
            if (resolveTableStyle(record, style))
                return;
        }
    }

    static bool resolveTableStyle(TableRecord& record, const TableRecord& style) {
        if (record.tableStyleHandle == 0 || record.handle == style.handle
            || !isTableStyleRecord(style) || record.tableStyleHandle != style.handle) {
            return false;
        }
        record.styleResolved = true;
        return true;
    }

    static bool tableCellReferences(const TableCellRecord& cell, duint32 handle) {
        if (handle == 0)
            return false;
        if (cell.valueHandle == handle
            || cell.textStyleHandle == handle
            || cell.textStyleOverrideHandle == handle
            || cell.blockHandle == handle
            || cell.geometryHandle == handle) {
            return true;
        }
        for (duint32 contentHandle : cell.contentHandles) {
            if (contentHandle == handle)
                return true;
        }
        for (duint32 attributeHandle : cell.attributeHandles) {
            if (attributeHandle == handle)
                return true;
        }
        return false;
    }

    static bool tableRecordReferences(const TableRecord& record, duint32 handle) {
        if (handle == 0)
            return false;
        if (record.tableStyleHandle == handle)
            return true;
        for (duint32 valueHandle : record.valueHandles) {
            if (valueHandle == handle)
                return true;
        }
        for (duint32 blockHandle : record.blockHandles) {
            if (blockHandle == handle)
                return true;
        }
        for (duint32 fieldHandle : record.fieldHandles) {
            if (fieldHandle == handle)
                return true;
        }
        for (duint32 attributeHandle : record.attributeHandles) {
            if (attributeHandle == handle)
                return true;
        }
        for (duint32 textStyleHandle : record.textStyleHandles) {
            if (textStyleHandle == handle)
                return true;
        }
        for (duint32 lineTypeHandle : record.lineTypeHandles) {
            if (lineTypeHandle == handle)
                return true;
        }
        for (duint32 geometryHandle : record.geometryHandles) {
            if (geometryHandle == handle)
                return true;
        }
        for (const TableCellRecord& cell : record.cells) {
            if (tableCellReferences(cell, handle))
                return true;
        }
        return false;
    }

    static bool cellStyleMapReferences(const CellStyleMapRecord& record,
                                       duint32 handle) {
        if (handle == 0)
            return false;
        return containsValue(record.m_textStyleHandles, handle)
               || containsValue(record.m_lineTypeHandles, handle);
    }

    template<typename T>
    static bool containsValue(const std::vector<T>& values, const T& value) {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    static bool mleaderRecordReferences(const MLeaderRecord& record, duint32 handle) {
        if (handle == 0)
            return false;
        if (record.styleHandle == handle
            || record.leaderLineTypeHandle == handle
            || record.arrowHeadHandle == handle
            || record.textStyleHandle == handle
            || record.blockHandle == handle
            || record.effectiveLeaderLineTypeHandle == handle
            || record.effectiveArrowHeadHandle == handle
            || record.effectiveTextStyleHandle == handle
            || record.effectiveBlockHandle == handle) {
            return true;
        }
        for (duint32 arrowHandle : record.arrowHeadOverrideHandles) {
            if (arrowHandle == handle)
                return true;
        }
        for (duint32 attributeHandle : record.blockAttributeDefinitionHandles) {
            if (attributeHandle == handle)
                return true;
        }
        return false;
    }

    static bool viewRecordReferences(const ViewRecord& record, duint32 handle) {
        if (handle == 0)
            return false;
        return record.namedUcsHandle == handle
               || record.baseUcsHandle == handle
               || record.backgroundHandle == handle
               || record.visualStyleHandle == handle
               || record.sunHandle == handle
               || record.liveSectionHandle == handle;
    }

    static bool mleaderStyleRecordReferences(const MLeaderStyleRecord& record,
                                             duint32 handle) {
        if (handle == 0)
            return false;
        return record.leaderLineTypeHandle == handle
               || record.arrowHeadBlockHandle == handle
               || record.textStyleHandle == handle
               || record.blockHandle == handle;
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
        return record.family == RawObjectFamily::Associative
               || associativeKindFromRecordName(record.recordName) != AssociativeKind::Unknown
               || containsSubstring(record.className, "Assoc")
               || containsSubstring(record.className, "PersSubent")
               || containsSubstring(record.recordName, "ACSH")
               || containsSubstring(record.className, "AcSh");
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

    static bool isMLeaderStyleRawObject(const RawObjectRecord& record) {
        return record.recordName == "MLEADERSTYLE"
               || record.recordName == "ACDB_MLEADERSTYLE"
               || record.className == "AcDbMLeaderStyle";
    }

    static bool isTableRawObject(const RawObjectRecord& record) {
        return record.recordName == "ACAD_TABLE"
               || record.recordName == "TABLECONTENT"
               || record.recordName == "TABLESTYLE"
               || record.recordName == "CELLSTYLEMAP"
               || record.className == "AcDbTable"
               || record.className == "AcDbTableContent"
               || record.className == "AcDbTableStyle"
               || record.className == "AcDbCellStyleMap";
    }

    void invalidateRawTableObject(duint32 handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (record.handle == handle && isTableRawObject(record))
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    void invalidateRawMLeaderStyle(duint32 handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (record.handle == handle && isMLeaderStyleRawObject(record))
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

    static bool containsSubstring(const std::string& value, const char* needle) {
        return needle != nullptr && needle[0] != '\0'
               && value.find(needle) != std::string::npos;
    }

    static AssociativePrefixKind associativePrefixKindFromDrw(
        DRW_AssociativePrefixStatus::Kind kind) {
        switch (kind) {
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocAction:
                return AssociativePrefixKind::AcDbAssocAction;
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam:
                return AssociativePrefixKind::AcDbAssocActionParam;
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency:
                return AssociativePrefixKind::AcDbAssocDependency;
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocGeomDependency:
                return AssociativePrefixKind::AcDbAssocGeomDependency;
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork:
                return AssociativePrefixKind::AcDbAssocNetwork;
            case DRW_AssociativePrefixStatus::Kind::AcDbAssocActionBody:
                return AssociativePrefixKind::AcDbAssocActionBody;
            case DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr:
                return AssociativePrefixKind::AcDbEvalExpr;
            case DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode:
                return AssociativePrefixKind::AcDbShHistoryNode;
            case DRW_AssociativePrefixStatus::Kind::AcShActionBody:
                return AssociativePrefixKind::AcShActionBody;
        }
        return AssociativePrefixKind::AcDbAssocAction;
    }

    static AssociativePrefixParseStatus associativePrefixStatusFromDrw(
        DRW_AssociativePrefixStatus::ParseStatus status) {
        switch (status) {
            case DRW_AssociativePrefixStatus::ParseStatus::Complete:
                return AssociativePrefixParseStatus::Complete;
            case DRW_AssociativePrefixStatus::ParseStatus::Partial:
                return AssociativePrefixParseStatus::Partial;
            case DRW_AssociativePrefixStatus::ParseStatus::Missing:
                return AssociativePrefixParseStatus::Missing;
            case DRW_AssociativePrefixStatus::ParseStatus::UnsupportedVersion:
                return AssociativePrefixParseStatus::UnsupportedVersion;
            case DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow:
                return AssociativePrefixParseStatus::BoundedCountOverflow;
        }
        return AssociativePrefixParseStatus::Missing;
    }

    static AssociativePrefixStatusRecord makeAssociativePrefixStatus(
        const DRW_AssociativePrefixStatus& source) {
        AssociativePrefixStatusRecord record;
        record.kind = associativePrefixKindFromDrw(source.m_kind);
        record.status = associativePrefixStatusFromDrw(source.m_status);
        record.startBit = source.m_startBit;
        record.bitSize = source.m_bitSize;
        record.classVersion = source.m_classVersion;
        record.decodedHandleCount = source.m_decodedHandleCount;
        record.decodedValueCount = source.m_decodedValueCount;
        record.decodedCountValue = source.m_decodedCountValue;
        record.sourceAssumption = source.m_sourceAssumption;
        return record;
    }

    static bool containsHandle(const std::vector<duint32>& handles,
                               duint32 handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

    void appendAssociativeEdges(const AssociativeRecord& record) {
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::UnknownHandleReference,
                           record.owningNetworkHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::ActionBody,
                           record.actionBodyHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        for (const DRW_AssociativeHandleRef& ref : record.dependencyRefs) {
            addAssociativeEdge(record.handle, record.recordName, record.kind,
                               AssociativeEdgeKind::DependsOn, ref.m_handle,
                               AssociativeEdgeConfidence::ExplicitHandle);
        }
        for (const DRW_AssociativeHandleRef& ref : record.actionRefs) {
            addAssociativeEdge(record.handle, record.recordName, record.kind,
                               AssociativeEdgeKind::OwnsAction, ref.m_handle,
                               AssociativeEdgeConfidence::ExplicitHandle);
        }
        for (duint32 handle : record.ownedParamHandles) {
            addAssociativeEdge(record.handle, record.recordName, record.kind,
                               AssociativeEdgeKind::OwnsParameter, handle,
                               AssociativeEdgeConfidence::ExplicitHandle);
        }
        for (duint32 handle : record.ownedActionHandles) {
            addAssociativeEdge(record.handle, record.recordName, record.kind,
                               AssociativeEdgeKind::OwnsAction, handle,
                               AssociativeEdgeConfidence::ExplicitHandle);
        }
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::DependsOn,
                           record.dependencyHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::ReadDependency,
                           record.readDependencyHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::WriteDependency,
                           record.writeDependencyHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::EvaluationExpression,
                           record.rNodeHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
        addAssociativeEdge(record.handle, record.recordName, record.kind,
                           AssociativeEdgeKind::EvaluationExpression,
                           record.dNodeHandle,
                           AssociativeEdgeConfidence::ExplicitHandle);
    }

    void appendAssociativeEdges(const AcShRecord& record) {
        addAssociativeEdge(record.handle, record.recordName,
                           AssociativeKind::Unknown,
                           AssociativeEdgeKind::HistoryNode,
                           record.ownerHandle,
                           AssociativeEdgeConfidence::InferredFromClassLayout);
    }

    void addAssociativeEdge(
        duint32 sourceHandle, const std::string& sourceRecordName,
        AssociativeKind sourceKind, AssociativeEdgeKind edgeKind,
        duint32 targetHandle, AssociativeEdgeConfidence confidence) {
        if (sourceHandle == 0 || targetHandle == 0)
            return;
        for (const AssociativeEdgeRecord& edge : m_associativeEdges) {
            if (edge.sourceHandle == sourceHandle
                && edge.targetHandle == targetHandle
                && edge.edgeKind == edgeKind) {
                return;
            }
        }
        AssociativeEdgeRecord edge;
        edge.sourceHandle = sourceHandle;
        edge.targetHandle = targetHandle;
        edge.sourceRecordName = sourceRecordName;
        edge.sourceKind = sourceKind;
        edge.edgeKind = edgeKind;
        edge.confidence = confidence;
        m_associativeEdges.push_back(std::move(edge));
    }

    void invalidateAssociativeSemanticRecord(duint32 handle) {
        if (handle == 0)
            return;
        for (AssociativeRecord& record : m_associativeObjects) {
            if (record.handle == handle
                && record.replayState == ReplayState::ReplayAllowed) {
                record.replayState = ReplayState::ReplayInvalidated;
            }
        }
        for (AcShRecord& record : m_acshObjects) {
            if (record.handle == handle
                && record.replayState == ReplayState::ReplayAllowed) {
                record.replayState = ReplayState::ReplayInvalidated;
            }
        }
    }

    static void incrementAssociativeEdgeKindCount(
        AssociativeEdgeCounts& counts, AssociativeEdgeKind kind) {
        switch (kind) {
            case AssociativeEdgeKind::OwnsAction:
                ++counts.ownsAction;
                return;
            case AssociativeEdgeKind::OwnsParameter:
                ++counts.ownsParameter;
                return;
            case AssociativeEdgeKind::DependsOn:
                ++counts.dependsOn;
                return;
            case AssociativeEdgeKind::ReadDependency:
                ++counts.readDependency;
                return;
            case AssociativeEdgeKind::WriteDependency:
                ++counts.writeDependency;
                return;
            case AssociativeEdgeKind::ActionBody:
                ++counts.actionBody;
                return;
            case AssociativeEdgeKind::EvaluationExpression:
                ++counts.evaluationExpression;
                return;
            case AssociativeEdgeKind::HistoryNode:
                ++counts.historyNode;
                return;
            case AssociativeEdgeKind::GeometryReference:
                ++counts.geometryReference;
                return;
            case AssociativeEdgeKind::UnknownHandleReference:
            default:
                ++counts.unknownHandleReference;
                return;
        }
    }

    static void incrementAssociativeEdgeConfidenceCount(
        AssociativeEdgeCounts& counts, AssociativeEdgeConfidence confidence) {
        switch (confidence) {
            case AssociativeEdgeConfidence::ExplicitHandle:
                ++counts.explicitHandle;
                return;
            case AssociativeEdgeConfidence::InferredFromClassLayout:
                ++counts.inferredFromClassLayout;
                return;
            case AssociativeEdgeConfidence::Unknown:
            default:
                return;
        }
    }

    static void incrementAssociativePrefixCounts(
        AssociativePrefixCounts& counts,
        const AssociativePrefixStatusRecord& prefix) {
        ++counts.prefixCount;
        counts.decodedHandleCount += prefix.decodedHandleCount;
        counts.decodedValueCount += prefix.decodedValueCount;
        switch (prefix.kind) {
            case AssociativePrefixKind::AcDbAssocAction:
                ++counts.assocAction;
                break;
            case AssociativePrefixKind::AcDbAssocActionParam:
                ++counts.assocActionParam;
                break;
            case AssociativePrefixKind::AcDbAssocDependency:
                ++counts.assocDependency;
                break;
            case AssociativePrefixKind::AcDbAssocGeomDependency:
                ++counts.assocGeomDependency;
                break;
            case AssociativePrefixKind::AcDbAssocNetwork:
                ++counts.assocNetwork;
                break;
            case AssociativePrefixKind::AcDbAssocActionBody:
                ++counts.assocActionBody;
                break;
            case AssociativePrefixKind::AcDbEvalExpr:
                ++counts.evalExpr;
                break;
            case AssociativePrefixKind::AcDbShHistoryNode:
                ++counts.shHistoryNode;
                break;
            case AssociativePrefixKind::AcShActionBody:
                ++counts.shActionBody;
                break;
        }
        switch (prefix.status) {
            case AssociativePrefixParseStatus::Complete:
                ++counts.complete;
                break;
            case AssociativePrefixParseStatus::Partial:
                ++counts.partial;
                break;
            case AssociativePrefixParseStatus::Missing:
                ++counts.missing;
                break;
            case AssociativePrefixParseStatus::UnsupportedVersion:
                ++counts.unsupportedVersion;
                break;
            case AssociativePrefixParseStatus::BoundedCountOverflow:
                ++counts.boundedCountOverflow;
                break;
        }
    }

    static void incrementModelerPayloadRangeKindCount(
        ModelerPayloadCounts& counts, ModelerPayloadRangeKind kind) {
        switch (kind) {
            case ModelerPayloadRangeKind::Sat:
                ++counts.satRanges;
                return;
            case ModelerPayloadRangeKind::Sab:
                ++counts.sabRanges;
                return;
            case ModelerPayloadRangeKind::History:
                ++counts.historyRanges;
                return;
            case ModelerPayloadRangeKind::Wire:
                ++counts.wireRanges;
                return;
            case ModelerPayloadRangeKind::Silhouette:
                ++counts.silhouetteRanges;
                return;
            case ModelerPayloadRangeKind::HandleStream:
                ++counts.handleStreamRanges;
                return;
            case ModelerPayloadRangeKind::UnknownTail:
            default:
                ++counts.unknownTailRanges;
                return;
        }
    }

    static void incrementModelerPayloadRangeConsistencyCount(
        ModelerPayloadCounts& counts,
        ModelerPayloadRangeConsistency consistency) {
        switch (consistency) {
            case ModelerPayloadRangeConsistency::Exact:
                ++counts.exactRanges;
                return;
            case ModelerPayloadRangeConsistency::Truncated:
                ++counts.truncatedRanges;
                return;
            case ModelerPayloadRangeConsistency::Overrun:
                ++counts.overrunRanges;
                return;
            case ModelerPayloadRangeConsistency::Unknown:
            default:
                return;
        }
    }

    static void incrementRawObjectFamilyCount(
        RawObjectFamilyCounts& counts, RawObjectFamily family) {
        switch (family) {
            case RawObjectFamily::Associative:
                ++counts.associative;
                return;
            case RawObjectFamily::EvaluationGraph:
                ++counts.evaluationGraph;
                return;
            case RawObjectFamily::DynamicBlock:
                ++counts.dynamicBlock;
                return;
            case RawObjectFamily::ObjectContext:
                ++counts.objectContext;
                return;
            case RawObjectFamily::Unknown:
            default:
                ++counts.unknown;
                return;
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

    static void incrementAssociativeKindCount(
        AssociativeShellCounts& counts, AssociativeKind kind) {
        switch (kind) {
            case AssociativeKind::Network:
                ++counts.network;
                break;
            case AssociativeKind::Action:
                ++counts.action;
                break;
            case AssociativeKind::Dependency:
                ++counts.dependency;
                break;
            case AssociativeKind::GeometryDependency:
                ++counts.geometryDependency;
                break;
            case AssociativeKind::PersistentSubentityManager:
                ++counts.persistentSubentityManager;
                break;
            case AssociativeKind::AlignedDimensionActionBody:
                ++counts.alignedDimensionActionBody;
                break;
            case AssociativeKind::VertexActionParam:
                ++counts.vertexActionParam;
                break;
            case AssociativeKind::OsnapPointRefActionParam:
                ++counts.osnapPointRefActionParam;
                break;
            case AssociativeKind::Unknown:
            default:
                ++counts.unknown;
                break;
        }
    }

    std::vector<RawObjectRecord> m_rawObjects;
    std::vector<ViewRecord> m_views;
    std::vector<LightRecord> m_lights;
    std::vector<SunRecord> m_suns;
    std::vector<ModelerGeometryRecord> m_modelerGeometry;
    std::vector<TableRecord> m_tables;
    std::vector<TableFallbackEntityRecord> m_tableFallbackEntities;
    std::vector<CellStyleMapRecord> m_cellStyleMaps;
    std::vector<AssociativeRecord> m_associativeObjects;
    std::vector<AssociativeEdgeRecord> m_associativeEdges;
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
