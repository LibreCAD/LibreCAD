/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_action_offsetcurve_restoreoriginal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <QMap>
#include <QSet>
#include <QStringList>

#include "lc_actioncontext.h"
#include "lc_ellipseoffsetgeometry.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "lc_selectedset.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;
using Exact = boost::multiprecision::cpp_rational;
using Interval = LC_Interval;

struct Basis {
    RS_EllipseData data;
    RS_EllipseData raw;
    geo::Sweep sweep;
};

bool sameAttributePen(const RS_Pen& a, const RS_Pen& b) {
    const auto sameNumber = [](const double x, const double y) {
        return std::memcmp(&x, &y, sizeof(double)) == 0;
    };
    return a == b && a.getFlags() == b.getFlags() && a.getLineTypeId() == b.getLineTypeId() &&
        sameNumber(a.getScreenWidth(), b.getScreenWidth()) && sameNumber(a.getAlpha(), b.getAlpha()) &&
        sameNumber(a.dashOffset(), b.dashOffset()) && a.getColor().colorName() == b.getColor().colorName();
}

// Pin the bounded inputs to the existing resolver; do not duplicate ByBlock/
// ByLayer resolution or call that callback-bearing resolver after the seal.
struct AttributeState {
    struct Entry {
        const RS_Entity* address;
        decltype(std::declval<const RS_Entity&>().getId()) id;
        RS_Layer* layer;
        RS_Pen pen;
        bool registered = false;
        RS_Pen layerPen;
    };
    std::vector<Entry> entries;
    const RS_Graphic* graphic = nullptr;

    static std::optional<AttributeState> capture(const RS_Entity& source) {
        AttributeState result;
        std::size_t count = 0;
        for (const RS_Entity* item = &source; item; item = item->getParent()) {
            if (++count > 64) return std::nullopt;
            if (dynamic_cast<const RS_Graphic*>(item)) break;
        }
        result.entries.reserve(count);
        for (const RS_Entity* item = &source; item; item = item->getParent()) {
            const auto pen = item->getPen(false);
            if (std::size_t(pen.getColor().colorName().size()) > LC_OffsetCurve::MaximumOwnedStringUnits)
                return std::nullopt;
            result.entries.push_back({item, item->getId(), item->getLayer(false), pen, false, {}});
            result.graphic = dynamic_cast<const RS_Graphic*>(item);
            if (result.graphic) break;
        }
        const auto* layers = result.graphic ?
            const_cast<RS_Graphic*>(result.graphic)->RS_Graphic::getLayerList() : nullptr;
        for (auto& entry : result.entries) {
            if (entry.layer && !layers) return std::nullopt;
            entry.registered = entry.layer && layers->contains(entry.layer);
            if (!entry.registered) continue;
            const auto pen = entry.layer->getPen(); // Registry membership precedes dereference.
            if (std::size_t(pen.getColor().colorName().size()) > LC_OffsetCurve::MaximumOwnedStringUnits)
                return std::nullopt;
            entry.layerPen = pen;
        }
        return result;
    }

    bool matches(const RS_Entity& source) const {
        const RS_Entity* item = &source;
        const RS_Graphic* currentGraphic = nullptr;
        for (const auto& entry : entries) {
            if (!item || item != entry.address || item->getId() != entry.id ||
                item->getLayer(false) != entry.layer || !sameAttributePen(item->getPen(false), entry.pen))
                return false;
            currentGraphic = dynamic_cast<const RS_Graphic*>(item);
            item = currentGraphic ? nullptr : item->getParent();
        }
        if (item || currentGraphic != graphic) return false;
        const auto* layers = currentGraphic ?
            const_cast<RS_Graphic*>(currentGraphic)->RS_Graphic::getLayerList() : nullptr;
        for (const auto& entry : entries) {
            const bool registered = entry.layer && layers && layers->contains(entry.layer);
            if (registered != entry.registered ||
                (registered && !sameAttributePen(entry.layer->getPen(), entry.layerPen))) return false;
        }
        return true;
    }
};

std::optional<Basis> normalized(const RS_EllipseData& raw) {
    auto data = raw;
    const auto domain = geo::sweep(data);
    if (!domain) return std::nullopt;
    // Do not promote a nearly full ordinary arc to a whole duplicate merely
    // because native normalization permits a small full-turn admission band.
    const double turn = 2 * std::acos(-1.);
    if (domain->whole && !(raw.angle1 == 0 && raw.angle2 == 0)) {
        // The subtraction itself must not round a real stored gap/overshoot
        // into the explicit full-turn sentinel. Compare exact input doubles.
        const Exact delta = Exact(raw.angle2) - Exact(raw.angle1);
        if (delta != Exact(turn) && delta != -Exact(turn)) return std::nullopt;
    }
    if (!geo::normaliseBasis(data)) return std::nullopt;
    const auto sweep = geo::sweep(data);
    if (!sweep) return std::nullopt;
    return Basis{data, raw, *sweep};
}

bool sameAxis(const RS_Vector& a, const RS_Vector& b) {
    return a.x == b.x && a.y == b.y;
}
std::array<Exact, 3> shape(const RS_EllipseData& basis) {
    const Exact x(basis.majorP.x), y(basis.majorP.y), ratio(basis.ratio);
    const Exact ratioSquared = ratio*ratio;
    // The exact local shape matrix M*M^T. This also handles an ordinary
    // ellipse whose axes need exchanging, without floating angle reconstruction.
    return {x*x + ratioSquared*y*y, (1-ratioSquared)*x*y, y*y + ratioSquared*x*x};
}
bool finite(const Interval& value) {
    return value.isValid() && std::isfinite(value.lo()) && std::isfinite(value.hi());
}
bool within(const Interval& a, const Interval& b, const double allowance) {
    const auto difference = a - b;
    return finite(difference) && difference.lo() >= -allowance && difference.hi() <= allowance;
}
struct PointBox { Interval x, y; };
bool close(const PointBox& a, const PointBox& b, const double allowance) {
    return within(a.x, b.x, allowance) && within(a.y, b.y, allowance);
}
bool separated(const PointBox& a, const PointBox& b, const double allowance) {
    const auto x = a.x - b.x, y = a.y - b.y;
    return finite(x) && finite(y) &&
        (x.lo() > allowance || x.hi() < -allowance || y.lo() > allowance || y.hi() < -allowance);
}
double ulp(const double value) {
    return std::nextafter(value, std::numeric_limits<double>::infinity()) - value;
}
bool anchors(const geo::ReferenceFrame& frame, const double commonScale,
             std::array<PointBox, 3>& points) {
    const auto factor = Interval::point(frame.scale()/commonScale);
    if (!finite(factor) || !factor.isPositive()) return false;
    const double end = frame.parameterEnd();
    unsigned index = 0;
    for (const double parameter : {0., end*.5, end}) {
        LC_CurveJetBounds jet;
        if (!frame.boundSourceJet(parameter, parameter, jet)) return false;
        points[index++] = {jet.x * factor, jet.y * factor};
    }
    return true;
}

/** Positive-only circle certificate: r*|phase difference| bounds point distance. */
bool circularAnchorsMatch(const Basis& a, const Basis& b, const double scale,
                          const double allowance) {
    const auto& axis = a.data.majorP;
    const auto& other = b.data.majorP;
    int quarterTurns;
    if (sameAxis(axis, other)) quarterTurns = 0;
    else if (sameAxis({-axis.y, axis.x}, other)) quarterTurns = 1;
    else if (sameAxis({-axis.x, -axis.y}, other)) quarterTurns = 2;
    else if (sameAxis({axis.y, -axis.x}, other)) quarterTurns = -1;
    else return false;

    struct Phase { Exact constant; int halfPiUnits; };
    const auto phases = [](const RS_EllipseData& data) {
        const Exact start(data.angle1), end(data.angle2);
        const int winding = !data.reversed && data.angle2 < data.angle1 ? 1 :
            data.reversed && data.angle2 > data.angle1 ? -1 : 0;
        return std::array<Phase, 3>{{{start, 0}, {Exact((start + end)/2), 2*winding},
            {end, 4*winding}}};
    };
    const auto first = phases(a.data), second = phases(b.data);
    const Exact radiusSquared = shape(a.raw)[0] / (Exact(scale)*Exact(scale));
    const Exact squaredAllowance = Exact(allowance)*Exact(allowance);
    const double turn = 2*std::acos(-1.);
    const auto closePhase = [&](const Phase& x, const Phase& y) {
        const Exact constant = y.constant - x.constant;
        const int units = y.halfPiUnits - x.halfPiUnits + quarterTurns;
        // Floating arithmetic selects a bounded candidate only, never a proof.
        const double winding = std::round((constant.convert_to<double>() + units*(turn/4))/turn);
        if (!std::isfinite(winding) || winding < -4 || winding > 4) return false;
        const Exact coefficient = Exact(units - 4*static_cast<int>(winding))/2;
        // The same real-pi enclosure as ReferenceFrame, combined exactly so
        // repeated interval range-reduction rounding cannot swamp the ULP policy.
        Exact lo = constant + coefficient*Exact(0x1.921fb54442d18p+1);
        Exact hi = constant + coefficient*Exact(0x1.921fb54442d19p+1);
        if (hi < lo) std::swap(lo, hi);
        const Exact absoluteLo = lo < 0 ? -lo : lo;
        const Exact absoluteHi = hi < 0 ? -hi : hi;
        const Exact bound = std::max(absoluteLo, absoluteHi);
        return bound*bound*radiusSquared <= squaredAllowance;
    };
    return closePhase(first[1], second[1]) &&
        ((closePhase(first[0], second[0]) && closePhase(first[2], second[2])) ||
         (closePhase(first[0], second[2]) && closePhase(first[2], second[0])));
}

/** Conservative geometric identity, independent of layer and traversal direction. */
std::optional<bool> equivalent(const Basis& a, const Basis& b) {
    // true: equal under the explicit local ULP policy; false: proved different;
    // nullopt: the bounded numeric witnesses cannot decide. Never insert on unknown.
    // No world-center-relative epsilon or ULP band: different stored centers
    // remain different even when their magnitude dwarfs the local geometry.
    if (a.data.center.x != b.data.center.x || a.data.center.y != b.data.center.y) return false;
    const bool circleA = a.data.ratio == 1., circleB = b.data.ratio == 1.;
    if (circleA != circleB) return false; // never turn a near-circle into a circle
    // No tolerance on shape: use exact input-double algebra, before any
    // axis-switch rounding. For circles the matrix is radius^2 times identity,
    // so (3,4) and (5,0) are equivalent without privileging either axis.
    if (shape(a.raw) != shape(b.raw)) return false;
    if (a.sweep.whole != b.sweep.whole) return false;
    if (a.sweep.whole) return true;
    if (sameAxis(a.data.majorP, b.data.majorP)) {
        if (a.data.angle1 == b.data.angle1 && a.data.angle2 == b.data.angle2 &&
            a.data.reversed == b.data.reversed) return true;
        if (a.data.angle1 == b.data.angle2 && a.data.angle2 == b.data.angle1 &&
            a.data.reversed != b.data.reversed) return true;
    }

    auto localA = a.data, localB = b.data;
    localA.center = localB.center = RS_Vector{0, 0};
    const auto frameA = geo::ReferenceFrame::create(localA, 0.);
    const auto frameB = geo::ReferenceFrame::create(localB, 0.);
    if (!frameA || !frameB) return std::nullopt;
    const double scale = std::max(frameA->scale(), frameB->scale());
    const double radius = std::max(localA.majorP.magnitude(), localB.majorP.magnitude()) / scale;
    // Sixteen ULPs in the power-of-two local frame cover only representation
    // and interval arithmetic. Bounds wider than this do not establish identity.
    const double allowance = 16 * ulp(radius);
    const double angleAllowance = 16 * ulp(2 * std::acos(-1.));
    if (!std::isfinite(allowance) || !(allowance > 0)) return std::nullopt;
    const auto extentA = frameA->traversalRate() * Interval::point(frameA->parameterEnd());
    const auto extentB = frameB->traversalRate() * Interval::point(frameB->parameterEnd());
    if (!within(extentA, extentB, angleAllowance)) {
        const auto delta = extentA - extentB;
        if (finite(delta) && (delta.lo() > angleAllowance || delta.hi() < -angleAllowance)) return false;
        return std::nullopt;
    }

    std::array<PointBox, 3> pointsA, pointsB;
    if (!anchors(*frameA, scale, pointsA) || !anchors(*frameB, scale, pointsB)) return std::nullopt;
    for (const auto* points : {&pointsA, &pointsB}) {
        if (!separated((*points)[0], (*points)[1], 64*allowance) ||
            !separated((*points)[1], (*points)[2], 64*allowance) ||
            !separated((*points)[0], (*points)[2], 64*allowance)) return std::nullopt;
    }
    const bool endpoints = (close(pointsA[0], pointsB[0], allowance) && close(pointsA[2], pointsB[2], allowance)) ||
        (close(pointsA[0], pointsB[2], allowance) && close(pointsA[2], pointsB[0], allowance));
    // The middle is essential: complementary arcs can share both endpoints.
    if (endpoints && close(pointsA[1], pointsB[1], allowance)) return true;
    if (circleA && circularAnchorsMatch(a, b, scale, allowance)) return true;
    const bool directExcluded = separated(pointsA[0], pointsB[0], allowance) ||
        separated(pointsA[2], pointsB[2], allowance);
    const bool reverseExcluded = separated(pointsA[0], pointsB[2], allowance) ||
        separated(pointsA[2], pointsB[0], allowance);
    if (separated(pointsA[1], pointsB[1], allowance) || (directExcluded && reverseExcluded)) return false;
    return std::nullopt;
}
}

LC_ActionOffsetCurveRestoreOriginal::LC_ActionOffsetCurveRestoreOriginal(LC_ActionContext* context)
    : LC_ActionPreSelectionAwareBase("ActionOffsetCurveRestoreOriginal", context,
          RS2::ActionOffsetCurveRestoreOriginal, {RS2::EntityOffsetCurve}) {
    m_liveView = m_graphicView;
    if (m_document) m_document->getSelection()->addListener(this);
}

void LC_ActionOffsetCurveRestoreOriginal::selectedSetDestroyed() {
    // The tracked selection registration is already removed; its document is partly destroyed.
    m_document = nullptr;
    m_graphic = nullptr;
    m_keyEntity = nullptr;
    if (m_preview) m_preview->setParent(nullptr);
    m_selectedEntities.clear();
    m_selectionComplete = false;
    m_finished = true;
    setFinished();
}

bool LC_ActionOffsetCurveRestoreOriginal::currentContext() const {
    // Selection destruction nulls m_document; a live view/context pointer alone cannot vouch for it.
    return m_actionContext && m_document && m_liveView && !m_liveView->isClosing() &&
        m_liveView.data() == m_graphicView && m_actionContext->getDocument() == m_document &&
        m_actionContext->getGraphicView() == m_liveView.data() &&
        m_liveView->getViewPort() == m_viewport && m_liveView->getDocument() == m_document &&
        (m_document->rtti() == RS2::EntityGraphic || m_document->rtti() == RS2::EntityBlock) &&
        m_document->isOwner() && m_document->isVisible() && !m_document->isLocked();
}
RS_Entity* LC_ActionOffsetCurveRestoreOriginal::liveMember(const RS_Entity* address) const {
    if (!address || !currentContext()) return nullptr;
    const int index = m_document->findEntityIndex(address);
    if (index < 0) return nullptr;
    auto* entity = m_document->entityAt(index);
    return entity && entity->getParent() == m_document && !entity->isDeleted() ? entity : nullptr;
}
bool LC_ActionOffsetCurveRestoreOriginal::isEntityAllowedToSelect(RS_Entity* entity) const {
    entity = liveMember(entity);
    return entity && entity->rtti() == RS2::EntityOffsetCurve && dynamic_cast<LC_OffsetCurve*>(entity);
}
void LC_ActionOffsetCurveRestoreOriginal::init(const int status) {
    if (!currentContext()) {
        commandMessage(tr("Restore Original: the editable document or view has changed."));
        return;
    }
    if (mayInitWithContextEntity(status)) {
        const auto* saved = m_actionContext->getContextMenuActionContextEntity();
        if (saved && !liveMember(saved))
            m_actionContext->saveContextMenuActionContext(nullptr, RS_Vector(false), false);
    }
    LC_ActionPreSelectionAwareBase::init(status);
}
void LC_ActionOffsetCurveRestoreOriginal::doInitWithContextEntity(RS_Entity* entity, const RS_Vector&) {
    if (!isEntityAllowedToSelect(entity)) return;
    // Explicit selection, not the base context helper's toggle.
    if (!entity->getFlag(RS2::FlagSelected)) m_document->select(QList<RS_Entity*>{entity}, true);
    m_selectedEntities = {entity};
    onSelectionCompleted(true, true);
}
void LC_ActionOffsetCurveRestoreOriginal::onSelectionCompleted(bool, const bool fromInit) {
    if (!currentContext()) return;
    if (!fromInit) collectSelectedEntities();
    m_targets.clear();
    QSet<RS_Entity*> seen;
    for (auto* address : std::as_const(m_selectedEntities)) {
        auto* entity = liveMember(address);
        if (!isEntityAllowedToSelect(entity) || seen.contains(entity)) continue;
        seen.insert(entity);
        m_targets.append({entity, entity->getId()});
    }
    m_selectionComplete = !m_targets.isEmpty();
    if (m_selectionComplete) trigger();
    else commandMessage(tr("Select offset curves first."));
}
void LC_ActionOffsetCurveRestoreOriginal::trigger() {
    if (!currentContext()) {
        m_report = {};
        for (const auto& target : std::as_const(m_targets)) {
            m_report.outcomes.append({target.id, tr("The editable document or view has changed."), false, false});
            ++m_report.refused;
        }
        publishReport();
        return;
    }
    if (m_targets.isEmpty()) {
        commandMessage(tr("Select offset curves first."));
        return;
    }
    LC_ActionPreSelectionAwareBase::trigger();
}

bool LC_ActionOffsetCurveRestoreOriginal::doTriggerModifications(LC_DocumentModificationBatch& batch) {
    batch.dontSetActiveLayerAndPen();
    m_report = {};
    if (!currentContext()) return false;
    auto* const admittedDocument = m_document;
    const auto admittedEpoch = m_document->undoStateEpoch();
    struct Prepared {
        Target target;
        std::unique_ptr<RS_Ellipse> ellipse;
        Basis basis;
        std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;
        qsizetype outcomeIndex;
        std::optional<AttributeState> attributes;
    };
    std::vector<Prepared> pending;
    std::size_t comparisons = 0;
    for (const auto& target : std::as_const(m_targets)) {
        Outcome outcome;
        outcome.targetId = target.id;
        auto* entity = liveMember(target.address);
        if (entity && entity->getId() != target.id) entity = nullptr;
        auto* curve = entity && entity->rtti() == RS2::EntityOffsetCurve
            ? dynamic_cast<LC_OffsetCurve*>(entity) : nullptr;
        if (!curve) outcome.refusal = tr("Target is no longer a live top-level offset curve.");
        else if (!curve->isVisible()) outcome.refusal = tr("Target is hidden.");
        else if (curve->isLocked()) outcome.refusal = tr("Target is locked.");
        else if (!curve->isDefined()) outcome.refusal = tr("Target has no editable definition.");
        else if (!curve->isConsistent()) outcome.refusal = tr("Target cache no longer matches its definition.");

        std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;
        if (outcome.refusal.isEmpty()) {
            snapshot = LC_OffsetCurveEditSnapshot::capture(*curve);
            if (!snapshot) outcome.refusal = tr("Target state exceeds the bounded snapshot allowance.");
        }
        std::optional<Basis> basis;
        if (outcome.refusal.isEmpty()) {
            basis = normalized(curve->definition().basis);
            if (!basis) outcome.refusal = tr("Embedded source geometry cannot be normalized safely.");
        }
        RS_Layer* layer = nullptr;
        RS_Pen pen;
        if (outcome.refusal.isEmpty()) {
            layer = curve->getLayer(true);
            pen = curve->getPen(true);
            if ((curve->getLayer(false) && !layer) || !pen.isValid())
                outcome.refusal = tr("Target attributes are unavailable.");
        }
        if (!outcome.refusal.isEmpty()) ++m_report.refused;
        else {
            // Fresh detached identities only; no native clone, engine, checker or source handles.
            auto ellipse = std::make_unique<RS_Ellipse>(nullptr, basis->data);
            ellipse->setLayer(layer);
            ellipse->setPen(pen);
            ellipse->clearSelectionFlag();
            pending.push_back({target, std::move(ellipse), *basis, std::move(snapshot),
                m_report.outcomes.size(), std::nullopt});
        }
        m_report.outcomes.append(std::move(outcome));
    }

    // Finish source snapshot validation before any handoff or duplicate suppression.
    // A later source may have changed an earlier source or an ordinary duplicate.
    for (auto& prepared : pending) {
        auto& outcome = m_report.outcomes[prepared.outcomeIndex];
        auto* entity = liveMember(prepared.target.address);
        auto* curve = entity && entity->getId() == prepared.target.id &&
            entity->rtti() == RS2::EntityOffsetCurve ? dynamic_cast<LC_OffsetCurve*>(entity) : nullptr;
        if (!curve || !curve->isVisible() || curve->isLocked() || !curve->isDefined() ||
            !curve->isConsistent() || !prepared.snapshot->matches(*curve)) {
            outcome.refusal = tr("Target changed while its original ellipse was prepared.");
        } else {
            // Effective layer attributes may change without changing the entity's ByLayer pen.
            auto attributes = AttributeState::capture(*curve);
            auto* layer = curve->getLayer(true);
            const auto pen = curve->getPen(true);
            if (!attributes || !attributes->matches(*curve) ||
                (curve->getLayer(false) && !layer) || !pen.isValid())
                outcome.refusal = tr("Target attributes are unavailable.");
            else {
                prepared.ellipse->setLayer(layer);
                prepared.ellipse->setPen(pen);
                prepared.attributes = std::move(attributes);
            }
        }
        if (!outcome.refusal.isEmpty()) {
            ++m_report.refused;
            prepared.ellipse.reset();
        }
    }

    if (std::none_of(pending.begin(), pending.end(), [](const auto& prepared) { return bool(prepared.ellipse); }))
        return false;
    // A caller's retiring originals cannot witness presence in the final container.
    QSet<const RS_Entity*> retiring;
    bool boundedPrefixes = true;
    for (const auto* entity : std::as_const(batch.entitiesToDelete)) {
        if (comparisons >= m_comparisonLimit) { boundedPrefixes = false; break; }
        ++comparisons;
        retiring.insert(entity);
    }
    bool contextStillCurrent = currentContext();
    bool boundedCandidates = boundedPrefixes;
    // Observe candidate callbacks once, before ANY duplicate decision. No
    // iterator or candidate pointer survives a callback; re-read current slots.
    for (std::size_t i = 0; boundedCandidates && contextStillCurrent && i < m_document->size(); ++i) {
        if (comparisons >= m_comparisonLimit || i > std::size_t(std::numeric_limits<int>::max())) {
            boundedCandidates = false;
            break;
        }
        ++comparisons;
        const auto* candidate = m_document->RS_EntityContainer::entityAt(static_cast<int>(i));
        if (candidate && !retiring.contains(candidate) && candidate->getParent() == m_document &&
            !candidate->isDeleted()) (void)candidate->rtti();
        contextStillCurrent = currentContext() && m_document == admittedDocument &&
            m_document->undoStateEpoch() == admittedEpoch;
    }
    for (qsizetype i = 0; boundedCandidates && contextStillCurrent && i < batch.entitiesToAdd.size(); ++i) {
        if (comparisons >= m_comparisonLimit) { boundedCandidates = false; break; }
        ++comparisons;
        const auto* candidate = std::as_const(batch.entitiesToAdd).at(i);
        if (candidate && !candidate->isDeleted()) (void)candidate->rtti();
        contextStillCurrent = currentContext() && m_document == admittedDocument &&
            m_document->undoStateEpoch() == admittedEpoch;
    }
    qsizetype preparedCount = 0;
    for (auto& prepared : pending) {
        if (!prepared.ellipse) continue;
        if (!contextStillCurrent || !boundedCandidates) {
            auto& outcome = m_report.outcomes[prepared.outcomeIndex];
            outcome.refusal = !contextStillCurrent ? tr("The editable document or view has changed.") :
                tr("Bounded duplicate search limit reached.");
            ++m_report.refused;
            prepared.ellipse.reset();
        } else ++preparedCount;
    }
    if (!preparedCount) return false;
    // Preserve caller prefixes and every candidate owner if detach/reserve/append throws.
    QList<RS_Entity*> additions = batch.entitiesToAdd;
    if (preparedCount > std::numeric_limits<qsizetype>::max() - additions.size())
        throw std::length_error("Restore Original batch exceeds native list capacity");
    additions.detach();
    additions.reserve(additions.size() + preparedCount);
    std::vector<LC_OffsetCurveFinalInput::Input> inputs;
    inputs.reserve(pending.size());
    for (const auto& prepared : pending)
        if (m_report.outcomes[prepared.outcomeIndex].refusal.isEmpty())
            inputs.push_back({static_cast<const LC_OffsetCurve*>(prepared.target.address), prepared.snapshot.get(), true, false});
    const auto lateRefusal = tr("Target changed while its original ellipse was prepared.");
    const auto duplicateLimitRefusal = tr("Bounded duplicate search limit reached.");
    const auto duplicateUncertainRefusal = tr("Duplicate geometry comparison is numerically uncertain.");
    const auto attributeRefusal = tr("Target effective attributes changed while its original ellipse was prepared.");
    // Context checks may themselves dispatch virtual document methods. They
    // precede both the stored seal and all raw ordinary geometry comparisons.
    if (currentContext() && m_document == admittedDocument &&
        m_document->undoStateEpoch() == admittedEpoch)
        LC_OffsetCurveFinalInput::validate(*m_document, inputs);
    std::size_t input = 0;
    qsizetype published = 0;
    for (auto& prepared : pending) {
        auto& outcome = m_report.outcomes[prepared.outcomeIndex];
        if (!outcome.refusal.isEmpty()) continue;
        if (!inputs[input++].matches) {
            ++m_report.refused;
            outcome.refusal = lateRefusal;
            prepared.ellipse.reset();
        } else if (!prepared.attributes ||
            !prepared.attributes->matches(*static_cast<const LC_OffsetCurve*>(prepared.target.address))) {
            ++m_report.refused;
            outcome.refusal = attributeRefusal;
            prepared.ellipse.reset();
        }
    }
    // No callback-bearing getters below. All ordinary and private witnesses
    // are decided from their final raw values, never a pre-callback presence bit.
    for (auto& prepared : pending) {
        if (!prepared.ellipse) continue;
        auto& outcome = m_report.outcomes[prepared.outcomeIndex];
        bool present = false;
        for (const auto* candidate : *m_document) {
            if (comparisons >= m_comparisonLimit) { outcome.refusal = duplicateLimitRefusal; break; }
            ++comparisons;
            if (!candidate || retiring.contains(candidate) || candidate->getParent() != m_document ||
                candidate->isDeleted()) continue;
            const auto* ellipse = dynamic_cast<const RS_Ellipse*>(candidate);
            if (!ellipse) continue;
            // Hidden/locked ordinary ellipses remain read-only duplicate evidence.
            const auto other = normalized(ellipse->RS_Ellipse::getData());
            if (!other) continue;
            const auto equality = equivalent(prepared.basis, *other);
            if (!equality) { outcome.refusal = duplicateUncertainRefusal; break; }
            if (*equality) { present = true; break; }
        }
        if (outcome.refusal.isEmpty() && !present) {
            for (const auto* candidate : std::as_const(batch.entitiesToAdd)) {
                if (comparisons >= m_comparisonLimit) { outcome.refusal = duplicateLimitRefusal; break; }
                ++comparisons;
                if (!candidate || candidate->isDeleted()) continue;
                const auto* ellipse = dynamic_cast<const RS_Ellipse*>(candidate);
                if (!ellipse) continue;
                const auto other = normalized(ellipse->RS_Ellipse::getData());
                if (!other) continue;
                const auto equality = equivalent(prepared.basis, *other);
                if (!equality) { outcome.refusal = duplicateUncertainRefusal; break; }
                if (*equality) { present = true; break; }
            }
        }
        if (outcome.refusal.isEmpty() && !present) for (const auto& earlier : pending) {
            if (&earlier == &prepared) break;
            if (!earlier.ellipse) continue;
            if (comparisons >= m_comparisonLimit) {
                outcome.refusal = duplicateLimitRefusal;
                break;
            }
            ++comparisons;
            const auto equality = equivalent(prepared.basis, earlier.basis);
            if (!equality) {
                outcome.refusal = duplicateUncertainRefusal;
                break;
            }
            if (*equality) { present = true; break; }
        }
        if (!outcome.refusal.isEmpty()) {
            ++m_report.refused; prepared.ellipse.reset();
        } else if (present) {
            ++m_report.alreadyPresent; outcome.alreadyPresent = true;
            prepared.ellipse.reset();
        } else {
            ++m_report.restored; outcome.restored = true;
            ++published;
        }
    }
    if (published == 0) return false;
    for (const auto& prepared : pending)
        if (prepared.ellipse) additions.append(prepared.ellipse.get());

    // No validation, allocation or virtual callback below the ownership boundary.
    static_assert(noexcept(batch.entitiesToAdd.swap(additions)));
    static_assert(noexcept(std::declval<std::unique_ptr<RS_Ellipse>&>().release()));
    batch.entitiesToAdd.swap(additions);
    for (auto& prepared : pending) prepared.ellipse.release();
    // No deletion/replacement of any native target, including on partial refusal.
    return true;
}
void LC_ActionOffsetCurveRestoreOriginal::doTriggerSelectionUpdate(
    const bool keepSelected, const LC_DocumentModificationBatch& batch) {
    if (keepSelected && !batch.entitiesToAdd.isEmpty()) select(batch.entitiesToAdd);
}
void LC_ActionOffsetCurveRestoreOriginal::publishReport() {
    QString message = tr("Restore Original: %1 restored, %2 already present, %3 refused.")
        .arg(m_report.restored).arg(m_report.alreadyPresent).arg(m_report.refused);
    QMap<QString, unsigned> reasons;
    for (const auto& outcome : std::as_const(m_report.outcomes))
        if (!outcome.refusal.isEmpty()) ++reasons[outcome.refusal];
    QStringList details;
    for (auto it = reasons.cbegin(); it != reasons.cend(); ++it)
        details.append(tr("%1 (%2)").arg(it.key()).arg(it.value()));
    if (!details.isEmpty()) message += " " + details.join("; ");
    commandMessage(message);
}
void LC_ActionOffsetCurveRestoreOriginal::doTriggerCompletion(bool) {
    publishReport();
    if (currentContext()) finish();
    else { m_finished = true; setFinished(); }
}
void LC_ActionOffsetCurveRestoreOriginal::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select offset curves") + getSelectionCompletionHintMsg(),
        MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Restore after selection")));
}
