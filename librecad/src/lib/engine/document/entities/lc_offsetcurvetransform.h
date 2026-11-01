/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVETRANSFORM_H
#define LC_OFFSETCURVETRANSFORM_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <memory>

#include "rs_entity.h"

namespace LC_OffsetCurveTransform {
inline constexpr std::size_t MaximumSteps = 8;

struct Step {
    enum class Kind { Move, Rotate, Scale, Mirror, Shear, Reverse };
    Kind kind = Kind::Move;
    RS_Vector first{0, 0};
    RS_Vector second{0, 0};
    double scalar = 0;

    static Step move(const RS_Vector &offset) { return {Kind::Move, offset, {0, 0}, 0}; }
    static Step rotate(const RS_Vector &center, double angle) { return {Kind::Rotate, center, {0, 0}, angle}; }
    static Step scale(const RS_Vector &center, const RS_Vector &factor) { return {Kind::Scale, center, factor, 0}; }
    static Step mirror(const RS_Vector &a, const RS_Vector &b) { return {Kind::Mirror, a, b, 0}; }
    static Step shear(double factor) { return {Kind::Shear, {0, 0}, {0, 0}, factor}; }
    static Step reverse() { return {Kind::Reverse, {0, 0}, {0, 0}, 0}; }
};

/** Fixed storage; overflow latches invalid even if the caller ignores append's result. */
class Steps {
public:
    Steps() = default;
    Steps(std::initializer_list<Step> steps) {
        if (steps.size() > MaximumSteps) { m_valid = false; return; }
        for (const auto &step : steps) append(step);
    }
    bool append(const Step &step) {
        if (!m_valid || m_size == MaximumSteps) { m_valid = false; return false; }
        m_steps[m_size++] = step;
        return true;
    }
    bool valid() const noexcept { return m_valid; }
    std::size_t size() const noexcept { return m_size; }
    const Step &operator[](std::size_t index) const { return m_steps[index]; }
private:
    std::array<Step, MaximumSteps> m_steps{};
    std::size_t m_size = 0;
    bool m_valid = true;
};

enum class Refusal { None, InvalidSteps, InvalidCache, UnsafeContainer, InvalidResult, SourceEditRejected };
struct Result {
    std::unique_ptr<RS_Entity> entity;
    Refusal refusal = Refusal::InvalidResult;
    bool keptPlain = false;
    // MaximumSteps means refusal was not associated with an individual step.
    std::size_t failedStep = MaximumSteps;
    explicit operator bool() const noexcept { return bool(entity) && refusal == Refusal::None; }
};

/** Stage all steps or refuse without modifying source data, children or attributes.
 * All parameters and the bounded owning subtree are admitted before cloning.
 * Empty lists and explicit identity steps are legitimate no-ops. Singular scales
 * can produce only finite plain geometry, never a retained native definition.
 * INSERT source-edit failures are checked after EVERY step, including descendants;
 * nonzero shear/reverse refuse rather than editing only its derived display cache.
 * Owned transparent groups demote native descendants before changing geometry;
 * specialized container paths refuse rather than corrupting their own data model.
 */
Result prepare(const RS_Entity &source, const Steps &steps, bool forcePlain = false);
/** Bounded read-only routing; ordinary text/image previews keep their existing path. */
bool needsStaging(const RS_Entity &source);
}
#endif
