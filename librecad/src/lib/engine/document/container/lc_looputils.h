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
 * IMPROVED: Enhanced for ownership safety (cloning entities), robust sorting (absolute areas, degenerates filtered),
 * and correct immediate parent assignment in sorting to fix nested hatching issues.
 */
namespace LC_LoopUtils {
class LC_Loops;
LC_Loops buildLC_Loops(const RS_EntityContainer* cont, const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops);

/**
 * @brief isEnclosed - whether the entity is enclosed in the loop. It's assumed the entity shouldn't intersect
 * with the loop.
 * @param loop - a simple closed contour
 * @param entity - an atomic entity
 * @return bool - true if the entity is enclosed
 */
bool isEnclosed(RS_EntityContainer& loop, RS_AtomicEntity& entity);

/**
 * @brief The LoopExtractor class, to extract closed loops from edges.
 * IMPROVED: Added handling for degenerate edges (zero length) and improved outermost selection with tolerance.
 * ...
 */ // (rest of docstring unchanged)

class LoopExtractor {

public:
    /**
     * @brief LoopExtractor constructor
     * @param edges - edges to process
     */
    LoopExtractor(const RS_EntityContainer& edges);
    ~LoopExtractor();

    std::vector<std::unique_ptr<RS_EntityContainer>> extract();
private:
    // validate the loop m_loop
    bool validate() const;

    RS_Entity* findFirst() const;

    bool findNext() const;

    std::vector<RS_Entity*> getConnected() const;

    RS_Entity* findOutermost(std::vector<RS_Entity*> edges) const;

    // IMPROVED: Added tolerance for endpoint matching
    static constexpr double ENDPOINT_TOLERANCE = 1e-10;

    mutable std::unique_ptr<RS_EntityContainer> m_loop;

    struct LoopData;
    std::unique_ptr<LoopData> m_data;
};

/**
 * @brief The LoopSorter class - find topologic relations of loops
 * IMPROVED: Sorts by absolute area for robustness; assigns immediate (smallest) parents to fix nested hierarchies.
 * Filters zero-area degenerates. Decoupled hierarchy building from parent setting.
 * ...
 */ // (rest of docstring unchanged)

class LoopSorter {
public:
    LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops);
    ~LoopSorter();

    struct AreaPredicate; // IMPROVED: Now uses abs(area) and secondary sort

    std::shared_ptr<std::vector<LC_Loops>> getResults() const;

private:
    void sortAndBuild();
    void init();

    void findParent(RS_EntityContainer* loop, const std::vector<RS_EntityContainer*>& sorted);
    std::shared_ptr<std::vector<LC_Loops>> forestToLoops(std::vector<RS_EntityContainer*> forest) const;

    struct Data;
    std::unique_ptr<Data> m_data;
};

/**
 * @brief The LoopOptimizer class - separate a collection of contours into loops
 * IMPROVED: Removed const_cast; fixed unnecessary initial vector; integrated safe hierarchy building.
 * ...
 */ // (rest of docstring unchanged)

class LoopOptimizer {
public:
    LoopOptimizer(const RS_EntityContainer& contour);
    ~LoopOptimizer();

    std::shared_ptr<std::vector<LC_Loops>> GetResults() const;

private:
    void AddContainer(const RS_EntityContainer& contour);
    LC_Loops buildLC_Loops(const RS_EntityContainer* cont, const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops) const; // IMPROVED: Now used for safe cloning

    struct Data;
    std::shared_ptr<Data> m_data;
};

/**
 * @brief The LC_Loops class - recursive representation of contour loops with holes.
 * IMPROVED: Explicit ownership handling in destructor; cloning support in building.
 * ...
 */ // (rest of docstring unchanged)

class LC_Loops {
public:
    LC_Loops(std::shared_ptr<RS_EntityContainer> loop = std::make_shared<RS_EntityContainer>(), bool ownsEntities = true);
    ~LC_Loops();

    void addChild(LC_Loops child);
    void addEntity(RS_Entity* entity);
    const RS_EntityContainer* loop() const;
    const std::vector<LC_Loops>& children() const;
    bool ownsEntities() const;
    bool isInside(const RS_Vector& point) const;
    int getContainingDepth(const RS_Vector& point) const;
    QPainterPath getPainterPath() const;
    std::vector<RS_Vector> createTiles(const RS_Pattern& pattern) const;
    std::unique_ptr<RS_EntityContainer> trimPatternEntities(const RS_Pattern& pattern) const;
    double getTotalArea() const;
    bool overlap(const LC_Rect& other) const;

private:
    std::shared_ptr<RS_EntityContainer> m_loop;
    std::vector<LC_Loops> m_children;
    bool m_ownsEntities;
    QPainterPath buildPathFromLoop(const RS_EntityContainer& cont) const;
    void getAllLoops(std::vector<const RS_EntityContainer*>& loops) const;
    LC_Rect getBoundingBox() const;
    bool isPointInside(const RS_Vector& p) const;
    RS_Vector e_point(const RS_Vector& center, double major, double minor, double rot, double t) const;
    RS_Vector e_prime(double major, double minor, double rot, double t) const;
    void addEllipticArc(QPainterPath& path, const RS_Vector& center, double major, double minor, double rot, double a1, double a2) const;
    std::vector<RS_Entity*> getAllBoundaries() const;
    std::vector<RS_Vector> sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const;
    RS_Entity* createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const;
    bool is_entity_closed(const RS_Entity* e) const;
};

}

#endif // LC_LOOPUTILS_H
