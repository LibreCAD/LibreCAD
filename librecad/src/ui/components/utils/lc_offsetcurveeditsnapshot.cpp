/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_offsetcurveeditsnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <map>
#include <set>
#include <stdexcept>
#include <typeinfo>
#include "drw_base.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurvereadutils.h"
#include "rs_pen.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_polyline.h"
#include "lc_parabola.h"
#include "lc_hyperbola.h"
#include "rs_graphic.h"
#include "rs_layer.h"

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
bool boundedStoredSource(const LC_OffsetCurve& curve) {
    const auto& data = curve.getData();
    const auto cap = LC_OffsetCheckLimits{}.maxVertices;
    if (!(data.degree >= 1 && data.degree <= 3 && data.controlPoints.size() <= cap &&
        data.fitPoints.size() <= cap && data.weights.size() <= cap && data.knotslist.size() <= 4*cap &&
        data.savedOpenKnots.size() <= 4*cap && curve.reactorHandles().size() <= cap &&
        curve.getDrwExtData().size() <= cap && boundedPen(curve.getPen(false)) &&
        curve.count() > 0 && curve.count() <= cap)) return false;
    for (const auto* child : curve) {
        const auto* line = dynamic_cast<const RS_Line*>(child);
        if (!line || line->getParent() != &curve || !boundedPen(line->getPen(false))) return false;
        const auto stored = line->RS_Line::getData();
        for (const auto& point : {stored.startpoint, stored.endpoint})
            if (!point.valid || !std::isfinite(point.x) || !std::isfinite(point.y) || point.z != 0)
                return false;
    }
    return true;
}
bool boundedSource(const LC_OffsetCurve& curve) {
    return boundedStoredSource(curve) && LC_OffsetCurveRead::cachedLength(curve).has_value() &&
        boundedStoredSource(curve);
}
std::optional<RS_LineData> storedLine(const RS_Entity* child) {
    const auto* line = dynamic_cast<const RS_Line*>(child);
    if (!line) return std::nullopt;
    return line->RS_Line::getData();
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
    RS_EntityContainer* parent;
    bool owner;
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
        std::optional<RS_LineData> stored;
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
        auto result = std::unique_ptr<Data>(new Data(curve, keys));
        // Child reads during capture must not admit a newly changed/oversized root to cloning.
        if (!result->matchesStoredState(curve)) return {};
        return result;
    }
    Data(const LC_OffsetCurve& curve, const std::vector<QString>& keys)
        : id(curve.getId()), cache(curve.getData()),
          definition(curve.definition()), defined(curve.isDefined()), consistent(curve.isConsistent()),
          tolerance(curve.checkedWorldTolerance()), parent(curve.getParent()), owner(curve.isOwner()),
          layer(curve.getLayer(false)), pen(curve.getPen(false)),
          flags(stableFlags(curve)), ownedFlags(curve.unknownOwnedFlags()),
          tail(curve.unknownOwnedTail()), provenance(metadata(curve)), reactors(curve.reactorHandles()),
          extData(curve.getDrwExtData()) {
        extValues.reserve(extData.size());
        for (const auto& tag : extData) extValues.emplace_back(*tag);
        for (const auto& key : keys) variables.emplace_back(key, curve.getUserDefVar(key));
        for (const auto* child : curve)
            children.push_back({child->getId(), child->getStartpoint(), child->getEndpoint(),
                stableFlags(*child), child->getPen(false), child->getLayer(false), storedLine(child)});
    }
    bool matchesStoredState(const LC_OffsetCurve& curve) const {
        if (curve.getId() != id || !boundedStoredSource(curve) || !boundedExtData(curve.getDrwExtData()) ||
            !sameCache(curve.getData(), cache) ||
            !sameDefinition(curve.definition(), definition) || curve.isDefined() != defined ||
            curve.isConsistent() != consistent || !sameDouble(curve.checkedWorldTolerance(), tolerance) ||
            curve.getParent() != parent || curve.isOwner() != owner ||
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
            const auto stored = storedLine(child);
            if (!stored || !before.stored || child->getId() != before.id ||
                !samePoint(stored->startpoint, before.stored->startpoint) ||
                !samePoint(stored->endpoint, before.stored->endpoint) ||
                stableFlags(*child) != before.flags || !samePen(child->getPen(false), before.pen) ||
                child->getLayer(false) != before.layer) return false;
        }
        return true;
    }
    bool matches(const LC_OffsetCurve& curve) const {
        if (!matchesStoredState(curve) || !boundedSource(curve)) return false;
        std::size_t i = 0;
        for (const auto* child : curve) {
            const auto& before = children[i++];
            if (child->getId() != before.id || !samePoint(child->getStartpoint(), before.start) ||
                !samePoint(child->getEndpoint(), before.end) || stableFlags(*child) != before.flags ||
                !samePen(child->getPen(false), before.pen) || child->getLayer(false) != before.layer) return false;
        }
        // The last successful virtual getter may have changed root or peer data.
        return matchesStoredState(curve);
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
bool LC_OffsetCurveEditSnapshot::matchesStoredState(const LC_OffsetCurve& curve) const {
    return m_data->matchesStoredState(curve);
}

namespace {
// Exact stored values, including dormant spline arrays/rendering fields. No
// virtual geometry reads or normalization may change a creation source seal.
unsigned creationFlags(const RS_Entity& entity) {
    return stableFlags(entity) | (entity.getFlags() & RS2::FlagSelected);
}
std::optional<std::vector<std::uint64_t>> creationGeometry(const RS_Entity& entity) {
    std::vector<std::uint64_t> key;
    const auto number = [&](double value) { std::uint64_t bits; std::memcpy(&bits, &value, sizeof(bits)); key.push_back(bits); };
    const auto point = [&](const RS_Vector& p) { number(p.x); number(p.y); number(p.z); key.push_back(p.valid); };
    const auto points = [&](const auto& values) {
        if (values.size() > LC_OffsetCreationSources::MaximumNodes) return false;
        key.push_back(values.size()); for (const auto& p : values) point(p); return true;
    };
    const auto numbers = [&](const auto& values) {
        if (values.size() > 4*LC_OffsetCreationSources::MaximumNodes) return false;
        key.push_back(values.size()); for (const auto value : values) number(value); return true;
    };
    if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&entity)) {
        key.push_back(0); const auto& d = native->getData();
        if (!points(d.controlPoints) || !points(d.fitPoints) || !numbers(d.knotslist) ||
            !numbers(d.weights) || !numbers(d.savedOpenKnots)) return {};
        key.insert(key.end(), {std::uint64_t(d.degree), std::uint64_t(d.type), std::uint64_t(d.savedOpenType),
            d.m_closedFlag, d.m_periodicFlag});
        const auto& def = native->definition(); point(def.basis.center); point(def.basis.majorP);
        for (auto value : {def.basis.ratio, def.basis.angle1, def.basis.angle2, def.basis.angleDegrees,
            def.basis.startAngleDegrees, def.basis.otherAngleDegrees, def.basis.angularLength,
            def.distance, def.relTolerance, native->checkedWorldTolerance()}) number(value);
        key.insert(key.end(), {def.basis.reversed, def.basis.isArc, def.towardCentre, def.cacheReversed,
            native->isDefined(), native->isConsistent(), native->unknownOwnedFlags()});
        if (native->unknownOwnedTail().size() > LC_OffsetCurve::MaximumOwnedTailItems) return {};
        // Tail values are sealed by the existing native snapshot below.
    } else if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
        key.push_back(1); const auto& d = spline->RS_Spline::getData();
        if (!points(d.controlPoints) || !points(d.fitPoints) || !numbers(d.knotslist) ||
            !numbers(d.weights) || !numbers(d.savedOpenKnots)) return {};
        key.insert(key.end(), {std::uint64_t(d.degree), std::uint64_t(d.type), std::uint64_t(d.savedOpenType),
            d.m_closedFlag, d.m_periodicFlag});
    } else if (const auto* parabola = dynamic_cast<const LC_Parabola*>(&entity)) {
        key.push_back(2); const auto& d = parabola->LC_Parabola::getData();
        for (const auto& p : d.m_controlPoints) point(p);
        point(d.m_focus); point(d.m_axis); point(d.m_vertex); key.push_back(d.m_valid);
        const auto& base = parabola->LC_SplinePoints::getData();
        if (!points(base.controlPoints) || !points(base.splinePoints)) return {};
        key.insert(key.end(), {base.closed, base.cut, base.useControlPoints});
    } else if (const auto* spline = dynamic_cast<const LC_SplinePoints*>(&entity)) {
        key.push_back(3); const auto& d = spline->LC_SplinePoints::getData();
        if (!points(d.controlPoints) || !points(d.splinePoints)) return {};
        key.insert(key.end(), {d.closed, d.cut, d.useControlPoints});
    } else if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity)) {
        key.push_back(4); const auto& d = ellipse->RS_Ellipse::getData();
        point(d.center); point(d.majorP);
        for (const auto value : {d.ratio, d.angle1, d.angle2, d.angleDegrees, d.startAngleDegrees,
            d.otherAngleDegrees, d.angularLength}) number(value);
        key.insert(key.end(), {d.reversed, d.isArc});
    } else if (const auto* hyperbola = dynamic_cast<const LC_Hyperbola*>(&entity)) {
        key.push_back(5); const auto& d = hyperbola->LC_Hyperbola::getData();
        point(d.center); point(d.majorP); number(d.ratio); number(d.angle1); number(d.angle2);
        key.insert(key.end(), {d.reversed, hyperbola->isValid()});
    } else if (const auto* line = dynamic_cast<const RS_Line*>(&entity)) {
        key.push_back(6); const auto d = line->RS_Line::getData(); point(d.startpoint); point(d.endpoint);
    } else if (const auto* arc = dynamic_cast<const RS_Arc*>(&entity)) {
        key.push_back(7); const auto& d = arc->RS_Arc::getData(); point(d.center);
        for (const auto value : {d.radius, d.angle1, d.angle2, d.startAngleDegrees, d.otherAngleDegrees, d.angularLength}) number(value);
        key.push_back(d.reversed);
    } else if (const auto* circle = dynamic_cast<const RS_Circle*>(&entity)) {
        key.push_back(8); const auto& d = circle->RS_Circle::getData(); point(d.center); number(d.radius);
    } else if (const auto* polyline = dynamic_cast<const RS_Polyline*>(&entity)) {
        key.push_back(9); const auto d = polyline->RS_Polyline::getData();
        point(d.startpoint); point(d.endpoint); key.push_back(d.getFlags());
    } else return {};
    return key;
}

std::unique_ptr<RS_Entity> creationClone(const RS_Entity& entity) {
    if (const auto* e = dynamic_cast<const LC_OffsetCurve*>(&entity)) return std::unique_ptr<RS_Entity>(e->LC_OffsetCurve::clone());
    if (const auto* e = dynamic_cast<const RS_Spline*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Spline::clone());
    if (const auto* e = dynamic_cast<const LC_Parabola*>(&entity)) return std::unique_ptr<RS_Entity>(e->LC_Parabola::clone());
    if (const auto* e = dynamic_cast<const LC_SplinePoints*>(&entity)) return std::unique_ptr<RS_Entity>(e->LC_SplinePoints::clone());
    if (const auto* e = dynamic_cast<const RS_Ellipse*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Ellipse::clone());
    if (const auto* e = dynamic_cast<const LC_Hyperbola*>(&entity)) return std::unique_ptr<RS_Entity>(e->LC_Hyperbola::clone());
    if (const auto* e = dynamic_cast<const RS_Line*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Line::clone());
    if (const auto* e = dynamic_cast<const RS_Arc*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Arc::clone());
    if (const auto* e = dynamic_cast<const RS_Circle*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Circle::clone());
    if (const auto* e = dynamic_cast<const RS_Polyline*>(&entity)) return std::unique_ptr<RS_Entity>(e->RS_Polyline::clone());
    return {};
}
}

struct LC_OffsetCreationSources::Data {
    struct Node {
        RS_Entity* address;
        unsigned long long id;
        RS_EntityContainer* parent;
        RS_Layer* layer;
        RS_Pen pen;
        unsigned flags;
        RS_Vector minimum, maximum;
        decltype(metadata(std::declval<const RS_Entity&>())) provenance;
        std::vector<quint32> reactors;
        std::vector<std::shared_ptr<DRW_Variant>> tags;
        std::vector<TagSnapshot> values;
        std::vector<std::pair<QString, QString>> variables;
        std::vector<std::uint64_t> geometry;
        QList<RS_Entity*> children;
        std::unique_ptr<LC_OffsetCurveEditSnapshot> native;
        bool owner = false;
    };
    struct Layer { RS_Layer* live; RS_Layer* frozen; QString name; RS_Pen pen; bool locked, hidden; };
    RS_Document* document;
    RS_Graphic* graphic = nullptr;
    RS_Layer* activeLayer = nullptr;
    RS_Layer* documentLayer = nullptr;
    // Destroy frozen entities before their privately owned layer registry.
    RS_Graphic frozen;
    QList<RS_Entity*> topLevel, roots, sources, originals;
    std::vector<Node> nodes;
    std::vector<Layer> layers;
    std::map<const RS_Entity*, RS_Entity*> addresses;
    std::map<const RS_Entity*, RS_Entity*> reverse;
    std::set<const RS_Entity*> seen;
    bool worker = false;
    std::size_t bytes = 0;
    explicit Data(RS_Document& doc) : document(&doc) {
        const RS_Entity* ancestor = &doc;
        for (unsigned i = 0; ancestor && i < 64; ++i, ancestor = ancestor->getParent())
            if ((graphic = const_cast<RS_Graphic*>(dynamic_cast<const RS_Graphic*>(ancestor)))) break;
        activeLayer = graphic ? graphic->getActiveLayer() : nullptr; documentLayer = doc.getLayer(false);
        for (auto* root : doc) topLevel.append(root);
    }
    bool charge(std::size_t count, std::size_t width = 1) {
        if (count > (MaximumBytes - bytes)/width) return false;
        bytes += count*width; return true;
    }
    RS_Layer* layer(RS_Layer* actual) {
        if (!actual) return nullptr;
        for (const auto& value : layers) if (value.live == actual) return value.frozen;
        if (!graphic || !graphic->RS_Graphic::getLayerList()->contains(actual) ||
            !charge(std::size_t(actual->getName().size()), 2) || !boundedPen(actual->getPen()))
            throw std::runtime_error("Unregistered or oversized offset source layer");
        auto owned = std::make_unique<RS_Layer>(QStringLiteral("offset-input-%1").arg(layers.size()));
        owned->setPen(actual->getPen()); owned->lock(actual->isLocked()); owned->freeze(actual->isFrozen());
        auto* result = owned.get(); frozen.addLayer(result); owned.release();
        layers.push_back({actual, result, actual->getName(), actual->getPen(), actual->isLocked(), actual->isFrozen()});
        return result;
    }
    bool captureNode(RS_Entity& entity, unsigned depth) {
        if (nodes.size() == MaximumNodes || depth > 64 || !charge(1024) ||
            !seen.insert(&entity).second ||
            !boundedExtData(entity.getDrwExtData()) || !boundedPen(entity.getPen(false)) ||
            entity.reactorHandles().size() > MaximumNodes || entity.userDefVarCount() > MaximumNodes) return false;
        auto geometry = creationGeometry(entity); if (!geometry || !charge(geometry->size(), 3*sizeof(std::uint64_t))) return false;
        Node node{&entity, entity.getId(), entity.getParent(), entity.getLayer(false), entity.getPen(false),
            creationFlags(entity), entity.getMin(), entity.getMax(), metadata(entity), entity.reactorHandles(),
            entity.getDrwExtData(), {}, {}, std::move(*geometry), {}, {}, false};
        layer(node.layer);
        if (!charge(node.reactors.size(), 3*sizeof(quint32)) ||
            !charge(std::size_t(node.pen.getColor().colorName().size()), 6)) return false;
        if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
            // Copy constructors retain latent arc-length tables too. Do not
            // rebuild/evaluate them just to admit a bounded owned input.
            if (!charge(spline->m_arcLength.t.size(), 3*sizeof(double)) ||
                !charge(spline->m_arcLength.length.size(), 3*sizeof(double))) return false;
        }
        for (const auto& tag : node.tags) {
            std::size_t size = sizeof(TagSnapshot);
            if (tag->type() == DRW_Variant::STRING) size += tag->content.s->size();
            if (tag->type() == DRW_Variant::BINARY) size += tag->binary()->size();
            if (!charge(size, 3)) return false;
            node.values.emplace_back(*tag);
        }
        for (const auto& key : entity.getAllKeys()) {
            auto value = entity.getUserDefVar(key);
            if (!charge(std::size_t(key.size()) + std::size_t(value.size()), 6)) return false;
            node.variables.emplace_back(key, std::move(value));
        }
        if (auto* container = dynamic_cast<RS_EntityContainer*>(&entity)) {
            if (!container->isOwner() || container->size() > MaximumNodes - nodes.size() ||
                !charge(container->size(), 3*sizeof(RS_Entity*))) return false;
            node.owner = true;
            for (auto* child : *container) {
                // Qualified root clones still invoke child clone(). Admit only
                // built-in implementations, with every descendant preflighted.
                if (!child || child->getParent() != container ||
                    (typeid(*child) != typeid(RS_Line) && typeid(*child) != typeid(RS_Arc) &&
                     typeid(*child) != typeid(RS_Ellipse) && typeid(*child) != typeid(RS_Circle) &&
                     typeid(*child) != typeid(RS_Polyline) && typeid(*child) != typeid(RS_Spline) &&
                     typeid(*child) != typeid(LC_OffsetCurve) && typeid(*child) != typeid(LC_SplinePoints) &&
                     typeid(*child) != typeid(LC_Parabola) && typeid(*child) != typeid(LC_Hyperbola))) return false;
                node.children.append(child);
            }
        }
        const auto children = node.children;
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&entity)) {
            if (!charge(native->unknownOwnedTail().size(), 3*sizeof(LC_OffsetCurveExtension)) ||
                !charge(256, 3*sizeof(RS_Vector))) return false; // Existing native grip hard cap.
            for (const auto& item : native->unknownOwnedTail()) if (const auto* text = std::get_if<QString>(&item.value))
                if (!charge(std::size_t(text->size()), 6)) return false;
            node.native = LC_OffsetCurveEditSnapshot::capture(*native); if (!node.native) return false;
        }
        nodes.push_back(std::move(node));
        for (auto* child : children) if (!captureNode(*child, depth + 1)) return false;
        return true;
    }
    void bind(RS_Entity& actual, RS_Entity& copy) {
        addresses.emplace(&actual, &copy); reverse.emplace(&copy, &actual);
        copy.RS_Entity::setLayer(layer(actual.getLayer(false)));
        copy.setFlags(actual.getFlags() & ~RS2::FlagHighlighted);
        auto* a = dynamic_cast<RS_EntityContainer*>(&actual); auto* b = dynamic_cast<RS_EntityContainer*>(&copy);
        if (!a || !b) return;
        if (a->count() != b->count()) throw std::runtime_error("Offset source clone membership changed");
        for (int i = 0; i < a->count(); ++i) bind(*a->entityAt(i), *b->entityAt(i));
    }
};

LC_OffsetCreationSources::LC_OffsetCreationSources(std::unique_ptr<Data> data) : m_data(std::move(data)) {}
LC_OffsetCreationSources::~LC_OffsetCreationSources() = default;

std::shared_ptr<LC_OffsetCreationSources> LC_OffsetCreationSources::capture(RS_Document& document,
    const QList<RS_Entity*>& selection) {
    if (selection.isEmpty() || selection.size() > 4096 || !document.isOwner() || document.size() > 262144) return {};
    auto d = std::make_unique<Data>(document); if (!d->graphic) return {};
    std::set<RS_Entity*> missing(selection.begin(), selection.end());
    std::map<RS_Entity*, RS_Entity*> rootOf;
    for (auto* root : d->topLevel) if (missing.erase(root)) rootOf.emplace(root, root);
    std::size_t searchedChildren = 0;
    for (auto* candidate : d->topLevel) {
        if (missing.empty()) break;
        const auto* container = dynamic_cast<const RS_EntityContainer*>(candidate);
        if (!container || (!dynamic_cast<const RS_Polyline*>(candidate) && !dynamic_cast<const LC_OffsetCurve*>(candidate))) continue;
        if (container->size() > MaximumNodes - searchedChildren) return {};
        searchedChildren += container->size();
        for (auto* child : *container) if (missing.erase(child)) rootOf.emplace(child, candidate);
    }
    if (!missing.empty()) return {};
    std::set<RS_Entity*> chosen;
    for (auto* address : selection) {
        if (!address) return {};
        auto* root = rootOf.at(address);
        if (!root || root->getParent() != &document) return {};
        auto* source = dynamic_cast<LC_OffsetCurve*>(root) ? root : address;
        if (std::find(d->originals.begin(), d->originals.end(), source) == d->originals.end()) d->originals.append(source);
        if (chosen.insert(root).second) d->roots.append(root);
    }
    // All counts/active metadata/children are bounded BEFORE the first clone.
    for (auto* root : d->roots) if (!d->captureNode(*root, 0)) return {};
    d->frozen.RS_Entity::setLayer(d->layer(document.getLayer(false)));
    d->frozen.activateLayer(d->layer(d->graphic->getActiveLayer()));
    for (auto* root : d->roots) {
        auto clone = creationClone(*root); if (!clone) return {};
        clone->setParent(&d->frozen); d->bind(*root, *clone);
        d->frozen.addEntity(clone.get()); clone.release();
    }
    for (auto* source : d->originals) {
        auto* copy = d->addresses.at(source); d->sources.append(copy);
        d->worker |= dynamic_cast<RS_Ellipse*>(source) || dynamic_cast<LC_OffsetCurve*>(source);
    }
    auto result = std::shared_ptr<LC_OffsetCreationSources>(new LC_OffsetCreationSources(std::move(d)));
    if (!result->matches(document)) return {};
    return result;
}

bool LC_OffsetCreationSources::matches(const RS_Document& document) const {
    const auto& d = *m_data;
    if (&document != d.document || !document.isOwner() || document.count() != d.topLevel.size() ||
        document.getLayer(false) != d.documentLayer || d.graphic->getActiveLayer() != d.activeLayer) return false;
    int index = 0; for (auto* member : document) if (member != d.topLevel[index++]) return false;
    for (const auto& layer : d.layers) {
        if (!d.graphic->RS_Graphic::getLayerList()->contains(layer.live) || layer.live->getName() != layer.name ||
            !samePen(layer.live->getPen(), layer.pen) || layer.live->isLocked() != layer.locked ||
            layer.live->isFrozen() != layer.hidden) return false;
    }
    for (const auto& node : d.nodes) {
        // Preorder parent lists are checked before any child address is read.
        const auto& entity = *node.address;
        if (entity.getId() != node.id || entity.getParent() != node.parent || entity.getLayer(false) != node.layer ||
            !samePen(entity.getPen(false), node.pen) || creationFlags(entity) != node.flags ||
            !samePoint(entity.getMin(), node.minimum) || !samePoint(entity.getMax(), node.maximum) ||
            metadata(entity) != node.provenance || entity.reactorHandles() != node.reactors ||
            entity.getDrwExtData() != node.tags || !boundedExtData(entity.getDrwExtData())) return false;
        auto geometry = creationGeometry(entity); if (!geometry || *geometry != node.geometry) return false;
        if (node.native && !node.native->matchesStoredState(static_cast<const LC_OffsetCurve&>(entity))) return false;
        for (std::size_t i = 0; i < node.values.size(); ++i) if (!node.values[i].matches(*node.tags[i])) return false;
        const auto keys = entity.getAllKeys(); if (keys.size() != node.variables.size()) return false;
        for (std::size_t i = 0; i < keys.size(); ++i)
            if (keys[i] != node.variables[i].first || entity.getUserDefVar(keys[i]) != node.variables[i].second) return false;
        if (node.owner) {
            const auto& container = static_cast<const RS_EntityContainer&>(entity);
            if (!container.isOwner() || container.count() != node.children.size()) return false;
            for (int i = 0; i < container.count(); ++i) if (container.entityAt(i) != node.children[i]) return false;
        }
    }
    return true;
}
const QList<RS_Entity*>& LC_OffsetCreationSources::sources() const noexcept { return m_data->sources; }
const QList<RS_Entity*>& LC_OffsetCreationSources::originals() const noexcept { return m_data->originals; }
const QList<RS_Entity*>& LC_OffsetCreationSources::roots() const noexcept { return m_data->roots; }
RS_Entity* LC_OffsetCreationSources::original(const RS_Entity* frozen) const noexcept {
    const auto found = m_data->reverse.find(frozen); return found == m_data->reverse.end() ? nullptr : found->second;
}
bool LC_OffsetCreationSources::needsWorker() const noexcept { return m_data->worker; }
bool LC_OffsetCreationSources::localize(const QList<RS_Entity*>& outputs, RS_Layer* active, bool useActive) const {
    std::vector<RS_Entity*> pending(outputs.begin(), outputs.end()); std::set<RS_Entity*> seen;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        auto* entity = pending[i];
        if (!entity || !seen.insert(entity).second || pending.size() > 4*MaximumNodes) return false;
        auto* target = entity->getLayer(false);
        if (target) {
            const auto found = std::find_if(m_data->layers.begin(), m_data->layers.end(),
                [&](const Data::Layer& layer) { return layer.frozen == target; });
            if (found == m_data->layers.end()) return false;
            entity->RS_Entity::setLayer(found->live);
        }
        if (i < std::size_t(outputs.size()) && useActive) entity->RS_Entity::setLayer(active);
        if (auto* container = dynamic_cast<RS_EntityContainer*>(entity)) {
            if (!container->isOwner()) return false;
            for (auto* child : *container) pending.push_back(child);
        }
    }
    return true;
}
