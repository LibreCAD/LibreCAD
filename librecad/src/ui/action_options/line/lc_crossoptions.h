/****************************************************************************
**
* Options widget for "DrawCross" action.

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
**********************************************************************/
#ifndef LC_CROSSOPTIONS_H
#define LC_CROSSOPTIONS_H

#include "lc_actiondrawcross.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_CrossOptions;
}

class LC_CrossOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT
public:
    explicit LC_CrossOptions();
    ~LC_CrossOptions() override;

public slots:
    void onXEditingFinished();
    void onYEditingFinished();
    void onAngleEditingFinished();
    void onModeIndexChanged(int index);
    void languageChange() override;
protected:
    void doSaveSettings() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
private:
    Ui::LC_CrossOptions *ui;
    LC_ActionDrawCross* action = nullptr;

    void setXToActionAndView(const QString &strValue);
    void setYToActionAndView(const QString &strValue);
    void setAngleToActionAndView(const QString &expr);
    void setModeToActionAndView(int mode);
};

#endif // LC_CROSSOPTIONS_H
