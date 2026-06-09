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

#include "lc_property_qstring_font_combobox.h"

#include "rs_font.h"

LC_PropertyQStringFontCombobox::LC_PropertyQStringFontCombobox(LC_PropertyQStringFontComboboxView* view, QWidget* parent) :
    QG_FontBox(parent), m_view{view}, m_property{&view->typedProperty()} {
    init();
    connect(this, &QG_FontBox::fontChanged, this, &LC_PropertyQStringFontCombobox::onFontChanged);
}

void LC_PropertyQStringFontCombobox::updateByProperty() {
    m_updating++;
    const auto stateProperty = m_view->getStateProperty();
    setEnabled(stateProperty->isEditableByUser());
    const QString fontName = m_property->value();
    setFont(fontName);
    m_updating--;
}

void LC_PropertyQStringFontCombobox::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonValue)) {
        updateByProperty();
    }
}

void LC_PropertyQStringFontCombobox::onFontChanged([[maybe_unused]] RS_Font* font) {
    if (m_updating) {
        return;
    }
    const QString name = currentText();
    m_property->setValue(name, m_view->changeReasonDueToEdit());
}

void LC_PropertyQStringFontCombobox::hidePopup() {
    QComboBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}
