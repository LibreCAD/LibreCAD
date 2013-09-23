/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

#include <qmessagebox.h>
#include "rs_system.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "qg_filedialog.h"

/*
 *  Constructs a QG_DlgOptionsGeneral as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgOptionsGeneral::QG_DlgOptionsGeneral(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
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

void QG_DlgOptionsGeneral::init() {
#ifdef QC_PREDEFINED_LOCALE
    bgLanguage->hide();
    Widget9Layout->addMultiCellWidget( bgGraphicView, 0, 2, 0, 0 ); //use empty space as well
#endif
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    languageList.sort();
    languageList.prepend("en");
    for (QStringList::Iterator it = languageList.begin();
         it!=languageList.end();
         it++) {

        RS_DEBUG->print("QG_DlgOptionsGeneral::init: adding %s to combobox",
                        (*it).toLatin1().data());

        QString l = RS_SYSTEM->symbolToLanguage(*it);
        if (l.isEmpty()==false && cbLanguage->findData(*it)==-1) {
            RS_DEBUG->print("QG_DlgOptionsGeneral::init: %s", l.toLatin1().data());
            cbLanguage->addItem(l,*it);
            cbLanguageCmd->addItem(l,*it);
        }
    }

    RS_SETTINGS->beginGroup("/Appearance");

    // set current language:
    QString def_lang = "en";
#ifdef QC_PREDEFINED_LOCALE
    def_lang = QC_PREDEFINED_LOCALE;
#endif
    QString lang = RS_SETTINGS->readEntry("/Language", def_lang);
    cbLanguage->setCurrentIndex( cbLanguage->findText(RS_SYSTEM->symbolToLanguage(lang)) );

    QString langCmd = RS_SETTINGS->readEntry("/LanguageCmd", def_lang);
    cbLanguageCmd->setCurrentIndex( cbLanguageCmd->findText(RS_SYSTEM->symbolToLanguage(langCmd)) );

    // graphic view:
    // crosshairs:
    QString showCrosshairs = RS_SETTINGS->readEntry("/ShowCrosshairs", "1");
    cbShowCrosshairs->setChecked(showCrosshairs=="1");
    
    // scale grid:
    QString scaleGrid = RS_SETTINGS->readEntry("/ScaleGrid", "1");
    cbScaleGrid->setChecked(scaleGrid=="1");
    QString minGridSpacing = RS_SETTINGS->readEntry("/MinGridSpacing", "10");
    cbMinGridSpacing->setCurrentIndex( cbMinGridSpacing->findText(minGridSpacing) );

    // preview:
	initComboBox(cbMaxPreview, RS_SETTINGS->readEntry("/MaxPreview", "100"));

    // colors:
	initComboBox(cbBackgroundColor, RS_SETTINGS->readEntry("/BackgroundColor", "Black"));
	initComboBox(cbGridColor, RS_SETTINGS->readEntry("/GridColor", "Gray"));
	initComboBox(cbMetaGridColor, RS_SETTINGS->readEntry("/MetaGridColor", "#404040"));
	initComboBox(cbSelectedColor, RS_SETTINGS->readEntry("/SelectedColor", "#a54747"));
	initComboBox(cbHighlightedColor, RS_SETTINGS->readEntry("/HighlightedColor", "#739373"));
	initComboBox(cbStartHandleColor, RS_SETTINGS->readEntry("/StartHandleColor", "#00FFFF"));
	initComboBox(cbHandleColor, RS_SETTINGS->readEntry("/HandleColor", "#0000FF"));
	initComboBox(cbEndHandleColor, RS_SETTINGS->readEntry("/EndHandleColor", "#0000FF"));

    // font size:
    QString sizeStatus = RS_SETTINGS->readEntry("/StatusBarFontSize", "9");
    cbSizeStatus->setCurrentIndex( cbSizeStatus->findText(sizeStatus) );

    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Paths");

    lePathTranslations->setText(RS_SETTINGS->readEntry("/Translations", ""));
    lePathHatch->setText(RS_SETTINGS->readEntry("/Patterns", ""));
    lePathFonts->setText(RS_SETTINGS->readEntry("/Fonts", ""));
    lePathScripts->setText(RS_SETTINGS->readEntry("/Scripts", ""));
    lePathLibrary->setText(RS_SETTINGS->readEntry("/Library", "").trimmed());
    leTemplate->setText(RS_SETTINGS->readEntry("/Template", "").trimmed());

    RS_SETTINGS->endGroup();

    // units:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        if (i!=(int)RS2::None)
            cbUnit->addItem(RS_Units::unitToString((RS2::Unit)i));
    }
    // RVT_PORT cbUnit->listBox()->sort();
    cbUnit->insertItem( 0, RS_Units::unitToString(RS2::None) );

    QString def_unit = "Millimeter";
#ifdef QC_PREDEFINED_UNIT
    def_unit = QC_PREDEFINED_UNIT;
#endif
    RS_SETTINGS->beginGroup("/Defaults");
//    cbUnit->setCurrentIndex( cbUnit->findText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit) )) );
    cbUnit->setCurrentIndex( cbUnit->findText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit).toUtf8().data() )) );
    // Auto save timer
    cbAutoSaveTime->setValue(RS_SETTINGS->readNumEntry("/AutoSaveTime", 5));
    cbAutoBackup->setChecked(RS_SETTINGS->readNumEntry("/AutoBackupDocument", 1)?true:false);
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

void QG_DlgOptionsGeneral::ok() {
    //RS_SYSTEM->loadTranslation(cbLanguage->currentText());
    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/Language",cbLanguage->itemData(cbLanguage->currentIndex()));
    RS_SETTINGS->writeEntry("/LanguageCmd",cbLanguageCmd->itemData(cbLanguageCmd->currentIndex()));
	RS_SETTINGS->writeEntry("/ShowCrosshairs", QString("%1").arg((int)cbShowCrosshairs->isChecked()));
	RS_SETTINGS->writeEntry("/ScaleGrid", QString("%1").arg((int)cbScaleGrid->isChecked()));
	RS_SETTINGS->writeEntry("/MinGridSpacing", cbMinGridSpacing->currentText());
	RS_SETTINGS->writeEntry("/MaxPreview", cbMaxPreview->currentText());
	RS_SETTINGS->writeEntry("/BackgroundColor", cbBackgroundColor->currentText());
	RS_SETTINGS->writeEntry("/GridColor", cbGridColor->currentText());
	RS_SETTINGS->writeEntry("/MetaGridColor", cbMetaGridColor->currentText());
	RS_SETTINGS->writeEntry("/SelectedColor", cbSelectedColor->currentText());
	RS_SETTINGS->writeEntry("/HighlightedColor", cbHighlightedColor->currentText());
	RS_SETTINGS->writeEntry("/StartHandleColor", cbStartHandleColor->currentText());
	RS_SETTINGS->writeEntry("/HandleColor", cbHandleColor->currentText());
	RS_SETTINGS->writeEntry("/EndHandleColor", cbEndHandleColor->currentText());
	RS_SETTINGS->writeEntry("/StatusBarFontSize", cbSizeStatus->currentText());
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Paths");
    RS_SETTINGS->writeEntry("/Translations", lePathTranslations->text());
    RS_SETTINGS->writeEntry("/Patterns", lePathHatch->text());
    RS_SETTINGS->writeEntry("/Fonts", lePathFonts->text());
    RS_SETTINGS->writeEntry("/Scripts", lePathScripts->text());
    RS_SETTINGS->writeEntry("/Library", lePathLibrary->text());
    RS_SETTINGS->writeEntry("/Template", leTemplate->text());
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Defaults");
    RS_SETTINGS->writeEntry("/Unit",
        RS_Units::unitToString( RS_Units::stringToUnit( cbUnit->currentText() ), false/*untr.*/) );
    RS_SETTINGS->writeEntry("/AutoSaveTime", cbAutoSaveTime->value() );
    RS_SETTINGS->writeEntry("/AutoBackupDocument", cbAutoBackup->isChecked()?1:0 );
    RS_SETTINGS->endGroup();

    if (restartNeeded==true) {
        QMessageBox::warning( this, tr("Preferences"),
                              tr("Please restart the application to apply all changes."),
                              QMessageBox::Ok,
                              Qt::NoButton);
    }
    accept();
}

