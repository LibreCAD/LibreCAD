/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_DLGABOUT_H
#define LC_DLGABOUT_H

#include<memory>
#include "lc_dialog.h"
#include "qc_applicationwindow.h"

namespace Ui {
class LC_DlgAbout;
}

class LC_DlgAbout : public LC_Dialog
{
    Q_OBJECT

public:
    explicit LC_DlgAbout(QWidget *parent = nullptr);
    ~LC_DlgAbout();
protected slots:
    void copyInfo();
private:
    std::unique_ptr<Ui::LC_DlgAbout> ui;
    QString info;
};

#endif // LC_DLGABOUT_H
