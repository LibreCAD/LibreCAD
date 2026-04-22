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

#include "lc_properties_sheet_model.h"

LC_PropertiesSheetModel::PropertyItem::PropertyItem()
    : property(nullptr), level(0), parentItem(nullptr) {
}

bool LC_PropertiesSheetModel::PropertyItem::collapsed() const {
    return property->isCollapsed();
}

LC_PropertiesSheetModel::PropertyItem* LC_PropertiesSheetModel::findItem(PropertyItem* currentItem, const LC_Property* property) const {
    if (currentItem != nullptr && property != nullptr) {
        if (property == currentItem->property) {
            return currentItem;
        }
        for (auto& item : currentItem->children) {
            const auto found = findItem(item.get(), property);
            if (found != nullptr) {
                return found;
            }
        }
    }
    return nullptr;
}

void LC_PropertiesSheetModel::setupPropertyItemView(PropertyItem* item) {
    const auto property = item->property;
    const auto view = m_viewFactory->createView(*property);
    Q_ASSERT(view != nullptr);

    item->view.reset(view);
    item->children.clear();

    applyViewAttributes(property, view);

    const auto container = property->asContainer();
    if (property->asAtomic() != nullptr && container != nullptr) {
        for (const auto child : container->childProperties()) {
            auto childItem = createItemsTree(child);
            childItem->parentItem = item;
            item->children.emplace_back(childItem);
        }
    }
    else {
        for (int i = 0, n = view->getSubPropertyCount(); i < n; ++i) {
            const auto child = view->getSubProperty(i);
            Q_ASSERT(child);

            auto childItem = createItemsTree(child);
            if (childItem != nullptr) {
                childItem->parentItem = item;
                item->children.emplace_back(childItem);
            }
        }
    }
}

LC_PropertiesSheetModel::PropertyItem* LC_PropertiesSheetModel::createItemsTree(LC_Property* rootProperty) {
    if (rootProperty == nullptr) {
        return nullptr;
    }

    auto item = new PropertyItem;
    item->property = rootProperty;
    auto& connections = item->connections;

    connections.push_back(connect(rootProperty, &LC_Property::afterPropertyChange, this,
                                  [item, this](const LC_PropertyChangeReason reason) {
                                      onPropertyDidChangeSelf(reason, item);
                                  }));

    setupPropertyItemView(item);
    return item;
}

void LC_PropertiesSheetModel::onPropertyDidChangeSelf(const LC_PropertyChangeReason reason, PropertyItem* item) {
    if (!reason) {
        return;
    }

    if ((reason & PropertyChangeReasonUpdateView) != 0u) {
        setupPropertyItemView(item);
    }
    else if ((reason & PropertyChangeReasonNewAttribute) != 0u) {
        applyItemViewAttr(item);
    }

    if (m_stopInvalidate != 0u) {
        m_lastChangeReason |= reason;
    }
    else {
        updateWithReason(reason);
    }

    emit propertyDidChange(reason, item);
}

void LC_PropertiesSheetModel::updateWithReason(const LC_PropertyChangeReason reason) {
    if ((reason & PropertyChangeReasonChildren) != 0u) {
        updateTree();
        emit modelChanged();
    }
    else if ((reason & (PropertyChangeReasonState | PropertyChangeReasonUpdateView)) != 0u) {
        emit modelChanged();
    }
    else {
        emit modelDataChanged();
    }
}

void LC_PropertiesSheetModel::applyViewAttributes(const LC_Property* property, LC_PropertyView* view) {
    const auto desc = property->getViewDescriptor();
    if (desc != nullptr) {
        view->applyAttributes(*desc);
    }
}

void LC_PropertiesSheetModel::applyItemViewAttr(const PropertyItem* item) {
    const auto property = item->property;
    const auto& view = item->view;
    applyViewAttributes(property, view.get());
    for (auto& i : item->children) {
        applyItemViewAttr(i.get());
    }
}

LC_Property* LC_PropertiesSheetModel::getParentProperty(const LC_Property* property) const {
    const auto item = findItem(m_itemsTree.get(), property);
    if (nullptr != item && nullptr != item->parentItem) {
        return item->parentItem->property;
    }
    return nullptr;
}

void LC_PropertiesSheetModel::setPropertyContainer(LC_PropertyContainer* container) {
    m_propertyContainer = container;
}

void LC_PropertiesSheetModel::updateTree() {
    const auto rootItem = createItemsTree(m_propertyContainer);
    m_itemsTree.reset(rootItem);
}

void LC_PropertiesSheetModel::stopInvalidate(const bool enable) {
    if (enable) {
        if (0 == m_stopInvalidate++) {
            m_lastChangeReason = LC_PropertyChangeReason(0);
        }
    }
    else {
        if (--m_stopInvalidate == 0) {
            updateWithReason(m_lastChangeReason);
        }
    }
}

void LC_PropertiesSheetModel::invalidateCached() const {
    const auto root = m_itemsTree.get();
    if (root != nullptr) {
        invalidateCached(root);
    }
}

void LC_PropertiesSheetModel::invalidateCached(const PropertyItem* currentItem) const {
    for (auto& item : currentItem->children) {
        item->view->invalidateCached();
        invalidateCached(item.get());
    }
}
