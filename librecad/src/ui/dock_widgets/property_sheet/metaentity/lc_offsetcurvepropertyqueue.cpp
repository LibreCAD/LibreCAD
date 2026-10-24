/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurvepropertybatch.h"

#include <QCoreApplication>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "drw_base.h"
#include "lc_actioncontext.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurvereadutils.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_pen.h"

namespace {
QString queueTr(const char* text) {
    return QCoreApplication::translate("LC_OffsetCurveUI", text);
}
bool unlocked(const RS_Entity* entity) {
    const auto* layer = entity->getLayer(true);
    return layer == nullptr || !layer->isLocked();
}

unsigned stableFlags(const RS_Entity& entity) {
    return entity.getFlags() & ~(RS2::FlagsTransient & ~RS2::FlagDeleted);
}
bool sameDouble(const double a, const double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}
bool samePoint(const RS_Vector& a, const RS_Vector& b) {
    return a.valid == b.valid && sameDouble(a.x, b.x) && sameDouble(a.y, b.y) && sameDouble(a.z, b.z);
}
bool sameDefinition(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    return samePoint(a.basis.center, b.basis.center) && samePoint(a.basis.majorP, b.basis.majorP) &&
        sameDouble(a.basis.ratio, b.basis.ratio) && sameDouble(a.basis.angle1, b.basis.angle1) &&
        sameDouble(a.basis.angle2, b.basis.angle2) && a.basis.reversed == b.basis.reversed &&
        a.basis.isArc == b.basis.isArc &&
        sameDouble(a.distance, b.distance) && a.towardCentre == b.towardCentre &&
        sameDouble(a.relTolerance, b.relTolerance) && a.cacheReversed == b.cacheReversed;
}
bool boundedPen(const RS_Pen& pen) {
    return std::size_t(pen.getColor().colorName().size()) <= LC_OffsetCurve::MaximumOwnedStringUnits;
}
bool samePen(const RS_Pen& a, const RS_Pen& b) {
    // Painting equality omits alpha, dash offset and passive naming metadata.
    return a == b && a.getFlags() == b.getFlags() && a.getLineTypeId() == b.getLineTypeId() &&
        sameDouble(a.getScreenWidth(), b.getScreenWidth()) && sameDouble(a.getAlpha(), b.getAlpha()) &&
        sameDouble(a.dashOffset(), b.dashOffset()) && a.getColor().colorName() == b.getColor().colorName();
}
bool boundedSource(const LC_OffsetCurve& curve) {
    const auto& data = curve.getData();
    const auto cap = LC_OffsetCheckLimits{}.maxVertices;
    if (!(data.degree >= 1 && data.degree <= 3 && data.controlPoints.size() <= cap &&
        data.fitPoints.size() <= cap && data.weights.size() <= cap && data.knotslist.size() <= 4*cap &&
        data.savedOpenKnots.size() <= 4*cap && curve.reactorHandles().size() <= cap &&
        curve.getDrwExtData().size() <= cap && boundedPen(curve.getPen(false)) &&
        LC_OffsetCurveRead::cachedLength(curve).has_value())) return false;
    for (const auto* child : curve) if (!boundedPen(child->getPen(false))) return false;
    return true;
}
auto metadata(const RS_Entity& entity) {
    return std::make_tuple(entity.sourceHandle(), entity.lineage(), entity.ownerLineage(),
        entity.xDictHandle(), entity.materialHandle(), entity.plotStyleHandle(), entity.shadowMode(),
        entity.shadowHandle(), entity.fullVisualStyleHandle(), entity.faceVisualStyleHandle(),
        entity.edgeVisualStyleHandle());
}
bool sameTail(const LC_OffsetCurve::OwnedTail& a, const LC_OffsetCurve::OwnedTail& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].code != b[i].code || a[i].value.index() != b[i].value.index()) return false;
        const bool equal = std::visit([&](const auto& left) {
            using T = std::decay_t<decltype(left)>;
            const auto& right = std::get<T>(b[i].value);
            if constexpr (std::is_same_v<T, RS_Vector>) return samePoint(left, right);
            else if constexpr (std::is_same_v<T, double>) return sameDouble(left, right);
            else return left == right;
        }, a[i].value);
        if (!equal) return false;
    }
    return true;
}

bool boundedExtData(const std::vector<std::shared_ptr<DRW_Variant>>& tags) {
    if (tags.size() > LC_OffsetCurvePropertyQueue::MaximumSnapshotItems) return false;
    std::size_t bytes = 0;
    for (const auto& tag : tags) {
        if (!tag) return false;
        std::size_t size = 0;
        switch (tag->type()) {
        case DRW_Variant::STRING:
            if (!tag->content.s) return false;
            size = tag->content.s->size();
            break;
        case DRW_Variant::BINARY:
            if (tag->content.b != tag->binary()) return false;
            size = tag->binary()->size();
            break;
        case DRW_Variant::COORD:
            if (!tag->coord()) return false;
            break;
        case DRW_Variant::INTEGER:
        case DRW_Variant::INTEGER64:
        case DRW_Variant::DOUBLE:
        case DRW_Variant::INVALID:
            break;
        default: return false;
        }
        if (size > LC_OffsetCurvePropertyQueue::MaximumSnapshotBytes - bytes) return false;
        bytes += size;
    }
    return true;
}

struct TagSnapshot {
    int code;
    DRW_Variant::TYPE type;
    bool layerRef;
    std::optional<DRW_Variant::ActiveDwgLayerReference> reference;
    using Payload = std::variant<std::monostate, std::string, std::int32_t, std::int64_t,
        double, DRW_Coord, std::vector<std::uint8_t>>;
    Payload payload;

    explicit TagSnapshot(const DRW_Variant& tag)
        : code(tag.code()), type(tag.type()), layerRef(tag.isLayerRef()),
          reference(tag.activeDwgLayerReference()) {
        // Copy only active payload, not potentially large inactive variant storage.
        switch (type) {
        case DRW_Variant::STRING: payload = *tag.content.s; break;
        case DRW_Variant::INTEGER: payload = tag.i_val(); break;
        case DRW_Variant::INTEGER64: payload = tag.i64_val(); break;
        case DRW_Variant::DOUBLE: payload = tag.d_val(); break;
        case DRW_Variant::COORD: payload = *tag.coord(); break;
        case DRW_Variant::BINARY: payload = *tag.binary(); break;
        case DRW_Variant::INVALID: break;
        }
    }
    bool matches(const DRW_Variant& tag) const {
        const auto active = tag.activeDwgLayerReference();
        if (tag.code() != code || tag.type() != type || tag.isLayerRef() != layerRef ||
            bool(active) != bool(reference) || (active &&
            (active->reference != reference->reference || active->sourceVersion != reference->sourceVersion)))
            return false;
        switch (type) {
        case DRW_Variant::STRING: return std::get<std::string>(payload) == *tag.content.s;
        case DRW_Variant::INTEGER: return std::get<std::int32_t>(payload) == tag.i_val();
        case DRW_Variant::INTEGER64: return std::get<std::int64_t>(payload) == tag.i64_val();
        case DRW_Variant::DOUBLE: return sameDouble(std::get<double>(payload), tag.d_val());
        case DRW_Variant::COORD: {
            const auto& point = std::get<DRW_Coord>(payload);
            return sameDouble(point.x, tag.coord()->x) && sameDouble(point.y, tag.coord()->y) &&
                sameDouble(point.z, tag.coord()->z);
        }
        case DRW_Variant::BINARY: return std::get<std::vector<std::uint8_t>>(payload) == *tag.binary();
        case DRW_Variant::INVALID: return true;
        }
        return false;
    }
};
}

/** Native-only replacement guard, captured before the first accepted edit. */
struct LC_OffsetCurvePropertyQueue::SourceSnapshot {
    unsigned long long id;
    std::uint64_t cache;
    LC_EllipseOffsetDefinition definition;
    bool defined, consistent;
    double tolerance;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned flags, ownedFlags;
    LC_OffsetCurve::OwnedTail tail;
    decltype(metadata(std::declval<const RS_Entity&>())) provenance;
    std::vector<quint32> reactors;
    std::vector<std::shared_ptr<DRW_Variant>> extData;
    std::vector<TagSnapshot> extValues;
    std::vector<std::pair<QString, QString>> variables;
    struct Child {
        unsigned long long id;
        RS_Vector start, end;
        unsigned flags;
        RS_Pen pen;
        RS_Layer* layer;
    };
    std::vector<Child> children;

    static std::unique_ptr<SourceSnapshot> capture(const LC_OffsetCurve& curve) {
        if (!boundedSource(curve) || !boundedExtData(curve.getDrwExtData()) ||
            curve.userDefVarCount() > LC_OffsetCurve::MaximumOwnedTailItems) return {};
        auto keys = curve.getAllKeys();
        if (keys.size() > LC_OffsetCurve::MaximumOwnedTailItems) return {};
        std::size_t units = 0;
        for (const auto& key : keys) {
            const auto value = curve.getUserDefVar(key);
            const auto size = std::size_t(key.size()) + std::size_t(value.size());
            if (size > LC_OffsetCurve::MaximumOwnedTailLogicalBytes/2 - units) return {};
            units += size;
        }
        return std::unique_ptr<SourceSnapshot>(new SourceSnapshot(curve, keys));
    }
    SourceSnapshot(const LC_OffsetCurve& curve, const std::vector<QString>& keys)
        : id(curve.getId()), cache(RS_Spline::geometryFingerprint(curve.getData())),
          definition(curve.definition()), defined(curve.isDefined()), consistent(curve.isConsistent()),
          tolerance(curve.checkedWorldTolerance()), layer(curve.getLayer(false)), pen(curve.getPen(false)),
          flags(stableFlags(curve)), ownedFlags(curve.unknownOwnedFlags()),
          tail(curve.unknownOwnedTail()), provenance(metadata(curve)), reactors(curve.reactorHandles()),
          extData(curve.getDrwExtData()) {
        extValues.reserve(extData.size());
        for (const auto& tag : extData) extValues.emplace_back(*tag);
        for (const auto& key : keys) variables.emplace_back(key, curve.getUserDefVar(key));
        for (const auto* child : curve)
            children.push_back({child->getId(), child->getStartpoint(), child->getEndpoint(),
                stableFlags(*child), child->getPen(false), child->getLayer(false)});
    }
    bool matches(const LC_OffsetCurve& curve) const {
        if (curve.getId() != id || !boundedSource(curve) || !boundedExtData(curve.getDrwExtData()) ||
            RS_Spline::geometryFingerprint(curve.getData()) != cache ||
            !sameDefinition(curve.definition(), definition) || curve.isDefined() != defined ||
            curve.isConsistent() != consistent || !sameDouble(curve.checkedWorldTolerance(), tolerance) ||
            curve.getLayer(false) != layer || !samePen(curve.getPen(false), pen) ||
            stableFlags(curve) != flags || curve.unknownOwnedFlags() != ownedFlags ||
            !sameTail(curve.unknownOwnedTail(), tail) || metadata(curve) != provenance ||
            curve.reactorHandles() != reactors || curve.getDrwExtData() != extData ||
            std::size_t(curve.count()) != children.size() || curve.userDefVarCount() != variables.size()) return false;
        for (std::size_t item = 0; item < extValues.size(); ++item)
            if (!extValues[item].matches(*curve.getDrwExtData()[item])) return false;
        const auto keys = curve.getAllKeys();
        if (keys.size() != variables.size()) return false;
        for (std::size_t i = 0; i < keys.size(); ++i)
            if (keys[i] != variables[i].first || curve.getUserDefVar(keys[i]) != variables[i].second) return false;
        std::size_t i = 0;
        for (const auto* child : curve) {
            const auto& before = children[i++];
            if (child->getId() != before.id || !samePoint(child->getStartpoint(), before.start) ||
                !samePoint(child->getEndpoint(), before.end) || stableFlags(*child) != before.flags ||
                !samePen(child->getPen(false), before.pen) || child->getLayer(false) != before.layer) return false;
        }
        return true;
    }
};

LC_OffsetCurvePropertyQueue::LC_OffsetCurvePropertyQueue(QObject* parent, LC_ActionContext* context)
    : QObject(parent), m_context(context), m_limits(LC_OffsetCurveUI::sheetLimits()) {
}
LC_OffsetCurvePropertyQueue::~LC_OffsetCurvePropertyQueue() = default;

void LC_OffsetCurvePropertyQueue::invalidateGetters() { ++m_epoch; }

void LC_OffsetCurvePropertyQueue::cancel() {
    invalidateGetters();
    ++m_contextSerial;
    m_scheduled = false;
    m_pending.clear(); // originals may already be dead; never dereference them here
}
void LC_OffsetCurvePropertyQueue::setContext(RS_Document* document, RS_GraphicView* view) {
    if (m_document == document && m_view.data() == view) return;
    cancel();
    m_document = document;
    m_view = view;
}
bool LC_OffsetCurvePropertyQueue::currentContext() const {
    return m_context && m_document && m_view &&
        m_context->getDocument() == m_document && m_context->getGraphicView() == m_view.data() &&
        m_view->getDocument() == m_document &&
        (m_document->rtti() == RS2::EntityGraphic || m_document->rtti() == RS2::EntityBlock);
}
bool LC_OffsetCurvePropertyQueue::isLive(const RS_Entity* original) const {
    // Membership/owner precede dereferencing a pointer retained across an event turn.
    return currentContext() && original && m_document->findEntityIndex(original) >= 0 &&
        original->getParent() == m_document && !original->isDeleted() &&
        original->rtti() == RS2::EntityOffsetCurve && dynamic_cast<const LC_OffsetCurve*>(original);
}
const LC_OffsetCurve* LC_OffsetCurvePropertyQueue::value(Epoch epoch, const RS_Entity* original) const {
    if (epoch != m_epoch || !isLive(original)) return nullptr;
    const auto* curve = static_cast<const LC_OffsetCurve*>(original);
    const auto found = m_pending.find(original);
    if (found == m_pending.end()) return curve;
    return found->second.source->matches(*curve) ? found->second.candidate.get() : nullptr;
}
void LC_OffsetCurvePropertyQueue::apply(Epoch epoch, const QList<RS_Entity*>& originals, const Edit& edit) {
    m_lastAttempts.clear();
    if (!edit) return;
    bool cancelled = false;
    {
        const LC_BusyCursorGuard busy;
        for (auto* original : originals) {
            Attempt attempt;
            attempt.original = original;
            auto found = m_pending.find(original);
            const LC_OffsetCurve* current = nullptr;
            std::unique_ptr<SourceSnapshot> snapshot;
            if (epoch != m_epoch || !isLive(original)) {
                attempt.contextRefusal = queueTr("the property editing session is no longer current");
            }
            else if (!unlocked(original) || m_document->isLocked()) {
                attempt.contextRefusal = queueTr("the source or editable container is locked");
            }
            else if (found != m_pending.end() &&
                     !found->second.source->matches(*static_cast<const LC_OffsetCurve*>(original))) {
                m_pending.erase(found);
                cancelled = true;
                attempt.contextRefusal = queueTr("the source changed after this edit was staged; the pending edit was cancelled");
            }
            else {
                current = found == m_pending.end() ? static_cast<const LC_OffsetCurve*>(original) :
                    found->second.candidate.get();
                if (found == m_pending.end()) snapshot = SourceSnapshot::capture(*current);
                if (found == m_pending.end() && !snapshot)
                    attempt.contextRefusal = queueTr("the source cannot be snapshotted within the bounded native read limits");
            }
            if (current && attempt.contextRefusal.isEmpty()) {
                auto candidate = std::unique_ptr<LC_OffsetCurve>(
                    static_cast<LC_OffsetCurve*>(current->clone()));
                attempt.editInvoked = true;
                attempt.result = edit(*candidate, m_limits);
                if (attempt.result && attempt.result.changed) {
                    if (found == m_pending.end())
                        m_pending.emplace(original, Pending{std::move(candidate), std::move(snapshot)});
                    else found->second.candidate = std::move(candidate);
                }
            }
            m_lastAttempts.push_back(std::move(attempt));
        }
    }
    report();
    if (cancelled) emit pendingCancelled();
    if (!m_pending.empty()) scheduleCommit();
}

void LC_OffsetCurvePropertyQueue::report() const {
    if (m_context == nullptr || m_lastAttempts.empty()) return;
    int changed = 0, unchanged = 0, refused = 0;
    QMap<QString, int> reasons;
    for (const auto& attempt : m_lastAttempts) {
        if (attempt.editInvoked && attempt.result) {
            if (attempt.result.changed) ++changed;
            else ++unchanged;
        }
        else {
            ++refused;
            const auto text = attempt.contextRefusal.isEmpty()
                ? LC_OffsetCurveUI::refusalText(attempt.result) : attempt.contextRefusal;
            ++reasons[text];
        }
    }
    QStringList detail;
    for (auto it = reasons.cbegin(); it != reasons.cend(); ++it) {
        detail.append(queueTr("%1: %2").arg(it.value()).arg(it.key()));
    }
    auto summary = queueTr("Offset Curve: %1 changed, %2 unchanged, %3 refused.")
        .arg(changed).arg(unchanged).arg(refused);
    if (!detail.isEmpty()) summary += " " + detail.join(queueTr("; "));
    m_context->commandMessage(summary);
}

void LC_OffsetCurvePropertyQueue::scheduleCommit() {
    if (m_scheduled) return;
    m_scheduled = true;
    const auto serial = m_contextSerial;
    QTimer::singleShot(30, this, [this, serial] {
        if (serial == m_contextSerial) commit(serial);
    });
}
void LC_OffsetCurvePropertyQueue::commit(Epoch contextSerial) {
    if (contextSerial != m_contextSerial) return;
    m_scheduled = false;
    const auto reportFailure = [this](const char* message) noexcept {
        try { if (m_context) m_context->commandMessage(queueTr(message)); }
        catch (const std::bad_alloc&) {} // Reporting must not escape the timer on OOM.
    };
    std::unique_ptr<LC_OffsetCurvePropertyBatch> prepared;
    try {
        if (!currentContext()) { cancel(); return; }
        bool cancelled = false;
        for (auto it = m_pending.begin(); it != m_pending.end();) {
            QString reason;
            if (!isLive(it->first)) reason = queueTr("the source is no longer live in its editable container");
            else if (!unlocked(it->first) || m_document->isLocked()) reason = queueTr("the source or editable container is locked");
            else if (!it->second.source->matches(*static_cast<const LC_OffsetCurve*>(it->first)))
                reason = queueTr("the source changed after the edit was staged");
            if (!reason.isEmpty()) {
                m_context->commandMessage(queueTr("Offset Curve: pending edit of entity %1 cancelled: %2.")
                    .arg(it->second.source->id).arg(reason));
                it = m_pending.erase(it);
                cancelled = true;
            } else ++it;
        }
        if (cancelled) emit pendingCancelled();
        if (m_pending.empty()) return;
        prepared = LC_OffsetCurvePropertyBatch::tryPrepare(m_pending, m_epoch);
    } catch (const std::bad_alloc&) {
        reportFailure("Offset Curve: allocation failed before publication; accepted edits remain pending.");
        return;
    }
    if (!prepared) {
        reportFailure("Offset Curve: batch preparation failed before publication; accepted edits remain pending.");
        return;
    }
    // std::map moves preserve the candidate-owner addresses stored by prepared.
    static_assert(std::is_nothrow_move_constructible_v<decltype(m_pending)>);
    auto pending = std::move(m_pending);
    m_pending.clear();
    const auto outcome = prepared->execute(*m_document, m_view->getViewPort());
    if (outcome == LC_OffsetCurvePropertyBatch::Outcome::AllocationBeforeHandoff) {
        static_assert(noexcept(m_pending.swap(pending)));
        m_pending.swap(pending);
        reportFailure("Offset Curve: allocation failed before publication; accepted edits remain pending.");
    } else if (outcome == LC_OffsetCurvePropertyBatch::Outcome::AllocationAfterHandoff) {
        reportFailure("Offset Curve: allocation failed after handoff; generic undo publication is not allocation-atomic.");
    }
}
