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
#include "qg_dlginitial.h"

#include "rs_system.h"
#include "rs_settings.h"
#include "rs_units.h"

/*
 *  Constructs a QG_DlgInitial as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgInitial::QG_DlgInitial(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgInitial::~QG_DlgInitial()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgInitial::languageChange()
{
    retranslateUi(this);
}

void QG_DlgInitial::init() {
    // Fill combobox with languages:
    QStringList languageList = RS_SYSTEM->getLanguageList();
    for (QStringList::Iterator it = languageList.begin();
        it!=languageList.end();
        it++) {

        QString l = RS_SYSTEM->symbolToLanguage(*it);
        cbLanguage->addItem(l);
        cbLanguageCmd->addItem(l);
    }


        // units:
        for (int i=RS2::None; i<RS2::LastUnit; i++) {
        cbUnit->addItem(RS_Units::unitToString((RS2::Unit)i));
    }

        cbUnit->setCurrentIndex( cbUnit->findText("Millimeter") );
        cbLanguage->setCurrentIndex( cbLanguage->findText("English") );
        cbLanguageCmd->setCurrentIndex( cbLanguageCmd->findText("English") );
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
