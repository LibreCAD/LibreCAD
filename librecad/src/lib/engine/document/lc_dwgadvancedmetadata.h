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
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "drw_classes.h"
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
        SemanticOnly,
        VersionMismatch
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

    enum class GraphReplayFamily {
        Unknown,
        DimensionAssociation,
        EvaluationGraph,
        AcDbAssoc,
        DynamicBlock,
        ObjectContext,
        AcShHistory
    };

    enum class AdvancedEntityWriterFamily {
        Unknown,
        Mesh,
        Shape,
        Ole2Frame,
        RasterImage,
        Wipeout,
        Underlay,
        MLeader,
        ArcDimension
    };

    enum class AdvancedEntityWriterOdaCoverage {
        Complete,
        Partial,
        Absent
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

    enum class MeshRawRangeStatus {
        Unknown,
        Complete,
        Missing,
        Incomplete
    };

    enum class ExternalReferencePathStatus {
        Unknown,
        Empty,
        Relative,
        AbsolutePresent,
        AbsoluteMissing,
        External,
        UnsupportedScheme,
        CaseMismatchCandidate
    };

    enum class ClipBoundaryStatus {
        Unknown,
        NoBoundary,
        Rectangular,
        Polygonal,
        Malformed
    };

    enum class DocumentMappingSource {
        View,
        Ucs,
        Vport
    };

    enum class VisualMetadataSource {
        View,
        Vport,
        VisualStyle,
        Light,
        Sun
    };

    enum class VisualMetadataSpecCoverage {
        OdaCovered,
        CrossReferenceSourced,
        RawOnly
    };

    struct RawObjectRecord {
        int objectType = 0;
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t bodyBitSize = 0;
        std::uint64_t objectOffset = 0;
        std::uint32_t objectSize = 0;
        bool isEntity = false;
        bool isCustomClass = false;
        RawObjectFamily family = RawObjectFamily::Unknown;
        std::string recordName;
        std::string className;
        std::vector<std::uint8_t> rawBytes;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct RawDwgSectionRecord {
        std::string name;
        DRW::Version version = DRW::UNKNOWNV;
        std::vector<std::uint8_t> data;
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

    struct GraphReplayFamilyCounts {
        size_t unknown = 0;
        size_t dimensionAssociation = 0;
        size_t evaluationGraph = 0;
        size_t acDbAssoc = 0;
        size_t dynamicBlock = 0;
        size_t objectContext = 0;
        size_t acShHistory = 0;

        size_t total() const {
            return unknown + dimensionAssociation + evaluationGraph + acDbAssoc
                + dynamicBlock + objectContext + acShHistory;
        }
    };

    struct GraphReplayPolicyCounts {
        GraphReplayFamilyCounts preserved;
        GraphReplayFamilyCounts suppressed;
        size_t invalidated = 0;
        size_t replaced = 0;
        size_t entityReplayUnsupported = 0;
        size_t missingRawBytes = 0;
        size_t missingClassMetadata = 0;
        size_t semanticOnlyAssociative = 0;
        size_t semanticOnlyAcSh = 0;
        size_t editedEntity = 0;
        size_t missingTarget = 0;
        size_t unsupportedEvaluator = 0;
        size_t parserPartial = 0;
        size_t fallbackGeometryEdited = 0;
        size_t nativeReplacement = 0;
        size_t cyclePathInvalidated = 0;
        size_t ownerDeleted = 0;

        size_t totalSemanticOnly() const {
            return semanticOnlyAssociative + semanticOnlyAcSh;
        }

        size_t totalReasons() const {
            return editedEntity + missingTarget + unsupportedEvaluator
                + parserPartial + fallbackGeometryEdited + nativeReplacement
                + cyclePathInvalidated + ownerDeleted;
        }
    };

    struct AdvancedEntityWriterReadiness {
        AdvancedEntityWriterFamily family = AdvancedEntityWriterFamily::Unknown;
        AdvancedEntityWriterOdaCoverage odaCoverage =
            AdvancedEntityWriterOdaCoverage::Absent;
        std::uint32_t handle = 0;
        std::string recordName;
        std::string className;
        bool nativeWriterAvailable = false;
        bool rawReplayAvailable = false;
        bool fallbackAvailable = false;
        bool editedFallbackInvalidated = false;
        bool missingRequiredMetadata = false;
        bool missingPayloadBytes = false;
        bool unsupportedAdvancedContent = false;
    };

    struct AdvancedEntityWriterBlockerCounts {
        size_t recordCount = 0;
        size_t mesh = 0;
        size_t shape = 0;
        size_t ole2Frame = 0;
        size_t rasterImage = 0;
        size_t wipeout = 0;
        size_t underlay = 0;
        size_t mleader = 0;
        size_t arcDimension = 0;
        size_t unknown = 0;
        size_t nativeWriterAvailable = 0;
        size_t rawReplayAvailable = 0;
        size_t fallbackAvailable = 0;
        size_t editedFallbackInvalidated = 0;
        size_t missingRequiredMetadata = 0;
        size_t missingPayloadBytes = 0;
        size_t unsupportedAdvancedContent = 0;
        size_t odaComplete = 0;
        size_t odaPartial = 0;
        size_t odaAbsent = 0;

        size_t totalBlockers() const {
            return editedFallbackInvalidated + missingRequiredMetadata
                + missingPayloadBytes + unsupportedAdvancedContent;
        }
    };

    struct MeshRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        bool isSubDMesh = false;
        int schemaVersion = 0;
        int classVersion = 0;
        int subdivisionLevel = 0;
        int vertexCount = 0;
        int faceCount = 0;
        int edgeCount = 0;
        int creaseCount = 0;
        int smoothM = 0;
        int smoothN = 0;
        int curveType = 0;
        int flags = 0;
        size_t preservedVertexCount = 0;
        bool hasCreaseData = false;
        bool fallbackPreviewGenerated = false;
        bool fallbackInvalidated = false;
        MeshRawRangeStatus rawRangeStatus = MeshRawRangeStatus::Unknown;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct MeshSidecarRecord {
        std::uint32_t sourceHandle = 0;
        unsigned long long fallbackEntityId = 0;
        std::string meshId;
        std::string role;
        int elementIndex = -1;
        int elementCount = 0;
        int roleIndex = -1;
        int flags = 0;
        int mCount = 0;
        int nCount = 0;
        int smoothM = 0;
        int smoothN = 0;
        int curveType = 0;
        size_t sourceVertexCount = 0;
        bool anchor = false;
        bool editedFallback = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct MeshWriterBlockerCounts {
        size_t meshCount = 0;
        size_t sidecarCount = 0;
        size_t completeRawRange = 0;
        size_t missingRawRange = 0;
        size_t incompleteRawRange = 0;
        size_t missingCreaseData = 0;
        size_t unsupportedSubdivisionData = 0;
        size_t fallbackOnlyPreview = 0;
        size_t editedFallback = 0;
        size_t missingOwnerOrClassHandle = 0;
        size_t malformedCountRelationships = 0;
        size_t invalidated = 0;
        size_t replaced = 0;

        size_t totalBlockers() const {
            return missingCreaseData + unsupportedSubdivisionData
                   + fallbackOnlyPreview + editedFallback
                   + missingOwnerOrClassHandle + malformedCountRelationships
                   + invalidated + replaced;
        }
    };

    struct ExternalReferencePathDiagnostic {
        std::string path;
        ExternalReferencePathStatus status =
            ExternalReferencePathStatus::Unknown;
        bool caseMismatchCandidate = false;
    };

    struct RasterImageRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        bool isWipeout = false;
        std::uint32_t definitionHandle = 0;
        DRW_Coord basePoint;
        DRW_Coord uVector;
        DRW_Coord vVector;
        double sizeU = 0.0;
        double sizeV = 0.0;
        int clip = 0;
        int brightness = 50;
        int contrast = 50;
        int fade = 0;
        bool clipMode = false;
        size_t clipVertexCount = 0;
        ClipBoundaryStatus clipStatus = ClipBoundaryStatus::Unknown;
        bool invertedClip = false;
        bool frameVisible = true;
        bool hasDefinitionHandle = false;
        bool hasReactorHandle = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ImageDefinitionRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string path;
        int classVersion = 0;
        double imageSizeU = 0.0;
        double imageSizeV = 0.0;
        double pixelSizeU = 0.0;
        double pixelSizeV = 0.0;
        int loaded = 0;
        int resolution = 0;
        std::vector<std::uint32_t> reactorHandles;
        ExternalReferencePathDiagnostic pathDiagnostic;
        bool hasReactorHandle = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct UnderlayRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t definitionHandle = 0;
        int kind = 0;
        DRW_Coord position;
        DRW_Coord scale;
        double rotation = 0.0;
        std::uint8_t flags = 0;
        std::uint8_t contrast = 0;
        std::uint8_t fade = 0;
        size_t clipVertexCount = 0;
        ClipBoundaryStatus clipStatus = ClipBoundaryStatus::Unknown;
        bool frameVisible = true;
        bool fallbackPreviewGenerated = false;
        bool fallbackInvalidated = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct UnderlayDefinitionRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int kind = 0;
        std::string path;
        std::string sheetName;
        ExternalReferencePathDiagnostic pathDiagnostic;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct RasterVariablesRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int classVersion = 0;
        int imageFrame = 0;
        int imageQuality = 0;
        int units = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ExternalReferenceCounts {
        size_t imageEntities = 0;
        size_t wipeouts = 0;
        size_t imageDefinitions = 0;
        size_t underlays = 0;
        size_t underlayDefinitions = 0;
        size_t rasterVariables = 0;
        size_t emptyPaths = 0;
        size_t relativePaths = 0;
        size_t absoluteMissingPaths = 0;
        size_t externalPaths = 0;
        size_t unsupportedSchemes = 0;
        size_t caseMismatchCandidates = 0;
        size_t missingDefinitionHandles = 0;
        size_t definitionsWithoutEntities = 0;
        size_t noBoundaryClips = 0;
        size_t rectangularClips = 0;
        size_t polygonalClips = 0;
        size_t malformedClips = 0;
        size_t invertedClips = 0;
        size_t hiddenFrames = 0;

        size_t totalPathIssues() const {
            return emptyPaths + absoluteMissingPaths + unsupportedSchemes
                   + caseMismatchCandidates;
        }
    };

    struct ShapeRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t shapeFileHandle = 0;
        std::uint16_t shapeIndex = 0;
        std::string styleName;  // SHAPEFILE/STYLE record name (DXF code 2)
        DRW_Coord insertionPoint;
        DRW_Coord extrusion;
        double scale = 1.0;
        double rotation = 0.0;
        double oblique = 0.0;
        double widthFactor = 1.0;
        double thickness = 0.0;
        size_t rawByteCount = 0;
        MeshRawRangeStatus rawRangeStatus = MeshRawRangeStatus::Unknown;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct Ole2FrameRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint16_t flags = 0;
        std::uint16_t mode = 0;
        std::uint32_t declaredPayloadLength = 0;
        std::uint32_t payloadByteCount = 0;
        std::uint64_t payloadStartBit = 0;
        bool payloadPresent = false;
        bool payloadTruncated = false;
        bool payloadTooLarge = false;
        bool hasR2000TrailingByte = false;
        std::uint8_t r2000TrailingByte = 0;
        size_t rawByteCount = 0;
        bool previewFrameAvailable = false;
        bool previewFrameInvalidated = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
        std::uint16_t oleVersion = 2;            // DXF 70
        DRW_Coord pt1;                           // DXF 10/20/30 upper-left
        DRW_Coord pt2;                           // DXF 11/21/31 lower-right
        std::vector<std::uint8_t> payloadBytes;  // opaque OLE payload (DXF 310)
    };

    struct ShapeOleWriterBlockerCounts {
        size_t shapeCount = 0;
        size_t ole2FrameCount = 0;
        size_t missingStyleHandle = 0;
        size_t unresolvedShapeStyle = 0;
        size_t missingOlePayload = 0;
        size_t truncatedOlePayload = 0;
        size_t oversizedOlePayload = 0;
        size_t editedPreviewFrame = 0;
        size_t unsupportedOlePayloadRegeneration = 0;
        size_t missingRawRange = 0;
        size_t incompleteRawRange = 0;
        size_t invalidated = 0;
        size_t replaced = 0;

        size_t totalBlockers() const {
            return missingStyleHandle + unresolvedShapeStyle
                   + missingOlePayload + truncatedOlePayload
                   + oversizedOlePayload + editedPreviewFrame
                   + unsupportedOlePayloadRegeneration + missingRawRange
                   + incompleteRawRange + invalidated + replaced;
        }
    };

    struct ViewRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
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
        std::uint32_t namedUcsHandle = 0;
        std::uint32_t baseUcsHandle = 0;
        std::uint32_t backgroundHandle = 0;
        std::uint32_t visualStyleHandle = 0;
        std::uint32_t sunHandle = 0;
        std::uint32_t liveSectionHandle = 0;
        bool hasUcsHandleRefs = false;
        bool hasVisualHandleRefs = false;
        bool sunResolved = false;
        bool useDefaultLights = true;
        std::uint8_t defaultLightingType = 1;
        double brightness = 0.0;
        double contrast = 0.0;
        std::uint32_t ambientColor = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct UcsRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        DRW_Coord origin;
        DRW_Coord xAxisDirection;
        DRW_Coord yAxisDirection;
        DRW_Coord orthoOrigin;
        double elevation = 0.0;
        int orthoType = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct VportRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        DRW_Coord center;
        DRW_Coord viewTarget;
        DRW_Coord viewDirection;
        double height = 0.0;
        double ratio = 0.0;
        int viewMode = 0;
        int grid = 0;
        int snap = 0;
        int gridBehavior = 0;
        std::uint32_t backgroundHandle = 0;
        std::uint32_t visualStyleHandle = 0;
        std::uint32_t sunHandle = 0;
        std::uint32_t namedUcsHandle = 0;
        std::uint32_t baseUcsHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct DocumentMappingRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t sourceHandle = 0;
        DocumentMappingSource sourceType = DocumentMappingSource::View;
        std::string sourceName;
        std::string documentItemName;
        int documentItemIndex = -1;
        std::uint32_t ownerHandle = 0;
        std::uint32_t layoutHandle = 0;
        std::uint32_t associatedUcsHandle = 0;
        std::uint32_t baseUcsHandle = 0;
        std::uint32_t namedUcsHandle = 0;
        std::uint32_t plotViewHandle = 0;
        std::uint32_t backgroundHandle = 0;
        std::uint32_t visualStyleHandle = 0;
        std::uint32_t sunHandle = 0;
        std::uint32_t liveSectionHandle = 0;
        std::uint32_t viewportHeaderHandle = 0;
        std::uint32_t layoutBlockRecordHandle = 0;
        size_t unresolvedReferenceCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;

        bool isMappedToDocumentItem() const {
            return documentItemIndex >= 0;
        }
    };

    struct DocumentMappingCounts {
        size_t viewMappings = 0;
        size_t ucsMappings = 0;
        size_t vportMappings = 0;
        size_t mappedDocumentItems = 0;
        size_t unresolvedReferences = 0;
        size_t invalidated = 0;
        size_t replaced = 0;
    };

    struct VisualStyleRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        std::string description;
        std::int32_t type = 0;   // mirrors DRW_VisualStyle::type widening BS->BL (P3.3)
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct VisualMetadataSummaryRecord {
        VisualMetadataSource sourceType = VisualMetadataSource::View;
        VisualMetadataSpecCoverage specCoverage =
            VisualMetadataSpecCoverage::OdaCovered;
        std::string displayName;
        std::uint32_t handle = 0;
        std::uint32_t ownerHandle = 0;
        std::uint32_t layoutHandle = 0;
        std::uint32_t lightOrSunType = 0;
        bool lightOrSunEnabled = false;
        double intensity = 0.0;
        std::uint32_t color = 0;
        std::int32_t julianDay = 0;
        std::int32_t milliseconds = 0;
        bool daylightSavings = false;
        std::uint32_t referencedSunHandle = 0;
        std::uint32_t referencedVisualStyleHandle = 0;
        std::uint32_t referencedBackgroundHandle = 0;
        std::uint32_t referencedLiveSectionHandle = 0;
        size_t unresolvedReferenceCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct VisualMetadataSummaryCounts {
        size_t view = 0;
        size_t vport = 0;
        size_t visualStyle = 0;
        size_t light = 0;
        size_t sun = 0;
        size_t odaCovered = 0;
        size_t crossReferenceSourced = 0;
        size_t rawOnly = 0;
        size_t invalidated = 0;
        size_t replaced = 0;
        size_t unresolvedReferences = 0;
        size_t ownerMapped = 0;
        size_t layoutMapped = 0;
    };

    struct VisualMetadataReplayEligibility {
        VisualMetadataSource sourceType = VisualMetadataSource::View;
        std::uint32_t handle = 0;
        bool hasSemanticRecord = false;
        bool hasRawPayload = false;
        bool rawReplayable = false;
        bool nativeWriterAvailable = false;
        bool unsupportedNativeWriter = false;
        ReplayBlocker rawBlocker = ReplayBlocker::SemanticOnly;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct VisualMetadataWriterBlockerCounts {
        size_t recordCount = 0;
        size_t rawPayloads = 0;
        size_t replayableRawPayloads = 0;
        size_t suppressedRawPayloads = 0;
        size_t unresolvedUcs = 0;
        size_t unresolvedBaseUcs = 0;
        size_t unresolvedVisualStyle = 0;
        size_t unresolvedSun = 0;
        size_t unresolvedBackground = 0;
        size_t unresolvedLiveSection = 0;
        size_t missingOwnerOrLayout = 0;
        size_t invalidatedRawPayload = 0;
        size_t replacedNativeUnavailablePayload = 0;
        size_t unsupportedVisualStyleWriter = 0;

        size_t totalBlockers() const {
            return unresolvedUcs + unresolvedBaseUcs + unresolvedVisualStyle
                   + unresolvedSun + unresolvedBackground
                   + unresolvedLiveSection + missingOwnerOrLayout
                   + invalidatedRawPayload
                   + replacedNativeUnavailablePayload
                   + unsupportedVisualStyleWriter;
        }
    };

    struct LightRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t classVersion = 0;
        std::string name;
        std::uint32_t type = 0;
        bool status = false;
        std::uint32_t color = 0;
        bool plotGlyph = false;
        double intensity = 0.0;
        DRW_Coord position;
        DRW_Coord target;
        std::uint32_t attenuationType = 0;
        bool useAttenuationLimits = false;
        double attenuationStartLimit = 0.0;
        double attenuationEndLimit = 0.0;
        double hotspotAngle = 0.0;
        double falloffAngle = 0.0;
        bool castShadows = false;
        std::uint32_t shadowType = 0;
        std::uint16_t shadowMapSize = 0;
        std::uint8_t shadowMapSoftness = 0;
        bool hasPhotometricData = false;
        bool hasWebFile = false;
        std::string webFile;
        std::uint16_t physicalIntensityMethod = 0;
        double physicalIntensity = 0.0;
        double illuminanceDistance = 0.0;
        std::uint16_t lampColorType = 0;
        double lampColorTemperature = 0.0;
        std::uint16_t lampColorPreset = 0;
        DRW_Coord webRotation{1.0, 0.0, 0.0};
        std::uint16_t extendedLightShape = 0;
        double extendedLightLength = 0.0;
        double extendedLightWidth = 0.0;
        double extendedLightRadius = 0.0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SunRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t classVersion = 0;
        bool isOn = false;
        std::uint32_t color = 0;
        double intensity = 0.0;
        bool hasShadow = false;
        std::int32_t julianDay = 0;
        std::int32_t milliseconds = 0;
        bool isDaylightSavings = false;
        std::uint32_t shadowType = 0;
        std::uint16_t shadowMapSize = 0;
        std::uint8_t shadowSoftness = 0;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        DRW::ETYPE type = DRW::UNKNOWN;
        std::uint16_t modelerVersion = 0;
        std::uint32_t bodyBitSize = 0;
        std::uint32_t objectSize = 0;
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
        std::uint32_t historyHandle = 0;
        size_t rawByteCount = 0;
        std::vector<std::uint8_t> rawBytes;
        std::vector<ModelerPayloadRangeRecord> payloadRanges;
        std::vector<std::shared_ptr<DRW_Variant>> extData;
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
        std::uint32_t overrideFlags = 0;
        std::uint32_t valueHandle = 0;
        std::uint32_t textStyleHandle = 0;
        std::uint32_t textStyleOverrideHandle = 0;
        std::uint32_t blockHandle = 0;
        std::uint32_t geometryHandle = 0;
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
        std::vector<std::uint32_t> contentHandles;
        std::vector<std::uint32_t> attributeHandles;
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
        std::uint32_t tableHandle = 0;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        std::uint32_t tableStyleHandle = 0;
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
        std::vector<std::uint32_t> valueHandles;
        std::vector<std::uint32_t> blockHandles;
        std::vector<std::uint32_t> fieldHandles;
        std::vector<std::uint32_t> attributeHandles;
        std::vector<std::uint32_t> textStyleHandles;
        std::vector<std::uint32_t> lineTypeHandles;
        std::vector<std::uint32_t> geometryHandles;
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
        std::uint32_t tableHandle = 0;
        size_t gridEntityCount = 0;
        size_t textEntityCount = 0;
        size_t placeholderEntityCount = 0;
        size_t unresolvedTextStyleCount = 0;
        size_t clampedDimensionCount = 0;
    };

    struct TableFallbackEntityRecord {
        std::uint32_t tableHandle = 0;
        std::uint32_t sourceHandle = 0;
        int row = -1;
        int column = -1;
        TableFallbackRole role = TableFallbackRole::Placeholder;
        unsigned long long entityId = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct CellStyleMapRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
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
        std::vector<std::uint32_t> m_textStyleHandles;
        std::vector<std::uint32_t> m_lineTypeHandles;
        std::vector<double> m_textHeights;
        std::vector<int> m_alignments;
        std::vector<int> m_colors;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativePrefixStatusRecord {
        AssociativePrefixKind kind = AssociativePrefixKind::AcDbAssocAction;
        AssociativePrefixParseStatus status =
            AssociativePrefixParseStatus::Missing;
        std::uint64_t startBit = 0;
        std::uint64_t bitSize = 0;
        std::uint16_t classVersion = 0;
        size_t decodedHandleCount = 0;
        size_t decodedValueCount = 0;
        std::int32_t decodedCountValue = 0;
        std::string sourceAssumption;
    };

    struct AssociativeRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        AssociativeKind kind = AssociativeKind::Unknown;
        std::uint16_t classVersion = 0;
        std::int32_t geometryStatus = 0;
        std::uint32_t owningNetworkHandle = 0;
        std::uint32_t actionBodyHandle = 0;
        std::int32_t actionIndex = 0;
        std::int32_t maxDependencyIndex = 0;
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
        std::vector<std::uint32_t> ownedParamHandles;
        std::vector<std::uint32_t> ownedActionHandles;
        std::uint32_t dependencyHandle = 0;
        std::uint32_t readDependencyHandle = 0;
        std::uint32_t writeDependencyHandle = 0;
        std::uint32_t rNodeHandle = 0;
        std::uint32_t dNodeHandle = 0;
        std::int32_t status = 0;
        std::uint8_t osnapMode = 0;
        double parameter = 0.0;
        DRW_Coord point;
        std::vector<AssociativePrefixStatusRecord> prefixStatuses;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct AssociativeEdgeRecord {
        std::uint32_t sourceHandle = 0;
        std::uint32_t targetHandle = 0;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        std::uint32_t major = 0;
        std::uint32_t minor = 0;
        std::uint32_t ownerHandle = 0;
        std::uint32_t historyNodeId = 0;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint16_t classVersion = 0;
        std::uint32_t styleHandle = 0;
        std::int32_t overrideFlags = 0;
        std::uint16_t leaderType = 0;
        std::uint16_t styleContentType = 0;
        std::uint32_t leaderLineTypeHandle = 0;
        std::uint32_t arrowHeadHandle = 0;
        std::uint32_t textStyleHandle = 0;
        std::uint32_t blockHandle = 0;
        std::uint16_t effectiveContentType = 0;
        std::uint16_t effectiveLeaderType = 0;
        std::uint32_t effectiveLeaderLineTypeHandle = 0;
        std::uint32_t effectiveArrowHeadHandle = 0;
        std::uint32_t effectiveTextStyleHandle = 0;
        std::uint32_t effectiveBlockHandle = 0;
        size_t rootCount = 0;
        size_t leaderLineCount = 0;
        size_t pointCount = 0;
        size_t breakCount = 0;
        size_t columnCount = 0;
        size_t arrowHeadOverrideCount = 0;
        size_t blockLabelCount = 0;
        std::vector<std::uint32_t> arrowHeadOverrideHandles;
        std::vector<std::uint32_t> blockAttributeDefinitionHandles;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        std::uint16_t styleVersion = 0;
        std::uint16_t contentType = 0;
        std::uint16_t drawMLeaderOrder = 0;
        std::uint16_t drawLeaderOrder = 0;
        std::int32_t maxLeaderPoints = 0;
        double firstSegmentAngle = 0.0;
        double secondSegmentAngle = 0.0;
        std::uint16_t leaderType = 0;
        int leaderColor = 0;
        std::uint32_t leaderLineTypeHandle = 0;
        std::int32_t leaderLineWeight = 0;
        bool landingEnabled = false;
        double landingGap = 0.0;
        bool autoIncludeLanding = false;
        double landingDistance = 0.0;
        std::string description;
        std::uint32_t arrowHeadBlockHandle = 0;
        std::uint32_t textStyleHandle = 0;
        std::uint32_t blockHandle = 0;
        double arrowHeadSize = 0.0;
        std::string textDefault;
        std::uint16_t leftAttachment = 0;
        std::uint16_t rightAttachment = 0;
        std::uint16_t textAngleType = 0;
        std::uint16_t textAlignmentType = 0;
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
        std::uint16_t blockConnectionType = 0;
        double scaleFactor = 1.0;
        bool propertyChanged = false;
        bool isAnnotative = false;
        double breakSize = 0.0;
        std::uint16_t attachmentDirection = 0;
        std::uint16_t topAttachment = 0;
        std::uint16_t bottomAttachment = 0;
        bool textExtended = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct DetailViewStyleRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        std::string description;
        std::string displayName;
        std::uint16_t classVersion = 0;
        std::uint32_t flags = 0;
        std::uint32_t identifierStyleHandle = 0;
        std::uint32_t arrowSymbolHandle = 0;
        std::uint32_t viewLabelTextStyleHandle = 0;
        std::uint32_t boundaryLineTypeHandle = 0;
        std::uint32_t connectionLineTypeHandle = 0;
        std::uint32_t borderLineTypeHandle = 0;
        std::string viewLabelPattern;
        double identifierHeight = 0.0;
        double arrowSymbolSize = 0.0;
        double viewLabelTextHeight = 0.0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SectionViewStyleRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        std::string description;
        std::string displayName;
        std::uint16_t classVersion = 0;
        std::uint32_t flags = 0;
        std::uint32_t identifierStyleHandle = 0;
        std::uint32_t arrowStartSymbolHandle = 0;
        std::uint32_t arrowEndSymbolHandle = 0;
        std::uint32_t planeLineTypeHandle = 0;
        std::uint32_t bendLineTypeHandle = 0;
        std::uint32_t viewLabelTextStyleHandle = 0;
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
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t dimensionHandle = 0;
        std::vector<std::uint32_t> pointRefHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct BreakPointRefRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct GroupRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string description;
        bool isUnnamed = false;
        bool selectable = true;
        std::vector<std::uint32_t> entityHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct ImageDefinitionReactorRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::int32_t classVersion = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct SpatialFilterRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        size_t boundaryPointCount = 0;
        bool displayBoundary = false;
        bool clipFrontPlane = false;
        bool clipBackPlane = false;
        double frontDistance = 0.0;
        double backDistance = 0.0;
        // PR 8d.1d: extended fields needed for round-trip encoding.
        std::vector<DRW_Coord> boundaryPoints;
        DRW_Coord normal{0.0, 0.0, 1.0};
        DRW_Coord origin{0.0, 0.0, 0.0};
        std::vector<double> inverseInsertTransform;   // 12 doubles (4x3 matrix)
        std::vector<double> insertTransform;          // 12 doubles
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct GeoMeshPointRecord {
        DRW_Coord source;
        DRW_Coord destination;
    };

    struct GeoMeshFaceRecord {
        std::int32_t index1 = 0;
        std::int32_t index2 = 0;
        std::int32_t index3 = 0;
    };

    struct GeoDataRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t hostBlockHandle = 0;
        std::int32_t version = 0;
        std::int16_t coordinatesType = 0;
        std::int32_t horizontalUnits = 0;
        std::int32_t verticalUnits = 0;
        double horizontalUnitScale = 1.0;
        double verticalUnitScale = 1.0;
        std::string coordinateSystemDefinition;
        std::string geoRssTag;
        size_t meshPointCount = 0;
        size_t meshFaceCount = 0;
        // PR 8d.1c: extended fields needed for round-trip encoding.
        DRW_Coord designPoint;
        DRW_Coord referencePoint;
        DRW_Coord upDirection{0.0, 0.0, 1.0};
        DRW_Coord northDirection{0.0, 1.0, 0.0};
        std::int32_t scaleEstimationMethod = 0;
        double userSpecifiedScaleFactor = 1.0;
        bool enableSeaLevelCorrection = false;
        double seaLevelElevation = 0.0;
        double coordinateProjectionRadius = 0.0;
        std::string observationFromTag;
        std::string observationToTag;
        std::string observationCoverageTag;
        std::vector<GeoMeshPointRecord> meshPoints;
        std::vector<GeoMeshFaceRecord> meshFaces;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct TableGeometryRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::int32_t rowCount = 0;
        std::int32_t columnCount = 0;
        std::int32_t cellCount = 0;
        size_t contentCount = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    struct PlaceholderRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// DICTIONARY (named-object container, ODA fixed type 42).  Round-trip
    /// grade — captures every field DRW_Dictionary::encodeDwg consumes
    /// (cloning, hardOwner, name + per-entry (name, handle) pairs).
    struct DictionaryEntryRecord {
        std::string name;
        std::uint32_t handle = 0;
    };
    struct DictionaryRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int cloning = 0;
        int hardOwner = 0;
        std::string name;
        std::vector<DictionaryEntryRecord> entries;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// XRECORD (extended data record, ODA fixed type 0x4f).  Round-trip
    /// grade — keeps the cloning flag, the DRW_Variant data list (carrying
    /// arbitrary DXF-coded primitives), and the (code, handle) handle-stream
    /// pairs.  Storing DRW_Variant directly mirrors existing records that
    /// already embed DRW_* types (DRW_Coord, DRW_TableMergedRangeRecord).
    struct XRecordRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int cloning = 0;
        std::vector<DRW_Variant> values;
        std::vector<std::pair<int, std::uint32_t>> handleValues;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// LAYOUT (paper-space layout, ODA fixed type 82, §20.4.84).  Round-trip
    /// grade — captures every PlotSettings-prefix field AND every Layout-
    /// specific field consumed by DRW_Layout::encodeDwg.  Surfaces in
    /// LibreCAD's UI via PR 9-11 (RS_Graphic::layouts() — thin view over
    /// this record).
    struct LayoutRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        // PlotSettings prefix (ODA §20.4.84 embeds these inline in LAYOUT).
        std::string pageSetupName;
        std::string printerConfig;
        int plotLayoutFlags = 0;
        double marginLeft = 0.0;
        double marginBottom = 0.0;
        double marginRight = 0.0;
        double marginTop = 0.0;
        double paperWidth = 0.0;
        double paperHeight = 0.0;
        std::string paperSize;
        double plotOriginX = 0.0;
        double plotOriginY = 0.0;
        int paperUnits = 0;
        int plotRotation = 0;
        int plotType = 0;
        double windowMinX = 0.0;
        double windowMinY = 0.0;
        double windowMaxX = 0.0;
        double windowMaxY = 0.0;
        std::string plotViewName;
        double realWorldUnits = 1.0;
        double drawingUnits = 1.0;
        std::string currentStyleSheet;
        int scaleType = 0;
        double scaleFactor = 1.0;
        double paperImageOriginX = 0.0;
        double paperImageOriginY = 0.0;
        int shadePlotMode = 0;
        int shadePlotResLevel = 0;
        int shadePlotCustomDPI = 0;
        // Layout-specific fields.
        std::string name;
        int layoutFlags = 0;
        int tabOrder = 0;
        DRW_Coord ucsOrigin;
        double limMinX = 0.0;
        double limMinY = 0.0;
        double limMaxX = 0.0;
        double limMaxY = 0.0;
        DRW_Coord insPoint;
        DRW_Coord ucsXAxis{1.0, 0.0, 0.0};
        DRW_Coord ucsYAxis{0.0, 1.0, 0.0};
        double elevation = 0.0;
        int orthoViewType = 0;
        DRW_Coord extMin;
        DRW_Coord extMax;
        std::int32_t viewportCount = 0;
        // Handle refs.
        std::uint32_t plotViewHandle = 0;
        std::uint32_t visualStyleHandle = 0;
        std::uint32_t paperSpaceBlockRecordHandle = 0;
        std::uint32_t lastActiveViewportHandle = 0;
        std::uint32_t baseUcsHandle = 0;
        std::uint32_t namedUcsHandle = 0;
        std::vector<std::uint32_t> viewportHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// PR 8d.2a — five small no-storage OBJECTS families.  All round-trip-grade
    /// (capture every field DRW_X::encodeDwg consumes).

    /// SCALE (AcDbScale, custom class 508) — annotation-scale entry.
    /// Body fields: flag + name (in DRW_TableEntry) + paperUnits + drawingUnits
    /// + isUnitScale.  The SCALE encoder leaves the common-handle prefix to
    /// the wrapper (see dwgWriter15::emitScaleObject).
    struct ScaleRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;
        std::uint16_t flag = 0;
        double paperUnits = 1.0;
        double drawingUnits = 1.0;
        bool isUnitScale = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// OBJECTCONTEXTDATA annotation-scale payloads. LibreCAD has no
    /// annotation-scale renderer yet, so this is intentionally a metadata
    /// shell: typed enough for corpus validation and future scale resolution,
    /// with raw DWG replay still carried by RawObjectRecord.
    struct ObjectContextDataRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        DRW_ObjectContextData::Kind kind = DRW_ObjectContextData::Kind::Unknown;
        std::uint16_t classVersion = 0;
        bool defaultFlag = false;
        std::uint32_t scaleHandle = 0;
        std::uint32_t blockHandle = 0;
        std::uint32_t annotatedHandle = 0;
        std::int16_t horizontalMode = 0;
        std::int32_t attachment = 0;
        double rotation = 0.0;
        DRW_Coord insertionPoint;
        DRW_Coord alignmentPoint;
        DRW_Coord xAxisDir{1.0, 0.0, 0.0};
        double rectangleWidth = 0.0;
        double rectangleHeight = 0.0;
        double extentsWidth = 0.0;
        double extentsHeight = 0.0;
        DRW_Coord definitionPoint;
        bool isDefaultTextLocation = false;
        bool dimTofl = false;
        bool dimOsxd = false;
        bool dimAtFit = false;
        bool dimTix = false;
        bool dimTMove = false;
        bool hasArrow2 = false;
        bool flipArrow2 = false;
        bool flipArrow1 = false;
        double textRotation = 0.0;
        std::int32_t overrideCode = 0;
        DRW_Coord featureLocationPoint;
        DRW_Coord leaderEndpoint;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// MLINESTYLE (AcDbMlineStyle, FIXED built-in type 73 — no CLASS record).
    /// Durable metadata so the DWG->DXF path can re-emit the style typed (the
    /// transient read cache in RS_FilterDXFRW is DXF-read-only).  Per-element
    /// lists mirror DRW_MLineStyle::elements (offset 49 / color 62 / linetype 6).
    struct MLineStyleElementRecord {
        double offset = 0.0;
        int color = 256;
        std::string linetype;
    };
    struct MLineStyleRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string name;            // DXF 2 (DRW_TableEntry::name)
        int flags = 0;               // DXF 70
        std::string description;     // DXF 3
        int fillColor = 256;         // DXF 62 (before any element)
        double startAngle = 0.0;     // DXF 51
        double endAngle = 0.0;       // DXF 52
        std::vector<MLineStyleElementRecord> elements;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class). Durable metadata so
    /// the DWG->DXF path can re-emit it typed (DXF read keeps it in the raw net;
    /// writeObjects dedups by handle). Only body field is the display-frame flag.
    struct WipeoutVariablesRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int displayFrame = 0;        // DXF 70
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// IDBUFFER (AcDbIdBuffer, custom class 509) — list of object handles
    /// (used by selection filters and LAYER_INDEX entries).
    struct IDBufferRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int classVersion = 0;
        std::vector<std::uint32_t> objIds;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// LAYER_INDEX (AcDbLayerIndex, custom class 510) — per-layer entity index
    /// for partial-load drawings.
    struct LayerIndexEntryRecord {
        int indexLong = 0;
        std::string name;
        std::uint32_t entryHandle = 0;
    };
    struct LayerIndexRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t timestamp1 = 0;
        std::uint32_t timestamp2 = 0;
        std::vector<LayerIndexEntryRecord> entries;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// SPATIAL_INDEX (AcDbSpatialIndex, custom class 511) — spatial entity
    /// index.  Only timestamps are parsed (body beyond is opaque per ODA spec).
    struct SpatialIndexRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::uint32_t timestamp1 = 0;
        std::uint32_t timestamp2 = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// DICTIONARYVAR (AcDbDictionaryVar, custom class 512) — schema + value
    /// pair stored under a named-object dictionary entry.
    struct DictionaryVarRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int schema = 0;
        std::string value;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// PR 8d.2b — four larger no-storage OBJECTS families.  Round-trip-grade
    /// capture mirroring the DRW_X::encodeDwg field set.

    /// DICTIONARYWDFLT (AcDbDictionaryWithDefault, custom class 513) — a
    /// regular dictionary plus a single fallback handle returned when a
    /// lookup misses.  Stores the full dictionary state plus the default.
    struct DictionaryWithDefaultEntryRecord {
        std::string name;
        std::uint32_t handle = 0;
    };
    struct DictionaryWithDefaultRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int cloning = 0;
        int hardOwner = 0;
        std::string name;
        std::vector<DictionaryWithDefaultEntryRecord> entries;
        std::uint32_t defaultEntryHandle = 0;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// SORTENTSTABLE (AcDbSortentsTable, custom class 514) — per-block draw-
    /// order override.  Stores parallel sort + entity handle lists plus the
    /// block-owner handle.
    struct SortEntsTableRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::vector<std::uint32_t> sortHandles;
        std::uint32_t blockOwnerHandle = 0;
        std::vector<std::uint32_t> entityHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// FIELDLIST (AcDbFieldList, custom class 515) — handle list of FIELD
    /// children plus the "unknown" bit captured by the parser.
    struct FieldListRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int unknown = 0;
        std::vector<std::uint32_t> fieldHandles;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// DATATABLE (AcDbDataTable, custom class 520) — a hidden key/value table
    /// (no geometry).  We mirror the decoded prefix (flags + column/row counts
    /// + table name) and the best-effort column/row body captured by the
    /// parser.  The full byte image is additionally preserved on the raw shelf
    /// (addUnsupportedObject) so the round-trip stays lossless even when the
    /// DEBUG-class body walk drifts.
    struct DataTableRowRecord {
        std::int32_t dataLong = 0;
        double dataDouble = 0.0;
        std::string dataString;
    };
    struct DataTableColumnRecord {
        std::int32_t type = 0;
        std::string text;
        std::vector<DataTableRowRecord> rows;
    };
    struct DataTableRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        int flags = 0;
        int columnCount = 0;
        int rowCount = 0;
        std::string tableName;
        std::vector<DataTableColumnRecord> columns;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// Dynamic-block object family (BLOCK*PARAMETER / BLOCK*ACTION / BLOCK*GRIP /
    /// BLOCKGRIPLOCATIONCOMPONENT / DYNAMICBLOCK* + singletons) — the largest
    /// custom-class family.  None has a native LibreCAD consumer, so we mirror
    /// the typed CAPTURE the parser delivers: the shared AcDbEvalExpr prefix for
    /// every evalexpr-bearing class, plus the AcDbBlockElement / AcDbBlockParameter
    /// element data for the parameter/grip/action classes, and the full body for
    /// BLOCKVISIBILITYPARAMETER / BLOCKMOVEACTION.  The exact byte image is
    /// preserved on the raw shelf (addUnsupportedObject) so the round-trip stays
    /// lossless regardless of how deep the typed decode reached.
    struct DynamicBlockRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string recordName;
        int kind = 0;                 ///< DRW_DynamicBlockObject::Kind
        // AcDbEvalExpr prefix.
        bool evalExprParsed = false;
        std::uint32_t parentId = 0;
        std::uint32_t major = 0;
        std::uint32_t minor = 0;
        int valueCode = 0;
        std::uint32_t nodeId = 0;
        // AcDbBlockElement prefix.
        bool elementParsed = false;
        std::string elementName;
        std::int32_t eed1071 = 0;
        // AcDbBlockParameter.
        bool showProperties = false;
        bool chainActions = false;
        // AcDbBlockGripExpr (BLOCKGRIPLOCATIONCOMPONENT).
        std::int32_t gripType = 0;
        std::string gripExpr;
        // BLOCKVISIBILITYPARAMETER body.
        bool isInitialized = false;
        std::string visibilityName;
        std::string visibilityDescription;
        int blockCount = 0;
        int stateCount = 0;
        std::vector<std::string> stateNames;
        // BLOCKMOVEACTION body.
        int dependencyCount = 0;
        int actionCount = 0;
        double actionOffsetX = 0.0;
        double actionOffsetY = 0.0;
        double angleOffset = 0.0;
        std::vector<std::string> connectionNames;
        bool bodyFullyDecoded = false;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    /// FIELD (AcDbField, custom class 516) — typed value with child field +
    /// object references and a CadValue payload.  We mirror the on-wire
    /// DRW_CadValue + ChildValue structs verbatim so the encoder can
    /// round-trip without lossy normalisation.
    struct CadValueRecord {
        int formatFlags = 0;
        int dataType = 0;
        std::uint32_t dataSize = 0;
        int unitType = 0;
        DRW_Variant value;
        std::string formatString;
        std::string valueString;
        std::uint32_t handle = 0;
        std::vector<std::uint8_t> rawData;
    };
    struct FieldChildValueRecord {
        std::string key;
        CadValueRecord value;
    };
    struct FieldRecord {
        std::uint32_t handle = 0;
        std::uint32_t parentHandle = 0;
        std::string evaluatorId;
        std::string fieldCode;
        std::string formatString;
        int evaluationOptionFlags = 0;
        int filingOptionFlags = 0;
        int fieldStateFlags = 0;
        int evaluationStatusFlags = 0;
        int evaluationErrorCode = 0;
        std::string evaluationErrorMessage;
        CadValueRecord value;
        std::string valueString;
        int valueStringLength = 0;
        std::vector<std::uint32_t> childHandles;
        std::vector<std::uint32_t> objectHandles;
        std::vector<FieldChildValueRecord> childValues;
        ReplayState replayState = ReplayState::ReplayAllowed;
    };

    void clear() {
        m_sourceDwgVersion = DRW::UNKNOWNV;
        m_rawObjects.clear();
        m_rawDwgSections.clear();
        m_dxfClasses.clear();
        m_rawDxfObjects.clear();
        m_rawDxfEntities.clear();
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
        m_rasterImages.clear();
        m_imageDefinitions.clear();
        m_underlays.clear();
        m_underlayDefinitions.clear();
        m_rasterVariables.clear();
        m_shapes.clear();
        m_ole2Frames.clear();
        m_imageDefinitionReactors.clear();
        m_spatialFilters.clear();
        m_geoData.clear();
        m_tableGeometry.clear();
        m_placeholders.clear();
        m_dictionaries.clear();
        m_xrecords.clear();
        m_layouts.clear();
        m_scales.clear();
        m_objectContextData.clear();
        m_mlineStyles.clear();
        m_wipeoutVariables.clear();
        m_idBuffers.clear();
        m_layerIndexes.clear();
        m_spatialIndexes.clear();
        m_dictionaryVars.clear();
        m_dictionariesWithDefault.clear();
        m_sortEntsTables.clear();
        m_fieldLists.clear();
        m_dataTables.clear();
        m_dynamicBlockObjects.clear();
        m_fields.clear();
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

    void addRawDwgSection(const DRW_RawDwgSection& section) {
        RawDwgSectionRecord record;
        record.name = section.m_name;
        record.version = section.m_version;
        record.data = section.m_data;
        m_rawDwgSections.push_back(std::move(record));
    }

    //Slice A2: lossless DXF passthrough store (group-text records captured by the
    //DXF reader's processRawObject/processRawEntity). Kept on the graphic so a
    //read filter and a later write filter share them across a DXF round-trip.
    void addRawDxfObject(const DRW_RawDxfObject& object) {
        m_rawDxfObjects.push_back(object);
    }

    void addRawDxfEntity(const DRW_RawDxfObject& entity) {
        m_rawDxfEntities.push_back(entity);
    }

    void addDxfClass(const DRW_Class& cls) {
        m_dxfClasses.push_back(cls);
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
        upsertDocumentMapping(documentMappingFromView(record, -1));
    }

    void addUcs(const DRW_UCS& ucs) {
        UcsRecord record;
        record.handle = ucs.handle;
        record.parentHandle = ucs.parentHandle;
        record.name = ucs.name;
        record.origin = ucs.origin;
        record.xAxisDirection = ucs.xAxisDirection;
        record.yAxisDirection = ucs.yAxisDirection;
        record.orthoOrigin = ucs.orthoOrigin;
        record.elevation = ucs.elevation;
        record.orthoType = ucs.orthoType;
        m_ucsRecords.push_back(record);
        upsertDocumentMapping(documentMappingFromUcs(record, -1));
        refreshDocumentMappingUnresolvedReferenceCounts();
    }

    void addVport(const DRW_Vport& vport) {
        VportRecord record;
        record.handle = vport.handle;
        record.parentHandle = vport.parentHandle;
        record.name = vport.name;
        record.center = vport.center;
        record.viewTarget = vport.viewTarget;
        record.viewDirection = vport.viewDir;
        record.height = vport.height;
        record.ratio = vport.ratio;
        record.viewMode = vport.viewMode;
        record.grid = vport.grid;
        record.snap = vport.snap;
        record.gridBehavior = vport.gridBehavior;
        record.backgroundHandle = vport.backgroundHandle;
        record.visualStyleHandle = vport.visualStyleHandle;
        record.sunHandle = vport.m_sunHandle;
        record.namedUcsHandle = vport.namedUcsHandle;
        record.baseUcsHandle = vport.baseUcsHandle;
        m_vports.push_back(record);
        upsertDocumentMapping(documentMappingFromVport(record, -1));
    }

    void addVisualStyle(const DRW_VisualStyle& visualStyle) {
        VisualStyleRecord record;
        record.handle = visualStyle.handle;
        record.parentHandle = visualStyle.parentHandle;
        record.name = visualStyle.name;
        record.description = visualStyle.desc;
        record.type = visualStyle.type;
        m_visualStyles.push_back(record);
        refreshDocumentMappingUnresolvedReferenceCounts();
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
        refreshDocumentMappingUnresolvedReferenceCounts();
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
        record.extData = geometry.extData;
        m_modelerGeometry.push_back(std::move(record));
    }

    void addMeshPolyline(const DRW_Polyline& polyline,
                         bool fallbackPreviewGenerated) {
        MeshRecord record;
        record.handle = polyline.handle;
        record.parentHandle = polyline.parentHandle;
        record.recordName = "POLYLINE_MESH";
        record.vertexCount = polyline.vertexcount;
        record.faceCount = polyline.facecount;
        record.smoothM = polyline.smoothM;
        record.smoothN = polyline.smoothN;
        record.curveType = polyline.curvetype;
        record.flags = polyline.flags;
        record.preservedVertexCount = polyline.vertlist.size();
        record.fallbackPreviewGenerated = fallbackPreviewGenerated;
        record.rawRangeStatus = meshRawRangeStatus(
            record.vertexCount, record.faceCount, record.preservedVertexCount);
        m_meshes.push_back(std::move(record));
    }

    void addMeshRecord(MeshRecord record) {
        if (record.rawRangeStatus == MeshRawRangeStatus::Unknown) {
            record.rawRangeStatus = meshRawRangeStatus(
                record.vertexCount, record.faceCount,
                record.preservedVertexCount);
        }
        m_meshes.push_back(std::move(record));
    }

    void addMeshSidecar(MeshSidecarRecord record) {
        m_meshSidecars.push_back(std::move(record));
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
        for (ImageDefinitionRecord& imageDefinition : m_imageDefinitions) {
            if (imageDefinition.handle == record.parentHandle)
                imageDefinition.hasReactorHandle = true;
        }
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
        // PR 8d.1d: extended fields for round-trip encoding.
        record.boundaryPoints = filter.m_boundaryPoints;
        record.normal = filter.m_normal;
        record.origin = filter.m_origin;
        record.inverseInsertTransform = filter.m_inverseInsertTransform;
        record.insertTransform = filter.m_insertTransform;
        m_spatialFilters.push_back(std::move(record));
    }

    void addRasterVariables(const DRW_RasterVariables& rasterVariables) {
        RasterVariablesRecord record;
        record.handle = rasterVariables.handle;
        record.parentHandle = rasterVariables.parentHandle;
        record.classVersion = rasterVariables.m_classVersion;
        record.imageFrame = rasterVariables.m_imageFrame;
        record.imageQuality = rasterVariables.m_imageQuality;
        record.units = rasterVariables.m_units;
        m_rasterVariables.push_back(record);
    }

    void addImageDefinition(const DRW_ImageDef& imageDefinition) {
        ImageDefinitionRecord record;
        record.handle = imageDefinition.handle;
        record.parentHandle = imageDefinition.parentHandle;
        record.path = imageDefinition.name;
        record.classVersion = imageDefinition.imgVersion;
        record.imageSizeU = imageDefinition.u;
        record.imageSizeV = imageDefinition.v;
        record.pixelSizeU = imageDefinition.up;
        record.pixelSizeV = imageDefinition.vp;
        record.loaded = imageDefinition.loaded;
        record.resolution = imageDefinition.resolution;
        for (const auto& reactor : imageDefinition.reactors) {
            const std::uint32_t handle = parseHexHandle(reactor.first);
            if (handle != 0)
                record.reactorHandles.push_back(handle);
        }
        record.pathDiagnostic = externalReferencePathDiagnostic(record.path);
        record.hasReactorHandle = !record.reactorHandles.empty();
        m_imageDefinitions.push_back(record);
    }

    void addRasterImage(const DRW_Image& image, bool isWipeout) {
        RasterImageRecord record;
        record.handle = image.handle;
        record.parentHandle = image.parentHandle;
        record.isWipeout = isWipeout;
        record.definitionHandle = image.ref;
        record.basePoint = image.basePoint;
        record.uVector = image.secPoint;
        record.vVector = image.vVector;
        record.sizeU = image.sizeu;
        record.sizeV = image.sizev;
        record.clip = image.clip;
        record.brightness = image.brightness;
        record.contrast = image.contrast;
        record.fade = image.fade;
        record.clipMode = image.clipMode;
        record.clipVertexCount = image.clipPath.size();
        record.clipStatus = clipBoundaryStatus(
            image.clip, image.clipPath.size(), isWipeout);
        record.invertedClip = image.clipMode;
        record.frameVisible = image.clip != 0;
        record.hasDefinitionHandle = image.ref != 0;
        record.hasReactorHandle =
            image.ref != 0 && findImageDefinitionByHandle(image.ref) != nullptr;
        m_rasterImages.push_back(record);
    }

    void addUnderlayDefinition(const DRW_UnderlayDefinition& definition) {
        UnderlayDefinitionRecord record;
        record.handle = definition.handle;
        record.parentHandle = definition.parentHandle;
        record.kind = static_cast<int>(definition.kind);
        record.path = definition.filename;
        record.sheetName = definition.sheetName;
        record.pathDiagnostic = externalReferencePathDiagnostic(record.path);
        m_underlayDefinitions.push_back(record);
    }

    void addUnderlay(const DRW_Underlay& underlay,
                     bool fallbackPreviewGenerated) {
        UnderlayRecord record;
        record.handle = underlay.handle;
        record.parentHandle = underlay.parentHandle;
        record.definitionHandle = underlay.definitionHandle;
        record.kind = static_cast<int>(underlay.kind);
        record.position = underlay.position;
        record.scale = underlay.scale;
        record.rotation = underlay.rotation;
        record.flags = underlay.flags;
        record.contrast = underlay.contrast;
        record.fade = underlay.fade;
        record.clipVertexCount = underlay.clipBoundary.size();
        record.clipStatus = underlayClipBoundaryStatus(
            underlay.clipBoundary.size());
        record.frameVisible = (underlay.flags & 2u) != 0;
        record.fallbackPreviewGenerated = fallbackPreviewGenerated;
        m_underlays.push_back(record);
    }

    void addShape(const DRW_Shape& shape) {
        ShapeRecord record;
        record.handle = shape.handle;
        record.parentHandle = shape.parentHandle;
        record.shapeFileHandle = shape.m_shapeFileHandle;
        record.shapeIndex = shape.m_shapeIndex;
        record.styleName = shape.m_styleName;
        record.insertionPoint = shape.m_insertionPoint;
        record.extrusion = shape.m_extrusion;
        record.scale = shape.m_scale;
        record.rotation = shape.m_rotation;
        record.oblique = shape.m_oblique;
        record.widthFactor = shape.m_widthFactor;
        record.thickness = shape.m_thickness;
        record.rawByteCount = shape.m_rawBytes.size();
        record.rawRangeStatus =
            shape.m_rawBytes.empty() ? MeshRawRangeStatus::Missing
                                     : MeshRawRangeStatus::Complete;
        m_shapes.push_back(record);
    }

    void addOle2Frame(const DRW_Ole2Frame& ole2Frame) {
        Ole2FrameRecord record;
        record.handle = ole2Frame.handle;
        record.parentHandle = ole2Frame.parentHandle;
        record.flags = ole2Frame.m_flags;
        record.mode = ole2Frame.m_mode;
        record.declaredPayloadLength = ole2Frame.m_declaredPayloadLength;
        record.payloadByteCount = ole2Frame.m_payloadByteCount;
        record.payloadStartBit = ole2Frame.m_payloadStartBit;
        record.payloadPresent = ole2Frame.m_payloadPresent;
        record.payloadTruncated = ole2Frame.m_payloadTruncated;
        record.payloadTooLarge = ole2Frame.m_payloadTooLarge;
        record.hasR2000TrailingByte = ole2Frame.m_hasR2000TrailingByte;
        record.r2000TrailingByte = ole2Frame.m_r2000TrailingByte;
        record.rawByteCount = ole2Frame.m_rawBytes.size();
        record.oleVersion = ole2Frame.m_oleVersion;
        record.pt1 = ole2Frame.m_pt1;
        record.pt2 = ole2Frame.m_pt2;
        record.payloadBytes = ole2Frame.m_payloadBytes;
        m_ole2Frames.push_back(record);
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
        // PR 8d.1c: extended fields for round-trip encoding.
        record.designPoint = geoData.m_designPoint;
        record.referencePoint = geoData.m_referencePoint;
        record.upDirection = geoData.m_upDirection;
        record.northDirection = geoData.m_northDirection;
        record.scaleEstimationMethod = geoData.m_scaleEstimationMethod;
        record.userSpecifiedScaleFactor = geoData.m_userSpecifiedScaleFactor;
        record.enableSeaLevelCorrection = geoData.m_enableSeaLevelCorrection;
        record.seaLevelElevation = geoData.m_seaLevelElevation;
        record.coordinateProjectionRadius = geoData.m_coordinateProjectionRadius;
        record.observationFromTag = geoData.m_observationFromTag;
        record.observationToTag = geoData.m_observationToTag;
        record.observationCoverageTag = geoData.m_observationCoverageTag;
        record.meshPoints.reserve(geoData.m_points.size());
        for (const DRW_GeoMeshPoint& point : geoData.m_points) {
            GeoMeshPointRecord mp;
            mp.source = point.m_source;
            mp.destination = point.m_destination;
            record.meshPoints.push_back(mp);
        }
        record.meshFaces.reserve(geoData.m_faces.size());
        for (const DRW_GeoMeshFace& face : geoData.m_faces) {
            GeoMeshFaceRecord mf;
            mf.index1 = face.m_index1;
            mf.index2 = face.m_index2;
            mf.index3 = face.m_index3;
            record.meshFaces.push_back(mf);
        }
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

    void addDictionary(const DRW_Dictionary& dictionary) {
        DictionaryRecord record;
        record.handle = dictionary.handle;
        record.parentHandle = dictionary.parentHandle;
        record.cloning = dictionary.cloning;
        record.hardOwner = dictionary.hardOwner;
        record.name = dictionary.name;
        record.entries.reserve(dictionary.m_entries.size());
        for (const DRW_Dictionary::Entry& entry : dictionary.m_entries) {
            DictionaryEntryRecord er;
            er.name = entry.m_name;
            er.handle = entry.m_handle;
            record.entries.push_back(std::move(er));
        }
        m_dictionaries.push_back(std::move(record));
    }

    void addXRecord(const DRW_XRecord& xrecord) {
        XRecordRecord record;
        record.handle = xrecord.handle;
        record.parentHandle = xrecord.parentHandle;
        record.cloning = xrecord.m_cloning;
        record.values = xrecord.m_values;
        record.handleValues = xrecord.m_handleValues;
        m_xrecords.push_back(std::move(record));
    }

    void addLayout(const DRW_Layout& layout) {
        LayoutRecord record;
        record.handle = layout.handle;
        record.parentHandle = layout.parentHandle;
        // PlotSettings prefix.
        record.pageSetupName = layout.pageSetupName;
        record.printerConfig = layout.printerConfig;
        record.plotLayoutFlags = layout.plotLayoutFlags;
        record.marginLeft = layout.marginLeft;
        record.marginBottom = layout.marginBottom;
        record.marginRight = layout.marginRight;
        record.marginTop = layout.marginTop;
        record.paperWidth = layout.paperWidth;
        record.paperHeight = layout.paperHeight;
        record.paperSize = layout.paperSize;
        record.plotOriginX = layout.plotOriginX;
        record.plotOriginY = layout.plotOriginY;
        record.paperUnits = layout.paperUnits;
        record.plotRotation = layout.plotRotation;
        record.plotType = layout.plotType;
        record.windowMinX = layout.windowMinX;
        record.windowMinY = layout.windowMinY;
        record.windowMaxX = layout.windowMaxX;
        record.windowMaxY = layout.windowMaxY;
        record.plotViewName = layout.plotViewName;
        record.realWorldUnits = layout.realWorldUnits;
        record.drawingUnits = layout.drawingUnits;
        record.currentStyleSheet = layout.currentStyleSheet;
        record.scaleType = layout.scaleType;
        record.scaleFactor = layout.scaleFactor;
        record.paperImageOriginX = layout.paperImageOriginX;
        record.paperImageOriginY = layout.paperImageOriginY;
        record.shadePlotMode = layout.shadePlotMode;
        record.shadePlotResLevel = layout.shadePlotResLevel;
        record.shadePlotCustomDPI = layout.shadePlotCustomDPI;
        // Layout-specific.
        record.name = layout.name;
        record.layoutFlags = layout.layoutFlags;
        record.tabOrder = layout.tabOrder;
        record.ucsOrigin = layout.ucsOrigin;
        record.limMinX = layout.limMinX;
        record.limMinY = layout.limMinY;
        record.limMaxX = layout.limMaxX;
        record.limMaxY = layout.limMaxY;
        record.insPoint = layout.insPoint;
        record.ucsXAxis = layout.ucsXAxis;
        record.ucsYAxis = layout.ucsYAxis;
        record.elevation = layout.elevation;
        record.orthoViewType = layout.orthoViewType;
        record.extMin = layout.extMin;
        record.extMax = layout.extMax;
        record.viewportCount = layout.viewportCount;
        record.plotViewHandle = layout.plotViewHandle.ref;
        record.visualStyleHandle = layout.visualStyleHandle.ref;
        record.paperSpaceBlockRecordHandle = layout.paperSpaceBlockRecordHandle.ref;
        record.lastActiveViewportHandle = layout.lastActiveViewportHandle.ref;
        record.baseUcsHandle = layout.baseUcsHandle.ref;
        record.namedUcsHandle = layout.namedUcsHandle.ref;
        record.viewportHandles = layout.viewportHandles;
        m_layouts.push_back(std::move(record));
    }

    // PR 8d.2a — five small no-storage OBJECTS families.  Round-trip-grade
    // capture mirroring the DRW_X::encodeDwg field set.
    void addScale(const DRW_Scale& scale) {
        ScaleRecord record;
        record.handle = scale.handle;
        record.parentHandle = scale.parentHandle;
        record.name = scale.name;
        record.flag = scale.flag;
        record.paperUnits = scale.paperUnits;
        record.drawingUnits = scale.drawingUnits;
        record.isUnitScale = scale.isUnitScale;
        m_scales.push_back(std::move(record));
    }

    void addObjectContextData(const DRW_ObjectContextData& context) {
        ObjectContextDataRecord record;
        record.handle = context.handle;
        record.parentHandle = context.parentHandle;
        record.recordName = context.m_recordName;
        record.kind = context.m_kind;
        record.classVersion = context.m_classVersion;
        record.defaultFlag = context.m_defaultFlag;
        record.scaleHandle = context.m_scaleHandle;
        record.blockHandle = context.m_blockHandle;
        record.annotatedHandle = context.m_annotatedHandle;
        record.horizontalMode = context.m_horizontalMode;
        record.attachment = context.m_attachment;
        record.rotation = context.m_rotation;
        record.insertionPoint = context.m_insertionPoint;
        record.alignmentPoint = context.m_alignmentPoint;
        record.xAxisDir = context.m_xAxisDir;
        record.rectangleWidth = context.m_rectangleWidth;
        record.rectangleHeight = context.m_rectangleHeight;
        record.extentsWidth = context.m_extentsWidth;
        record.extentsHeight = context.m_extentsHeight;
        record.definitionPoint = context.m_definitionPoint;
        record.isDefaultTextLocation = context.m_isDefaultTextLocation;
        record.dimTofl = context.m_dimTofl;
        record.dimOsxd = context.m_dimOsxd;
        record.dimAtFit = context.m_dimAtFit;
        record.dimTix = context.m_dimTix;
        record.dimTMove = context.m_dimTMove;
        record.hasArrow2 = context.m_hasArrow2;
        record.flipArrow2 = context.m_flipArrow2;
        record.flipArrow1 = context.m_flipArrow1;
        record.textRotation = context.m_textRotation;
        record.overrideCode = context.m_overrideCode;
        record.featureLocationPoint = context.m_featureLocationPoint;
        record.leaderEndpoint = context.m_leaderEndpoint;
        m_objectContextData.push_back(std::move(record));
    }

    void addMLineStyle(const DRW_MLineStyle& style) {
        MLineStyleRecord record;
        record.handle = style.handle;
        record.parentHandle = style.parentHandle;
        record.name = style.name;
        record.flags = style.flags;
        record.description = style.description;
        record.fillColor = style.fillColor;
        record.startAngle = style.startAngle;
        record.endAngle = style.endAngle;
        record.elements.reserve(style.elements.size());
        for (const DRW_MLineElement& e : style.elements) {
            MLineStyleElementRecord er;
            er.offset = e.offset;
            er.color = e.color;
            er.linetype = e.linetype;
            record.elements.push_back(std::move(er));
        }
        m_mlineStyles.push_back(std::move(record));
    }

    void addWipeoutVariables(const DRW_WipeoutVariables& wv) {
        WipeoutVariablesRecord record;
        record.handle = wv.handle;
        record.parentHandle = wv.parentHandle;
        record.displayFrame = wv.m_displayFrame;
        m_wipeoutVariables.push_back(std::move(record));
    }

    void addIDBuffer(const DRW_IDBuffer& idBuffer) {
        IDBufferRecord record;
        record.handle = idBuffer.handle;
        record.parentHandle = idBuffer.parentHandle;
        record.classVersion = idBuffer.classVersion;
        record.objIds = idBuffer.objIds;
        m_idBuffers.push_back(std::move(record));
    }

    void addLayerIndex(const DRW_LayerIndex& layerIndex) {
        LayerIndexRecord record;
        record.handle = layerIndex.handle;
        record.parentHandle = layerIndex.parentHandle;
        record.timestamp1 = layerIndex.timestamp1;
        record.timestamp2 = layerIndex.timestamp2;
        record.entries.reserve(layerIndex.entries.size());
        for (const DRW_LayerIndexEntry& e : layerIndex.entries) {
            LayerIndexEntryRecord er;
            er.indexLong = e.indexLong;
            er.name = e.name;
            er.entryHandle = e.entryHandle;
            record.entries.push_back(std::move(er));
        }
        m_layerIndexes.push_back(std::move(record));
    }

    void addSpatialIndex(const DRW_SpatialIndex& spatialIndex) {
        SpatialIndexRecord record;
        record.handle = spatialIndex.handle;
        record.parentHandle = spatialIndex.parentHandle;
        record.timestamp1 = spatialIndex.timestamp1;
        record.timestamp2 = spatialIndex.timestamp2;
        m_spatialIndexes.push_back(std::move(record));
    }

    void addDictionaryVar(const DRW_DictionaryVar& dictionaryVar) {
        DictionaryVarRecord record;
        record.handle = dictionaryVar.handle;
        record.parentHandle = dictionaryVar.parentHandle;
        record.schema = dictionaryVar.m_schema;
        record.value = dictionaryVar.m_value;
        m_dictionaryVars.push_back(std::move(record));
    }

    // PR 8d.2b — four larger no-storage OBJECTS families.
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& dictionary) {
        DictionaryWithDefaultRecord record;
        record.handle = dictionary.handle;
        record.parentHandle = dictionary.parentHandle;
        record.cloning = dictionary.cloning;
        record.hardOwner = dictionary.hardOwner;
        record.name = dictionary.name;
        record.entries.reserve(dictionary.m_entries.size());
        for (const auto& e : dictionary.m_entries) {
            DictionaryWithDefaultEntryRecord er;
            er.name = e.m_name;
            er.handle = e.m_handle;
            record.entries.push_back(std::move(er));
        }
        record.defaultEntryHandle = dictionary.m_defaultEntryHandle;
        m_dictionariesWithDefault.push_back(std::move(record));
    }

    void addSortEntsTable(const DRW_SortEntsTable& sortEntsTable) {
        SortEntsTableRecord record;
        record.handle = sortEntsTable.handle;
        record.parentHandle = sortEntsTable.parentHandle;
        record.sortHandles = sortEntsTable.m_sortHandles;
        record.blockOwnerHandle = sortEntsTable.m_blockOwnerHandle;
        record.entityHandles = sortEntsTable.m_entityHandles;
        m_sortEntsTables.push_back(std::move(record));
    }

    void addFieldList(const DRW_FieldList& fieldList) {
        FieldListRecord record;
        record.handle = fieldList.handle;
        record.parentHandle = fieldList.parentHandle;
        record.unknown = fieldList.m_unknown;
        record.fieldHandles = fieldList.m_fieldHandles;
        m_fieldLists.push_back(std::move(record));
    }

    void addDataTable(const DRW_DataTable& dataTable) {
        DataTableRecord record;
        record.handle = dataTable.handle;
        record.parentHandle = dataTable.parentHandle;
        record.flags = dataTable.flags;
        record.columnCount = dataTable.columnCount;
        record.rowCount = dataTable.rowCount;
        record.tableName = dataTable.tableName;
        record.columns.reserve(dataTable.columns.size());
        for (const auto& col : dataTable.columns) {
            DataTableColumnRecord cr;
            cr.type = col.type;
            cr.text = col.text;
            cr.rows.reserve(col.rows.size());
            for (const auto& row : col.rows) {
                DataTableRowRecord rr;
                rr.dataLong = row.dataLong;
                rr.dataDouble = row.dataDouble;
                rr.dataString = row.dataString;
                cr.rows.push_back(std::move(rr));
            }
            record.columns.push_back(std::move(cr));
        }
        m_dataTables.push_back(std::move(record));
    }

    void addDynamicBlockObject(const DRW_DynamicBlockObject& obj) {
        DynamicBlockRecord record;
        record.handle = obj.handle;
        record.parentHandle = obj.parentHandle;
        record.recordName = obj.m_recordName;
        record.kind = static_cast<int>(obj.m_kind);
        record.evalExprParsed = obj.m_evalExprParsed;
        record.parentId = obj.m_parentId;
        record.major = obj.m_major;
        record.minor = obj.m_minor;
        record.valueCode = obj.m_valueCode;
        record.nodeId = obj.m_nodeId;
        record.elementParsed = obj.m_elementParsed;
        record.elementName = obj.m_elementName;
        record.eed1071 = obj.m_eed1071;
        record.showProperties = obj.m_showProperties;
        record.chainActions = obj.m_chainActions;
        record.gripType = obj.m_gripType;
        record.gripExpr = obj.m_gripExpr;
        record.isInitialized = obj.m_isInitialized;
        record.visibilityName = obj.m_visibilityName;
        record.visibilityDescription = obj.m_visibilityDescription;
        record.blockCount = obj.m_blockCount;
        record.stateCount = obj.m_stateCount;
        record.stateNames.reserve(obj.m_stateNames.size());
        for (const auto& n : obj.m_stateNames)
            record.stateNames.push_back(n);
        record.dependencyCount = obj.m_dependencyCount;
        record.actionCount = obj.m_actionCount;
        record.actionOffsetX = obj.m_actionOffsetX;
        record.actionOffsetY = obj.m_actionOffsetY;
        record.angleOffset = obj.m_angleOffset;
        record.connectionNames.reserve(obj.m_connectionNames.size());
        for (const auto& n : obj.m_connectionNames)
            record.connectionNames.push_back(n);
        record.bodyFullyDecoded = obj.m_bodyFullyDecoded;
        m_dynamicBlockObjects.push_back(std::move(record));
    }

    static CadValueRecord cadValueFromDrw(const DRW_CadValue& v) {
        CadValueRecord c;
        c.formatFlags = v.m_formatFlags;
        c.dataType = v.m_dataType;
        c.dataSize = v.m_dataSize;
        c.unitType = v.m_unitType;
        c.value = v.m_value;
        c.formatString = v.m_formatString;
        c.valueString = v.m_valueString;
        c.handle = v.m_handle;
        c.rawData = v.m_rawData;
        return c;
    }

    void addField(const DRW_Field& field) {
        FieldRecord record;
        record.handle = field.handle;
        record.parentHandle = field.parentHandle;
        record.evaluatorId = field.m_evaluatorId;
        record.fieldCode = field.m_fieldCode;
        record.formatString = field.m_formatString;
        record.evaluationOptionFlags = field.m_evaluationOptionFlags;
        record.filingOptionFlags = field.m_filingOptionFlags;
        record.fieldStateFlags = field.m_fieldStateFlags;
        record.evaluationStatusFlags = field.m_evaluationStatusFlags;
        record.evaluationErrorCode = field.m_evaluationErrorCode;
        record.evaluationErrorMessage = field.m_evaluationErrorMessage;
        record.value = cadValueFromDrw(field.m_value);
        record.valueString = field.m_valueString;
        record.valueStringLength = field.m_valueStringLength;
        record.childHandles = field.m_childHandles;
        record.objectHandles = field.m_objectHandles;
        record.childValues.reserve(field.m_childValues.size());
        for (const auto& cv : field.m_childValues) {
            FieldChildValueRecord cvr;
            cvr.key = cv.m_key;
            cvr.value = cadValueFromDrw(cv.m_value);
            record.childValues.push_back(std::move(cvr));
        }
        m_fields.push_back(std::move(record));
    }

    const std::vector<RawObjectRecord>& rawObjects() const { return m_rawObjects; }
    const std::vector<RawDwgSectionRecord>& rawDwgSections() const {
        return m_rawDwgSections;
    }
    const std::vector<DRW_RawDxfObject>& rawDxfObjects() const { return m_rawDxfObjects; }
    const std::vector<DRW_RawDxfObject>& rawDxfEntities() const { return m_rawDxfEntities; }
    const std::vector<DRW_Class>& dxfClasses() const { return m_dxfClasses; }

    // Source DWG version of the document this metadata was read from. Used to
    // gate raw-replay (both emit + CLASSES registration) on source==target.
    void setSourceDwgVersion(DRW::Version v) { m_sourceDwgVersion = v; }
    DRW::Version sourceDwgVersion() const { return m_sourceDwgVersion; }
    const std::vector<ViewRecord>& views() const { return m_views; }
    const std::vector<UcsRecord>& ucsRecords() const { return m_ucsRecords; }
    const std::vector<VportRecord>& vports() const { return m_vports; }
    const std::vector<DocumentMappingRecord>& documentMappings() const {
        return m_documentMappings;
    }
    const std::vector<VisualStyleRecord>& visualStyles() const {
        return m_visualStyles;
    }
    const std::vector<LightRecord>& lights() const { return m_lights; }
    const std::vector<SunRecord>& suns() const { return m_suns; }
    const std::vector<ModelerGeometryRecord>& modelerGeometry() const { return m_modelerGeometry; }
    const std::vector<MeshRecord>& meshes() const { return m_meshes; }
    const std::vector<MeshSidecarRecord>& meshSidecars() const { return m_meshSidecars; }
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
    const std::vector<RasterImageRecord>& rasterImages() const { return m_rasterImages; }
    const std::vector<ImageDefinitionRecord>& imageDefinitions() const {
        return m_imageDefinitions;
    }
    const std::vector<UnderlayRecord>& underlays() const { return m_underlays; }
    const std::vector<UnderlayDefinitionRecord>& underlayDefinitions() const {
        return m_underlayDefinitions;
    }
    const std::vector<RasterVariablesRecord>& rasterVariables() const {
        return m_rasterVariables;
    }
    const std::vector<ShapeRecord>& shapes() const { return m_shapes; }
    const std::vector<Ole2FrameRecord>& ole2Frames() const { return m_ole2Frames; }
    const std::vector<SpatialFilterRecord>& spatialFilters() const { return m_spatialFilters; }
    const std::vector<GeoDataRecord>& geoData() const { return m_geoData; }
    const std::vector<TableGeometryRecord>& tableGeometry() const { return m_tableGeometry; }
    const std::vector<PlaceholderRecord>& placeholders() const { return m_placeholders; }
    const std::vector<DictionaryRecord>& dictionaries() const { return m_dictionaries; }
    const std::vector<XRecordRecord>& xrecords() const { return m_xrecords; }
    const std::vector<LayoutRecord>& layouts() const { return m_layouts; }
    /** Non-const overload — UI-facing mutation hook (PR 9
     *  RS_Graphic::setLayoutMargins).  Direct callers must bump the
     *  modified flag themselves; prefer the RS_Graphic setters. */
    std::vector<LayoutRecord>& layouts() { return m_layouts; }
    // PR 8d.2a — five small no-storage OBJECTS families.
    const std::vector<ScaleRecord>& scales() const { return m_scales; }
    const std::vector<ObjectContextDataRecord>& objectContextData() const {
        return m_objectContextData;
    }
    const std::vector<MLineStyleRecord>& mlineStyles() const { return m_mlineStyles; }
    const std::vector<WipeoutVariablesRecord>& wipeoutVariables() const { return m_wipeoutVariables; }
    const std::vector<IDBufferRecord>& idBuffers() const { return m_idBuffers; }
    const std::vector<LayerIndexRecord>& layerIndexes() const { return m_layerIndexes; }
    const std::vector<SpatialIndexRecord>& spatialIndexes() const { return m_spatialIndexes; }
    const std::vector<DictionaryVarRecord>& dictionaryVars() const { return m_dictionaryVars; }
    // PR 8d.2b — four larger no-storage OBJECTS families.
    const std::vector<DictionaryWithDefaultRecord>& dictionariesWithDefault() const {
        return m_dictionariesWithDefault;
    }
    const std::vector<SortEntsTableRecord>& sortEntsTables() const { return m_sortEntsTables; }
    const std::vector<FieldListRecord>& fieldLists() const { return m_fieldLists; }
    const std::vector<DataTableRecord>& dataTables() const { return m_dataTables; }
    const std::vector<DynamicBlockRecord>& dynamicBlockObjects() const { return m_dynamicBlockObjects; }
    const std::vector<FieldRecord>& fields() const { return m_fields; }

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

    const TableRecord* findTableByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const TableRecord& record : m_tables) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const TableRecord* findTableStyleByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record) && record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const TableRecord*> findTableStylesReferencingHandle(
        std::uint32_t handle) const {
        std::vector<const TableRecord*> result;
        if (handle == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (isTableStyleRecord(record) && tableRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableRecord*> findTablesUsingStyle(std::uint32_t styleHandle) const {
        std::vector<const TableRecord*> result;
        if (styleHandle == 0)
            return result;
        for (const TableRecord& record : m_tables) {
            if (!isTableStyleRecord(record) && record.tableStyleHandle == styleHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const TableRecord*> findTablesReferencingHandle(std::uint32_t handle) const {
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
        std::uint32_t tableHandle) const {
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
    const CellStyleMapRecord* findCellStyleMapByHandle(std::uint32_t handle) const {
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
        std::uint32_t handle) const {
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
        std::uint32_t handle, DRW::Version version) const {
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
    MeshWriterBlockerCounts meshWriterBlockerCounts() const {
        MeshWriterBlockerCounts counts;
        counts.sidecarCount = m_meshSidecars.size();
        for (const MeshRecord& record : m_meshes) {
            ++counts.meshCount;
            if (record.rawRangeStatus == MeshRawRangeStatus::Complete)
                ++counts.completeRawRange;
            else if (record.rawRangeStatus == MeshRawRangeStatus::Missing)
                ++counts.missingRawRange;
            else if (record.rawRangeStatus == MeshRawRangeStatus::Incomplete)
                ++counts.incompleteRawRange;
            if (meshHasMalformedCounts(record))
                ++counts.malformedCountRelationships;
            if (record.parentHandle == 0
                || (record.isSubDMesh && record.recordName.empty())) {
                ++counts.missingOwnerOrClassHandle;
            }
            if (record.isSubDMesh || record.subdivisionLevel > 0)
                ++counts.unsupportedSubdivisionData;
            if ((record.edgeCount > 0 || record.creaseCount > 0
                 || record.subdivisionLevel > 0) && !record.hasCreaseData) {
                ++counts.missingCreaseData;
            }
            if (record.fallbackPreviewGenerated)
                ++counts.fallbackOnlyPreview;
            if (record.fallbackInvalidated)
                ++counts.editedFallback;
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
        }
        for (const MeshSidecarRecord& sidecar : m_meshSidecars) {
            if (sidecar.editedFallback)
                ++counts.editedFallback;
            if (sidecar.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            if (sidecar.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
        }
        return counts;
    }
    ExternalReferenceCounts externalReferenceCounts() const {
        ExternalReferenceCounts counts;
        counts.imageDefinitions = m_imageDefinitions.size();
        counts.underlayDefinitions = m_underlayDefinitions.size();
        counts.rasterVariables = m_rasterVariables.size();
        for (const RasterImageRecord& record : m_rasterImages) {
            if (record.isWipeout)
                ++counts.wipeouts;
            else
                ++counts.imageEntities;
            if (record.definitionHandle == 0)
                ++counts.missingDefinitionHandles;
            incrementClipBoundaryCounts(counts, record.clipStatus);
            if (record.invertedClip)
                ++counts.invertedClips;
            if (!record.frameVisible)
                ++counts.hiddenFrames;
        }
        for (const UnderlayRecord& record : m_underlays) {
            ++counts.underlays;
            if (record.definitionHandle == 0)
                ++counts.missingDefinitionHandles;
            incrementClipBoundaryCounts(counts, record.clipStatus);
            if (!record.frameVisible)
                ++counts.hiddenFrames;
        }
        for (const ImageDefinitionRecord& record : m_imageDefinitions) {
            incrementPathStatusCounts(counts, record.pathDiagnostic);
            if (findRasterImagesByDefinitionHandle(record.handle).empty())
                ++counts.definitionsWithoutEntities;
        }
        for (const UnderlayDefinitionRecord& record : m_underlayDefinitions) {
            incrementPathStatusCounts(counts, record.pathDiagnostic);
            if (findUnderlaysByDefinitionHandle(record.handle).empty())
                ++counts.definitionsWithoutEntities;
        }
        return counts;
    }
    ShapeOleWriterBlockerCounts shapeOleWriterBlockerCounts() const {
        ShapeOleWriterBlockerCounts counts;
        counts.shapeCount = m_shapes.size();
        counts.ole2FrameCount = m_ole2Frames.size();
        for (const ShapeRecord& record : m_shapes) {
            if (record.shapeFileHandle == 0)
                ++counts.missingStyleHandle;
            if (record.rawRangeStatus == MeshRawRangeStatus::Missing)
                ++counts.missingRawRange;
            else if (record.rawRangeStatus == MeshRawRangeStatus::Incomplete)
                ++counts.incompleteRawRange;
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            else if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
        }
        for (const Ole2FrameRecord& record : m_ole2Frames) {
            if (!record.payloadPresent || record.declaredPayloadLength == 0)
                ++counts.missingOlePayload;
            if (record.payloadTruncated)
                ++counts.truncatedOlePayload;
            if (record.payloadTooLarge)
                ++counts.oversizedOlePayload;
            if (record.previewFrameInvalidated)
                ++counts.editedPreviewFrame;
            ++counts.unsupportedOlePayloadRegeneration;
            if (record.rawByteCount == 0)
                ++counts.missingRawRange;
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            else if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
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
    GraphReplayPolicyCounts graphReplayPolicyCounts() const {
        GraphReplayPolicyCounts counts;
        for (const RawObjectRecord& record : m_rawObjects) {
            const GraphReplayFamily family = graphReplayFamilyFromRawObject(record);
            const ReplayBlocker blocker = rawReplayBlocker(record);
            if (blocker == ReplayBlocker::None) {
                incrementGraphReplayFamilyCount(counts.preserved, family);
                continue;
            }
            incrementGraphReplayFamilyCount(counts.suppressed, family);
            incrementGraphReplayBlockerCounts(counts, blocker);
        }
        for (const AssociativeRecord& record : m_associativeObjects) {
            if (record.handle == 0
                || !hasRawGraphObjectForHandle(
                    record.handle, GraphReplayFamily::AcDbAssoc)) {
                ++counts.semanticOnlyAssociative;
            }
        }
        for (const AcShRecord& record : m_acshObjects) {
            if (record.handle == 0
                || !hasRawGraphObjectForHandle(
                    record.handle, GraphReplayFamily::AcShHistory)) {
                ++counts.semanticOnlyAcSh;
            }
        }
        const AssociativePrefixCounts prefixCounts = associativePrefixCounts();
        counts.parserPartial = prefixCounts.partial + prefixCounts.missing
            + prefixCounts.unsupportedVersion + prefixCounts.boundedCountOverflow;
        return counts;
    }
    std::vector<AdvancedEntityWriterReadiness> advancedEntityWriterLedger(
        DRW::Version version) const {
        std::vector<AdvancedEntityWriterReadiness> result;
        for (const RawObjectRecord& rawRecord : m_rawObjects) {
            AdvancedEntityWriterReadiness record =
                advancedEntityWriterReadinessFromRawObject(rawRecord, version);
            if (record.family != AdvancedEntityWriterFamily::Unknown
                || rawRecord.isEntity) {
                result.push_back(std::move(record));
            }
        }
        for (const MLeaderRecord& mleader : m_mleaders)
            result.push_back(advancedEntityWriterReadinessFromMLeader(mleader, version));
        for (const MeshRecord& mesh : m_meshes)
            result.push_back(advancedEntityWriterReadinessFromMesh(mesh, version));
        for (const ShapeRecord& shape : m_shapes)
            result.push_back(advancedEntityWriterReadinessFromShape(shape, version));
        for (const Ole2FrameRecord& ole2Frame : m_ole2Frames)
            result.push_back(
                advancedEntityWriterReadinessFromOle2Frame(ole2Frame, version));
        return result;
    }

    AdvancedEntityWriterBlockerCounts advancedEntityWriterBlockerCounts(
        DRW::Version version) const {
        AdvancedEntityWriterBlockerCounts counts;
        for (const AdvancedEntityWriterReadiness& record :
             advancedEntityWriterLedger(version)) {
            ++counts.recordCount;
            incrementAdvancedEntityWriterFamilyCount(counts, record.family);
            incrementAdvancedEntityWriterCoverageCount(counts, record.odaCoverage);
            if (record.nativeWriterAvailable)
                ++counts.nativeWriterAvailable;
            if (record.rawReplayAvailable)
                ++counts.rawReplayAvailable;
            if (record.fallbackAvailable)
                ++counts.fallbackAvailable;
            if (record.editedFallbackInvalidated)
                ++counts.editedFallbackInvalidated;
            if (record.missingRequiredMetadata)
                ++counts.missingRequiredMetadata;
            if (record.missingPayloadBytes)
                ++counts.missingPayloadBytes;
            if (record.unsupportedAdvancedContent)
                ++counts.unsupportedAdvancedContent;
        }
        return counts;
    }
    const TableCellRecord* findTableCell(std::uint32_t tableHandle, int row, int column) const {
        const TableRecord* table = findTableByHandle(tableHandle);
        if (table == nullptr || row < 0 || column < 0)
            return nullptr;
        for (const TableCellRecord& cell : table->cells) {
            if (cell.row == row && cell.column == column)
                return &cell;
        }
        return nullptr;
    }
    std::vector<const TableCellRecord*> findTableCellsReferencingHandle(std::uint32_t handle) const {
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

    const MLeaderRecord* findMLeaderByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const MLeaderRecord& record : m_mleaders) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const MLeaderStyleRecord* findMLeaderStyleByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const MLeaderStyleRecord& record : m_mleaderStyles) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const MLeaderRecord*> findMLeadersUsingStyle(std::uint32_t styleHandle) const {
        std::vector<const MLeaderRecord*> result;
        if (styleHandle == 0)
            return result;
        for (const MLeaderRecord& record : m_mleaders) {
            if (record.styleHandle == styleHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const MLeaderRecord*> findMLeadersReferencingHandle(std::uint32_t handle) const {
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
        std::uint32_t handle) const {
        std::vector<const MLeaderStyleRecord*> result;
        if (handle == 0)
            return result;
        for (const MLeaderStyleRecord& record : m_mleaderStyles) {
            if (mleaderStyleRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }

    const ModelerGeometryRecord* findModelerGeometryByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const ModelerGeometryRecord& record : m_modelerGeometry) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const ModelerGeometryRecord*> findModelerGeometryByHistoryHandle(
        std::uint32_t historyHandle) const {
        std::vector<const ModelerGeometryRecord*> result;
        if (historyHandle == 0)
            return result;
        for (const ModelerGeometryRecord& record : m_modelerGeometry) {
            if (record.historyHandle == historyHandle)
                result.push_back(&record);
        }
        return result;
    }

    const MeshRecord* findMeshByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const MeshRecord& record : m_meshes) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    std::vector<const MeshSidecarRecord*> findMeshSidecarsBySourceHandle(
        std::uint32_t sourceHandle) const {
        std::vector<const MeshSidecarRecord*> result;
        if (sourceHandle == 0)
            return result;
        for (const MeshSidecarRecord& record : m_meshSidecars) {
            if (record.sourceHandle == sourceHandle)
                result.push_back(&record);
        }
        return result;
    }

    const MeshSidecarRecord* findMeshSidecarByFallbackEntityId(
        unsigned long long fallbackEntityId) const {
        if (fallbackEntityId == 0)
            return nullptr;
        for (const MeshSidecarRecord& record : m_meshSidecars) {
            if (record.fallbackEntityId == fallbackEntityId)
                return &record;
        }
        return nullptr;
    }

    const RasterImageRecord* findRasterImageByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const RasterImageRecord& record : m_rasterImages) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    const ImageDefinitionRecord* findImageDefinitionByHandle(
        std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const ImageDefinitionRecord& record : m_imageDefinitions) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    std::vector<const RasterImageRecord*> findRasterImagesByDefinitionHandle(
        std::uint32_t definitionHandle) const {
        std::vector<const RasterImageRecord*> result;
        if (definitionHandle == 0)
            return result;
        for (const RasterImageRecord& record : m_rasterImages) {
            if (record.definitionHandle == definitionHandle)
                result.push_back(&record);
        }
        return result;
    }

    std::vector<const ImageDefinitionReactorRecord*>
    findImageDefinitionReactorsByDefinitionHandle(std::uint32_t definitionHandle) const {
        std::vector<const ImageDefinitionReactorRecord*> result;
        if (definitionHandle == 0)
            return result;
        for (const ImageDefinitionReactorRecord& record :
             m_imageDefinitionReactors) {
            if (record.parentHandle == definitionHandle)
                result.push_back(&record);
        }
        return result;
    }

    const UnderlayDefinitionRecord* findUnderlayDefinitionByHandle(
        std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const UnderlayDefinitionRecord& record : m_underlayDefinitions) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    std::vector<const UnderlayRecord*> findUnderlaysByDefinitionHandle(
        std::uint32_t definitionHandle) const {
        std::vector<const UnderlayRecord*> result;
        if (definitionHandle == 0)
            return result;
        for (const UnderlayRecord& record : m_underlays) {
            if (record.definitionHandle == definitionHandle)
                result.push_back(&record);
        }
        return result;
    }

    std::vector<const ImageDefinitionRecord*> findImageDefinitionsByPath(
        const std::string& path) const {
        std::vector<const ImageDefinitionRecord*> result;
        if (path.empty())
            return result;
        for (const ImageDefinitionRecord& record : m_imageDefinitions) {
            if (record.path == path)
                result.push_back(&record);
        }
        return result;
    }

    std::vector<const UnderlayDefinitionRecord*> findUnderlayDefinitionsByPath(
        const std::string& path) const {
        std::vector<const UnderlayDefinitionRecord*> result;
        if (path.empty())
            return result;
        for (const UnderlayDefinitionRecord& record : m_underlayDefinitions) {
            if (record.path == path)
                result.push_back(&record);
        }
        return result;
    }

    const ShapeRecord* findShapeByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const ShapeRecord& record : m_shapes) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    std::vector<const ShapeRecord*> findShapesByShapeFileHandle(
        std::uint32_t shapeFileHandle) const {
        std::vector<const ShapeRecord*> result;
        if (shapeFileHandle == 0)
            return result;
        for (const ShapeRecord& record : m_shapes) {
            if (record.shapeFileHandle == shapeFileHandle)
                result.push_back(&record);
        }
        return result;
    }

    const Ole2FrameRecord* findOle2FrameByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const Ole2FrameRecord& record : m_ole2Frames) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }

    const AssociativeRecord* findAssociativeObjectByHandle(std::uint32_t handle) const {
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
        std::uint32_t handle) const {
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
        std::uint32_t handle) const {
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
        std::uint32_t handle) const {
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
        std::uint32_t handle) const {
        std::vector<const AssociativeRecord*> result;
        for (std::uint32_t sourceHandle : findAssociativeClosureFrom(handle, 32u)) {
            if (const AssociativeRecord* record =
                    findAssociativeObjectByHandle(sourceHandle)) {
                result.push_back(record);
            }
        }
        return result;
    }
    std::vector<std::uint32_t> findAssociativeClosureFrom(
        std::uint32_t handle, size_t maxDepth) const {
        std::vector<std::uint32_t> result;
        if (handle == 0)
            return result;
        std::vector<std::uint32_t> visited;
        std::vector<std::pair<std::uint32_t, size_t>> queue;
        visited.push_back(handle);
        queue.push_back({handle, 0u});
        for (size_t index = 0; index < queue.size(); ++index) {
            const std::uint32_t targetHandle = queue[index].first;
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

    const AcShRecord* findAcShObjectByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const AcShRecord& record : m_acshObjects) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const AcShRecord*> findAcShObjectsByOwnerHandle(std::uint32_t ownerHandle) const {
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
        std::uint32_t bodyBitSize, size_t rawByteCount) {
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
        const std::vector<std::uint8_t>& bytes) {
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

    static ModelerPayloadKind classifyModelerPayload(const std::vector<std::uint8_t>& bytes) {
        return scanModelerPayloadMarker(bytes).kind;
    }

    static std::vector<ModelerPayloadRangeRecord> scanModelerPayloadRanges(
        const std::vector<std::uint8_t>& bytes, std::uint32_t bodyBitSize,
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
            const std::vector<std::uint8_t> terminator =
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
        const std::vector<std::uint8_t>& bytes, const char* marker,
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
                if (bytes[offset + index] != static_cast<std::uint8_t>(marker[index])) {
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
        const std::vector<std::uint8_t>& bytes, size_t start, size_t end) {
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
        const std::vector<std::uint8_t>& bytes, const char* marker,
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
                    static_cast<std::uint8_t>(marker[index])) {
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

    static std::vector<std::uint8_t> sabTerminatorBytes() {
        return {'E', 'n', 'd', 0x0Eu, 0x02u, 'o', 'f', 0x0Eu, 0x04u,
                'A', 'C', 'I', 'S', 0x0Du, 0x04u, 'd', 'a', 't', 'a'};
    }

    static size_t findByteSequence(const std::vector<std::uint8_t>& bytes,
                                   const std::vector<std::uint8_t>& marker,
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
    const ViewRecord* findViewByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const ViewRecord& record : m_views) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const UcsRecord* findUcsByName(const std::string& name) const {
        for (const UcsRecord& record : m_ucsRecords) {
            if (record.name == name)
                return &record;
        }
        return nullptr;
    }
    const UcsRecord* findUcsByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const UcsRecord& record : m_ucsRecords) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const VportRecord* findVportByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const VportRecord& record : m_vports) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    const VisualStyleRecord* findVisualStyleByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const VisualStyleRecord& record : m_visualStyles) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const ViewRecord*> findViewsReferencingHandle(std::uint32_t handle) const {
        std::vector<const ViewRecord*> result;
        if (handle == 0)
            return result;
        for (const ViewRecord& record : m_views) {
            if (viewRecordReferences(record, handle))
                result.push_back(&record);
        }
        return result;
    }
    const DocumentMappingRecord* findDocumentMappingBySourceHandle(
        std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceHandle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const DocumentMappingRecord*> findDocumentMappingsByName(
        DocumentMappingSource sourceType, const std::string& name) const {
        std::vector<const DocumentMappingRecord*> result;
        for (const DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceType == sourceType
                && (record.documentItemName == name || record.sourceName == name)) {
                result.push_back(&record);
            }
        }
        return result;
    }
    std::vector<const DocumentMappingRecord*> findDocumentMappingsByOwner(
        std::uint32_t ownerHandle) const {
        std::vector<const DocumentMappingRecord*> result;
        if (ownerHandle == 0)
            return result;
        for (const DocumentMappingRecord& record : m_documentMappings) {
            if (record.ownerHandle == ownerHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const DocumentMappingRecord*> findDocumentMappingsByLayout(
        std::uint32_t layoutHandle) const {
        std::vector<const DocumentMappingRecord*> result;
        if (layoutHandle == 0)
            return result;
        for (const DocumentMappingRecord& record : m_documentMappings) {
            if (record.layoutHandle == layoutHandle)
                result.push_back(&record);
        }
        return result;
    }
    void mapViewToDocumentItem(
        std::uint32_t handle, const std::string& name, int documentItemIndex) {
        mapDocumentItem(
            DocumentMappingSource::View, handle, name, documentItemIndex);
    }
    void mapUcsToDocumentItem(
        std::uint32_t handle, const std::string& name, int documentItemIndex) {
        mapDocumentItem(
            DocumentMappingSource::Ucs, handle, name, documentItemIndex);
    }
    bool invalidateDocumentMappingByItem(
        DocumentMappingSource sourceType, const std::string& name) {
        bool invalidated = false;
        for (DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceType != sourceType)
                continue;
            if (record.documentItemName != name && record.sourceName != name)
                continue;
            if (record.replayState == ReplayState::ReplayAllowed) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidated = true;
            }
            invalidateByHandle(record.sourceHandle);
        }
        return invalidated;
    }
    DocumentMappingCounts documentMappingCounts() const {
        DocumentMappingCounts counts;
        for (const DocumentMappingRecord& record : m_documentMappings) {
            switch (record.sourceType) {
                case DocumentMappingSource::View:
                    ++counts.viewMappings;
                    break;
                case DocumentMappingSource::Ucs:
                    ++counts.ucsMappings;
                    break;
                case DocumentMappingSource::Vport:
                    ++counts.vportMappings;
                    break;
            }
            if (record.isMappedToDocumentItem())
                ++counts.mappedDocumentItems;
            counts.unresolvedReferences += record.unresolvedReferenceCount;
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
        }
        return counts;
    }
    std::vector<VisualMetadataSummaryRecord> visualMetadataSummaries() const {
        std::vector<VisualMetadataSummaryRecord> result;
        result.reserve(m_views.size() + m_vports.size() + m_visualStyles.size()
                       + m_lights.size() + m_suns.size());
        for (const ViewRecord& record : m_views)
            result.push_back(visualSummaryFromView(record));
        for (const VportRecord& record : m_vports)
            result.push_back(visualSummaryFromVport(record));
        for (const VisualStyleRecord& record : m_visualStyles)
            result.push_back(visualSummaryFromVisualStyle(record));
        for (const LightRecord& record : m_lights)
            result.push_back(visualSummaryFromLight(record));
        for (const SunRecord& record : m_suns)
            result.push_back(visualSummaryFromSun(record));
        return result;
    }
    VisualMetadataSummaryCounts visualMetadataSummaryCounts() const {
        VisualMetadataSummaryCounts counts;
        for (const VisualMetadataSummaryRecord& record :
             visualMetadataSummaries()) {
            incrementVisualMetadataSourceCount(counts, record.sourceType);
            incrementVisualMetadataSpecCoverageCount(
                counts, record.specCoverage);
            if (record.replayState == ReplayState::ReplayInvalidated)
                ++counts.invalidated;
            if (record.replayState == ReplayState::ReplayReplaced)
                ++counts.replaced;
            if (record.ownerHandle != 0)
                ++counts.ownerMapped;
            if (record.layoutHandle != 0)
                ++counts.layoutMapped;
            counts.unresolvedReferences += record.unresolvedReferenceCount;
        }
        return counts;
    }
    std::vector<const VisualMetadataSummaryRecord*> findVisualSummariesByOwner(
        const std::vector<VisualMetadataSummaryRecord>& summaries,
        std::uint32_t ownerHandle) const {
        std::vector<const VisualMetadataSummaryRecord*> result;
        if (ownerHandle == 0)
            return result;
        for (const VisualMetadataSummaryRecord& record : summaries) {
            if (record.ownerHandle == ownerHandle)
                result.push_back(&record);
        }
        return result;
    }
    std::vector<const VisualMetadataSummaryRecord*> findVisualSummariesByLayout(
        const std::vector<VisualMetadataSummaryRecord>& summaries,
        std::uint32_t layoutHandle) const {
        std::vector<const VisualMetadataSummaryRecord*> result;
        if (layoutHandle == 0)
            return result;
        for (const VisualMetadataSummaryRecord& record : summaries) {
            if (record.layoutHandle == layoutHandle)
                result.push_back(&record);
        }
        return result;
    }
    VisualMetadataWriterBlockerCounts visualMetadataWriterBlockerCounts(
        DRW::Version version) const {
        VisualMetadataWriterBlockerCounts counts;
        const std::vector<VisualMetadataSummaryRecord> summaries =
            visualMetadataSummaries();
        counts.recordCount = summaries.size();
        for (const VisualMetadataSummaryRecord& summary : summaries) {
            if (summary.ownerHandle == 0 && summary.layoutHandle == 0)
                ++counts.missingOwnerOrLayout;
            if (summary.sourceType == VisualMetadataSource::VisualStyle) {
                const VisualMetadataReplayEligibility eligibility =
                    visualMetadataReplayEligibility(summary.handle, version);
                if (eligibility.unsupportedNativeWriter)
                    ++counts.unsupportedVisualStyleWriter;
            }
        }
        for (const ViewRecord& record : m_views)
            accumulateVisualReferenceBlockers(counts, record);
        for (const VportRecord& record : m_vports)
            accumulateVisualReferenceBlockers(counts, record);
        for (const RawObjectRecord& record : m_rawObjects) {
            if (!isVisualMetadataRawObject(record))
                continue;
            ++counts.rawPayloads;
            const ReplayBlocker blocker = rawReplayBlocker(record);
            if (blocker == ReplayBlocker::None) {
                ++counts.replayableRawPayloads;
                continue;
            }
            ++counts.suppressedRawPayloads;
            if (blocker == ReplayBlocker::Invalidated)
                ++counts.invalidatedRawPayload;
            else if (blocker == ReplayBlocker::Replaced)
                ++counts.replacedNativeUnavailablePayload;
        }
        return counts;
    }
    VisualMetadataReplayEligibility visualMetadataReplayEligibility(
        std::uint32_t handle, DRW::Version version) const {
        VisualMetadataReplayEligibility eligibility;
        eligibility.handle = handle;
        for (const VisualMetadataSummaryRecord& summary :
             visualMetadataSummaries()) {
            if (summary.handle != handle)
                continue;
            eligibility.sourceType = summary.sourceType;
            eligibility.hasSemanticRecord = true;
            eligibility.replayState = summary.replayState;
            eligibility.nativeWriterAvailable =
                visualMetadataNativeWriterAvailable(summary.sourceType, version);
            eligibility.unsupportedNativeWriter =
                summary.sourceType == VisualMetadataSource::VisualStyle
                && !eligibility.nativeWriterAvailable;
            break;
        }
        for (const RawObjectRecord& record : m_rawObjects) {
            if (record.handle != handle || !isVisualMetadataRawObject(record))
                continue;
            eligibility.hasRawPayload = true;
            eligibility.rawBlocker = rawReplayBlocker(record);
            eligibility.rawReplayable =
                eligibility.rawBlocker == ReplayBlocker::None;
            if (eligibility.rawReplayable)
                eligibility.unsupportedNativeWriter = false;
            break;
        }
        return eligibility;
    }
    const LightRecord* findLightByHandle(std::uint32_t handle) const {
        if (handle == 0)
            return nullptr;
        for (const LightRecord& record : m_lights) {
            if (record.handle == handle)
                return &record;
        }
        return nullptr;
    }
    std::vector<const LightRecord*> findLightsByParentHandle(std::uint32_t parentHandle) const {
        std::vector<const LightRecord*> result;
        if (parentHandle == 0)
            return result;
        for (const LightRecord& record : m_lights) {
            if (record.parentHandle == parentHandle)
                result.push_back(&record);
        }
        return result;
    }
    const SunRecord* findSunByHandle(std::uint32_t handle) const {
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
    const SunRecord* findSunForViewHandle(std::uint32_t handle) const {
        const ViewRecord* view = findViewByHandle(handle);
        return view == nullptr ? nullptr : findSunByHandle(view->sunHandle);
    }

    static ReplayBlocker rawReplayBlocker(const RawObjectRecord& record) {
        if (record.replayState == ReplayState::ReplayInvalidated)
            return ReplayBlocker::Invalidated;
        if (record.replayState == ReplayState::ReplayReplaced)
            return ReplayBlocker::Replaced;
        if (record.isEntity && !isReplayableFixedModelerRawEntity(record))
            return ReplayBlocker::EntityReplayUnsupported;
        if (record.rawBytes.empty())
            return ReplayBlocker::MissingRawBytes;
        if (record.isCustomClass && record.recordName.empty() && record.className.empty())
            return ReplayBlocker::MissingClassMetadata;
        return ReplayBlocker::None;
    }

    static bool isReplayableFixedModelerRawEntity(const RawObjectRecord& record) {
        return record.isEntity && !record.isCustomClass
            && (record.objectType == 34   // VIEWPORT (paper-space; no typed RS entity)
                || record.objectType == 37 || record.objectType == 38
                || record.objectType == 39);
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

    static GraphReplayFamily graphReplayFamilyFromNames(
        const std::string& recordName, const std::string& className) {
        if (recordName == "DIMASSOC"
            || recordName == "ACDBDIMASSOC"
            || containsSubstring(className, "DimAssoc")) {
            return GraphReplayFamily::DimensionAssociation;
        }
        if (recordName == "ACAD_EVALUATION_GRAPH"
            || containsSubstring(className, "EvalGraph")
            || containsSubstring(className, "EvalExpr")) {
            return GraphReplayFamily::EvaluationGraph;
        }
        if (containsSubstring(recordName, "ACSH")
            || containsSubstring(className, "AcSh")) {
            return GraphReplayFamily::AcShHistory;
        }
        if (associativeKindFromRecordName(recordName) != AssociativeKind::Unknown
            || containsSubstring(className, "Assoc")
            || containsSubstring(className, "PersSubent")) {
            return GraphReplayFamily::AcDbAssoc;
        }
        if (containsSubstring(recordName, "OBJECTCONTEXTDATA")
            || containsSubstring(className, "ObjectContextData")) {
            return GraphReplayFamily::ObjectContext;
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
            return GraphReplayFamily::DynamicBlock;
        return GraphReplayFamily::Unknown;
    }

    static GraphReplayFamily graphReplayFamilyFromRawObject(
        const RawObjectRecord& record) {
        GraphReplayFamily family =
            graphReplayFamilyFromNames(record.recordName, record.className);
        if (family != GraphReplayFamily::Unknown)
            return family;
        switch (record.family) {
            case RawObjectFamily::Associative:
                return GraphReplayFamily::AcDbAssoc;
            case RawObjectFamily::EvaluationGraph:
                return GraphReplayFamily::EvaluationGraph;
            case RawObjectFamily::DynamicBlock:
                return GraphReplayFamily::DynamicBlock;
            case RawObjectFamily::ObjectContext:
                return GraphReplayFamily::ObjectContext;
            case RawObjectFamily::Unknown:
            default:
                return GraphReplayFamily::Unknown;
        }
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

    static const char* graphReplayFamilyName(GraphReplayFamily family) {
        switch (family) {
            case GraphReplayFamily::DimensionAssociation:
                return "DIMASSOC";
            case GraphReplayFamily::EvaluationGraph:
                return "ACAD_EVALUATION_GRAPH";
            case GraphReplayFamily::AcDbAssoc:
                return "ACDBASSOC";
            case GraphReplayFamily::DynamicBlock:
                return "dynamic block";
            case GraphReplayFamily::ObjectContext:
                return "object context";
            case GraphReplayFamily::AcShHistory:
                return "ACSH";
            case GraphReplayFamily::Unknown:
            default:
                return "unknown";
        }
    }

    static AdvancedEntityWriterFamily advancedEntityWriterFamilyFromNames(
        const std::string& recordName, const std::string& className) {
        if (containsSubstring(recordName, "MESH")
            || containsSubstring(className, "Mesh")
            || containsSubstring(className, "SubDMesh")) {
            return AdvancedEntityWriterFamily::Mesh;
        }
        if (recordName == "SHAPE" || className == "AcDbShape")
            return AdvancedEntityWriterFamily::Shape;
        if (recordName == "OLE2FRAME" || containsSubstring(className, "Ole2Frame"))
            return AdvancedEntityWriterFamily::Ole2Frame;
        if (recordName == "WIPEOUT" || className == "AcDbWipeout")
            return AdvancedEntityWriterFamily::Wipeout;
        if (recordName == "PDFUNDERLAY" || recordName == "DGNUNDERLAY"
            || recordName == "DWFUNDERLAY"
            || containsSubstring(className, "Underlay")
            || className == "AcDbPdfReference"
            || className == "AcDbDgnReference"
            || className == "AcDbDwfReference") {
            return AdvancedEntityWriterFamily::Underlay;
        }
        if (recordName == "IMAGE" || className == "AcDbRasterImage")
            return AdvancedEntityWriterFamily::RasterImage;
        if (recordName == "MULTILEADER" || recordName == "MLEADER"
            || className == "AcDbMLeader") {
            return AdvancedEntityWriterFamily::MLeader;
        }
        if (recordName == "ARC_DIMENSION" || className == "AcDbArcDimension")
            return AdvancedEntityWriterFamily::ArcDimension;
        return AdvancedEntityWriterFamily::Unknown;
    }

    static const char* advancedEntityWriterFamilyName(
        AdvancedEntityWriterFamily family) {
        switch (family) {
            case AdvancedEntityWriterFamily::Mesh:
                return "MESH";
            case AdvancedEntityWriterFamily::Shape:
                return "SHAPE";
            case AdvancedEntityWriterFamily::Ole2Frame:
                return "OLE2FRAME";
            case AdvancedEntityWriterFamily::RasterImage:
                return "IMAGE";
            case AdvancedEntityWriterFamily::Wipeout:
                return "WIPEOUT";
            case AdvancedEntityWriterFamily::Underlay:
                return "UNDERLAY";
            case AdvancedEntityWriterFamily::MLeader:
                return "MLEADER";
            case AdvancedEntityWriterFamily::ArcDimension:
                return "ARC_DIMENSION";
            case AdvancedEntityWriterFamily::Unknown:
            default:
                return "unknown";
        }
    }

    static const char* advancedEntityWriterOdaCoverageName(
        AdvancedEntityWriterOdaCoverage coverage) {
        switch (coverage) {
            case AdvancedEntityWriterOdaCoverage::Complete:
                return "complete";
            case AdvancedEntityWriterOdaCoverage::Partial:
                return "partial";
            case AdvancedEntityWriterOdaCoverage::Absent:
            default:
                return "absent";
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
            case ReplayBlocker::VersionMismatch:
                return "source/target version mismatch";
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

    static size_t graphReplayFamilyCount(
        const GraphReplayFamilyCounts& counts, GraphReplayFamily family) {
        switch (family) {
            case GraphReplayFamily::DimensionAssociation:
                return counts.dimensionAssociation;
            case GraphReplayFamily::EvaluationGraph:
                return counts.evaluationGraph;
            case GraphReplayFamily::AcDbAssoc:
                return counts.acDbAssoc;
            case GraphReplayFamily::DynamicBlock:
                return counts.dynamicBlock;
            case GraphReplayFamily::ObjectContext:
                return counts.objectContext;
            case GraphReplayFamily::AcShHistory:
                return counts.acShHistory;
            case GraphReplayFamily::Unknown:
            default:
                return counts.unknown;
        }
    }

    static size_t advancedEntityWriterFamilyCount(
        const AdvancedEntityWriterBlockerCounts& counts,
        AdvancedEntityWriterFamily family) {
        switch (family) {
            case AdvancedEntityWriterFamily::Mesh:
                return counts.mesh;
            case AdvancedEntityWriterFamily::Shape:
                return counts.shape;
            case AdvancedEntityWriterFamily::Ole2Frame:
                return counts.ole2Frame;
            case AdvancedEntityWriterFamily::RasterImage:
                return counts.rasterImage;
            case AdvancedEntityWriterFamily::Wipeout:
                return counts.wipeout;
            case AdvancedEntityWriterFamily::Underlay:
                return counts.underlay;
            case AdvancedEntityWriterFamily::MLeader:
                return counts.mleader;
            case AdvancedEntityWriterFamily::ArcDimension:
                return counts.arcDimension;
            case AdvancedEntityWriterFamily::Unknown:
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
            + m_rasterImages.size() + m_imageDefinitions.size()
            + m_underlays.size() + m_underlayDefinitions.size()
            + m_rasterVariables.size() + m_shapes.size()
            + m_ole2Frames.size()
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
            || hasReplayable(m_rasterImages)
            || hasReplayable(m_imageDefinitions)
            || hasReplayable(m_underlays)
            || hasReplayable(m_underlayDefinitions)
            || hasReplayable(m_rasterVariables)
            || hasReplayable(m_shapes)
            || hasReplayable(m_ole2Frames)
            || hasReplayable(m_imageDefinitionReactors)
            || hasReplayable(m_spatialFilters)
            || hasReplayable(m_geoData)
            || hasReplayable(m_tableGeometry)
            || hasReplayable(m_placeholders);
    }

    void invalidateByHandle(std::uint32_t handle) {
        invalidateMatching([handle](std::uint32_t recordHandle, std::uint32_t) {
            return recordHandle == handle;
        });
    }

    void invalidateByOwner(std::uint32_t ownerHandle) {
        invalidateMatching([ownerHandle](std::uint32_t, std::uint32_t parentHandle) {
            return parentHandle == ownerHandle;
        });
    }

    void invalidateAssociativeGraphForHandle(std::uint32_t dependentHandle) {
        if (dependentHandle == 0)
            return;
        const std::vector<std::uint32_t> affectedHandles =
            findAssociativeClosureFrom(dependentHandle, 32u);
        for (std::uint32_t affectedHandle : affectedHandles) {
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

    void invalidateTableGraphForHandle(std::uint32_t dependentHandle) {
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

    void invalidateMLeaderGraphForHandle(std::uint32_t dependentHandle) {
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

    void invalidateViewGraphForHandle(std::uint32_t dependentHandle) {
        if (dependentHandle == 0)
            return;
        for (ViewRecord& record : m_views) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (viewRecordReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateDocumentMappingForSource(
                    DocumentMappingSource::View, record.handle);
                invalidateRawVisualMetadataObject(record.handle);
            }
        }
        for (VportRecord& record : m_vports) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (vportRecordReferences(record, dependentHandle)) {
                record.replayState = ReplayState::ReplayInvalidated;
                invalidateDocumentMappingForSource(
                    DocumentMappingSource::Vport, record.handle);
                invalidateRawVisualMetadataObject(record.handle);
            }
        }
    }

    void markMLeaderReplayReplacedForHandle(std::uint32_t handle) {
        if (handle == 0)
            return;
        for (MLeaderRecord& record : m_mleaders) {
            if (record.handle == handle)
                record.replayState = ReplayState::ReplayReplaced;
        }
    }

    void markRawReplayReplacedForHandle(std::uint32_t handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
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
        invalidateContainer(m_rasterImages);
        invalidateContainer(m_imageDefinitions);
        invalidateContainer(m_underlays);
        invalidateContainer(m_underlayDefinitions);
        invalidateContainer(m_rasterVariables);
        invalidateContainer(m_shapes);
        invalidateContainer(m_ole2Frames);
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

    static bool tableCellReferences(const TableCellRecord& cell, std::uint32_t handle) {
        if (handle == 0)
            return false;
        if (cell.valueHandle == handle
            || cell.textStyleHandle == handle
            || cell.textStyleOverrideHandle == handle
            || cell.blockHandle == handle
            || cell.geometryHandle == handle) {
            return true;
        }
        for (std::uint32_t contentHandle : cell.contentHandles) {
            if (contentHandle == handle)
                return true;
        }
        for (std::uint32_t attributeHandle : cell.attributeHandles) {
            if (attributeHandle == handle)
                return true;
        }
        return false;
    }

    static bool tableRecordReferences(const TableRecord& record, std::uint32_t handle) {
        if (handle == 0)
            return false;
        if (record.tableStyleHandle == handle)
            return true;
        for (std::uint32_t valueHandle : record.valueHandles) {
            if (valueHandle == handle)
                return true;
        }
        for (std::uint32_t blockHandle : record.blockHandles) {
            if (blockHandle == handle)
                return true;
        }
        for (std::uint32_t fieldHandle : record.fieldHandles) {
            if (fieldHandle == handle)
                return true;
        }
        for (std::uint32_t attributeHandle : record.attributeHandles) {
            if (attributeHandle == handle)
                return true;
        }
        for (std::uint32_t textStyleHandle : record.textStyleHandles) {
            if (textStyleHandle == handle)
                return true;
        }
        for (std::uint32_t lineTypeHandle : record.lineTypeHandles) {
            if (lineTypeHandle == handle)
                return true;
        }
        for (std::uint32_t geometryHandle : record.geometryHandles) {
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
                                       std::uint32_t handle) {
        if (handle == 0)
            return false;
        return containsValue(record.m_textStyleHandles, handle)
               || containsValue(record.m_lineTypeHandles, handle);
    }

    template<typename T>
    static bool containsValue(const std::vector<T>& values, const T& value) {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    static bool mleaderRecordReferences(const MLeaderRecord& record, std::uint32_t handle) {
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
        for (std::uint32_t arrowHandle : record.arrowHeadOverrideHandles) {
            if (arrowHandle == handle)
                return true;
        }
        for (std::uint32_t attributeHandle : record.blockAttributeDefinitionHandles) {
            if (attributeHandle == handle)
                return true;
        }
        return false;
    }

private:
    VisualMetadataSummaryRecord visualSummaryFromView(
        const ViewRecord& record) const {
        VisualMetadataSummaryRecord summary;
        summary.sourceType = VisualMetadataSource::View;
        summary.specCoverage = VisualMetadataSpecCoverage::OdaCovered;
        summary.displayName = record.name;
        summary.handle = record.handle;
        summary.ownerHandle = record.parentHandle;
        summary.referencedSunHandle = record.sunHandle;
        summary.referencedVisualStyleHandle = record.visualStyleHandle;
        summary.referencedBackgroundHandle = record.backgroundHandle;
        summary.referencedLiveSectionHandle = record.liveSectionHandle;
        summary.unresolvedReferenceCount =
            visualSummaryUnresolvedReferenceCount(summary);
        summary.replayState = record.replayState;
        return summary;
    }

    VisualMetadataSummaryRecord visualSummaryFromVport(
        const VportRecord& record) const {
        VisualMetadataSummaryRecord summary;
        summary.sourceType = VisualMetadataSource::Vport;
        summary.specCoverage = VisualMetadataSpecCoverage::OdaCovered;
        summary.displayName = record.name;
        summary.handle = record.handle;
        summary.ownerHandle = record.parentHandle;
        summary.referencedSunHandle = record.sunHandle;
        summary.referencedVisualStyleHandle = record.visualStyleHandle;
        summary.referencedBackgroundHandle = record.backgroundHandle;
        summary.unresolvedReferenceCount =
            visualSummaryUnresolvedReferenceCount(summary);
        summary.replayState = record.replayState;
        return summary;
    }

    static VisualMetadataSummaryRecord visualSummaryFromVisualStyle(
        const VisualStyleRecord& record) {
        VisualMetadataSummaryRecord summary;
        summary.sourceType = VisualMetadataSource::VisualStyle;
        summary.specCoverage = VisualMetadataSpecCoverage::RawOnly;
        summary.displayName = !record.name.empty() ? record.name : record.description;
        summary.handle = record.handle;
        summary.ownerHandle = record.parentHandle;
        summary.lightOrSunType = record.type;
        summary.replayState = record.replayState;
        return summary;
    }

    static VisualMetadataSummaryRecord visualSummaryFromLight(
        const LightRecord& record) {
        VisualMetadataSummaryRecord summary;
        summary.sourceType = VisualMetadataSource::Light;
        summary.specCoverage = VisualMetadataSpecCoverage::CrossReferenceSourced;
        summary.displayName = record.name;
        summary.handle = record.handle;
        summary.ownerHandle = record.parentHandle;
        summary.lightOrSunType = record.type;
        summary.lightOrSunEnabled = record.status;
        summary.intensity = record.intensity;
        summary.color = record.color;
        summary.replayState = record.replayState;
        return summary;
    }

    static VisualMetadataSummaryRecord visualSummaryFromSun(
        const SunRecord& record) {
        VisualMetadataSummaryRecord summary;
        summary.sourceType = VisualMetadataSource::Sun;
        summary.specCoverage = VisualMetadataSpecCoverage::CrossReferenceSourced;
        summary.displayName = "SUN";
        summary.handle = record.handle;
        summary.ownerHandle = record.parentHandle;
        summary.lightOrSunEnabled = record.isOn;
        summary.intensity = record.intensity;
        summary.color = record.color;
        summary.julianDay = record.julianDay;
        summary.milliseconds = record.milliseconds;
        summary.daylightSavings = record.isDaylightSavings;
        summary.replayState = record.replayState;
        return summary;
    }

    size_t visualSummaryUnresolvedReferenceCount(
        const VisualMetadataSummaryRecord& record) const {
        size_t count = 0;
        if (record.referencedSunHandle != 0
            && findSunByHandle(record.referencedSunHandle) == nullptr) {
            ++count;
        }
        if (record.referencedVisualStyleHandle != 0
            && findVisualStyleByHandle(record.referencedVisualStyleHandle) == nullptr) {
            ++count;
        }
        if (record.referencedBackgroundHandle != 0)
            ++count;
        if (record.referencedLiveSectionHandle != 0)
            ++count;
        return count;
    }

    static void incrementVisualMetadataSourceCount(
        VisualMetadataSummaryCounts& counts, VisualMetadataSource source) {
        switch (source) {
            case VisualMetadataSource::View:
                ++counts.view;
                break;
            case VisualMetadataSource::Vport:
                ++counts.vport;
                break;
            case VisualMetadataSource::VisualStyle:
                ++counts.visualStyle;
                break;
            case VisualMetadataSource::Light:
                ++counts.light;
                break;
            case VisualMetadataSource::Sun:
                ++counts.sun;
                break;
        }
    }

    static void incrementVisualMetadataSpecCoverageCount(
        VisualMetadataSummaryCounts& counts,
        VisualMetadataSpecCoverage coverage) {
        switch (coverage) {
            case VisualMetadataSpecCoverage::OdaCovered:
                ++counts.odaCovered;
                break;
            case VisualMetadataSpecCoverage::CrossReferenceSourced:
                ++counts.crossReferenceSourced;
                break;
            case VisualMetadataSpecCoverage::RawOnly:
                ++counts.rawOnly;
                break;
        }
    }

    void accumulateVisualReferenceBlockers(
        VisualMetadataWriterBlockerCounts& counts,
        const ViewRecord& record) const {
        if (record.namedUcsHandle != 0
            && findUcsByHandle(record.namedUcsHandle) == nullptr) {
            ++counts.unresolvedUcs;
        }
        if (record.baseUcsHandle != 0
            && findUcsByHandle(record.baseUcsHandle) == nullptr) {
            ++counts.unresolvedBaseUcs;
        }
        if (record.visualStyleHandle != 0
            && findVisualStyleByHandle(record.visualStyleHandle) == nullptr) {
            ++counts.unresolvedVisualStyle;
        }
        if (record.sunHandle != 0 && findSunByHandle(record.sunHandle) == nullptr)
            ++counts.unresolvedSun;
        if (record.backgroundHandle != 0)
            ++counts.unresolvedBackground;
        if (record.liveSectionHandle != 0)
            ++counts.unresolvedLiveSection;
    }

    void accumulateVisualReferenceBlockers(
        VisualMetadataWriterBlockerCounts& counts,
        const VportRecord& record) const {
        if (record.namedUcsHandle != 0
            && findUcsByHandle(record.namedUcsHandle) == nullptr) {
            ++counts.unresolvedUcs;
        }
        if (record.baseUcsHandle != 0
            && findUcsByHandle(record.baseUcsHandle) == nullptr) {
            ++counts.unresolvedBaseUcs;
        }
        if (record.visualStyleHandle != 0
            && findVisualStyleByHandle(record.visualStyleHandle) == nullptr) {
            ++counts.unresolvedVisualStyle;
        }
        if (record.sunHandle != 0 && findSunByHandle(record.sunHandle) == nullptr)
            ++counts.unresolvedSun;
        if (record.backgroundHandle != 0)
            ++counts.unresolvedBackground;
    }

    static bool visualMetadataNativeWriterAvailable(
        VisualMetadataSource sourceType, DRW::Version version) {
        if (version == DRW::UNKNOWNV)
            return false;
        switch (sourceType) {
            case VisualMetadataSource::Light:
                return version >= DRW::AC1024;
            case VisualMetadataSource::Sun:
                return version >= DRW::AC1021;
            case VisualMetadataSource::View:
            case VisualMetadataSource::Vport:
            case VisualMetadataSource::VisualStyle:
            default:
                return false;
        }
    }

    DocumentMappingRecord documentMappingFromView(
        const ViewRecord& record, int documentItemIndex) const {
        DocumentMappingRecord mapping;
        mapping.sourceHandle = record.handle;
        mapping.handle = record.handle;
        mapping.sourceType = DocumentMappingSource::View;
        mapping.sourceName = record.name;
        mapping.documentItemName = record.name;
        mapping.documentItemIndex = documentItemIndex;
        mapping.ownerHandle = record.parentHandle;
        mapping.parentHandle = record.parentHandle;
        mapping.associatedUcsHandle = record.hasUcs ? record.namedUcsHandle : 0;
        mapping.baseUcsHandle = record.baseUcsHandle;
        mapping.namedUcsHandle = record.namedUcsHandle;
        mapping.backgroundHandle = record.backgroundHandle;
        mapping.visualStyleHandle = record.visualStyleHandle;
        mapping.sunHandle = record.sunHandle;
        mapping.liveSectionHandle = record.liveSectionHandle;
        mapping.unresolvedReferenceCount =
            documentMappingUnresolvedReferenceCount(mapping);
        mapping.replayState = record.replayState;
        return mapping;
    }

    DocumentMappingRecord documentMappingFromUcs(
        const UcsRecord& record, int documentItemIndex) const {
        DocumentMappingRecord mapping;
        mapping.sourceHandle = record.handle;
        mapping.handle = record.handle;
        mapping.sourceType = DocumentMappingSource::Ucs;
        mapping.sourceName = record.name;
        mapping.documentItemName = record.name;
        mapping.documentItemIndex = documentItemIndex;
        mapping.ownerHandle = record.parentHandle;
        mapping.parentHandle = record.parentHandle;
        mapping.unresolvedReferenceCount =
            documentMappingUnresolvedReferenceCount(mapping);
        mapping.replayState = record.replayState;
        return mapping;
    }

    DocumentMappingRecord documentMappingFromVport(
        const VportRecord& record, int documentItemIndex) const {
        DocumentMappingRecord mapping;
        mapping.sourceHandle = record.handle;
        mapping.handle = record.handle;
        mapping.sourceType = DocumentMappingSource::Vport;
        mapping.sourceName = record.name;
        mapping.documentItemName = record.name;
        mapping.documentItemIndex = documentItemIndex;
        mapping.ownerHandle = record.parentHandle;
        mapping.parentHandle = record.parentHandle;
        mapping.baseUcsHandle = record.baseUcsHandle;
        mapping.namedUcsHandle = record.namedUcsHandle;
        mapping.backgroundHandle = record.backgroundHandle;
        mapping.visualStyleHandle = record.visualStyleHandle;
        mapping.sunHandle = record.sunHandle;
        mapping.unresolvedReferenceCount =
            documentMappingUnresolvedReferenceCount(mapping);
        mapping.replayState = record.replayState;
        return mapping;
    }

    void upsertDocumentMapping(DocumentMappingRecord mapping) {
        for (DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceHandle != 0
                && record.sourceHandle == mapping.sourceHandle) {
                if (mapping.documentItemIndex < 0) {
                    mapping.documentItemIndex = record.documentItemIndex;
                    if (mapping.documentItemName.empty())
                        mapping.documentItemName = record.documentItemName;
                }
                record = std::move(mapping);
                return;
            }
        }
        m_documentMappings.push_back(std::move(mapping));
    }

    void mapDocumentItem(DocumentMappingSource sourceType, std::uint32_t handle,
                         const std::string& name, int documentItemIndex) {
        for (DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceType != sourceType)
                continue;
            if ((handle != 0 && record.sourceHandle == handle)
                || (!name.empty() && record.sourceName == name)) {
                record.documentItemName = name;
                record.documentItemIndex = documentItemIndex;
                record.unresolvedReferenceCount =
                    documentMappingUnresolvedReferenceCount(record);
                return;
            }
        }
        DocumentMappingRecord mapping;
        mapping.sourceHandle = handle;
        mapping.handle = handle;
        mapping.sourceType = sourceType;
        mapping.sourceName = name;
        mapping.documentItemName = name;
        mapping.documentItemIndex = documentItemIndex;
        mapping.unresolvedReferenceCount =
            documentMappingUnresolvedReferenceCount(mapping);
        m_documentMappings.push_back(std::move(mapping));
    }

    void refreshDocumentMappingUnresolvedReferenceCounts() {
        for (DocumentMappingRecord& record : m_documentMappings) {
            record.unresolvedReferenceCount =
                documentMappingUnresolvedReferenceCount(record);
        }
    }

    void invalidateDocumentMappingForSource(
        DocumentMappingSource sourceType, std::uint32_t handle) {
        if (handle == 0)
            return;
        for (DocumentMappingRecord& record : m_documentMappings) {
            if (record.sourceType == sourceType && record.sourceHandle == handle
                && record.replayState == ReplayState::ReplayAllowed) {
                record.replayState = ReplayState::ReplayInvalidated;
            }
        }
    }

    size_t documentMappingUnresolvedReferenceCount(
        const DocumentMappingRecord& record) const {
        size_t count = 0;
        std::vector<std::uint32_t> unresolvedHandles;
        appendUnresolvedUcsReference(
            unresolvedHandles, record.associatedUcsHandle);
        appendUnresolvedUcsReference(unresolvedHandles, record.baseUcsHandle);
        appendUnresolvedUcsReference(unresolvedHandles, record.namedUcsHandle);
        appendUnresolvedOpaqueReference(unresolvedHandles, record.plotViewHandle);
        appendUnresolvedOpaqueReference(unresolvedHandles, record.backgroundHandle);
        appendUnresolvedOpaqueReference(unresolvedHandles, record.visualStyleHandle);
        appendUnresolvedSunReference(unresolvedHandles, record.sunHandle);
        appendUnresolvedOpaqueReference(unresolvedHandles, record.liveSectionHandle);
        appendUnresolvedOpaqueReference(
            unresolvedHandles, record.viewportHeaderHandle);
        appendUnresolvedOpaqueReference(
            unresolvedHandles, record.layoutBlockRecordHandle);
        for (std::uint32_t handle : unresolvedHandles) {
            if (handle != 0 && !containsValue(unresolvedHandles, handle, count))
                ++count;
        }
        return count;
    }

    void appendUnresolvedUcsReference(
        std::vector<std::uint32_t>& unresolvedHandles, std::uint32_t handle) const {
        if (handle != 0 && findUcsByHandle(handle) == nullptr)
            unresolvedHandles.push_back(handle);
    }

    void appendUnresolvedSunReference(
        std::vector<std::uint32_t>& unresolvedHandles, std::uint32_t handle) const {
        if (handle != 0 && findSunByHandle(handle) == nullptr)
            unresolvedHandles.push_back(handle);
    }

    static void appendUnresolvedOpaqueReference(
        std::vector<std::uint32_t>& unresolvedHandles, std::uint32_t handle) {
        if (handle != 0)
            unresolvedHandles.push_back(handle);
    }

    static bool containsValue(
        const std::vector<std::uint32_t>& values, std::uint32_t value, size_t limit) {
        for (size_t i = 0; i < limit && i < values.size(); ++i) {
            if (values[i] == value)
                return true;
        }
        return false;
    }

    static bool viewRecordReferences(const ViewRecord& record, std::uint32_t handle) {
        if (handle == 0)
            return false;
        return record.namedUcsHandle == handle
               || record.baseUcsHandle == handle
               || record.backgroundHandle == handle
               || record.visualStyleHandle == handle
               || record.sunHandle == handle
               || record.liveSectionHandle == handle;
    }

    static bool vportRecordReferences(const VportRecord& record, std::uint32_t handle) {
        if (handle == 0)
            return false;
        return record.namedUcsHandle == handle
               || record.baseUcsHandle == handle
               || record.backgroundHandle == handle
               || record.visualStyleHandle == handle
               || record.sunHandle == handle;
    }

    static bool mleaderStyleRecordReferences(const MLeaderStyleRecord& record,
                                             std::uint32_t handle) {
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
                                            std::uint32_t handle) {
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
        for (std::uint32_t refHandle : record.ownedParamHandles) {
            if (refHandle == handle)
                return true;
        }
        for (std::uint32_t refHandle : record.ownedActionHandles) {
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

    void invalidateRawAssociativeObject(std::uint32_t handle) {
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

    static bool isVisualMetadataRawObject(const RawObjectRecord& record) {
        return record.recordName == "VIEW"
               || record.recordName == "VPORT"
               || record.recordName == "UCS"
               || record.recordName == "VISUALSTYLE"
               || record.recordName == "SUN"
               || record.recordName == "LIGHT"
               || record.className == "AcDbViewTableRecord"
               || record.className == "AcDbViewportTableRecord"
               || record.className == "AcDbUCSTableRecord"
               || record.className == "AcDbVisualStyle"
               || record.className == "AcDbSun"
               || record.className == "AcDbLight";
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

    void invalidateRawTableObject(std::uint32_t handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (record.handle == handle && isTableRawObject(record))
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    void invalidateRawVisualMetadataObject(std::uint32_t handle) {
        if (handle == 0)
            return;
        for (RawObjectRecord& record : m_rawObjects) {
            if (record.replayState != ReplayState::ReplayAllowed)
                continue;
            if (record.handle == handle && isVisualMetadataRawObject(record))
                record.replayState = ReplayState::ReplayInvalidated;
        }
    }

    void invalidateRawMLeaderStyle(std::uint32_t handle) {
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

    static bool containsHandle(const std::vector<std::uint32_t>& handles,
                               std::uint32_t handle) {
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
        for (std::uint32_t handle : record.ownedParamHandles) {
            addAssociativeEdge(record.handle, record.recordName, record.kind,
                               AssociativeEdgeKind::OwnsParameter, handle,
                               AssociativeEdgeConfidence::ExplicitHandle);
        }
        for (std::uint32_t handle : record.ownedActionHandles) {
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
        std::uint32_t sourceHandle, const std::string& sourceRecordName,
        AssociativeKind sourceKind, AssociativeEdgeKind edgeKind,
        std::uint32_t targetHandle, AssociativeEdgeConfidence confidence) {
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

    void invalidateAssociativeSemanticRecord(std::uint32_t handle) {
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

    static AdvancedEntityWriterReadiness advancedEntityWriterReadinessFromRawObject(
        const RawObjectRecord& rawRecord, DRW::Version version) {
        AdvancedEntityWriterReadiness record;
        record.family = advancedEntityWriterFamilyFromNames(
            rawRecord.recordName, rawRecord.className);
        record.odaCoverage = advancedEntityWriterOdaCoverage(record.family);
        record.handle = rawRecord.handle;
        record.recordName = rawRecord.recordName;
        record.className = rawRecord.className;
        record.nativeWriterAvailable =
            advancedEntityNativeWriterAvailable(record.family, version);
        record.rawReplayAvailable =
            rawReplayBlocker(rawRecord) == ReplayBlocker::None
            && !rawRecord.isEntity;
        record.fallbackAvailable =
            advancedEntityFallbackAvailable(record.family);
        record.editedFallbackInvalidated =
            rawRecord.replayState == ReplayState::ReplayInvalidated
            || rawRecord.replayState == ReplayState::ReplayReplaced
            || rawRecord.isEntity;
        record.missingRequiredMetadata =
            rawRecord.isCustomClass && rawRecord.recordName.empty()
            && rawRecord.className.empty();
        record.missingPayloadBytes = rawRecord.rawBytes.empty();
        record.unsupportedAdvancedContent =
            record.family != AdvancedEntityWriterFamily::Unknown
            && !record.nativeWriterAvailable
            && !record.rawReplayAvailable;
        return record;
    }

    static AdvancedEntityWriterReadiness advancedEntityWriterReadinessFromMLeader(
        const MLeaderRecord& mleader, DRW::Version version) {
        AdvancedEntityWriterReadiness record;
        record.family = AdvancedEntityWriterFamily::MLeader;
        record.odaCoverage = AdvancedEntityWriterOdaCoverage::Complete;
        record.handle = mleader.handle;
        record.recordName = "MULTILEADER";
        record.className = "AcDbMLeader";
        record.nativeWriterAvailable =
            advancedEntityNativeWriterAvailable(record.family, version);
        record.fallbackAvailable = true;
        record.editedFallbackInvalidated =
            mleader.replayState != ReplayState::ReplayAllowed;
        record.missingRequiredMetadata =
            (mleader.styleHandle != 0 && !mleader.styleResolved)
            || (mleader.effectiveContentType == 2 && !mleader.hasTextContent)
            || mleader.rootCount == 0 || mleader.leaderLineCount == 0;
        record.unsupportedAdvancedContent =
            mleader.effectiveContentType == 1 || mleader.effectiveContentType == 3
            || mleader.hasBlockContent || mleader.overrideFlags != 0;
        return record;
    }

    static AdvancedEntityWriterReadiness advancedEntityWriterReadinessFromMesh(
        const MeshRecord& mesh, DRW::Version version) {
        AdvancedEntityWriterReadiness record;
        record.family = AdvancedEntityWriterFamily::Mesh;
        record.odaCoverage = AdvancedEntityWriterOdaCoverage::Partial;
        record.handle = mesh.handle;
        record.recordName =
            mesh.recordName.empty() ? std::string("MESH") : mesh.recordName;
        record.className = mesh.isSubDMesh ? "AcDbSubDMesh" : "AcDbPolyline";
        record.nativeWriterAvailable =
            advancedEntityNativeWriterAvailable(record.family, version);
        record.fallbackAvailable = mesh.fallbackPreviewGenerated;
        record.editedFallbackInvalidated =
            mesh.fallbackInvalidated
            || mesh.replayState != ReplayState::ReplayAllowed;
        record.missingRequiredMetadata =
            mesh.parentHandle == 0 || meshHasMalformedCounts(mesh);
        record.unsupportedAdvancedContent =
            mesh.isSubDMesh || mesh.subdivisionLevel > 0
            || mesh.edgeCount > 0 || mesh.creaseCount > 0
            || mesh.rawRangeStatus != MeshRawRangeStatus::Complete;
        return record;
    }

    static AdvancedEntityWriterReadiness advancedEntityWriterReadinessFromShape(
        const ShapeRecord& shape, DRW::Version version) {
        AdvancedEntityWriterReadiness record;
        record.family = AdvancedEntityWriterFamily::Shape;
        record.odaCoverage = AdvancedEntityWriterOdaCoverage::Complete;
        record.handle = shape.handle;
        record.recordName = "SHAPE";
        record.className = "AcDbShape";
        record.nativeWriterAvailable =
            advancedEntityNativeWriterAvailable(record.family, version);
        record.rawReplayAvailable = false;
        record.fallbackAvailable = false;
        record.editedFallbackInvalidated =
            shape.replayState != ReplayState::ReplayAllowed;
        record.missingRequiredMetadata =
            shape.shapeFileHandle == 0 || shape.shapeIndex == 0;
        record.missingPayloadBytes =
            shape.rawRangeStatus == MeshRawRangeStatus::Missing;
        record.unsupportedAdvancedContent = !record.nativeWriterAvailable;
        return record;
    }

    static AdvancedEntityWriterReadiness advancedEntityWriterReadinessFromOle2Frame(
        const Ole2FrameRecord& ole2Frame, DRW::Version version) {
        AdvancedEntityWriterReadiness record;
        record.family = AdvancedEntityWriterFamily::Ole2Frame;
        record.odaCoverage = AdvancedEntityWriterOdaCoverage::Complete;
        record.handle = ole2Frame.handle;
        record.recordName = "OLE2FRAME";
        record.className = "AcDbOle2Frame";
        record.nativeWriterAvailable =
            advancedEntityNativeWriterAvailable(record.family, version);
        record.rawReplayAvailable = false;
        record.fallbackAvailable = ole2Frame.previewFrameAvailable;
        record.editedFallbackInvalidated =
            ole2Frame.previewFrameInvalidated
            || ole2Frame.replayState != ReplayState::ReplayAllowed;
        record.missingRequiredMetadata =
            !ole2Frame.payloadPresent || ole2Frame.payloadTruncated
            || ole2Frame.payloadTooLarge;
        record.missingPayloadBytes = ole2Frame.rawByteCount == 0;
        record.unsupportedAdvancedContent = !record.nativeWriterAvailable;
        return record;
    }

    static bool advancedEntityNativeWriterAvailable(
        AdvancedEntityWriterFamily family, DRW::Version version) {
        if (version == DRW::UNKNOWNV)
            return false;
        switch (family) {
            case AdvancedEntityWriterFamily::MLeader:
                return version >= DRW::AC1024;
            default:
                return false;
        }
    }

    static bool advancedEntityFallbackAvailable(
        AdvancedEntityWriterFamily family) {
        switch (family) {
            case AdvancedEntityWriterFamily::Mesh:
            case AdvancedEntityWriterFamily::RasterImage:
            case AdvancedEntityWriterFamily::Wipeout:
            case AdvancedEntityWriterFamily::Underlay:
            case AdvancedEntityWriterFamily::MLeader:
                return true;
            default:
                return false;
        }
    }

    static AdvancedEntityWriterOdaCoverage advancedEntityWriterOdaCoverage(
        AdvancedEntityWriterFamily family) {
        switch (family) {
            case AdvancedEntityWriterFamily::Shape:
            case AdvancedEntityWriterFamily::Ole2Frame:
            case AdvancedEntityWriterFamily::RasterImage:
            case AdvancedEntityWriterFamily::Wipeout:
            case AdvancedEntityWriterFamily::MLeader:
                return AdvancedEntityWriterOdaCoverage::Complete;
            case AdvancedEntityWriterFamily::Mesh:
            case AdvancedEntityWriterFamily::Underlay:
            case AdvancedEntityWriterFamily::ArcDimension:
                return AdvancedEntityWriterOdaCoverage::Partial;
            case AdvancedEntityWriterFamily::Unknown:
            default:
                return AdvancedEntityWriterOdaCoverage::Absent;
        }
    }

    static void incrementAdvancedEntityWriterFamilyCount(
        AdvancedEntityWriterBlockerCounts& counts,
        AdvancedEntityWriterFamily family) {
        switch (family) {
            case AdvancedEntityWriterFamily::Mesh:
                ++counts.mesh;
                return;
            case AdvancedEntityWriterFamily::Shape:
                ++counts.shape;
                return;
            case AdvancedEntityWriterFamily::Ole2Frame:
                ++counts.ole2Frame;
                return;
            case AdvancedEntityWriterFamily::RasterImage:
                ++counts.rasterImage;
                return;
            case AdvancedEntityWriterFamily::Wipeout:
                ++counts.wipeout;
                return;
            case AdvancedEntityWriterFamily::Underlay:
                ++counts.underlay;
                return;
            case AdvancedEntityWriterFamily::MLeader:
                ++counts.mleader;
                return;
            case AdvancedEntityWriterFamily::ArcDimension:
                ++counts.arcDimension;
                return;
            case AdvancedEntityWriterFamily::Unknown:
            default:
                ++counts.unknown;
                return;
        }
    }

    static void incrementAdvancedEntityWriterCoverageCount(
        AdvancedEntityWriterBlockerCounts& counts,
        AdvancedEntityWriterOdaCoverage coverage) {
        switch (coverage) {
            case AdvancedEntityWriterOdaCoverage::Complete:
                ++counts.odaComplete;
                return;
            case AdvancedEntityWriterOdaCoverage::Partial:
                ++counts.odaPartial;
                return;
            case AdvancedEntityWriterOdaCoverage::Absent:
            default:
                ++counts.odaAbsent;
                return;
        }
    }

    static void incrementGraphReplayFamilyCount(
        GraphReplayFamilyCounts& counts, GraphReplayFamily family) {
        switch (family) {
            case GraphReplayFamily::DimensionAssociation:
                ++counts.dimensionAssociation;
                return;
            case GraphReplayFamily::EvaluationGraph:
                ++counts.evaluationGraph;
                return;
            case GraphReplayFamily::AcDbAssoc:
                ++counts.acDbAssoc;
                return;
            case GraphReplayFamily::DynamicBlock:
                ++counts.dynamicBlock;
                return;
            case GraphReplayFamily::ObjectContext:
                ++counts.objectContext;
                return;
            case GraphReplayFamily::AcShHistory:
                ++counts.acShHistory;
                return;
            case GraphReplayFamily::Unknown:
            default:
                ++counts.unknown;
                return;
        }
    }

    static void incrementGraphReplayBlockerCounts(
        GraphReplayPolicyCounts& counts, ReplayBlocker blocker) {
        switch (blocker) {
            case ReplayBlocker::Invalidated:
                ++counts.invalidated;
                ++counts.cyclePathInvalidated;
                return;
            case ReplayBlocker::Replaced:
                ++counts.replaced;
                ++counts.nativeReplacement;
                return;
            case ReplayBlocker::EntityReplayUnsupported:
                ++counts.entityReplayUnsupported;
                ++counts.editedEntity;
                return;
            case ReplayBlocker::MissingRawBytes:
                ++counts.missingRawBytes;
                ++counts.missingTarget;
                return;
            case ReplayBlocker::MissingClassMetadata:
                ++counts.missingClassMetadata;
                ++counts.unsupportedEvaluator;
                return;
            case ReplayBlocker::WriterRejected:
            case ReplayBlocker::SemanticOnly:
            case ReplayBlocker::None:
            default:
                return;
        }
    }

    bool hasRawGraphObjectForHandle(
        std::uint32_t handle, GraphReplayFamily family) const {
        if (handle == 0)
            return false;
        for (const RawObjectRecord& record : m_rawObjects) {
            if (record.handle == handle
                && graphReplayFamilyFromRawObject(record) == family) {
                return true;
            }
        }
        return false;
    }

    static MeshRawRangeStatus meshRawRangeStatus(
        int vertexCount, int faceCount, size_t preservedVertexCount) {
        if (preservedVertexCount == 0)
            return MeshRawRangeStatus::Missing;
        if (vertexCount <= 0 || faceCount <= 0)
            return MeshRawRangeStatus::Incomplete;
        const size_t expectedVertexCount =
            static_cast<size_t>(vertexCount) * static_cast<size_t>(faceCount);
        return preservedVertexCount == expectedVertexCount
                   ? MeshRawRangeStatus::Complete
                   : MeshRawRangeStatus::Incomplete;
    }

    static bool meshHasMalformedCounts(const MeshRecord& record) {
        if (record.vertexCount <= 0 || record.faceCount <= 0)
            return true;
        const size_t expectedVertexCount =
            static_cast<size_t>(record.vertexCount)
            * static_cast<size_t>(record.faceCount);
        return record.preservedVertexCount != 0
            && record.preservedVertexCount != expectedVertexCount;
    }

    static std::uint32_t parseHexHandle(const std::string& text) {
        if (text.empty())
            return 0;
        try {
            return static_cast<std::uint32_t>(std::stoul(text, nullptr, 16));
        } catch (...) {
            return 0;
        }
    }

    static ExternalReferencePathDiagnostic externalReferencePathDiagnostic(
        const std::string& path) {
        ExternalReferencePathDiagnostic diagnostic;
        diagnostic.path = path;
        if (path.empty()) {
            diagnostic.status = ExternalReferencePathStatus::Empty;
            return diagnostic;
        }
        const size_t schemePos = path.find("://");
        if (schemePos != std::string::npos) {
            const std::string scheme = path.substr(0, schemePos);
            if (scheme == "http" || scheme == "https" || scheme == "file")
                diagnostic.status = ExternalReferencePathStatus::External;
            else
                diagnostic.status = ExternalReferencePathStatus::UnsupportedScheme;
            return diagnostic;
        }
        if (!isAbsolutePath(path)) {
            diagnostic.status = ExternalReferencePathStatus::Relative;
            return diagnostic;
        }
        std::ifstream file(path);
        diagnostic.status = file.good()
                                ? ExternalReferencePathStatus::AbsolutePresent
                                : ExternalReferencePathStatus::AbsoluteMissing;
        return diagnostic;
    }

    static bool isAbsolutePath(const std::string& path) {
        if (path.empty())
            return false;
        if (path.front() == '/' || path.front() == '\\')
            return true;
        return path.size() > 2
            && ((path[0] >= 'A' && path[0] <= 'Z')
                || (path[0] >= 'a' && path[0] <= 'z'))
            && path[1] == ':';
    }

    static ClipBoundaryStatus clipBoundaryStatus(
        int clip, size_t vertexCount, bool isWipeout) {
        if (clip == 0 && !isWipeout)
            return ClipBoundaryStatus::NoBoundary;
        if (vertexCount == 0)
            return isWipeout ? ClipBoundaryStatus::Malformed
                             : ClipBoundaryStatus::NoBoundary;
        if (vertexCount == 2)
            return ClipBoundaryStatus::Rectangular;
        if (vertexCount >= 3)
            return ClipBoundaryStatus::Polygonal;
        return ClipBoundaryStatus::Malformed;
    }

    static ClipBoundaryStatus underlayClipBoundaryStatus(size_t vertexCount) {
        if (vertexCount == 0)
            return ClipBoundaryStatus::NoBoundary;
        if (vertexCount == 2)
            return ClipBoundaryStatus::Rectangular;
        if (vertexCount >= 3)
            return ClipBoundaryStatus::Polygonal;
        return ClipBoundaryStatus::Malformed;
    }

    static void incrementClipBoundaryCounts(
        ExternalReferenceCounts& counts, ClipBoundaryStatus status) {
        switch (status) {
            case ClipBoundaryStatus::NoBoundary:
                ++counts.noBoundaryClips;
                return;
            case ClipBoundaryStatus::Rectangular:
                ++counts.rectangularClips;
                return;
            case ClipBoundaryStatus::Polygonal:
                ++counts.polygonalClips;
                return;
            case ClipBoundaryStatus::Malformed:
                ++counts.malformedClips;
                return;
            case ClipBoundaryStatus::Unknown:
            default:
                return;
        }
    }

    static void incrementPathStatusCounts(
        ExternalReferenceCounts& counts,
        const ExternalReferencePathDiagnostic& diagnostic) {
        switch (diagnostic.status) {
            case ExternalReferencePathStatus::Empty:
                ++counts.emptyPaths;
                break;
            case ExternalReferencePathStatus::Relative:
                ++counts.relativePaths;
                break;
            case ExternalReferencePathStatus::AbsoluteMissing:
                ++counts.absoluteMissingPaths;
                break;
            case ExternalReferencePathStatus::External:
                ++counts.externalPaths;
                break;
            case ExternalReferencePathStatus::UnsupportedScheme:
                ++counts.unsupportedSchemes;
                break;
            case ExternalReferencePathStatus::CaseMismatchCandidate:
                ++counts.caseMismatchCandidates;
                break;
            case ExternalReferencePathStatus::AbsolutePresent:
            case ExternalReferencePathStatus::Unknown:
            default:
                break;
        }
        if (diagnostic.caseMismatchCandidate)
            ++counts.caseMismatchCandidates;
    }

    template<typename Predicate>
    void invalidateMatching(Predicate predicate) {
        invalidateMatching(m_rawObjects, predicate);
        invalidateMatching(m_views, predicate);
        invalidateMatching(m_ucsRecords, predicate);
        invalidateMatching(m_vports, predicate);
        invalidateMatching(m_documentMappings, predicate);
        invalidateMatching(m_visualStyles, predicate);
        invalidateMatching(m_lights, predicate);
        invalidateMatching(m_suns, predicate);
        invalidateMatching(m_modelerGeometry, predicate);
        invalidateMatching(m_meshes, predicate);
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
        invalidateMatching(m_rasterImages, predicate);
        invalidateMatching(m_imageDefinitions, predicate);
        invalidateMatching(m_underlays, predicate);
        invalidateMatching(m_underlayDefinitions, predicate);
        invalidateMatching(m_rasterVariables, predicate);
        invalidateMatching(m_shapes, predicate);
        invalidateMatching(m_ole2Frames, predicate);
        invalidateMatching(m_imageDefinitionReactors, predicate);
        invalidateMatching(m_spatialFilters, predicate);
        invalidateMatching(m_geoData, predicate);
        invalidateMatching(m_tableGeometry, predicate);
        invalidateMatching(m_placeholders, predicate);
        invalidateMatching(m_objectContextData, predicate);
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

    DRW::Version m_sourceDwgVersion = DRW::UNKNOWNV;
    std::vector<RawObjectRecord> m_rawObjects;
    std::vector<RawDwgSectionRecord> m_rawDwgSections;
    std::vector<DRW_Class> m_dxfClasses;
    std::vector<DRW_RawDxfObject> m_rawDxfObjects;
    std::vector<DRW_RawDxfObject> m_rawDxfEntities;
    std::vector<ViewRecord> m_views;
    std::vector<UcsRecord> m_ucsRecords;
    std::vector<VportRecord> m_vports;
    std::vector<DocumentMappingRecord> m_documentMappings;
    std::vector<VisualStyleRecord> m_visualStyles;
    std::vector<LightRecord> m_lights;
    std::vector<SunRecord> m_suns;
    std::vector<ModelerGeometryRecord> m_modelerGeometry;
    std::vector<MeshRecord> m_meshes;
    std::vector<MeshSidecarRecord> m_meshSidecars;
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
    std::vector<RasterImageRecord> m_rasterImages;
    std::vector<ImageDefinitionRecord> m_imageDefinitions;
    std::vector<UnderlayRecord> m_underlays;
    std::vector<UnderlayDefinitionRecord> m_underlayDefinitions;
    std::vector<RasterVariablesRecord> m_rasterVariables;
    std::vector<ShapeRecord> m_shapes;
    std::vector<Ole2FrameRecord> m_ole2Frames;
    std::vector<ImageDefinitionReactorRecord> m_imageDefinitionReactors;
    std::vector<SpatialFilterRecord> m_spatialFilters;
    std::vector<GeoDataRecord> m_geoData;
    std::vector<TableGeometryRecord> m_tableGeometry;
    std::vector<PlaceholderRecord> m_placeholders;
    std::vector<DictionaryRecord> m_dictionaries;
    std::vector<XRecordRecord> m_xrecords;
    std::vector<LayoutRecord> m_layouts;
    // PR 8d.2a — five small no-storage OBJECTS families.
    std::vector<ScaleRecord> m_scales;
    std::vector<ObjectContextDataRecord> m_objectContextData;
    std::vector<MLineStyleRecord> m_mlineStyles;
    std::vector<WipeoutVariablesRecord> m_wipeoutVariables;
    std::vector<IDBufferRecord> m_idBuffers;
    std::vector<LayerIndexRecord> m_layerIndexes;
    std::vector<SpatialIndexRecord> m_spatialIndexes;
    std::vector<DictionaryVarRecord> m_dictionaryVars;
    // PR 8d.2b — four larger no-storage OBJECTS families.
    std::vector<DictionaryWithDefaultRecord> m_dictionariesWithDefault;
    std::vector<SortEntsTableRecord> m_sortEntsTables;
    std::vector<FieldListRecord> m_fieldLists;
    std::vector<DataTableRecord> m_dataTables;
    std::vector<DynamicBlockRecord> m_dynamicBlockObjects;
    std::vector<FieldRecord> m_fields;
};

#endif
