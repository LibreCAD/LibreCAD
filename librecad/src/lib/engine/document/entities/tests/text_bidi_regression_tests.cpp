/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
** USA.
**
**********************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <limits>

#include <QApplication>
#include <QFile>
#include <QClipboard>
#include <QDropEvent>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QRadioButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextLayout>

#include "lc_graphicviewport.h"
#include "lc_mtextpropertieseditingwidget.h"
#include "lc_textbidi.h"
#include "lc_textedit.h"
#include "lc_textpropertieseditingwidget.h"
#include "qg_dlg_mtext.h"
#include "qg_dlg_text.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_text.h"

namespace {
struct TestApplication {
  TestApplication() {
    if (!QCoreApplication::instance()) {
      qputenv("QT_QPA_PLATFORM", "offscreen");
      static int argc = 1;
      static char name[] = "text_bidi_tests";
      static char *argv[] = {name, nullptr};
      if (qEnvironmentVariableIsSet("LIBRECAD_TEST_TEXT_HEADLESS")) {
        new QCoreApplication(argc, argv);
      } else {
        new QApplication(argc, argv);
      }
    }
  }
} testApplication;

class SourceFonts {
public:
  SourceFonts() {
    previousDebugLevel = RS_DEBUG->getLevel();
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
    static bool initialized = [] {
      RS_Settings::init("LibreCAD", "LibreCAD-tests");
      RS_SYSTEM->init("LibreCAD", "tests", "librecad", "text_bidi_tests");
      return true;
    }();
    (void)initialized;
    LC_GROUP_GUARD("Paths");
    previous = LC_GET_STR("Fonts", "");
    LC_SET("Fonts",
           QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
    RS_FONTLIST->clearFonts();
    RS_FONTLIST->init();
  }
  ~SourceFonts() {
    LC_GROUP_GUARD("Paths");
    LC_SET("Fonts", previous);
    RS_FONTLIST->clearFonts();
    RS_FONTLIST->init();
    RS_DEBUG->setLevel(previousDebugLevel);
  }

private:
  QString previous;
  RS_Debug::RS_DebugLevel previousDebugLevel;
};

RS_MTextData mtextData(const QString &text) {
  return {RS_Vector(0.0, 0.0),
          9.0,
          100.0,
          RS_MTextData::VATop,
          RS_MTextData::HALeft,
          RS_MTextData::RightToLeft,
          RS_MTextData::Exact,
          1.0,
          text,
          "iso3098",
          0.0,
          RS2::Update};
}

QString glyphNames(const RS_EntityContainer &container) {
  QString result;
  double previousX = -std::numeric_limits<double>::infinity();
  for (auto *entity : container.getEntityList()) {
    if (entity->rtti() == RS2::EntityInsert) {
      auto *glyph = static_cast<RS_Insert *>(entity);
      CHECK(glyph->getInsertionPoint().x >= previousX);
      previousX = glyph->getInsertionPoint().x;
      result += glyph->getName();
    }
  }
  return result;
}

RS_MText *findMText(RS_Graphic &graphic) {
  for (auto *entity : graphic.getEntityList()) {
    if (entity->rtti() == RS2::EntityMText)
      return static_cast<RS_MText *>(entity);
  }
  return nullptr;
}

std::vector<std::int32_t> libreCadIntegers(const RS_Entity &entity) {
  std::vector<std::int32_t> values;
  bool inLibreCad = false;
  for (const auto &tag : entity.getDrwExtData()) {
    if (tag->code() == 1001)
      inLibreCad = std::string(tag->c_str()) == "LibreCad";
    if (inLibreCad && tag->code() == 1071 &&
        tag->type() == DRW_Variant::INTEGER)
      values.push_back(tag->content.i);
  }
  return values;
}

void keyPress(QWidget &edit, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier,
              const QString &text = QString()) {
  QKeyEvent event(QEvent::KeyPress, code, modifiers, text);
  QApplication::sendEvent(&edit, &event);
  QApplication::processEvents();
}

void mouseEvent(LC_TextEdit &edit, QEvent::Type type, const QPoint &position,
                Qt::MouseButton button = Qt::LeftButton,
                Qt::MouseButtons buttons = Qt::LeftButton) {
  QMouseEvent event(type, position, edit.viewport()->mapToGlobal(position),
                    button, buttons, Qt::NoModifier);
  QApplication::sendEvent(edit.viewport(), &event);
  QApplication::processEvents();
}
} // namespace

TEST_CASE("RTL entity geometry preserves numbers", "[text][bidi][issue1859]") {
  SourceFonts fonts;
  for (const QString &value :
       {QStringLiteral("123"), QStringLiteral("12.5"),
        QStringLiteral("1,234.56"), QStringLiteral("12/34"),
        QStringLiteral("(123)"), QStringLiteral("Main 123")}) {
    INFO(value.toStdString());
    RS_MText mtext(nullptr, mtextData(value));
    REQUIRE(mtext.count() == 1);
    auto *line = dynamic_cast<RS_EntityContainer *>(mtext.entityAt(0));
    REQUIRE(line);
    CHECK(glyphNames(*line) == QString(value).remove(QLatin1Char(' ')));
    CHECK(mtext.getText() == value);

    RS_TextData data(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0), 9.0, 1.0,
                     RS_TextData::VATop, RS_TextData::HALeft, RS_TextData::None,
                     value, "iso3098", 0.0, RS2::Update);
    data.drawingDirection = RS_TextData::RightToLeft;
    RS_Text text(nullptr, data);
    CHECK(glyphNames(text) == QString(value).remove(QLatin1Char(' ')));
  }
}

TEST_CASE("RTL rendering keeps code points and hides bidi controls",
          "[text][bidi][issue1859]") {
  SourceFonts fonts;
  const char32_t nonBmp[] = {0x1f600};
  RS_MText supplementary(nullptr, mtextData(QString::fromUcs4(nonBmp, 1)));
  auto *line = dynamic_cast<RS_EntityContainer *>(supplementary.entityAt(0));
  REQUIRE(line);
  CHECK(line->count() == 1);

  const QString isolated =
      QString(QChar(0x2066)) + QStringLiteral("123") + QChar(0x2069);
  RS_MText controls(nullptr, mtextData(isolated));
  line = dynamic_cast<RS_EntityContainer *>(controls.entityAt(0));
  REQUIRE(line);
  CHECK(line->count() == 3);
  CHECK(glyphNames(*line) == "123");

  RS_MText combining(nullptr, mtextData(QStringLiteral("A") + QChar(0x0301)));
  line = dynamic_cast<RS_EntityContainer *>(combining.entityAt(0));
  REQUIRE(line);
  REQUIRE(line->count() == 2);
  auto *base = static_cast<RS_Insert *>(line->entityAt(0));
  auto *mark = static_cast<RS_Insert *>(line->entityAt(1));
  CHECK(base->getInsertionPoint() == mark->getInsertionPoint());
}

TEST_CASE("Hebrew and decimal labels render in Unicode visual order",
          "[text][bidi][issue1859]") {
  SourceFonts fonts;
  const QString input = QString::fromUtf8(u8"שלום (12.5)");
  const QString expected = QString::fromUtf8(u8"(12.5)םולש");
  auto data = mtextData(input);
  data.style = "unicode";
  RS_MText mtext(nullptr, data);
  auto *line = dynamic_cast<RS_EntityContainer *>(mtext.entityAt(0));
  REQUIRE(line);
  CHECK(glyphNames(*line) == expected);

  RS_TextData textData;
  textData.text = input;
  textData.style = "unicode";
  textData.drawingDirection = RS_TextData::ByContent;
  textData.updateMode = RS2::Update;
  RS_Text text(nullptr, textData);
  CHECK(glyphNames(text) == expected);
}

TEST_CASE("TEXT and MTEXT consume complete UTF-16 clusters",
          "[text][bidi]") {
  SourceFonts fonts;
  const char32_t scalar = 0x1f600;
  const auto pair = QString::fromUcs4(&scalar, 1);
  const QString values[] = {
      "A" + pair + "B", "A" + pair, "A" + QChar(0xd83d),
      "A" + QChar(0xde00), "A" + pair + QChar(0x0301) + "B"};
  for (const QString &value : values) {
    for (bool rtl : {false, true}) {
      INFO(value.toStdString());
      INFO(rtl);
      const auto glyphCount = value.toUcs4().size();
      RS_TextData data;
      data.text = value;
      data.style = "iso3098";
      data.drawingDirection = rtl ? RS_TextData::RightToLeft
                                 : RS_TextData::LeftToRight;
      data.updateMode = RS2::Update;
      RS_Text text(nullptr, data);
      CHECK(text.count() == glyphCount);
      CHECK(text.getText() == value);

      auto multilineData = mtextData(value);
      multilineData.drawingDirection = rtl ? RS_MTextData::RightToLeft
                                          : RS_MTextData::LeftToRight;
      RS_MText mtext(nullptr, multilineData);
      auto *line = dynamic_cast<RS_EntityContainer *>(mtext.entityAt(0));
      REQUIRE(line);
      CHECK(line->count() == glyphCount);
      CHECK(mtext.getText() == value);
    }
  }
}

TEST_CASE("Supplementary controls and marks do not advance LFF geometry",
          "[text][bidi]") {
  SourceFonts fonts;
  const char32_t format = 0xe0001, mark = 0x1e944;
  const auto controlText = "1" + QString::fromUcs4(&format, 1) + "2";
  const auto markedText = "1" + QString::fromUcs4(&mark, 1) + "2";
  auto checkGeometry = [](const RS_EntityContainer &baseline,
                          const RS_EntityContainer &controlled,
                          const RS_EntityContainer &marked) {
    REQUIRE(baseline.count() == 2);
    REQUIRE(controlled.count() == 2);
    REQUIRE(marked.count() == 3);
    auto point = [](const RS_EntityContainer &container, int i) {
      auto *glyph = dynamic_cast<RS_Insert *>(container.entityAt(i));
      REQUIRE(glyph);
      return glyph->getInsertionPoint();
    };
    CHECK(point(marked, 0) == point(marked, 1));
    const auto advance = point(baseline, 1) - point(baseline, 0);
    CHECK(point(controlled, 1) - point(controlled, 0) == advance);
    CHECK(point(marked, 2) - point(marked, 0) == advance);
  };
  auto makeMText = [](const QString &value) {
    auto data = mtextData(value);
    data.drawingDirection = RS_MTextData::LeftToRight;
    return data;
  };
  RS_MText baseline(nullptr, makeMText("12"));
  RS_MText controlled(nullptr, makeMText(controlText));
  RS_MText marked(nullptr, makeMText(markedText));
  auto line = [](const RS_MText &text) {
    auto *result = dynamic_cast<RS_EntityContainer *>(text.entityAt(0));
    REQUIRE(result);
    return result;
  };
  checkGeometry(*line(baseline), *line(controlled), *line(marked));
  auto makeText = [](const QString &value) {
    RS_TextData data;
    data.text = value;
    data.style = "iso3098";
    data.drawingDirection = RS_TextData::LeftToRight;
    data.updateMode = RS2::Update;
    return data;
  };
  RS_Text textBaseline(nullptr, makeText("12"));
  RS_Text textControlled(nullptr, makeText(controlText));
  RS_Text textMarked(nullptr, makeText(markedText));
  checkGeometry(textBaseline, textControlled, textMarked);
}

TEST_CASE("Unicode bidi preserves clusters and mixed numeric runs",
          "[textbidi][issue1859]") {
  const QString hebrew = QString::fromUtf8("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d");
  for (const QString &number :
       {QStringLiteral("123"), QStringLiteral("12.5"),
        QStringLiteral("1,234.56"), QStringLiteral("12/34"),
        QStringLiteral("Main 123")}) {
    const QString input = hebrew + QLatin1Char(' ') + number;
    QString visual;
    for (const auto &cluster :
         lc::textbidi::visualClusters(input, Qt::RightToLeft))
      visual += input.mid(cluster.start, cluster.length);
    CHECK(visual ==
          number + QLatin1Char(' ') + lc::textbidi::mirrorByLine(hebrew));
  }
  const QString marked =
      QString(QChar(0x05e9)) + QChar(0x05b0) + QStringLiteral(" 123");
  const auto clusters = lc::textbidi::visualClusters(marked, Qt::RightToLeft);
  REQUIRE(!clusters.empty());
  CHECK(clusters.back().start == 0);
  CHECK(clusters.back().length == 2);
  const char32_t nonBmp[] = {0x1f600};
  const auto pair = lc::textbidi::visualClusters(QString::fromUcs4(nonBmp, 1),
                                                 Qt::RightToLeft);
  REQUIRE(pair.size() == 1);
  CHECK(pair.front().length == 2);

  const QString isolated = hebrew + QLatin1Char(' ') + QChar(0x2066) +
                           QStringLiteral("Main 12.5") + QChar(0x2069);
  QString visual;
  for (const auto &cluster :
       lc::textbidi::visualClusters(isolated, Qt::RightToLeft)) {
    auto fragment = isolated.mid(cluster.start, cluster.length);
    if (fragment.front().category() != QChar::Other_Format)
      visual += fragment;
  }
  CHECK(visual ==
        QStringLiteral("Main 12.5 ") + lc::textbidi::mirrorByLine(hebrew));
}

TEST_CASE("MTEXT formatting retains bidi context and stack ownership",
          "[text][bidi]") {
  SourceFonts fonts;
  const auto hebrew = QString::fromUtf8(u8"שלום");
  const auto ending = QString::fromUtf8(u8"אב");
  auto data = mtextData(hebrew + " \\f{iso3098}12.5\\f{unicode} " + ending);
  data.style = "unicode";
  RS_MText formatted(nullptr, data);
  auto *line = dynamic_cast<RS_EntityContainer *>(formatted.entityAt(0));
  REQUIRE(line);
  CHECK(glyphNames(*line) == lc::textbidi::mirrorByLine(ending) + "12.5" +
                            lc::textbidi::mirrorByLine(hebrew));

  data.text = hebrew + " \\S12^34; " + ending;
  RS_MText stacked(nullptr, data);
  line = dynamic_cast<RS_EntityContainer *>(stacked.entityAt(0));
  REQUIRE(line);
  CHECK(glyphNames(*line) == lc::textbidi::mirrorByLine(ending) +
                            lc::textbidi::mirrorByLine(hebrew));
  int children = 0;
  for (auto *entity : line->getEntityList()) {
    if (auto *child = dynamic_cast<RS_MText *>(entity)) {
      CHECK(child->getParent() == line);
      CHECK(child->getText() == (children == 0 ? "12" : "34"));
      auto *childLine = dynamic_cast<RS_EntityContainer *>(child->entityAt(0));
      REQUIRE(childLine);
      CHECK(glyphNames(*childLine) == child->getText());
      ++children;
    }
  }
  CHECK(children == 2);

  RS_MText truncated(nullptr, mtextData("12.5\\f"));
  line = dynamic_cast<RS_EntityContainer *>(truncated.entityAt(0));
  REQUIRE(line);
  CHECK(glyphNames(*line) == "12.5");
  CHECK(truncated.getText() == "12.5\\f");
}

TEST_CASE("RTL editors keep logical text and cursor selections",
          "[text][bidi][issue1859][gui]") {
  SourceFonts fonts;
  const QString input = QStringLiteral("ABC ") +
                        QString::fromUtf8("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d") +
                        QStringLiteral(" 12.5");
  const QString hebrewVisual =
      QString::fromUtf8("\xd7\x9d\xd7\x95\xd7\x9c\xd7\xa9");
  auto data = mtextData(input);
  data.style = "unicode";
  RS_MText mtext(nullptr, data);
  const auto checkLayout = [&](bool rtl) {
    CHECK(mtext.getDrawingDirection() ==
          (rtl ? RS_MTextData::RightToLeft : RS_MTextData::LeftToRight));
    auto *line = dynamic_cast<RS_EntityContainer *>(mtext.entityAt(0));
    REQUIRE(line);
    const QString expected = rtl ? QString("12.5" + hebrewVisual + "ABC")
                                 : QString("ABC12.5" + hebrewVisual);
    CHECK(glyphNames(*line) == expected);
  };
  LC_GraphicViewport viewport;
  LC_MTextPropertiesEditingWidget editor(nullptr);
  editor.setGraphicViewport(&viewport);
  editor.setEntity(&mtext);
  auto *edit = editor.findChild<LC_TextEdit *>("teText");
  REQUIRE(edit);
  auto cursor = edit->textCursor();
  cursor.setPosition(1);
  cursor.setPosition(3, QTextCursor::KeepAnchor);
  edit->setTextCursor(cursor);
  for (const char *button : {"rbLeftToRight", "rbRightToLeft"}) {
    auto *radio = editor.findChild<QRadioButton *>(button);
    REQUIRE(radio);
    radio->click();
    const bool rtl = radio->objectName() == "rbRightToLeft";
    CHECK(edit->layoutDirection() == (rtl ? Qt::RightToLeft : Qt::LeftToRight));
    CHECK(edit->document()->defaultTextOption().alignment().testFlag(
          rtl ? Qt::AlignRight : Qt::AlignLeft));
    checkLayout(rtl);
    CHECK(edit->toPlainText() == input);
    CHECK(mtext.getText() == input);
    CHECK(edit->textCursor().anchor() == 1);
    CHECK(edit->textCursor().position() == 3);
  }
  edit->clear();
  CHECK(edit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));
  edit->setPlainText(input);
  CHECK(edit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));

  QG_DlgMText dialog(nullptr, &viewport, &mtext, false);
  auto *dialogEdit = dialog.findChild<QTextEdit *>("teText");
  REQUIRE(dialogEdit);
  REQUIRE(dialogEdit->toPlainText() == input);
  for (const char *button : {"rbLeftToRight", "rbRightToLeft"}) {
    auto *radio = dialog.findChild<QRadioButton *>(button);
    REQUIRE(radio);
    radio->click();
    const bool rtl = radio->objectName() == "rbRightToLeft";
    CHECK(dialogEdit->layoutDirection() ==
          (rtl ? Qt::RightToLeft : Qt::LeftToRight));
    CHECK(dialogEdit->document()->defaultTextOption().alignment().testFlag(
          rtl ? Qt::AlignRight : Qt::AlignLeft));
    dialog.updateEntity();
    checkLayout(rtl);
    CHECK(dialogEdit->toPlainText() == input);
  }
  CHECK(mtext.getText() == input);
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  const auto textPath = directory.filePath("logical.txt");
  dialog.save(textPath);
  QFile saved(textPath);
  REQUIRE(saved.open(QIODevice::ReadOnly));
  CHECK(QString::fromUtf8(saved.readAll()) == input);
  saved.close();
  dialogEdit->clear();
  CHECK(dialogEdit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));
  dialog.load(textPath);
  CHECK(dialogEdit->toPlainText() == input);
  CHECK(dialogEdit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));
  dialogEdit->setPlainText("<b>" + input + "</b>");
  dialog.updateEntity();
  CHECK(mtext.getText() == "<b>" + input + "</b>");
  dialog.reject();

  RS_TextData textData;
  textData.text = input;
  textData.style = "iso3098";
  textData.drawingDirection = RS_TextData::RightToLeft;
  RS_Text text(nullptr, textData);
  LC_TextPropertiesEditingWidget textEditor(nullptr);
  textEditor.setGraphicViewport(&viewport);
  textEditor.setEntity(&text);
  auto *textEdit = textEditor.findChild<QTextEdit *>("leText");
  REQUIRE(textEdit);
  REQUIRE(textEdit->toPlainText() == input);
  cursor = textEdit->textCursor();
  cursor.setPosition(1);
  cursor.setPosition(3, QTextCursor::KeepAnchor);
  textEdit->setTextCursor(cursor);
  for (const char *button : {"rbAuto", "rbLeftToRight", "rbRightToLeft"}) {
    textEditor.findChild<QRadioButton *>(button)->setChecked(true);
    CHECK(textEdit->toPlainText() == input);
    CHECK(textEdit->textCursor().anchor() == 1);
    CHECK(textEdit->textCursor().position() == 3);
    const auto direction = QString(button) == "rbAuto" ? Qt::LayoutDirectionAuto
        : QString(button) == "rbRightToLeft" ? Qt::RightToLeft : Qt::LeftToRight;
    CHECK(textEdit->document()->defaultTextOption().textDirection() == direction);
  }
  textEdit->clear();
  CHECK(textEdit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));
  textEdit->setPlainText(input);
  CHECK(textEdit->document()->defaultTextOption().textDirection() == Qt::RightToLeft);
  CHECK(textEdit->document()->defaultTextOption().alignment().testFlag(Qt::AlignRight));

  for (bool isNew : {false, true}) {
    QG_DlgText textDialog(nullptr, &viewport, &text, isNew);
    auto *textDialogEdit = textDialog.findChild<QTextEdit *>("teText");
    REQUIRE(textDialogEdit);
    textDialogEdit->setPlainText(input);
    for (const auto direction : {RS_TextData::LeftToRight,
                                RS_TextData::RightToLeft, RS_TextData::ByContent}) {
      const char *button = direction == RS_TextData::ByContent ? "rbAuto"
          : direction == RS_TextData::RightToLeft ? "rbRightToLeft" : "rbLeftToRight";
      auto *radio = textDialog.findChild<QRadioButton *>(button);
      REQUIRE(radio);
      radio->click();
      textDialog.updateEntity();
      CHECK(text.getDrawingDirection() == direction);
      CHECK(text.getText() == input);
      const auto qtDirection = direction == RS_TextData::ByContent ? Qt::LayoutDirectionAuto
          : direction == RS_TextData::RightToLeft ? Qt::RightToLeft : Qt::LeftToRight;
      CHECK(textDialogEdit->document()->defaultTextOption().textDirection() == qtDirection);
      textDialogEdit->clear();
      CHECK(textDialogEdit->document()->defaultTextOption().alignment().testFlag(
          direction == RS_TextData::RightToLeft ? Qt::AlignRight : Qt::AlignLeft));
      textDialogEdit->setPlainText(input);
      CHECK(textDialogEdit->document()->defaultTextOption().textDirection() == qtDirection);
    }
    textDialog.reject();
  }
}

TEST_CASE("Text input paragraphs follow the selected direction",
          "[text][bidi][gui]") {
  const QString input = QStringLiteral("ABC ") +
                        QString::fromUtf8("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d") +
                        QStringLiteral(" 12.5");
  const auto checkEditor = [&](auto &edit) {
    INFO(edit.metaObject()->className());
    edit.resize(600, 160);
    edit.show();
    for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft, Qt::LayoutDirectionAuto}) {
      edit.setPlainText(input + "\n" + input);
      edit.setTextDirection(direction);
      QApplication::processEvents();
      for (auto block = edit.document()->begin(); block.isValid(); block = block.next()) {
        CHECK(edit.document()->defaultTextOption().textDirection() == direction);
        REQUIRE(block.layout()->lineCount() > 0);
        const auto line = block.layout()->lineAt(0);
        const auto firstLetter = line.cursorToX(0);
        const auto firstDigit = line.cursorToX(input.indexOf("12.5"));
        CHECK((firstLetter > firstDigit) == (direction == Qt::RightToLeft));
      }
      edit.setPlainText("123");
      edit.setTextDirection(direction);
      QApplication::processEvents();
      const QTextCursor start(edit.document());
      CHECK((edit.cursorRect(start).left() > edit.viewport()->width() / 2) ==
            (direction == Qt::RightToLeft));
      CHECK(edit.toPlainText() == "123");
      edit.clear();
      edit.setTextDirection(direction);
      QApplication::processEvents();
      CHECK((edit.cursorRect().left() > edit.viewport()->width() / 2) ==
            (direction == Qt::RightToLeft));
      auto cursor = edit.textCursor();
      cursor.insertText(input);
      cursor.insertBlock();
      cursor.insertText(input);
      QApplication::processEvents();
      for (auto block = edit.document()->begin(); block.isValid(); block = block.next()) {
        const auto line = block.layout()->lineAt(0);
        CHECK((line.cursorToX(0) > line.cursorToX(input.indexOf("12.5"))) ==
              (direction == Qt::RightToLeft));
      }
      CHECK(edit.toPlainText() == input + "\n" + input);
    }
  };
  LC_TextEdit multiline;
  checkEditor(multiline);
  LC_SingleLineTextEdit singleLine;
  checkEditor(singleLine);
}

TEST_CASE("Single-line text input preserves editing behavior",
          "[text][bidi][gui]") {
  struct Input : LC_SingleLineTextEdit {
    using LC_SingleLineTextEdit::insertFromMimeData;
  } edit;
  edit.setText("<b>123</b>");
  edit.setTextDirection(Qt::RightToLeft);
  CHECK(edit.text() == "<b>123</b>");
  edit.selectAll();
  QMimeData clipboard;
  clipboard.setText("A\r\nB\nC\rD" + QString(QChar::LineSeparator) + "E" +
                    QChar::ParagraphSeparator + "F");
  edit.insertFromMimeData(&clipboard);
  CHECK(edit.text() == "A B C D E F");
  CHECK(edit.document()->blockCount() == 1);
  edit.undo();
  CHECK(edit.text() == "<b>123</b>");
  edit.redo();
  CHECK(edit.text() == "A B C D E F");
  int finished = 0;
  QObject::connect(&edit, &LC_SingleLineTextEdit::editingFinished, [&] { ++finished; });
  for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(&edit, &event);
    CHECK_FALSE(event.isAccepted());
  }
  CHECK(finished == 2);
  CHECK(edit.document()->blockCount() == 1);
  edit.resize(100, edit.sizeHint().height());
  edit.show();
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    edit.setTextDirection(direction);
    for (const auto &value : {QStringLiteral("123"), QStringLiteral("CAD123 ").repeated(30)}) {
      edit.setText(value);
      QApplication::processEvents();
      REQUIRE(edit.document()->begin().layout()->lineCount() == 1);
      auto caret = QTextCursor(edit.document());
      for (const int position : {0, int(value.size()) / 2, int(value.size())}) {
        caret.setPosition(position);
        edit.setTextCursor(caret);
        QApplication::processEvents();
        CAPTURE(direction, position);
        CHECK(edit.cursorRect().left() >= 0);
        CHECK(edit.cursorRect().right() < edit.viewport()->width());
      }
    }
  }
}

TEST_CASE("Chinese direction switching keeps editor and entity geometry consistent",
          "[text][bidi][chinese][gui]") {
  SourceFonts fonts;
  const auto input = QStringLiteral("\u4e2d\u6587123");
  const auto rtl = QStringLiteral("123\u6587\u4e2d");
  auto data = mtextData(input);
  data.style = "unicode";
  RS_MText mtext(nullptr, data);
  RS_TextData textData;
  textData.text = input;
  textData.style = "unicode";
  textData.updateMode = RS2::Update;
  RS_Text text(nullptr, textData);
  LC_GraphicViewport viewport;
  LC_MTextPropertiesEditingWidget mtextEditor(nullptr);
  LC_TextPropertiesEditingWidget textEditor(nullptr);
  mtextEditor.setGraphicViewport(&viewport);
  textEditor.setGraphicViewport(&viewport);
  mtextEditor.setEntity(&mtext);
  textEditor.setEntity(&text);
  for (auto *panel : {static_cast<QWidget *>(&mtextEditor), static_cast<QWidget *>(&textEditor)}) {
    panel->resize(600, 500);
    panel->show();
    auto *edit = panel->findChild<LC_TextEdit *>();
    REQUIRE(edit);
    auto cursor = edit->textCursor();
    cursor.setPosition(1);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    edit->setTextCursor(cursor);
    for (const char *button : {"rbLeftToRight", "rbRightToLeft", "rbLeftToRight", "rbRightToLeft"}) {
      panel->findChild<QRadioButton *>(button)->click();
      QApplication::processEvents();
      const bool isRtl = QString(button) == "rbRightToLeft";
      CHECK(edit->toPlainText() == input);
      CHECK(edit->document()->toPlainText() == input);
      CHECK(edit->textCursor().anchor() == 1);
      CHECK(edit->textCursor().position() == 3);
      auto start = QTextCursor(edit->document());
      auto second = start;
      second.setPosition(1);
      CHECK((edit->cursorRect(start).x() > edit->cursorRect(second).x()) == isRtl);
      auto digit = start;
      digit.setPosition(2);
      auto nextDigit = start;
      nextDigit.setPosition(3);
      CHECK(edit->cursorRect(digit).x() < edit->cursorRect(nextDigit).x());
      if (panel == &mtextEditor) {
        CHECK(glyphNames(*static_cast<RS_EntityContainer *>(mtext.entityAt(0))) ==
              (isRtl ? rtl : input));
      } else {
        CHECK(glyphNames(text) == (isRtl ? rtl : input));
      }
    }
    panel->hide();
  }
}

TEST_CASE("Chinese mixed-number editing keeps logical text and native undo",
          "[text][bidi][chinese][gui]") {
  const auto input = QStringLiteral("\u4e2d\u6587123");
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    LC_TextEdit edit;
    edit.resize(600, 160);
    edit.setPlainText(input);
    edit.setTextDirection(direction);
    edit.show();
    edit.setFocus();
    QApplication::processEvents();
    auto cursor = QTextCursor(edit.document());
    cursor.setPosition(2);
    edit.setTextCursor(cursor);
    keyPress(edit, Qt::Key_Backspace);
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d123"));
    edit.undo();
    CHECK(edit.toPlainText() == input);
    edit.redo();
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d123"));
    edit.undo();
    cursor.setPosition(3);
    edit.setTextCursor(cursor);
    keyPress(edit, Qt::Key_Delete);
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d\u658713"));
    edit.setTextDirection(direction == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft);
    edit.undo();
    CHECK(edit.toPlainText() == input);
    edit.redo();
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d\u658713"));
    edit.undo();
    edit.selectAll();
    edit.copy();
    CHECK(QApplication::clipboard()->text() == input);
    keyPress(edit, Qt::Key_X, Qt::ControlModifier);
    CHECK(edit.toPlainText().isEmpty());
    edit.paste();
    CHECK(edit.toPlainText() == input);
    cursor = QTextCursor(edit.document());
    cursor.setPosition(1);
    edit.setTextCursor(cursor);
    CHECK(edit.inputMethodQuery(Qt::ImSurroundingText).toString() == input);
    QInputMethodEvent preedit(QStringLiteral("\u6587"), {});
    QApplication::sendEvent(&edit, &preedit);
    CHECK(edit.toPlainText() == input);
    QInputMethodEvent commit;
    commit.setCommitString(QStringLiteral("\u6587"));
    QApplication::sendEvent(&edit, &commit);
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d\u6587\u6587123"));
    edit.undo();
    CHECK(edit.toPlainText() == input);
    edit.setPlainText(input);
    edit.setTextDirection(direction);
    QApplication::processEvents();
    cursor = QTextCursor(edit.document());
    edit.setTextCursor(cursor);
    const int before = edit.cursorRect().x();
    keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right);
    CHECK(edit.textCursor().position() == 1);
    CHECK((edit.cursorRect().x() < before) == (direction == Qt::RightToLeft));
  }
}

TEST_CASE("Chinese editor navigation follows visual runs without exposing controls",
          "[text][bidi][chinese][gui]") {
  const auto input = QStringLiteral("\u4e2d\u6587123");
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    LC_TextEdit edit;
    edit.resize(600, 160);
    edit.setPlainText(input);
    edit.setTextDirection(direction);
    edit.show();
    edit.setFocus();
    QApplication::processEvents();
    edit.setTextCursor(QTextCursor(edit.document()));
    const std::vector<int> path = direction == Qt::RightToLeft
                                   ? std::vector<int>{0, 1, 2, 5, 4, 3, 2}
                                   : std::vector<int>{0, 1, 2, 3, 4, 5};
    int previous = edit.cursorRect().x();
    for (int i = 1; i < int(path.size()); ++i) {
      keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right);
      CHECK(edit.textCursor().position() == path[i]);
      const int x = edit.cursorRect().x();
      CHECK((direction == Qt::RightToLeft ? x <= previous : x >= previous));
      previous = x;
    }
    for (int i = int(path.size()) - 2; i >= 0; --i) {
      keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Right : Qt::Key_Left);
      CHECK(edit.textCursor().position() == path[i]);
    }
    for (const auto command : {QKeySequence::MoveToEndOfLine, QKeySequence::MoveToStartOfLine}) {
      const auto binding = QKeySequence::keyBindings(command).first()[0];
      keyPress(edit, binding.key(), binding.keyboardModifiers());
      CHECK(edit.textCursor().position() == (command == QKeySequence::MoveToEndOfLine ? input.size() : 0));
    }
    edit.setTextCursor(QTextCursor(edit.document()));
    for (int i = 0; i < 2; ++i)
      keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right, Qt::ShiftModifier);
    CHECK(edit.textCursor().selectedText() == input.left(2));
    edit.copy();
    CHECK(QApplication::clipboard()->text() == input.left(2));
    keyPress(edit, Qt::Key_Left);
    CHECK_FALSE(edit.textCursor().hasSelection());
    CHECK(edit.textCursor().position() == (direction == Qt::RightToLeft ? 2 : 0));
    if (direction == Qt::RightToLeft) {
      auto digit = QTextCursor(edit.document());
      digit.setPosition(3);
      CHECK(edit.cursorRect().x() > edit.cursorRect(digit).x());
    }
    edit.setTextCursor(QTextCursor(edit.document()));
    keyPress(edit, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("\u56fd"));
    CHECK(edit.toPlainText() == QStringLiteral("\u56fd") + input);
    edit.undo();
    CHECK(edit.toPlainText() == input);
    const char32_t supplementary[] = {0x20000, 0xe0100};
    edit.setPlainText(QString::fromUcs4(supplementary, 2) + input);
    edit.setTextCursor(QTextCursor(edit.document()));
    keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right);
    CHECK(edit.textCursor().position() == 4);
  }
}

TEST_CASE("Chinese editor hit testing selection and drop use logical indices",
          "[text][bidi][chinese][gui]") {
  struct Editor : LC_TextEdit { using LC_TextEdit::dropEvent; };
  const auto input = QStringLiteral("\u4e2d\u6587123");
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    Editor edit;
    edit.resize(600, 160);
    edit.setPlainText(input);
    edit.setTextDirection(direction);
    edit.show();
    QApplication::processEvents();
    for (const int position : {1, 3, 4}) {
      auto cursor = QTextCursor(edit.document());
      cursor.setPosition(position);
      const auto point = edit.cursorRect(cursor).center();
      CHECK(edit.cursorForPosition(point).position() == position);
      CHECK(edit.inputMethodQuery(Qt::ImCursorPosition, QPointF(point)).toInt() == position);
      CHECK(edit.inputMethodQuery(Qt::ImAbsolutePosition, QPointF(point)).toInt() == position);
      mouseEvent(edit, QEvent::MouseButtonPress, point);
      mouseEvent(edit, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
      CHECK(edit.textCursor().position() == position);
    }
    auto first = QTextCursor(edit.document());
    first.setPosition(3);
    auto last = first;
    last.setPosition(4);
    const auto start = edit.cursorRect(first).center();
    const auto end = edit.cursorRect(last).center();
    mouseEvent(edit, QEvent::MouseButtonPress, start);
    mouseEvent(edit, QEvent::MouseMove, end, Qt::NoButton);
    mouseEvent(edit, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    CHECK(edit.textCursor().selectedText() == "2");
    mouseEvent(edit, QEvent::MouseButtonDblClick, (start + end) / 2);
    mouseEvent(edit, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    auto word = QTextCursor(edit.document());
    word.setPosition(3);
    word.select(QTextCursor::WordUnderCursor);
    CHECK(edit.textCursor().selectedText() == word.selectedText());
    edit.setTextCursor(first);
    QMimeData mime;
    mime.setText("4");
    QDropEvent drop(start, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    edit.dropEvent(&drop);
    CHECK(drop.isAccepted());
    CHECK(edit.toPlainText() == QStringLiteral("\u4e2d\u65871423"));
    edit.undo();
    CHECK(edit.toPlainText() == input);
  }
}

TEST_CASE("Chinese editor pixels match Qt layout without storing generated controls",
          "[text][bidi][chinese][gui]") {
  const auto input = QStringLiteral("\u4e2d\u6587 123\n\u4e2d\u6587 12.5 CAD123");
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    LC_TextEdit edit;
    QTextEdit reference;
    edit.resize(600, 160);
    reference.resize(600, 160);
    edit.setReadOnly(true);
    reference.setReadOnly(true);
    edit.setPlainText(input);
    edit.setTextDirection(direction);
    const auto display = lc::textbidi::directionalText(input, direction);
    reference.setPlainText(display.text);
    reference.setLayoutDirection(direction);
    reference.document()->setDefaultTextOption(edit.document()->defaultTextOption());
    edit.show();
    reference.show();
    QApplication::processEvents();
    edit.clearFocus();
    reference.clearFocus();
    QApplication::processEvents();
    CHECK(edit.viewport()->grab().toImage() == reference.viewport()->grab().toImage());
    auto selection = QTextCursor(edit.document());
    selection.setPosition(0);
    selection.setPosition(2, QTextCursor::KeepAnchor);
    edit.setTextCursor(selection);
    auto projectedSelection = QTextCursor(reference.document());
    projectedSelection.setPosition(display.displayPositions[0]);
    projectedSelection.setPosition(display.displayPositions[2], QTextCursor::KeepAnchor);
    reference.setTextCursor(projectedSelection);
    QApplication::processEvents();
    CHECK(edit.viewport()->grab().toImage() == reference.viewport()->grab().toImage());
    CHECK(edit.toPlainText() == input);
    edit.selectAll();
    reference.selectAll();
    QApplication::processEvents();
    CHECK(edit.viewport()->grab().toImage() == reference.viewport()->grab().toImage());
    edit.zoomIn(3);
    reference.zoomIn(3);
    QApplication::processEvents();
    CAPTURE(direction);
    // Keep native paragraph heights even if controls change fallback-font metrics.
    const QRect firstLine(0, 0, edit.viewport()->width(),
                          int(edit.document()->documentMargin() + edit.document()->begin().layout()->boundingRect().height()));
    CHECK(edit.viewport()->grab(firstLine).toImage() == reference.viewport()->grab(firstLine).toImage());
  }
}

TEST_CASE("Chinese editor wrapping scrolling and composition preserve the document",
          "[text][bidi][chinese][gui]") {
  const auto input = QStringLiteral("\u4e2d\u6587123");
  for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
    LC_TextEdit edit;
    edit.resize(100, 300);
    edit.setPlainText(input.repeated(8) + "\n" + input);
    edit.setTextDirection(direction);
    edit.show();
    edit.setFocus();
    QApplication::processEvents();
    const auto block = edit.document()->begin();
    REQUIRE(block.layout()->lineCount() > 1);
    for (int position = 0; position < block.length() - 1; ++position) {
      auto cursor = QTextCursor(block);
      cursor.setPosition(position);
      const auto nativeLine = block.layout()->lineForTextPosition(position);
      CHECK(edit.cursorRect(cursor).y() == qRound(edit.document()->documentMargin() + nativeLine.y()));
    }
    edit.setTextCursor(QTextCursor(edit.document()));
    const auto top = edit.cursorRect();
    keyPress(edit, Qt::Key_Down);
    CHECK(edit.cursorRect().y() > top.y());
    keyPress(edit, Qt::Key_Up);
    CHECK(edit.textCursor().position() == 0);
    std::vector<bool> reached(8 * input.size() + 1, false);
    for (int step = 0; step < 300 && edit.textCursor().blockNumber() == 0; ++step) {
      reached.at(edit.textCursor().position()) = true;
      keyPress(edit, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right);
    }
    for (int position = 0; position < int(reached.size()) - 1; ++position) {
      CAPTURE(direction, position);
      CHECK(reached[position]);
    }
    edit.setPlainText(input);
    auto cursor = QTextCursor(edit.document());
    cursor.setPosition(1);
    edit.setTextCursor(cursor);
    QTextCharFormat format;
    format.setFontUnderline(true);
    QList<QInputMethodEvent::Attribute> attributes{
      {QInputMethodEvent::TextFormat, 0, 2, format},
      {QInputMethodEvent::Cursor, 1, 1, {}}
    };
    QInputMethodEvent preedit(QStringLiteral("\u6587\u56fd"), attributes);
    QApplication::sendEvent(&edit, &preedit);
    CHECK(edit.toPlainText() == input);
    CHECK(edit.inputMethodQuery(Qt::ImCursorPosition).toInt() == 1);
    CHECK(edit.inputMethodQuery(Qt::ImCursorRectangle).toRect() == edit.cursorRect());
    const auto composition = edit.document()->begin().layout()->preeditAreaText();
    edit.setTextDirection(direction == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft);
    CHECK(edit.document()->begin().layout()->preeditAreaText() == composition);
    QInputMethodEvent cancel;
    QApplication::sendEvent(&edit, &cancel);
    CHECK(edit.toPlainText() == input);
    QInputMethodEvent replacement;
    replacement.setCommitString(QStringLiteral("\u56fd"), -1, 1);
    QApplication::sendEvent(&edit, &replacement);
    CHECK(edit.toPlainText() == QStringLiteral("\u56fd\u6587123"));
    edit.undo();
    CHECK(edit.toPlainText() == input);
    edit.setTextDirection(direction);
    edit.clear();
    QInputMethodEvent firstPreedit(input, {{QInputMethodEvent::Cursor, 0, 1, {}}});
    QApplication::sendEvent(&edit, &firstPreedit);
    const int preeditStart = edit.cursorRect().x();
    QInputMethodEvent secondPreedit(input, {{QInputMethodEvent::Cursor, 1, 1, {}}});
    QApplication::sendEvent(&edit, &secondPreedit);
    CHECK((edit.cursorRect().x() < preeditStart) == (direction == Qt::RightToLeft));
    CHECK(edit.toPlainText().isEmpty());
    QApplication::sendEvent(&edit, &cancel);
    {
      const QSignalBlocker blocker(&edit);
      edit.setPlainText(input + "45");
    }
    CHECK(edit.cursorForPosition(edit.cursorRect(cursor).center()).position() == cursor.position());
    LC_SingleLineTextEdit single;
    single.resize(100, single.sizeHint().height());
    single.setText(input.repeated(30));
    single.setTextDirection(direction);
    single.show();
    QApplication::processEvents();
    auto caret = QTextCursor(single.document());
    for (const int position : {0, 100, 25, 149}) {
      caret.setPosition(position);
      single.setTextCursor(caret);
      QApplication::processEvents();
      CAPTURE(direction, position, single.horizontalScrollBar()->value(), single.horizontalScrollBar()->maximum());
      CHECK(single.cursorRect().left() >= 0);
      CHECK(single.cursorRect().right() < single.viewport()->width());
    }
    single.setText(input);
    QInputMethodEvent multiline;
    multiline.setCommitString(QStringLiteral("\u56fd\n\u6587"));
    QApplication::sendEvent(&single, &multiline);
    CHECK(single.document()->blockCount() == 1);
    CHECK(single.text() == QStringLiteral("\u56fd \u6587") + input);
    single.undo();
    CHECK(single.text() == input);
  }
}

TEST_CASE("MTEXT RTL layout marker round trips without rewriting text",
          "[dxf][dwg][bidi][issue1859]") {
  SourceFonts fonts;
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  for (const auto format : {RS2::FormatDXFRW, RS2::FormatDWG2004}) {
    for (const bool legacy : {false, true}) {
      INFO("format " << format << ", legacy " << legacy);
      RS_Graphic original;
      auto data = mtextData("123");
      data.legacyRtlLayout = legacy;
      auto *source = new RS_MText(&original, data);
      source->setDrwExtData(
          {std::make_shared<DRW_Variant>(1001, std::string("LibreCad")),
           std::make_shared<DRW_Variant>(1071, std::int32_t{1}),
           std::make_shared<DRW_Variant>(1071, std::int32_t{2}),
           std::make_shared<DRW_Variant>(1071, std::int32_t{99})});
      original.addEntity(source);
      const QString path =
          directory.filePath(QString(legacy ? "legacy" : "logical") +
                             (format == RS2::FormatDXFRW ? ".dxf" : ".dwg"));
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(original, path, format));
      RS_Graphic imported;
      RS_FilterDXFRW reader;
      REQUIRE(reader.fileImport(imported, path, format));
      auto *entity = findMText(imported);
      REQUIRE(entity);
      CHECK(entity->getText() == "123");
      CHECK(entity->getDrawingDirection() == RS_MTextData::RightToLeft);
      CHECK(entity->getData().legacyRtlLayout == legacy);
      CHECK(libreCadIntegers(*entity) ==
            std::vector<std::int32_t>{99, legacy ? 1 : 2});
      auto *line = dynamic_cast<RS_EntityContainer *>(entity->entityAt(0));
      REQUIRE(line);
      CHECK(glyphNames(*line) == (legacy ? "321" : "123"));

      entity->setDrawingDirection(RS_MTextData::LeftToRight);
      REQUIRE(writer.fileExport(imported, path, format));
      RS_Graphic ltr;
      REQUIRE(reader.fileImport(ltr, path, format));
      auto *ltrEntity = findMText(ltr);
      REQUIRE(ltrEntity);
      CHECK(ltrEntity->getDrawingDirection() == RS_MTextData::LeftToRight);
      CHECK_FALSE(ltrEntity->getData().legacyRtlLayout);
      CHECK(ltrEntity->getText() == "123");
      CHECK(libreCadIntegers(*ltrEntity) == std::vector<std::int32_t>{99});

      // Explicitly choosing RTL again upgrades legacy layout without text
      // edits.
      ltrEntity->setDrawingDirection(RS_MTextData::RightToLeft);
      REQUIRE(writer.fileExport(ltr, path, format));
      RS_Graphic upgraded;
      REQUIRE(reader.fileImport(upgraded, path, format));
      auto *logicalEntity = findMText(upgraded);
      REQUIRE(logicalEntity);
      CHECK(logicalEntity->getDrawingDirection() == RS_MTextData::RightToLeft);
      CHECK_FALSE(logicalEntity->getData().legacyRtlLayout);
      CHECK(logicalEntity->getText() == "123");
      CHECK(libreCadIntegers(*logicalEntity) ==
            std::vector<std::int32_t>{99, 2});
      line = dynamic_cast<RS_EntityContainer *>(logicalEntity->entityAt(0));
      REQUIRE(line);
      CHECK(glyphNames(*line) == "123");
    }
  }
}

TEST_CASE("Chinese RTL MTEXT round trips logical text and display policy",
          "[dxf][dwg][bidi][chinese]") {
  SourceFonts fonts;
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  const auto input = QStringLiteral("\u4e2d\u6587123");
  for (const auto format : {RS2::FormatDXFRW, RS2::FormatDWG2004}) {
    INFO("format " << format);
    RS_Graphic original;
    auto data = mtextData(input);
    data.style = "unicode";
    original.addEntity(new RS_MText(&original, data));
    const auto path = directory.filePath(format == RS2::FormatDXFRW ? "chinese.dxf" : "chinese.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(original, path, format));
    RS_Graphic imported;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(imported, path, format));
    auto *entity = findMText(imported);
    REQUIRE(entity);
    CHECK(entity->getText() == input);
    CHECK_FALSE(entity->getData().legacyRtlLayout);
    CHECK(libreCadIntegers(*entity) == std::vector<std::int32_t>{2});
    // Keep this layout check independent of the file's STYLE resolution.
    entity->setStyle("unicode");
    entity->update();
    auto *line = dynamic_cast<RS_EntityContainer *>(entity->entityAt(0));
    REQUIRE(line);
    CHECK(glyphNames(*line) == QStringLiteral("123\u6587\u4e2d"));
  }
}
