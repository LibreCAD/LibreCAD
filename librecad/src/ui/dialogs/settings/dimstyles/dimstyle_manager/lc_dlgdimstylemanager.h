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

#ifndef LC_DLGDIMSTYLEMANAGER_H
#define LC_DLGDIMSTYLEMANAGER_H


#include "lc_dialog.h"
#include "lc_dimarrowregistry.h"
#include "lc_dimstyle.h"

class LC_DimStylePreviewGraphicView;
class QGroupBox;
class QComboBox;
class QG_GraphicView;
class RS_Font;
class LC_DimStyle;

namespace Ui {
    class LC_DlgDimStyleManager;
}

class LC_DlgDimStyleManager : public LC_Dialog{
    Q_OBJECT
public:
    LC_DlgDimStyleManager(QWidget *parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic, RS2::EntityType dimensionType);
    LC_DlgDimStyleManager(QWidget *parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic, const RS_Dimension* entity, const QString& baseStyleName);
    ~LC_DlgDimStyleManager() override;
    LC_DimStyle*  getDimStyle() const {return m_dimStyle;}
    void addDimStyle(LC_DimStyle* dimStyle) const;
    void refreshPreview() const;
    void resizeEvent(QResizeEvent*) override;
    void setReadOnly();

    static int computeToleranceMethod(const LC_DimStyle* dimStyle, LC_DimStyle::LatteralTolerance* tolerance,
                                      bool& enable, bool& showVerticalPosition, bool& showLowerLimit, bool& showUpperLimit);

    static void applyToleranceMethod(LC_DimStyle::LatteralTolerance* tol, LC_DimStyle::DimensionLine* dimLine,int index, bool& enable, bool& showLowerLimit,
                              bool& showVerticalPosition, bool& additionallyHideToleranceAdjustment, bool& drawFrame);
protected slots:
    // lines tab slots
    void onDimLineColorChanged(const RS_Color& color) const;
    void onDimLineTypeChanged(RS2::LineType) const;
    void onDimLineWidthChanged(RS2::LineWidth) const;
    void onDimLineExtBeyondChanged(double) const;
    void onDimLineBaselineSpacingChanged(double) const;
    void onDimLineSuppress1Toggled(bool val) const;
    void onDimLineSuppress2Toggled(bool val) const;
    void onExtLineColorChanged(const RS_Color& color) const;
    void onExtLineType1Changed(RS2::LineType) const;
    void onExtLineType2Changed(RS2::LineType) const;
    void onExtLineWidthChanged(RS2::LineWidth) const;
    void onExtLineBeyondDimChanged(double) const;
    void onExtLineOffsetFromOriginChanged(double) const;
    void onExtLineFixedLengthToggled(bool val) const;
    void onExtLineFixedLengthChanged(double) const;
    void onExtLineSuppress1Toggled(bool val) const;
    void onExtLineSuppress2Toggled(bool val) const;
    // arrows tab slots
    void onArrowheadTheSameToggled(bool val) const;
    void setCustomArrowBlockName(QComboBox* arrowComboBox, const QString& dimblk1, int customBlockIndex, const QString& extData);
    bool processArrowComboboxChange(QComboBox* arrowComboBox, QString& dimblk1);
    void onArrowheadFirstChanged(int index);
    void onArrowheadSecondChanged(int index);
    void onArrowheadLeaderChanged(int index);
    void onArrowheadArrowSizeChanged(double d) const;
    void onDimBreakChanged(double d) const;
    void onArrowheadTickSizeChanged(double d) const;
    void onTextLineGapChanged(double d) const;
    void onCenterMarkTypeToggled(bool val) const;
    void setCentermarkSize(double d) const;
    void onCenterMarkSizeChanged(double d) const;
    void onDimarkSymbolToggled(bool val) const;
    void onJogHeightFactorChanged(double d) const;
    void onJogAngleChanged(double d) const;
    // text tab slots
    void onTextStyleChanged(const RS_Font* font) const;
    void onTextColorChanged(const RS_Color& color) const;
    void onTextFillByBackgroundToggled(bool b) const;
    void onTextFillColorChanged(const RS_Color& color) const;
    void onTextHeightChanged(double d) const;
    void onTextFractionHeightChanged(double d) const;
    void onTextDrawFrameAround(bool checked) const;
    void onTextPlacementVerticalChanged(int d) const;
    void onTextPlacementHorizontalChanged(int d) const;
    void onTextReadingDirectionChanged(int d) const;
    void onTextOffsetFromDimLine(double d) const;
    void onTextAlignmentToggled(bool d) const;
    // fit tab slots
    void onTextFitToggled(bool d) const;
    void onFitSuppressArrowsToggled(bool d) const;
    void onTextPlacementToggled(bool d) const;
    void onFitScaleToggled(bool d) const;
    void onFitScaleExplicitChanged(double d) const;
    void onFitFineManuallyToggled(bool d) const;
    void onFitFineDrawDimlineBetweenToggled(bool d) const;
    // primaryUnit tab slots
    void onLinearDimUnitFormatIndexChanged(int index) const;
    void onLinearDimPrecisionIndexChanged(int index) const;
    void onLinearDimFractionIndexChanged(int index) const;
    void onLinearDimUnitDecimalSeparatorIndexChanged(int index) const;
    void onLinearDimRoundOffChanged(double d) const;
    void onLinearDimPrefixEditingFinished() const;
    void onLinearDimSuffixEditingFinished() const;
    void onLinearScaleFactorChanged(double d) const;
    void onLinearScaleApplyToLayoutDimsOnlyToggled(bool val) const;
    void onLinearZerosSuppressionToggled(bool val) const;
    void onLinearUnitFactorChanged(double d) const;
    void onLinearUnitPrefixEditingFinished() const;
    void onAngularFormatIndexChanged(int index) const;
    void onAngularPrecisionIndexChanged(int index) const;
    void onAngularZerosSuppressionToggled(bool d) const;
    // Alt Unit tab slots
    void onAlternateUnitsDisplayToggled(bool val) const;
    void onAlternateLinearFormatIndexChanged(int index) const;
    void onAlternateLinearPrecisionIndexChanged(int index) const;
    void onAltMutliplierChanged(double d) const;
    void onAlternateRoundToChanged(double d) const;
    void onAlternatePrefixEditingFinished() const;
    void onAlternateSuffixEditingFinished() const;
    void cbAltZerosSuppressionToggled(bool val) const;
    void onAlternateSubUnitFactorChanged(double d) const;
    void onAlternateSubUnitPrefixEditingFinished() const;
    void cbAlternatePlacementToggled(bool val) const;
    // Tolerance tab slots
    void onTolMethodChangedIndexChanged(int index) const;
    void onTolPrecisionIndexChanged(int index) const;
    void onTolUpperLimitChanged(double d) const;
    void onTolLowerLimitChangedChanged(double d) const;
    void onTolHeightScaleChanged(double d) const;
    void onTolVerticalPositionIndexChanged(int index) const;
    void onTolMeasurementAlignToggled(bool val) const;
    void onTolLinearZerosSuppressionToggled(bool val) const;
    void onTolAltPrecisionIndexChanged(int index) const;
    void cbTolAlternateZerosSuppressionToggled(bool val) const;
    void disableContainer(const QWidget* tab);
private:
    void setDimStyle(LC_DimStyle *dimStyle);
    void createPreviewGraphicView(RS2::EntityType dimensionType);
    void createPreviewGraphicView(const RS_Dimension* entity);
    void initPreview(RS2::EntityType dimensionType);
    void initPreview(const RS_Dimension* entity);
    void hideFieldsReservedForTheFuture() const;
    void adjustUIForDimensionType(RS2::EntityType dimensionType);
    void init(RS2::EntityType dimensionType);
    void initConnections();
    void initBlocksList();
    void addPreviewProxy(QWidget* proxyMe, QGroupBox* preview);
    void setupPreview();
    void languageChange();
    void connectLinesTab();
    void connectArrowsTab();
    void connectTextTab();
    void connectFitTab();
    void connectPrimaryUnitTab();
    void connectAltUnitTab();
    void connectToleranceTab();
    void fillLinesTab(const LC_DimStyle* dimStyle, const LC_DimStyle::DimensionLine* dimLine) const;
    void setDimGap(LC_DimStyle::DimensionLine* dimLine, double lineGap) const;
    void fillArrowsTab(const LC_DimStyle* dimStyle, LC_DimStyle::DimensionLine* dimLine) const;
    void fillTextTab(const LC_DimStyle* dimStyle) const;
    void setArrowComboboxValue(QComboBox* arrowComboBox, const QString& arrowBlockName) const;
    void uiUpdateArrowsControlsByTickSize(double tickSize) const;
    void uiUpdateLinearFormat(RS2::LinearFormat format) const;
    void uiUpdateTextOffsetFromDimLine(LC_DimStyle::Text::VerticalPositionPolicy verticalPositioning) const;
    void uiUpdateAltLinearFormat(RS2::LinearFormat format) const;
    void uiUpdateZerosLeading(bool suppressLeading) const;
    void uiUpdateAltZerosLeading(bool suppressLeading) const;
    void fillFitTab(const LC_DimStyle* dimStyle) const;
    void fillPrimaryUnitTab(const LC_DimStyle* dimStyle) const;
    void fillAltUnitTab(const LC_DimStyle* dimStyle) const;

    void fillToleranceTab(const LC_DimStyle* dimStyle) const;
    void enableAltUnitsControls(bool enable) const;
    void uiUpdateToleranceControls(bool enable, bool showLowerLimit, bool showVerticalPosition) const;
    LC_DimStyle* m_dimStyle {nullptr};
    LC_DimStylePreviewGraphicView* m_previewView{nullptr};
    Ui::LC_DlgDimStyleManager *ui;
    RS_Graphic* m_originalGraphic {nullptr};
    QStringList m_blocksList;
    std::vector<LC_DimArrowRegistry::ArrowInfo> m_defaultArrowsInfo;

    enum EditMode {
        DIMSTYLE_EDITING,
        OVERRIDE_EDITING
    };

    EditMode m_editMode {DIMSTYLE_EDITING};
    QString m_baseStyleName = "";
};

#endif
