/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
#ifndef LC_DLGICONSSETUP_H
#define LC_DLGICONSSETUP_H
#include <qcombobox.h>

#include "lc_dialog.h"
#include "lc_iconengineshared.h"
class LC_IconColorsOptions;

namespace Ui {
    class LC_DlgIconsSetup;
}

class LC_DlgIconsSetup:public LC_Dialog {
    Q_OBJECT

public:
    explicit LC_DlgIconsSetup(QWidget *parent = nullptr);
    ~LC_DlgIconsSetup();

    void setIconsOptions(LC_IconColorsOptions *iconColorsOptions);
    void initComboBox(QComboBox *cb, const QString &text);
    void accept() override;
public slots:
    void onPbGenericMainClicked();
    void onPbGenericAccentClicked();
    void onPbGenericBackClicked();
    void onPbActiveOnMainClicked();
    void onPbActiveOnAccentClicked();
    void onPbActiveOnBackClicked();
    void onPbActiveOffMainClicked();
    void onPbActiveOffAccentClicked();
    void onPbActiveOffBackClicked();
    void onPbNormalOnMainClicked();
    void onPbNormalOnAccentClicked();
    void onPbNormalOnBackClicked();
    void onPbNormalOffMainClicked();
    void onPbNormalOffAccentClicked();
    void onPbNormalOffBackClicked();
    void onPbSelectedOnMainClicked();
    void onPbSelectedOnAccentClicked();
    void onPbSelectedOnBackClicked();
    void onPbSelectedOffMainClicked();
    void onPbSelectedOffAccentClicked();
    void onPbSelectedOffBackClicked();
    void onPbDisabledOnMainClicked();
    void onPbDisabledOnAccentClicked();
    void onPbDisabledOnBackClicked();
    void onPbDisabledOffMainClicked();
    void onPbDisabledOffAccentClicked();
    void onPbDisabledOffBackClicked();
    void resetToDefaults();
protected:
    Ui::LC_DlgIconsSetup *ui;
    LC_IconColorsOptions *iconColorsOptions;
    void set_color(QComboBox *combo);
    void initCombobox(LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state,
        LC_SVGIconEngineAPI::ColorType type, QComboBox *ctrl);
    void saveColor(LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state,
        LC_SVGIconEngineAPI::ColorType type, QComboBox *ctrl);
};
#endif // LC_DLGICONSSETUP_H
