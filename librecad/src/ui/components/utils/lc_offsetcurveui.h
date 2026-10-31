/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVEUI_H
#define LC_OFFSETCURVEUI_H

#include <QString>
#include "lc_offsetcurve.h"

namespace LC_OffsetCurveUI {
/** Preserve absent stages: they mean not run, never a synthetic success. */
QString refusalText(const LC_OffsetCurveEditResult& result);
/** Bounded sheet edits; no retry with full/default limits after a refusal. */
LC_OffsetRunLimits sheetLimits();
}
#endif
