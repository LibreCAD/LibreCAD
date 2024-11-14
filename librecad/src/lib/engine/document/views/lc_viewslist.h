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
        return namedViews.count();
    }

/**
 * @return View at given position or NULL if i is out of range.
 */
    LC_View *at(unsigned int i) {
        return namedViews.at(i);
    }

    QList<LC_View *>::iterator begin();

    QList<LC_View *>::iterator end();

    QList<LC_View *>::const_iterator begin() const;

    QList<LC_View *>::const_iterator end() const;

    void add(LC_View *layer);
    void addNew(LC_View *layer);

    void remove(LC_View *layer);

    void remove(const QString &name);

    void edited(LC_View *view);

    void rename(LC_View* view, const QString& newName);

    LC_View *find(const QString &name);

    int getIndex(const QString &name);

    int getIndex(LC_View *layer);

    /**
    * Sets the views lists modified status to 'm'.
    */
    void setModified(bool m);

    /**
     * @retval true The views list has been modified.
     * @retval false The views list has not been modified.
     */
    virtual bool isModified() const {
        return modified;
    }

    void addListener(LC_ViewListListener *listener) {
        if (listener != nullptr) {
            layerListListeners.push_back(listener);
        }
    }

    void removeListener(LC_ViewListListener *listener) {
        if (listener != nullptr) {
            layerListListeners.removeOne(listener);
        }
    }

    void fireModified(bool value) {
        for (auto l: layerListListeners) {
            l->viewsListModified(value);
        }
    }

protected:
    QList<LC_View *> namedViews;
    QList<LC_ViewListListener *> layerListListeners;
    bool modified = false;
};

#endif // LC_VIEWSLIST_H
