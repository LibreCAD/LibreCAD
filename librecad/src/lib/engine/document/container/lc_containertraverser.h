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

class RS_AtomicEntity;
class RS_Entity;
class RS_EntityContainer;
class RS_Line;
class RS_Vector;
class RS_VectorSolutions;

namespace RS2 {
    enum ResolveLevel: unsigned short;
}
namespace lc {

class LC_ContainerTraverser {
public:
    enum class Direction: short
    {
        Forward = 0,
        Backword = 1
    };
    LC_ContainerTraverser(const RS_EntityContainer& container,
                          RS2::ResolveLevel level,
                          LC_ContainerTraverser::Direction direction = Direction::Forward);
    ~LC_ContainerTraverser();

    RS_Entity* first();
    RS_Entity* last();
    RS_Entity* next();
    RS_Entity* prev();
    std::vector<RS_Entity*> entities();


private:
    RS_Entity* get();
    void collect(std::vector<RS_Entity*>& items, const RS_EntityContainer* container) const;
    // for forward/backward support
    size_t currentIndex() const;
    struct Data;
    std::unique_ptr<LC_ContainerTraverser::Data> m_pImp;
    LC_ContainerTraverser::Direction m_direction = Direction::Forward;


};

}

#endif // LC_ContainerTraverser_H


