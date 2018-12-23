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
#include <QColorDialog>
#include "rs_system.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "qg_filedialog.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_DlgOptionsGeneral as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */

int QG_DlgOptionsGeneral::current_tab = 0;

QG_DlgOptionsGeneral::QG_DlgOptionsGeneral(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
    tabWidget->setCurrentIndex(current_tab);
    init();
    connect(variablefile_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setVariableFile);
    connect(fonts_button, &QToolButton::clicked,
            this, &QG_DlgOptionsGeneral::setFontsFolder);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsGeneral::~QG_DlgOptionsGeneral()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsGeneral::languageChange()
{
    retranslateUi(this);
}

void QG_DlgOptionsGeneral::init()
{
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    languageList.sort();
    languageList.prepend("en");
	for(auto const& lang: languageList){

        RS_DEBUG->print("QG_DlgOptionsGeneral::init: adding %s to combobox",
						lang.toLatin1().data());

		QString l = RS_SYSTEM->symbolToLanguage(lang);
		if (!l.isEmpty() && cbLanguage->findData(lang)==-1) {
            RS_DEBUG->print("QG_DlgOptionsGeneral::init: %s", l.toLatin1().data());
			cbLanguage->addItem(l,lang);
			cbLanguageCmd->addItem(l, lang);
        }
    }

    RS_SETTINGS->beginGroup("/Appearance");

    // set current language:
    QString def_lang = "en";

    QString lang = RS_SETTINGS->readEntry("/Language", def_lang);
    cbLanguage->setCurrentIndex( cbLanguage->findText(RS_SYSTEM->symbolToLanguage(lang)) );

    QString langCmd = RS_SETTINGS->readEntry("/LanguageCmd", def_lang);
    cbLanguageCmd->setCurrentIndex( cbLanguageCmd->findText(RS_SYSTEM->symbolToLanguage(langCmd)) );

    // graphic view:

    // Snap Indicators
    bool indicator_lines_state = RS_SETTINGS->readNumEntry("/indicator_lines_state", 1);
    indicator_lines_checkbox->setChecked(indicator_lines_state);

    QString indicator_lines_type = RS_SETTINGS->readEntry("/indicator_lines_type", "Crosshair");
    int index = indicator_lines_combobox->findText(indicator_lines_type);
    indicator_lines_combobox->setCurrentIndex(index);

    bool indicator_shape_state = RS_SETTINGS->readNumEntry("/indicator_shape_state", 1);
    indicator_shape_checkbox->setChecked(indicator_shape_state);

    QString indicator_shape_type = RS_SETTINGS->readEntry("/indicator_shape_type", "Circle");
    index = indicator_shape_combobox->findText(indicator_shape_type);
    indicator_shape_combobox->setCurrentIndex(index);

    bool cursor_hiding = RS_SETTINGS->readNumEntry("/cursor_hiding", 0);
    cursor_hiding_checkbox->setChecked(cursor_hiding);
    
    // scale grid:
    QString scaleGrid = RS_SETTINGS->readEntry("/ScaleGrid", "1");
    cbScaleGrid->setChecked(scaleGrid=="1");
    QString minGridSpacing = RS_SETTINGS->readEntry("/MinGridSpacing", "10");
    cbMinGridSpacing->setCurrentIndex( cbMinGridSpacing->findText(minGridSpacing) );

    int checked = RS_SETTINGS->readNumEntry("/Antialiasing");
    cb_antialiasing->setChecked(checked?true:false);

    checked = RS_SETTINGS->readNumEntry("/ScrollBars");
    scrollbars_check_box->setChecked(checked?true:false);

    // preview:
	initComboBox(cbMaxPreview, RS_SETTINGS->readEntry("/MaxPreview", "100"));

    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("Colors");
    initComboBox(cbBackgroundColor, RS_SETTINGS->readEntry("/background", Colors::background));
    initComboBox(cbGridColor, RS_SETTINGS->readEntry("/grid", Colors::grid));
    initComboBox(cbMetaGridColor, RS_SETTINGS->readEntry("/meta_grid", Colors::meta_grid));
    initComboBox(cbSelectedColor, RS_SETTINGS->readEntry("/select", Colors::select));
    initComboBox(cbHighlightedColor, RS_SETTINGS->readEntry("/highlight", Colors::highlight));
    initComboBox(cbStartHandleColor, RS_SETTINGS->readEntry("/start_handle", Colors::start_handle));
    initComboBox(cbHandleColor, RS_SETTINGS->readEntry("/handle", Colors::handle));
    initComboBox(cbEndHandleColor, RS_SETTINGS->readEntry("/end_handle", Colors::end_handle));
    initComboBox(cb_snap_color, RS_SETTINGS->readEntry("/snap_indicator", Colors::snap_indicator));
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Paths");

    lePathTranslations->setText(RS_SETTINGS->readEntry("/Translations", ""));
    lePathHatch->setText(RS_SETTINGS->readEntry("/Patterns", ""));
    lePathFonts->setText(RS_SETTINGS->readEntry("/Fonts", ""));
    lePathLibrary->setText(RS_SETTINGS->readEntry("/Library", "").trimmed());
    leTemplate->setText(RS_SETTINGS->readEntry("/Template", "").trimmed());
    variablefile_field->setText(RS_SETTINGS->readEntry("/VariableFile", "").trimmed());
    RS_SETTINGS->endGroup();

    // units:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        if (i!=(int)RS2::None)
            cbUnit->addItem(RS_Units::unitToString((RS2::Unit)i));
    }
    // RVT_PORT cbUnit->listBox()->sort();
    cbUnit->insertItem( 0, RS_Units::unitToString(RS2::None) );

    QString def_unit = "Millimeter";

    RS_SETTINGS->beginGroup("/Defaults");
//    cbUnit->setCurrentIndex( cbUnit->findText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit) )) );
    cbUnit->setCurrentIndex( cbUnit->findText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit).toUtf8().data() )) );
    // Auto save timer
    cbAutoSaveTime->setValue(RS_SETTINGS->readNumEntry("/AutoSaveTime", 5));
    cbAutoBackup->setChecked(RS_SETTINGS->readNumEntry("/AutoBackupDocument", 1));
    cbUseQtFileOpenDialog->setChecked(RS_SETTINGS->readNumEntry("/UseQtFileOpenDialog", 1));
    cbWheelScrollInvertH->setChecked(RS_SETTINGS->readNumEntry("/WheelScrollInvertH", 0));
    cbWheelScrollInvertV->setChecked(RS_SETTINGS->readNumEntry("/WheelScrollInvertV", 0));
    RS_SETTINGS->endGroup();

	//update entities to selected entities to the current active layer
	RS_SETTINGS->beginGroup("/Modify");
	auto toActive=RS_SETTINGS->readNumEntry("/ModifyEntitiesToActiveLayer", 0);
	cbToActiveLayer->setChecked(toActive==1);
	RS_SETTINGS->writeEntry("/ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked()?1:0);
	RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("Startup");
    cbSplash->setChecked(RS_SETTINGS->readNumEntry("/ShowSplash",1)==1);
    tab_mode_check_box->setChecked(RS_SETTINGS->readNumEntry("/TabMode", 0));
    maximize_checkbox->setChecked(RS_SETTINGS->readNumEntry("/Maximize", 0));
    left_sidebar_checkbox->setChecked(RS_SETTINGS->readNumEntry("/EnableLeftSidebar", 1));
    cad_toolbars_checkbox->setChecked(RS_SETTINGS->readNumEntry("/EnableCADToolbars", 1));
    RS_SETTINGS->endGroup();

    restartNeeded = false;
}

void QG_DlgOptionsGeneral::initComboBox(QComboBox* cb, QString text) {
	int idx = cb->findText(text);
	if( idx < 0) {
		idx =0;
		cb->insertItem(idx, text);
	}
	cb->setCurrentIndex( idx );
}

void QG_DlgOptionsGeneral::destroy() {
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

void QG_DlgOptionsGeneral::ok()
{
    if (RS_Settings::save_is_allowed)
    {
        //RS_SYSTEM->loadTranslation(cbLanguage->currentText());
        RS_SETTINGS->beginGroup("/Appearance");
        RS_SETTINGS->writeEntry("/ScaleGrid", QString("%1").arg((int)cbScaleGrid->isChecked()));
        RS_SETTINGS->writeEntry("/MinGridSpacing", cbMinGridSpacing->currentText());
        RS_SETTINGS->writeEntry("/MaxPreview", cbMaxPreview->currentText());
        RS_SETTINGS->writeEntry("/Language",cbLanguage->itemData(cbLanguage->currentIndex()));
        RS_SETTINGS->writeEntry("/LanguageCmd",cbLanguageCmd->itemData(cbLanguageCmd->currentIndex()));
        RS_SETTINGS->writeEntry("/indicator_lines_state", indicator_lines_checkbox->isChecked());
        RS_SETTINGS->writeEntry("/indicator_lines_type", indicator_lines_combobox->currentText());
        RS_SETTINGS->writeEntry("/indicator_shape_state", indicator_shape_checkbox->isChecked());      
        RS_SETTINGS->writeEntry("/indicator_shape_type", indicator_shape_combobox->currentText());
        RS_SETTINGS->writeEntry("/cursor_hiding", cursor_hiding_checkbox->isChecked());
        RS_SETTINGS->writeEntry("/Antialiasing", cb_antialiasing->isChecked()?1:0);
        RS_SETTINGS->writeEntry("/ScrollBars", scrollbars_check_box->isChecked()?1:0);
        RS_SETTINGS->endGroup();

        RS_SETTINGS->beginGroup("Colors");
        RS_SETTINGS->writeEntry("/background", cbBackgroundColor->currentText());
        RS_SETTINGS->writeEntry("/grid", cbGridColor->currentText());
        RS_SETTINGS->writeEntry("/meta_grid", cbMetaGridColor->currentText());
        RS_SETTINGS->writeEntry("/select", cbSelectedColor->currentText());
        RS_SETTINGS->writeEntry("/highlight", cbHighlightedColor->currentText());
        RS_SETTINGS->writeEntry("/start_handle", cbStartHandleColor->currentText());
        RS_SETTINGS->writeEntry("/handle", cbHandleColor->currentText());
        RS_SETTINGS->writeEntry("/end_handle", cbEndHandleColor->currentText());
        RS_SETTINGS->writeEntry("/snap_indicator", cb_snap_color->currentText());
        RS_SETTINGS->endGroup();

        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/Translations", lePathTranslations->text());
        RS_SETTINGS->writeEntry("/Patterns", lePathHatch->text());
        RS_SETTINGS->writeEntry("/Fonts", lePathFonts->text());
        RS_SETTINGS->writeEntry("/Library", lePathLibrary->text());
        RS_SETTINGS->writeEntry("/Template", leTemplate->text());
        RS_SETTINGS->writeEntry("/VariableFile", variablefile_field->text());
        RS_SETTINGS->endGroup();

        RS_SETTINGS->beginGroup("/Defaults");
        RS_SETTINGS->writeEntry("/Unit",
            RS_Units::unitToString( RS_Units::stringToUnit( cbUnit->currentText() ), false/*untr.*/) );
        RS_SETTINGS->writeEntry("/AutoSaveTime", cbAutoSaveTime->value() );
        RS_SETTINGS->writeEntry("/AutoBackupDocument", cbAutoBackup->isChecked() ? 1 : 0);
        RS_SETTINGS->writeEntry("/UseQtFileOpenDialog", cbUseQtFileOpenDialog->isChecked() ? 1 : 0);
        RS_SETTINGS->writeEntry("/WheelScrollInvertH", cbWheelScrollInvertH->isChecked() ? 1 : 0);
        RS_SETTINGS->writeEntry("/WheelScrollInvertV", cbWheelScrollInvertV->isChecked() ? 1 : 0);
        RS_SETTINGS->endGroup();

        //update entities to selected entities to the current active layer
        RS_SETTINGS->beginGroup("/Modify");
        RS_SETTINGS->writeEntry("/ModifyEntitiesToActiveLayer", cbToActiveLayer->isChecked()?1:0);
        RS_SETTINGS->endGroup();

        RS_SETTINGS->beginGroup("Startup");
        RS_SETTINGS->writeEntry("/ShowSplash", cbSplash->isChecked()?1:0);
        RS_SETTINGS->writeEntry("/TabMode", tab_mode_check_box->isChecked()?1:0);
        RS_SETTINGS->writeEntry("/Maximize", maximize_checkbox->isChecked()?1:0);
        RS_SETTINGS->writeEntry("/EnableLeftSidebar", left_sidebar_checkbox->isChecked()?1:0);
        RS_SETTINGS->writeEntry("/EnableCADToolbars", cad_toolbars_checkbox->isChecked()?1:0);
        RS_SETTINGS->endGroup();
    }


    if (restartNeeded==true) {
        QMessageBox::warning( this, tr("Preferences"),
                              tr("Please restart the application to apply all changes."),
                              QMessageBox::Ok,
                              Qt::NoButton);
    }
    accept();
}


void QG_DlgOptionsGeneral::on_tabWidget_currentChanged(int index)
{
    current_tab = index;
}

void QG_DlgOptionsGeneral::set_color(QComboBox* combo, QColor custom)
{
    QColor current;
    current.setNamedColor(combo->lineEdit()->text());

    QColorDialog dlg;
	dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
        combo->lineEdit()->setText(color.name());
    }
}

void QG_DlgOptionsGeneral::on_pb_background_clicked()
{
    set_color(cbBackgroundColor, QColor(Colors::background));
}

void QG_DlgOptionsGeneral::on_pb_grid_clicked()
{
    set_color(cbGridColor, QColor(Colors::grid));
}

void QG_DlgOptionsGeneral::on_pb_meta_clicked()
{
    set_color(cbMetaGridColor, QColor(Colors::meta_grid));
}

void QG_DlgOptionsGeneral::on_pb_selected_clicked()
{
    set_color(cbSelectedColor, QColor(Colors::select));
}

void QG_DlgOptionsGeneral::on_pb_highlighted_clicked()
{
    set_color(cbHighlightedColor, QColor(Colors::highlight));
}

void QG_DlgOptionsGeneral::on_pb_start_clicked()
{
    set_color(cbStartHandleColor, QColor(Colors::start_handle));
}

void QG_DlgOptionsGeneral::on_pb_handle_clicked()
{
    set_color(cbHandleColor, QColor(Colors::handle));
}

void QG_DlgOptionsGeneral::on_pb_end_clicked()
{
    set_color(cbEndHandleColor, QColor(Colors::end_handle));
}

void QG_DlgOptionsGeneral::on_pb_snap_color_clicked()
{
    set_color(cb_snap_color, QColor(Colors::snap_indicator));
}

void QG_DlgOptionsGeneral::on_pb_clear_all_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear settings"),
                                tr("This will also include custom menus and toolbars. Continue?"),
                                QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
      RS_SETTINGS->clear_all();
      QMessageBox::information(this, "info", "You must restart LibreCAD to see the changes.");
    }
}

void QG_DlgOptionsGeneral::on_pb_clear_geometry_clicked()
{
    RS_SETTINGS->clear_geometry();
    QMessageBox::information(this, "info", "You must restart LibreCAD to see the changes.");
}

void QG_DlgOptionsGeneral::setVariableFile()
{
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty())
    {
        variablefile_field->setText(QDir::toNativeSeparators(path));
    }
}

/*!
 * \brief slot for the font folder selection icon
 * \author ravas
 * \date 2016-286
 */
void QG_DlgOptionsGeneral::setFontsFolder()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);

    if (dlg.exec())
    {
        auto dir = dlg.selectedFiles()[0];
        lePathFonts->setText(QDir::toNativeSeparators(dir));
    }
}
