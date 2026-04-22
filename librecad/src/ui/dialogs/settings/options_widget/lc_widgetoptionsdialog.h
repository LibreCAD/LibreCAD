/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_WIDGETOPTIONSDIALOG_H
#define LC_WIDGETOPTIONSDIALOG_H

#include "lc_dialog.h"
#include "lc_iconcolorsoptions.h"
#include "ui_lc_widgetoptionsdialog.h"

class LC_WidgetOptionsDialog: public LC_Dialog, public Ui::LC_WidgetOptionsDialog{
    Q_OBJECT
public:
    QString selectFolder(const QString& title);
    void updateUIByOptions() const;
    explicit LC_WidgetOptionsDialog(QWidget* parent = nullptr);
    void reject() override;
public slots:
    void chooseStyleSheet();
    void accept() override;
    void applyIconColors();
    void onpbMainClicked();
    void onpbAccentClicked();
    void onpbBackClicked();
    void onMainIconColorChanged(const QString &);
    void onAccentIconColorChanged(const QString &);
    void onBackIconColorChanged(const QString &);
    void showAdvancedSetup();
    void setIconsOverrideFoler();
    void onSaveStylePressed();
    void onRemoveStylePressed();
    void onStyleChanged(const QString& val);
protected:
    QString m_currentIconsStyleName;
    LC_IconColorsOptions m_iconColorsOptions;
    QString setComboBoxColor(const QComboBox *combo);

    bool setupStylesCombobox() const;
    void updateStylesCombobox(QStringList options) const;
 };

#endif
