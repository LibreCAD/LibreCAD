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

LC_PenPaletteOptions::LC_PenPaletteOptions(){}

/**
 * Straightforwards storing options to settings
 */
void LC_PenPaletteOptions:: loadFromSettings(){

    LC_PenPaletteOptions defaults;

    RS_SETTINGS->beginGroup("PenPaletteWidget");

    activeItemBGColor = QColor(RS_SETTINGS->readEntry("/activeItemBgColor", defaults.activeItemBGColor.name()));
    matchedItemColor = QColor(RS_SETTINGS->readEntry("/matchedItemBgColor", defaults.matchedItemColor.name()));    
    itemsGridColor =  QColor(RS_SETTINGS->readEntry("/gridColor", defaults.itemsGridColor.name()));

    showToolTip = RS_SETTINGS->readNumEntry("/showToolTip", defaults.showToolTip ? 1:0) == 1;
    
    showColorName = RS_SETTINGS->readNumEntry("/showColorNameCol", defaults.showColorName ? 1:0) == 1;    
    showColorIcon = RS_SETTINGS->readNumEntry("/showColorIconCol", defaults.showColorIcon ? 1:0) == 1;

    showTypeName = RS_SETTINGS->readNumEntry("/showLineTypeNameCol", defaults.showTypeName ? 1:0) == 1;
    showTypeIcon = RS_SETTINGS->readNumEntry("/showLineTypeIconCol", defaults.showTypeIcon ? 1:0) == 1;

    showWidthName = RS_SETTINGS->readNumEntry("/showLineWidthNameCol", defaults.showWidthName ? 1:0) == 1;
    showWidthIcon = RS_SETTINGS->readNumEntry("/showLineWidthIconCol", defaults.showWidthIcon ? 1:0) == 1;

    showEntireRowBold = RS_SETTINGS->readNumEntry("/showEntireActiveRowBold", defaults.showEntireRowBold ? 1:0) == 1;
    filterIsInHighlightMode = RS_SETTINGS->readNumEntry("/filterInHighlightsMode", defaults.filterIsInHighlightMode ? 1 : 0) == 1;
    ignoreCaseOnMatch = RS_SETTINGS->readNumEntry("/ignoreCaseOnMatch", defaults.ignoreCaseOnMatch ? 1:0) == 1;
    showNoSelectionMessage = RS_SETTINGS->readNumEntry("/showNoSelectionMessage", defaults.showNoSelectionMessage ? 1:0) == 1;

    colorNameDisplayMode = RS_SETTINGS->readNumEntry("/colorDisplayMode", defaults.colorNameDisplayMode);
    doubleClickOnTableMode = RS_SETTINGS->readNumEntry("/doubleClickOnTableMode", defaults.doubleClickOnTableMode);

    pensFileName = RS_SETTINGS->readEntry("/pensFile", defaults.pensFileName);

    RS_SETTINGS->endGroup();

}

/**
 * Straightforward loading from settings
 */
void LC_PenPaletteOptions::saveToSettings(){
    LC_PenPaletteOptions defaults;

    RS_SETTINGS->beginGroup("PenPaletteWidget");

    RS_SETTINGS->writeEntry("/activeItemBgColor", activeItemBGColor.name());
    RS_SETTINGS->writeEntry("/matchedItemBgColor", matchedItemColor.name());
    RS_SETTINGS->writeEntry("/gridColor", itemsGridColor.name());

    RS_SETTINGS->writeEntry("/showToolTip", showToolTip ? 1 : 0);

    RS_SETTINGS->writeEntry("/showColorNameCol", showColorName ? 1 : 0);
    RS_SETTINGS->writeEntry("/showColorIconCol", showColorIcon ? 1 : 0);

    RS_SETTINGS->writeEntry("/showLineTypeNameCol", showTypeName ? 1 : 0);
    RS_SETTINGS->writeEntry("/showLineTypeIconCol", showTypeIcon ? 1 : 0);

    RS_SETTINGS->writeEntry("/showLineWidthNameCol", showWidthName ? 1 : 0);
    RS_SETTINGS->writeEntry("/showLineWidthIconCol", showWidthIcon ? 1 : 0);

    RS_SETTINGS->writeEntry("/showEntireActiveRowBold", showEntireRowBold ? 1 : 0);
    RS_SETTINGS->writeEntry("/filterInHighlightsMode", filterIsInHighlightMode ? 1 : 0);
    RS_SETTINGS->writeEntry("/ignoreCaseOnMatch", ignoreCaseOnMatch ? 1 : 0);
    RS_SETTINGS->writeEntry("/showNoSelectionMessage", showNoSelectionMessage ? 1 : 0);

    RS_SETTINGS->writeEntry("/colorDisplayMode", colorNameDisplayMode);
    RS_SETTINGS->writeEntry("/doubleClickOnTableMode", doubleClickOnTableMode);


    RS_SETTINGS->writeEntry("/pensFile", pensFileName);

    RS_SETTINGS->endGroup();
}

