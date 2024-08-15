/****************************************************************************
**
* Options widget for pen transform action

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
#ifndef LC_PASTETRANSFORMOPTIONS_H
#define LC_PASTETRANSFORMOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "lc_actioneditpastetransform.h"

namespace Ui {
    class LC_PasteTransformOptions;
}

class LC_PasteTransformOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT
public:
    explicit LC_PasteTransformOptions();
    ~LC_PasteTransformOptions() override;

public slots:
    void languageChange() override;
    void onAngleEditingFinished();
    void onFactorEditingFinished();
    void onArraySpacingXEditingFinished();
    void onArraySpacingYEditingFinished();
    void onArrayClicked(bool clicked);
    void onArrayAngleEditingFinished();
    void onArrayXCountChanged(int value);
    void onArrayYCountChanged(int value);
    void cbSameAnglesClicked(bool value);
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_PasteTransformOptions *ui;
    LC_ActionEditPasteTransform* action = nullptr;

    void setAngleToActionAndView(QString val);
    void setFactorToActionAndView(QString val);
    void setIsArrayToActionAndView(bool val);
    void setSameAnglesToActionAndView(bool val);
    void setArrayXCountToActionAndView(int count);
    void setArrayYCountToActionAndView(int count);
    void setArrayXSpacingToActionAndView(QString val);
    void setArrayYSpacingToActionAndView(QString val);
    void setArrayAngleToActionAndView(QString val);
};

#endif // LC_PASTETRANSFORMOPTIONS_H
