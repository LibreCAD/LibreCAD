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

#ifndef LC_SELECTEDSETLISTENER_H
#define LC_SELECTEDSETLISTENER_H

#include "lc_trackedlistener.h"

class LC_SelectedSetListener : public LC_TrackedListener<LC_SelectedSetListener> {
public:
    virtual ~LC_SelectedSetListener() = default;
    LC_SelectedSetListener() = default;
    virtual void selectionChanged() = 0;
    /**
     * Called once, from the destructor of the selection (its drawing is being destroyed: the selection
     * is the last part of it to go), after the selection was emptied and this listener was unregistered
     * from it (the listeners not told yet are still listed, and unregister themselves if a callback
     * deletes them). The drawing is partly destroyed, and the entities that were
     * selected are still there but their layers and blocks are not: forget the drawing (drop any
     * pointer to it, and to the selected entities) and do not read it, call selectionChanged() logic
     * that does, or list yourself with the selection again. (A listener that is destroyed while listed
     * needs nothing: see LC_TrackedListener.)
     */
    virtual void selectedSetDestroyed() {}
};
#endif
