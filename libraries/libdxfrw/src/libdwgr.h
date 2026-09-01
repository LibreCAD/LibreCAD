/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef LIBDWGR_H
#define LIBDWGR_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
//#include <deque>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_classes.h"
#include "drw_interface.h"
#include "intern/dwgutil.h"

class dwgReader;
class dwgBuffer;
class dwgWriter;
class DwgCompoundWriteTestAccess;
class DwgWriteFailureTestAccess;

/// Closed identity for a typed OBJECTS writer that can carry the AC1027+
/// DataStorage presence bit.  The identity is deliberately separate from a
/// DWG class ordinal: class ordinals can be remapped when two producer
/// classes collide in one output file.
enum class DwgDataStorageWriterBinding : std::uint8_t {
    None,
    MLeaderStyle,
    TableStyle,
    Material,
    LightList,
    Group,
    Dictionary,
    XRecord,
    Layout,
    MLineStyle,
    AcDbPlaceholder,
    RasterVariables,
    WipeoutVariables,
    GeoData,
    SpatialFilter,
    Scale,
    IDBuffer,
    LayerIndex,
    SpatialIndex,
    DictionaryVar,
    DictionaryWithDefault,
    SortEntsTable,
    FieldList,
    Field,
    UnderlayPdf,
    UnderlayDgn,
    UnderlayDwf,
    PointCloudDefinition,
    PointCloudDefinitionEx,
    PointCloudDefinitionReactor,
    PointCloudDefinitionReactorEx,
    NavisworksModelDefinition,
    PointCloudColorMap,
    DbColor,
    DimensionAssociation,
    EvaluationGraph,
    TvDeviceProperties,
    VxControl,
    VxTableRecord
};

enum class DwgDataStorageWriterPhase : std::uint8_t {
    Objects,
    Section
};

/// Severity of a DWG integrity observation. Structural observations are
/// failures; checksum observations remain warnings for compatibility with
/// existing third-party files.
enum class DwgIntegritySeverity : std::uint8_t {
    Error,
    Warning
};

/// Address space for an optional diagnostic offset. A decoded offset is not a
/// file offset; keeping the distinction explicit prevents callers from
/// seeking to a buffer-local position in the source DWG.
enum class DwgIntegrityAddressSpace : std::uint8_t {
    None,
    PhysicalFile,
    DecodedBuffer
};

/// Parser phase in which an integrity observation was made.
enum class DwgIntegrityPhase : std::uint8_t {
    Metadata,
    FileHeader,
    PageMap,
    SectionMap,
    DataPage,
    ObjectMap,
    ObjectFrame,
    SectionParser
};

/// Stable categories used by the DWG integrity diagnostics API.
enum class DwgIntegrityCheckKind : std::uint8_t {
    FileHeaderCrc,
    ClassesCrc,
    SystemPageCrc,
    SystemPageUncompressedCrc,
    SystemPageChecksum,
    DataPageCrc,
    DataPageChecksum,
    PageMapReference,
    SectionPageReference,
    PageRange,
    PageGeometry,
    ReedSolomonDecode,
    Decompression,
    ObjectMapProgress,
    ObjectMapCrc,
    ObjectMapDuplicateOffset,
    ObjectFrameBounds,
    FrameLedgerTransition
};

/// Version of the field and enum contract carried by DwgIntegrityDiagnostic.
/// Increment this when a field changes meaning or an enum value is removed;
/// adding a new optional diagnostic category is otherwise backward-compatible.
constexpr std::uint16_t DwgIntegrityDiagnosticSchemaVersion = 1;

/// Location-bearing evidence from one DWG structural or checksum check.
/// Optional fields use explicit presence flags because zero is meaningful for
/// file offsets, expected values, and null handles in the file format.
struct DwgIntegrityDiagnostic {
    static constexpr std::uint16_t schemaVersion =
        DwgIntegrityDiagnosticSchemaVersion;
    DRW::Version version {DRW::UNKNOWNV};
    DwgIntegritySeverity severity {DwgIntegritySeverity::Error};
    DwgIntegrityAddressSpace offsetSpace {DwgIntegrityAddressSpace::None};
    DwgIntegrityPhase phase {DwgIntegrityPhase::FileHeader};
    DwgIntegrityCheckKind kind {DwgIntegrityCheckKind::PageRange};
    /// Logical section identity from secEnum::DWGSection, when known.
    std::int32_t logicalSectionId {-1};
    /// File-local descriptor ordinal from dwgSectionInfo::Id, when known.
    std::int32_t sectionDescriptorId {-1};
    std::string sectionName;
    std::uint64_t pageId {0};
    std::uint64_t fileOffset {0};
    std::uint32_t logicalHandle {0};
    std::uint64_t expected {0};
    std::uint64_t observed {0};
    bool hasPageId {false};
    bool hasFileOffset {false};
    bool hasLogicalHandle {false};
    bool hasExpected {false};
    bool hasObserved {false};
};

/// Executable capability metadata for one low-level typed writer.  A zero
/// class number means that the writer allocates/remaps the file-local ordinal.
struct DwgDataStorageWriterCapability {
    DwgDataStorageWriterBinding binding {
        DwgDataStorageWriterBinding::None};
    DwgDataStorageWriterOperation operation {
        DwgDataStorageWriterOperation::None};
    const char* family {""};
    const char* className {""};
    const char* recordName {""};
    std::uint16_t classNumber {0};
    std::uint16_t fixedObjectType {0};
    DRW::Version minVersion {DRW::UNKNOWNV};
    DRW::Version maxVersion {DRW::UNKNOWNV};
    DwgDataStorageWriterPhase phase {DwgDataStorageWriterPhase::Objects};
    bool requiresClass {false};
    bool acceptsPresenceBit {false};
};

/// Return the low-level capability for a binding, or false for None.
bool getDwgDataStorageWriterCapability(
    DwgDataStorageWriterBinding binding,
    DwgDataStorageWriterCapability& capability);

/// Copy the complete executable capability inventory used by DWG admission.
/// The result is empty when no capabilities are compiled in.
bool getDwgDataStorageWriterCapabilities(
    std::vector<DwgDataStorageWriterCapability>& capabilities);

/// Resolve an exact family/class/record identity to a low-level binding.
/// Unknown or unsupported identities return None.
DwgDataStorageWriterBinding findDwgDataStorageWriterBinding(
    const char* family, const char* className, const char* recordName);

/// Public DWG read/write API.  Renamed from `class dwgR` (read-only)
/// to `class dwgRW` (read + write) on 2026-05-14 to mirror the
/// combined `class dxfRW` for DXF.  The legacy name `dwgR` remains
/// available via a `using dwgR = dwgRW;` alias at the bottom of this
/// header so existing call sites compile unchanged.
class dwgRW {
public:
    explicit dwgRW(const char* name);
    ~dwgRW();
    //read: return true if all ok
    [[nodiscard]] bool read(DRW_Interface *interface_, bool ext);
    [[nodiscard]] bool readBuffer(const std::uint8_t *data, std::uint64_t size,
                    DRW_Interface *interface_, bool ext);

    /// Write the in-memory model (driven via DRW_Interface callbacks)
    /// out to the file named at construction.
    /// The `bin` parameter is ignored — DWG is always binary — but
    /// kept for API symmetry with `dxfRW::write`.  Returns true on
    /// success, false on error; error code accessible via `getError()`.
    [[nodiscard]] bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);

    struct WriteSkipCounters {
        std::size_t entityWrites { 0 };
        std::size_t tableRecordWrites { 0 };
        std::size_t objectWrites { 0 };
        std::size_t classRegistrations { 0 };
        std::size_t rawObjectWrites { 0 };
        std::size_t rawSectionWrites { 0 };
        std::size_t blockDefinitions { 0 };

        std::size_t total() const {
            return entityWrites + tableRecordWrites + objectWrites
                + classRegistrations + rawObjectWrites + rawSectionWrites
                + blockDefinitions;
        }
    };
    WriteSkipCounters getWriteSkipCounters() const { return m_writeSkipCounters; }
    /// Mark the current callback-driven write as incomplete. Ordinary
    /// per-record false returns remain recoverable skips; callers use this
    /// latch when a required phase item could not be emitted.
    void markDwgWriteFailure() noexcept { m_requiredWriteFailure = true; }
    bool hasDwgWriteFailure() const noexcept { return m_requiredWriteFailure; }
    /// Number of entity frames successfully encoded since the last write.
    /// This is used by higher-level filters to distinguish a prepared entity
    /// from one that was actually committed to the object stream.
    std::size_t getSuccessfulEntityWriteCount() const {
        return m_successfulEntityWrites;
    }

    /// Copy the handles emitted by the completed deferred DWG table phase.
    /// Returns false until the writer has completed writeDwgObjects().
    bool getEmittedDwgTableRecordHandles(
        std::vector<std::uint32_t>& handles) const;

    /// Copy the handles emitted by the completed deferred DWG table-control
    /// phase. Returns false until the writer has completed writeDwgObjects().
    bool getEmittedDwgTableControlHandles(
        std::vector<std::uint32_t>& handles) const;

    /// Register one source-independent child of the physical Named Objects
    /// Dictionary. Registration closes when the deferred OBJECTS phase starts.
    /// A rejected registration latches this write as incomplete.
    bool registerDwgNamedObjectDictionaryEntry(
        const std::string& name, std::uint32_t childHandle);

    /// Copy the physical Named Objects Dictionary receipt after the deferred
    /// OBJECTS-table phase has committed.
    bool getEmittedDwgNamedObjectDictionaryEntries(
        std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const;

    /// Copy the completed deferred BLOCK_CONTROL/BLOCK_RECORD result.
    /// Returns false until the structural block phase has succeeded.
    bool getEmittedDwgBlockWriteResult(
        DRW::DwgBlockWriteResult& result) const;

    /// Handle of the last successful top-level entity write. Compound writes
    /// publish their parent handle; failed attempts publish zero.
    std::uint32_t getLastSuccessfulEntityHandle() const {
        return m_lastSuccessfulEntityHandle;
    }

    /// Copy handle tokens from the most recently committed DWG object frame.
    /// The result is valid while the write callback is processing that frame.
    bool getLastDwgObjectHandleOccurrences(
        std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const;

    /// Copy the physical layout and handle tokens from the most recently
    /// committed DWG object frame.
    bool getLastDwgObjectFrame(DRW::DwgObjectFrameReceipt& frame) const;

    /// Copy ordered frame evidence for the most recently committed
    /// POLYLINE/VERTEX/SEQEND write. Other writes and any rollback clear it.
    bool getLastDwgCompoundEntityWriteReceipt(
        DRW::DwgCompoundEntityWriteReceipt& receipt) const;

    /// Bind the next low-level object frame to a filter admission. The
    /// writer consumes this binding when it begins the next object.
    bool setDwgObjectFrameProvenance(
        std::uint16_t classNumber, std::uint16_t writerOperation,
        std::uint64_t admissionToken);

    /// Resolve the writer-local class ordinal after class admission. A zero
    /// fallback is returned when the class is not registered.
    std::uint16_t getDwgTypedClassNumber(
        const char* className, const char* recordName,
        std::uint16_t fallback = 0) const;

    /// Resolve the writer-local class ordinal admitted for an output handle.
    /// Returns false when the handle has no class-backed OBJECTS instance.
    bool getDwgClassInstanceClass(std::uint32_t handle,
                                  std::uint16_t& classNumber) const;

    /// Return whether the frozen writer class table contains this ordinal.
    /// Fixed entity types are not class-backed and therefore return false.
    bool hasDwgClassDefinition(std::uint16_t classNumber) const;

    /// Clear a frame binding when the next operation is not an admitted
    /// DataStorage carrier or when the planned write is abandoned.
    void clearDwgObjectFrameProvenance();

    /// Roll back the most recent top-level entity or compound-entity write.
    /// The checkpoint is retained until the next write so a higher-level
    /// filter can reject a frame after validating its references without
    /// leaving serialized bytes or writer ledgers behind.
    /// If restoreCallerState is true, restore caller-owned fields captured by
    /// the write. Filter callbacks pass false because their temporary DRW
    /// records may already be out of scope when reference validation rejects
    /// the frame.
    bool rollbackLastDwgObjectWrite(bool restoreCallerState = true);

    /// Begin an opaque LIFO transaction for one or more DWG writes. The
    /// returned token is valid only on this writer and must be committed or
    /// rolled back before its parent scope. Allocator reservations remain
    /// monotonic; all serialized and completion ledgers are transactional.
    /// If suppressFailureCounters is true, failures recorded inside this
    /// speculative scope are discarded when the scope is rolled back.
    /// trackCallerMutations enables restoration of caller-owned DRW records
    /// when this explicit scope is rolled back. Tracked records must remain
    /// alive until commit/rollback; leave it false for callback-local records.
    /// includeAdmissionState includes deferred table vectors and name maps;
    /// filter-internal per-object scopes may pass false to retain the compact
    /// frame checkpoint because table admission is already complete.
    std::uint64_t beginDwgWriteTransaction(
        bool suppressFailureCounters = false,
        bool trackCallerMutations = false,
        bool includeAdmissionState = true);

    /// Commit the most recently opened explicit DWG transaction.
    bool commitDwgWriteTransaction(std::uint64_t token);

    /// Roll back the most recently opened explicit DWG transaction.
    bool rollbackDwgWriteTransaction(std::uint64_t token);

    /// Per-entity write API — invoked from the caller's `writeEntities`
    /// iface callback.  Each method allocates a handle, populates the
    /// entity's `handle` + `layerH.ref` fields, encodes the entity to
    /// the object stream, and records the `(handle, offset)` pair so
    /// the HANDLES section emit later finds it.  Returns true on
    /// success.  Layer-by-name resolution lands in Phase 4d; for now
    /// every entity is placed on layer "0" (handle 0x12).
    bool writePoint(DRW_Point *ent);
    bool writeLine(DRW_Line *ent);
    bool write3DLine(DRW_3DLine *ent);
    bool writeCircle(DRW_Circle *ent);
    bool writeArc(DRW_Arc *ent);
    bool writeEllipse(DRW_Ellipse *ent);
    bool writeText(DRW_Text *ent);
    bool writeRText(DRW_RText *ent);
    bool writeArcAlignedText(DRW_ArcAlignedText *ent);
    bool writeLWPolyline(DRW_LWPolyline *ent);
    bool writeRay(DRW_Ray *ent);
    bool writeXline(DRW_Xline *ent);
    bool writeTrace(DRW_Trace *ent);
    bool writeSolid(DRW_Solid *ent);
    bool write3dface(DRW_3Dface *ent);
    bool writeInsert(DRW_Insert *ent);
    bool writeTable(DRW_Table *ent);
    bool writeMText(DRW_MText *ent);
    bool writeSpline(DRW_Spline *ent);
    bool writeHelix(DRW_Helix *ent);
    bool writeAttrib(DRW_Attrib *ent);
    bool writeAttdef(DRW_Attdef *ent);
    bool writeHatch(DRW_Hatch *ent);
    bool writeMPolygon(DRW_MPolygon *ent);
    bool writeDimension(DRW_Dimension *ent);
    bool writeTolerance(DRW_Tolerance *ent);
    bool writeLight(DRW_Light *ent);
    bool writeCamera(DRW_Camera *ent);
    bool writeGeoPositionMarker(DRW_GeoPositionMarker *ent);
    bool writeSectionObject(DRW_SectionObject *ent);
    bool writeMLine(DRW_MLine *ent);
    bool writeUnderlay(DRW_Underlay *ent);
    bool writePolyline(DRW_Polyline *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeMLeader(DRW_MLeader *ent);
    bool writeViewport(DRW_Viewport *ent);
    bool writeShape(DRW_Shape *ent);
    bool writeOle2Frame(DRW_Ole2Frame *ent);
    bool writeOleFrame(DRW_OleFrame *ent);
    bool writeMesh(DRW_Mesh *ent);
    bool writeWipeout(DRW_Wipeout *ent);
    /// Write IMAGE entity. When `fileName` is non-null, also emits IMAGEDEF
    /// (+ IMAGEDEF_REACTOR) and fills `ent->ref` / reactor handles. An
    /// optional templates supply preserved IMAGEDEF and IMAGEDEF_REACTOR
    /// metadata. The reactor is rebound to the emitted IMAGE entity.
    bool writeImage(DRW_Image *ent, const std::string *fileName = nullptr,
                    const DRW_ImageDef *definition = nullptr,
                    const DRW_ImageDefinitionReactor *reactorDefinition = nullptr);
    bool writeImageDef(DRW_ImageDef *object);
    bool writeImageDefinitionReactor(DRW_ImageDefinitionReactor *object);
    bool registerPointCloudDefObjectClass(DRW_PointCloudDef *object);
    bool writePointCloudDef(DRW_PointCloudDef *object);
    bool registerNavisworksModelDefObjectClass(DRW_NavisworksModelDef *object);
    bool writeNavisworksModelDef(DRW_NavisworksModelDef *object);
    bool registerPointCloudColorMapObjectClass(DRW_PointCloudColorMap *object);
    bool writePointCloudColorMap(DRW_PointCloudColorMap *object);
    bool writePointCloud(DRW_PointCloud *ent);
    bool writePointCloudEx(DRW_PointCloudEx *ent);
    bool writeNavisworksModel(DRW_NavisworksModel *ent);
    bool writeSurface(DRW_Surface *ent);
    bool registerSurfaceEntityClass(DRW_Surface *ent);
    bool registerDwgEntityClassInstance(std::uint16_t classNumber,
                                        std::uint32_t handle);

    /// Define a user-block and return its BLOCK_RECORD handle. Use the
    /// balanced beginBlockContent()/endBlockContent() scope from
    /// `writeBlocks()` to write its owned entities. INSERT names are resolved
    /// against the set of registered user blocks during encoding.
    std::uint32_t defineBlock(const std::string& name, const DRW_Coord& basePoint,
                        int insUnits = 0);

    bool beginBlockContent(std::uint32_t blockRecordHandle);
    bool endBlockContent();

    /// Reserve a preserved source handle BEFORE write() so it can never be
    /// minted by defineBlock()/next() during the write.  The caller (filter)
    /// uses this for fixed-type OBJECTS (DICTIONARY/XRECORD/GROUP/LAYOUT/
    /// ACDBPLACEHOLDER/MLINESTYLE) and raw objects that carry a low source
    /// handle: without it, a block-record handle minted by defineBlock can
    /// collide with such an object's preserved handle → duplicate object-map
    /// entry → writeDwgHandles() fails → BAD_OPEN aborts the whole save.
    /// Reserved handles are seeded into the writer at the top of write()
    /// (before writeBlocks); calling this is a no-op once write() has run.
    /// Mirrors `dxfRW::reserveHandle`; returns false and latches the writer if
    /// the reservation cannot be recorded. (write-review P3 #1)
    bool reserveHandle(std::uint32_t h) {
        if (h == (std::numeric_limits<std::uint32_t>::max)()) {
            m_handleReservationFailed = true;
            return false;
        }
        try {
            m_reservedHandles.insert(h);
            return true;
        } catch (...) {
            m_handleReservationFailed = true;
            return false;
        }
    }

    /// Allocate a collision-free entity handle after caller reservations have
    /// been seeded into the DWG writer.  The filter uses this before entity
    /// emission so OBJECTS references can resolve source handles to the
    /// handles actually written to the file.
    std::uint32_t allocNextHandle();

    /// Table-record registration API — invoked from the iface's writeLTypes/
    /// writeLayers/etc. callbacks.  Each method normalises the name, deduplicates
    /// against standard entries, allocates a handle, and queues the record for
    /// emission in writeDwgObjects().  Returns false if invoked outside write().
    bool addLType(DRW_LType *ent);
    bool addLayer(DRW_Layer *ent);
    bool addTextstyle(DRW_Textstyle *ent);
    bool addUCS(DRW_UCS *ent);
    bool addView(DRW_View *ent);
    bool addVport(DRW_Vport *ent);
    bool addViewportEntityHeader(DRW_ViewportEntityHeader *ent);
    bool addDimstyle(DRW_Dimstyle *ent);
    bool addAppId(DRW_AppId *ent);
    bool writeAcDbPlaceholder(DRW_AcDbPlaceholder *object);
    bool writeVbaProject(DRW_VbaProject *object);
    bool writeDbColor(DRW_DbColor *object);
    bool registerDbColorObjectClass(DRW_DbColor *object);
    bool registerPlotSettingsObjectClass(std::uint32_t handle = 0);
    bool validateDwgClassInstanceCounts() const;
    bool registerDimensionAssociationObjectClass(DRW_DimensionAssociation *object);
    bool writeDimensionAssociation(DRW_DimensionAssociation *object);
    bool registerEvaluationGraphObjectClass(DRW_EvaluationGraph *object);
    bool writeEvaluationGraph(DRW_EvaluationGraph *object);
    bool writeBlockRepresentationData(DRW_BlockRepresentationData *object);
    bool registerSunObjectClass(DRW_Sun *object);
    bool writeSun(DRW_Sun *object);
    bool registerMLeaderStyleObjectClass(DRW_MLeaderStyle *object);
    bool writeMLeaderStyle(DRW_MLeaderStyle *object);
    bool registerTableStyleObjectClass(DRW_TableStyle *object);
    bool writeTableStyle(DRW_TableStyle *object);
    bool registerMaterialObjectClass(DRW_Material *object);
    bool writeMaterial(DRW_Material *object);
    bool registerLightListObjectClass(DRW_LightList *object);
    bool writeLightList(DRW_LightList *object);
    bool registerBackgroundObjectClass(DRW_Background *object);
    bool writeBackground(DRW_Background *object);
    bool registerSunStudyObjectClass(DRW_SunStudy *object);
    bool writeSunStudy(DRW_SunStudy *object);
    bool registerMotionPathObjectClass(DRW_MotionPath *object);
    bool writeMotionPath(DRW_MotionPath *object);
    bool registerCurvePathObjectClass(DRW_CurvePath *object);
    bool writeCurvePath(DRW_CurvePath *object);
    bool registerPointPathObjectClass(DRW_PointPath *object);
    bool writePointPath(DRW_PointPath *object);
    bool registerObjectPtrObjectClass(DRW_ObjectPtr *object);
    bool writeObjectPtr(DRW_ObjectPtr *object);
    bool registerPartialViewingIndexObjectClass(DRW_PartialViewingIndex *object);
    bool writePartialViewingIndex(DRW_PartialViewingIndex *object);
    bool registerRenderSettingsObjectClass(DRW_RenderSettings *object);
    bool writeRenderSettings(DRW_RenderSettings *object);
    bool registerVisualStyleObjectClass(DRW_VisualStyle *object);
    bool writeVisualStyle(DRW_VisualStyle *object);
    bool writeDictionary(DRW_Dictionary *object);
    bool writeXRecord(DRW_XRecord *object);
    bool writePlotSettings(DRW_PlotSettings *object);
    bool writeLayout(DRW_Layout *object);
    bool writeGroup(DRW_Group *object);
    bool writeMLineStyle(DRW_MLineStyle *object);
    bool registerRasterVariablesObjectClass(DRW_RasterVariables *object);
    bool writeRasterVariables(DRW_RasterVariables *object);
    bool registerWipeoutVariablesObjectClass(DRW_WipeoutVariables *object);
    bool writeWipeoutVariables(DRW_WipeoutVariables *object);
    /// Source-compatible no-op: WIPEOUT is fixed DWG entity type 1109.
    bool registerWipeoutEntityClass();
    bool registerImageDefReactorObjectClass(DRW_ImageDefinitionReactor *object);
    bool registerGeoDataObjectClass(DRW_GeoData *object);
    bool writeGeoData(DRW_GeoData *object);
    bool registerSpatialFilterObjectClass(DRW_SpatialFilter *object);
    bool writeSpatialFilter(DRW_SpatialFilter *object);
    // PR 8d.2a — five small no-storage OBJECTS families.
    bool registerScaleObjectClass(DRW_Scale *object);
    bool writeScale(DRW_Scale *object);
    bool registerIDBufferObjectClass(DRW_IDBuffer *object);
    bool writeIDBuffer(DRW_IDBuffer *object);
    bool registerLayerIndexObjectClass(DRW_LayerIndex *object);
    bool writeLayerIndex(DRW_LayerIndex *object);
    bool registerSpatialIndexObjectClass(DRW_SpatialIndex *object);
    bool writeSpatialIndex(DRW_SpatialIndex *object);
    bool registerDictionaryVarObjectClass(DRW_DictionaryVar *object);
    bool writeDictionaryVar(DRW_DictionaryVar *object);
    // PR 8d.2b — four larger no-storage OBJECTS families.
    bool registerDictionaryWithDefaultObjectClass(DRW_DictionaryWithDefault *object);
    bool writeDictionaryWithDefault(DRW_DictionaryWithDefault *object);
    bool registerSortEntsTableObjectClass(DRW_SortEntsTable *object);
    bool writeSortEntsTable(DRW_SortEntsTable *object);
    bool registerFieldListObjectClass(DRW_FieldList *object);
    bool writeFieldList(DRW_FieldList *object);
    bool registerFieldObjectClass(DRW_Field *object);
    bool writeField(DRW_Field *object);
    bool registerUnderlayDefinitionObjectClass(DRW_UnderlayDefinition *object);
    bool registerUnderlayEntityClass(DRW_Underlay::Kind kind);
    bool registerNavisworksModelEntityClass();
    bool writeUnderlayDefinition(DRW_UnderlayDefinition *object);
    bool registerRawDwgObjectClass(const DRW_UnsupportedObject *object);
    bool writeRawDwgObject(DRW_UnsupportedObject *object);
    bool writeRawDwgSection(const DRW_RawDwgSection *section);
    bool registerSectionObjectClass(DRW_Section *object);
    bool writeSection(const DRW_Section *section);
    bool registerTvDevicePropertiesObjectClass(DRW_TvDeviceProperties *object);
    bool writeTvDeviceProperties(DRW_TvDeviceProperties *object);
    bool registerVxControlObjectClass(DRW_VxControl *object);
    bool writeVxControl(DRW_VxControl *object);
    bool registerVxTableRecordObjectClass(DRW_VxTableRecord *object);
    bool writeVxTableRecord(DRW_VxTableRecord *object);

    bool getPreview();
    DRW::Version getVersion() const {return version;}

    /// Return whether the active target writer can encode the modern
    /// DataStorage presence bit for the supplied typed binding.
    /// Unknown bindings, unsupported phases, and version-gated writers are
    /// rejected before the filter admits a typed OBJECTS record.
    bool canWriteDwgDataStorageBinding(
        DwgDataStorageWriterBinding binding) const;
    DRW::error getError() const {return error;}
    /// The resolved source codepage name (e.g. "ANSI_1252"), captured from the
    /// reader's DRW_TextCodec after a successful read. Empty before any read.
    std::string getCodePage() const { return codePage; }
    /// Per-entity parseDwg failures accumulated during the load. These
    /// are warnings — the file still loads with the surviving entities.
    /// Zero on a clean load. Surface alongside the entity count so users
    /// know how many entities were skipped.
    size_t getEntityParseFailures() const;
    /// Per-object parseDwg failures accumulated during the OBJECTS-section
    /// load. Like getEntityParseFailures, these are non-fatal warnings — the
    /// file still loads with the surviving objects. Zero on a clean load.
    size_t getObjectParseFailures() const;
    /// CLASSES-section CRC mismatches (warn-only compatibility policy). Zero
    /// CRC fields are treated as absent for legacy writers; non-zero
    /// mismatches are surfaced as a non-fatal diagnostic.
    size_t getClassesCrcMismatch() const;
    /// AC1027+ header, system-page, and data-page CRC/checksum mismatches.
    /// These diagnostics do not include structural parse failures.
    size_t getR2007CrcMismatch() const;
    /// AC1018 encrypted file-header CRC mismatches. These are non-fatal
    /// diagnostics; structural section/page failures still fail the read.
    size_t getR2004CrcMismatch() const;
    /// AC1027+ modeler/surface entities that advertised a DataStorage payload
    /// but had no matching handle-keyed record. Non-fatal compatibility data.
    size_t getDataStorageLinkFailures() const;
    /// AC1027+ DataStorage records that were not referenced by a parsed
    /// modeler/surface entity. Non-fatal compatibility data.
    size_t getDataStorageOrphanRecords() const;
    /// Copy the location-bearing integrity observations from the most recent
    /// read. Records are appended in parser encounter order, including
    /// distinct observations for the same page or frame. The vector is a
    /// snapshot after read() returns; while a read is in progress it may expose
    /// the live best-effort prefix. Warnings do not make the read fail;
    /// structural errors do.
    std::vector<DwgIntegrityDiagnostic> getIntegrityDiagnostics() const;
    /// Number of integrity observations omitted after the bounded first-prefix
    /// report was full. The count includes allocation failures while recording
    /// a diagnostic and saturates at size_t max. Legacy aggregate counters
    /// still include omitted observations. A retry starts a fresh snapshot.
    std::size_t getIntegrityDiagnosticsDropped() const;
    /// Vendor-extension custom-class entities (oType >= 500) silently
    /// dropped because libdxfrw has no parser for their proprietary
    /// binary layout — typically AutoCAD Mechanical (AmgStdPart aka
    /// STDPART2D, AcmBomRow, etc.) or other vertical-product classes.
    /// Their geometry, if any, never reaches the renderer.  Keyed by
    /// DXF recName, value is the instance count.  Empty on a stock
    /// AutoCAD file.  Caller can format a user-facing summary.
    std::unordered_map<std::string, size_t> getSkippedCustomClasses() const;
    /// Unsupported OBJECTS-section records encountered during read. Keyed by
    /// DXF recName for custom classes or by fixed type code for fixed objects.
    std::unordered_map<std::string, size_t> getSkippedUnsupportedObjects() const;
    /// Number of render primitives recovered by decoding the cached proxy
    /// graphics of raw-net custom entities (STDPART2D, AEC_*, …). Non-zero
    /// means previously-invisible geometry now renders.
    size_t getDecodedProxyPrimitives() const { return m_decodedProxyPrimitives; }
    /// Layer / linetype names in file storage order — the index space the
    /// proxy-graphics ATTRIBUTE_LAYER/LINETYPE opcodes reference. Exposed for
    /// regression-testing the index→name mapping against the dwgread oracle.
    const std::vector<std::string>& getLayerNameOrder() const { return m_layerNameOrder; }
    const std::vector<std::string>& getLtypeNameOrder() const { return m_ltypeNameOrder; }
bool testReader();
    void setDebug(DRW::DebugLevel lvl);

    friend class dwgReader;

private:
    friend class DwgCompoundWriteTestAccess;
    friend class DwgWriteFailureTestAccess;

    enum class DwgWriteFailurePoint : std::uint8_t {
        AfterFirstDeferredTableRecord = 1,
        AfterFirstBlockRecord = 2,
        BeforeEndBlockFrame = 3,
        AfterEndBlockFrame = 4,
        AfterFirstBlockOwnedEntity = 5,
    };

    void setDwgWriteFailurePointForTest(DwgWriteFailurePoint point);
    bool writeDwgObjectsForTest();
    bool emitDeferredBlockControlForTest();

    struct DwgWriteTransaction;

    enum class WriteSkipKind {
        Entity,
        TableRecord,
        Object,
        ClassRegistration,
        RawObject,
        RawSection,
        BlockDefinition
    };

    [[nodiscard]] bool openFile(std::ifstream *filestr);
    [[nodiscard]] bool openBuffer(std::unique_ptr<dwgBuffer> buffer);
    [[nodiscard]] bool readInstalledReader();
    void captureReaderDiagnostics();
    void resetReadDiagnostics();
    void resetWriteSkipCounters();
    [[nodiscard]] bool recordWriteResult(WriteSkipKind kind, bool ok);
    [[nodiscard]] bool writeObjectTransaction(
        WriteSkipKind kind, const std::function<bool()>& write);
    [[nodiscard]] bool writeObjectTransaction(
        WriteSkipKind kind, DRW_TableEntry *mutationTarget,
        const std::function<bool()>& write);
    [[nodiscard]] bool encodeEntityForWrite(
        DRW_Entity *ent, DRW_Entity *mutationTarget = nullptr);
    struct DwgEntityWriteState;
    struct DwgTableWriteState;
    [[nodiscard]] DwgEntityWriteState captureEntityWriteState(
        DRW_Entity *entity) const;
    [[nodiscard]] DwgTableWriteState captureTableWriteState(
        DRW_TableEntry *entry) const;
    void restoreEntityWriteState(const DwgEntityWriteState& state);
    void restoreTableWriteState(const DwgTableWriteState& state);
    void restoreEntityWriteStates(
        const std::vector<DwgEntityWriteState>& states);
    void restoreTableWriteStates(
        const std::vector<DwgTableWriteState>& states);
    void rememberCallerMutations(
        const std::vector<DwgEntityWriteState>& states);
    void rememberCallerMutation(const DwgTableWriteState& state);
    void forgetCallerMutations(
        const std::vector<DwgEntityWriteState>& states);
    [[nodiscard]] bool consumeCompoundSeqEndFailureForTest() noexcept {
        const bool fail = m_failNextCompoundSeqEndForTest;
        m_failNextCompoundSeqEndForTest = false;
        return fail;
    }
    [[nodiscard]] bool consumeFieldClassRegistrationFailureForTest() noexcept {
        const bool fail = m_failNextFieldClassRegistrationForTest;
        m_failNextFieldClassRegistrationForTest = false;
        return fail;
    }
    [[nodiscard]] bool processDwg();
    static DRW::Version sniffVersion(dwgBuffer *buffer);
    static std::unique_ptr< dwgReader > createReaderForVersion(DRW::Version version, std::unique_ptr<dwgBuffer> buffer, dwgRW *p);

    DRW::Version version { DRW::UNKNOWNV };
    DRW::error error { DRW::BAD_NONE };
    std::string fileName;
    bool applyExt { false }; /*apply extrusion in entities to conv in 2D?*/
    std::string codePage;
    DRW_Interface *iface { nullptr };
    std::unique_ptr< dwgReader > reader;
    std::unique_ptr< dwgWriter > writer;
    /// Handles reserved by the caller via reserveHandle() before write();
    /// seeded into the writer's HandleAllocator before writeBlocks so a
    /// preserved source handle can't be minted by defineBlock. (P3 #1)
    std::set<std::uint32_t> m_reservedHandles;
    bool m_handleReservationFailed {false};
    /// Caller-populated on write via `iface->writeHeader(header)` —
    /// mirrors `dxfRW`'s local `DRW_Header header;` at the top of
    /// `dxfRW::write`.  Owned for the lifetime of the writer instance.
    DRW_Header header;
    /// Captured from reader->m_entityParseFailures before reader.reset()
    /// so getEntityParseFailures() works post-read.
    size_t m_entityParseFailures { 0 };
    /// Captured from reader->m_objectParseFailures before reader.reset()
    /// so getObjectParseFailures() works post-read.
    size_t m_objectParseFailures { 0 };
    /// Captured from reader->m_classesCrcMismatch before reader.reset()
    /// so getClassesCrcMismatch() works post-read.
    size_t m_classesCrcMismatch { 0 };
    size_t m_r2007CrcMismatch { 0 };
    size_t m_r2004CrcMismatch { 0 };
    /// Captured location-bearing integrity diagnostics before reader.reset().
    std::vector<DwgIntegrityDiagnostic> m_integrityDiagnostics;
    std::size_t m_integrityDiagnosticsDropped {0};
    /// Captured from reader->m_dataStorageLinkFailures before reader.reset().
    size_t m_dataStorageLinkFailures { 0 };
    /// Captured from reader->m_dataStorageOrphanRecords before reader.reset().
    size_t m_dataStorageOrphanRecords { 0 };
    /// Captured from reader->m_skippedCustomClasses before reader.reset()
    /// so getSkippedCustomClasses() works post-read.
    std::unordered_map<std::string, size_t> m_skippedCustomClasses;
    /// Captured from reader->m_skippedUnsupportedObjects before reader.reset()
    /// so getSkippedUnsupportedObjects() works post-read.
    std::unordered_map<std::string, size_t> m_skippedUnsupportedObjects;
    /// Captured from reader->m_decodedProxyPrimitives before reader.reset()
    /// so getDecodedProxyPrimitives() works post-read.
    size_t m_decodedProxyPrimitives { 0 };
    /// Captured layer / linetype storage order (proxy index space) before
    /// reader.reset() so the getters work post-read.
    std::vector<std::string> m_layerNameOrder;
    std::vector<std::string> m_ltypeNameOrder;
    WriteSkipCounters m_writeSkipCounters;
    std::size_t m_successfulEntityWrites { 0 };
    std::uint32_t m_lastSuccessfulEntityHandle { 0 };
    DRW::DwgObjectFrameReceipt m_lastDwgObjectFrame;
    bool m_lastDwgObjectFrameValid { false };
    DRW::DwgCompoundEntityWriteReceipt m_lastDwgCompoundEntityWriteReceipt;
    std::unique_ptr<DwgWriteTransaction> m_lastWriteTransaction;
    std::vector<std::unique_ptr<DwgWriteTransaction>>
        m_dwgWriteTransactions;
    std::uint64_t m_nextDwgWriteTransaction { 1 };
    bool m_requiredWriteFailure { false };
    bool m_failNextCompoundSeqEndForTest { false };
    bool m_failNextFieldClassRegistrationForTest { false };
    bool m_failAfterFirstBlockOwnedEntityForTest { false };

};

/// Deprecated alias: existing call sites continue to compile.  Remove
/// after one release cycle once internal renames are propagated.
using dwgR = dwgRW;

#endif
