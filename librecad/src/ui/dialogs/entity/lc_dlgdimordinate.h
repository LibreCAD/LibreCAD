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

#ifndef LC_DLDDIMORDINATE_H
#define LC_DLDDIMORDINATE_H

#include "lc_entitypropertiesdlg.h"

class LC_DimOrdinate;

namespace Ui {
    class LC_DlgDimOrdinate;
}

class LC_DlgDimOrdinate : public LC_EntityPropertiesDlg{
    Q_OBJECT
public:
    LC_DlgDimOrdinate(QWidget *parent, LC_GraphicViewport *pViewport, LC_DimOrdinate* dim);
    ~LC_DlgDimOrdinate() override;
public slots:
    void updateEntity() override;
protected slots:
    virtual void languageChange();
protected:
    LC_DimOrdinate* m_entity = nullptr;
    void setEntity(LC_DimOrdinate *d);
private:
    Ui::LC_DlgDimOrdinate *ui;
};

#endif // LC_DLDDIMORDINATE_H
