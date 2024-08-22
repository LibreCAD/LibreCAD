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

#ifndef LC_MODIFYSTRETCHOPTIONS_H
#define LC_MODIFYSTRETCHOPTIONS_H

#include <QWidget>

#include "rs_actionmodifystretch.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_ModifyStretchOptions;
}

class LC_ModifyStretchOptions : public LC_ActionOptionsWidgetBase {
    Q_OBJECT

public:
    explicit LC_ModifyStretchOptions();
    ~LC_ModifyStretchOptions() override;
protected slots:
    void languageChange() override;
    void onKeepOriginalsClicked(bool val);
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_ModifyStretchOptions *ui;
    RS_ActionModifyStretch* action = nullptr;
    void setKeepOriginalsToActionAndView(bool val);
};

#endif // LC_MODIFYSTRETCHOPTIONS_H
