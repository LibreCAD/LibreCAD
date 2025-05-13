/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#ifndef LC_PENPALETTEOPTIONSDIALOG_H
#define LC_PENPALETTEOPTIONSDIALOG_H

#include "lc_dialog.h"
#include "ui_lc_penpaletteoptionsdialog.h"

class LC_PenPaletteOptions;

class LC_PenPaletteOptionsDialog : public LC_Dialog, public Ui::LC_PenPaletteOptionsDialog{
    Q_OBJECT
public:
    explicit LC_PenPaletteOptionsDialog(QWidget *parent, LC_PenPaletteOptions* m_options, bool focusOnfile);
    ~LC_PenPaletteOptionsDialog() override;
public slots:
    void validate();
protected slots:
    void languageChange();
private:
    void selectActivePenBGColor();
    void set_color(QComboBox *combo, QColor &custom);
    void initComboBox(QComboBox *cb, const QColor &color);
    LC_PenPaletteOptions* m_options = nullptr;
    void selectGridColor();
    void selectMatchedItemColor();
    void showInvalidColorMessage(const QString &name);
};

#endif // LC_PENPALETTEOPTIONSDIALOG_H
