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

#include <QMessageBox>
#include "qc_applicationwindow.h"
#include <QColorDialog>

#include "qg_filedialog.h"

#include "rs_debug.h"
#include "rs_system.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "lc_defaults.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgOptionsGeneral as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */

int QG_DlgOptionsGeneral::current_tab = 0;

QG_DlgOptionsGeneral::QG_DlgOptionsGeneral(QWidget* parent)
    : LC_Dialog(parent, "OptionsGeneral"){
    setModal(false);
    setupUi(this);
    tabWidget->setCurrentIndex(current_tab);
    init();
    connect(variablefile_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setVariableFile);
    connect(fonts_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setFontsFolder);

    connect(translation_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setTranslationsFolder);

    connect(hatchpatterns_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setHatchPatternsFolder);

    connect(cbAutoBackup, &QCheckBox::stateChanged,
            this, &QG_DlgOptionsGeneral::onAutoBackupChanged);

    connect(cbVisualizeHovering, &QCheckBox::stateChanged,
            this, &QG_DlgOptionsGeneral::on_cbVisualizeHoveringClicked);

    connect(cbPersistentDialogs, &QCheckBox::stateChanged,
            this, &QG_DlgOptionsGeneral::on_cbPersistentDialogsClicked);


    connect(tbShortcuts, &QToolButton::clicked, this, &QG_DlgOptionsGeneral::setShortcutsMappingsFoler);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsGeneral::languageChange() {
    retranslateUi(this);
}

void QG_DlgOptionsGeneral::init() {
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

    LC_GROUP("Appearance");
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

        QString indicator_lines_type = LC_GET_STR("indicator_lines_type", "Crosshair");
        int index = indicator_lines_combobox->findText(indicator_lines_type);
        indicator_lines_combobox->setCurrentIndex(index);

        bool indicator_shape_state = LC_GET_BOOL("indicator_shape_state", true);
        indicator_shape_checkbox->setChecked(indicator_shape_state);

        QString indicator_shape_type = LC_GET_STR("indicator_shape_type", "Circle");
        index = indicator_shape_combobox->findText(indicator_shape_type);
        indicator_shape_combobox->setCurrentIndex(index);

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

        checked = LC_GET_BOOL("UnitlessGrid");
        cb_unitless_grid->setChecked(checked);

        checked = LC_GET_BOOL("Autopanning");
        cb_autopanning->setChecked(checked);

        checked = LC_GET_INT("ScrollBars");
        scrollbars_check_box->setChecked(checked);

        // preview:
        initComboBox(cbMaxPreview, LC_GET_STR("MaxPreview", "100"));

        checked = LC_GET_BOOL("ShowKeyboardShortcutsInTooltips", true);
        cbShowKeyboardShortcutsInToolTips->setChecked(checked);
    }
    LC_GROUP_END();

    LC_GROUP("Colors");
    {
        initComboBox(cbBackgroundColor, LC_GET_STR("background", RS_Settings::background));
        initComboBox(cbGridColor, LC_GET_STR("grid", RS_Settings::grid));
        initComboBox(cbMetaGridColor, LC_GET_STR("meta_grid", RS_Settings::meta_grid));
        initComboBox(cbSelectedColor, LC_GET_STR("select", RS_Settings::select));
        initComboBox(cbHighlightedColor, LC_GET_STR("highlight", RS_Settings::highlight));
        initComboBox(cbStartHandleColor, LC_GET_STR("start_handle", RS_Settings::start_handle));
        initComboBox(cbHandleColor, LC_GET_STR("handle", RS_Settings::handle));
        initComboBox(cbEndHandleColor, LC_GET_STR("end_handle", RS_Settings::end_handle));
        initComboBox(cbRelativeZeroColor, LC_GET_STR("relativeZeroColor", RS_Settings::relativeZeroColor));
        initComboBox(cbPreviewRefColor, LC_GET_STR("previewReferencesColor", RS_Settings::previewRefColor));
        initComboBox(cbPreviewRefHighlightColor,
                     LC_GET_STR("previewReferencesHighlightColor", RS_Settings::previewRefHighlightColor));
        initComboBox(cb_snap_color, LC_GET_STR("snap_indicator", RS_Settings::snap_indicator));
    }
    LC_GROUP_END();

    LC_GROUP("Paths");
    {
        lePathTranslations->setText(LC_GET_STR("Translations", ""));
        lePathHatch->setText(LC_GET_STR("Patterns", ""));
        lePathFonts->setText(LC_GET_STR("Fonts", ""));
        lePathLibrary->setText(LC_GET_STR("Library", "").trimmed());
        leTemplate->setText(LC_GET_STR("Template", "").trimmed());
        variablefile_field->setText(LC_GET_STR("VariableFile", "").trimmed());
        leShortcutsMappingDirectory->setText(LC_GET_STR("ShortcutsMappings", "").trimmed());
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

    LC_GROUP("Defaults");
    {
//    cbUnit->setCurrentIndex( cbUnit->findText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit) )) );
        cbUnit->setCurrentIndex(cbUnit->findText(QObject::tr(LC_GET_STR("Unit", def_unit).toUtf8().data())));
        // Auto save timer
        cbAutoSaveTime->setValue(LC_GET_INT("AutoSaveTime", 5));
        cbAutoBackup->setChecked(LC_GET_BOOL("AutoBackupDocument", true));
        cbUseQtFileOpenDialog->setChecked(LC_GET_BOOL("UseQtFileOpenDialog", true));
        cbWheelScrollInvertH->setChecked(LC_GET_BOOL("WheelScrollInvertH"));
        cbWheelScrollInvertV->setChecked(LC_GET_BOOL("WheelScrollInvertV"));
        cbInvertZoomDirection->setChecked(LC_GET_BOOL("InvertZoomDirection"));
        cbAngleSnap->setCurrentIndex(LC_GET_INT("AngleSnapStep", 3));
    }
    LC_GROUP_END();

//update entities to selected entities to the current active layer
    LC_GROUP("Modify");
    {
        auto toActive = LC_GET_BOOL("ModifyEntitiesToActiveLayer");
        cbToActiveLayer->setChecked(toActive);
        LC_SET("ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked());
    }
    LC_GROUP_END();

    LC_GROUP("CADPreferences");
    {
        cbAutoZoomDrawing->setChecked(LC_GET_BOOL("AutoZoomDrawing"));
    }
    LC_GROUP_END();

    LC_GROUP("Startup");
    {
        cbSplash->setChecked(LC_GET_BOOL("ShowSplash", true));
        tab_mode_check_box->setChecked(LC_GET_BOOL("TabMode"));
        maximize_checkbox->setChecked(LC_GET_BOOL("Maximize"));
        left_sidebar_checkbox->setChecked(LC_GET_BOOL("EnableLeftSidebar", true));
        cad_toolbars_checkbox->setChecked(LC_GET_BOOL("EnableCADToolbars", true));
        cbOpenLastFiles->setChecked(LC_GET_BOOL("OpenLastOpenedFiles", true));
    }
    LC_GROUP_END();

    LC_GROUP("Keyboard");
    {
        cbEvaluateOnSpace->setChecked(LC_GET_BOOL("EvaluateCommandOnSpace"));
        cbToggleFreeSnapOnSpace->setChecked(LC_GET_BOOL("ToggleFreeSnapOnSpace"));
    }

    initReferencePoints();

    restartNeeded = false;
}

void QG_DlgOptionsGeneral::initComboBox(QComboBox *cb, const QString &text) {
    int idx = cb->findText(text);
    if (idx < 0) {
        idx = 0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex(idx);
}

void QG_DlgOptionsGeneral::setRestartNeeded() {
    restartNeeded = true;
}

void QG_DlgOptionsGeneral::setTemplateFile() {
    RS2::FormatType type = RS2::FormatDXFRW;
    QG_FileDialog dlg(this);
    QString fileName = dlg.getOpenFile(&type);
    leTemplate->setText(fileName);
}

void QG_DlgOptionsGeneral::ok(){
    if (RS_Settings::save_is_allowed){
        //RS_SYSTEM->loadTranslation(cbLanguage->currentText());
        LC_GROUP("Appearance");
        {
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
            LC_SET("indicator_lines_type", indicator_lines_combobox->currentText());
            LC_SET("indicator_shape_state", indicator_shape_checkbox->isChecked());
            LC_SET("indicator_shape_type", indicator_shape_combobox->currentText());
            LC_SET("cursor_hiding", cursor_hiding_checkbox->isChecked());
            LC_SET("showSnapOptionsInSnapToolbar", cbShowSnapOptionsInSnapBar->isChecked());
            LC_SET("UnitlessGrid", cb_unitless_grid->isChecked());
            LC_SET("Antialiasing", cb_antialiasing->isChecked());
            LC_SET("Autopanning", cb_autopanning->isChecked());
            LC_SET("ScrollBars", scrollbars_check_box->isChecked());
            LC_SET("ShowKeyboardShortcutsInTooltips", cbShowKeyboardShortcutsInToolTips->isChecked());
            LC_SET("PersistDialogPositions", cbPersistentDialogs->isChecked());
            LC_SET("PersistDialogRestoreSizeOnly", cbPersistentDialogSizeOnly->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Colors");
        {
            LC_SET("background", cbBackgroundColor->currentText());
            LC_SET("grid", cbGridColor->currentText());
            LC_SET("meta_grid", cbMetaGridColor->currentText());
            LC_SET("select", cbSelectedColor->currentText());
            LC_SET("highlight", cbHighlightedColor->currentText());
            LC_SET("start_handle", cbStartHandleColor->currentText());
            LC_SET("handle", cbHandleColor->currentText());
            LC_SET("end_handle", cbEndHandleColor->currentText());
            LC_SET("relativeZeroColor", cbRelativeZeroColor->currentText());
            LC_SET("previewReferencesColor", cbPreviewRefColor->currentText());
            LC_SET("previewReferencesHighlightColor", cbPreviewRefHighlightColor->currentText());
            LC_SET("snap_indicator", cb_snap_color->currentText());
        }
        LC_GROUP_END();

        LC_GROUP("Paths");
        {
            LC_SET("Translations", lePathTranslations->text());
            LC_SET("Patterns", lePathHatch->text());
            LC_SET("Fonts", lePathFonts->text());
            LC_SET("Library", lePathLibrary->text());
            LC_SET("Template", leTemplate->text());
            LC_SET("VariableFile", variablefile_field->text());
            LC_SET("ShortcutsMappings", leShortcutsMappingDirectory->text());
        }
        LC_GROUP_END();

        LC_GROUP("Defaults");
        {
            LC_SET("Unit", RS_Units::unitToString(RS_Units::stringToUnit(cbUnit->currentText()), false/*untr.*/));
            LC_SET("AutoSaveTime", cbAutoSaveTime->value());
            LC_SET("AutoBackupDocument", cbAutoBackup->isChecked());
            LC_SET("UseQtFileOpenDialog", cbUseQtFileOpenDialog->isChecked());
            LC_SET("WheelScrollInvertH", cbWheelScrollInvertH->isChecked());
            LC_SET("WheelScrollInvertV", cbWheelScrollInvertV->isChecked());
            LC_SET("InvertZoomDirection", cbInvertZoomDirection->isChecked());
            LC_SET("AngleSnapStep", cbAngleSnap->currentIndex());
        }
        LC_GROUP_END();

        //update entities to selected entities to the current active layer
        LC_GROUP("Modify");
        {
            LC_SET("ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("CADPreferences");
        {
            LC_SET("AutoZoomDrawing", cbAutoZoomDrawing->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Startup");
        {
            LC_SET("ShowSplash", cbSplash->isChecked());
            LC_SET("TabMode", tab_mode_check_box->isChecked());
            LC_SET("Maximize", maximize_checkbox->isChecked());
            LC_SET("EnableLeftSidebar", left_sidebar_checkbox->isChecked());
            LC_SET("EnableCADToolbars", cad_toolbars_checkbox->isChecked());
            LC_SET("OpenLastOpenedFiles", cbOpenLastFiles->isChecked());
        }
        LC_GROUP_END();

        LC_GROUP("Keyboard");
        {
            LC_SET("EvaluateCommandOnSpace", cbEvaluateOnSpace->isChecked());
            LC_SET("ToggleFreeSnapOnSpace", cbToggleFreeSnapOnSpace->isChecked());
        }
        LC_GROUP_END();
        saveReferencePoints();
    }
    RS_SETTINGS->emitOptionsChanged();
    if (restartNeeded == true) {
        QMessageBox::warning(this, tr("Preferences"),
                             tr("Please restart the application to apply all changes."));
    }
    accept();
}

void QG_DlgOptionsGeneral::on_tabWidget_currentChanged(int index){
    current_tab = index;
}

void QG_DlgOptionsGeneral::set_color(QComboBox *combo, QColor custom) {
    QColor current = QColor::fromString(combo->lineEdit()->text());

    QColorDialog dlg;
    dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        combo->lineEdit()->setText(color.name());
    }
}

void QG_DlgOptionsGeneral::on_pb_background_clicked() {
    set_color(cbBackgroundColor, QColor(RS_Settings::background));
}

void QG_DlgOptionsGeneral::on_pb_grid_clicked() {
    set_color(cbGridColor, QColor(RS_Settings::grid));
}

void QG_DlgOptionsGeneral::on_pb_meta_clicked() {
    set_color(cbMetaGridColor, QColor(RS_Settings::meta_grid));
}

void QG_DlgOptionsGeneral::on_pb_selected_clicked() {
    set_color(cbSelectedColor, QColor(RS_Settings::select));
}

void QG_DlgOptionsGeneral::on_pb_highlighted_clicked() {
    set_color(cbHighlightedColor, QColor(RS_Settings::highlight));
}

void QG_DlgOptionsGeneral::on_pb_start_clicked() {
    set_color(cbStartHandleColor, QColor(RS_Settings::start_handle));
}

void QG_DlgOptionsGeneral::on_pb_handle_clicked() {
    set_color(cbHandleColor, QColor(RS_Settings::handle));
}

void QG_DlgOptionsGeneral::on_pb_end_clicked() {
    set_color(cbEndHandleColor, QColor(RS_Settings::end_handle));
}

void QG_DlgOptionsGeneral::on_pb_snap_color_clicked() {
    set_color(cb_snap_color, QColor(RS_Settings::snap_indicator));
}

void QG_DlgOptionsGeneral::on_pb_relativeZeroColor_clicked() {
    set_color(cbRelativeZeroColor, QColor(RS_Settings::relativeZeroColor));
}

void QG_DlgOptionsGeneral::on_pb_previewRefColor_clicked() {
    set_color(cbPreviewRefColor, QColor(RS_Settings::previewRefColor));
}

void QG_DlgOptionsGeneral::on_pb_previewRefHighlightColor_clicked() {
    set_color(cbPreviewRefHighlightColor, QColor(RS_Settings::previewRefHighlightColor));
}

void QG_DlgOptionsGeneral::on_pb_clear_all_clicked() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear settings"),
                                  tr("This will also include custom menus and toolbars. Continue?"),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        RS_SETTINGS->clear_all();
        QMessageBox::information(this, "info", "You must restart LibreCAD to see the changes.");
    }
}

void QG_DlgOptionsGeneral::on_pb_clear_geometry_clicked() {
    RS_SETTINGS->clear_geometry();
    QMessageBox::information(this, "info", "You must restart LibreCAD to see the changes.");
}

void QG_DlgOptionsGeneral::setVariableFile() {
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
void QG_DlgOptionsGeneral::setFontsFolder() {
    QString folder = selectFolder("Select Fonts Folder");
    if (folder != nullptr) {
        lePathFonts->setText(QDir::toNativeSeparators(folder));
    }
}

void QG_DlgOptionsGeneral::setTranslationsFolder() {
    QString folder = selectFolder("Select Translations Folder");
    if (folder != nullptr) {
        lePathTranslations->setText(QDir::toNativeSeparators(folder));
    }
}

void QG_DlgOptionsGeneral::setHatchPatternsFolder() {
    QString folder = selectFolder("Select Hatch Patterns Folder");
    if (folder != nullptr) {
        lePathHatch->setText(QDir::toNativeSeparators(folder));
    }
}
void QG_DlgOptionsGeneral::setShortcutsMappingsFoler() {
    QString folder = selectFolder("Select Shortcuts Mappings Folder");
    if (folder != nullptr) {
        leShortcutsMappingDirectory->setText(QDir::toNativeSeparators(folder));
    }
}

QString QG_DlgOptionsGeneral::selectFolder(const char* title) {
    QString folder = nullptr;
    QFileDialog dlg(this);
    if (title != nullptr) {
        QString dlgTitle = tr(title);
        dlg.setWindowTitle(dlgTitle);
    }
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);

    if (dlg.exec()) {
        folder = dlg.selectedFiles()[0];
    }
    return folder;
}

void QG_DlgOptionsGeneral::setLibraryPath() {
    QG_FileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);

    if (dlg.exec()) {
        auto dir = dlg.selectedFiles()[0];
        lePathLibrary->setText(QDir::toNativeSeparators(dir));
        setRestartNeeded();
    }
}

void QG_DlgOptionsGeneral::on_cbVisualizeHoveringClicked() {
    cbShowRefPointsOnHovering->setEnabled(cbVisualizeHovering->isChecked());
}

void QG_DlgOptionsGeneral::on_cbPersistentDialogsClicked() {
    cbPersistentDialogSizeOnly->setEnabled(cbPersistentDialogs->isChecked());
}

void QG_DlgOptionsGeneral::onAutoBackupChanged([[maybe_unused]] int state) {
    bool allowBackup = cbAutoBackup->checkState() == Qt::Checked;
    auto &appWindow = QC_ApplicationWindow::getAppWindow();
    appWindow->startAutoSave(allowBackup);
}

void QG_DlgOptionsGeneral::initReferencePoints() {
    int pdmode;
    QString pdsizeStr;
    LC_GROUP_GUARD("Appearance");
    {
        // Points drawing style:
        pdmode = LC_GET_INT("RefPointType",DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot));
        pdsizeStr= LC_GET_STR("RefPointSize", "2.0");
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

void QG_DlgOptionsGeneral::updateLPtSzUnits() {
//	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::updateLPtSzUnits, rbRelSize->isChecked() = %d",rbRelSize->isChecked());
    if (rbRelSize->isChecked())
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Screen %", nullptr));
    else
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Dwg Units", nullptr));
}

void QG_DlgOptionsGeneral::saveReferencePoints() {
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

    LC_GROUP_GUARD("Appearance");
    {
        // Points drawing style:
        LC_SET("RefPointType", pdmode);
        LC_SET("RefPointSize", pdsizeStr);
    }

}

void QG_DlgOptionsGeneral::on_rbRelSize_toggled([[maybe_unused]] bool checked) {
//	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::on_rbRelSize_toggled, checked = %d",checked);
    updateLPtSzUnits();
}
