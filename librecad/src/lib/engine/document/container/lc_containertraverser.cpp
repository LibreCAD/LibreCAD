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

#include "lc_containertraverser.h"

#include "rs.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"


namespace {

// ParentNode used to track containers during traversing
struct ParentNode
{
    ParentNode(const RS_EntityContainer* container, int index):
        container{container}
        , index{index}
    {}

    // Whether the index is valid within the current parent node
    bool isValid() const
    {
        return container != nullptr && index >= 0 && size_t(index) + 1 <= container->count();
    }
    const RS_EntityContainer* container = nullptr;
    int index = 0;
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

// pImp struct
struct LC_ContainerTraverser::Data {
    Data(const RS_EntityContainer& container, RS2::ResolveLevel level):
        container{&container}
        , indices{{&container, 0}}
        , level{level}
    {
    }

    // whether to traverse into
    bool canResolve(const RS_Entity* entity) const
    {
        if (entity == nullptr || !entity->isContainer())
            return false;
        switch(level) {
        case RS2::ResolveNone:
            return false;
        case RS2::ResolveAllButInserts:
            return entity->rtti() != RS2::EntityInsert;
        case RS2::ResolveAllButTextImage:
            return (entity->rtti() != RS2::EntityImage) && isText(*entity);
        case RS2::ResolveAllButTexts:
            return !isText(*entity);
        case RS2::ResolveAll:
        default:
            return true;
        }
    }
    const RS_EntityContainer* container = nullptr;
    std::vector<ParentNode> indices;
    RS2::ResolveLevel level = RS2::ResolveNone;
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
    // collecting entities by the DFS order
    collect(ret, m_pImp->container);
    return ret;
}

void LC_ContainerTraverser::collect(std::vector<RS_Entity*>& items, const RS_EntityContainer* container) const
{
    if (container == nullptr)
        return;

    for (RS_Entity* entity: std::as_const(*container)) {
        if (entity == nullptr)
            continue;

        if (entity->isContainer() && m_pImp->canResolve(container)) {
            collect(items, static_cast<RS_EntityContainer*>(entity));
        } else {
            items.push_back(entity);
        }
    }
}

RS_Entity* LC_ContainerTraverser::first()
{
    m_pImp->indices = std::vector<ParentNode>{{m_pImp->container, 0}};
    return get();
}

RS_Entity* LC_ContainerTraverser::next()
{
    return get();
}

RS_Entity* LC_ContainerTraverser::prev()
{
    // create a traverser with reverted direction
    // so the next traversed node is the previous of the current traverser
    LC_ContainerTraverser revTraverser{*m_pImp->container, m_pImp->level, LC_ContainerTraverser::Direction::Backword};
    revTraverser.m_direction = (m_direction == Direction::Forward) ?
                                   Direction::Backword : Direction::Backword;

    // revert the indices
    for (ParentNode& node: revTraverser.m_pImp->indices) {
        if (node.isValid())
            node.index = node.container->count() - 1 - node.index;
    }

    // the previous.
    // The index always points to the next
    [[maybe_unused]] RS_Entity* next = revTraverser.get();
    // current
    [[maybe_unused]] RS_Entity* current = revTraverser.get();
    // previous
    return revTraverser.get();
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
    if (ii < 0 || size_t(ii) >= container->count()) {
        // exhausted the current
        m_pImp->indices.pop_back();
        return get();
    }

    RS_Entity* current = container->entityAt(currentIndex());
    // advance the index, pointing to the next candidate
    ++ii;
    if (current->isContainer() && m_pImp->canResolve(current)) {
        m_pImp->indices.emplace_back(static_cast<RS_EntityContainer*>(current), 0);
        return get();
    }
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
