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
#include "rs_settings.h"
#include "qc_applicationwindow.h"
#include <QFileDialog>
#include <QStyleFactory>
#include <QStatusBar>

LC_WidgetOptionsDialog::LC_WidgetOptionsDialog(QWidget* parent)
    : LC_Dialog(parent, "WidgetOptions"){
    setupUi(this);
    connect(stylesheet_button, SIGNAL(released()),
            this, SLOT(chooseStyleSheet()));

    LC_GROUP_GUARD("Widgets");{
        bool allow_style = LC_GET_BOOL("AllowStyle", false);
        style_checkbox->setChecked(allow_style);
        style_combobox->addItems(QStyleFactory::keys());
        if (allow_style) {
            QString a_style = LC_GET_STR("Style", "");
            if (!a_style.isEmpty()) {
                int index = style_combobox->findText(a_style);
                style_combobox->setCurrentIndex(index);
            }
        }

        QString sheet_path = LC_GET_STR("StyleSheet", "");
        if (!sheet_path.isEmpty() && QFile::exists(sheet_path))
            stylesheet_field->setText(sheet_path);

        bool allow_theme = LC_GET_BOOL("AllowTheme", false);
        theme_checkbox->setChecked(allow_theme);

        bool allow_toolbar_icon_size = LC_GET_BOOL("AllowToolbarIconSize", false);
        toolbar_icon_size_checkbox->setChecked(allow_toolbar_icon_size);

        int toolbar_icon_size = LC_GET_INT("ToolbarIconSize", 24);
        toolbar_icon_size_spinbox->setValue(toolbar_icon_size);

        bool allow_statusbar_height = LC_GET_BOOL("AllowStatusbarHeight", false);
        statusbar_height_checkbox->setChecked(allow_statusbar_height);

        int statusbar_height = LC_GET_INT("StatusbarHeight", 32);
        statusbar_height_spinbox->setValue(statusbar_height);

        bool allow_statusbar_fontsize = LC_GET_BOOL("AllowStatusbarFontSize", false);
        statusbar_fontsize_checkbox->setChecked(allow_statusbar_fontsize);

        int statusbar_fontsize = LC_GET_INT("StatusbarFontSize", 12);
        statusbar_fontsize_spinbox->setValue(statusbar_fontsize);

        int leftToolbarColumnsCount = LC_GET_INT("LeftToolbarColumnsCount", 5);
        left_toobar_columns_spinbox->setValue(leftToolbarColumnsCount);
    }

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);

    statusbar_height_spinbox->setEnabled(useClassicalStatusBar);
    statusbar_height_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_spinbox->setEnabled(useClassicalStatusBar);

//    lClassicStatusBarOnly->setVisible(!useClassicalStatusBar);

}

void LC_WidgetOptionsDialog::accept() {
    LC_GROUP_GUARD("Widgets");
    {
        bool allow_style = style_checkbox->isChecked();
        LC_SET("AllowStyle", allow_style);
        if (allow_style) {
            QString style = style_combobox->currentText();
            LC_SET("Style", style);
            QApplication::setStyle(QStyleFactory::create(style));
        }

        QString sheet_path = stylesheet_field->text();
        LC_SET("StyleSheet", sheet_path);
        if (QC_ApplicationWindow::loadStyleSheet(sheet_path)) {
           // nothing to do
        }

        bool allow_theme = theme_checkbox->isChecked();
        LC_SET("AllowTheme", allow_theme);
        auto& appWindow = QC_ApplicationWindow::getAppWindow();
        int allow_toolbar_icon_size = toolbar_icon_size_checkbox->isChecked();
        LC_SET("AllowToolbarIconSize", allow_toolbar_icon_size);
        if (allow_toolbar_icon_size) {
            int toolbar_icon_size = toolbar_icon_size_spinbox->value();
            LC_SET("ToolbarIconSize", toolbar_icon_size);
            appWindow->setIconSize(QSize(toolbar_icon_size, toolbar_icon_size));
        }

        int allow_statusbar_fontsize = statusbar_fontsize_checkbox->isChecked();
        LC_SET("AllowStatusbarFontSize", allow_statusbar_fontsize);
        if (allow_statusbar_fontsize) {
            int statusbar_fontsize = statusbar_fontsize_spinbox->value();
            LC_SET("StatusbarFontSize", statusbar_fontsize);
            QFont font;
            font.setPointSize(statusbar_fontsize);
            appWindow->statusBar()->setFont(font);
        }

        int allow_statusbar_height = statusbar_height_checkbox->isChecked();
        LC_SET("AllowStatusbarHeight", allow_statusbar_height);
        if (allow_statusbar_height) {
            int statusbar_height = statusbar_height_spinbox->value();
            LC_SET("StatusbarHeight", statusbar_height);
            appWindow->statusBar()->setMinimumHeight(statusbar_height);
        }

        int columnCount = left_toobar_columns_spinbox->value();
        LC_SET("LeftToolbarColumnsCount", columnCount);
    }
    LC_Dialog::accept();
}

void LC_WidgetOptionsDialog::chooseStyleSheet()
{
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty())
    {
        stylesheet_field->setText(QDir::toNativeSeparators(path));
    }
}
