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
#include <algorithm>
#include <array>
#include <map>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

#include "lc_looputils.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_vector.h"

namespace {

constexpr double contourGapTolerance = 1E-7;

// a random angle between 0 and 2 pi
double getRandomAngle();

// Create a random ray: starting from an internal point of the loop
std::unique_ptr<RS_Line> getRandomRay(RS_EntityContainer* loop);

// Find intersection between a line and a loop
RS_VectorSolutions getIntersection(const RS_Entity& line, const RS_EntityContainer& loop);

// Build a loop to loop area lookup table
std::unordered_map<const RS_EntityContainer*, double> findAreas(const std::vector<std::unique_ptr<RS_EntityContainer>>& loops );

struct ComparePoints {
    ComparePoints() = default;
    ComparePoints(const RS_Vector& ref) : m_ref{ref}{}
  bool operator()(const RS_Vector &p0, const RS_Vector &p1) const;
  RS_Vector m_ref{false};
};

// the closest distance from a point to the end points of an entity
double getEndPointDistance(const RS_Vector& point, const RS_Entity& entity);

// Whether two entities are connected by an end point
bool isConnected( const RS_Entity& entity1, const RS_Entity& entity2);

// randomEngine
std::default_random_engine randomEngine;

// ------------------------------------------------------------------------------------- //
// a random angle between 0 and 2 pi
double getRandomAngle() {
    static std::uniform_real_distribution<double> uniformDistribution(0.0,
                                                                      2. * M_PI);
    return uniformDistribution(randomEngine);
}

// the closest distance from a point to the end points of an entity
double getEndPointDistance(const RS_Vector& point, const RS_Entity& entity)
{
    double distance = RS_MAXDOUBLE;
    entity.getNearestEndpoint(point, &distance);
    return distance;
}

// Whether two entities are connected by an end point
bool isConnected( const RS_Entity& entity1, const RS_Entity& entity2)
{
    const double distance = std::min(getEndPointDistance(entity1.getStartpoint(), entity2),
                                     getEndPointDistance(entity1.getEndpoint(), entity2));
    if (distance > contourGapTolerance)
    {
        LC_ERR<<"loop gap of "<<distance<<" between "<<entity1.getId()<<", "<<entity2.getId();
        LC_ERR<<"("<<entity1.getStartpoint().x<<", "<<entity1.getEndpoint().y<<") - "
        "("<<entity2.getStartpoint().x<<", "<<entity2.getEndpoint().y<<")";
        return false;
    }
    return true;
}

RS_Vector getInternalPoint(const RS_EntityContainer& loop)
{
    RS_Vector p0 = loop.firstEntity()->getNearestPointOnEntity(loop.getMin(), true);
    double size = loop.getSize().magnitude();
    for(short i=0; i<16; i++)
    {
        RS_Vector offset =  RS_Vector(getRandomAngle())*size;
        auto line = std::make_unique<RS_Line>(p0 - offset, p0 + offset);
        RS_VectorSolutions results = getIntersection(*line, loop);
        // need even number of intersections
        if (results.empty() || results.size() % 2 == 1)
            continue;
        std::sort(results.begin(), results.end(), ComparePoints{});
        // find an internal point
        return (results.at(0) + results.at(1)) * 0.5;
    }
    LC_LOG<<__func__<<"(): failed to find a line passing the loop: "<<loop.getId();
    return RS_Vector{false};
}

std::unique_ptr<RS_Line> getRandomRay(RS_EntityContainer* loop)
{
    assert(loop != nullptr);
    RS_Vector p0 = getInternalPoint(*loop);
    double size = loop->getSize().magnitude();
    return std::make_unique<RS_Line>(nullptr, p0, p0 + RS_Vector{getRandomAngle()}*size);
}

std::unordered_map<const RS_EntityContainer*, double> findAreas(const std::vector<std::unique_ptr<RS_EntityContainer>>& loops )
{
    std::unordered_map<const RS_EntityContainer*, double> ret;
    for(const std::unique_ptr<RS_EntityContainer>& loop: loops)
        ret.emplace(loop.get(), std::abs(loop->areaLineIntegral()));
    return ret;
}

RS_VectorSolutions getIntersection(const RS_Entity& line,
                                   const RS_EntityContainer &loop) {
    RS_VectorSolutions ret;
    for (RS_Entity *entity : loop) {
        if (entity != nullptr && entity->isEdge()) {
            auto intersections = RS_Information::getIntersection(&line, entity, true);
            ret.push_back(intersections);
        }
    }
    return ret;
}

bool ComparePoints::operator()(const RS_Vector &p0, const RS_Vector &p1) const {
    if (m_ref.valid)
        return m_ref.squaredTo(p0) < m_ref.squaredTo(p1);
    if (p0.x + RS_TOLERANCE < p1.x)
        return true;
    else if (p0.x > p1.x + RS_TOLERANCE)
        return false;
    return p0.y + RS_TOLERANCE < p1.y;
}
} // namespace

namespace LC_LoopUtils {

struct CompareDistance {
    CompareDistance(const RS_Vector& point):
        m_point{point}
    {}

    bool operator() (const RS_Vector &p0, const RS_Vector &p1) const {
        return p0.squaredTo(m_point) < p1.squaredTo(m_point);
    }

    const RS_Vector& m_point;
};

bool isEnclosed(RS_EntityContainer& loop, RS_AtomicEntity& entity)
{
    RS_VectorSolutions intersections = getIntersection(entity, loop);
    if (!intersections.empty())
        return false;
    RS_Line line{nullptr, {getInternalPoint(loop), entity.getMiddlePoint()}};
    intersections = getIntersection(line, loop);
    return intersections.size()%2 == 0;
}

struct LoopExtractor::LoopData {
    LoopData(RS_EntityContainer &edges):
        size{edges.getSize().magnitude()}
    , edges{edges}
    {}
    const double size = 0.;
    RS_Vector vertex;
    RS_Vector vertexTarget;
    RS_Entity* current = nullptr;
    RS_EntityContainer& edges;
};

LoopExtractor::LoopExtractor(RS_EntityContainer &edges) :
    m_data{std::make_unique<LoopData>(edges)}
{
    assert(m_data->size > RS_TOLERANCE);
    assert(!edges.isEmpty());
}

//------------------------------------------------------------------------------------//
std::vector<RS_Entity*> LoopExtractor::getConnected() const
{
    std::vector<RS_Entity *> connected;
    std::copy_if(m_data->edges.begin(), m_data->edges.end(), std::back_inserter(connected),
                 [vertex = m_data->vertex, current = m_data->current](const RS_Entity *e) {
        if (e == current)
            return false;
        double dist = RS_MAXDOUBLE;
        e->getNearestEndpoint(vertex, &dist);
        return dist < contourGapTolerance;
    });
    return connected;
}

//------------------------------------------------------------------------------------//
LoopExtractor::~LoopExtractor() = default;


//------------------------------------------------------------------------------------//
// The algorithm:
// Keep finding outermost loops from unprocessed edges and remove the loops from unprocess edges.
// To find the first edge on the outermost, draw a line acrossing one middle point, sort the intersections
// with all edges by coordinates, the first and last intersections are on the outermost loop
// To find the next connected edge, search the along the counterclockwise direction, the next outermost edge
// requires the smallest left turning angles.
RS_Entity* LoopExtractor::findFirst() const
{

    // draw a line crossing the first edge
    RS_Entity* first = m_data->edges.firstEntity();
    RS_Vector p0 = first->getMiddlePoint();
    RS_Vector t0 = first->getTangentDirection(p0).normalize();
    // The dP0 direction is off the normal direction by a random angle smaller than 0.06*Pi
    RS_Vector dP0 = t0.rotate(M_PI/2 + (getRandomAngle() - M_PI) * 0.06) * m_data->size * 1.1;

    // draw a line
    std::array<RS_Vector, 2> linePoints = {{p0 - dP0, p0 + dP0}};
    std::sort(std::begin(linePoints), std::end(linePoints), ComparePoints{});
    RS_Line line0{linePoints.front(), linePoints.back()};

    // Find intersections: only keep the intersection of minimum xy-coordinates
    double dist=RS_MAXDOUBLE * RS_MAXDOUBLE;
    for(RS_Entity* edge: m_data->edges)
    {
        RS_VectorSolutions sol0 = RS_Information::getIntersection(&line0, edge, true);
        if (!sol0.empty()) {
            for (const RS_Vector& p00: sol0) {
                double dist2 = p00.squaredTo(linePoints.front());
                if (dist2 < dist) {
                    first = edge;
                    dist = dist2;
                    p0 = p00;
                }
            }
        }
    }

    // The outermost intersection is from an outermost edge, and choose this edge as the beginning of a loop

    // Always search the next loop edge in the counter-clock direction
    // getTangentDirection() always along the curve, the direction is from the curve start point to the end point
    // if cross.z is positive, the tangential direction is counterclockwise.
    RS_Vector cross = RS_Vector::crossP(linePoints.front() - p0, first->getTangentDirection(p0));
    bool reversed = std::signbit(cross.z);

    m_data->vertex = reversed ? first->getStartpoint() : first->getEndpoint();
    m_data->vertexTarget = reversed ? first->getEndpoint() : first->getStartpoint();
    m_data->current = first;
    m_loop = std::make_unique<RS_EntityContainer>(nullptr, false);
    m_loop->addEntity(m_data->current);
    m_data->edges.removeEntity(first);
    return first;
}

//------------------------------------------------------------------------------------//
RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const
{
    assert(edges.size() >= 2);
    double edgeLength = RS_MAXDOUBLE;
    for(RS_Entity* edge: edges)
        edgeLength = std::min({edgeLength, edge->getLength(), edge->getStartpoint().distanceTo(edge->getEndpoint())});

    // draw a small circle around the current end point
    RS_Circle circle{nullptr, {m_data->vertex, edgeLength * 0.01}};
    auto getCut = [&circle, p0 = m_data->vertex](RS_Entity* edge){
        RS_VectorSolutions sol = RS_Information::getIntersection(&circle, edge, true);
        assert(!sol.empty());
        return std::make_pair(edge, p0.angleTo(sol.at(0)));
    };
    using CutPair = std::pair<RS_Entity*, double>;
    // find the angle for the current edge
    CutPair current = getCut(m_data->current);
    std::vector<CutPair> cuts;
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(cuts), getCut);

    // find the minimum left turning angle to get the next outermost edge
    std::sort(cuts.begin(), cuts.end(),
              [a0=current.second](const CutPair& cut0, const CutPair& cut1){
                using namespace RS_Math;
                return getAngleDifference(a0, cut0.second) < getAngleDifference(a0, cut1.second);
    });
    return cuts.front().first;
}

//------------------------------------------------------------------------------------//
bool LoopExtractor::findNext() const
{
    std::vector<RS_Entity*> connected = getConnected();
    switch (connected.size()) {
    case 0:
        LC_ERR << __func__
               << "(): disconnected at point: ("<<m_data->vertex.x<<", "<<m_data->vertex.y<<")";
        return false;
    case 1:
        m_data->current = connected.front();
        break;
    default:
    {
        m_data->current = findOutermost(connected);
    }
    }
    m_data->vertex = (m_data->vertex.squaredTo(m_data->current->getStartpoint()) > RS_TOLERANCE) ? m_data->current->getStartpoint() : m_data->current->getEndpoint();
    m_loop->addEntity(m_data->current);
    m_data->edges.removeEntity(m_data->current);
    return true;
}


//------------------------------------------------------------------------------------//
std::vector<std::unique_ptr<RS_EntityContainer>> LoopExtractor::extract() {
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    LC_LOG<<__func__<<"(): begin";

    bool success = true;
    while(success && !m_data->edges.isEmpty()) {
        findFirst();
        while(success && m_data->vertex.squaredTo(m_data->vertexTarget) > RS_TOLERANCE) {
            LC_LOG<<m_data->vertex.x<<", "<< m_data->vertex.y<<" : "<<" : ds2 = "
                 <<m_data->vertex.squaredTo(m_data->vertexTarget);
            LC_LOG<<"id = "<<m_data->current->getId();
            success = findNext();
        }
        if (!validate())
            LC_ERR << __func__<<"(): invalid loop of size = "<<m_loop->count();
        loops.push_back(std::move(m_loop));
    }
    LC_LOG<<__func__<<"(): loops.size() = "<<loops.size();
    return loops;
}

//------------------------------------------------------------------------------------//
bool LoopExtractor::validate() const
{
    if (m_loop == nullptr || m_loop->isEmpty())
        return false;
    auto testEdge = [it = m_loop->begin()](const RS_Entity* entity) mutable {
        return entity != nullptr && isConnected(*entity, **it++);
    };
    switch (m_loop->count())
    {
    case 0:
    case 1:
        // single edge loop is not expected
        return false;
    default:
        return std::all_of(m_loop->begin() + 1, m_loop->end(), testEdge);
    }
}

//------------------------------------------------------------------------------------//
struct LoopSorter::AreaPredicate {
    AreaPredicate(const LoopSorter& sorter);
    bool operator () (const RS_EntityContainer* lhs, const RS_EntityContainer* rhs) const;
    const LoopSorter& m_sorter;
};

//------------------------------------------------------------------------------------//
struct LoopSorter::Data {
    Data(LoopSorter* sorter, std::vector<std::unique_ptr<RS_EntityContainer>> loops):
        loops{std::move(loops)}
      , area{findAreas(this->loops)}
      , areaComparison{*sorter}
      , toProcess{areaComparison}
    {}

    // hold input loops
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    // lookup table to find enclosed area of each loop
    std::unordered_map<const RS_EntityContainer*, double> area;
    // compare loops by their enclosed areas
    // The area of any ancestor loop is larger than the child loop.
    // Always find ancestors using the loop with the smallest unprocessed loop.
    // For each loop, only need to find its parent once.
    LoopSorter::AreaPredicate areaComparison;
    // Candidate loops, sorted by their enclosed areas
    std::multiset<RS_EntityContainer*, LoopSorter::AreaPredicate> toProcess;
    // lookup table for parent loops
    std::unordered_map<RS_EntityContainer*, RS_EntityContainer*> parents;
};

//------------------------------------------------------------------------------------//
LoopSorter::AreaPredicate::AreaPredicate(const LoopSorter &sorter):
    m_sorter{sorter}
{}

//------------------------------------------------------------------------------------//
bool LoopSorter::AreaPredicate::operator ()(const RS_EntityContainer* lhs, const RS_EntityContainer* rhs) const
{
    return m_sorter.m_data->area.at(lhs) + RS_TOLERANCE < m_sorter.m_data->area.at(rhs);
}

//------------------------------------------------------------------------------------//
LoopSorter::LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops):
    m_data{std::make_unique<LoopSorter::Data>(this, std::move(loops))}
{
    init();
}

//------------------------------------------------------------------------------------//
LoopSorter::~LoopSorter() = default;

//------------------------------------------------------------------------------------//
void LoopSorter::init()
{
    for(const auto& loop: m_data->loops)
        m_data->toProcess.insert(loop.get());
    std::vector<RS_EntityContainer*> loops{m_data->toProcess.begin(), m_data->toProcess.end()};

    for (RS_EntityContainer* loop : loops)
        findAncestors(loop);
}

//------------------------------------------------------------------------------------//
void LoopSorter::findAncestors(RS_EntityContainer* loop)
{
    if (m_data->toProcess.erase(loop) == 0)
        return;

    // use a random direction to avoid passing tangential directions
    // TODO, to complete avoid tangential
    auto ray = getRandomRay(loop);
    // sorting by floating points is okay, the loops shouldn't be close to each other, with exception
    // of touching points
    std::map<double, RS_EntityContainer*> ancestors;
    for(RS_EntityContainer* candidate: m_data->toProcess) {
        if (candidate == loop)
            continue;
        RS_VectorSolutions intersections = getIntersection(*ray, *candidate);
        if (intersections.size()%2 == 0)
            continue;
        // a parent loop has odd intersections for a ray starting from an inner point
        double distance = intersections.getClosestDistance(ray->getStartpoint());
        ancestors.emplace(distance, candidate);
    }
    // ancestors found: from innermost to outermost
    RS_EntityContainer* current = loop;
    for (const auto& entry: ancestors)
    {
        RS_EntityContainer* parent = entry.second;
        m_data->toProcess.erase(parent);
        if (m_data->parents.count(current) == 1)
            break;
        m_data->parents[current] = parent;
        parent->addEntity(current);
        current = parent;
    }
}


//------------------------------------------------------------------------------------//
std::vector<RS_EntityContainer*> LoopSorter::getResults() const
{
    std::set<RS_EntityContainer*> roots;
    for (const auto& loop: m_data->loops)
        if (m_data->parents.count(loop.get()) == 0)
            roots.insert(loop.get());
    return {roots.begin(), roots.end()};
}
} // namespace LC_LoopUtils
