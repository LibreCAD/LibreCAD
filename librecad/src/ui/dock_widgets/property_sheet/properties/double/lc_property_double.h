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

#ifndef LC_PROPERTYDOUBLE_H
#define LC_PROPERTYDOUBLE_H

#include "lc_actioncontext.h"
#include "lc_formatter.h"
#include "lc_property_numeric.h"

class LC_PropertyDouble : public LC_PropertyNumeric<double> {
    Q_OBJECT

public:
    explicit LC_PropertyDouble(QObject* parent, bool holdValue = true);
    LC_PropertyDouble(const LC_PropertyDouble& other) = delete;

    LC_ActionContext::InteractiveInputInfo::InputType getInteractiveInputType() const {
        return m_interactiveInputType;
    }

    void setInteractiveInputType(const LC_ActionContext::InteractiveInputInfo::InputType interactiveInputType) {
        m_interactiveInputType = interactiveInputType;
    }

    void setActionContextAndLaterRequestor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor);
    void requestInteractiveInput() const;

    void setFormatter(LC_Formatter* formatter) {
        m_formatter = formatter;
    }

    LC_Formatter* getFormatter() const {
        return m_formatter;
    }

protected:
    LC_ActionContext::InteractiveInputInfo::InputType m_interactiveInputType = LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
    LC_ActionContext* m_actionContext = nullptr;
    LC_LateCompletionRequestor* m_lateCompletionRequestor = nullptr;
    LC_Formatter* m_formatter = nullptr;
};
#endif
