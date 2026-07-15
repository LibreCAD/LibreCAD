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

#ifndef LC_PROPERTYNUMERIC_H
#define LC_PROPERTYNUMERIC_H

#undef max
#undef min

#include <limits>

#include "lc_property.h"
#include "lc_property_single.h"

template <typename ValueType>
class LC_PropertyNumeric : public LC_PropertySingle<ValueType> {
    template <class T, class Enable = void>
    struct interval_t {
    };

    template <class T>
    struct interval_t<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        using type = double;
        using maxsigned = double;
    };

    template <class T>
    struct interval_t<T, std::enable_if_t<std::is_integral_v<T>>> {
        using type = typename std::make_unsigned<T>::type;
        using maxsigned = qint64;
    };

public:
    using IntervalType = typename interval_t<ValueType>::type;
    using SignedMaxType = typename interval_t<ValueType>::maxsigned;
    using MaxIntervalType = typename interval_t<SignedMaxType>::type;

    // fixme - rename
    ValueType value() const {
        auto value = LC_PropertySingle<ValueType>::value();
        return correctValue(value);
    }
    ValueType minValue() const {
        return m_minValue;
    }

    void setMinValue(ValueType minValue) {
        if (m_minValue == minValue) {
            return;
        }

        emit this->beforePropertyChange(PropertyChangeReasonValueNew, nullptr, 0);
        m_minValue = minValue;
        m_maxValue = std::max(m_minValue, m_maxValue);
        emit this->afterPropertyChange(PropertyChangeReasonValueNew);
    }

    ValueType maxValue() const {
        return m_maxValue;
    }

    void setMaxValue(ValueType maxValue) {
        if (maxValue == m_maxValue) {
            return;
        }

        emit this->beforePropertyChange(PropertyChangeReasonValueNew, nullptr, 0);
        m_maxValue = maxValue;
        m_minValue = std::min(m_minValue, m_maxValue);
        emit this->afterPropertyChange(PropertyChangeReasonValueNew);
    }

    ValueType correctValue(ValueType value) const {
        if (value < m_minValue) {
            value = m_minValue;
        }
        if (value > m_maxValue) {
            value = m_maxValue;
        }
        return value;
    }


    void setStepValue(ValueType stepValue) {
        if (stepValue == m_stepValue) {
            return;
        }

        emit this->beforePropertyChange(PropertyChangeReasonStateLocal, nullptr, 0);
        m_stepValue = stepValue;
        emit this->afterPropertyChange(PropertyChangeReasonStateLocal);
    }

    ValueType stepValue() const {
        return m_stepValue;
    }

protected:
    explicit LC_PropertyNumeric(QObject* parent, bool holdValue = true)
        : LC_PropertySingle<ValueType>(parent, holdValue), m_minValue(std::numeric_limits<ValueType>::lowest()),
          m_maxValue(std::numeric_limits<ValueType>::max()), m_stepValue(ValueType(1)) {
    }

    bool doAcceptValue(ValueType valueToAccept) override {
        if (valueToAccept < m_minValue) {
            return false;
        }
        if (valueToAccept > m_maxValue) {
            return false;
        }
        return true;
    }

private:
    ValueType m_minValue;
    ValueType m_maxValue;
    ValueType m_stepValue;

    Q_DISABLE_COPY(LC_PropertyNumeric)
};

#endif
