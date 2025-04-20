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

#ifndef LC_SELECTWINDOWOPTIONS_H
#define LC_SELECTWINDOWOPTIONS_H

#include "lc_actionoptionswidgetbase.h"

class RS_ActionSelectWindow;
namespace Ui {
    class LC_SelectWindowOptions;
}

class LC_SelectWindowOptions: public LC_ActionOptionsWidgetBase {
Q_OBJECT
public:
    LC_SelectWindowOptions();
    ~LC_SelectWindowOptions() override;
public slots:
    void languageChange() override;
    void onAllToggled(bool value);
    void onTypeToggled(bool value);
protected:
    Ui::LC_SelectWindowOptions *ui;
    RS_ActionSelectWindow* m_action = nullptr;
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void setSelectAllToActionAndView(bool value);
    void setEntityTypesToActinAndView(QList<RS2::EntityType> entityTypes);
    void enableEntityTypes(bool enable) const;
};
#endif // LC_SELECTWINDOWOPTIONS_H
