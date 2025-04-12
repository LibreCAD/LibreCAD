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

#ifndef LC_MODIFYALIGNOPTIONS_H
#define LC_MODIFYALIGNOPTIONS_H

#include "lc_actionoptionswidget.h"

class LC_ActionModifyAlignData;
namespace Ui {
    class LC_ModifyAlignOptions;
}

class LC_ModifyAlignOptions : public LC_ActionOptionsWidget{
    Q_OBJECT

public:
    explicit LC_ModifyAlignOptions();
    ~LC_ModifyAlignOptions();

protected slots:
    void languageChange() override;
    void onAsGroupChanged(bool val);
    void onVAlignChanged(bool val);
    void onHAlignChanged(bool val);
    void onAlignToIndexChanged(int idx);
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    int getHAlignFromUI();
    int getVAlignFromUI();
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsGroupName() override;
    QString getSettingsOptionNamePrefix() override;
private:
    Ui::LC_ModifyAlignOptions *ui;
    LC_ActionModifyAlignData* m_action = nullptr;
    bool m_forAlignAction = false;
    void setAlignTypeToActionAndView(int type);
    void setVAlignToActionAndView(int valign);
    void setHAlignToActionAndView(int halign);
    void setAsGroupToActionAndView(bool group);
};

#endif // LC_MODIFYALIGNOPTIONS_H
