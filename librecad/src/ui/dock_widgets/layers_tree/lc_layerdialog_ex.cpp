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

#include "lc_layerdialog_ex.h"

#include <QMessageBox>

#include "lc_layertreeitem.h"
#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

LC_LayerDialogEx::LC_LayerDialogEx(QWidget* parent, const QString& name, LC_LayerTreeModel* model, LC_LayerTreeItem *treeItem, RS_LayerList* layerList)
      : LC_Dialog(parent, "LayersTreeLayerDialog"), m_mode{-1}{
    setupUi(this);
    setModal(true);
    setObjectName(name);

    m_layerTreeModel = model;
    m_layerList = layerList;
    m_editedTreeItem = treeItem;
}

void LC_LayerDialogEx::languageChange(){
    retranslateUi(this);
}

void LC_LayerDialogEx::setMode(const int viewMode){
    const int mode = viewMode;
    this->setProperty("mode", mode);
    leParentPath->setEnabled(false);
    switch (mode){
        case MODE_RENAME_VIRTUAL:{
            leName->setFocus();
            wPen->setVisible(false);
            cbConstructionLayer ->setVisible(false);
            gbLayerType->setVisible(false);
            allowChangingLayerType(false);
            setWindowTitle(tr("Rename Virtual Layer"));
            break;
        }
        case MODE_ADD_SECONDARY_LAYER:{
            setWindowTitle(tr("Add Secondary Layer"));
            leName->setFocus();
            allowChangingLayerType(false);
          break;
        }
        case MODE_ADD_CHILD_LAYER:{
            setWindowTitle(tr("Add Layer"));
            leParentPath->setEnabled(false);
            leName->setFocus();
            allowChangingLayerType(true);
            break;
        }
        case MODE_ADD_LAYER:{
            setWindowTitle(tr("Add Layer"));
            leName->setFocus();
            allowChangingLayerType(true);
            break;
        }
        case MODE_EDIT_LAYER:{
            setWindowTitle(tr("Edit Layer"));
            if (m_editedTreeItem->isZero()){
                allowChangingLayerType(false);
                leName->setDisabled(true);
            }
            else {
                leName->setFocus();
                allowChangingLayerType(true);
            }
            break;
        }
        default:
            break;
    }
}

void LC_LayerDialogEx::allowChangingLayerType(const bool value){
    gbLayerType->setEnabled(value);
    if (value){
        connect(rbNormal, &QRadioButton::clicked, this, &LC_LayerDialogEx::layerTypeChanged);
        connect(rbDimensions, &QRadioButton::clicked, this, &LC_LayerDialogEx::layerTypeChanged);
        connect(rbAlternativePosition, &QRadioButton::clicked, this, &LC_LayerDialogEx::layerTypeChanged);
        connect(rbInformational, &QRadioButton::clicked, this, &LC_LayerDialogEx::layerTypeChanged);
    }
}

void LC_LayerDialogEx::setLayerType(const int type) const {
    switch (type){
        case RS_Layer::LayerType::VIRTUAL: {
            gbLayerType->setVisible(false);
            break;
        }
        case RS_Layer::NOT_DEFINED_LAYER_TYPE:
        case RS_Layer::LayerType::NORMAL: {
            gbLayerType->setVisible(true);
            rbNormal->setChecked(true);
            break;
        }
        case RS_Layer::LayerType::DIMENSIONAL: {
            gbLayerType->setVisible(true);
            rbDimensions->setChecked(true);
            break;
        }
        case RS_Layer::LayerType::ALTERNATE_POSITION: {
            gbLayerType->setVisible(true);
            rbAlternativePosition->setChecked(true);
            break;
        }
        case RS_Layer::LayerType::INFORMATIONAL: {
            gbLayerType->setVisible(true);
            rbInformational->setChecked(true);
            break;
        }
        default:
            break;
    }
}

int LC_LayerDialogEx::getEditedLayerType() const {
    int result = RS_Layer::NOT_DEFINED_LAYER_TYPE;
    if (rbNormal->isChecked()){
        result = RS_Layer::LayerType::NORMAL;
    }
    else if (rbDimensions->isChecked()){
        result = RS_Layer::LayerType::DIMENSIONAL;
    }
    else if (rbAlternativePosition->isChecked()){
        result = RS_Layer::LayerType::ALTERNATE_POSITION;
    }
    else if (rbInformational->isChecked()){
        result = RS_Layer::LayerType::INFORMATIONAL;
    }
    return result;
}

void LC_LayerDialogEx::setLayerName(const QString& name) const {
    leName ->setText(name);
}

void LC_LayerDialogEx::setParentPath(const QString &name) const {
    leParentPath -> setText(name);
}

void LC_LayerDialogEx::disableNames() const {
    leName ->setEnabled(false);
    leParentPath ->setVisible(false);
    lParentPathName->setVisible(false);
}

void LC_LayerDialogEx::setLayer(const RS_Layer* layer) const {
    if (layer != nullptr){
        wPen->setVisible(true);
        wPen->setPen(layer->getPen(), false, false, tr("Default Pen"));
        cbConstructionLayer->setChecked(layer->isConstruction());
        cbConstructionLayer->setVisible(true);
    }
    else{
        wPen ->setVisible(false);
        cbConstructionLayer ->setVisible(false);
    }
}

RS_Pen LC_LayerDialogEx::getPen() const {
    return wPen->getPen();
}
void LC_LayerDialogEx::init(){
    leName->setFocus();
    wPen->setVisible(false);
    cbConstructionLayer->setVisible(false);
    setWindowTitle(tr("Rename Layer"));
}

void LC_LayerDialogEx::layerTypeChanged() const {
    int layerType = -1;
    if (rbNormal->isChecked()){
        layerType = RS_Layer::LayerType::NORMAL;
    }
    else if (rbDimensions ->isChecked()){
        layerType = RS_Layer::LayerType::DIMENSIONAL;
    }
    else if (rbAlternativePosition -> isChecked()){
        layerType = RS_Layer::LayerType::ALTERNATE_POSITION;
    }
    else if (rbInformational->isChecked()){
        layerType = RS_Layer::LayerType::INFORMATIONAL;
    }

    if (layerType > 0){
        RS_Pen defaultPen = m_layerTreeModel ->getOptions()->getDefaultPen(layerType);
        const auto penCopy  = RS_Pen(defaultPen);
        wPen->setPen(penCopy, false, false, tr("Default Pen"));
    }
}

void LC_LayerDialogEx::validate() {
    QString layerName = leName->text();
    if (layerName.trimmed().isEmpty()){
        QMessageBox::information(parentWidget(),
                                 QMessageBox::tr("Layer Properties"),
                                 QMessageBox::tr("Layer empty name is not allowed."),
                                 QMessageBox::Ok);
    }
    else{
        const int newLayerType = getEditedLayerType();
        QStringList newLayerNamesList;

        const int dialogMode = property("mode").toInt();
        switch (dialogMode) {
            case MODE_ADD_LAYER: {
                const QString newLayerName = m_layerTreeModel->createFullLayerName(
                    m_editedTreeItem, layerName, newLayerType, true);
                newLayerNamesList << newLayerName;
                break;
            }
            case MODE_ADD_CHILD_LAYER:
            case MODE_ADD_SECONDARY_LAYER: {
                const QString newLayerName = m_layerTreeModel->createFullLayerName(
                    m_editedTreeItem, layerName, newLayerType, true);
                newLayerNamesList << newLayerName;
                break;
            }
            case MODE_RENAME_VIRTUAL: {
                const QString oldLayerName = m_editedTreeItem->getName();
                if (layerName != oldLayerName) {
                    newLayerNamesList = m_layerTreeModel->getLayersListForRenamedVirtualLayer(m_editedTreeItem, layerName);
                }
                break;
            }
            case MODE_EDIT_LAYER: {
                const QString originalName = m_editedTreeItem->getName();
                const int originalLayerType = m_editedTreeItem->getLayerType();
                const bool typeChanged = newLayerType != originalLayerType;
                const bool nameChanged = originalName != layerName;
                if (nameChanged || typeChanged) {
                    // inner name should be changed - so we have to check for possible duplicates.
                    newLayerNamesList = m_layerTreeModel->getLayersListForRenamedPrimary(
                        m_editedTreeItem, layerName, newLayerType);
                }
                break;
            }
            default:
                break;
        }

        if (newLayerNamesList.isEmpty()){
          accept();
        }
        else{
            const bool duplicateNameFound = checkForDuplicatedNames(newLayerNamesList);
            if (!duplicateNameFound){
                accept();
            }
        }
    }
}

bool LC_LayerDialogEx::checkForDuplicatedNames(const QStringList &newLayerNamesList) const {
    const int count = newLayerNamesList.size();
    bool duplicateNameFound = false;
    for (int i = 0; i < count; i++) {
        const QString& candidateLayerName = newLayerNamesList.at(i);
        // here we check against layer list and not against model, since some filtering may be applied
        const RS_Layer* existingLayer = m_layerList->find(candidateLayerName);
        if (existingLayer != nullptr){
            QMessageBox::information(parentWidget(),
                                     QMessageBox::tr("Layer Properties"),
                                     QMessageBox::tr("Attempt to create layer with duplicating name. Duplicated layer name is \n[%1].\n"
                                                     "Please specify a different name.")
                                     .arg(candidateLayerName),
                                     QMessageBox::Ok);
            leName->setFocus();
            leName->selectAll();
            duplicateNameFound = true;
            break;
        }
    }
    return duplicateNameFound;
}

QString LC_LayerDialogEx::getLayerName() const {
    return leName->text();
}
