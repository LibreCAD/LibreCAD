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
#include <string>
#include <memory>
#include <set>
#include <unordered_map>
//#include <deque>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_classes.h"
#include "drw_interface.h"

class dwgReader;
class dwgBuffer;
class dwgWriter;

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

    /// Per-entity write API — invoked from the caller's `writeEntities`
    /// iface callback.  Each method allocates a handle, populates the
    /// entity's `handle` + `layerH.ref` fields, encodes the entity to
    /// the object stream, and records the `(handle, offset)` pair so
    /// the HANDLES section emit later finds it.  Returns true on
    /// success.  Layer-by-name resolution lands in Phase 4d; for now
    /// every entity is placed on layer "0" (handle 0x12).
    bool writePoint(DRW_Point *ent);
    bool writeLine(DRW_Line *ent);
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
    bool writeMLine(DRW_MLine *ent);
    bool writeUnderlay(DRW_Underlay *ent);
    bool writePolyline(DRW_Polyline *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeMLeader(DRW_MLeader *ent);
    bool writeViewport(DRW_Viewport *ent);
    bool writeShape(DRW_Shape *ent);
    bool writeOle2Frame(DRW_Ole2Frame *ent);
    bool writeMesh(DRW_Mesh *ent);
    bool writeWipeout(DRW_Wipeout *ent);
    bool writePointCloud(DRW_PointCloud *ent);
    bool writePointCloudEx(DRW_PointCloudEx *ent);
    bool writeSurface(DRW_Surface *ent);

    /// Define an empty user-block.  Allocates fresh Block_Record + Block
    /// + ENDBLK handles, emits all three into the object stream, and
    /// appends the new Block_Record to BLOCK_CONTROL's child list.
    /// Returns the Block_Record handle, which callers use as the
    /// `blockRecH.ref` on subsequent `DRW_Insert` entities.  Must be
    /// invoked from the iface's `writeBlocks()` callback (before
    /// `writeEntities`); returns 0 if invoked outside that window.
    std::uint32_t defineBlock(const std::string& name, const DRW_Coord& basePoint,
                        int insUnits = 0);

    /// Reserve a preserved source handle BEFORE write() so it can never be
    /// minted by defineBlock()/next() during the write.  The caller (filter)
    /// uses this for fixed-type OBJECTS (DICTIONARY/XRECORD/GROUP/LAYOUT/
    /// ACDBPLACEHOLDER/MLINESTYLE) and raw objects that carry a low source
    /// handle: without it, a block-record handle minted by defineBlock can
    /// collide with such an object's preserved handle → duplicate object-map
    /// entry → writeDwgHandles() fails → BAD_OPEN aborts the whole save.
    /// Reserved handles are seeded into the writer at the top of write()
    /// (before writeBlocks); calling this is a no-op once write() has run.
    /// Mirrors `dxfRW::reserveHandle`. (write-review P3 #1)
    void reserveHandle(std::uint32_t h) { m_reservedHandles.insert(h); }

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
    bool addDimstyle(DRW_Dimstyle *ent);
    bool addAppId(DRW_AppId *ent);
    bool writeAcDbPlaceholder(DRW_AcDbPlaceholder *object);
    bool registerSunObjectClass(DRW_Sun *object);
    bool writeSun(DRW_Sun *object);
    bool registerMLeaderStyleObjectClass(DRW_MLeaderStyle *object);
    bool writeMLeaderStyle(DRW_MLeaderStyle *object);
    bool writeDictionary(DRW_Dictionary *object);
    bool writeXRecord(DRW_XRecord *object);
    bool writeLayout(DRW_Layout *object);
    bool writeGroup(DRW_Group *object);
    bool writeMLineStyle(DRW_MLineStyle *object);
    bool registerRasterVariablesObjectClass(DRW_RasterVariables *object);
    bool writeRasterVariables(DRW_RasterVariables *object);
    bool registerWipeoutVariablesObjectClass(DRW_WipeoutVariables *object);
    bool writeWipeoutVariables(DRW_WipeoutVariables *object);
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
    bool writeUnderlayDefinition(DRW_UnderlayDefinition *object);
    bool registerRawDwgObjectClass(const DRW_UnsupportedObject *object);
    bool writeRawDwgObject(DRW_UnsupportedObject *object);
    bool writeRawDwgSection(const DRW_RawDwgSection *section);

    bool getPreview();
    DRW::Version getVersion() const {return version;}
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
    /// R13/R15 CLASSES-section CRC mismatches (warn-only — a mismatch no longer
    /// fails the import). Non-fatal diagnostic; zero on a clean load.
    size_t getClassesCrcMismatch() const;
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

private:
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
    [[nodiscard]] bool encodeEntityForWrite(DRW_Entity *ent);
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

};

/// Deprecated alias: existing call sites continue to compile.  Remove
/// after one release cycle once internal renames are propagated.
using dwgR = dwgRW;

#endif
