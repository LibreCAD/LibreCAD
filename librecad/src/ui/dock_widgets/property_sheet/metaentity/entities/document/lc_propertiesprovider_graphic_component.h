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

#ifndef LC_PROPERTIESPROVIDERGRAPHICCOMPONENT_H
#define LC_PROPERTIESPROVIDERGRAPHICCOMPONENT_H

#include <QTimer>

#include "lc_entity_type_propertiesprovider.h"
#include "qc_applicationwindow.h"

class LC_PropertiesProviderDocumentComponent: public LC_EntityTypePropertiesProvider {
    Q_OBJECT
public:
    LC_PropertiesProviderDocumentComponent(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget, RS2::EntityType entityType)
        : LC_EntityTypePropertiesProvider(entityType, actionContext, widget) {
    }

    virtual void fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) = 0;
protected:
    void doCreateEntitySpecificProperties([[maybe_unused]]LC_PropertyContainer* container,[[maybe_unused]] const QList<RS_Entity*>& list) override{}

    void notifyDrawingOptionsChanged() const {
        m_widget->stopInplaceEdit();
        QTimer::singleShot(30, []()-> void {
            QC_ApplicationWindow::getAppWindow()->notifyCurrentDrawingOptionsChanged();
        });
    }
};

class LC_PropertiesProviderGraphicComponent: public LC_PropertiesProviderDocumentComponent {
public:
    LC_PropertiesProviderGraphicComponent(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderDocumentComponent(actionContext, widget, RS2::EntityGraphic) {
    }
};

class LC_PropertiesProviderBlockComponent: public LC_EntityTypePropertiesProvider {
public:
    LC_PropertiesProviderBlockComponent(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_EntityTypePropertiesProvider(RS2::EntityBlock, actionContext, widget) {
    }
    virtual void fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic, RS_Block *block) = 0;
protected:
    void doCreateEntitySpecificProperties([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& list) override{}
};

class LC_PropertiesProviderGenericComponent: public LC_EntityTypePropertiesProvider {
public:
    LC_PropertiesProviderGenericComponent(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_EntityTypePropertiesProvider(RS2::EntityUnknown, actionContext, widget) {
    }
    virtual void fillDocumentProperties(LC_PropertyContainer* container) = 0;
protected:
    void doCreateEntitySpecificProperties([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& list) override{}
};
#endif
