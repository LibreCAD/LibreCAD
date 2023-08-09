#include <algorithm>
#include <array>
#include <random>
#include <vector>

#include "lc_looputils.h"
#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_vector.h"

namespace {
std::default_random_engine randomEngine;

std::vector<RS_Entity *> getConnected(RS_EntityContainer &ec,
                                      const RS_Vector &point) {
  std::vector<RS_Entity *> connected;
  std::copy_if(ec.begin(), ec.end(), std::back_inserter(connected),
               [&point](const RS_Entity *e) {
                 double dist = RS_MAXDOUBLE;
                 e->getNearestEndpoint(point, &dist);
                 return dist < 1E-7;
               });
  return connected;
}

} // namespace

namespace LC_LoopUtils {

// a random angle between 0 and 2 pi
double getRandomAngle() {
  static std::uniform_real_distribution<double> uniformDistribution(0.0,
                                                                    2. * M_PI);
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
};

RS_VectorSolutions getIntersection(RS_AtomicEntity *line,
                                   const RS_EntityContainer &loop) {
  RS_VectorSolutions ret;
  for (RS_Entity *entity : loop) {
    if (entity && entity->isEdge()) {
      auto intersections = RS_Information::getIntersection(line, entity, true);
      ret.push_back(intersections);
    }
  }
  return ret;
}

RS_Vector getInternalPoint(RS_EntityContainer &loop) {
  RS_Vector p0 =
      loop.firstEntity()->getNearestPointOnEntity(loop.getMin(), true);
  double size = loop.count();
  for (short i = 0; i < 16; i++) {
    RS_Vector offset = RS_Vector(getRandomAngle()) * size;
    auto line = std::make_unique<RS_Line>(nullptr, p0 - offset, p0 + offset);
    auto results = getIntersection(line.get(), loop);
    // need even number of intersections
    if (results.empty() || results.size() % 2 == 1)
      continue;
    std::sort(results.begin(), results.end(), ComparePoints{});
    // find an internal point
    return (results.at(0) + results.at(1)) * 0.5;
  }
  LC_LOG << __func__
         << "(): failed to find a line passing the loop: " << loop.getId();
  return RS_Vector{false};
}

bool isEnclosed(RS_EntityContainer &loop, RS_Entity &entity) {
  RS_Vector internal = getInternalPoint(loop);

  static std::uniform_int_distribution<> uniformDistribution(1, 100);
  int i = uniformDistribution(randomEngine);
  RS_Vector point = entity.getNearestMiddle(internal, nullptr, i);
  auto line = std::make_unique<RS_Line>(nullptr, internal, point);
  auto results = getIntersection(line.get(), loop);
  return results.size() % 2 == 0;
}

std::unique_ptr<RS_Line> getRandomRay(RS_EntityContainer &loop) {
  RS_Vector p0 = getInternalPoint(loop);
  double size = loop.count();
  return std::make_unique<RS_Line>(nullptr, p0,
                                   p0 + RS_Vector{getRandomAngle()} * size);
}

LoopExtractor::LoopExtractor(RS_EntityContainer &edges) : m_edges{edges} {}

std::unique_ptr<RS_EntityContainer> LoopExtractor::extract() {

  auto loop = std::make_unique<RS_EntityContainer>(nullptr, false);
  while (!m_edges.isEmpty()) {
    RS_Entity *e = m_edges.firstEntity();
    m_edges.removeEntity(e);
    RS_Vector target = e->getStartpoint();
    loop->addEntity(e);
    RS_Vector endPoint = e->getEndpoint();
    while (endPoint.squaredTo(target) > RS_TOLERANCE && !m_edges.isEmpty()) {
      double distance = 0.;
      RS_Entity *next = nullptr;
      RS_Vector startPoint =
          m_edges.getNearestEndpoint(endPoint, &distance, &next);
      std::array<RS_Vector, 2> points{next->getStartpoint(),
                                      next->getEndpoint()};
      if (startPoint.squaredTo(points[1]) < RS_TOLERANCE15)
        std::swap(points[0], points[1]);
      loop->addEntity(next);
      m_edges.removeEntity(next);
      endPoint = points[1];
    }
    if (endPoint.squaredTo(target) < RS_TOLERANCE)
      return loop;
  }
  return loop;
}

} // namespace LC_LoopUtils
