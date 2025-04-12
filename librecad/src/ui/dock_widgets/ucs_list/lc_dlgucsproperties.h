/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_DLGUCSPROPERTIES_H
#define LC_DLGUCSPROPERTIES_H

#include "lc_dialog.h"
#include "rs.h"

namespace Ui {
    class LC_DlgUCSProperties;
}

class LC_UCSList;
class LC_UCS;

class LC_DlgUCSProperties : public LC_Dialog{
    Q_OBJECT
public:
    explicit LC_DlgUCSProperties(QWidget *parent = nullptr);
    ~LC_DlgUCSProperties();
    void setUCS(LC_UCSList *ucsList, bool applyDuplicateSilently, LC_UCS* u, RS2::Unit unit,  RS2::LinearFormat linearFormat, int linearPrec, RS2::AngleFormat angleFormat, int anglePrec);
    void updateUCS();
public slots:
    void languageChange();
private:
    Ui::LC_DlgUCSProperties *ui;
    LC_UCS* m_ucs;
    LC_UCSList* m_ucsList;
    bool m_applyDuplicateSilently = true;
};

#endif // LC_DLGUCSPROPERTIES_H
