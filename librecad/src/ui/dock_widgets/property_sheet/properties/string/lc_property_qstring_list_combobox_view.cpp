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

#include "lc_property_qstring_list_combobox_view.h"

#include "lc_property_combobox.h"
#include "lc_property_qstring_list_combobox_view_handler.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyQStringListComboBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameComboBox();
const QByteArray LC_PropertyQStringListComboBoxView::ATTR_ITEMS = QByteArrayLiteral("items");
const QByteArray LC_PropertyQStringListComboBoxView::ATTR_ITEMS_ICON_NAMES = QByteArrayLiteral("itemsIconNames");
const QByteArray LC_PropertyQStringListComboBoxView::ATTR_ITEMS_DATA = QByteArrayLiteral("itemsData");
const QByteArray LC_PropertyQStringListComboBoxView::ATTR_EDITABLE = QByteArrayLiteral("editable");


LC_PropertyQStringListComboBoxView::LC_PropertyQStringListComboBoxView(LC_PropertyQString* property)
    : LC_PropertyQStringLineEditView(property) {
}

void LC_PropertyQStringListComboBoxView::doApplyAttributes(const LC_PropertyViewDescriptor& attrs) {
    m_viewDescriptor = attrs;
}

QWidget* LC_PropertyQStringListComboBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    auto* editor = new LC_PropertyComboBox(this, parent);
    editor->setGeometry(rect);
    const auto handler = new LC_PropertyQStringListComboBoxViewHandler(this, *editor, m_viewDescriptor);
    handler->connectCombobox(*editor);
    handler->doUpdateEditor();
    if (isInplaceEditAllowed(ctx)) {
        editor->showPopup();
    }
    return editor;
}
