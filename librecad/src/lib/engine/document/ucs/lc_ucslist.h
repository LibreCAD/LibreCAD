/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_UCSLIST_H
#define LC_UCSLIST_H

#include <QList>
#include "lc_ucs.h"

class LC_UCSListListener {
public:
    virtual void ucsListModified([[maybe_unused]]bool changed) {};
};


class LC_UCSList{
public:
    LC_UCSList();
    virtual ~LC_UCSList() = default;
    void clear();

/**
 * @return Number of ucss in the list.
 */
    unsigned int count() const {
        return ucsList.count();
    }

/**
 * @return ucs at given position or NULL if it is out of range.
 */
    LC_UCS *at(unsigned int i) {
        return ucsList.at(i);
    }

    QList<LC_UCS *>::iterator begin();
    QList<LC_UCS *>::iterator end();
    QList<LC_UCS *>::const_iterator begin() const;
    QList<LC_UCS *>::const_iterator end() const;
    void add(LC_UCS *ucs);
    void addNew(LC_UCS *ucs);
    void remove(LC_UCS *ucs);
    void remove(const QString &name);
    void edited(LC_UCS *ucs);
    void rename(LC_UCS* ucs, const QString& newName);
    LC_UCS *find(const QString &name);
    int getIndex(const QString &name);
    int getIndex(LC_UCS *ucs);
    /**
    * Sets the ucss lists modified status to 'm'.
    */
    void setModified(bool m);

    /**
     * @retval true The ucss list has been modified.
     * @retval false The ucss list has not been modified.
     */
    virtual bool isModified() const {
        return modified;
    }

    void addListener(LC_UCSListListener *listener) {
        if (listener != nullptr) {
            ucsListListeners.push_back(listener);
        }
    }

    void removeListener(LC_UCSListListener *listener) {
        if (listener != nullptr) {
            ucsListListeners.removeOne(listener);
        }
    }

    void fireModified(bool value) {
        for (auto l: ucsListListeners) {
            l->ucsListModified(value);
        }
    }

    LC_UCS *tryAddUCS(LC_UCS *candidate);
    LC_UCS *findExisting(LC_UCS *candidate);
    LC_UCS *getWCS();
    LC_UCS* getActive(){return activeUCS;}

    void tryToSetActive(LC_UCS *ucs);

protected:
    QList<LC_UCS *> ucsList;
    QList<LC_UCSListListener *> ucsListListeners;
    LC_UCS* activeUCS = nullptr;
    bool modified = false;
    LC_WCS* wcs = new LC_WCS();
};

#endif // LC_UCSLIST_H
