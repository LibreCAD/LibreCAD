/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#ifndef LC_OFFSETCONTACTMATCHING_H
#define LC_OFFSETCONTACTMATCHING_H

#include "lc_offsetcachecontacts.h"

namespace LC_EllipseOffsetGeometry {

struct CacheContactPosition {
    /** Directed, normalized contact-provider segment, NOT an original raw span ID. */
    std::size_t segment = std::size_t(-1);
    double parameter = 0;
};

struct CacheContactPort {
    /** Opaque caller-owned incidence identity; this helper cannot certify reference geometry. */
    std::size_t referenceNode = std::size_t(-1);
    std::array<CacheContactPosition, 2> positions{};
    /** One occurrence, or two representations of the SAME exact ordinal and world point. */
    std::size_t positionCount = 0;
};

enum class CacheContactMatchStatus { Matched, InvalidInput, Topology, Ambiguous, LimitExceeded };

struct CacheContactMatch {
    std::size_t rootIndex = std::size_t(-1);
    std::size_t firstPort = std::size_t(-1), secondPort = std::size_t(-1);
};

struct CacheContactMatchResult {
    CacheContactMatchStatus status = CacheContactMatchStatus::InvalidInput;
    CheckWork work{};
    /** Complete only on Matched; no successful prefix is exposed on refusal. */
    std::vector<CacheContactMatch> matches;
    explicit operator bool() const noexcept { return status == CacheContactMatchStatus::Matched; }
};

/**
 * Anchor genuine issued strict roots to exact candidate positions at caller-supplied ports.
 * Binary64 Bernstein evaluation and ordinal membership are exact rational operations, not
 * tolerance comparisons or numerical root-seed tests. Two positions in one port must denote
 * one ordinal; only an explicitly closed provider folds ordinal spanCount onto zero.
 * Distinct ports must denote distinct ordinals. Every actual root needs a same-node pair,
 * and every same-node port pair needs at least one root (duplicate tail roots are allowed).
 * Unknown/non-exact alignment is Ambiguous. A complete empty contact query proves missing
 * required contacts; roots with no authorized same-node pairs prove unexpected contacts.
 *
 * One record per prepared port and per returned match; each scan/evaluation/snapshot is
 * pair-admitted and comparisons are ordering-admitted. Capacity precedes logical debit.
 * Arithmetic is fixed-degree binary64 (bounded operand sizes), not an unbounded solver.
 * Invalid port input latches InvalidInput; geometric refusal leaves the shared budget usable.
 * A moved-from or invalidated proof owner is rejected before any count/geometry access.
 * The result retains neither budget nor borrowed geometry. Root indices are meaningful
 * only with the caller-retained CacheContactAnalysis; this is NOT a global validity proof.
 */
CacheContactMatchResult matchCacheContacts(const CacheContactAnalysis& contacts,
    const std::vector<CacheContactPort>& ports, WorkBudget& budget);

} // namespace LC_EllipseOffsetGeometry

#endif
