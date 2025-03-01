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

#include "lc_dlgiconssetup.h"

#include <QColorDialog>
#include <QLineEdit>

#include "lc_iconcolorsoptions.h"
#include "ui_lc_dlgiconssetup.h"

LC_DlgIconsSetup::LC_DlgIconsSetup(QWidget *parent)
    : LC_Dialog(parent, "IconsStyling")
    , ui(new Ui::LC_DlgIconsSetup){
    ui->setupUi(this);

    connect(ui->pbGenericMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbGenericMainClicked);
    connect(ui->pbGenericAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbGenericAccentClicked);
    connect(ui->pbGenericBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbGenericBackClicked);

    connect(ui->pbActiveOnMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOnMainClicked);
    connect(ui->pbActiveOnAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOnAccentClicked);
    connect(ui->pbActiveOnBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOnBackClicked);
    connect(ui->pbActiveOffMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOffMainClicked);
    connect(ui->pbActiveOffAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOffAccentClicked);
    connect(ui->pbActiveOffBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbActiveOffBackClicked);

    connect(ui->pbNormalOnMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOnMainClicked);
    connect(ui->pbNormalOnAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOnAccentClicked);
    connect(ui->pbNormalOnBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOnBackClicked);
    connect(ui->pbNormalOffMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOffMainClicked);
    connect(ui->pbNormalOffAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOffAccentClicked);
    connect(ui->pbNormalOffBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbNormalOffBackClicked);

    connect(ui->pbSelectedOnMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOnMainClicked);
    connect(ui->pbSelectedOnAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOnAccentClicked);
    connect(ui->pbSelectedOnBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOnBackClicked);
    connect(ui->pbSelectedOffMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOffMainClicked);
    connect(ui->pbSelectedOffAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOffAccentClicked);
    connect(ui->pbSelectedOffBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbSelectedOffBackClicked);

    connect(ui->pbDisabledOnMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOnMainClicked);
    connect(ui->pbDisabledOnAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOnAccentClicked);
    connect(ui->pbDisabledOnBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOnBackClicked);
    connect(ui->pbDisabledOffMain, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOffMainClicked);
    connect(ui->pbDisabledOffAccent, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOffAccentClicked);
    connect(ui->pbDisabledOffBack, &QPushButton::clicked, this, &LC_DlgIconsSetup::onPbDisabledOffBackClicked);

    connect(ui->pbResetToDefaults,&QPushButton::clicked, this, &LC_DlgIconsSetup::resetToDefaults);
}

LC_DlgIconsSetup::~LC_DlgIconsSetup(){
    delete ui;
}

void LC_DlgIconsSetup::initCombobox(LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QComboBox *ctrl){
    auto color = options->getColor(mode, state, type);
    ctrl->setCurrentText(color);
    initComboBox(ctrl,color);
}

void LC_DlgIconsSetup::saveColor(LC_IconColorsOptions *options, LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QComboBox *ctrl){
    auto color = ctrl->currentText();
    options->setColor(mode, state, type, color);
}

void LC_DlgIconsSetup::setIconsOptions(LC_IconColorsOptions *options){
    iconColorsOptions = options;

    initCombobox(options, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, ui->cbGenericMain);
    initCombobox(options, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, ui->cbGenericAccent);
    initCombobox(options, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, ui->cbBackGeneric);

    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbActiveOnMain);
    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbActiveOffMain);
    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbActiveOnAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbActiveOffAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbActiveOnBack);
    initCombobox(options, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbActiveOffBack);

    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbNormalOnMain);
    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbNormalOffMain);
    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbNormalOnAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbNormalOffAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbNormalOnBack);
    initCombobox(options, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbNormalOffBack);

    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbSelectedOnMain);
    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbSelectedOffMain);
    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOnAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOffAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbSelectedOnBack);
    initCombobox(options, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbSelectedOffBack);

    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbDisabledOnMain);
    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbDisabledOffMain);
    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOnAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOffAccent);
    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbDisabledOnBack);
    initCombobox(options, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbDisabledOffBack);
}

void LC_DlgIconsSetup::initComboBox(QComboBox *cb, const QString &text) {
    int idx = cb->findText(text);
    if (idx < 0) {
        idx = 0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex(idx);
}

void LC_DlgIconsSetup::accept(){
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, ui->cbGenericMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, ui->cbGenericAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, ui->cbBackGeneric);

    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbActiveOnMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbActiveOffMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbActiveOnAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbActiveOffAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbActiveOnBack);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbActiveOffBack);

    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbNormalOnMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbNormalOffMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbNormalOnAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbNormalOffAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbNormalOnBack);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbNormalOffBack);

    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbSelectedOnMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbSelectedOffMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOnAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOffAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbSelectedOnBack);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbSelectedOffBack);

    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbDisabledOnMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbDisabledOffMain);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOnAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOffAccent);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbDisabledOnBack);
    saveColor(iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbDisabledOffBack);
    LC_Dialog::accept();
}

void LC_DlgIconsSetup::onPbGenericMainClicked(){
    set_color(ui->cbGenericMain);
}

void LC_DlgIconsSetup::onPbGenericAccentClicked(){
    set_color(ui->cbGenericAccent);
}

void LC_DlgIconsSetup::onPbGenericBackClicked(){
    set_color(ui->cbBackGeneric);
}

void LC_DlgIconsSetup::onPbActiveOnMainClicked(){
    set_color(ui->cbActiveOnMain);
}

void LC_DlgIconsSetup::onPbActiveOnAccentClicked(){
    set_color(ui->cbActiveOnAccent);
}

void LC_DlgIconsSetup::onPbActiveOnBackClicked(){
    set_color(ui->cbActiveOnBack);
}

void LC_DlgIconsSetup::onPbActiveOffMainClicked(){
    set_color(ui->cbActiveOffMain);
}

void LC_DlgIconsSetup::onPbActiveOffAccentClicked(){
    set_color(ui->cbActiveOffAccent);
}

void LC_DlgIconsSetup::onPbActiveOffBackClicked(){
    set_color(ui->cbActiveOffBack);
}

void LC_DlgIconsSetup::onPbNormalOnMainClicked(){
    set_color(ui->cbNormalOnMain);
}

void LC_DlgIconsSetup::onPbNormalOnAccentClicked(){
    set_color(ui->cbNormalOnAccent);
}

void LC_DlgIconsSetup::onPbNormalOnBackClicked(){
    set_color(ui->cbNormalOnBack);
}

void LC_DlgIconsSetup::onPbNormalOffMainClicked(){
    set_color(ui->cbNormalOffMain);
}

void LC_DlgIconsSetup::onPbNormalOffAccentClicked(){
    set_color(ui->cbNormalOffAccent);
}

void LC_DlgIconsSetup::onPbNormalOffBackClicked(){
    set_color(ui->cbNormalOffBack);
}

void LC_DlgIconsSetup::onPbSelectedOnMainClicked(){
    set_color(ui->cbSelectedOnMain);
}

void LC_DlgIconsSetup::onPbSelectedOnAccentClicked(){
    set_color(ui->cbSelectedOnAccent);
}

void LC_DlgIconsSetup::onPbSelectedOnBackClicked(){
    set_color(ui->cbSelectedOnBack);
}

void LC_DlgIconsSetup::onPbSelectedOffMainClicked(){
    set_color(ui->cbSelectedOffMain);
}

void LC_DlgIconsSetup::onPbSelectedOffAccentClicked(){
    set_color(ui->cbSelectedOffAccent);
}

void LC_DlgIconsSetup::onPbSelectedOffBackClicked(){
    set_color(ui->cbSelectedOffBack);
}

void LC_DlgIconsSetup::onPbDisabledOnMainClicked(){
    set_color(ui->cbDisabledOnMain);
}

void LC_DlgIconsSetup::onPbDisabledOnAccentClicked(){
    set_color(ui->cbDisabledOnAccent);
}

void LC_DlgIconsSetup::onPbDisabledOnBackClicked(){
    set_color(ui->cbDisabledOnBack);
}

void LC_DlgIconsSetup::onPbDisabledOffMainClicked(){
    set_color(ui->cbDisabledOffMain);
}

void LC_DlgIconsSetup::onPbDisabledOffAccentClicked(){
    set_color(ui->cbDisabledOffAccent);
}

void LC_DlgIconsSetup::onPbDisabledOffBackClicked(){
    set_color(ui->cbDisabledOffBack);
}

void LC_DlgIconsSetup::set_color(QComboBox *combo) {
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    // dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, tr("Select Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        combo->lineEdit()->setText(color.name());
    }
}

void LC_DlgIconsSetup::resetToDefaults(){
    iconColorsOptions->resetToDefaults();
    setIconsOptions(iconColorsOptions);
}
