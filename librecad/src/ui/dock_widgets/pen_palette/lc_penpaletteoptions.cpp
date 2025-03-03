/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
#include "lc_penpaletteoptions.h"
#include "rs_settings.h"

/**
 * Straightforwards storing options to settings
 */
void LC_PenPaletteOptions:: loadFromSettings(){

    LC_PenPaletteOptions defaults;

    LC_GROUP_GUARD("PenPaletteWidget");
    {
        activeItemBGColor = QColor(LC_GET_STR("activeItemBgColor", defaults.activeItemBGColor.name()));
        matchedItemColor = QColor(LC_GET_STR("matchedItemBgColor", defaults.matchedItemColor.name()));
        itemsGridColor = QColor(LC_GET_STR("gridColor", defaults.itemsGridColor.name()));

        showToolTip = LC_GET_BOOL("showToolTip", defaults.showToolTip);

        showColorName = LC_GET_BOOL("showColorNameCol", defaults.showColorName);
        showColorIcon = LC_GET_BOOL("showColorIconCol", defaults.showColorIcon);

        showTypeName = LC_GET_BOOL("showLineTypeNameCol", defaults.showTypeName);
        showTypeIcon = LC_GET_BOOL("showLineTypeIconCol", defaults.showTypeIcon);

        showWidthName = LC_GET_BOOL("showLineWidthNameCol", defaults.showWidthName);
        showWidthIcon = LC_GET_BOOL("showLineWidthIconCol", defaults.showWidthIcon);

        showEntireRowBold = LC_GET_BOOL("showEntireActiveRowBold", defaults.showEntireRowBold);
        filterIsInHighlightMode = LC_GET_BOOL("filterInHighlightsMode", defaults.filterIsInHighlightMode);
        ignoreCaseOnMatch = LC_GET_BOOL("ignoreCaseOnMatch", defaults.ignoreCaseOnMatch);
        showNoSelectionMessage = LC_GET_BOOL("showNoSelectionMessage", defaults.showNoSelectionMessage);

        colorNameDisplayMode = LC_GET_INT("colorDisplayMode", defaults.colorNameDisplayMode);
        doubleClickOnTableMode = LC_GET_INT("doubleClickOnTableMode", defaults.doubleClickOnTableMode);

        pensFileName = LC_GET_STR("pensFile", defaults.pensFileName);
    }
}

/**
 * Straightforward loading from settings
 */
void LC_PenPaletteOptions::saveToSettings(){
    LC_PenPaletteOptions defaults;

    LC_GROUP_GUARD("PenPaletteWidget");
    {
        LC_SET("activeItemBgColor", activeItemBGColor.name());
        LC_SET("matchedItemBgColor", matchedItemColor.name());
        LC_SET("gridColor", itemsGridColor.name());

        LC_SET("showToolTip", showToolTip);

        LC_SET("showColorNameCol", showColorName);
        LC_SET("showColorIconCol", showColorIcon);

        LC_SET("showLineTypeNameCol", showTypeName);
        LC_SET("showLineTypeIconCol", showTypeIcon);

        LC_SET("showLineWidthNameCol", showWidthName);
        LC_SET("showLineWidthIconCol", showWidthIcon);

        LC_SET("showEntireActiveRowBold", showEntireRowBold);
        LC_SET("filterInHighlightsMode", filterIsInHighlightMode);
        LC_SET("ignoreCaseOnMatch", ignoreCaseOnMatch);
        LC_SET("showNoSelectionMessage", showNoSelectionMessage);

        LC_SET("colorDisplayMode", colorNameDisplayMode);
        LC_SET("doubleClickOnTableMode", doubleClickOnTableMode);

        LC_SET("pensFile", pensFileName);
    }
}
