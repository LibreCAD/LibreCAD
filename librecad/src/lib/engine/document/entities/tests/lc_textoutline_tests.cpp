/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textoutline.h"
#include "lc_textshaper.h"
#include "rs.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_polyline.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
using Command = LC_OpenTypeFont::Command;
using Type = Command::Type;
using Outline = LC_OpenTypeFont::Outline;
using Affine = LC_TextOutline::Affine;
QPointF mapped(const Affine &a, const QPointF &p) {
    return {a.xx * p.x() + a.xy * p.y() + a.dx, a.yx * p.x() + a.yy * p.y() + a.dy};
}
Outline quadratic() {
    Outline outline;
    outline.hasInk = true;
    outline.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                        {Type::QuadraticTo, {0.5, 1}, {1, 0}, {}},
                        {Type::Close, {}, {}, {}}};
    return outline;
}
Outline cubic() {
    Outline outline;
    outline.hasInk = true;
    outline.commands = {{Type::MoveTo, {-1, 0}, {}, {}},
                        {Type::CubicTo, {-1, 2}, {1, -2}, {1, 0}},
                        {Type::Close, {}, {}, {}}};
    return outline;
}
std::vector<const RS_Line *> lines(const LC_TextOutline::Result &result) {
    std::vector<const RS_Line *> output;
    REQUIRE(result.geometry);
    for (const auto *entity : *result.geometry) {
        REQUIRE(entity->rtti() == RS2::EntityPolyline);
        const auto *polyline = static_cast<const RS_Polyline *>(entity);
        REQUIRE(polyline->isClosed());
        for (const auto *segment : *polyline) {
            REQUIRE(segment->rtti() == RS2::EntityLine);
            output.push_back(static_cast<const RS_Line *>(segment));
        }
    }
    REQUIRE(output.size() == result.segmentCount);
    return output;
}
double distance(const QPointF &p, const RS_Line &line) {
    const auto a = line.getStartpoint(), b = line.getEndpoint();
    const long double dx = static_cast<long double>(b.x) - a.x;
    const long double dy = static_cast<long double>(b.y) - a.y;
    const long double px = static_cast<long double>(p.x()) - a.x;
    const long double py = static_cast<long double>(p.y()) - a.y;
    const long double square = dx * dx + dy * dy;
    const long double t = square == 0 ? 0 : std::clamp((px * dx + py * dy) / square, 0.0L, 1.0L);
    return static_cast<double>(std::hypot(px - t * dx, py - t * dy));
}
void checkQuadraticError(const Affine &transform, double tolerance, unsigned samples = 500) {
    const auto result = LC_TextOutline::build(quadratic(), transform, {tolerance, 100000});
    REQUIRE(result);
    const auto native = lines(result);
    REQUIRE_FALSE(native.empty());
    for (unsigned sample = 0; sample <= samples; ++sample) {
        const double t = static_cast<double>(sample) / samples;
        const QPointF exact = mapped(transform, {t, 2 * t * (1 - t)});
        double closest = std::numeric_limits<double>::infinity();
        for (const auto *line : native) closest = std::min(closest, distance(exact, *line));
        REQUIRE(closest <= tolerance);
    }
}
double signedArea(const RS_Polyline &polyline) {
    const auto vertices = polyline.getVertexes();
    long double area = 0;
    for (int i = 1; i < vertices.size(); ++i)
        area += static_cast<long double>(vertices.at(i - 1).x) * vertices.at(i).y -
                static_cast<long double>(vertices.at(i).x) * vertices.at(i - 1).y;
    return static_cast<double>(area / 2);
}
} // namespace

TEST_CASE("Font curves become native contours within final document tolerance", "[opentype][native][outline]") {
    checkQuadraticError({}, 1e-4);
    Affine transformed;
    const double angle = 0.731;
    const double cosine = std::cos(angle), sine = std::sin(angle);
    transformed.xx = -1e6 * cosine;
    transformed.yx = -1e6 * sine;
    transformed.xy = -3e5 * sine;
    transformed.yy = 3e5 * cosine;
    transformed.dx = 120;
    transformed.dy = -42;
    checkQuadraticError(transformed, 0.01, 300);
    // Extreme anisotropy is measured after transformation, not via one scalar.
    transformed = {1e8, 0, 0, 1e-8, -10, 30};
    checkQuadraticError(transformed, 1e-4);

    const Affine shear{3, 7, -2, 5, -11, 4};
    const auto result = LC_TextOutline::build(cubic(), shear, {1e-4, 100000});
    REQUIRE(result);
    const auto native = lines(result);
    for (unsigned sample = 0; sample <= 1000; ++sample) {
        const double t = sample / 1000.0, u = 1 - t;
        const QPointF point{-u * u * u - 3 * u * u * t + 3 * u * t * t + t * t * t,
                             6 * u * u * t - 6 * u * t * t};
        double closest = std::numeric_limits<double>::infinity();
        for (const auto *line : native) closest = std::min(closest, distance(mapped(shear, point), *line));
        REQUIRE(closest <= 1e-4);
    }
}

TEST_CASE("Native contours preserve holes, bearings and exact tiny closing edges", "[opentype][native][closure]") {
    Outline outline;
    outline.commands = {
        {Type::MoveTo, {-2, -3}, {}, {}}, {Type::LineTo, {8, -3}, {}, {}},
        {Type::LineTo, {8, 7}, {}, {}}, {Type::LineTo, {-2, 7}, {}, {}}, {Type::Close, {}, {}, {}},
        {Type::MoveTo, {0, -1}, {}, {}}, {Type::LineTo, {0, 5}, {}, {}},
        {Type::LineTo, {6, 5}, {}, {}}, {Type::LineTo, {6, -1}, {}, {}}, {Type::Close, {}, {}, {}}};
    const auto result = LC_TextOutline::build(outline, {-2, 0, 0, 3, 100, -10});
    REQUIRE(result);
    REQUIRE(result.hasInk);
    REQUIRE(result.geometry->count() == 2);
    REQUIRE(result.segmentCount == 8);
    REQUIRE(result.inkBounds == QRectF(84, -19, 20, 30));
    const auto *outer = static_cast<RS_Polyline *>(result.geometry->entityAt(0));
    const auto *inner = static_cast<RS_Polyline *>(result.geometry->entityAt(1));
    REQUIRE(signedArea(*outer) == Catch::Approx(-600));
    REQUIRE(signedArea(*inner) == Catch::Approx(216));
    for (const auto *line : lines(result)) REQUIRE(line->getStartpoint().valid);

    Outline tiny;
    tiny.commands = {{Type::MoveTo, {0, 0}, {}, {}}, {Type::LineTo, {1e-6, 0}, {}, {}},
                     {Type::LineTo, {1e-6, 1e-6}, {}, {}}, {Type::Close, {}, {}, {}}};
    const auto closed = LC_TextOutline::build(tiny, {});
    REQUIRE(closed);
    REQUIRE(closed.segmentCount == 3);
    const auto native = lines(closed);
    REQUIRE(native.back()->getEndpoint().distanceTo(native.front()->getStartpoint()) == 0);
}

TEST_CASE("Collinear reversing and zero-chord curves are not flattened into nothing", "[opentype][native][degenerate]") {
    Outline outline;
    outline.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                        {Type::QuadraticTo, {10, 0}, {0, 0}, {}}, {Type::Close, {}, {}, {}}};
    auto result = LC_TextOutline::build(outline, {});
    REQUIRE(result);
    REQUIRE(result.hasInk);
    REQUIRE(result.inkBounds.right() == Catch::Approx(5));
    REQUIRE(result.segmentCount >= 2);
    outline.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                        {Type::CubicTo, {10, 0}, {-10, 0}, {0, 0}}, {Type::Close, {}, {}, {}}};
    result = LC_TextOutline::build(outline, {});
    REQUIRE(result);
    REQUIRE(result.inkBounds.left() < -2.8);
    REQUIRE(result.inkBounds.right() > 2.8);
    outline.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                        {Type::CubicTo, {1e-6, 2e-6}, {-1e-6, 2e-6}, {0, 0}}, {Type::Close, {}, {}, {}}};
    result = LC_TextOutline::build(outline, {});
    REQUIRE(result);
    REQUIRE(result.hasInk);
    REQUIRE(result.geometry->hasValidBorders());
    REQUIRE(result.inkBounds.bottom() > 0);
    REQUIRE(result.segmentCount >= 4);
    REQUIRE(signedArea(*static_cast<const RS_Polyline *>(result.geometry->entityAt(0))) > 0);
    outline.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                        {Type::QuadraticTo, {1e-6, 0}, {0, 0}, {}}, {Type::Close, {}, {}, {}}};
    result = LC_TextOutline::build(outline, {});
    REQUIRE(result);
    REQUIRE(result.hasInk);
    REQUIRE(result.inkBounds.right() > 0);
    result = LC_TextOutline::build(quadratic(), {0, 0, 0, 0, 2, -4});
    REQUIRE(result);
    REQUIRE_FALSE(result.hasInk);
    REQUIRE(result.geometry->isEmpty());
    REQUIRE_FALSE(result.geometry->getMin().valid);
    REQUIRE_FALSE(result.geometry->getMax().valid);
    const auto empty = LC_TextOutline::build({}, {});
    REQUIRE(empty);
    REQUIRE_FALSE(empty.hasInk);
    REQUIRE(empty.segmentCount == 0);
    REQUIRE_FALSE(empty.geometry->getMin().valid);
    REQUIRE_FALSE(empty.geometry->getMax().valid);
    REQUIRE_FALSE(empty.geometry->hasValidBorders());
}

TEST_CASE("Native coordinate boundaries are exact and unsupported ranges never clamp ink", "[opentype][native][range]") {
    Outline triangle;
    triangle.commands = {{Type::MoveTo, {0, 0}, {}, {}}, {Type::LineTo, {1, 0}, {}, {}},
                         {Type::LineTo, {0, 1}, {}, {}}, {Type::Close, {}, {}, {}}};
    const double limit = RS_MAXDOUBLE;
    const LC_TextOutline::Options options{0.01, 100};
    for (const Affine &transform : {Affine{1, 0, 0, 1, limit - 1, -limit},
                                    Affine{1, 0, 0, 1, -limit, limit - 1}}) {
        const auto result = LC_TextOutline::build(triangle, transform, options);
        REQUIRE(result);
        REQUIRE(result.hasInk);
        REQUIRE(result.geometry->hasValidBorders());
        REQUIRE(result.geometry->getMin().x == transform.dx);
        REQUIRE(result.geometry->getMin().y == transform.dy);
        REQUIRE(result.geometry->getMax().x == transform.dx + 1);
        REQUIRE(result.geometry->getMax().y == transform.dy + 1);
        REQUIRE(result.inkBounds == QRectF(transform.dx, transform.dy, 1, 1));
        REQUIRE(result.segmentCount == 3);
    }
    const double outside = std::nextafter(limit, std::numeric_limits<double>::infinity());
    for (const QPointF &point : {QPointF(outside, 0), QPointF(-outside, 0),
                                 QPointF(0, outside), QPointF(0, -outside)}) {
        auto unsupported = triangle;
        unsupported.commands.front().first = point;
        const auto result = LC_TextOutline::build(unsupported, {}, options);
        REQUIRE(result.error.code == LC_TextOutline::ErrorCode::NumericRange);
        REQUIRE(result.error.message.contains("native coordinate range"));
        REQUIRE_FALSE(result.geometry);
        REQUIRE_FALSE(result.hasInk);
        REQUIRE(result.segmentCount == 0);
    }
    const auto translated = LC_TextOutline::build(triangle, {1, 0, 0, 1, limit, 0}, options);
    REQUIRE(translated.error.code == LC_TextOutline::ErrorCode::NumericRange);
    REQUIRE_FALSE(translated.geometry);

    // QPointF's fuzzy equality would erase this entire triangle even though
    // its edges exceed the requested tolerance and are exactly representable.
    auto shortEdges = triangle;
    shortEdges.commands[1].first.setX(8e-5);
    shortEdges.commands[2].first.setY(8e-5);
    const auto precise = LC_TextOutline::build(shortEdges, {1, 0, 0, 1, 1e8, 1e8}, {2.5e-5, 100});
    REQUIRE(precise);
    REQUIRE(precise.hasInk);
    REQUIRE(precise.segmentCount == 3);
    REQUIRE(precise.inkBounds.width() > 7e-5);
    REQUIRE(precise.inkBounds.height() > 7e-5);
    REQUIRE(lines(precise).size() == 3);
}

TEST_CASE("Native contour failure is explicit and never returns partial geometry", "[opentype][native][failure]") {
    REQUIRE(LC_TextOutline::build(quadratic(), {}, {0, 100}).error.code == LC_TextOutline::ErrorCode::InvalidOptions);
    REQUIRE(LC_TextOutline::build(quadratic(), {}, {1e-4, 0}).error.code == LC_TextOutline::ErrorCode::InvalidOptions);
    auto capped = LC_TextOutline::build(quadratic(), {}, {1e-6, 2});
    REQUIRE(capped.error.code == LC_TextOutline::ErrorCode::ComplexityLimit);
    REQUIRE_FALSE(capped.geometry);
    auto transform = Affine{};
    transform.xx = std::numeric_limits<double>::infinity();
    REQUIRE(LC_TextOutline::build(quadratic(), transform).error.code == LC_TextOutline::ErrorCode::InvalidTransform);
    transform.xx = 1e200;
    REQUIRE(LC_TextOutline::build(quadratic(), transform).error.code == LC_TextOutline::ErrorCode::NumericRange);
    transform = {};
    transform.dx = 1e8;
    REQUIRE(LC_TextOutline::build(quadratic(), transform, {1e-8, 100000}).error.code == LC_TextOutline::ErrorCode::PrecisionLoss);
    Outline malformed;
    malformed.commands = {{Type::LineTo, {0, 0}, {}, {}}};
    REQUIRE(LC_TextOutline::build(malformed, {}).error.code == LC_TextOutline::ErrorCode::InvalidOutline);
    malformed.commands = {{Type::MoveTo, {0, 0}, {}, {}}};
    REQUIRE(LC_TextOutline::build(malformed, {}).error.code == LC_TextOutline::ErrorCode::InvalidOutline);
    malformed.commands = {{Type::MoveTo, {0, 0}, {}, {}}, {Type::MoveTo, {1, 1}, {}, {}}};
    REQUIRE(LC_TextOutline::build(malformed, {}).error.code == LC_TextOutline::ErrorCode::InvalidOutline);
    malformed.commands = {{Type::MoveTo, {std::numeric_limits<double>::quiet_NaN(), 0}, {}, {}}};
    REQUIRE(LC_TextOutline::build(malformed, {}).error.code == LC_TextOutline::ErrorCode::InvalidOutline);
    malformed.commands.resize(100001);
    REQUIRE(LC_TextOutline::build(malformed, {}).error.code == LC_TextOutline::ErrorCode::ComplexityLimit);
}

TEST_CASE("Tiny nonzero chords do not underflow the native flatness calculation", "[opentype][native][outline][tiny]") {
    Outline straight;
    straight.commands = {{Type::MoveTo, {0, 0}, {}, {}},
                         {Type::QuadraticTo, {1, 0}, {2, 0}, {}},
                         {Type::Close, {}, {}, {}}};
    const auto result = LC_TextOutline::build(straight,
        {5e-171, 0, 0, 5e-171, 0, 0}, {1e-180, 100000});
    INFO(result.error.message.toStdString());
    REQUIRE(result);
    REQUIRE(result.hasInk);
    REQUIRE(result.segmentCount == 2);
    REQUIRE(result.geometry->hasValidBorders());
    REQUIRE(result.inkBounds.left() == 0);
    REQUIRE(result.inkBounds.right() == 1e-170);
    REQUIRE(lines(result).size() == 2);
}

TEST_CASE("Absolute subnormal rounding cannot bypass the native precision guard", "[opentype][native][outline][subnormal]") {
    const double unit = std::numeric_limits<double>::denorm_min();
    Outline triangle;
    triangle.commands = {{Type::MoveTo, {0.5, 0.5}, {}, {}},
                         {Type::LineTo, {2.5, 0.5}, {}, {}},
                         {Type::LineTo, {0.5, 2.5}, {}, {}},
                         {Type::Close, {}, {}, {}}};
    // Exact vertices, in units of denorm_min, are (1,3), (3,5), (3,13).
    // Nested correctly-rounded fma produces (0,2), (2,4), (2,12): the last
    // vertex is sqrt(2) units from the native ink, exceeding this tolerance.
    // Relative epsilon*magnitude alone underflows to zero and misses the loss.
    const auto result = LC_TextOutline::build(triangle,
        {unit, unit, unit, 5 * unit, 0, 0}, {unit, 100});
    REQUIRE(result.error.code == LC_TextOutline::ErrorCode::PrecisionLoss);
    REQUIRE_FALSE(result.geometry);
    REQUIRE_FALSE(result.hasInk);
    REQUIRE(result.segmentCount == 0);
}

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE && defined(LC_OPENTYPE_TEST_FONT_PATH)
TEST_CASE("Pinned contextual glyph contours convert without mutating exact font cache", "[opentype][native][font]") {
    const auto loaded = LC_OpenTypeFont::load(QString::fromUtf8(LC_OPENTYPE_TEST_FONT_PATH));
    REQUIRE(loaded);
    LC_TextShaper::Run run;
    run.contextText = QString::fromUtf8(u8"ببب لَا بَّ");
    const auto shaped = LC_TextShaper::shape(*loaded.font, run);
    REQUIRE(shaped);
    double pen = 0;
    const double factor = loaded.font->normalization();
    for (const auto &glyph : shaped.glyphs) {
        const auto outline = loaded.font->outline(glyph.id);
        REQUIRE(outline);
        const auto original = outline.outline->commands;
        const auto native = LC_TextOutline::build(*outline.outline,
            {factor, 0, 0, factor, (pen + glyph.xOffset) * factor, glyph.yOffset * factor});
        REQUIRE(native);
        REQUIRE(native.hasInk == outline.outline->hasInk);
        REQUIRE(original.size() == outline.outline->commands.size());
        for (std::size_t index = 0; index < original.size(); ++index) {
            REQUIRE(original[index].type == outline.outline->commands[index].type);
            REQUIRE(original[index].first == outline.outline->commands[index].first);
            REQUIRE(original[index].second == outline.outline->commands[index].second);
            REQUIRE(original[index].third == outline.outline->commands[index].third);
        }
        pen += glyph.xAdvance;
    }
}
#endif
