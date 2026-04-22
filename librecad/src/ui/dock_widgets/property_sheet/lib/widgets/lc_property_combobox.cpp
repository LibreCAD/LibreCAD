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

#include "lc_property_combobox.h"

#include <QPaintEvent>

#include "lc_property_view_utils.h"

LC_PropertyComboBox::LC_PropertyComboBox(LC_PropertyView* view, QWidget* parent)
    : QComboBox(parent), m_view(view) {
}

void LC_PropertyComboBox::paintEvent(QPaintEvent* event) {
    if (m_view == nullptr || m_paintDisabled) {
        return;
    }
    const auto rect = event->rect();
    QComboBox::paintEvent(event);

    QPainter painter(this);
    const auto stateProperty = getStateProperty();
    if (stateProperty != nullptr && stateProperty->isMultiValue()) {
        if (isEnabled()) {
            const auto color = palette().color(QPalette::Active, QPalette::PlaceholderText);
            painter.setPen(color);
        }
        LC_PropertyViewUtils::drawElidedText(painter, LC_Property::getMultiValuePlaceholder(), rect, style());
    }
    else if (currentIndex() >= 0) {
        doCustomPaint(painter, rect);
    }
}

void LC_PropertyComboBox::doCustomPaint(QPainter&, const QRect&) {
    // do nothing
}

LC_PropertyView* LC_PropertyComboBox::getPropertyView() const {
    return m_view;
}

LC_Property* LC_PropertyComboBox::getProperty() const {
    return m_view->getProperty();
}

LC_Property* LC_PropertyComboBox::getStateProperty() const {
    return m_view->getStateProperty();
}
