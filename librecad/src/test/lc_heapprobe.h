/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// How much heap the process holds right now, for tests that must show a piece
// of code frees what it allocates (build it many times, and the heap must not
// grow) where the entities are created inside the code, so that a counting
// subclass can't be slipped in. Test code only.

#ifndef LC_HEAPPROBE_H
#define LC_HEAPPROBE_H

#include <cstddef>
#include <optional>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define LC_TEST_HEAP_ASAN 1
#endif
#endif
#if !defined(LC_TEST_HEAP_ASAN) && defined(__SANITIZE_ADDRESS__)
#define LC_TEST_HEAP_ASAN 1
#endif

#if defined(LC_TEST_HEAP_ASAN)
#include <sanitizer/allocator_interface.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#endif

namespace lc::test {

/**
 * Bytes the heap holds right now, where the allocator can tell us (ASan's
 * allocator statistics, or malloc_zone_statistics on macOS); otherwise nothing,
 * and the tests that use it check only the behaviour.
 */
inline std::optional<std::size_t> heapBytesInUse() {
#if defined(LC_TEST_HEAP_ASAN)
    return __sanitizer_get_current_allocated_bytes();
#elif defined(__APPLE__)
    malloc_statistics_t stats{};
    malloc_zone_statistics(nullptr, &stats);
    return stats.size_in_use;
#else
    return std::nullopt;
#endif
}

} // namespace lc::test

#endif
