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

#ifndef LC_PROPERTYLINETYPECOMBOBOX_H
#define LC_PROPERTYLINETYPECOMBOBOX_H

#include "lc_property.h"
#include "lc_property_linetype.h"
#include "lc_property_linetype_combobox_view.h"
#include "qg_linetypebox.h"

class LC_PropertyLineTypeCombobox : public QG_LineTypeBox {
public:
    explicit LC_PropertyLineTypeCombobox(LC_PropertyLineTypeComboboxView* view, QWidget* parent, bool showByLayer, bool showUnchanged);
    void hidePopup() override;

protected:
    void updateByProperty();
    void onPropertyDidChange(LC_PropertyChangeReason reason);
    void onLinetypeChanged(const RS2::LineType& linetype);

private:
    LC_PropertyLineTypeComboboxView* m_view = nullptr;
    LC_PropertyLineType* m_property = nullptr;
    unsigned m_updating = 0;
};

#endif
