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

#ifndef LC_ITEMINFO_H
#define LC_ITEMINFO_H

#include <QIcon>
#include <QVariant>

struct LC_ItemInfo {
    QIcon icon;
    QString displayName;
    QVariant data;

    LC_ItemInfo(LC_ItemInfo& src) {
        icon = src.icon;
        displayName = src.displayName;
        data = src.data;
    }
    LC_ItemInfo(const LC_ItemInfo& src) {
        icon = src.icon;
        displayName = src.displayName;
        data = src.data;
    }

    LC_ItemInfo(const QIcon& icon, const QString& displayName, const QVariant& data)
        : icon{icon}, displayName{displayName}, data{data} {
    }

    LC_ItemInfo(const QString& displayName, const QVariant& data)
    : icon{QIcon()}, displayName{displayName}, data{data} {
    }

    LC_ItemInfo(const QIcon& icon, const QString& displayName, const QString& val)
        : icon{icon}, displayName{displayName}, data{QVariant(val)} {
    }

    LC_ItemInfo(const QIcon& icon, const QString& displayName, const int val)
        : icon{icon}, displayName{displayName}, data{QVariant{val}} {
    }

    LC_ItemInfo &operator =(const LC_ItemInfo &src) {
        icon = src.icon;
        displayName = src.displayName;
        data = src.data;
        return *this;
    }
};

#endif
