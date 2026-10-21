/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_offsetoutputbudget.h"

#include <unordered_set>

#include "rs_entitycontainer.h"

LC_OffsetTreeCost measureOffsetOutput(const std::vector<const RS_Entity*>& roots, const std::size_t cap) {
    std::unordered_set<const RS_Entity*> seen;
    std::vector<const RS_Entity*> pending;
    for (const RS_Entity* root : roots) {
        if (root == nullptr || !seen.insert(root).second) {
            return {LC_OffsetTreeStatus::InvalidTree, 0};
        }
        pending.push_back(root);
    }
    std::size_t leaves = 0;
    while (!pending.empty()) {
        const RS_Entity* entity = pending.back();
        pending.pop_back();
        const auto* container = entity->isContainer() ? dynamic_cast<const RS_EntityContainer*>(entity) : nullptr;
        if (container != nullptr && container->count() > 0) {
            for (const RS_Entity* child : *container) {
                if (child == nullptr || !seen.insert(child).second) {
                    return {LC_OffsetTreeStatus::InvalidTree, leaves};
                }
                pending.push_back(child);
            }
        }
        else {
            if (leaves >= cap) {
                return {LC_OffsetTreeStatus::LimitExceeded, leaves};
            }
            ++leaves;
        }
    }
    return {LC_OffsetTreeStatus::Ok, leaves};
}
