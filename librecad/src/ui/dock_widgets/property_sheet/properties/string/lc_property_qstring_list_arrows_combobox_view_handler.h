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

#ifndef LC_PROPERTYQSTRINGLISTARROWSCOMBOBOXVIEWHANDLER_H
#define LC_PROPERTYQSTRINGLISTARROWSCOMBOBOXVIEWHANDLER_H

#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_qstring.h"
#include "lc_property_qstring_list_combobox_view_handler.h"

class LC_PropertyComboBox;

class LC_PropertyQStringListArrowsComboboxViewHandler : LC_PropertyQStringListComboBoxViewHandler {
    Q_OBJECT

public:
    LC_PropertyQStringListArrowsComboboxViewHandler(LC_PropertyViewEditable* view, LC_PropertyComboBox& editor,
                                                    const LC_PropertyViewDescriptor& descriptor);
    void connectCombobox(LC_PropertyComboBox& editor) override;
    void doUpdateEditor() override;

private:
    QStringList m_blocksList;
};
#endif
