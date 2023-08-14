#ifndef LC_LOOPUTILS_H
#define LC_LOOPUTILS_H

#include <memory>
#include <vector>

class RS_AtomicEntity;
class RS_Entity;
class RS_EntityContainer;
class RS_Line;
class RS_Vector;
class RS_VectorSolutions;

/**
 * @brief the name space contains utils to analyze contour topology.
 * The initial motivation is to allow proper calculation of hatched areas. For example, the hatched areas
 * contain holes(say, denoted as lakes), and the lakes may further contain islands of their own. The hatched
 * area calculation should find the total area of all land including any islands in lakes.
 */
namespace LC_LoopUtils {

/**
 * @brief isEnclosed - whether the entity is enclosed in the loop. It's assumed the entity shouldn't intersect
 * with the loop.
 * @param loop - a simple closed contour
 * @param RS_AtomicEntity& entity - an entity
 * @return bool - true, an entity
 */
bool isEnclosed(RS_EntityContainer& loop, RS_AtomicEntity& entity);

/**
 * @brief The LoopExtractor class, to extract closed loops from edges.
 * The algorithm：
 * 0. Move label all edges as unprocessed
 * 1. Find an edge closest to the bounding box of all edges;
 * 2. Set the start point of the edge as the target point;
 * 3. Label the edge as processed;
 * 4. Find internal point of the edge;
 * 5. From the end point of the current edge, find all unprocessed edges connected to this point;
 * 6. From the end point draw a small circle, find intersections between the circle and edges(the current, and connected);
 * 7. Find the direction angle from the circle center to the intersections. Add another angle from the end point to the
 * internal point from Step 4.;
 * 8. Sort the angles, find the direction right next to the direction of the current edge;
 * 9. Choose the edge which has an angle right next to the current edge's angle;
 * 10. Repeat steps 2- 19, until the target point set in Step 2 to find a closed loop;
 * 11. Repeat Steps 1-10, untill all edges are processed.
 *
 * The algorithm has the following assumptions:

 * 1. Contours are closed loops, so each edge has its start/end points connected to other edges;
 * 2. Each loop is simply closed with number of edges equals the number of vertices (Shared endpoints),
 * i.e. Euler characteristic 0;
 * 3. Full circles/ellipses are accepted as individual closed contours;
 * 4. No self-intersection among contours; i.e. no edge crosses another edge;
 * 5. Multiple edges are allowed to be connected at a single point;
 * 6. Closed contours may share edge endpoints, but no edge is shared by more than one contours.
 */
class LoopExtractor {
public:
    LoopExtractor(RS_EntityContainer& edges);
    ~LoopExtractor();

    /**
     * @brief extract - extract loops from connected edges
     * @return std::vector<std::unique_ptr<RS_EntityContainer>> - loops. each element is simply closed
     */
    std::vector<std::unique_ptr<RS_EntityContainer>> extract();
private:
    RS_Entity* findFirst() const;
    bool findNext() const;
    std::vector<RS_Entity*> getConnected() const;
    RS_Entity* findOutermost(std::vector<RS_Entity*> edges) const;
    mutable std::unique_ptr<RS_EntityContainer> m_loop;
    mutable std::vector<std::unique_ptr<RS_EntityContainer>> m_loops;
    RS_Vector getInternalPoint() const;
    struct LoopData;
    std::unique_ptr<LoopData> m_data;

    RS_EntityContainer& m_edges;
    double m_size;
};


/**
 * @brief The LoopSorter class - find topologic relations of loops
 * The input loops must not cross each other; no edge is shared among loops.
 * Tangential edges are allowed.
 */
class LoopSorter {
public:
    /**
     * Each input loop is assumed to be a simple closed loop, and contains only edges.
     * The input loops should not contain sub-loops
     * Ownership of the input loops is transferred to this LoopSorter
     * @param std::vector<std::unique_ptr<RS_EntityContainer>> loops input loops
     */
    LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops);
    ~LoopSorter();

    struct AreaPredicate;

    /**
     * @brief getResults - the sorting results
     * @return std::vector<RS_EntityContainer*> - the top level loops, i.e. outermost loops, after sorting
     * each inner loop is added as a child of its immediate parent loop.
     */
    std::vector<RS_EntityContainer*> getResults() const;

private:
    void init();
    // find all ancestor loops of a given loop
    void findAncestors(RS_EntityContainer* loop);

    struct Data;
    std::unique_ptr<Data> m_data;
};

}

#endif // LC_LOOPUTILS_H
