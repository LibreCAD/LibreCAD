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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**********************************************************************/

#ifndef LC_HATCHDENSITY_H
#define LC_HATCHDENSITY_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

class RS_Pattern;

namespace LC_LoopUtils {
class LC_Loops;
}

/**
 * Injected policy for the hatch density guard. The engine never reads these
 * values from settings; a UI or test caller installs a value through
 * LC_HatchDensity::ScopedPolicy. All fields are nonnegative doubles so the
 * caller can inject fractional multiples of billions without worrying about
 * integer wrapping; the engine clamps and converts to size_t at the boundary.
 */
struct LC_HatchDensityPolicy {
    /** Interim tile-piece cap (soft limit, honoured unless forced). */
    double maxPieces = 1e6;
    /** Hard tile-piece ceiling; honoured even in forced mode. */
    double hardMax = 1e7;
    /** Work ceiling: grid cells * boundary edges + line * edge pairs past the bbox filter. */
    double workCap = 5e7;
    /** When true, bypass soft caps up to hardMax; hard limits still apply. */
    bool forced = false;
};

/**
 * Diagnostic report attached to every hatch after a pattern update attempt.
 * Zero-initialised by RS_Hatch::update(); filled by updatePatternHatch().
 */
struct LC_HatchDensityReport {
    enum class Reason {
        None,
        Estimate,
        Work,
        Budget,
        NonFinite,
        SharedBudget,
    };
    Reason reason = Reason::None;
    /** Even-odd area used by the estimate (sum of signed child areas). */
    double area = 0.0;
    /** E * A / A_t, the lines-fill-area term. */
    double areaTerm = 0.0;
    /** Sum of boundary cross-pattern lengths, scaled by 1/(2 A_t). */
    double boundaryTerm = 0.0;
    /** areaTerm + boundaryTerm. */
    double estimatedPieces = 0.0;
    /** gridWork + pairWork; zero when the estimate decided first. */
    double work = 0.0;
    /** Soft cap that applied to this generation (0 when no cap applied). */
    double limit = 0.0;
    /** Pieces emitted; equals limit + 1 on a Budget trip. */
    std::size_t attempted = 0;
    /** Σℓ_e / A_t, 1/unit, used by the density tint. */
    double lineDensity = 0.0;
    /** E / A_t, 1/unit^2, used by the density tint. */
    double pieceDensity = 0.0;
    /** False when any input or intermediate was non-finite / unrepresentable. */
    bool finite = true;
};

/**
 * ONE budget per hatch generation; shared by every root through the trimmer.
 * emit() in LC_Loops::trimPatternEntities calls take() for every produced
 * piece. Once tripped, the trimmer aborts deterministically and discards all
 * buffered output.
 */
struct LC_HatchPieceBudget {
    std::size_t limit = 0;
    std::size_t taken = 0;
    bool tripped = false;

    bool take() noexcept {
        if (taken >= limit) {
            tripped = true;
            return false;
        }
        ++taken;
        return true;
    }
};

namespace LC_HatchDensity {

/**
 * The currently installed policy, replaced by ScopedPolicy. Default-constructed
 * at process start.
 */
const LC_HatchDensityPolicy& policy() noexcept;

/**
 * Scope guard that installs a replacement policy for its lifetime. Nestable;
 * dtor restores the previous value.
 */
class ScopedPolicy {
public:
    explicit ScopedPolicy(const LC_HatchDensityPolicy& p);
    ~ScopedPolicy();

    ScopedPolicy(const ScopedPolicy&) = delete;
    ScopedPolicy& operator=(const ScopedPolicy&) = delete;

private:
    LC_HatchDensityPolicy m_previous;
};

/**
 * Monotone counter bumped on every ScopedPolicy ctor/dtor, so a later
 * geometry cache can invalidate itself when the active cap changed.
 */
std::uint64_t policyRevision() noexcept;

/**
 * Even-odd area of a loop forest: Σ_roots f(root, 0),
 *   f(L, d) = (d even ? +1 : -1) * |L.outer area| + Σ_children f(c, d+1).
 * The result is a containment-tree area; it equals odd-even area only for a
 * noncrossing valid forest.
 */
double evenOddArea(const std::vector<LC_LoopUtils::LC_Loops>& roots);

/**
 * Build an estimate of the number of pattern pieces that will be clipped
 * against the loop forest. Pattern must already be scaled; patternTurn is 0
 * while loops live in the pattern frame and nonzero only when a caller
 * passes WCS boundaries directly.
 *
 * Returns a report with areaTerm, boundaryTerm, estimatedPieces, lineDensity
 * and pieceDensity set, and finite = true when every input and intermediate
 * value is finite and nonnegative with A_t > 0.
 */
LC_HatchDensityReport estimate(const RS_Pattern& scaledPattern,
                               const std::vector<LC_LoopUtils::LC_Loops>& roots,
                               double patternTurn);

/**
 * The scale s* at which the estimate predicts exactly maxPieces at the given
 * boundary configuration. Solves c1*(s/s*)^2 + c2*(s/s*) = M for s/s*, so the
 * returned value is a scaling factor relative to currentScale. Returns +inf
 * when the estimate has no bounded solution (empty area + boundary) and NaN
 * on invalid inputs.
 */
double minimumScale(const LC_HatchDensityReport& r,
                    double currentScale,
                    double maxPieces);

} // namespace LC_HatchDensity

#endif // LC_HATCHDENSITY_H
