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
**********************************************************************************
**/

#include <utility>
#include <vector>

#include "lc_containertraverser.h"

#include "rs.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"


namespace {
struct Node
{
    Node(const RS_EntityContainer* container, size_t index):
        container{container}
        , index{index}
    {}

    const RS_EntityContainer* container = nullptr;
    size_t index = 0;
};

bool isText(const RS_Entity& entity)
{
    switch (entity.rtti()) {
    case RS2::EntityText:
    case RS2::EntityMText:
        return true;
    default:
        return false;
    }
}

} // namespace

namespace lc {

struct LC_ContainerTraverser::Data {
    Data(const RS_EntityContainer& container, RS2::ResolveLevel level):
        container{&container}
        , indices{{&container, size_t(0)}}
        , level{level}
    {
    }

    // whether to traverse into
    bool canResolve(const RS_Entity* entity) const
    {
        if (entity == nullptr || !entity->isContainer())
            return false;
        switch(level) {
        case RS2::ResolveAllButInserts:
            return entity->rtti() != RS2::EntityInsert;
        case RS2::ResolveAllButTextImage:
            if (entity->rtti() == RS2::EntityImage)
                return false;
            [[fallthrough]];
        case RS2::ResolveAllButTexts:
            if (isText(*entity))
                return false;
            [[fallthrough]];
        default:
            return true;
        }
    }
    const RS_EntityContainer* container;
    RS_Entity* previous[2] = {};
    std::vector<Node> indices;
    RS2::ResolveLevel level;
};

LC_ContainerTraverser::LC_ContainerTraverser(const RS_EntityContainer& container,
                                             RS2::ResolveLevel level,
                                             LC_ContainerTraverser::Direction direction):
    m_pImp{std::make_unique<LC_ContainerTraverser::Data>(container, level)}
    , m_direction{direction}
{
}

LC_ContainerTraverser::~LC_ContainerTraverser() = default;

std::vector<RS_Entity*> LC_ContainerTraverser::entities()
{
    std::vector<RS_Entity*> ret;
    // DFS
    if (m_pImp->container->empty())
        return {};
    collect(ret, m_pImp->container);
    return ret;
}

void LC_ContainerTraverser::collect(std::vector<RS_Entity*>& items, const RS_EntityContainer* container) const
{
    for (RS_Entity* entity: std::as_const(*container)) {
        if (entity->isContainer() && m_pImp->canResolve(container)) {
            collect(items, static_cast<RS_EntityContainer*>(entity));
        } else {
            items.push_back(entity);
        }
    }
}

RS_Entity* LC_ContainerTraverser::first()
{
    m_pImp->indices = std::vector<Node>{{m_pImp->container, 0}};
    return get();
}
RS_Entity* LC_ContainerTraverser::next()
{
    return get();
}
RS_Entity* LC_ContainerTraverser::prev()
{
    return m_pImp->previous[0];
}


RS_Entity* LC_ContainerTraverser::last()
{
    LC_ContainerTraverser revTraverser{*m_pImp->container, m_pImp->level, LC_ContainerTraverser::Direction::Backword};
    return revTraverser.get();
}

RS_Entity* LC_ContainerTraverser::get()
{
    if (m_pImp->indices.empty())
        return nullptr;
    auto& [container, ii] = m_pImp->indices.back();
    if (ii >= container->count()) {
        // exhausted  the current
        m_pImp->indices.pop_back();
        return get();
    }

    RS_Entity* current = container->entityAt(currentIndex());
    // advance the index, pointing to the next candidate
    if (current->isContainer() && m_pImp->canResolve(current)) {
        m_pImp->indices.emplace_back(static_cast<RS_EntityContainer*>(current), 0);
        current = get();
    }
    m_pImp->previous[0] = m_pImp->previous[1];
    m_pImp->previous[1] = current;
    ++ii;
    return current;
}

size_t LC_ContainerTraverser::currentIndex() const
{
    if (!m_pImp->indices.empty() && !m_pImp->indices.back().container->empty()) {
        size_t index = m_pImp->indices.back().index;
        if (m_direction == Direction::Backword) {
            return m_pImp->indices.back().container->count() - 1 - index;
        }
        return index;
    }
    return 0;
}
} // namespace lc
