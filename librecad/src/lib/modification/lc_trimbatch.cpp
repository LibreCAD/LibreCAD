/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 * Licensed under GPL-2.0-or-later.
 */
#include "lc_trimbatch.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

#include "lc_curvesplit.h"
#include "lc_containertraverser.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"

namespace {
constexpr double kJoinTolerance = 1e-4;
constexpr double kSideTolerance = 1e-6;

double cross2(const RS_Vector& a, const RS_Vector& b) {
    return a.x * b.y - a.y * b.x;
}

int sign(const double value, const double tolerance = kSideTolerance) {
    return value > tolerance ? 1 : (value < -tolerance ? -1 : 0);
}

bool finiteEndpoint(const RS_Vector& point) {
    return point.valid && std::isfinite(point.x) && std::isfinite(point.y);
}

double parameterLimit(const RS_AtomicEntity& entity) {
    switch (entity.rtti()) {
    case RS2::EntityLine:
        return 1.0;
    case RS2::EntityArc:
        return static_cast<const RS_Arc&>(entity).getData().getAngleLength();
    case RS2::EntityCircle:
        return 2.0 * M_PI;
    case RS2::EntityEllipse:
        return static_cast<const RS_Ellipse&>(entity).isEllipticArc()
                   ? static_cast<const RS_Ellipse&>(entity).getAngleLength()
                   : 2.0 * M_PI;
    default:
        return 0.0;
    }
}

struct BoundaryHit {
    LC_TrimCut cut;
};

struct Piece {
    double tA = 0.0;
    double tB = 0.0;
    RS_Vector pA;
    RS_Vector pB;
    std::vector<int> sides;
};

enum class PieceVerdict { Kept, Removed, Undetermined };

PieceVerdict verdict(const Piece& piece, const std::vector<int>& cursorSides) {
    bool known = false;
    bool removed = false;
    for (std::size_t component = 0; component < piece.sides.size() && component < cursorSides.size(); ++component) {
        const int side = piece.sides[component];
        if (side == 0) continue;
        known = true;
        if (cursorSides[component] != 0 && side != cursorSides[component]) removed = true;
    }
    return removed ? PieceVerdict::Removed : (known ? PieceVerdict::Kept : PieceVerdict::Undetermined);
}

std::vector<int> pieceSidesFor(const RS_AtomicEntity& target,
                               const double tA,
                               const double tB,
                               const std::vector<LC_TrimCut>& cuts,
                               const int componentCount) {
    std::vector<int> sides(static_cast<std::size_t>(componentCount), 0);
    for (int component = 0; component < componentCount; ++component) {
        const LC_TrimCut* before = nullptr;
        const LC_TrimCut* after = nullptr;
        for (const LC_TrimCut& cut : cuts) {
            if (cut.component != component) {
                continue;
            }
            if (cut.t <= tA + 1e-9 && (before == nullptr || cut.t > before->t)) {
                before = &cut;
            }
            if (cut.t >= tB - 1e-9 && (after == nullptr || cut.t < after->t)) {
                after = &cut;
            }
        }
        if (before != nullptr && after != nullptr && before != after) {
            const int afterSide = before->sideAfter;
            const int beforeSide = after->sideBefore;
            sides[static_cast<std::size_t>(component)] = afterSide == beforeSide ? afterSide : 0;
        }
        else if (before != nullptr) {
            sides[static_cast<std::size_t>(component)] = before->sideAfter;
        }
        else if (after != nullptr) {
            sides[static_cast<std::size_t>(component)] = after->sideBefore;
        }
        // A closed target's last segment crosses the parameter seam.
        else if (lc::curvesplit::isClosed(target)) {
            const LC_TrimCut* last = nullptr;
            const LC_TrimCut* first = nullptr;
            for (const LC_TrimCut& cut : cuts) {
                if (cut.component != component) continue;
                if (last == nullptr || cut.t > last->t) last = &cut;
                if (first == nullptr || cut.t < first->t) first = &cut;
            }
            if (last != nullptr && first != nullptr) {
                sides[static_cast<std::size_t>(component)] = last->sideAfter == first->sideBefore
                                                                  ? last->sideAfter : 0;
            }
        }
    }
    return sides;
}

std::vector<Piece> makePieces(const RS_AtomicEntity& source,
                              const std::vector<LC_TrimCut>& cuts,
                              const int componentCount) {
    std::vector<Piece> pieces;
    if (cuts.empty()) return pieces;
    const bool closed = lc::curvesplit::isClosed(source);
    if (!closed) {
        std::vector<double> ts{0.0};
        std::vector<RS_Vector> points{source.getStartpoint()};
        for (const LC_TrimCut& cut : cuts) {
            if (cut.t > 1e-9 && cut.t < parameterLimit(source) - 1e-9) {
                ts.push_back(cut.t);
                points.push_back(cut.point);
            }
        }
        ts.push_back(parameterLimit(source));
        points.push_back(source.getEndpoint());
        for (std::size_t i = 0; i + 1 < ts.size(); ++i) {
            Piece piece{ts[i], ts[i + 1], points[i], points[i + 1], {}};
            piece.sides = pieceSidesFor(source, piece.tA, piece.tB, cuts, componentCount);
            pieces.push_back(std::move(piece));
        }
        return pieces;
    }

    if (cuts.size() < 2) return pieces;
    for (std::size_t i = 0; i < cuts.size(); ++i) {
        const LC_TrimCut& first = cuts[i];
        const LC_TrimCut& second = cuts[(i + 1) % cuts.size()];
        Piece piece{first.t, second.t, first.point, second.point, {}};
        piece.sides = pieceSidesFor(source, piece.tA, piece.tB, cuts, componentCount);
        if (i + 1 == cuts.size()) {
            for (int component = 0; component < componentCount; ++component) {
                const LC_TrimCut* last = nullptr;
                const LC_TrimCut* earliest = nullptr;
                for (const LC_TrimCut& cut : cuts) {
                    if (cut.component != component) continue;
                    if (last == nullptr || cut.t > last->t) last = &cut;
                    if (earliest == nullptr || cut.t < earliest->t) earliest = &cut;
                }
                if (last != nullptr && earliest != nullptr) {
                    piece.sides[static_cast<std::size_t>(component)] =
                        last->sideAfter == earliest->sideBefore ? last->sideAfter : 0;
                }
            }
        }
        pieces.push_back(std::move(piece));
    }
    return pieces;
}
}

bool isReplaceableByTrim(const RS_Entity& entity) {
    const RS_EntityContainer* parent = entity.getParent();
    return parent == nullptr || parent->isDocument();
}

struct LC_TrimLimitSet::Data {
    struct Element {
        std::unique_ptr<RS_AtomicEntity> entity;
        int component = 0;
        int orientation = 1;
        bool closed = false;
    };
    std::vector<Element> elements;
    std::vector<bool> closedComponents;
    std::unordered_set<unsigned long long> sourceIds;
};

LC_TrimLimitSet::LC_TrimLimitSet() : m_data(std::make_unique<Data>()) {}
LC_TrimLimitSet::~LC_TrimLimitSet() = default;
LC_TrimLimitSet::LC_TrimLimitSet(LC_TrimLimitSet&&) noexcept = default;
LC_TrimLimitSet& LC_TrimLimitSet::operator=(LC_TrimLimitSet&&) noexcept = default;

void LC_TrimLimitSet::add(const RS_Entity& limit) {
    m_data->sourceIds.insert(limit.getId());
    auto append = [this](const RS_Entity* candidate) {
        if (candidate == nullptr || !candidate->isAtomic()) return;
        const auto* atomic = static_cast<const RS_AtomicEntity*>(candidate);
        if (!lc::curvesplit::isSupported(*atomic)) return;
        auto* clone = static_cast<RS_AtomicEntity*>(atomic->clone());
        clone->setParent(nullptr);
        clone->setLayer(nullptr);
        m_data->elements.push_back({std::unique_ptr<RS_AtomicEntity>(clone), -1, 1,
                                    lc::curvesplit::isClosed(*clone)});
        m_data->sourceIds.insert(candidate->getId());
    };
    if (limit.isAtomic()) {
        append(&limit);
    }
    else if (limit.isContainer()) {
        const auto& container = static_cast<const RS_EntityContainer&>(limit);
        for (RS_Entity* child : lc::LC_ContainerTraverser{container, RS2::ResolveAll}.entities()) {
            append(child);
        }
    }
}

void LC_TrimLimitSet::build() {
    const std::size_t count = m_data->elements.size();
    std::vector<int> parent(count);
    for (std::size_t i = 0; i < count; ++i) parent[i] = static_cast<int>(i);
    auto root = [&parent](int i) {
        while (parent[static_cast<std::size_t>(i)] != i) {
            parent[static_cast<std::size_t>(i)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(i)])];
            i = parent[static_cast<std::size_t>(i)];
        }
        return i;
    };
    std::vector<int> degree(count, 0);
    std::vector<std::vector<std::pair<int, bool>>> links(count);
    for (std::size_t i = 0; i < count; ++i) {
        RS_Entity* a = m_data->elements[i].entity.get();
        const RS_Vector a0 = a->getStartpoint();
        const RS_Vector a1 = a->getEndpoint();
        if (!finiteEndpoint(a0) || !finiteEndpoint(a1)) continue;
        for (std::size_t j = i + 1; j < count; ++j) {
            RS_Entity* b = m_data->elements[j].entity.get();
            const RS_Vector b0 = b->getStartpoint();
            const RS_Vector b1 = b->getEndpoint();
            if (!finiteEndpoint(b0) || !finiteEndpoint(b1)) continue;
            const bool same = a1.distanceTo(b0) <= kJoinTolerance || a0.distanceTo(b1) <= kJoinTolerance;
            const bool opposite = a1.distanceTo(b1) <= kJoinTolerance || a0.distanceTo(b0) <= kJoinTolerance;
            if (same || opposite) {
                links[i].push_back({static_cast<int>(j), same});
                links[j].push_back({static_cast<int>(i), same});
                ++degree[i]; ++degree[j];
            }
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        for (const auto& [j, same] : links[i]) {
            if (degree[i] > 2 || degree[static_cast<std::size_t>(j)] > 2) continue;
            const int ri = root(static_cast<int>(i));
            const int rj = root(j);
            if (ri != rj) parent[static_cast<std::size_t>(rj)] = ri;
            Q_UNUSED(same);
        }
    }
    std::vector<int> rootToComponent(count, -1);
    m_data->closedComponents.clear();
    for (std::size_t i = 0; i < count; ++i) {
        const int r = root(static_cast<int>(i));
        if (rootToComponent[static_cast<std::size_t>(r)] < 0) {
            rootToComponent[static_cast<std::size_t>(r)] = static_cast<int>(m_data->closedComponents.size());
            m_data->closedComponents.push_back(false);
        }
        auto& element = m_data->elements[i];
        element.component = rootToComponent[static_cast<std::size_t>(r)];
        element.orientation = 1;
        if (element.closed) m_data->closedComponents[static_cast<std::size_t>(element.component)] = true;
    }
    // Orient each connected chain consistently. At an endpoint match, a segment is reversed when its native
    // start/start or end/end endpoint meets the current segment's oriented end.
    for (int component = 0; component < static_cast<int>(m_data->closedComponents.size()); ++component) {
        std::vector<std::size_t> members;
        for (std::size_t i = 0; i < count; ++i) if (m_data->elements[i].component == component) members.push_back(i);
        if (members.size() < 2) continue;
        std::size_t first = members.front();
        for (const std::size_t member : members) {
            if (links[member].size() <= 1) { first = member; break; }
        }
        auto endpointConnected = [this, &members](const std::size_t member, const RS_Vector& point) {
            for (const std::size_t other : members) {
                if (other == member) continue;
                const RS_Entity* entity = m_data->elements[other].entity.get();
                if (entity->getStartpoint().distanceTo(point) <= kJoinTolerance ||
                    entity->getEndpoint().distanceTo(point) <= kJoinTolerance) return true;
            }
            return false;
        };
        const RS_Entity* firstEntity = m_data->elements[first].entity.get();
        const bool startIsFree = !endpointConnected(first, firstEntity->getStartpoint());
        const bool endIsFree = !endpointConnected(first, firstEntity->getEndpoint());
        if (!startIsFree && endIsFree) m_data->elements[first].orientation = -1;
        std::unordered_set<std::size_t> visited;
        visited.insert(first);
        RS_Vector end = m_data->elements[first].orientation > 0 ? firstEntity->getEndpoint() : firstEntity->getStartpoint();
        while (visited.size() < members.size()) {
            bool advanced = false;
            for (const std::size_t next : members) {
                if (visited.find(next) != visited.end()) continue;
                RS_Entity* e = m_data->elements[next].entity.get();
                if (end.distanceTo(e->getStartpoint()) <= kJoinTolerance) {
                    m_data->elements[next].orientation = 1;
                    end = e->getEndpoint();
                }
                else if (end.distanceTo(e->getEndpoint()) <= kJoinTolerance) {
                    m_data->elements[next].orientation = -1;
                    end = e->getStartpoint();
                }
                else continue;
                visited.insert(next);
                advanced = true;
                break;
            }
            if (!advanced) break;
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        const int component = m_data->elements[i].component;
        if (component < 0 || m_data->closedComponents[static_cast<std::size_t>(component)]) continue;
        std::size_t members = 0;
        bool closed = true;
        for (std::size_t j = 0; j < count; ++j) {
            if (m_data->elements[j].component != component) continue;
            ++members;
            closed = closed && degree[j] == 2;
        }
        if (members > 1 && closed) m_data->closedComponents[static_cast<std::size_t>(component)] = true;
    }
}

int LC_TrimLimitSet::componentCount() const { return static_cast<int>(m_data->closedComponents.size()); }
bool LC_TrimLimitSet::isClosed(const int component) const {
    return component >= 0 && component < componentCount() && m_data->closedComponents[static_cast<std::size_t>(component)];
}

int LC_TrimLimitSet::sideOfPoint(const int component, const RS_Vector& point) const {
    double bestDistance = std::numeric_limits<double>::max();
    RS_Vector bestPoint{false};
    for (const Data::Element& element : m_data->elements) {
        if (element.component != component) continue;
        double distance = std::numeric_limits<double>::max();
        const RS_Vector nearest = element.entity->getNearestPointOnEntity(point, true, &distance);
        if (!std::isfinite(distance)) distance = point.distanceTo(nearest);
        if (distance <= kJoinTolerance) return 0;
        if (distance < bestDistance) {
            bestDistance = distance;
            bestPoint = nearest;
        }
    }
    return bestPoint.valid ? sideOfDirection(component, bestPoint, point - bestPoint) : 0;
}

int LC_TrimLimitSet::sideOfDirection(const int component, const RS_Vector& point, const RS_Vector& direction) const {
    RS_Vector incoming{false};
    RS_Vector outgoing{false};
    RS_Vector freeTangent{false};
    for (const Data::Element& element : m_data->elements) {
        if (element.component != component) continue;
        const RS_Entity& entity = *element.entity;
        const RS_Vector start = entity.getStartpoint();
        const RS_Vector end = entity.getEndpoint();
        if (!finiteEndpoint(start) || !finiteEndpoint(end)) {
            const RS_Vector nearest = entity.getNearestPointOnEntity(point, true);
            if (nearest.distanceTo(point) <= kJoinTolerance) {
                return sign(cross2(lc::curvesplit::tangentAt(*element.entity, point) * element.orientation, direction));
            }
            continue;
        }
        const bool atStart = start.distanceTo(point) <= kJoinTolerance;
        const bool atEnd = end.distanceTo(point) <= kJoinTolerance;
        if (!atStart && !atEnd) continue;
        if (atStart) {
            RS_Vector tangent = lc::curvesplit::tangentAt(*element.entity, start);
            if (element.orientation > 0) outgoing = tangent;
            else incoming = tangent * -1.0;
        }
        if (atEnd) {
            RS_Vector tangent = lc::curvesplit::tangentAt(*element.entity, end);
            if (element.orientation > 0) incoming = tangent;
            else outgoing = tangent * -1.0;
        }
    }
    if (!incoming.valid && !outgoing.valid) {
        for (const Data::Element& element : m_data->elements) {
            if (element.component != component) continue;
            const RS_Vector nearest = element.entity->getNearestPointOnEntity(point, true);
            if (nearest.valid && nearest.distanceTo(point) <= kJoinTolerance) {
                freeTangent = lc::curvesplit::tangentAt(*element.entity, nearest) * element.orientation;
                break;
            }
        }
    }
    if (incoming.valid && outgoing.valid) {
        const double fromIncoming = cross2(incoming, direction);
        const double fromOutgoing = cross2(outgoing, direction);
        if (std::abs(fromIncoming) <= kSideTolerance || std::abs(fromOutgoing) <= kSideTolerance) return 0;
        const bool left = cross2(incoming, outgoing) > 0.0
                              ? fromIncoming > 0.0 && fromOutgoing > 0.0
                              : fromIncoming > 0.0 || fromOutgoing > 0.0;
        return left ? 1 : -1;
    }
    if (incoming.valid) return sign(cross2(incoming, direction));
    if (outgoing.valid) return sign(cross2(outgoing, direction));
    return freeTangent.valid ? sign(cross2(freeTangent, direction)) : 0;
}

bool LC_TrimLimitSet::containsSourceOf(const RS_Entity* entity) const {
    return entity != nullptr && m_data->sourceIds.find(entity->getId()) != m_data->sourceIds.end();
}

struct LC_TrimBatch::Data {
    struct Target {
        RS_AtomicEntity* source = nullptr;
        std::vector<LC_TrimCut> cuts;
        std::vector<Piece> pieces;
    };
    const LC_TrimLimitSet* limits = nullptr;
    std::vector<Target> targets;
};

LC_TrimBatch::LC_TrimBatch(const QList<RS_Entity*>& targets, const LC_TrimLimitSet& limits)
    : m_data(std::make_unique<Data>()) {
    m_data->limits = &limits;
    for (RS_Entity* entity : targets) {
        if (entity == nullptr || !entity->isAtomic() || !isReplaceableByTrim(*entity) || limits.containsSourceOf(entity)) continue;
        auto* target = static_cast<RS_AtomicEntity*>(entity);
        if (!lc::curvesplit::isSupported(*target)) continue;
        Data::Target data;
        data.source = target;
        for (const LC_TrimLimitSet::Data::Element& element : limits.m_data->elements) {
            RS_VectorSolutions intersections = RS_Information::getIntersection(target, element.entity.get(), true);
            for (const RS_Vector& point : intersections) {
                if (!point.valid || !target->isPointOnEntity(point, kJoinTolerance) ||
                    !element.entity->isPointOnEntity(point, kJoinTolerance)) continue;
                if (!lc::curvesplit::isClosed(*target) &&
                    (point.distanceTo(target->getStartpoint()) <= kJoinTolerance ||
                     point.distanceTo(target->getEndpoint()) <= kJoinTolerance)) continue;
                const double t = lc::curvesplit::paramOf(*target, point);
                if (!std::isfinite(t)) continue;
                const RS_Vector targetTangent = lc::curvesplit::tangentAt(*target, point);
                const int before = limits.sideOfDirection(element.component, point, targetTangent * -1.0);
                const int after = limits.sideOfDirection(element.component, point, targetTangent);
                data.cuts.push_back({t, point, element.component, before, after});
            }
        }
        std::sort(data.cuts.begin(), data.cuts.end(), [](const LC_TrimCut& a, const LC_TrimCut& b) { return a.t < b.t; });
        data.cuts.erase(std::unique(data.cuts.begin(), data.cuts.end(), [](const LC_TrimCut& a, const LC_TrimCut& b) {
            return a.component == b.component && a.point.distanceTo(b.point) <= kJoinTolerance;
        }), data.cuts.end());
        data.pieces = makePieces(*target, data.cuts, limits.componentCount());
        m_data->targets.push_back(std::move(data));
    }
}

LC_TrimBatch::~LC_TrimBatch() = default;
LC_TrimBatch::LC_TrimBatch(LC_TrimBatch&&) noexcept = default;
LC_TrimBatch& LC_TrimBatch::operator=(LC_TrimBatch&&) noexcept = default;

bool LC_TrimBatch::anyCut() const {
    return std::any_of(m_data->targets.begin(), m_data->targets.end(), [](const Data::Target& target) { return !target.cuts.empty(); });
}
std::vector<RS_Vector> LC_TrimBatch::cutPoints() const {
    std::vector<RS_Vector> points;
    for (const Data::Target& target : m_data->targets) {
        for (const LC_TrimCut& cut : target.cuts) {
            const bool duplicate = std::any_of(points.begin(), points.end(), [&cut](const RS_Vector& point) {
                return point.distanceTo(cut.point) <= kJoinTolerance;
            });
            if (!duplicate) points.push_back(cut.point);
        }
    }
    return points;
}

std::vector<int> LC_TrimBatch::cursorSides(const RS_Vector& cursor) const {
    std::vector<int> sides;
    if (m_data->limits == nullptr) return sides;
    for (int component = 0; component < m_data->limits->componentCount(); ++component) {
        sides.push_back(m_data->limits->sideOfPoint(component, cursor));
    }
    return sides;
}

std::vector<std::unique_ptr<RS_AtomicEntity>> LC_TrimBatch::removedPieces(const std::vector<int>& cursorSides) const {
    std::vector<std::unique_ptr<RS_AtomicEntity>> result;
    for (const Data::Target& target : m_data->targets) {
        for (std::size_t i = 0; i < target.pieces.size(); ++i) {
            if (verdict(target.pieces[i], cursorSides) != PieceVerdict::Removed) continue;
            const Piece& piece = target.pieces[i];
            auto removed = lc::curvesplit::subCurve(*target.source, piece.tA, piece.pA, piece.tB, piece.pB);
            if (removed != nullptr) result.push_back(std::move(removed));
        }
    }
    return result;
}

LC_TrimBatch::Stats LC_TrimBatch::apply(const std::vector<int>& cursorSides,
                                        LC_DocumentModificationBatch& context) const {
    Stats stats;
    for (std::size_t ti = 0; ti < m_data->targets.size(); ++ti) {
        const Data::Target& target = m_data->targets[ti];
        if (target.source == nullptr || target.pieces.empty()) continue;
        std::vector<bool> keep(target.pieces.size(), false);
        bool allKept = true;
        for (std::size_t pi = 0; pi < target.pieces.size(); ++pi) {
            const PieceVerdict result = verdict(target.pieces[pi], cursorSides);
            keep[pi] = result != PieceVerdict::Removed;
            if (result == PieceVerdict::Removed) ++stats.removed;
            else if (result == PieceVerdict::Undetermined) ++stats.undetermined;
            allKept = allKept && keep[pi];
        }
        if (allKept) continue;
        if (!isReplaceableByTrim(*target.source) || target.source->isLocked() || !target.source->isVisible()) {
            continue;
        }

        std::vector<std::pair<std::size_t, std::size_t>> runs;
        if (!lc::curvesplit::isClosed(*target.source)) {
            std::size_t i = 0;
            while (i < keep.size()) {
                if (!keep[i]) { ++i; continue; }
                const std::size_t start = i;
                while (i + 1 < keep.size() && keep[i + 1]) ++i;
                runs.emplace_back(start, i);
                ++i;
            }
        }
        else {
            const auto removed = std::find(keep.begin(), keep.end(), false);
            if (removed != keep.end()) {
                const std::size_t firstRemoved = static_cast<std::size_t>(removed - keep.begin());
                std::size_t consumed = 0;
                std::size_t i = (firstRemoved + 1) % keep.size();
                while (consumed < keep.size() - 1) {
                    if (!keep[i]) { i = (i + 1) % keep.size(); ++consumed; continue; }
                    const std::size_t start = i;
                    std::size_t end = i;
                    while (consumed + 1 < keep.size() && keep[(end + 1) % keep.size()]) {
                        end = (end + 1) % keep.size();
                        ++consumed;
                    }
                    runs.emplace_back(start, end);
                    i = (end + 1) % keep.size();
                    ++consumed;
                }
            }
        }

        std::vector<std::unique_ptr<RS_AtomicEntity>> outputs;
        outputs.reserve(runs.size());
        for (const auto [first, last] : runs) {
            const Piece& start = target.pieces[first];
            const Piece& end = target.pieces[last];
            auto output = lc::curvesplit::subCurve(*target.source, start.tA, start.pA, end.tB, end.pB);
            if (output == nullptr) break;
            outputs.push_back(std::move(output));
        }
        if (outputs.size() != runs.size()) continue;
        for (auto& output : outputs) {
            output->setPen(target.source->getPen(false));
            output->setLayer(target.source->getLayer(false));
            context += output.release();
        }
        context -= target.source;
        ++stats.trimmed;
    }
    if (stats.trimmed > 0) context.dontSetActiveLayerAndPen();
    return stats;
}
