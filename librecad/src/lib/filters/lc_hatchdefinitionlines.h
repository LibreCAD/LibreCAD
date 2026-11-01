/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LC_HATCHDEFINITIONLINES_H
#define LC_HATCHDEFINITIONLINES_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "rs_hatch.h"
#include "rs_vector.h"

namespace LC_HatchDefinitionLines {

/// One PAT-local pattern definition line. The convention mirrors acad.pat:
/// base is a point in the pattern-local frame, offset is a line-local
/// vector (dx along the line direction, dy perpendicular to it), and dashes
/// are per-dash lengths (positive = ink, negative = gap, zero = dot).
struct LC_HatchLineFamily {
    double angleDeg = 0.0;
    RS_Vector base{0.0, 0.0};
    RS_Vector offset{0.0, 0.0};
    std::vector<double> dashes;
};

/// Realized WCS output ready to be fed to DRW_Hatch::patternLines.
struct ExportLines {
    std::vector<DRW_Hatch::PatternLine> lines;
    /// True when the realized lines are LibreCAD's placeholder or an
    /// approximation. False when the realized lines capture every piece a
    /// consuming reader would draw. See the HATCH_LINES_APPROXIMATE XDATA
    /// tag and setApproximateTag/stripApproximateTag below.
    bool approximate = true;
};

/// XDATA application name used by the approximate-tag group. LibreCAD's
/// APPID table always carries this name (writeAppId). Case-sensitive here;
/// readers compare 1001 case-insensitively through normalizeDwgTableName.
constexpr const char* kApplication = "LibreCad";

/// 1000 XDATA group content inside the kApplication group. The hatch writer
/// adds it to a pattern hatch that uses the placeholder or any approximate
/// family; the reader strips it on import so a re-export of an unchanged
/// hatch never duplicates it.
constexpr const char* kApproximateTag = "HATCH_LINES_APPROXIMATE";

/// Realize one PAT-local family into a DRW_Hatch::PatternLine (WCS/OCS).
/// Returns nullopt when the offset's perpendicular component is below a
/// relative-magnitude floor: a zero spacing is infinite density.
std::optional<DRW_Hatch::PatternLine> realize(const LC_HatchLineFamily& f,
                                              const RS_HatchPatternFrame& frame);

/// Build the placeholder definition line used when no exact family is known:
/// a single line at 45 degrees relative to the hatch angle, with offset
/// magnitude a quarter of the geometric mean of the pattern tile size (or
/// of the hatch scale when the tile is unavailable). Never empty.
DRW_Hatch::PatternLine placeholderLine(const RS_HatchPatternFrame& frame);

/// What the DXF/DWG hatch writer should emit as definition lines for the
/// given hatch. Today this is always the placeholder; a later step will add
/// exact families when the pattern permits one.
ExportLines forExport(const RS_Hatch& hatch);

/// Make sure the kApproximateTag is present (on=true) or absent (on=false)
/// in extData, inside a group whose 1001 equals kApplication (case-
/// insensitive). Removes a kApplication group left with no items. The strip
/// path drops a stale tag when the writer rebuilds the realized lines.
void setApproximateTag(std::vector<std::shared_ptr<DRW_Variant>>& extData, bool on);

/// Remove every kApproximateTag inside a kApplication group. Returns true
/// when at least one was removed, so the caller can record that this hatch
/// imported with an approximate marker; a re-export then rebuilds it on its
/// own instead of leaking a second copy.
bool stripApproximateTag(std::vector<std::shared_ptr<DRW_Variant>>& extData);

/// Boundary-path flag (DXF code 92) from the loop's depth in the extracted
/// tree: 1 at depth 0 (External), 16 at depth 1 (Outermost), 0 deeper.
/// Declared here so the writer-side change can use it; the writer side
/// itself stays in rs_filterdxfrw.cpp.
int boundaryPathFlags(int depth);

} // namespace LC_HatchDefinitionLines

#endif // LC_HATCHDEFINITIONLINES_H
