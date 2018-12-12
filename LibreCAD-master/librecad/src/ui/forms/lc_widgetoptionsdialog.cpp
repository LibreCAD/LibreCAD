/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#include "lc_widgetoptionsdialog.h"
#include <QFileDialog>

LC_WidgetOptionsDialog::LC_WidgetOptionsDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(stylesheet_button, SIGNAL(released()),
            this, SLOT(chooseStyleSheet()));
}

void LC_WidgetOptionsDialog::chooseStyleSheet()
{
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty())
    {
        stylesheet_field->setText(QDir::toNativeSeparators(path));
    }
}
