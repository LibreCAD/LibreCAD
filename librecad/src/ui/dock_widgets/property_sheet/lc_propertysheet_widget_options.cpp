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

#include "lc_propertysheet_widget_options.h"

#include "rs_settings.h"

void LC_PropertySheetWidgetOptions::save() const {
    LC_GROUP_GUARD("PropertySheet");
    {
        LC_SET("noSelectionActivePen", noSelectionActivePen);
        LC_SET("noSelectionActiveLayer", noSelectionActiveLayer);
        LC_SET("noSelectionNamedView", noSelectionNamedView);
        LC_SET("noSelectionUCS", noSelectionUCS);
        LC_SET("noSelectionGrid", noSelectionGrid);
        LC_SET("noSelectionDrawingUnits", noSelectionDrawingUnits);
        LC_SET("noSelectionPrintPaper", noSelectionPrintPaper);
        LC_SET("noSelectionWorkspace", noSelectionWorkspace);
        LC_SET("noSelectionGraphicView", noSelectionGraphicView);
        LC_SET("showLinks", showLinks);
        LC_SET("showSingleEntityCommands", showSingleEntityCommands);
        LC_SET("showMultiEntityCommands", showMultiEntityCommands);
        LC_SET("showComputed", showComputed);
        LC_SET("duplicateSelectionAction", duplicateSelectionAction);
        LC_SET("showToolOptions", showToolOptions);
        LC_SET("fontSize", fontSize);
    }
}

void LC_PropertySheetWidgetOptions::load() {
    LC_GROUP_GUARD("PropertySheet");
    {
        noSelectionActivePen = LC_GET_BOOL("noSelectionActivePen", true);
        noSelectionActiveLayer = LC_GET_BOOL("noSelectionActiveLayer", true);
        noSelectionNamedView = LC_GET_BOOL("noSelectionNamedView", true);
        noSelectionUCS = LC_GET_BOOL("noSelectionUCS", true);
        noSelectionGrid = LC_GET_BOOL("noSelectionGrid", true);
        noSelectionDrawingUnits = LC_GET_BOOL("noSelectionDrawingUnits", true);
        noSelectionPrintPaper = LC_GET_BOOL("noSelectionPrintPaper", true);
        noSelectionWorkspace = LC_GET_BOOL("noSelectionWorkspace", true);
        noSelectionGraphicView = LC_GET_BOOL("noSelectionGraphicView", true);
        showLinks = LC_GET_BOOL("showLinks", true);
        showSingleEntityCommands = LC_GET_BOOL("showSingleEntityCommands", true);
        showMultiEntityCommands = LC_GET_BOOL("showMultiEntityCommands", true);
        showComputed = LC_GET_BOOL("showComputed", true);
        duplicateSelectionAction = LC_GET_BOOL("duplicateSelectionAction", true);
        showToolOptions = LC_GET_BOOL("showToolOptions", true);
        fontSize = LC_GET_INT("fontSize", 9);
    }
}
