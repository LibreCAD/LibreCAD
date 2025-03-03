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
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include "lc_penpaletteoptionsdialog.h"

LC_PenPaletteOptionsDialog::LC_PenPaletteOptionsDialog(QWidget *parent, LC_PenPaletteOptions* options, bool focusOnFile) :
    QDialog(parent)
{
    this->options = options;

    setupUi(this);

    chkShowColorIcon->setChecked(options->showColorIcon);
    chkShowColorName->setChecked(options->showColorName);
    chkShowLineTypeIcon->setChecked(options->showTypeIcon);
    chkShowLineTypeName->setChecked(options->showTypeName);
    chkShowWidthIcon->setChecked(options->showWidthIcon);
    chkShowWidthName->setChecked(options->showWidthName);
    cbShowMessageForNoSelection->setChecked(options->showNoSelectionMessage);
    cbFilterCaseInsensitive->setChecked(options->ignoreCaseOnMatch);

    cbDoubleClickMode->addItem(tr("Do nothing"));
    cbDoubleClickMode->addItem(tr("Select entities by attributes pen"));
    cbDoubleClickMode->addItem(tr("Select entities by drawing pen"));

    cbDoubleClickMode->setCurrentIndex(options->doubleClickOnTableMode);

    cbShowTooltip->setChecked(options->showToolTip);
    cbAllRowBold->setChecked(options->showEntireRowBold);

    int colorMode = options->colorNameDisplayMode;
    switch (colorMode){
        case LC_PenInfoRegistry::ColorNameDisplayMode::RGB:
            rbRGB->setChecked(true);
            break;
        case LC_PenInfoRegistry::ColorNameDisplayMode::HEX:
            rbHEX->setChecked(true);
            break;
        case LC_PenInfoRegistry::ColorNameDisplayMode::NATURAL:
            rbNatural->setChecked(true);
            break;
        default:
            break;
    }



    lePensFile->setText(options->pensFileName);
    if (focusOnFile){
        lePensFile->setFocus();
    }
    connect(tbActiveColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectActivePenBGColor);
    connect(tbGridColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectGridColor);
    connect(tbMatchedItemColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectMatchedItemColor);

    connect(tbBrowse, &QToolButton::clicked,
            this, &LC_PenPaletteOptionsDialog::setPensFile);

    initComboBox(cbColorActiveBg, options->activeItemBGColor);
    initComboBox(cbColorGrid, options->itemsGridColor);
    initComboBox(cbColorMatchedItem, options->matchedItemColor);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &LC_PenPaletteOptionsDialog::validate);
}

LC_PenPaletteOptionsDialog::~LC_PenPaletteOptionsDialog()= default;

void LC_PenPaletteOptionsDialog::languageChange(){
    retranslateUi(this);
}

/**
 * Color combobox initialization
 * @param cb
 * @param color
 */
void LC_PenPaletteOptionsDialog::initComboBox(QComboBox* cb, const QColor &color){
    QString text = color.name();
    int idx = cb->findText(text);
    if (idx < 0){
        idx = 0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex(idx);
}

void LC_PenPaletteOptionsDialog::selectActivePenBGColor(){
    set_color(cbColorActiveBg, options->activeItemBGColor);
}

void LC_PenPaletteOptionsDialog::selectGridColor(){
    set_color(cbColorGrid, options->itemsGridColor);
}

void LC_PenPaletteOptionsDialog::selectMatchedItemColor(){
    set_color(cbColorMatchedItem, options->matchedItemColor);
}

/**
 * Color selection
 * @param combo
 * @param custom
 */
void LC_PenPaletteOptionsDialog::set_color(QComboBox* combo, QColor &custom)
{
    auto current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog::setCustomColor(0, custom.rgb());

    QColor color = QColorDialog::getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid()){
        combo->lineEdit()->setText(color.name());
    }
}

/**
 * Pens file selection
 */
void LC_PenPaletteOptionsDialog::setPensFile()
{
    QString fileName = lePensFile ->text();

    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setNameFilter("Pens Palette (*.pens)");
    dlg.selectFile(fileName);

    if (dlg.exec()){
        auto dir = dlg.selectedFiles()[0];
        lePensFile->setText(QDir::toNativeSeparators(dir));
    }
}
/**
 * Validation of entered data on button closing
 */
void LC_PenPaletteOptionsDialog::validate(){
    bool doAccept = true;
    
    bool showToolTip = cbShowTooltip->isChecked();
    bool showColorIcon = chkShowColorIcon->isChecked();
    bool showColorName = chkShowColorName->isChecked();
    bool showLineTypeIcon = chkShowLineTypeIcon->isChecked();
    bool showLineTypeName = chkShowLineTypeName->isChecked();
    bool showLineWidthIcon = chkShowWidthIcon->isChecked();
    bool showLineWidthName = chkShowWidthName->isChecked();
    bool allRowBold = cbAllRowBold->isChecked();
    bool showNoSelectionMessage = cbShowMessageForNoSelection->isChecked();
    bool ignoreCaseOnMatch = cbFilterCaseInsensitive->isChecked();

    int colorMode = 0;

    if (rbRGB->isChecked()){
        colorMode = LC_PenInfoRegistry::ColorNameDisplayMode::RGB;
    } else if (rbHEX->isChecked()){
        colorMode = LC_PenInfoRegistry::ColorNameDisplayMode::HEX;
    } else if (rbNatural->isChecked()){
      colorMode = LC_PenInfoRegistry::ColorNameDisplayMode::NATURAL;
    }

    int doubleClickMode = cbDoubleClickMode->currentIndex();

    QString activeBgColorName = cbColorActiveBg->currentText();
    QColor activeBgColor = QColor(activeBgColorName);
    if (!activeBgColor.isValid()){
        showInvalidColorMessage("active row background");
        cbColorActiveBg ->setFocus();
        doAccept = false;
    }

    QString gridColorName = cbColorGrid->currentText();
    QColor gridColor = QColor(gridColorName);
    if (!gridColor.isValid()){
        showInvalidColorMessage("grid");
        cbColorGrid ->setFocus();
        doAccept = false;
    }

    QString matchedColorName = cbColorMatchedItem->currentText();
    QColor matchedItemColor = QColor(matchedColorName);
    if (!gridColor.isValid()){
        showInvalidColorMessage("filter matched item");
        cbColorMatchedItem ->setFocus();
        doAccept = false;
    }

    QString fileName = lePensFile->text().trimmed();
    if (fileName.isEmpty()){
        showInvalidFileMessage("Name may not be empty.");
        lePensFile->setFocus();
        doAccept = false;
    }
    else{
        // check whether we are able to open specified file
        QFile file(fileName);
        if (!file.exists()){
            if (file.open(QIODevice::ReadWrite)){
                file.close();
            }
            else{
                showInvalidFileMessage("Unable to create pens palette file by given path.");
                doAccept = false;
                lePensFile->setFocus();
            }
        }
    }
    // all fine, store user's input to options
    if (doAccept){

        options->matchedItemColor = matchedItemColor;
        options->itemsGridColor =  gridColor;
        options->activeItemBGColor = activeBgColor;

        options->showEntireRowBold = allRowBold;
        options->showToolTip = showToolTip;
        options->showWidthIcon = showLineWidthIcon;
        options->showWidthName = showLineWidthName;
        options->showTypeIcon = showLineTypeIcon;
        options->showTypeName = showLineTypeName;
        options->showColorIcon = showColorIcon;
        options->showColorName = showColorName;
        options->ignoreCaseOnMatch = ignoreCaseOnMatch;
        options->showNoSelectionMessage = showNoSelectionMessage;

        options->colorNameDisplayMode = colorMode;
        options->doubleClickOnTableMode = doubleClickMode;
        options -> pensFileName = fileName;

        accept();
    }
}

/**
 * report invalid color (if value was entered manually directly in combobox)
 * @param name
 */
void LC_PenPaletteOptionsDialog::showInvalidColorMessage(const QString &name){
    QMessageBox::warning(this, QMessageBox::tr("Error"),
                         QMessageBox::tr("Invalid value provided for %1 color.\n"
                                         "Please specify a different value.")
                             .arg(QMessageBox::tr(name.toStdString().c_str())),
                         QMessageBox::Ok);
}
/**
 * report invalid file specified for pens
 * @param msg
 */
void LC_PenPaletteOptionsDialog::showInvalidFileMessage(const QString &msg){
    QMessageBox::warning(this, QMessageBox::tr("Error"),
                         QMessageBox::tr("Invalid path to pens file.\n%1 \n"
                                         "Please specify a different value.")
                             .arg(QMessageBox::tr(msg.toStdString().c_str())),
                         QMessageBox::Ok);
}

