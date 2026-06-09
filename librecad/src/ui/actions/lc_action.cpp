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

#include "lc_action.h"

#include <QEvent>

bool LC_Action::event(QEvent* event) {
    const auto type = event->type();
    switch (type) {
        case QEvent::Type::Shortcut: {
            m_elapsedTimer.start();
            break;
        }
        case QEvent::Type::ActionChanged: {
            break;
        }
        default:
            m_elapsedTimer.invalidate();
            break;
    }
    return QAction::event(event);
}

bool LC_Action::isInvokedViaShortcut() {
    if (m_elapsedTimer.isValid()) {
        const bool result = !m_elapsedTimer.hasExpired(10);
        m_elapsedTimer.invalidate();
        return result;
    }
    return false;

}
