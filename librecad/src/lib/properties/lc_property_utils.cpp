/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_property_utils.h"

#include "lc_property_multi.h"

void LC_PropertyContainerUtils::addChildPropertyToContainer(QObject* parent, LC_Property* child, const bool moveOwnership) {
    auto* container = qobject_cast<LC_PropertyContainer*>(parent);
    if (container != nullptr) {
        container->addChildProperty(child, moveOwnership);
    }
}

void LC_PropertyContainerUtils::gatherPropertiesToMultiSet(LC_PropertyContainer* target, LC_PropertyContainer* source,
                                                           const bool takeOwnership) {
    Q_ASSERT(target);
    Q_ASSERT(source);

    auto& targetProperties = target->childProperties();
    for (auto prop : source->childProperties()) {
        auto findPredicate = [prop](const LC_Property* targetProperty) -> bool {
            return prop->propertyMetaObject() == targetProperty->propertyMetaObject() && prop->getDisplayName() == targetProperty->
                getDisplayName();
        };
        auto it = std::find_if(targetProperties.begin(), targetProperties.end(), findPredicate);
        // fixme - duplicated code to propertyViewMultiple!!
        const auto container = prop->asContainer();
        if (container != nullptr) {
            LC_PropertyContainer* targetContainer;
            if (it == targetProperties.end()) {
                targetContainer = new LC_PropertyContainer();
                targetContainer->setName(container->getName());
                targetContainer->setDisplayName(container->getDisplayName());
                targetContainer->setDescription(container->getDescription());
                targetContainer->setState(container->stateLocal());
                target->addChildProperty(targetContainer, true);
            }
            else {
                targetContainer = (*it)->asContainer();
            }
            gatherPropertiesToMultiSet(targetContainer, container, takeOwnership);
        }
        else {
            LC_PropertyMulti* multiProperty;
            if (it == targetProperties.end()) {
                multiProperty = new LC_PropertyMulti(prop->metaObject());
                multiProperty->setName(prop->getName());
                multiProperty->setDisplayName(prop->getDisplayName());
                multiProperty->setDescription(prop->getDescription());
                target->addChildProperty(multiProperty, true);
            }
            else {
                Q_ASSERT(qobject_cast<LC_PropertyMulti*>(*it));
                multiProperty = static_cast<LC_PropertyMulti*>(*it);
            }

            multiProperty->addProperty(prop->asAtomic(), takeOwnership);
        }
    }
    if (takeOwnership) {
        source->clearChildProperties();
    }
}

void LC_PropertyContainerUtils::removeChildPropertyFromContainer(QObject* parent, LC_Property* child) {
    auto* container = qobject_cast<LC_PropertyContainer*>(parent);
    if (container != nullptr) {
        container->removeChildProperty(child);
    }
}
