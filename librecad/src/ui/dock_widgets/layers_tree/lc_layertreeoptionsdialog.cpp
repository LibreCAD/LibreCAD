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

#include "lc_layertreeoptionsdialog.h"
#include "ui_lc_layertreeoptionsdialog.h"
#include "lc_layertreemodel_options.h"

#include <QColorDialog>
#include <QtWidgets>

LC_LayerTreeOptionsDialog::LC_LayerTreeOptionsDialog(QWidget *parent, LC_LayerTreeModelOptions *ops) :
    QDialog(parent)
{
    setupUi(this);
    options = ops;
    init();
}

void LC_LayerTreeOptionsDialog::init(){
   leLayerLevelSeparator->setText(options->layerLevelSeparator);
   leDimSuffix->setText(options->dimensionalLayerNameSuffix);
   leInfoSuffix->setText(options->informationalLayerNameSuffix);
   leAltPosSuffix->setText(options->alternatePositionLayerNameSuffix);

   cbDragDrop->setChecked(options->dragDropEnabled);
   cbShowIdented->setChecked(options->showIndentedName);
   cbShowToolTip->setChecked(options->showToolTips);
   cbRenameWithPrimary->setChecked(options->renameSecondaryLayersOnPrimaryRename);

   cbShowTypeIcons ->setChecked(!options->hideLayerTypeIcons);

   leDuplicatedPrefix->setText(options->copiedNamePathPrefix);
   leDuplicatedSuffix->setText(options->copiedNamePathSuffix);

   QColor itemsGrid = options->itemsGridColor;
   QColor virtualLayerBgColor = options->virtualLayerBgColor;
   QColor matchHighlightColor = options->matchedItemColor;
   QColor selectedItemBgColor = options->selectedItemBgColor;
   QColor activeLayerBgColor = options->activeLayerBgColor;


   initComboBox(cbHighlightedColor, matchHighlightColor);
   initComboBox(cbGridColor, itemsGrid);
   initComboBox(cbVirtualLayerBackgroundColor, virtualLayerBgColor);
   initComboBox(cbSelectedItemBgColor, selectedItemBgColor);
   initComboBox(cbActiveLayerBgColor, activeLayerBgColor);

   sbIndentSize ->setValue(options->identSize);

   wPenNormal->setPen(options->defaultPenNormal, false, false, tr("Normal Layer"));
   wPenDimensional -> setPen(options->defaultPenDimensional, false, false,tr("Dimensional Layer"));
   wPenInfo -> setPen(options->defaultPenInformational, false, false, tr("Informational Layer"));
   wPenAltPos -> setPen(options->defaultPenAlternatePosition, false, false, tr("Alternative Position Layer"));

   tabWidget-> setCurrentIndex(0);
}

void LC_LayerTreeOptionsDialog::languageChange()
{
    retranslateUi(this);
}

void LC_LayerTreeOptionsDialog::validate(){
    bool doAccept = true;
    QString layerListSeparator = leLayerLevelSeparator->text();
    QString dimSuffix = leDimSuffix ->text();
    QString infoSuffix = leInfoSuffix->text();
    QString altPosSuffix = leAltPosSuffix ->text();

    QString duplicatePrefix = leDuplicatedPrefix->text();
    QString duplicatedSuffix = leDuplicatedSuffix->text();
    bool showTypeIcons = cbShowTypeIcons->isChecked();
    bool allowDragDrop = cbDragDrop->isChecked();
    bool showToolTip = cbShowToolTip->isChecked();
    bool showIndented = cbShowIdented->isChecked();
    bool renameWitPrimary = cbRenameWithPrimary->isChecked();

    int indentSize = sbIndentSize->value();

    QString itemsGridColorName = cbGridColor->currentText();

    QColor itemsGridColor = QColor(itemsGridColorName);
    if (!itemsGridColor.isValid()){
        showInvalidColorMessage("grid");
        cbGridColor ->setFocus();
        doAccept = false;
    }
    QString higlightedColorName = cbHighlightedColor->currentText();

    QColor highlightedColor = QColor(higlightedColorName);
    if (!highlightedColor.isValid()){
        showInvalidColorMessage("highlighted item");
        cbHighlightedColor ->setFocus();
        doAccept = false;
    }
    QString virtualLayerBgColorName = cbVirtualLayerBackgroundColor->currentText();
    QColor virtualLayerBgColor = QColor(virtualLayerBgColorName);
    if (!virtualLayerBgColor.isValid()){
        showInvalidColorMessage("virtual layer background");
        cbVirtualLayerBackgroundColor ->setFocus();
        doAccept = false;
    }

    QString selectedItemBgColorName = cbSelectedItemBgColor->currentText();
    QColor selectedItemBgColor = QColor(selectedItemBgColorName);
    if (!selectedItemBgColor.isValid()){
        showInvalidColorMessage("selected item background");
        cbSelectedItemBgColor->setFocus();
        doAccept = false;
    }

    QString activeLayerBgColorName = cbActiveLayerBgColor->currentText();
    QColor activeLayerBgColor = QColor(activeLayerBgColorName);
    if (!activeLayerBgColor.isValid()){
        showInvalidColorMessage("active layer background");
        cbActiveLayerBgColor->setFocus();
        doAccept = false;
    }


    if (duplicatePrefix.trimmed().isEmpty() && duplicatedSuffix.trimmed().isEmpty()){
        // TODO - think about this - whether it's allowed or not.
//        QMessageBox::warning(this, QMessageBox::tr("Error"),
//                                 QMessageBox::tr("Both prefix and suffix for duplicated name part are empty.\n"
//                                                 "This is incorrect, as it may lead to the layer"
//                                                 "Please specify a different value.")
//                                 .arg(name),
//                                 QMessageBox::Ok);
    }

    if (layerListSeparator.isEmpty()){
        QMessageBox::warning(this, QMessageBox::tr("Error"),
                                       QMessageBox::tr("Layer list separator string is empty. It will not be possible to build layers tree.\n"
                                                       "Please specify a different value."));
        leLayerLevelSeparator -> setFocus();
        doAccept = false;
    }

    options->layerLevelSeparator = layerListSeparator;
    options->dimensionalLayerNameSuffix = dimSuffix;
    options->informationalLayerNameSuffix = infoSuffix;
    options->alternatePositionLayerNameSuffix = altPosSuffix;
    options->showToolTips = showToolTip;
    options->showIndentedName = showIndented;
    options->dragDropEnabled = allowDragDrop;
    options->renameSecondaryLayersOnPrimaryRename = renameWitPrimary;
    options->hideLayerTypeIcons = !showTypeIcons;
    options->itemsGridColor = itemsGridColor;
    options->matchedItemColor = highlightedColor;
    options->virtualLayerBgColor = virtualLayerBgColor;
    options->activeLayerBgColor= activeLayerBgColor;
    options->selectedItemBgColor = selectedItemBgColor;
    options->identSize = indentSize;

    options->copiedNamePathPrefix= duplicatePrefix;
    options->copiedNamePathSuffix = duplicatedSuffix;

    options->defaultPenNormal = wPenNormal->getPen();
    options->defaultPenDimensional= wPenDimensional->getPen();
    options->defaultPenInformational = wPenInfo->getPen();
    options->defaultPenAlternatePosition = wPenAltPos->getPen();

    if (doAccept){
        accept();
    }
}

void LC_LayerTreeOptionsDialog::showInvalidColorMessage(QString name){
    QMessageBox::warning(this, QMessageBox::tr("Error"),
                             QMessageBox::tr("Invalid value provide for %1 color.\n"
                                             "Please specify a different value.")
                             .arg(QMessageBox::tr(name.toStdString().c_str())),
                             QMessageBox::Ok);
}

void LC_LayerTreeOptionsDialog::pb_highlightedColorClicked(){
    set_color(cbHighlightedColor, options->matchedItemColor);
}
void LC_LayerTreeOptionsDialog::pb_gridColorClicked(){
    set_color(cbGridColor, options->itemsGridColor);
}
void LC_LayerTreeOptionsDialog::pb_selectedItemColorClicked(){
    set_color(cbVirtualLayerBackgroundColor, options->itemsGridColor);
}

void LC_LayerTreeOptionsDialog::pbSelectedItemsBgColorClicked(){
    set_color(cbSelectedItemBgColor, options->selectedItemBgColor);
}

void LC_LayerTreeOptionsDialog::pbActiveLayerBgColorClicked(){
    set_color(cbActiveLayerBgColor, options->activeLayerBgColor);
}


void LC_LayerTreeOptionsDialog::showIndentedClicked(){
    sbIndentSize->setEnabled(cbShowIdented->isChecked());
}

void LC_LayerTreeOptionsDialog::initComboBox(QComboBox* cb, const QColor color) {
    QString text = color.name();
    int idx = cb->findText(text);
    if( idx < 0) {
        idx =0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex( idx );
}

void LC_LayerTreeOptionsDialog::set_color(QComboBox* combo, QColor custom)
{
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, tr("Select Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
        combo->lineEdit()->setText(color.name());
    }
}


LC_LayerTreeOptionsDialog::~LC_LayerTreeOptionsDialog(){}
