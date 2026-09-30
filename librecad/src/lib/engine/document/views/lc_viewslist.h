/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_VIEWSLIST_H
#define LC_VIEWSLIST_H

#include <QList>

#include "lc_trackedlistener.h"
#include "lc_view.h"

class LC_ViewListListener : public LC_TrackedListener<LC_ViewListListener> {
public:
    virtual void viewsListModified([[maybe_unused]]bool changed) {}
    /**
     * Called once, from the destructor of the list (its drawing is being destroyed), after the list
     * was emptied and this listener was unregistered from it (the listeners not told yet are still
     * listed, and unregister themselves if a callback deletes them), and before the views are deleted. The list and its drawing are partly destroyed: forget them (drop the pointer
     * to the list, to the drawing, and any row or pointer for a view) and do not read them, or list
     * yourself with the list again. Nothing is forwarded to viewsListModified(): a listener that keeps
     * nothing needs no override. (A listener that is destroyed while listed needs nothing: see
     * LC_TrackedListener.)
     */
    virtual void viewsListDestroyed() {}
    virtual ~LC_ViewListListener() = default;
};


/**
 * List of named views. The list owns the views it holds: it frees them on
 * clear(), remove() and destruction, and add()/addNew() free a view that is
 * rejected because its name is already taken.
 */
class LC_ViewList {
public:
    LC_ViewList();
    virtual ~LC_ViewList();
    LC_ViewList(const LC_ViewList&) = delete;
    LC_ViewList& operator=(const LC_ViewList&) = delete;
    void clear();

/**
 * @return Number of views in the list.
 */
    unsigned int count() const {
        return m_namedViews.count();
    }

    bool isEmpty() const {
        return m_namedViews.isEmpty();
    }

/**
 * @return View at given position or NULL if i is out of range.
 */
    LC_View *at(const unsigned int i) const {
        return m_namedViews.at(i);
    }

    void add(LC_View *view);
    void addNew(LC_View *view);
    void remove(LC_View *view);
    void remove(const QString &name);
    void edited(LC_View *view);
    void rename(LC_View* view, const QString& newName);
    LC_View *find(const QString &name) const;
    int getIndex(const QString &name) const;
    int getIndex(LC_View *view) const;
    /**
    * Sets the views lists modified status to 'm'.
    */
    void setModified(bool m);

    /**
     * @retval true The views list has been modified.
     * @retval false The views list has not been modified.
     */
    virtual bool isModified() const {
        return m_modified;
    }

    /// listed once; the listener removes itself when it is destroyed
    void addListener(LC_ViewListListener *listener) {
        m_viewListListeners.add(listener);
    }

    void removeListener(LC_ViewListListener *listener) {
        m_viewListListeners.remove(listener);
    }

    /// how many listeners are registered (a widget that is destroyed while attached must have unregistered)
    int listenerCount() const {
        return static_cast<int>(m_viewListListeners.size());
    }

    void fireModified(const bool value) {
        for (const auto l: std::as_const(m_viewListListeners)) {
            l->viewsListModified(value);
        }
    }

protected:
    QList<LC_View *> m_namedViews;
    LC_ListenerList<LC_ViewListListener, LC_ViewList> m_viewListListeners{this};
    bool m_modified = false;
};

#endif
