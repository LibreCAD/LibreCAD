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

#include "lc_property_qstring_list_arrows_combobox_view.h"

#include "lc_property_combobox.h"
#include "lc_property_qstring_list_arrows_combobox_view_handler.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyQStringListArrowsComboboxView::VIEW_NAME = QByteArrayLiteral("ArrowsCombobox");
const QByteArray LC_PropertyQStringListArrowsComboboxView::ATTR_BLOCK_NAMES = QByteArrayLiteral("blockNames");
const QByteArray LC_PropertyQStringListArrowsComboboxView::ATTR_CURRENT_INDEX = QByteArrayLiteral("currentIdx");


QWidget* LC_PropertyQStringListArrowsComboboxView::doCreateValueEditor(QWidget* parent, const QRect& rect,
                                                                       const EditActivationContext* ctx) {
    const auto editor = new LC_PropertyComboBox(this, parent);
    editor->setGeometry(rect);
    const auto handler = new LC_PropertyQStringListArrowsComboboxViewHandler(this, *editor, m_viewDescriptor);
    handler->connectCombobox(*editor);
    handler->doUpdateEditor();
    if (isInplaceEditAllowed(ctx)) {
        editor->showPopup();
    }
    return editor;
}

void LC_PropertyQStringListArrowsComboboxView::doDrawValue([[maybe_unused]]LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) {
    if (this->isMultiValue()) {
        LC_PropertyViewUtils::drawElidedText(painter, LC_Property::getMultiValuePlaceholder(), rect, painter.style());
        return;
    }

    // auto value = this->typedProperty().value();

    QRect textRect = rect;

    QRect colorRect = rect;
    colorRect.setWidth(colorRect.height());
    colorRect.adjust(2, 2, -2, -2);

    m_icon.paint(&painter, colorRect);

    // painter.drawPixmap(0.0, 0.0, m_icon.pixmap(colorRect.width(), colorRect.height()));

    /*painter.fillRect(colorRect, Qt::black);
    colorRect.adjust(1, 1, -1, -1);
    painter.fillRect(colorRect, layer->getPen().getColor());
    */

    textRect.setLeft(colorRect.right() + 3);

    if (textRect.isValid()) {
        LC_PropertyViewUtils::drawElidedText(painter, m_currentName, textRect, painter.style());
    }
}

void LC_PropertyQStringListArrowsComboboxView::doApplyAttributes(const LC_PropertyViewDescriptor& attrs) {
    LC_PropertyQStringListComboBoxView::doApplyAttributes(attrs);

    int idx = -1;

    const auto idxAttr = m_viewDescriptor.attributes[ATTR_CURRENT_INDEX];
    if (idxAttr.isValid()) {
        bool ok = false;
        const int val = idxAttr.toInt(&ok);
        if (ok) {
            idx = val;
        }
    }

    m_currentName.clear();
    m_currentIndex = idx;

    if (idx != -1) {
        QStringList iconNames;
        m_viewDescriptor.load(ATTR_ITEMS_ICON_NAMES, iconNames);

        QStringList items;
        m_viewDescriptor.load(ATTR_ITEMS, items);

        m_currentName = items.at(idx);
        const QString iconName = iconNames.at(idx);

        m_icon = QIcon(iconName);
    }
    else {
        m_icon = QIcon();
    }
}
