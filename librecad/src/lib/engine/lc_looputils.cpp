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
#include "rs_vector.h"

namespace {

constexpr double contourGapTolerance = 1E-7;

// a random angle between 0 and 2 pi
double getRandomAngle();

// a random number in [0, 1]
double getRandom();

// Create a random ray: starting from an internal point of the loop
std::unique_ptr<RS_Line> getRandomRay(RS_EntityContainer* loop);

// Find intersection between a line and a loop
RS_VectorSolutions getIntersection(const RS_Entity& line, const RS_EntityContainer& loop);

// Build a loop to loop area lookup table
std::unordered_map<const RS_EntityContainer*, double> findAreas(const std::vector<std::unique_ptr<RS_EntityContainer>>& loops );

struct ComparePoints {
  bool operator()(const RS_Vector &p0, const RS_Vector &p1) const;
};

// randomEngine
std::default_random_engine randomEngine;

// ------------------------------------------------------------------------------------- //
// a random angle between 0 and 2 pi
double getRandomAngle() {
    static std::uniform_real_distribution<double> uniformDistribution(0.0,
                                                                      2. * M_PI);
    return uniformDistribution(randomEngine);
}

double getRandom() {
    static std::uniform_real_distribution<double> uniformDistribution(0.0, 1.);
    return uniformDistribution(randomEngine);
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
    RS_Vector vertex;
    RS_Vector vertexTarget;
    RS_Vector internalPoint;
    RS_Entity* current = nullptr;
};

LoopExtractor::LoopExtractor(RS_EntityContainer &edges) :
    m_data{std::make_unique<LoopData>()}
    , m_edges{edges}
{
    assert(!edges.isEmpty());
}

//------------------------------------------------------------------------------------//
std::vector<RS_Entity*> LoopExtractor::getConnected() const
{
    std::vector<RS_Entity *> connected;
    std::copy_if(m_edges.begin(), m_edges.end(), std::back_inserter(connected),
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
RS_Vector LoopExtractor::getInternalPoint() const
{
    RS_Vector p0 =m_data->current->getMiddlePoint();
    for(short i = 0; i < 16; ++i) {
        RS_Vector offset = m_data->current->getTangentDirection(p0).rotate(M_PI/4 +getRandomAngle()/8.);
        offset *= m_edges.getSize().magnitude()/offset.magnitude();
        auto line = RS_Line{p0 - offset, p0 + offset};
        auto results = getIntersection(line, m_edges);
        // need even number of intersections
        if (results.empty() || results.size() % 2 == 1)
            continue;
        std::sort(results.begin(), results.end(), CompareDistance{p0});
        // find an internal point
        const double mixFactor = 0.1 + 0.8 * getRandom();
        return results.at(0) * mixFactor + results.at(1) * (1.0 - mixFactor);
    }
    LC_LOG << __func__
           << "(): failed: "<<__func__<<"(): failed";
    return RS_Vector{false};
}


//------------------------------------------------------------------------------------//
RS_Entity* LoopExtractor::findFirst() const
{
    RS_Entity* first = nullptr;
    RS_Vector p0 = m_edges.getMin();
    double dist=RS_MAXDOUBLE;
    for(RS_Entity* edge: m_edges)
    {
        double dist2=RS_MAXDOUBLE;
        edge->getNearestPointOnEntity(p0, true, &dist2);
        if (dist2 < dist) {
            first = edge;
            dist = dist2;
        }
    }

    m_data->vertex = first->getEndpoint();
    m_data->vertexTarget = first->getStartpoint();
    m_data->current = first;
    m_loop = std::make_unique<RS_EntityContainer>(nullptr, false);
    m_loop->addEntity(m_data->current);
    m_data->internalPoint = getInternalPoint();
    m_edges.removeEntity(first);
    return first;
}

//------------------------------------------------------------------------------------//
bool LoopExtractor::isOutermost(RS_Entity* edge) const
{
    assert(edge != nullptr);
    RS_Vector middle = edge->getMiddlePoint();
    RS_Vector direction = (middle - m_data->internalPoint).normalize();
    auto line = std::make_unique<RS_Line>(middle, middle + direction * m_edges.getSize().magnitude());
    auto hasIntersection = [l=line.get(), edge](RS_Entity* e) {
        return e != edge && RS_Information::getIntersection(l, e, true).size() % 2 == 0;
    };
    return std::none_of(m_edges.begin(), m_edges.end(), hasIntersection);
}


//------------------------------------------------------------------------------------//
RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const
{
    assert(edges.size() >= 2);
    double edgeLength = RS_MAXDOUBLE;
    for(RS_Entity* edge: edges)
        edgeLength = std::min({edgeLength, edge->getLength(), edge->getStartpoint().distanceTo(edge->getEndpoint())});

    RS_Circle circle{nullptr, {m_data->vertex, edgeLength * 0.01}};
    auto getCut = [&circle, p0 = m_data->vertex](RS_Entity* edge){
        RS_VectorSolutions sol = RS_Information::getIntersection(&circle, edge, true);
        assert(!sol.empty());
        return std::make_pair(edge, p0.angleTo(sol.at(0)));
    };
    using CutPair = std::pair<RS_Entity*, double>;
    std::vector<CutPair> cuts{{{nullptr, m_data->vertex.angleTo(m_data->internalPoint)},
                                                       getCut(m_data->current)}};
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(cuts), getCut);
    std::sort(cuts.begin(), cuts.end(),
              [](const CutPair& cut0, const CutPair& cut1){
                return cut0.second < cut1.second;
    });
    size_t index = 0;
    while(index < cuts.size() && cuts[index].first != m_data->current) index++;
    RS_Entity* e2[2] = {cuts[(index + cuts.size() - 1)%cuts.size()].first, cuts[(index + cuts.size() + 1)%cuts.size()].first};
    return (e2[0] == nullptr) ? e2[1] : e2[0];
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
    m_edges.removeEntity(m_data->current);
    return true;
}


//------------------------------------------------------------------------------------//
std::vector<std::unique_ptr<RS_EntityContainer>> LoopExtractor::extract() {
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    LC_LOG<<__func__<<"(): begin";

    bool success = true;
    while(success && !m_edges.isEmpty()) {
        LC_ERR<<"0: size="<<m_edges.count();
        findFirst();
        while(m_data->vertex.squaredTo(m_data->vertexTarget) > RS_TOLERANCE) {
            LC_LOG<<m_data->vertex.x<<", "<< m_data->vertex.y<<" : "<<" : ds2 = "
                 <<m_data->vertex.squaredTo(m_data->vertexTarget);
            LC_LOG<<"id = "<<m_data->current->getId();
            success = findNext();
        }
        LC_LOG<<"1: loop.size() = "<<m_loop->count()<<": size="<<m_edges.count();
        loops.push_back(std::move(m_loop));
    }
    LC_LOG<<__func__<<"(): loops.size() = "<<loops.size();
    //if (loops.size() == 2)
     //   loops.pop_back();
    return loops;
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
