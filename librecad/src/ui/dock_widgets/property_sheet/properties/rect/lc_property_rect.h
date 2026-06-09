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

#ifndef LC_PROPERTYRECT_H
#define LC_PROPERTYRECT_H
#include "lc_property_structbase.h"
#include "rs_graphic.h"

class LC_Formatter;
class LC_ActionContext;
class LC_PropertyDouble;

class LC_PropertyRect : public LC_PropertyStructBase<LC_MarginsRect, LC_PropertyDouble>{
   Q_OBJECT
public:
    using ValueType = LC_MarginsRect;
    LC_PropertyRect(const LC_PropertyRect& other) = delete;
    explicit LC_PropertyRect(QObject* parent, bool holdValue = true);

    LC_PropertyAtomic* createLeftProperty();
    LC_PropertyAtomic* createRightProperty();
    LC_PropertyAtomic* createTopProperty();
    LC_PropertyAtomic* createBottomProperty();

    virtual QString getLeftLabel() const;
    virtual QString getRightLabel() const;
    virtual QString getTopLabel() const;
    virtual QString getBottomLabel() const;
    QString getLeftDescriptionFormat() const;
    QString getRightDescriptionFormat() const;
    QString getTopDescriptionFormat() const;
    QString getBottomDescriptionFormat() const;

    static QString getToStrValueFormat();

    void setActionContext(LC_ActionContext* actionContext);

    LC_ActionContext* getActionContext() const {
        return m_actionContext;
    }

    LC_Formatter* getFormatter() const {
        return m_formatter;
    }

    void setLeftDescription(const QString &v) {
        m_leftDescription = v;
    }

    void setRightDescription(const QString &v) {
        m_rightDescription = v;
    }

    void setTopDescription(const QString &v)  {
        m_topDescription = v;
    }

    void setBottomDescription(const QString &v) {
        m_bottomDescription = v;
    }

protected:
    LC_Formatter* m_formatter {nullptr};
    LC_ActionContext* m_actionContext {nullptr};
    QString m_leftDescription;
    QString m_rightDescription;
    QString m_topDescription;
    QString m_bottomDescription;
};

#endif
