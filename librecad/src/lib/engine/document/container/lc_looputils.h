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
// File: lc_looputils.h
#ifndef LC_LOOPUTILS_H
#define LC_LOOPUTILS_H

#include <memory>
#include <vector>

class QPainterPath;
class RS_AtomicEntity;
class RS_Entity;
class RS_EntityContainer;
class RS_Pattern;
class RS_Vector;
class RS_VectorSolutions;

namespace lc {
namespace geo {
class Area;
}
}
using LC_Rect = lc::geo::Area;

/**
 * @brief The namespace contains utilities to analyze contour topology.
 * The initial motivation is to allow proper calculation of hatched areas. For example, the hatched areas
 * contain holes (say, denoted as lakes), and the lakes may further contain islands of their own. The hatched
 * area calculation should find the total area of all land including any islands in lakes.
 */
namespace LC_LoopUtils {

/**
 * @brief The LC_Loops class - recursive representation of contour loops with holes.
 * Represents a hierarchical structure for contours, where each loop can have child loops (holes or islands).
 * Supports safe ownership of entities via smart pointers and cloning.
 */
class LC_Loops {
public:
    /**
     * @brief Constructor for LC_Loops.
     * @param loop Shared pointer to the entity container representing the outer loop.
     * @param ownsEntities Flag indicating if this object owns the entities (default: true).
     */
    LC_Loops(std::shared_ptr<RS_EntityContainer> loop = std::make_shared<RS_EntityContainer>(), bool ownsEntities = true);
    ~LC_Loops();

    /**
     * @brief Adds a child loop to this loop's hierarchy.
     * @param child The child LC_Loops to add.
     */
    void addChild(LC_Loops child);
    /**
     * @brief Adds an entity to the outer loop.
     * @param entity Pointer to the entity to add.
     */
    void addEntity(RS_Entity* entity);
    /**
     * @brief Checks if a point is inside this loop (considering odd-even rule for hierarchy).
     * @param point The point to test.
     * @return True if inside.
     */
    bool isInside(const RS_Vector& point) const;
    /**
     * @brief Computes the containing depth of a point in the hierarchy.
     * @param point The point to test.
     * @return The winding depth (odd means inside).
     */
    int getContainingDepth(const RS_Vector& point) const;
    /**
     * @brief Generates a QPainterPath for this loop and its children up to the specified level.
     * @param level Recursion level for path building (default: 0).
     * @return The combined QPainterPath with OddEvenFill rule.
     */
    QPainterPath getPainterPath(int level = 0) const;
    /**
     * @brief Trims pattern entities to the boundaries of this loop hierarchy.
     * @param pattern The pattern to trim.
     * @return Unique pointer to a container of trimmed entities.
     */
    std::unique_ptr<RS_EntityContainer> trimPatternEntities(const RS_Pattern& pattern) const;
    /**
     * @brief Computes the total area of this loop, adding islands and subtracting holes.
     * @return The net area as a double.
     */
    double getTotalArea() const;
    /**
     * @brief Checks if this loop overlaps with a given rectangle.
     * @param other The rectangle to check against.
     * @return True if overlap exists.
     */
    bool overlap(const LC_Rect& other) const;
    /**
     * @brief Gets the outer loop container.
     * @return Shared pointer to the outer RS_EntityContainer.
     */
    std::shared_ptr<RS_EntityContainer> getOuterLoop() const {
        return m_loop;
    }

    /**
     * @brief Gets the child loops.
     * @return Const reference to the vector of children.
     */
    const std::vector<LC_Loops>& getChildren() const {
        return m_children;
    }

private:
    /**
     * @brief Checks if a point is inside the outer loop only (non-recursive).
     * @param point The point to test.
     * @return True if inside the outer contour.
     */
    bool isInsideOuter(const RS_Vector& point) const;
    /**
     * @brief Builds a QPainterPath from the entities in a container.
     * @param cont The entity container.
     * @return The resulting path.
     */
    QPainterPath buildPathFromLoop(const RS_EntityContainer& cont) const;
    /**
     * @brief Collects all descendant loops recursively.
     * @param loops Output vector of loop pointers.
     */
    void getAllLoops(std::vector<const RS_EntityContainer*>& loops) const;
    /**
     * @brief Computes the bounding box of the outer loop.
     * @return The LC_Rect bounding box.
     */
    LC_Rect getBoundingBox() const;
    /**
     * @brief Alias for isInside (odd-even rule).
     * @param p The point.
     * @return True if inside.
     */
    bool isPointInside(const RS_Vector& p) const;
    /**
     * @brief Parametric point on ellipse.
     * @param center Ellipse center.
     * @param major Major radius.
     * @param minor Minor radius.
     * @param rot Rotation angle.
     * @param t Parameter.
     * @return The point vector.
     */
    RS_Vector e_point(const RS_Vector& center, double major, double minor, double rot, double t) const;
    /**
     * @brief Derivative (tangent) for ellipse parametric.
     * @param major Major radius.
     * @param minor Minor radius.
     * @param rot Rotation.
     * @param t Parameter.
     * @return Tangent vector.
     */
    RS_Vector e_prime(double major, double minor, double rot, double t) const;
    /**
     * @brief Adds an elliptic arc to a QPainterPath using cubic Beziers.
     * @param path The path to append to.
     * @param center Center.
     * @param major Major radius.
     * @param minor Minor radius.
     * @param rot Rotation.
     * @param a1 Start angle.
     * @param a2 End angle.
     */
    void addEllipticArc(QPainterPath& path, const RS_Vector& center, double major, double minor, double rot, double a1, double a2) const;
    /**
     * @brief Collects all atomic boundary entities from this hierarchy.
     * @return Vector of RS_Entity pointers.
     */
    std::vector<RS_Entity*> getAllBoundaries() const;
    /**
     * @brief Sorts intersection points along an entity by parameter.
     * @param e The entity.
     * @param inters Unsorted intersections.
     * @return Sorted vector of points.
     */
    std::vector<RS_Vector> sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const;
    /**
     * @brief Creates a sub-entity between two points.
     * @param e Original entity.
     * @param p1 Start point.
     * @param p2 End point.
     * @return New entity or nullptr.
     */
    RS_Entity* createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const;
    /**
     * @brief Checks if an entity is closed (full loop).
     * @param e The entity.
     * @return True if closed.
     */
    bool isEntityClosed(const RS_Entity* e) const;
    /**
     * @brief Creates tile offsets for a pattern within the bounding box.
     * @param pattern The pattern.
     * @return Vector of tile offsets.
     */
    std::vector<RS_Vector> createTiles(const RS_Pattern& pattern) const;

    std::shared_ptr<RS_EntityContainer> m_loop;  ///< Outer loop container
    std::vector<LC_Loops> m_children;             ///< Child loops (holes/islands)
};

/**
 * @brief The LoopExtractor class, to extract closed loops from edges.
 * Processes a container of edges to form closed loops, handling connectivity and orientation.
 *
 *  * The algorithm:
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
     * @brief Constructor for LoopExtractor.
     * @param edges Container of edges to process.
     */
    LoopExtractor(const RS_EntityContainer& edges);
    ~LoopExtractor();

    /**
     * @brief Extracts all closed loops from the edges.
     * @return Vector of unique_ptr to loop containers.
     */
    std::vector<std::unique_ptr<RS_EntityContainer>> extract();

private:
    // validate the loop m_loop
    /**
     * @brief Validates the current loop for closure.
     * @return True if valid.
     */
    bool validate() const;

    /**
     * @brief Finds the first edge to start a loop.
     * @return Starting entity or nullptr.
     */
    RS_Entity* findFirst() const;

    /**
     * @brief Finds the next connected edge in the loop.
     * @return True if found.
     */
    bool findNext() const;

    /**
     * @brief Gets entities connected to the current endpoint.
     * @return Vector of connected entities.
     */
    std::vector<RS_Entity*> getConnected() const;

    /**
     * @brief Selects the outermost (preferred direction) from connected edges.
     * @param edges Connected edges.
     * @return Selected entity.
     */
    RS_Entity* findOutermost(std::vector<RS_Entity*> edges) const;

    // Tolerance for endpoint matching
    static constexpr double ENDPOINT_TOLERANCE = 1e-10;

    mutable std::unique_ptr<RS_EntityContainer> m_loop;  ///< Current loop being built

    struct LoopData;
    std::unique_ptr<LoopData> m_data;  ///< Private data for extraction state
};

/**
 * @brief The LoopSorter class - find topologic relations of loops
 * Sorts loops by containment to build a hierarchy, filtering degenerates and assigning parents.
 *
 * Each input loop is assumed to be a simple closed loop, and contains only edges.
 * The input loops should not contain sub-loops

 */
class LoopSorter {
public:
    LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops);
    ~LoopSorter();

    struct AreaPredicate;  // Sorts by ascending absolute area (small to large), ties by bounding box diagonal

    /**
     * @brief Gets the sorted hierarchical results.
     * @return Shared pointer to vector of LC_Loops.
     */
    std::shared_ptr<std::vector<LC_Loops>> getResults() const;

private:
    /**
     * @brief Sorts loops and builds the hierarchy forest.
     */
    void sortAndBuild();
    /**
     * @brief Initializes data structures.
     */
    void init();

    /**
     * @brief Finds and assigns the parent for a loop.
     * @param loop The child loop.
     * @param sorted Sorted list of all loops.
     */
    void findParent(RS_EntityContainer* loop, const std::vector<RS_EntityContainer*>& sorted);
    /**
     * @brief Converts a forest of containers to LC_Loops trees.
     * @param forest Vector of root containers.
     * @return Shared vector of LC_Loops.
     */
    std::shared_ptr<std::vector<LC_Loops>> forestToLoops(std::vector<RS_EntityContainer*> forest) const;

    struct Data;
    std::unique_ptr<Data> m_data;  ///< Private data for sorting state
};

/**
 * @brief The LoopOptimizer class - separate a collection of contours into loops
 * High-level class to extract, sort, and build hierarchical loops from contours.
 */
class LoopOptimizer {
public:
    LoopOptimizer(const RS_EntityContainer& contour);
    ~LoopOptimizer();

    /**
     * @brief Gets the optimized hierarchical results.
     * @return Shared pointer to vector of LC_Loops.
     */
    std::shared_ptr<std::vector<LC_Loops>> GetResults() const;

private:
    /**
     * @brief Adds and processes a contour container.
     * @param contour The input contour.
     */
    void AddContainer(const RS_EntityContainer& contour);

    struct Data;
    std::shared_ptr<Data> m_data;  ///< Private data for optimization results
};
}

#endif // LC_LOOPUTILS_H
