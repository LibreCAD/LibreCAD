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

#include "lc_dlgdimstylemanager.h"

#include "lc_dimarrowregistry.h"
#include "lc_dimstyle.h"
#include "lc_dimstylepreviewgraphicview.h"
#include "lc_dimstylepreviewpanel.h"
#include "lc_graphicviewport.h"
#include "lc_inputtextdialog.h"
#include "lc_linemath.h"
#include "lc_propertiesprovider_dim_linear.h"
#include "lc_tabproxywidget.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_graphicview.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_font.h"
#include "ui_lc_dlgdimstylemanager.h"

#define CUSTOM_BLOCK_NAME "_CUSTOM"
#define CUSTOM_SELECT_BLOCK_NAME "_CUSTOM_SELECT"

/**
 *
 * @param parent Used for editing shared dimstyle
 * @param dimStyle
 * @param originalGraphic
 * @param dimensionType
 */
LC_DlgDimStyleManager::LC_DlgDimStyleManager(QWidget* parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic, const RS2::EntityType dimensionType)
    : LC_Dialog(parent, "DimStyleManager")
    , ui(new Ui::LC_DlgDimStyleManager), m_originalGraphic{originalGraphic} {
    ui->setupUi(this);
    initBlocksList();
    init(dimensionType);
    setDimStyle(dimStyle);
    initPreview(dimensionType);
    ui->tabWidget->setCurrentIndex(0);
}

/**
 *
 * @param parent Used for editing style override
 * @param dimStyle
 * @param originalGraphic
 * @param entity
 * @param baseStyleName
 */
LC_DlgDimStyleManager::LC_DlgDimStyleManager(QWidget* parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic, const RS_Dimension* entity, const QString& baseStyleName)
    : LC_Dialog(parent, "DimStyleManager")
    , ui(new Ui::LC_DlgDimStyleManager), m_originalGraphic{originalGraphic}, m_editMode{OVERRIDE_EDITING},
      m_baseStyleName{baseStyleName}{
    ui->setupUi(this);
    initBlocksList();
    init(entity->rtti());
    setDimStyle(dimStyle);
    initPreview(entity);
    ui->tabWidget->setCurrentIndex(0);
}

LC_DlgDimStyleManager::~LC_DlgDimStyleManager() {
    delete ui;
}

void LC_DlgDimStyleManager::addDimStyle(LC_DimStyle* dimStyle) const {
    m_previewView->addDimStyle(dimStyle);
}

void LC_DlgDimStyleManager::refreshPreview() const {
    if (m_editMode == OVERRIDE_EDITING) {
        m_previewView->setEntityDimStyle(m_dimStyle, true, m_baseStyleName);
    }
    m_previewView->updateDims();
}

void LC_DlgDimStyleManager::resizeEvent(QResizeEvent* resizeEvent) {
    m_previewView->zoomAuto();
    LC_Dialog::resizeEvent(resizeEvent);
}

void LC_DlgDimStyleManager::setReadOnly() {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    disableContainer(ui->gbLinesDimLines);
    disableContainer(ui->gbLinesExtLines);
    disableContainer(ui->gbSymbolsArrowheads);
    disableContainer(ui->gbSymbolsDimBreak);
    disableContainer(ui->gbSymbolsCentermarks);
    disableContainer(ui->gbSymbolsArcLength);
    disableContainer(ui->gbLinearJog );
    disableContainer(ui->gbRadiusJog );
    disableContainer(ui->gbTextAppearance);
    disableContainer(ui->gbTextPlacement);
    disableContainer(ui->gbTextAlignment);
    disableContainer(ui->bgFitOptions);
    disableContainer(ui->bgFitTextPlacement);
    disableContainer(ui->gbFitScale);
    disableContainer(ui->gbFitFineTune);
    disableContainer(ui->gbPrimaryUnitLinear);
    disableContainer(ui->gbPrimaryUnitAngular);

    ui->cbAlternateUnitsDisplay->setEnabled(false);
    disableContainer(ui->gbAltUnits);
    disableContainer(ui->dsbTolScalingHeight);
    disableContainer(ui->grpTolAltUnit);
}

int LC_DlgDimStyleManager::computeToleranceMethod(const LC_DimStyle* dimStyle, LC_DimStyle::LatteralTolerance* tolerance, bool& enable,
                                                  bool& showVerticalPosition, bool& showLowerLimit, bool& showUpperLimit) {

    bool dimTol = tolerance->isAppendTolerancesToDimText();
    const bool dimLim = tolerance->isLimitsGeneratedAsDefaultText();

    int tolMethod = 0;
    enable = true;
    showVerticalPosition = false;
    showLowerLimit = true;
    showUpperLimit = true;

    const auto dimLine = dimStyle->dimensionLine();
    const double lineGap = dimLine->lineGap();
    const bool linegapNegative = std::signbit(lineGap);

    if (linegapNegative) {
        // it should be just basic if frame is set explicitly
        dimTol = false;
        tolerance->setAppendTolerancesToDimText(false);
    }

    if (dimTol) {
        const double lowerLimit = tolerance->lowerToleranceLimit();
        showVerticalPosition = true;
        if (LC_LineMath::isNotMeaningful(lowerLimit)) {
            tolMethod = 1; // symmetrical
            showLowerLimit = false;
            showUpperLimit = true;
        }
        else {
            tolMethod = 2; // deviation
            showUpperLimit = true;
        }
    }
    else {
        if (dimLim) {
            tolMethod = 3; // limits
            showVerticalPosition = false;
            showUpperLimit = true;
        }
        else {
            enable = false;
            if (linegapNegative) {
                tolMethod = 4; // basic
                showUpperLimit = false;
                showLowerLimit = false;
            }
            else {
                tolMethod = 0; // None
                showUpperLimit = false;
                showLowerLimit = false;
            }
        }
    }
    return tolMethod;
}

void LC_DlgDimStyleManager::applyToleranceMethod(LC_DimStyle::LatteralTolerance* tol, LC_DimStyle::DimensionLine* dimLine, const int index,
                                                 bool& enable, bool& showLowerLimit, bool& showVerticalPosition,
                                                 bool& additionallyHideToleranceAdjustment, bool& drawFrame) {
    switch (index) {
        case 0: { // none
            enable = false;
            tol->setAppendTolerancesToDimText(false);
            tol->setLimitsAreGeneratedAsDefaultText(false);
            break;
        }
        case 1: { //Symmetrical
            tol->setAppendTolerancesToDimText(true);
            tol->setLimitsAreGeneratedAsDefaultText(false);
            tol->setLowerToleranceLimit(0.0);
            showLowerLimit = false;
            showVerticalPosition = false;
            additionallyHideToleranceAdjustment = true;
            break;
        }
        case 2: { //Deviation
            tol->setAppendTolerancesToDimText(true);
            tol->setLimitsAreGeneratedAsDefaultText(false);
            showVerticalPosition = true;
            break;
        }
        case 3: { //Limits
            tol->setAppendTolerancesToDimText(false);
            tol->setLimitsAreGeneratedAsDefaultText(true);
            break;
        }
        case 4: { // Basic
            tol->setAppendTolerancesToDimText(false);
            tol->setLimitsAreGeneratedAsDefaultText(false);
            showLowerLimit = false;
            enable = false;

            const auto lineGap = dimLine->lineGap();
            if (!std::signbit(lineGap)) {
                dimLine->setLineGap(-lineGap);
            }
            drawFrame = true;
            break;
        }
        default:
            break;
    }
}

void LC_DlgDimStyleManager::onDimLineColorChanged(const RS_Color& color) const {
    m_dimStyle->dimensionLine()->setColor(color);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineTypeChanged(const RS2::LineType lineType) const {
    m_dimStyle->dimensionLine()->setLineType(lineType);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineWidthChanged(const RS2::LineWidth w) const {
    m_dimStyle->dimensionLine()->setLineWidth(w);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineExtBeyondChanged(const double d) const {
    m_dimStyle->dimensionLine()->setDistanceBeyondExtLinesForObliqueStroke(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineBaselineSpacingChanged(const double d) const {
    m_dimStyle->dimensionLine()->setBaselineDimLinesSpacing(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineSuppress1Toggled(const bool val) const {
    m_dimStyle->dimensionLine()->setSuppressFirstLine(val? LC_DimStyle::DimensionLine::SUPPRESS : LC_DimStyle::DimensionLine::DONT_SUPPRESS);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimLineSuppress2Toggled(const bool val) const {
    m_dimStyle->dimensionLine()->setSuppressSecondLine(val? LC_DimStyle::DimensionLine::SUPPRESS : LC_DimStyle::DimensionLine::DONT_SUPPRESS);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineColorChanged(const RS_Color& color) const {
    m_dimStyle->extensionLine()->setColor(color);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineType1Changed(const RS2::LineType lineType) const {
    m_dimStyle->extensionLine()->setLineTypeFirst(lineType);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineType2Changed(const RS2::LineType lineType) const {
    m_dimStyle->extensionLine()->setLineTypeSecond(lineType);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineWidthChanged(const RS2::LineWidth w) const {
    m_dimStyle->extensionLine()->setLineWidth(w);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineBeyondDimChanged(const double d) const {
    m_dimStyle->extensionLine()->setDistanceBeyondDimLine(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineOffsetFromOriginChanged(const double d) const {
    m_dimStyle->extensionLine()->setDistanceFromOriginPoint(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineFixedLengthToggled(const bool val) const {
    m_dimStyle->extensionLine()->setHasFixedLength(val);

    ui->bsbExtLineFixedLength->setEnabled(val);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineFixedLengthChanged(const double d) const {
    m_dimStyle->extensionLine()->setFixedLength(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineSuppress1Toggled(const bool val) const {
    m_dimStyle->extensionLine()->setSuppressFirstLine(val? LC_DimStyle::ExtensionLine::SUPPRESS : LC_DimStyle::ExtensionLine::DONT_SUPPRESS);
    refreshPreview();
}

void LC_DlgDimStyleManager::onExtLineSuppress2Toggled(const bool val) const {
    m_dimStyle->extensionLine()->setSuppressSecondLine(val? LC_DimStyle::ExtensionLine::SUPPRESS : LC_DimStyle::ExtensionLine::DONT_SUPPRESS);
    refreshPreview();
}

void LC_DlgDimStyleManager::onArrowheadTheSameToggled(const bool val) const {
    m_dimStyle->arrowhead()->setUseSeparateArrowHeads(!val);
    ui->cbArrowheadSecond->setEnabled(!val);
    refreshPreview();
}

void LC_DlgDimStyleManager::setCustomArrowBlockName(QComboBox* arrowComboBox, const QString& dimblk1, const int customBlockIndex, const QString& extData) {
    arrowComboBox->blockSignals(true);

    if (extData == CUSTOM_SELECT_BLOCK_NAME) {
        arrowComboBox->insertItem(customBlockIndex, QIcon(), dimblk1, dimblk1);
    }
    else { // custom block was selected
        arrowComboBox->setItemText(customBlockIndex, dimblk1);
        arrowComboBox->setItemData(customBlockIndex, dimblk1);
    }
    arrowComboBox->setCurrentIndex(customBlockIndex);
    arrowComboBox->blockSignals(false);
}

bool LC_DlgDimStyleManager::processArrowComboboxChange(QComboBox* arrowComboBox, QString& dimblk1) {
    const auto data = arrowComboBox->currentData();
    dimblk1 = data.toString();
    const int customBlockIndex = m_defaultArrowsInfo.size();
    const auto existingData = arrowComboBox->itemData(customBlockIndex);
    const auto extData = existingData.toString();
    if (dimblk1 == CUSTOM_SELECT_BLOCK_NAME) {
        bool ok = false;
        dimblk1 = LC_InputTextDialog::getText(this, tr("Select Block for arrow"),
                                              "Enter the name of existing block that will be used as arrow", m_blocksList, false,
                                              "", &ok);
        if (!ok) {
            return true;
        }
        setCustomArrowBlockName(arrowComboBox, dimblk1, customBlockIndex, extData);
    }
    else if (extData != CUSTOM_SELECT_BLOCK_NAME){
        arrowComboBox->blockSignals(true);
        arrowComboBox->removeItem(customBlockIndex);
        arrowComboBox->blockSignals(false);
    }
    return false;
}

void LC_DlgDimStyleManager::onArrowheadFirstChanged([[maybe_unused]]int index) {
    const auto arrowComboBox = ui->cbArrowheadFirst;
    QString dimblk1;
    const auto arrowhead = m_dimStyle->arrowhead();
    if (processArrowComboboxChange(arrowComboBox, dimblk1)) {
        // selection of custom block cancelled. Restore
        setArrowComboboxValue(arrowComboBox, arrowhead->arrowHeadBlockNameFirst());
        return;
    }

    if (ui->cbArrowheadTheSame->isChecked()) {
        arrowhead->setSameBlockName(dimblk1);
        arrowhead->setArrowHeadBlockNameFirst(dimblk1);
        arrowhead->setArrowHeadBlockNameSecond(dimblk1);
        ui->cbArrowheadSecond->blockSignals(true);
        setArrowComboboxValue(ui->cbArrowheadSecond,dimblk1);
        ui->cbArrowheadSecond->blockSignals(false);
    }
    else {
        arrowhead->setArrowHeadBlockNameFirst(dimblk1);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onArrowheadSecondChanged([[maybe_unused]]int index) {
    const auto arrowComboBox = ui->cbArrowheadSecond;
    QString dimblk2;
    if (processArrowComboboxChange(arrowComboBox, dimblk2)) {
        // selection of custom block cancelled. Restore
        setArrowComboboxValue(arrowComboBox, m_dimStyle->arrowhead()->arrowHeadBlockNameSecond());
        return;
    }
    m_dimStyle->arrowhead()->setArrowHeadBlockNameSecond(dimblk2);
    refreshPreview();
}

void LC_DlgDimStyleManager::onArrowheadLeaderChanged([[maybe_unused]]int index) {
    const auto arrowComboBox = ui->cbArrowheadLeader;
    QString dimldrblk;
    if (processArrowComboboxChange(arrowComboBox, dimldrblk)) {
        return;
    }
    m_dimStyle->leader()->setArrowBlockName(dimldrblk);
    refreshPreview();
}

void LC_DlgDimStyleManager::onArrowheadArrowSizeChanged(const double d) const {
    m_dimStyle->arrowhead()->setSize(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimBreakChanged([[maybe_unused]]double d) const {
    // m_dimStyle->arrowhead()->setSize(d);
    // fixme - complete later if we'll support dim breaks
    refreshPreview();
}

void LC_DlgDimStyleManager::onArrowheadTickSizeChanged([[maybe_unused]] const double d) const {
    m_dimStyle->arrowhead()->setTickSize(d);
    uiUpdateArrowsControlsByTickSize(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextLineGapChanged(const double d) const {
    if (ui->cbTextDrawFrameAroundText->isChecked()) {
        m_dimStyle->dimensionLine()->setLineGap(-d);
    }
    else {
        m_dimStyle->dimensionLine()->setLineGap(d);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onCenterMarkTypeToggled([[maybe_unused]]bool val) const {
    const bool centermarkEnabled =  !ui->rbCentermarkNone->isChecked();
    ui->bsbCentermarkSize->setEnabled(centermarkEnabled);
    const double centermarkVal = ui->bsbCentermarkSize->value();
    setCentermarkSize(centermarkVal);
    refreshPreview();
}

void LC_DlgDimStyleManager::setCentermarkSize(const double d) const {
    const auto radial = m_dimStyle->radial();
    if (ui->rbCentermarkNone->isChecked()) {
        radial->setCenterMarkOrLineSize(0.0);
    }
    else {
        if (ui->rbCentermarkMark->isChecked()) {
            radial->setCenterMarkOrLineSize(d);
        }
        else { // centerline is negative
            radial->setCenterMarkOrLineSize(-d);
        }
    }
}

void LC_DlgDimStyleManager::onCenterMarkSizeChanged(const double d) const {
    setCentermarkSize(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onDimarkSymbolToggled([[maybe_unused]]bool val) const {
    const auto dimarc = m_dimStyle->arc();
    if (ui->rbDimarcSymAbove->isChecked()) {
        dimarc->setArcSymbolPosition(LC_DimStyle::Arc::ABOVE);
    }
    else if (ui->rbDimarcSymPreceeding->isChecked()) {
        dimarc->setArcSymbolPosition(LC_DimStyle::Arc::BEFORE);
    }
    else if (ui->rbDimarcSymNone->isChecked()) {
        dimarc->setArcSymbolPosition(LC_DimStyle::Arc::NONE);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onJogHeightFactorChanged([[maybe_unused]]double d) const {
    // fixme - sand - discover where this setting is stored in DIMSTYLE?
    refreshPreview();
}

void LC_DlgDimStyleManager::onJogAngleChanged(const double d) const {
    const auto radial = m_dimStyle->radial();
    radial->setTransverseSegmentAngleInJoggedRadius(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextStyleChanged(const RS_Font* font) const {
    // fixme - sand - complete support of text style
    const auto text = m_dimStyle->text();
    const auto names = font->getNames();
    QString textStyleName = "";
    if (names.empty()) {
        textStyleName = font->getFileName();
    }
    else {
        textStyleName = names.at(0); // fixme - sand - check whether this is correct
    }
    text->setStyle(textStyleName);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextColorChanged(const RS_Color& color) const {
    m_dimStyle->text()->setColor(color);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextFillByBackgroundToggled(const bool b) const {
    m_dimStyle->text()->setBackgroundFillMode(b ? LC_DimStyle::Text::EXPLICIT : LC_DimStyle::Text::NONE);
    ui->cbTextFillColor->setEnabled(b);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextFillColorChanged(const RS_Color& color) const {
    m_dimStyle->text()->setExplicitBackgroundFillColor(color);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextHeightChanged(const double d) const {
    m_dimStyle->text()->setHeight(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextFractionHeightChanged(const double d) const {
    m_dimStyle->latteralTolerance()->setHeightScaleFactorToDimText(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextDrawFrameAround(const bool checked) const {
    const auto dimLine = m_dimStyle->dimensionLine();
    const double newLineGap = dimLine->lineGap();
    const bool lingapNegative = std::signbit(newLineGap);
    if (checked) {
        if (!lingapNegative) {
            dimLine->setLineGap(-newLineGap);
        }
    }
    else if (lingapNegative) {
        dimLine->setLineGap(-newLineGap);
    }
    // update tolerance method, if needed
    if (checked) {
        ui->cbTolMethod->setCurrentIndex(4); // basic
    }
    else {
        if (ui->cbTolMethod->currentIndex() == 4) {
            ui->cbTolMethod->setCurrentIndex(0);
        }
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextPlacementVerticalChanged(int d) const {
    m_dimStyle->text()->setVerticalPositioningRaw(d);
    uiUpdateTextOffsetFromDimLine(static_cast<LC_DimStyle::Text::VerticalPositionPolicy>(d));
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextPlacementHorizontalChanged(const int d) const {
    m_dimStyle->text()->setHorizontalPositioningRaw(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextReadingDirectionChanged(const int d) const {
    m_dimStyle->text()->setReadingDirectionRaw(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextOffsetFromDimLine(const double d) const {
    m_dimStyle->text()->setVerticalDistanceToDimLine(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextAlignmentToggled([[maybe_unused]]bool d) const {
    if (ui->rbTextAlignmentAligned->isChecked()) {
        m_dimStyle->text()->setOrientationInside(LC_DimStyle::Text::ALIGN_WITH_DIM_LINE);
        m_dimStyle->text()->setOrientationOutside(LC_DimStyle::Text::ALIGN_WITH_DIM_LINE);
    }
    else if (ui->rbTextAlignmentHorizontal->isChecked()) {
        m_dimStyle->text()->setOrientationInside(LC_DimStyle::Text::DRAW_HORIZONTALLY);
        m_dimStyle->text()->setOrientationOutside(LC_DimStyle::Text::DRAW_HORIZONTALLY);
    }
    else if (ui->rbTextAlignmentIsoStandard->isChecked()) {
        m_dimStyle->text()->setOrientationInside(LC_DimStyle::Text::ALIGN_WITH_DIM_LINE);
        m_dimStyle->text()->setOrientationOutside(LC_DimStyle::Text::DRAW_HORIZONTALLY);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextFitToggled([[maybe_unused]]bool d) const {
    const auto text = m_dimStyle->text();
    if (ui->rbFitEither->isChecked()) {
        text->setUnsufficientSpacePolicy(LC_DimStyle::Text::EITHER_TEXT_OR_ARROW);
        text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
    }
    else if (ui->rbFirArrows->isChecked()) {
        text->setUnsufficientSpacePolicy(LC_DimStyle::Text::ARROW_FIRST_THEN_TEXT);
        text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
    }
    else if (ui->rbFitText->isChecked()) {
        text->setUnsufficientSpacePolicy(LC_DimStyle::Text::TEXT_FIRST_THEN_ARROW);
        text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
    }
    else if (ui->rbFitBoth->isChecked()) {
        text->setUnsufficientSpacePolicy(LC_DimStyle::Text::OUTSIDE_EXT_LINES);
        text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
    }
    else if (ui->rbFitAlways->isChecked()) {
        text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_ALWAYS_INSIDE);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onFitSuppressArrowsToggled([[maybe_unused]]bool d) const {
    const auto arrowhead = m_dimStyle->arrowhead();
    arrowhead->setSuppressionsRaw(ui->cbFitSuppressArrows->isChecked() ? LC_DimStyle::Arrowhead::SUPPRESS :  LC_DimStyle::Arrowhead::DONT_SUPPRESS);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTextPlacementToggled([[maybe_unused]]bool d) const {
    const auto text = m_dimStyle->text();
    if (ui->rbTextPlacementBeside->isChecked()) {
        text->setPositionMovementPolicy(LC_DimStyle::Text::DIM_LINE_WITH_TEXT);
    } else if (ui->rbTextPlacementOverWLeader->isChecked()) {
        text->setPositionMovementPolicy(LC_DimStyle::Text::ADDS_LEADER);
    } else if (ui->rbTextPlacementOverWoLeader->isChecked()) {
        text->setPositionMovementPolicy(LC_DimStyle::Text::ALLOW_FREE_POSITIONING);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onFitScaleToggled([[maybe_unused]]bool d) const {
    // fixme - sand - dims - complete, define how to set these values!!
    const auto scaling = m_dimStyle->scaling();
    if (ui->rbFitScaleToLayout->isChecked()) {
        scaling->setScale(1.0);
        ui->dsbFitScaleExplicit->setValue(1.0);
    }
    else if (ui->rbFitScaleExplicit->isChecked()) {

    }
    ui->dsbFitScaleExplicit->setEnabled(ui->rbFitScaleExplicit->isChecked());
    refreshPreview();
}

void LC_DlgDimStyleManager::onFitScaleExplicitChanged(const double d) const {
    const auto scaling = m_dimStyle->scaling();
    scaling->setScale(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onFitFineManuallyToggled([[maybe_unused]]bool d) const {
    const auto text = m_dimStyle->text();
    if (ui->cbFitFineManually->isChecked()) {
        text->setCursorControlPolicy( LC_DimStyle::Text::TEXT_AND_DIM_LINE_LOCATION);
    }
    else {
        text->setCursorControlPolicy( LC_DimStyle::Text::DIM_LINE_LOCATION_ONLY);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onFitFineDrawDimlineBetweenToggled([[maybe_unused]]bool d) const {
    const auto dimLine = m_dimStyle->dimensionLine();
    dimLine->setDrawPolicyForOutsideText(ui->cbFitFineDrawDimLineBetwenExt->isChecked() ?
                                             LC_DimStyle::DimensionLine::DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE : LC_DimStyle::DimensionLine::DONT_DRAW_IF_ARROWHEADS_ARE_OUTSIDE);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimUnitFormatIndexChanged(const int index) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setFormatRaw(index+1);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(linear->format(), ui->cbLinearDimPrecision);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(linear->format(), ui->cbTolPrecision);
    uiUpdateLinearFormat(linear->format());
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimPrecisionIndexChanged(const int index) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setDecimalPlaces(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimFractionIndexChanged(const int index) const {
    const auto fractions = m_dimStyle->fractions();
    fractions->setStyleRaw(index);
}

void LC_DlgDimStyleManager::onLinearDimUnitDecimalSeparatorIndexChanged(const int index) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setDecimalFormatSeparatorChar(index == 1 ?  ',' : '.');
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimRoundOffChanged(const double d) const {
    const auto roundOff = m_dimStyle->roundOff();
    roundOff->setRoundToValue(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimPrefixEditingFinished() const {
    const auto linear = m_dimStyle->linearFormat();
    const QString val = ui->leLinearDimPrefix->text();
    LC_DimStyle::LinearFormat::TextPattern* pattern = linear->getPrimaryPrefixOrSuffix();
    pattern->setPrefix(val);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearDimSuffixEditingFinished() const {
    const auto linear = m_dimStyle->linearFormat();
    const QString val = ui->leLinearDimSuffix->text();
    LC_DimStyle::LinearFormat::TextPattern* pattern = linear->getPrimaryPrefixOrSuffix();
    pattern->setSuffix(val);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearScaleFactorChanged(double d) const {
    const auto scale = m_dimStyle->scaling();
    if (ui->cbLinearScaleApplyToLayoutDimsOnly->isChecked()) {
        d = -d; // negative value is used for layout dims
    }
    scale->setLinearFactor(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearScaleApplyToLayoutDimsOnlyToggled(const bool val) const {
    const auto scale = m_dimStyle->scaling();
    double currentVar = std::abs(scale->linearFactor());
    if (val) {
        currentVar = -currentVar;
    }
    scale->setLinearFactor(currentVar); // fixme - sand - dim - test init and change!!
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearZerosSuppressionToggled([[maybe_unused]]bool val) const {
    const auto zerosSuppression = m_dimStyle->zerosSuppression();
    zerosSuppression->clearLinear();
    // linear
    if (ui->cbZerosLeading->isChecked()) {
        zerosSuppression->setLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::LEADING_IN_DECIMAL, true);
    }
    if (ui->cbZerosTrailing->isChecked()) {
        zerosSuppression->setLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::TRAILING_IN_DECIMAL, true);
    }
    //imperial:
    if (ui->cbZeros0Feet->isChecked()) {
        if (!ui->cbZeros0Inch->isChecked()) {
            zerosSuppression->setLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET, true);
        }
    }
    else {
        if (ui->cbZeros0Inch->isChecked()) {
            zerosSuppression->setLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES, true);
        }
        else {
            zerosSuppression->setLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_FEET_AND_ZERO_INCHES, true);
        }
    }

    uiUpdateZerosLeading(ui->cbZerosLeading->isChecked());

    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearUnitFactorChanged([[maybe_unused]]double d) const {
    // fixme - sand - dim - where to store?
    refreshPreview();
}

void LC_DlgDimStyleManager::onLinearUnitPrefixEditingFinished() const {
    // fixme - sand - dim - where to store?
    refreshPreview();
}

void LC_DlgDimStyleManager::onAngularFormatIndexChanged(const int index) const {
    const auto angular = m_dimStyle->angularFormat();
    angular->setFormatRaw(index);

    QG_DlgOptionsDrawing::updateAnglePrecisionCombobox(angular->format(), ui->cbAngularPrecision);
    QG_DlgOptionsDrawing::updateAnglePrecisionCombobox(angular->format(), ui->cbTolAltPrecision);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAngularPrecisionIndexChanged(const int index) const {
    const auto angular = m_dimStyle->angularFormat();
    angular->setDecimalPlaces(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAngularZerosSuppressionToggled([[maybe_unused]]bool d) const {
    const auto zeroSuppress = m_dimStyle->zerosSuppression();
    zeroSuppress->clearAngular();
    if (ui->cbAngularZerosLeading->isChecked()) {
        zeroSuppress->setAngularFlag(LC_DimStyle::ZerosSuppression::AngularSuppressionPolicy::SUPPRESS_LEADING_DECIMAL, true);
    }
    if (ui->cbAngularZerosTrailing->isChecked()) {
        zeroSuppress->setAngularFlag(LC_DimStyle::ZerosSuppression::AngularSuppressionPolicy::SUPPRESS_TRALINING_DECIMAL, true);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateUnitsDisplayToggled(const bool val) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setAlternateUnits(val ? LC_DimStyle::LinearFormat::ENABLE : LC_DimStyle::LinearFormat::DISABLE);
    enableAltUnitsControls(val);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateLinearFormatIndexChanged(const int index) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setAltFormatRaw(index+1);
    const auto unit = linear->altFormat();
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(unit, ui->cbAlternateLinearPrecision);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(unit, ui->cbTolAltPrecision);
    uiUpdateAltLinearFormat(unit);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateLinearPrecisionIndexChanged(const int index) const {
    const auto linear = m_dimStyle->linearFormat();
    linear->setAltDecimalPlaces(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAltMutliplierChanged(const double d) const {
    const auto linearFormat = m_dimStyle->linearFormat();
    linearFormat->setAltUnitsMultiplier(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateRoundToChanged(const double d) const {
    const auto roundOff = m_dimStyle->roundOff();
    roundOff->setAltRoundToValue(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternatePrefixEditingFinished() const {
    const auto linear = m_dimStyle->linearFormat();
    LC_DimStyle::LinearFormat::TextPattern* pattern = linear->getAlternativePrefixOrSuffix();
    pattern->setPrefix(ui->leAlternatePrefix->text());
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateSuffixEditingFinished() const {
    const auto linear = m_dimStyle->linearFormat();
    LC_DimStyle::LinearFormat::TextPattern* pattern = linear->getAlternativePrefixOrSuffix();
    pattern->setSuffix(ui->leAlternateSuffix->text());
    refreshPreview();
}

void LC_DlgDimStyleManager::cbAltZerosSuppressionToggled([[maybe_unused]]bool val) const {
    const auto zerosSuppression = m_dimStyle->zerosSuppression();
    zerosSuppression->clearAltLinear();
    // linear
    if (ui->cbAltZerosLeading->isChecked()) {
        zerosSuppression->setAltLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::LEADING_IN_DECIMAL, true);
    }
    if (ui->cbAltZerosTrailing->isChecked()) {
        zerosSuppression->setAltLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::TRAILING_IN_DECIMAL, true);
    }
    //imperial:
    if (ui->cbAltZeros0Feet->isChecked()) {
        if (!ui->cbAltZeros0Inches->isChecked()) {
            zerosSuppression->setAltLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET, true);
        }
    }
    else {
        if (ui->cbAltZeros0Inches->isChecked()) {
            zerosSuppression->setAltLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES, true);
        }
        else {
            zerosSuppression->setAltLinearFlag(LC_DimStyle::ZerosSuppression::LinearSuppressionPolicy::INCLUDE_ZERO_FEET_AND_ZERO_INCHES, true);
        }
    }

    uiUpdateAltZerosLeading(ui->cbAltZerosLeading->isChecked());
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateSubUnitFactorChanged([[maybe_unused]]double d) const {
    // fixme - sand - dim - where to store?
    refreshPreview();
}

void LC_DlgDimStyleManager::onAlternateSubUnitPrefixEditingFinished() const {
    // fixme - sand - dim - where to store?
    refreshPreview();
}

void LC_DlgDimStyleManager::cbAlternatePlacementToggled([[maybe_unused]]bool val) const {
    const auto linear = m_dimStyle->linearFormat();
    const bool placeAfter = ui->rbAlternatePlacementAfter->isChecked();

    const auto primaryPrefixSuffixPattern = linear->getPrimaryPrefixOrSuffix();
    primaryPrefixSuffixPattern->setSuffixEndsWithNewLineFeed(!placeAfter);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolMethodChangedIndexChanged(const int index) const {
    bool enable = true;
    const auto tol = m_dimStyle->latteralTolerance();
    const auto dimline = m_dimStyle->dimensionLine();
    bool showLowerLimit = true;
    bool showVerticalPosition = false;
    bool additionallyHideToleranceAdjustment = false;

    bool drawFrame = false;

    applyToleranceMethod(tol, dimline, index, enable, showLowerLimit, showVerticalPosition, additionallyHideToleranceAdjustment, drawFrame);

    ui->cbTextDrawFrameAroundText->setChecked(drawFrame);
    uiUpdateToleranceControls(enable,showLowerLimit, showVerticalPosition);
    if (additionallyHideToleranceAdjustment) {
        ui->gbTolAdjustment->setEnabled(false);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolPrecisionIndexChanged(const int index) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setDecimalPlaces(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolUpperLimitChanged(const double d) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setUpperToleranceLimit(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolLowerLimitChangedChanged(const double d) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setLowerToleranceLimit(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolHeightScaleChanged(const double d) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setHeightScaleFactorToDimText(d);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolVerticalPositionIndexChanged(const int index) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setVerticalJustificationRaw(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolMeasurementAlignToggled([[maybe_unused]]bool val) const {
    const auto tol = m_dimStyle->latteralTolerance();
    if (ui->rbTolAjustAlignDecimal->isChecked()) {
        tol->setAdjustment(LC_DimStyle::LatteralTolerance::ALIGN_DECIMAL_SEPARATORS);
    }
    else if (ui->rbTolAjustAlignOpSymbols ->isChecked()) {
        tol->setAdjustment(LC_DimStyle::LatteralTolerance::ALIGN_OPERATIONAL_SYMBOLS);
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolLinearZerosSuppressionToggled([[maybe_unused]]bool val) const {
    const auto zerosSuppression = m_dimStyle->zerosSuppression();
    zerosSuppression->clearTolerance();
    // linear
    if (ui->cbTolAltZerosLeading->isChecked()) {
        zerosSuppression->setToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_LEADING_ZEROS, true);
    }
    if (ui->cbTolZerosTrailing->isChecked()) {
        zerosSuppression->setToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_TRAILING_ZEROS, true);
    }
    //imperial:
    if (ui->cbTolZeros0Feet->isChecked()) {
        if (!ui->cbTolZeros0Inches->isChecked()) {
            zerosSuppression->setToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET, true);
        }
    }
    else {
        if (ui->cbTolZeros0Inches->isChecked()) {
            zerosSuppression->setToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES, true);
        }
        else {
            zerosSuppression->setToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES, true);
        }
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::onTolAltPrecisionIndexChanged(const int index) const {
    const auto tol = m_dimStyle->latteralTolerance();
    tol->setDecimalPlacesAltDim(index);
    refreshPreview();
}

void LC_DlgDimStyleManager::cbTolAlternateZerosSuppressionToggled([[maybe_unused]]bool val) const {
    const auto zerosSuppression = m_dimStyle->zerosSuppression();
    zerosSuppression->clearAltTolerance();
    // linear
    if (ui->cbTolAltZerosLeading->isChecked()) {
        zerosSuppression->setAltToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_LEADING_ZEROS, true);
    }
    if (ui->cbTolZerosTrailing->isChecked()) {
        zerosSuppression->setAltToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_TRAILING_ZEROS, true);
    }
    //imperial:
    if (ui->cbTolZeros0Feet->isChecked()) {
        if (!ui->cbTolZeros0Inches->isChecked()) {
            zerosSuppression->setAltToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET, true);
        }
    }
    else {
        if (ui->cbTolZeros0Inches->isChecked()) {
            zerosSuppression->setAltToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES, true);
        }
        else {
            zerosSuppression->setAltToleranceFlag(LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES, true);
        }
    }
    refreshPreview();
}

void LC_DlgDimStyleManager::disableContainer(const QWidget* tab) {
    auto widgets = tab->findChildren<QWidget*>();
    for (const auto it: std::as_const(widgets)) {
        it->setDisabled(true);
    }
}

void LC_DlgDimStyleManager::setDimStyle(LC_DimStyle* dimStyle) {
    m_dimStyle = dimStyle;

    const auto dimLine = dimStyle->dimensionLine();

    fillLinesTab(dimStyle, dimLine);
    fillArrowsTab(dimStyle, dimLine);
    fillTextTab(dimStyle);
    fillFitTab(dimStyle);
    fillPrimaryUnitTab(dimStyle);
    fillAltUnitTab(dimStyle);
    fillToleranceTab(dimStyle);

    initConnections();
}

void LC_DlgDimStyleManager::createPreviewGraphicView(const RS2::EntityType dimensionType) {
    m_previewView = LC_DimStylePreviewGraphicView::init(this, m_originalGraphic, dimensionType);
    m_previewView->setDimStyle(m_dimStyle);
    m_previewView->setFocusPolicy(Qt::ClickFocus);
}

void LC_DlgDimStyleManager::createPreviewGraphicView(const RS_Dimension* entity) {
    m_previewView = LC_DimStylePreviewGraphicView::init(this, m_originalGraphic, entity);
    m_previewView->setDimStyle(m_dimStyle);
    m_previewView->setFocusPolicy(Qt::ClickFocus);
}

void LC_DlgDimStyleManager::initPreview(const RS2::EntityType dimensionType) {
    createPreviewGraphicView(dimensionType);
    setupPreview();
}

void LC_DlgDimStyleManager::initPreview(const RS_Dimension* entity) {
    createPreviewGraphicView(entity);
    setupPreview();
}

void LC_DlgDimStyleManager::hideFieldsReservedForTheFuture() const {
    // temporary method that should be eliminated later.
    // Some Ui settings are supported by AutoCAD UI, but it seems that they are not directly supported by DXF.
    // potentially, they might be used via Dimension Style override mechanism, or dictionaries...
    // hide them for now, they will be restored later as it will be clear how to handle them.

    // Primary Unit - sub-unit factor and suffix
    ui->bsbLinearZerosSuppressionSubUnitFactor->setVisible(false);
    ui->leLinearZerosSuppressionSubUnitSuffix->setVisible(false);
    ui->lblPrimarySubUnitFactor->setVisible(false);
    ui->lblPrimarySubUnitSuffix->setVisible(false);

    // Alternate Unit - sub-unit factor and suffix
    ui->dbsAlternateSubUnitFactor->setVisible(false);
    ui->leAlternateSubUnitSuffix->setVisible(false);
    ui->lblAltSubUnitFactor->setVisible(false);
    ui->lblAltSubUnitSuffix->setVisible(false);

    ui->gbLinearJog->setVisible(false);
    ui->gbRadiusJog->setVisible(false);

    ui->gbSymbolsDimBreak->setVisible(false);
}

void LC_DlgDimStyleManager::adjustUIForDimensionType(const RS2::EntityType dimensionType) {
    if (dimensionType != RS2::EntityUnknown) {

    }
}

void LC_DlgDimStyleManager::init(const RS2::EntityType dimensionType) {
    ui->cbDimLineColor->init(true, false);
    ui->cbExtLineColor->init(true, false);
    ui->cbTextColor->init(true, false);
    ui->cbTextFillColor->init(true, false);

    ui->cbDimLineWidth->init(true, false);
    ui->cbExtLineWidth->init(true, false);

    ui->cbDimLineLineType->init(true, false);
    ui->cbExtLineType1->init(true, false);
    ui->cbExtLineType2->init(true, false);

    // unit comboboxes

    QG_DlgOptionsDrawing::fillLinearUnitsCombobox(ui->cbLinearDimUnitFormat);
    QG_DlgOptionsDrawing::fillLinearUnitsCombobox(ui->cbAlternateLinearFormat);

    // init angle units combobox:
    QG_DlgOptionsDrawing::fillAngleUnitsCombobox(ui->cbAngularFormat);
    LC_DimArrowRegistry::fillDefaultArrowTypes(m_defaultArrowsInfo);

    for (const LC_DimArrowRegistry::ArrowInfo& arrowInfo : m_defaultArrowsInfo) {
        QString blockName = arrowInfo.blockName.toLower();
        QString iconName = ":/arrows/arrow" + blockName + ".lci";
        ui->cbArrowheadFirst->addItem(QIcon(iconName), arrowInfo.name, arrowInfo.blockName);
        ui->cbArrowheadSecond->addItem(QIcon(iconName), arrowInfo.name, arrowInfo.blockName);
        ui->cbArrowheadLeader->addItem(QIcon(iconName), arrowInfo.name, arrowInfo.blockName);
    }

    const bool hasCustomBlocks = !m_blocksList.isEmpty();

    if (hasCustomBlocks) {
        ui->cbArrowheadFirst->addItem(QIcon(), tr("User Block..."), CUSTOM_SELECT_BLOCK_NAME);
        ui->cbArrowheadSecond->addItem(QIcon(), tr("User Block..."), CUSTOM_SELECT_BLOCK_NAME);
        ui->cbArrowheadLeader->addItem(QIcon(), tr("User Block..."), CUSTOM_SELECT_BLOCK_NAME);
    }

    ui->cbDimTxSty->init();

    hideFieldsReservedForTheFuture();
    adjustUIForDimensionType(dimensionType);
}

void LC_DlgDimStyleManager::initConnections() {
    connectLinesTab();
    connectArrowsTab();
    connectTextTab();
    connectFitTab();
    connectPrimaryUnitTab();
    connectAltUnitTab();
    connectToleranceTab();
}

void LC_DlgDimStyleManager::initBlocksList() {
    const auto blocksList = m_originalGraphic->getBlockList();
    const int blocksCount = blocksList->count();
    if (blocksCount > 0) {
        for (const RS_Block* block: *blocksList) {
            QString blockName = block->getName();
            m_blocksList << blockName;
        }
    }

    m_blocksList.sort();
}

void LC_DlgDimStyleManager::addPreviewProxy(QWidget* proxyMe, QGroupBox* preview) {
    const auto layout = new QHBoxLayout();
    preview->setLayout(layout);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->addWidget(new LC_TabProxyWidget(proxyMe));
}

void LC_DlgDimStyleManager::setupPreview() {
    const auto  previewGroup = new QWidget(this);
    auto* layout = new QVBoxLayout(previewGroup);
    layout->setContentsMargins(0,0,0,0);
    previewGroup->setLayout(layout);

    const auto previewToolbar = new LC_DimStylePreviewPanel(previewGroup);
    previewToolbar->setGraphicView(m_previewView);

    layout->addWidget(previewToolbar);
    layout->addWidget(m_previewView, 10);

    addPreviewProxy(previewGroup, ui->gbPreviewLines);
    addPreviewProxy(previewGroup, ui->gbPreviewArrows);
    addPreviewProxy(previewGroup, ui->gbPreviewText);
    addPreviewProxy(previewGroup, ui->gbPreviewFit);
    addPreviewProxy(previewGroup, ui->gbPreviewPrimary);
    addPreviewProxy(previewGroup, ui->gbPreviewAlt);
    addPreviewProxy(previewGroup, ui->gbPreviewTolerance);
    m_previewView->zoomAuto();
}

void LC_DlgDimStyleManager::languageChange() {
    ui->retranslateUi(this);
}

void LC_DlgDimStyleManager::connectLinesTab() {
    // lines
    connect(ui->cbDimLineColor, &QG_ColorBox::colorChanged, this, &LC_DlgDimStyleManager::onDimLineColorChanged);
    connect(ui->cbDimLineLineType, &QG_LineTypeBox::lineTypeChanged, this, &LC_DlgDimStyleManager::onDimLineTypeChanged);
    connect(ui->cbDimLineWidth, &QG_WidthBox::widthChanged, this, &LC_DlgDimStyleManager::onDimLineWidthChanged);

    connect(ui->dsbDimLineExtendBeyond, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onDimLineExtBeyondChanged);
    connect(ui->dsbBaselineSpacing, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onDimLineBaselineSpacingChanged);

    connect(ui->cbDimLineSuppress1, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onDimLineSuppress1Toggled);
    connect(ui->cbDimLineSuppress2, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onDimLineSuppress2Toggled);

    connect(ui->cbExtLineColor, &QG_ColorBox::colorChanged, this, &LC_DlgDimStyleManager::onExtLineColorChanged);
    connect(ui->cbExtLineType1, &QG_LineTypeBox::lineTypeChanged, this, &LC_DlgDimStyleManager::onExtLineType1Changed);
    connect(ui->cbExtLineType2, &QG_LineTypeBox::lineTypeChanged, this, &LC_DlgDimStyleManager::onExtLineType2Changed);
    connect(ui->cbExtLineWidth, &QG_WidthBox::widthChanged, this, &LC_DlgDimStyleManager::onExtLineWidthChanged);

    connect(ui->bsbExtLineBeyondDimLine, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onExtLineBeyondDimChanged);
    connect(ui->dsbExtLineOffsetFromOrigin, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onExtLineOffsetFromOriginChanged);

    connect(ui->cbExtLineFixedLengthOn, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onExtLineFixedLengthToggled);
    connect(ui->bsbExtLineFixedLength, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onExtLineFixedLengthChanged);

    connect(ui->cbExtLineSuppress1, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onExtLineSuppress1Toggled);
    connect(ui->cbExtLineSuppress2, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onExtLineSuppress2Toggled);
}

void LC_DlgDimStyleManager::connectArrowsTab() {
    connect(ui->dsbTickSize, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onArrowheadTickSizeChanged);
    connect(ui->cbArrowheadTheSame, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onArrowheadTheSameToggled);

    connect(ui->cbArrowheadFirst, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onArrowheadFirstChanged);
    connect(ui->cbArrowheadSecond, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onArrowheadSecondChanged);
    connect(ui->cbArrowheadLeader, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onArrowheadLeaderChanged);

    connect(ui->dsbArrowheadArrowSize, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onArrowheadArrowSizeChanged);
    connect(ui->dsbDimBreakSize, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onDimBreakChanged);

    connect(ui->rbCentermarkNone, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onCenterMarkTypeToggled);
    connect(ui->rbCentermarkMark, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onCenterMarkTypeToggled);
    connect(ui->rbCentermarkLine, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onCenterMarkTypeToggled);

    connect(ui->bsbCentermarkSize, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onCenterMarkSizeChanged);

    connect(ui->rbDimarcSymPreceeding, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onDimarkSymbolToggled);
    connect(ui->rbDimarcSymAbove, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onDimarkSymbolToggled);
    connect(ui->rbDimarcSymNone, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onDimarkSymbolToggled);

    connect(ui->bsbJogHeightFactor, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onJogHeightFactorChanged);
    connect(ui->dsbJogAngle, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onJogAngleChanged);
}

void LC_DlgDimStyleManager::connectTextTab() {
    // fixme - temporary, change to style support!
    connect(ui->cbDimTxSty, &QG_FontBox::fontChanged, this, &LC_DlgDimStyleManager::onTextStyleChanged);

    connect(ui->cbTextColor, &QG_ColorBox::colorChanged, this, &LC_DlgDimStyleManager::onTextColorChanged);

    connect(ui->cbTextFillByBackground, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTextFillByBackgroundToggled);
    connect(ui->cbTextFillColor, &QG_ColorBox::colorChanged, this, &LC_DlgDimStyleManager::onTextFillColorChanged);

    connect(ui->dcbTextHeight, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTextHeightChanged);
    connect(ui->dcbTextFractionHeightScale, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTextFractionHeightChanged);

    connect(ui->cbTextDrawFrameAroundText, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTextDrawFrameAround);

    connect(ui->dsbTextGapFromDimLine, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTextLineGapChanged);

    connect(ui->cbTextPlacementVertical, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTextPlacementVerticalChanged);
    connect(ui->cbTextPlacementHorizontal, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTextPlacementHorizontalChanged);
    connect(ui->cbTextPlacementReadingDirection, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTextReadingDirectionChanged);
    connect(ui->dsbTextOffsetFromDimLine, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTextOffsetFromDimLine);

    connect(ui->rbTextAlignmentHorizontal, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextAlignmentToggled);
    connect(ui->rbTextAlignmentAligned, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextAlignmentToggled);
    connect(ui->rbTextAlignmentIsoStandard, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextAlignmentToggled);
}

void LC_DlgDimStyleManager::connectFitTab() {
    connect(ui->rbFitEither, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextFitToggled);
    connect(ui->rbFirArrows, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextFitToggled);
    connect(ui->rbFitText, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextFitToggled);
    connect(ui->rbFitBoth, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextFitToggled);
    connect(ui->rbFitAlways, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextFitToggled);

    connect(ui->cbFitSuppressArrows, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onFitSuppressArrowsToggled);

    connect(ui->rbTextPlacementBeside, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextPlacementToggled);
    connect(ui->rbTextPlacementOverWLeader, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextPlacementToggled);
    connect(ui->rbTextPlacementOverWoLeader, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTextPlacementToggled);

    connect(ui->rbFitScaleToLayout, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onFitScaleToggled);
    connect(ui->rbFitScaleExplicit, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onFitScaleToggled);

    connect(ui->dsbFitScaleExplicit, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onFitScaleExplicitChanged);

    connect(ui->cbFitFineManually, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onFitFineManuallyToggled);
    connect(ui->cbFitFineDrawDimLineBetwenExt, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onFitFineDrawDimlineBetweenToggled);
}

void LC_DlgDimStyleManager::connectPrimaryUnitTab() {
    connect(ui->cbLinearDimUnitFormat, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onLinearDimUnitFormatIndexChanged);
    connect(ui->cbLinearDimPrecision, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onLinearDimPrecisionIndexChanged);

    connect(ui->cbLinearDimFractionFormat, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onLinearDimFractionIndexChanged);
    connect(ui->cbLinearDimDecimalSeparator, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onLinearDimUnitDecimalSeparatorIndexChanged);

    connect(ui->dsbLinearDimRoundOff, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onLinearDimRoundOffChanged);
    connect(ui->leLinearDimPrefix, &QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onLinearDimPrefixEditingFinished);
    connect(ui->leLinearDimSuffix, &QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onLinearDimSuffixEditingFinished);

    connect(ui->dsbLinearScaleFactor, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onLinearScaleFactorChanged);
    connect(ui->cbLinearScaleApplyToLayoutDimsOnly, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onLinearScaleApplyToLayoutDimsOnlyToggled);

    connect(ui->cbZerosLeading, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onLinearZerosSuppressionToggled);
    connect(ui->cbZerosTrailing, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onLinearZerosSuppressionToggled);
    connect(ui->cbZeros0Feet, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onLinearZerosSuppressionToggled);
    connect(ui->cbZeros0Inch, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onLinearZerosSuppressionToggled);

    connect(ui->bsbLinearZerosSuppressionSubUnitFactor,&QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onLinearUnitFactorChanged);
    connect(ui->leLinearZerosSuppressionSubUnitSuffix,&QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onLinearUnitPrefixEditingFinished);

    connect(ui->cbAngularFormat, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onAngularFormatIndexChanged);
    connect(ui->cbAngularPrecision, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onAngularPrecisionIndexChanged);

    connect(ui->cbAngularZerosLeading, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onAngularZerosSuppressionToggled);
    connect(ui->cbAngularZerosTrailing, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onAngularZerosSuppressionToggled);
}

void LC_DlgDimStyleManager::connectAltUnitTab() {
    connect(ui->cbAlternateUnitsDisplay, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onAlternateUnitsDisplayToggled);

    connect(ui->cbAlternateLinearFormat, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onAlternateLinearFormatIndexChanged);
    connect(ui->cbAlternateLinearPrecision, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onAlternateLinearPrecisionIndexChanged);

    connect(ui->dsbAlternateMutliplier, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onAltMutliplierChanged);
    connect(ui->bsbAlternateRoundTo, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onAlternateRoundToChanged);

    connect(ui->leAlternatePrefix, &QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onAlternatePrefixEditingFinished);
    connect(ui->leAlternateSuffix, &QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onAlternateSuffixEditingFinished);

    connect(ui->cbAltZerosLeading, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAltZerosSuppressionToggled);
    connect(ui->cbAltZerosTrailing, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAltZerosSuppressionToggled);
    connect(ui->cbAltZeros0Feet, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAltZerosSuppressionToggled);
    connect(ui->cbAltZeros0Inches, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAltZerosSuppressionToggled);

    connect(ui->dbsAlternateSubUnitFactor,&QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onAlternateSubUnitFactorChanged);
    connect(ui->leAlternateSubUnitSuffix,&QLineEdit::editingFinished, this, &LC_DlgDimStyleManager::onAlternateSubUnitPrefixEditingFinished);

    connect(ui->rbAlternatePlacementAfter, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAlternatePlacementToggled);
    connect(ui->rbAlternatePlacementBelow, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbAlternatePlacementToggled);
}

void LC_DlgDimStyleManager::connectToleranceTab() {
    connect(ui->cbTolMethod, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTolMethodChangedIndexChanged);

    connect(ui->cbTolPrecision, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTolPrecisionIndexChanged);

    connect(ui->dsbTolUpperLimit, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTolUpperLimitChanged);
    connect(ui->dsbTolLowerLimit, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTolLowerLimitChangedChanged);
    connect(ui->dsbTolHeightScale, &QDoubleSpinBox::valueChanged, this, &LC_DlgDimStyleManager::onTolHeightScaleChanged);

    connect(ui->cbTolVerticalPosition, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTolVerticalPositionIndexChanged);

    connect(ui->rbTolAjustAlignDecimal, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTolMeasurementAlignToggled);
    connect(ui->rbTolAjustAlignOpSymbols, &QRadioButton::toggled, this, &LC_DlgDimStyleManager::onTolMeasurementAlignToggled);

    connect(ui->cbTolZerosLeading, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTolLinearZerosSuppressionToggled);
    connect(ui->cbTolZerosTrailing, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTolLinearZerosSuppressionToggled);
    connect(ui->cbTolZeros0Feet, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTolLinearZerosSuppressionToggled);
    connect(ui->cbTolZeros0Inches, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::onTolLinearZerosSuppressionToggled);

    connect(ui->cbTolAltPrecision, &QComboBox::currentIndexChanged, this, &LC_DlgDimStyleManager::onTolAltPrecisionIndexChanged);

    connect(ui->cbTolAltZerosLeading, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbTolAlternateZerosSuppressionToggled);
    connect(ui->cbTolAltZerosTrailing, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbTolAlternateZerosSuppressionToggled);
    connect(ui->cbTolAltZeros0Feet, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbTolAlternateZerosSuppressionToggled);
    connect(ui->cbTolAltZeros0Inches, &QCheckBox::toggled, this, &LC_DlgDimStyleManager::cbTolAlternateZerosSuppressionToggled);
}

void LC_DlgDimStyleManager::fillLinesTab(const LC_DimStyle* dimStyle, const LC_DimStyle::DimensionLine* dimLine) const {
    ui->cbDimLineColor->setColor(dimLine->color());
    ui->cbDimLineLineType->setLineType(dimLine->lineType());
    ui->cbDimLineWidth->setWidth(dimLine->lineWidth());

    ui->dsbDimLineExtendBeyond->setValue(dimLine->distanceBeyondExtLinesForObliqueStroke());
    ui->dsbBaselineSpacing->setValue(dimLine->baseLineDimLinesSpacing());

    ui->cbDimLineSuppress1->setChecked(dimLine->suppressFirstLine() == LC_DimStyle::DimensionLine::SUPPRESS);
    ui->cbDimLineSuppress2->setChecked(dimLine->suppressSecondLine() == LC_DimStyle::DimensionLine::SUPPRESS);

    const auto extLine = dimStyle->extensionLine();

    ui->cbExtLineColor->setColor(extLine->color());
    ui->cbExtLineType1->setLineType(extLine->lineTypeFirst());
    ui->cbExtLineType2->setLineType(extLine->lineTypeSecond());
    ui->cbExtLineWidth->setWidth(extLine->lineWidth());

    ui->bsbExtLineBeyondDimLine->setValue(extLine->distanceBeyondDimLine());
    ui->dsbExtLineOffsetFromOrigin->setValue(extLine->distanceFromOriginPoint());

    const bool hasFixedLength = extLine->hasFixedLength();
    ui->cbExtLineFixedLengthOn->setChecked(hasFixedLength);
    ui->bsbExtLineFixedLength->setEnabled(hasFixedLength);

    ui->bsbExtLineFixedLength->setValue(extLine->fixedLength());

    ui->cbExtLineSuppress1->setChecked(extLine->suppressFirstLine() == LC_DimStyle::ExtensionLine::SUPPRESS);
    ui->cbExtLineSuppress2->setChecked(extLine->suppressSecondLine() == LC_DimStyle::ExtensionLine::SUPPRESS);
}

void LC_DlgDimStyleManager::setDimGap([[maybe_unused]]LC_DimStyle::DimensionLine* dimLine, const double lineGap) const {
    const bool drawFrame = std::signbit(lineGap); // draw is line gap less than zero
    double val = lineGap;
    if (drawFrame) {
        val = -lineGap;
    }

    ui->dsbTextGapFromDimLine->setValue(val);
}

void LC_DlgDimStyleManager::fillArrowsTab(const LC_DimStyle* dimStyle, [[maybe_unused]]LC_DimStyle::DimensionLine* dimLine) const {
    const auto arrows = dimStyle->arrowhead();

    const bool sameArrowHead = !arrows->isUseSeparateArrowHeads();

    ui->cbArrowheadTheSame->setChecked(sameArrowHead);
    ui->cbArrowheadSecond->setEnabled(!sameArrowHead);

    const auto firstArrow = arrows->arrowHeadBlockNameFirst();

    setArrowComboboxValue( ui->cbArrowheadFirst, firstArrow);
    if (sameArrowHead) {
        setArrowComboboxValue( ui->cbArrowheadSecond, firstArrow);
    }
    else {
        setArrowComboboxValue( ui->cbArrowheadSecond, arrows->arrowHeadBlockNameSecond());
    }

    setArrowComboboxValue( ui->cbArrowheadLeader, dimStyle->leader()->arrowBlockName());

    const double arrowSize = arrows->size();
    ui->dsbArrowheadArrowSize->setValue(arrowSize);

    const double tickSize = arrows->tickSize();
    ui->dsbTickSize->setValue(tickSize);
    uiUpdateArrowsControlsByTickSize(tickSize);


    const auto radial = dimStyle->radial();
    switch (radial->drawingMode()) {
        case LC_DimStyle::Radial::DRAW_NOTHING: {
            ui->rbCentermarkNone->setChecked(true);
            ui->bsbCentermarkSize->setEnabled(false);
            break;
        }
        case LC_DimStyle::Radial::DRAW_CENTERLINES: {
            ui->rbCentermarkLine->setChecked(true);
            break;
        }
        case LC_DimStyle::Radial::DRAW_CENTERMARKS: {
            ui->rbCentermarkMark->setChecked(true);
            break;
        }
        default:
            break;
    }

    ui->bsbCentermarkSize->setValue(radial->size());

    const auto arc = dimStyle->arc();

    switch (arc->arcSymbolPosition()) {
        case LC_DimStyle::Arc::NONE: {
            ui->rbDimarcSymNone->setChecked(true);
            break;
        }
        case LC_DimStyle::Arc::ABOVE: {
            ui->rbDimarcSymAbove->setChecked(true);
            break;
        }
        case LC_DimStyle::Arc::BEFORE: {
            ui->rbDimarcSymPreceeding->setChecked(true);
            break;
        }
    }

    // fixme - jog height ???

    const double joggedRadius = radial->transverseSegmentAngleInJoggedRadius();
    ui->dsbJogAngle->setValue(joggedRadius);
}

void LC_DlgDimStyleManager::fillTextTab(const LC_DimStyle* dimStyle) const {
    const auto text = dimStyle->text();
    // fixme - rework to normal text styles support!!
    ui->cbDimTxSty->setFont(text->style());

    ui->cbTextColor->setColor(text->color());
    const bool fillByBackground = text->backgroundFillMode() == LC_DimStyle::Text::EXPLICIT;

    ui->cbTextFillByBackground->setChecked(fillByBackground);
    ui->cbTextFillColor->setEnabled(fillByBackground);

    ui->cbTextFillColor->setColor(text->explicitBackgroundFillColor());

    ui->dcbTextHeight->setValue(text->height());

    const auto tolerance = dimStyle->latteralTolerance(); // fixme - sand - check that this correct
    ui->dcbTextFractionHeightScale->setValue(tolerance->heightScaleFactorToDimText());

    const auto verticalPositioning = text->verticalPositioning();
    ui->cbTextPlacementVertical->setCurrentIndex(verticalPositioning);
    ui->cbTextPlacementHorizontal->setCurrentIndex(text->horizontalPositioning());
    ui->cbTextPlacementReadingDirection->setCurrentIndex(text->readingDirection());

    uiUpdateTextOffsetFromDimLine(verticalPositioning);
    ui->dsbTextOffsetFromDimLine->setValue(text->verticalDistanceToDimLine());

    // https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-38DAEEF0-AB44-4C58-8D0E-955BFCFF71A7
    // ui->dsbTextOffsetFromDimLine->setValue(dimline->lineGap());

    const auto dimline = dimStyle->dimensionLine();
    const double lineGap = dimline->lineGap();
    const bool drawFrame = std::signbit(lineGap); // draw is line gap less than zero
    ui->cbTextDrawFrameAroundText->setChecked(drawFrame);

    setDimGap(dimline, lineGap);

    switch (text->orientationInside()) {
        case LC_DimStyle::Text::DRAW_HORIZONTALLY: {
            ui->rbTextAlignmentHorizontal->setChecked(true);
            break;
        }
        case LC_DimStyle::Text::ALIGN_WITH_DIM_LINE: {
            const auto orientationOutside = text->orientationOutside();
            if (orientationOutside == LC_DimStyle::Text::DRAW_HORIZONTALLY) {
                ui->rbTextAlignmentIsoStandard->setChecked(true);
            }
            else {
                ui->rbTextAlignmentAligned->setChecked(true);
            }
            break;
        }
    }
}

void LC_DlgDimStyleManager::setArrowComboboxValue(QComboBox* arrowComboBox, const QString& arrowBlockName) const {
    arrowComboBox->blockSignals(true);
    int existingItemIndex = 0;
    if (arrowBlockName.isEmpty()) {
        // first item in the list should be _CLOSEDFILLED, that corresponds as default to "" block name.
        existingItemIndex = 0;
    }
    else {
        existingItemIndex = arrowComboBox->findData(QVariant(arrowBlockName.toUpper()));
    }
    if (existingItemIndex == -1) {
        // this is custom block from the drawing, so we'll need to add another item into combobox for it
        const int customBlockIndex = m_defaultArrowsInfo.size();
        arrowComboBox->insertItem(customBlockIndex, QIcon(), arrowBlockName, arrowBlockName);
        arrowComboBox->setCurrentIndex(customBlockIndex);
    }
    else {
        arrowComboBox->setCurrentIndex(existingItemIndex);
    }
    arrowComboBox->blockSignals(false);
}

void LC_DlgDimStyleManager::uiUpdateArrowsControlsByTickSize(const double tickSize) const {
    const bool drawObliqueTicks = LC_LineMath::isMeaningful(tickSize);

    ui->cbArrowheadTheSame->setEnabled(!drawObliqueTicks);
    ui->lblArrowheadFirst->setEnabled(!drawObliqueTicks);
    ui->cbArrowheadFirst->setEnabled(!drawObliqueTicks);
    ui->lblArrowheadSecond->setEnabled(!drawObliqueTicks);
    ui->cbArrowheadSecond->setEnabled(!drawObliqueTicks && !ui->cbArrowheadTheSame->isChecked());
    ui->lblArrowheadSize->setEnabled(!drawObliqueTicks);
    ui->dsbArrowheadArrowSize->setEnabled(!drawObliqueTicks);
}

void LC_DlgDimStyleManager::uiUpdateLinearFormat(const RS2::LinearFormat format) const {
    switch (format) {
        case RS2::Scientific:
        case RS2::Decimal:
        case RS2::ArchitecturalMetric:
        case RS2::Engineering:
            ui->cbLinearDimFractionFormat->setEnabled(false);
            ui->dcbTextFractionHeightScale->setEnabled(false);
            break;
        default:
            ui->cbLinearDimFractionFormat->setEnabled(true);
            ui->dcbTextFractionHeightScale->setEnabled(true);
            break;
    }
    ui->cbLinearDimDecimalSeparator->setEnabled(format == RS2::Decimal);

    const bool imperialEnabled = format == RS2::Engineering || format == RS2::Fractional || format == RS2::Architectural;
    ui->cbZeros0Inch->setEnabled(imperialEnabled);
    ui->cbZeros0Feet->setEnabled(imperialEnabled);
    ui->cbTolZeros0Inches->setEnabled(imperialEnabled);
    ui->cbTolZeros0Feet->setEnabled(imperialEnabled);

    if (!imperialEnabled) {
        ui->cbZeros0Inch->setChecked(false);
        ui->cbZeros0Feet->setChecked(false);
        ui->cbTolZeros0Inches->setChecked(false);
        ui->cbTolZeros0Feet->setChecked(false);
    }

    const bool leadingTrailingEnabled = format != RS2::Architectural;
    ui->cbZerosLeading->setEnabled(leadingTrailingEnabled);
    ui->cbZerosTrailing->setEnabled(leadingTrailingEnabled);
    ui->cbTolZerosLeading->setEnabled(leadingTrailingEnabled);
    ui->cbTolZerosTrailing->setEnabled(leadingTrailingEnabled);

    if (!leadingTrailingEnabled) {
        ui->cbZerosLeading->setChecked(false);
        ui->cbZerosTrailing->setChecked(false);
        ui->cbTolZerosLeading->setChecked(false);
        ui->cbTolZerosTrailing->setChecked(false);
    }
}

void LC_DlgDimStyleManager::uiUpdateTextOffsetFromDimLine(const LC_DimStyle::Text::VerticalPositionPolicy verticalPositioning) const {
    const bool textIsNotCenteredVertically = verticalPositioning != LC_DimStyle::Text::CENTER_BETWEEN_EXT_LINES;
    ui->dsbTextOffsetFromDimLine->setEnabled(textIsNotCenteredVertically);
    ui->lblTextOffsetFromDimLine->setEnabled(textIsNotCenteredVertically);
}

void LC_DlgDimStyleManager::uiUpdateAltLinearFormat(const RS2::LinearFormat format) const {
    const bool imperialEnabled = format == RS2::Engineering || format == RS2::Fractional || format == RS2::Architectural;
    ui->cbAltZeros0Inches->setEnabled(imperialEnabled);
    ui->cbAltZeros0Feet->setEnabled(imperialEnabled);
    ui->cbTolAltZeros0Inches->setEnabled(imperialEnabled);
    ui->cbTolAltZeros0Feet->setEnabled(imperialEnabled);

    if (!imperialEnabled) {
        ui->cbAltZeros0Inches->setChecked(false);
        ui->cbAltZeros0Feet->setChecked(false);
        ui->cbTolAltZeros0Inches->setChecked(false);
        ui->cbTolAltZeros0Feet->setChecked(false);
    }

    const bool leadingTrailingEnabled = format != RS2::Architectural;
    ui->cbAltZerosLeading->setEnabled(leadingTrailingEnabled);
    ui->cbAltZerosTrailing->setEnabled(leadingTrailingEnabled);
    ui->cbTolAltZerosLeading->setEnabled(leadingTrailingEnabled);
    ui->cbTolAltZerosTrailing->setEnabled(leadingTrailingEnabled);

    if (!leadingTrailingEnabled) {
        ui->cbAltZerosLeading->setChecked(false);
        ui->cbAltZerosTrailing->setChecked(false);
        ui->cbTolAltZerosLeading->setChecked(false);
        ui->cbTolAltZerosTrailing->setChecked(false);
    }
}

void LC_DlgDimStyleManager::uiUpdateZerosLeading(const bool suppressLeading) const {
    ui->lblPrimarySubUnitFactor->setEnabled(suppressLeading);
    ui->lblPrimarySubUnitSuffix->setEnabled(suppressLeading);
    ui->bsbLinearZerosSuppressionSubUnitFactor->setEnabled(suppressLeading);
    ui->leLinearZerosSuppressionSubUnitSuffix->setEnabled(suppressLeading);
}

void LC_DlgDimStyleManager::uiUpdateAltZerosLeading(const bool suppressLeading) const {
    ui->lblAltSubUnitFactor->setEnabled(suppressLeading);
    ui->lblAltSubUnitSuffix->setEnabled(suppressLeading);
    ui->dbsAlternateSubUnitFactor->setEnabled(suppressLeading);
    ui->leAlternateSubUnitSuffix->setEnabled(suppressLeading);
}

void LC_DlgDimStyleManager::fillFitTab(const LC_DimStyle* dimStyle) const {
    const auto text = dimStyle->text();

    // fit box with several radio buttons is actually created based on values of several variables...
    // DIMATFIT, DIMTIX
    bool setByDIMATFIT = false;
    switch (text->unsufficientSpacePolicy()) {
        case LC_DimStyle::Text::EITHER_TEXT_OR_ARROW: {
            ui->rbFitEither->setChecked(true);
            setByDIMATFIT = true;
            break;
        }
        case LC_DimStyle::Text::ARROW_FIRST_THEN_TEXT: {
            ui->rbFirArrows->setChecked(true);
            setByDIMATFIT = true;
            break;
        }
        case LC_DimStyle::Text::TEXT_FIRST_THEN_ARROW: {
            ui->rbFitText->setChecked(true);
            setByDIMATFIT = true;
            break;
        }
        case LC_DimStyle::Text::OUTSIDE_EXT_LINES: {
            ui->rbFitBoth->setChecked(true);
            setByDIMATFIT = true;
            break;
        }
    }

    if (!setByDIMATFIT) {
        if (text->extLinesRelativePlacement() == LC_DimStyle::Text::PLACE_ALWAYS_INSIDE) {
            ui->rbFitAlways->setChecked(true);
        }
    }

    const auto arrowhead = dimStyle->arrowhead();

    ui->cbFitSuppressArrows->setChecked(arrowhead->suppression() == LC_DimStyle::Arrowhead::SUPPRESS);

    switch (text->positionMovementPolicy()) {
        case LC_DimStyle::Text::DIM_LINE_WITH_TEXT: {
            ui->rbTextPlacementBeside->setChecked(true);
            break;
        }
        case LC_DimStyle::Text::ADDS_LEADER: {
            ui->rbTextPlacementOverWLeader->setChecked(true);
            break;
        }
        case LC_DimStyle::Text::ALLOW_FREE_POSITIONING: {
            ui->rbTextPlacementOverWoLeader->setChecked(true);
            break;
        }
    }

    const auto scaling = dimStyle->scaling();
    const double scale = scaling->scale();
    ui->dsbFitScaleExplicit->setValue(scale);

    if (LC_LineMath::isNotMeaningful(scale - 1.0)) {
        ui->rbFitScaleToLayout->setChecked(true);
        ui->dsbFitScaleExplicit->setEnabled(false);
    }
    else {
        ui->rbFitScaleExplicit->setChecked(true);
        ui->dsbFitScaleExplicit->setEnabled(true);
    }

    // fixme - sand - dim - review UI and doc for DIMUPT var
    ui->cbFitFineManually->setChecked(text->cursorControlPolicy() == LC_DimStyle::Text::TEXT_AND_DIM_LINE_LOCATION);

    const auto dimLine = dimStyle->dimensionLine();
    ui->cbFitFineDrawDimLineBetwenExt->setChecked(
        dimLine->drawPolicyForOutsideText() == LC_DimStyle::DimensionLine::DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE);
}

void LC_DlgDimStyleManager::fillPrimaryUnitTab(const LC_DimStyle* dimStyle) const {
    const auto linear = dimStyle->linearFormat();

    const auto linearFormat = linear->format();
    ui->cbLinearDimUnitFormat->setCurrentIndex(linearFormat);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(linearFormat, ui->cbLinearDimPrecision);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(linearFormat, ui->cbTolPrecision);

    uiUpdateLinearFormat(linearFormat);

    ui->cbLinearDimPrecision->setCurrentIndex(linear->decimalPlaces());

    const auto fractions = dimStyle->fractions();
    ui->cbLinearDimFractionFormat->setCurrentIndex(fractions->style());

    const int dimSeparator = linear->decimalFormatSeparatorChar();
    const int dimSepIndex = dimSeparator == 44 ? 1 : 0;
    ui->cbLinearDimDecimalSeparator->setCurrentIndex(dimSepIndex);

    const auto roundOff = dimStyle->roundOff();
    ui->dsbLinearDimRoundOff->setValue(roundOff->roundTo());

    LC_DimStyle::LinearFormat::TextPattern* pattern = linear->getPrimaryPrefixOrSuffix();
    ui->leLinearDimPrefix->setText(pattern->getPrefix());
    ui->leLinearDimSuffix->setText(pattern->getSuffix());

    const auto scaling = dimStyle->scaling();

    ui->dsbLinearScaleFactor->setValue(scaling->linearFactor());
    // fixme- sand -  where it's stored?
    ui->cbLinearScaleApplyToLayoutDimsOnly->setChecked(false); // fixme - setup !!!!

    const auto zerosSuppression = dimStyle->zerosSuppression();
    bool feetSuppress = false;
    bool inchesSuppress = false;

    // a kind of magic logic port from LG_DimZerosBox...
    if (zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
        if (zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    } else {
        inchesSuppress = true;
        if (!zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    }

    const bool linearLeadingSuppress = zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::LEADING_IN_DECIMAL);
    const bool linearTrailingSuppress = zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::TRAILING_IN_DECIMAL);
    ui->cbZerosLeading->setChecked(linearLeadingSuppress);
    ui->cbZerosTrailing->setChecked(linearTrailingSuppress);
    ui->cbZeros0Feet->setChecked(feetSuppress);
    ui->cbZeros0Inch->setChecked(inchesSuppress);

    uiUpdateZerosLeading(linearLeadingSuppress);

    // fixme - sand - dims - how to setup? From which vars?
    const QString subUnitSuffix = "";
    const double subUnitFactor = 1.0;
    ui->bsbLinearZerosSuppressionSubUnitFactor->setValue(subUnitFactor);
    ui->leLinearZerosSuppressionSubUnitSuffix->setText(subUnitSuffix);

    const auto angular = dimStyle->angularFormat();

    ui->cbAngularFormat->setCurrentIndex(angular->format());
    QG_DlgOptionsDrawing::updateAnglePrecisionCombobox(angular->format(), ui->cbAngularPrecision);
    ui->cbAngularPrecision->setCurrentIndex(angular->decimalPlaces());

    const bool angularLeadingSuppress = zerosSuppression->isAngularSuppress(LC_DimStyle::ZerosSuppression::AngularSuppressionPolicy::SUPPRESS_LEADING_DECIMAL);
    const bool angularTrailingSuppress = zerosSuppression->isAngularSuppress(LC_DimStyle::ZerosSuppression::AngularSuppressionPolicy::SUPPRESS_TRALINING_DECIMAL);
    ui->cbAngularZerosLeading->setChecked(angularLeadingSuppress);
    ui->cbAngularZerosTrailing->setChecked(angularTrailingSuppress);
}

void LC_DlgDimStyleManager::fillAltUnitTab(const LC_DimStyle* dimStyle) const {
    const auto linearFormat = dimStyle->linearFormat();
    const bool altUnitsEnabled = linearFormat->alternateUnits() == LC_DimStyle::LinearFormat::ENABLE;

    ui->cbAlternateUnitsDisplay->setChecked(altUnitsEnabled);
    const auto altFormat = linearFormat->altFormat();
    ui->cbAlternateLinearFormat->setCurrentIndex(altFormat);

    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(altFormat, ui->cbAlternateLinearPrecision);
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(altFormat, ui->cbTolAltPrecision);

    uiUpdateAltLinearFormat(altFormat);

    ui->cbAlternateLinearPrecision->setCurrentIndex(linearFormat->altDecimalPlaces());

    ui->dsbAlternateMutliplier->setValue(linearFormat->altUnitsMultiplier());

    const auto roundOff = dimStyle->roundOff();
    ui->bsbAlternateRoundTo->setValue(roundOff->altRoundTo());

    LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
    ui->leAlternatePrefix->setText(pattern->getPrefix());
    ui->leAlternateSuffix->setText(pattern->getSuffix());

    const auto zerosSuppression = dimStyle->zerosSuppression();

    bool feetSuppress{false};
    bool inchesSuppress{false};
    // a kind of magic logic port from LG_DimZerosBox...
    if (zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
        if (zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    } else {
        inchesSuppress = true;
        if (!zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    }

    const bool linearLeadingSuppress = zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_LEADING_ZEROS);
    const bool linearTrailingSuppress = zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_TRAILING_ZEROS);

    ui->cbAltZerosLeading->setChecked(linearLeadingSuppress);
    ui->cbAltZerosTrailing->setChecked(linearTrailingSuppress);
    ui->cbAltZeros0Feet->setChecked(feetSuppress);
    ui->cbAltZeros0Inches->setChecked(inchesSuppress);

    uiUpdateAltZerosLeading(linearLeadingSuppress);

    // fixme - sand - dims - how to setup? From which vars?
    const QString subUnitSuffix = "";
    const double subUnitFactor = 1.0;

    ui->dbsAlternateSubUnitFactor->setValue(subUnitFactor);
    ui->leAlternateSubUnitSuffix->setText(subUnitSuffix);

    const LC_DimStyle::LinearFormat::TextPattern* primaryPattern = linearFormat->getPrimaryPrefixOrSuffix();

    const bool altPlacementBelow = primaryPattern->isSuffixEndsWithLineFeed();

    ui->rbAlternatePlacementAfter->setChecked(!altPlacementBelow);
    ui->rbAlternatePlacementBelow->setChecked(altPlacementBelow);

    enableAltUnitsControls(altUnitsEnabled);
}

void LC_DlgDimStyleManager::fillToleranceTab(const LC_DimStyle* dimStyle) const {
    const auto tolerance = dimStyle->latteralTolerance();

    bool enable = false;
    bool showVerticalPosition = false;
    bool showLowerLimit = false, showUpperLimit = false;
    const int tolMethod = computeToleranceMethod(dimStyle,  tolerance, enable, showVerticalPosition, showLowerLimit, showUpperLimit);

    uiUpdateToleranceControls(enable,showLowerLimit, showVerticalPosition);

    ui->cbTolMethod->setCurrentIndex(tolMethod); // basic
    const auto linear = dimStyle->linearFormat();
    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(linear->format(), ui->cbTolPrecision);
    ui->cbTolPrecision->setCurrentIndex(tolerance->decimalPlaces());

    ui->dsbTolUpperLimit->setValue(tolerance->upperToleranceLimit());
    ui->dsbTolLowerLimit->setValue(tolerance->lowerToleranceLimit());

    ui->dsbTolHeightScale->setValue(tolerance->heightScaleFactorToDimText());
    ui->cbTolVerticalPosition->setCurrentIndex(tolerance->verticalJustification());

    const bool alignDecimals = tolerance->adjustment() == LC_DimStyle::LatteralTolerance::ALIGN_DECIMAL_SEPARATORS;

    ui->rbTolAjustAlignDecimal->setChecked(alignDecimals);
    ui->rbTolAjustAlignOpSymbols->setChecked(!alignDecimals);

    const auto zerosSuppression = dimStyle->zerosSuppression();

    bool feetSuppress{false};
    bool inchesSuppress{false};
    // a kind of magic logic port from LG_DimZerosBox...
    if (zerosSuppression->isToleranceSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
        if (zerosSuppression->isToleranceSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    } else {
        inchesSuppress = true;
        if (!zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    }

    bool linearLeadingSuppress = zerosSuppression->isToleranceSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_LEADING_ZEROS);
    bool linearTrailingSuppress = zerosSuppression->isToleranceSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_TRAILING_ZEROS);

    ui->cbTolZerosLeading->setChecked(linearLeadingSuppress);
    ui->cbTolZerosTrailing->setChecked(linearTrailingSuppress);
    ui->cbTolZeros0Feet->setChecked(feetSuppress);
    ui->cbTolZeros0Inches->setChecked(inchesSuppress);

    QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(static_cast<RS2::LinearFormat>(ui->cbAlternateLinearFormat->currentIndex()), ui->cbTolAltPrecision); // fixme - connect for update
    ui->cbTolAltPrecision->setCurrentIndex(tolerance->decimalPlacesAltDim());

    if (zerosSuppression->isAltToleranceSuppress(LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
        if (zerosSuppression->isAltToleranceSuppress(LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    } else {
        inchesSuppress = true;
        if (!zerosSuppression->isAltToleranceSuppress(LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
            feetSuppress = true;
        }
    }

    linearLeadingSuppress = zerosSuppression->isAltToleranceSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_LEADING_ZEROS);
    linearTrailingSuppress = zerosSuppression->isAltToleranceSuppress(LC_DimStyle::ZerosSuppression::SUPPRESS_TRAILING_ZEROS);

    ui->cbTolAltZerosLeading->setChecked(linearLeadingSuppress);
    ui->cbTolAltZerosTrailing->setChecked(linearTrailingSuppress);
    ui->cbTolAltZeros0Feet->setChecked(feetSuppress);
    ui->cbTolAltZeros0Inches->setChecked(inchesSuppress);
}

void LC_DlgDimStyleManager::enableAltUnitsControls(const bool enable) const {
    ui->gbAltUnits->setEnabled(enable);
    ui->grpTolAltUnit->setEnabled(enable);
}

void LC_DlgDimStyleManager::uiUpdateToleranceControls(const bool enable, const bool showLowerLimit, const bool showVerticalPosition) const {
    ui->cbTolPrecision->setEnabled(enable);
    ui->dsbTolUpperLimit->setEnabled(enable);
    ui->dsbTolLowerLimit->setEnabled(enable & showLowerLimit);
    ui->dsbTolHeightScale->setEnabled(enable);
    ui->cbTolVerticalPosition->setEnabled(enable & showVerticalPosition);
    ui->gbTolAdjustment->setEnabled(enable);
    ui->gbTolZeros->setEnabled(enable);
    ui->grpTolAltUnit->setEnabled(enable);
}
