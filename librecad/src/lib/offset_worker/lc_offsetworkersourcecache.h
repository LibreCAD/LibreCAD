/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERSOURCECACHE_H
#define LC_OFFSETWORKERSOURCECACHE_H

#include "lc_offsetworkerrequest.h"

namespace LC_OffsetWorkerSourceCache {
/**
 * Canonicalizes complete ordinary branches into one editable SPLINE cache.
 * Multiple branches are represented with degree-three pieces separated by
 * full-multiplicity knots; no geometry is joined or discarded.
 */
bool combine(const std::vector<LC_OffsetWorkerRequest::Cutter>& branches,
             RS_SplineData& cache);
}

#endif
