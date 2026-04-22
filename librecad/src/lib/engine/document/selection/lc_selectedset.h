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

#ifndef LC_SELECTEDSET_H
#define LC_SELECTEDSET_H

#include "lc_dimstyleslistmodel.h"
#include "lc_selectedsetlistener.h"

class RS_Entity;

class LC_SelectedSet{
public:
    explicit LC_SelectedSet();
    virtual ~LC_SelectedSet();
    void clear();
    void add(RS_Entity* entity);
    void remove(RS_Entity* entity);
    void replaceBy(QList<RS_Entity*>& entities);

    bool isEmpty() const {return m_entitiesList.isEmpty();}

    RS_Entity* first() const {
        return m_entitiesList.first();
    }

    RS_Entity *last() const {
        return m_entitiesList.last();
    }

    QList<RS_Entity *>::const_iterator begin() const {
        return m_entitiesList.begin();
    }

    QList<RS_Entity *>::const_iterator end() const {
        return m_entitiesList.end();
    }

    void disableListeners();
    bool enableListeners();
    void addListener(LC_SelectedSetListener* listener);
    void removeListener(LC_SelectedSetListener* listener);
    void fireSelectionChanged();
    bool isSilent() const {return m_silentMode != 0;}
    void cleanup();
    bool collectSelectedEntities(QList<RS_Entity*>& list);
    bool collectSelectedEntities(QList<RS_Entity*>& list, const QList<RS2::EntityType>& types);
    bool hasSelection();

private:
    QList<RS_Entity*> m_entitiesList;
    QList<LC_SelectedSetListener*> m_listeners;
    int m_silentMode{0};
    bool m_changedInSilent{false};
};

#endif
