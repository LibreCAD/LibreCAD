/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurveagain.h"

#include <cmath>
#include <initializer_list>
#include <limits>

#include <boost/multiprecision/cpp_int.hpp>

namespace LC_OffsetCurveAgain {
namespace {
namespace geo = LC_EllipseOffsetGeometry;
using Exact = boost::multiprecision::cpp_rational;

bool validDefinition(const LC_EllipseOffsetDefinition &source) {
    const auto &basis = source.basis;
    if (!geo::isFinite2D(basis.center) || !geo::isFinite2D(basis.majorP) ||
        !std::isfinite(basis.ratio) || !(basis.ratio > 0 && basis.ratio <= 1) ||
        !std::isfinite(source.distance) || !(source.distance > 0) ||
        !std::isfinite(source.relTolerance) || !(source.relTolerance > 0)) return false;
    const double a = std::hypot(basis.majorP.x, basis.majorP.y);
    const double b = a * basis.ratio;
    if (!std::isfinite(a) || !(a > 0) || !std::isfinite(b) || !(b > 0)) return false;
    const RS_Vector minor{-basis.majorP.y * basis.ratio, basis.majorP.x * basis.ratio};
    for (const auto &axis : {basis.majorP, minor}) {
        const auto positive = basis.center + axis, negative = basis.center - axis;
        if (!geo::isFinite2D(positive) || !geo::isFinite2D(negative) ||
            (positive.x == negative.x && positive.y == negative.y)) return false;
    }
    return true;
}

bool canonical(const RS_EllipseData &basis, const geo::Sweep &sweep) {
    if (sweep.whole) return basis.angle1 == 0 && basis.angle2 == 0;
    const double turn = 2 * M_PI;
    return basis.angle1 >= 0 && basis.angle1 < turn && basis.angle2 >= 0 && basis.angle2 < turn &&
        !(basis.angle1 < RS_TOLERANCE_ANGLE && basis.angle2 < RS_TOLERANCE_ANGLE);
}

Exact squaredAxis(const RS_EllipseData &basis) {
    return Exact(basis.majorP.x) * Exact(basis.majorP.x) + Exact(basis.majorP.y) * Exact(basis.majorP.y);
}

bool belowInradius(const RS_EllipseData &basis, double distance) {
    const Exact q(basis.ratio), d(distance);
    return d * d < squaredAxis(basis) * q * q;
}

bool belowGlobalMinimumRadius(const RS_EllipseData &basis, double distance) {
    const Exact q(basis.ratio), d(distance);
    const Exact q2 = q * q;
    // Squaring positive quantities avoids rounding sqrt(major.x^2+major.y^2).
    return d * d < squaredAxis(basis) * q2 * q2;
}

bool containsMajorTip(const RS_EllipseData &basis, const geo::Sweep &sweep) {
    if (sweep.whole) return true;
    const double start = basis.reversed ? basis.angle2 : basis.angle1;
    const double end = basis.reversed ? basis.angle1 : basis.angle2;
    if (start > end || start == 0 || end == 0) return true; // The CCW set crosses zero.
    // Widen the representation of pi, not the geometric sweep. If either end
    // is this close to a major tip, the global lower radius is conservative.
    const double piLow = std::nextafter(M_PI, -std::numeric_limits<double>::infinity());
    const double piHigh = std::nextafter(M_PI, std::numeric_limits<double>::infinity());
    return start <= piHigh && end >= piLow;
}

bool untrimmed(const RS_EllipseData &basis, const geo::Sweep &sweep, double signedOutward) {
    if (signedOutward >= 0) return true;
    const double inward = -signedOutward;
    if (basis.ratio == 1 || containsMajorTip(basis, sweep)) return belowGlobalMinimumRadius(basis, inward);
    // No major tip in this arc: rho has no interior minimum. The existing
    // interval reference primitive encloses 1-inward/rho at the two endpoints.
    const auto frame = geo::ReferenceFrame::create(basis, signedOutward);
    if (!frame) return false;
    LC_Interval factor, derivative;
    if (!frame->boundOffsetFactor(0, 0, factor, derivative) || !factor.isPositive()) return false;
    const double end = frame->parameterEnd();
    return frame->boundOffsetFactor(end, end, factor, derivative) && factor.isPositive();
}

Result refuse(Refusal reason) {
    Result result;
    result.refusal = reason;
    return result;
}
}

Result propose(const LC_EllipseOffsetDefinition &source, const LC_CurveOffsetSide resolvedCacheSide,
    const double incrementMagnitude) {
    if (!validDefinition(source)) return refuse(Refusal::InvalidDefinition);
    const auto sweep = geo::sweep(source.basis);
    if (!sweep) return refuse(Refusal::InvalidDefinition);
    if (!canonical(source.basis, *sweep)) return refuse(Refusal::NonCanonicalBasis);
    if (!std::isfinite(incrementMagnitude) || !(incrementMagnitude > 0)) return refuse(Refusal::InvalidIncrement);
    if (resolvedCacheSide != LC_CurveOffsetSide::Left && resolvedCacheSide != LC_CurveOffsetSide::Right)
        return refuse(Refusal::UnresolvedSide);
    if (sweep->whole && source.towardCentre && !belowInradius(source.basis, source.distance))
        return refuse(Refusal::InvalidCurrentProfile);

    const bool displayedClockwise = source.basis.reversed != source.cacheReversed;
    const auto inwardSide = displayedClockwise ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left;
    const double original = source.towardCentre ? -source.distance : source.distance;
    const double increment = resolvedCacheSide == inwardSide ? -incrementMagnitude : incrementMagnitude;
    const Exact exactTotal = Exact(original) + Exact(increment);
    if (exactTotal == 0) return refuse(Refusal::ZeroSum);
    const double total = original + increment;
    if (!std::isfinite(total)) return refuse(Refusal::NonFiniteCombination);
    if (Exact(total) != exactTotal) return refuse(Refusal::InexactCombination);

    Proof proof;
    if (sweep->whole) {
        if (!source.towardCentre) proof = Proof::WholeOutwardFirst;
        else if (increment < 0) proof = Proof::WholeInwardErosion;
        else {
            if (!untrimmed(source.basis, *sweep, original)) return refuse(Refusal::FirstNotProvedUntrimmed);
            proof = Proof::WholeUntrimmedFirst;
        }
    } else {
        if (!untrimmed(source.basis, *sweep, original)) return refuse(Refusal::FirstNotProvedUntrimmed);
        if (!untrimmed(source.basis, *sweep, total)) return refuse(Refusal::FinalNotProvedUntrimmed);
        proof = Proof::ArcUntrimmed;
    }
    auto combined = source;
    combined.distance = std::abs(total);
    combined.towardCentre = total < 0;
    Result result;
    result.refusal = Refusal::None;
    result.proposal = Proposal{combined, original, increment, total, proof};
    return result;
}
} // namespace LC_OffsetCurveAgain
