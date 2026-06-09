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

#include "lc_property_rscolor_combobox.h"

#include <QPaintEvent>
#include <QStyledItemDelegate>

#include "lc_property_rscolor_combobox_view.h"

LC_PropertyRSColorComboBox::LC_PropertyRSColorComboBox(LC_PropertyRSColorComboBoxView* view, QWidget* parent, const bool showByLayer,
                                                       const bool showUnchanged)
    : QG_ColorBox(showByLayer, showUnchanged, parent), m_view(view), m_property(static_cast<LC_PropertyRSColor*>(view->getProperty())),
      m_updating(0) {
    setLineEdit(nullptr);
    updateByProperty();

    connect(m_property, &LC_Property::afterPropertyChange, this, &LC_PropertyRSColorComboBox::onPropertyDidChange);
    connect(this, &QG_ColorBox::colorChanged, this, &LC_PropertyRSColorComboBox::onColorChanged);
}

void LC_PropertyRSColorComboBox::updateByProperty() {
    m_updating++;
    const auto stateProperty = m_view->getStateProperty();
    setEnabled(stateProperty->isEditableByUser());
    const RS_Color color = m_property->value();
    setColor(color);
    m_updating--;
}

void LC_PropertyRSColorComboBox::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonValue)) {
        updateByProperty();
    }
}

void LC_PropertyRSColorComboBox::onColorChanged(const RS_Color& color) {
    if (m_updating) {
        return;
    }
    m_property->setValue(color, m_view->changeReasonDueToEdit());
}

void LC_PropertyRSColorComboBox::hidePopup() {
    QG_ColorBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}
