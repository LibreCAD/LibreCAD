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

#include "lc_property_double_spinbox.h"

LC_PropertyDoubleSpinBox::LC_PropertyDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent) {
}

QValidator::State LC_PropertyDoubleSpinBox::validate(QString& text, int& pos) const {
    for (auto& chr : text) {
        if (chr == QLatin1Char('.') || chr == QLatin1Char(',')) {
            chr = locale().decimalPoint().front();
        }
    }
    return QDoubleSpinBox::validate(text, pos);
}

QString LC_PropertyDoubleSpinBox::textFromValue(const double val) const {
    return valueToText(val, locale(), decimals(), isGroupSeparatorShown());
}

QString LC_PropertyDoubleSpinBox::valueToText(const double value, const QLocale& locale, int decimals, const bool groupSeparatorShown) {
    if (!qIsFinite(value)) {
        return locale.toString(value);
    }
    auto str = QByteArray::number(static_cast<quint64>(qAbs(value)));

    int i = str.length();
    if (!str.startsWith('0')) {
        i++;
    }
    const int maxDecimals = std::numeric_limits<double>::digits10 - i;
    decimals = std::max(0, std::min(maxDecimals, decimals));
    str = QByteArray::number(value, 'f', decimals);
    if (decimals >= 2 && decimals == maxDecimals && (str.endsWith("99") || str.endsWith("01"))) {
        decimals--;
    }
    auto result = locale.toString(value, 'f', decimals);
    const auto decimalPoint = locale.decimalPoint();
    const auto groupSeparator = locale.groupSeparator();
    if (!groupSeparatorShown) {
        result.remove(groupSeparator);
    }
    i = result.indexOf(decimalPoint);
    if (i >= 0) {
        const auto zeroDigit = locale.zeroDigit();
        const auto begin = result.constData();
        auto data = &begin[result.length() - 1];
        const auto decBegin = &begin[i];
        while (data >= decBegin && (*data == zeroDigit || *data == decimalPoint || *data == groupSeparator)) {
            data--;
        }
        result.resize(static_cast<int>(data + 1 - begin));
    }
    return result;
}
