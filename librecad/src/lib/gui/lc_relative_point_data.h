/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_RELATIVEPOINTDATA_H
#define LC_RELATIVEPOINTDATA_H
#include "rs_vector.h"

struct LC_RelativePositionData {
    bool valid = false;
    RS_Vector wcsBasePoint;
    RS_Vector wcsProjection;
    double length{0.0};
    double wcsAngle{0.0};

    bool isSingleSolution{true};

    bool explicitLength{false};
    bool explicitAngle{false};
    bool explicitDX{false};
    bool explicitDY{false};
    bool showLengthNormal{true};

    void setUnmodified() {
        explicitAngle = false;
        explicitLength = false;
        explicitDX = false;
        explicitDY = false;
        showLengthNormal = true;
    }

    void updateBy(const LC_RelativePositionData* other) {
        if (other == nullptr) {
            valid = false;
        }
        else {
            valid = true;
            wcsBasePoint = other->wcsBasePoint;
            wcsProjection = other->wcsProjection;
            length = other->length;
            wcsAngle = other->wcsAngle;
            isSingleSolution = other->isSingleSolution;
            explicitLength = other->explicitLength;
            explicitAngle = other->explicitAngle;
            explicitDX = other->explicitDX;
            explicitDY = other->explicitDY;
            isSingleSolution = other->isSingleSolution;
            showLengthNormal = true;
        }
    }
};
#endif
