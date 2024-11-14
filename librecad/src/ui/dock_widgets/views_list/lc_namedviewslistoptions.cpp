/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_namedviewslistoptions.h"
#include "rs_settings.h"

LC_NamedViewsListOptions::LC_NamedViewsListOptions() {}

void LC_NamedViewsListOptions::load() {
    LC_GROUP_GUARD("ViewsList");
    {
       showViewInfoToolTip = LC_GET_BOOL("ShowTooltip", true);
       showTypeIcon = LC_GET_BOOL("ShowTypeIcon", false);
       askForDeletionConfirmation = LC_GET_BOOL("ConfirmDelete", false);
       duplicatedNameReplacesSilently = LC_GET_BOOL("ReplaceDuplicateSilently", false);
       doubleClickPolicy = LC_GET_INT("DoubleClickPolicy", RENAME);
       restoreViewBySingleClick = LC_GET_BOOL("RestoreViewBySingleClick", true);
    }
}

void LC_NamedViewsListOptions::save() const {
    LC_GROUP_GUARD("ViewsList");
    {
        LC_SET("ShowTooltip", showViewInfoToolTip);
        LC_SET("ShowTypeIcon", showTypeIcon);
        LC_SET("ConfirmDelete", askForDeletionConfirmation);
        LC_SET("ReplaceDuplicateSilently", duplicatedNameReplacesSilently);
        LC_SET("DoubleClickPolicy", doubleClickPolicy);
        LC_SET("RestoreViewBySingleClick", restoreViewBySingleClick);
    }
}
