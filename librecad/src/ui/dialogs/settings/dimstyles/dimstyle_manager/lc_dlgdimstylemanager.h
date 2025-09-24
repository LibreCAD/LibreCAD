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
    LC_DlgDimStyleManager(QWidget *parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic, RS_Dimension* entity, const QString& baseStyleName);
    ~LC_DlgDimStyleManager() override;
    LC_DimStyle*  getDimStyle(){return m_dimStyle;}
    void addDimStyle(LC_DimStyle* dimStyle);
    void refreshPreview() const;
    void resizeEvent(QResizeEvent*) override;
    void setReadOnly();
protected slots:
    // lines tab slots
    void onDimLineColorChanged(const RS_Color& color);
    void onDimLineTypeChanged(RS2::LineType);
    void onDimLineWidthChanged(RS2::LineWidth);
    void onDimLineExtBeyondChanged(double);
    void onDimLineBaselineSpacingChanged(double);
    void onDimLineSuppress1Toggled(bool val);
    void onDimLineSuppress2Toggled(bool val);
    void onExtLineColorChanged(const RS_Color& color);
    void onExtLineType1Changed(RS2::LineType);
    void onExtLineType2Changed(RS2::LineType);
    void onExtLineWidthChanged(RS2::LineWidth);
    void onExtLineBeyondDimChanged(double);
    void onExtLineOffsetFromOriginChanged(double);
    void onExtLineFixedLengthToggled(bool val);
    void onExtLineFixedLengthChanged(double);
    void onExtLineSuppress1Toggled(bool val);
    void onExtLineSuppress2Toggled(bool val);
    // arrows tab slots
    void onArrowheadTheSameToggled(bool val);
    void setCustomArrowBlockName(QComboBox* arrowComboBox, QString dimblk1, int customBlockIndex, QString extData);
    bool processArrowComboboxChange(QComboBox* arrowComboBox, QString& dimblk1);
    void onArrowheadFirstChanged(int index);
    void onArrowheadSecondChanged(int index);
    void onArrowheadLeaderChanged(int index);
    void onArrowheadArrowSizeChanged(double d);
    void onDimBreakChanged(double d);
    void onArrowheadTickSizeChanged(double d);
    void onTextLineGapChanged(double d) const;
    void onCenterMarkTypeToggled(bool val) const;
    void setCentermarkSize(double d) const;
    void onCenterMarkSizeChanged(double d) const;
    void onDimarkSymbolToggled(bool val) const;
    void onJogHeightFactorChanged(double d);
    void onJogAngleChanged(double d) const;
    // text tab slots
    void onTextStyleChanged(RS_Font* font);
    void onTextColorChanged(const RS_Color& color);
    void onTextFillByBackgroundToggled(bool b);
    void onTextFillColorChanged(const RS_Color& color);
    void onTextHeightChanged(double d);
    void onTextFractionHeightChanged(double d);
    void onTextDrawFrameAround(bool checked) const;
    void onTextPlacementVerticalChanged(int d);
    void onTextPlacementHorizontalChanged(int d);
    void onTextReadingDirectionChanged(int d);
    void onTextOffsetFromDimLine(double d);
    void onTextAlignmentToggled(bool d);
    // fit tab slots
    void onTextFitToggled(bool d);
    void onFitSuppressArrowsToggled(bool d);
    void onTextPlacementToggled(bool d);
    void onFitScaleToggled(bool d);
    void onFitScaleExplicitChanged(double d);
    void onFitFineManuallyToggled(bool d);
    void onFitFineDrawDimlineBetweenToggled(bool d);
    // primaryUnit tab slots
    void onLinearDimUnitFormatIndexChanged(int index);
    void onLinearDimPrecisionIndexChanged(int index);
    void onLinearDimFractionIndexChanged(int index);
    void onLinearDimUnitDecimalSeparatorIndexChanged(int index);
    void onLinearDimRoundOffChanged(double d);
    void onLinearDimPrefixEditingFinished();
    void onLinearDimSuffixEditingFinished();
    void onLinearScaleFactorChanged(double d);
    void onLinearScaleApplyToLayoutDimsOnlyToggled(bool val);
    void onLinearZerosSuppressionToggled(bool val);
    void onLinearUnitFactorChanged(double d);
    void onLinearUnitPrefixEditingFinished();
    void onAngularFormatIndexChanged(int index) const;
    void onAngularPrecisionIndexChanged(int index) const;
    void onAngularZerosSuppressionToggled(bool d);
    // Alt Unit tab slots
    void onAlternateUnitsDisplayToggled(bool val);
    void onAlternateLinearFormatIndexChanged(int index);
    void onAlternateLinearPrecisionIndexChanged(int index);
    void onAltMutliplierChanged(double d);
    void onAlternateRoundToChanged(double d);
    void onAlternatePrefixEditingFinished();
    void onAlternateSuffixEditingFinished();
    void cbAltZerosSuppressionToggled(bool val);
    void onAlternateSubUnitFactorChanged(double d);
    void onAlternateSubUnitPrefixEditingFinished();
    void cbAlternatePlacementToggled(bool val);
    // Tolerance tab slots
    void onTolMethodChangedIndexChanged(int index);
    void onTolPrecisionIndexChanged(int index);
    void onTolUpperLimitChanged(double d);
    void onTolLowerLimitChangedChanged(double d);
    void onTolHeightScaleChanged(double d);
    void onTolVerticalPositionIndexChanged(int index);
    void onTolMeasurementAlignToggled(bool val);
    void onTolLinearZerosSuppressionToggled(bool val);
    void onTolAltPrecisionIndexChanged(int index);
    void cbTolAlternateZerosSuppressionToggled(bool val);
    void disableContainer(QWidget* tab);
private:
    void setDimStyle(LC_DimStyle *dimStyle);
    void createPreviewGraphicView(RS2::EntityType dimensionType);
    void createPreviewGraphicView(RS_Dimension* entity);
    void initPreview(RS2::EntityType dimensionType);
    void initPreview(RS_Dimension* entity);
    void hideFieldsReservedForTheFuture();
    void adjustUIForDimensionType(RS2::EntityType entity);
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
    void fillLinesTab(LC_DimStyle* dimStyle, const LC_DimStyle::DimensionLine* dimLine);
    void setDimGap(LC_DimStyle::DimensionLine* dimLine, double lineGap);
    void fillArrowsTab(LC_DimStyle* dimStyle, LC_DimStyle::DimensionLine* dimLine);
    void fillTextTab(LC_DimStyle* dimStyle);
    void setArrowComboboxValue(QComboBox* arrowComboBox, const QString& arrowBlockName);
    void uiUpdateArrowsControlsByTickSize(double tickSize);
    void uiUpdateLinearFormat(RS2::LinearFormat format);
    void uiUpdateTextOffsetFromDimLine(LC_DimStyle::Text::VerticalPositionPolicy verticalPositioning);
    void uiUpdateAltLinearFormat(RS2::LinearFormat format);
    void uiUpdateZerosLeading(bool suppressLeading);
    void uiUpdateAltZerosLeading(bool suppressLeading);
    void fillFitTab(LC_DimStyle* dimStyle);
    void fillPrimaryUnitTab(LC_DimStyle* dimStyle);
    void fillAltUnitTab(LC_DimStyle* dimStyle);
    void fillToleranceTab(LC_DimStyle* dimStyle);
    void enableAltUnitsControls(bool enable);
    void uiUpdateToleranceControls(bool enable, bool showLowerLimit, bool showVerticalPosition);
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

#endif // LC_DLGDIMSTYLEMANAGER_H
