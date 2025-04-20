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
#include <memory>
#include "lc_ucs.h"

class LC_UCSListListener {
public:
    virtual void ucsListModified([[maybe_unused]]bool changed) {}
    virtual ~LC_UCSListListener() = default;
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
        return m_ucsList.count();
    }

/**
 * @return ucs at given position or NULL if it is out of range.
 */
    LC_UCS *at(unsigned int i) {
        return m_ucsList.at(i);
    }

    void add(LC_UCS *ucs);
    void addNew(LC_UCS *ucs);
    void remove(LC_UCS *ucs);
    void remove(const QString &name);
    void edited(LC_UCS *ucs);
    void rename(LC_UCS* ucs, const QString& newName);
    LC_UCS *find(const QString &name) const;
    int getIndex(const QString &name) const;
    int getIndex(LC_UCS *ucs) const;
    /**
    * Sets the ucss lists modified status to 'm'.
    */
    void setModified(bool m);

    /**
     * @retval true The ucss list has been modified.
     * @retval false The ucss list has not been modified.
     */
    virtual bool isModified() const {
        return m_modified;
    }

    void addListener(LC_UCSListListener *listener) {
        if (listener != nullptr) {
            m_ucsListListeners.push_back(listener);
        }
    }

    void removeListener(LC_UCSListListener *listener) {
        if (listener != nullptr) {
            m_ucsListListeners.removeOne(listener);
        }
    }

    void fireModified(bool value) {
        for (auto l: m_ucsListListeners) {
            l->ucsListModified(value);
        }
    }

    LC_UCS *tryAddUCS(LC_UCS *candidate);
    LC_UCS *findExisting(LC_UCS *candidate);
    LC_UCS *getWCS() const;
    LC_UCS* getActive() const {return m_activeUCS;}
    void tryToSetActive(LC_UCS *ucs);
protected:
    QList<LC_UCS *> m_ucsList;
    QList<LC_UCSListListener *> m_ucsListListeners;
    LC_UCS* m_activeUCS = nullptr;
    bool m_modified = false;
    std::unique_ptr<LC_WCS> m_wcs = std::make_unique<LC_WCS>();
};

#endif // LC_UCSLIST_H
