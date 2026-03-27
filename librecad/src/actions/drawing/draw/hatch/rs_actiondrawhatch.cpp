/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "rs_actiondrawhatch.h"

#include "lc_containertraverser.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_pen.h"

namespace {
    bool hatchAble(const RS_Entity *entity) {
        if (entity == nullptr) {
            return false;
        }
        switch (entity->rtti()) {
            case RS2::EntityHatch:
            case RS2::EntityPoint:
            case RS2::EntityImage:
            case RS2::EntityMText:
            case RS2::EntityText:
                return false;
            default: {
                if (RS_Information::isDimension(entity->rtti())) {
                    return false;
                }
                return entity->getLength() > RS_TOLERANCE;
            }
        }
    }
}

// fixme - review hatching and check the possibility to add preview mode!!

RS_ActionDrawHatch::RS_ActionDrawHatch(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("Draw Hatch", actionContext, RS2::ActionDrawHatch)
    , m_hatchData{std::make_unique<RS_HatchData>()}{
}

RS_ActionDrawHatch::~RS_ActionDrawHatch() = default;

void RS_ActionDrawHatch::setShowArea(const bool s){
    m_bShowArea=s;
}

void RS_ActionDrawHatch::init(const int status){
    LC_ActionPreSelectionAwareBase::init(status);
}

bool RS_ActionDrawHatch::isAllowTriggerOnEmptySelection() {
    return false;
}

bool RS_ActionDrawHatch::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    // fixme - complete ! !!!!
    RS_DEBUG->print("RS_ActionDrawHatch::trigger()");

    LC_SelectedSet* selection = m_document->getSelection();
    QList<RS_Entity*> selectedEntities;
    selection->collectSelectedEntities(selectedEntities);

    QList<RS_Entity*> entitiesList;
    // deselect unhatchable entities:
    for (const auto e : selectedEntities) {
        if (!hatchAble(e)) {
            unselect(e);
        }
        else {
            entitiesList.push_back(e); // store to later use and to avoid full traversal
        }
    }

    // look for selected contours:
    const bool hasContour = !entitiesList.isEmpty();
    if (!hasContour) {
        LC_ERR << "RS_ActionDrawHatch:: " << __func__ << "(): line " << __LINE__ << ", no contour selected\n";
        return false;
    }
    RS_Hatch tmp(m_document, *m_hatchData);
    setPenAndLayerToActive(&tmp);

    // fixme - sand - todo - Hatches may be automatically put into some specific layer (like dimensional) based on settings

    // fixme - how to validate the hatch BEFORE displaying the dialog? Dialog is not needed if no valid hatch contour
    if (RS_DIALOGFACTORY->requestHatchDialog(&tmp, m_viewport)) {
        *m_hatchData = tmp.getData();
        auto* hatch  = new RS_Hatch(m_document, *m_hatchData);
        auto* loop = new RS_EntityContainer(hatch);
        loop->setPen(RS_Pen(RS2::FlagInvalid));

        // add selected contour:
        for (const RS_Entity* e : entitiesList) {
            RS_Entity* clone = e->clone();
            clone->setPen(RS_Pen(RS2::FlagInvalid));
            clone->reparent(loop);
            loop->addEntity(clone);
        }

        hatch->addEntity(loop);
        if (hatch->validate()) {
            hatch->update();
            bool printArea = true;
            bool ok        = false;
            switch (hatch->getUpdateError()) {
                case RS_Hatch::HATCH_OK:
                    ctx += hatch;
                    ok = true;
                    commandMessage(tr("Hatch created successfully."));
                    break;
                case RS_Hatch::HATCH_INVALID_CONTOUR:
                    commandMessage(tr("Hatch Error: Invalid contour found!"));
                    printArea = false;
                    break;
                case RS_Hatch::HATCH_PATTERN_NOT_FOUND:
                    commandMessage(tr("Hatch Error: Pattern not found!"));
                    break;
                case RS_Hatch::HATCH_TOO_SMALL:
                    commandMessage(tr("Hatch Error: Contour or pattern too small!"));
                    break;
                case RS_Hatch::HATCH_AREA_TOO_BIG:
                    commandMessage(tr("Hatch Error: Contour too big!"));
                    break;
                default:
                    commandMessage(tr("Hatch Error: Undefined Error!"));
                    printArea = false;
                    break;
            }
            if (m_bShowArea && printArea) {
                commandMessage(tr("Total hatch area = %1").arg(hatch->getTotalArea(), 12, 'g', 10));
            }
            if (!ok) {
                delete hatch;
            }
        }
        else {
            delete hatch;
            commandMessage(tr("Invalid hatch area. Please check that the entities chosen form one or more closed contours."));
        }
    }
    return true;
}

void RS_ActionDrawHatch::doTriggerSelectionUpdate([[maybe_unused]]bool keepSelected, [[maybe_unused]]const LC_DocumentModificationBatch& ctx) {
    // fixme - complete
}

void RS_ActionDrawHatch::doTriggerCompletion([[maybe_unused]]bool success) {
    unselectAll();
    finish();
}

void RS_ActionDrawHatch::doSelectEntity(RS_Entity* entityToSelect, [[maybe_unused]] bool selectContour) const {
    // try to minimize selection clicks - and select contour based on selected entity. May be optional, but what for?
    RS_ActionSelectBase::doSelectEntity(entityToSelect, true);
}

bool RS_ActionDrawHatch::isEntityAllowedToSelect(RS_Entity *ent) const {
    return hatchAble(ent);
}

void RS_ActionDrawHatch::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to hatch") + getSelectionCompletionHintMsg(), MOD_CTRL(tr("Hatch immediately after selection")));
}

RS2::CursorType RS_ActionDrawHatch::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::SelectCursor;
}
