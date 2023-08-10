#ifndef LC_LOOPUTILS_H
#define LC_LOOPUTILS_H

#include <memory>
#include <vector>

class RS_Entity;
class RS_EntityContainer;
class RS_Line;
class RS_Vector;
class RS_VectorSolutions;

namespace LC_LoopUtils {

// a random angle between 0 and 2 pi
double getRandomAngle();

// find a point inside a given loop
RS_Vector  getInternalPoint(RS_EntityContainer& loop, RS_Entity* edge);

// Create a random ray: starting from an internal point of the loop
std::unique_ptr<RS_Line> getRandomRay(RS_EntityContainer* loop);

// Find intersection between a line and a loop
RS_VectorSolutions getIntersection(RS_Entity* line, const RS_EntityContainer& loop);

/**
 * @brief isEnclosed - whether the entity is enclosed in the loop. It's assumed the entity shouldn't intersect
 * with the loop.
 * @param loop - a simple closed contour
 * @param entity - an entity
 * @return bool - true, an entity
 */
bool isEnclosed(RS_EntityContainer& loop, RS_Entity& entity);

double getSize(const RS_EntityContainer& loop);


class LoopExtractor {
public:
    LoopExtractor(RS_EntityContainer& edges);
    ~LoopExtractor();

    std::vector<std::unique_ptr<RS_EntityContainer>> extract();
private:
    RS_Entity* findFirst() const;
    void findNext() const;
    std::vector<RS_Entity*> getConnected() const;
    bool isOutermost(RS_Entity* edge, const RS_Vector& innerPoint) const;
    RS_Entity* findOutermost(std::vector<RS_Entity*> edges) const;
    mutable std::unique_ptr<RS_EntityContainer> m_loop;
    mutable std::vector<std::unique_ptr<RS_EntityContainer>> m_loops;
    RS_Vector getInternalPoint() const;
    struct LoopData;
    std::unique_ptr<LoopData> m_data;

    RS_EntityContainer& m_edges;
};


}

#endif // LC_LOOPUTILS_H
