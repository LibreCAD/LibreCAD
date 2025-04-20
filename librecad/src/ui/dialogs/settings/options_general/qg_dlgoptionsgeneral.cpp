/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2015, 2016 ravas (github.com/r-a-v-a-s)
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
#include "qg_dlgoptionsgeneral.h"

#include <QColorDialog>
#include <QMessageBox>

#include "dxf_format.h"
#include "lc_defaults.h"
#include "lc_settingsexporter.h"
#include "main.h"
#include "qc_applicationwindow.h"
#include "qg_filedialog.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_units.h"
/*
 *  Constructs a QG_DlgOptionsGeneral as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
int QG_DlgOptionsGeneral::m_currentTab = 0;

QG_DlgOptionsGeneral::QG_DlgOptionsGeneral(QWidget *parent)
    : LC_Dialog(parent, "OptionsGeneral"){
    setModal(false);
    setupUi(this);
    tabWidget->setCurrentIndex(m_currentTab);
    init();
    connect(variablefile_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setVariableFile);
    connect(fonts_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setFontsFolder);
    connect(translation_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setTranslationsFolder);
    connect(hatchpatterns_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setHatchPatternsFolder);

    // starting Qt-6.7.0, use QCheckBox::checkStateChanged
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    auto stateChangedSignal = &QCheckBox::checkStateChanged;
#else
    auto stateChangedSignal = &QCheckBox::stateChanged;
#endif
    connect(cbAutoBackup, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::onAutoBackupChanged);
    connect(cbVisualizeHovering, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::on_cbVisualizeHoveringClicked);
    connect(cbPersistentDialogs, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::on_cbPersistentDialogsClicked);
    connect(translation_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setTranslationsFolder);
    connect(hatchpatterns_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setHatchPatternsFolder);
    connect(cbAutoBackup, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::onAutoBackupChanged);
    connect(cbVisualizeHovering, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::on_cbVisualizeHoveringClicked);
    connect(cbPersistentDialogs, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::on_cbPersistentDialogsClicked);
    connect(cbGridExtendAxisLines, &QCheckBox::toggled,
            this, &QG_DlgOptionsGeneral::on_cbGridExtendAxisLinesToggled);
    connect(tbOtherSettings, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setOtherSettingsFolder);
    connect(cbCheckNewVersion, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::onCheckNewVersionChanged);
    connect(cbClassicStatusBar, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::on_cbClassicStatusBarToggled);
    connect(cbTabCloseButton, stateChangedSignal,
            this, &QG_DlgOptionsGeneral::onTabCloseButtonChanged);

    connect(pbExportSettings, &QPushButton::clicked, this, &QG_DlgOptionsGeneral::exportSettings);
    connect(pbImportSettings, &QPushButton::clicked, this, &QG_DlgOptionsGeneral::importSettings);

    connect(cbExpandToolsMenu, &QCheckBox::toggled, this, &QG_DlgOptionsGeneral::onExpandToolsMenuToggled);
}

void QG_DlgOptionsGeneral::onExpandToolsMenuToggled([[maybe_unused]]bool checked){
    cbExpandToolsMenuTillEntity->setEnabled(cbExpandToolsMenu->isChecked());
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsGeneral::languageChange(){
    retranslateUi(this);
}

void QG_DlgOptionsGeneral::init(){
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    languageList.sort();
    languageList.prepend("en");
    for (auto const &lang: languageList) {
        RS_DEBUG->print("QG_DlgOptionsGeneral::init: adding %s to combobox",
                        lang.toLatin1().data());

        QString l = RS_SYSTEM->symbolToLanguage(lang);
        if (!l.isEmpty() && cbLanguage->findData(lang) == -1) {
            RS_DEBUG->print("QG_DlgOptionsGeneral::init: %s", l.toLatin1().data());
            cbLanguage->addItem(l, lang);
            cbLanguageCmd->addItem(l, lang);
        }
    }

    LC_GROUP("Appearance"); // fixme - refactor to several groups?
    {
        // set current language:
        QString def_lang = "en";
        QString lang = LC_GET_STR("Language", def_lang);
        cbLanguage->setCurrentIndex(cbLanguage->findText(RS_SYSTEM->symbolToLanguage(lang)));

        QString langCmd = LC_GET_STR("LanguageCmd", def_lang);
        cbLanguageCmd->setCurrentIndex(cbLanguageCmd->findText(RS_SYSTEM->symbolToLanguage(langCmd)));

        // graphic view:

        // Snap Indicators
        bool indicator_lines_state = LC_GET_BOOL("indicator_lines_state", true);
        indicator_lines_checkbox->setChecked(indicator_lines_state);

        int indicator_lines_type = LC_GET_INT("indicator_lines_type", 0);
        indicator_lines_combobox->setCurrentIndex(indicator_lines_type);

        bool indicator_shape_state = LC_GET_BOOL("indicator_shape_state", true);
        indicator_shape_checkbox->setChecked(indicator_shape_state);

        int indicator_shape_type = LC_GET_INT("indicator_shape_type", 0);
        indicator_shape_combobox->setCurrentIndex(indicator_shape_type);

        wSnapLinesLineType->init(false, false, false);
        RS2::LineType snapIndicatorLineType = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_type", RS2::DashLine2));
        wSnapLinesLineType->setLineType(snapIndicatorLineType);

        int snapIndicatorLineWidth = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_width", 1));
        sbSnapLinesLineWidth->setValue(snapIndicatorLineWidth);

        bool cursor_hiding = LC_GET_BOOL("cursor_hiding");
        cursor_hiding_checkbox->setChecked(cursor_hiding);

        bool showSnapOptionsInSnapToolbar = LC_GET_BOOL("showSnapOptionsInSnapToolbar");
        cbShowSnapOptionsInSnapBar->setChecked(showSnapOptionsInSnapToolbar);

        bool hideRelativeZero = LC_GET_BOOL("hideRelativeZero");
        cbHideRelativeZero->setChecked(hideRelativeZero);

        bool visualizeHovering = LC_GET_BOOL("VisualizeHovering", true);
        cbVisualizeHovering->setChecked(visualizeHovering);

        bool visualizeHoveringRefPoints = LC_GET_BOOL("VisualizeHoveringRefPoints", true);
        cbShowRefPointsOnHovering->setChecked(visualizeHoveringRefPoints);
        cbShowRefPointsOnHovering->setEnabled(visualizeHovering);

        bool visualizePreviewRefPoints = LC_GET_BOOL("VisualizePreviewRefPoints", true);
        cbDisplayRefPoints->setChecked(visualizePreviewRefPoints);

        bool persistDialogs = LC_GET_BOOL("PersistDialogPositions", false);
        cbPersistentDialogs->setChecked((persistDialogs));
        cbPersistentDialogSizeOnly->setEnabled(persistDialogs);

        bool persistSizeOnly = LC_GET_BOOL("PersistDialogRestoreSizeOnly", false);
        cbPersistentDialogSizeOnly->setChecked(persistSizeOnly);

        // scale grid:
        QString scaleGrid = LC_GET_STR("ScaleGrid", "1");
        cbScaleGrid->setChecked(scaleGrid == "1");
        QString minGridSpacing = LC_GET_STR("MinGridSpacing", "10");
        cbMinGridSpacing->setCurrentIndex(cbMinGridSpacing->findText(minGridSpacing));

        bool checked = LC_GET_BOOL("Antialiasing");
        cb_antialiasing->setChecked(checked);

        checked = LC_GET_BOOL("ClassicRenderer", true);
        cbClassicRendering->setChecked(checked);

        checked = LC_GET_BOOL("UnitlessGrid");
        cb_unitless_grid->setChecked(checked);

        checked = LC_GET_BOOL("Autopanning");
        cb_autopanning->setChecked(checked);

        checked = LC_GET_INT("ScrollBars");
        scrollbars_check_box->setChecked(checked);

        checked = LC_GET_BOOL("ExtendAxisLines", false);
        cbGridExtendAxisLines->setChecked(checked);

        int xAxisExtensionType = LC_GET_INT("ExtendModeXAxis", 0);
        cbXAxisAreas->setCurrentIndex(xAxisExtensionType);

        int yAxisExtensionType = LC_GET_INT("ExtendModeYAxis", 0);
        cbYAxisAreas->setCurrentIndex(yAxisExtensionType);

        int gridType = LC_GET_INT("GridType", 0);
        cbGridType->setCurrentIndex(gridType);

        // preview:
        initComboBox(cbMaxPreview, LC_GET_STR("MaxPreview", "100"));

        checked = LC_GET_BOOL("ShowKeyboardShortcutsInTooltips", true);
        cbShowKeyboardShortcutsInToolTips->setChecked(checked);

        int handleSize = LC_GET_INT("EntityHandleSize", 4);
        sbHandleSize->setValue(handleSize);

        int relZeroRadius = LC_GET_INT("RelZeroMarkerRadius", 5);
        sbRelZeroRadius->setValue(relZeroRadius);

        int axisSize = LC_GET_INT("ZeroShortAxisMarkSize", 20);
        sbAxisSize->setValue(axisSize);

        m_originalAllowsMenusTearOff = LC_GET_BOOL("AllowMenusTearOff", true);
        cbAllowMenusDetaching->setChecked(m_originalAllowsMenusTearOff);

        checked = LC_GET_BOOL("GridDraw", true);
        cbDrawGrid->setChecked(checked);

        checked = LC_GET_BOOL("metaGridDraw", true);
        cbDrawMetaGrid->setChecked(checked);

        wMetaGridLinesLineType->init(false, false, false);
        wMetaGridPointsLineType->init(false, false, false);

        wGridLinesLineType->init(false, false, false);

        RS2::LineType metaGridPointsLineType = static_cast<RS2::LineType>(LC_GET_INT("metaGridPointsLineType", RS2::DotLineTiny));
        wMetaGridPointsLineType->setLineType(metaGridPointsLineType);

        RS2::LineType metaGridLinesLineType = static_cast<RS2::LineType>(LC_GET_INT("metaGridLinesLineType", RS2::SolidLine));
        wMetaGridLinesLineType->setLineType(metaGridLinesLineType);

        RS2::LineType gridLinesLineType = static_cast<RS2::LineType>(LC_GET_INT("GridLinesLineType", RS2::DotLine));
        wGridLinesLineType->setLineType(gridLinesLineType);

        int metagridPointsWidthPx = LC_GET_INT("metaGridPointsLineWidth", 1);
        sbMetaGridPointsWidth->setValue(metagridPointsWidthPx);

        int metagridLinesWidthPx = LC_GET_INT("metaGridLinesLineWidth", 1);
        sbMetaGridLinesWidth->setValue(metagridLinesWidthPx);

        int gridLinesWidthPx = LC_GET_INT("GridLinesLineWidth", 1);
        sbGridLinesLineWidth->setValue(gridLinesWidthPx);

        checked = LC_GET_BOOL("GridRenderSimple", false);
        cbSimpleGridRendring->setChecked(checked);

        checked = LC_GET_BOOL("GridDisableWithinPan", false);
        cbDisableGridOnPanning->setChecked(checked);

        checked = LC_GET_BOOL("GridDrawIsoVerticalForTop", true);
        cbDrawVerticalForIsoTop->setChecked(checked);

        int zoomFactor1000 = LC_GET_INT("ScrollZoomFactor", 1137);
        double zoomFactor = zoomFactor1000 / 1000.0;
        sbDefaultZoomFactor->setValue(zoomFactor);

        checked = LC_GET_BOOL("IgnoreDraftForHighlight", false);
        cbHighlightWIthLinewidthInDraft->setChecked(checked);

        checked = LC_GET_BOOL("ShowCloseButton", true);
        cbTabCloseButton->setChecked(checked);

        cbTabCloseButtonMode->setEnabled(checked);
        // it's hardly possible that there will be other options, so direct index' check of combobox items is ok
        bool showActiveOnly = LC_GET_BOOL("ShowCloseButtonActiveOnly", true);
        if (showActiveOnly) {
            cbTabCloseButtonMode->setCurrentIndex(1);
        } else {
            cbTabCloseButtonMode->setCurrentIndex(0);
        }

        checked = LC_GET_BOOL("ShowActionIconInOptions", true);
        cbShowCurrentActionIconInOptions->setChecked(checked);

        checked = LC_GET_BOOL("ShowEntityIDs", false);
        cbShowEntityIDs->setChecked(checked);

        checked = LC_GET_BOOL("PanOnZoom", false);
        cbPanOnWheelZoom->setChecked(checked);

        checked = LC_GET_BOOL("FirstTimeNoZoom", false);
        cbFirstTimeNoZoom->setChecked(checked);

        checked = LC_GET_BOOL("ShowUCSZeroMarker", false);
        cbShowUCSZeroMarker->setChecked(checked);

        checked = LC_GET_BOOL("ShowWCSZeroMarker", true);
        cbShowWCSZeroMarker->setChecked(checked);

        int zeroMarkerSize = LC_GET_INT("ZeroMarkerSize", 30);
        sbCoordinateSystemMarkerSize->setValue(zeroMarkerSize);

        int zeroMarkerFntSize = LC_GET_INT("ZeroMarkerFontSize", 10);
        sbUCSFontSize->setValue(zeroMarkerFntSize);

        QString fontName = LC_GET_STR("ZeroMarkerFontName", "Verdana");
        QFont font(fontName);
        fcbUCSFont->setCurrentFont(font);

        checked = LC_GET_BOOL("ShowDraftModeMarker", true);
        cbShowDraftModeMarker->setChecked(checked);

        fontName = LC_GET_STR("DraftMarkerFontName", "Verdana");
        fcbDraftModeFont->setCurrentFont(QFont(fontName));

        int draftMarkerFntSize = LC_GET_INT("DraftMarkerFontSize", 10);
        sbDraftModeFontSize->setValue(draftMarkerFntSize);

        bool showAnglesBasisMark = LC_GET_BOOL("AnglesBasisMarkEnabled", true);
        cbAnglesMarkVisible->setChecked(showAnglesBasisMark);

        int anglesBasisMarkPolicy = LC_GET_INT("AnglesBasisMarkPolicy", 0);
        cbAnglesBaseShowPolicy->setCurrentIndex(anglesBasisMarkPolicy);

        int angleSnapMarkerSize = LC_GET_INT("AngleSnapMarkerSize", 20);
        sbAngleSnapMarkRadius->setValue(angleSnapMarkerSize);

        checked = LC_GET_BOOL("ModifyOnViewChange", true);
        cbChangingViewOnlyModifiesDrawing->setChecked(checked);

        checked = LC_GET_BOOL("SnapGridIgnoreIfNoGrid", false);
        cbDontSnapToInvisibleGrid->setChecked(checked);
    }
    LC_GROUP_END();

    LC_GROUP("Snap"); {
        double val = LC_GET_INT("AdvSnapOnEntitySwitchToFreeDistance", 500) / 100.0;
        sbFreeSnapSwitchDistance->setValue(val);

        int catchEntitySnapDistance = LC_GET_INT("AdvSnapEntityCatchRange", 32);
        sbCatchEntitySnapDistance->setValue(catchEntitySnapDistance);

        double gridCellFactor = LC_GET_INT("AdvSnapGridCellSnapFactor", 25) / 100.0;
        sbMinGridCellSnapFactor->setValue(gridCellFactor);
    }
    LC_GROUP_END();

    LC_GROUP("InfoOverlayCursor"); {
        bool checked = LC_GET_BOOL("Enabled", true);
        cbInfoOverlayEnable->setChecked(checked);

        checked = LC_GET_BOOL("ShowAbsolute", true);
        cbInfoOverlayAbsolutePosition->setChecked(checked);

        checked = LC_GET_BOOL("ShowAbsoluteWCS", false);
        cbShowWorldCoordinates->setChecked(checked);

        checked = LC_GET_BOOL("ShowRelativeDA", true);
        cbInfoOverlayRelative->setChecked(checked);

        checked = LC_GET_BOOL("ShowRelativeDD", true);
        cbInfoOverlayRelativeDeltas->setChecked(checked);

        checked = LC_GET_BOOL("ShowSnapInfo", true);
        cbInfoOverlaySnap->setChecked(checked);

        checked = LC_GET_BOOL("ShowPrompt", true);
        cbInfoOverlayCommandPrompt->setChecked(checked);

        checked = LC_GET_BOOL("ShowActionName", true);
        cbInfoOverlayCommandName->setChecked(checked);

        checked = LC_GET_BOOL("ShowLabels", false);
        cbInfoOverlayShowLabels->setChecked(checked);

        checked = LC_GET_BOOL("SingleLine", true);
        cbInfoOverlayInOneLine->setChecked(checked);

        int fontSize = LC_GET_INT("FontSize", 10);
        sbInfoOverlayFontSize->setValue(fontSize);

        QString fontName = LC_GET_STR("FontName", "Verdana");
        QFont font(fontName);
        fcbInfoOverlayFont->setCurrentFont(font);

        int offset = LC_GET_INT("OffsetFromCursor", 15);
        sbInfoOverlayOffset->setValue(offset);

        checked = LC_GET_BOOL("ShowPropertiesCatched", true);
        cbInfoOverlaySnapEntityInfo->setChecked(checked);

        checked = LC_GET_BOOL("ShowPropertiesEdit", true);
        cbInfoOverlayPreviewEditingEntity->setChecked(checked);

        checked = LC_GET_BOOL("ShowPropertiesCreating", true);
        cbInfoOverlayPreviewCreatingEntity->setChecked(checked);
    }

    LC_GROUP("Render"); {
        int minTextHeight = LC_GET_INT("MinRenderableTextHeightPx", 4);
        sbTextMinHeight->setValue(minTextHeight);

        int minArcRadius100 = LC_GET_INT("MinArcRadius", 80);
        double minArcRadius = minArcRadius100 / 100.0;
        sbRenderMinArcRadius->setValue(minArcRadius);

        int minCircleRadius100 = LC_GET_INT("MinCircleRadius", 200);
        double minCircleRaidus = minCircleRadius100 / 100.0;
        sbRenderMinCircleRadius->setValue(minCircleRaidus);

        int minLineLen100 = LC_GET_INT("MinLineLen", 200);
        double minLineLen = minLineLen100 / 100.0;
        sbRenderMinLineLen->setValue(minLineLen);

        int minEllipseMajor100 = LC_GET_INT("MinEllipseMajor", 200);
        double minEllipseMajor = minEllipseMajor100 / 100.0;
        sbRenderMinEllipseMajor->setValue(minEllipseMajor);

        int minEllipseMinor100 = LC_GET_INT("MinEllipseMinor", 200);
        double minEllipseMinor = minEllipseMinor100 / 100.0;
        sbRenderMinEllipseMinor->setValue(minEllipseMinor);

        bool drawTextsAsDraftInPanning = LC_GET_BOOL("DrawTextsAsDraftInPanning", true);
        cbTextDraftOnPanning->setChecked(drawTextsAsDraftInPanning);

        bool drawTextsAsDraftInPreview = LC_GET_BOOL("DrawTextsAsDraftInPreview", true);
        cbTextDraftInPreview->setChecked(drawTextsAsDraftInPreview);

        bool drawInterpolate = LC_GET_BOOL("ArcRenderInterpolate", false);
        rbRenderArcInterpolate->setChecked(drawInterpolate);
        rbRenderArcQT->setChecked(!drawInterpolate);

        bool segmentFixed = LC_GET_BOOL("ArcRenderInterpolateSegmentFixed", true);
        rbRenderArcMethodFixed->setChecked(segmentFixed);
        rbRenderArcMethodSagitta->setChecked(!segmentFixed);

        int angle100 = LC_GET_INT("ArcRenderInterpolateSegmentAngle", 500);
        sbRenderArcSegmentAngle->setValue(angle100 / 100.0);

        int sagittaMax = LC_GET_INT("ArcRenderInterpolateSegmentSagitta", 90);
        sbRenderArcMaxSagitta->setValue(sagittaMax / 100.0);

        bool checked = LC_GET_BOOL("CircleRenderAsArcs", false);
        rbRenderCirclesAsArcs->setChecked(checked);

        int fontLettersColumnsCount = LC_GET_INT("FontLettersColumnsCount", 10);
        sbFontLettersColumnCount->setValue(fontLettersColumnsCount);
    }

    LC_GROUP("NewDrawingDefaults");
    LC_GROUP_END();

    LC_GROUP("Colors"); {
        initComboBox(cbBackgroundColor, LC_GET_STR("background", RS_Settings::background));
        initComboBox(cbGridPointsColor, LC_GET_STR("grid", RS_Settings::color_meta_grid_points));
        initComboBox(cbGridLinesColor, LC_GET_STR("gridLines", RS_Settings::color_meta_grid_lines));
        initComboBox(cbMetaGridPointsColor, LC_GET_STR("meta_grid", RS_Settings::color_meta_grid_points));
        initComboBox(cbMetaGridLinesColor, LC_GET_STR("meta_grid_lines", RS_Settings::color_meta_grid_lines));
        initComboBox(cbSelectedColor, LC_GET_STR("select", RS_Settings::select));
        initComboBox(cbHighlightedColor, LC_GET_STR("highlight", RS_Settings::select));
        initComboBox(cbStartHandleColor, LC_GET_STR("start_handle", RS_Settings::start_handle));
        initComboBox(cbHandleColor, LC_GET_STR("handle", RS_Settings::handle));
        initComboBox(cbEndHandleColor, LC_GET_STR("end_handle", RS_Settings::end_handle));
        initComboBox(cbRelativeZeroColor, LC_GET_STR("relativeZeroColor", RS_Settings::relativeZeroColor));
        initComboBox(cbPreviewRefColor, LC_GET_STR("previewReferencesColor", RS_Settings::previewRefColor));
        initComboBox(cbPreviewRefHighlightColor,LC_GET_STR("previewReferencesHighlightColor", RS_Settings::previewRefHighlightColor));
        initComboBox(cb_snap_color, LC_GET_STR("snap_indicator", RS_Settings::snap_indicator));
        initComboBox(cb_snap_lines_color, LC_GET_STR("snap_indicator_lines", RS_Settings::snap_indicator_lines));
        initComboBox(cbAxisXColor, LC_GET_STR("grid_x_axisColor", RS_Settings::xAxisColor));
        initComboBox(cbAxisYColor, LC_GET_STR("grid_y_axisColor", RS_Settings::yAxisColor));

        initComboBox(cbOverlayBoxLine, LC_GET_STR("overlay_box_line", RS_Settings::overlayBoxLine));
        initComboBox(cbOverlayBoxFill, LC_GET_STR("overlay_box_fill", RS_Settings::overlayBoxFill));
        initComboBox(cbOverlayBoxLineInverted, LC_GET_STR("overlay_box_line_inv", RS_Settings::overlayBoxLineInverted));
        initComboBox(cbOverlayBoxFillInverted, LC_GET_STR("overlay_box_fill_inv", RS_Settings::overlayBoxFillInverted));

        initComboBox(cbInfoOverlayAbsolutePositionColor, LC_GET_STR("info_overlay_absolute", RS_Settings::overlayInfoCursorAbsolutePos));
        initComboBox(cbInfoOverlaySnapColor, LC_GET_STR("info_overlay_snap", RS_Settings::overlayInfoCursorSnap));
        initComboBox(cbInfoOverlayCommandPromptColor, LC_GET_STR("info_overlay_prompt", RS_Settings::overlayInfoCursorCommandPrompt));
        initComboBox(cbInfoOverlayRelativeColor, LC_GET_STR("info_overlay_relative", RS_Settings::overlayInfoCursorRelativePos));

        initComboBox(cbDraftModeMarkerColor, LC_GET_STR("draft_mode_marker", RS_Settings::select));

        initComboBox(cbAnglesMarkColorDirection, LC_GET_STR("angles_basis_direction", RS_Settings::anglesBasisDirection));
        initComboBox(cbAnglesMarkColorAngleRay, LC_GET_STR("angles_basis_angleray", RS_Settings::anglesBasisAngleRay));

        int overlayTransparency = LC_GET_INT("overlay_box_transparency", 90);
        sbOverlayBoxTransparency->setValue(overlayTransparency);
    }
    LC_GROUP_END();

    LC_GROUP("Paths"); {
        lePathTranslations->setText(LC_GET_STR("Translations", ""));
        lePathHatch->setText(LC_GET_STR("Patterns", ""));
        lePathFonts->setText(LC_GET_STR("Fonts", ""));
        m_originalLibraryPath = LC_GET_STR("Library", "").trimmed();
        lePathLibrary->setText(m_originalLibraryPath);
        leTemplate->setText(LC_GET_STR("Template", "").trimmed());
        variablefile_field->setText(LC_GET_STR("VariableFile", "").trimmed());
        leOtherSettingsDirectory->setText(LC_GET_STR("OtherSettingsDir", RS_System::instance()->getAppDataDir()).trimmed());
    }
    LC_GROUP_END();

    // units:
    for (int i = RS2::None; i < RS2::LastUnit; i++) {
        if (i != (int) RS2::None)
            cbUnit->addItem(RS_Units::unitToString((RS2::Unit) i));
    }
    // RVT_PORT cbUnit->listBox()->sort();
    cbUnit->insertItem(0, RS_Units::unitToString(RS2::None));

    QString def_unit = "Millimeter";

    LC_GROUP("Defaults"); {
        cbUnit->setCurrentIndex(cbUnit->findText(QObject::tr(LC_GET_STR("Unit", def_unit).toUtf8().data())));
        // Auto save timer
        cbAutoSaveTime->setValue(LC_GET_INT("AutoSaveTime", 5));
        bool autoBackup = LC_GET_BOOL("AutoBackupDocument", true);

        QString autosaveFileNamePrefix = LC_GET_STR("AutosaveFilePrefix", "#");
        cbAutoSaveFileNamePrefix->setCurrentText(autosaveFileNamePrefix);

        QString backupFileNameSuffix = LC_GET_STR("BackupFileSuffix", "#");
        cbBackupFileSuffix->setCurrentText(backupFileNameSuffix);


        cbAutoBackup->setChecked(autoBackup);
        cbAutoSaveTime->setEnabled(autoBackup);
        cbUseQtFileOpenDialog->setChecked(LC_GET_BOOL("UseQtFileOpenDialog", true));
        cbWheelScrollInvertH->setChecked(LC_GET_BOOL("WheelScrollInvertH"));
        cbWheelScrollInvertV->setChecked(LC_GET_BOOL("WheelScrollInvertV"));
        cbInvertZoomDirection->setChecked(LC_GET_BOOL("InvertZoomDirection"));
        cbAngleSnapStep->setCurrentIndex(LC_GET_INT("AngleSnapStep", 3));

        cbNewDrawingGridOff->setChecked(LC_GET_BOOL("GridOffForNewDrawing", false));

        bool defaultIsometricGrid = LC_GET_BOOL("IsometricGrid", false);
        int defaultIsoView = LC_GET_INT("IsoGridView", RS2::IsoGridViewType::IsoTop);

        if (defaultIsometricGrid) {
            switch (defaultIsoView) {
                case RS2::IsoGridViewType::IsoLeft: {
                    rbGridIsoLeft->setChecked(true);
                    break;
                }
                case RS2::IsoGridViewType::IsoTop: {
                    rbGridIsoTop->setChecked(true);
                    break;
                }
                case RS2::IsoGridViewType::IsoRight: {
                    rbGridIsoRight->setChecked(true);
                    break;
                }
                default: {
                    rbGridIsoTop->setChecked(true);
                    break;
                }
            }
            rbGridOrtho->setChecked(false);
        } else {
            rbGridOrtho->setChecked(true);
        }

        const QString &defaultAnglesBase = LC_GET_STR("AnglesBaseAngle", "0.0");
        bool anglesCounterClockwise = LC_GET_BOOL("AnglesCounterClockwise", true);
        rbDefAngleBasePositive->setChecked(anglesCounterClockwise);
        leDefAngleBaseZero->setText(defaultAnglesBase);
    }
    LC_GROUP_END();

    //update entities to selected entities to the current active layer
    LC_GROUP("Modify"); {
        auto toActive = LC_GET_BOOL("ModifyEntitiesToActiveLayer");
        cbToActiveLayer->setChecked(toActive);
        LC_SET("ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked());

        bool keepModify = LC_GET_BOOL("KeepModifiedSelected", true);
        cbKeepModifiedSelected->setChecked(keepModify);
    }
    LC_GROUP_END();

    LC_GROUP("CADPreferences"); {
        cbAutoZoomDrawing->setChecked(LC_GET_BOOL("AutoZoomDrawing"));
        cbAnglesInputInDecimalDegreesOnly->setChecked(LC_GET_BOOL("InputAnglesAsDecimalsOnly", false));
    }
    LC_GROUP_END();

    LC_GROUP("Startup"); {
        cbSplash->setChecked(LC_GET_BOOL("ShowSplash", true));
        tab_mode_check_box->setChecked(LC_GET_BOOL("TabMode"));
        maximize_checkbox->setChecked(LC_GET_BOOL("Maximize"));
        left_sidebar_checkbox->setChecked(LC_GET_BOOL("EnableLeftSidebar", true));
        cad_toolbars_checkbox->setChecked(LC_GET_BOOL("EnableCADToolbars", true));
        cbOpenLastFiles->setChecked(LC_GET_BOOL("OpenLastOpenedFiles", true));
        m_originalUseClassicToolbar = LC_GET_BOOL("UseClassicStatusBar", false);
        cbClassicStatusBar->setChecked(m_originalUseClassicToolbar);

        cbCheckNewVersion->setChecked(LC_GET_BOOL("CheckForNewVersions", true));
        cbCheckNewVersionIgnorePreRelease->setChecked(LC_GET_BOOL("IgnorePreReleaseVersions", true));

        bool checked = LC_GET_BOOL("ShowCommandPromptInStatusBar", true);
        cbDuplicateActionsPromptsInStatusBar->setChecked(checked);
        cbDuplicateActionsPromptsInStatusBar->setEnabled(!m_originalUseClassicToolbar);

        bool useExpandedToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        m_originalExpandedToolsMenu = useExpandedToolsMenu;
        cbExpandToolsMenu->setChecked(useExpandedToolsMenu);

        bool expandToolsMenuTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
        m_originalExpandedToolsMenuTillEntity = expandToolsMenuTillEntity;
        cbExpandToolsMenuTillEntity->setChecked(expandToolsMenuTillEntity);

        cbExpandToolsMenuTillEntity->setEnabled(useExpandedToolsMenu);
    }
    LC_GROUP_END();

    LC_GROUP("Keyboard"); {
        cbEvaluateOnSpace->setChecked(LC_GET_BOOL("EvaluateCommandOnSpace"));
        cbToggleFreeSnapOnSpace->setChecked(LC_GET_BOOL("ToggleFreeSnapOnSpace"));
    }

    cbCheckNewVersionIgnorePreRelease->setEnabled(!XSTR(LC_PRERELEASE));

    initReferencePoints();

    m_restartNeeded = false;
}

void QG_DlgOptionsGeneral::initComboBox(QComboBox *cb, const QString &text){
    int idx = cb->findText(text);
    if (idx < 0) {
        idx = 0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex(idx);
}

void QG_DlgOptionsGeneral::setRestartNeeded(){
    m_restartNeeded = true;
}

void QG_DlgOptionsGeneral::setTemplateFile(){
    RS2::FormatType type = RS2::FormatDXFRW;
    QG_FileDialog dlg(this);
    QString fileName = dlg.getOpenFile(&type);
    leTemplate->setText(fileName);
}

void QG_DlgOptionsGeneral::ok(){
    if (RS_Settings::save_is_allowed) {
        //RS_SYSTEM->loadTranslation(cbLanguage->currentText());
        LC_GROUP("Appearance"); {
            LC_SET("ScaleGrid", cbScaleGrid->isChecked());
            LC_SET("hideRelativeZero", cbHideRelativeZero->isChecked());
            LC_SET("VisualizeHovering", cbVisualizeHovering->isChecked());
            LC_SET("VisualizeHoveringRefPoints", cbShowRefPointsOnHovering->isChecked());
            LC_SET("VisualizePreviewRefPoints", cbDisplayRefPoints->isChecked());
            LC_SET("MinGridSpacing", cbMinGridSpacing->currentText());
            LC_SET("MaxPreview", cbMaxPreview->currentText());
            LC_SET("Language", cbLanguage->itemData(cbLanguage->currentIndex()).toString());
            LC_SET("LanguageCmd", cbLanguageCmd->itemData(cbLanguageCmd->currentIndex()).toString());
            LC_SET("indicator_lines_state", indicator_lines_checkbox->isChecked());
            LC_SET("indicator_lines_type", indicator_lines_combobox->currentIndex());
            LC_SET("indicator_shape_state", indicator_shape_checkbox->isChecked());
            LC_SET("indicator_shape_type", indicator_shape_combobox->currentIndex());
            LC_SET("indicator_lines_line_type", wSnapLinesLineType->getLineType());
            LC_SET("indicator_lines_line_width", sbSnapLinesLineWidth->value());
            LC_SET("cursor_hiding", cursor_hiding_checkbox->isChecked());
            LC_SET("showSnapOptionsInSnapToolbar", cbShowSnapOptionsInSnapBar->isChecked());
            LC_SET("UnitlessGrid", cb_unitless_grid->isChecked());
            LC_SET("Antialiasing", cb_antialiasing->isChecked());
            LC_SET("ClassicRenderer", cbClassicRendering->isChecked());
            LC_SET("Autopanning", cb_autopanning->isChecked());
            LC_SET("ScrollBars", scrollbars_check_box->isChecked());
            LC_SET("ShowKeyboardShortcutsInTooltips", cbShowKeyboardShortcutsInToolTips->isChecked());
            LC_SET("PersistDialogPositions", cbPersistentDialogs->isChecked());
            LC_SET("PersistDialogRestoreSizeOnly", cbPersistentDialogSizeOnly->isChecked());
            LC_SET("GridType", cbGridType->currentIndex());
            LC_SET("ExtendAxisLines", cbGridExtendAxisLines->isChecked());
            LC_SET("ExtendModeXAxis", cbXAxisAreas->currentIndex());
            LC_SET("ExtendModeYAxis", cbYAxisAreas->currentIndex());
            LC_SET("EntityHandleSize", sbHandleSize->value());
            LC_SET("RelZeroMarkerRadius", sbRelZeroRadius->value());
            LC_SET("ZeroShortAxisMarkSize", sbAxisSize->value());
            LC_SET("AllowMenusTearOff", cbAllowMenusDetaching->isChecked());

            LC_SET("metaGridDraw", cbDrawMetaGrid->isChecked());
            LC_SET("GridDraw", cbDrawGrid->isChecked());
            LC_SET("metaGridPointsLineType", wMetaGridPointsLineType->getLineType());
            LC_SET("metaGridLinesLineType", wMetaGridLinesLineType->getLineType());
            LC_SET("metaGridPointsLineWidth", sbMetaGridPointsWidth->value());
            LC_SET("metaGridLinesLineWidth", sbMetaGridLinesWidth->value());
            LC_SET("GridLinesLineType", wGridLinesLineType->getLineType());
            LC_SET("GridLinesLineWidth", sbGridLinesLineWidth->value());
            LC_SET("GridRenderSimple", cbSimpleGridRendring->isChecked());
            LC_SET("GridDisableWithinPan", cbDisableGridOnPanning->isChecked());
            LC_SET("GridDrawIsoVerticalForTop", cbDrawVerticalForIsoTop->isChecked());
            double zoomFactor = sbDefaultZoomFactor->value();
            int zoomFactor1000 = (int) (zoomFactor * 1000.0);
            LC_SET("ScrollZoomFactor", zoomFactor1000);
            LC_SET("IgnoreDraftForHighlight", cbHighlightWIthLinewidthInDraft->isChecked());

            LC_SET("ShowCloseButton", cbTabCloseButton->isChecked());
            LC_SET("ShowCloseButtonActiveOnly", cbTabCloseButtonMode->currentIndex() == 1);

            LC_SET("ShowActionIconInOptions", cbShowCurrentActionIconInOptions->isChecked());
            LC_SET("ShowEntityIDs", cbShowEntityIDs->isChecked());

            LC_SET("PanOnZoom", cbPanOnWheelZoom->isChecked());
            LC_SET("FirstTimeNoZoom", cbFirstTimeNoZoom->isChecked());

            LC_SET("ShowUCSZeroMarker", cbShowUCSZeroMarker->isChecked());
            LC_SET("ShowWCSZeroMarker", cbShowWCSZeroMarker->isChecked());
            LC_SET("ZeroMarkerSize", sbCoordinateSystemMarkerSize->value());
            LC_SET("ZeroMarkerFontSize", sbUCSFontSize->value());
            LC_SET("ZeroMarkerFontName", fcbUCSFont->currentText());

            LC_SET("ShowDraftModeMarker", cbShowDraftModeMarker->isChecked());
            LC_SET("DraftMarkerFontName", fcbDraftModeFont->currentText());
            LC_SET("DraftMarkerFontSize", sbDraftModeFontSize->value());

            LC_SET("AnglesBasisMarkEnabled", cbAnglesMarkVisible->isChecked());
            LC_SET("AnglesBasisMarkPolicy", cbAnglesBaseShowPolicy->currentIndex());
            LC_SET("AngleSnapMarkerSize", sbAngleSnapMarkRadius->value());
            LC_SET("ModifyOnViewChange", cbChangingViewOnlyModifiesDrawing->isChecked());
            LC_SET("SnapGridIgnoreIfNoGrid", cbDontSnapToInvisibleGrid->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Snap"); {
            LC_SET("AdvSnapOnEntitySwitchToFreeDistance", (int) (sbFreeSnapSwitchDistance->value() * 100));
            LC_SET("AdvSnapEntityCatchRange", sbCatchEntitySnapDistance->value());
            LC_SET("AdvSnapGridCellSnapFactor", (int) (sbMinGridCellSnapFactor->value() * 100));
        }
        LC_GROUP_END();

        LC_GROUP("InfoOverlayCursor"); {
            LC_SET("Enabled", cbInfoOverlayEnable->isChecked());
            LC_SET("ShowAbsolute", cbInfoOverlayAbsolutePosition->isChecked());
            LC_SET("ShowAbsoluteWCS", cbShowWorldCoordinates->isChecked());
            LC_SET("ShowRelativeDA", cbInfoOverlayRelative->isChecked());
            LC_SET("ShowRelativeDD", cbInfoOverlayRelativeDeltas->isChecked());
            LC_SET("ShowSnapInfo", cbInfoOverlaySnap->isChecked());
            LC_SET("ShowPrompt", cbInfoOverlayCommandPrompt->isChecked());
            LC_SET("ShowActionName", cbInfoOverlayCommandName->isChecked());
            LC_SET("ShowLabels", cbInfoOverlayShowLabels->isChecked());
            LC_SET("SingleLine", cbInfoOverlayInOneLine->isChecked());
            LC_SET("FontSize", sbInfoOverlayFontSize->value());
            LC_SET("FontName", fcbInfoOverlayFont->currentText());
            LC_SET("OffsetFromCursor", sbInfoOverlayOffset->value());
            LC_SET("ShowPropertiesCatched", cbInfoOverlaySnapEntityInfo->isChecked());
            LC_SET("ShowPropertiesEdit", cbInfoOverlayPreviewEditingEntity->isChecked());
            LC_SET("ShowPropertiesCreating", cbInfoOverlayPreviewCreatingEntity->isChecked());
        }

        LC_GROUP("Render"); {
            LC_SET("MinRenderableTextHeightPx", sbTextMinHeight->value());
            LC_SET("MinArcRadius", (int) (sbRenderMinArcRadius->value() * 100));
            LC_SET("MinCircleRadius", (int) (sbRenderMinCircleRadius->value() * 100));
            LC_SET("MinLineLen", (int) (sbRenderMinLineLen->value() * 100));
            LC_SET("MinEllipseMajor", (int) (sbRenderMinEllipseMajor->value() * 100));
            LC_SET("MinEllipseMinor", (int) (sbRenderMinEllipseMinor->value() * 100));
            LC_SET("DrawTextsAsDraftInPanning", cbTextDraftOnPanning->isChecked());
            LC_SET("DrawTextsAsDraftInPreview", cbTextDraftInPreview->isChecked());

            LC_SET("ArcRenderInterpolate", rbRenderArcInterpolate->isChecked());
            LC_SET("ArcRenderInterpolateSegmentFixed", rbRenderArcMethodFixed->isChecked());
            LC_SET("ArcRenderInterpolateSegmentAngle", sbRenderArcSegmentAngle->value() * 100);
            LC_SET("ArcRenderInterpolateSegmentSagitta", sbRenderArcMaxSagitta->value() * 100);
            LC_SET("CircleRenderAsArcs", rbRenderCirclesAsArcs->isChecked());

            LC_SET("FontLettersColumnsCount", sbFontLettersColumnCount->value());
        }

        LC_GROUP("Colors"); {
            LC_SET("background", cbBackgroundColor->currentText());
            LC_SET("grid", cbGridPointsColor->currentText());
            LC_SET("gridLines", cbGridLinesColor->currentText());
            LC_SET("meta_grid", cbMetaGridPointsColor->currentText());
            LC_SET("meta_grid_lines", cbMetaGridLinesColor->currentText());
            LC_SET("select", cbSelectedColor->currentText());
            LC_SET("highlight", cbHighlightedColor->currentText());
            LC_SET("start_handle", cbStartHandleColor->currentText());
            LC_SET("handle", cbHandleColor->currentText());
            LC_SET("end_handle", cbEndHandleColor->currentText());
            LC_SET("relativeZeroColor", cbRelativeZeroColor->currentText());
            LC_SET("previewReferencesColor", cbPreviewRefColor->currentText());
            LC_SET("previewReferencesHighlightColor", cbPreviewRefHighlightColor->currentText());
            LC_SET("snap_indicator", cb_snap_color->currentText());
            LC_SET("snap_indicator_lines", cb_snap_lines_color->currentText());
            LC_SET("grid_x_axisColor", cbAxisXColor->currentText());
            LC_SET("grid_y_axisColor", cbAxisYColor->currentText());

            LC_SET("overlay_box_line", cbOverlayBoxLine->currentText());
            LC_SET("overlay_box_fill", cbOverlayBoxFill->currentText());
            LC_SET("overlay_box_line_inv", cbOverlayBoxLineInverted->currentText());
            LC_SET("overlay_box_fill_inv", cbOverlayBoxFillInverted->currentText());
            LC_SET("overlay_box_transparency", sbOverlayBoxTransparency->value());

            LC_SET("info_overlay_absolute", cbInfoOverlayAbsolutePositionColor->currentText());
            LC_SET("info_overlay_snap", cbInfoOverlaySnapColor->currentText());
            LC_SET("info_overlay_prompt", cbInfoOverlayCommandPromptColor->currentText());
            LC_SET("info_overlay_relative", cbInfoOverlayRelativeColor->currentText());

            LC_SET("angles_basis_direction", cbAnglesMarkColorDirection->currentText());
            LC_SET("angles_basis_angleray", cbAnglesMarkColorAngleRay->currentText());

            LC_SET("draft_mode_marker", cbDraftModeMarkerColor->currentText());
        }
        LC_GROUP_END();

        LC_GROUP("Paths"); {
            // fixme - well, it's also good to check that specified directories does exsit
            LC_SET("Translations", lePathTranslations->text().trimmed());
            LC_SET("Patterns", lePathHatch->text().trimmed());
            LC_SET("Fonts", lePathFonts->text().trimmed());
            LC_SET("Library", lePathLibrary->text().trimmed());
            LC_SET("Template", leTemplate->text().trimmed());
            LC_SET("VariableFile", variablefile_field->text());
            LC_SET("OtherSettingsDir", leOtherSettingsDirectory->text().trimmed());
        }
        LC_GROUP_END();

        LC_GROUP("Defaults"); {
            LC_SET("Unit", RS_Units::unitToString(RS_Units::stringToUnit(cbUnit->currentText()), false/*untr.*/));
            LC_SET("AutoSaveTime", cbAutoSaveTime->value());
            LC_SET("AutoBackupDocument", cbAutoBackup->isChecked());

            QString autosaveFileNamePrefix = cbAutoSaveFileNamePrefix->currentText();
            LC_SET("AutosaveFilePrefix", autosaveFileNamePrefix);

            QString backupFileNameSuffix = cbBackupFileSuffix->currentText();
            LC_SET("BackupFileSuffix", backupFileNameSuffix);

            LC_SET("UseQtFileOpenDialog", cbUseQtFileOpenDialog->isChecked());
            LC_SET("WheelScrollInvertH", cbWheelScrollInvertH->isChecked());
            LC_SET("WheelScrollInvertV", cbWheelScrollInvertV->isChecked());
            LC_SET("InvertZoomDirection", cbInvertZoomDirection->isChecked());
            LC_SET("AngleSnapStep", cbAngleSnapStep->currentIndex());
            LC_SET("GridOffForNewDrawing", cbNewDrawingGridOff->isChecked());

            bool defaultIsometricGrid = !rbGridOrtho->isChecked();
            LC_SET("IsometricGrid", defaultIsometricGrid);
            if (defaultIsometricGrid) {
                int defaultIsoView;

                if (rbGridIsoLeft->isChecked()) {
                    defaultIsoView = RS2::IsoGridViewType::IsoLeft;
                } else if (rbGridIsoTop->isChecked()) {
                    defaultIsoView = RS2::IsoGridViewType::IsoTop;
                } else if (rbGridIsoRight->isChecked()) {
                    defaultIsoView = RS2::IsoGridViewType::IsoRight;
                } else {
                    defaultIsoView = RS2::IsoGridViewType::IsoTop;
                }

                LC_SET("IsoGridView", defaultIsoView);
            }

            LC_SET("AnglesBaseAngle", leDefAngleBaseZero->text());
            LC_SET("AnglesCounterClockwise", rbDefAngleBasePositive->isChecked());
        }
        LC_GROUP_END();

        //update entities to selected entities to the current active layer
        LC_GROUP("Modify"); {
            LC_SET("ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked());
            LC_SET("KeepModifiedSelected", cbKeepModifiedSelected->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("CADPreferences"); {
            LC_SET("AutoZoomDrawing", cbAutoZoomDrawing->isChecked());
            LC_SET("InputAnglesAsDecimalsOnly", cbAnglesInputInDecimalDegreesOnly->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Startup"); {
            LC_SET("ShowSplash", cbSplash->isChecked());
            LC_SET("TabMode", tab_mode_check_box->isChecked());
            LC_SET("Maximize", maximize_checkbox->isChecked());
            LC_SET("EnableLeftSidebar", left_sidebar_checkbox->isChecked());
            LC_SET("EnableCADToolbars", cad_toolbars_checkbox->isChecked());
            LC_SET("OpenLastOpenedFiles", cbOpenLastFiles->isChecked());
            LC_SET("UseClassicStatusBar", cbClassicStatusBar->isChecked());
            LC_SET("ShowCommandPromptInStatusBar", cbDuplicateActionsPromptsInStatusBar->isChecked());
            LC_SET("CheckForNewVersions", cbCheckNewVersion->isChecked());
            LC_SET("IgnorePreReleaseVersions", cbCheckNewVersionIgnorePreRelease->isChecked());
            LC_SET("ExpandedToolsMenu", cbExpandToolsMenu->isChecked());
            LC_SET("ExpandedToolsMenuTillEntity", cbExpandToolsMenuTillEntity->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Keyboard"); {
            LC_SET("EvaluateCommandOnSpace", cbEvaluateOnSpace->isChecked());
            LC_SET("ToggleFreeSnapOnSpace", cbToggleFreeSnapOnSpace->isChecked());
        }
        LC_GROUP_END();
        saveReferencePoints();
    }
    // fixme - sand - files - RESTORE ! change to main windows emit!
    RS_SETTINGS->emitOptionsChanged();
    if (checkRestartNeeded()) {
        QMessageBox::warning(this, tr("Preferences"),
                             tr("Please restart the application to apply all changes."));
    }
    accept();
}

bool QG_DlgOptionsGeneral::checkRestartNeeded(){
    bool result = m_originalUseClassicToolbar != cbClassicStatusBar->isChecked() ||
                  m_originalLibraryPath != lePathLibrary->text().trimmed() ||
                  m_originalAllowsMenusTearOff != cbAllowMenusDetaching->isChecked();
    return result;
}

void QG_DlgOptionsGeneral::on_tabWidget_currentChanged(int index){
    m_currentTab = index;
}

void QG_DlgOptionsGeneral::set_color(QComboBox *combo, QColor custom){
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, tr("Select Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        combo->lineEdit()->setText(color.name());
    }
}

void QG_DlgOptionsGeneral::on_pb_background_clicked(){
    set_color(cbBackgroundColor, QColor(RS_Settings::background));
}

void QG_DlgOptionsGeneral::on_pb_gridPoints_clicked(){
    set_color(cbGridPointsColor, QColor(RS_Settings::color_meta_grid_points));
}

void QG_DlgOptionsGeneral::on_pb_gridLines_clicked(){
    set_color(cbGridLinesColor, QColor(RS_Settings::color_meta_grid_lines));
}

void QG_DlgOptionsGeneral::on_pb_metaPoints_clicked(){
    set_color(cbMetaGridPointsColor, QColor(RS_Settings::color_meta_grid_points));
}

void QG_DlgOptionsGeneral::on_pb_metaLines_clicked(){
    set_color(cbMetaGridLinesColor, QColor(RS_Settings::color_meta_grid_lines));
}

void QG_DlgOptionsGeneral::on_pb_selected_clicked(){
    set_color(cbSelectedColor, QColor(RS_Settings::select));
}

void QG_DlgOptionsGeneral::on_pb_highlighted_clicked(){
    set_color(cbHighlightedColor, QColor(RS_Settings::highlight));
}

void QG_DlgOptionsGeneral::on_pb_start_clicked(){
    set_color(cbStartHandleColor, QColor(RS_Settings::start_handle));
}

void QG_DlgOptionsGeneral::on_pb_handle_clicked(){
    set_color(cbHandleColor, QColor(RS_Settings::handle));
}

void QG_DlgOptionsGeneral::on_pb_end_clicked(){
    set_color(cbEndHandleColor, QColor(RS_Settings::end_handle));
}

void QG_DlgOptionsGeneral::on_pb_snap_color_clicked(){
    set_color(cb_snap_color, QColor(RS_Settings::snap_indicator));
}

void QG_DlgOptionsGeneral::on_pb_snap_lines_color_clicked(){
    set_color(cb_snap_lines_color, QColor(RS_Settings::snap_indicator_lines));
}

void QG_DlgOptionsGeneral::on_pb_relativeZeroColor_clicked(){
    set_color(cbRelativeZeroColor, QColor(RS_Settings::relativeZeroColor));
}

void QG_DlgOptionsGeneral::on_pb_previewRefColor_clicked(){
    set_color(cbPreviewRefColor, QColor(RS_Settings::previewRefColor));
}

void QG_DlgOptionsGeneral::on_pb_previewRefHighlightColor_clicked(){
    set_color(cbPreviewRefHighlightColor, QColor(RS_Settings::previewRefHighlightColor));
}

void QG_DlgOptionsGeneral::on_pb_axis_X_clicked(){
    set_color(cbAxisXColor, QColor(RS_Settings::xAxisColor));
}

void QG_DlgOptionsGeneral::on_pb_axis_Y_clicked(){
    set_color(cbAxisYColor, QColor(RS_Settings::yAxisColor));
}

void QG_DlgOptionsGeneral::on_pbOverlayBoxLine_clicked(){
    set_color(cbOverlayBoxLine, QColor(RS_Settings::overlayBoxLine));
}

void QG_DlgOptionsGeneral::on_pbOverlayBoxFill_clicked(){
    set_color(cbOverlayBoxFill, QColor(RS_Settings::overlayBoxFill));
}

void QG_DlgOptionsGeneral::on_pbOverlayBoxLineInverted_clicked(){
    set_color(cbOverlayBoxLineInverted, QColor(RS_Settings::overlayBoxLineInverted));
}

void QG_DlgOptionsGeneral::on_pbOverlayBoxFillInverted_clicked(){
    set_color(cbOverlayBoxFillInverted, QColor(RS_Settings::overlayBoxFillInverted));
}

void QG_DlgOptionsGeneral::on_pbDraftModeColor_clicked(){
    set_color(cbDraftModeMarkerColor, QColor(RS_Settings::select));
}

void QG_DlgOptionsGeneral::on_pbcbInfoOverlayAbsolutePositionColor_clicked(){
    set_color(cbInfoOverlayAbsolutePositionColor, QColor(RS_Settings::overlayInfoCursorAbsolutePos));
}

void QG_DlgOptionsGeneral::on_pbInfoOverlaySnapColor_clicked(){
    set_color(cbInfoOverlayAbsolutePositionColor, QColor(RS_Settings::overlayInfoCursorSnap));
}

void QG_DlgOptionsGeneral::on_pbInfoOverlayRelativeColor_clicked(){
    set_color(cbInfoOverlayAbsolutePositionColor, QColor(RS_Settings::overlayInfoCursorRelativePos));
}

void QG_DlgOptionsGeneral::on_pbInfoOverlayCommandPromptColor_clicked(){
    set_color(cbInfoOverlayAbsolutePositionColor, QColor(RS_Settings::overlayInfoCursorCommandPrompt));
}

void QG_DlgOptionsGeneral::on_pbAnglesMarkDirection_clicked(){
    set_color(cbAnglesMarkColorDirection, QColor(RS_Settings::anglesBasisDirection));
}

void QG_DlgOptionsGeneral::on_pbAnglesMarkAngleRay_clicked(){
    set_color(cbAnglesMarkColorAngleRay, QColor(RS_Settings::anglesBasisAngleRay));
}

void QG_DlgOptionsGeneral::on_pb_clear_all_clicked(){
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear settings"),
                                  tr("This will also include custom menus and toolbars. Continue?"),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        RS_SETTINGS->clear_all();
        QMessageBox::information(this, "info", tr("You must restart LibreCAD to see the changes."));
    }
}

void QG_DlgOptionsGeneral::on_pb_clear_geometry_clicked(){
    RS_SETTINGS->clear_geometry();
    QMessageBox::information(this, "info", tr("You must restart LibreCAD to see the changes."));
}

void QG_DlgOptionsGeneral::setVariableFile(){
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty()) {
        variablefile_field->setText(QDir::toNativeSeparators(path));
    }
}

/*!
 * \brief slot for the font folder selection icon
 * \author ravas
 * \date 2016-286
 */
void QG_DlgOptionsGeneral::setFontsFolder(){
    QString folder = selectFolder(tr("Select Fonts Folder"));
    if (folder != nullptr) {
        lePathFonts->setText(QDir::toNativeSeparators(folder));
    }
}

void QG_DlgOptionsGeneral::setTranslationsFolder(){
    QString folder = selectFolder(tr("Select Translations Folder"));
    if (folder != nullptr) {
        lePathTranslations->setText(QDir::toNativeSeparators(folder));
    }
}

void QG_DlgOptionsGeneral::setHatchPatternsFolder(){
    QString folder = selectFolder(tr("Select Hatch Patterns Folder"));
    if (folder != nullptr) {
        lePathHatch->setText(QDir::toNativeSeparators(folder));
    }
}

void QG_DlgOptionsGeneral::setOtherSettingsFolder(){
    QString folder = selectFolder(tr("Select Other Settings Folder"));
    if (folder != nullptr) {
        leOtherSettingsDirectory->setText(QDir::toNativeSeparators(folder));
    }
}

QString QG_DlgOptionsGeneral::selectFolder(const QString &title){
    QString folder = nullptr;
    QFileDialog dlg(this);
    if (title != nullptr) {
        QString dlgTitle = title;
        dlg.setWindowTitle(dlgTitle);
    }
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);

    if (dlg.exec()) {
        folder = dlg.selectedFiles()[0];
    }
    return folder;
}

// fixme - sand - this function is called by signal, but but if the user changes path manually - no restart. Rework this.
void QG_DlgOptionsGeneral::setLibraryPath(){
    QG_FileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);

    if (dlg.exec()) {
        auto dir = dlg.selectedFiles()[0];
        lePathLibrary->setText(QDir::toNativeSeparators(dir));
        setRestartNeeded();
    }
}

void QG_DlgOptionsGeneral::on_cbVisualizeHoveringClicked(){
    cbShowRefPointsOnHovering->setEnabled(cbVisualizeHovering->isChecked());
}

void QG_DlgOptionsGeneral::on_cbPersistentDialogsClicked(){
    cbPersistentDialogSizeOnly->setEnabled(cbPersistentDialogs->isChecked());
}

void QG_DlgOptionsGeneral::on_cbClassicStatusBarToggled(){
    cbDuplicateActionsPromptsInStatusBar->setEnabled(!cbClassicStatusBar->isChecked());
}

void QG_DlgOptionsGeneral::onTabCloseButtonChanged(){
    cbTabCloseButtonMode->setEnabled(cbTabCloseButton->isChecked());
}

void QG_DlgOptionsGeneral::onInfoCursorPromptChanged(){
}

void QG_DlgOptionsGeneral::onInfoCursorAbsolutePositionChanged(){
}

void QG_DlgOptionsGeneral::onInfoCursorRelativeChanged(){
}

void QG_DlgOptionsGeneral::onInfoCursorSnapChanged(){
}

void QG_DlgOptionsGeneral::on_cbGridExtendAxisLinesToggled(){
    bool extend = cbGridExtendAxisLines->isChecked();
    sbAxisSize->setEnabled(!extend);
    cbXAxisAreas->setEnabled(extend);
    cbYAxisAreas->setEnabled(extend);
}

void QG_DlgOptionsGeneral::onCheckNewVersionChanged(){
    if (cbCheckNewVersion->isChecked()) {
        cbCheckNewVersionIgnorePreRelease->setEnabled(!XSTR(LC_PRERELEASE));
    } else {
        cbCheckNewVersionIgnorePreRelease->setEnabled(false);
    }
}

void QG_DlgOptionsGeneral::onAutoBackupChanged([[maybe_unused]] int state){
    bool allowBackup = cbAutoBackup->isChecked();
    cbAutoSaveTime->setEnabled(allowBackup);
    auto &appWindow = QC_ApplicationWindow::getAppWindow(); // fixme - sand - files - remove static
    appWindow->startAutoSaveTimer(allowBackup);
}

void QG_DlgOptionsGeneral::initReferencePoints(){
    int pdmode;
    QString pdsizeStr;
    LC_GROUP_GUARD("Appearance"); {
        // Points drawing style:
        pdmode = LC_GET_INT("RefPointType",DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot));
        pdsizeStr = LC_GET_STR("RefPointSize", "2.0");
    }

    // Set button checked for the currently selected point style
    switch (pdmode) {
        case DXF_FORMAT_PDMode_CentreDot:
        default:
            bDot->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreBlank:
            bBlank->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentrePlus:
            bPlus->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreCross:
            bCross->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreTick:
            bTick->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreDot):
            bDotCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreBlank):
            bBlankCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentrePlus):
            bPlusCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreCross):
            bCrossCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreTick):
            bTickCircle->setChecked(true);
            break;

        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot):
            bDotSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreBlank):
            bBlankSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentrePlus):
            bPlusSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreCross):
            bCrossSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreTick):
            bTickSquare->setChecked(true);
            break;

        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreDot):
            bDotCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreBlank):
            bBlankCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentrePlus):
            bPlusCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreCross):
            bCrossCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreTick):
            bTickCircleSquare->setChecked(true);
            break;
    }

    // Fill points display size value string, and set button checked for screen-size
    // relative vs. absolute drawing units radio buttons. Negative pdsize => value
    // gives points size as percent of screen size; positive pdsize => value gives
    // points size in absolute drawing units; pdsize == 0 implies points size to be
    // 5% relative to screen size.

    bool ok;
    double pdsize = RS_Math::eval(pdsizeStr, &ok);
    if (!ok) {
        pdsize = LC_DEFAULTS_PDSize;
    }
    if (pdsize <= 0.0)
        rbRelSize->setChecked(true);
    else
        rbAbsSize->setChecked(true);

    lePointSize->setText(QString::number(std::abs(pdsize), 'g', 6));

    // Set the appropriate text for the display size value label
    updateLPtSzUnits();
}

void QG_DlgOptionsGeneral::updateLPtSzUnits(){
    //	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::updateLPtSzUnits, rbRelSize->isChecked() = %d",rbRelSize->isChecked());
    if (rbRelSize->isChecked())
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Screen %", nullptr));
    else
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Dwg Units", nullptr));
}

void QG_DlgOptionsGeneral::saveReferencePoints(){
    // Points drawing style:
    // Get currently selected point style from which button is checked
    int pdmode = LC_DEFAULTS_PDMode;

    if (bDot->isChecked())
        pdmode = DXF_FORMAT_PDMode_CentreDot;
    else if (bBlank->isChecked())
        pdmode = DXF_FORMAT_PDMode_CentreBlank;
    else if (bPlus->isChecked())
        pdmode = DXF_FORMAT_PDMode_CentrePlus;
    else if (bCross->isChecked())
        pdmode = DXF_FORMAT_PDMode_CentreCross;
    else if (bTick->isChecked())
        pdmode = DXF_FORMAT_PDMode_CentreTick;

    else if (bDotCircle->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreDot);
    else if (bBlankCircle->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreBlank);
    else if (bPlusCircle->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentrePlus);
    else if (bCrossCircle->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreCross);
    else if (bTickCircle->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreTick);

    else if (bDotSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);
    else if (bBlankSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreBlank);
    else if (bPlusSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentrePlus);
    else if (bCrossSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreCross);
    else if (bTickSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreTick);

    else if (bDotCircleSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreDot);
    else if (bBlankCircleSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreBlank);
    else if (bPlusCircleSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentrePlus);
    else if (bCrossCircleSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreCross);
    else if (bTickCircleSquare->isChecked())
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreTick);

    // Get points display size from the value string and the relative vs. absolute
    // size radio buttons state
    bool ok;
    double pdsize = RS_Math::eval(lePointSize->text(), &ok);
    if (!ok)
        pdsize = LC_DEFAULTS_PDSize;

    if (pdsize > 0.0 && rbRelSize->isChecked())
        pdsize = -pdsize;

    QString pdsizeStr = QString::number(pdsize);

    LC_GROUP_GUARD("Appearance"); {
        // Points drawing style:
        LC_SET("RefPointType", pdmode);
        LC_SET("RefPointSize", pdsizeStr);
    }
}

void QG_DlgOptionsGeneral::on_rbRelSize_toggled([[maybe_unused]] bool checked){
    //	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::on_rbRelSize_toggled, checked = %d",checked);
    updateLPtSzUnits();
}

void QG_DlgOptionsGeneral::exportSettings(){
    LC_SettingsExporter exporter;
    exporter.exportSettings(this);
}

void QG_DlgOptionsGeneral::importSettings(){
    LC_SettingsExporter importer;
    if (importer.importSettings(this)) {
        init();
        QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow(); // fixme - sand - files - remove static
        appWin.initSettings();
    }
}
