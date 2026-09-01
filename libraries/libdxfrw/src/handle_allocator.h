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

#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <set>

#include "drw_base.h"
#include "intern/dwg_fixed_handles.h"

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
        reserve(DRW::DwgBlockControlHandle);
        reserve(DRW::DwgLayerControlHandle);
        reserve(DRW::DwgStyleControlHandle);
        reserve(DRW::DwgLtypeControlHandle);
        reserve(DRW::DwgViewControlHandle);
        reserve(DRW::DwgUcsControlHandle);
        reserve(DRW::DwgVportControlHandle);
        reserve(DRW::DwgAppIdControlHandle);
        reserve(DRW::DwgDimstyleControlHandle);
        reserve(DRW::DwgViewportEntityHeaderControlHandle);
        // Named Objects Dictionary and its mandatory ACAD_GROUP child.
        reserve(DRW::DwgNamedObjectsDictionaryHandle);
        reserve(DRW::DwgAcadGroupDictionaryHandle);
        // Table records.
        reserve(DRW::DwgLtypeByBlockHandle);
        reserve(DRW::DwgLtypeByLayerHandle);
        reserve(DRW::DwgLtypeContinuousHandle);
        reserve(DRW::DwgLayer0Handle);
        reserve(DRW::DwgStandardTextStyleHandle);
        reserve(DRW::DwgAcadAppIdHandle);
        reserve(DRW::DwgStandardDimstyleHandle);
        reserve(DRW::DwgActiveVportHandle);
        reserve(DRW::DwgModelSpaceBlockRecordHandle);
        reserve(DRW::DwgPaperSpaceBlockRecordHandle);
        // Phase 4d Block + ENDBLK entities for *Model_Space / *Paper_Space.
        // Master plan calls 0x19-0x1E "reserved but unused"; we use 0x1B-0x1E
        // for the four Block entities the BLOCK_CONTROL phantom-handle pair
        // points at via their Block_Records.
        reserve(DRW::DwgModelSpaceBlockEntityHandle);
        reserve(DRW::DwgModelSpaceEndBlockEntityHandle);
        reserve(DRW::DwgPaperSpaceBlockEntityHandle);
        reserve(DRW::DwgPaperSpaceEndBlockEntityHandle);
    }

    /// Mark a specific handle as in-use.  Used during read-then-write
    /// to preserve source handles; idempotent.
    void reserve(std::uint32_t h) {
        if (h >= m_next && h == std::numeric_limits<std::uint32_t>::max())
            throw std::overflow_error("DWG/DXF handle allocator exhausted");
        // Keep caller-owned reservations separate from handles minted during
        // a write.  This lets a codec be reused without losing source/raw
        // handles that were reserved before the write started.
        m_explicitReserved.insert(h);
        if (h >= m_next) {
            m_next = h + 1;
        }
    }

    /// Allocate the next unused handle ≥ `m_next`, skipping reserved.
    /// Marks the returned handle as reserved so subsequent calls don't
    /// return the same value.
    std::uint32_t next() {
        while (isReserved(m_next)) {
            if (m_next == std::numeric_limits<std::uint32_t>::max())
                throw std::overflow_error("DWG/DXF handle allocator exhausted");
            ++m_next;
        }
        if (m_next == std::numeric_limits<std::uint32_t>::max())
            throw std::overflow_error("DWG/DXF handle allocator exhausted");
        std::uint32_t h = m_next++;
        m_allocated.insert(h);
        return h;
    }

    /// Reset handles minted by the previous write while retaining explicit
    /// source/raw reservations.  Failed allocations are not retained as
    /// reservations across writes; within one write, callers never receive a
    /// handle twice because `next()` is monotonic.
    void resetGenerated() {
        m_allocated.clear();
        m_next = 0x30;
        if (!m_explicitReserved.empty()) {
            const auto highest = *m_explicitReserved.rbegin();
            if (highest == std::numeric_limits<std::uint32_t>::max())
                m_next = highest;
            else if (highest >= m_next)
                m_next = highest + 1;
        }
    }

    /// High-water mark.  Used to populate the HANDSEED header variable.
    std::uint32_t current() const { return m_next; }

private:
    /// First candidate for user-allocated handles.  All canonical
    /// reserved handles are below 0x30, so seeding starts here.
    std::uint32_t m_next {0x30};
    bool isReserved(std::uint32_t h) const {
        return m_explicitReserved.count(h) != 0 || m_allocated.count(h) != 0;
    }

    std::set<std::uint32_t> m_explicitReserved;
    std::set<std::uint32_t> m_allocated;
};

#endif // HANDLE_ALLOCATOR_H
