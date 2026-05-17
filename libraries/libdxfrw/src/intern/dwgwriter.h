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

#include <fstream>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "../drw_base.h"
#include "../drw_header.h"
#include "dwgbufferw.h"
#include "dwgutil.h"

class DRW_TextCodec;
class DRW_Entity;

/// Allocates object handles for a fresh DWG write.  Reserves the
/// canonical R2000 fixed-handle table (0x01-0x18, skipping 0x04 which
/// is unused) so user-allocated handles never collide with them.
///
/// Lifecycle: instantiate, call `seedReserved()` once, then `next()`
/// for each new object that needs a handle.  Reserved handles are
/// referenced directly (e.g. layer "0" is always 0x12) — callers do
/// not request them through `next()`.
///
/// For round-trip reads-then-writes, future work will add a `seed(maxSeen)`
/// hook that initializes from the document's HANDSEED header variable
/// and skips any handles already in use.  Phase 3 only needs fresh-doc.
class HandleAllocator {
public:
    HandleAllocator() = default;

    /// Pre-seed the reserved set with R2000's canonical fixed handles.
    /// After this, `next()` skips anything in the reserved set.
    void seedReserved() {
        // Control objects (0x01..0x0B).  0x04 is intentionally unused.
        m_reserved.insert(0x01);  // BLOCK_CONTROL_OBJECT
        m_reserved.insert(0x02);  // LAYER_CONTROL_OBJECT
        m_reserved.insert(0x03);  // STYLE_CONTROL_OBJECT
        m_reserved.insert(0x05);  // LTYPE_CONTROL_OBJECT
        m_reserved.insert(0x06);  // VIEW_CONTROL_OBJECT
        m_reserved.insert(0x07);  // UCS_CONTROL_OBJECT
        m_reserved.insert(0x08);  // VPORT_CONTROL_OBJECT
        m_reserved.insert(0x09);  // APPID_CONTROL_OBJECT
        m_reserved.insert(0x0A);  // DIMSTYLE_CONTROL_OBJECT
        m_reserved.insert(0x0B);  // VPORT_ENTITY_HEADER_CONTROL_OBJECT (R2000 only)
        // Phase 3.5 reserves 0x0C/0x0D/0x0E for the NOD + sub-dicts; Phase 3
        // leaves them free so user allocations could in principle use them,
        // but the convention is first-user = 0x30 anyway.
        // Table records.
        m_reserved.insert(0x0F);  // LTYPE "BYBLOCK"
        m_reserved.insert(0x10);  // LTYPE "BYLAYER"
        m_reserved.insert(0x11);  // LTYPE "CONTINUOUS"
        m_reserved.insert(0x12);  // LAYER "0"
        m_reserved.insert(0x13);  // STYLE "STANDARD"
        m_reserved.insert(0x14);  // APPID "ACAD"
        m_reserved.insert(0x15);  // DIMSTYLE "STANDARD"
        m_reserved.insert(0x16);  // VPORT "*ACTIVE"
        m_reserved.insert(0x17);  // BLOCK_RECORD "*MODEL_SPACE"
        m_reserved.insert(0x18);  // BLOCK_RECORD "*PAPER_SPACE"
        // Phase 4d Block + ENDBLK entities for *Model_Space / *Paper_Space.
        // Master plan calls 0x19-0x1E "reserved but unused"; we use 0x1B-0x1E
        // for the four Block entities the BLOCK_CONTROL phantom-handle pair
        // points at via their Block_Records.
        m_reserved.insert(0x1B);  // BLOCK "*Model_Space"
        m_reserved.insert(0x1C);  // ENDBLK "*Model_Space"
        m_reserved.insert(0x1D);  // BLOCK "*Paper_Space"
        m_reserved.insert(0x1E);  // ENDBLK "*Paper_Space"
    }

    /// Mark a specific handle as in-use.  Used during read-then-write
    /// to preserve source handles; idempotent.
    void reserve(duint32 h) { m_reserved.insert(h); }

    /// Allocate the next unused handle ≥ `m_next`, skipping reserved.
    /// Marks the returned handle as reserved so subsequent calls don't
    /// return the same value.
    duint32 next() {
        while (m_reserved.count(m_next))
            ++m_next;
        duint32 h = m_next++;
        m_reserved.insert(h);
        return h;
    }

    /// High-water mark.  Used to populate the HANDSEED header variable.
    duint32 current() const { return m_next; }

private:
    /// First candidate for user-allocated handles.  All canonical
    /// reserved handles are below 0x30, so seeding starts here.
    duint32 m_next {0x30};
    std::set<duint32> m_reserved;
};

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

    /// Define an empty user-block.  Allocates fresh handles for the
    /// Block_Record + Block + ENDBLK trio and emits all three to the
    /// object stream.  Appends the block_record handle to the deferred
    /// BLOCK_CONTROL list so a later `emitDeferredBlockControl` call
    /// includes it.  Returns the Block_Record handle (suitable for
    /// `DRW_Insert::blockRecH.ref`).  Returns 0 on failure.
    virtual duint32 defineBlock(const std::string& name,
                                const DRW_Coord& basePoint) = 0;

    /// Emit BLOCK_CONTROL with the user-block list captured by all
    /// prior `defineBlock` calls.  Invoked by the orchestrator after
    /// `iface->writeBlocks()` so the BLOCK_CONTROL.numEntries reflects
    /// user blocks (+ the canonical 2 phantom modelspace/paperspace).
    virtual bool emitDeferredBlockControl() = 0;

    /// Accumulator (exposed for tests + for sibling classes that
    /// need byte-level inspection).  Reserved unless a test asks.
    const std::vector<duint8>& buffer() const { return m_buf.data(); }

    /// Byte offset of the start of the OBJECTS data region within m_buf.
    /// writeDwgHandles subtracts this from each object's m_buf offset so
    /// the HANDLES section stores section-relative positions.  For R2000
    /// (whole-file buffer) the base is 0; R2004 overrides to point past
    /// the HEADER + CLASSES sections that precede the object stream.
    virtual duint32 objectBaseOffset() const { return 0; }

    /// First-available user handle from the allocator.  `dwgRW::write`
    /// uses this to auto-populate `DRW_Header::handSeed` for fresh
    /// documents where the caller did not set one explicitly — without
    /// it, the encoder emits null HANDSEED and AutoCAD refreshes on
    /// first open, marking the file modified.  See [Risk 4j].
    duint32 highWaterHandle() const { return m_handles.current(); }

protected:
    /// In-memory byte accumulator. All section bodies append here;
    /// final flush copies to `m_stream` in one `write()` call.
    dwgBufferW m_buf;

    /// Per-section start byte offsets, indexed by `secEnum::DWGSection`.
    /// Populated as each section begins emitting; consumed by `finalize()`
    /// to back-patch the file-header section locator records.
    std::map<int, duint32> m_sectionOffsets;

    /// Per-section byte sizes, same indexing as `m_sectionOffsets`.
    std::map<int, duint32> m_sectionSizes;

    std::ofstream *m_stream {nullptr};
    DRW_Header *m_header {nullptr};

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
};

#endif // DWGWRITER_H
