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
#include "qg_dlgmtext.h"

#include <QTextCodec>
#include <QTextStream>
#include <QFileDialog>
#include "rs_system.h"
#include "rs_settings.h"
#include "rs_font.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgMText as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMText::QG_DlgMText(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl), saveSettings(true)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgMText::~QG_DlgMText()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgMText::languageChange()
{
    retranslateUi(this);
}

void QG_DlgMText::init() {
    cbFont->init();
    font=NULL;
    text = NULL;
    isNew = false;
    updateUniCharComboBox(0);
    updateUniCharButton(0);
}


void QG_DlgMText::updateUniCharComboBox(int) {
    QString t = cbUniPage->currentText();
    int i1 = t.indexOf('-');
    int i2 = t.indexOf(']');
    int min = t.mid(1, i1-1).toInt(NULL, 16);
    int max = t.mid(i1+1, i2-i1-1).toInt(NULL, 16);

    cbUniChar->clear();
    for (int c=min; c<=max; c++) {
        char buf[5];
        snprintf(buf,5, "%04X", c);
        cbUniChar->addItem(QString("[%1] %2").arg(buf).arg(QChar(c)));
    }
}

//set saveText to false, so, settings won't be saved during destroy, feature request#3445306
void QG_DlgMText::reject() {
    saveSettings=false;
    QDialog::reject();
}

void QG_DlgMText::destroy() {
    if (isNew&&saveSettings) {
        RS_SETTINGS->beginGroup("/Draw");
        RS_SETTINGS->writeEntry("/TextHeight", leHeight->text());
        RS_SETTINGS->writeEntry("/TextFont", cbFont->currentText());
        RS_SETTINGS->writeEntry("/TextDefault", (int)cbDefault->isChecked());
        RS_SETTINGS->writeEntry("/TextAlignment", getAlignment());
        //RS_SETTINGS->writeEntry("/TextLetterSpacing", leLetterSpacing->text());
        //RS_SETTINGS->writeEntry("/TextWordSpacing", leWordSpacing->text());
        RS_SETTINGS->writeEntry("/TextLineSpacingFactor",
                                leLineSpacingFactor->text());
        RS_SETTINGS->writeEntry("/TextString", teText->toPlainText());
        //RS_SETTINGS->writeEntry("/TextShape", getShape());
        RS_SETTINGS->writeEntry("/TextAngle", leAngle->text());
        //RS_SETTINGS->writeEntry("/TextRadius", leRadius->text());
        RS_SETTINGS->endGroup();
    }
}


/**
 * Sets the text entity represented by this dialog.
 */
void QG_DlgMText::setText(RS_MText& t, bool isNew) {
    text = &t;
    this->isNew = isNew;

    QString fon;
    QString height;
    QString def;
    QString alignment;
    //QString letterSpacing;
    //QString wordSpacing;
    QString lineSpacingFactor;
    QString str;
    //QString shape;
    QString angle;

    if (isNew) {
        wPen->hide();
        lLayer->hide();
        cbLayer->hide();
        RS_SETTINGS->beginGroup("/Draw");
        //default font depending on locale
        //default font depending on locale (RLZ-> check this: QLocale::system().name() returns "fr_FR")
        QByteArray iso = RS_System::localeToISO( QLocale::system().name().toLocal8Bit() );
//        QByteArray iso = RS_System::localeToISO( QTextCodec::locale() );
        if (iso=="ISO8859-1") {
             fon = RS_SETTINGS->readEntry("/TextFont", "normallatin1");
        } else if (iso=="ISO8859-2") {
             fon = RS_SETTINGS->readEntry("/TextFont", "normallatin2");
        } else if (iso=="ISO8859-7") {
             fon = RS_SETTINGS->readEntry("/TextFont", "greekc");
        } else if (iso=="KOI8-U" || iso=="KOI8-R") {
             fon = RS_SETTINGS->readEntry("/TextFont", "cyrillic_ii");
        } else {
             fon = RS_SETTINGS->readEntry("/TextFont", "standard");
                }
        height = RS_SETTINGS->readEntry("/TextHeight", "1.0");
        def = RS_SETTINGS->readEntry("/TextDefault", "1");
        alignment = RS_SETTINGS->readEntry("/TextAlignment", "1");
        //letterSpacing = RS_SETTINGS->readEntry("/TextLetterSpacing", "0");
        //wordSpacing = RS_SETTINGS->readEntry("/TextWordSpacing", "0");
        lineSpacingFactor = RS_SETTINGS->readEntry("/TextLineSpacingFactor", "1");
        str = RS_SETTINGS->readEntry("/TextString", "");
        //shape = RS_SETTINGS->readEntry("/TextShape", "0");
        angle = RS_SETTINGS->readEntry("/TextAngle", "0");
        //radius = RS_SETTINGS->readEntry("/TextRadius", "10");
        RS_SETTINGS->endGroup();
    } else {
        fon = text->getStyle();
        setFont(fon);
        height = QString("%1").arg(text->getHeight());
        if (font) {
            if (font->getLineSpacingFactor()==text->getLineSpacingFactor()) {
                def = "1";
            } else {
                def = "0";
            }
        }
        alignment = QString("%1").arg(text->getAlignment());
        //QString letterSpacing = RS_SETTINGS->readEntry("/TextLetterSpacing", "0");
        //QString wordSpacing = RS_SETTINGS->readEntry("/TextWordSpacing", "0");
        lineSpacingFactor = QString("%1").arg(text->getLineSpacingFactor());

/* // Doesn't make sense. We don't want to show native DXF strings in the Dialog.
#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        QTextCodec *codec = QTextCodec::codecForName(iso);
        if (codec) {
            str = codec->toUnicode(RS_FilterDXF::toNativeString(text->getText().local8Bit()));
        } else {
            str = RS_FilterDXF::toNativeString(text->getText().local8Bit());
        }
#else*/
       str = text->getText();
//#endif
        //QString shape = RS_SETTINGS->readEntry("/TextShape", "0");
        angle = QString("%1").arg(RS_Math::rad2deg(text->getAngle()));
        wPen->setPen(text->getPen(false), true, false, "Pen");
        RS_Graphic* graphic = text->getGraphic();
        if (graphic) {
            cbLayer->init(*(graphic->getLayerList()), false, false);
        }
        RS_Layer* lay = text->getLayer(false);
        if (lay) {
            cbLayer->setLayer(*lay);
        }
    }

    cbDefault->setChecked(def=="1");
    setFont(fon);
    leHeight->setText(height);
    setAlignment(alignment.toInt());
    if (def!="1" || font==NULL) {
        //leLetterSpacing->setText(letterSpacing);
        //leWordSpacing->setText(wordSpacing);
        leLineSpacingFactor->setText(lineSpacingFactor);
    } else {
        //leLetterSpacing->setText(font->getLetterSpacing());
        //leWordSpacing->setText(font->getWordSpacing());
        leLineSpacingFactor->setText(
            QString("%1").arg(font->getLineSpacingFactor()));
    }
    teText->setText(str);
    //setShape(shape.toInt());
    leAngle->setText(angle);
    //leRadius->setText(radius);
    teText->setFocus();
    teText->selectAll();
}


/**
 * Updates the text entity represented by the dialog to fit the choices of the user.
 */
void QG_DlgMText::updateText() {
    if (text) {
        text->setStyle(cbFont->currentText());
        text->setHeight(leHeight->text().toDouble());

//fix for windows (causes troubles if locale returns en_us):
/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        text->setText(
            RS_FilterDXF::toNativeString(
             QString::fromLocal8Bit( QTextCodec::codecForName( iso )->fromUnicode( teText->text() ) )
            )
        );
#else*/
        text->setText(teText->toPlainText());
//#endif
        //text->setLetterSpacing(leLetterSpacing.toDouble());
        text->setLineSpacingFactor(leLineSpacingFactor->text().toDouble());
        text->setAlignment(getAlignment());
        text->setAngle(RS_Math::deg2rad(leAngle->text().toDouble()));
    }
    if (text && !isNew) {
        text->setPen(wPen->getPen());
        text->setLayer(cbLayer->currentText());
        text->update();
    }
}


void QG_DlgMText::setAlignmentTL() {
    setAlignment(1);
}

void QG_DlgMText::setAlignmentTC() {
    setAlignment(2);
}

void QG_DlgMText::setAlignmentTR() {
    setAlignment(3);
}

void QG_DlgMText::setAlignmentML() {
    setAlignment(4);
}

void QG_DlgMText::setAlignmentMC() {
    setAlignment(5);
}

void QG_DlgMText::setAlignmentMR() {
    setAlignment(6);
}

void QG_DlgMText::setAlignmentBL() {
    setAlignment(7);
}

void QG_DlgMText::setAlignmentBC() {
    setAlignment(8);
}

void QG_DlgMText::setAlignmentBR() {
    setAlignment(9);
}

void QG_DlgMText::setAlignment(int a) {
    bTL->setChecked(false);
    bTC->setChecked(false);
    bTR->setChecked(false);
    bML->setChecked(false);
    bMC->setChecked(false);
    bMR->setChecked(false);
    bBL->setChecked(false);
    bBC->setChecked(false);
    bBR->setChecked(false);

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
        bBL->setChecked(true);
        break;
    case 8:
        bBC->setChecked(true);
        break;
    case 9:
        bBR->setChecked(true);
        break;
    default:
        break;
    }
}

int QG_DlgMText::getAlignment() {
    if (bTL->isChecked()) {
        return 1;
    } else if (bTC->isChecked()) {
        return 2;
    } else if (bTR->isChecked()) {
        return 3;
    } else if (bML->isChecked()) {
        return 4;
    } else if (bMC->isChecked()) {
        return 5;
    } else if (bMR->isChecked()) {
        return 6;
    } else if (bBL->isChecked()) {
        return 7;
    } else if (bBC->isChecked()) {
        return 8;
    } else if (bBR->isChecked()) {
        return 9;
    }

    return 1;
}

void QG_DlgMText::setFont(const QString& f) {
    cbFont->setCurrentIndex( cbFont->findText(f) );
    font = cbFont->getFont();
    defaultChanged(false);
}

/*
void QG_DlgText::setShape(int s) {
    switch (s) {
    case 0:
        rbStraight->setChecked(true);
        break;
    case 1:
        rbRound1->setChecked(true);
        break;
    case 2:
        rbRound2->setChecked(true);
        break;
    default:
        break;
    }
}

int QG_DlgText::getShape() {
    if (rbStraight->isOn()) {
        return 0;
    } else if (rbRound1->isOn()) {
        return 1;
    } else if (rbRound2->isOn()) {
        return 2;
    }
    return 1;
}
*/

void QG_DlgMText::defaultChanged(bool) {
    if (cbDefault->isChecked() && font) {
        leLineSpacingFactor->setText(
                        QString("%1").arg(font->getLineSpacingFactor()));
    }
}

void QG_DlgMText::loadText() {
    QString fn = QFileDialog::getOpenFileName( this, QString::null, QString::null);
    if (!fn.isEmpty()) {
        load(fn);
    }
}

void QG_DlgMText::load(const QString& fn) {
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream ts(&f);
    teText->setText(ts.readAll());
}

void QG_DlgMText::saveText() {
    QString fn = QFileDialog::getSaveFileName(this, QString::null, QString::null);
    if (!fn.isEmpty()) {
        save(fn);
    }
}

void QG_DlgMText::save(const QString& fn) {
    QString text = teText->toPlainText();
    QFile f(fn);
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream t(&f);
        t << text;
        f.close();
    }
}

void QG_DlgMText::insertSymbol(int) {
    QString str = cbSymbol->currentText();
    int i=str.indexOf('(');
    if (i!=-1) {
        teText->textCursor().insertText(QString("%1").arg(str.at(i+1)));
    }
}

void QG_DlgMText::updateUniCharButton(int) {
    QString t = cbUniChar->currentText();
    int i1 = t.indexOf(']');
    int c = t.mid(1, i1-1).toInt(NULL, 16);
    bUnicode->setText(QString("%1").arg(QChar(c)));
}

void QG_DlgMText::insertChar() {
    QString t = cbUniChar->currentText();
    int i1 = t.indexOf(']');
    int c = t.mid(1, i1-1).toInt(NULL, 16);
    teText->textCursor().insertText( QString("%1").arg(QChar(c)) );
}
