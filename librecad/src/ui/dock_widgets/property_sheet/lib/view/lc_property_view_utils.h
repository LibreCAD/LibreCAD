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

#ifndef LC_PROPERTYVIEWUTILS_H
#define LC_PROPERTYVIEWUTILS_H

#include <QPainter>
#include <QRect>
#include <QString>
#include <QStyle>
#include <QVariant>

class QPainter;

namespace LC_PropertyViewUtils {
    QString getElidedText(const QPainter& painter, const QString& text, const QRect& rect, bool* elided = nullptr);
    void drawElidedText(QPainter& painter, const QString& text, const QRect& rect, const QStyle* style = nullptr);

    inline QByteArray getViewNameLineEdit() {
        return QByteArrayLiteral("LineEdit");
    }

    inline QByteArray getViewNameComboBox() {
        return QByteArrayLiteral("ComboBox");
    }

    inline QByteArray getViewNameCheckBox() {
        return QByteArrayLiteral("CheckBox");
    }

    inline QByteArray getViewNameSpinBox() {
        return QByteArrayLiteral("SpinBox");
    }

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    void fixMinMaxVariant(QVariant& minv, QVariant& maxv) {
        bool minOk = false;
        bool maxOk = false;
        double min = minv.toDouble(&minOk);
        double max = maxv.toDouble(&maxOk);
        if (!minOk || max < min || !qIsFinite(min) || min < std::numeric_limits<T>::lowest() || min > std::numeric_limits<T>::max()) {
            minv = QVariant();
        }
        else {
            minv = min;
        }
        if (!maxOk || max < min || !qIsFinite(max) || max < std::numeric_limits<T>::lowest() || max > std::numeric_limits<T>::max()) {
            maxv = QVariant();
        }
        else {
            maxv = max;
        }
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    void fixMinMaxVariant(QVariant& minv, QVariant& maxv) {
        if (minv.typeId() == QMetaType::Type::ULongLong) {
            quint64 min = minv.toULongLong();
            if (min > std::numeric_limits<T>::max()) {
                minv = QVariant();
            }
            else {
                minv = T(min);
            }
        }
        else {
            bool minOk = false;
            qint64 min = minv.toLongLong(&minOk);
            if (!minOk || min < std::numeric_limits<T>::min() || (min >= 0 && min > std::numeric_limits<T>::max())) {
                minv = QVariant();
            }
            else {
                minv = T(min);
            }
        }

        if (maxv.typeId() == QMetaType::Type::ULongLong) {
            quint64 max = maxv.toULongLong();
            if (max > std::numeric_limits<T>::max()) {
                maxv = QVariant();
            }
            else {
                maxv = T(max);
            }
        }
        else {
            bool maxOk = false;
            qint64 max = maxv.toLongLong(&maxOk);
            if (!maxOk || max < std::numeric_limits<T>::min() || (max >= 0 && max > std::numeric_limits<T>::max())) {
                maxv = QVariant();
            }
            else {
                maxv = T(max);
            }
        }

        if (minv.isValid() && maxv.isValid() && maxv.value<T>() < minv.value<T>()) {
            minv = QVariant();
            maxv = QVariant();
        }
    }
}

#endif
