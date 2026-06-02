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

#ifndef HANDLE_ALLOCATOR_H
#define HANDLE_ALLOCATOR_H

#include <cstdint>
#include <set>

/// Allocates object handles for a fresh DWG or DXF write.  Both write paths
/// reserve their canonical fixed-handle table up front so user-allocated
/// handles (and verbatim handles preserved from a raw passthrough net) can
/// never collide with them.
///
/// Lifecycle: instantiate, call the path-specific seed once (the DWG path uses
/// `seedReserved()`; the DXF codec seeds its own literal set), optionally
/// `reserve()` any preserved/raw handle, then `next()` for each new object that
/// needs a handle.  Reserved handles are referenced directly (e.g. the DWG
/// layer "0" is always 0x12) — callers do not request them through `next()`.
class HandleAllocator {
public:
    HandleAllocator() = default;

    /// Pre-seed the reserved set with R2000's canonical fixed handles (DWG
    /// write path).  After this, `next()` skips anything in the reserved set.
    /// The DXF codec does NOT use this — its fixed literals differ; it reserves
    /// its own set via `reserve()`.
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
    void reserve(std::uint32_t h) {
        m_reserved.insert(h);
        if (h >= m_next)
            m_next = h + 1;
    }

    /// Allocate the next unused handle ≥ `m_next`, skipping reserved.
    /// Marks the returned handle as reserved so subsequent calls don't
    /// return the same value.
    std::uint32_t next() {
        while (m_reserved.count(m_next))
            ++m_next;
        std::uint32_t h = m_next++;
        m_reserved.insert(h);
        return h;
    }

    /// High-water mark.  Used to populate the HANDSEED header variable.
    std::uint32_t current() const { return m_next; }

private:
    /// First candidate for user-allocated handles.  All canonical
    /// reserved handles are below 0x30, so seeding starts here.
    std::uint32_t m_next {0x30};
    std::set<std::uint32_t> m_reserved;
};

#endif // HANDLE_ALLOCATOR_H
