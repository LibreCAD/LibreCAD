/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
 * Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * This copyright notice MUST APPEAR in all copies of the script!
 * ********************************************************************************
 */
#include "rs_selection.h"

#include "dl_jww.h"
#include "lc_containertraverser.h"
#include "lc_graphicviewport.h"
#include "lc_selectedset.h"
#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_solid.h"

RS_Selection::RS_Selection(RS_Document* container, LC_GraphicViewport* graphicView) : m_document{container}, m_viewPort{graphicView} {
    m_additiveSelection = LC_GET_ONE_BOOL("Selection", "Additivity", true);
}

RS_Selection::RS_Selection(const RS_GraphicView* graphicView) : m_document{graphicView->getDocument()}, m_viewPort{graphicView->getViewPort()} {
    m_additiveSelection = LC_GET_ONE_BOOL("Selection", "Additivity", true);
}

/**
 * Selects or deselects the given entity.
 */
void RS_Selection::selectSingle(RS_Entity* e) const {
    if (e != nullptr && (e->getLayer() != nullptr && !e->getLayer()->isLocked())) {
        const bool selected = e->isSelected();
        if (m_additiveSelection) {
            m_document->select(e, !selected);
        }
        else {
            if (selected) {
                m_document->select(e, false);
            }
            else {
                performBulkSelection([e, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
                    doUnselectAll(container, doc);
                    doc->select(e, true);
                });
            }
        }
        if (m_viewPort != nullptr) {
            // fixme - how it could be null?
            if (e->isSelected() && (e->rtti() == RS2::EntityInsert)) {
                const RS_Block* selectedBlock = static_cast<RS_Insert*>(e)->getBlockForInsert();

                if (selectedBlock != nullptr) {
                    // Display the selected block as active in the block widget
                    QC_ApplicationWindow::getAppWindow()->showBlockActivated(selectedBlock);
                    // fixme - remove
                    // Display the selected block name
                    // QG_DIALOGFACTORY->displayBlockName(selectedBlock->getName(), true);
                }
            }
            else {
                // fixme - remove
                // QG_DIALOGFACTORY->displayBlockName("", false);
            }
            m_viewPort->notifyChanged(); // fixme - is it needed?
        }
    }
}

using FunBulkSelection = std::function<void(RS_EntityContainer*, LC_GraphicViewport*, RS_Document*)>;

void RS_Selection::unselectAllInDocument(RS_Document* document, LC_GraphicViewport* vp) {
    const RS_Selection select(document, vp);
    select.selectAll(false);
}

void RS_Selection::selectEntitiesList(RS_Document* document, LC_GraphicViewport* vp, const QList<RS_Entity*>& entities, const bool doSelect) {
    const RS_Selection select(document, vp);
    select.selectEntitiesList(entities, doSelect);
}

void RS_Selection::selectEntitiesVector(RS_Document* document, LC_GraphicViewport* vp, const std::vector<RS_Entity*>& entities,
                                        bool doSelect) {
    RS_Selection select(document, vp);
    select.performBulkSelection([doSelect, entities, select](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        if (doSelect) {
            select.doUnselectAllIfNeeded(container, doc);
        }
        for (const auto e : entities) {
            doc->select(e, doSelect);
        }
    });
}

void RS_Selection::unselectLayer(RS_Document* document, LC_GraphicViewport* vp, RS_Layer* layer) {
    const RS_Selection sel(document, vp);
    sel.selectIfMatched(document->getEntityList(), false, [layer](const RS_Entity* e)-> bool {
        return e != nullptr && e->isVisible() && e->getLayer() == layer;
    });
}

void RS_Selection::selectEntitiesList(const QList<RS_Entity*>& entities, bool select) const {
    performBulkSelection([select, entities, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        if (select) {
            doUnselectAllIfNeeded(container, doc);
        }
        for (const auto e : entities) {
            doc->select(e, select);
        }
    });
}

void RS_Selection::selectIfMatched(const QList<RS_Entity*>& entities, bool select, FunEntityMatch matchFun) const {
    performBulkSelection([select, entities, matchFun, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        if (select) {
            doUnselectAllIfNeeded(container, doc);
        }
        for (const auto e : entities) {
            if (matchFun(e)) {
                doc->select(e, select);
            }
        }
    });
}

/**
 * Selects all entities on visible layers.
 */
void RS_Selection::selectAll(bool select) const {
    if (m_viewPort != nullptr) {
        performBulkSelection([select, this](RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
            if (select) {
                for (const auto e : *container) {
                    if (e != nullptr && e->isVisible()) {
                        doc->select(e);
                        // fixme - sand - selectAll by entity type - check whether it will not break plugin interface:
                        // NOTE:
                        // this is actually bad practice and development by side-effect.
                        // type to select for graphic view is set only in  Doc_plugin_interface::getSelectByType
                        // and in general it's hardly that this flag is properly used (as later
                        // RS_ActionSelectSingle with own check is invoked.
                        // So better just create separate function with explicit type of entity, if one will be really
                        // necessary.
                        /* if (graphicView->getTypeToSelect() == RS2::EntityType::EntityUnknown){
                             e->setSelected(select);
                         } else {
                             if (e->rtti() == graphicView->getTypeToSelect()){
                                 e->setSelected(select);
                             }
                         }*/
                    }
                }
            }
            else {
                doUnselectAll(container, doc);
            }
        });
    }
}

void RS_Selection::performBulkSelection(const FunBulkSelection& fun) const {
    const auto doc = m_document->getDocument();
    const auto selectedSet = doc->getSelection();
    selectedSet->disableListeners();

    fun(m_document, m_viewPort, doc);

    const bool listenersFired = selectedSet->enableListeners();
    if (!listenersFired && !selectedSet->isSilent()) {
        selectedSet->fireSelectionChanged();
    }
    m_viewPort->notifyChanged();
}

void RS_Selection::doUnselectAllIfNeeded(const RS_EntityContainer* container, const RS_Document* doc) const {
    if (!m_additiveSelection) {
        doUnselectAll(container, doc);
    }
}

void RS_Selection::doUnselectAll(const RS_EntityContainer* container, const RS_Document* doc) const {
    for (const auto e : *container) {
        if (e != nullptr) {
            e->clearSelectionFlag();
        }
    }
    doc->getSelection()->clear();
}

/**
 * Selects all entities on visible layers.
 */
void RS_Selection::invertSelection() const {
    // fixme - review which container is actually used there
    performBulkSelection([](RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        for (const auto e : *container) {
            if (e != nullptr && e->isVisible()) {
                doc->select(e, !e->isSelected());
            }
        }
    });
}

/**
 * Selects all entities that are completely in the given window.
 *
 * @param typeToSelect
 * @param v1 First corner of the window to select.
 * @param v2 Second corner of the window to select.
 * @param select true: select, false: invertSelectionOperation
 * @param cross
 */
void RS_Selection::selectWindow(RS2::EntityType typeToSelect, const RS_Vector& v1, const RS_Vector& v2, bool select, bool cross) {
    performBulkSelection(
        [typeToSelect, v1, v2, select, cross, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
            if (select) {
                doUnselectAllIfNeeded(container, doc);
            }
            doSelectEntitiesWithTypeInWindow(container, doc, typeToSelect, v1, v2, select, cross);
        });
}

void RS_Selection::selectWindow(const QList<RS2::EntityType>& typesToSelect, const RS_Vector& v1, const RS_Vector& v2, bool select,
                                bool cross) {
    performBulkSelection(
        [typesToSelect, v1, v2, select, cross, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
            if (select) {
                doUnselectAllIfNeeded(container, doc);
            }
            doSelectEntitiesWithTypesInWindow(container, doc, typesToSelect, v1, v2, select, cross);
        });
}

void RS_Selection::selectIntersected(RS_Entity* entity, bool select) const {
    performBulkSelection([entity, select, this](const RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        if (select) {
            doUnselectAllIfNeeded(container, doc);
        }
        if (entity->isAtomic()) {
            doSelectIntersectedAtomic(entity, doc, select);
        }
        else if (entity->isContainer()) {
            doSelectIntersectedContainer(entity, doc, select);
        }
    });
}

void RS_Selection::doSelectIntersectedContainer(RS_Entity* entity, RS_Document* doc, const bool select) const {
    const auto* cont = static_cast<RS_EntityContainer*>(entity);
    const auto containerEntities = lc::LC_ContainerTraverser{*cont, RS2::ResolveAll}.entities();

    for (const auto e : *m_document) {
        // fixme - iteration over ALL entities, limit area
        if (e == nullptr || e == entity || !e->isVisible()) {
            continue;
        }
        bool hasIntersection = false;

        // select containers / groups:
        if (e->isContainer()) {
            const auto* ec = static_cast<RS_EntityContainer*>(e);
            if (entity->getParent() == e) {
                // case for segment of polyline
                continue;
            }
            for (const RS_Entity* e2 : lc::LC_ContainerTraverser{*ec, RS2::ResolveAll}.entities()) {
                for (const RS_Entity* e3 : containerEntities) {
                    RS_VectorSolutions sol = RS_Information::getIntersection(e3, e2, true);
                    if (sol.hasValid()) {
                        hasIntersection = true;
                        break;
                    }
                }
            }
        }
        else {
            for (const RS_Entity* e2 : containerEntities) {
                RS_VectorSolutions sol = RS_Information::getIntersection(e2, e, true);
                if (sol.hasValid()) {
                    hasIntersection = true;
                    break;
                }
            }
        }
        if (hasIntersection) {
            doc->select(e, select);
        }
    }
    m_viewPort->notifyChanged();
}

void RS_Selection::doSelectIntersectedAtomic(const RS_Entity* entity, RS_Document* doc, const bool select) const {
    for (const auto e : *m_document) {
        // fixme - iteration over ALL entities, limit area
        if (e != nullptr && e->isVisible()) {
            bool hasIntersections = false;

            // select containers / groups:
            if (e->isContainer()) {
                const auto* ec = static_cast<RS_EntityContainer*>(e);
                if (entity->getParent() == e) {
                    // case for segment of polyline
                    continue;
                }
                for (const RS_Entity* e2 : lc::LC_ContainerTraverser{*ec, RS2::ResolveAll}.entities()) {
                    RS_VectorSolutions sol = RS_Information::getIntersection(entity, e2, true);
                    if (sol.hasValid()) {
                        hasIntersections = true;
                    }
                }
            }
            else {
                RS_VectorSolutions sol = RS_Information::getIntersection(entity, e, true);
                if (sol.hasValid()) {
                    hasIntersections = true;
                }
            }
            if (hasIntersections) {
                doc->select(e, select);
            }
        }
    }
    m_viewPort->notifyChanged();
}

/**
 * Selects all entities that are intersected by the given line.
 *
 * @param v1 Startpoint of line.
 * @param v2 Endpoint of line.
 * @param select true: select, false: invertSelectionOperation
 */
void RS_Selection::selectIntersected(const RS_Vector& v1, const RS_Vector& v2, const bool select) const {
    RS_Line line{v1, v2};
    selectIntersected(&line, select);
}

/**
 * Selects all entities that are connected to the given entity.
 *
 * @param e The entity where the algorithm starts. Must be an atomic entity.
 */
void RS_Selection::selectContour(RS_Entity* e) const {
    if (e == nullptr) {
        return;
    }
    if (!e->isAtomic()) {
        return;
    }

    performBulkSelection([e, this](RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        const bool select = !e->isSelected();
        doUnselectAllIfNeeded(container, doc);

        auto* atomicEntity = static_cast<RS_AtomicEntity*>(e);
        RS_Vector p1 = atomicEntity->getStartpoint();
        RS_Vector p2 = atomicEntity->getEndpoint();
        bool found = false;

        // (de)select 1st entity:
        doc->select(e, select);
        do {
            found = false;
            for (const auto en : *container) {
                if (en != nullptr && en->isAtomic() && en->isVisible() && (en->getFlag(RS2::FlagSelected) != select) && (!(en->getLayer() &&
                    en->getLayer()->isLocked()))) {
                    atomicEntity = static_cast<RS_AtomicEntity*>(en);
                    bool neighbourFound = false;
                    auto atomicStart = atomicEntity->getStartpoint();
                    auto atomicEnd = atomicEntity->getEndpoint();

                    // startpoint connects to 1st point
                    if (atomicStart.distanceTo(p1) < 1.0e-4) {
                        // fixme - use constant for tolerance
                        neighbourFound = true;
                        p1 = atomicEnd;
                    }

                    // endpoint connects to 1st point
                    else if (atomicEnd.distanceTo(p1) < 1.0e-4) {
                        neighbourFound = true;
                        p1 = atomicStart;
                    }

                    // startpoint connects to 2nd point
                    else if (atomicStart.distanceTo(p2) < 1.0e-4) {
                        neighbourFound = true;
                        p2 = atomicEnd;
                    }

                    // endpoint connects to 1st point
                    else if (atomicEnd.distanceTo(p2) < 1.0e-4) {
                        neighbourFound = true;
                        p2 = atomicStart;
                    }

                    if (neighbourFound) {
                        doc->select(atomicEntity, select);
                        found = true;
                    }
                }
            }
        }
        while (found);
    });

    m_viewPort->notifyChanged();
}

/**
 * Selects all entities on the given layer.
 */
void RS_Selection::selectLayer(const RS_Entity* e) const {
    if (e == nullptr) {
        return;
    }

    const bool select = !e->isSelected();
    const RS_Layer* layer = e->getLayer(true);
    if (layer == nullptr) {
        return;
    }
    if (layer->isFrozen() || layer->isLocked()) {
        return;
    }

    selectLayer(layer, select);
}

void RS_Selection:: selectLayer(const RS_Layer* layer, bool select) const {
    performBulkSelection([layer, select, this](RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        doUnselectAllIfNeeded(container, doc);
        for (const auto en : *container) {
            if (en == nullptr) {
                continue;
            }
            if (en->isDeleted()) {
                continue;
            }
            if (en->isSet(RS2::FlagSelected) == select) {
                continue;
            }
            const RS_Layer* l = en->getLayer(true);
            if (l == layer) {
                doc->select(en, select);
            }
        }
    });
}

// fixme - rework, use setSelected? At least, fix the selection state issue
void RS_Selection::conditionalSelection(const ConditionalSelectionOptions& options) const {
    // doUnselectAllIfNeeded(container, doc);
    performBulkSelection([this, options]([[maybe_unused]] RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc)-> void {
        const auto funRttiMatch = options.funRttiMatcher;
        const auto funEntityMatch = options.funEntityMatcher;
        QList<RS_Entity*> newEntitiesSet;
        const auto selectedSet = m_document->getSelection();
        const bool includeIntoSelectionSet = options.includeIntoSelectionSet;
        const bool excludeFromSelectionSet = !options.includeIntoSelectionSet;
        if (options.applyArea == ConditionalSelectionOptions::Document) {
            // criteria applied to entire document
            for (const auto e : *doc) {
                if (e->isDeleted()) {
                    continue;
                }
                if (e->isInvisible()) {
                    continue;
                }
                if (funRttiMatch(e)) {
                    const bool matched = funEntityMatch(e);
                    if (matched) {
                        if (includeIntoSelectionSet) {
                            if (!e->isSelected() || !options.appendToSelectionSet) {
                                e->setSelectionFlag(true);
                                newEntitiesSet.append(e);
                            }
                        }
                    }
                    else {
                        if (excludeFromSelectionSet) {
                            if (!e->isSelected() || !options.appendToSelectionSet) {
                                e->setSelectionFlag(true);
                                newEntitiesSet.append(e);
                            }
                        }
                    }
                }
            }
            if (options.appendToSelectionSet) {
                for (const auto e : newEntitiesSet) {
                    selectedSet->add(e);
                }
            }
            else {
                // just replace by matched entities
                selectedSet->replaceBy(newEntitiesSet);
            }
        }
        else {
            // criteria applied to current selection
            QList<RS_Entity*> selectedEntities;
            m_document->collectSelected(selectedEntities);
            for (const auto e : std::as_const(selectedEntities)) {
                if (e->isDeleted()) {
                    continue;
                }
                if (e->isInvisible()) {
                    continue;
                }
                if (funRttiMatch(e)) {
                    const bool matched = funEntityMatch(e);
                    if (matched) {
                        if (includeIntoSelectionSet) {
                            newEntitiesSet.append(e);
                            e->setSelectionFlag(true);
                        }
                        else {
                            e->setSelectionFlag(false);
                        }
                    }
                    else {
                        if (excludeFromSelectionSet) {
                            newEntitiesSet.append(e);
                            e->setSelectionFlag(true);
                        }
                        else {
                            e->setSelectionFlag(false);
                        }
                    }
                }
                else {
                    if (options.appendToSelectionSet) {
                        // entity with other rtti stays in the selection
                        newEntitiesSet.append(e);
                        e->setSelectionFlag(true);
                    }
                    else {
                        e->setSelectionFlag(false);
                    }
                }
            }
            // replace existing selection by matched entities
            selectedSet->replaceBy(newEntitiesSet);
        }
    });
}

void RS_Selection::countSelectedEntities(QMap<RS2::EntityType, int>& entityTypeMaps) const {
    if (m_viewPort != nullptr) {
        const auto doc = m_document->getDocument();
        QList<RS_Entity*> selectedEntities;
        doc->collectSelected(selectedEntities);
        for (const auto e : std::as_const(selectedEntities)) {
            auto rtti = e->rtti();
            int count = entityTypeMaps[rtti];
            count++;
            entityTypeMaps[rtti] = count;
        }
    }
}

void RS_Selection::collectCurrentSelectionState(CurrentSelectionState& selectionState) const {
    if (m_viewPort != nullptr) {
        for (const auto e : *m_document) {
            // iterating over all visible entities in the document
            if (e != nullptr && e->isVisible()) {
                auto rtti = e->rtti();
                int count = selectionState.documentEntityTypes[rtti];
                count++;
                selectionState.totalDocumentEntities++;
                selectionState.documentEntityTypes[rtti] = count;
                if (e->getFlag(RS2::FlagSelected)) {
                    int selectedCount = selectionState.selectedEntityTypes[rtti];
                    selectedCount++;
                    selectionState.totalSelectedEntities++;
                    selectionState.selectedEntityTypes[rtti] = selectedCount;
                }
            }
        }
    }
}

/**
 * Selects all entities within the given area.
 *
 * @param container
 * @param doc
 * @param typeToSelect
 * @param v1
 * @param v2
 * @param select True to select, False to invertSelectionOperation the entities.
 * @param cross
 */
// todo - sand - ucs - add method for selecting entities within rect that is rotated in wcs
// Such method is needed for better support UCS with rotation and more precise selection of m_entities.
void RS_Selection::doSelectEntitiesWithTypeInWindow(const RS_EntityContainer* container, RS_Document* doc,
                                                    const RS2::EntityType typeToSelect, const RS_Vector& v1, const RS_Vector& v2,
                                                    const bool select, const bool cross) {
    for (RS_Entity* e : *container) {
        bool included = false;
        if (e->isVisible()) {
            if (e->isInWindow(v1, v2)) {
                included = true;
            }
            else if (cross) {
                RS_EntityContainer l;
                l.addRectangle(v1, v2);
                RS_VectorSolutions sol;

                if (e->isContainer()) {
                    const auto* ec = static_cast<RS_EntityContainer*>(e);
                    lc::LC_ContainerTraverser traverser{*ec, RS2::ResolveAll};
                    for (RS_Entity* se = traverser.first(); se != nullptr && !included; se = traverser.next()) {
                        if (se->rtti() == RS2::EntitySolid) {
                            included = static_cast<RS_Solid*>(se)->isInCrossWindow(v1, v2);
                        }
                        else {
                            for (const RS_Entity* line : l) {
                                sol = RS_Information::getIntersection(se, line, true);
                                if (sol.hasValid()) {
                                    included = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (e->rtti() == RS2::EntitySolid) {
                    included = static_cast<RS_Solid*>(e)->isInCrossWindow(v1, v2);
                }
                else {
                    for (const RS_Entity* line : l) {
                        sol = RS_Information::getIntersection(e, line, true);
                        if (sol.hasValid()) {
                            included = true;
                            break;
                        }
                    }
                }
            }
        }

        if (included) {
            if (typeToSelect != RS2::EntityType::EntityUnknown) {
                if (typeToSelect == e->rtti()) {
                    doc->select(e, select);
                }
                else {
                    //Do not select
                }
            }
            else {
                doc->select(e, select);
            }
        }
    }
}

/**
 * Selects all entities within the given area with given types.
 *
 * @param container
 * @param doc
 * @param typesToSelect
 * @param v1
 * @param v2
 * @param select True to select, False to invertSelectionOperation the entities.
 * @param cross
 */
void RS_Selection::doSelectEntitiesWithTypesInWindow(const RS_EntityContainer* container, RS_Document* doc,
                                                     const QList<RS2::EntityType>& typesToSelect, const RS_Vector& v1, const RS_Vector& v2,
                                                     const bool select, const bool cross) {
    for (RS_Entity* e : *container) {
        if (!typesToSelect.contains(e->rtti())) {
            continue;
        }
        bool included = false;
        if (e->isVisible()) {
            if (e->isInWindow(v1, v2)) {
                included = true;
            }
            else if (cross) {
                RS_EntityContainer l;
                l.addRectangle(v1, v2);
                RS_VectorSolutions sol;

                if (e->isContainer()) {
                    const auto* ec = static_cast<RS_EntityContainer*>(e);
                    lc::LC_ContainerTraverser traverser{*ec, RS2::ResolveAll};
                    for (RS_Entity* se = traverser.first(); se != nullptr && !included; se = traverser.next()) {
                        if (se->rtti() == RS2::EntitySolid) {
                            included = static_cast<RS_Solid*>(se)->isInCrossWindow(v1, v2);
                        }
                        else {
                            for (const auto line : l) {
                                sol = RS_Information::getIntersection(se, line, true);
                                if (sol.hasValid()) {
                                    included = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (e->rtti() == RS2::EntitySolid) {
                    included = static_cast<RS_Solid*>(e)->isInCrossWindow(v1, v2);
                }
                else {
                    for (const auto line : l) {
                        sol = RS_Information::getIntersection(e, line, true);
                        if (sol.hasValid()) {
                            included = true;
                            break;
                        }
                    }
                }
            }
        }

        if (included) {
            doc->select(e, select);
        }
    }
}
