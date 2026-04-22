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

#include "lc_property_view_editable.h"

#include <QKeyEvent>
#include <QLineEdit>

#include "lc_properties_sheet.h"
#include "lc_property_edit_context.h"
#include "lc_property_editor_handler_base.h"
#include "lc_property_event_context.h"
#include "lc_property_view_part.h"
#include "lc_property_view_utils.h"

LC_PropertyViewEditable::LC_PropertyViewEditable(LC_Property& property)
    : LC_PropertyViewWithValue(property) {
}

LC_PropertyViewEditable::~LC_PropertyViewEditable() {
    if (m_editorHandler != nullptr) {
        m_editorHandler->cleanup();
    }
}

void LC_PropertyViewEditable::stopInplaceEditing(const bool deleteLater, const bool restoreParentFocus) const {
    if (m_inplaceEditStopper != nullptr) {
        m_inplaceEditStopper->stopInplaceEdit(deleteLater, restoreParentFocus);
    }
}

bool LC_PropertyViewEditable::propertyValueToStr(QString& strValue) const {
    return doPropertyValueToStr(strValue);
}

bool LC_PropertyViewEditable::doPropertyValueToStrForView(QString& strValue) {
    return doPropertyValueToStr(strValue);
}

bool LC_PropertyViewEditable::doPropertyValueToStrForEdit(QString& strValue) const {
    return doPropertyValueToStr(strValue);
}

bool LC_PropertyViewEditable::doPropertyValueToStr([[maybe_unused]] QString& strValue) const {
    return false;
}

bool LC_PropertyViewEditable::doGetToolTip(QString& strValue) {
    return doPropertyValueToStrForView(strValue);
}

bool LC_PropertyViewEditable::doBuildPartValue(LC_PropertyPaintContext&, LC_PropertyViewPart& partValue) {
    partValue.funPaint = [this](LC_PropertyPaintContext& ctx, const LC_PropertyViewPart& item) {
        const auto oldBrush = ctx.painter->brush();
        const auto oldPen = ctx.painter->pen();
        const auto isUsualText = isEditableByUser() && !isMultiValue();
        const auto cg = isUsualText ? ctx.getCurrentColorGroup() : QPalette::Disabled;
        const auto color = ctx.getTextColor(isUsualText);
        const auto bgColor = ctx.isActive ? ctx.getPalette().color(cg, QPalette::Highlight) : ctx.getAlternatedButtonColor();

        ctx.painter->setBrush(bgColor);
        ctx.painter->setPen(color);
        doDrawValue(ctx, *ctx.painter, item.rect);
        ctx.painter->setBrush(oldBrush);
        ctx.painter->setPen(oldPen);
    };

    partValue.funHandleEvent = [this](LC_PropertyEventContext& eventCtx, const LC_PropertyViewPart& part,
                                      LC_PropertyEditContext* editCtx) -> bool {
        bool doEdit = false;
        switch (eventCtx.eventType()) {
            case QEvent::MouseButtonDblClick:
                doEdit = eventCtx.sheet->getPropertyViewStyle() & PropertiesSheetStyleDblClickActivation; // fixme - add method in widget?
                break;

            case QEvent::MouseButtonRelease:
                doEdit = !(eventCtx.sheet->getPropertyViewStyle() & PropertiesSheetStyleDblClickActivation);
                break;

            case QEvent::KeyPress:
                doEdit = doAcceptKeyPressedForInplaceEdit(eventCtx.typedEvent<QKeyEvent>());
                break;
            default:
                break;
        }

        if (doEdit) {
            auto ctx = &eventCtx;
            auto it = &part;
            editCtx->setup(getProperty(), [this, ctx, it]() -> QWidget* {
                const EditActivationContext activationContext(ctx->event);
                this->m_inplaceEditStopper = ctx->sheet;
                return doCreateValueEditor(ctx->sheet->viewport(), it->rect, &activationContext);
            });
            return true;
        }

        return false;
    };

    partValue.funGetTooltip = [this](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
        if (isMultiValue()) {
            return LC_Property::getMultiValuePlaceholder();
        }
        QString valueText;
        if (!doGetToolTip(valueText)) {
            return QString();
        }
        return valueText;
    };

    return true;
}

bool LC_PropertyViewEditable::isNormalPainterState(const QStylePainter& painter) const {
    const auto property = getStateProperty();
    if (property == nullptr) {
        return false;
    }

    if (!property->isEditableByUser()) {
        return false;
    }

    const auto palette = painter.style()->standardPalette();
    const bool result = palette.currentColorGroup() != QPalette::Disabled && painter.brush().color() != palette.color(QPalette::Highlight);
    return result;
}

bool LC_PropertyViewEditable::isPlaceholderColor() const {
    return false;
}

void LC_PropertyViewEditable::doDrawValue([[maybe_unused]] LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) {
    if (isMultiValue()) {
        LC_PropertyViewUtils::drawElidedText(painter, LC_Property::getMultiValuePlaceholder(), rect, painter.style());
        return;
    }

    QString strValue;
    if (doPropertyValueToStrForView(strValue)) {
        QPen oldPen;
        bool penChanged = false;
        if (isNormalPainterState(painter) && isPlaceholderColor()) {
            oldPen = painter.pen();
            penChanged = true;
            painter.setPen(activeTextColor(painter));
        }
        LC_PropertyViewUtils::drawElidedText(painter, strValue, rect, painter.style());
        if (penChanged) {
            painter.setPen(oldPen);
        }
    }
}

bool LC_PropertyViewEditable::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    const int key = keyEvent->key();
    return (key == Qt::Key_Space) || (key == Qt::Key_Return) || (key == Qt::Key_Enter);
}

QLineEdit* LC_PropertyViewEditable::createValueEditorLineEdit(QWidget* parent, const QRect& rect, const bool readOnly,
                                                              const EditActivationContext* ctx) const {
    auto* lineEdit = new QLineEdit(parent);
    lineEdit->setGeometry(rect);
    lineEdit->setReadOnly(readOnly);
    lineEdit->setPlaceholderText(LC_Property::getMultiValuePlaceholder());

    if (!isMultiValue()) {
        QString strValue;
        doPropertyValueToStrForEdit(strValue);
        lineEdit->setText(strValue);
    }

    if (ctx != nullptr) {
        lineEdit->selectAll();
    }

    return lineEdit;
}
