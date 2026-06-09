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
    ~LC_DlgIconsSetup() override;
    void setIconsOptions(LC_IconColorsOptions *options);
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
    void onGenericMainColorChanged(const QString &value) const;
    void onGenericAccentColorChanged(const QString &value) const;
    void onGenericBackColorChanged(const QString &value) const;
    void onActiveOnMainColorChanged(const QString &value) const;
    void onActiveOnAccentColorChanged(const QString &value) const;
    void onActiveOnBackColorChanged(const QString &value) const;
    void onActiveOffMainColorChanged(const QString &value) const;
    void onActiveOffAccentColorChanged(const QString &value) const;
    void onActiveOffBackColorChanged(const QString &value) const;
    void onNormalOnMainColorChanged(const QString &value) const;
    void resetToDefaults();
    void applyIconColors() const;
protected:
    Ui::LC_DlgIconsSetup *ui;
    LC_IconColorsOptions *m_iconColorsOptions;
    void onNormalOnAccentColorChanged(const QString &value) const;
    void onNormalOnBackColorChanged(const QString &value) const;
    void onNormalOffMainColorChanged(const QString &value) const;
    void onNormalOffAccentColorChanged(const QString &value) const;
    void onNormalOffBackColorChanged(const QString &value) const;
    void onSelectedOnMainColorChanged(const QString &value) const;
    void onSelectedOnAccentColorChanged(const QString &value) const;
    void onSelectedOnBackColorChanged(const QString &value) const;
    void onSelectedOffMainColorChanged(const QString &value) const;
    void onSelectedOffAccentColorChanged(const QString &value) const;
    void onSelectedOffBackColorChanged(const QString &value) const;
    void onDisabledOnMainColorChanged(const QString &value) const;
    void onDisabledOnAccentColorChanged(const QString &value) const;
    void onDisabledOnBackColorChanged(const QString &value) const;
    void onDisabledOffMainColorChanged(const QString &value) const;
    void onDisabledOffAccentColorChanged(const QString &value) const;
    void onDisabledOffBackColorChanged(const QString &value) const;
    QString setComboBoxColor(const QComboBox *combo);
    void initCombobox(const LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state,
        LC_SVGIconEngineAPI::ColorType type, QComboBox *ctrl);
    void saveColor(LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state,
        LC_SVGIconEngineAPI::ColorType type, const QComboBox *ctrl);
};
#endif
