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
#include "lc_property_linetype_combobox.h"

#include "lc_peninforegistry.h"
#include "lc_linetypelist.h"
#include "lc_property_linetype_combobox_view.h"

LC_PropertyLineTypeCombobox::LC_PropertyLineTypeCombobox(LC_PropertyLineTypeComboboxView* view, QWidget* parent, const bool showByLayer,
                                                         const bool showUnchanged)
    : QG_LineTypeBox(showByLayer, showUnchanged, parent), m_view(view), m_property(static_cast<LC_PropertyLineType*>(view->getProperty())) {
    setLineEdit(nullptr);
    updateByProperty();

    connect(m_property, &LC_Property::afterPropertyChange, this, &LC_PropertyLineTypeCombobox::onPropertyDidChange);
    connect(this, &QG_LineTypeBox::lineTypeChanged, this, &LC_PropertyLineTypeCombobox::onLinetypeChanged);
}

void LC_PropertyLineTypeCombobox::hidePopup() {
    QG_LineTypeBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}

void LC_PropertyLineTypeCombobox::updateByProperty() {
    m_updating++;
    const auto stateProperty = m_view->getStateProperty();
    setEnabled(stateProperty->isEditableByUser());
    const RS2::LineType linetype = m_property->value();
    setLineType(linetype);
    m_updating--;
}

void LC_PropertyLineTypeCombobox::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonValue)) {
        updateByProperty();
    }
}

void LC_PropertyLineTypeCombobox::onLinetypeChanged(const RS2::LineType& linetype) {
    if (m_updating) {
        return;
    }
    m_property->setValue(linetype, m_view->changeReasonDueToEdit());
}

LC_LineTypeIdentity LC_LineTypeIdentity::fromPen(const RS_Pen& pen) {
    LC_LineTypeIdentity value;
    value.type = pen.getLineType();
    value.hasName = pen.hasLineTypeName();
    if (value.hasName) value.name = pen.getLineTypeName();
    return value;
}

void LC_LineTypeIdentity::applyTo(RS_Pen& pen) const {
    if (hasName) pen.setLineTypeName(name);
    else pen.setLineType(type);
}

LC_PropertyNamedLineTypeCombobox::LC_PropertyNamedLineTypeCombobox(
        LC_PropertyNamedLineTypeComboboxView* view, QWidget* parent)
    : QG_LineTypeBox(parent), m_view(view),
      m_property(static_cast<LC_PropertyNamedLineType*>(view->getProperty())) {
    if (const auto* list = m_property->lineTypeList()) {
        init(list, true, false, true);
    } else {
        // List mode retains an unknown source name as a temporary row even
        // when no drawing is attached. init() copies the row data.
        LC_LineTypeList fallback;
        init(&fallback, true, false, true);
    }
    setLineEdit(nullptr);
    updateByProperty();
    connect(m_property, &LC_Property::afterPropertyChange, this,
            [this](LC_PropertyChangeReason reason) {
                if (!(reason & PropertyChangeReasonValue)) updateByProperty();
            });
    connect(this, &QG_LineTypeBox::lineTypeChanged, this, [this](RS2::LineType) {
        if (m_updating) return;
        RS_Pen pen;
        applyLineTypeTo(pen);
        m_property->setValue(LC_LineTypeIdentity::fromPen(pen), m_view->changeReasonDueToEdit());
    });
}

void LC_PropertyNamedLineTypeCombobox::hidePopup() {
    QG_LineTypeBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}

void LC_PropertyNamedLineTypeCombobox::updateByProperty() {
    ++m_updating;
    setEnabled(m_view->getStateProperty()->isEditableByUser());
    RS_Pen pen;
    m_property->value().applyTo(pen);
    setLineTypeOf(pen);
    --m_updating;
}
