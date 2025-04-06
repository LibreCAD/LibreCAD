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
#include "lc_view.h"

class LC_ViewListListener {
public:
    virtual void viewsListModified([[maybe_unused]]bool changed) {};
};


class LC_ViewList {
public:
    LC_ViewList();
    virtual ~LC_ViewList() = default;
    void clear();

/**
 * @return Number of views in the list.
 */
    unsigned int count() const {
        return m_namedViews.count();
    }

/**
 * @return View at given position or NULL if i is out of range.
 */
    LC_View *at(unsigned int i) {
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

    void addListener(LC_ViewListListener *listener) {
        if (listener != nullptr) {
            m_viewListListeners.push_back(listener);
        }
    }

    void removeListener(LC_ViewListListener *listener) {
        if (listener != nullptr) {
            m_viewListListeners.removeOne(listener);
        }
    }

    void fireModified(bool value) {
        for (auto l: m_viewListListeners) {
            l->viewsListModified(value);
        }
    }

protected:
    QList<LC_View *> m_namedViews;
    QList<LC_ViewListListener *> m_viewListListeners;
    bool m_modified = false;
};

#endif // LC_VIEWSLIST_H
