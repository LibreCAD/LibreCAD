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


void QG_DlgInitial::init() {
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    for (RS_StringList::Iterator it = languageList.begin();
    	it!=languageList.end();
        it++) {
            
        QString l = RS_SYSTEM->symbolToLanguage(*it);
        cbLanguage->insertItem(l);
        cbLanguageCmd->insertItem(l);
    }
        
        
        // units:
        for (int i=RS2::None; i<RS2::LastUnit; i++) {
        cbUnit->insertItem(RS_Units::unitToString((RS2::Unit)i));
    }
        
        cbUnit->setCurrentText("Millimeter");
        cbLanguage->setCurrentText("English");
        cbLanguageCmd->setCurrentText("English");
}

void QG_DlgInitial::setText(const QString& t) {
    lWelcome->setText(t);
}

void QG_DlgInitial::setPixmap(const QPixmap& p) {
    lImage->setPixmap(p);
}

void QG_DlgInitial::ok() {
    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/Language", 
                            RS_SYSTEM->languageToSymbol(cbLanguage->currentText()));
    RS_SETTINGS->writeEntry("/LanguageCmd", 
                            RS_SYSTEM->languageToSymbol(cbLanguageCmd->currentText()));
    RS_SETTINGS->endGroup();
    
    RS_SETTINGS->beginGroup("/Defaults");
    RS_SETTINGS->writeEntry("/Unit", cbUnit->currentText());
    RS_SETTINGS->endGroup();
    accept();
}
