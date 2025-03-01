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
#include "lc_iconengineshared.h"
#include "rs_settings.h"
#include "qc_applicationwindow.h"
#include <QFileDialog>
#include <QStyleFactory>
#include <QStatusBar>
#include <QApplication>
#include <QColorDialog>
#include <QMessageBox>
#include <QPixmapCache>

#include "lc_dlgiconssetup.h"
/**
     *Fixme - sand - add preview for icon colors changes
     *
     * so far, when widget options dialog is invoked (in modal), applyIconColors has no effect during open dialog.
     * this method is called, yet due to some reasons that are not clear for me, icons are rendered using old colors.
     *
     * Only on closing the dialog, icons are rendered with specified colors. At least, such behavior is uner Win.
     *
     * P.S - of course, it's possible to illustrate colors via custom widget and SvgRenderer (and replacing colors in svg here, in addition to
     * iconengine. However, it might be an overkill - that implementation may be added if there will be no more important features to do.
     */
LC_WidgetOptionsDialog::LC_WidgetOptionsDialog(QWidget* parent)
    : LC_Dialog(parent, "WidgetOptions"){
    setupUi(this);
    connect(stylesheet_button,&QPushButton::released, this, &LC_WidgetOptionsDialog::chooseStyleSheet);

    connect(pbMain, &QToolButton::clicked, this, &LC_WidgetOptionsDialog::onpbMainClicked);
    connect(pbAccent, &QToolButton::clicked, this, &LC_WidgetOptionsDialog::onpbAccentClicked);
    connect(pbBack, &QToolButton::clicked, this, &LC_WidgetOptionsDialog::onpbBackClicked);

    connect(pbAdvancedIcons, &QPushButton::clicked, this, &LC_WidgetOptionsDialog::showAdvancedSetup);

    LC_GROUP("Widgets");{
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

        bool leftToolbarFlatIcons = LC_GET_BOOL("LeftToolbarFlatIcons", true);
        cbLeftTBFlatButtons->setChecked(leftToolbarFlatIcons);

        int leftToolbarIconSize = LC_GET_INT("LeftToolbarIconSize", 24);
        sbLeftTBIconSize->setValue(leftToolbarIconSize);

        bool dockWidgetsFlatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        cbDockWidgetsFlatButtons->setChecked(dockWidgetsFlatIcons);

        int docWidgetsIconSize = LC_GET_INT("DockWidgetsIconSize", 16);
        sbDocWidgtetIconSize->setValue(docWidgetsIconSize);
    }
    LC_GROUP_END();

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);

    statusbar_height_spinbox->setEnabled(useClassicalStatusBar);
    statusbar_height_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_spinbox->setEnabled(useClassicalStatusBar);

    iconColorsOptions.loadSettings();
    iconColorsOptions.mark();

    QString iconsOverrideDir = iconColorsOptions.getIconsOverridesDir();
    leIconsOverrideDir->setText(iconsOverrideDir);

    updateUIByOptions();
    connect(cbIconColorMain->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onMainIconColorChanged);
    connect(cbIconColorAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onAccentIconColorChanged);
    connect(cbIconColorBack->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onBackIconColorChanged);

    connect(tbOverridesDir, &QToolButton::clicked, this, &LC_WidgetOptionsDialog::setIconsOverrideFoler);

//    lClassicStatusBarOnly->setVisible(!useClassicalStatusBar);
}

void LC_WidgetOptionsDialog::setIconsOverrideFoler() {
    QString folder = selectFolder(tr("Select Shortcuts Mappings Folder"));
    if (folder != nullptr) {
        leIconsOverrideDir->setText(QDir::toNativeSeparators(folder));
    }
}

QString LC_WidgetOptionsDialog::selectFolder(QString title) {
    QString folder = nullptr;
    QFileDialog dlg(this);
    if (title != nullptr) {
        QString dlgTitle = title;
        dlg.setWindowTitle(dlgTitle);
    }
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);

    if (dlg.exec()) {
        folder = dlg.selectedFiles()[0];
    }
    return folder;
}

void LC_WidgetOptionsDialog::updateUIByOptions(){
    QString colorMain = iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main);
    QString colorAccent = iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent);
    QString colorBack = iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background);

    cbIconColorMain->setCurrentText(colorMain);
    cbIconColorAccent->setCurrentText(colorAccent);
    cbIconColorBack->setCurrentText(colorBack);
}

void LC_WidgetOptionsDialog::onpbMainClicked() {
    set_color(cbIconColorMain);
}

void LC_WidgetOptionsDialog::onpbAccentClicked() {
    set_color(cbIconColorAccent);
}

void LC_WidgetOptionsDialog::onpbBackClicked() {
    set_color(cbIconColorBack);
}

void LC_WidgetOptionsDialog::onMainIconColorChanged(const QString &value){
    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_WidgetOptionsDialog::onAccentIconColorChanged(const QString &value){
    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_WidgetOptionsDialog::onBackIconColorChanged(const QString &value){
    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_WidgetOptionsDialog::set_color(QComboBox *combo) {
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    // dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, tr("Select Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        combo->lineEdit()->setText(color.name());
    }
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

        LC_SET("LeftToolbarFlatIcons", cbLeftTBFlatButtons->isChecked());
        LC_SET("LeftToolbarIconSize", sbLeftTBIconSize->value());

        LC_SET("DockWidgetsFlatIcons", cbDockWidgetsFlatButtons->isChecked());
        LC_SET("DockWidgetsIconSize", sbDocWidgtetIconSize->value());
    }

    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, cbIconColorMain->currentText());
    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, cbIconColorAccent->currentText());
    iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, cbIconColorBack->currentText());

    QString iconsOverrideDir = leIconsOverrideDir->text();
    iconColorsOptions.setIconsOverridesDir(iconsOverrideDir);

    /*LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_MAIN,-1,-1, "#000");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_ACCENT,-1,-1, "#c7c7c7");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,-1,-1, "#e2e2e2");

    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Active,QIcon::State::On, "#fff");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Active,QIcon::State::Off, "#000");

    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Normal,QIcon::State::On, "#f00");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Normal,QIcon::State::Off, "#ff0");

    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Selected,QIcon::State::On, "#0f0");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Selected,QIcon::State::Off, "#0ff");

    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Disabled,QIcon::State::On, "#00f");
    LC_SVGIconEngineAPI::setColorAppProperty(LC_SVGIconEngineAPI::KEY_COLOR_BG,QIcon::Mode::Disabled,QIcon::State::Off, "#f0f");*/

    applyIconColors();
    iconColorsOptions.save();

    if (iconColorsOptions.isIconOverridesChanged()) {
        QMessageBox::warning(this, tr("Preferences"),
                             tr("Icons overrides directory changed. Please restart the application to apply."));
    }

    LC_Dialog::accept();
}

void LC_WidgetOptionsDialog::reject(){
    iconColorsOptions.restore();
    applyIconColors();
    LC_Dialog::reject();
}

void LC_WidgetOptionsDialog::showAdvancedSetup(){
    LC_DlgIconsSetup dlg(this);
    LC_IconColorsOptions copy = LC_IconColorsOptions(iconColorsOptions);
    dlg.setIconsOptions(&copy);
    if (dlg.exec() == QDialog::Accepted){
        iconColorsOptions.apply(copy);
        updateUIByOptions();
    }
}

/**
 * NOTE: This method properly called only on closing of the dialog. Calling it when modal dialog is open, does lead to clearing pixmap cached and invalidation
 * of icons (and so re-expanding templates in icon engine) at least under Windows. Don't have idea why it's so...
 */

void LC_WidgetOptionsDialog::applyIconColors(){
    iconColorsOptions.applyOptions();
    QPixmapCache::clear();
    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    if (appWindow != nullptr) {
        appWindow->fireIconsRefresh();
    }
    appWindow->update();
    appWindow->repaint();
    QApplication::processEvents();
}

void LC_WidgetOptionsDialog::chooseStyleSheet(){
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty()){
        stylesheet_field->setText(QDir::toNativeSeparators(path));
    }
}
