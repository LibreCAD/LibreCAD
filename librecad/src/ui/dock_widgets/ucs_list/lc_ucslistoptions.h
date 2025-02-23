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

#ifndef LC_UCSLISTOPTIONS_H
#define LC_UCSLISTOPTIONS_H

class LC_UCSListOptions{
public:
    LC_UCSListOptions();
    void load();
    void save() const;

    bool showColumnTypeIcon = true;
    bool showColumnPositionAndAngle = true;
    bool showColumnGridType = true;
    bool showViewInfoToolTip = true;
    bool duplicatedNameReplacesSilently = false;
    bool askForDeletionConfirmation = true;
    bool restoreViewBySingleClick= false;
    int doubleClickPolicy = DO_NOTHING;
    int ucsApplyingPolicy = 0;
    int highlightBlinksCount = 10;
    int highlightBlinksDelay = 250;

    enum DoubleClickPolicy{
        DO_NOTHING,
        EDIT_UCS,
        APPLY_UCS,
        SHOW_MARKER
    };
};

#endif // LC_UCSLISTOPTIONS_H
