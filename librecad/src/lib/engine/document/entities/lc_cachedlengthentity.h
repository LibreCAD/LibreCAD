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

#ifndef LC_CACHEDLENGTHENTITY_H
#define LC_CACHEDLENGTHENTITY_H

#include "rs_atomicentity.h"

class LC_CachedLengthEntity:public RS_AtomicEntity{
public:
    explicit LC_CachedLengthEntity(RS_EntityContainer* parent = nullptr);

  LC_CachedLengthEntity(const LC_CachedLengthEntity& entity)
      : RS_AtomicEntity{entity}
        , m_cachedLength{entity.m_cachedLength}{
  }

  LC_CachedLengthEntity& operator = (const LC_CachedLengthEntity& other)
  {
    if (this != &other) {
      RS_AtomicEntity::operator=(other);
      m_cachedLength = other.m_cachedLength;
    }
    return *this;
  }

  LC_CachedLengthEntity(LC_CachedLengthEntity&& entity) noexcept
      : RS_AtomicEntity{std::move(entity)}
      , m_cachedLength{entity.m_cachedLength}
  {
  }

  LC_CachedLengthEntity& operator = (LC_CachedLengthEntity&& other) noexcept {
    if (this != &other) {
      RS_AtomicEntity::operator=(std::move(other));
      m_cachedLength = other.m_cachedLength;
    }
    return *this;
  }

  double getLength() const override{
    return m_cachedLength;
  }

protected:
    // cached length for painting speedup
    double m_cachedLength = 0.0;
    virtual void updateLength() = 0;
};

#endif
