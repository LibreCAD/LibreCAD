/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD.org                                           **
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
    /// out to the file named at construction.  v1 supports `DRW::AC1015`
    /// (R2000) only; other versions return false with `BAD_VERSION`.
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

    /// Define an empty user-block.  Allocates fresh Block_Record + Block
    /// + ENDBLK handles, emits all three into the object stream, and
    /// appends the new Block_Record to BLOCK_CONTROL's child list.
    /// Returns the Block_Record handle, which callers use as the
    /// `blockRecH.ref` on subsequent `DRW_Insert` entities.  Must be
    /// invoked from the iface's `writeBlocks()` callback (before
    /// `writeEntities`); returns 0 if invoked outside that window.
    duint32 defineBlock(const std::string& name, const DRW_Coord& basePoint);

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

};

/// Deprecated alias: existing call sites continue to compile.  Remove
/// after one release cycle once internal renames are propagated.
using dwgR = dwgRW;

#endif // LIBDWGR_H
