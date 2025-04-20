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

#include <QtWidgets>
#include "lc_layertreeoptionsdialog.h"
#include "lc_dialog.h"
#include "lc_layertreemodel_options.h"
#include <ui_lc_layertreeoptionsdialog.h>

LC_LayerTreeOptionsDialog::LC_LayerTreeOptionsDialog(QWidget *parent, LC_LayerTreeModelOptions *ops) :
    ::LC_Dialog(parent, "LayerTreeOptions")
{
    setupUi(this);
    m_options = ops;
    init();
}

void LC_LayerTreeOptionsDialog::init(){
   leLayerLevelSeparator->setText(m_options->layerLevelSeparator);
   leDimSuffix->setText(m_options->dimensionalLayerNameSuffix);
   leInfoSuffix->setText(m_options->informationalLayerNameSuffix);
   leAltPosSuffix->setText(m_options->alternatePositionLayerNameSuffix);

   cbDragDrop->setChecked(m_options->dragDropEnabled);
   cbShowIdented->setChecked(m_options->showIndentedName);
   cbShowToolTip->setChecked(m_options->showToolTips);
   cbRenameWithPrimary->setChecked(m_options->renameSecondaryLayersOnPrimaryRename);

   cbShowTypeIcons ->setChecked(!m_options->hideLayerTypeIcons);

   leDuplicatedPrefix->setText(m_options->copiedNamePathPrefix);
   leDuplicatedSuffix->setText(m_options->copiedNamePathSuffix);

   QColor itemsGrid = m_options->itemsGridColor;
   QColor virtualLayerBgColor = m_options->virtualLayerBgColor;
   QColor matchHighlightColor = m_options->matchedItemColor;
   QColor selectedItemBgColor = m_options->selectedItemBgColor;
   QColor activeLayerBgColor = m_options->activeLayerBgColor;


   initComboBox(cbHighlightedColor, matchHighlightColor);
   initComboBox(cbGridColor, itemsGrid);
   initComboBox(cbVirtualLayerBackgroundColor, virtualLayerBgColor);
   initComboBox(cbSelectedItemBgColor, selectedItemBgColor);
   initComboBox(cbActiveLayerBgColor, activeLayerBgColor);

   sbIndentSize ->setValue(m_options->identSize);

   wPenNormal->setPen(m_options->defaultPenNormal, false, false, tr("Normal Layer"));
   wPenDimensional -> setPen(m_options->defaultPenDimensional, false, false,tr("Dimensional Layer"));
   wPenInfo -> setPen(m_options->defaultPenInformational, false, false, tr("Informational Layer"));
   wPenAltPos -> setPen(m_options->defaultPenAlternatePosition, false, false, tr("Alternative Position Layer"));

   tabWidget-> setCurrentIndex(0);
}

void LC_LayerTreeOptionsDialog::languageChange(){
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

    auto itemsGridColor = QColor(itemsGridColorName);
    if (!itemsGridColor.isValid()){
        showInvalidColorMessage(tr("grid"));
        cbGridColor ->setFocus();
        doAccept = false;
    }
    QString higlightedColorName = cbHighlightedColor->currentText();

    auto highlightedColor = QColor(higlightedColorName);
    if (!highlightedColor.isValid()){
        showInvalidColorMessage(tr("highlighted item"));
        cbHighlightedColor ->setFocus();
        doAccept = false;
    }
    QString virtualLayerBgColorName = cbVirtualLayerBackgroundColor->currentText();
    auto virtualLayerBgColor = QColor(virtualLayerBgColorName);
    if (!virtualLayerBgColor.isValid()){
        showInvalidColorMessage(tr("virtual layer background"));
        cbVirtualLayerBackgroundColor ->setFocus();
        doAccept = false;
    }

    QString selectedItemBgColorName = cbSelectedItemBgColor->currentText();
    auto selectedItemBgColor = QColor(selectedItemBgColorName);
    if (!selectedItemBgColor.isValid()){
        showInvalidColorMessage(tr("selected item background"));
        cbSelectedItemBgColor->setFocus();
        doAccept = false;
    }

    QString activeLayerBgColorName = cbActiveLayerBgColor->currentText();
    auto activeLayerBgColor = QColor(activeLayerBgColorName);
    if (!activeLayerBgColor.isValid()){
        showInvalidColorMessage(tr("active layer background"));
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

    m_options->layerLevelSeparator = layerListSeparator;
    m_options->dimensionalLayerNameSuffix = dimSuffix;
    m_options->informationalLayerNameSuffix = infoSuffix;
    m_options->alternatePositionLayerNameSuffix = altPosSuffix;
    m_options->showToolTips = showToolTip;
    m_options->showIndentedName = showIndented;
    m_options->dragDropEnabled = allowDragDrop;
    m_options->renameSecondaryLayersOnPrimaryRename = renameWitPrimary;
    m_options->hideLayerTypeIcons = !showTypeIcons;
    m_options->itemsGridColor = itemsGridColor;
    m_options->matchedItemColor = highlightedColor;
    m_options->virtualLayerBgColor = virtualLayerBgColor;
    m_options->activeLayerBgColor= activeLayerBgColor;
    m_options->selectedItemBgColor = selectedItemBgColor;
    m_options->identSize = indentSize;

    m_options->copiedNamePathPrefix= duplicatePrefix;
    m_options->copiedNamePathSuffix = duplicatedSuffix;

    m_options->defaultPenNormal = wPenNormal->getPen();
    m_options->defaultPenDimensional= wPenDimensional->getPen();
    m_options->defaultPenInformational = wPenInfo->getPen();
    m_options->defaultPenAlternatePosition = wPenAltPos->getPen();

    if (doAccept){
        accept();
    }
}

void LC_LayerTreeOptionsDialog::showInvalidColorMessage(const QString& name){
    QMessageBox::warning(this, tr("Error"),
                             tr("Invalid value provide for %1 color.\n"
                                             "Please specify a different value.").arg(name),QMessageBox::Ok);
}

void LC_LayerTreeOptionsDialog::pb_highlightedColorClicked(){
    set_color(cbHighlightedColor, m_options->matchedItemColor);
}
void LC_LayerTreeOptionsDialog::pb_gridColorClicked(){
    set_color(cbGridColor, m_options->itemsGridColor);
}
void LC_LayerTreeOptionsDialog::pb_selectedItemColorClicked(){
    set_color(cbVirtualLayerBackgroundColor, m_options->itemsGridColor);
}

void LC_LayerTreeOptionsDialog::pbSelectedItemsBgColorClicked(){
    set_color(cbSelectedItemBgColor, m_options->selectedItemBgColor);
}

void LC_LayerTreeOptionsDialog::pbActiveLayerBgColorClicked(){
    set_color(cbActiveLayerBgColor, m_options->activeLayerBgColor);
}

void LC_LayerTreeOptionsDialog::showIndentedClicked() const {
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

void LC_LayerTreeOptionsDialog::set_color(QComboBox* combo, QColor custom){
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
