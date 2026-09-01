/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGWRITER_H
#define DWGWRITER_H

#include <algorithm>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../drw_base.h"
#include "../drw_entities.h"
#include "../drw_header.h"
#include "../drw_objects.h"
#include "../handle_allocator.h"
#include "dwgbufferw.h"
#include "dwgutil.h"

class DRW_TextCodec;
class DRW_Entity;

/// DWG CLASSES section entry used by custom-class object/entity writers.
/// DWG custom class numbers start at 500; fixed built-in object types are not
/// represented here.
struct DwgClassDefinition {
    std::uint16_t m_classNum {0};
    std::uint16_t m_proxyFlag {0};
    std::string m_appName;
    std::string m_className;
    std::string m_recordName;
    bool m_wasProxy {false};
    std::uint16_t m_entityFlagRaw {0};
    std::int32_t m_instanceCount {0};
    std::int32_t m_dwgVersion {0};
    std::int32_t m_maintenanceVersion {0};
    std::int32_t m_unknown1 {0};
    std::int32_t m_unknown2 {0};
};

/// Identity key for a raw class ordinal remap. Class numbers are file-local,
/// so an ordinal alone cannot distinguish two opaque classes imported from
/// malformed or merged source metadata.
struct DwgRawClassIdentityKey {
    std::uint16_t m_sourceClassNum {0};
    std::string m_className;
    std::string m_recordName;
    std::uint16_t m_entityFlagRaw {0};

    bool operator<(const DwgRawClassIdentityKey& other) const {
        if (m_sourceClassNum != other.m_sourceClassNum)
            return m_sourceClassNum < other.m_sourceClassNum;
        if (m_className != other.m_className)
            return m_className < other.m_className;
        if (m_recordName != other.m_recordName)
            return m_recordName < other.m_recordName;
        return m_entityFlagRaw < other.m_entityFlagRaw;
    }
};

/// `HandleAllocator` (the shared reserve-and-mint handle subsystem used by both
/// the DWG and DXF write paths) lives in handle_allocator.h, included above.

/// Abstract base for per-version DWG writers.  Concrete subclasses
/// (dwgWriter15 for R2000) drive the section emission order and
/// own the in-memory accumulator that is flushed to disk at the end
/// of `finalize()`.
///
/// Mirror of `class dwgReader` ([intern/dwgreader.h](dwgreader.h)).
/// Read side dispatches via `dwgRW::createReaderForVersion`; write
/// side will dispatch similarly once additional target versions land.
class dwgWriter {
public:
    /// Construct around an output stream and a populated header.
    /// The stream is held by reference for the final flush; the
    /// writer does NOT close it (caller manages lifetime).
    dwgWriter(std::ofstream *stream, DRW_Header *header)
        : m_stream{stream}, m_header{header}
    {
        m_handles.seedReserved();
        registerDwgClass({500, 0x401, "ACAD", "AcDbArcDimension",
                          "ARC_DIMENSION", false, 0x1F2});
        registerDwgClass({501, 0x401, "ACAD",
                          "AcDbMLeader", "MULTILEADER", false, 0x1F2});
        registerDwgClass({502, 0x401, "ACAD",
                          "AcDbLight", "LIGHT", false, 0x1F2});
        registerDwgClass({DRW_Camera::kDwgClassNum, 0x401, "ACAD",
                          "AcDbCamera", "CAMERA", false, 0x1F2});
        registerDwgClass({DRW_SectionObject::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbSection",
                          "SECTIONOBJECT", false, 0x1F2});
        // HELIX (class 503): DRW_Helix::encodeDwg sets oType=503 but the class
        // was never registered, so libdxfrw's own reader (and ODA) dropped the
        // type — the spline body bytes were correct but unparseable. Mirror the
        // siblings above. (write-review #14)
        registerDwgClass({503, 0x401, "ACAD",
                          "AcDbHelix", "HELIX", false, 0x1F2});
        // MPOLYGON (class 518): classes 500..517 are already occupied by the
        // writer's typed entity/object set; appName follows the DXF CLASS
        // metadata and AutoCAD Map/Civil object naming.
        registerDwgClass({518, 0x401, "AcMPolygonObj15",
                          "AcDbMPolygon", "MPOLYGON", false, 0x1F2});
        registerDwgClass({519, 0x401, "ACAD",
                          "AcDbRadialDimensionLarge", "LARGE_RADIAL_DIMENSION",
                          false, 0x1F2});
        registerDwgClass({520, 0x401, "SCENEOE",
                          "AcDbSubDMesh", "MESH", false, 0x1F2});
        registerDwgClass({521, 0x401, "EXPRESS",
                          "AcDbRText", "RTEXT", false, 0x1F2});
        registerDwgClass({522, 0x401, "EXPRESS",
                          "AcDbArcAlignedText", "ARCALIGNEDTEXT", false,
                          0x1F2});
        registerDwgClass({523, 0x401, "ObjectDBX Classes",
                          "AcDbPdfReference", "PDFUNDERLAY", false, 0x1F2});
        registerDwgClass({524, 0x401, "ObjectDBX Classes",
                          "AcDbDgnReference", "DGNUNDERLAY", false, 0x1F2});
        registerDwgClass({525, 0x401, "ObjectDBX Classes",
                          "AcDbDwfReference", "DWFUNDERLAY", false, 0x1F2});
        registerDwgClass({DRW_PointCloud::kDwgClassNum, 0x401, "ACTION",
                          "AcDbPointCloud", "POINTCLOUD", false, 0x1F2});
        registerDwgClass({DRW_PointCloudEx::kDwgClassNum, 0x401, "ACTION",
                          "AcDbPointCloudEx", "POINTCLOUDEX", false, 0x1F2});
        registerDwgClass({DRW_NavisworksModel::kDwgClassNum, 0x401,
                          "ACTION", "AcDbNavisworksModel",
                          "NAVISWORKSMODEL", false, 0x1F2});
        registerDwgClass({DRW_PointCloudDef::kDwgClassNumDefinition, 0x401,
                          "ObjectDBX Classes", "AcDbPointCloudDef",
                          "POINTCLOUDDEFINITION", false, 0x1F3});
        registerDwgClass({DRW_PointCloudDef::kDwgClassNumDefinitionEx, 0x401,
                          "ObjectDBX Classes", "AcDbPointCloudDefEx",
                          "POINTCLOUDDEFINITIONEX", false, 0x1F3});
        registerDwgClass({DRW_PointCloudDef::kDwgClassNumReactor, 0x401,
                          "ObjectDBX Classes", "AcDbPointCloudDefReactor",
                          "POINTCLOUDDEFREACTOR", false, 0x1F3});
        registerDwgClass({DRW_PointCloudDef::kDwgClassNumReactorEx, 0x401,
                          "ObjectDBX Classes", "AcDbPointCloudDefReactorEx",
                          "POINTCLOUDDEFREACTOREX", false, 0x1F3});
        registerDwgClass({DRW_NavisworksModelDef::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbNavisworksModelDef",
                          "NAVISWORKSMODELDEF", false, 0x1F3});
        registerDwgClass({DRW_PointCloudColorMap::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbPointCloudColorMap",
                          "POINTCLOUDCOLORMAP", false, 0x1F3});
        // IMAGEDEF_REACTOR is a custom object class (fixed IMAGEDEF is type 102).
        // Ordinal must match DRW_ImageDefinitionReactor::kDwgClassNum (532).
        registerDwgClass({DRW_ImageDefinitionReactor::kDwgClassNum, 0x401, "ACAD",
                          "AcDbRasterImageDefReactor", "IMAGEDEF_REACTOR",
                          false, 0x1F3});
        registerDwgClass({DRW_SunStudy::kDwgClassNum, 0x401, "SCENEOE",
                          "AcDbSunStudy", "SUNSTUDY", false, 0x1F3});
        registerDwgClass({DRW_MotionPath::kDwgClassNum, 0x401, "ACTION",
                          "AcDbMotionPath", "MOTIONPATH", false, 0x1F3});
        registerDwgClass({DRW_CurvePath::kDwgClassNum, 0x401, "ACTION",
                          "AcDbCurvePath", "CURVEPATH", false, 0x1F3});
        registerDwgClass({DRW_PointPath::kDwgClassNum, 0x401, "ACTION",
                          "AcDbPointPath", "POINTPATH", false, 0x1F3});
        registerDwgClass({DRW_ObjectPtr::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbObjectPtr",
                          "OBJECT_PTR", false, 0x1F3});
        registerDwgClass({DRW_PartialViewingIndex::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbPartialViewingIndex",
                          "PARTIAL_VIEWING_INDEX", false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumEntry, 0x401,
                          "SCENEOE", "AcDbRenderEntry", "RENDERENTRY",
                          false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumEnvironment, 0x401,
                          "SCENEOE", "AcDbRenderEnvironment",
                          "RENDERENVIRONMENT", false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumGlobal, 0x401,
                          "SCENEOE", "AcDbRenderGlobal", "RENDERGLOBAL",
                          false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumSettings, 0x401,
                          "SCENEOE", "AcDbRenderSettings", "RENDERSETTINGS",
                          false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumMentalRay, 0x401,
                          "SCENEOE", "AcDbMentalRayRenderSettings",
                          "MENTALRAYRENDERSETTINGS", false, 0x1F3});
        registerDwgClass({DRW_RenderSettings::kDwgClassNumRapidRT, 0x401,
                          "ODBX", "AcDbRapidRTRenderSettings",
                          "RAPIDRTRENDERSETTINGS", false, 0x1F3});
        registerDwgClass({DRW_VisualStyle::kDwgClassNum, 0x401,
                          "ObjectDBX Classes", "AcDbVisualStyle",
                          "VISUALSTYLE", false, 0x1F3});
        registerDwgClass({DRW_3DLine::kDwgClassNum, 0x401, "ACTION",
                          "_3DLINE", "3DLINE", false, 0x1F2});
        registerDwgClass({DRW_DbColor::kDwgClassNum, 0x401, "ACTION",
                          "AcDbColor", "DBCOLOR", false, 0x1F3});
        // PLOTSETTINGS is a custom OBJECT class. Its ordinal is file-local,
        // so allocate a free slot instead of copying a producer's ordinal.
        registerDwgClass({nextFreeCustomClassNum(), 0, "ODBX",
                          "AcDbPlotSettings", "PLOTSETTINGS", false, 0x1F3});
    }

    virtual ~dwgWriter() = default;

    /// Emit the file-header stub at offsets 0..(0x19 + 9N + 1) with
    /// placeholder zeros for section addresses + sizes.  Final values
    /// are back-patched in `finalize()`.
    virtual bool writeFileHeaderStub() = 0;

    /// Emit the HEADER section (sentinel-bracketed bit-packed header
    /// variables).  Empty graphic: uses DRW_Header defaults.
    virtual bool writeDwgHeader() = 0;

    /// Emit the CLASSES section.  v1: empty (size = 0).
    virtual bool writeDwgClasses() = 0;

    /// Emit the object stream — the unsentinel'd byte region between
    /// the CLASSES section's end and the HANDLES section's start.
    /// Phase 3a/b emit nothing; Phase 3d adds control objects; Phase
    /// 3e adds table records; Phase 4+ adds entities.  Returns true
    /// on success.
    virtual bool writeDwgObjects() = 0;

    /// Emit the HANDLES (object map) section terminator.  v1 with
    /// zero entities just emits the empty-page terminator.
    virtual bool writeDwgHandles() = 0;

    /// Emit the 2NDHEADER block.
    virtual bool writeSecondHeader() = 0;

    /// Back-patch file-header section addresses + sizes, recompute
    /// the file-header CRC with seed-XOR adjust, and flush the
    /// in-memory accumulator to the output stream.
    virtual bool finalize() = 0;

    /// Encode a single entity into the object stream.  If the caller
    /// set `ent->handle` to a non-zero value, that handle is used as-is
    /// (round-trip preservation); otherwise a fresh handle is allocated
    /// via `m_handles.next()`.  Caller is responsible for setting the
    /// entity's `layerH.ref` and other type-specific fields before
    /// calling.  Returns true on success.
    virtual bool encodeEntity(DRW_Entity *ent) = 0;

    /// True when an entity EED/XDATA payload could not be represented without
    /// loss. dwgRW uses this sticky diagnostic to fail an export even if an
    /// interface callback ignored the individual entity-write result.
    virtual bool hasEntityEedWriteFailure() const { return false; }

    /// True when a typed OBJECT EED/XDATA payload could not be represented
    /// without loss. This is sticky for the same reason as entity EED failures.
    virtual bool hasObjectEedWriteFailure() const { return false; }

    /// Return whether this concrete writer has an encoder for the named
    /// DataStorage operation. Payload and reference validity remain the
    /// responsibility of the typed encoder itself.
    virtual bool canWriteDwgDataStorageOperation(
        DwgDataStorageWriterOperation operation) const {
        (void)operation;
        return false;
    }

    /// Register APPIDs referenced by queued table-record EED before the APPID
    /// table is emitted.
    virtual bool collectPendingTableEedAppIds() { return true; }

    /// Define a user-block. Allocates fresh handles for the Block_Record,
    /// Block, and ENDBLK trio, and appends its record handle to the deferred
    /// BLOCK_CONTROL list. Call beginBlockContent()/endBlockContent() to
    /// associate subsequently encoded entities with this definition.
    /// Returns the Block_Record handle (suitable for
    /// `DRW_Insert::blockRecH.ref`), or 0 on failure.
    virtual std::uint32_t defineBlock(const std::string& name,
                                const DRW_Coord& basePoint,
                                int insUnits = 0) = 0;

    /// Select a previously defined user block as the owner for subsequently
    /// encoded entities. Calls may not nest and must be balanced by
    /// endBlockContent() before emitDeferredBlockControl().
    virtual bool beginBlockContent(std::uint32_t blockRecordHandle) = 0;
    virtual bool endBlockContent() = 0;
    virtual bool hasActiveBlockContent() const { return false; }

    /// Emit BLOCK_CONTROL with the user-block list captured by all
    /// prior `defineBlock` calls.  Invoked by the orchestrator after
    /// `iface->writeBlocks()` so the BLOCK_CONTROL.numEntries reflects
    /// user blocks (+ the canonical 2 phantom modelspace/paperspace).
    virtual bool emitDeferredBlockControl() = 0;

    /// Accumulator (exposed for tests + for sibling classes that
    /// need byte-level inspection).  Reserved unless a test asks.
    const std::vector<std::uint8_t>& buffer() const { return m_buf.data(); }

    /// State needed to make a compound entity write atomic. The allocator is
    /// intentionally excluded: failed attempts leave their handles reserved,
    /// so a later write cannot reuse a handle that escaped to the caller.
    struct CompoundWriteCheckpoint {
        std::size_t bufferSize {0};
        std::size_t objectMapSize {0};
        std::size_t modelSpaceEntityCount {0};
        std::size_t paperSpaceEntityCount {0};
        std::size_t modelSpaceInsertCount {0};
        std::size_t paperSpaceInsertCount {0};
        std::vector<std::size_t> userBlockEntityCounts;
        std::vector<std::size_t> userBlockInsertCounts;

        // These ledgers are part of the serialized result.  A frame may have
        // updated them before a later owner/reference check rejects the
        // write, so restoring only bytes and vector lengths is insufficient.
        std::vector<DwgClassDefinition> classDefinitions;
        std::vector<DwgClassDefinition> classManifest;
        std::set<std::pair<std::uint16_t, std::uint32_t>>
            rawClassInstanceHandles;
        std::map<std::uint32_t, std::uint16_t> classInstanceHandleOwners;
        std::set<std::uint32_t> classInstanceEmittedHandles;
        std::map<std::uint16_t, std::uint32_t> classInstanceEmissionCounts;
        std::map<DwgRawClassIdentityKey, std::uint16_t> rawClassNumRemap;
        std::set<std::pair<std::uint16_t, std::uint32_t>> rawObjectOverrides;
        bool hasDwgClassConflict {false};
        bool dwgClassManifestPrepared {false};
        bool dwgClassesFrozen {false};
        bool frameWriteError {false};
        bool entityEedWriteFailure {false};
        bool objectEedWriteFailure {false};

        // dwgWriter15 owns these state machines, but all concrete DWG writer
        // versions use this checkpoint type for compound writes.
        std::set<std::uint32_t> emittedDwgTableRecordHandles;
        std::set<std::uint32_t> emittedDwgTableControlHandles;
        bool dwgTableRecordsComplete {false};
        std::vector<DRW::DwgNamedObjectDictionaryEntry>
            emittedDwgNamedObjectDictionaryEntries;
        bool dwgNamedObjectDictionaryComplete {false};
        std::vector<DRW::DwgBlockWriteRecord> emittedDwgBlockRecords;
        std::set<std::uint32_t> emittedDwgBlockEntityHandles;
        bool dwgBlockStructureComplete {false};
        std::size_t userBlockCount {0};
        // The name index is part of the block admission graph. Restoring the
        // vector without restoring this index makes a retry return a stale
        // handle whose PendingUserBlock was already rolled back.
        std::map<std::string, std::uint32_t> userBlockHandles;
        std::uint32_t activeUserBlockRecordHandle {0};
        bool blockControlEmitted {false};

        // Public transactions may surround deferred table admission, which
        // is intentionally outside the compact per-frame checkpoint.
        bool hasAdmissionState {false};
        DRW_WritingContext writingContext;
        std::vector<std::pair<std::uint32_t, DRW_LType>> pendingLTypes;
        std::vector<std::pair<std::uint32_t, DRW_Layer>> pendingLayers;
        DRW_Layer layer0;
        bool haveLayer0 {false};
        std::vector<std::pair<std::uint32_t, DRW_Textstyle>> pendingStyles;
        std::vector<std::pair<std::uint32_t, DRW_UCS>> pendingUcs;
        std::vector<std::pair<std::uint32_t, DRW_View>> pendingViews;
        std::vector<std::pair<std::uint32_t, DRW_Vport>> pendingVports;
        std::vector<std::pair<std::uint32_t, DRW_ViewportEntityHeader>>
            pendingViewportEntityHeaders;
        std::vector<std::pair<std::uint32_t, DRW_Dimstyle>> pendingDimstyles;
        std::vector<std::pair<std::uint32_t, DRW_AppId>> pendingAppIds;
        DRW_LType ltypeByBlock;
        DRW_LType ltypeByLayer;
        DRW_LType ltypeContinuous;
        bool haveLtypeByBlock {false};
        bool haveLtypeByLayer {false};
        bool haveLtypeContinuous {false};
        DRW_Textstyle standardStyle;
        bool haveStandardStyle {false};
        DRW_Vport activeVport;
        bool haveActiveVport {false};
        DRW_Dimstyle standardDimstyle;
        bool haveStandardDimstyle {false};
        std::vector<DRW::DwgNamedObjectDictionaryEntry>
            pendingDwgNamedObjectDictionaryEntries;
        DRW::DwgObjectFrameProvenance pendingDwgObjectFrameProvenance;
        DRW::DwgObjectFrameProvenance currentDwgObjectFrameProvenance;
    };

    virtual CompoundWriteCheckpoint checkpointCompoundWrite() const {
        CompoundWriteCheckpoint checkpoint;
        checkpoint.bufferSize = m_buf.size();
        checkpoint.classDefinitions = m_dwgClassDefinitions;
        checkpoint.classManifest = m_dwgClassManifest;
        checkpoint.rawClassInstanceHandles = m_rawClassInstanceHandles;
        checkpoint.classInstanceHandleOwners =
            m_dwgClassInstanceHandleOwners;
        checkpoint.classInstanceEmittedHandles =
            m_dwgClassInstanceEmittedHandles;
        checkpoint.classInstanceEmissionCounts =
            m_dwgClassInstanceEmissionCounts;
        checkpoint.rawClassNumRemap = m_rawClassNumRemap;
        checkpoint.rawObjectOverrides = m_rawObjectOverrides;
        checkpoint.hasDwgClassConflict = m_hasDwgClassConflict;
        checkpoint.dwgClassManifestPrepared = m_dwgClassManifestPrepared;
        checkpoint.dwgClassesFrozen = m_dwgClassesFrozen;
        checkpoint.pendingDwgObjectFrameProvenance =
            m_pendingDwgObjectFrameProvenance;
        checkpoint.currentDwgObjectFrameProvenance =
            m_currentDwgObjectFrameProvenance;
        return checkpoint;
    }

    /// Full checkpoint for a caller-owned transaction. Deferred table
    /// admission is included by dwgWriter15; frame-local checkpoints remain
    /// compact so entity encoding does not copy all pending table records.
    virtual CompoundWriteCheckpoint checkpointPublicTransaction() const {
        return checkpointCompoundWrite();
    }

    virtual void rollbackCompoundWrite(
        const CompoundWriteCheckpoint& checkpoint) {
        if (checkpoint.bufferSize <= m_buf.size())
            m_buf.truncate(checkpoint.bufferSize);
        m_dwgClassDefinitions = checkpoint.classDefinitions;
        m_dwgClassManifest = checkpoint.classManifest;
        m_rawClassInstanceHandles = checkpoint.rawClassInstanceHandles;
        m_dwgClassInstanceHandleOwners = checkpoint.classInstanceHandleOwners;
        m_dwgClassInstanceEmittedHandles =
            checkpoint.classInstanceEmittedHandles;
        m_dwgClassInstanceEmissionCounts =
            checkpoint.classInstanceEmissionCounts;
        m_rawClassNumRemap = checkpoint.rawClassNumRemap;
        m_rawObjectOverrides = checkpoint.rawObjectOverrides;
        m_hasDwgClassConflict = checkpoint.hasDwgClassConflict;
        m_dwgClassManifestPrepared = checkpoint.dwgClassManifestPrepared;
        m_dwgClassesFrozen = checkpoint.dwgClassesFrozen;
        m_pendingDwgObjectFrameProvenance =
            checkpoint.pendingDwgObjectFrameProvenance;
        m_currentDwgObjectFrameProvenance =
            checkpoint.currentDwgObjectFrameProvenance;
    }

    /// Bind provenance to the next object frame. The binding is consumed by
    /// beginObject() or raw replay and is therefore never inherited by an
    /// unrelated subsequent write.
    void setDwgObjectFrameProvenance(
        std::uint16_t classNumber, std::uint16_t writerOperation,
        std::uint64_t admissionToken) {
        m_pendingDwgObjectFrameProvenance = {
            classNumber, writerOperation, admissionToken};
    }

    /// Remove a pending binding after a failed or non-DataStorage operation.
    void clearDwgObjectFrameProvenance() {
        m_pendingDwgObjectFrameProvenance = {};
    }

    /// Latch a failed frame when a public wrapper cannot acquire the writer's
    /// completion receipt after encodeEntity() returned success.
    virtual void markObjectWriteFailure() {}

    /// Resolve the byte offset of the start of the OBJECTS data region within
    /// m_buf. writeDwgHandles subtracts this from each object's m_buf offset
    /// so the HANDLES section stores section-relative positions. For R2000
    /// (whole-file buffer) the base is 0; R2004+ points past the HEADER +
    /// CLASSES sections that precede the object stream. A missing or
    /// unrepresentable section map is an error, not an implicit base of zero.
    virtual bool objectBaseOffset(std::uint32_t& offset) const {
        offset = 0;
        return true;
    }

    /// First-available user handle from the allocator.  `dwgRW::write`
    /// uses this to auto-populate `DRW_Header::handSeed` for fresh
    /// documents where the caller did not set one explicitly — without
    /// it, the encoder emits null HANDSEED and AutoCAD refreshes on
    /// first open, marking the file modified.  See [Risk 4j].
    std::uint32_t highWaterHandle() const { return m_handles.current(); }

    /// Validate the handles published in the object map and return the
    /// largest emitted handle.  The base writer has no object map of its own;
    /// concrete object-stream writers override this hook.  Keeping the hook
    /// here makes HANDSEED validation happen before a version-specific
    /// HANDLES encoder can publish a stale high-water mark.
    virtual bool validateEmittedHandleMap(std::uint32_t& maxHandle) const {
        maxHandle = 0;
        return true;
    }

    /// Patch the already-emitted HEADER field after all object handles have
    /// been minted. The fixed four-byte H payload keeps section offsets stable.
    bool finalizeHeaderHandseed() {
        if (m_header == nullptr
            || m_headerHandseedBitOffset == DRW_Header::kInvalidDwgHandseedBitOffset)
            return false;
        std::uint32_t maxHandle = 0;
        if (!validateEmittedHandleMap(maxHandle)
            || maxHandle == (std::numeric_limits<std::uint32_t>::max)())
            return false;
        const std::uint32_t requiredSeed = maxHandle + 1u;
        const std::uint32_t seed = std::max(
            std::max(m_header->getHandSeed(), highWaterHandle()), requiredSeed);
        if (seed <= maxHandle)
            return false;
        const std::uint8_t bytes[] = {
            static_cast<std::uint8_t>(seed >> 24),
            static_cast<std::uint8_t>(seed >> 16),
            static_cast<std::uint8_t>(seed >> 8),
            static_cast<std::uint8_t>(seed)
        };
        if (!m_buf.patchRawBytesAtBit(m_headerHandseedBitOffset, bytes,
                                      sizeof(bytes)))
            return false;
        m_header->setHandSeed(seed);
        return true;
    }

    /// Allocate a fresh user handle and return it without encoding any
    /// object. Used by writePolyline to reserve the parent before its
    /// VERTEX handles, preserving deterministic aggregate ownership.
    std::uint32_t allocNextHandle() { return m_handles.next(); }

    /// Reserve a specific handle so `next()` never returns it again.
    void reserveHandle(std::uint32_t h) { m_handles.reserve(h); }

    virtual bool addRawDwgSection(const DRW_RawDwgSection& section) {
        (void)section;
        return false;
    }

    /// Remap a raw class using its complete identity. The source ordinal is
    /// not sufficient because malformed merged metadata can reuse one ordinal
    /// for multiple opaque classes.
    std::uint16_t remappedRawClassNum(
        const DRW_UnsupportedObject& object) const {
        if (!object.m_isCustomClass || object.m_objectType < 500)
            return static_cast<std::uint16_t>(object.m_objectType);
        const std::uint16_t sourceClassNum =
            static_cast<std::uint16_t>(object.m_objectType);
        const std::string className = object.m_className.empty()
            ? object.m_recordName : object.m_className;
        const std::string recordName = object.m_recordName.empty()
            ? object.m_className : object.m_recordName;
        const DwgRawClassIdentityKey key{
            sourceClassNum, className, recordName,
            static_cast<std::uint16_t>(object.m_isEntity ? 0x1F2 : 0x1F3)};
        auto it = m_rawClassNumRemap.find(key);
        return it != m_rawClassNumRemap.end() ? it->second : sourceClassNum;
    }

    /// Return the writer-local ordinal assigned to a typed class identity.
    /// Custom class ordinals are file-local; the fallback is used when a
    /// caller asks before registration.
    std::uint16_t typedDwgClassNum(const std::string& className,
                                   const std::string& recordName,
                                   std::uint16_t fallback) const {
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_className == className
                && definition.m_recordName == recordName)
                return definition.m_classNum;
        }
        return fallback;
    }

    /// Return the class ordinal admitted for a handle-associated OBJECTS
    /// instance.  This is the authoritative class/frame link after typed or
    /// raw class registration, including raw class-number remapping.
    bool getDwgClassInstanceClass(std::uint32_t handle,
                                  std::uint16_t& classNumber) const {
        classNumber = 0;
        if (handle == 0)
            return false;
        const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
        if (owner == m_dwgClassInstanceHandleOwners.end())
            return false;
        classNumber = owner->second;
        return classNumber != 0;
    }

    std::uint16_t nextFreeCustomClassNum() const {
        std::uint16_t candidate = 500;
        for (;;) {
            bool taken = false;
            for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
                if (existing.m_classNum == candidate) {
                    taken = true;
                    break;
                }
            }
            if (!taken)
                return candidate;
            if (candidate == 0xFFFF)
                return 0;
            ++candidate;
        }
    }

    bool registerRawObjectClass(const DRW_UnsupportedObject& object) {
        if (object.m_isCustomClass
            && (object.m_objectType < 500
                || static_cast<std::uint64_t>(object.m_objectType)
                       > (std::numeric_limits<std::uint16_t>::max)()))
            return false;
        if (!object.m_isCustomClass || object.m_objectType < 500) {
            // R2000's viewport-header control is emitted canonically unless a
            // validated raw carrier explicitly replaces that fixed record.
            if (object.m_objectType == DRW_ViewportEntityHeader::kDwgControlType
                && object.m_handle == DRW_ViewportEntityHeader::kDwgControlHandle) {
                m_rawObjectOverrides.emplace(
                    static_cast<std::uint16_t>(object.m_objectType),
                    object.m_handle);
            }
            return true;
        }
        DwgClassDefinition definition;
        definition.m_classNum = static_cast<std::uint16_t>(object.m_objectType);
        if (object.m_hasClassDefinition) {
            definition.m_proxyFlag = object.m_classProxyFlag;
            definition.m_appName = object.m_classAppName;
            definition.m_wasProxy = object.m_classWasProxy;
            definition.m_entityFlagRaw = object.m_classEntityFlagRaw;
            definition.m_dwgVersion = object.m_classDwgVersion;
            definition.m_maintenanceVersion = object.m_classMaintenanceVersion;
            definition.m_unknown1 = object.m_classUnknown1;
            definition.m_unknown2 = object.m_classUnknown2;
        } else {
            definition.m_proxyFlag = 0x401;
            definition.m_appName = "ACAD";
            definition.m_entityFlagRaw = object.m_isEntity ? 0x1F2 : 0x1F3;
        }
        definition.m_className = object.m_className.empty()
            ? object.m_recordName
            : object.m_className;
        definition.m_recordName = object.m_recordName.empty()
            ? object.m_className
            : object.m_recordName;
        // item_class_id: 0x1F2 for entities, 0x1F3 for objects (ODA/libreDWG
        // decode.c "1f2 for entities, 1f3 for objects"). The reader maps
        // 0x1F2->entity and everything else->object.
        const std::uint16_t sourceClassNum = definition.m_classNum;
        const DwgRawClassIdentityKey identityKey{
            sourceClassNum, definition.m_className, definition.m_recordName,
            definition.m_entityFlagRaw};
        const auto previousRemap = m_rawClassNumRemap.find(identityKey);
        const bool hadPreviousRemap = previousRemap != m_rawClassNumRemap.end();
        const std::uint16_t previousRemapValue = hadPreviousRemap
            ? previousRemap->second : 0;
        const bool previousClassConflict = m_hasDwgClassConflict;

        // Resolve ordinal collisions with a different identity by remapping to
        // a free class number (writer-local). Do not set m_hasDwgClassConflict
        // for remappable raw classes — that aborted whole-file saves.
        for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
            if (existing.m_classNum != definition.m_classNum)
                continue;
            if (sameDwgClassIdentity(existing, definition))
                break;  // same identity — fall through to registerDwgClass
            // Prefer an already-registered slot with the same identity.
            bool reused = false;
            for (const DwgClassDefinition& other : m_dwgClassDefinitions) {
                if (sameDwgClassIdentity(other, definition)) {
                    m_rawClassNumRemap[identityKey] = other.m_classNum;
                    definition.m_classNum = other.m_classNum;
                    reused = true;
                    break;
                }
            }
            if (!reused) {
                const std::uint16_t freeNum = nextFreeCustomClassNum();
                if (freeNum < 500)
                    return false;
                m_rawClassNumRemap[identityKey] = freeNum;
                definition.m_classNum = freeNum;
            }
            break;
        }

        bool insertedInstance = false;
        if (!stageDwgClassInstance(definition.m_classNum, object.m_handle,
                                   insertedInstance)) {
            if (hadPreviousRemap)
                m_rawClassNumRemap[identityKey] = previousRemapValue;
            else
                m_rawClassNumRemap.erase(identityKey);
            m_hasDwgClassConflict = previousClassConflict;
            return false;
        }
        if (insertedInstance) {
            definition.m_instanceCount = 1;
        }
        if (registerDwgClass(definition))
            return true;

        if (insertedInstance)
            rollbackDwgClassInstance(definition.m_classNum, object.m_handle);
        if (hadPreviousRemap)
            m_rawClassNumRemap[identityKey] = previousRemapValue;
        else
            m_rawClassNumRemap.erase(identityKey);
        m_hasDwgClassConflict = previousClassConflict;
        return false;
    }

    // A class may be registered during CLASSES before its raw body is
    // validated. Remove that pending instance when replay is intentionally
    // skipped, so CLASSES does not advertise a frame that OBJECTS lacks.
    void rollbackRawObjectClassInstance(
        const DRW_UnsupportedObject& object) {
        if (!object.m_isCustomClass || object.m_objectType < 500
            || object.m_handle == 0)
            return;
        const std::uint16_t classNum = remappedRawClassNum(object);
        const auto instance = m_rawClassInstanceHandles.find(
            {classNum, object.m_handle});
        if (instance == m_rawClassInstanceHandles.end())
            return;
        m_rawClassInstanceHandles.erase(instance);
        m_dwgClassInstanceHandleOwners.erase(object.m_handle);
        for (DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_classNum != classNum)
                continue;
            if (definition.m_instanceCount > 0)
                --definition.m_instanceCount;
            break;
        }
    }

    bool hasRawObjectOverride(std::uint16_t objectType,
                              std::uint32_t handle) const {
        return m_rawObjectOverrides.count({objectType, handle}) != 0;
    }

    bool registerSunObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_Sun::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "SCENEOE";
        definition.m_className = "AcDbSun";
        definition.m_recordName = "SUN";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerTvDevicePropertiesObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_TvDeviceProperties::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACTION";
        definition.m_className = "AcDbTvDeviceProperties";
        definition.m_recordName = "TVDEVICEPROPERTIES";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerVxControlObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_VxControl::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbVxControl";
        definition.m_recordName = "VXCONTROL";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerVxTableRecordObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_VxTableRecord::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbVxTableRecord";
        definition.m_recordName = "VXTABLERECORD";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerMLeaderStyleObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_MLeaderStyle::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACDB_MLEADERSTYLE_CLASS";
        definition.m_className = "AcDbMLeaderStyle";
        definition.m_recordName = "MLEADERSTYLE";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerTableStyleObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_TableStyle::kDwgClassNum;
        definition.m_proxyFlag = 0xFFF;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbTableStyle";
        definition.m_recordName = "TABLESTYLE";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerMaterialObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_Material::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbMaterial";
        definition.m_recordName = "MATERIAL";
        definition.m_entityFlagRaw = 0x1F3;  // object class
        return registerTypedObjectClass(definition, handle);
    }

    bool registerDbColorObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_DbColor::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACTION";
        definition.m_className = "AcDbColor";
        definition.m_recordName = "DBCOLOR";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerPlotSettingsObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = typedDwgClassNum(
            "AcDbPlotSettings", "PLOTSETTINGS", 0);
        definition.m_proxyFlag = 0;
        definition.m_appName = "ODBX";
        definition.m_className = "AcDbPlotSettings";
        definition.m_recordName = "PLOTSETTINGS";
        definition.m_entityFlagRaw = 0x1F3;
        definition.m_instanceCount = 1;
        if (definition.m_classNum < 500)
            return false;
        if (handle != 0)
            return registerTypedObjectClass(definition, handle);
        return registerDwgClass(definition);
    }

    bool registerLightListObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        // Custom class ordinals are file-local. Allocate this one instead of
        // assuming an ordinal from a particular producer's CLASSES section.
        definition.m_classNum = nextFreeCustomClassNum();
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "SCENEOE";
        definition.m_className = "AcDbLightList";
        definition.m_recordName = "LIGHTLIST";
        definition.m_entityFlagRaw = 0x1F3;
        return definition.m_classNum >= 500
            && registerTypedObjectClass(definition, handle);
    }

    bool registerBackgroundObjectClass(DRW_Background::Kind kind,
                                        std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "SCENEOE";
        definition.m_entityFlagRaw = 0x1F3;
        switch (kind) {
        case DRW_Background::Skylight:
            definition.m_classNum = DRW_Background::kDwgClassNumSkylight;
            definition.m_className = "AcDbSkyBackground";
            definition.m_recordName = "SKYLIGHTBACKGROUND";
            break;
        case DRW_Background::Solid:
            definition.m_classNum = DRW_Background::kDwgClassNumSolid;
            definition.m_className = "AcDbSolidBackground";
            definition.m_recordName = "SOLIDBACKGROUND";
            break;
        case DRW_Background::Gradient:
            definition.m_classNum = DRW_Background::kDwgClassNumGradient;
            definition.m_className = "AcDbGradientBackground";
            definition.m_recordName = "GRADIENTBACKGROUND";
            break;
        case DRW_Background::GroundPlane:
            definition.m_classNum = DRW_Background::kDwgClassNumGroundPlane;
            definition.m_className = "AcDbGroundPlaneBackground";
            definition.m_recordName = "GROUNDPLANEBACKGROUND";
            break;
        case DRW_Background::Ibl:
            definition.m_classNum = DRW_Background::kDwgClassNumIbl;
            definition.m_className = "AcDbIBLBackground";
            definition.m_recordName = "IBLBACKGROUND";
            break;
        case DRW_Background::Image:
            definition.m_classNum = DRW_Background::kDwgClassNumImage;
            definition.m_className = "AcDbImageBackground";
            definition.m_recordName = "IMAGEBACKGROUND";
            break;
        }
        return registerTypedObjectClass(definition, handle);
    }

    bool registerSunStudyObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_SunStudy::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "SCENEOE";
        definition.m_className = "AcDbSunStudy";
        definition.m_recordName = "SUNSTUDY";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerMotionPathObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_MotionPath::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACTION";
        definition.m_className = "AcDbMotionPath";
        definition.m_recordName = "MOTIONPATH";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerCurvePathObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_CurvePath::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACTION";
        definition.m_className = "AcDbCurvePath";
        definition.m_recordName = "CURVEPATH";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerPointPathObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_PointPath::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACTION";
        definition.m_className = "AcDbPointPath";
        definition.m_recordName = "POINTPATH";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerPartialViewingIndexObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_PartialViewingIndex::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbPartialViewingIndex";
        definition.m_recordName = "PARTIAL_VIEWING_INDEX";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerObjectPtrObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_ObjectPtr::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbObjectPtr";
        definition.m_recordName = "OBJECT_PTR";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerRenderSettingsObjectClass(
        DRW_RenderSettings::Kind kind, std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "SCENEOE";
        definition.m_entityFlagRaw = 0x1F3;
        switch (kind) {
        case DRW_RenderSettings::Entry:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumEntry;
            definition.m_className = "AcDbRenderEntry";
            definition.m_recordName = "RENDERENTRY";
            break;
        case DRW_RenderSettings::Environment:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumEnvironment;
            definition.m_className = "AcDbRenderEnvironment";
            definition.m_recordName = "RENDERENVIRONMENT";
            break;
        case DRW_RenderSettings::Global:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumGlobal;
            definition.m_className = "AcDbRenderGlobal";
            definition.m_recordName = "RENDERGLOBAL";
            break;
        case DRW_RenderSettings::Settings:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumSettings;
            definition.m_appName = "SCENEOE";
            definition.m_className = "AcDbRenderSettings";
            definition.m_recordName = "RENDERSETTINGS";
            break;
        case DRW_RenderSettings::MentalRay:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumMentalRay;
            definition.m_appName = "SCENEOE";
            definition.m_className = "AcDbMentalRayRenderSettings";
            definition.m_recordName = "MENTALRAYRENDERSETTINGS";
            break;
        case DRW_RenderSettings::RapidRT:
            definition.m_classNum = DRW_RenderSettings::kDwgClassNumRapidRT;
            definition.m_appName = "ODBX";
            definition.m_className = "AcDbRapidRTRenderSettings";
            definition.m_recordName = "RAPIDRTRENDERSETTINGS";
            break;
        default:
            return false;
        }
        return registerTypedObjectClass(definition, handle);
    }

    bool registerVisualStyleObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_VisualStyle::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbVisualStyle";
        definition.m_recordName = "VISUALSTYLE";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerEvaluationGraphObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_EvaluationGraph::kDwgClassNum;
        definition.m_proxyFlag = 0x481;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbEvalGraph";
        definition.m_recordName = "ACAD_EVALUATION_GRAPH";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerDimensionAssociationObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_DimensionAssociation::kDwgClassNum;
        definition.m_proxyFlag = 0;
        definition.m_appName =
            "AcDbDimAssoc|Product Desc:     AcDim ARX App For Dimension|"
            "Company:          Autodesk, Inc.|WEB Address:      www.autodesk.com";
        definition.m_className = "AcDbDimAssoc";
        definition.m_recordName = "DIMASSOC";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerRasterVariablesObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_RasterVariables::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ISM";
        definition.m_className = "AcDbRasterVariables";
        definition.m_recordName = "RASTERVARIABLES";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerWipeoutVariablesObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_WipeoutVariables::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "WipeOut";
        definition.m_className = "AcDbWipeoutVariables";
        definition.m_recordName = "WIPEOUTVARIABLES";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerWipeoutEntityClass() {
        // Kept as a source-compatible no-op.  WIPEOUT is fixed type 1109 and
        // therefore has no CLASSES entry; WIPEOUTVARIABLES remains custom.
        return true;
    }

    bool registerImageDefReactorObjectClass(std::uint32_t handle = 0) {
        // The convenience IMAGE writer creates this reactor while emitting
        // entities, after the CLASSES section has been frozen. The standard
        // class is declared by the writer constructor, so a late instance
        // notification must not reject an otherwise valid IMAGE record.
        if (m_dwgClassesFrozen
            && hasDwgClassDefinition(DRW_ImageDefinitionReactor::kDwgClassNum))
            return true;
        DwgClassDefinition definition;
        definition.m_classNum = DRW_ImageDefinitionReactor::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbRasterImageDefReactor";
        definition.m_recordName = "IMAGEDEF_REACTOR";
        definition.m_entityFlagRaw = 0x1F3;  // object class
        return registerTypedObjectClass(definition, handle);
    }

    bool registerPointCloudDefObjectClass(DRW_PointCloudDef::Kind kind,
                                          std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_entityFlagRaw = 0x1F3;
        switch (kind) {
        case DRW_PointCloudDef::Definition:
            definition.m_classNum = DRW_PointCloudDef::kDwgClassNumDefinition;
            definition.m_className = "AcDbPointCloudDef";
            definition.m_recordName = "POINTCLOUDDEFINITION";
            break;
        case DRW_PointCloudDef::DefinitionEx:
            definition.m_classNum = DRW_PointCloudDef::kDwgClassNumDefinitionEx;
            definition.m_className = "AcDbPointCloudDefEx";
            definition.m_recordName = "POINTCLOUDDEFINITIONEX";
            break;
        case DRW_PointCloudDef::Reactor:
            definition.m_classNum = DRW_PointCloudDef::kDwgClassNumReactor;
            definition.m_className = "AcDbPointCloudDefReactor";
            definition.m_recordName = "POINTCLOUDDEFREACTOR";
            break;
        case DRW_PointCloudDef::ReactorEx:
            definition.m_classNum = DRW_PointCloudDef::kDwgClassNumReactorEx;
            definition.m_className = "AcDbPointCloudDefReactorEx";
            definition.m_recordName = "POINTCLOUDDEFREACTOREX";
            break;
        default:
            return false;
        }
        return registerTypedObjectClass(definition, handle);
    }

    bool registerNavisworksModelDefObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_NavisworksModelDef::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbNavisworksModelDef";
        definition.m_recordName = "NAVISWORKSMODELDEF";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerPointCloudColorMapObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_PointCloudColorMap::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbPointCloudColorMap";
        definition.m_recordName = "POINTCLOUDCOLORMAP";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerGeoDataObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_GeoData::kDwgClassNum;
        definition.m_proxyFlag = 0xFFF;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = "AcDbGeoData";
        definition.m_recordName = "GEODATA";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerSpatialFilterObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_SpatialFilter::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbSpatialFilter";
        definition.m_recordName = "SPATIAL_FILTER";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    // PR 8d.2a — five small no-storage OBJECTS families.  All are custom-class
    // (≥ 500); recName / className strings follow the dwgreader.cpp dispatch
    // (case-sensitive look-up against classesmap).
    bool registerScaleObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_Scale::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbScale";
        definition.m_recordName = "SCALE";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerIDBufferObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_IDBuffer::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbIdBuffer";
        definition.m_recordName = "IDBUFFER";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerLayerIndexObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_LayerIndex::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbLayerIndex";
        definition.m_recordName = "LAYER_INDEX";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerSpatialIndexObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_SpatialIndex::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbSpatialIndex";
        definition.m_recordName = "SPATIAL_INDEX";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerDictionaryVarObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_DictionaryVar::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbDictionaryVar";
        definition.m_recordName = "DICTIONARYVAR";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    // PR 8d.2b — four larger no-storage OBJECTS families.  Same shape as
    // PR 8d.2a; recName / className strings follow the dwgreader.cpp dispatch
    // (case-sensitive look-up against classesmap).
    bool registerDictionaryWithDefaultObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_DictionaryWithDefault::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbDictionaryWithDefault";
        definition.m_recordName = "ACDBDICTIONARYWDFLT";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerSortEntsTableObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_SortEntsTable::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbSortentsTable";
        definition.m_recordName = "SORTENTSTABLE";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerFieldListObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_FieldList::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbFieldList";
        definition.m_recordName = "FIELDLIST";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerFieldObjectClass(std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = DRW_Field::kDwgClassNum;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ACAD";
        definition.m_className = "AcDbField";
        definition.m_recordName = "FIELD";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        return registerTypedObjectClass(definition, handle);
    }

    bool registerSectionObjectClass(DRW_Section::Kind kind,
                                    std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_classNum = kind == DRW_Section::Manager
            ? DRW_Section::kDwgClassNumManager
            : DRW_Section::kDwgClassNumSettings;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_className = kind == DRW_Section::Manager
            ? "AcDbSectionManager" : "AcDbSectionSettings";
        definition.m_recordName = kind == DRW_Section::Manager
            ? "SECTION_MANAGER" : "SECTION_SETTINGS";
        definition.m_entityFlagRaw = 0x1F3;
        return registerTypedObjectClass(definition, handle);
    }

    bool registerUnderlayDefinitionObjectClass(DRW_UnderlayDefinition::Kind kind,
                                               std::uint32_t handle = 0) {
        DwgClassDefinition definition;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_entityFlagRaw = 0x1F3;  // object class (ODA item_class_id)
        switch (kind) {
        case DRW_UnderlayDefinition::DGN:
            definition.m_classNum = DRW_UnderlayDefinition::kDwgClassNumDgn;
            definition.m_className = "AcDbDgnDefinition";
            definition.m_recordName = "DGNDEFINITION";
            break;
        case DRW_UnderlayDefinition::DWF:
            definition.m_classNum = DRW_UnderlayDefinition::kDwgClassNumDwf;
            definition.m_className = "AcDbDwfDefinition";
            definition.m_recordName = "DWFDEFINITION";
            break;
        case DRW_UnderlayDefinition::PDF:
            definition.m_classNum = DRW_UnderlayDefinition::kDwgClassNumPdf;
            definition.m_className = "AcDbPdfDefinition";
            definition.m_recordName = "PDFDEFINITION";
            break;
        default:
            return false;
        }
        return registerTypedObjectClass(definition, handle);
    }

    /// Register UNDERLAY *entity* class (PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY).
    /// Required before encodeEntity can emit a custom-class underlay body —
    /// definition registration alone is not enough (read-side classes both).
    bool registerUnderlayEntityClass(DRW_Underlay::Kind kind) {
        DwgClassDefinition definition;
        definition.m_proxyFlag = 0x401;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_entityFlagRaw = 0x1F2;  // entity class
        switch (kind) {
        case DRW_Underlay::DGN:
            definition.m_classNum = DRW_Underlay::kDwgClassNumDgn;
            definition.m_className = "AcDbDgnReference";
            definition.m_recordName = "DGNUNDERLAY";
            break;
        case DRW_Underlay::DWF:
            definition.m_classNum = DRW_Underlay::kDwgClassNumDwf;
            definition.m_className = "AcDbDwfReference";
            definition.m_recordName = "DWFUNDERLAY";
            break;
        case DRW_Underlay::PDF:
        default:
            definition.m_classNum = DRW_Underlay::kDwgClassNumPdf;
            definition.m_className = "AcDbPdfReference";
            definition.m_recordName = "PDFUNDERLAY";
            break;
        }
        return registerDwgClass(definition);
    }

    bool registerNavisworksModelEntityClass() {
        return registerDwgClass({DRW_NavisworksModel::kDwgClassNum, 0x401,
                                 "ACTION", "AcDbNavisworksModel",
                                 "NAVISWORKSMODEL", false, 0x1F2});
    }

    bool registerSurfaceEntityClass(DRW_Surface *surface) {
        if (surface == nullptr)
            return false;

        DwgClassDefinition definition;
        definition.m_appName = "ObjectDBX Classes";
        definition.m_entityFlagRaw = 0x1F2;
        switch (surface->eType) {
        case DRW::EXTRUDEDSURFACE:
            definition.m_classNum = DRW_ExtrudedSurface::kDwgClassNum;
            definition.m_className = "AcDbExtrudedSurface";
            definition.m_recordName = "EXTRUDEDSURFACE";
            break;
        case DRW::LOFTEDSURFACE:
            definition.m_classNum = DRW_LoftedSurface::kDwgClassNum;
            definition.m_className = "AcDbLoftedSurface";
            definition.m_recordName = "LOFTEDSURFACE";
            break;
        case DRW::REVOLVEDSURFACE:
            definition.m_classNum = DRW_RevolvedSurface::kDwgClassNum;
            definition.m_className = "AcDbRevolvedSurface";
            definition.m_recordName = "REVOLVEDSURFACE";
            break;
        case DRW::SWEPTSURFACE:
            definition.m_classNum = DRW_SweptSurface::kDwgClassNum;
            definition.m_className = "AcDbSweptSurface";
            definition.m_recordName = "SWEPTSURFACE";
            break;
        case DRW::PLANESURFACE:
            definition.m_classNum = DRW_PlaneSurface::kDwgClassNum;
            definition.m_className = "AcDbPlaneSurface";
            definition.m_recordName = "PLANESURFACE";
            definition.m_proxyFlag = 0x0FFF;
            break;
        case DRW::NURBSURFACE:
            definition.m_classNum = DRW_NurbsSurface::kDwgClassNum;
            definition.m_className = "AcDbNurbSurface";
            definition.m_recordName = "NURBSURFACE";
            break;
        default:
            return false;
        }
        if (!registerTypedEntityClass(definition))
            return false;
        surface->setDwgClassNum(typedDwgClassNum(
            definition.m_className, definition.m_recordName,
            definition.m_classNum));
        return true;
    }

    bool hasDwgClassDefinition(std::uint16_t classNum) const {
        return std::any_of(m_dwgClassDefinitions.begin(), m_dwgClassDefinitions.end(),
                           [classNum](const DwgClassDefinition& definition) {
                               return definition.m_classNum == classNum;
                           });
    }

    bool hasDwgClassConflict() const { return m_hasDwgClassConflict; }

    /// Record one successfully emitted instance of a custom class.  Most
    /// classes are matched to a reserved object handle automatically at the
    /// frame commit point; handle-less registrations such as PLOTSETTINGS use
    /// this explicit hook after their frame has committed.
    bool noteDwgClassInstanceEmitted(std::uint16_t classNum) {
        if (!hasDwgClassDefinition(classNum))
            return false;
        ++m_dwgClassInstanceEmissionCounts[classNum];
        return true;
    }

    /// Verify that every declared custom-class instance was emitted exactly
    /// once.  This runs after OBJECTS, before the final file sections are
    /// assembled, so a failed object cannot leave stale CLASSES counts.
    bool validateDwgClassInstanceCounts() const {
        if (!m_rawClassInstanceHandles.empty())
            return false;
        for (const auto& owner : m_dwgClassInstanceHandleOwners) {
            const DwgClassDefinition* definition =
                findDwgClassDefinition(owner.second);
            if (definition == nullptr || !isDwgClassEnabled(*definition))
                return false;
            if (m_rawClassInstanceHandles.count({owner.second, owner.first}) == 0
                && m_dwgClassInstanceEmittedHandles.count(owner.first) == 0)
                return false;
        }
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (!isDwgClassEnabled(definition))
                continue;
            const auto it = m_dwgClassInstanceEmissionCounts.find(
                definition.m_classNum);
            const std::uint32_t emitted = it == m_dwgClassInstanceEmissionCounts.end()
                ? 0u : it->second;
            if (definition.m_instanceCount < 0
                || emitted != static_cast<std::uint32_t>(
                                  definition.m_instanceCount)) {
                return false;
            }
        }
        return true;
    }

    /// Verify that every handle-associated custom class instance has exactly
    /// one physical object-map frame after the OBJECTS phase. Handle-less
    /// classes such as PLOTSETTINGS use the explicit count ledger above.
    virtual bool validateDwgClassInstanceFrames() const { return true; }

    /// Capture the complete class set before any file section is serialized.
    /// Placeholder entries are derived from this bounded 16-bit class range at
    /// emission time rather than retained in the manifest.
    bool prepareDwgClassManifest() {
        if (m_dwgClassesFrozen || m_hasDwgClassConflict)
            return false;
        std::set<std::uint16_t> classNumbers;
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_instanceCount < 0)
                return false;
            if (!isDwgClassEnabled(definition))
                continue;
            if (!classNumbers.insert(definition.m_classNum).second)
                return false;
        }
        for (const auto& instance : m_rawClassInstanceHandles) {
            const DwgClassDefinition* definition =
                findDwgClassDefinition(instance.first);
            if (definition == nullptr || !isDwgClassEnabled(*definition))
                return false;
            const auto owner = m_dwgClassInstanceHandleOwners.find(
                instance.second);
            if (owner == m_dwgClassInstanceHandleOwners.end()
                || owner->second != instance.first)
                return false;
        }
        m_dwgClassManifest = sortedDwgClassDefinitions();
        m_dwgClassManifestPrepared = true;
        return true;
    }

    /// Confirm that the class registration graph has not changed after the
    /// caller's pre-CLASSES admission pass.
    bool validateDwgClassManifest() const {
        return m_dwgClassManifestPrepared && !m_hasDwgClassConflict
            && sameDwgClassManifest(sortedDwgClassDefinitions(),
                                    m_dwgClassManifest);
    }

    /// Freeze the class table after its section has been emitted.  Repeated
    /// registration of an already-emitted, zero-instance definition remains
    /// valid; new definitions or instance counts cannot be represented.
    void freezeDwgClasses() { m_dwgClassesFrozen = true; }

    /// Stage a handle-bearing entity instance before CLASSES is serialized.
    /// Entity encoders normally discover their class from the wire type, so
    /// they do not pass through registerTypedObjectClass().  Custom entities
    /// still need the same count/handle ledger as custom OBJECTS; otherwise
    /// the class table advertises zero instances for a physically emitted
    /// entity.
    bool registerDwgEntityClassInstance(std::uint16_t classNum,
                                        std::uint32_t handle) {
        if (classNum < 500 || handle == 0 || m_dwgClassesFrozen)
            return false;
        const DwgClassDefinition* existing =
            findDwgClassDefinition(classNum);
        if (existing == nullptr || !isDwgClassEnabled(*existing))
            return false;

        bool insertedInstance = false;
        if (!stageDwgClassInstance(classNum, handle, insertedInstance))
            return false;
        if (!insertedInstance)
            return true;

        DwgClassDefinition definition = *existing;
        definition.m_instanceCount = 1;
        if (registerDwgClass(definition))
            return true;

        rollbackDwgClassInstance(classNum, handle);
        return false;
    }

    /// Return the table-record handles emitted by the completed deferred
    /// OBJECTS-table phase.  The result is valid only after a successful
    /// writeDwgObjects() call; concrete writers that do not have deferred
    /// table records return false.
    virtual bool getEmittedDwgTableRecordHandles(
        std::vector<std::uint32_t>& handles) const {
        handles.clear();
        return false;
    }

    /// Return the table-control handles emitted by the completed deferred
    /// OBJECTS-table phase. The result is valid only after a successful
    /// writeDwgObjects() call.
    virtual bool getEmittedDwgTableControlHandles(
        std::vector<std::uint32_t>& handles) const {
        handles.clear();
        return false;
    }

    /// Register one source-independent child of the physical Named Objects
    /// Dictionary before the deferred OBJECTS phase begins.
    virtual bool registerDwgNamedObjectDictionaryEntry(
        const std::string&, std::uint32_t) {
        return false;
    }

    /// Return the physical Named Objects Dictionary receipt after the
    /// deferred OBJECTS-table phase has committed.
    virtual bool getEmittedDwgNamedObjectDictionaryEntries(
        std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const {
        entries.clear();
        return false;
    }

    /// Verify that every published Named Objects Dictionary child has an
    /// object-map frame. Concrete writers without this phase remain valid.
    virtual bool validateDwgNamedObjectDictionaryEntries() const {
        return true;
    }

    /// Return the completed deferred block-structure result. Concrete writers
    /// that do not expose deferred block records return false.
    virtual bool getEmittedDwgBlockWriteResult(
        DRW::DwgBlockWriteResult& result) const {
        result = {};
        return false;
    }

    /// Copy handle tokens from the most recently committed object frame.
    /// Returns false when no complete frame has been committed.
    virtual bool getLastDwgObjectHandleOccurrences(
        std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const {
        occurrences.clear();
        return false;
    }

    /// Copy the physical layout and handle tokens from the most recently
    /// committed object frame. Ranges are relative to the final data or
    /// handle segment and are valid only after a complete frame succeeds.
    virtual bool getLastDwgObjectFrame(
        DRW::DwgObjectFrameReceipt& frame) const {
        frame = {};
        return false;
    }

protected:
    /// Mark handle-associated class instances at the same commit point as the
    /// object-map entry.  A missing handle is harmless for fixed-type objects;
    /// custom registrations are required to have exactly one matching frame.
    void markDwgClassInstanceEmitted(std::uint32_t handle) {
        const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
        if (owner == m_dwgClassInstanceHandleOwners.end())
            return;
        const auto instance = m_rawClassInstanceHandles.find(
            {owner->second, handle});
        if (instance == m_rawClassInstanceHandles.end())
            return;
        ++m_dwgClassInstanceEmissionCounts[owner->second];
        m_rawClassInstanceHandles.erase(instance);
        m_dwgClassInstanceEmittedHandles.insert(handle);
    }

    /// Record the absolute buffer position of the header's fixed-width
    /// HANDSEED payload after a version-specific header writer emits it.
    void recordHeaderHandseedOffset(std::uint32_t dataStartBit) {
        if (m_header == nullptr)
            return;
        const std::uint32_t relative = m_header->dwgHandseedBitOffset();
        if (relative == DRW_Header::kInvalidDwgHandseedBitOffset
            || relative > (std::numeric_limits<std::uint32_t>::max)() - dataStartBit)
            return;
        m_headerHandseedBitOffset = dataStartBit + relative;
    }

    bool sameDwgClassIdentity(const DwgClassDefinition& left,
                              const DwgClassDefinition& right) const {
        return left.m_recordName == right.m_recordName
            && left.m_className == right.m_className
            && left.m_appName == right.m_appName
            && left.m_entityFlagRaw == right.m_entityFlagRaw;
    }

    const DwgClassDefinition* findDwgClassDefinition(
        std::uint16_t classNum) const {
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_classNum == classNum)
                return &definition;
        }
        return nullptr;
    }

    bool stageDwgClassInstance(std::uint16_t classNum,
                               std::uint32_t handle,
                               bool& inserted) {
        inserted = false;
        if (handle == 0)
            return true;
        const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
        if (owner != m_dwgClassInstanceHandleOwners.end()) {
            if (owner->second != classNum
                || m_dwgClassInstanceEmittedHandles.count(handle) != 0)
                return false;
        }
        if (!m_rawClassInstanceHandles.insert({classNum, handle}).second)
            return true;
        m_dwgClassInstanceHandleOwners.emplace(handle, classNum);
        inserted = true;
        return true;
    }

    void rollbackDwgClassInstance(std::uint16_t classNum,
                                  std::uint32_t handle) {
        if (handle == 0)
            return;
        m_rawClassInstanceHandles.erase({classNum, handle});
        m_dwgClassInstanceHandleOwners.erase(handle);
    }

    bool sameDwgClassDefinition(const DwgClassDefinition& left,
                                const DwgClassDefinition& right) const {
        return left.m_classNum == right.m_classNum
            && left.m_proxyFlag == right.m_proxyFlag
            && left.m_appName == right.m_appName
            && left.m_className == right.m_className
            && left.m_recordName == right.m_recordName
            && left.m_wasProxy == right.m_wasProxy
            && left.m_entityFlagRaw == right.m_entityFlagRaw
            && left.m_instanceCount == right.m_instanceCount
            && left.m_dwgVersion == right.m_dwgVersion
            && left.m_maintenanceVersion == right.m_maintenanceVersion
            && left.m_unknown1 == right.m_unknown1
            && left.m_unknown2 == right.m_unknown2;
    }

    bool sameDwgClassManifest(
        const std::vector<DwgClassDefinition>& left,
        const std::vector<DwgClassDefinition>& right) const {
        if (left.size() != right.size())
            return false;
        for (std::size_t i = 0; i < left.size(); ++i) {
            if (!sameDwgClassDefinition(left[i], right[i]))
                return false;
        }
        return true;
    }

    bool registerTypedObjectClass(DwgClassDefinition definition,
                                  std::uint32_t handle) {
        bool foundIdentity = false;
        for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
            if (sameDwgClassIdentity(existing, definition)) {
                definition.m_classNum = existing.m_classNum;
                foundIdentity = true;
                break;
            }
        }
        if (!foundIdentity) {
            for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
                if (existing.m_classNum != definition.m_classNum)
                    continue;
                definition.m_classNum = nextFreeCustomClassNum();
                if (definition.m_classNum < 500)
                    return false;
                break;
            }
        }
        bool insertedInstance = false;
        if (!stageDwgClassInstance(definition.m_classNum, handle,
                                   insertedInstance))
            return false;
        if (insertedInstance)
            definition.m_instanceCount = 1;
        if (registerDwgClass(definition))
            return true;
        if (insertedInstance) {
            rollbackDwgClassInstance(definition.m_classNum, handle);
        }
        return false;
    }

    bool registerTypedEntityClass(DwgClassDefinition definition) {
        for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
            if (sameDwgClassIdentity(existing, definition)) {
                definition.m_classNum = existing.m_classNum;
                return registerDwgClass(definition);
            }
        }
        for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
            if (existing.m_classNum == definition.m_classNum) {
                definition.m_classNum = nextFreeCustomClassNum();
                return definition.m_classNum >= 500
                    && registerDwgClass(definition);
            }
        }
        return registerDwgClass(definition);
    }

    bool isDwgClassEnabled(const DwgClassDefinition& definition) const {
        // Associative object class numbers are writer-local and may be moved
        // when they collide with another custom class.  Gate by identity so a
        // remapped class cannot leak into pre-R2007 output.
        if (definition.m_className == "AcDbDimAssoc"
            || definition.m_className == "AcDbEvalGraph")
            return m_version >= DRW::AC1021;
        if (definition.m_className == "AcDbSectionManager"
            || definition.m_className == "AcDbSectionSettings")
            return m_version >= DRW::AC1021;
        if (definition.m_className == "AcDbTvDeviceProperties")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbVxControl"
            || definition.m_className == "AcDbVxTableRecord")
            return m_version >= DRW::AC1015;
        // AcDbLight is a modern visualisation entity.  Advertising its custom
        // class in AC1015/AC1018 files makes older writer smoke files fail
        // reader compatibility even when no LIGHT entity is present.
        if (definition.m_className == "AcDbLight")
            return m_version >= DRW::AC1021;
        if (definition.m_className == "AcDbColor")
            // Keep an explicitly replayed legacy raw instance visible even
            // before native DBCOLOR encoding is available. The constructor's
            // zero-instance placeholder remains gated from old files.
            return m_version >= DRW::AC1018 || definition.m_instanceCount > 0;
        if (definition.m_className == "AcDbPlotSettings")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbExtrudedSurface"
            || definition.m_className == "AcDbLoftedSurface"
            || definition.m_className == "AcDbRevolvedSurface"
            || definition.m_className == "AcDbSweptSurface"
            || definition.m_className == "AcDbPlaneSurface"
            || definition.m_className == "AcDbNurbSurface")
            // Typed surface bodies are emitted only from R2007 onward, but
            // same-version raw surface entities may be replayed from R2004.
            return m_version >= DRW::AC1021 || definition.m_instanceCount > 0;
        if (definition.m_className == "AcDbCamera")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbPointCloud")
            return m_version >= DRW::AC1021;
        if (definition.m_className == "AcDbPointCloudEx")
            return m_version >= DRW::AC1027;
        if (definition.m_className == "AcDbNavisworksModel")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbPointCloudDef"
            || definition.m_className == "AcDbPointCloudDefEx"
            || definition.m_className == "AcDbPointCloudDefReactor"
            || definition.m_className == "AcDbPointCloudDefReactorEx")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbNavisworksModelDef"
            || definition.m_className == "AcDbPointCloudColorMap")
            return m_version >= DRW::AC1015;
        // SUN remains gated AC1021+; MLeaderStyle is available from R2000
        // onward and its pre-R2010 class version is implicit.
        if (definition.m_className == "AcDbSun")
            return m_version >= DRW::AC1021;
        if (definition.m_className == "AcDbMLeaderStyle")
            return m_version >= DRW::AC1015;
        // PR 13f — RasterVariables / GeoData / SpatialFilter broadened to
        // AC1015+.  Their encoders + parsers are version-clean (only the
        // standard `version > AC1018` split-buffer routing) and the
        // matching filter-side gate (`canRegisterCustomClassObjects`)
        // already issues the registration at AC1015+.
        if (definition.m_className == "AcDbRasterVariables")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbWipeoutVariables")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbGeoData")
            return m_version >= DRW::AC1015;
        if (definition.m_className == "AcDbSpatialFilter")
            return m_version >= DRW::AC1015;
        return true;
    }

    bool registerDwgClass(const DwgClassDefinition& definition) {
        if (definition.m_instanceCount < 0)
            return false;
        if (m_dwgClassesFrozen) {
            for (const DwgClassDefinition& existing : m_dwgClassDefinitions) {
                if (sameDwgClassIdentity(existing, definition)
                    && existing.m_classNum == definition.m_classNum
                    && definition.m_instanceCount == 0) {
                    return true;
                }
            }
            return false;
        }
        if (definition.m_classNum < 500)
            return true;
        for (auto& existing : m_dwgClassDefinitions) {
            if (existing.m_classNum == definition.m_classNum) {
                if (!sameDwgClassIdentity(existing, definition)) {
                    m_hasDwgClassConflict = true;
                    return false;
                }
                constexpr std::int32_t maxInstanceCount =
                    (std::numeric_limits<std::int32_t>::max)();
                if (existing.m_instanceCount < 0
                    || definition.m_instanceCount > maxInstanceCount
                        - existing.m_instanceCount) {
                    return false;
                }
                existing.m_instanceCount += definition.m_instanceCount;
                return true;
            }
        }
        m_dwgClassDefinitions.push_back(definition);
        return true;
    }

    std::vector<DwgClassDefinition> sortedDwgClassDefinitions() const {
        std::vector<DwgClassDefinition> definitions;
        definitions.reserve(m_dwgClassDefinitions.size());
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (isDwgClassEnabled(definition))
                definitions.push_back(definition);
        }
        std::sort(definitions.begin(), definitions.end(),
                  [](const DwgClassDefinition& left,
                     const DwgClassDefinition& right) {
                      return left.m_classNum < right.m_classNum;
                  });
        return definitions;
    }

    template <typename Callback>
    bool emitDwgClassDefinitions(Callback&& callback) const {
        if (!m_dwgClassManifestPrepared)
            return false;

        auto it = m_dwgClassManifest.cbegin();
        const std::uint32_t maxClass = maxDwgClassNumber();
        DwgClassDefinition placeholder;
        placeholder.m_proxyFlag = 0x401;
        placeholder.m_appName = "ACAD";
        placeholder.m_className = "AcDbUnusedClass";
        placeholder.m_recordName = "UNUSED_DWG_CLASS";
        placeholder.m_entityFlagRaw = 0x1F3;
        for (std::uint32_t classNum = 500; classNum <= maxClass;
             ++classNum) {
            if (it != m_dwgClassManifest.cend()
                && it->m_classNum == classNum) {
                if (!callback(*it))
                    return false;
                ++it;
                continue;
            }
            placeholder.m_classNum = static_cast<std::uint16_t>(classNum);
            if (!callback(placeholder))
                return false;
        }
        return it == m_dwgClassManifest.cend();
    }

    std::uint16_t maxDwgClassNumber() const {
        std::uint16_t maxClass = 499;
        const auto& definitions = m_dwgClassManifestPrepared
            ? m_dwgClassManifest : m_dwgClassDefinitions;
        for (const auto& definition : definitions) {
            if (!m_dwgClassManifestPrepared && !isDwgClassEnabled(definition))
                continue;
            if (definition.m_classNum > maxClass)
                maxClass = definition.m_classNum;
        }
        return maxClass;
    }

    bool writeDwgClassDefinition(const DwgClassDefinition& definition,
                                 dwgBufferW *dataBuf,
                                 dwgBufferW *stringBuf) const {
        if (dataBuf == nullptr)
            return false;
        dwgBufferW *textBuf = stringBuf != nullptr ? stringBuf : dataBuf;
        dataBuf->putBitShort(definition.m_classNum);
        dataBuf->putBitShort(definition.m_proxyFlag);
        textBuf->putVariableText(m_version, definition.m_appName);
        textBuf->putVariableText(m_version, definition.m_className);
        textBuf->putVariableText(m_version, definition.m_recordName);
        dataBuf->putBit(definition.m_wasProxy ? 1 : 0);
        dataBuf->putBitShort(definition.m_entityFlagRaw);
        if (m_version > DRW::AC1015) {
            dataBuf->putBitLong(definition.m_instanceCount);
            dataBuf->putBitLong(definition.m_dwgVersion);
            dataBuf->putBitLong(definition.m_maintenanceVersion);
            dataBuf->putBitLong(definition.m_unknown1);
            dataBuf->putBitLong(definition.m_unknown2);
        }
        return dataBuf->isGood()
            && (stringBuf == nullptr || stringBuf->isGood());
    }

    /// In-memory byte accumulator. All section bodies append here;
    /// final flush copies to `m_stream` in one `write()` call.
    dwgBufferW m_buf;

    /// Per-section start byte offsets, indexed by `secEnum::DWGSection`.
    /// Populated as each section begins emitting; consumed by `finalize()`
    /// to back-patch the file-header section locator records.
    std::map<int, std::uint32_t> m_sectionOffsets;

    /// Per-section byte sizes, same indexing as `m_sectionOffsets`.
    std::map<int, std::uint32_t> m_sectionSizes;

    std::ofstream *m_stream {nullptr};
    DRW_Header *m_header {nullptr};
    std::uint32_t m_headerHandseedBitOffset {
        DRW_Header::kInvalidDwgHandseedBitOffset};

    /// Target write version.  Default AC1015 (R2000).  Subclasses for
    /// higher versions set this in their constructor before any emit calls
    /// so all inherited section-emit helpers use the correct format.
    DRW::Version m_version {DRW::AC1015};

    /// Handle allocator pre-seeded with the canonical R2000 reserved
    /// set.  Subclasses call `m_handles.next()` for each user-emitted
    /// object that needs a fresh handle; the reserved handles
    /// (0x01–0x18, skipping 0x04) are referenced by their fixed values
    /// directly.
    HandleAllocator m_handles;

    std::vector<DwgClassDefinition> m_dwgClassDefinitions;
    std::vector<DwgClassDefinition> m_dwgClassManifest;
    std::set<std::pair<std::uint16_t, std::uint32_t>> m_rawClassInstanceHandles;
    std::map<std::uint32_t, std::uint16_t> m_dwgClassInstanceHandleOwners;
    std::set<std::uint32_t> m_dwgClassInstanceEmittedHandles;
    std::map<std::uint16_t, std::uint32_t> m_dwgClassInstanceEmissionCounts;
    /// Source class identity → writer-local ordinal (raw replay only).
    std::map<DwgRawClassIdentityKey, std::uint16_t> m_rawClassNumRemap;
    std::set<std::pair<std::uint16_t, std::uint32_t>> m_rawObjectOverrides;
    bool m_hasDwgClassConflict {false};
    bool m_dwgClassManifestPrepared {false};
    bool m_dwgClassesFrozen {false};
    DRW::DwgObjectFrameProvenance m_pendingDwgObjectFrameProvenance;
    DRW::DwgObjectFrameProvenance m_currentDwgObjectFrameProvenance;
};

#endif // DWGWRITER_H
