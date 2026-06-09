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

#include "qg_dlg_text.h"

#include <QFileDialog>
#include <QTextStream>

#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_text.h"

/*
 *  Constructs a QG_DlgText as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgText::QG_DlgText(QWidget *parent, LC_GraphicViewport *pViewport, RS_Text* text, bool forNew)
    :LC_EntityPropertiesDlg(parent, "TextProperties", pViewport), m_saveSettings(true){
    setupUi(this);
    init();
    setEntity(text, forNew);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgText::~QG_DlgText(){
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgText::languageChange(){
    retranslateUi(this);
}

void QG_DlgText::init() {
    cbFont->init();
    m_font=nullptr;
    m_entity = nullptr;
    m_isNew = false;
    // leOblique->setDisabled(true);
    updateUniCharComboBox(0);
    updateUniCharButton(0);

    /*
     * Ensure tabbing order of the widgets is as we would like. Using the Edit Tab
     * Order tool in Qt Designer omits the "pen" compound widget from the tabbing
     * list (as the tool is not aware that this user-written widget is a tabbable
     * thing). The .ui file can be manually edited, but then if Qt Designer is used
     * again to alter the layout, the pen widget gets dropped out of the order once
     * more. Seems that the only reliable way of ensuring the order is correct is
     * to set it programmatically.
     */
    setTabOrder(cbLayer, wPen); // Layer -> Pen compound widget
    setTabOrder(wPen, cbFont); // Pen compound widget -> Font widget
    setTabOrder(cbFont, leHeight); // etc
    setTabOrder(leHeight, leAngle);
    // setTabOrder(leAngle, leOblique);
    // setTabOrder(leOblique, leWidthRel);
    setTabOrder(leAngle, leWidthRel);
    setTabOrder(leWidthRel, bTL);
    setTabOrder(bTL, bTC);
    setTabOrder(bTC, bTR);
    setTabOrder(bTR, bML);
    setTabOrder(bML, bMC);
    setTabOrder(bMC, bMR);
    setTabOrder(bMR, bLL);
    setTabOrder(bLL, bLC);
    setTabOrder(bLC, bLR);
    setTabOrder(bLR, bBL);
    setTabOrder(bBL, bBC);
    setTabOrder(bBC, bBR);
    setTabOrder(bBR, rbFit);
    setTabOrder(rbFit, rbAligned);
    setTabOrder(rbAligned, rbMiddle);
    setTabOrder(rbMiddle, cbSymbol);
    setTabOrder(cbSymbol, cbUniPage);
    setTabOrder(cbUniPage, cbUniChar);
    setTabOrder(cbUniChar, bUnicode);
    setTabOrder(bUnicode, buttonBox);
    setTabOrder(buttonBox, bClear);
    setTabOrder(bClear, bLoad);
    setTabOrder(bLoad, bSave);
    setTabOrder(bSave, bCut);
    setTabOrder(bCut, bCopy);
    setTabOrder(bCopy, bPaste);
    setTabOrder(bPaste, teText); // Paste loops back to Text
    setTabOrder(teText, cbLayer); // Text widget -> Layer widget
}


void QG_DlgText::updateUniCharComboBox(int) const {
    const QString t = cbUniPage->currentText();
    const int i1 = t.indexOf('-');
    const int i2 = t.indexOf(']');
    const int min = t.mid(1, i1-1).toInt(nullptr, 16);
    const int max = t.mid(i1+1, i2-i1-1).toInt(nullptr, 16);

    cbUniChar->clear();
    for (int c=min; c<=max; c++) {
        char buf[5];
        snprintf(buf, 5, "%04X", c);
        cbUniChar->addItem(QString("[%1] %2").arg(buf).arg(QChar(c)));
    }
}

//set saveText to false, so, settings won't be saved during destroy, feature request#3445306
void QG_DlgText::reject() {
    m_saveSettings=false;
    QDialog::reject();
}

void QG_DlgText::destroy() const {
    if (m_isNew&&m_saveSettings) {
        LC_GROUP_GUARD("Draw");{
            LC_SET("TextHeight", leHeight->text());
            LC_SET("TextFont", cbFont->currentText());
            LC_SET("TextAlignmentT", getAlignment());
            LC_SET("TextWidthRelation", leWidthRel->text());
            LC_SET("TextStringT", teText->text());
            LC_SET("TextAngle", leAngle->text());
        }
    }
}

/**
 * Sets the text entity represented by this dialog.
 */
void QG_DlgText::setEntity(RS_Text* t, const bool isNew) {
    m_entity = t;
    this->m_isNew = isNew;

    QString font;
    QString height;
    QString def; // fixme - sand - not used
    QString alignment;
    //QString letterSpacing;
    //QString wordSpacing;
    QString widthRelation;
    QString str;
    //QString shape;
    QString angle;

    if (isNew) {
        wPen->hide();
        lLayer->hide();
        cbLayer->hide();
        LC_GROUP_GUARD("Draw");{
            //default font depending on locale
            //default font depending on locale (RLZ-> check this: QLocale::system().name() returns "fr_FR")
            QByteArray iso = RS_System::localeToISO(QLocale::system().name().toLocal8Bit());
            font = LC_GET_STR("TextFont", RS_FontList::getDefaultFont());
            height = LC_GET_STR("TextHeight", "1.0");
            def = LC_GET_STR("TextDefault", "1");
            alignment = LC_GET_STR("TextAlignmentT", "7");
            widthRelation = LC_GET_STR("TextWidthRelation", "1");
            str = LC_GET_STR("TextStringT", "");
            angle = LC_GET_STR("TextAngle", "0");
        }
    } else {
        font = m_entity->getStyle();
        setFont(font);
        height = QString("%1").arg(m_entity->getHeight());
        widthRelation = QString("%1").arg(m_entity->getWidthRel());
        alignment = QString("%1").arg(m_entity->getAlignment());
        str = m_entity->getText();


        const double wcsAngle = m_entity->getAngle();
        angle = toUIAngleDeg(wcsAngle);

        RS_Graphic* graphic = m_entity->getGraphic();
        if (graphic != nullptr) {
            cbLayer->init(*(graphic->getLayerList()), false, false);
        }
        RS_Layer* lay = m_entity->getLayer(false);
        if (lay != nullptr) {
            cbLayer->setLayer(*lay);
        }

        wPen->setPen(m_entity, lay, tr("Pen"));
    }

    setFont(font);
    leHeight->setText(height);
    setAlignment(alignment.toInt());
//    setwidthRel(widthRelation.toDouble());
    leWidthRel->setText(widthRelation);
    teText->setText(str);
    leAngle->setText(angle);
    teText->setFocus();
    teText->selectAll();
}


/**
 * Updates the text entity represented by the dialog to fit the choices of the user.
 */
void QG_DlgText::updateEntity() {
    if (m_entity == nullptr) {
        return;
    }
        m_entity->setStyle(cbFont->currentText());
        m_entity->setHeight(leHeight->text().toDouble());
        m_entity->setWidthRel(leWidthRel->text().toDouble());

        m_entity->setText(teText->text());
        m_entity->setAlignment(getAlignment());
        const double wcsAngle = toWCSAngle(leAngle, m_entity->getAngle());
        m_entity->setAngle(wcsAngle);
        if (!m_isNew) {
            m_entity->setPen(wPen->getPen());
            m_entity->setLayer(cbLayer->getLayer());
        }
        m_entity->update();

}


void QG_DlgText::setAlignmentTL() const {
    setAlignment(1);
}

void QG_DlgText::setAlignmentTC() const {
    setAlignment(2);
}

void QG_DlgText::setAlignmentTR() const {
    setAlignment(3);
}

void QG_DlgText::setAlignmentML() const {
    setAlignment(4);
}

void QG_DlgText::setAlignmentMC() const {
    setAlignment(5);
}

void QG_DlgText::setAlignmentMR() const {
    setAlignment(6);
}

void QG_DlgText::setAlignmentLL() const {
    setAlignment(7);
}

void QG_DlgText::setAlignmentLC() const {
    setAlignment(8);
}

void QG_DlgText::setAlignmentLR() const {
    setAlignment(9);
}

void QG_DlgText::setAlignmentBL() const {
    setAlignment(10);
}

void QG_DlgText::setAlignmentBC() const {
    setAlignment(11);
}

void QG_DlgText::setAlignmentBR() const {
    setAlignment(12);
}

void QG_DlgText::setAlignmentFit() const {
    setAlignment(13);
}

void QG_DlgText::setAlignmentAlign() const {
    setAlignment(14);
}

void QG_DlgText::setAlignmentMiddle() const {
    setAlignment(15);
}

void QG_DlgText::setAlignment(const int a) const {
    bTL->setChecked(false);
    bTC->setChecked(false);
    bTR->setChecked(false);
    bML->setChecked(false);
    bMC->setChecked(false);
    bMR->setChecked(false);
    bLL->setChecked(false);
    bLC->setChecked(false);
    bLR->setChecked(false);
    bBL->setChecked(false);
    bBC->setChecked(false);
    bBR->setChecked(false);
    rbFit->setChecked(false);
    rbAligned->setChecked(false);
    rbMiddle->setChecked(false);

    switch (a) {
    case 1:
        bTL->setChecked(true);
        break;
    case 2:
        bTC->setChecked(true);
        break;
    case 3:
        bTR->setChecked(true);
        break;
    case 4:
        bML->setChecked(true);
        break;
    case 5:
        bMC->setChecked(true);
        break;
    case 6:
        bMR->setChecked(true);
        break;
    case 7:
        bLL->setChecked(true);
        break;
    case 8:
        bLC->setChecked(true);
        break;
    case 9:
        bLR->setChecked(true);
        break;
    case 10:
        bBL->setChecked(true);
        break;
    case 11:
        bBC->setChecked(true);
        break;
    case 12:
        bBR->setChecked(true);
        break;
    case 13:
        rbFit->setChecked(true);
        break;
    case 14:
        rbAligned->setChecked(true);
        break;
    case 15:
        rbMiddle->setChecked(true);
        break;
    default:
        break;
    }
}

int QG_DlgText::getAlignment() const {
    if (bTL->isChecked()) {
        return 1;
    }
    if (bTC->isChecked()) {
        return 2;
    }
    if (bTR->isChecked()) {
        return 3;
    }
    if (bML->isChecked()) {
        return 4;
    }
    if (bMC->isChecked()) {
        return 5;
    }
    if (bMR->isChecked()) {
        return 6;
    }
    if (bLL->isChecked()) {
        return 7;
    }
    if (bLC->isChecked()) {
        return 8;
    }
    if (bLR->isChecked()) {
        return 9;
    }
    if (bBL->isChecked()) {
        return 10;
    }
    if (bBC->isChecked()) {
        return 11;
    }
    if (bBR->isChecked()) {
        return 12;
    }
    if (rbFit->isChecked()) {
        return 13;
    }
    if (rbAligned->isChecked()) {
        return 14;
    }
    if (rbMiddle->isChecked()) {
        return 15;
    }

    return 7;
}

void QG_DlgText::setFont(const QString& f) {
    int index = cbFont->findText(f);

    // Issue #2069: default to unicode fonts
    if (index == -1) {
        index = cbFont->findText("unicode");
    }
    if (index >= 0) {
        cbFont->setCurrentIndex(index);
        m_font = cbFont->getFont();
    }
}

void QG_DlgText::loadText() {
    const QString fn = QFileDialog::getOpenFileName( this, QString(), QString());
    if (!fn.isEmpty()) {
        load(fn);
    }
}

void QG_DlgText::load(const QString& fn) const {
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream ts(&f);
    teText->setText(ts.readAll());
}

void QG_DlgText::saveText() {
    const QString fn = QFileDialog::getSaveFileName(this, QString(), QString());
    if (!fn.isEmpty()) {
        save(fn);
    }
}

void QG_DlgText::save(const QString& fn) const {
    QFile f(fn);
    if (f.open(QIODevice::WriteOnly)) {
        const QString text = teText->text();
        QTextStream t(&f);
        t << text;
        f.close();
    }
}

void QG_DlgText::insertSymbol(int) const {
    const QString str = cbSymbol->currentText();
    const int i=str.indexOf('(');
    if (i!=-1) {
//        teText->textCursor().insertText(QString("%1").arg(str.at(i+1)));
        teText->insert(QString("%1").arg(str.at(i+1)));
    }
}

void QG_DlgText::updateUniCharButton(int) const {
    const QString t = cbUniChar->currentText();
    const int i1 = t.indexOf(']');
    const int c = t.mid(1, i1-1).toInt(nullptr, 16);
    bUnicode->setText(QString("%1").arg(QChar(c)));
}

void QG_DlgText::insertChar() const {
    const QString t = cbUniChar->currentText();
    const int i1 = t.indexOf(']');
    const int c = t.mid(1, i1-1).toInt(nullptr, 16);
//    teText->textCursor().insertText( QString("%1").arg(QChar(c)) );
    teText->insert( QString("%1").arg(QChar(c)) );
}
