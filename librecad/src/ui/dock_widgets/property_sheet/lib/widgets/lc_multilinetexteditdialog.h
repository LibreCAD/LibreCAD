/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_MULTILINETEXTEDITDIALOG_H
#define LC_MULTILINETEXTEDITDIALOG_H

#include <QDialog>

namespace Ui {
    class LC_MultilineTextEditDialog;
}

class QAbstractButton;

class LC_MultilineTextEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit LC_MultilineTextEditDialog(QWidget* parent = nullptr);
    ~LC_MultilineTextEditDialog() override;
    void setReadOnly(bool value) const;
    QString getText() const;
    void setText(const QString& text) const;

private
    slots :
    void on_buttonBox_clicked(QAbstractButton* button);

private:
    Ui::LC_MultilineTextEditDialog* ui;
};

#endif
