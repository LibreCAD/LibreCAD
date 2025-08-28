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

#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmapCache>
#include <QStatusBar>
#include <QStyleFactory>

#include "lc_dlgiconssetup.h"
#include "lc_inputtextdialog.h"
#include "lc_widgetfactory.h"
#include "qc_applicationwindow.h"
#include "rs_settings.h"

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

        // bool allow_theme = LC_GET_BOOL("AllowTheme", false);
        // theme_checkbox->setChecked(allow_theme);

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

        bool pickValuesButtonsFlatIcons = LC_GET_BOOL("PickValueButtonsFlatIcons", true);
        cbFlatPickValuesButtons->setChecked(pickValuesButtonsFlatIcons);

        int docWidgetsIconSize = LC_GET_INT("DockWidgetsIconSize", 16);
        sbDocWidgtetIconSize->setValue(docWidgetsIconSize);

        bool allowDockNesting = LC_GET_BOOL("DockAllowNested", true);
        cbDockingAllowNested->setChecked(allowDockNesting);

        bool titleBarVertical = LC_GET_BOOL("DockTitleBarVertical", false);
        cbDockingVerticalTitleBar->setChecked(titleBarVertical);

        bool verticalTabs = LC_GET_BOOL("DockVerticalTabs", false);
        cbDockingVerticalTabs->setChecked(verticalTabs);
    }
    LC_GROUP_END();

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);

    statusbar_height_spinbox->setEnabled(useClassicalStatusBar);
    statusbar_height_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_checkbox->setEnabled(useClassicalStatusBar);
    statusbar_fontsize_spinbox->setEnabled(useClassicalStatusBar);

    m_iconColorsOptions.loadSettings();
    m_iconColorsOptions.mark();

    QString iconsOverrideDir = m_iconColorsOptions.getIconsOverridesDir();
    leIconsOverrideDir->setText(iconsOverrideDir);

    updateUIByOptions();
    connect(cbIconColorMain->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onMainIconColorChanged);
    connect(cbIconColorAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onAccentIconColorChanged);
    connect(cbIconColorBack->lineEdit(), &QLineEdit::textEdited, this, &LC_WidgetOptionsDialog::onBackIconColorChanged);

    connect(tbOverridesDir, &QToolButton::clicked, this, &LC_WidgetOptionsDialog::setIconsOverrideFoler);

    QFile iconsDir(iconsOverrideDir);
    bool directoryExists = iconsDir.exists();

    bool readingStyleEnabled = directoryExists;
    bool writingStyleEnabled = directoryExists;

    // fixme - sand - check why here we have false?
    /*bool readingStyleEnabled = false;
    bool writingStyleEnabled = false;
    if (directoryExists){
        readingStyleEnabled = iconsDir.isReadable();
        writingStyleEnabled = iconsDir.isWritable();
    }*/

    lblStyle->setEnabled(readingStyleEnabled);
    cbIconsStyle->setEnabled(readingStyleEnabled);
    pbStyleSave->setEnabled(writingStyleEnabled);

    if (readingStyleEnabled){
        if (!setupStylesCombobox()){
            cbIconsStyle->setEnabled(false);
            pbRemoveStyle->setEnabled(false);
        }
        else{
            cbIconsStyle->insertItem(0,"");
            cbIconsStyle->blockSignals(true);
            cbIconsStyle->setCurrentIndex(0);
            cbIconsStyle->blockSignals(false);
            pbRemoveStyle->setEnabled(true);
        }
        connect(cbIconsStyle, &QComboBox::currentTextChanged, this, &LC_WidgetOptionsDialog::onStyleChanged);
    }

    if (writingStyleEnabled){
        connect(pbStyleSave, &QPushButton::clicked, this, &LC_WidgetOptionsDialog::onSaveStylePressed);
    }

    connect(pbRemoveStyle, &QPushButton::clicked, this, &LC_WidgetOptionsDialog::onRemoveStylePressed);
}

void LC_WidgetOptionsDialog::onStyleChanged(const QString & /*val*/){
    QString style = cbIconsStyle->currentText();
    if (!style.isEmpty()) {
        if (m_iconColorsOptions.loadFromFile(style)) {
            m_currentIconsStyleName = style;
            updateUIByOptions();
            applyIconColors();
        }
    }
}

bool LC_WidgetOptionsDialog::setupStylesCombobox() {
    QStringList existingStyles;
    m_iconColorsOptions.getAvailableStyles(existingStyles);
    if (!existingStyles.isEmpty()) {
        for (const auto& style:existingStyles){
          cbIconsStyle->addItem(style);
        }
        return true;
    }
    return false;
}

void LC_WidgetOptionsDialog::updateStylesCombobox(QStringList options){
    options.clear();
    m_iconColorsOptions.getAvailableStyles(options);
    pbRemoveStyle->setEnabled(!options.isEmpty());
    cbIconsStyle->clear();
    for (const auto& style:options){
        cbIconsStyle->addItem(style);
    }
}

void LC_WidgetOptionsDialog::onSaveStylePressed(){
    bool ok;
    QStringList options;
    m_iconColorsOptions.getAvailableStyles(options);
    auto styleName = LC_InputTextDialog::getText(this, tr("Save Icons Style"), tr("Enter name of icons style:"), options, true, m_currentIconsStyleName, &ok);
    if (ok){
        m_iconColorsOptions.saveToFile(styleName);
        updateStylesCombobox(options);
    }
}

void LC_WidgetOptionsDialog::onRemoveStylePressed(){
    bool ok;
    QStringList options;
    m_iconColorsOptions.getAvailableStyles(options);
    auto styleName = LC_InputTextDialog::getText(this, tr("Remove Icons Style"), tr("Select style to remove:"), options, false, m_currentIconsStyleName, &ok);
    if (ok) {
        if (m_iconColorsOptions.removeStyle(styleName)) {
            updateStylesCombobox(options);
        }
    }
}

void LC_WidgetOptionsDialog::setIconsOverrideFoler() {
    QString folder = selectFolder(tr("Select External Icons Folder"));
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
    QString colorMain = m_iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main);
    QString colorAccent = m_iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent);
    QString colorBack = m_iconColorsOptions.getColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background);

    cbIconColorMain->setCurrentText(colorMain);
    cbIconColorAccent->setCurrentText(colorAccent);
    cbIconColorBack->setCurrentText(colorBack);
}

void LC_WidgetOptionsDialog::onpbMainClicked() {
    QString colorName = set_color(cbIconColorMain);
    if (!colorName.isEmpty()) {
        onMainIconColorChanged(colorName);
    }
}

void LC_WidgetOptionsDialog::onpbAccentClicked() {
    QString colorName = set_color(cbIconColorAccent);
    if (!colorName.isEmpty()) {
        onAccentIconColorChanged(colorName);
    }
}

void LC_WidgetOptionsDialog::onpbBackClicked() {
    QString colorName = set_color(cbIconColorBack);
    if (!colorName.isEmpty()) {
        onBackIconColorChanged(colorName);
    }
}

void LC_WidgetOptionsDialog::onMainIconColorChanged(const QString &value){
    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_WidgetOptionsDialog::onAccentIconColorChanged(const QString &value){
    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_WidgetOptionsDialog::onBackIconColorChanged(const QString &value){
    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

QString LC_WidgetOptionsDialog::set_color(QComboBox *combo) {
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    // dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, tr("Select Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        auto colorName = color.name();
        combo->lineEdit()->setText(colorName);
        return colorName;
    }
    return "";
}

void LC_WidgetOptionsDialog::accept() {
    LC_GROUP_GUARD("Widgets");
    {
        bool allow_style = style_checkbox->isChecked();
        LC_SET("AllowStyle", allow_style);
        if (allow_style) {
            QString style = style_combobox->currentText();
            LC_SET("Style", style);
            QApplication::setStyle(QStyleFactory::create(style)); // fixme - sand - move to style helper?
        }

        auto& appWindow = QC_ApplicationWindow::getAppWindow(); // fixme - avoid static?

        QString sheet_path = stylesheet_field->text();
        LC_SET("StyleSheet", sheet_path);
        if (appWindow->loadStyleSheet(sheet_path)) {
           // nothing to do
        }

        bool pickValuesButtonsFlatIcons = cbFlatPickValuesButtons->isChecked();
        LC_SET("PickValueButtonsFlatIcons", pickValuesButtonsFlatIcons);

        bool allow_theme = false; //theme_checkbox->isChecked();
        LC_SET("AllowTheme", allow_theme);
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


        bool allowDockNesting =cbDockingAllowNested->isChecked();
        LC_SET("DockAllowNested", allowDockNesting);

        bool titleBarVertical = cbDockingVerticalTitleBar->isChecked();
        LC_SET("DockTitleBarVertical", titleBarVertical);

        bool verticalTabs = cbDockingVerticalTabs->isChecked();
        LC_SET("DockVerticalTabs", verticalTabs);

        LC_WidgetFactory::updateDockOptions(appWindow.get(), allowDockNesting, verticalTabs);
        LC_WidgetFactory::updateDockWidgetsTitleBarType(appWindow.get(), titleBarVertical);
    }

    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, cbIconColorMain->currentText());
    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, cbIconColorAccent->currentText());
    m_iconColorsOptions.setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, cbIconColorBack->currentText());

    QString iconsOverrideDir = leIconsOverrideDir->text();
    m_iconColorsOptions.setIconsOverridesDir(iconsOverrideDir);

    applyIconColors();
    m_iconColorsOptions.save();

    if (m_iconColorsOptions.isIconOverridesChanged()) {
        QMessageBox::warning(this, tr("Preferences"),
                             tr("Icons overrides directory changed. Please restart the application to apply."));
    }

    LC_Dialog::accept();
}

void LC_WidgetOptionsDialog::reject(){
    m_iconColorsOptions.restore();
    applyIconColors();
    LC_Dialog::reject();
}

void LC_WidgetOptionsDialog::showAdvancedSetup(){
    LC_DlgIconsSetup dlg(this);
    LC_IconColorsOptions copy = LC_IconColorsOptions(m_iconColorsOptions);
    dlg.setIconsOptions(&copy);
    if (dlg.exec() == QDialog::Accepted){
        m_iconColorsOptions.apply(copy);
        updateUIByOptions();
        applyIconColors();
    }
}

/**
 * NOTE: This method properly called only on closing of the dialog. Calling it when modal dialog is open, does lead to clearing pixmap cached and invalidation
 * of icons (and so re-expanding templates in icon engine) at least under Windows. Don't have idea why it's so...
 */

void LC_WidgetOptionsDialog::applyIconColors(){
    m_iconColorsOptions.applyOptions();
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
