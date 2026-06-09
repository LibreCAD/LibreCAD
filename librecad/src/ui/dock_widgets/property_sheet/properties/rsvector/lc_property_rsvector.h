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

#ifndef PROPERTYQPOINTF_H
#define PROPERTYQPOINTF_H

#include "lc_property_double.h"
#include "lc_property_structbase.h"
#include "rs_vector.h"

class LC_PropertyRSVector : public LC_PropertyStructBase<RS_Vector, LC_PropertyDouble> {
    Q_OBJECT

public:
    using ValueType = RS_Vector;
    LC_PropertyRSVector(const LC_PropertyRSVector& other) = delete;
    explicit LC_PropertyRSVector(QObject* parent, bool holdValue = true);
    static QString getXKey();
    static QString getYKey();

    LC_PropertyAtomic* createXProperty();
    LC_PropertyAtomic* createYProperty();

    virtual QString getXLabel() const;
    virtual QString getXDescriptionFormat() const;
    virtual QString getYLabel() const;
    virtual QString getYDescriptionFormat() const;
    static QString getToStrValueFormat();

    LC_ActionContext::InteractiveInputInfo::InputType getInteractiveInputType() const {
        return m_interactiveInputType;
    }

    void setInteractiveInputType(const LC_ActionContext::InteractiveInputInfo::InputType interactiveInputType) {
        m_interactiveInputType = interactiveInputType;
    }

    LC_LateCompletionRequestor* getInteractiveInputRequestor() const {
        return m_lateCompletionRequestor;
    }

    LC_ActionContext* getActionContext() const {
        return m_actionContext;
    }

    void setActionContextAndLaterRequestor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor);
    void requestInteractiveInput() const;

    LC_Formatter* getFormatter() const {
        return m_formatter;
    }

protected:
    LC_ActionContext::InteractiveInputInfo::InputType m_interactiveInputType = LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
    LC_ActionContext* m_actionContext{nullptr};
    LC_LateCompletionRequestor* m_lateCompletionRequestor{nullptr};
    LC_Formatter* m_formatter{nullptr};
};

#endif
