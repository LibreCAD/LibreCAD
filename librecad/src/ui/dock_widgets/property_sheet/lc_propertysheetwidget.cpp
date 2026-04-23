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

#include "lc_propertysheetwidget.h"

#include <QMenu>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_dlg_propertysheet_widget_options.h"
#include "lc_entity_property_containerprovider.h"
#include "lc_entitymetauiutils.h"
#include "lc_graphicviewport.h"
#include "lc_properties_sheet.h"
#include "lc_property_container.h"
#include "lc_property_double.h"
#include "lc_property_layer.h"
#include "lc_property_multi.h"
#include "lc_property_rsvector.h"
#include "lc_property_view_registrator.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_shortcuts_manager.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "ui_lc_propertysheetwidget.h"

class LC_ToolOptionsPropertiesContainerProvider;

void LC_PropertySheetWidget::setupSelectionButton(QToolButton* selectionButton, QAction* selectionPointerAction,
                                                  LC_ActionGroupManager* actionGroupManager) {
    selectionButton->setDefaultAction(selectionPointerAction);
    selectionButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* menu = new QMenu();
    QList<QAction*> actions;
    actions.push_back(actionGroupManager->getActionByName("SelectAll"));
    actions.push_back(actionGroupManager->getActionByName("SelectSingle"));
    actions.push_back(actionGroupManager->getActionByName("SelectContour"));
    actions.push_back(actionGroupManager->getActionByName("SelectWindow"));
    actions.push_back(actionGroupManager->getActionByName("DeselectWindow"));
    actions.push_back(actionGroupManager->getActionByName("SelectIntersected"));
    actions.push_back(actionGroupManager->getActionByName("DeselectIntersected"));
    actions.push_back(actionGroupManager->getActionByName("SelectLayer"));
    actions.push_back(actionGroupManager->getActionByName("SelectPoints"));
    actions.push_back(actionGroupManager->getActionByName("SelectInvert"));
    menu->addActions(actions);
    selectionButton->setMenu(menu);
}

LC_PropertySheetWidget::LC_PropertySheetWidget(QWidget* parent, LC_ActionContext* actionContext, LC_ActionGroupManager* actionGroupManager)
    : LC_GraphicViewAwareWidget(parent), ui(new Ui::LC_PropertySheetWidget), m_actionContext{actionContext},
      m_propertySheetOptions{std::make_unique<LC_PropertySheetWidgetOptions>()} {
    const auto viewFactory = LC_PropertyViewFactory::staticInstance();
    LC_PropertyViewRegistrator registrator(*viewFactory);
    registrator.registerViews();

    ui->setupUi(this);

    const auto propertiesSheet = ui->propertySheet->propertiesSheet();
    connect(propertiesSheet, &LC_PropertiesSheet::propertyEdited, this, &LC_PropertySheetWidget::onPropertyEdited);
    connect(propertiesSheet, &LC_PropertiesSheet::beforePropertyEdited, this, &LC_PropertySheetWidget::onBeforePropertyEdited);
    connect(propertiesSheet, &LC_PropertiesSheet::activePropertyChanged, this, &LC_PropertySheetWidget::onActivePropertyChanged);
    connect(ui->cbSelection, &QComboBox::currentIndexChanged, this, &LC_PropertySheetWidget::onSelectionIndexChanged);

    connect(ui->tbSettings, &QToolButton::clicked, this, &LC_PropertySheetWidget::onSettingsClicked);

    ui->propertySheet->setParts(PROPERTY_WIDGET_AREA_INFO);

    QAction* quickSelectAction = actionGroupManager->getActionByName("SelectQuick");
    QAction* toggleSelectModeAction = actionGroupManager->getActionByName("SelectionModeToggle");
    QAction* selectEntitiesAction = actionGroupManager->getActionByName("SelectionGeneric");
    QAction* selectionPointerAction = actionGroupManager->getActionByName("EditKillAllActions");

    ui->tbSelectQuick->setDefaultAction(quickSelectAction);
    ui->tbPickAddSwitch->setDefaultAction(toggleSelectModeAction);
    ui->tbSelectObjects->setDefaultAction(selectEntitiesAction);

    ui->tbSelectQuickLeft->setDefaultAction(quickSelectAction);
    ui->tbPickAddSwitchLeft->setDefaultAction(toggleSelectModeAction);
    ui->tbSelectObjectsLeft->setDefaultAction(selectEntitiesAction);

    setupSelectionButton(ui->tbSelectionGeneral, selectionPointerAction, actionGroupManager);
    setupSelectionButton(ui->tbSelectionGeneralLeft, selectionPointerAction, actionGroupManager);

    ui->tbCancel->setDefaultAction(selectionPointerAction);

    m_entityContainerProvider = std::make_unique<LC_EntityPropertyContainerProvider>();
    m_entityContainerProvider->init(this, actionContext);

    loadCollapsedSections();
    m_propertySheetOptions->load();
    updatePropertiesSheetFont();

    ui->tbSelectionGeneral->setVisible(m_propertySheetOptions->duplicateSelectionAction);
    ui->tbSelectionGeneralLeft->setVisible(m_propertySheetOptions->duplicateSelectionAction);

    ui->headerAction->setVisible(false);
    updateWidgetSettings();
}

LC_PropertySheetWidget::~LC_PropertySheetWidget() {
    delete ui;
}

void LC_PropertySheetWidget::updatePropertiesSheetFont() const {
    auto font = ui->propertySheet->propertiesSheet()->font();
    font.setPointSize(m_propertySheetOptions->fontSize);
    ui->propertySheet->propertiesSheet()->setFont(font);
    ui->propertySheet->propertiesSheet()->updateStylingVars();
}

void LC_PropertySheetWidget::loadCollapsedSections() {
    const QString sectionsList = LC_GET_ONE_STR("PropertySheet", "CollapsedSections", "");
    if (!sectionsList.isEmpty()) {
        QStringList parts = sectionsList.split(",", Qt::SkipEmptyParts);
        for (const auto& sectionName : std::as_const(parts)) {
            m_collapsedContainerNames << sectionName;
        }
    }
}

void LC_PropertySheetWidget::saveCollapsedSections() {
    QString settingsValue;
    for (const auto& sectionName : std::as_const(m_collapsedContainerNames)) {
        settingsValue = settingsValue + "," + sectionName;
    }
    LC_SET_ONE("PropertySheet", "CollapsedSections", settingsValue);
}

void LC_PropertySheetWidget::setGraphicView(RS_GraphicView* gv) {
    // remove tracking of relative point from old view
    if (m_graphicView != nullptr) {
        disconnect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_PropertySheetWidget::onRelativeZeroChanged);
        disconnect(m_graphicView, &RS_GraphicView::ucsChanged, this, &LC_PropertySheetWidget::onUcsChanged);
        disconnect(m_graphicView, &RS_GraphicView::defaultActionActivated, this, &LC_PropertySheetWidget::onViewDefaultActionActivated);
        if (m_document != nullptr) {
            m_document->getSelection()->removeListener(this);
            if (!m_document->is(RS2::EntityBlock)) {
                m_document->getUCSList()->removeListener(this);
                m_document->getLayerList()->removeListener(this);
                m_document->getViewList()->removeListener(this);
            }
        }
    }

    m_graphicView = gv;

    // add tracking of relative point for new view
    if (gv != nullptr) {
        RS_Document* doc = nullptr;
        LC_GraphicViewport* viewport = nullptr;
        connect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_PropertySheetWidget::onRelativeZeroChanged);
        connect(m_graphicView, &RS_GraphicView::ucsChanged, this, &LC_PropertySheetWidget::onUcsChanged);
        connect(m_graphicView, &RS_GraphicView::defaultActionActivated, this, &LC_PropertySheetWidget::onViewDefaultActionActivated);
        viewport = gv->getViewPort();
        doc = gv->getDocument();
        m_document = doc;
        if (doc != nullptr) {
            m_document->getSelection()->addListener(this);
            if (!m_document->is(RS2::EntityBlock)) {
                m_document->getUCSList()->addListener(this);
                m_document->getLayerList()->addListener(this);
                m_document->getViewList()->addListener(this);
            }
        }

        m_viewport = viewport;

        m_entityContainerProvider->setGraphicView(gv);

        initPropertySheet();
        setupSelectionTypeCombobox(RS2::EntityContainer, "");
        setEnabled(true);
    }
    else {
        m_viewport = nullptr;
        m_document = nullptr;
        ui->cbSelection->blockSignals(true);
        ui->propertySheet->blockSignals(true);
        ui->cbSelection->clear();
        const auto previousContainer = ui->propertySheet->propertyContainer();
        if (previousContainer != nullptr) {
            m_entityContainerProvider->clearEntities();
            previousContainer->clearChildProperties();
            previousContainer->deleteLater();
            ui->propertySheet->setPropertyContainer(nullptr);
        }
        ui->cbSelection->blockSignals(false);
        ui->propertySheet->blockSignals(false);
        setEnabled(false);
    }
}

void LC_PropertySheetWidget::stopInplaceEdit() const {
    ui->propertySheet->stopInplaceEdit();
}

void LC_PropertySheetWidget::refill() {
    // LC_ERR << "On Selection Changed!";
    stopInplaceEdit();
    if (m_actionContext->getDocument() == nullptr) {
        return;
    }
    if (m_operationMode == MODE_SELECTION) {
        if (m_handleSelectionChange) {
            const int selectionIndex = ui->cbSelection->currentIndex();
            if (selectionIndex >= 0) {
                // save current state
                int itemEntityType = getCurrentlySelectedEntityType(selectionIndex);
                const auto entityType = static_cast<RS2::EntityType>(itemEntityType);
                const auto activeProperty = ui->propertySheet->propertiesSheet()->activeProperty();
                const QString propertyName = activeProperty != nullptr ? activeProperty->getName() : "";

                setupSelectionTypeCombobox(entityType, propertyName);
            }
            else {
                // clean init
                setupSelectionTypeCombobox(RS2::EntityContainer, "");
            }
        }
    }
    else {
        LC_PropertyContainer* container = prepareToolOptionsContainer(m_toolOptionsPropertiesContainerProvider);
        replaceTopLevelContainer(container);
    }

    if (!m_activePropertyName.isEmpty()) {
        const auto propertiesSheet = ui->propertySheet->propertiesSheet();
        const QString propertyName = m_activePropertyName;
        const auto foundProperty = propertiesSheet->getPropertyWithName(propertyName);
        if (foundProperty != nullptr) {
            propertiesSheet->setActiveProperty(foundProperty, true);
        }
    }
}

void LC_PropertySheetWidget::showToolOptions(LC_ToolOptionsPropertiesContainerProvider* provider) {
    m_toolOptionsPropertiesContainerProvider = provider;
    m_operationMode = MODE_TOOL_OPTIONS;
    refill();
}

void LC_PropertySheetWidget::setShouldHandleSelectionChange(const bool value) {
    m_handleSelectionChange = value;
}

void LC_PropertySheetWidget::updateFormats() {
    refill();
}

void LC_PropertySheetWidget::doProcessLateRequest(const LC_ActionContext::InteractiveInputInfo& interactiveInputInfo) {
    const auto inputType = interactiveInputInfo.inputType;
    switch (inputType) {
        case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
            setPickedPropertyValue(interactiveInputInfo.requestorTag, interactiveInputInfo.distance, inputType);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::ANGLE: {
            setPickedPropertyValue(interactiveInputInfo.requestorTag, interactiveInputInfo.angleRad, inputType);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT: {
            const RS_Vector ucsVector = m_viewport->toUCS(interactiveInputInfo.wcsPoint);
            setPickedPointPropertyValue(interactiveInputInfo.requestorTag, ucsVector);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT_X: {
            const RS_Vector ucsVector = m_viewport->toUCS(interactiveInputInfo.wcsPoint);
            setPickedPropertyCoordinateValue(interactiveInputInfo.requestorTag, ucsVector.x, true);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
            const RS_Vector ucsVector = m_viewport->toUCS(interactiveInputInfo.wcsPoint);
            setPickedPropertyCoordinateValue(interactiveInputInfo.requestorTag, ucsVector.y, false);
            break;
        }
        default:
            break;
    }
}

void LC_PropertySheetWidget::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        const auto interactiveInput = m_actionContext->getInteractiveInputInfo();
        interactiveInput->requestor = nullptr;
        interactiveInput->state = LC_ActionContext::InteractiveInputInfo::NONE;
        // fixme - cancel editing or defreese state?
    }
    else {
        const auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
        const bool updateInteractiveInputValues = interactiveInputInfo->state == LC_ActionContext::InteractiveInputInfo::REQUESTED;
        if (updateInteractiveInputValues) {
            if (m_operationMode == MODE_SELECTION) {
                doProcessLateRequest(*interactiveInputInfo);
            }
            else {
                // delayed call, as we may be in pick action and property sheet could be empty (without tool options properties)
                LC_ActionContext::InteractiveInputInfo inputCopy;
                interactiveInputInfo->copyTo(inputCopy);
                QTimer::singleShot(10, [inputCopy, this]() -> void {
                    doProcessLateRequest(inputCopy);
                });
            }
        }
    }
}

void LC_PropertySheetWidget::onUcsChanged([[maybe_unused]] LC_UCS* ucs) {
    m_handleSelectionChange = true;
    refill();
}

void LC_PropertySheetWidget::onViewDefaultActionActivated(const bool defaultActionActivated,
                                                          [[maybe_unused]] const RS2::ActionType actionRtti,
                                                          const RS2::ActionType prevActionRtti) {
    setShouldHandleSelectionChange(defaultActionActivated);
    if (defaultActionActivated) {
        // prevent refresh of the widget if activation is called after interactive input  - it might be that update
        // is already performed as result of setting value of property
        const bool shouldRefill = !RS2::isInteractiveInputAction(prevActionRtti) && (prevActionRtti != RS2::ActionDefault && prevActionRtti != RS2::ActionNone);
        if (shouldRefill) {
            refill();
        }
    }
}

void LC_PropertySheetWidget::onRelativeZeroChanged(const RS_Vector&) {
}

void LC_PropertySheetWidget::setupSelectionTypeCombobox(const RS2::EntityType entityTypeTryToSet,
                                                        [[maybe_unused]] QString propertyTryToSet) {
    ui->cbSelection->blockSignals(true);
    ui->cbSelection->clear();
    int entityTypesCount = 0;
    if (m_document != nullptr) {
        const auto selection = RS_Selection(m_document, m_viewport);
        QMap<RS2::EntityType, int> selectedEntityTypesMap;
        selection.countSelectedEntities(selectedEntityTypesMap);
        LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(ui->cbSelection, selectedEntityTypesMap);
        entityTypesCount = selectedEntityTypesMap.count();
    }
    ui->cbSelection->blockSignals(false);

    int indexToSet = 0;

    if (entityTypesCount == 1) {
        indexToSet = 1;
    }
    else {
        if (entityTypeTryToSet != RS2::EntityContainer) {
            const int indexForEntityType = ui->cbSelection->findData(entityTypeTryToSet, Qt::UserRole);
            if (indexForEntityType != -1) {
                indexToSet = indexForEntityType;
            }
        }
    }

    const int currentIndex = ui->cbSelection->currentIndex();
    if (currentIndex == indexToSet) {
        onSelectionIndexChanged(currentIndex);
    }
    else {
        ui->cbSelection->setCurrentIndex(indexToSet);
    }
}

void LC_PropertySheetWidget::entityModified([[maybe_unused]] RS_Entity* originalEntity, [[maybe_unused]] RS_Entity* entityClone) {
    // LC_ERR << "entity Modified!";
    m_orginalEntities.push_back(originalEntity);
    m_modifiedEntities.push_back(entityClone);
}

void LC_PropertySheetWidget::clearContextEntities() {
    for (const auto e : std::as_const(m_orginalEntities)) {
        if (RS2::isDimensionalEntity(e->rtti())) {
            auto* d = static_cast<RS_Dimension*>(e);
            d->clearCachedDimStyle();
        }
    }
    for (const auto e : std::as_const(m_modifiedEntities)) {
        if (RS2::isDimensionalEntity(e->rtti())) {
            const auto d = static_cast<RS_Dimension*>(e);
            d->clearCachedDimStyle();
        }
    }
    m_orginalEntities.clear();
    m_modifiedEntities.clear();
}

void LC_PropertySheetWidget::initPropertySheet() {
}

void LC_PropertySheetWidget::onBeforePropertyEdited([[maybe_unused]] LC_Property* property,
                                                    [[maybe_unused]] LC_Property::PropertyValuePtr newValue, [[maybe_unused]] int typeId) {
    // LC_ERR << "On Before Edit " << property->getName() << " " << property->getRootProperty()->getName();
}

void LC_PropertySheetWidget::onPropertyEdited(LC_Property* property) {
    const bool propertyIsVirtual = isVirtualProperty(property);
    if (propertyIsVirtual) {
        return;
    }
    // special processing for layer property - hidden layer may be selected, so such entity should not be selected
    const QString propertyName = property->getName();
    bool layerHidden = false;
    if (propertyName == "layer") {
        LC_PropertyAtomic* p = property->asAtomic();
        const auto* multiProperty = dynamic_cast<LC_PropertyMulti*>(p);
        if (multiProperty != nullptr) {
            const auto layerAtomic = multiProperty->getFirstProperty();
            const auto layerProperty = dynamic_cast<LC_PropertyLayer*>(layerAtomic);
            if (layerProperty != nullptr) {
                const RS_Layer* layer = layerProperty->value();
                if (layer != nullptr) {
                    layerHidden = layer->isFrozen();
                }
            }
        }
    }

    // fixme - add delayed modification method
    QTimer::singleShot(30, [this, layerHidden]()-> void {
        // LC_ERR << "On Edited - " << propertyName;
        m_document->undoableModify(m_viewport, [this](LC_DocumentModificationBatch& ctx)-> bool {
                                       ctx.entitiesToAdd.append(m_modifiedEntities);
                                       ctx.entitiesToDelete.append(m_orginalEntities);
                                       ctx.dontSetActiveLayerAndPen();
                                       clearContextEntities();
                                       return true;
                                   }, [layerHidden](const LC_DocumentModificationBatch& ctx, RS_Document* doc)-> void {
                                       if (!layerHidden) {
                                           doc->select(ctx.entitiesToAdd);
                                       }
                                   });
    });
}

int LC_PropertySheetWidget::getCurrentlySelectedEntityType(const int index) const {
    const int itemEntityType = ui->cbSelection->itemData(index, Qt::UserRole).toInt();
    return itemEntityType;
}

QLayout* LC_PropertySheetWidget::getTopLevelLayout() const {
    return ui->gridLayout;
}

void LC_PropertySheetWidget::replaceTopLevelContainer(LC_PropertyContainer* const newContainer) {
    const auto previousContainer = ui->propertySheet->propertyContainer();
    ui->propertySheet->setPropertyContainer(newContainer);
    if (previousContainer != nullptr) {
        destroyContainer(previousContainer);
    }
}

void LC_PropertySheetWidget::onSelectionIndexChanged(const int index) {
    int itemEntityType = getCurrentlySelectedEntityType(index);
    const auto entityType = static_cast<RS2::EntityType>(itemEntityType);
    const auto newContainer = preparePropertiesContainer(entityType);
    replaceTopLevelContainer(newContainer);
}

LC_PropertyContainer* LC_PropertySheetWidget::preparePropertiesContainer(const RS2::EntityType entityType) {
    QList<RS_Entity*> entitiesToModify;
    collectEntitiesToModify(entityType, entitiesToModify);
    LC_PropertyContainer* result = createPropertiesContainer(entityType, entitiesToModify);
    return result;
}

LC_PropertyContainer* LC_PropertySheetWidget::prepareToolOptionsContainer(
    LC_ToolOptionsPropertiesContainerProvider* toolOptionsContainerProvider) {
    LC_PropertyContainer* result = nullptr;
    if (isVisible()) {
        result = new LC_PropertyContainer(this);
    }
    m_entityContainerProvider->fillPropertyContainerToolOptions(m_document, result, toolOptionsContainerProvider);
    return result;
}

void LC_PropertySheetWidget::collectEntitiesToModify(RS2::EntityType entityType, QList<RS_Entity*>& entitiesToModify) const {
    if (entityType == RS2::EntityUnknown || entityType == RS2::EntityContainer) {
        m_document->collectSelected(entitiesToModify);
    }
    else {
        m_document->collectSelected(entitiesToModify, false, {entityType});
    }
}

LC_PropertyContainer* LC_PropertySheetWidget::createPropertiesContainer(const RS2::EntityType entityType, const QList<RS_Entity*>& list) {
    LC_PropertyContainer* result = nullptr;
    if (isVisible()) {
        result = new LC_PropertyContainer(this);
    }
    if (list.isEmpty()) {
        m_entityContainerProvider->fillPropertyContainerForNoSelection(m_document, result);
    }
    else {
        m_entityContainerProvider->fillPropertyContainerForSelection(m_document, result, entityType, list);
    }
    return result;
}

void LC_PropertySheetWidget::destroyContainer(LC_PropertyContainer* previousContainer) const {
    m_entityContainerProvider->cleanup();
    m_viewport->clearLocationsHighlight();
    previousContainer->deleteLater();
}

void LC_PropertySheetWidget::setPickedPropertyValue(const QString& propertyName, const double interactiveInputValue,
                                                    [[maybe_unused]] LC_ActionContext::InteractiveInputInfo::InputType input) const {
    const auto propertyContainer = ui->propertySheet->propertyContainer();
    auto propertiesByName = propertyContainer->findChildProperties(propertyName);
    const auto propertiesSheet = ui->propertySheet->propertiesSheet();
    if (propertiesByName.isEmpty()) {
        LC_ERR << "Unable to find property by name " << propertyName;
    }
    else {
        const auto sheetModel = propertiesSheet->getModel();
        for (const auto p : std::as_const(propertiesByName)) {
            auto* valueMultiProperty = dynamic_cast<LC_PropertyMulti*>(p);
            if (valueMultiProperty != nullptr) {
                const auto propertyItem = sheetModel->findItem(sheetModel->getRootItem(), valueMultiProperty);
                const auto valuePropertyAtomic = valueMultiProperty->getFirstProperty();
                const auto valuePropertyDouble = dynamic_cast<LC_PropertyDouble*>(valuePropertyAtomic);
                if (valuePropertyDouble != nullptr) {
                    const auto coordinatePropertyView = propertyItem->view.get();
                    valueMultiProperty->markEdited(true);
                    propertiesSheet->connectOnPropertyChange(valueMultiProperty, true);
                    valuePropertyDouble->setValue(interactiveInputValue, coordinatePropertyView->changeReasonDueToEdit());
                    propertiesSheet->connectOnPropertyChange(valueMultiProperty, false);
                }
            }
            else {
                const auto valuePropertyDouble = dynamic_cast<LC_PropertyDouble*>(p);
                if (valuePropertyDouble != nullptr) {
                    valuePropertyDouble->setValue(interactiveInputValue, PropertyChangeReasonEdit);
                }
            }
        }
    }
}

void LC_PropertySheetWidget::setPickedPropertyCoordinateValue(const QString& propertyName, const double interactiveInputValue,
                                                              const bool forX) const {
    const auto propertyContainer = ui->propertySheet->propertyContainer();
    auto propertiesByName = propertyContainer->findChildProperties(propertyName);
    const auto propertiesSheet = ui->propertySheet->propertiesSheet();

    if (propertiesByName.isEmpty()) {
        LC_ERR << "Unable to find property by name " << propertyName;
    }
    else {
        const auto sheetModel = propertiesSheet->getModel();
        for (const auto p : std::as_const(propertiesByName)) {
            const auto* vectorMultiProperty = dynamic_cast<LC_PropertyMulti*>(p);
            if (vectorMultiProperty == nullptr) {
                const auto vectorProperty = dynamic_cast<LC_PropertyRSVector*>(p);
                if (vectorProperty != nullptr) {
                    // this is vertext of polyline, spline etc.
                    const int subPropertyIdx = forX ? 0 : 1; // x should be first component, y - second one
                    const auto propertyItem = sheetModel->findItem(sheetModel->getRootItem(), vectorProperty);
                    const auto coordinatePropertyItem = propertyItem->children.at(subPropertyIdx).get();
                    const auto coordinatePropertyView = coordinatePropertyItem->view.get();

                    const auto coordinateProperty = static_cast<LC_PropertyDouble*>(coordinatePropertyView->getProperty());

                    propertiesSheet->connectOnPropertyChange(vectorProperty, true);
                    coordinateProperty->setValue(interactiveInputValue, propertyItem->view->changeReasonDueToEdit());
                    propertiesSheet->connectOnPropertyChange(vectorProperty, false);
                }
            }
            else {
                // this is normal case - we're in multi property
                const auto propertyItem = sheetModel->findItem(sheetModel->getRootItem(), vectorMultiProperty);
                if (propertyItem != nullptr) {
                    const auto vectorMultiPropertyView = propertyItem->view.get();
                    const int subPropertyIdx = forX ? 0 : 1; // x should be first component, y - second one]
                    const auto coordinateSubProperty = vectorMultiPropertyView->getSubProperty(subPropertyIdx);
                    auto* coordinateMultiProperty = dynamic_cast<LC_PropertyMulti*>(coordinateSubProperty);
                    if (coordinateMultiProperty != nullptr) {
                        const auto coordinateFirstAtomic = coordinateMultiProperty->getFirstProperty();
                        const auto coordinateDoubleProperty = dynamic_cast<LC_PropertyDouble*>(coordinateFirstAtomic);
                        if (coordinateDoubleProperty != nullptr) {
                            const auto coordinatePropertyItem = propertyItem->children.at(subPropertyIdx).get();
                            const auto coordinatePropertyView = coordinatePropertyItem->view.get();
                            coordinateMultiProperty->markEdited(true);
                            propertiesSheet->connectOnPropertyChange(vectorMultiProperty, true);
                            coordinateDoubleProperty->setValue(interactiveInputValue, coordinatePropertyView->changeReasonDueToEdit());
                            propertiesSheet->connectOnPropertyChange(vectorMultiProperty, false);
                        }
                    }
                }
            }
        }
    }
}

void LC_PropertySheetWidget::setPickedPointPropertyValue(const QString& propertyName, const RS_Vector& ucsVector) const {
    const auto propertyContainer = ui->propertySheet->propertyContainer();

    auto lcProperties = propertyContainer->findChildProperties(propertyName);
    if (lcProperties.isEmpty()) {
        LC_ERR << "Unable to find property by name " << propertyName;
    }
    else {
        const auto propertiesSheet = ui->propertySheet->propertiesSheet();
        const auto sheetModel = propertiesSheet->getModel();
        for (const auto p : std::as_const(lcProperties)) {
            const auto vectorPropertyMulti = dynamic_cast<LC_PropertyMulti*>(p);
            if (vectorPropertyMulti == nullptr) {
                const auto vectorProperty = dynamic_cast<LC_PropertyRSVector*>(p);
                if (vectorProperty != nullptr) {
                    // this is vertex of polyline, spline etc.
                    propertiesSheet->connectOnPropertyChange(vectorProperty, true);
                    const auto propertyItem = sheetModel->findItem(sheetModel->getRootItem(), vectorProperty);
                    vectorProperty->setValue(ucsVector, propertyItem->view->changeReasonDueToEdit());
                    propertiesSheet->connectOnPropertyChange(vectorProperty, false);
                }
            }
            else {
                // normal multi property
                const auto propertyItem = sheetModel->findItem(sheetModel->getRootItem(), vectorPropertyMulti);
                if (propertyItem != nullptr) {
                    const auto vectorAtomic = vectorPropertyMulti->getFirstProperty();
                    const auto vectorProperty = dynamic_cast<LC_PropertyRSVector*>(vectorAtomic);
                    if (vectorProperty != nullptr) {
                        vectorPropertyMulti->markEdited(true);
                        propertiesSheet->connectOnPropertyChange(vectorPropertyMulti, true);
                        vectorProperty->setValue(ucsVector, propertyItem->view->changeReasonDueToEdit());
                        propertiesSheet->connectOnPropertyChange(vectorPropertyMulti, false);
                    }
                }
            }
        }
    }
}

bool LC_PropertySheetWidget::isVirtualProperty(const LC_Property* property) {
    const auto descriptor = property->getViewDescriptor();
    bool result = false;
    if (descriptor != nullptr) {
        descriptor->load(LC_PropertyView::ATTR_VIRTUAL, result);
    }
    return result;
}

bool LC_PropertySheetWidget::isCollapsedSection(const QString& name) const {
    return m_collapsedContainerNames.contains(name);
}

void LC_PropertySheetWidget::markContainerCollapsed(const QString& name, const bool collapse) {
    if (collapse) {
        m_collapsedContainerNames << name;
    }
    else {
        m_collapsedContainerNames.remove(name);
    }
    saveCollapsedSections();
}

void LC_PropertySheetWidget::checkSectionCollapsed(LC_PropertyContainer* result) {
    const QString name = result->getName();
    const bool containerCollapsed = isCollapsedSection(name);
    if (containerCollapsed) {
        result->collapse();
    }
    connect(result, &LC_Property::afterPropertyChange, this, &LC_PropertySheetWidget::onSectionPropertyChanged);
}

void LC_PropertySheetWidget::onSectionPropertyChanged(const LC_PropertyChangeReason reason) {
    if (reason.testFlag(PropertyChangeReasonStateLocal)) {
        const auto property = qobject_cast<LC_Property*>(sender());
        const QString name = property->getName();
        const bool collapsed = property->isCollapsed();
        markContainerCollapsed(name, collapsed);
    }
}

void LC_PropertySheetWidget::onActivePropertyChanged(LC_Property* activeProperty) {
    if (m_viewport == nullptr) {
        return;
    }
    m_viewport->clearLocationsHighlight();
    if (activeProperty == nullptr) {
        return;
    }
    const auto masterProperty = activeProperty->getPrimaryProperty();
    if (masterProperty != nullptr) {
        checkIfVectorAndGetLocation(masterProperty);
    }
    else {
        checkIfVectorAndGetLocation(activeProperty);
    }

    m_activePropertyName = activeProperty->getName();
}

void LC_PropertySheetWidget::highlightVectorPropertyPosition(const LC_PropertyRSVector* vectorProperty) const {
    if (!vectorProperty->isMultiValue()) {
        const RS_Vector ucsPosition = vectorProperty->value();
        const auto wcsPosition = m_viewport->toWorld(ucsPosition);
        m_viewport->highlightLocation(wcsPosition);
    }
}

void LC_PropertySheetWidget::checkIfVectorAndGetLocation(LC_Property* property) {
    auto vectorProperty = dynamic_cast<LC_PropertyRSVector*>(property);
    if (vectorProperty == nullptr) {
        const auto multiCandidate = dynamic_cast<LC_PropertyMulti*>(property);
        if (multiCandidate != nullptr) {
            const auto firstProperty = multiCandidate->getFirstProperty();
            if (firstProperty != nullptr) {
                vectorProperty = dynamic_cast<LC_PropertyRSVector*>(firstProperty);
                if (vectorProperty != nullptr) {
                    highlightVectorPropertyPosition(vectorProperty);
                }
            }
        }
    }
    else {
        highlightVectorPropertyPosition(vectorProperty);
    }
}

void LC_PropertySheetWidget::invalidateCached() const {
    const auto propertiesSheet = ui->propertySheet->propertiesSheet();
    const auto sheetModel = propertiesSheet->getModel();
    sheetModel->invalidateCached();
}

void LC_PropertySheetWidget::onDockVisibilityChanged(const bool visible) {
    // LC_ERR << "Dock visibility changed - " << visible;
    if (visible) {
        m_handleSelectionChange = true;
        refill();
    }
    else {
        m_handleSelectionChange = false;
    }
}

void LC_PropertySheetWidget::onActivePenChanged(RS_Pen) {
    const auto container = ui->propertySheet->propertyContainer();
    if (container != nullptr) {
        const int tag = container->getTag();
        if (tag == LC_EntityPropertyContainerProvider::TAG_CONTAINER_NO_SELECTION) {
            refill();
        }
    }
}

void LC_PropertySheetWidget::onSettingsClicked() {
    LC_DlgPropertySheetWidgetOptions dlg(this, m_propertySheetOptions.get());
    if (dlg.exec() == QDialog::Accepted) {
        const bool showSelectionButton = m_propertySheetOptions->duplicateSelectionAction;
        ui->tbSelectionGeneral->setVisible(showSelectionButton);
        ui->tbSelectionGeneralLeft->setVisible(showSelectionButton);
        updatePropertiesSheetFont();
        refill();
    }
}

void LC_PropertySheetWidget::doAdjustForDockLocation(Qt::DockWidgetArea area) {
    LC_GraphicViewAwareWidget::doAdjustForDockLocation(area);
    switch (area) {
        case Qt::DockWidgetArea::LeftDockWidgetArea: {
            ui->wCommandsForLeft->setVisible(true);
            ui->wCommandsForRight->setVisible(false);
            break;
        }
        case Qt::DockWidgetArea::RightDockWidgetArea: {
            ui->wCommandsForLeft->setVisible(false);
            ui->wCommandsForRight->setVisible(true);
            break;
        }
        default:
            // do nothing so far
            break;
    }
}

void LC_PropertySheetWidget::setCurrentQAction(const QAction* a) {
    bool showIcon = a != nullptr /*&& LC_GET_ONE_BOOL("Appearance", "ShowActionIconInOptions", true)*/;
    if (showIcon) {
        // check for actions those icons should not be shown
        const auto property = a->property("_SetAsCurrentActionInView");
        if (property.isValid()) {
            showIcon = property.toBool();
        }
    }
    if (showIcon) {
        const QIcon icon = a->icon();
        const QString text = LC_ShortcutsManager::getPlainActionToolTip(a);
        ui->lblActionIcon->setVisible(!icon.isNull());
        constexpr int m_iconSize = 24;
        ui->lblActionIcon->setPixmap(icon.pixmap(m_iconSize));
        ui->lblActionName->setText(text);
        ui->headerAction->setVisible(true);
        ui->headerSelection->setVisible(false);
        m_operationMode = MODE_TOOL_OPTIONS;
    }
    else {
        ui->headerAction->setVisible(false);
        ui->headerSelection->setVisible(true);
        m_operationMode = MODE_SELECTION;

        /*auto graphicView = m_actionContext->getGraphicView();
        if (graphicView != nullptr) {
            auto gv = static_cast<QG_GraphicView*>(graphicView);
            auto recentActions = gv->getRecentActions();
            if (recentActions.empty()) {
                ui->tbLastAction->setVisible(false);
            }
            else {
                ui->tbLastAction->setVisible(true);
                ui->tbLastAction->setDefaultAction(recentActions.first());
            }
        }*/
    }
}
