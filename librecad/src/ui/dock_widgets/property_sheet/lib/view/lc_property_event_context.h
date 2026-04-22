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

#ifndef LC_PROPERTYEVENTCONTEXT_H
#define LC_PROPERTYEVENTCONTEXT_H

#include <QEvent>
#include <QPoint>

class LC_PropertiesSheet;
struct LC_PropertyViewPart;

struct LC_PropertyEventContext {
    QEvent* event{nullptr};
    LC_PropertiesSheet* sheet{nullptr};

    int eventType() const {
        return event->type();
    }

    template <class EventT>
    EventT* typedEvent() {
        return static_cast<EventT*>(event);
    }

    void updateWidget() const;

private:
    bool grabMouse(LC_PropertyViewPart* part, QPoint mousePos) const;
    bool releaseMouse(const LC_PropertyViewPart* part, QPoint mousePos) const;
    friend struct LC_PropertyViewPart;
};

#endif
