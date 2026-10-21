/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "lc_insert_transform.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "rs_insert.h"
#include "rs_math.h"

namespace {

constexpr double kNegativeIdentityScale = -LC_InsertTransform::IdentityScale;
constexpr double kTwo = 2.0;

// Whether a component that is zero in exact arithmetic is zero to the tolerance the
// decomposition uses (LC_InsertTransform::ShearTolerance); a bare epsilon rejected the
// rounding of the angle round trip (atan2, cos, sin) at about 6% of angles.
bool isNumericallyZero(double value, double scale) {
    return std::abs(value) <= LC_InsertTransform::ShearTolerance
                                  * std::max(LC_InsertTransform::IdentityScale, scale);
}

LC_InsertSourceEditStatus sourceEditStatus(LC_InsertTransformStatus status) {
    switch (status) {
    case LC_InsertTransformStatus::Ok:
        return LC_InsertSourceEditStatus::Ok;
    case LC_InsertTransformStatus::InvalidFields:
        return LC_InsertSourceEditStatus::InvalidSource;
    case LC_InsertTransformStatus::UnsupportedNormal:
        return LC_InsertSourceEditStatus::UnsupportedSourceNormal;
    case LC_InsertTransformStatus::NonFiniteResult:
        return LC_InsertSourceEditStatus::NonFinite;
    }
    return LC_InsertSourceEditStatus::InvalidSource;
}

LC_InsertSourceEditStatus sourceEditStatus(
    LC_InsertTransformDecompositionStatus status) {
    switch (status) {
    case LC_InsertTransformDecompositionStatus::Ok:
        return LC_InsertSourceEditStatus::Ok;
    case LC_InsertTransformDecompositionStatus::NonFinite:
        return LC_InsertSourceEditStatus::NonFinite;
    case LC_InsertTransformDecompositionStatus::Degenerate:
    case LC_InsertTransformDecompositionStatus::Shear:
        return LC_InsertSourceEditStatus::Unrepresentable;
    }
    return LC_InsertSourceEditStatus::Unrepresentable;
}

} // namespace

const char* lcInsertTransformStatusName(LC_InsertTransformStatus status) {
    switch (status) {
    case LC_InsertTransformStatus::Ok:
        return "ok";
    case LC_InsertTransformStatus::InvalidFields:
        return "invalid fields";
    case LC_InsertTransformStatus::UnsupportedNormal:
        return "unsupported normal";
    case LC_InsertTransformStatus::NonFiniteResult:
        return "non-finite result";
    }
    return "unknown status";
}

const char* lcInsertTransformDecompositionStatusName(
    LC_InsertTransformDecompositionStatus status) {
    switch (status) {
    case LC_InsertTransformDecompositionStatus::Ok:
        return "ok";
    case LC_InsertTransformDecompositionStatus::NonFinite:
        return "non-finite";
    case LC_InsertTransformDecompositionStatus::Degenerate:
        return "degenerate";
    case LC_InsertTransformDecompositionStatus::Shear:
        return "shear";
    }
    return "unknown decomposition status";
}

const char* lcInsertSourceEditStatusName(LC_InsertSourceEditStatus status) {
    switch (status) {
    case LC_InsertSourceEditStatus::Ok:
        return "ok";
    case LC_InsertSourceEditStatus::InvalidEdit:
        return "invalid edit";
    case LC_InsertSourceEditStatus::InvalidSource:
        return "invalid source fields";
    case LC_InsertSourceEditStatus::UnsupportedSourceNormal:
        return "unsupported source normal";
    case LC_InsertSourceEditStatus::NonFinite:
        return "non-finite result";
    case LC_InsertSourceEditStatus::Unrepresentable:
        return "unrepresentable affine result";
    }
    return "unknown source edit status";
}

const RS_Vector& lcInsertTransformOrigin() {
    static const RS_Vector origin(LC_InsertTransform::Zero, LC_InsertTransform::Zero);
    return origin;
}

const RS_Vector& lcInsertTransformReflectionAxisPoint() {
    static const RS_Vector point(LC_InsertTransform::IdentityScale,
                                 LC_InsertTransform::Zero);
    return point;
}

double lcInsertTransformFullTurnRadians() {
    static const double fullTurn = kTwo * std::acos(kNegativeIdentityScale);
    return fullTurn;
}

bool LC_InsertTransform::isFinite() const {
    return std::isfinite(a) && std::isfinite(b) && std::isfinite(c)
           && std::isfinite(d) && std::isfinite(tx) && std::isfinite(ty);
}

RS_Vector LC_InsertTransform::mapVector(const RS_Vector& vector) const {
    return RS_Vector(a * vector.x + c * vector.y,
                     b * vector.x + d * vector.y, vector.z);
}

RS_Vector LC_InsertTransform::mapLeafPoint(const RS_Vector& point) const {
    RS_Vector result = mapVector(point);
    result.move(RS_Vector(tx, ty));
    return result;
}

bool LC_InsertTransform::compose(const LC_InsertTransform& parent,
                                 const LC_InsertTransform& child,
                                 LC_InsertTransform& result) {
    result.a = parent.a * child.a + parent.c * child.b;
    result.b = parent.b * child.a + parent.d * child.b;
    result.c = parent.a * child.c + parent.c * child.d;
    result.d = parent.b * child.c + parent.d * child.d;
    result.tx = parent.a * child.tx + parent.c * child.ty + parent.tx;
    result.ty = parent.b * child.tx + parent.d * child.ty + parent.ty;
    return result.isFinite();
}

bool LC_InsertTransform::translation(const RS_Vector& offset,
                                     LC_InsertTransform& result) {
    if (!offset.valid || !std::isfinite(offset.x) || !std::isfinite(offset.y))
        return false;
    result = LC_InsertTransform{};
    result.tx = offset.x;
    result.ty = offset.y;
    return true;
}

bool LC_InsertTransform::rotation(const RS_Vector& center, double angle,
                                  LC_InsertTransform& result) {
    if (!center.valid || !std::isfinite(center.x) || !std::isfinite(center.y)
        || !std::isfinite(angle)) {
        return false;
    }
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    result.a = cosine;
    result.b = sine;
    result.c = -sine;
    result.d = cosine;
    result.tx = center.x - cosine * center.x + sine * center.y;
    result.ty = center.y - sine * center.x - cosine * center.y;
    return result.isFinite();
}

bool LC_InsertTransform::scale(const RS_Vector& center, const RS_Vector& factor,
                               LC_InsertTransform& result) {
    if (!center.valid || !factor.valid || !std::isfinite(center.x)
        || !std::isfinite(center.y) || !std::isfinite(factor.x)
        || !std::isfinite(factor.y) || factor.x == LC_InsertTransform::Zero
        || factor.y == LC_InsertTransform::Zero) {
        return false;
    }
    result.a = factor.x;
    result.b = LC_InsertTransform::Zero;
    result.c = LC_InsertTransform::Zero;
    result.d = factor.y;
    result.tx = center.x - factor.x * center.x;
    result.ty = center.y - factor.y * center.y;
    return result.isFinite();
}

bool LC_InsertTransform::reflection(const RS_Vector& axisPoint1,
                                    const RS_Vector& axisPoint2,
                                    LC_InsertTransform& result) {
    if (!axisPoint1.valid || !axisPoint2.valid || !std::isfinite(axisPoint1.x)
        || !std::isfinite(axisPoint1.y) || !std::isfinite(axisPoint2.x)
        || !std::isfinite(axisPoint2.y)) {
        return false;
    }
    const double dx = axisPoint2.x - axisPoint1.x;
    const double dy = axisPoint2.y - axisPoint1.y;
    const double length = std::hypot(dx, dy);
    if (!std::isfinite(length) || length == LC_InsertTransform::Zero)
        return false;
    const double ux = dx / length;
    const double uy = dy / length;
    result.a = kTwo * ux * ux - LC_InsertTransform::IdentityScale;
    result.b = kTwo * ux * uy;
    result.c = result.b;
    result.d = kTwo * uy * uy - LC_InsertTransform::IdentityScale;
    result.tx = axisPoint1.x - result.a * axisPoint1.x - result.c * axisPoint1.y;
    result.ty = axisPoint1.y - result.b * axisPoint1.x - result.d * axisPoint1.y;
    return result.isFinite();
}

RS_Vector LC_InsertTransform::mapArrayOffset(const RS_Vector& spacing, int column,
                                              int row, double cosine, double sine,
                                              double ocsXAxis) {
    const RS_Vector arrayOffset(spacing.x * column, spacing.y * row);
    return RS_Vector(ocsXAxis * (cosine * arrayOffset.x - sine * arrayOffset.y),
                     sine * arrayOffset.x + cosine * arrayOffset.y);
}

LC_InsertTransformStatus LC_InsertTransform::fromInsert(const RS_InsertData& data,
                                                         const RS_Vector& base,
                                                         int column, int row,
                                                         LC_InsertTransform& result) {
    const double sx = data.scaleFactor.x;
    const double sy = data.scaleFactor.y;
    if (column < 0 || row < 0 || !data.scaleFactor.valid || !data.spacing.valid
        || !std::isfinite(sx) || !std::isfinite(sy) || sx == LC_InsertTransform::Zero
        || sy == LC_InsertTransform::Zero
        || !std::isfinite(data.angle) || !data.insertionPoint.valid || !base.valid
        || !std::isfinite(data.insertionPoint.x) || !std::isfinite(data.insertionPoint.y)
        || !std::isfinite(base.x) || !std::isfinite(base.y)
        || !std::isfinite(data.spacing.x) || !std::isfinite(data.spacing.y)) {
        return LC_InsertTransformStatus::InvalidFields;
    }
    if (!data.extrusion.valid || !std::isfinite(data.extrusion.x)
        || !std::isfinite(data.extrusion.y) || !std::isfinite(data.extrusion.z)
        || data.extrusion.x != LC_InsertTransform::Zero
        || data.extrusion.y != LC_InsertTransform::Zero
        || data.extrusion.z == LC_InsertTransform::Zero) {
        return LC_InsertTransformStatus::UnsupportedNormal;
    }
    const double cosine = std::cos(data.angle);
    const double sine = std::sin(data.angle);
    const double ocsXAxis = data.extrusion.z < LC_InsertTransform::Zero
                            ? kNegativeIdentityScale
                            : LC_InsertTransform::IdentityScale;
    result.a = ocsXAxis * cosine * sx;
    result.b = sine * sx;
    result.c = -ocsXAxis * sine * sy;
    result.d = cosine * sy;
    const RS_Vector rotatedArrayOffset = mapArrayOffset(data.spacing, column, row,
                                                         cosine, sine, ocsXAxis);
    const RS_Vector mappedBase = result.mapVector(base);
    result.tx = ocsXAxis * data.insertionPoint.x + rotatedArrayOffset.x
                - mappedBase.x;
    result.ty = data.insertionPoint.y + rotatedArrayOffset.y - mappedBase.y;
    if (!result.isFinite() || !std::isfinite(rotatedArrayOffset.x)
        || !std::isfinite(rotatedArrayOffset.y)) {
        return LC_InsertTransformStatus::NonFiniteResult;
    }
    return LC_InsertTransformStatus::Ok;
}

LC_InsertTransformDecompositionStatus
LC_InsertTransform::decompose(LC_InsertTransformParts& result) const {
    const double col0 = std::hypot(a, b);
    const double col1 = std::hypot(c, d);
    const double dot = a * c + b * d;
    // relative to the columns' lengths, not to the two products (which are about zero for a
    // quarter turn, and that left no tolerance at all): see ShearTolerance
    const double tolerance = LC_InsertTransform::ShearTolerance * col0 * col1;
    if (!std::isfinite(col0) || !std::isfinite(col1) || !std::isfinite(dot))
        return LC_InsertTransformDecompositionStatus::NonFinite;
    if (col0 == LC_InsertTransform::Zero || col1 == LC_InsertTransform::Zero)
        return LC_InsertTransformDecompositionStatus::Degenerate;
    if (std::abs(dot) > tolerance)
        return LC_InsertTransformDecompositionStatus::Shear;

    const double scaleY = (a * d - b * c) / col0;
    const double angle = std::atan2(b, a);
    if (!std::isfinite(scaleY) || !std::isfinite(angle))
        return LC_InsertTransformDecompositionStatus::NonFinite;
    if (scaleY == LC_InsertTransform::Zero)
        return LC_InsertTransformDecompositionStatus::Degenerate;

    result.scaleX = col0;
    result.scaleY = scaleY;
    result.angle = angle;
    result.reversesOrientation = scaleY < LC_InsertTransform::Zero;
    return LC_InsertTransformDecompositionStatus::Ok;
}

LC_InsertSourceEditStatus
lcApplyInsertSourceEdit(const RS_InsertData& source, const LC_InsertTransform& edit,
                        RS_InsertData& result) {
    LC_InsertTransform sourceFrame;
    const auto sourceStatus = LC_InsertTransform::fromInsert(
        source, lcInsertTransformOrigin(), 0, 0, sourceFrame);
    if (sourceStatus != LC_InsertTransformStatus::Ok)
        return sourceEditStatus(sourceStatus);

    const double ocsXAxis = source.extrusion.z < LC_InsertTransform::Zero
                            ? kNegativeIdentityScale
                            : LC_InsertTransform::IdentityScale;

    // An edit whose linear part is the identity (a translation; the paste's identity scale
    // and rotation) only moves the insertion point. Rebuilding the angle, the scale
    // factors and the spacing from the matrix would round each of them to a new value at
    // every edit, and a few dozen edits of a rotated block later the source could no longer
    // be edited at all, or its nested blocks would no longer expand.
    if (edit.a == LC_InsertTransform::IdentityScale && edit.b == LC_InsertTransform::Zero
        && edit.c == LC_InsertTransform::Zero && edit.d == LC_InsertTransform::IdentityScale) {
        if (!edit.isFinite())
            return LC_InsertSourceEditStatus::NonFinite;
        result = source;
        // as every other edit does, put an angle stored outside a full turn (a DXF file keeps
        // what it was written with) back into it, and leave one inside it as it is
        if (!(source.angle >= LC_InsertTransform::Zero && source.angle < lcInsertTransformFullTurnRadians()))
            result.angle = RS_Math::correctAngle(source.angle);
        result.insertionPoint.x = source.insertionPoint.x + ocsXAxis * edit.tx;
        result.insertionPoint.y = source.insertionPoint.y + edit.ty;
        return result.insertionPoint.valid && std::isfinite(result.insertionPoint.x)
                       && std::isfinite(result.insertionPoint.y)
                   ? LC_InsertSourceEditStatus::Ok
                   : LC_InsertSourceEditStatus::NonFinite;
    }

    LC_InsertTransform transformed;
    if (!LC_InsertTransform::compose(edit, sourceFrame, transformed))
        return LC_InsertSourceEditStatus::NonFinite;

    const double normalizedA = ocsXAxis * transformed.a;
    const double normalizedB = transformed.b;
    const double normalizedC = ocsXAxis * transformed.c;
    const double normalizedD = transformed.d;
    LC_InsertTransform normalized;
    normalized.a = normalizedA;
    normalized.b = normalizedB;
    normalized.c = normalizedC;
    normalized.d = normalizedD;
    LC_InsertTransformParts parts;
    const auto decompositionStatus = normalized.decompose(parts);
    if (decompositionStatus != LC_InsertTransformDecompositionStatus::Ok)
        return sourceEditStatus(decompositionStatus);

    const double oldCosine = std::cos(source.angle);
    const double oldSine = std::sin(source.angle);
    const double newCosine = std::cos(parts.angle);
    const double newSine = std::sin(parts.angle);
    if (!std::isfinite(oldCosine) || !std::isfinite(oldSine)
        || !std::isfinite(newCosine) || !std::isfinite(newSine)) {
        return LC_InsertSourceEditStatus::NonFinite;
    }

    const RS_Vector oldColumnAxis(ocsXAxis * oldCosine, oldSine);
    const RS_Vector oldRowAxis(-ocsXAxis * oldSine, oldCosine);
    const RS_Vector newColumnAxis(ocsXAxis * newCosine, newSine);
    const RS_Vector newRowAxis(-ocsXAxis * newSine, newCosine);
    const RS_Vector mappedColumn = edit.mapVector(oldColumnAxis);
    const RS_Vector mappedRow = edit.mapVector(oldRowAxis);
    const double columnX = newColumnAxis.x * mappedColumn.x
                           + newColumnAxis.y * mappedColumn.y;
    const double columnY = newRowAxis.x * mappedColumn.x
                           + newRowAxis.y * mappedColumn.y;
    const double rowX = newColumnAxis.x * mappedRow.x
                        + newColumnAxis.y * mappedRow.y;
    const double rowY = newRowAxis.x * mappedRow.x
                        + newRowAxis.y * mappedRow.y;
    if (!std::isfinite(columnX) || !std::isfinite(columnY)
        || !std::isfinite(rowX) || !std::isfinite(rowY)
        || !isNumericallyZero(columnY, std::abs(columnX) + std::abs(columnY))
        || !isNumericallyZero(rowX, std::abs(rowX) + std::abs(rowY))) {
        return LC_InsertSourceEditStatus::Unrepresentable;
    }

    result = source;
    result.insertionPoint.x = ocsXAxis * transformed.tx;
    result.insertionPoint.y = transformed.ty;
    result.scaleFactor.x = parts.scaleX;
    result.scaleFactor.y = parts.scaleY;
    result.angle = RS_Math::correctAngle(parts.angle);
    result.spacing.x = source.spacing.x * columnX;
    result.spacing.y = source.spacing.y * rowY;
    return result.insertionPoint.valid && std::isfinite(result.insertionPoint.x)
                   && std::isfinite(result.insertionPoint.y)
                   && std::isfinite(result.spacing.x) && std::isfinite(result.spacing.y)
               ? LC_InsertSourceEditStatus::Ok
               : LC_InsertSourceEditStatus::NonFinite;
}
