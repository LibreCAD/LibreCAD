/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#include "lc_action_modify_revert_direction.h"

#include <utility>
#include <unordered_set>
#include <vector>

#include "lc_offsetcurvetransform.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_modification.h"
#include "rs_selection.h"

namespace {
    RS_Entity* nativeReverseOwner(RS_Entity* entity) {
        if (entity != nullptr && entity->getParent() != nullptr &&
            entity->getParent()->rtti() == RS2::EntityOffsetCurve) return entity->getParent();
        return entity;
    }

    bool needsNativeReverse(const RS_Entity& root) {
        constexpr std::size_t limit = std::size_t{1} << 20;
        std::vector<const RS_Entity*> pending{&root};
        std::size_t work = 0;
        while (!pending.empty()) {
            if (++work > limit) return true;
            const auto* entity = pending.back();
            pending.pop_back();
            if (entity == nullptr || entity->rtti() == RS2::EntityOffsetCurve) return true;
            if (entity->isContainer()) {
                const auto* container = static_cast<const RS_EntityContainer*>(entity);
                if (container->count() > limit - work || pending.size() > limit - work - container->count()) return true;
                for (const auto* child : *container) pending.push_back(child);
            }
        }
        return false;
    }
}

LC_ActionModifyRevertDirection::LC_ActionModifyRevertDirection(LC_ActionContext* actionContext)
    : LC_ActionPreSelectionAwareBase("ActionModifyRevertDirection", actionContext, RS2::ActionModifyRevertDirection, {}) {
}

bool LC_ActionModifyRevertDirection::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    QList<RS_Entity*> ordinary;
    std::unordered_set<const RS_Entity*> staged;
    bool success = false;
    for (auto* entity : std::as_const(m_selectedEntities)) {
        entity = nativeReverseOwner(entity);
        if (entity == nullptr) continue;
        if (!needsNativeReverse(*entity)) {
            ordinary.append(entity);
            continue;
        }
        if (!staged.insert(entity).second) continue;
        if (entity->isDeleted() || m_document == nullptr || m_document->findEntityIndex(entity) < 0) continue;
        auto prepared = LC_OffsetCurveTransform::prepare(*entity, {LC_OffsetCurveTransform::Step::reverse()});
        if (!prepared) continue;
        ctx.replace(entity, prepared.entity.release());
        success = true;
    }
    if (!ordinary.isEmpty()) {
        RS_Modification::revertDirection(ordinary, ctx);
        success = true;
    }
    ctx.success = success;
    ctx.dontSetActiveLayerAndPen();
    return success;
}

void LC_ActionModifyRevertDirection::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    // fixme - INCLUDE TO GENERIC FLOW?
    if (keepSelected) {
        auto selected = ctx.entitiesToAdd;
        for (auto* entity : std::as_const(m_selectedEntities)) {
            entity = nativeReverseOwner(entity);
            if (entity != nullptr && !ctx.entitiesToDelete.contains(entity)) selected.append(entity);
        }
        select(selected);
    }
}

void LC_ActionModifyRevertDirection::doTriggerCompletion([[maybe_unused]] bool success) {
    // fixme - remove?
}

bool LC_ActionModifyRevertDirection::isShowRefPointsOnHighlight() {
    return true;
}

void LC_ActionModifyRevertDirection::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to revert direction") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Revert immediately after selection")));
}

bool LC_ActionModifyRevertDirection::isEntityAllowedToSelect(RS_Entity* ent) const {
    if (ent == nullptr || (ent->getParent() != nullptr && ent->getParent()->rtti() == RS2::EntityOffsetCurve)) return false;
    if (ent->rtti() == RS2::EntityOffsetCurve) return true;
    if (ent->isContainer()) {
        // todo - check this, it seems not all containers are properly supported
        return true;
    }
    const int rtti = ent->rtti();
    switch (rtti) {
        case RS2::EntityParabola:
        case RS2::EntityPolyline:
        case RS2::EntityLine:
        case RS2::EntityContainer:
        case RS2::EntityArc:
        case RS2::EntitySplinePoints:
            return true;
        case RS2::EntityEllipse: {
            const auto ellipse = static_cast<RS_Ellipse*>(ent);
            return ellipse->isEllipticArc();
        }
        default:
            return false;
    }
}
