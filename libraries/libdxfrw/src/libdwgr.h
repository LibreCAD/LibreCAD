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

#include <string>
#include <memory>
#include <unordered_map>
//#include <deque>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_classes.h"
#include "drw_interface.h"

class dwgReader;
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
    bool read(DRW_Interface *interface_, bool ext);

    /// Write the in-memory model (driven via DRW_Interface callbacks)
    /// out to the file named at construction.
    /// The `bin` parameter is ignored — DWG is always binary — but
    /// kept for API symmetry with `dxfRW::write`.  Returns true on
    /// success, false on error; error code accessible via `getError()`.
    bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);

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
    bool writeLWPolyline(DRW_LWPolyline *ent);
    bool writeRay(DRW_Ray *ent);
    bool writeXline(DRW_Xline *ent);
    bool writeTrace(DRW_Trace *ent);
    bool writeSolid(DRW_Solid *ent);
    bool write3dface(DRW_3Dface *ent);
    bool writeInsert(DRW_Insert *ent);
    bool writeMText(DRW_MText *ent);
    bool writeSpline(DRW_Spline *ent);
    bool writeAttrib(DRW_Attrib *ent);
    bool writeAttdef(DRW_Attdef *ent);
    bool writeHatch(DRW_Hatch *ent);
    bool writeDimension(DRW_Dimension *ent);
    bool writeTolerance(DRW_Tolerance *ent);
    bool writeLight(DRW_Light *ent);
    bool writeMLine(DRW_MLine *ent);
    bool writePolyline(DRW_Polyline *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeMLeader(DRW_MLeader *ent);
    bool writeViewport(DRW_Viewport *ent);
    bool writeShape(DRW_Shape *ent);
    bool writeOle2Frame(DRW_Ole2Frame *ent);

    /// Define an empty user-block.  Allocates fresh Block_Record + Block
    /// + ENDBLK handles, emits all three into the object stream, and
    /// appends the new Block_Record to BLOCK_CONTROL's child list.
    /// Returns the Block_Record handle, which callers use as the
    /// `blockRecH.ref` on subsequent `DRW_Insert` entities.  Must be
    /// invoked from the iface's `writeBlocks()` callback (before
    /// `writeEntities`); returns 0 if invoked outside that window.
    std::uint32_t defineBlock(const std::string& name, const DRW_Coord& basePoint,
                        int insUnits = 0);

    /// Table-record registration API — invoked from the iface's writeLTypes/
    /// writeLayers/etc. callbacks.  Each method normalises the name, deduplicates
    /// against standard entries, allocates a handle, and queues the record for
    /// emission in writeDwgObjects().  Returns false if invoked outside write().
    bool addLType(DRW_LType *ent);
    bool addLayer(DRW_Layer *ent);
    bool addTextstyle(DRW_Textstyle *ent);
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
    bool registerRasterVariablesObjectClass(DRW_RasterVariables *object);
    bool writeRasterVariables(DRW_RasterVariables *object);
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
    bool registerRawDwgObjectClass(const DRW_UnsupportedObject *object);
    bool writeRawDwgObject(DRW_UnsupportedObject *object);

    bool getPreview();
    DRW::Version getVersion(){return version;}
    DRW::error getError(){return error;}
    /// Per-entity parseDwg failures accumulated during the load. These
    /// are warnings — the file still loads with the surviving entities.
    /// Zero on a clean load. Surface alongside the entity count so users
    /// know how many entities were skipped.
    size_t getEntityParseFailures() const;
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
bool testReader();
    void setDebug(DRW::DebugLevel lvl);

private:
    bool openFile(std::ifstream *filestr);
    bool processDwg();
    static std::unique_ptr< dwgReader > createReaderForVersion(DRW::Version version, std::ifstream *stream, dwgRW *p);

private:
    DRW::Version version { DRW::UNKNOWNV };
    DRW::error error { DRW::BAD_NONE };
    std::string fileName;
    bool applyExt { false }; /*apply extrusion in entities to conv in 2D?*/
    std::string codePage;
    DRW_Interface *iface { nullptr };
    std::unique_ptr< dwgReader > reader;
    std::unique_ptr< dwgWriter > writer;
    /// Caller-populated on write via `iface->writeHeader(header)` —
    /// mirrors `dxfRW`'s local `DRW_Header header;` at the top of
    /// `dxfRW::write`.  Owned for the lifetime of the writer instance.
    DRW_Header header;
    /// Captured from reader->m_entityParseFailures before reader.reset()
    /// so getEntityParseFailures() works post-read.
    size_t m_entityParseFailures { 0 };
    /// Captured from reader->m_skippedCustomClasses before reader.reset()
    /// so getSkippedCustomClasses() works post-read.
    std::unordered_map<std::string, size_t> m_skippedCustomClasses;
    /// Captured from reader->m_skippedUnsupportedObjects before reader.reset()
    /// so getSkippedUnsupportedObjects() works post-read.
    std::unordered_map<std::string, size_t> m_skippedUnsupportedObjects;

};

/// Deprecated alias: existing call sites continue to compile.  Remove
/// after one release cycle once internal renames are propagated.
using dwgR = dwgRW;

#endif // LIBDWGR_H
