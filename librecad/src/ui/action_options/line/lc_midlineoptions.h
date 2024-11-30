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

#ifndef LC_MIDLINEOPTIONS_H
#define LC_MIDLINEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "lc_actiondrawmidline.h"

namespace Ui {
class LC_MidLineOptions;
}

class LC_MidLineOptions : public LC_ActionOptionsWidgetBase
{
    Q_OBJECT
public:
    explicit LC_MidLineOptions();
    ~LC_MidLineOptions() override;

protected slots:
    void languageChange() override;
    void onOffsetEditingFinished();
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
    void setOffsetToActionAndView(const QString& val);
private:
    Ui::LC_MidLineOptions *ui;
    LC_ActionDrawMidLine* action;
};

#endif // LC_MIDLINEOPTIONS_H
