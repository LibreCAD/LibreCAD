#include <algorithm>
#include <array>
#include <random>
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
std::default_random_engine randomEngine;


} // namespace

namespace LC_LoopUtils {

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

struct ComparePoints {
    bool operator()(const RS_Vector &p0, const RS_Vector &p1) const {
        if (p0.x + RS_TOLERANCE < p1.x)
            return true;
        else if (p0.x > p1.x + RS_TOLERANCE)
            return false;
        return p0.y + RS_TOLERANCE < p1.y;
    }
    bool operator()(const std::pair<RS_Vector, RS_Entity*> &p0, const std::pair<RS_Vector, RS_Entity*> &p1) const {
        return (*this)(p0.first, p1.first);
    }
};
struct CompareDistance {
    CompareDistance(const RS_Vector& point):
        m_point{point}
    {}

    bool operator() (const RS_Vector &p0, const RS_Vector &p1) const {
        return p0.squaredTo(m_point) < p1.squaredTo(m_point);
    }

    const RS_Vector& m_point;
};

RS_VectorSolutions getIntersection(RS_AtomicEntity *line,
                                   const RS_EntityContainer &loop) {
    RS_VectorSolutions ret;
    for (RS_Entity *entity : loop) {
        if (entity != nullptr && entity->isEdge()) {
            auto intersections = RS_Information::getIntersection(line, entity, true);
            ret.push_back(intersections);
        }
    }
    return ret;
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

std::vector<RS_Entity*> LoopExtractor::getConnected() const
{
    std::vector<RS_Entity *> connected;
    std::copy_if(m_edges.begin(), m_edges.end(), std::back_inserter(connected),
                 [vertex = m_data->vertex, current = m_data->current](const RS_Entity *e) {
        if (e == current)
            return false;
        double dist = RS_MAXDOUBLE;
        e->getNearestEndpoint(vertex, &dist);
        return dist < 1E-7;
    });
    return connected;
}

LoopExtractor::~LoopExtractor() = default;

RS_Vector LoopExtractor::getInternalPoint() const
{
    RS_Vector p0 =m_data->current->getMiddlePoint();
    for(short i = 0; i < 16; ++i) {
        RS_Vector offset = m_data->current->getTangentDirection(p0).rotate(M_PI/4 +getRandomAngle()/8.);
        offset *= m_edges.getSize().magnitude()/offset.magnitude();
        auto line = std::make_unique<RS_Line>(nullptr, p0 - offset, p0 + offset);
        auto results = getIntersection(line.get(), m_edges);
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

} // namespace LC_LoopUtils
