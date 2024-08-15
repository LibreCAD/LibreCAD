/****************************************************************************
**
* Options widget for ModifyRotate action

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

#ifndef LC_MODIFYROTATEOPTIONS_H
#define LC_MODIFYROTATEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "rs_actionmodifyrotate.h"

namespace Ui {
    class LC_ModifyRotateOptions;
}

class LC_ModifyRotateOptions : public LC_ActionOptionsWidgetBase{
Q_OBJECT

public:
    explicit LC_ModifyRotateOptions();
    ~LC_ModifyRotateOptions() override;

    enum UpdateMode{
        UPDATE_ANGLE,
        DISABLE_SECOND_ROTATION,
        ENABLE_SECOND_ROTATION,
        UPDATE_ANGLE2
    };
    void updateUI(int mode) override;
public slots:
    void languageChange() override;
    void cbKeepOriginalsClicked(bool val);
    void cbMultipleCopiesClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void cbFreeAngleClicked(bool val);
    void cbFreeRefAngleClicked(bool val);
    void onTwoRotationsClicked(bool val);
    void onAbsoluteRefAngleClicked(bool val);
    void onAngleEditingFinished();
    void onRefPointAngleEditingFinished();
    void onCopiesNumberValueChanged(int value);
protected:

    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_ModifyRotateOptions *ui;
    RS_ActionModifyRotate* action = nullptr;
    void setUseMultipleCopiesToActionAndView(bool copies);
    void setUseCurrentLayerToActionAndView(bool val);
    void setUseCurrentAttributesToActionAndView(bool val);
    void setKeepOriginalsToActionAndView(bool val);
    void setCopiesNumberToActionAndView(int number);
    void setFreeAngleToActionAndView(bool val);
    void setFreeRefAngleToActionAndView(bool val);
    void setAbsoluteRefAngleToActionAndView(bool val);
    void setTwoRotationsToActionAndView(bool val);
    void setAngleToActionAndView(QString val);
    void setRefPointAngleToActionAndView(QString val);
    void allowSecondRotationUI(bool enable);
};
#endif // LC_MODIFYROTATEOPTIONS_H
