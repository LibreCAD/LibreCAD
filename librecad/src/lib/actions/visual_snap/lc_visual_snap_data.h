
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

#ifndef LC_VISUALSNAPDATA_H
#define LC_VISUALSNAPDATA_H

#include <QMutex>
#include <list>
#include <memory>

#include "lc_relative_point_data.h"
#include "lc_visual_snap_solution.h"

class LC_VisualSnapData {
public:
    LC_VisualSnapData() = default;
    ~LC_VisualSnapData();
    void clear();
    bool isContentLocked() const;
    void lockContent(bool performLock);
    bool isEmpty() const;
    void lock() {
        m_mutex.lock();
    }
    void unlock() {m_mutex.unlock();}
    void addRelativePositionInfo(const LC_RelativePositionData* relativePositionData);
    void storeEntityRef(RS_Entity* snapEntity, RS_Entity* documentViewSnapEntity, unsigned long long entityId);
    void storeVertexRef(LC_VisualSnapVertex* vertex);
    void removeVertexWithPoint(const RS_Vector& pos);
    void removeDocumentEntityWithId(unsigned long long entityId);
    void removeVertex(LC_VisualSnapVertex* vertex);
    bool doesNotContainsDocEntityWithId(unsigned long long entityId) const;
    const RS_Vector& getLastSnappedPoint() const {return m_lastSnappedBasePoint;}
    bool isLastSnappedPointValid() const {return m_lastSnappedBasePoint.valid;}
    void removeVertexWithLastSnappedData();
    void saveLastSnappedPoint(const RS_Vector& pos);
    bool removeLastAddition();
    bool forEachVertexTillFound(const std::function<bool(LC_VisualSnapVertex*)>& fun) const;
    void forEachVertex(const std::function<void(LC_VisualSnapVertex*)>& fun) const;
    void forEachDocRef(const std::function<void(LC_VisualSnapDocumentEntityRef*)>& fun) const;
    bool forEachDocRefTillFound(const std::function<bool(LC_VisualSnapDocumentEntityRef*)>& fun) const;
    void forEachItem(const std::function<void(LC_VisualSnapItem*)>& fun) const;
    void forEachRelativePosition(const std::function<void(const LC_RelativePositionData&)>& fun) const;
private:
     QMutex m_mutex;
     bool m_contentLocked = false;
     std::list<LC_RelativePositionData> m_relativePositionData;
     std::list<std::unique_ptr<LC_VisualSnapItem>> m_itemsList;
     RS_Vector m_lastSnappedBasePoint{false};
     RS_Vector m_previousSnappedBasePoint{false};
};

#endif
