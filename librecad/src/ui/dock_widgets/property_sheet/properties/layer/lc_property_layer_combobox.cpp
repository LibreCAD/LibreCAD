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

#include "lc_property_layer_combobox.h"

#include <QPaintEvent>

#include "lc_property_layer_combobox_view.h"

LC_PropertyLayerComboBox::LC_PropertyLayerComboBox(LC_PropertyLayerComboBoxView* view, QWidget* parent)
    : QG_LayerBox(parent), m_view{view}, m_property{&view->typedProperty()}, m_updating{0} {
    setLineEdit(nullptr);

    init(*m_property->layerList(), m_property->isAllowByBlockValues(), false);
    updateByProperty();

    connect(m_property, &LC_Property::afterPropertyChange, this, &LC_PropertyLayerComboBox::onPropertyDidChange);
    connect(this, &QG_LayerBox::layerChanged, this, &LC_PropertyLayerComboBox::onLayerChanged);
}

void LC_PropertyLayerComboBox::hidePopup() {
    QComboBox::hidePopup();
    m_view->stopInplaceEditing(true, true);
}

void LC_PropertyLayerComboBox::updateByProperty() {
    m_updating++;
    const auto stateProperty = m_view->getStateProperty();
    setEnabled(stateProperty->isEditableByUser());
    RS_Layer* layer = m_property->value();
    setLayer(*layer);
    m_updating--;
}

void LC_PropertyLayerComboBox::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonValue)) {
        updateByProperty();
    }
}

void LC_PropertyLayerComboBox::onLayerChanged(RS_Layer* layer) {
    if (m_updating) {
        return;
    }
    const auto newValue = layer;
    m_property->setValue(newValue, m_view->changeReasonDueToEdit());
}
