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

#include "lc_dlgquickselection.h"

#include <QMessageBox>

#include "lc_convert.h"
#include "lc_entitymatchdescriptorsregistry.h"
#include "lc_entitymetauiutils.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "ui_lc_dlgquickselection.h"

LC_DlgQuickSelection::LC_DlgQuickSelection(QWidget* parent, LC_ActionContext* actionContext,
                                           const LC_ActionContext::InteractiveInputInfo::InputType interactiveInputType,
                                           const LC_QuickSearchSelectionDialogState* savedState, const double interactiveInputValue1,
                                           const double interactiveInputValue2)
    : LC_Dialog(parent, "QuickSelection"), ui(new Ui::LC_DlgQuickSelection), m_actionContext{actionContext},
      m_document{actionContext->getDocument()}, m_viewport{actionContext->getViewport()} {
    ui->setupUi(this);

    const auto selection = RS_Selection(m_document, m_viewport);
    selection.collectCurrentSelectionState(m_selectionState);

    const bool hasSelection = !m_selectionState.selectedEntityTypes.empty();

    ui->cbApplyTo->addItem(tr("Entire Drawing"));
    if (hasSelection) {
        ui->cbApplyTo->addItem(tr("Current Selection"));
    }

    connect(ui->cbApplyTo, &QComboBox::currentIndexChanged, this, &LC_DlgQuickSelection::onApplyToCurrentIndexChanged);
    connect(ui->cbEntityType, &QComboBox::currentIndexChanged, this, &LC_DlgQuickSelection::onEntityTypeIndexChanged);
    connect(ui->lvProperties, &QListWidget::currentRowChanged, this, &LC_DlgQuickSelection::onPropertyListRowChanged);

    connect(ui->cbAppendToCurrentSelectionSet, &QCheckBox::checkStateChanged, this, &LC_DlgQuickSelection::onAppendToCurrentSetClicked);
    connect(ui->rbExcludeFromSelectionSet, &QRadioButton::toggled, this, &LC_DlgQuickSelection::onAppendToCurrentSetClicked);
    connect(ui->cbOperator, &QComboBox::currentIndexChanged, this, &LC_DlgQuickSelection::onOperatorChanged);

    connect(ui->tbPickLength, &QToolButton::clicked, this, &LC_DlgQuickSelection::onPickLengthClicked);
    connect(ui->tbPickCoordX, &QToolButton::clicked, this, &LC_DlgQuickSelection::onPickCoord);
    connect(ui->tbPickCoordY, &QToolButton::clicked, this, &LC_DlgQuickSelection::onPickCoord);
    connect(ui->tbPickAngle, &QToolButton::clicked, this, &LC_DlgQuickSelection::onPickAngle);
    connect(ui->tbManualSelect, &QToolButton::clicked, this, &LC_DlgQuickSelection::onManualSelection);
    connect(ui->tbPrecisionUpdateByDoc, &QToolButton::clicked, this, &LC_DlgQuickSelection::onUpdatePrecisionByDocumentSettings);
    connect(ui->lePrecision, &QLineEdit::editingFinished, this, &LC_DlgQuickSelection::onPrecisionTextEditingFinished);

    ui->cbValueColor->init(true, false);
    ui->cbLineWidth->init(true, false);
    ui->cbLineType->init(true, false);

    ui->cbValueColorResolved->init(false, false);
    ui->cbLineWidthResolved->init(false, false);
    ui->cbLineTypeResolved->init(false, false);

    const auto layerList = m_document->getGraphic()->getLayerList();
    ui->cbValueLayer->init(*layerList, false, false);

    if (interactiveInputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
        restoreFromSavedState(savedState, interactiveInputType, interactiveInputValue1, interactiveInputValue2);
    }
    else {
        onApplyToCurrentIndexChanged(0);
        onEntityTypeIndexChanged(0);

        tryToRestorePreviousState();
    }

    updateWidgetSettings();
}

LC_DlgQuickSelection::~LC_DlgQuickSelection() {
    delete ui;
}

RS_Selection::ConditionalSelectionOptions LC_DlgQuickSelection::getSelectionOptions(RS_Selection::ConditionalSelectionOptions& result) const {
    saveState();

    result.applyArea = ui->cbApplyTo->currentIndex() == 0
                           ? RS_Selection::ConditionalSelectionOptions::ApplyTo::Document
                           : RS_Selection::ConditionalSelectionOptions::ApplyTo::Selection;
    result.appendToSelectionSet = ui->cbAppendToCurrentSelectionSet->isChecked();
    result.includeIntoSelectionSet = ui->rbIncludeInSelectionSet->isChecked();

    LC_EntityMatcher* matcher = obtainEntityMatcher();
    Q_ASSERT(matcher != nullptr);

    result.funEntityMatcher = [matcher](RS_Entity* e)-> bool {
        return matcher->isMatch(e);
    };

    result.funCleanup = [matcher] {
        delete matcher;
    };

    RS2::EntityType typeToMatch = getEntityTypeToMatch();
    if (typeToMatch == RS2::EntityUnknown) {
        result.funRttiMatcher = []([[maybe_unused]] RS_Entity* e)-> bool {
            return true;
        };
    }
    else {
        result.funRttiMatcher = [typeToMatch](const RS_Entity* e)-> bool {
            const RS2::EntityType rtti = e->rtti();
            return rtti == typeToMatch;
        };
    }

    return result;
}

void LC_DlgQuickSelection::accept() {
    bool validate = !m_selectionRequested;
    if (validate) {
        validate = m_interactiveInputRequested == LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
    }
    if (validate && m_propertyDescriptor != nullptr) {
        const auto propertyType = m_propertyDescriptor->getPropertyType();
        bool checkValue = false;
        bool checkPrecision = false;
        switch (propertyType) {
            case ENTITY_PROPERTY_COORD_X:
            case ENTITY_PROPERTY_COORD_Y:
            case ENTITY_PROPERTY_COORD_X_CONTAINS:
            case ENTITY_PROPERTY_COORD_Y_CONTAINS:
            case ENTITY_PROPERTY_ANGLE:
            case ENTITY_PROPERTY_LENGTH: {
                checkPrecision = true;
                [[fallthrough]];
            }
            case ENTITY_PROPERTY_INT:
            case ENTITY_PROPERTY_STRING:
                checkValue = true;
                break;
            default:
                break;
        }
        if (checkValue) {
            if (ui->leValue->text().isEmpty()) {
                QMessageBox::warning(this, "Error", tr("Please provide non empty value to match"), QMessageBox::Ok);
                return;
            }
            if (checkPrecision) {
                if (ui->lePrecision->text().isEmpty()) {
                    QMessageBox::warning(this, "Error", tr("Please provide non empty value of precision"), QMessageBox::Ok);
                    return;
                }
            }
        }
    }
    LC_Dialog::accept();
}

void LC_DlgQuickSelection::onApplyToCurrentIndexChanged(const int index) {
    ui->cbEntityType->blockSignals(true);
    ui->cbEntityType->clear();

    const RS2::EntityType savedEntityType = m_entityType;
    QString savedPropertyName = "";
    const auto savedOperationType = m_operationType;
    if (m_propertyDescriptor != nullptr) {
        savedPropertyName = m_propertyDescriptor->getName();
    }

    if (index == 0) {
        ui->cbEntityType->addItem(tr("Multiple") + " (" + QString::number(m_selectionState.totalDocumentEntities) + ")",
                                  RS2::EntityType::EntityUnknown);
        setupEntitiesTypesList(m_selectionState.documentEntityTypes);
    }
    else {
        ui->cbEntityType->addItem(tr("Multiple") + " (" + QString::number(m_selectionState.totalSelectedEntities) + ")",
                                  RS2::EntityType::EntityUnknown);
        setupEntitiesTypesList(m_selectionState.selectedEntityTypes);
    }
    ui->cbEntityType->blockSignals(false);

    if (setCurrentEntityType(savedEntityType)) {
        if (!savedPropertyName.isEmpty()) {
            setCurrentPropertyName(savedPropertyName);
            setCurrentOperation(savedOperationType);
        }
    }
}

void LC_DlgQuickSelection::onOperatorChanged(const int index) {
    m_operationType = obtainOperation(index);
    const bool notOperationAll = m_operationType != LC_PropertyMatchOperation::MATCH_OPERATION_ALL;
    ui->lblValue->setVisible(notOperationAll);
    ui->swValues->setVisible(notOperationAll);
    ui->lvProperties->setEnabled(notOperationAll);
}

void LC_DlgQuickSelection::onEntityTypeIndexChanged(const int index) {
    const auto propertiesList = ui->lvProperties;
    m_entityType = obtainEntityType(index);

    m_entityMatchDescriptor = LC_EntityMatchDescriptorsRegistry::instance(m_actionContext)->findEntityMatchDescriptor(m_entityType);

    Q_ASSERT(m_entityMatchDescriptor != nullptr);

    propertiesList->blockSignals(true);
    propertiesList->clear();

    m_entityMatchDescriptor->fillPropertiesInfo([propertiesList](const QString& name, const QString& displayName, const QString& description)-> void {
        const auto newItem = new QListWidgetItem;
        newItem->setText(displayName);
        newItem->setData(Qt::UserRole, name);
        newItem->setToolTip(description);
        propertiesList->addItem(newItem);
    });

    propertiesList->blockSignals(false);
    propertiesList->setCurrentRow(0);
}

LC_EntityMatcher* LC_DlgQuickSelection::obtainEntityMatcher() const {
    auto propertyType = m_propertyDescriptor->getPropertyType();
    LC_PropertyMatchOperation operatorType = obtainCurrentOperation();
    bool operationAll = operatorType == LC_PropertyMatchOperation::MATCH_OPERATION_ALL;
    if (operationAll) {
        auto* matcher = static_cast<LC_EntityMatcherBase*>(m_propertyDescriptor->createMatcher());
        matcher->updateForOperationAll();
        return matcher;
    }

    switch (propertyType) {
        case ENTITY_PROPERTY_LAYER: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<RS_Layer*>*>(m_propertyDescriptor->createMatcher());
            RS_Layer* valueToMatch = ui->cbValueLayer->getLayer();
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_DIM_STYLE: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<LC_DimStyle*>*>(m_propertyDescriptor->createMatcher());
            LC_DimStyle* valueToMatch = nullptr; /*ui->cbValueLayer->getLayer();*/ //FIXME - COMPLETE SUPPORT OF DIMSTYLES!!!
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_COLOR_RESOLVED:
        case ENTITY_PROPERTY_COLOR: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<RS_Color>*>(m_propertyDescriptor->createMatcher());
            RS_Color valueToMatch;
            if (propertyType == ENTITY_PROPERTY_COLOR) {
                valueToMatch = ui->cbValueColor->getColor();
            }
            else {
                valueToMatch = ui->cbValueColorResolved->getColor();
            }
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_LINETYPE:
        case ENTITY_PROPERTY_LINETYPE_RESOLVED: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<RS2::LineType>*>(m_propertyDescriptor->createMatcher());
            RS2::LineType valueToMatch;
            if (propertyType == ENTITY_PROPERTY_LINETYPE) {
                valueToMatch = ui->cbLineType->getLineType();
            }
            else {
                valueToMatch = ui->cbLineTypeResolved->getLineType();
            }
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_LINEWIDTH:
        case ENTITY_PROPERTY_LINEWIDTH_RESOLVED: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<RS2::LineWidth>*>(m_propertyDescriptor->createMatcher());
            RS2::LineWidth valueToMatch;
            if (propertyType == ENTITY_PROPERTY_LINEWIDTH) {
                valueToMatch = ui->cbLineWidth->getWidth();
            }
            else {
                valueToMatch = ui->cbLineWidthResolved->getWidth();
            }
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_BOOL: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<bool>*>(m_propertyDescriptor->createMatcher());
            bool valueToMatch = ui->cbValueBool->currentIndex() == 0; // YES = 0, NO = 1
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_INT: {
            QString valText = ui->leValue->text().trimmed();
            int valueToMatch{0};
            if (LC_Convert::toInt(valText, valueToMatch)) {
                auto* typedMatcher = static_cast<LC_GenericEntityMatcher<int>*>(m_propertyDescriptor->createMatcher());
                typedMatcher->update(valueToMatch, valueToMatch, operatorType);
                return typedMatcher;
            }
            // invalid input!! we'll ignore it silently (which is bad, yet AutoCAD does this too)
            return nullptr;
        }
        case ENTITY_PROPERTY_INT_CHOICE: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<int>*>(m_propertyDescriptor->createMatcher());
            int valueToMatch = ui->cbValue->currentData(Qt::UserRole).toInt();
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_STRING_CHOICE: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<QString>*>(m_propertyDescriptor->createMatcher());
            QString valueToMatch = ui->cbValue->currentData(Qt::UserRole).toString();
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
        case ENTITY_PROPERTY_LENGTH:
        case ENTITY_PROPERTY_DOUBLE: {
            QString valText = ui->leValue->text().trimmed();
            QString toleranceText = ui->lePrecision->text().trimmed();
            double valueToMatch{0};
            if (LC_Convert::toDouble(valText, valueToMatch, 0.0, false)) {
                auto* typedMatcher = static_cast<LC_GenericEntityMatcher<double>*>(m_propertyDescriptor->createMatcher());
                double toleranceValue{RS_TOLERANCE};
                // fixme - check which format should be used there. What if non-decimal?
                if (!LC_Convert::toDouble(toleranceText, toleranceValue, RS_TOLERANCE, true)) {
                    toleranceValue = RS_TOLERANCE;
                }
                typedMatcher->update(valueToMatch, toleranceValue, operatorType);
                return typedMatcher;
            }
            // invalid input!! we'll ignore it silently (which is bad, yet AutoCAD does this too)
            return nullptr;
        }
        case ENTITY_PROPERTY_ANGLE: {
            QString valText = ui->leValue->text().trimmed();
            QString toleranceText = ui->lePrecision->text().trimmed();
            double ucsAngleDeg{0};
            if (LC_Convert::parseToToDoubleAngleDegrees(valText, ucsAngleDeg, 0.0, false)) {
                // perform conversion of angle form ui (where it's in degrees and in UCS) to WCS angle.
                double ucsAngleBasis = RS_Math::deg2rad(ucsAngleDeg);
                double ucsAngleAbs = m_viewport->toAbsUCSAngle(ucsAngleBasis);
                double wcsAngle = m_viewport->toWorldAngle(ucsAngleAbs);
                auto* typedMatcher = static_cast<LC_GenericEntityMatcher<double>*>(m_propertyDescriptor->createMatcher());
                double toleranceValue{RS_TOLERANCE_ANGLE};
                if (!LC_Convert::toDouble(toleranceText, toleranceValue, RS_TOLERANCE_ANGLE, true)) {
                    // fixme - check which format should be used there. What if non-decimal?
                    toleranceValue = RS_TOLERANCE_ANGLE;
                }
                typedMatcher->update(wcsAngle, toleranceValue, operatorType);
                return typedMatcher;
            }
            // invalid input!! we'll ignore it silently (which is bad, yet AutoCAD does this too)
            return nullptr;
        }
        case ENTITY_PROPERTY_COORD_X:
        case ENTITY_PROPERTY_COORD_Y:
        case ENTITY_PROPERTY_COORD_X_CONTAINS:
        case ENTITY_PROPERTY_COORD_Y_CONTAINS: {
            QString valText = ui->leValue->text().trimmed();
            QString toleranceText = ui->lePrecision->text().trimmed();
            double coord{0};
            if (LC_Convert::toDouble(valText, coord, 0.0, false)) {
                auto* typedMatcher = static_cast<LC_GenericEntityMatcher<double>*>(m_propertyDescriptor->createMatcher());
                double toleranceValue{RS_TOLERANCE};
                // fixme - check which format should be used there. What if non-decimal?
                if (!LC_Convert::toDouble(toleranceText, toleranceValue, RS_TOLERANCE, true)) {
                    toleranceValue = RS_TOLERANCE;
                }
                typedMatcher->updateWithMapper(coord, toleranceValue, operatorType, m_viewport);
                return typedMatcher;
            }
            // invalid input!! we'll ignore it silently (which is bad, yet AutoCAD does this too)
            return nullptr;
        }
        case ENTITY_PROPERTY_STRING: {
            auto* typedMatcher = static_cast<LC_GenericEntityMatcher<QString>*>(m_propertyDescriptor->createMatcher());
            QString valueToMatch = ui->leValue->text().trimmed();
            typedMatcher->update(valueToMatch, valueToMatch, operatorType);
            return typedMatcher;
        }
    }
    return nullptr;
}

RS2::EntityType LC_DlgQuickSelection::getEntityTypeToMatch() const {
    return m_entityType;
}

void LC_DlgQuickSelection::addOperationItem(LC_PropertyMatchDescriptor* propertyDescriptor, const LC_PropertyMatchOperation type, const QString& label) const {
    if (propertyDescriptor->isSupportedOperation(type)) {
        ui->cbOperator->addItem(label, QVariant(type));
    }
}

void LC_DlgQuickSelection::setupOperationsCombobox(LC_PropertyMatchDescriptor* propertyDescriptor) const {
    addOperationItem(propertyDescriptor, MATCH_OPERATION_EQUALS, tr("= Equals"));
    addOperationItem(propertyDescriptor, MATCH_OPERATION_NOT_EQUALS, tr("<> Not Equals"));
    addOperationItem(propertyDescriptor, MATCH_OPERATION_GREATER, tr("> Greater than"));
    addOperationItem(propertyDescriptor, MATCH_OPERATION_LESS, tr("< Less than"));
    addOperationItem(propertyDescriptor, MATCH_OPERATION_PATTERN_MATCH, tr("* Wildcard match"));
    addOperationItem(propertyDescriptor, MATCH_OPERATION_ALL, tr("Select All"));
}

void LC_DlgQuickSelection::enablePrecisionLength() {
    ui->lblPrecision->setVisible(true);
    ui->lePrecision->setVisible(true);
    ui->tbPrecisionUpdateByDoc->setVisible(true);
    if (m_precisionLength.isEmpty()) {
        updatePrecisionLength();
    }
    else {
        ui->lePrecision->setText(m_precisionLength);
    }
}

void LC_DlgQuickSelection::enablePrecisionAngle() {
    ui->lblPrecision->setVisible(true);
    ui->lePrecision->setVisible(true);
    ui->tbPrecisionUpdateByDoc->setVisible(true);
    if (m_precisionLength.isEmpty()) {
        updatePrecisionAngle();
    }
    else {
        ui->lePrecision->setText(m_precisionLength);
    }
}

void LC_DlgQuickSelection::disablePrecision() const {
    ui->lblPrecision->setVisible(false);
    ui->lePrecision->setVisible(false);
    ui->tbPrecisionUpdateByDoc->setVisible(false);
}

#define PAGE_LAYER  0
#define PAGE_COLOR  1
#define PAGE_COLOR_RESOLVED  2
#define PAGE_LINE_TYPE 3
#define PAGE_LINE_TYPE_RESOLVED 4
#define PAGE_LINE_WIDTH 5
#define PAGE_LINE_WIDTH_RESOLVED 6
#define PAGE_CHOICE 7
#define PAGE_BOOL 8
#define PAGE_VALUE 9

void LC_DlgQuickSelection::setPropertyValueInput(LC_PropertyMatchDescriptor* propertyDescriptor) {
    const auto propertyType = propertyDescriptor->getPropertyType();
    ui->tbPickAngle->setVisible(false);
    ui->tbPickCoordX->setVisible(false);
    ui->tbPickCoordY->setVisible(false);
    ui->tbPickLength->setVisible(false);
    disablePrecision();
    switch (propertyType) {
        case ENTITY_PROPERTY_COORD_X:
        case ENTITY_PROPERTY_COORD_X_CONTAINS: {
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            ui->tbPickCoordX->setVisible(true);
            enablePrecisionLength();
            break;
        }
        case ENTITY_PROPERTY_COORD_Y:
        case ENTITY_PROPERTY_COORD_Y_CONTAINS: {
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            ui->tbPickCoordY->setVisible(true);
            enablePrecisionLength();
            break;
        }
        case ENTITY_PROPERTY_ANGLE: {
            ui->tbPickAngle->setVisible(true);
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            enablePrecisionAngle();
            break;
        }
        case ENTITY_PROPERTY_LENGTH: {
            ui->tbPickLength->setVisible(true);
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            enablePrecisionLength();
            break;
        }
        case ENTITY_PROPERTY_DOUBLE: {
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            enablePrecisionLength();
            break;
        }
        case ENTITY_PROPERTY_STRING:
        case ENTITY_PROPERTY_INT: {
            ui->swValues->setCurrentIndex(PAGE_VALUE);
            break;
        }
        case ENTITY_PROPERTY_LAYER: {
            ui->swValues->setCurrentIndex(PAGE_LAYER);
            break;
        }
        case ENTITY_PROPERTY_COLOR: {
            ui->swValues->setCurrentIndex(PAGE_COLOR);
            break;
        }
        case ENTITY_PROPERTY_COLOR_RESOLVED: {
            ui->swValues->setCurrentIndex(PAGE_COLOR_RESOLVED);
            break;
        }
        case ENTITY_PROPERTY_LINEWIDTH: {
            ui->swValues->setCurrentIndex(PAGE_LINE_WIDTH);
            break;
        }
        case ENTITY_PROPERTY_LINEWIDTH_RESOLVED: {
            ui->swValues->setCurrentIndex(PAGE_LINE_WIDTH_RESOLVED);
            break;
        }
        case ENTITY_PROPERTY_LINETYPE: {
            ui->swValues->setCurrentIndex(PAGE_LINE_TYPE);
            break;
        }
        case ENTITY_PROPERTY_LINETYPE_RESOLVED: {
            ui->swValues->setCurrentIndex(PAGE_LINE_TYPE_RESOLVED);
            break;
        }
        case ENTITY_PROPERTY_DIM_STYLE: {
            //ui->cbLineType->setVisible(true);
            // fixme - complete support of dim styles
            break;
        }
        case ENTITY_PROPERTY_BOOL: {
            ui->swValues->setCurrentIndex(PAGE_BOOL);
            break;
        }
        case ENTITY_PROPERTY_STRING_CHOICE:
        case ENTITY_PROPERTY_INT_CHOICE: {
            if (propertyDescriptor->isChoice()) {
                ui->swValues->setCurrentIndex(PAGE_CHOICE);
                ui->cbValue->blockSignals(true);
                ui->cbValue->clear();
                QList<QPair<QString, QVariant>> choiceValues;
                propertyDescriptor->getChoiceValues(choiceValues);
                for (const auto& [text, userData] : std::as_const(choiceValues)) {
                    ui->cbValue->addItem(text, userData);
                }
                ui->cbValue->setCurrentIndex(0);
                ui->cbValue->blockSignals(false);
            }
            break;
        }
    }
}

struct LC_QuickSearchSelectionDialogState {
    int applyToIndex = 0;
    int entityTypeIndex = 0;
    int propertyNameIndex = 0;
    int operatorIndex = 0;
    bool includeIntoNewSet = false;
    bool appendToCurrent = false;
    QString precisionLength = "";
    QString precisionAngle = "";
};

LC_QuickSearchSelectionDialogState* LC_DlgQuickSelection::getSavedState() const {
    auto* result = new LC_QuickSearchSelectionDialogState();
    result->applyToIndex = ui->cbApplyTo->currentIndex();
    result->entityTypeIndex = ui->cbEntityType->currentIndex();
    result->propertyNameIndex = ui->lvProperties->currentRow();
    result->operatorIndex = ui->cbOperator->currentIndex();
    result->includeIntoNewSet = ui->rbIncludeInSelectionSet->isChecked();
    result->appendToCurrent = ui->cbAppendToCurrentSelectionSet->isChecked();
    result->precisionLength = m_precisionLength;
    result->precisionAngle = m_precisionAngle;
    return result;
}

void LC_DlgQuickSelection::onPropertyListRowChanged(const int currentRow) {
    const QString propertyName = obtainPropertyName(currentRow);

    ui->cbOperator->blockSignals(true);
    ui->cbOperator->clear();

    m_propertyDescriptor = m_entityMatchDescriptor->findPropertyDescriptor(propertyName);
    Q_ASSERT(m_propertyDescriptor != nullptr);
    const QString propertyInfo = "<b>" + m_propertyDescriptor->getDisplayName() + "</b> - " + m_propertyDescriptor->getDescription();
    ui->lblPropertyInfo->setText(propertyInfo);
    setupOperationsCombobox(m_propertyDescriptor);
    ui->cbOperator->setCurrentIndex(0);
    setPropertyValueInput(m_propertyDescriptor);

    ui->cbOperator->blockSignals(false);
}

void LC_DlgQuickSelection::onAppendToCurrentSetClicked() const {
    const bool appendToCurrentSet = ui->cbAppendToCurrentSelectionSet->isChecked();
    const bool excludeFromCurrentSet = ui->rbExcludeFromSelectionSet->isChecked();
    const bool enableManualSelect = !appendToCurrentSet && !excludeFromCurrentSet;
    ui->tbManualSelect->setEnabled(enableManualSelect);
}

void LC_DlgQuickSelection::onPickLengthClicked() {
    m_interactiveInputRequested = LC_ActionContext::InteractiveInputInfo::DISTANCE;
    accept();
}

void LC_DlgQuickSelection::onPickCoord() {
    m_interactiveInputRequested = LC_ActionContext::InteractiveInputInfo::POINT;
    accept();
}

void LC_DlgQuickSelection::onPickAngle() {
    m_interactiveInputRequested = LC_ActionContext::InteractiveInputInfo::ANGLE;
    accept();
}

void LC_DlgQuickSelection::onManualSelection() {
    m_selectionRequested = true;
    accept();
}

void LC_DlgQuickSelection::onPrecisionTextEditingFinished() {
    const auto propertyType = m_propertyDescriptor->getPropertyType();
    switch (propertyType) {
        case ENTITY_PROPERTY_ANGLE: {
            m_precisionAngle = ui->lePrecision->text();
            break;
        }
        default: {
            m_precisionLength = ui->lePrecision->text();
            break;
        }
    }
}

void LC_DlgQuickSelection::onUpdatePrecisionByDocumentSettings() {
    const auto propertyType = m_propertyDescriptor->getPropertyType();

    switch (propertyType) {
        case ENTITY_PROPERTY_ANGLE: {
            updatePrecisionAngle();
            break;
        }
        case ENTITY_PROPERTY_COORD_X:
        case ENTITY_PROPERTY_COORD_X_CONTAINS:
        case ENTITY_PROPERTY_COORD_Y:
        case ENTITY_PROPERTY_COORD_Y_CONTAINS:
        case ENTITY_PROPERTY_LENGTH:
        case ENTITY_PROPERTY_DOUBLE: {
            updatePrecisionLength();
            break;
        }
        default:
            break;
    }
}

void LC_DlgQuickSelection::saveState() const {
    LC_GROUP_GUARD(getPositionSettingsGroupName());
    {
        const bool applyToSelection = ui->cbApplyTo->currentIndex() == 1;
        LC_SET("ApplyToSelection", applyToSelection);

        const int entityType = ui->cbEntityType->currentData().toInt();
        LC_SET("EntityType", entityType);

        const QString propertyName = obtainCurrentPropertyName();
        LC_SET("PropertyName", propertyName);

        const int operation = ui->cbOperator->currentData().toInt();
        LC_SET("Operation", operation);

        LC_SET("IncludeIntoSet", ui->rbIncludeInSelectionSet->isChecked());
        LC_SET("AppendToSet", ui->cbAppendToCurrentSelectionSet->isChecked());

        LC_SET("PrecisionLength", m_precisionLength);
        LC_SET("PrecisionAngle", m_precisionAngle);
        LC_SET("EditValue", ui->leValue->text());
    }
}

void LC_DlgQuickSelection::setupEntitiesTypesList(const QMap<RS2::EntityType, int>& map) const {
    const auto entityTypeCombobox = ui->cbEntityType;
    LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(entityTypeCombobox, map, false);
}

void LC_DlgQuickSelection::restoreFromSavedState(const LC_QuickSearchSelectionDialogState* savedState,
                                                 const LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                                 const double interactiveInputValue1, const double interactiveInputValue2) {
    if (savedState != nullptr) {
        const int index = savedState->applyToIndex;
        ui->cbApplyTo->setCurrentIndex(index);
        if (index == 0) {
            // force put listener for update
            onApplyToCurrentIndexChanged(0);
        }
        const int entityType = savedState->entityTypeIndex;
        ui->cbEntityType->setCurrentIndex(entityType);

        ui->lvProperties->setCurrentRow(savedState->propertyNameIndex);
        ui->cbOperator->setCurrentIndex(savedState->operatorIndex);
        const bool includeIntoNewSet = savedState->includeIntoNewSet;
        if (includeIntoNewSet) {
            ui->rbIncludeInSelectionSet->setChecked(true);
        }
        else {
            ui->rbExcludeFromSelectionSet->setChecked(true);
        }
        ui->cbAppendToCurrentSelectionSet->setChecked(savedState->appendToCurrent);

        const auto propertyType = m_propertyDescriptor->getPropertyType();
        if (propertyType == ENTITY_PROPERTY_ANGLE) {
            ui->lePrecision->setText(savedState->precisionAngle);
        }
        else {
            ui->lePrecision->setText(savedState->precisionLength);
        }
        delete savedState;
    }

    // set value to match based on interactive input.
    // value should be in ucs?
    switch (inputType) {
        case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
            const double dist = interactiveInputValue1;
            const QString distanceAsString = LC_Convert::asString(dist);
            ui->leValue->setText(distanceAsString);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::ANGLE: {
            const double angel = interactiveInputValue1;
            const QString angleStrDegree = LC_Convert::asStringAngleDeg(angel);
            ui->leValue->setText(angleStrDegree);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT:
        case LC_ActionContext::InteractiveInputInfo::POINT_X:
        case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
            if (m_propertyDescriptor != nullptr) { // we may got null if there is no saved state
                // here we may use only one component from picked coordinate, based on property type
                const double xCoord = interactiveInputValue1;
                const double yCoord = interactiveInputValue2;

                const auto wcs = RS_Vector(xCoord, yCoord);
                const auto ucs = m_viewport->toUCS(wcs);
                const auto lcPropertyMatchTypeEnum = m_propertyDescriptor->getPropertyType();
                double coordToUse = ucs.getX();
                if (lcPropertyMatchTypeEnum == ENTITY_PROPERTY_COORD_Y) {
                    coordToUse = ucs.getY();
                }
                const QString coordinateComponentString = LC_Convert::asString(coordToUse);
                ui->leValue->setText(coordinateComponentString);
            }
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::NOTNEEDED:
            break;
    }
}

QString LC_DlgQuickSelection::obtainPropertyName(const int currentRow) const {
    const auto item = ui->lvProperties->item(currentRow);
    const auto variant = item->data(Qt::UserRole);
    const QString propertyName = variant.toString();
    return propertyName;
}

QString LC_DlgQuickSelection::obtainCurrentPropertyName() const {
    return obtainPropertyName(ui->lvProperties->currentRow());
}

RS2::EntityType LC_DlgQuickSelection::obtainEntityType(const int index) const {
    const auto itemData = ui->cbEntityType->itemData(index, Qt::UserRole);
    Q_ASSERT(itemData.isValid());
    int intEntityType = itemData.toInt();
    const auto entityType = static_cast<RS2::EntityType>(intEntityType);
    return entityType;
}

RS2::EntityType LC_DlgQuickSelection::obtainCurrentEntityType() const {
    return obtainEntityType(ui->cbEntityType->currentIndex());
}

LC_PropertyMatchOperation LC_DlgQuickSelection::obtainCurrentOperation() const {
    const auto operationData = ui->cbOperator->currentData(Qt::UserRole);
    const auto operatorType = static_cast<LC_PropertyMatchOperation>(operationData.toInt());
    return operatorType;
}

LC_PropertyMatchOperation LC_DlgQuickSelection::obtainOperation(const int index) const {
    const auto operationData = ui->cbOperator->itemData(index, Qt::UserRole);
    const auto operatorType = static_cast<LC_PropertyMatchOperation>(operationData.toInt());
    return operatorType;
}

void LC_DlgQuickSelection::updateWidgetSettings() const {
    LC_GROUP("Widgets");
    {
        const bool flatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        const int iconSize = LC_GET_INT("DockWidgetsIconSize", 16);

        const QSize size(iconSize, iconSize);

        QList<QToolButton*> widgets = this->findChildren<QToolButton*>();
        foreach(QToolButton *w, widgets) {
            w->setAutoRaise(flatIcons);
            w->setIconSize(size);
        }
    }
    LC_GROUP_END();
}

bool LC_DlgQuickSelection::setCurrentEntityType(const int entityType) const {
    int entityTypeIdx = ui->cbEntityType->findData(entityType);
    bool result = true;
    if (entityTypeIdx == -1) {
        entityTypeIdx = 0;
        result = false;
    }

    const int currentIndex = ui->cbEntityType->currentIndex();
    if (currentIndex == entityTypeIdx) {
        ui->cbEntityType->currentIndexChanged(currentIndex);
    }
    else {
        ui->cbEntityType->setCurrentIndex(entityTypeIdx);
    }
    return result;
}

void LC_DlgQuickSelection::setCurrentPropertyName(const QString& propertyName) const {
    if (!propertyName.isEmpty()) {
        int index = -1;
        const int propCount = ui->lvProperties->count();
        for (int row = 0; row < propCount; row++) {
            const auto item = ui->lvProperties->item(row);
            if (item->data(Qt::UserRole).toString() == propertyName) {
                index = row;
                break;
            }
        }
        if (index != -1) {
            const int currentIndex = ui->lvProperties->currentRow();
            if (currentIndex == index) {
                ui->lvProperties->currentRowChanged(currentIndex);
            }
            else {
                ui->lvProperties->setCurrentRow(index);
            }
        }
    }
}

void LC_DlgQuickSelection::setCurrentOperation(const int operation) const {
    const int operationIndex = ui->cbOperator->findData(operation);
    if (operationIndex != -1) {
        if (operationIndex == ui->cbOperator->currentIndex()) {
            ui->cbOperator->currentIndexChanged(operationIndex);
        }
        else {
            ui->cbOperator->setCurrentIndex(operationIndex);
        }
    }
}

void LC_DlgQuickSelection::tryToRestorePreviousState() {
    LC_GROUP_GUARD(getPositionSettingsGroupName());
    {
        const bool applyToSelection = LC_GET_BOOL("ApplyToSelection", false);
        if (applyToSelection && ui->cbApplyTo->count() > 1) {
            ui->cbApplyTo->setCurrentIndex(1);
        }
        else {
            ui->cbApplyTo->setCurrentIndex(0);
        }

        const int entityType = LC_GET_INT("EntityType", RS2::EntityUnknown);
        if (setCurrentEntityType(entityType)) {
            const QString propertyName = LC_GET_STR("PropertyName", "");
            setCurrentPropertyName(propertyName);

            const int operation = LC_GET_INT("Operation", LC_PropertyMatchOperation::MATCH_OPERATION_EQUALS);
            setCurrentOperation(operation);
        }
        else {
            ui->lvProperties->setCurrentRow(0);
        }

        const bool includeIntoSet = LC_GET_BOOL("IncludeIntoSet", true);
        if (includeIntoSet) {
            ui->rbIncludeInSelectionSet->setChecked(true);
        }
        else {
            ui->rbExcludeFromSelectionSet->setChecked(true);
        }

        const bool append = LC_GET_BOOL("AppendToSet", false);
        ui->cbAppendToCurrentSelectionSet->setChecked(append);

        m_precisionAngle = LC_GET_STR("PrecisionAngle", "");
        if (m_precisionAngle.isEmpty()) {
            updatePrecisionAngle();
        }
        else {
            ui->lePrecision->setText(m_precisionAngle);
        }

        m_precisionLength = LC_GET_STR("PrecisionLength", "");
        if (m_precisionLength.isEmpty()) {
            updatePrecisionLength();
        }
        else {
            ui->lePrecision->setText(m_precisionLength);
        }

        ui->leValue->setText(LC_GET_STR("EditValue", ""));
    }
}

void LC_DlgQuickSelection::updatePrecisionLength() {
    const int precision = m_viewport->getGraphic()->getLinearPrecision();
    const RS2::LinearFormat unit = m_viewport->getGraphic()->getLinearFormat();

    QString stringValue;

    switch (unit) {
        // scientific
        case RS2::Scientific: {
            const QString values[] = {"1E+01", "1E-1", "1E-2", "1E-3", "1E-4", "1E-5", "1E-6", "1E-7", "1E-8"};
            stringValue = values[precision];
            break;
        }
        case RS2::ArchitecturalMetric:
        case RS2::Decimal: {
            const QString values[] = {"1.00", "0.1", "0.01", "0.001", "0.0001", "0.00001", "0.000001", "0.0000001", "0.00000001"};
            stringValue = values[precision];
            break;
        }
        case RS2::Architectural: {
            const QString values[] = {
                "0'-0\"",
                "0'-0 1/2\"",
                "0'-0 1/4\"",
                "0'-0 1/8\"",
                "0'-0 1/16\"",
                "0'-0 1/32\"",
                "0'-0 1/64\"",
                "0'-0 1/128\""
            };
            stringValue = values[precision];
            break;
        }
        case RS2::Engineering: {
            const QString values[] = {
                "0'-0\"",
                "0'-0.0\"",
                "0'-0.00\"",
                "0'-0.000\"",
                "0'-0.0000\"",
                "0'-0.00000\"",
                "0'-0.000000\"",
                "0'-0.0000000\"",
                "0'-0.00000000\""
            };
            stringValue = values[precision];
            break;
        }
        case RS2::Fractional: {
            const QString values[] = {"1", "0 1/2", "0 1/4", "0 1/8", "0 1/16", "0 1/32", "0 1/64", "0 1/128"};
            stringValue = values[precision];
            break;
        }
        default: Q_ASSERT("Unknown unit type");
            break;
    }
    ui->lePrecision->setText(stringValue);
    m_precisionLength = stringValue;
}

void LC_DlgQuickSelection::updatePrecisionAngle() {
    const int precision = m_viewport->getGraphic()->getLinearPrecision();
    const RS2::AngleFormat format = m_viewport->getGraphic()->getAngleFormat();
    QString stringValue;
    switch (format) {
        case RS2::DegreesDecimal: {
            const QString values[] = {"1.00", "0.1", "0.01", "0.001", "0.0001", "0.00001", "0.000001", "0.0000001", "0.00000001"};
            stringValue = values[precision];
            break;
        }
        case RS2::AngleFormat::DegreesMinutesSeconds: {
            const QString values[] = {
                QString("0%1").arg(QChar(0xB0)),
                QString("0%100'").arg(QChar(0xB0)),
                QString("0%100'00\"").arg(QChar(0xB0)),
                QString("0%100'00.0\"").arg(QChar(0xB0)),
                QString("0%100'00.00\"").arg(QChar(0xB0)),
                QString("0%100'00.000\"").arg(QChar(0xB0)),
                QString("0%100'00.0000\"").arg(QChar(0xB0))
            };
            stringValue = values[precision];
            break;
        }
        case RS2::AngleFormat::Gradians: {
            const QString values[] = {"1g", "0.1g", "0.01g", "0.001g", "0.0001g", "0.00001g", "0.000001g", "0.0000001g", "0.00000001g"};
            stringValue = values[precision];
            break;
        }
        case RS2::AngleFormat::Radians: {
            const QString values[] = {"1r", "0.1r", "0.01r", "0.001r", "0.0001r", "0.00001r", "0.000001r", "0.0000001r", "0.00000001r"};
            stringValue = values[precision];
            break;
        }
        case RS2::AngleFormat::Surveyors: {
            const QString values[] = {
                "N 1d E",
                "N 0d01' E",
                "N 0d00'01\" E",
                "N 0d00'00.1\" E",
                "N 0d00'00.01\" E",
                "N 0d00'00.001\" E",
                "N 0d00'00.0001\" E"
            };
            stringValue = values[precision];
            break;
        }
        default:
            break;
    }
    ui->lePrecision->setText(stringValue);
    m_precisionAngle = stringValue;
}
