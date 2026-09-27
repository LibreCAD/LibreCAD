/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_VIEWMATH_H
#define LC_VIEWMATH_H

#include <cmath>

/**
 * Small helpers for converting view-space (pixel) values between double and int.
 */
namespace LC_ViewMath {
    /** The largest pixel magnitude any view value is allowed to reach (2^31 - 2^24). */
    constexpr double kMaxViewPixel = 2147483648.0 - 16777216.0;

    /**
     * Rounds a pixel value to int without undefined behaviour: non-finite input gives 0,
     * and the result is clamped to +/- kMaxViewPixel before rounding.
     */
    inline int saturatingRound(const double value) {
        if (!std::isfinite(value)) {
            return 0;
        }
        if (value >= kMaxViewPixel) {
            return static_cast<int>(kMaxViewPixel);
        }
        if (value <= -kMaxViewPixel) {
            return -static_cast<int>(kMaxViewPixel);
        }
        return static_cast<int>(std::lround(value));
    }
}

#endif
