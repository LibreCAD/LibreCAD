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

#include "lc_trackedlistener.h"
#include "lc_ucs.h"

class LC_UCSListListener : public LC_TrackedListener<LC_UCSListListener> {
public:
    virtual void ucsListModified([[maybe_unused]]bool changed) {}
    /**
     * Called once, from the destructor of the list (its drawing is being destroyed), after the list
     * was emptied and this listener was unregistered from it (the listeners not told yet are still
     * listed, and unregister themselves if a callback deletes them), and before the UCSs are deleted. The list and its drawing are partly destroyed: forget them (drop the pointer
     * to the list, to the drawing, and any row or pointer for a UCS) and do not read them, or list
     * yourself with the list again. Nothing is forwarded to ucsListModified(): a listener that keeps
     * nothing needs no override. (A listener that is destroyed while listed needs nothing: see
     * LC_TrackedListener.)
     */
    virtual void ucsListDestroyed() {}
    virtual ~LC_UCSListListener() = default;
};


/**
 * The list owns every UCS it holds except the WCS, which m_wcs owns.
 * add(), addNew() and tryAddUCS() take ownership of their argument: one the
 * list does not keep (its name or geometry is already present) is deleted.
 */
class LC_UCSList{
public:
    LC_UCSList();
    virtual ~LC_UCSList();
    void clear();

/**
 * @return Number of ucss in the list.
 */
    unsigned int count() const {
        return m_ucsList.count();
    }

    bool isEmpty() const {
        return m_ucsList.isEmpty();
    }

/**
 * @return ucs at given position or NULL if it is out of range.
 */
    LC_UCS *at(const unsigned int i) const {
        return m_ucsList.at(i);
    }

    const QList<LC_UCS*>* getItems() const {return &m_ucsList;}
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

    /// listed once; the listener removes itself when it is destroyed
    void addListener(LC_UCSListListener *listener) {
        m_ucsListListeners.add(listener);
    }

    void removeListener(LC_UCSListListener *listener) {
        m_ucsListListeners.remove(listener);
    }

    /// how many listeners are registered (a widget that is destroyed while attached must have unregistered)
    int listenerCount() const {
        return static_cast<int>(m_ucsListListeners.size());
    }

    void fireModified(const bool value) {
        for (const auto l: std::as_const(m_ucsListListeners)) {
            l->ucsListModified(value);
        }
    }

    LC_UCS *tryAddUCS(LC_UCS *candidate);
    LC_UCS *findExisting(const LC_UCS *candidate);
    LC_UCS *getWCS() const;
    LC_UCS* getActive() const {return m_activeUCS;}
    void tryToSetActive(const LC_UCS *ucs);
protected:
    void deleteOwnedEntries();

    QList<LC_UCS *> m_ucsList;
    LC_ListenerList<LC_UCSListListener, LC_UCSList> m_ucsListListeners{this};
    LC_UCS* m_activeUCS = nullptr;
    bool m_modified = false;
    std::unique_ptr<LC_WCS> m_wcs = std::make_unique<LC_WCS>();
};

#endif
