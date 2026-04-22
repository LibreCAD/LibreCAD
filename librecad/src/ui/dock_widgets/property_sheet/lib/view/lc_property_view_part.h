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

#ifndef LC_PROPERTYVIEWPART_H
#define LC_PROPERTYVIEWPART_H

#include "lc_property_edit_context.h"
#include "lc_property_event_context.h"
#include "lc_property_paint_context.h"

class LC_PropertyViewPartEvent : public QEvent {
public:
    enum Type {
        Activated    = 3 * User + 15,
        Deactivated  = 3 * User + 16,
        PressMouse   = 3 * User + 17,
        ReleaseMouse = 3 * User + 18
    };

    LC_PropertyViewPartEvent(Type type, QPoint mousePos);

    QPoint pos() const {
        return m_mousePos;
    }

    int x() const {
        return m_mousePos.x();
    }

    int y() const {
        return m_mousePos.y();
    }

private:
    QPoint m_mousePos;
};

struct LC_PropertyViewPart {
    using FunDraw = std::function<void(LC_PropertyPaintContext &, const LC_PropertyViewPart &)>;
    using FunProcessEvent = std::function<bool(LC_PropertyEventContext &, const LC_PropertyViewPart &, LC_PropertyEditContext *)>;
    using FunGetToolTip = std::function<QString(LC_PropertyEventContext &, const LC_PropertyViewPart &)>;

    explicit LC_PropertyViewPart(const QRect& rect);

    QRect rect;

    FunDraw funPaint{nullptr};
    FunProcessEvent funHandleEvent{nullptr};
    FunGetToolTip funGetTooltip{nullptr};

    bool isPushedOrUnderCursor() const {
        return m_state != PartStateNone;
    }

    bool isPushed() const {
        return m_state == PartStatePushed;
    }

    bool isUnderCursor() const {
        return m_state == PartUnderCursor;
    }

    inline void trackState();

    void setTextAsTooltip(const QString& text);
    void setPropertyNameAsTooltip(const LC_Property& property);
    void setPropertyDescriptionAsTooltip(const LC_Property& property);

    bool isRectValid() const {
        return rect.isValid();
    }

    bool isRectInValid() const {
        return !rect.isValid();
    }

private:
    enum State {
        PartStateNone,
        PartUnderCursor,
        PartStatePushed
    };

    bool activate(LC_PropertiesSheet* sheet, QPoint mousePos);
    bool deactivate(LC_PropertiesSheet* sheet, QPoint mousePos);

    bool grabMouse(LC_PropertiesSheet* sheet, QPoint mousePos);
    bool releaseMouse(LC_PropertiesSheet* sheet, QPoint mousePos);

    void paint(LC_PropertyPaintContext& ctx) const;
    bool handleEvent(LC_PropertyEventContext& ctx, LC_PropertyEditContext& propertyEditContext) const;
    QString getTooltip(LC_PropertyEventContext& ctx) const;
    void startEditing(const LC_PropertyEventContext& ctx, const LC_PropertyEditContext& propertyEditContext);
    void startInplaceEdit(const LC_PropertyEventContext& ctx, QWidget* editor);
    bool event(LC_PropertyEventContext& ctx);
    bool selfEvent(LC_PropertyViewPartEvent::Type type, LC_PropertiesSheet* sheet, QPoint mousePos);

    bool m_trackState;
    quint8 m_activeCount;
    State m_state;

    friend class LC_PropertiesSheet;
};

void LC_PropertyViewPart::trackState() {
    m_trackState = true;
}

#endif
