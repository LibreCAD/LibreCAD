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

#ifndef LC_INPUTTEXTDIALOG_H
#define LC_INPUTTEXTDIALOG_H

#include "lc_dialog.h"

namespace Ui {
    class LC_InputTextDialog;
}

class LC_InputTextDialog : public LC_Dialog{
    Q_OBJECT
public:
    explicit LC_InputTextDialog(QWidget *parent = nullptr);
    ~LC_InputTextDialog() override;
    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           const QStringList &options,
                           bool allowsEditing,
                           const QString &text = QString(), bool *ok = nullptr);
    static int selectId(QWidget *parent, const QString &title, const QString &label,
                           const QList<QPair<int, QString>> &options, bool *ok = nullptr);
private:
    Ui::LC_InputTextDialog *ui;
};

#endif // LC_INPUTTEXTDIALOG_H
