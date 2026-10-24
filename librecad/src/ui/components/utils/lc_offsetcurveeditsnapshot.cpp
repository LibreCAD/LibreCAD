/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_offsetcurveeditsnapshot.h"

#include <algorithm>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "drw_base.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurvereadutils.h"
#include "rs_pen.h"

namespace {
unsigned stableFlags(const RS_Entity& entity) {
    return entity.getFlags() & ~(RS2::FlagsTransient & ~RS2::FlagDeleted);
}
bool sameDouble(const double a, const double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}
bool samePoint(const RS_Vector& a, const RS_Vector& b) {
    return a.valid == b.valid && sameDouble(a.x, b.x) && sameDouble(a.y, b.y) && sameDouble(a.z, b.z);
}
bool sameNumbers(const std::vector<double>& a, const std::vector<double>& b) {
    return a.size() == b.size() && (a.empty() ||
        std::memcmp(a.data(), b.data(), a.size()*sizeof(double)) == 0);
}
bool sameCache(const RS_SplineData& a, const RS_SplineData& b) {
    const auto points = [](const auto& x, const auto& y) {
        return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), samePoint);
    };
    return a.degree == b.degree && a.type == b.type && a.savedOpenType == b.savedOpenType &&
        a.m_closedFlag == b.m_closedFlag && a.m_periodicFlag == b.m_periodicFlag &&
        points(a.controlPoints, b.controlPoints) && points(a.fitPoints, b.fitPoints) &&
        sameNumbers(a.knotslist, b.knotslist) && sameNumbers(a.weights, b.weights) &&
        sameNumbers(a.savedOpenKnots, b.savedOpenKnots);
}
bool sameDefinition(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    return samePoint(a.basis.center, b.basis.center) && samePoint(a.basis.majorP, b.basis.majorP) &&
        sameDouble(a.basis.ratio, b.basis.ratio) && sameDouble(a.basis.angle1, b.basis.angle1) &&
        sameDouble(a.basis.angle2, b.basis.angle2) && a.basis.reversed == b.basis.reversed &&
        a.basis.isArc == b.basis.isArc &&
        sameDouble(a.basis.angleDegrees, b.basis.angleDegrees) &&
        sameDouble(a.basis.startAngleDegrees, b.basis.startAngleDegrees) &&
        sameDouble(a.basis.otherAngleDegrees, b.basis.otherAngleDegrees) &&
        sameDouble(a.basis.angularLength, b.basis.angularLength) &&
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
    if (tags.size() > LC_OffsetCurveEditSnapshot::MaximumSnapshotItems) return false;
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
        if (size > LC_OffsetCurveEditSnapshot::MaximumSnapshotBytes - bytes) return false;
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
struct LC_OffsetCurveEditSnapshot::Data {
    unsigned long long id;
    RS_SplineData cache;
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

    static std::unique_ptr<Data> capture(const LC_OffsetCurve& curve) {
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
        return std::unique_ptr<Data>(new Data(curve, keys));
    }
    Data(const LC_OffsetCurve& curve, const std::vector<QString>& keys)
        : id(curve.getId()), cache(curve.getData()),
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
            !sameCache(curve.getData(), cache) ||
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

LC_OffsetCurveEditSnapshot::LC_OffsetCurveEditSnapshot(std::unique_ptr<Data> data) noexcept
    : m_data(std::move(data)) {}
LC_OffsetCurveEditSnapshot::~LC_OffsetCurveEditSnapshot() = default;

std::unique_ptr<LC_OffsetCurveEditSnapshot> LC_OffsetCurveEditSnapshot::capture(const LC_OffsetCurve& curve) {
    auto data = Data::capture(curve); // All count/active-payload/key bounds precede copying or cloning.
    if (!data) return {};
    return std::unique_ptr<LC_OffsetCurveEditSnapshot>(new LC_OffsetCurveEditSnapshot(std::move(data)));
}
bool LC_OffsetCurveEditSnapshot::matches(const LC_OffsetCurve& curve) const {
    return m_data->matches(curve);
}
