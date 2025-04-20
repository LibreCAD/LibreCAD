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

#include "lc_penpaletteoptionsdialog.h"

#include <QColorDialog>
#include <QLineEdit>
#include <QMessageBox>

#include "lc_dialog.h"
#include "lc_peninforegistry.h"
#include "lc_penpaletteoptions.h"


class LC_PenPaletteOptions;

LC_PenPaletteOptionsDialog::LC_PenPaletteOptionsDialog(QWidget *parent, LC_PenPaletteOptions* options, [[maybe_unused]]bool focusOnFile) :
    LC_Dialog(parent, "PenPaletteOptions"), m_options{options}{
    setupUi(this);

    chkShowColorIcon->setChecked(m_options->showColorIcon);
    chkShowColorName->setChecked(m_options->showColorName);
    chkShowLineTypeIcon->setChecked(m_options->showTypeIcon);
    chkShowLineTypeName->setChecked(m_options->showTypeName);
    chkShowWidthIcon->setChecked(m_options->showWidthIcon);
    chkShowWidthName->setChecked(m_options->showWidthName);
    cbShowMessageForNoSelection->setChecked(m_options->showNoSelectionMessage);
    cbFilterCaseInsensitive->setChecked(m_options->ignoreCaseOnMatch);

    cbDoubleClickMode->addItem(tr("Do nothing"));
    cbDoubleClickMode->addItem(tr("Select entities by attributes pen"));
    cbDoubleClickMode->addItem(tr("Select entities by drawing pen"));

    cbDoubleClickMode->setCurrentIndex(m_options->doubleClickOnTableMode);

    cbShowTooltip->setChecked(m_options->showToolTip);
    cbAllRowBold->setChecked(m_options->showEntireRowBold);

    int colorMode = m_options->colorNameDisplayMode;
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

    connect(tbActiveColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectActivePenBGColor);
    connect(tbGridColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectGridColor);
    connect(tbMatchedItemColorSelect, &QToolButton::clicked, this, &LC_PenPaletteOptionsDialog::selectMatchedItemColor);

    initComboBox(cbColorActiveBg, m_options->activeItemBGColor);
    initComboBox(cbColorGrid, m_options->itemsGridColor);
    initComboBox(cbColorMatchedItem, m_options->matchedItemColor);

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
    set_color(cbColorActiveBg, m_options->activeItemBGColor);
}

void LC_PenPaletteOptionsDialog::selectGridColor(){
    set_color(cbColorGrid, m_options->itemsGridColor);
}

void LC_PenPaletteOptionsDialog::selectMatchedItemColor(){
    set_color(cbColorMatchedItem, m_options->matchedItemColor);
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

    // all fine, store user's input to options
    if (doAccept){
        m_options->matchedItemColor = matchedItemColor;
        m_options->itemsGridColor =  gridColor;
        m_options->activeItemBGColor = activeBgColor;

        m_options->showEntireRowBold = allRowBold;
        m_options->showToolTip = showToolTip;
        m_options->showWidthIcon = showLineWidthIcon;
        m_options->showWidthName = showLineWidthName;
        m_options->showTypeIcon = showLineTypeIcon;
        m_options->showTypeName = showLineTypeName;
        m_options->showColorIcon = showColorIcon;
        m_options->showColorName = showColorName;
        m_options->ignoreCaseOnMatch = ignoreCaseOnMatch;
        m_options->showNoSelectionMessage = showNoSelectionMessage;

        m_options->colorNameDisplayMode = colorMode;
        m_options->doubleClickOnTableMode = doubleClickMode;
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
