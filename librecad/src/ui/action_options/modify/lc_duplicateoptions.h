/****************************************************************************
**
* Options widget for "Duplicate" action.

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
#ifndef LC_DUPLICATEOPTIONS_H
#define LC_DUPLICATEOPTIONS_H

#include "lc_actionmodifyduplicate.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_DuplicateOptions;
}

class LC_DuplicateOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_DuplicateOptions();
    ~LC_DuplicateOptions() override;

protected slots:
    void onOffsetXEditingFinished();
    void onOffsetYEditingFinished();
    void onInPlaceClicked(bool value);
    void onPenModeIndexChanged(int mode);
    void onLayerModeIndexChanged(int mode);
    void languageChange() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
private:
    Ui::LC_DuplicateOptions *ui;
    LC_ActionModifyDuplicate * action;
    void setOffsetXToActionAndView(const QString &val);
    void setOffsetYToActionAndView(const QString &val);
    void setInPlaceDuplicateToActionAndView(bool inplace);
    void setPenModeToActionAndView(int mode);
    void setLayerModeToActionAndeView(int mode);
};

#endif // LC_DUPLICATEOPTIONS_H
