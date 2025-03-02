/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_ucslistoptions.h"
#include "rs_settings.h"

LC_UCSListOptions::LC_UCSListOptions() {}

void LC_UCSListOptions::load() {
    LC_GROUP("Appearance");
    {
        ucsApplyingPolicy = LC_GET_INT("UCSApplyPolicy",0);
        highlightBlinksCount = LC_GET_INT("UCSHighlightBlinkCount",10);
        highlightBlinksDelay =  LC_GET_INT("UCSHighlightBlinkDelay",250);
    }
    LC_GROUP_END();

    LC_GROUP_GUARD("Widget.UCSList");
    {
        showViewInfoToolTip = LC_GET_BOOL("ShowTooltip", true);
        showColumnTypeIcon = LC_GET_BOOL("ShowColumnTypeIcon", false);
        showColumnGridType = LC_GET_BOOL("ShowColumnGridType", false);
        showColumnPositionAndAngle = LC_GET_BOOL("ShowColumnPositionAndAngle", false);
        askForDeletionConfirmation = LC_GET_BOOL("ConfirmDelete", false);
        restoreViewBySingleClick = LC_GET_BOOL("SingleClickRestore", false);
        doubleClickPolicy = LC_GET_INT("DoubleClickPolicy",APPLY_UCS);
    }
}

void LC_UCSListOptions::save() const{
    LC_GROUP("Appearance");
    {
        LC_SET("UCSApplyPolicy",ucsApplyingPolicy);
        LC_SET("UCSHighlightBlinkCount",highlightBlinksCount);
        LC_SET("UCSHighlightBlinkDelay",highlightBlinksDelay);
    }
    LC_GROUP_END();
    LC_GROUP_GUARD("Widget.UCSList");
    {
        LC_SET("ShowTooltip", showViewInfoToolTip);
        LC_SET("ShowColumnTypeIcon", showColumnTypeIcon);
        LC_SET("ShowColumnGridType", showColumnGridType);
        LC_SET("ShowColumnPositionAndAngle", showColumnPositionAndAngle);
        LC_SET("ConfirmDelete", askForDeletionConfirmation);
        LC_SET("DoubleClickPolicy", doubleClickPolicy);
        LC_GET_BOOL("SingleClickRestore", restoreViewBySingleClick);
    }
}
