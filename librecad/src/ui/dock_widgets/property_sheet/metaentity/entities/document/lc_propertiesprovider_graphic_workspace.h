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

#ifndef LC_PROPERTIESPROVIDERGRAPHICWORKSPACE_H
#define LC_PROPERTIESPROVIDERGRAPHICWORKSPACE_H

#include "lc_propertiesprovider_graphic_component.h"

class LC_PropertiesProviderGraphicWorkspace : public LC_PropertiesProviderGenericComponent {
    Q_OBJECT
public:
    LC_PropertiesProviderGraphicWorkspace(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderGenericComponent(actionContext, widget) {
    }

    void fillDocumentProperties(LC_PropertyContainer* container) override;
    static const QString SECTION_WORKSPACE;
protected:
    void createWorkspaceSelector(LC_PropertyContainer* container) const;
    void createUIActionProperty(LC_PropertyContainer* cont, const char* actionName, const LC_Property::Names& names);
    void createShowFullScreen(LC_PropertyContainer* cont);
    void createShowMainMenu(LC_PropertyContainer* cont);
    void createShowStatusBar(LC_PropertyContainer* cont);
    void createWorkspaceCommands(LC_PropertyContainer* cont) const;
};

#endif
