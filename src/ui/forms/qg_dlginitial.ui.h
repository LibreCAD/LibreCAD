//Added by qt3to4:
#include <QPixmap>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


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
