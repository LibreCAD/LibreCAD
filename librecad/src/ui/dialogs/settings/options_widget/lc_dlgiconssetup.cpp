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
#include <QPixmapCache>

#include "lc_iconcolorsoptions.h"
#include "qc_applicationwindow.h"
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


    connect(ui->cbGenericMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onGenericMainColorChanged);
    connect(ui->cbGenericAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onGenericAccentColorChanged);
    connect(ui->cbBackGeneric->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onGenericBackColorChanged);

    connect(ui->cbActiveOnMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOnMainColorChanged);
    connect(ui->cbActiveOnAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOnAccentColorChanged);
    connect(ui->cbActiveOnBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOnBackColorChanged);
    connect(ui->cbActiveOffMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOffMainColorChanged);
    connect(ui->cbActiveOffAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOffAccentColorChanged);
    connect(ui->cbActiveOffBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onActiveOffBackColorChanged);

    connect(ui->cbNormalOnMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOnMainColorChanged);
    connect(ui->cbNormalOnAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOnAccentColorChanged);
    connect(ui->cbNormalOnBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOnBackColorChanged);
    connect(ui->cbNormalOffMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOffMainColorChanged);
    connect(ui->cbNormalOffAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOffAccentColorChanged);
    connect(ui->cbNormalOffBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onNormalOffBackColorChanged);

    connect(ui->cbDisabledOnMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOnMainColorChanged);
    connect(ui->cbDisabledOnAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOnAccentColorChanged);
    connect(ui->cbDisabledOnBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOnBackColorChanged);
    connect(ui->cbDisabledOffMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOffMainColorChanged);
    connect(ui->cbDisabledOffAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOffAccentColorChanged);
    connect(ui->cbDisabledOffBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onDisabledOffBackColorChanged);

    connect(ui->cbSelectedOnMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOnMainColorChanged);
    connect(ui->cbSelectedOnAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOnAccentColorChanged);
    connect(ui->cbSelectedOnBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOnBackColorChanged);
    connect(ui->cbSelectedOffMain->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOffMainColorChanged);
    connect(ui->cbSelectedOffAccent->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOffAccentColorChanged);
    connect(ui->cbSelectedOffBack->lineEdit(), &QLineEdit::textEdited, this, &LC_DlgIconsSetup::onSelectedOffBackColorChanged);

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
    m_iconColorsOptions = options;

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
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, ui->cbGenericMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, ui->cbGenericAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, ui->cbBackGeneric);

    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbActiveOnMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbActiveOffMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbActiveOnAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbActiveOffAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbActiveOnBack);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbActiveOffBack);

    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbNormalOnMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbNormalOffMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbNormalOnAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbNormalOffAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbNormalOnBack);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbNormalOffBack);

    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbSelectedOnMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbSelectedOffMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOnAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbSelectedOffAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbSelectedOnBack);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbSelectedOffBack);

    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, ui->cbDisabledOnMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, ui->cbDisabledOffMain);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOnAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, ui->cbDisabledOffAccent);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, ui->cbDisabledOnBack);
    saveColor(m_iconColorsOptions, LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, ui->cbDisabledOffBack);
    LC_Dialog::accept();
}

void LC_DlgIconsSetup::onPbGenericMainClicked(){
    QString colorName = set_color(ui->cbGenericMain);
    if (!colorName.isEmpty()) {
        onGenericMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbGenericAccentClicked(){
    QString colorName = set_color(ui->cbGenericAccent);
    if (!colorName.isEmpty()) {
        onGenericAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbGenericBackClicked(){
    QString colorName = set_color(ui->cbBackGeneric);
    if (!colorName.isEmpty()) {
        onGenericBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOnMainClicked(){
    QString colorName = set_color(ui->cbActiveOnMain);
    if (!colorName.isEmpty()) {
        onActiveOnMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOnAccentClicked(){
    QString colorName = set_color(ui->cbActiveOnAccent);
    if (!colorName.isEmpty()) {
        onActiveOnAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOnBackClicked(){
    QString colorName = set_color(ui->cbActiveOnBack);
    if (!colorName.isEmpty()) {
        onActiveOnBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOffMainClicked(){
    QString colorName = set_color(ui->cbActiveOffMain);
    if (!colorName.isEmpty()) {
        onActiveOffMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOffAccentClicked(){
    QString colorName = set_color(ui->cbActiveOffAccent);
    if (!colorName.isEmpty()) {
        onActiveOffAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbActiveOffBackClicked(){
    QString colorName = set_color(ui->cbActiveOffBack);
    if (!colorName.isEmpty()) {
        onActiveOffBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOnMainClicked(){
    QString colorName = set_color(ui->cbNormalOnMain);
    if (!colorName.isEmpty()) {
        onNormalOnMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOnAccentClicked(){
    QString colorName = set_color(ui->cbNormalOnAccent);
    if (!colorName.isEmpty()) {
        onNormalOnAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOnBackClicked(){
    QString colorName = set_color(ui->cbNormalOnBack);
    if (!colorName.isEmpty()) {
        onNormalOnBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOffMainClicked(){
    QString colorName = set_color(ui->cbNormalOffMain);
    if (!colorName.isEmpty()) {
        onNormalOffMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOffAccentClicked(){
    QString colorName = set_color(ui->cbNormalOffAccent);
    if (!colorName.isEmpty()) {
        onNormalOffAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbNormalOffBackClicked(){
    QString colorName = set_color(ui->cbNormalOffBack);
    if (!colorName.isEmpty()) {
        onNormalOffBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOnMainClicked(){
    QString colorName = set_color(ui->cbSelectedOnMain);
    if (!colorName.isEmpty()) {
        onSelectedOnMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOnAccentClicked(){
    QString colorName = set_color(ui->cbSelectedOnAccent);
    if (!colorName.isEmpty()) {
        onSelectedOnAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOnBackClicked(){
    QString colorName = set_color(ui->cbSelectedOnBack);
    if (!colorName.isEmpty()) {
        onSelectedOnBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOffMainClicked(){
    QString colorName = set_color(ui->cbSelectedOffMain);
    if (!colorName.isEmpty()) {
        onSelectedOffMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOffAccentClicked(){
    QString colorName = set_color(ui->cbSelectedOffAccent);
    if (!colorName.isEmpty()) {
        onSelectedOffAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbSelectedOffBackClicked(){
    QString colorName = set_color(ui->cbSelectedOffBack);
    if (!colorName.isEmpty()) {
        onSelectedOffBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOnMainClicked(){
    QString colorName = set_color(ui->cbDisabledOnMain);
    if (!colorName.isEmpty()) {
        onDisabledOnMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOnAccentClicked(){
    QString colorName = set_color(ui->cbDisabledOnAccent);
    if (!colorName.isEmpty()) {
        onDisabledOnAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOnBackClicked(){
    QString colorName = set_color(ui->cbDisabledOnBack);
    if (!colorName.isEmpty()) {
        onDisabledOnBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOffMainClicked(){
    QString colorName = set_color(ui->cbDisabledOffMain);
    if (!colorName.isEmpty()) {
        onDisabledOffMainColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOffAccentClicked(){
    QString colorName = set_color(ui->cbDisabledOffAccent);
    if (!colorName.isEmpty()) {
        onDisabledOffAccentColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onPbDisabledOffBackClicked(){
    QString colorName = set_color(ui->cbDisabledOffBack);
    if (!colorName.isEmpty()) {
        onDisabledOffBackColorChanged(colorName);
    }
}

void LC_DlgIconsSetup::onGenericMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onGenericAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onGenericBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOnMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOnAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOnBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOffMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOffAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onActiveOffBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOnMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOnAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOnBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOffMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOffAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onNormalOffBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOnMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOnAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOnBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOffMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOffAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onSelectedOffBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOnMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOnAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOnBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOffMainColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Main, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOffAccentColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Accent, value);
    applyIconColors();
}

void LC_DlgIconsSetup::onDisabledOffBackColorChanged(const QString &value){
    m_iconColorsOptions->setColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::Background, value);
    applyIconColors();
}


QString LC_DlgIconsSetup::set_color(QComboBox *combo) {
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

void LC_DlgIconsSetup::resetToDefaults(){
    m_iconColorsOptions->resetToDefaults();
    setIconsOptions(m_iconColorsOptions);
}

void LC_DlgIconsSetup::applyIconColors(){
    m_iconColorsOptions->applyOptions();
    QPixmapCache::clear();
    auto& appWindow = QC_ApplicationWindow::getAppWindow(); //      fixme - sand - files - remove static
    if (appWindow != nullptr) {
        appWindow->fireIconsRefresh();
    }
    appWindow->update();
    appWindow->repaint();
    QApplication::processEvents();
}
