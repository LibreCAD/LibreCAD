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

#include "lc_property_label_with_button.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include "lc_property.h"

LC_PropertyLabelWithButton::LC_PropertyLabelWithButton(QWidget* parent, const QString& bttnText, QLabel* label) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (label == nullptr) {
        m_label = new QLabel(this);
    }
    else {
        label->setParent(this);
        m_label = label;
    }
    layout->addWidget(m_label);

    m_toolButton = new QToolButton(this);
    m_toolButton->setText(bttnText);
    m_toolButton->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(m_toolButton);

    setFocusProxy(m_toolButton);
    setAutoFillBackground(true);
}

void LC_PropertyLabelWithButton::setTextForProperty(const LC_Property* property, const QString& text) const {
    if (property->isMultiValue()) {
        m_label->setText(LC_Property::getMultiValuePlaceholder());
    }
    else {
        m_label->setText(text);
    }
}
