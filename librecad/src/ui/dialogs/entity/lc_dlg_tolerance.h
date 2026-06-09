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

#ifndef LC_DLGTOLERANCE_H
#define LC_DLGTOLERANCE_H

#include "lc_entitypropertiesdlg.h"
#include "lc_tolerance.h"

class QComboBox;

namespace Ui {
    class LC_DlgTolerance;
}

class LC_DlgTolerance : public LC_EntityPropertiesDlg{
    Q_OBJECT
public:
    explicit LC_DlgTolerance(QWidget *parent, LC_GraphicViewport *viewport, LC_Tolerance* tol, bool isNew);
    ~LC_DlgTolerance() override;
public slots:
    QString generateDataString();
    void updateEntity() override;
    void accept() override;
protected slots:
    void languageChange();
    void parseAndSetFields(const QString& string);
    void setEntity(const LC_Tolerance* e);
private:
    Ui::LC_DlgTolerance *ui;
    void initModifiersComboBox(QComboBox* comboBox) const;
    static void initGeometricCharacterCombobox(QComboBox* comboBox);
    LC_Tolerance* m_entity;
    bool m_isNew = false;
    bool m_showExtendedModifiers = false;
};

#endif
