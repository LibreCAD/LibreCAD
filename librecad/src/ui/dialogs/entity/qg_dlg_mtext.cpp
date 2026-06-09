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


#include "qg_dlg_mtext.h"

#include <QFileDialog>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>

#include "lc_textbidi.h"
#include "lc_linemath.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_settings.h"

using lc::textbidi::mirrorByLine;

/*
 *  Constructs a QG_DlgMText as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMText::QG_DlgMText(QWidget *parent, LC_GraphicViewport *viewport, RS_MText* mtext, bool forNew)
    :LC_EntityPropertiesDlg(parent, "MTextProperties", viewport){
    setupUi(this);
    m_alignmentButtons = {{bTL, bTC, bTR, bML, bMC, bMR, bBL, bBC, bBR}};
    init();
    setEntity(mtext, forNew);
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
    std::array<QWidget*,31> buttons{cbLayer,       wPen,      cbFont,
                          leHeight,      cbDefault, leLineSpacingFactor,
                          teText,        bTL,       bTC,
                          bTR,           bML,       bMC,
                          bMR,           bBL,       bBC,
                          bBR,           leAngle,   rbLeftToRight,
                          rbRightToLeft, cbSymbol,  cbUniPage,
                          cbUniChar,     bUnicode,  buttonBox,
                          bClear,        bLoad,     bSave,
                          bCut,          bCopy,     bPaste};
    // the order is cyclic
    buttons[30] = buttons.front();
    for (auto it = std::next(buttons.cbegin()); it != buttons.cend(); ++it) {
        setTabOrder(*std::prev(it), *it);
    }

    /*
     * We are using the frame colour of the teText QTextEdit widget to indicate when
     * this is the selected widget - there is no automatic highlight of this widget
     * class on selection. Ensure that known state of frame on start, and install Qt
     * Event filter in order to get callback on focus in/out of the widget.
     */
    teText->setLineWidth(2);
    teText->setMidLineWidth(0);
    teText->setFrameStyle(QFrame::Box|QFrame::Plain);
    teText->installEventFilter(this);

    // events
    connect(rbLeftToRight, &QRadioButton::toggled, this, &QG_DlgMText::layoutDirectionChanged);
}


void QG_DlgMText::updateUniCharComboBox(int) const {
    const QString t = cbUniPage->currentText();
    const int i1 = t.indexOf('-');
    const int i2 = t.indexOf(']');
    const int min = t.mid(1, i1-1).toInt(nullptr, 16);
    const int max = t.mid(i1+1, i2-i1-1).toInt(nullptr, 16);

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

void QG_DlgMText::destroy() const {
    if (m_saveSettings) {
        LC_GROUP_GUARD("Draw");
        {
            LC_SET("TextHeight", leHeight->text());
            LC_SET("TextFont", cbFont->currentText());
            LC_SET("TextDefault", cbDefault->isChecked());
            LC_SET("TextAlignment", getAlignment());
            //RS_SETTINGS->writeEntry("/TextLetterSpacing", leLetterSpacing->text());
            //RS_SETTINGS->writeEntry("/TextWordSpacing", leWordSpacing->text());
            LC_SET("TextLineSpacingFactor", leLineSpacingFactor->text());
            const bool ltr = rbLeftToRight->isChecked();
            const QString visible = teText->toPlainText();
            LC_SET("TextString", ltr ? visible : mirrorByLine(visible));
            //RS_SETTINGS->writeEntry("/TextShape", getShape());
            LC_SET("TextAngle", leAngle->text());
            //RS_SETTINGS->writeEntry("/TextRadius", leRadius->text());
            LC_SET("TextLeftToRight", ltr);
        }
    }
}

/**
 * Sets the text m_entity represented by this dialog.
 */
void QG_DlgMText::setEntity(RS_MText* t, const bool isNew) {
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
            // QByteArray iso = RS_System::localeToISO(QLocale::system().name().toLocal8Bit());
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
            leftToRight = LC_GET_BOOL("TextLeftToRight", true);
        }
    } else {
        font = m_entity->getStyle();
        setFont(font);
        height = QString::number(m_entity->getHeight());
        if (m_font != nullptr) {
            if (LC_LineMath::isSameLength(m_font->getLineSpacingFactor(),m_entity->getLineSpacingFactor())) {
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

        const double wcsAngle = m_entity->getAngle();
        angle = toUIAngleDeg(wcsAngle);

        RS_Graphic* graphic = m_entity->getGraphic();
        if (graphic != nullptr) {
            const auto layerList = graphic->getLayerList();
            cbLayer->init(*layerList, false, false);
        }

        RS_Layer* layer = m_entity->getLayer(false);
        if (layer != nullptr) {
            cbLayer->setLayer(*layer);
        }

        wPen->setPen(m_entity, layer, tr("Pen"));
        leftToRight = m_entity->getDrawingDirection() == RS_MTextData::LeftToRight;
    }

    cbDefault->setChecked(def=="1");
    setFont(font);
    leHeight->setText(height);
    const unsigned index = alignment.toUInt() - 1;
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
    teText->setText(leftToRight ? str : mirrorByLine(str));
    //setShape(shape.toInt());
    leAngle->setText(angle);
    //leRadius->setText(radius);
    teText->setFocus();
    teText->selectAll();
    // Block the radio buttons' toggled signal: layoutDirectionChanged is
    // wired to rbLeftToRight and would re-mirror the buffer we just set.
    {
      QSignalBlocker bL(rbLeftToRight);
      QSignalBlocker bR(rbRightToLeft);
      rbLeftToRight->setChecked(leftToRight);
      rbRightToLeft->setChecked(!leftToRight);
    }
    applyDirectionVisuals();
}

void QG_DlgMText::layoutDirectionChanged const()
{
  const bool leftToRight = rbLeftToRight->isChecked();
  rbRightToLeft->setChecked(!leftToRight);

  // Capture the cursor's (block, column, blockLength) BEFORE the mirror so
  // we can map back to the mirrored column afterwards. Block structure is
  // preserved by per-line mirroring (newlines unchanged), so block number
  // and length stay the same — only the column flips within its block.
  auto capture = [](const QTextDocument *doc, int pos) {
    const QTextBlock block = doc->findBlock(pos);
    const int blockNum = block.blockNumber();
    int blockLen = block.length();
    if (block.next().isValid())
      --blockLen; // strip trailing newline
    const int col = pos - block.position();
    return std::tuple<int, int, int>{blockNum, col, blockLen};
  };
  const QTextCursor oldCursor = teText->textCursor();
  const auto anchorInfo = capture(teText->document(), oldCursor.anchor());
  const auto posInfo = capture(teText->document(), oldCursor.position());

  // Flip the visible buffer so the same logical text now reads in the
  // newly-selected direction. The entity's text field is unchanged here;
  // it gets rewritten in updateEntity() from the (mirrored-back) buffer.
  {
    QSignalBlocker textBlocker(teText);
    const QString visible = teText->toPlainText();
    teText->setPlainText(mirrorByLine(visible));
  }

  // Restore cursor: the column index flips within its block (length is
  // identical pre- and post-mirror, so col → blockLen - col).
  auto restore = [](const QTextDocument *doc,
                    const std::tuple<int, int, int> &info) -> int {
    const auto [blockNum, col, blockLen] = info;
    const QTextBlock block = doc->findBlockByNumber(blockNum);
    if (!block.isValid())
      return 0;
    return block.position() + (blockLen - col);
  };
  {
    QTextCursor c = teText->textCursor();
    c.setPosition(restore(teText->document(), anchorInfo));
    c.setPosition(restore(teText->document(), posInfo),
                  QTextCursor::KeepAnchor);
    teText->setTextCursor(c);
  }

  if (m_entity != nullptr) {
    m_entity->setDrawingDirection(leftToRight ? RS_MTextData::LeftToRight
                                              : RS_MTextData::RightToLeft);
  }
  applyDirectionVisuals();
}

void QG_DlgMText::applyDirectionVisuals() {
  const bool leftToRight = rbLeftToRight->isChecked();
  const Qt::LayoutDirection direction =
      leftToRight ? Qt::LeftToRight : Qt::RightToLeft;
  teText->setLayoutDirection(direction);

  QTextDocument *doc = teText->document();
  if (doc == nullptr)
    return;

  QTextOption option = doc->defaultTextOption();
  option.setTextDirection(direction);
  doc->setDefaultTextOption(option);

  // Stamp per-block layout direction so already-typed blocks reflow now —
  // setDefaultTextOption alone only governs future content.
  QSignalBlocker textBlocker(teText);
  QTextCursor cursor(doc);
  cursor.beginEditBlock();
  cursor.movePosition(QTextCursor::Start);
  do {
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setLayoutDirection(direction);
    cursor.setBlockFormat(fmt);
  } while (cursor.movePosition(QTextCursor::NextBlock));
  cursor.endEditBlock();

  teText->update();
}

/**
 * Updates the text m_entity represented by the dialog to fit the choices of the user.
 */
void QG_DlgMText::updateEntity() {
    if (m_entity == nullptr) {
        return;
    }

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
    {
      const bool ltr = rbLeftToRight->isChecked();
      const QString visible = teText->toPlainText();
      m_entity->setText(ltr ? visible : mirrorByLine(visible));
    }
    //#endif
    //text->setLetterSpacing(leLetterSpacing.toDouble());
    m_entity->setLineSpacingFactor(leLineSpacingFactor->text().toDouble());
    m_entity->setAlignment(getAlignment());

    const double wcsAngle = toWCSAngle(leAngle, m_entity->getAngle());
    m_entity->setAngle(wcsAngle);

    m_entity->setDrawingDirection(rbLeftToRight->isChecked() ? RS_MTextData::LeftToRight : RS_MTextData::RightToLeft);
    if (!m_isNew) {
        m_entity->setPen(wPen->getPen());
        m_entity->setLayer(cbLayer->getLayer());
    }
    m_entity->update();
}

size_t QG_DlgMText::alignmentButtonIdex(const QToolButton* button) const
{
    const auto it = std::find(m_alignmentButtons.cbegin(), m_alignmentButtons.cend(), button);
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

int QG_DlgMText::getAlignment() const {

    const auto it = std::find_if(m_alignmentButtons.cbegin(), m_alignmentButtons.cend(), [](const QToolButton* button){
            return button->isChecked();});
    const int index = (it == m_alignmentButtons.cend()) ? 0 : std::distance(m_alignmentButtons.cbegin(), it);
    return index + 1;
}

void QG_DlgMText::setFont(const QString& f) {
    int index = cbFont->findText(f);

    // Issue #2069: default to the last font, likely unicode fonts
    if (index == -1) {
        index = cbFont->findText("unicode");
    }
    if (index >= 0) {
        cbFont->setCurrentIndex(index);
        m_font = cbFont->getFont();
        defaultChanged(false);
    }
}

void QG_DlgMText::defaultChanged(bool) const {
    if (cbDefault->isChecked() && m_font != nullptr) {
        leLineSpacingFactor->setText(
                        QString::number(m_font->getLineSpacingFactor()));
    }
}

void QG_DlgMText::loadText() {
    const QString fn = QFileDialog::getOpenFileName(this);
    if (!fn.isEmpty()) {
        load(fn);
    }
}

void QG_DlgMText::load(const QString& fn) const {
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream ts(&f);
    const QString contents = ts.readAll();
    const bool ltr = rbLeftToRight->isChecked();
    teText->setText(ltr ? contents : mirrorByLine(contents));
}

void QG_DlgMText::saveText() {
    const QString fn = QFileDialog::getSaveFileName(this);
    if (!fn.isEmpty()) {
        save(fn);
    }
}

void QG_DlgMText::save(const QString& fn) const {
  const bool ltr = rbLeftToRight->isChecked();
  const QString visible = teText->toPlainText();
  const QString text = ltr ? visible : mirrorByLine(visible);
  QFile f(fn);
  if (f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << text;
    f.close();
  }
}

void QG_DlgMText::insertSymbol(int) const {
    const QString str = cbSymbol->currentText();
    const int i=str.indexOf('(');
    if (i!=-1) {
        teText->textCursor().insertText(str.mid(i+1, 1));
    }
}

void QG_DlgMText::updateUniCharButton(int) const {
    const QString t = cbUniChar->currentText();
    const int i1 = t.indexOf(']');
    const int c = t.mid(1, i1-1).toInt(nullptr, 16);
    bUnicode->setText(QString{QChar{c}});
}

void QG_DlgMText::insertChar() const {
    const QString t = cbUniChar->currentText();
    const int i1 = t.indexOf(']');
    const int c = t.mid(1, i1-1).toInt(nullptr, 16);
    teText->textCursor().insertText( QString{QChar{c}} );
}

/*
 * Event filter is used to detect focus in/out of the teText QTextEdit widget.
 * On focus in set the widget frame highlighted, on focus out revert to normal.
 */
bool QG_DlgMText::eventFilter(QObject *obj, QEvent *event) {
    const int type = event->type();
    if (type == QEvent::FocusIn) {
        if (obj == teText) {
            QPalette palette = teText->palette();
            const QColor color = palette.color(QPalette::Highlight);
            palette.setColor(QPalette::WindowText, color);
            teText->setPalette(palette);
        }
    } else if (type == QEvent::FocusOut) {
        if (obj == teText) {
            QPalette palette = teText->palette();
            const QColor color = palette.color(QPalette::Dark);
            palette.setColor(QPalette::WindowText, color);
            teText->setPalette(palette);
        }
    }
    return false;
}
