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

#ifndef LC_DLGNEWCUSTOMVARIABLE_H
#define LC_DLGNEWCUSTOMVARIABLE_H

#include "lc_dialog.h"
#include "rs_variable.h"

namespace Ui{
    class LC_DlgNewCustomVariable;
}

class LC_DlgNewCustomVariable : public LC_Dialog{
    Q_OBJECT
public:
    explicit LC_DlgNewCustomVariable(QWidget *parent = nullptr);
    ~LC_DlgNewCustomVariable() override;

    QString getPropertyName() const {
        return m_varName;
    }

    QString getPropertyValue() const {
        return m_varValue;
    }
    void accept() override;

    void setPropertyNames(QStringList* propertyNames){m_existingPropertyNames = propertyNames;}
private:
    Ui::LC_DlgNewCustomVariable *ui;
    QString m_varName;
    QString m_varValue;
    QStringList *m_existingPropertyNames;
};

#endif // LC_DLGNEWCUSTOMVARIABLE_H
