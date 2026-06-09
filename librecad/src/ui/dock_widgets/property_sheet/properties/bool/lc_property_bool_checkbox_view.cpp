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

#include "lc_property_bool_checkbox_view.h"

#include <QKeyEvent>
#include <QStyleOptionButton>

#include "lc_properties_sheet.h"
#include "lc_property_paint_context.h"
#include "lc_property_view_part.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyBoolCheckBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameCheckBox();

LC_PropertyBoolCheckBoxView::LC_PropertyBoolCheckBoxView(LC_PropertyBool* property)
    : LC_PropertyViewTyped(property) {
}

bool LC_PropertyBoolCheckBoxView::doBuildPartValue(LC_PropertyPaintContext& ctx, LC_PropertyViewPart& part) {
    part.trackState();
    part.rect.adjust(ctx.sheet->getValueLeftMargin(), 0, 0, 0);
    part.rect.setWidth(ctx.style()->pixelMetric(QStyle::PM_IndicatorWidth));

    part.funPaint = [this](const LC_PropertyPaintContext& rctx, const LC_PropertyViewPart& item) {
        QStyleOptionButton opt;
        opt.rect = item.rect;
        opt.state = getState(rctx.isActive, item);

        if (isMultiValue()) {
            opt.state |= QStyle::State_NoChange;
        }
        else {
            const bool value = propertyValue();
            opt.state |= (value ? QStyle::State_On : QStyle::State_Off);
        }

        rctx.painter->drawControl(QStyle::CE_CheckBox, opt);
    };

    part.funHandleEvent = [this](LC_PropertyEventContext& eventCtx, const LC_PropertyViewPart&, LC_PropertyEditContext* editCtx) {
        bool toggleValue = false;
        switch (eventCtx.eventType()) {
            case QEvent::MouseButtonRelease: {
                toggleValue = true;
                break;
            }
            case QEvent::KeyPress: {
                const int key = eventCtx.typedEvent<QKeyEvent>()->key();
                toggleValue = (key == Qt::Key_Space) || (key == Qt::Key_Return);
                break;
            }
            default:
                return false;
        }

        if (toggleValue) {
            editCtx->setup(getProperty(), [this]() -> QWidget* {
                const auto thiz = this;
                auto& p = thiz->typedProperty();
                p.setValue(!p.value(), changeReasonDueToEdit());
                return nullptr;
            });
        }

        return true;
    };
    return true;
}
