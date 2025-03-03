/****************************************************************************
**
*Options widget for ModifyScale action

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

#ifndef LC_MODIFYSCALEOPTIONS_H
#define LC_MODIFYSCALEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "rs_actionmodifyscale.h"

namespace Ui {
    class LC_ModifyScaleOptions;
}

class LC_ModifyScaleOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_ModifyScaleOptions();
    ~LC_ModifyScaleOptions();
public slots:
    void languageChange() override;
    void cbKeepOriginalsClicked(bool val);
    void cbMultipleCopiesClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void cbExplicitFactorClicked(bool val);
    void cbIsotropicClicked(bool val);
    void onFactorXEditingFinished();
    void onFactorYEditingFinished();
    void onCopiesNumberValueChanged(int value);
    void updateUI(int mode) override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_ModifyScaleOptions *ui;
    RS_ActionModifyScale* action = nullptr;
    void setUseMultipleCopiesToActionAndView(bool copies);
    void setUseCurrentLayerToActionAndView(bool val);
    void setUseCurrentAttributesToActionAndView(bool val);
    void setKeepOriginalsToActionAndView(bool val);
    void setCopiesNumberToActionAndView(int number);
    void setExplicitFactorToActionAndView(bool val);
    void setIsotropicScalingFactorToActionAndView(bool val);
    void setFactorXToActionAndView(QString val);
    void setFactorYToActionAndView(QString val);
};

#endif // LC_MODIFYSCALEOPTIONS_H
