/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
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
