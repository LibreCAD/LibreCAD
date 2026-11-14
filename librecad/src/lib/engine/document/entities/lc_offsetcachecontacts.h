/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#ifndef LC_OFFSETCACHECONTACTS_H
#define LC_OFFSETCACHECONTACTS_H

#include "lc_ellipseoffsetgeometry.h"

namespace LC_EllipseOffsetGeometry {

struct CacheContactResult;

/**
 * Complete strict contact query over an owned, directed copy of the cache.
 * Segment k has branch 0 and domain [k,k+1]. This is a contact certificate only:
 * it says nothing about reference correspondence, coverage or global validity.
 * No budget, layout or caller-owned geometry is retained.
 */
class CacheContactAnalysis {
public:
    ~CacheContactAnalysis();
    CacheContactAnalysis(CacheContactAnalysis&&) noexcept;
    CacheContactAnalysis& operator=(CacheContactAnalysis&&) noexcept;
    CacheContactAnalysis(const CacheContactAnalysis&) = delete;
    CacheContactAnalysis& operator=(const CacheContactAnalysis&) = delete;

    /** Immutable normalized geometry, also owned by the certified session. */
    const LC_ParametricCurves& curves() const noexcept;
    std::size_t spanCount() const noexcept;
    /** Original layout index, or nullopt for an invalid normalized segment. */
    std::optional<std::size_t> rawSpanId(std::size_t segment) const noexcept;
    /** Original layout data, not the oriented [0,1] provider copy. */
    const CacheSpan* rawSpan(std::size_t segment) const noexcept;
    bool reversed() const noexcept;
    bool closed() const noexcept;

    std::size_t contactCount() const noexcept;
    bool hasValidProofs() const noexcept;
    LC_CertifiedRootHandle contactHandle(std::size_t index) const;
    std::optional<LC_CertifiedRootSnapshot> snapshot(const LC_CertifiedRootHandle&) const;
    /** Accounts exactly once on success, refusal and exception; never retains budget. */
    LC_CertifiedContractionResult contract(const LC_CertifiedRootHandle&, WorkBudget& budget);

private:
    struct Impl;
    explicit CacheContactAnalysis(std::unique_ptr<Impl> impl);
    friend CacheContactResult analyseCacheContacts(const CacheLayoutResult&, const ReferenceFrame&,
                                                   bool, bool, WorkBudget&);
    std::unique_ptr<Impl> m_impl;
};

struct CacheContactResult {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    /** Cumulative caller work, including any work spent before this call. */
    CheckWork work{};
    /** Strict-core diagnostics, including the exact refusal stage's entered work. */
    LC_IntersectionWorkStats queryWork{};
    /** Present only on complete success; there are no partial public isolations. */
    std::unique_ptr<CacheContactAnalysis> analysis;
};

/**
 * Inspect a previously validated degree-1/degree-3 normalized cache layout.
 * Copies and reverses spans/controls when requested, preserving raw identities.
 * Adjacent segments and a declared, exactly closed seam are excluded ONLY by
 * a whole-path common-projection proof, never by adjacency or chord direction.
 * closed=true requires exact end-to-start equality; false does not infer closure.
 */
CacheContactResult analyseCacheContacts(const CacheLayoutResult& layout, const ReferenceFrame& frame,
                                       bool cacheReversed, bool closed, WorkBudget& budget);

} // namespace LC_EllipseOffsetGeometry

#endif
