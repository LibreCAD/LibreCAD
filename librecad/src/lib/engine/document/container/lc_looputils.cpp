/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2023 librecad (www.librecad.org)
** Copyright (C) 2023 dxli (github.com/dxli)
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#include <map>
#include <QPen>
#include <QPainterPath>
#include <QTransform>

#include "lc_looputils.h"
#include "lc_rect.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_vector.h"
#include "rs.h"

namespace LC_LoopUtils {

bool isEnclosed(RS_EntityContainer& loop, RS_AtomicEntity& entity) {
    RS_Vector mid = entity.getMiddlePoint();
    bool onContour = false;
    return RS_Information::isPointInsideContour(mid, &loop, &onContour);
}

// LoopExtractor implementation

struct LoopExtractor::LoopData {
    std::vector<RS_Entity*> unprocessed;
    std::map<RS_Entity*, bool> processed;
    RS_Entity* current = nullptr;
    RS_Vector endPoint;
    RS_Vector targetPoint;
    bool reversed = false;
};

LoopExtractor::LoopExtractor(const RS_EntityContainer& edges) :
    m_data(std::make_unique<LoopData>())
{
    for (RS_Entity* e : edges) {
        if (e->isAtomic()) {
            m_data->unprocessed.push_back(e);
            m_data->processed[e] = false;
        }
    }
}

LoopExtractor::~LoopExtractor() = default;

std::vector<std::unique_ptr<RS_EntityContainer>> LoopExtractor::extract() {
    std::vector<std::unique_ptr<RS_EntityContainer>> results;
    while (!m_data->unprocessed.empty()) {
        m_loop = std::make_unique<RS_EntityContainer>();
        RS_Entity* first = findFirst();
        if (first) {
            RS_Entity* cloned_first = first->clone();
            m_loop->addEntity(cloned_first);
            m_data->processed[first] = true;
            m_data->current = cloned_first;
            RS_Vector start = cloned_first->getStartpoint();
            RS_Vector end = cloned_first->getEndpoint();
            m_data->targetPoint = start;
            m_data->endPoint = end;
            m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), first), m_data->unprocessed.end());
            while (m_data->endPoint != m_data->targetPoint) {
                if (findNext()) {
                    if (m_data->endPoint == m_data->targetPoint) break;
                } else {
                    break;
                }
            }
            if (validate()) {
                double area = m_loop->areaLineIntegral();
                if (area < 0.0) {
                    auto new_loop = std::make_unique<RS_EntityContainer>();
                    for (int i = static_cast<int>(m_loop->count()) - 1; i >= 0; --i) {
                        RS_Entity* e = m_loop->entityAt(static_cast<unsigned>(i))->clone();
                        e->revertDirection();
                        new_loop->addEntity(e);
                    }
                    m_loop = std::move(new_loop);
                }
                results.push_back(std::move(m_loop));
            }
        }
    }
    return results;
}

bool LoopExtractor::validate() const {
    if (m_loop->count() == 0) return false;
    if (m_loop->count() == 1) {
        RS_Entity* e = m_loop->entityAt(0);
        RS2::EntityType type = e->rtti();
        if (type == RS2::EntityCircle) return true;
        if (type == RS2::EntityEllipse) {
            RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
            return ell->getAngleLength() >= 2 * M_PI - RS_TOLERANCE;
        }
        return e->getStartpoint() == e->getEndpoint();
    }
    RS_Entity* first = m_loop->entityAt(0);
    RS_Entity* last = m_loop->last();
    return first->getStartpoint() == last->getEndpoint();
}

RS_Entity* LoopExtractor::findFirst() const {
    if (m_data->unprocessed.empty()) return nullptr;
    RS_Entity* firstEdge = m_data->unprocessed[0];
    RS_Vector mid = firstEdge->getMiddlePoint();
    RS_Vector lineStart(mid.x - 1000, mid.y);
    RS_Line testLine(lineStart, mid);
    double minDist = RS_MAXDOUBLE;
    RS_Entity* closest = nullptr;
    for (RS_Entity* e : m_data->unprocessed) {
        RS_VectorSolutions sol = RS_Information::getIntersection(&testLine, e, true);
        if (sol.hasValid()) {
            for (RS_Vector v : sol) {
                double d = v.distanceTo(lineStart);
                if (d < minDist) {
                    minDist = d;
                    closest = e;
                }
            }
        }
    }
    return closest ? closest : firstEdge;
}

bool LoopExtractor::findNext() const {
    std::vector<RS_Entity*> connected = getConnected();
    if (connected.empty()) return false;
    RS_Entity* next = nullptr;
    if (connected.size() == 1) {
        next = connected[0];
    } else {
        next = findOutermost(connected);
    }
    if (next) {
        RS_Entity* cloned = next->clone();
        m_loop->addEntity(cloned);
        m_data->processed[next] = true;
        m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), next), m_data->unprocessed.end());
        if (cloned->getStartpoint() != m_data->endPoint) {
            cloned->revertDirection();
        }
        m_data->endPoint = cloned->getEndpoint();
        m_data->current = cloned;
        return true;
    }
    return false;
}

std::vector<RS_Entity*> LoopExtractor::getConnected() const {
    std::vector<RS_Entity*> ret;
    for (RS_Entity* e : m_data->unprocessed) {
        if (e->getStartpoint() == m_data->endPoint || e->getEndpoint() == m_data->endPoint) {
            ret.push_back(e);
        }
    }
    return ret;
}

RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const {
    double radius = 1e-6;
    RS_Circle circle(nullptr, RS_CircleData(m_data->endPoint, radius));
    double currentAngle = m_data->current->getDirection2();  // Incoming tangent at junction
    std::vector<std::pair<double, RS_Entity*>> angleDiffs;
    for (RS_Entity* e : edges) {
        // Compute outgoing tangent, accounting for reversal
        bool needsReversal = (e->getStartpoint() != m_data->endPoint);
        double outgoingAngle;
        if (!needsReversal) {
            outgoingAngle = e->getDirection1();  // Tangent at start
        } else {
            outgoingAngle = RS_Math::correctAngle(e->getDirection2() + M_PI);  // Flip by pi for reversal
        }

        RS_VectorSolutions sol = RS_Information::getIntersection(&circle, e, true);
        if (!sol.hasValid()) continue;

        // Select the intersection that best matches the outgoing direction
        RS_Vector selected_v;
        double min_adiff = RS_MAXDOUBLE;
        for (size_t k = 0; k < sol.getNumber(); ++k) {
            RS_Vector v = sol.get(k);
            double dist = v.distanceTo(m_data->endPoint);
            if (fabs(dist - radius) > 1e-10) continue;  // Floating-point tolerance

            double a = (v - m_data->endPoint).angle();
            double adiff = RS_Math::correctAngle(a - outgoingAngle);
            if (adiff > M_PI) adiff = 2 * M_PI - adiff;  // Minimal unsigned difference
            if (adiff < min_adiff) {
                min_adiff = adiff;
                selected_v = v;
            }
        }

        if (min_adiff > 1e-4) continue;  // Skip if no good match (numerical issues or curvature)

        double angle = (selected_v - m_data->endPoint).angle();
        double diff = RS_Math::correctAngle(angle - currentAngle);
        if (diff > M_PI) diff -= 2 * M_PI;  // Signed difference in (-pi, pi]

        angleDiffs.push_back({diff, e});
    }

    if (angleDiffs.empty()) return nullptr;
    std::sort(angleDiffs.begin(), angleDiffs.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;  // Ascending signed diff for outermost (prefer right turns)
    });
    return angleDiffs[0].second;
}

// LoopSorter implementation

struct LoopSorter::Data {
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
};

LoopSorter::LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops) : m_data(new Data) {
    m_data->loops = std::move(loops);
}

LoopSorter::~LoopSorter() = default;

struct LoopSorter::AreaPredicate {
    bool operator() (const RS_EntityContainer* a, const RS_EntityContainer* b) const {
        return a->areaLineIntegral() > b->areaLineIntegral();
    }
};

std::vector<LC_Loops> LoopSorter::getResults() {
    std::vector<RS_EntityContainer*> sorted;
    for (auto& p : m_data->loops) {
        sorted.push_back(p.get());
    }
    std::sort(sorted.begin(), sorted.end(), AreaPredicate());
    for (size_t i = 1; i < sorted.size(); ++i) {
        RS_EntityContainer* child = sorted[i];
        findAncestors(child);
    }
    std::vector<LC_Loops> tops;
    for (auto* l : sorted) {
        if (l->getParent() == nullptr) {
            tops.emplace_back(buildLC_Loops(l, m_data->loops));
        }
    }
    return tops;
}

LC_Loops LoopSorter::buildLC_Loops(RS_EntityContainer* cont, const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops) const {
    auto loopCopy = std::make_shared<RS_EntityContainer>(*cont);
    LC_Loops lc(loopCopy, true);
    for (const auto& p : allLoops) {
        if (p.get()->getParent() == cont) {
            lc.addChild(buildLC_Loops(p.get(), allLoops));
        }
    }
    return lc;
}

const std::vector<std::unique_ptr<RS_EntityContainer>>& LoopSorter::getAllLoops() const {
    return m_data->loops;
}

void LoopSorter::init() {
    // Optional initialization
}

void LoopSorter::findAncestors(RS_EntityContainer* loop) {
    std::vector<std::pair<double, RS_EntityContainer*>> enclosing;
    for (auto& p : m_data->loops) {
        RS_EntityContainer* potentialParent = p.get();
        if (potentialParent != loop && isEnclosed(*potentialParent, *(RS_AtomicEntity*)loop->entityAt(0))) {
            double area = potentialParent->areaLineIntegral();
            if (area > 0.0) {  // Skip if invalid/orientation issue
                enclosing.emplace_back(area, potentialParent);
            }
        }
    }
    if (enclosing.empty()) return;
    // Sort by area ascending (smallest/immediate first)
    std::sort(enclosing.begin(), enclosing.end());
    RS_EntityContainer* immediateParent = enclosing[0].second;
    loop->setParent(immediateParent);
}

// LoopOptimizer implementation

struct LoopOptimizer::Data {
    std::shared_ptr<LC_Loops> results;
};

LoopOptimizer::LoopOptimizer(const RS_EntityContainer& contour) : m_data(new Data) {
    m_data->results = std::make_shared<LC_Loops>(true);
    AddContainer(contour);
}

LoopOptimizer::~LoopOptimizer() = default;

std::shared_ptr<LC_Loops> LoopOptimizer::GetResults() const {
    return m_data->results;
}

void LoopOptimizer::AddContainer(const RS_EntityContainer& contour) {
    LoopExtractor extractor(const_cast<RS_EntityContainer&>(contour));
    auto loops = extractor.extract();
    LoopSorter sorter(std::move(loops));
    auto top = sorter.getResults();
    const auto& allLoops = sorter.getAllLoops();
    for (auto* l : top) {
        m_data->results->addChild(buildLC_Loops(l, allLoops));
    }
}

LC_Loops LoopOptimizer::buildLC_Loops(const RS_EntityContainer* cont, const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops) const {
    auto loopCopy = std::make_shared<RS_EntityContainer>(*cont);
    LC_Loops lc(loopCopy, true);
    for (const auto& p : allLoops) {
        if (p.get()->getParent() == cont) {
            lc.addChild(buildLC_Loops(p.get(), allLoops));
        }
    }
    return lc;
}

// LC_Loops implementation

LC_Loops::LC_Loops(bool ownsEntities) : m_ownsEntities(ownsEntities) {}

LC_Loops::LC_Loops(std::shared_ptr<RS_EntityContainer> loop, bool ownsEntities) : m_loop(loop), m_ownsEntities(ownsEntities) {}

LC_Loops::~LC_Loops() {}

void LC_Loops::addChild(LC_Loops child) {
    m_children.push_back(std::move(child));
}

const RS_EntityContainer* LC_Loops::loop() const {
    return m_loop.get();
}

const std::vector<LC_Loops>& LC_Loops::children() const {
    return m_children;
}

bool LC_Loops::ownsEntities() const {
    return m_ownsEntities;
}

bool LC_Loops::isInside(const RS_Vector& point) const {
    bool onContour;
    return RS_Information::isPointInsideContour(point, const_cast<RS_EntityContainer*>(loop()), &onContour);
}

int LC_Loops::getContainingDepth(const RS_Vector& point) const {
    int depth = 0;
    if (isInside(point)) {
        depth = 1;
        for (const auto& child : m_children) {
            depth += child.getContainingDepth(point);
        }
    }
    return depth;
}

QPainterPath LC_Loops::getPainterPath() const {
    QPainterPath path = buildPathFromLoop(*m_loop);
    for (const auto& child : m_children) {
        path.addPath(child.buildPathFromLoop(*child.m_loop));  // Direct build, no isHole
    }
    path.setFillRule(Qt::OddEvenFill);
    return path;
}

RS_Vector LC_Loops::e_point(const RS_Vector& center, double major, double minor, double rot, double t) const {
    double ct = std::cos(t);
    double st = std::sin(t);
    double cr = std::cos(rot);
    double sr = std::sin(rot);
    double x = major * ct * cr - minor * st * sr;
    double y = major * ct * sr + minor * st * cr;
    return center + RS_Vector(x, y);
}

RS_Vector LC_Loops::e_prime(double major, double minor, double rot, double t) const {
    double ct = std::cos(t);
    double st = std::sin(t);
    double cr = std::cos(rot);
    double sr = std::sin(rot);
    double dx = -major * st * cr - minor * ct * sr;
    double dy = -major * st * sr + minor * ct * cr;
    return RS_Vector(dx, dy);
}

void LC_Loops::addEllipticArc(QPainterPath& path, const RS_Vector& center, double major, double minor, double rot, double a1, double a2) const {
    double aspect = std::max(major, minor) / std::min(major, minor);
    int extra_segments = static_cast<int>(std::ceil(aspect - 1.0));
    double sweep = a2 - a1;
    int n = static_cast<int>(std::ceil(std::fabs(sweep) / (M_PI / 2.0))) + extra_segments;
    double dt = sweep / n;
    double current_t = a1;
    for (int i = 0; i < n; ++i) {
        double t0 = current_t;
        double t1 = current_t + dt;
        double lambda = (4.0 / 3.0) * std::tan(dt / 4.0);
        RS_Vector p0 = e_point(center, major, minor, rot, t0);
        RS_Vector p3 = e_point(center, major, minor, rot, t1);
        RS_Vector prime0 = e_prime(major, minor, rot, t0) * lambda;
        RS_Vector prime1 = e_prime(major, minor, rot, t1) * lambda;
        RS_Vector p1 = p0 + prime0;
        RS_Vector p2 = p3 - prime1;
        path.cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
        current_t += dt;
    }
}

QPainterPath LC_Loops::buildPathFromLoop(const RS_EntityContainer& cont) const {
    QPainterPath path;
    if (cont.count() == 0) return path;
    RS_Entity* first = cont.first();
    RS_Vector start = first->getStartpoint();
    path.moveTo(start.x, start.y);
    for (RS_Entity* e : cont) {
        if (e->isAtomic()) {
            RS_Vector end = e->getEndpoint();
            switch (e->rtti()) {
            case RS2::EntityLine:
                path.lineTo(end.x, end.y);
                break;
            case RS2::EntityArc:
            {
                RS_Arc* arc = static_cast<RS_Arc*>(e);
                double r = arc->getRadius();
                addEllipticArc(path, arc->getCenter(), r, r, 0.0, arc->getAngle1(), arc->getAngle2());
            }
            break;
            case RS2::EntityCircle:
            {
                RS_Circle* circle = static_cast<RS_Circle*>(e);
                double r = circle->getRadius();
                addEllipticArc(path, circle->getCenter(), r, r, 0.0, 0.0, 2 * M_PI);
            }
            break;
            case RS2::EntityEllipse:
            {
                RS_Ellipse* ellipse = static_cast<RS_Ellipse*>(e);
                addEllipticArc(path, ellipse->getCenter(), ellipse->getMajorRadius(), ellipse->getMinorRadius(), ellipse->getAngle(), ellipse->getAngle1(), ellipse->getAngle2());
            }
            break;
            default:
                RS_DEBUG->print("LC_Loops::buildPathFromLoop: Unsupported entity type");
                break;
            }
        }
    }
    path.closeSubpath();
    return path;
}

void LC_Loops::getAllLoops(std::vector<const RS_EntityContainer*>& loops) const {
    if (m_loop) loops.push_back(m_loop.get());
    for (const auto& child : m_children) {
        child.getAllLoops(loops);
    }
}

LC_Rect LC_Loops::getBoundingBox() const {
    return {m_loop->getMin(), m_loop->getMax()};
}

bool LC_Loops::isPointInside(const RS_Vector& p) const {
    return getContainingDepth(p) % 2 == 1;
}

std::vector<RS_Vector> LC_Loops::createTiles(const RS_Pattern& pattern) const {
    LC_Rect bBox = getBoundingBox();
    LC_Rect pBox{pattern.getMin(), pattern.getMax()};
    const double pWidth = pBox.right().x - pBox.left().x;
    const double pHeight = pBox.top().y - pBox.bottom().y;
    if (std::min(pWidth, pHeight) < 1e-6)
        return {};
    RS_Vector offsetBase = bBox.lowerLeftCorner() - pBox.lowerLeftCorner();

    std::vector<RS_Vector> tiles;
    int nx = int((bBox.right().x - bBox.left().x)/pWidth) + 1;
    int ny = int((bBox.top().y - bBox.bottom().y)/pHeight) + 1;
    for (int i=0; i < nx; ++i) {
        for (int j=0; j < ny; ++j) {
            RS_Vector tile = offsetBase + RS_Vector{pWidth * i, pHeight * j};
            if (overlap(LC_Rect{pBox.lowerLeftCorner() + tile, pBox.upperRightCorner() + tile}) || isPointInside(tile)) {
                tiles.push_back(tile);
            }
        }

    }

    return tiles;
}

std::vector<RS_Entity*> LC_Loops::getAllBoundaries() const {
    std::vector<const RS_EntityContainer*> loops;
    getAllLoops(loops);
    std::vector<RS_Entity*> bounds;
    for (auto* l : loops) {
        for (RS_Entity* e : *l) {
            if (e->isAtomic()) bounds.push_back(e);
        }
    }
    return bounds;
}

bool LC_Loops::is_entity_closed(const RS_Entity* e) const {
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityCircle) return true;
    if (type == RS2::EntityEllipse) {
        const RS_Ellipse* ell = static_cast<const RS_Ellipse*>(e);
        return fabs(ell->getAngleLength() - 2 * M_PI) < RS_TOLERANCE_ANGLE;
    }
    if (type == RS2::EntityArc) {
        const RS_Arc* arc = static_cast<const RS_Arc*>(e);
        return fabs(arc->getAngleLength() - 2 * M_PI) < RS_TOLERANCE_ANGLE;
    }
    if (type == RS2::EntityLine) return e->getStartpoint() == e->getEndpoint();
    return false;
}

std::unique_ptr<RS_EntityContainer> LC_Loops::trimPatternEntities(const RS_Pattern& pattern) const {
    std::unique_ptr<RS_EntityContainer> trimmed = std::make_unique<RS_EntityContainer>();
    std::vector<RS_Vector> tiles = createTiles(pattern);
    auto boundaries = getAllBoundaries();
    for (const RS_Vector& tile : tiles) {
        for (RS_Entity* e : pattern) {
            if (!e->isAtomic()) continue;
            auto cloned = std::unique_ptr<RS_Entity>(e->clone());
            cloned->move(tile);
            std::vector<RS_Vector> all_inters;
            for (auto* b : boundaries) {
                RS_VectorSolutions sol = RS_Information::getIntersection(cloned.get(), b, true);
                for (RS_Vector v : sol) {
                    all_inters.push_back(v);
                }
            }
            // Remove duplicates
            std::sort(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
                return a.x < b.x || (RS_Math::equal(a.x, b.x) && a.y < b.y);
            });
            auto last = std::unique(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
                return a.distanceTo(b) < RS_TOLERANCE;
            });
            all_inters.erase(last, all_inters.end());
            auto sorted_inters = sortPointsAlongEntity(cloned.get(), all_inters);
            bool closed = is_entity_closed(cloned.get());
            if (sorted_inters.empty()) {
                if (isPointInside(cloned->getMiddlePoint())) {
                    trimmed->addEntity(cloned.release());
                }
                continue;
            }
            if (closed && sorted_inters.size() % 2 != 0) {
                RS_DEBUG->print("LC_Loops::trimPatternEntities: Odd intersections for closed entity, skipping");
                continue;
            }
            std::vector<RS_Vector> points;
            if (!closed) {
                points.push_back(cloned->getStartpoint());
                points.insert(points.end(), sorted_inters.begin(), sorted_inters.end());
                points.push_back(cloned->getEndpoint());
            } else {
                points = sorted_inters;
            }
            for (size_t i = 0; i < points.size() - 1; i += 2) {
                RS_Vector p1 = points[i];
                RS_Vector p2 = points[i + 1];
                if (p1.distanceTo(p2) < RS_TOLERANCE) continue;
                auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), p1, p2));
                if (sub && isPointInside(sub->getMiddlePoint())) {
                    trimmed->addEntity(sub.release());
                }
            }
            if (closed && sorted_inters.size() > 0) {
                RS_Vector pw1 = points.back();
                RS_Vector pw2 = points[0];
                if (pw1.distanceTo(pw2) >= RS_TOLERANCE) {
                    auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), pw1, pw2));
                    if (sub && isPointInside(sub->getMiddlePoint())) {
                        trimmed->addEntity(sub.release());
                    }
                }
            }
        }
    }
    return trimmed;
}

RS_Entity* LC_Loops::createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const {
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        return new RS_Line(nullptr, RS_LineData(p1, p2));
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, arc->getRadius(), ang1, ang2, arc->isReversed()));
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, circle->getRadius(), ang1, ang2, false));
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        RS_Vector lp1 = (p1 - center).rotate(-rot);
        double lang1 = std::atan2(lp1.y / ell->getMinorRadius(), lp1.x / ell->getMajorRadius());
        RS_Vector lp2 = (p2 - center).rotate(-rot);
        double lang2 = std::atan2(lp2.y / ell->getMinorRadius(), lp2.x / ell->getMajorRadius());
        return new RS_Ellipse(nullptr, RS_EllipseData(center, ell->getMajorP(), ell->getRatio(), lang1, lang2, ell->isReversed()));
    }
    return nullptr;
}
std::vector<RS_Vector> LC_Loops::sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const {
    std::vector<std::pair<double, RS_Vector>> param_points;
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        RS_Line* line = static_cast<RS_Line*>(e);
        RS_Vector start = line->getStartpoint();
        RS_Vector dir = line->getEndpoint() - start;
        double len = dir.magnitude();
        if (len < RS_TOLERANCE) return {};
        RS_Vector unit = dir / len;
        for (RS_Vector v : inters) {
            double t = (v - start).dotP(unit);
            if (t >= 0 - RS_TOLERANCE && t <= len + RS_TOLERANCE) param_points.emplace_back(t, v);
        }
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double a1 = arc->getAngle1();
        bool reversed = arc->isReversed();
        for (RS_Vector v : inters) {
            double ang = (v - center).angle();
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        if (!inters.empty()) {
            double ref_ang = (inters[0] - center).angle();
            for (RS_Vector v : inters) {
                double ang = (v - center).angle();
                double diff = RS_Math::correctAngle(ang - ref_ang);
                param_points.emplace_back(diff, v);
            }
        }
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        double a1 = ell->getAngle1();
        bool reversed = ell->isReversed();
        for (RS_Vector v : inters) {
            RS_Vector local = (v - center).rotate(-rot);
            double ang = std::atan2(local.y / ell->getMinorRadius(), local.x / ell->getMajorRadius());
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    }
    std::sort(param_points.begin(), param_points.end(), [](const auto& a, const auto& b){
        return a.first < b.first;
    });
    std::vector<RS_Vector> sorted;
    for (auto& p : param_points) sorted.push_back(p.second);
    return sorted;
}

double LC_Loops::getTotalArea() const {
    double area = std::abs(m_loop->areaLineIntegral());
    for(const auto& child: m_children) area -= child.getTotalArea();
    return area;
}

bool LC_Loops::overlap(const LC_Rect& other) const {
    const bool ret = std::any_of(m_loop->begin(), m_loop->end(), [&other](const RS_Entity* entity) {
        return entity != nullptr && other.overlaps(LC_Rect{entity->getMin(), entity->getMax()});
    });
    return ret || std::any_of(m_children.cbegin(), m_children.cend(), [&other](const LC_Loops& loop) {
               return loop.overlap(other);
           });
}

std::vector<RS_Entity*> LC_Loops::getAllBoundaries() const {
    std::vector<const RS_EntityContainer*> loops;
    getAllLoops(loops);
    std::vector<RS_Entity*> bounds;
    for (auto* l : loops) {
        for (RS_Entity* e : *l) {
            if (e->isAtomic()) bounds.push_back(e);
        }
    }
    return bounds;
}

std::vector<RS_Vector> LC_Loops::sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const {
    std::vector<std::pair<double, RS_Vector>> param_points;
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        RS_Line* line = static_cast<RS_Line*>(e);
        RS_Vector start = line->getStartpoint();
        RS_Vector dir = line->getEndpoint() - start;
        double len = dir.magnitude();
        if (len < RS_TOLERANCE) return {};
        RS_Vector unit = dir / len;
        for (RS_Vector v : inters) {
            double t = (v - start).dotP(unit);
            if (t >= 0 - RS_TOLERANCE && t <= len + RS_TOLERANCE) param_points.emplace_back(t, v);
        }
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double a1 = arc->getAngle1();
        bool reversed = arc->isReversed();
        for (RS_Vector v : inters) {
            double ang = (v - center).angle();
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        if (!inters.empty()) {
            double ref_ang = (inters[0] - center).angle();
            for (RS_Vector v : inters) {
                double ang = (v - center).angle();
                double diff = RS_Math::correctAngle(ang - ref_ang);
                param_points.emplace_back(diff, v);
            }
        }
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        double a1 = ell->getAngle1();
        bool reversed = ell->isReversed();
        for (RS_Vector v : inters) {
            RS_Vector local = (v - center).rotate(-rot);
            double ang = std::atan2(local.y / ell->getMinorRadius(), local.x / ell->getMajorRadius());
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    }
    std::sort(param_points.begin(), param_points.end(), [](const auto& a, const auto& b){
        return a.first < b.first;
    });
    std::vector<RS_Vector> sorted;
    for (auto& p : param_points) sorted.push_back(p.second);
    return sorted;
}

RS_Entity* LC_Loops::createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const {
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        return new RS_Line(nullptr, RS_LineData(p1, p2));
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, arc->getRadius(), ang1, ang2, arc->isReversed()));
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, circle->getRadius(), ang1, ang2, false));
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        RS_Vector lp1 = (p1 - center).rotate(-rot);
        double lang1 = std::atan2(lp1.y / ell->getMinorRadius(), lp1.x / ell->getMajorRadius());
        RS_Vector lp2 = (p2 - center).rotate(-rot);
        double lang2 = std::atan2(lp2.y / ell->getMinorRadius(), lp2.x / ell->getMajorRadius());
        return new RS_Ellipse(nullptr, RS_EllipseData(center, ell->getMajorP(), ell->getRatio(), lang1, lang2, ell->isReversed()));
    }
    return nullptr;
}

double LC_Loops::getTotalArea() const {
    double area = std::abs(m_loop->areaLineIntegral());
    for(const auto& child: m_children) area -= child.getTotalArea();
    return area;
}

bool LC_Loops::overlap(const LC_Rect& other) const {
    const bool ret = std::any_of(m_loop->begin(), m_loop->end(), [&other](const RS_Entity* entity) {
        return entity != nullptr && other.overlaps(LC_Rect{entity->getMin(), entity->getMax()});
    });
    return ret || std::any_of(m_children.cbegin(), m_children.cend(), [&other](const LC_Loops& loop) {
               return loop.overlap(other);
           });
}

std::vector<RS_Entity*> LC_Loops::getAllBoundaries() const {
    std::vector<const RS_EntityContainer*> loops;
    getAllLoops(loops);
    std::vector<RS_Entity*> bounds;
    for (auto* l : loops) {
        for (RS_Entity* e : *l) {
            if (e->isAtomic()) bounds.push_back(e);
        }
    }
    return bounds;
}

std::vector<RS_Vector> LC_Loops::sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const {
    std::vector<std::pair<double, RS_Vector>> param_points;
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        RS_Line* line = static_cast<RS_Line*>(e);
        RS_Vector start = line->getStartpoint();
        RS_Vector dir = line->getEndpoint() - start;
        double len = dir.magnitude();
        if (len < RS_TOLERANCE) return {};
        RS_Vector unit = dir / len;
        for (RS_Vector v : inters) {
            double t = (v - start).dotP(unit);
            if (t >= 0 - RS_TOLERANCE && t <= len + RS_TOLERANCE) param_points.emplace_back(t, v);
        }
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double a1 = arc->getAngle1();
        bool reversed = arc->isReversed();
        for (RS_Vector v : inters) {
            double ang = (v - center).angle();
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        if (!inters.empty()) {
            double ref_ang = (inters[0] - center).angle();
            for (RS_Vector v : inters) {
                double ang = (v - center).angle();
                double diff = RS_Math::correctAngle(ang - ref_ang);
                param_points.emplace_back(diff, v);
            }
        }
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        double a1 = ell->getAngle1();
        bool reversed = ell->isReversed();
        for (RS_Vector v : inters) {
            RS_Vector local = (v - center).rotate(-rot);
            double ang = std::atan2(local.y / ell->getMinorRadius(), local.x / ell->getMajorRadius());
            double diff = RS_Math::getAngleDifference(a1, ang, reversed);
            param_points.emplace_back(diff, v);
        }
    }
    std::sort(param_points.begin(), param_points.end(), [](const auto& a, const auto& b){
        return a.first < b.first;
    });
    std::vector<RS_Vector> sorted;
    for (auto& p : param_points) sorted.push_back(p.second);
    return sorted;
}

RS_Entity* LC_Loops::createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const {
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityLine) {
        return new RS_Line(nullptr, RS_LineData(p1, p2));
    } else if (type == RS2::EntityArc) {
        RS_Arc* arc = static_cast<RS_Arc*>(e);
        RS_Vector center = arc->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, arc->getRadius(), ang1, ang2, arc->isReversed()));
    } else if (type == RS2::EntityCircle) {
        RS_Circle* circle = static_cast<RS_Circle*>(e);
        RS_Vector center = circle->getCenter();
        double ang1 = (p1 - center).angle();
        double ang2 = (p2 - center).angle();
        return new RS_Arc(nullptr, RS_ArcData(center, circle->getRadius(), ang1, ang2, false));
    } else if (type == RS2::EntityEllipse) {
        RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
        RS_Vector center = ell->getCenter();
        double rot = ell->getAngle();
        RS_Vector lp1 = (p1 - center).rotate(-rot);
        double lang1 = std::atan2(lp1.y / ell->getMinorRadius(), lp1.x / ell->getMajorRadius());
        RS_Vector lp2 = (p2 - center).rotate(-rot);
        double lang2 = std::atan2(lp2.y / ell->getMinorRadius(), lp2.x / ell->getMajorRadius());
        return new RS_Ellipse(nullptr, RS_EllipseData(center, ell->getMajorP(), ell->getRatio(), lang1, lang2, ell->isReversed()));
    }
    return nullptr;
}

}  // namespace LC_LoopUtils
