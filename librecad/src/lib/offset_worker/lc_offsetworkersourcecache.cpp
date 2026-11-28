/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkersourcecache.h"

#include <algorithm>
#include <cmath>

#include "lc_ellipseoffsetgeometry.h"
#include "lc_offsetoutputbudget.h"
#include "rs_line.h"

namespace {
constexpr std::size_t maxControls = 16384;
constexpr std::size_t maxKnots = 32768;

bool validOrdinarySpline(const RS_SplineData& data) {
    const std::size_t degree = data.degree;
    const std::size_t controls = data.controlPoints.size();
    if (data.fitTangentsConstrained || (degree != 1 && degree != 3) || controls < degree + 1 ||
        controls > maxControls || (controls - 1) % degree ||
        data.knotslist.size() != controls + degree + 1 || data.weights.size() != controls ||
        data.type != RS_SplineData::SplineType::ClampedOpen || data.savedOpenType != data.type ||
        !data.fitPoints.empty() || !data.savedOpenKnots.empty()) {
        return false;
    }
    if (!std::all_of(data.controlPoints.begin(), data.controlPoints.end(),
                     LC_EllipseOffsetGeometry::isFinite2D) ||
        !std::all_of(data.weights.begin(), data.weights.end(), [](double weight) { return weight == 1.0; })) {
        return false;
    }
    const std::size_t spans = (controls - 1) / degree;
    if (spans > makeDefaultOffsetSourceBudget().maxCubicPieces) {
        return false;
    }
    std::size_t index = 0;
    double previous = 0.0;
    for (std::size_t knot = 0; knot <= spans; ++knot) {
        if (index >= data.knotslist.size()) {
            return false;
        }
        const double value = data.knotslist[index];
        if (!std::isfinite(value) || (knot && !(previous < value))) {
            return false;
        }
        const std::size_t multiplicity = knot == 0 || knot == spans ? degree + 1 : degree;
        for (std::size_t i = 0; i < multiplicity; ++i) {
            if (index >= data.knotslist.size() || data.knotslist[index++] != value) {
                return false;
            }
        }
        previous = value;
    }
    return index == data.knotslist.size();
}

struct CubicComponent {
    std::vector<RS_Vector> controls;
    std::vector<double> knots;
    std::size_t spans = 0;
};

bool makeComponent(const LC_OffsetWorkerRequest::Cutter& branch, CubicComponent& output) {
    output = {};
    if (const auto* segment = std::get_if<LC_OffsetWorkerRequest::Segment>(&branch)) {
        if (!LC_EllipseOffsetGeometry::isFinite2D(segment->start) ||
            !LC_EllipseOffsetGeometry::isFinite2D(segment->end)) {
            return false;
        }
        const RS_Vector delta = segment->end - segment->start;
        output.controls = {segment->start, segment->start + delta / 3.0,
                           segment->start + delta * (2.0 / 3.0), segment->end};
        output.knots = {0, 0, 0, 0, 1, 1, 1, 1};
        output.spans = 1;
        return std::all_of(output.controls.begin(), output.controls.end(),
                           LC_EllipseOffsetGeometry::isFinite2D);
    }

    const auto* spline = std::get_if<RS_SplineData>(&branch);
    if (!spline || !validOrdinarySpline(*spline)) {
        return false;
    }
    if (spline->degree == 1) {
        const std::size_t spans = spline->controlPoints.size() - 1;
        output.controls.reserve(4 * spans);
        output.knots.reserve(4 * (spans + 1));
        for (std::size_t span = 0; span < spans; ++span) {
            const RS_Vector start = spline->controlPoints[span];
            const RS_Vector delta = spline->controlPoints[span + 1] - start;
            output.controls.insert(output.controls.end(), {start, start + delta / 3.0,
                start + delta * (2.0 / 3.0), spline->controlPoints[span + 1]});
        }
        for (std::size_t knot = 0; knot <= spans; ++knot) {
            const double parameter = static_cast<double>(knot);
            output.knots.insert(output.knots.end(), 4, parameter);
        }
        output.spans = spans;
        return true;
    }

    const std::size_t controls = spline->controlPoints.size();
    const std::size_t spans = (controls - 1) / 3;
    output.controls = spline->controlPoints;
    output.knots.reserve(controls + 4);
    for (std::size_t knot = 0; knot <= spans; ++knot) {
        const std::size_t multiplicity = knot == 0 || knot == spans ? 4 : 3;
        output.knots.insert(output.knots.end(), multiplicity, static_cast<double>(knot));
    }
    output.spans = spans;
    return output.knots.size() == controls + 4;
}

bool validComposite(const RS_SplineData& data) {
    const std::size_t controls = data.controlPoints.size();
    if (data.degree != 3 || controls < 4 || controls > maxControls ||
        data.knotslist.size() != controls + 4 || data.weights.size() != controls ||
        data.type != RS_SplineData::SplineType::ClampedOpen || data.savedOpenType != data.type ||
        data.m_closedFlag || data.m_periodicFlag || !data.fitPoints.empty() ||
        !data.savedOpenKnots.empty() ||
        !std::all_of(data.controlPoints.begin(), data.controlPoints.end(),
                     LC_EllipseOffsetGeometry::isFinite2D) ||
        !std::all_of(data.weights.begin(), data.weights.end(), [](double weight) { return weight == 1.0; }) ||
        data.knotslist.size() > maxKnots) {
        return false;
    }
    std::size_t index = 0;
    double previous = 0.0;
    std::size_t groups = 0;
    while (index < data.knotslist.size()) {
        const double knot = data.knotslist[index];
        if (!std::isfinite(knot) || (groups && !(previous < knot))) {
            return false;
        }
        std::size_t multiplicity = 0;
        while (index < data.knotslist.size() && data.knotslist[index] == knot) {
            ++multiplicity;
            ++index;
        }
        const bool endpoint = groups == 0 || index == data.knotslist.size();
        if ((endpoint && multiplicity != 4) || (!endpoint && multiplicity != 3 && multiplicity != 4)) {
            return false;
        }
        previous = knot;
        ++groups;
    }
    RS_Spline checked{nullptr, data};
    return checked.validate() && RS_Spline::geometryFingerprint(checked.getData()) ==
                                     RS_Spline::geometryFingerprint(data);
}
} // namespace

bool LC_OffsetWorkerSourceCache::combine(
    const std::vector<LC_OffsetWorkerRequest::Cutter>& branches, RS_SplineData& cache) {
    cache = {};
    if (branches.empty() || branches.size() > makeDefaultOffsetSourceBudget().maxOutputEntities) {
        return false;
    }
    if (branches.size() == 1) {
        if (const auto* spline = std::get_if<RS_SplineData>(&branches.front())) {
            if (!validOrdinarySpline(*spline)) return false;
            cache = *spline;
            return true;
        }
        if (const auto* segment = std::get_if<LC_OffsetWorkerRequest::Segment>(&branches.front())) {
            if (!LC_EllipseOffsetGeometry::isFinite2D(segment->start) ||
                !LC_EllipseOffsetGeometry::isFinite2D(segment->end)) return false;
            cache.degree = 1;
            cache.controlPoints = {segment->start, segment->end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            return true;
        }
        return false;
    }

    RS_SplineData composite;
    composite.degree = 3;
    composite.type = RS_SplineData::SplineType::ClampedOpen;
    composite.savedOpenType = composite.type;
    std::size_t spans = 0;
    for (const auto& branch : branches) {
        CubicComponent component;
        if (!makeComponent(branch, component) || component.spans == 0 ||
            component.spans > makeDefaultOffsetSourceBudget().maxCubicPieces - spans ||
            component.controls.size() > maxControls - composite.controlPoints.size() ||
            component.knots.size() > maxKnots - composite.knotslist.size()) {
            return false;
        }
        const double shift = static_cast<double>(spans);
        if (!composite.controlPoints.empty()) {
            if (component.knots.size() < 4) return false;
            component.knots.erase(component.knots.begin(), component.knots.begin() + 4);
        }
        for (double& knot : component.knots) {
            knot += shift;
            if (!std::isfinite(knot)) return false;
        }
        composite.controlPoints.insert(composite.controlPoints.end(),
                                       component.controls.begin(), component.controls.end());
        composite.weights.insert(composite.weights.end(), component.controls.size(), 1.0);
        composite.knotslist.insert(composite.knotslist.end(), component.knots.begin(), component.knots.end());
        spans += component.spans;
    }
    if (validComposite(composite)) {
        cache = std::move(composite);
        return true;
    }
    return false;
}
