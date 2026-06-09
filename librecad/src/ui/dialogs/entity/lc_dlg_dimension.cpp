/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_dlg_dimension.h"

#include <QMenu>
#include <QMessageBox>

#include "lc_dimordinate.h"
#include "lc_dimstylepreviewgraphicview.h"
#include "lc_dimstylepreviewpanel.h"
#include "lc_dimstylestreemodel.h"
#include "lc_dlgdimstylemanager.h"
#include "lc_inputtextdialog.h"
#include "rs_dimension.h"
#include "rs_dimlinear.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "ui_lc_dlg_dimension.h"

#define STYLES_IMPORT_NOT_SUPPORTED

LC_DlgDimension::LC_DlgDimension(QWidget* parent, LC_GraphicViewport* viewport, RS_Dimension* dim)
    : LC_EntityPropertiesDlg(parent, "DlgDimOrdinate", viewport)
    , ui(new Ui::LC_DlgDimension){
    ui->setupUi(this);
    setEntity(dim);
}

LC_DlgDimension::~LC_DlgDimension(){
    delete ui;
}

void LC_DlgDimension::saveDimensionTypeDependentProperties() const {
    const RS2::EntityType dimType = m_entity->rtti();
    switch (dimType){
        case RS2::EntityDimOrdinate: {
            auto* dimOrdinate = static_cast<LC_DimOrdinate*>(m_entity);
            dimOrdinate->setForXDirection(ui->rbOrdinateX->isChecked());
            break;
        }
        case RS2::EntityDimLinear: {
            const auto dimLinear = static_cast<RS_DimLinear*>(m_entity);
            const RS_DimLinear* original = dynamic_cast<RS_DimLinear*>(m_entity);
            if (original != nullptr) {
                dimLinear->setAngle(toWCSAngle(ui->leAngle, original->getAngle()));
            }
            else {
                dimLinear->setAngle(toWCSAngle(ui->leAngle, 0.0));
            }
        }
        default:
            break;
    }
}

void LC_DlgDimension::updateEntity() {
    m_entity->setLabel(ui->wLabel->getLabel());
    m_entity->setPen(ui->wPenEditor->getPen());
    m_entity->setLayer(ui->cbLayer->getLayer());

    saveDimensionTypeDependentProperties();
    saveDimensionStyles();

    m_entity->setFlipArrow1(ui->cbFlipArrow1->isChecked());
    m_entity->setFlipArrow2(ui->cbFlipArrow2->isChecked());
    m_entity->updateDim(true);
}

void LC_DlgDimension::saveDimensionStyles() {
    const auto model = getDimStylesModel();

    // setup dimension style of entity
    const auto entityStyleItem = model->getEntityStyleItem();
    if (entityStyleItem->isOverrideItem()) {
        // handle style override. Make a copy of style to simplify deletion of model.
        m_entity->setDimStyleOverride(entityStyleItem->dimStyle());
        const auto parentItem = entityStyleItem->parentItem();
        const auto parentStyle = parentItem->dimStyle();
        const QString styleName = parentStyle->getName();
        m_entity->setStyle(styleName);
    }
    else {
        // just set name of entity
        const QString styleName = entityStyleItem->dimStyle()->getName();
        m_entity->setStyle(styleName);
        m_entity->setDimStyleOverride(nullptr);
    }

    // process active dim style, if any
    const auto defaultStyleItem = model->getActiveStyleItem();
    const auto defaultStyleName = defaultStyleItem->baseName();
    m_graphic->setDefaultDimStyleName(defaultStyleName);

    // add created styles (overrides saved as new styles) to the graphic

    QList<LC_DimStyleItem*> unsavedItems;
    model->collectUnsavedItems(unsavedItems);

    for (const auto dsi: std::as_const(unsavedItems)) {
       const auto createdStyle = dsi->dimStyle();
       const auto createdStyleToAdd = createdStyle->getCopy();
       m_graphic->addDimStyle(createdStyleToAdd);
    }

    clearDimStylesModel(model);
}

void LC_DlgDimension::clearDimStylesModel(LC_DimStyleTreeModel* model) {
    // the model should always contain copies of dim styles.
    // thus we have to remove dim styles too.
    model->cleanup(true);
}

void LC_DlgDimension::setupDimensionTypeDependentUI(RS_Dimension* dim) {
    const RS2::EntityType dimType = dim->rtti();
    switch (dimType) {
        case RS2::EntityDimOrdinate: {
            setWindowTitle(tr("Ordinate Dimension"));

            const auto* dimOrdinate = static_cast<LC_DimOrdinate*>(dim);
            const bool ordinateX = dimOrdinate->isForXDirection();
            ui->bgOrdinateGeometry->setVisible(true);

            ui->rbOrdinateX->setChecked(ordinateX);
            ui->rbOrdinateY->setChecked(!ordinateX);

            ui->gbFlipArrows->setVisible(false);
            break;
        }
        case RS2::EntityDimLinear: {
            setWindowTitle(tr("Linear Dimension"));

            const auto dimLinear = static_cast<RS_DimLinear*>(dim);
            toUIAngleDeg(dimLinear->getAngle(), ui->leAngle);
            ui->bgLinearGeometry->setVisible(true);

            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setChecked(dim->isFlipArrow2());
            break;
        }
        case RS2::EntityDimAligned: {
            setWindowTitle(tr("Aligned Dimension"));
            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setChecked(dim->isFlipArrow2());
            break;
        }
        case RS2::EntityDimAngular: {
            setWindowTitle(tr("Angular Dimension"));
            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setChecked(dim->isFlipArrow2());
            break;
        }
        case RS2::EntityDimArc: {
            setWindowTitle(tr("Arc Dimension"));
            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setChecked(dim->isFlipArrow2());
            break;
        }
        case RS2::EntityDimDiametric: {
            setWindowTitle(tr("Diametric Dimension"));
            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setChecked(dim->isFlipArrow2());
            break;
        }
        case RS2::EntityDimRadial: {
            setWindowTitle(tr("Radial Dimension"));
            ui->cbFlipArrow1->setChecked(dim->isFlipArrow1());
            ui->cbFlipArrow2->setVisible(false);
            break;
        }
        default:
            break;
    }

    connect(ui->cbFlipArrow1, &QCheckBox::toggled, this,&LC_DlgDimension::onFlipArrowChanged);
    connect(ui->cbFlipArrow2, &QCheckBox::toggled, this,&LC_DlgDimension::onFlipArrowChanged);
}

void LC_DlgDimension::setupDimensionAttributesUI(const RS_Dimension* dim) {
    m_graphic = m_entity->getGraphic();
    if (m_graphic != nullptr) {
        ui->cbLayer->init(*m_graphic->getLayerList(), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay != nullptr) {
        ui->cbLayer->setLayer(*lay);
    }

    ui->wPenEditor->setPen(m_entity, lay, tr("Pen"));

    ui->wLabel->setRadialType(*dim);
    ui->wLabel->setLabel(m_entity->getLabel(false));

    ui->bgOrdinateGeometry->setVisible(false);
    ui->bgLinearGeometry->setVisible(false);

    connect(ui->wPenEditor, &QG_WidgetPen::penChanged, this,&LC_DlgDimension::onPenChanged);

    // fixme - sand - should we add preview modification for label change?
}

void LC_DlgDimension::onPenChanged() const {
    const RS_Pen pen = ui->wPenEditor->getPen();
    updateDimStylePreview(pen);
}

void LC_DlgDimension::onFlipArrowChanged([[maybe_unused]]bool val) const {
    updateDimStylePreview(ui->cbFlipArrow1->isChecked(), ui->cbFlipArrow2->isChecked());
}

void LC_DlgDimension::setEntity(RS_Dimension* dim) {
    m_entity = dim;
    setupDimensionAttributesUI(dim);
    setupDimensionTypeDependentUI(dim);
    const QModelIndex dimensionStyleItemIndex = setupStylesList();
    setupPreview();
    selectStyleItem(dimensionStyleItemIndex);
}

QModelIndex LC_DlgDimension::setupStylesList() {
    QList<LC_DimStyleItem*> items;
    prepareDimStyleItems(items);

    auto* model = new LC_DimStyleTreeModel(this, items, false);
    QString styleName = m_entity->getStyle();
    auto entityStyleItem = model->findByName(styleName);
    if (entityStyleItem == nullptr) {
        // try to handle the case, when base style name is set of the entity (on creation or editing). However, later, the type-specific style was added.
        // so even if such case, the item for the type-specific style should be returned.
        const RS2::EntityType entityType = adjustDimentityTypeForStyleName(m_entity->rtti());
        styleName = LC_DimStyle::getStyleNameForBaseAndType(styleName, entityType);
        entityStyleItem = model->findByName(styleName);
        if (entityStyleItem == nullptr) {
            ui->lvDimStyles->setModel(model);
            return QModelIndex();
        }
    }
    const int itemStyleRow = entityStyleItem->row();
    QModelIndex selectedItemIndex;

    const LC_DimStyleItem* actualEntityStyleItem = nullptr;

    const auto overrideStyle = m_entity->getDimStyleOverride();
    if (overrideStyle != nullptr) {
        // merge with initial style, so override item contains properties that were explicitly overriden,
        // and properties that are not included into override comes from the style that was overriden.
        const auto overrideCopy = overrideStyle->getCopy();
        overrideCopy->mergeWith(entityStyleItem->dimStyle(), LC_DimStyle::ModificationAware::UNSET, LC_DimStyle::ModificationAware::SET);

        auto* overrideItem = new LC_DimStyleItem(overrideCopy, 0, false);
        overrideItem->setOverrideItem(true);
        entityStyleItem->appendChild(overrideItem);
        overrideItem->setEntityStyleItem(true);
        const auto parentIndex = model->index(itemStyleRow, 0, QModelIndex());
        selectedItemIndex = model->index(0, 0, parentIndex);
        actualEntityStyleItem = overrideItem;
    }
    else {
        selectedItemIndex = model->index(itemStyleRow, 0, QModelIndex());
        entityStyleItem->setEntityStyleItem(true);
        actualEntityStyleItem = entityStyleItem;
    }

    ui->lvDimStyles->setModel(model);
    ui->lvDimStyles->setSelectionMode(QAbstractItemView::SingleSelection);

    updateActiveStyleInfoLabel();
    updateEntityStyleInfoLabels(actualEntityStyleItem);
    updateActionButtons(actualEntityStyleItem);

    connect(ui->lvDimStyles->selectionModel(), &QItemSelectionModel::currentChanged, this,
         &LC_DlgDimension::onDimCurrentChanged);

    connect(ui->tbSetStyle, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleSet);
    connect(ui->tbDimNew, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleOverrideNew);
    connect(ui->tbDimEdit, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleOverrideEdit);
    connect(ui->tbDimSaveAsStyle, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleOverrideSave);
    connect(ui->tbDimRemove, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleOverrideRemove);
    connect(ui->tbDimDefault, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleSetDefault);
    connect(ui->tbSelectEntityStyle, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleEntitySelect);

    const bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
    ui->tbSetStyle->setAutoRaise(autoRaiseButtons);
    ui->tbDimNew->setAutoRaise(autoRaiseButtons);
    ui->tbDimRemove->setAutoRaise(autoRaiseButtons);
    ui->tbDimEdit->setAutoRaise(autoRaiseButtons);
    ui->tbDimSaveAsStyle->setAutoRaise(autoRaiseButtons);
    ui->tbDimDefault->setAutoRaise(autoRaiseButtons);
    ui->tbSelectEntityStyle->setAutoRaise(autoRaiseButtons);

    // disable export/import buttons for later use
#ifdef STYLES_IMPORT_NOT_SUPPORTED
    ui->tbDimExport->setVisible(false);
    ui->tbDimImport->setVisible(false);
#else
    ui->tbDimExport->setAutoRaise(autoRaiseButtons);
    ui->tbDimImport->setAutoRaise(autoRaiseButtons);
    connect(ui->tbDimExport, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleExport);
    connect(ui->tbDimImport, &QToolButton::clicked, this, &LC_DlgDimension::onDimStyleImport);
#endif

    ui->lvDimStyles->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->lvDimStyles, &QTreeView::customContextMenuRequested, this,
          &LC_DlgDimension::onDimStylesListMenuRequested);
    connect(ui->lvDimStyles, &QTreeView::doubleClicked, this, &LC_DlgDimension::onDimStyleDoubleClick);

    expandStylesTree();

    return selectedItemIndex;
}

void LC_DlgDimension::getOverrideItemIndex(const LC_DimStyleTreeModel* model, const LC_DimStyleItem* entityStyleItem, QModelIndex& itemIndex) {
    const auto parentItem = entityStyleItem->parentItem();
    const int parentRow = parentItem->row();
    const auto parentIndex = model->index(parentRow, 0, QModelIndex());
    itemIndex = model->index(0, 0, parentIndex);
}

void LC_DlgDimension::onDimStyleEntitySelect([[maybe_unused]]bool val) {
    const auto model = getDimStylesModel();
    const auto entityStyleItem = model->getEntityStyleItem();
    QModelIndex itemIndex;
    if (entityStyleItem->isOverrideItem()) {
        getOverrideItemIndex(model, entityStyleItem, itemIndex);
    }
    else {
        itemIndex = model->index(entityStyleItem->row(), 0, QModelIndex());
    }
    selectStyleItem(itemIndex);
    ui->lvDimStyles->update();
}

void LC_DlgDimension::onDimStyleSet([[maybe_unused]]bool val)  {
    const QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        const auto model = getDimStylesModel();
        model->setEntityItem(selectedItemIndex);
        const LC_DimStyleItem* itemToSet = model->getItemForIndex(selectedItemIndex);
        updateEntityStyleInfoLabels(itemToSet);
    }
    expandStylesTree();
}

void LC_DlgDimension::onDimStyleSetDefault([[maybe_unused]]bool val)  {
    const QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        const auto model = getDimStylesModel();
        model->setActiveStyleItem(selectedItemIndex);
    }
    expandStylesTree();
    updateActiveStyleInfoLabel();
}

void LC_DlgDimension::onDimStyleOverrideRemove([[maybe_unused]]bool val)  {
    const QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        const auto model = getDimStylesModel();
        LC_DimStyleItem* itemToRemove = model->getItemForIndex(selectedItemIndex);
        if (itemToRemove->isOverrideItem()) {
            LC_DimStyleItem* parentItem = itemToRemove->parentItem();
            if (itemToRemove->isEntityStyleItem()) {
                parentItem->setEntityStyleItem(true);
            }
            parentItem->removeChild(itemToRemove);
            const auto dimStyle = itemToRemove->dimStyle();
            delete dimStyle; // this is a copy of actual override, so delete it too
            delete itemToRemove;

            const LC_DimStyleItem* entityItem = model->getEntityStyleItem();
            updateDimStylePreview(entityItem->dimStyle(), model, entityItem->isOverrideItem(), entityItem->baseName());
            updateActionButtons(entityItem);
            updateEntityStyleInfoLabels(entityItem);
            model->emitDataChanged();
        }
        expandStylesTree();
    }
}

void LC_DlgDimension::onDimStyleOverrideSave([[maybe_unused]]bool val) {
    const QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        const auto model = getDimStylesModel();
        LC_DimStyleItem* itemToSave = model->getItemForIndex(selectedItemIndex);
        if (itemToSave->isOverrideItem()) {
            QString nameCandidate;
            bool styleNameNotUnique {true};
            do {
                bool ok = false;
                nameCandidate = LC_InputTextDialog::getText(this, tr("Saving Style Override"), tr("New Style Name"), {}, true, "", &ok);
                nameCandidate = nameCandidate.trimmed();
                if (ok) {
                    const auto foundStyle = model->findByName(nameCandidate);
                    styleNameNotUnique = foundStyle != nullptr;
                    if (styleNameNotUnique) {
                        QMessageBox::critical ( this, tr("Saving Style Override"), tr("Style with such name already exists, please enter another unique one.") );
                    }
                }
                else {
                    return;
                }
            }
            while (styleNameNotUnique);

            // do actual save there

            LC_DimStyleItem* parent = itemToSave->parentItem();
            parent->removeChild(itemToSave);

            itemToSave->setOverrideItem(false);

            itemToSave->dimStyle()->setName(nameCandidate);
            itemToSave->updateNameAndType();
            model->addItem(itemToSave);

            model->setEntityItem(itemToSave);
            model->emitDataChanged();
            updateEntityStyleInfoLabels(itemToSave);

            const auto parentIndex = model->index(itemToSave->row(), 0, QModelIndex());
            selectStyleItem(parentIndex);

            itemToSave->setUnsaved(true);
            expandStylesTree();
        }
    }
}

void LC_DlgDimension::onDimStyleOverrideNew([[maybe_unused]]bool val) {
    const QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        const auto model = getDimStylesModel();
        LC_DimStyleItem* itemToOverride = model->getItemForIndex(selectedItemIndex);
        if (!itemToOverride->isOverrideItem()) {
            const auto styleToOverride = itemToOverride->dimStyle();
            LC_DimStyle *newOverrideStyle = styleToOverride->getCopy();
            newOverrideStyle->resetFlags(true);

            const QString styleName = styleToOverride->getName();

            QApplication::setOverrideCursor(Qt::WaitCursor);
            LC_DlgDimStyleManager dimStyleManager(this, newOverrideStyle, m_graphic, m_entity, styleName);
            dimStyleManager.refreshPreview();
            dimStyleManager.setWindowTitle(tr("New Dimension Style Override - ") + itemToOverride->displayName());
            QApplication::restoreOverrideCursor();
            if (dimStyleManager.exec() == Accepted) {
                const auto item = new LC_DimStyleItem(newOverrideStyle, 0, false);
                item->setOverrideItem(true);
                item->setUnsaved(true);
                itemToOverride->appendChild(item);
                model->setEntityItem(item);
                model->emitDataChanged();
                updateEntityStyleInfoLabels(item);

                QModelIndex itemIndex;
                getOverrideItemIndex(model, item, itemIndex);
                selectStyleItem(itemIndex);
                expandStylesTree();
                ui->lvDimStyles->update();
            }
            else {
              delete newOverrideStyle;
            }
        }
    }
}

void LC_DlgDimension::onDimStyleOverrideEdit([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        auto model = getDimStylesModel();
        LC_DimStyleItem* itemToEdit = model->getItemForIndex(selectedItemIndex);
        auto originalStyleToEdit = itemToEdit->dimStyle();

        RS2::EntityType dimType = itemToEdit->forDimensionType();

        if (itemToEdit->isOverrideItem()) {
            QString baseName = itemToEdit->baseName();
            LC_DimStyle* styleCopyToEdit = originalStyleToEdit->getCopy();
            LC_DimStyleItem* baseItem = itemToEdit->parentItem();
            QString baseStyleName = baseItem->dimStyle()->getName();

            QApplication::setOverrideCursor(Qt::WaitCursor);
            LC_DlgDimStyleManager dimStyleManager(this, styleCopyToEdit, m_graphic, m_entity, baseStyleName);
            QList<LC_DimStyleItem*> itemsMatchedStyle;
            model->collectItemsForBaseStyleName(baseName, &itemsMatchedStyle);
            for (auto dsi: std::as_const(itemsMatchedStyle)) {
                auto styleToAdd = dsi->dimStyle();
                if (itemToEdit != dsi){
                    dimStyleManager.addDimStyle(styleToAdd);
                }
            }
            dimStyleManager.refreshPreview();
            dimStyleManager.setWindowTitle(tr("Edit Dimension Style Override - ") +
                    LC_DimStyleItem::getDisplayDimStyleName(originalStyleToEdit));
            QApplication::restoreOverrideCursor();
            if (dimStyleManager.exec() == Accepted) {
                styleCopyToEdit->copyTo(originalStyleToEdit);
                updateDimStylePreview(originalStyleToEdit, model, true, baseStyleName);
            }
            delete styleCopyToEdit;
        }
        else { // just display style setting in read only mode
            QApplication::setOverrideCursor(Qt::WaitCursor);
            LC_DlgDimStyleManager dimStyleManager(this, originalStyleToEdit, m_graphic, dimType);
            dimStyleManager.setWindowTitle(tr("View Dimension Style - ") +
                    LC_DimStyleItem::getDisplayDimStyleName(originalStyleToEdit));
            QString baseName = itemToEdit->baseName();
            QList<LC_DimStyleItem*> itemsMatchedStyle;
            model->collectItemsForBaseStyleName(baseName, &itemsMatchedStyle);
            for (auto dsi: std::as_const(itemsMatchedStyle)) {
                auto styleToAdd = dsi->dimStyle();
                if (itemToEdit != dsi){
                    dimStyleManager.addDimStyle(styleToAdd);
                }
            }
            dimStyleManager.refreshPreview();
            dimStyleManager.setReadOnly();
            QApplication::restoreOverrideCursor();
            dimStyleManager.exec();
        }
    }
}

void LC_DlgDimension::onDimStyleDoubleClick() {
    onDimStyleOverrideEdit(false);
}

void LC_DlgDimension::onDimStyleExport([[maybe_unused]]bool val) {
    // fixme - support later
}

void LC_DlgDimension::onDimStyleImport([[maybe_unused]]bool val) {
    // fixme - support later
}

void LC_DlgDimension::onDimStylesListMenuRequested(const QPoint& pos) {
    auto* contextMenu = new QMenu(this);
    // auto* caption = new QLabel(tr("Dim Styles Menu"), this);
    /*QPalette palette;
    palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
    palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
    caption->setPalette(palette);
    caption->setAlignment(Qt::AlignCenter);*/

    const QModelIndex index = ui->lvDimStyles->indexAt(pos);
    using ActionMemberFunc = void (LC_DlgDimension::*)(bool);
    const auto addActionFunc = [this, &contextMenu](const QString& iconName, const QString& name, ActionMemberFunc func)
    {
        contextMenu->addAction(QIcon(":/icons/" + iconName + ".lci"), name, this, func);
    };

    if (index.isValid()) {
        const auto* model = getDimStylesModel();
        const LC_DimStyleItem* item = model->getItemForIndex(index);
        const bool normalStyle = !item->isOverrideItem();

        if (!item->isEntityStyleItem()) {
            addActionFunc("dim_apply_style", tr("&Apply Style"), &LC_DlgDimension::onDimStyleSet);
            addActionFunc("nview_visible", tr("&Select Style"), &LC_DlgDimension::onDimStyleSet);
        }

        if (!item->isActive() && normalStyle) {
            contextMenu->addSeparator();
            addActionFunc("dim_default", tr("&Set as Active"), &LC_DlgDimension::onDimStyleSetDefault);
        }
        contextMenu->addSeparator();
        if (normalStyle && item->childCount() == 0) {
            addActionFunc("add", tr("&Create override"), &LC_DlgDimension::onDimStyleOverrideNew);
        }

        if (normalStyle) {
            addActionFunc("dim_style_manager", tr("&View Style"), &LC_DlgDimension::onDimStyleOverrideEdit);
        }
        else{
            addActionFunc("attributes", tr("&Edit override"), &LC_DlgDimension::onDimStyleOverrideEdit);
            addActionFunc("save_as", tr("&Save As Style"), &LC_DlgDimension::onDimStyleOverrideSave);
            addActionFunc("remove", tr("&Remove Override"), &LC_DlgDimension::onDimStyleOverrideRemove);
        }
#ifndef STYLES_IMPORT_NOT_SUPPORTED
        contextMenu->addSeparator();
        addActionFunc("export", tr("E&xport Styles"), &LC_DlgDimension::onDimStyleExport);
        addActionFunc("import", tr("&Import Styles"), &LC_DlgDimension::onDimStyleImport);
#endif
    }
    else {
        addActionFunc("nview_visible", tr("&Select Style"), &LC_DlgDimension::onDimStyleSet);
        // addActionFunc("add", tr("&Create Override"), &LC_DlgDimension::onDimStyleOverrideNew);
#ifndef STYLES_IMPORT_NOT_SUPPORTED
        addActionFunc("import", tr("&Import Styles"), &LC_DlgDimension::onDimStyleImport);
#endif
    }
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void LC_DlgDimension::onDimCurrentChanged(const QModelIndex &current, [[maybe_unused]]const QModelIndex &previous) const {
    if (current.isValid()) {
        const auto model = getDimStylesModel();
        const LC_DimStyleItem* item = model->getItemForIndex(current);
        updateActionButtons(item);
        const bool override = item->isOverrideItem();
        const auto dimStyle = item->dimStyle();
        if (override) {
            const auto parentItem = item->parentItem();
            if (parentItem != nullptr) {
                updateDimStylePreview(dimStyle, model, override, parentItem->dimStyle()->getName());
            }
            else {
                // ugly fall-back that should actually not happen
                updateDimStylePreview(dimStyle, model, override, item->baseName());
            }
        }
        else {
            updateDimStylePreview(dimStyle, model, override, item->baseName());
        }
    }
}

void LC_DlgDimension::selectStyleItem(const QModelIndex& indexToSelect) const {
    if (indexToSelect.isValid()) {
        ui->lvDimStyles->selectionModel()->setCurrentIndex(indexToSelect, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->lvDimStyles->scrollTo(indexToSelect); // Ensure visibility
    }
}

void LC_DlgDimension::updateActiveStyleInfoLabel() const {
    const auto model = getDimStylesModel();
    const auto defaultStyle = model->getActiveStyleItem();
    QString currentStyleName = "";
    if (defaultStyle != nullptr) {
        currentStyleName = defaultStyle->baseName();
    }
    ui->lblDefaultStyle->setText(currentStyleName);
}

void LC_DlgDimension::updateEntityStyleInfoLabels(const LC_DimStyleItem* item) const {
    QString styleName;
    if (item->isOverrideItem()) {
        QFont font = ui->lblStyleOverrideState->font();
        font.setBold(true);
        ui->lblStyleOverrideState->setFont(font);
        ui->lblStyleOverrideState->setText(tr("YES"));
        styleName = item->parentItem()->baseName();
    }
    else {
        QFont font = ui->lblStyleOverrideState->font();
        font.setBold(false);
        ui->lblStyleOverrideState->setFont(font);
        ui->lblStyleOverrideState->setText(tr("No"));
        styleName = item->baseName();
    }
    ui->lblEntityStyle->setText(styleName);
}

void LC_DlgDimension::updateActionButtons(const LC_DimStyleItem* item) const {
    const bool overrideItem = item->isOverrideItem();
    const bool is_active = item->isActive();
    ui->tbDimDefault->setEnabled(!is_active && !overrideItem);
    ui->tbDimSaveAsStyle->setEnabled(overrideItem);
    ui->tbDimEdit->setEnabled(overrideItem);
    ui->tbDimRemove->setEnabled(overrideItem);
    ui->tbDimNew->setEnabled(!overrideItem && item->childCount() == 0);
    const bool notEntityStyleItems = !item->isEntityStyleItem();
    ui->tbSetStyle->setEnabled(notEntityStyleItems);
    ui->tbSelectEntityStyle->setEnabled(notEntityStyleItems);
}

QModelIndex LC_DlgDimension::getSelectedDimStyleIndex() const {
    return ui->lvDimStyles->selectionModel()->currentIndex();
}

void LC_DlgDimension::updateDimStylePreview(LC_DimStyle* dimStyle,[[maybe_unused]] LC_DimStyleTreeModel* model, const bool override, const QString& baseName) const {
    m_previewView->setEntityDimStyle(dimStyle, override, baseName);
    m_previewView->updateDims();
    m_previewView->zoomAuto();
    m_previewView->refresh();
}

void LC_DlgDimension::updateDimStylePreview(const RS_Pen& pen) const {
    m_previewView->setEntityPen(pen);
    m_previewView->updateDims();
    m_previewView->zoomAuto();
    m_previewView->refresh();
}

void LC_DlgDimension::updateDimStylePreview(const bool flipArrow1, const bool flipArrow2) const {
    m_previewView->setEntityArrowsFlipMode(flipArrow1, flipArrow2);
    m_previewView->updateDims();
    m_previewView->zoomAuto();
    m_previewView->refresh();
}

LC_DimStyleTreeModel* LC_DlgDimension::getDimStylesModel() const {
    return dynamic_cast<LC_DimStyleTreeModel*>(ui->lvDimStyles->model());
}

bool LC_DlgDimension::isDimensionTypeMatchedToStyleType(const RS2::EntityType styleType, const RS2::EntityType dimensionType) {
    bool accept = false;
    switch (styleType) {
        case RS2::EntityDimLinear:
        case RS2::EntityDimAligned: {
            accept = dimensionType == RS2::EntityDimLinear || dimensionType == RS2::EntityDimAligned;
            break;
        }
        case RS2::EntityDimAngular: {
            accept = dimensionType == RS2::EntityDimAngular;
            break;
        }
        case RS2::EntityDimDiametric: {
            accept = dimensionType == RS2::EntityDimDiametric;
            break;
        }
        case RS2::EntityDimRadial: {
            accept = dimensionType == RS2::EntityDimRadial;
            break;
        }
        case RS2::EntityDimOrdinate:{
            accept = dimensionType == RS2::EntityDimOrdinate;
            break;
        }
        case RS2::EntityTolerance:
        case RS2::EntityDimLeader:{
            accept = dimensionType == RS2::EntityTolerance || dimensionType == RS2::EntityDimLeader;
            break;
        }
        default:
            break;
    }
    return accept;
}

RS2::EntityType LC_DlgDimension::adjustDimentityTypeForStyleName(const RS2::EntityType entityType) {
    switch (entityType) {
        case RS2::EntityDimAngular: {
            return RS2::EntityDimLinear;
        }
        case RS2::EntityTolerance: {
            return RS2::EntityDimLeader;
        }
        default:
            return entityType;
    }
}

void LC_DlgDimension::prepareDimStyleItems(QList<LC_DimStyleItem*> &items) {
    const QString defaultDimStyleName = m_graphic->getDefaultDimStyleName();
    const LC_DimStyle* styleThatIsDefault = m_graphic->getDimStyleByName(defaultDimStyleName);

    const auto dimStylesList = m_graphic->getDimStyleList();
    const auto dimStyles = dimStylesList->getStylesList();

    const RS2::EntityType entityType = adjustDimentityTypeForStyleName(m_entity->rtti());

    // here we filter styles - and show only styles that are specific for particular entity type or generic ones

    // first we collect base names of styles for which type-specific styles exists
    QSet<QString> typeSpecificStyles;
    for (const auto dimStyle : *dimStyles) {
        QString baseName;
        RS2::EntityType styleType;
        LC_DimStyle::parseStyleName(dimStyle->getName(), baseName, styleType);
        if (isDimensionTypeMatchedToStyleType(styleType, entityType)) {
           typeSpecificStyles.insert(baseName);
        }
    }

    for (const auto dimStyle : *dimStyles) {
        QString baseName;
        RS2::EntityType styleType;
        LC_DimStyle::parseStyleName(dimStyle->getName(), baseName, styleType);

        if (styleType == RS2::EntityUnknown) {
            // this is basic style, but potentially there might be type-specific style based on it.
            // so add that style only if there is no such type-specific style
            if (!typeSpecificStyles.contains(baseName)) {
                LC_DimStyle* ds = dimStyle->getCopy();
                const auto item = new LC_DimStyleItem(ds, 0, styleThatIsDefault == dimStyle);
                items << item;
            }
        } // this is type-specific style. Check that it matched to entity type and add if it is so
        else if (isDimensionTypeMatchedToStyleType(styleType, entityType)){
            LC_DimStyle* ds = dimStyle->getCopy();
            const auto item = new LC_DimStyleItem(ds, 0, styleThatIsDefault == dimStyle || baseName == defaultDimStyleName);
            items << item;
        }
    }
}

void LC_DlgDimension::expandStylesTree() const {
    ui->lvDimStyles->expandAll();
    ui->lvDimStyles->setItemsExpandable(false);
}

void LC_DlgDimension::setupPreview() {
    m_previewView = LC_DimStylePreviewGraphicView::init(this, m_graphic,m_entity);

    m_previewView->setFocusPolicy(Qt::ClickFocus);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    ui->gbDimStylesPreview->setLayout(layout);

    const auto previewToolbar = new LC_DimStylePreviewPanel(this);
    previewToolbar->setGraphicView(m_previewView);

    layout->addWidget(previewToolbar);
    layout->addWidget(m_previewView, 10);
}

void LC_DlgDimension::showEvent(QShowEvent* event) {
    LC_EntityPropertiesDlg::showEvent(event);
    m_previewView->zoomAuto();
    m_previewView->refresh();
}

void LC_DlgDimension::languageChange() {
    ui->retranslateUi(this);
}
