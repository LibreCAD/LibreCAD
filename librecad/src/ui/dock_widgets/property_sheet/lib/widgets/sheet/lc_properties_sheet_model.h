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

#ifndef LC_PROPERTIESSHEETMODEL_H
#define LC_PROPERTIESSHEETMODEL_H
#include <memory>

#include "lc_guardedconnectionslist.h"
#include "lc_property_container.h"
#include "lc_property_view_factory.h"

class LC_PropertyView;

class LC_PropertiesSheetModel : public QObject {
    Q_OBJECT

public:
    struct PropertyItem {
        PropertyItem();
        bool collapsed() const;

        LC_Property* property;
        std::unique_ptr<LC_PropertyView> view;
        int level;

        PropertyItem* parentItem;
        std::vector<std::unique_ptr<PropertyItem>> children;
        LC_GuardedConnectionsList connections;
    };

    explicit LC_PropertiesSheetModel(LC_PropertyContainer* propertySet)
        : m_propertyContainer{propertySet}, m_viewFactory{nullptr} {
    }

    PropertyItem* findItem(PropertyItem* currentItem, const LC_Property* property) const;
    void setupPropertyItemView(PropertyItem* item);
    PropertyItem* createItemsTree(LC_Property* rootProperty);
    void applyItemViewAttr(const PropertyItem* item);
    LC_Property* getParentProperty(const LC_Property* property) const;

    PropertyItem* getRootItem() const {
        return m_itemsTree.get();
    }

    void setPropertyContainer(LC_PropertyContainer* container);

    LC_PropertyContainer* getPropertyContainer() const {
        return m_propertyContainer;
    }

    void updateTree();

    LC_PropertyViewFactory* getViewFactory() const {
        return m_viewFactory;
    }

    void setViewFactory(LC_PropertyViewFactory* viewFactory) {
        m_viewFactory = viewFactory;
    }

    void stopInvalidate(bool enable);
    void invalidateCached() const;
    void invalidateCached(const PropertyItem* currentItem) const;
signals :
    void propertyDidChange(LC_PropertyChangeReason reason, LC_PropertiesSheetModel::PropertyItem* item);
    void modelChanged();
    void modelDataChanged();

protected slots :
    void onPropertyDidChangeSelf(LC_PropertyChangeReason reason, LC_PropertiesSheetModel::PropertyItem* item);

protected:
    void updateWithReason(LC_PropertyChangeReason reason);
    void applyViewAttributes(const LC_Property* property, LC_PropertyView* view);

private:
    std::unique_ptr<PropertyItem> m_itemsTree;
    LC_PropertyContainer* m_propertyContainer{nullptr};
    LC_PropertyViewFactory* m_viewFactory;
    unsigned m_stopInvalidate{0};
    LC_PropertyChangeReason m_lastChangeReason{0};
};

#endif
