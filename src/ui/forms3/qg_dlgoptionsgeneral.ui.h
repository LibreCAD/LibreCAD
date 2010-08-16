/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <q3listbox.h>

void QG_DlgOptionsGeneral::init() {
#ifdef QC_PREDEFINED_LOCALE
    bgLanguage->hide();
    Widget9Layout->addMultiCellWidget( bgGraphicView, 0, 2, 0, 0 ); //use empty space as well
#endif
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    languageList.prepend("en");
    for (RS_StringList::Iterator it = languageList.begin();
            it!=languageList.end();
            it++) {

	RS_DEBUG->print("QG_DlgOptionsGeneral::init: adding %s to combobox",
			(*it).latin1());

        QString l = RS_SYSTEM->symbolToLanguage(*it);
	if (l.isEmpty()==false) {
                RS_DEBUG->print("QG_DlgOptionsGeneral::init: %s", l.latin1());
        	cbLanguage->insertItem(l);
        	cbLanguageCmd->insertItem(l);
	}
    }

    RS_SETTINGS->beginGroup("/Appearance");

    // set current language:
    QString def_lang = "en";
#ifdef QC_PREDEFINED_LOCALE
    def_lang = QC_PREDEFINED_LOCALE;
#endif
    QString lang = RS_SETTINGS->readEntry("/Language", def_lang);
    cbLanguage->setCurrentText(RS_SYSTEM->symbolToLanguage(lang));

    QString langCmd = RS_SETTINGS->readEntry("/LanguageCmd", def_lang);
    cbLanguageCmd->setCurrentText(RS_SYSTEM->symbolToLanguage(langCmd));

    // graphic view:
    // crosshairs:
    QString showCrosshairs = RS_SETTINGS->readEntry("/ShowCrosshairs", "1");
    cbShowCrosshairs->setChecked(showCrosshairs=="1");
    
    // scale grid:
    QString scaleGrid = RS_SETTINGS->readEntry("/ScaleGrid", "1");
    cbScaleGrid->setChecked(scaleGrid=="1");
    QString minGridSpacing = RS_SETTINGS->readEntry("/MinGridSpacing", "10");
    cbMinGridSpacing->setCurrentText(minGridSpacing);

    // preview:
    QString maxPreview = RS_SETTINGS->readEntry("/MaxPreview", "100");
    cbMaxPreview->setCurrentText(maxPreview);
    

    // colors:
    QString backgroundColor = RS_SETTINGS->readEntry("/BackgroundColor", "Black");
    cbBackgroundColor->setCurrentText(backgroundColor);
    QString gridColor = RS_SETTINGS->readEntry("/GridColor", "Gray");
    cbGridColor->setCurrentText(gridColor);
    QString metaGridColor = RS_SETTINGS->readEntry("/MetaGridColor", "#404040");
    cbMetaGridColor->setCurrentText(metaGridColor);
    QString selectedColor = RS_SETTINGS->readEntry("/SelectedColor", "#a54747");
    cbSelectedColor->setCurrentText(selectedColor);
    QString highlightedColor = RS_SETTINGS->readEntry("/HighlightedColor", "#739373");
    cbHighlightedColor->setCurrentText(highlightedColor);
    
    // font size:
    QString sizeStatus = RS_SETTINGS->readEntry("/StatusBarFontSize", "9");
    cbSizeStatus->setCurrentText(sizeStatus);

    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Paths");

    lePathTranslations->setText(RS_SETTINGS->readEntry("/Translations", ""));
    lePathHatch->setText(RS_SETTINGS->readEntry("/Patterns", ""));
    lePathFonts->setText(RS_SETTINGS->readEntry("/Fonts", ""));
    lePathScripts->setText(RS_SETTINGS->readEntry("/Scripts", ""));
    lePathLibrary->setText(RS_SETTINGS->readEntry("/Library", ""));

    RS_SETTINGS->endGroup();

    // units:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        if (i!=(int)RS2::None)
            cbUnit->insertItem(RS_Units::unitToString((RS2::Unit)i));
    }
    // RVT_PORT cbUnit->listBox()->sort();
    cbUnit->insertItem( RS_Units::unitToString(RS2::None), 0 );

    QString def_unit = "Millimeter";
#ifdef QC_PREDEFINED_UNIT
    def_unit = QC_PREDEFINED_UNIT;
#endif
    RS_SETTINGS->beginGroup("/Defaults");
    cbUnit->setCurrentText(QObject::tr( RS_SETTINGS->readEntry("/Unit", def_unit) ));
    RS_SETTINGS->endGroup();

    restartNeeded = false;
}

void QG_DlgOptionsGeneral::destroy() {
}

void QG_DlgOptionsGeneral::setRestartNeeded() {
    restartNeeded = true;
}

void QG_DlgOptionsGeneral::ok() {
    //RS_SYSTEM->loadTranslation(cbLanguage->currentText());
    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/Language",
                            RS_SYSTEM->languageToSymbol(cbLanguage->currentText()));
    RS_SETTINGS->writeEntry("/LanguageCmd",
                            RS_SYSTEM->languageToSymbol(cbLanguageCmd->currentText()));
    RS_SETTINGS->writeEntry("/ShowCrosshairs",
                            QString("%1").arg((int)cbShowCrosshairs->isChecked()));
    RS_SETTINGS->writeEntry("/ScaleGrid",
                            QString("%1").arg((int)cbScaleGrid->isChecked()));
    RS_SETTINGS->writeEntry("/MinGridSpacing",
                            cbMinGridSpacing->currentText());
    RS_SETTINGS->writeEntry("/MaxPreview",
                            cbMaxPreview->currentText());
    RS_SETTINGS->writeEntry("/BackgroundColor",
                            cbBackgroundColor->currentText());
    RS_SETTINGS->writeEntry("/GridColor",
                            cbGridColor->currentText());
    RS_SETTINGS->writeEntry("/MetaGridColor",
                            cbMetaGridColor->currentText());
    RS_SETTINGS->writeEntry("/SelectedColor",
                            cbSelectedColor->currentText());
    RS_SETTINGS->writeEntry("/HighlightedColor",
                            cbHighlightedColor->currentText());
    RS_SETTINGS->writeEntry("/StatusBarFontSize",
                            cbSizeStatus->currentText());
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Paths");
    RS_SETTINGS->writeEntry("/Translations", lePathTranslations->text());
    RS_SETTINGS->writeEntry("/Patterns", lePathHatch->text());
    RS_SETTINGS->writeEntry("/Fonts", lePathFonts->text());
    RS_SETTINGS->writeEntry("/Scripts", lePathScripts->text());
    RS_SETTINGS->writeEntry("/Library", lePathLibrary->text());
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Defaults");
    RS_SETTINGS->writeEntry("/Unit", 
        RS_Units::unitToString( RS_Units::stringToUnit( cbUnit->currentText() ), false/*untr.*/) );
    RS_SETTINGS->endGroup();

    if (restartNeeded==true) {
        QMessageBox::warning( this, tr("Preferences"),
                              tr("Please restart the application to apply all changes."),
                              QMessageBox::Ok,
                              QMessageBox::NoButton);
    }
    accept();
    //return true;
}
