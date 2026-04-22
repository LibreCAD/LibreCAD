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

#include "lc_property_lineedit_with_button.h"

#include <QHBoxLayout>
#include <QToolButton>

LC_PropertyLineEditWithButton::LC_PropertyLineEditWithButton(QWidget* parent, const QString& bttnText, QLineEdit* lineEdit)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (lineEdit == nullptr) {
        m_lineEdit = new QLineEdit(this);
    }
    else {
        lineEdit->setParent(this);
        m_lineEdit = lineEdit;
    }

    layout->addWidget(m_lineEdit);

    m_toolButton = new QToolButton(this);
    m_toolButton->setText(bttnText);
    m_toolButton->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(m_toolButton);

    setFocusProxy(m_lineEdit);
    setAutoFillBackground(true);
}

void LC_PropertyLineEditWithButton::setTextForProperty(const LC_Property* property, const QString& text) const {
    if (property->isMultiValue()) {
        m_lineEdit->clear();
        m_lineEdit->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
    }
    else {
        m_lineEdit->setText(text);
        m_lineEdit->setPlaceholderText(QString());
    }
}
