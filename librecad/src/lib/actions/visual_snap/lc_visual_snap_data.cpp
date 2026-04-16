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

#include "lc_visual_snap_data.h"

LC_VisualSnapData::~LC_VisualSnapData() {
    // LC_ERR << "LC_VisualSnapData destructor";
}

void LC_VisualSnapData::clear() {
    if (!m_contentLocked) {
        for (const auto& p : m_itemsList) {
            if (p->isVertexItem) {
                // nothing to clear
            }
            else {
                p->docEntityRef->documentViewEntity->delFlag(RS2::FlagInVisualSnap);
            }
        }
        m_relativePositionData.clear();
        m_itemsList.clear();
    }
}

bool LC_VisualSnapData::isContentLocked() const {
    return m_contentLocked;
}

void LC_VisualSnapData::lockContent(bool performLock) {
    m_contentLocked = performLock;
}

bool LC_VisualSnapData::isEmpty() const {
    bool result = m_itemsList.empty() && m_relativePositionData.empty();
    return result;
}

void LC_VisualSnapData::addRelativePositionInfo(const LC_RelativePositionData* relativePositionData) {
    LC_RelativePositionData data;
    data.updateBy(relativePositionData);
    m_relativePositionData.push_back(data);
}

void LC_VisualSnapData::storeEntityRef(RS_Entity* const snapEntity, RS_Entity* documentViewSnapEntity,  unsigned long long entityId) {
    if (!m_contentLocked) {
        m_itemsList.push_back(std::make_unique<LC_VisualSnapItem>(snapEntity, documentViewSnapEntity, entityId));
    }
}

void LC_VisualSnapData::storeVertexRef(LC_VisualSnapVertex* const vertex) {
    if (!m_contentLocked) {
        m_itemsList.push_back(std::make_unique<LC_VisualSnapItem>(vertex));
    }
}

void LC_VisualSnapData::removeVertexWithPoint(const RS_Vector& pos) {
    // todo - remove by erase_if in C++ 20
    auto itr = find_if(m_itemsList.begin(), m_itemsList.end(), [pos](std::unique_ptr<LC_VisualSnapItem>& it) -> bool {
        if (it->getVertex() != nullptr) {
            return it->getVertex()->wcsSnapCoordinate == pos;
        }
        return false;
    });
    if (itr != m_itemsList.end()) {
        itr = m_itemsList.erase(itr);
    }
}

void LC_VisualSnapData::removeDocumentEntityWithId(unsigned long long entityId) {
    // todo - remove by erase_if in C++ 20
    auto itr = find_if(m_itemsList.begin(), m_itemsList.end(), [entityId](std::unique_ptr<LC_VisualSnapItem>& it) {
        if (it->isVertexItem) {
            return false;
        }
        auto result = it->docEntityRef->originalEntityId == entityId;
        return result;
    });
    if (itr != m_itemsList.end()) {
        itr = m_itemsList.erase(itr);
    }
}

void LC_VisualSnapData::removeVertex(LC_VisualSnapVertex* v) {
    // todo - remove by erase_if in C++ 20
    auto itr = find_if(m_itemsList.begin(), m_itemsList.end(), [v](std::unique_ptr<LC_VisualSnapItem>& it) {
        return it->getVertex() == v;
    });
    if (itr != m_itemsList.end()) {
        itr = m_itemsList.erase(itr);
    }
}

bool LC_VisualSnapData::doesNotContainsDocEntityWithId(unsigned long long entityId) const {
    for (const auto& i : m_itemsList) {
        if (!i->isVertexItem) {
            if (entityId ==  i->docEntityRef->originalEntityId) {
                return false;
            }
        }
    }
    return true;
}

void LC_VisualSnapData::removeVertexWithLastSnappedData() {
    if (m_lastSnappedBasePoint.valid) {
        removeVertexWithPoint(m_lastSnappedBasePoint);
    }
}

void LC_VisualSnapData::saveLastSnappedPoint(const RS_Vector& v) {
    m_previousSnappedBasePoint = m_lastSnappedBasePoint;
    m_lastSnappedBasePoint = v;
}

bool LC_VisualSnapData::removeLastAddition() {
    if (!m_contentLocked) {
        if (!m_itemsList.empty()) {
            const auto& item = m_itemsList.back();
            if (item->isVertexItem) {
            }
            else {
                item->docEntityRef->documentViewEntity->delFlag(RS2::FlagInVisualSnap);
            }
            m_itemsList.remove(item);
            return true;
        }
    }
    return false;
}

bool LC_VisualSnapData::forEachVertexTillFound(const std::function<bool(LC_VisualSnapVertex*)>& fun) const {
    for (const auto& item : m_itemsList) {
        const auto v = item->getVertex();
        if (v != nullptr) {
            const bool found = fun(v);
            if (found) {
                return true;
            }
        }
    }
    return false;
}

void LC_VisualSnapData::forEachVertex(const std::function<void(LC_VisualSnapVertex*)>& fun) const {
    for (const auto& item : m_itemsList) {
        const auto v = item->getVertex();
        if (v != nullptr) {
            fun(v);
        }
    }
}

bool LC_VisualSnapData::forEachDocRefTillFound(const std::function<bool(LC_VisualSnapDocumentEntityRef*)>& fun) const {
    for (const auto& item : m_itemsList) {
        const auto v = item->getVertex();
        if (v == nullptr) {
            const bool found = fun(item->docEntityRef.get());
            if (found) {
                return true;
            }
        }
    }
    return false;
}

void LC_VisualSnapData::forEachDocRef(const std::function<void(LC_VisualSnapDocumentEntityRef*)>& fun) const {
    for (const auto& item : m_itemsList) {
        const auto v = item->getVertex();
        if (v == nullptr) {
            fun(item->docEntityRef.get());
        }
    }
}


void LC_VisualSnapData::forEachItem(const std::function<void(LC_VisualSnapItem*)>& fun) const {
    for (const auto& item : m_itemsList) {
        fun(item.get());
    }
}

void LC_VisualSnapData::forEachRelativePosition(const std::function<void(const LC_RelativePositionData&)>& fun) const {
    for (const auto& item : m_relativePositionData) {
        fun(item);
    }
}
