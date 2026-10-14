/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTOUTLINE_H
#define LC_TEXTOUTLINE_H

#include "lc_opentypefont.h"
#include <QRectF>
#include <QString>
#include <cstddef>
#include <memory>

class RS_EntityContainer;

/** Native stroked contours, tessellated only after the complete entity affine.
 * Construct native entities on the document/model thread, as for other
 * RS_Entity objects. Exact font contour retrieval remains independently safe
 * for concurrent callers; this adapter does not change RS_Entity ID ownership.
 */
class LC_TextOutline final {
public:
    struct Affine {
        // x' = xx*x + xy*y + dx; y' = yx*x + yy*y + dy.
        // Include glyph origin/offset, normalization, entity scale and rotation.
        // Transformed control points must remain within +/-RS_MAXDOUBLE.
        double xx = 1;
        double xy = 0;
        double yx = 0;
        double yy = 1;
        double dx = 0;
        double dy = 0;
    };
    struct Options {
        double tolerance = 1.0e-4; // Maximum error in final document units.
        std::size_t maximumSegments = 100000; // Includes contour closing edges.
    };
    enum class ErrorCode {
        None, InvalidOptions, InvalidTransform, InvalidOutline, NumericRange,
        PrecisionLoss, ComplexityLimit
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Result {
        // Caller applies the effective glyph pen to this group and reparents it.
        // Child contours/segments use inherited (invalid) pens, not ByLayer.
        std::unique_ptr<RS_EntityContainer> geometry;
        QRectF inkBounds; // Native tessellated bounds, not a logical advance box.
        bool hasInk = false;
        std::size_t segmentCount = 0;
        Error error;
        Result();
        ~Result();
        Result(Result &&) noexcept;
        Result &operator=(Result &&) noexcept;
        Result(const Result &) = delete;
        Result &operator=(const Result &) = delete;
        explicit operator bool() const { return geometry && !error; }
    };

    static Result build(const LC_OpenTypeFont::Outline &outline,
                        const Affine &transform, const Options &options);
    static Result build(const LC_OpenTypeFont::Outline &outline,
                        const Affine &transform);
};

#endif
