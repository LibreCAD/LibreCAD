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

#include "lc_property_view_part.h"

#include <QMouseEvent>
#include <QToolTip>

#include "lc_guardedconnectionslist.h"
#include "lc_properties_sheet.h"
#include "lc_property_event_context.h"

LC_PropertyViewPartEvent::LC_PropertyViewPartEvent(Type type, const QPoint mousePos)
    : QEvent(static_cast<QEvent::Type>(type)), m_mousePos(mousePos) {
}

LC_PropertyViewPart::LC_PropertyViewPart(const QRect& rect)
    : rect(rect), m_trackState(false), m_activeCount(0), m_state(PartStateNone) {
}

void LC_PropertyViewPart::paint(LC_PropertyPaintContext& ctx) const {
    if (funPaint != nullptr) {
        funPaint(ctx, *this);
    }
}

bool LC_PropertyViewPart::handleEvent(LC_PropertyEventContext& ctx, LC_PropertyEditContext& propertyEditContext) const {
    return funHandleEvent(ctx, *this, &propertyEditContext);
}

QString LC_PropertyViewPart::getTooltip(LC_PropertyEventContext& ctx) const {
    return funGetTooltip(ctx, *this);
}

void LC_PropertyViewPart::setTextAsTooltip(const QString& text) {
    funGetTooltip = [text](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
        return text;
    };
}

void LC_PropertyViewPart::setPropertyNameAsTooltip(const LC_Property& property) {
    funGetTooltip = [&property](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
        return property.getDisplayName();
    };
}

void LC_PropertyViewPart::setPropertyDescriptionAsTooltip(const LC_Property& property) {
    funGetTooltip = [&property](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
        auto descr = property.getDescription();
        if (descr.isEmpty()) {
            return property.getDisplayName();
        }
        return descr;
    };
}

bool LC_PropertyViewPart::activate(LC_PropertiesSheet* sheet, const QPoint mousePos) {
    if (!m_trackState) {
        return false;
    }

    Q_ASSERT(m_activeCount <= 1);
    if (m_activeCount > 1) {
        return false;
    }

    ++m_activeCount;

    if (m_state != PartUnderCursor) {
        m_state = PartUnderCursor;
        sheet->viewport()->update();
        selfEvent(LC_PropertyViewPartEvent::Activated, sheet, mousePos);
    }

    return true;
}

bool LC_PropertyViewPart::deactivate(LC_PropertiesSheet* sheet, const QPoint mousePos) {
    if (!m_trackState) {
        return false;
    }

    Q_ASSERT(m_activeCount > 0);
    if (m_activeCount == 0) {
        return false;
    }

    --m_activeCount;

    if ((m_activeCount == 0) && isPushedOrUnderCursor()) {
        m_state = PartStateNone;
        sheet->viewport()->update();
        selfEvent(LC_PropertyViewPartEvent::Deactivated, sheet, mousePos);
    }

    return true;
}

bool LC_PropertyViewPart::grabMouse(LC_PropertiesSheet* sheet, const QPoint mousePos) {
    Q_ASSERT(m_activeCount > 0);
    Q_ASSERT(m_state == PartUnderCursor);

    m_state = PartStatePushed;
    sheet->viewport()->update();
    selfEvent(LC_PropertyViewPartEvent::PressMouse, sheet, mousePos);
    return true;
}

bool LC_PropertyViewPart::releaseMouse(LC_PropertiesSheet* sheet, const QPoint mousePos) {
    Q_ASSERT(m_activeCount > 0);
    Q_ASSERT(m_state == PartStatePushed);

    m_state = PartUnderCursor;
    sheet->viewport()->update();
    selfEvent(LC_PropertyViewPartEvent::ReleaseMouse, sheet, mousePos);
    return true;
}

void LC_PropertyViewPart::startEditing(const LC_PropertyEventContext& ctx, const LC_PropertyEditContext& propertyEditContext) {
    const auto editProperty = propertyEditContext.getProperty();
    const bool editable = editProperty->isEditableByUser();
    std::shared_ptr<LC_GuardedConnectionsList> connections;
    if (editable) {
        connections = std::make_shared<LC_GuardedConnectionsList>();
        std::weak_ptr weakConnections = connections;
        connections->push_back(QObject::connect(editProperty, &QObject::destroyed, [weakConnections]()-> void {
            if (!weakConnections.expired()) {
                weakConnections.lock()->clear();
            }
        }));
        ctx.sheet->connectPropertyToEdit(editProperty, *connections.get());
    }
    const auto editor = propertyEditContext.createEditor();
    if (editor != nullptr) {
        if (editable) {
            QObject::connect(editor, &QObject::destroyed, [connections]()-> void {
                connections->disconnect();
            });
        }
        startInplaceEdit(ctx, editor);
    }
}

bool LC_PropertyViewPart::event(LC_PropertyEventContext& ctx) {
    if (m_trackState) {
        switch (ctx.eventType()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick: {
                const auto event = ctx.typedEvent<QMouseEvent>();
                if (event->button() == Qt::LeftButton) {
                    ctx.grabMouse(this, event->pos());
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                const auto event = ctx.typedEvent<QMouseEvent>();
                if ((event->button() == Qt::LeftButton) && (m_state == PartStatePushed)) {
                    ctx.releaseMouse(this, event->pos());
                }
                break;
            }
            default:
                break;
        }
    }

    if (ctx.eventType() == QEvent::ToolTip) {
        if (!funGetTooltip) {
            return false;
        }
        const QString tooltipText = getTooltip(ctx);
        if (!tooltipText.isEmpty()) {
            const auto event = ctx.typedEvent<QHelpEvent>();
            QToolTip::showText(event->globalPos(), tooltipText, ctx.sheet, rect);
            return true;
        }
    }

    if (funHandleEvent != nullptr) {
        LC_PropertyEditContext propertyEditContext;
        const bool result = handleEvent(ctx, propertyEditContext);
        if (propertyEditContext.isValid()) {
            startEditing(ctx, propertyEditContext);
        }
        return result;
    }
    return false;
}

void LC_PropertyViewPart::startInplaceEdit(const LC_PropertyEventContext& ctx, QWidget* editor) {
    ctx.sheet->startInplaceEdit(editor);
}

bool LC_PropertyViewPart::selfEvent(const LC_PropertyViewPartEvent::Type type, LC_PropertiesSheet* sheet, const QPoint mousePos) {
    LC_PropertyViewPartEvent event_(type, mousePos);
    LC_PropertyEventContext context{&event_, sheet};
    return event(context);
}
