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

#include "lc_action_options_widget.h"

class LC_ActionModifyScale;

namespace Ui {
    class LC_ScaleOptionsWidget;
}

class LC_ScaleOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_ScaleOptionsWidget();
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
    void updateUI(int mode, const QVariant* value) override;
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    Ui::LC_ScaleOptionsWidget *ui = nullptr;
    LC_ActionModifyScale* m_action = nullptr;
};

#endif
