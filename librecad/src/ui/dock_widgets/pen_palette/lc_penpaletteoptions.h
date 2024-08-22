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
#ifndef LC_PENPALETTEOPTIONS_H
#define LC_PENPALETTEOPTIONS_H

#include <QColor>
#include "lc_peninforegistry.h"

class LC_PenPaletteOptions
{
public:
    enum {
        DOUBLE_CLICK_DOES_NOTHING,
        DOUBLE_CLICK_SELECT_ENTITIES_BY_ATTRIBUTES_PEN,
        DOUBLE_CLICK_SELECT_ENTITIES_BY_DRAWING_PEN
    };

    /**
     * color for active item background in table
     */
    QColor activeItemBGColor = QColor("azure");
    /**
     * color used to highlight items that matched to regexp
     */
    QColor matchedItemColor = QColor("blue");
    /**
     * color for table's grid drawing
     */
    QColor itemsGridColor = QColor(216, 216, 216);
    /**
     * mode for displaying color name
     */
    int colorNameDisplayMode {LC_PenInfoRegistry::RGB};
    /**
     * should we show all fields for active pen in the table or only for name
     */
    bool showEntireRowBold {true};
    /**
     *  controls whether items matched by regexp should be highlighted or filtered
     */
    bool filterIsInHighlightMode {false};
    /**
     * controls whether items regexp matching should ignore case
     */
    bool ignoreCaseOnMatch {true};

    // fixme - well, probably we should use some reasonable defaults with paths related to the app there...
    /**
     * name of file where pens are stored
     */
    QString pensFileName{""};

    /**
     * columns visibility flags
     */
    bool showColorIcon {true};
    bool showColorName {false};
    bool showTypeIcon{true};
    bool showTypeName{false};
    bool showWidthIcon{true};
    bool showWidthName{false};
    bool showNoSelectionMessage{false};

    /**
     * flag that defines how to process double click on table item
     */
    int doubleClickOnTableMode = DOUBLE_CLICK_SELECT_ENTITIES_BY_ATTRIBUTES_PEN;

    /**
     * should we show tooltip in table or not
     */
    bool showToolTip {true};

    /**
     * Saves options in settings
     */
    void saveToSettings();
    /**
     * loads options from settings
     */
    void loadFromSettings();

};

#endif // LC_PENPALETTEOPTIONS_H
