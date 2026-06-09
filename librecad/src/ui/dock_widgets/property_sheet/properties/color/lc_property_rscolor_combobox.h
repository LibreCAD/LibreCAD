/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_PROPERTYRSCOLORCOMBOBOX_H
#define LC_PROPERTYRSCOLORCOMBOBOX_H

#include "lc_property_rscolor.h"
#include "qg_colorbox.h"

class LC_PropertyRSColor;
class LC_PropertyRSColorComboBoxView;

class LC_PropertyRSColorComboBox : public QG_ColorBox {
public:
    explicit LC_PropertyRSColorComboBox(LC_PropertyRSColorComboBoxView* view, QWidget* parent, bool showByLayer, bool showUnchanged);
    void hidePopup() override;

protected:
    void updateByProperty();
    void onPropertyDidChange(LC_PropertyChangeReason reason);
    void onColorChanged(const RS_Color& color);
    class StyledItemDelegate;

private:
    LC_PropertyRSColorComboBoxView* m_view;
    LC_PropertyRSColor* m_property;
    unsigned m_updating;
};

#endif
