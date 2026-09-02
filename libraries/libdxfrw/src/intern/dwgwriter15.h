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

#ifndef DWGWRITER15_H
#define DWGWRITER15_H

#include <initializer_list>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dwgwriter.h"
#include "../drw_objects.h"
#include "drw_textcodec.h"

class DRW_Entity;
class dwgRW;

/// Section record indices used in m_sectionOffsets / m_sectionSizes.
namespace recno {
    constexpr std::uint8_t HEADER    = 0;
    constexpr std::uint8_t CLASSES   = 1;
    constexpr std::uint8_t HANDLES   = 2;
    constexpr std::uint8_t UNKNOWN   = 3;
    constexpr std::uint8_t TEMPLATE  = 4;
    constexpr std::uint8_t AUXHEADER = 5;
}

/// R2000 (AC1015) concrete DWG writer.  Structural mirror of
/// [dwgReader15](dwgreader15.h).
///
/// Section emission order matches LibreDWG (encode.c §3144 ff):
///   HEADER → CLASSES → OBJECTS body → HANDLES → 2NDHEADER.
/// AUXHEADER / TEMPLATE / PREVIEW are optional and emit only if
/// the input document carries them.  Fresh R2000 writes include the
/// AcDb:AuxHeader locator and stream so section layout matches
/// ACadSharp/ODA expectations.
class dwgWriter15 : public dwgWriter {
public:
    dwgWriter15(std::ofstream *stream, DRW_Header *header)
        : dwgWriter(stream, header) {
        configureTextCodec();
        // Pre-seed with canonical reserved handles so add*() deduplication
        // works before writeDwgObjects() fires.  Values mirror reservedHandle::*
        // in dwgwriter15.cpp (0x0F-0x16 are the R2000 fixed-table handles).
        m_writingCtx.ltypeMap    = {{"CONTINUOUS", 0x11u},
                                    {"BYLAYER",    0x10u},
                                    {"BYBLOCK",    0x0Fu}};
        m_writingCtx.layerMap    = {{"0",        0x12u}};
        m_writingCtx.styleMap    = {{"STANDARD", 0x13u}};
        m_writingCtx.ucsMap      = {};
        m_writingCtx.viewMap     = {};
        m_writingCtx.appidMap    = {{"ACAD",     0x14u}};
        m_writingCtx.dimstyleMap = {{"STANDARD", 0x15u}};
        m_writingCtx.vportMap    = {{"*ACTIVE",  DRW_Vport::kDwgActiveHandle}};
        m_writingCtx.mlineStyleMap = {};
    }

    bool writeFileHeaderStub() override;
    bool writeDwgHeader() override;
    bool writeDwgClasses() override;
    bool writeDwgObjects() override;
    bool writeDwgHandles() override;
    bool writeSecondHeader() override;
    bool finalize() override;

    /// Encode a single entity into the object stream.  See
    /// dwgWriter::encodeEntity for the contract.
    bool encodeEntity(DRW_Entity *ent) override;
    bool hasEntityEedWriteFailure() const override {
        return m_entityEedWriteFailure;
    }
    bool hasObjectEedWriteFailure() const override {
        return m_objectEedWriteFailure;
    }
    bool canWriteDwgDataStorageOperation(
        DwgDataStorageWriterOperation operation) const override;
    bool collectPendingTableEedAppIds() override;

    bool validateEmittedHandleMap(std::uint32_t& maxHandle) const override;
    bool validateDwgClassInstanceFrames() const override;

    std::uint32_t defineBlock(const std::string& name,
                        const DRW_Coord& basePoint,
                        int insUnits = 0) override;
    bool beginBlockContent(std::uint32_t blockRecordHandle) override;
    bool endBlockContent() override;
    bool hasActiveBlockContent() const override {
        return m_activeUserBlockRecordHandle != 0;
    }
    bool emitDeferredBlockControl() override;

    CompoundWriteCheckpoint checkpointCompoundWrite() const override;
    CompoundWriteCheckpoint checkpointPublicTransaction() const override;
    void rollbackCompoundWrite(
        const CompoundWriteCheckpoint& checkpoint) override;
    void markObjectWriteFailure() override { m_frameWriteError = true; }

    /// Accept a user-defined table record for deferred emission in
    /// writeDwgObjects().  Normalises the name to upper-case, deduplicates
    /// against standard entries, allocates a handle, updates m_writingCtx.
    /// Return the existing or newly allocated output handle, or zero for an
    /// invalid table name.
    std::uint32_t addLType(const DRW_LType& lt);
    std::uint32_t addLayer(const DRW_Layer& lay);
    /// Return the existing or newly allocated output handle, or zero for an
    /// invalid table name.
    std::uint32_t addTextstyle(const DRW_Textstyle& ts);
    // Assigns the writer's output handle back to `ucs` for source-reference
    // remapping before the deferred UCS table is emitted. Returns false for
    // an invalid name.
    bool addUcs(DRW_UCS& ucs);
    // Assigns the writer's output handle back to `view` so callers can map
    // source DWG VIEW references before the deferred table is emitted.
    // Returns false for an invalid name.
    bool addView(DRW_View& view);
    // Assigns the writer's output handle back to `vp` for deferred-table
    // source-reference remapping. Returns false for an invalid name.
    bool addVport(DRW_Vport& vp);
    // Assigns the accepted output handle back to `header`.  The record is
    // deferred until writeDwgObjects(), so the caller must retain the handle
    // for source-reference binding rather than treating acceptance as proof
    // that the frame was emitted.
    bool addViewportEntityHeader(DRW_ViewportEntityHeader& header);
    std::uint32_t addDimstyle(const DRW_Dimstyle& ds);
    std::uint32_t addAppId(const DRW_AppId& ai);

    bool getEmittedDwgTableRecordHandles(
        std::vector<std::uint32_t>& handles) const override;
    bool getEmittedDwgTableControlHandles(
        std::vector<std::uint32_t>& handles) const override;
    bool registerDwgNamedObjectDictionaryEntry(
        const std::string& name, std::uint32_t childHandle) override;
    bool getEmittedDwgNamedObjectDictionaryEntries(
        std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const override;
    bool validateDwgNamedObjectDictionaryEntries() const override;
    bool getEmittedDwgBlockWriteResult(
        DRW::DwgBlockWriteResult& result) const override;
    bool getLastDwgObjectHandleOccurrences(
        std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const override;
    bool getLastDwgObjectFrame(
        DRW::DwgObjectFrameReceipt& frame) const override;

    bool replayRawObject(const DRW_UnsupportedObject& object);
    bool writeAcDbPlaceholder(const DRW_AcDbPlaceholder& placeholder);
    bool writeVbaProject(const DRW_VbaProject& project);
    bool writeDbColor(const DRW_DbColor& color);
    bool writeDimensionAssociation(const DRW_DimensionAssociation& association);
    bool writeEvaluationGraph(const DRW_EvaluationGraph& graph);
    bool writeBlockRepresentationData(
        const DRW_BlockRepresentationData& data);
    bool writeSun(const DRW_Sun& sun);
    bool writeMLeaderStyle(const DRW_MLeaderStyle& style);
    bool writeTableStyle(const DRW_TableStyle& style);
    bool writeMaterial(const DRW_Material& material);
    bool writeLightList(const DRW_LightList& lightList);
    bool writeBackground(const DRW_Background& background);
    bool writeSunStudy(const DRW_SunStudy& study);
    bool writeMotionPath(const DRW_MotionPath& path);
    bool writeCurvePath(const DRW_CurvePath& path);
    bool writePointPath(const DRW_PointPath& path);
    bool writeObjectPtr(const DRW_ObjectPtr& object);
    bool writePartialViewingIndex(const DRW_PartialViewingIndex& index);
    bool writeRenderSettings(const DRW_RenderSettings& settings);
    bool writeVisualStyle(const DRW_VisualStyle& style);
    bool writeDictionary(const DRW_Dictionary& dictionary);
    bool writeXRecord(const DRW_XRecord& xrecord);
    bool writePlotSettings(const DRW_PlotSettings& plotSettings);
    bool writeLayout(const DRW_Layout& layout);
    bool writeGroup(const DRW_Group& group);
    bool writeMLineStyle(const DRW_MLineStyle& style);
    bool writeRasterVariables(const DRW_RasterVariables& rasterVariables);
    bool writeWipeoutVariables(const DRW_WipeoutVariables& wipeoutVariables);
    bool writeTvDeviceProperties(const DRW_TvDeviceProperties& object);
    bool writeVxControl(const DRW_VxControl& object);
    bool writeVxTableRecord(const DRW_VxTableRecord& object);
    /// IMAGEDEF (fixed type 102). Mutates `imageDef.handle` when zero.
    bool writeImageDef(DRW_ImageDef& imageDef);
    /// IMAGEDEF_REACTOR (custom class 532). Mutates `reactor.handle` when zero.
    bool writeImageDefinitionReactor(DRW_ImageDefinitionReactor& reactor);
    bool writePointCloudDef(DRW_PointCloudDef& definition);
    bool writeNavisworksModelDef(DRW_NavisworksModelDef& definition);
    bool writePointCloudColorMap(DRW_PointCloudColorMap& colorMap);
    bool writeGeoData(const DRW_GeoData& geoData);
    bool writeSpatialFilter(const DRW_SpatialFilter& filter);
    // PR 8d.2a — five small no-storage OBJECTS families.
    bool writeScale(const DRW_Scale& scale);
    bool writeIDBuffer(const DRW_IDBuffer& idBuffer);
    bool writeLayerIndex(const DRW_LayerIndex& layerIndex);
    bool writeSpatialIndex(const DRW_SpatialIndex& spatialIndex);
    bool writeDictionaryVar(const DRW_DictionaryVar& dictionaryVar);
    // PR 8d.2b — four larger no-storage OBJECTS families.
    bool writeDictionaryWithDefault(const DRW_DictionaryWithDefault& dictionary);
    bool writeSortEntsTable(const DRW_SortEntsTable& sortEntsTable);
    bool writeFieldList(const DRW_FieldList& fieldList);
    bool writeField(const DRW_Field& field);
    bool writeUnderlayDefinition(const DRW_UnderlayDefinition& definition);
    bool writeSection(const DRW_Section& section);

protected:
    /// Begin a new object in the object stream (the unsentinel'd byte
    /// region between CLASSES and HANDLES).  Records the offset of
    /// `m_buf` at frame start, captures `handle` for the eventual
    /// `m_objectMap` entry, and returns a reference to the scratch
    /// body buffer the caller writes into.  `m_objectBody` is cleared
    /// at every call so callers can use it directly without prep.
    dwgBufferW& beginObject(std::uint32_t handle);

    /// Finish the current object: byte-align the body, emit the
    /// per-object frame (MS objectSize + body bytes + RS CRC16 LE) to
    /// `m_buf`, record the (handle, offset) pair in `m_objectMap`.
    /// CRC is over the MS prefix + body bytes per LibreDWG convention.
    virtual void finishObject();

    /// Derived R2007+ writers can reject a frame during final assembly.
    /// Callers must observe that failure before publishing ownership state.
    virtual bool objectWriteFailed() const { return m_frameWriteError; }

    /// Phase 3d helper: emit one control object at `handle` into the
    /// object stream.  `numEntries` is emitted as the DWG-specified BS or BL
    /// field (and does NOT include the +2 phantom adjustment the reader
    /// applies to BLOCK_CONTROL / LTYPE_CONTROL).  `childHandles` are
    /// the offset-handle entries that get walked by the reader's
    /// `for (int i=0; i<numEntries; i++)` loop after the +2 phantom
    /// adjustment — so for LTYPE_CONTROL with a real CONTINUOUS entry
    /// you pass numEntries=1 and childHandles={BYBLOCK, BYLAYER,
    /// CONTINUOUS} (the +2 phantoms are part of the same offset-handle
    /// sequence on the wire).  Handles are emitted as absolute soft
    /// ownership references (code 2) so the reader's `getOffsetHandle`
    /// returns them as-is.
    bool emitControlObject(std::uint16_t oType, std::uint32_t handle, std::uint32_t numEntries,
                           std::initializer_list<std::uint32_t> childHandles);
    bool emitControlObject(std::uint16_t oType, std::uint32_t handle, std::uint32_t numEntries,
                           const std::vector<std::uint32_t>& childHandles);

    /// Phase 4d helper: emit a full Block_Record at `handle` with the
    /// `block`/`endBlock` handles pointing at DRW_Block entities the
    /// caller has already emitted.  The owned entity and INSERT lists are
    /// supplied separately; layout remains null for freshly written blocks.
    /// Needed so `readDwgBlocks` can resolve
    /// the BLOCK_CONTROL `+2` phantom handles (0x17, 0x18) without
    /// failing the block walk.
    bool emitBlockRecord(std::uint32_t handle, const std::string& name,
                         const DRW_Coord& basePoint,
                         std::uint32_t blockHandle, std::uint32_t endBlockHandle,
                         const std::vector<std::uint32_t>& entityHandles,
                         const std::vector<std::uint32_t>& insertHandles = {},
                         int insUnits = 0);

    /// Phase 4d helper: emit a Block entity at `handle`.  `isEnd=true`
    /// suppresses the name field and emits an ENDBLK (oType=5) rather
    /// than a BLOCK (oType=4).
    bool emitBlockEntity(std::uint32_t handle, const std::string& name,
                         bool isEnd);

    /// Full table-record emitters — preamble + encodeDwg + finishObject.
    bool emitLtypeRecord(std::uint32_t handle, const DRW_LType& lt);
    bool emitLayerRecord(std::uint32_t handle, const DRW_Layer& lay);
    bool emitStyleRecord(std::uint32_t handle, const DRW_Textstyle& ts);
    bool emitUcsRecord(std::uint32_t handle, const DRW_UCS& ucs);
    bool emitViewRecord(std::uint32_t handle, const DRW_View& view);
    bool emitVportRecord(std::uint32_t handle, const DRW_Vport& vp);
    bool emitViewportEntityHeaderRecord(
        std::uint32_t handle, const DRW_ViewportEntityHeader& header);
    bool emitAppIdRecord(std::uint32_t handle, const DRW_AppId& ai);
    bool emitDimstyleRecord(std::uint32_t handle, const DRW_Dimstyle& ds);
    bool finishTableRecord(std::uint32_t handle);
    bool emitAcDbPlaceholderObject(
        std::uint32_t handle, const DRW_AcDbPlaceholder& placeholder,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitVbaProjectObject(
        std::uint32_t handle, const DRW_VbaProject& project,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitSunObject(
        std::uint32_t handle, const DRW_Sun& sun,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitTvDevicePropertiesObject(
        std::uint32_t handle, const DRW_TvDeviceProperties& properties,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitBlockRepresentationDataObject(
        std::uint32_t handle, const DRW_BlockRepresentationData& data,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitMLeaderStyleObject(
        std::uint32_t handle, const DRW_MLeaderStyle& style,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitTableStyleObject(
        std::uint32_t handle, const DRW_TableStyle& style,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitMaterialObject(
        std::uint32_t handle, const DRW_Material& material,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitLightListObject(
        std::uint32_t handle, const DRW_LightList& lightList,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitDictionaryObject(
        std::uint32_t handle, const DRW_Dictionary& dictionary,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitXRecordObject(
        std::uint32_t handle, const DRW_XRecord& xrecord,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitPlotSettingsObject(
        std::uint32_t handle, const DRW_PlotSettings& plotSettings,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitLayoutObject(
        std::uint32_t handle, const DRW_Layout& layout,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitGroupObject(
        std::uint32_t handle, const DRW_Group& group,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitMLineStyleObject(std::uint32_t handle,
                              const DRW_MLineStyle& style,
                              const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                              const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitRasterVariablesObject(std::uint32_t handle,
                                   const DRW_RasterVariables& rasterVariables,
                                   const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                   const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitWipeoutVariablesObject(std::uint32_t handle,
                                    const DRW_WipeoutVariables& wipeoutVariables,
                                    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitGeoDataObject(
        std::uint32_t handle, const DRW_GeoData& geoData,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitSpatialFilterObject(std::uint32_t handle,
                                 const DRW_SpatialFilter& filter,
                                 const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                 const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    // PR 8d.2a — five small no-storage OBJECTS families.
    bool emitScaleObject(
        std::uint32_t handle, const DRW_Scale& scale,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitIDBufferObject(
        std::uint32_t handle, const DRW_IDBuffer& idBuffer,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitLayerIndexObject(
        std::uint32_t handle, const DRW_LayerIndex& layerIndex,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitSpatialIndexObject(
        std::uint32_t handle, const DRW_SpatialIndex& spatialIndex,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitDictionaryVarObject(
        std::uint32_t handle, const DRW_DictionaryVar& dictionaryVar,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    // PR 8d.2b — four larger no-storage OBJECTS families.
    bool emitDictionaryWithDefaultObject(
        std::uint32_t handle, const DRW_DictionaryWithDefault& dictionary,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitSortEntsTableObject(
        std::uint32_t handle, const DRW_SortEntsTable& sortEntsTable,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitFieldListObject(
        std::uint32_t handle, const DRW_FieldList& fieldList,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitFieldObject(
        std::uint32_t handle, const DRW_Field& field,
        const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);
    bool emitUnderlayDefinitionObject(std::uint32_t handle,
                                      const DRW_UnderlayDefinition& definition,
                                      const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                      const std::vector<DRW_Entity::PendingHandleRef>& layerRefs);

    /// Apply the active BLOCK content scope to one entity and resolve a named
    /// INSERT to its registered BLOCK_RECORD handle when needed.
    void prepareBlockOwnedEntity(DRW_Entity& entity);

    /// Record a successfully encoded entity in the active BLOCK content scope.
    bool canRecordBlockOwnedEntity(const DRW_Entity& entity) const;
    bool recordBlockOwnedEntity(const DRW_Entity& entity);

    /// Record a successfully encoded INSERT in the BLOCK_RECORD it targets.
    /// R2000+ stores this reverse reference in the target record's handle
    /// stream; unresolved targets remain unlisted for compatibility.
    bool recordBlockInsertReference(const DRW_Entity& entity);

    /// Publish the final frame layout and handle tokens after the corresponding
    /// object-map entry has been committed. The merge offsets are measured in
    /// the final serialized data segment.
    bool captureLastDwgObjectHandleOccurrences(
        std::uint64_t mergedStringBaseBit = 0,
        std::uint64_t mergedHandleBaseBit = 0,
        bool stringsMergedIntoData = false,
        bool handlesMergedIntoData = false);

    /// Raw custom entities are accepted only when their validated owner names
    /// an emitted user BLOCK_RECORD. This keeps replay and entMap publication
    /// atomic at the object boundary.
    bool canRecordRawBlockOwnedEntity(
        const DRW_UnsupportedObject& object) const;
    bool recordRawBlockOwnedEntity(const DRW_UnsupportedObject& object);

    /// Configure the primary DWG text codec and the secondary byte codec used
    /// by ENC names after a derived writer selects its target version.
    void configureTextCodec();

    /// Return the file-header codepage id corresponding to $DWGCODEPAGE.
    /// Unknown or absent values use the DWG ANSI_1252 default.
    std::uint16_t fileCodePageId() const;

protected:
    bool blockControlEmitted() const { return m_blockControlEmitted; }

    /// Populate m_header's ctrl-handle fields with canonical reserved values
    /// where they are still zero (caller may have pre-filled them on read).
    /// Called from writeDwgHeader (and overrides) so the HEADER section's
    /// control-handle block references the right objects.
    void initHeaderControlHandles();

    /// Per-section-frame helper: emit BEGIN sentinel + 4-byte RL size
    /// placeholder, mark the start offset.  Returns the byte offset of
    /// the placeholder RL so `endSentinelSection` can patch it.
    size_t beginSentinelSection(const std::uint8_t (&beginSentinel)[16]);

    /// Per-section-frame helper: emit END sentinel + CRC16 LE over the
    /// section bytes between begin sentinel and end sentinel.  Patches
    /// the RL size at `sizeOffset` with the actual payload size.
    void endSentinelSection(size_t sectionStart, size_t sizeOffset,
                            const std::uint8_t (&endSentinel)[16],
                            size_t unknownTailBytes = 0);

protected:
    DRW_TextCodec m_textCodec;

    /// Scratch buffer for the in-flight object body (DATA section).
    /// Cleared at every `beginObject` call.
    dwgBufferW m_objectBody;

    /// Scratch buffer for string fields (AC1024+ only — strings are
    /// written into the tail of the data section before the handle
    /// section).  Ignored by dwgWriter15/18 finishObject.
    dwgBufferW m_objectStrings;

    /// Scratch buffer for handle fields (AC1024+ only — handles are
    /// written into a separate handle section after the data section).
    /// Ignored by dwgWriter15/18 finishObject.
    dwgBufferW m_objectHandles;

    /// Handle of the in-flight object (set by `beginObject`, cleared
    /// at `finishObject`).  Used to record the (handle, offset) pair.
    std::uint32_t m_currentHandle {0};

    DRW::DwgObjectFrameReceipt m_lastDwgObjectFrame;
    bool m_lastDwgObjectFrameValid {false};
    std::uint64_t m_nextDwgObjectFrameGeneration {1};

    /// Sticky failure state for legacy frame assembly and finalization.
    bool m_frameWriteError {false};

    /// Object-map collector.  Each entry is `(handle, byte-offset of
    /// the object's MS prefix in m_buf)`.  Sorted by handle in
    /// `writeDwgHandles` before page emission for monotonic deltas.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> m_objectMap;

    /// Writing context: maps normalised (upper-case) table record names
    /// to their allocated DWG handles.  Pre-seeded with standard entries;
    /// extended by add*() as user records are registered.  Used by
    /// encodeEntity() to resolve layer/ltype names to handles.
    DRW_WritingContext m_writingCtx;

    /// Pending user-defined table records.  Populated by add*() during
    /// the table-callback phase; consumed by writeDwgObjects().
    std::vector<std::pair<std::uint32_t, DRW_LType>>     m_pendingLTypes;
    std::vector<std::pair<std::uint32_t, DRW_Layer>>     m_pendingLayers;
    // Real data for the reserved "0" layer (fixed handle 0x12). Captured in
    // addLayer() and emitted via emitLayerRecord() so its plot flag / color /
    // linetype round-trip, instead of a default stub that loses them.
    DRW_Layer m_layer0;
    bool      m_haveLayer0{false};
    std::vector<std::pair<std::uint32_t, DRW_Textstyle>> m_pendingStyles;
    std::vector<std::pair<std::uint32_t, DRW_UCS>>        m_pendingUcs;
    std::vector<std::pair<std::uint32_t, DRW_View>>      m_pendingViews;
    std::vector<std::pair<std::uint32_t, DRW_Vport>>     m_pendingVports;
    std::vector<std::pair<std::uint32_t, DRW_ViewportEntityHeader>>
        m_pendingViewportEntityHeaders;
    std::vector<std::pair<std::uint32_t, DRW_Dimstyle>>  m_pendingDimstyles;
    std::vector<std::pair<std::uint32_t, DRW_AppId>>     m_pendingAppIds;

    // Populated only after every deferred table/control record has emitted
    // successfully.  A failed write never exposes a completion result.
    std::set<std::uint32_t> m_emittedDwgTableRecordHandles;
    std::set<std::uint32_t> m_emittedDwgTableControlHandles;
    bool m_dwgTableRecordsComplete {false};

    std::vector<DRW::DwgNamedObjectDictionaryEntry>
        m_pendingDwgNamedObjectDictionaryEntries;
    std::vector<DRW::DwgNamedObjectDictionaryEntry>
        m_emittedDwgNamedObjectDictionaryEntries;
    bool m_dwgNamedObjectDictionaryComplete {false};

    /// Published only after the deferred BLOCK_CONTROL transaction completes.
    std::vector<DRW::DwgBlockWriteRecord> m_emittedDwgBlockRecords;
    std::set<std::uint32_t> m_emittedDwgBlockEntityHandles;
    bool m_dwgBlockStructureComplete {false};

    /// Resolve named APPID/LAYER EED references into the writer's final
    /// table-handle namespace before an entity serializes its common header.
    bool prepareEntityEed(DRW_Entity& entity);
    bool prepareTableEntryEed(
        const DRW_TableEntry& entry,
        std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
        std::vector<DRW_Entity::PendingHandleRef>& layerRefs) const;

    bool m_entityEedWriteFailure {false};
    bool m_objectEedWriteFailure {false};

    DRW_LType m_ltypeByBlock;
    DRW_LType m_ltypeByLayer;
    DRW_LType m_ltypeContinuous;
    bool m_haveLtypeByBlock {false};
    bool m_haveLtypeByLayer {false};
    bool m_haveLtypeContinuous {false};
    DRW_Textstyle m_standardStyle;
    bool m_haveStandardStyle {false};
    DRW_Vport m_activeVport;
    bool m_haveActiveVport {false};
    DRW_Dimstyle m_standardDimstyle;
    bool m_haveStandardDimstyle {false};

    friend class dwgRW;
    static constexpr std::uint8_t kAfterFirstDeferredTableRecord = 1;
    static constexpr std::uint8_t kAfterFirstBlockRecord = 2;
    static constexpr std::uint8_t kBeforeEndBlockFrame = 3;
    static constexpr std::uint8_t kAfterEndBlockFrame = 4;
    bool consumeWriteFailurePointForTest(std::uint8_t point) noexcept {
        if (m_writeFailurePointForTest != point)
            return false;
        m_writeFailurePointForTest = 0;
        return true;
    }
    std::uint8_t m_writeFailurePointForTest {0};

private:
    /// Test-only fault injection used by the DWG transaction regressions.
    /// Normal writers leave this at zero.
    void setWriteFailurePointForTest(std::uint8_t point) noexcept {
        m_writeFailurePointForTest = point;
    }

    /// File offset of the first section-locator record byte.  Used by
    /// `finalize()` to back-patch addresses + sizes.  Set during
    /// `writeFileHeaderStub`.
    std::uint32_t m_recordsOffset {0};

    /// Number of section-locator records emitted.  R2000 writes the
    /// canonical HEADER, CLASSES, HANDLES, ObjFreeSpace, Template, and
    /// AuxHeader records.
    std::uint8_t m_numSections {6};

    struct PendingUserBlock {
        std::uint32_t blockRecordHandle {0};
        std::uint32_t blockHandle {0};
        std::uint32_t endBlockHandle {0};
        std::string name;
        DRW_Coord basePoint;
        int insUnits {0};
        std::vector<std::uint32_t> entityHandles;
        std::vector<std::uint32_t> insertHandles;
    };

    /// User-defined blocks from defineBlock(). The Block/ENDBLK entities are
    /// emitted immediately, while the Block_Record is deferred until after
    /// the balanced block-content scopes have recorded block-owned entities.
    std::vector<PendingUserBlock> m_userBlocks;
    std::unordered_map<std::string, std::uint32_t> m_userBlockHandles;
    std::uint32_t m_activeUserBlockRecordHandle {0};
    // Once BLOCK_CONTROL and all BLOCK_RECORD lists are emitted, adding an
    // entity or block would leave it unreachable from the ownership graph.
    bool m_blockControlEmitted {false};
    // Modelspace and paperspace are real BLOCK_RECORD owners in R2004+ and
    // use entmode 2/1 (no owner handle) on their entities.  Their owned-handle
    // lists are emitted only after writeEntities() has supplied all members.
    std::vector<std::uint32_t> m_modelSpaceEntityHandles;
    std::vector<std::uint32_t> m_paperSpaceEntityHandles;
    std::vector<std::uint32_t> m_modelSpaceInsertHandles;
    std::vector<std::uint32_t> m_paperSpaceInsertHandles;
};

#endif // DWGWRITER15_H
