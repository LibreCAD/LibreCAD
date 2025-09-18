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
 * The algorithm:
 * 0. Mark all edges as unprocessed.
 * 1. Draw a line crossing the first edge and find intersections with all unprocessed edges; select the edge with the closest intersection to the line start as the first edge on an outermost loop.
 * 2. Set the start or end point as the target point based on direction for counterclockwise traversal.
 * 3. Mark the edge as processed and add to the current loop.
 * 4. From the current end point, find all unprocessed edges connected to it.
 * 5. If one connected edge, use it as next; if multiple, draw a small circle around the end point, find intersections with the current and connected edges.
 * 6. Calculate angles from the end point to these intersections.
 * 7. Sort by the smallest left-turning angle difference from the current edge's angle to find the next outermost edge.
 * 8. Update the current edge and end point.
 * 9. Repeat steps 4-8 until the end point matches the target point, closing the loop.
 * 10. Add the closed loop and repeat steps 1-9 until all edges are processed.
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
    /**
     * @brief LoopExtractor: - LoopExtractor constructor
     * @param edges - edges to process
     */
    LoopExtractor(RS_EntityContainer& edges);
    ~LoopExtractor();

    /**
     * @brief       extract - extract loops from connected edges
     * @return      std::vector<std::unique_ptr<RS_EntityContainer>> - loops. each element is simply closed
     */
    std::vector<std::unique_ptr<RS_EntityContainer>> extract();
private:
    // validate the loop m_loop
    bool validate() const;

    /**
     * @brief:      Find the first edge on an outermost contour of all unprocessed edges; use the edge as the current
     * @returns:    <RS_Entity*> - an edge on an outermost contour
     */
    RS_Entity* findFirst() const;


    /**
     * @brief:      Find another edge connected to be used as the current edge
     * @returns:     bool - true if the edge found closes the contour with the edge from findFirst().
     */
    bool findNext() const;

    /**
     * @brief:      Find all other edges connected to the current end point
     * @returns:    std::vector<RS_Entity*> - all other edges connected to the current end point of the current edge
     */
    std::vector<RS_Entity*> getConnected() const;

    /**
     * @brief findOutermost: From the input edges, find the edge on an outermost contour of all
     *  unprocessed edges.
     * @param edges:         std::vector<RS_Entity*> edges - edges connected to the current end point
     * @returns RS_Entity* - the outermost edge of input edges
     */
    RS_Entity* findOutermost(std::vector<RS_Entity*> edges) const;
    // the edges found for the current loop to form
    mutable std::unique_ptr<RS_EntityContainer> m_loop;

    // The internal data
    struct LoopData;
    std::unique_ptr<LoopData> m_data;
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
     * @param std::vector<std::unique_ptr<RS_EntityContainer>> - loops input loops
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

/**
 * @brief The LoopOptimizer class - separate a collection of contours into loops
 *                                  The results are a two level entityContainer, with each child container
 *                                  as a loop;
 *                                  each closed edge (circles/ellipses) in its own child entityContainer;
 *                                  each other child container contains edge entities ordered following the contour,
 *                                  i.e. the end point of the current edge coincident with the start point of
 *                                  its next edge, and the end point of the last entity is coincident with the
 *                                  start point of the first edge in the loop.
 *                                  All containers have ownership of its contents;
 *                                  All edges are clones.
 */
class LoopOptimizer {
public:
    LoopOptimizer(const RS_EntityContainer& contour);
    ~LoopOptimizer();

    std::shared_ptr<RS_EntityContainer> GetResults() const;

private:
    void AddContainer(const RS_EntityContainer& contour);
    struct Data;
    std::shared_ptr<Data> m_data;

};

}

#endif // LC_LOOPUTILS_H
