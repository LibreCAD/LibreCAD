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


#include <QFileDialog>

#include "qg_dlgmtext.h"
#include "rs_dialogfactory.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_system.h"

/*
 *  Constructs a QG_DlgMText as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMText::QG_DlgMText(QWidget *parent, LC_GraphicViewport *pViewport, RS_MText* text, bool forNew)
    :LC_EntityPropertiesDlg(parent, "MTextProperties", pViewport){
    setupUi(this);
    m_alignmentButtons = {{bTL, bTC, bTR, bML, bMC, bMR, bBL, bBC, bBR}};
    init();
    setEntity(text, forNew);
}

QG_DlgMText::~QG_DlgMText()
{
    try {
        destroy();
    } catch(...)
    {}
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgMText::languageChange(){
    retranslateUi(this);
}

void QG_DlgMText::init() {
    cbFont->init();
    m_font = nullptr;
    m_entity = nullptr;
    m_isNew = false;
    updateUniCharComboBox(0);
    updateUniCharButton(0);

    /*
     * Ensure tabbing order of the widgets is as we would like. Using the Edit
     * Tab Order tool in Qt Designer omits the "pen" compound widget from the
     * tabbing list (as the tool is not aware that this user-written widget is a
     * tabbable thing). The .ui file can be manually edited, but then if Qt
     * Designer is used again to alter the layout, the pen widget gets dropped
     * out of the order once more. Seems that the only reliable way of ensuring
     * the order is correct is to set it programmatically.
     */
    std::vector<QWidget*> buttons{{cbLayer,       wPen,      cbFont,
                          leHeight,      cbDefault, leLineSpacingFactor,
                          teText,        bTL,       bTC,
                          bTR,           bML,       bMC,
                          bMR,           bBL,       bBC,
                          bBR,           leAngle,   rbLeftToRight,
                          rbRightToLeft, cbSymbol,  cbUniPage,
                          cbUniChar,     bUnicode,  buttonBox,
                          bClear,        bLoad,     bSave,
                          bCut,          bCopy,     bPaste}};
    // the order is cyclic
    buttons.push_back(buttons.front());
    for (auto it = std::next(buttons.cbegin()); it != buttons.cend(); ++it)
        QWidget::setTabOrder(*std::prev(it), *it);

    /*
     * We are using the frame colour of the teText QTextEdit widget to indicate when
     * this is the selected widget - there is no automatic highlight of this widget
     * class on selection. Ensure that known state of frame on start, and install Qt
     * Event filter in order to get callback on focus in/out of the widget.
     */
    teText->QFrame::setLineWidth(2);
    teText->QFrame::setMidLineWidth(0);
    teText->QFrame::setFrameStyle(QFrame::Box|QFrame::Plain);
    teText->installEventFilter(this);

    // events
    connect(rbLeftToRight, &QRadioButton::toggled, this, &QG_DlgMText::layoutDirectionChanged);
}


void QG_DlgMText::updateUniCharComboBox(int) {
    QString t = cbUniPage->currentText();
    int i1 = t.indexOf('-');
    int i2 = t.indexOf(']');
    int min = t.mid(1, i1-1).toInt(nullptr, 16);
    int max = t.mid(i1+1, i2-i1-1).toInt(nullptr, 16);

    cbUniChar->clear();
    for (int c=min; c<=max; c++) {
        cbUniChar->addItem(QString{"[%1] %2"}.arg(c, 4, 16, QChar{'0'}).arg(QChar{c}));
    }
}

//set saveText to false, so, settings won't be saved during destroy, feature request#3445306
void QG_DlgMText::reject() {
    m_saveSettings=false;
    QDialog::reject();
}

#include "rs_debug.h"
void QG_DlgMText::destroy() {
    if (m_saveSettings) {
        LC_GROUP_GUARD("Draw");
        {
            LC_SET("TextHeight", leHeight->text());
            LC_SET("TextFont", cbFont->currentText());
            LC_SET("TextDefault", (int) cbDefault->isChecked());
            LC_SET("TextAlignment", getAlignment());
            //RS_SETTINGS->writeEntry("/TextLetterSpacing", leLetterSpacing->text());
            //RS_SETTINGS->writeEntry("/TextWordSpacing", leWordSpacing->text());
            LC_SET("TextLineSpacingFactor", leLineSpacingFactor->text());
            LC_SET("TextString", teText->toPlainText());
            //RS_SETTINGS->writeEntry("/TextShape", getShape());
            LC_SET("TextAngle", leAngle->text());
            //RS_SETTINGS->writeEntry("/TextRadius", leRadius->text());
            LC_SET("TextLeftToRight", rbLeftToRight->isChecked());
        }
    }
}

/**
 * Sets the text m_entity represented by this dialog.
 */
void QG_DlgMText::setEntity(RS_MText* t, bool isNew) {
    m_entity = t;
    m_isNew = isNew;

    QString font;
    QString height;
    QString def;
    QString alignment;
    //QString letterSpacing;
    //QString wordSpacing;
    QString lineSpacingFactor;
    QString str;
    //QString shape;
    QString angle;
    bool leftToRight = locale().textDirection() == Qt::LeftToRight;

    if (isNew) {
        wPen->hide();
        lLayer->hide();
        cbLayer->hide();
        LC_GROUP("Draw");
        {
            //default font depending on locale
            //default font depending on locale (RLZ-> check this: QLocale::system().name() returns "fr_FR")
            QByteArray iso = RS_System::localeToISO(QLocale::system().name().toLocal8Bit());
            font = LC_GET_STR("TextFont", RS_FontList::getDefaultFont());
            height = LC_GET_STR("TextHeight", "1.0");
            def = LC_GET_STR("TextDefault", "1");
            alignment = LC_GET_STR("TextAlignment", "1");
            //letterSpacing = RS_SETTINGS->readEntry("/TextLetterSpacing", "0");
            //wordSpacing = RS_SETTINGS->readEntry("/TextWordSpacing", "0");
            lineSpacingFactor = LC_GET_STR("TextLineSpacingFactor", "1");
            str = LC_GET_STR("TextString", "");
            //shape = RS_SETTINGS->readEntry("/TextShape", "0");
            angle = LC_GET_STR("TextAngle", "0");
            //radius = RS_SETTINGS->readEntry("/TextRadius", "10");
            // leftToRight = RS_SETTINGS->readNumEntry("/TextLeftToRight", 1);
        }
    } else {
        font = m_entity->getStyle();
        setFont(font);
        height = QString::number(m_entity->getHeight());
        if (m_font) {
            if (m_font->getLineSpacingFactor() == m_entity->getLineSpacingFactor()) {
                def = "1";
            } else {
                def = "0";
            }
        }
        alignment = QString::number(m_entity->getAlignment());
        //QString letterSpacing = RS_SETTINGS->readEntry("/TextLetterSpacing", "0");
        //QString wordSpacing = RS_SETTINGS->readEntry("/TextWordSpacing", "0");
        lineSpacingFactor = QString::number(m_entity->getLineSpacingFactor());

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
       str = m_entity->getText();
//#endif
        //QString shape = RS_SETTINGS->readEntry("/TextShape", "0");

        double wcsAngle = m_entity->getAngle();
        angle = toUIAngleDeg(wcsAngle);

        RS_Graphic* graphic = m_entity->getGraphic();
        if (graphic) {
            cbLayer->init(*(graphic->getLayerList()), false, false);
        }

        RS_Layer* layer = m_entity->getLayer(false);
        if (layer) {
            cbLayer->setLayer(*layer);
        }

        wPen->setPen(m_entity, layer, tr("Pen"));
        leftToRight = m_entity->getDrawingDirection() == RS_MTextData::LeftToRight;
    }

    cbDefault->setChecked(def=="1");
    setFont(font);
    leHeight->setText(height);
    unsigned index = alignment.toUInt() - 1;
    setAlignment(*m_alignmentButtons[index%m_alignmentButtons.size()]);
    if (def!="1" || m_font==nullptr) {
        //leLetterSpacing->setText(letterSpacing);
        //leWordSpacing->setText(wordSpacing);
        leLineSpacingFactor->setText(lineSpacingFactor);
    } else {
        //leLetterSpacing->setText(font->getLetterSpacing());
        //leWordSpacing->setText(font->getWordSpacing());
        leLineSpacingFactor->setText(
            QString::number(m_font->getLineSpacingFactor()));
    }
    teText->setText(str);
    //setShape(shape.toInt());
    leAngle->setText(angle);
    //leRadius->setText(radius);
    teText->setFocus();
    teText->selectAll();
    rbLeftToRight->setChecked(leftToRight);
    layoutDirectionChanged();
}

void QG_DlgMText::layoutDirectionChanged()
{
    bool leftToRight = rbLeftToRight->isChecked();
    rbRightToLeft->setChecked(!leftToRight);
    Qt::LayoutDirection direction =  leftToRight ? Qt::LeftToRight : Qt::RightToLeft;
    teText->setLayoutDirection(direction);
    m_entity->setDrawingDirection(leftToRight ? RS_MTextData::LeftToRight : RS_MTextData::RightToLeft);
    QTextDocument* doc = teText->document();
    if (doc) {
        QTextOption option = doc->defaultTextOption();
        option.setTextDirection(direction);
        doc->setDefaultTextOption(option);
    }
    teText->update();
}

/**
 * Updates the text m_entity represented by the dialog to fit the choices of the user.
 */
void QG_DlgMText::updateEntity() {
    if (m_entity == nullptr)
        return;

    m_entity->setStyle(cbFont->currentText());
    m_entity->setHeight(leHeight->text().toDouble());

    //fix for windows (causes troubles if locale returns en_us):
    /*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        text->setText(
            RS_FilterDXF::toNativeString(
             QString::fromLocal8Bit( QTextCodec::codecForName( iso )->fromUnicode( teText->text() ) )
            )
        );
#else*/
    m_entity->setText(teText->toPlainText());
    //#endif
    //text->setLetterSpacing(leLetterSpacing.toDouble());
    m_entity->setLineSpacingFactor(leLineSpacingFactor->text().toDouble());
    m_entity->setAlignment(getAlignment());

    double wcsAngle = toWCSAngle(leAngle, m_entity->getAngle());
    m_entity->setAngle(wcsAngle);

    m_entity->setDrawingDirection(rbLeftToRight->isChecked() ? RS_MTextData::LeftToRight : RS_MTextData::RightToLeft);
    if (!m_isNew) {
        m_entity->setPen(wPen->getPen());
        m_entity->setLayer(cbLayer->getLayer());
    }
    m_entity->update();
}

size_t QG_DlgMText::alignmentButtonIdex(QToolButton* button) const
{
    auto it = std::find(m_alignmentButtons.cbegin(), m_alignmentButtons.cend(), button);
    return it == m_alignmentButtons.cend() ? 0 : std::distance(m_alignmentButtons.cbegin(), it);
}

void QG_DlgMText::setAlignmentTL() {
    setAlignment(*bTL);
}

void QG_DlgMText::setAlignmentTC() {
    setAlignment(*bTC);
}

void QG_DlgMText::setAlignmentTR() {
    setAlignment(*bTR);
}

void QG_DlgMText::setAlignmentML() {
    setAlignment(*bML);
}

void QG_DlgMText::setAlignmentMC() {
    setAlignment(*bMC);
}

void QG_DlgMText::setAlignmentMR() {
    setAlignment(*bMR);
}

void QG_DlgMText::setAlignmentBL() {
    setAlignment(*bBL);
}

void QG_DlgMText::setAlignmentBC() {
    setAlignment(*bBC);
}

void QG_DlgMText::setAlignmentBR() {
    setAlignment(*bBR);
}

void QG_DlgMText::setAlignment(QToolButton& button) {
    button.setChecked(true);
}

int QG_DlgMText::getAlignment() {

    auto it = std::find_if(m_alignmentButtons.cbegin(), m_alignmentButtons.cend(), [](QToolButton* button){
            return button->isChecked();});
    int index = (it == m_alignmentButtons.cend()) ? 0 : std::distance(m_alignmentButtons.cbegin(), it);
    return index + 1;
}

void QG_DlgMText::setFont(const QString& f) {
    int index = cbFont->findText(f);

    // Issue #2069: default to the last font, likely unicode fonts
    if (index == -1)
        index = cbFont->findText("unicode");
    if (index >= 0) {
        cbFont->setCurrentIndex(index);
        m_font = cbFont->getFont();
        defaultChanged(false);
    }
}

void QG_DlgMText::defaultChanged(bool) {
    if (cbDefault->isChecked() && m_font != nullptr) {
        leLineSpacingFactor->setText(
                        QString::number(m_font->getLineSpacingFactor()));
    }
}

void QG_DlgMText::loadText() {
    QString fn = QFileDialog::getOpenFileName(this);
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
    QString fn = QFileDialog::getSaveFileName(this);
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
        teText->textCursor().insertText(str.mid(i+1, 1));
    }
}

void QG_DlgMText::updateUniCharButton(int) {
    QString t = cbUniChar->currentText();
    int i1 = t.indexOf(']');
    int c = t.mid(1, i1-1).toInt(nullptr, 16);
    bUnicode->setText(QString{QChar{c}});
}

void QG_DlgMText::insertChar() {
    QString t = cbUniChar->currentText();
    int i1 = t.indexOf(']');
    int c = t.mid(1, i1-1).toInt(nullptr, 16);
    teText->textCursor().insertText( QString{QChar{c}} );
}

/*
 * Event filter is used to detect focus in/out of the teText QTextEdit widget.
 * On focus in set the widget frame highlighted, on focus out revert to normal.
 */
bool QG_DlgMText::eventFilter(QObject *obj, QEvent *event) {
    int type = event->type();
    if (type == QEvent::FocusIn) {
        if (obj == teText) {
            QPalette palette = teText->QFrame::QWidget::palette();
            QColor color = palette.color(QPalette::Highlight);
            palette.setColor(QPalette::WindowText, color);
            teText->QFrame::QWidget::setPalette(palette);
        }
    } else if (type == QEvent::FocusOut) {
        if (obj == teText) {
            QPalette palette = teText->QFrame::QWidget::palette();
            QColor color = palette.color(QPalette::Dark);
            palette.setColor(QPalette::WindowText, color);
            teText->QFrame::QWidget::setPalette(palette);
        }
    }
    return false;
}
