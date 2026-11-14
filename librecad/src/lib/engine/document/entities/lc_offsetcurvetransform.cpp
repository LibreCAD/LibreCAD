/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvetransform.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "lc_offsetcurve.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_point.h"

namespace {
using namespace LC_OffsetCurveTransform;
constexpr std::size_t MaximumEntities = std::size_t{1} << 20;
constexpr unsigned MaximumDepth = 256;

bool samePoints(const std::vector<RS_Vector> &a, const std::vector<RS_Vector> &b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const auto &x, const auto &y) {
        return x.valid == y.valid && x.x == y.x && x.y == y.y && x.z == y.z;
    });
}

bool sameCache(const RS_SplineData &a, const RS_SplineData &b) {
    return a.degree == b.degree && a.type == b.type && a.savedOpenType == b.savedOpenType &&
        a.m_closedFlag == b.m_closedFlag && a.m_periodicFlag == b.m_periodicFlag &&
        samePoints(a.controlPoints, b.controlPoints) && samePoints(a.fitPoints, b.fitPoints) &&
        a.weights == b.weights && a.knotslist == b.knotslist && a.savedOpenKnots == b.savedOpenKnots;
}

bool knownType(RS_SplineData::SplineType type) {
    using Type = RS_SplineData::SplineType;
    return type == Type::Standard || type == Type::ClampedOpen || type == Type::WrappedClosed;
}

bool validKnots(const std::vector<double> &knots, std::size_t controls, std::size_t degree,
    RS_SplineData::SplineType type) {
    if (controls < degree + 1 || knots.size() != controls + degree + 1 ||
        !std::all_of(knots.begin(), knots.end(), [](double k) { return std::isfinite(k); }) ||
        !std::is_sorted(knots.begin(), knots.end()) || !(knots[degree] < knots[controls])) return false;
    std::size_t multiplicity = 1;
    for (std::size_t i = 1; i < knots.size(); ++i) {
        multiplicity = knots[i] == knots[i - 1] ? multiplicity + 1 : 1;
        if (multiplicity > degree + 1) return false;
    }
    if (type == RS_SplineData::SplineType::ClampedOpen) {
        for (std::size_t i = 0; i <= degree; ++i)
            if (knots[i] != knots.front() || knots[knots.size() - 1 - i] != knots.back()) return false;
    } else if (knots[0] == knots[1] || knots[knots.size() - 1] == knots[knots.size() - 2]) return false;
    return true;
}

bool finiteCache(const RS_Spline &spline) {
    const auto &data = spline.getData();
    const LC_OffsetCheckLimits hard;
    const auto controls = data.controlPoints.size();
    if (!knownType(data.type) || !knownType(data.savedOpenType) || data.degree < 1 || data.degree > 3 ||
        controls < data.degree + 1 || controls > hard.maxVertices || data.fitPoints.size() > hard.maxVertices ||
        data.weights.size() != controls || data.knotslist.size() > 4 * hard.maxVertices ||
        data.savedOpenKnots.size() > 4 * hard.maxVertices) return false;
    const auto finitePoint = [](const auto &p) { return LC_EllipseOffsetGeometry::isFinite2D(p); };
    if (!std::all_of(data.controlPoints.begin(), data.controlPoints.end(), finitePoint) ||
        !std::all_of(data.fitPoints.begin(), data.fitPoints.end(), finitePoint) ||
        !std::all_of(data.weights.begin(), data.weights.end(), [](double w) { return std::isfinite(w) && w > 0; }) ||
        !validKnots(data.knotslist, controls, data.degree, data.type)) return false;
    const bool wrapped = data.type == RS_SplineData::SplineType::WrappedClosed;
    if (wrapped) {
        if (controls - data.degree < data.degree + 1) return false;
        for (std::size_t i = 0; i < data.degree; ++i) {
            const auto &a = data.controlPoints[i], &b = data.controlPoints[controls - data.degree + i];
            if (a.x != b.x || a.y != b.y || a.z != b.z || data.weights[i] != data.weights[controls - data.degree + i])
                return false;
        }
    }
    if (!data.savedOpenKnots.empty() &&
        (data.savedOpenType == RS_SplineData::SplineType::WrappedClosed ||
         !validKnots(data.savedOpenKnots, controls - (wrapped ? data.degree : 0), data.degree, data.savedOpenType)))
        return false;
    return spline.validate();
}

struct TreeInfo {
    Refusal refusal = Refusal::None;
    bool offsetDescendants = false;
    bool inserts = false;
};

bool finiteBounds(const RS_Entity &entity) {
    const auto low = entity.getMin(), high = entity.getMax();
    return LC_EllipseOffsetGeometry::isFinite2D(low) && LC_EllipseOffsetGeometry::isFinite2D(high) &&
        low.x <= high.x && low.y <= high.y;
}

TreeInfo inspectTree(const RS_Entity &source, bool checkInsertStatus = false, bool requireKnownGeometry = false) {
    struct Pending { const RS_Entity *entity; unsigned depth; };
    TreeInfo info;
    std::vector<Pending> pending{{&source, 0}};
    std::unordered_set<const RS_Entity *> seen;
    std::size_t visits = 0;
    bool unknownGeometry = false;
    requireKnownGeometry = requireKnownGeometry || dynamic_cast<const LC_OffsetCurve *>(&source) != nullptr;
    while (!pending.empty()) {
        if (++visits > MaximumEntities) { info.refusal = Refusal::UnsafeContainer; return info; }
        const auto node = pending.back(); pending.pop_back();
        if (!node.entity || node.depth > MaximumDepth || !seen.insert(node.entity).second) {
            info.refusal = Refusal::UnsafeContainer; return info;
        }
        const auto &entity = *node.entity;
        if (&entity != &source && dynamic_cast<const LC_OffsetCurve *>(&entity)) info.offsetDescendants = true;
        if (const auto *insert = dynamic_cast<const RS_Insert *>(&entity)) {
            info.inserts = true;
            if (checkInsertStatus && !insert->lastSourceEditSucceeded()) {
                info.refusal = Refusal::SourceEditRejected; return info;
            }
        }
        const auto *container = dynamic_cast<const RS_EntityContainer *>(&entity);
        if (container) {
            // Copy constructors of non-owners retain their borrowed child pointers.
            if (!container->isOwner() || container->size() > MaximumEntities - visits ||
                pending.size() > MaximumEntities - visits - container->size()) {
                info.refusal = Refusal::UnsafeContainer; return info;
            }
            for (const auto *child : *container) pending.push_back({child, node.depth + 1});
        }
        const auto finite = [](const RS_Vector &p) { return LC_EllipseOffsetGeometry::isFinite2D(p); };
        bool valid = true, primitive = true;
        if (const auto *spline = dynamic_cast<const RS_Spline *>(&entity)) {
            if (!finiteCache(*spline)) { info.refusal = Refusal::InvalidCache; return info; }
        } else if (const auto *line = dynamic_cast<const RS_Line *>(&entity)) {
            valid = finite(line->getStartpoint()) && finite(line->getEndpoint());
        } else if (const auto *circle = dynamic_cast<const RS_Circle *>(&entity)) {
            valid = finite(circle->getCenter()) && std::isfinite(circle->getRadius()) && circle->getRadius() >= 0;
        } else if (const auto *arc = dynamic_cast<const RS_Arc *>(&entity)) {
            const auto &data = arc->getData();
            valid = finite(data.center) && std::isfinite(data.radius) && data.radius >= 0 &&
                std::isfinite(data.angle1) && std::isfinite(data.angle2) &&
                std::isfinite(data.startAngleDegrees) && std::isfinite(data.otherAngleDegrees) &&
                std::isfinite(data.angularLength) && finite(arc->getStartpoint()) && finite(arc->getEndpoint());
        } else if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(&entity)) {
            const auto &data = ellipse->getData();
            valid = finite(data.center) && finite(data.majorP) && std::isfinite(data.ratio) && data.ratio >= 0 &&
                std::isfinite(data.angle1) && std::isfinite(data.angle2) && std::isfinite(data.angleDegrees) &&
                std::isfinite(data.startAngleDegrees) && std::isfinite(data.otherAngleDegrees) &&
                std::isfinite(data.angularLength);
            if (valid && data.isArc) valid = finite(ellipse->getStartpoint()) && finite(ellipse->getEndpoint());
        } else if (const auto *point = dynamic_cast<const RS_Point *>(&entity)) {
            valid = finite(point->getPos());
        } else {
            primitive = false;
            if (entity.rtti() != RS2::EntityContainer && !dynamic_cast<const RS_Insert *>(&entity))
                unknownGeometry = true;
        }
        if (!valid || (primitive && !finiteBounds(entity))) {
            info.refusal = Refusal::InvalidResult; return info;
        }
        // Empty/all-hidden groups may legitimately have reset aggregate bounds.
        // Every admitted primitive is checked independently, including hidden ones.
        if (!primitive && container && (entity.rtti() == RS2::EntityContainer ||
                                       dynamic_cast<const RS_Insert *>(&entity))) {
            const bool hasVisibleBounds = std::any_of(container->begin(), container->end(), [](const RS_Entity *child) {
                return child && child->isVisible() && child->hasValidBorders();
            });
            if (hasVisibleBounds && !finiteBounds(entity)) { info.refusal = Refusal::InvalidResult; return info; }
        }
    }
    // Discover native descendants before applying the carrier policy, so sibling
    // order cannot let an unknown geometry type escape the native finite contract.
    if (unknownGeometry && (requireKnownGeometry || info.offsetDescendants)) info.refusal = Refusal::UnsafeContainer;
    return info;
}

bool validStep(const Step &step) {
    const auto finite = [](const auto &p) { return LC_EllipseOffsetGeometry::isFinite2D(p); };
    switch (step.kind) {
    case Step::Kind::Move: return finite(step.first);
    case Step::Kind::Rotate: return finite(step.first) && std::isfinite(step.scalar);
    case Step::Kind::Scale: return finite(step.first) && finite(step.second);
    case Step::Kind::Mirror:
        return finite(step.first) && finite(step.second) &&
            (step.first.x != step.second.x || step.first.y != step.second.y);
    case Step::Kind::Shear: return std::isfinite(step.scalar);
    case Step::Kind::Reverse: return true;
    }
    return false;
}

bool demoteTransparentOffsets(RS_EntityContainer &root) {
    struct Pending { RS_EntityContainer *container; bool transparent; };
    struct Replacement { RS_EntityContainer *parent; int index; std::unique_ptr<RS_Spline> plain; };
    std::vector<Pending> pending{{&root, true}};
    std::vector<Replacement> replacements;
    std::size_t visits = 0;
    while (!pending.empty()) {
        if (++visits > MaximumEntities) return false;
        const auto item = pending.back(); pending.pop_back();
        auto &container = *item.container;
        if (!container.isOwner() || container.size() > MaximumEntities - visits ||
            pending.size() > MaximumEntities - visits - container.size()) return false;
        const bool transparent = item.transparent && container.rtti() == RS2::EntityContainer;
        for (std::size_t i = 0; i < container.size(); ++i) {
            if (++visits > MaximumEntities) return false;
            auto *child = container.entityAt(static_cast<int>(i));
            if (const auto *native = dynamic_cast<const LC_OffsetCurve *>(child)) {
                if (!transparent) return false;
                auto plain = native->toPlainSpline(); plain->reparent(&container);
                replacements.push_back({&container, static_cast<int>(i), std::move(plain)});
            }
            if (auto *nested = dynamic_cast<RS_EntityContainer *>(child))
                pending.push_back({nested, transparent});
        }
    }
    // All conversions and ownership checks precede edits to the private clone.
    for (auto &row : replacements) {
        row.parent->setEntityAt(row.index, row.plain.get());
        row.plain.release();
    }
    return true;
}

bool identity(const Step &step) {
    switch (step.kind) {
    case Step::Kind::Move: return step.first.x == 0 && step.first.y == 0;
    case Step::Kind::Rotate: case Step::Kind::Shear: return step.scalar == 0;
    case Step::Kind::Scale: return step.second.x == 1 && step.second.y == 1;
    case Step::Kind::Mirror: case Step::Kind::Reverse: return false;
    }
    return false;
}

bool prepareMirror(Step &step) {
    if (step.kind != Step::Kind::Mirror) return true;
    const double dx = step.second.x - step.first.x, dy = step.second.y - step.first.y;
    const double length = std::hypot(dx, dy);
    if (!std::isfinite(length)) return false;
    // RS_Vector/RS_EntityContainer silently ignore short mirror axes. Express the
    // same line with a longer direction before invoking either implementation.
    if (length <= 2 * RS_TOLERANCE) {
        using Exact = boost::multiprecision::cpp_rational;
        const Exact originalX = Exact(step.second.x) - Exact(step.first.x);
        const Exact originalY = Exact(step.second.y) - Exact(step.first.y);
        const double scale = std::max(std::abs(dx), std::abs(dy));
        if (!(scale > 0)) return false;
        step.second = {step.first.x + dx / scale, step.first.y + dy / scale};
        if (!LC_EllipseOffsetGeometry::isFinite2D(step.second)) return false;
        const Exact extendedX = Exact(step.second.x) - Exact(step.first.x);
        const Exact extendedY = Exact(step.second.y) - Exact(step.first.y);
        // Rounded endpoint construction must not silently change the picked line.
        if (originalX * extendedY != originalY * extendedX) return false;
    }
    const auto direction = step.second - step.first;
    const double squared = direction.x * direction.x + direction.y * direction.y;
    return std::isfinite(squared) && squared > RS_TOLERANCE2;
}

void apply(RS_Entity &entity, const Step &step) {
    switch (step.kind) {
    case Step::Kind::Move: entity.move(step.first); break;
    case Step::Kind::Rotate: entity.rotate(step.first, step.scalar); break;
    case Step::Kind::Scale: entity.scale(step.first, step.second); break;
    case Step::Kind::Mirror: entity.mirror(step.first, step.second); break;
    case Step::Kind::Shear: entity.shear(step.scalar); break;
    case Step::Kind::Reverse: entity.revertDirection(); break;
    }
}

Refusal applyTree(RS_Entity &entity, const Step &step) {
    // Transparent generic containers can recurse without the old small-scale
    // early return, and can observe an INSERT refusal before visiting siblings.
    if (entity.rtti() == RS2::EntityContainer) {
        auto &container = static_cast<RS_EntityContainer &>(entity);
        if (step.kind == Step::Kind::Reverse) container.revertDirection();
        else for (auto *child : container) {
            const auto refusal = applyTree(*child, step);
            if (refusal != Refusal::None) return refusal;
        }
        container.calculateBorders();
    } else {
        apply(entity, step);
        if (const auto *insert = dynamic_cast<const RS_Insert *>(&entity); insert && !insert->lastSourceEditSucceeded())
            return Refusal::SourceEditRejected;
    }
    return Refusal::None;
}
}

bool LC_OffsetCurveTransform::needsStaging(const RS_Entity &source) {
    std::vector<const RS_Entity *> pending{&source};
    std::unordered_set<const RS_Entity *> seen;
    std::size_t visits = 0;
    while (!pending.empty()) {
        if (++visits > MaximumEntities) return true;
        const auto *entity = pending.back(); pending.pop_back();
        if (!entity || !seen.insert(entity).second) continue;
        if (dynamic_cast<const LC_OffsetCurve *>(entity) || dynamic_cast<const RS_Insert *>(entity)) return true;
        if (const auto *container = dynamic_cast<const RS_EntityContainer *>(entity)) {
            if (container->size() > MaximumEntities - visits ||
                pending.size() > MaximumEntities - visits - container->size()) return true;
            for (const auto *child : *container) pending.push_back(child);
        }
    }
    return false;
}

LC_OffsetCurveTransform::Result LC_OffsetCurveTransform::prepare(const RS_Entity &source,
    const Steps &steps, const bool forcePlain) {
    Result result;
    if (!steps.valid()) { result.refusal = Refusal::InvalidSteps; return result; }
    std::array<Step, MaximumSteps> admitted{};
    for (std::size_t i = 0; i < steps.size(); ++i) {
        admitted[i] = steps[i];
        if (!validStep(admitted[i]) || !prepareMirror(admitted[i])) {
            result.refusal = Refusal::InvalidSteps; result.failedStep = i; return result;
        }
    }
    const auto tree = inspectTree(source);
    if (tree.refusal != Refusal::None) { result.refusal = tree.refusal; return result; }
    if (tree.offsetDescendants && source.rtti() != RS2::EntityContainer &&
        !dynamic_cast<const RS_Insert *>(&source)) {
        result.refusal = Refusal::UnsafeContainer; return result;
    }
    // These inherited INSERT operations only alter expansion children, not its
    // persistent source transform. No native source-edit API exists for them.
    for (std::size_t i = 0; tree.inserts && i < steps.size(); ++i) {
        if (admitted[i].kind == Step::Kind::Reverse ||
            (admitted[i].kind == Step::Kind::Shear && !identity(admitted[i]))) {
            result.refusal = Refusal::SourceEditRejected; result.failedStep = i; return result;
        }
    }
    if (const auto *native = dynamic_cast<const LC_OffsetCurve *>(&source)) {
        auto plain = native->toPlainSpline();
        std::unique_ptr<LC_OffsetCurve> transported;
        if (!forcePlain && native->hasEditableDefinition())
            transported.reset(static_cast<LC_OffsetCurve *>(native->clone()));
        for (std::size_t i = 0; i < steps.size(); ++i) {
            const auto &step = admitted[i];
            if (identity(step)) continue;
            // Equality after two silent no-ops is not evidence of native success.
            // Explicit identities were handled above; rounded-away non-identities
            // may retain only the ordinary finite spline result.
            RS_SplineData before;
            if (transported) before = plain->getData();
            apply(*plain, step);
            if (!finiteCache(*plain) || inspectTree(*plain, false, true).refusal != Refusal::None) {
                result.failedStep = i; return result;
            }
            const bool losesDefinition = step.kind == Step::Kind::Scale &&
                (step.second.x == 0 || step.second.y == 0 || std::abs(step.second.x) != std::abs(step.second.y));
            if (transported && (losesDefinition || sameCache(before, plain->getData()))) transported.reset();
            if (transported) {
                apply(*transported, step);
                if (!transported->hasEditableDefinition() || !sameCache(transported->getData(), plain->getData()))
                    transported.reset();
            }
        }
        if (transported) result.entity = std::move(transported);
        else { result.entity = std::move(plain); result.keptPlain = true; }
    } else {
        auto staged = std::unique_ptr<RS_Entity>(source.clone());
        if (!staged) return result;
        bool changes = forcePlain;
        for (std::size_t i = 0; i < steps.size(); ++i) changes = changes || !identity(admitted[i]);
        if (changes && tree.offsetDescendants && source.rtti() == RS2::EntityContainer) {
            if (!demoteTransparentOffsets(static_cast<RS_EntityContainer &>(*staged))) {
                result.refusal = Refusal::UnsafeContainer; return result;
            }
            result.keptPlain = true;
        }
        for (std::size_t i = 0; i < steps.size(); ++i) {
            if (identity(admitted[i])) continue;
            auto refusal = applyTree(*staged, admitted[i]);
            if (refusal == Refusal::None) refusal = inspectTree(*staged, true, tree.offsetDescendants).refusal;
            if (refusal != Refusal::None) {
                result.refusal = refusal == Refusal::InvalidCache ? Refusal::InvalidResult : refusal;
                result.failedStep = i; return result;
            }
        }
        result.entity = std::move(staged);
    }
    result.refusal = Refusal::None;
    return result;
}
