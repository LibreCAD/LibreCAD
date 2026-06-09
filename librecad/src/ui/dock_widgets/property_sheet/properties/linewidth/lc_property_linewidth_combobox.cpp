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
#include "lc_property_linewidth_combobox.h"

#include "lc_peninforegistry.h"

LC_PropertyLineWidthCombobox::LC_PropertyLineWidthCombobox(LC_PropertyLineWidthComboboxView* view, QWidget* parent, const bool showByLayer,
                                                           const bool showUnchanged)
    : QG_WidthBox(showByLayer, showUnchanged, parent), m_view(view), m_property(static_cast<LC_PropertyLineWidth*>(view->getProperty())) {
    setLineEdit(nullptr);
    updateByProperty();

    connect(m_property, &LC_Property::afterPropertyChange, this, &LC_PropertyLineWidthCombobox::onPropertyDidChange);
    connect(this, &QG_WidthBox::widthChanged, this, &LC_PropertyLineWidthCombobox::onLineWidthChanged);
}

void LC_PropertyLineWidthCombobox::updateByProperty() {
    m_updating++;
    const auto stateProperty = m_view->getStateProperty();
    setEnabled(stateProperty->isEditableByUser());
    const RS2::LineWidth lineWidth = m_property->value();
    setWidth(lineWidth);
    m_updating--;
}

void LC_PropertyLineWidthCombobox::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonValue)) {
        updateByProperty();
    }
}

void LC_PropertyLineWidthCombobox::onLineWidthChanged(const RS2::LineWidth& linewidth) const {
    if (m_updating) {
        return;
    }
    m_property->setValue(linewidth, m_view->changeReasonDueToEdit());
}

void LC_PropertyLineWidthCombobox::hidePopup() {
    QG_WidthBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}
