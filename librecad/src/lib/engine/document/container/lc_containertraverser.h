/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2025 librecad (www.librecad.org)
** Copyright (C) 2025 dxli (github.com/dxli)
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
**********************************************************************************/

#ifndef LC_ContainerTraverser_H
#define LC_ContainerTraverser_H

#include <memory>
#include <vector>

class RS_Entity;
class RS_EntityContainer;

namespace RS2 {
    enum ResolveLevel: unsigned short;
}
namespace lc {

/**
 * @brief The LC_ContainerTraverser class
 *        This is an implementation of RS_EntityContainer traversing methods to
 *        avoid using member attributes of the container.
 *        Use the traverser instead of RS_EntityContainer methods:
 *   RS_Entity* firstEntity(RS2::ResolveLevel) const;
 *   RS_Entity* lastEntity(RS2::ResolveLevel) const;
 *   RS_Entity* nextEntity(RS2::ResolveLevel) const;
 *   RS_Entity* prevEntity(RS2::ResolveLevel) const;
 *
 *   The deprecated way to traverse a container:
 *     for(auto* entity=container.firstEntity(level); entity; entity=container.nextEntity());
 *
 *   The recommended way:
 *     LC_ContainerTraverser traverser{container, level};
 *     for(auto* entity=traverser.first(); entity; entity=traverser.next());
 *   or,
 *     for(auto* entity: LC_ContainerTraverser{container, level}.entities());
 *   The difference between those two new methods is whether to generate a list
 *   of container entities before looping through them. The difference is only
 *   important, if traverse must be customized based on the current entity.
 * @author Dongxu Li
 */
class LC_ContainerTraverser {
public:

    // The direction of traversing
    enum class Direction: short
    {
        Forward = 0,
        Backword = 1
    };
    LC_ContainerTraverser(const RS_EntityContainer& container,
                          RS2::ResolveLevel level,
                          LC_ContainerTraverser::Direction direction = Direction::Forward);
    ~LC_ContainerTraverser();

    /**
     * @brief first() - intended to replace RS_Container::firstEntity()
     * @return - the first entity in the container
     */
    RS_Entity* first();
    /**
     * @brief last() - intended to replace RS_Container::lastEntity()
     * @return - the last entity
     */
    RS_Entity* last();
    /**
     * @brief next() - intended to replace RS_Container::nextEntity()
     * @return - the next entity
     */
    RS_Entity* next();
    /**
     * @brief prev - the entity traversed before the current one
     *               This is NOT the same as RS_Container::prevEntity()
     * @return the previous entity traversed
     */
    RS_Entity* prev();
    std::vector<RS_Entity*> entities();


private:
    // One traverse by one step
    RS_Entity* get();
    // Collect entities in the container by the DFS order
    void collect(std::vector<RS_Entity*>& items, const RS_EntityContainer* container) const;
    // for forward/backward support
    size_t currentIndex() const;
    struct Data;
    std::unique_ptr<LC_ContainerTraverser::Data> m_pImp;
    LC_ContainerTraverser::Direction m_direction = Direction::Forward;
};
}
#endif // LC_ContainerTraverser_H


