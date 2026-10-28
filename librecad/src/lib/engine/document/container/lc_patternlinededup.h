/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
****************************************************************************/

#ifndef LC_PATTERNLINEDEDUP_H
#define LC_PATTERNLINEDEDUP_H

#include <vector>

#include "rs_line.h"

namespace LC_LoopUtils {

/**
 * @brief Merges collinear LINE pieces of a hatch pattern that overlap or touch,
 * so that no ink is drawn twice.
 *
 * The result depends only on the multiset of input pieces: a shuffled input
 * produces the same output, element by element and bitwise. Non-LINE pieces
 * (arcs, circles, ellipses, ...) are handled by the caller; this routine
 * covers only pattern LINE pieces, where the tile-border overlaps live.
 *
 * The algorithm sorts and sweeps:
 *   1. Canonicalise each piece (swap ends so start < end lexicographically).
 *      Pieces shorter than tolOffset go to the "shorts" bucket.
 *   2. Cluster by direction angle in [0, pi) with tolAngle, single linkage,
 *      with wrap-around across 0 / pi.
 *   3. Within each direction cluster, cluster by signed perpendicular offset c
 *      with tolOffset, and merge overlapping/touching intervals along the
 *      cluster's unit direction.
 *   4. Drop shorts that fall within tolOffset of a merged segment; de-duplicate
 *      the rest on a tolOffset grid with 3x3 neighbour lookup.
 *
 * @param pieces     Input LINE pieces.
 * @param tolOffset  Positive tolerance along the perpendicular to a direction
 *                   cluster and along merged intervals. Caller passes
 *                   1e-6 x the longer side of the scaled pattern tile, so a
 *                   shuffled or scaled input yields the same output and real
 *                   gaps longer than 1e-6 x tile are preserved.
 * @param tolAngle   Positive tolerance on direction angles, in radians.
 * @return Merged LINE pieces in cluster/group/interval order, then the
 *         deduplicated shorts.
 */
std::vector<RS_LineData> deduplicatePatternLines(
    std::vector<RS_LineData> pieces,
    double tolOffset,
    double tolAngle = 1e-6);

}  // namespace LC_LoopUtils

#endif  // LC_PATTERNLINEDEDUP_H
