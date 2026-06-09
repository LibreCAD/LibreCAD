/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
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
 * ********************************************************************************
 */

#include "lc_selectedset.h"

#include "rs_entity.h"
#include "rs_layer.h"

LC_SelectedSet::LC_SelectedSet() = default;

LC_SelectedSet::~LC_SelectedSet() {
    m_entitiesList.clear();
    m_listeners.clear();
}

void LC_SelectedSet::clear() {
    for (const auto e: std::as_const(m_entitiesList)) {
        e->setSelectionFlag(false);
    }
    m_entitiesList.clear();
    fireSelectionChanged();
}

#define DEBUG_UNIQUE_SELECTION

void LC_SelectedSet::add(RS_Entity* entity) {
#ifdef DEBUG_UNIQUE_SELECTION
    for (const auto e : std::as_const(m_entitiesList)) {
        Q_ASSERT_X(e != entity, "LC_SelectedSet::add()", "Entity not unique in selection");
    }
#endif
    m_entitiesList.push_back(entity);
    fireSelectionChanged();
}

void LC_SelectedSet::remove(RS_Entity* entity) {
    entity->setSelectionFlag(false);
    m_entitiesList.removeOne(entity);
    fireSelectionChanged();
}

void LC_SelectedSet::replaceBy(QList<RS_Entity*>& entities) {
    for (const auto e: std::as_const(m_entitiesList)) {
        e->setSelectionFlag(false);
    }
    m_entitiesList.clear();
    m_entitiesList.append(entities);
    // need to update flag again, as the same entity might be in current selection list (so it will be un-selected) and
    // be added in new list (so it should be selected).
    for (const auto e: std::as_const(entities)) {
        e->setSelectionFlag(true);
    }
    fireSelectionChanged();
}

void LC_SelectedSet::addListener(LC_SelectedSetListener* listener) {
    if (listener == nullptr) {
        return;
    }
    for (const auto l : std::as_const(m_listeners)) {
        if (l == listener) {
            return;
        }
    }
    m_listeners.append(listener);
}

void LC_SelectedSet::removeListener(LC_SelectedSetListener* listener) {
    m_listeners.removeOne(listener);
}

void LC_SelectedSet::fireSelectionChanged() {
    if (m_silentMode == 0) {
        for (const auto l : std::as_const(m_listeners)) {
            l->selectionChanged();
        }
    }
    else{
        m_changedInSilent = true;
    }
}

void LC_SelectedSet::cleanup() {
    if (m_entitiesList.empty()) {
        return;
    }
    QList<RS_Entity*> validEntities;
    for (const auto e: std::as_const(m_entitiesList)) {
        bool valid = false;
        if (e->isSet(RS2::FlagSelected) && e->isAlive()) {
            const RS_Layer* layer = e->getLayerResolved();
            if (!layer->isLocked() && !layer->isFrozen()) {  // also clear selection for locked and freezed layers
                valid = true;
            }
        }
        if (valid) {
            validEntities.append(e);
        }
        else {
            e->setSelectionFlag(false);
        }
    }
    const auto validEntitiesCount = validEntities.count();
    const auto currentEntitiesCount        = m_entitiesList.count();
    if (validEntitiesCount != currentEntitiesCount) {
        m_entitiesList.clear();
        m_entitiesList.append(validEntities);
        fireSelectionChanged();
    }
}

bool LC_SelectedSet::collectSelectedEntities(QList<RS_Entity*>& list) {
    bool cleanupNeeded = false;
    for (const auto e: std::as_const(m_entitiesList)) {
        if (e != nullptr) {
            if (e->isSet(RS2::FlagSelected) && e->isNotSet(RS2::FlagDeleted)) {
                list.append(e);
            }
            else {
                cleanupNeeded = true;
            }
        }
    }
    if (cleanupNeeded) {
        cleanup();
    }
    return !list.isEmpty();
}

bool LC_SelectedSet::collectSelectedEntities(QList<RS_Entity*>& list, const QList<RS2::EntityType>&types) {
    const bool specificTypesNeeded = !types.empty();
    bool cleanupNeeded = false;
    for (const auto e: std::as_const(m_entitiesList)) {
        if (e != nullptr) {
            if (e->isSet(RS2::FlagSelected) && e->isNotSet(RS2::FlagDeleted)) {
                if (specificTypesNeeded) {
                    RS2::EntityType type = e->rtti();
                    if (types.count(type) > 0) {
                        list.append(e);
                    }
                }
                else {
                    list.append(e);
                }
            }
            else {
                cleanupNeeded = true;
            }
        }
    }
    if (cleanupNeeded) {
        cleanup();
    }
    return !list.isEmpty();
}

bool LC_SelectedSet::hasSelection() {
    bool cleanupNeeded = false;
    bool hasSelection = false;
    for (const auto e: std::as_const(m_entitiesList)) {
        if (e != nullptr) {
            if (e->isSet(RS2::FlagSelected) && e->isNotSet(RS2::FlagDeleted)) {
                hasSelection = true;
                break;
            }
            cleanupNeeded = true;
        }
    }
    if (cleanupNeeded) {
        cleanup();
    }
    return hasSelection;
}

void LC_SelectedSet::disableListeners() {
    m_silentMode++;
}

bool LC_SelectedSet::enableListeners() {
    m_silentMode --;
    if (m_silentMode == 0) {
        if (m_changedInSilent) {
            m_changedInSilent = false; // fixme - review what to return!
            fireSelectionChanged();
            return true;
        }
        return false;
    }
    return false;
}
