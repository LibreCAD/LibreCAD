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
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <QApplication>
#include <QFile>
#include <QClipboard>
#include <QDropEvent>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextLayout>

#include "lc_action_draw_mtext.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_m_text_options_widget.h"
#include "lc_mtextpropertieseditingwidget.h"
#include "lc_multilinetexteditdialog.h"
#include "lc_textbidi.h"
#include "lc_textedit.h"
#include "lc_textpropertieseditingwidget.h"
#include "lc_tolerance.h"
#include "qg_dlg_mtext.h"
#include "qg_dlg_text.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
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

TEST_CASE("MText empty lines retain invalid bounds through alignment and transforms",
          "[text][bidi][empty-bounds]") {
  SourceFonts fonts;
  REQUIRE(RS_FONTLIST->requestFont("iso3098"));
  for (const auto &value : {QString(), QStringLiteral(" \t\n\t"), QString(QChar(0x2066))}) {
    for (const auto horizontal : {RS_MTextData::HALeft, RS_MTextData::HACenter, RS_MTextData::HARight}) {
      for (const auto vertical : {RS_MTextData::VATop, RS_MTextData::VAMiddle, RS_MTextData::VABottom}) {
        auto data = mtextData(value);
        data.halign = horizontal;
        data.valign = vertical;
        data.angle = 0.5;
        RS_MText text(nullptr, data);
        CHECK_FALSE(text.getMin().valid);
        CHECK_FALSE(text.getMax().valid);
        CHECK(text.getUsedTextWidth() == 0.0);
        CHECK(std::isfinite(text.getUsedTextHeight()));
        text.rotate({0.0, 0.0}, 0.5);
        text.scale({0.0, 0.0}, {2.0, 2.0});
        CHECK_FALSE(text.getMin().valid);
        CHECK_FALSE(text.getMax().valid);
        CHECK(text.getCanonicalText() == value);
      }
    }
  }
}

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

TEST_CASE("Chinese editor cursor rectangle survives an input method query with no cursor",
          "[text][bidi][chinese][gui][regression]") {
  const auto input = QStringLiteral("\u4e2d\u6587123");
  const auto checkEditor = [&](LC_TextEdit &edit) {
    edit.resize(600, 160);
    edit.show();
    int queries = 0;
    // Input methods can query the transient cursor during setPlainText().
    const auto connection = QObject::connect(edit.document(), &QTextDocument::contentsChanged, &edit, [&] {
      ++queries;
      auto cursor = edit.textCursor();
      CHECK(edit.inputMethodQuery(Qt::ImCursorRectangle).toRect() == edit.cursorRect(cursor));
      cursor.setPosition(cursor.anchor());
      CHECK(edit.inputMethodQuery(Qt::ImAnchorRectangle).toRect() == edit.cursorRect(cursor));
    });
    for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
      CAPTURE(direction);
      edit.setPlainText(input);
      edit.setTextDirection(direction);
      QApplication::processEvents();
      CHECK(edit.cursorRect().isValid());
      CHECK(edit.cursorRect(QTextCursor()).isNull());
      for (const auto &replacement : {QStringLiteral("\u4e2d\u6587456"), QString(), input}) {
        CAPTURE(replacement);
        queries = 0;
        edit.setPlainText(replacement);
        CHECK(queries > 0);
        CHECK(edit.toPlainText() == replacement);
        CHECK(edit.cursorRect().isValid());
      }
    }
    QObject::disconnect(connection);
  };
  LC_TextEdit multiline;
  checkEditor(multiline);
  LC_SingleLineTextEdit singleLine;
  checkEditor(singleLine);
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

// ---------------------------------------------------------------------------
// Issue #3032: MText indented by leading spaces and tabs; paragraph commands.
// Positions are compared to the same text without the indentation, or derived
// from the spacing of the font, so that no glyph outline is copied in here.

namespace {
using Catch::Matchers::WithinAbs;

constexpr double inkTolerance = 1e-9;

/** The ink of one glyph, or of one part of stacked text. */
struct InkSpan {
  QString name;
  double left = 0.0;
  double right = 0.0;
  double bottom = 0.0;
};

/** Left-attached text that flows from the left, in a font without left bearings. */
RS_MTextData leftToRightData(const QString &text,
                             const QString &font = QStringLiteral("standard"),
                             double height = 9.0) {
  auto data = mtextData(text);
  data.drawingDirection = RS_MTextData::LeftToRight;
  data.style = font;
  data.height = height;
  return data;
}

RS_EntityContainer *lineOf(const RS_MText &text, int index) {
  return dynamic_cast<RS_EntityContainer *>(text.entityAt(index));
}

/** What one line draws, in drawing order. */
std::vector<InkSpan> inkOf(const RS_MText &text, int index = 0) {
  std::vector<InkSpan> spans;
  auto *line = lineOf(text, index);
  REQUIRE(line);
  for (auto *entity : line->getEntityList()) {
    const QString name = entity->rtti() == RS2::EntityInsert
        ? static_cast<RS_Insert *>(entity)->getName()
        : QStringLiteral("<stack>");
    spans.push_back({name, entity->getMin().x, entity->getMax().x, entity->getMin().y});
  }
  return spans;
}

QString drawnBy(const RS_MText &text, int index = 0) {
  QString drawn;
  for (const auto &span : inkOf(text, index))
    drawn += span.name;
  return drawn;
}

/** Checks that a line draws what a line of @p control draws, @p dx further right. */
void checkSameInk(const RS_MText &text, const RS_MText &control, double dx = 0.0,
                  int index = 0) {
  const auto ink = inkOf(text, index);
  const auto expected = inkOf(control, index);
  REQUIRE(ink.size() == expected.size());
  for (std::size_t i = 0; i < ink.size(); ++i) {
    INFO("glyph " << i << " '" << ink[i].name.toStdString() << "' of line " << index);
    CHECK(ink[i].name == expected[i].name);
    CHECK_THAT(ink[i].left, WithinAbs(expected[i].left + dx, inkTolerance));
    CHECK_THAT(ink[i].right, WithinAbs(expected[i].right + dx, inkTolerance));
    CHECK_THAT(ink[i].bottom, WithinAbs(expected[i].bottom, inkTolerance));
  }
}

double wordSpacing(const QString &font, double height) {
  auto *loaded = RS_FONTLIST->requestFont(font);
  REQUIRE(loaded);
  return loaded->getWordSpacing() * height / 9.0;
}

double letterSpacing(const QString &font, double height) {
  auto *loaded = RS_FONTLIST->requestFont(font);
  REQUIRE(loaded);
  return loaded->getLetterSpacing() * height / 9.0;
}

/** The value of a group of the first MTEXT record of an ASCII DXF file, as written. */
QString mtextGroup(const QString &path, int group) {
  QFile file(path);
  REQUIRE(file.open(QIODevice::ReadOnly));
  const QList<QByteArray> lines = file.readAll().split('\n');
  bool inMText = false;
  QString value;
  for (qsizetype i = 0; i + 1 < lines.size(); i += 2) {
    QByteArray line = lines[i + 1];
    if (line.endsWith('\r'))
      line.chop(1);
    const int code = lines[i].trimmed().toInt();
    if (code == 0) {
      if (inMText)
        break;
      inMText = line.trimmed() == "MTEXT";
    } else if (inMText && code == group) {
      value += QString::fromUtf8(line);
    }
  }
  return value;
}
} // namespace

TEST_CASE("MText leading spaces indent a left-attached line",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  for (const char *font : {"standard", "romans"}) {
    for (const double height : {9.0, 2.5}) {
      const double space = wordSpacing(font, height);
      RS_MText control(nullptr, leftToRightData("ABC", font, height));
      CHECK_THAT(control.getMin().x, WithinAbs(0.0, inkTolerance));
      for (const int count : {1, 4}) {
        INFO(font << ", height " << height << ", " << count << " spaces");
        const QString content = QString(count, QLatin1Char(' ')) + "ABC";
        RS_MText indented(nullptr, leftToRightData(content, font, height));
        checkSameInk(indented, control, count * space);
        CHECK_THAT(indented.getMin().x, WithinAbs(count * space, inkTolerance));
        // The width counts from the insertion point: it is what a file declares.
        CHECK_THAT(indented.getUsedTextWidth(),
                   WithinAbs(control.getUsedTextWidth() + count * space, inkTolerance));
        CHECK(indented.getUsedTextHeight() == control.getUsedTextHeight());
        CHECK(indented.getText() == content);
      }
    }
  }

  // Wherever the text is attached vertically, a left-attached line is indented.
  const double space = wordSpacing("standard", 9.0);
  for (const auto valign : {RS_MTextData::VATop, RS_MTextData::VAMiddle, RS_MTextData::VABottom}) {
    auto data = leftToRightData("ABC");
    data.valign = valign;
    RS_MText control(nullptr, data);
    data.text = "    ABC";
    RS_MText indented(nullptr, data);
    checkSameInk(indented, control, 4 * space);
  }

  // ByStyle and TopToBottom are laid out from the left as well.
  for (const auto direction : {RS_MTextData::ByStyle, RS_MTextData::TopToBottom}) {
    auto data = leftToRightData("ABC");
    data.drawingDirection = direction;
    RS_MText control(nullptr, data);
    data.text = "    ABC";
    RS_MText indented(nullptr, data);
    checkSameInk(indented, control, 4 * space);
  }

  // The indentation runs along the text: with the text turned upwards it is
  // a distance upwards.
  auto turned = leftToRightData("ABC");
  turned.angle = M_PI_2;
  RS_MText control(nullptr, turned);
  turned.text = "    ABC";
  RS_MText indented(nullptr, turned);
  CHECK_THAT(indented.getMin().x, WithinAbs(control.getMin().x, 1e-6));
  CHECK_THAT(indented.getMax().x, WithinAbs(control.getMax().x, 1e-6));
  CHECK_THAT(indented.getMin().y, WithinAbs(control.getMin().y + 4 * space, 1e-6));
  CHECK_THAT(indented.getMax().y, WithinAbs(control.getMax().y + 4 * space, 1e-6));

  // Mirrored through its insertion point the text has a negative height. The
  // indentation stays a distance from the edge the text is attached to.
  RS_MText mirrored(nullptr, leftToRightData("    AB"));
  RS_MText mirroredControl(nullptr, leftToRightData("AB"));
  mirrored.scale(RS_Vector(0.0, 0.0), RS_Vector(-1.0, -1.0));
  mirroredControl.scale(RS_Vector(0.0, 0.0), RS_Vector(-1.0, -1.0));
  CHECK_THAT(mirrored.getMin().x, WithinAbs(mirroredControl.getMin().x + 4 * space, inkTolerance));
  CHECK_THAT(mirrored.getUsedTextWidth(),
             WithinAbs(mirroredControl.getUsedTextWidth() + 4 * space, inkTolerance));
}

TEST_CASE("MText indentation belongs to one line and to its leading blanks",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  const double space = wordSpacing("standard", 9.0);
  RS_MText control(nullptr, leftToRightData("ABC\nDEF"));
  for (const QString &lineBreak : {QStringLiteral("\n"), QStringLiteral("\\P")}) {
    RS_MText text(nullptr, leftToRightData("ABC" + lineBreak + "    DEF"));
    REQUIRE(text.count() == 2);
    checkSameInk(text, control, 0.0, 0);
    checkSameInk(text, control, 4 * space, 1);
  }

  RS_MText plain(nullptr, leftToRightData("ABC"));
  RS_MText trailing(nullptr, leftToRightData("ABC    "));
  checkSameInk(trailing, plain);
  CHECK_THAT(trailing.getUsedTextWidth(), WithinAbs(plain.getUsedTextWidth(), inkTolerance));

  RS_MText words(nullptr, leftToRightData("A B"));
  RS_MText wider(nullptr, leftToRightData("  A   B"));
  REQUIRE(inkOf(words).size() == 2);
  REQUIRE(inkOf(wider).size() == 2);
  CHECK_THAT(inkOf(wider)[0].left, WithinAbs(inkOf(words)[0].left + 2 * space, inkTolerance));
  CHECK_THAT(inkOf(wider)[1].left, WithinAbs(inkOf(words)[1].left + 4 * space, inkTolerance));

  // A font switch and a bidi control draw nothing: the blanks still lead the line.
  for (const QString &prefix : {QStringLiteral("\\f{standard}"), QString(QChar(0x200E))}) {
    RS_MText text(nullptr, leftToRightData(prefix + "  ABC"));
    checkSameInk(text, plain, 2 * space);
  }

  // A stack without ink moves the pen, but it is no indentation.
  RS_MText letter(nullptr, leftToRightData("A"));
  RS_MText emptyStack(nullptr, leftToRightData("\\S^;A"));
  checkSameInk(emptyStack, letter);
  CHECK_THAT(emptyStack.getUsedTextWidth(), WithinAbs(letter.getUsedTextWidth(), inkTolerance));

  // A left-to-right line that holds right-to-left text is indented as well.
  const auto hebrew = QString::fromUtf8(u8"\u05d0\u05d1\u05d2");
  const double unicodeSpace = wordSpacing("unicode", 9.0);
  RS_MText mixed(nullptr, leftToRightData(hebrew, "unicode"));
  RS_MText mixedIndented(nullptr, leftToRightData("    " + hebrew, "unicode"));
  checkSameInk(mixedIndented, mixed, 4 * unicodeSpace);

  // What counts is where the blanks are drawn. A right-to-left mark behind the
  // blanks makes them part of the Hebrew run, which puts them at its far end:
  // the left one.
  RS_MText marked(nullptr, leftToRightData(hebrew + "  " + QChar(0x200F) + "A", "unicode"));
  CHECK_THAT(marked.getMin().x, WithinAbs(2 * unicodeSpace, inkTolerance));
}

TEST_CASE("MText indentation is measured to the ink of the line",
          "[text][mtext][issue3032][layout]") {
  // The letters of iso3098 have a left bearing. An unindented line starts with
  // its ink on the insertion point; an indented one starts the indentation on.
  // In "iBC" and "1.A" the ink that is furthest left is not that of the first
  // letter.
  SourceFonts fonts;
  const double space = wordSpacing("iso3098", 9.0);
  for (const QString &letters : {QStringLiteral("ABC"), QStringLiteral("iBC"), QStringLiteral("1.A")}) {
    INFO(letters.toStdString());
    RS_MText control(nullptr, leftToRightData(letters, "iso3098"));
    CHECK_THAT(control.getMin().x, WithinAbs(0.0, inkTolerance));
    RS_MText indented(nullptr, leftToRightData("    " + letters, "iso3098"));
    checkSameInk(indented, control, 4 * space);
    CHECK_THAT(indented.getMin().x, WithinAbs(4 * space, inkTolerance));

    // Behind a tab the same letters are laid out the same way, on the stop,
    // whether the tab leads the line or follows other text.
    RS_MText leading(nullptr, leftToRightData("\t" + letters, "iso3098"));
    checkSameInk(leading, control, 36.0);
    RS_MText following(nullptr, leftToRightData("X\t" + letters, "iso3098"));
    const auto ink = inkOf(following);
    const auto expected = inkOf(control);
    REQUIRE(ink.size() == expected.size() + 1);
    for (std::size_t i = 0; i < expected.size(); ++i) {
      CHECK_THAT(ink[i + 1].left, WithinAbs(expected[i].left + 36.0, inkTolerance));
      CHECK_THAT(ink[i + 1].right, WithinAbs(expected[i].right + 36.0, inkTolerance));
    }
  }
}

TEST_CASE("MText blanks leave centered and right-to-left lines alone",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  for (const auto halign : {RS_MTextData::HACenter, RS_MTextData::HARight}) {
    for (const auto valign : {RS_MTextData::VATop, RS_MTextData::VAMiddle, RS_MTextData::VABottom}) {
      INFO("halign " << halign << ", valign " << valign);
      auto data = leftToRightData("ABC");
      data.halign = halign;
      data.valign = valign;
      RS_MText plain(nullptr, data);
      for (const QString &content : {QStringLiteral("    ABC  "), QStringLiteral("\tABC"),
                                     QStringLiteral(" \t ABC")}) {
        data.text = content;
        RS_MText blanks(nullptr, data);
        checkSameInk(blanks, plain);
        CHECK_THAT(blanks.getUsedTextWidth(), WithinAbs(plain.getUsedTextWidth(), inkTolerance));
      }
    }
  }

  // In a right-to-left line the blanks on the left are the trailing ones.
  const auto hebrew = QString::fromUtf8(u8"\u05e9\u05dc\u05d5\u05dd");
  for (const bool legacy : {false, true}) {
    for (const QString &letters : {QStringLiteral("ABC"), hebrew, QString(hebrew + " 12")}) {
      auto data = mtextData(letters);
      data.style = "unicode";
      data.legacyRtlLayout = legacy;
      RS_MText plain(nullptr, data);
      for (const QString &content : {QString("    " + letters), QString(letters + "    "),
                                     QString("  " + letters + "  ")}) {
        INFO("legacy " << legacy << ": '" << content.toStdString() << "'");
        data.text = content;
        RS_MText blanks(nullptr, data);
        checkSameInk(blanks, plain);
        CHECK_THAT(blanks.getUsedTextWidth(), WithinAbs(plain.getUsedTextWidth(), inkTolerance));
      }
    }
  }
}

TEST_CASE("MText lines of blanks keep their pitch and have no ink",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  RS_MText empty(nullptr, leftToRightData("ABC\n\nDEF"));
  REQUIRE(empty.count() == 3);
  for (const QString &blank : {QString(), QStringLiteral("    "), QStringLiteral("\t"),
                               QStringLiteral(" \t "), QStringLiteral("\\pi2;")}) {
    INFO("'" << blank.toStdString() << "'");
    RS_MText alone(nullptr, leftToRightData(blank));
    REQUIRE(alone.count() == 1);
    CHECK(lineOf(alone, 0)->count() == 0);
    CHECK_FALSE(alone.hasValidBorders());
    CHECK(alone.getUsedTextWidth() == 0.0);

    RS_MText between(nullptr, leftToRightData("ABC\n" + blank + "\nDEF"));
    REQUIRE(between.count() == 3);
    CHECK_FALSE(lineOf(between, 1)->hasValidBorders());
    checkSameInk(between, empty, 0.0, 0);
    checkSameInk(between, empty, 0.0, 2);
    CHECK(between.getMin().distanceTo(empty.getMin()) < inkTolerance);
    CHECK(between.getMax().distanceTo(empty.getMax()) < inkTolerance);
    CHECK_THAT(between.getUsedTextWidth(), WithinAbs(empty.getUsedTextWidth(), inkTolerance));
    CHECK(between.getUsedTextHeight() == empty.getUsedTextHeight());
  }
}

TEST_CASE("MText tabs advance to stops four text heights apart",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  for (const double height : {9.0, 2.5}) {
    const double stop = 4.0 * height;
    const double space = wordSpacing("standard", height);
    const double letter = letterSpacing("standard", height);
    struct Case {
      QString text;
      double expected; // where the last letter starts
    };
    const Case cases[] = {
        {"A\tB", stop},
        {"\tB", stop},
        {"A\t\tB", 2 * stop},
        {"\t\tB", 2 * stop},
        {"A\t  B", stop + 2 * space},
        {"  A\tB", stop},
        {"   A\tB", stop},
        {"ABCDEFGH\tX", 2 * stop},
        {"A\tB\tC", 2 * stop},
        {"AB\tC", stop},
    };
    for (const auto &item : cases) {
      INFO("height " << height << ": '" << item.text.toStdString() << "'");
      RS_MText text(nullptr, leftToRightData(item.text, "standard", height));
      CHECK_THAT(inkOf(text).back().left, WithinAbs(item.expected, inkTolerance));
      CHECK(text.getText() == item.text);
      CHECK(glyphNames(*lineOf(text, 0)) == QString(item.text).remove('\t').remove(' '));
      CHECK_THAT(text.getUsedTextWidth(), WithinAbs(inkOf(text).back().right, inkTolerance));
    }

    // Text that ends on a stop goes to the next one; text that ends just short
    // of it does not. The pen is one letter spacing behind the last letter,
    // and the blanks that follow the letter move it on.
    struct Near {
      QString text;
      int blanksBeforeTab;
      double fromStop; // where the pen is, measured from the first stop
      double expected;
    };
    const double quarter = 0.25 * height / 9.0;
    const Near near[] = {
        {"    A\tX", 0, 0.0, 2 * stop},
        {"B CCC\tX", 0, -quarter, stop},
        {"B B  \tX", 2, quarter, 2 * stop},
    };
    for (const auto &item : near) {
      INFO("height " << height << ": '" << item.text.toStdString() << "'");
      RS_MText text(nullptr, leftToRightData(item.text, "standard", height));
      const auto ink = inkOf(text);
      REQUIRE(ink.size() >= 2);
      const double pen = ink[ink.size() - 2].right + letter + item.blanksBeforeTab * space;
      REQUIRE_THAT(pen, WithinAbs(stop + item.fromStop, inkTolerance));
      CHECK_THAT(ink.back().left, WithinAbs(item.expected, inkTolerance));
    }
  }

  // A tab that nothing follows changes neither the ink nor the width.
  RS_MText plain(nullptr, leftToRightData("A"));
  RS_MText trailing(nullptr, leftToRightData("A\t"));
  checkSameInk(trailing, plain);
  CHECK_THAT(trailing.getUsedTextWidth(), WithinAbs(plain.getUsedTextWidth(), inkTolerance));

  // Far from the edge the stops are still exact.
  RS_MText far(nullptr, leftToRightData(QString(1000, QLatin1Char('\t')) + "X"));
  CHECK_THAT(inkOf(far).back().left, WithinAbs(36000.0, 1e-6));

  // Stacked text after a tab starts on the stop with both of its parts, and a
  // stack without ink in front of a letter does not push the letter off it.
  RS_MText stack(nullptr, leftToRightData("A\t\\S1^2;Y"));
  const auto stackInk = inkOf(stack);
  REQUIRE(stackInk.size() == 4);
  CHECK_THAT(stackInk[1].left, WithinAbs(36.0, inkTolerance));
  CHECK_THAT(stackInk[2].left, WithinAbs(36.0, inkTolerance));
  CHECK(stackInk[3].left > std::max(stackInk[1].right, stackInk[2].right));
  RS_MText emptyStack(nullptr, leftToRightData("X\t\\S^;A"));
  CHECK_THAT(inkOf(emptyStack).back().left, WithinAbs(36.0, inkTolerance));

  // A mark stays on its letter when the letter is moved back onto the stop.
  const QString marked = QStringLiteral("e\u0301");
  RS_MText markControl(nullptr, leftToRightData(marked, "unicode"));
  RS_MText markField(nullptr, leftToRightData("W\t\\S^;" + marked, "unicode"));
  const auto markControlInk = inkOf(markControl);
  const auto markFieldInk = inkOf(markField);
  REQUIRE(markControlInk.size() == 2);
  REQUIRE(markFieldInk.size() == 3);
  CHECK_THAT(markFieldInk[1].left, WithinAbs(36.0, inkTolerance));
  CHECK_THAT(markFieldInk[2].left - markFieldInk[1].left,
             WithinAbs(markControlInk[1].left - markControlInk[0].left, inkTolerance));
  CHECK_THAT(markFieldInk[2].right - markFieldInk[1].left,
             WithinAbs(markControlInk[1].right - markControlInk[0].left, inkTolerance));
}

TEST_CASE("MText tab stops are at the same place in every line",
          "[text][mtext][issue3032][layout]") {
  // The lines start with different letters, with blanks or with the tab itself.
  // In iso3098 those letters have different left bearings, and one has no width.
  SourceFonts fonts;
  const double stop = 36.0;
  for (const char *font : {"standard", "iso3098"}) {
    INFO(font);
    RS_MText text(nullptr, leftToRightData(
        "1.\tAlpha\n\tAlpha\nA.\tAlpha\n  i\tAlpha\n.\tAlpha", font));
    REQUIRE(text.count() == 5);
    for (int line = 0; line < 5; ++line) {
      const auto ink = inkOf(text, line);
      REQUIRE(ink.size() >= 5);
      const auto &alpha = ink[ink.size() - 5];
      INFO("line " << line);
      CHECK(alpha.name == "A");
      CHECK_THAT(alpha.left, WithinAbs(stop, inkTolerance));
    }
  }

  // The ink of a line can start with a later letter: in iso3098 the "M"
  // reaches further left than the "i" in front of it. The stops count from
  // where the ink starts.
  RS_MText later(nullptr, leftToRightData("iM\t1", "iso3098"));
  const auto laterInk = inkOf(later);
  REQUIRE(laterInk.size() == 3);
  REQUIRE(laterInk[1].left < laterInk[0].left);
  CHECK_THAT(laterInk[1].left, WithinAbs(0.0, inkTolerance));
  CHECK_THAT(laterInk[2].left, WithinAbs(stop, inkTolerance));
}

TEST_CASE("MText tabs separate fields in every attachment and direction",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  const double stop = 36.0;
  for (const auto halign : {RS_MTextData::HACenter, RS_MTextData::HARight}) {
    INFO("halign " << halign);
    auto data = leftToRightData("AB\tC");
    data.halign = halign;
    RS_MText text(nullptr, data);
    const auto ink = inkOf(text);
    REQUIRE(ink.size() == 3);
    // The stops count from the first ink of such a line.
    CHECK_THAT(ink[2].left - ink[0].left, WithinAbs(stop, inkTolerance));
    if (halign == RS_MTextData::HACenter)
      CHECK_THAT(ink[0].left + ink[2].right, WithinAbs(0.0, inkTolerance));
    else
      CHECK_THAT(ink[2].right, WithinAbs(0.0, inkTolerance));

    // Blanks in front of the line are without effect there, also on its stops.
    for (const QString &blanks : {QStringLiteral(" "), QStringLiteral("    "), QStringLiteral("\t ")}) {
      data.text = blanks + "AB\tC";
      RS_MText indented(nullptr, data);
      checkSameInk(indented, text);
    }
  }

  // A pen that stands on a stop goes on to the next one, also when rounding
  // has left it a hair short of the stop: here the second of the two tabs.
  {
    auto data = leftToRightData("\tl\t\ti! M\tl");
    data.halign = RS_MTextData::HACenter;
    RS_MText text(nullptr, data);
    const auto ink = inkOf(text);
    REQUIRE(ink.size() == 5);
    CHECK_THAT(ink[1].left - ink[0].left, WithinAbs(2 * stop, inkTolerance));
    CHECK_THAT(ink[4].left - ink[0].left, WithinAbs(3 * stop, inkTolerance));
  }

  // A right-to-left line has no stops yet. Its tab is as wide as four word
  // spaces, which is what every release has drawn for the "^I" of a DXF file.
  const double space = wordSpacing("unicode", 9.0);
  const double letter = letterSpacing("unicode", 9.0);
  for (const bool legacy : {false, true}) {
    INFO("legacy " << legacy);
    auto data = mtextData("AB\tC");
    data.style = "unicode";
    data.legacyRtlLayout = legacy;
    RS_MText text(nullptr, data);
    const auto ink = inkOf(text);
    REQUIRE(ink.size() == 3);
    // the field behind the tab is drawn in front of the other
    CHECK(ink[0].name == "C");
    CHECK_THAT(ink[1].left - ink[0].right, WithinAbs(letter + 4 * space, inkTolerance));
    CHECK(text.getText() == "AB\tC");
  }
}

TEST_CASE("Single-line text draws a tab as four word spaces",
          "[text][issue3032][layout]") {
  // A DXF file spells a tab "^I". Every release has read that as four spaces.
  SourceFonts fonts;
  const double space = wordSpacing("standard", 9.0);
  const auto textOf = [](const QString &content) {
    return RS_TextData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0), 9.0, 1.0, RS_TextData::VABaseline,
                       RS_TextData::HALeft, RS_TextData::None, content, "standard", 0.0, RS2::Update);
  };
  const auto inkLeft = [](const RS_Text &text) {
    std::vector<double> lefts;
    for (auto *entity : text.getEntityList())
      if (entity->rtti() == RS2::EntityInsert)
        lefts.push_back(entity->getMin().x);
    return lefts;
  };
  RS_Text tabbed(nullptr, textOf("A\tB"));
  RS_Text spaced(nullptr, textOf("A    B"));
  RS_Text plain(nullptr, textOf("AB"));
  REQUIRE(inkLeft(tabbed).size() == 2);
  CHECK_THAT(inkLeft(tabbed)[1], WithinAbs(inkLeft(spaced)[1], inkTolerance));
  CHECK_THAT(inkLeft(tabbed)[1], WithinAbs(inkLeft(plain)[1] + 4 * space, inkTolerance));
  CHECK(tabbed.getText() == "A\tB");
}

TEST_CASE("MText paragraph commands are not drawn",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  RS_MText plain(nullptr, leftToRightData("ABC"));

  // The commands of real drawings, most of them written by AutoCAD. They are
  // not applied: the line is laid out as if the command were not there.
  for (const QString &command :
       {QStringLiteral("\\pi-12,l12,t12;"), QStringLiteral("\\pt7.1875,8.38542;"),
        QStringLiteral("\\pi102.25;"), QStringLiteral("\\pl0.899488,t8.38542;"),
        QStringLiteral("\\pl0.899488,t7.1875,8.38542;"), QStringLiteral("\\pxt4;"),
        QStringLiteral("\\pq*;"), QStringLiteral("\\pi*;"), QStringLiteral("\\pxql;"),
        QStringLiteral("\\pxqc;"), QStringLiteral("\\pxqr;"), QStringLiteral("\\ptz;"),
        QStringLiteral("\\pl0,tz;"), QStringLiteral("\\pxi0,qj;"), QStringLiteral("\\pi1,q*;"),
        QStringLiteral("\\pxi-3,l4t4;"), QStringLiteral("\\pi*,l*,t;"),
        QStringLiteral("\\pxsm1,ql;"), QStringLiteral("\\pi1,qj;"),
        QStringLiteral("\\pi*,l*,r*,q*,t;"), QStringLiteral("\\pb1,a1.5;"),
        QStringLiteral("\\pr2;"), QStringLiteral("\\pxt1,r5,c8;")}) {
    INFO(command.toStdString());
    RS_MText atStart(nullptr, leftToRightData(command + "ABC"));
    checkSameInk(atStart, plain);
    CHECK_THAT(atStart.getUsedTextWidth(), WithinAbs(plain.getUsedTextWidth(), inkTolerance));
    CHECK(atStart.getText() == command + "ABC");
    RS_MText inside(nullptr, leftToRightData("AB" + command + "C"));
    checkSameInk(inside, plain);
  }

  // A numbered list: its tab advances to the default stop.
  RS_MText list(nullptr, leftToRightData("\\pi-12,l12,t12;1.\tABC\n2.\tDEF"));
  REQUIRE(list.count() == 2);
  for (int line = 0; line < 2; ++line) {
    const auto ink = inkOf(list, line);
    REQUIRE(ink.size() == 5);
    CHECK(drawnBy(list, line) == (line == 0 ? "1.ABC" : "2.DEF"));
    CHECK_THAT(ink[0].left, WithinAbs(0.0, inkTolerance));
    CHECK_THAT(ink[2].left, WithinAbs(36.0, inkTolerance));
  }

  // A line that holds nothing but a command draws nothing.
  RS_MText lines(nullptr, leftToRightData("ABC\n\\pi2;\nABC"));
  REQUIRE(lines.count() == 3);
  CHECK(lineOf(lines, 1)->count() == 0);
  checkSameInk(lines, plain, 0.0, 0);

  // The other attachments and directions hide the command as well.
  auto centered = leftToRightData("ABC");
  centered.halign = RS_MTextData::HACenter;
  RS_MText centeredPlain(nullptr, centered);
  centered.text = "\\pi2,l4;ABC";
  RS_MText centeredCommand(nullptr, centered);
  checkSameInk(centeredCommand, centeredPlain);
  auto rightToLeft = mtextData("ABC");
  rightToLeft.style = "unicode";
  RS_MText rightToLeftPlain(nullptr, rightToLeft);
  rightToLeft.text = "\\pi2,l4;ABC";
  RS_MText rightToLeftCommand(nullptr, rightToLeft);
  checkSameInk(rightToLeftCommand, rightToLeftPlain);
}

TEST_CASE("MText draws a backslash p that is not a paragraph command",
          "[text][mtext][issue3032][layout]") {
  // LibreCAD users type backslashes as ordinary text.
  SourceFonts fonts;
  for (const QString &content : {QStringLiteral("C:\\plans\\a.dxf"), QStringLiteral("\\parallel;"),
                                 QStringLiteral("\\pi;x"), QStringLiteral("\\pl2"),
                                 QStringLiteral("\\px"), QStringLiteral("\\p;"),
                                 QStringLiteral("\\px;A"), QStringLiteral("\\pt;A"),
                                 QStringLiteral("\\pxt,;A"), QStringLiteral("\\pq1;A"),
                                 QStringLiteral("\\pi2,k3;A"), QStringLiteral("\\pi1e5;A"),
                                 QStringLiteral("\\pI2;A"), QStringLiteral("\\ptx;A")}) {
    INFO(content.toStdString());
    RS_MText text(nullptr, leftToRightData(content));
    CHECK(drawnBy(text) == content);
    CHECK_THAT(inkOf(text).front().left, WithinAbs(0.0, inkTolerance));
  }

  // "\\" is a backslash, and what follows it is text.
  RS_MText escaped(nullptr, leftToRightData("\\\\pi2;A"));
  CHECK(drawnBy(escaped) == "\\pi2;A");

  // A command has to end near its start.
  const QString endless = "\\pt" + QStringLiteral("1,").repeated(200) + "1;A";
  RS_MText tooLong(nullptr, leftToRightData(endless));
  CHECK(drawnBy(tooLong) == endless);
}

TEST_CASE("MText stacked text makes room for blanks in front of a part",
          "[text][mtext][issue3032][layout]") {
  SourceFonts fonts;
  // A part is drawn at height 4 in the frame of the line.
  const double indentation = wordSpacing("standard", 4.0);
  const double letter = letterSpacing("standard", 9.0);
  RS_MText plain(nullptr, leftToRightData("X\\S1^2;Y"));
  const auto plainInk = inkOf(plain); // X, numerator, denominator, Y
  REQUIRE(plainInk.size() == 4);
  // What follows a stack is one letter spacing behind its widest part.
  CHECK_THAT(plainInk[3].left - std::max(plainInk[1].right, plainInk[2].right),
             WithinAbs(letter, inkTolerance));

  // A blank in front of the numerator. The first blank after "^" belongs to
  // the divider.
  RS_MText upper(nullptr, leftToRightData("X\\S 1^ 2;Y"));
  const auto upperInk = inkOf(upper);
  REQUIRE(upperInk.size() == 4);
  CHECK_THAT(upperInk[0].left, WithinAbs(plainInk[0].left, inkTolerance));
  CHECK_THAT(upperInk[1].left, WithinAbs(plainInk[1].left + indentation, inkTolerance));
  CHECK_THAT(upperInk[2].left, WithinAbs(plainInk[2].left, inkTolerance));
  CHECK_THAT(upperInk[3].left - std::max(upperInk[1].right, upperInk[2].right),
             WithinAbs(letter, inkTolerance));

  // Blanks in front of the denominator.
  RS_MText lower(nullptr, leftToRightData("X\\S1^   2;Y"));
  const auto lowerInk = inkOf(lower);
  REQUIRE(lowerInk.size() == 4);
  CHECK_THAT(lowerInk[1].left, WithinAbs(plainInk[1].left, inkTolerance));
  CHECK_THAT(lowerInk[2].left, WithinAbs(plainInk[2].left + 2 * indentation, inkTolerance));
  CHECK_THAT(lowerInk[3].left - std::max(lowerInk[1].right, lowerInk[2].right),
             WithinAbs(letter, inkTolerance));
  CHECK(lowerInk[3].left > plainInk[3].left);
}

TEST_CASE("Tolerance frames make room for blanks in front of a field",
          "[text][mtext][tolerance][issue3032][layout]") {
  SourceFonts fonts;
  for (const QString &code : {QStringLiteral("0.05%%vA"), QStringLiteral("  0.05%%vA"),
                              QStringLiteral("\t0.05%%v A")}) {
    INFO(code.toStdString());
    RS_Graphic graphic;
    auto *tolerance = new LC_Tolerance(
        &graphic, LC_ToleranceData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0), code, "Standard"));
    graphic.addEntity(tolerance);
    tolerance->update();

    std::vector<const RS_Entity *> fields;
    std::vector<double> dividers;
    for (const auto *entity : tolerance->getEntityList()) {
      if (entity->rtti() == RS2::EntityMText) {
        fields.push_back(entity);
      } else if (entity->rtti() == RS2::EntityLine) {
        const auto *line = static_cast<const RS_Line *>(entity);
        if (std::abs(line->getStartpoint().x - line->getEndpoint().x) < inkTolerance)
          dividers.push_back(line->getStartpoint().x);
      }
    }
    std::sort(dividers.begin(), dividers.end());
    REQUIRE(fields.size() == 2);
    REQUIRE(dividers.size() == 3);
    CHECK(dividers[0] < fields[0]->getMin().x);
    CHECK(fields[0]->getMax().x < dividers[1]);
    CHECK(dividers[1] < fields[1]->getMin().x);
    CHECK(fields[1]->getMax().x < dividers[2]);
  }
}

TEST_CASE("MText indentation is saved and the saved width covers it",
          "[text][mtext][issue3032][dxf][dwg][layout]") {
  SourceFonts fonts;
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  struct Sample {
    const char *name;
    QString text;
    QString saved; // groups 3 and 1 of the DXF record
    double declaredWidth;
  };
  // The long text crosses the 250 bytes at which a DXF record is cut in two.
  const QString longText = QString(246, QLatin1Char('A')) + "  \t  B";
  const Sample samples[] = {
      {"spaces", "    ABC", "    ABC", 100.0},
      {"tab", "\tABC", "^IABC", 0.0},
      {"inner_tab", "A\tB", "A^IB", 3.0},
      {"second_line", "ABC\n    DEF", "ABC\\P    DEF", 250.0},
      {"command", "\\pi2,l4;  ABC", "\\pi2,l4;  ABC", 100.0},
      {"long", longText, QString(longText).replace('\t', "^I"), 100.0},
  };
  struct Format {
    RS2::FormatType type;
    const char *suffix;
  };
  const Format formats[] = {{RS2::FormatDXFRW2000, "_2000.dxf"},
                            {RS2::FormatDXFRW, "_2007.dxf"},
                            {RS2::FormatDXFRW2018, "_2018.dxf"},
                            {RS2::FormatDWG2004, ".dwg"}};
  for (const auto &format : formats) {
    for (const auto &sample : samples) {
      INFO(sample.name << format.suffix);
      RS_Graphic original;
      auto data = leftToRightData(sample.text);
      data.width = sample.declaredWidth;
      auto *source = new RS_MText(&original, data);
      original.addEntity(source);
      // The text reaches this far from its insertion point, blanks included.
      const double occupied = source->getMax().x;

      const QString path = directory.filePath(QString(sample.name) + format.suffix);
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(original, path, format.type));
      if (path.endsWith(".dxf")) {
        CHECK(mtextGroup(path, 3) + mtextGroup(path, 1) == sample.saved);
        bool isNumber = false;
        const double width = mtextGroup(path, 41).toDouble(&isNumber);
        REQUIRE(isNumber);
        CHECK(width >= occupied - 1e-6);
      }

      RS_Graphic imported;
      RS_FilterDXFRW reader;
      REQUIRE(reader.fileImport(imported, path, format.type));
      auto *entity = findMText(imported);
      REQUIRE(entity);
      CHECK(entity->getText() == sample.text);
      CHECK(entity->getHAlign() == RS_MTextData::HALeft);
      CHECK(entity->getWidth() >= occupied - 1e-6);
      // Keep this layout check independent of the file's STYLE resolution.
      entity->setStyle("standard");
      entity->update();
      REQUIRE(entity->count() == source->count());
      for (unsigned line = 0; line < source->count(); ++line)
        checkSameInk(*entity, *source, 0.0, static_cast<int>(line));
    }
  }

  // R12 has no MTEXT. Each line becomes a TEXT, which keeps its leading blanks.
  RS_Graphic original;
  original.addEntity(new RS_MText(&original, leftToRightData("ABC\n    DEF")));
  const QString path = directory.filePath("lines_r12.dxf");
  RS_FilterDXFRW writer;
  REQUIRE(writer.fileExport(original, path, RS2::FormatDXFRW12));
  RS_Graphic imported;
  RS_FilterDXFRW reader;
  REQUIRE(reader.fileImport(imported, path, RS2::FormatDXFRW12));
  QStringList lines;
  for (auto *entity : imported.getEntityList()) {
    if (entity->rtti() == RS2::EntityText)
      lines << static_cast<RS_Text *>(entity)->getText();
  }
  CHECK(lines == QStringList{"ABC", "    DEF"});
}

TEST_CASE("MText with a paragraph command keeps the width it was given",
          "[text][mtext][issue3032][dxf][layout]") {
  // The indentations of a paragraph command are not laid out, so the width the
  // text uses here says less than the box it came with. Other text is saved
  // with the width it uses.
  SourceFonts fonts;
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  enum class Saved { Given, Used };
  struct Sample {
    QString text;
    bool hasCommand;
    double givenWidth;
    Saved saved;
  };
  const double infinity = std::numeric_limits<double>::infinity();
  const double notANumber = std::numeric_limits<double>::quiet_NaN();
  const Sample samples[] = {
      {"\\pi-12,l12,t12;ABC", true, 250.0, Saved::Given},
      {"AB\\pxqc;C", true, 250.0, Saved::Given},
      {"\\pi-12,l12,t12;ABC", true, 3.0, Saved::Used},
      // a box of width 0 does not wrap; it is not given a width that would
      {"\\pi-12,l12,t12;ABC", true, 0.0, Saved::Given},
      // a width that is no width does not get into the file
      {"\\pi-12,l12,t12;ABC", true, infinity, Saved::Used},
      {"\\pi-12,l12,t12;ABC", true, -infinity, Saved::Used},
      {"\\pi-12,l12,t12;ABC", true, notANumber, Saved::Used},
      {"\\pi-12,l12,t12;ABC", true, -250.0, Saved::Used},
      {"ABC", false, 250.0, Saved::Used},
      {"ABC", false, 0.0, Saved::Used},
      {"C:\\plans\\a.dxf", false, 250.0, Saved::Used},
  };
  int index = 0;
  for (const auto &sample : samples) {
    INFO(sample.text.toStdString() << ", width " << sample.givenWidth);
    RS_Graphic original;
    auto data = leftToRightData(sample.text);
    data.width = sample.givenWidth;
    auto *source = new RS_MText(&original, data);
    original.addEntity(source);
    CHECK(source->hasParagraphCommands() == sample.hasCommand);
    REQUIRE(source->getUsedTextWidth() > 3.0);
    const QString path = directory.filePath(QString("width_%1.dxf").arg(index++));
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(original, path, RS2::FormatDXFRW));
    bool isNumber = false;
    const double width = mtextGroup(path, 41).toDouble(&isNumber);
    REQUIRE(isNumber);
    const double expected = sample.saved == Saved::Given ? sample.givenWidth : source->getUsedTextWidth();
    CHECK_THAT(width, WithinAbs(expected, 1e-6));
    const QString dwgPath = directory.filePath(QString("width_%1.dwg").arg(index));
    REQUIRE(writer.fileExport(original, dwgPath, RS2::FormatDWG2004));
    RS_Graphic restored;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(restored, dwgPath, RS2::FormatDWG2004));
    const auto *restoredText = findMText(restored);
    REQUIRE(restoredText);
    CHECK_THAT(restoredText->getWidth(), WithinAbs(expected, 1e-6));
  }
}

namespace {
/** Rejects a dialog on the way out of a test, so that a failed check does not let it save its settings. */
struct RejectAtEnd {
  QDialog &dialog;
  ~RejectAtEnd() { dialog.reject(); }
};

void typeTab(QWidget &edit) {
  // A key press carries its character; without it Qt takes Tab for navigation.
  keyPress(edit, Qt::Key_Tab, Qt::NoModifier, QStringLiteral("\t"));
}

void showWithFocus(QWidget &window, QWidget &edit) {
  window.show();
  window.activateWindow();
  QApplication::processEvents();
  edit.setFocus();
  QApplication::processEvents();
}
} // namespace

TEST_CASE("The MText dialog types a tab", "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  LC_GraphicViewport viewport;
  RS_MText mtext(nullptr, leftToRightData("ABC"));
  QG_DlgMText dialog(nullptr, &viewport, &mtext, false);
  const RejectAtEnd rejectAtEnd{dialog};
  auto *edit = dialog.findChild<QTextEdit *>("teText");
  REQUIRE(edit);
  CHECK_FALSE(edit->tabChangesFocus());
  showWithFocus(dialog, *edit);

  // The dialog opens with its text selected. A tab typed then replaces the
  // text, as any other character does, and one undo brings the text back.
  REQUIRE(edit->textCursor().selectedText() == "ABC");
  typeTab(*edit);
  CHECK(edit->toPlainText() == "\t");
  edit->undo();
  CHECK(edit->toPlainText() == "ABC");

  auto cursor = edit->textCursor();
  cursor.setPosition(0);
  edit->setTextCursor(cursor);
  typeTab(*edit);
  CHECK(edit->toPlainText() == "\tABC");
  dialog.updateEntity();
  CHECK(mtext.getText() == "\tABC");
  CHECK_THAT(inkOf(mtext).front().left, WithinAbs(36.0, inkTolerance));

  // In right-to-left mode a tab is typed the same way.
  auto *rightToLeft = dialog.findChild<QRadioButton *>("rbRightToLeft");
  REQUIRE(rightToLeft);
  rightToLeft->click();
  cursor = edit->textCursor();
  cursor.movePosition(QTextCursor::End);
  edit->setTextCursor(cursor);
  typeTab(*edit);
  CHECK(edit->toPlainText() == "\tABC\t");
}

TEST_CASE("The MText dialog is left with Shift+Tab and Ctrl+Tab",
          "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  LC_GraphicViewport viewport;
  RS_MText mtext(nullptr, leftToRightData("ABC"));
  QG_DlgMText dialog(nullptr, &viewport, &mtext, false);
  const RejectAtEnd rejectAtEnd{dialog};
  auto *edit = dialog.findChild<QTextEdit *>("teText");
  REQUIRE(edit);

  struct Key {
    const char *name;
    int key;
    Qt::KeyboardModifiers modifiers;
    QString text;
  };
  // Windows sends Shift+Tab with a tab as its text; X11 and macOS do not.
  const Key keys[] = {{"Shift+Tab", Qt::Key_Backtab, Qt::ShiftModifier, QString()},
                      {"Shift+Tab as Windows sends it", Qt::Key_Backtab, Qt::ShiftModifier, "\t"},
                      {"Ctrl+Tab", Qt::Key_Tab, Qt::ControlModifier, "\t"},
                      {"Control+Tab on macOS", Qt::Key_Tab, Qt::MetaModifier, "\t"}};
  QWidget *previous = nullptr;
  QWidget *next = nullptr;
  for (const auto &key : keys) {
    INFO(key.name);
    showWithFocus(dialog, *edit);
    REQUIRE(edit->hasFocus());
    edit->setPlainText("ABC");
    keyPress(*edit, key.key, key.modifiers, key.text);
    CHECK(edit->toPlainText() == "ABC");
    CHECK_FALSE(edit->hasFocus());
    QWidget *reached = QApplication::focusWidget();
    REQUIRE(reached);
    CHECK(dialog.isAncestorOf(reached));
    (key.key == Qt::Key_Backtab ? previous : next) = reached;
  }
  // the two directions end on different widgets
  CHECK(previous != next);
}

TEST_CASE("The MText editor of the property dialog types a tab",
          "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  LC_GraphicViewport viewport;
  RS_MText mtext(nullptr, leftToRightData("ABC"));
  LC_MTextPropertiesEditingWidget editor(nullptr);
  editor.setGraphicViewport(&viewport);
  editor.setEntity(&mtext);
  auto *edit = editor.findChild<LC_TextEdit *>("teText");
  REQUIRE(edit);
  CHECK_FALSE(edit->tabChangesFocus());
  auto cursor = edit->textCursor();
  cursor.setPosition(1);
  edit->setTextCursor(cursor);
  typeTab(*edit);
  CHECK(edit->toPlainText() == "A\tBC");
  CHECK(mtext.getText() == "A\tBC");

  // Shift+Tab is not typed, also not the way Windows sends it.
  keyPress(*edit, Qt::Key_Backtab, Qt::ShiftModifier, QStringLiteral("\t"));
  CHECK(edit->toPlainText() == "A\tBC");

  // Blanks, tabs and line breaks reach the entity as they are.
  const QString content = QStringLiteral("  A\tB\n    C  ");
  edit->setPlainText(content);
  CHECK(mtext.getText() == content);
}

TEST_CASE("The content dialog of the property sheet types a tab",
          "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  LC_MultilineTextEditDialog dialog;
  const RejectAtEnd rejectAtEnd{dialog};
  dialog.setText("ABC");
  auto *edit = dialog.findChild<QPlainTextEdit *>("plainTextEdit");
  auto *buttons = dialog.findChild<QDialogButtonBox *>("buttonBox");
  REQUIRE(edit);
  REQUIRE(buttons);
  CHECK_FALSE(edit->tabChangesFocus());
  typeTab(*edit);
  CHECK(dialog.getText() == "\tABC");

  const QString content = QStringLiteral("  A\tB\n    C  ");
  dialog.setText(content);
  CHECK(dialog.getText() == content);

  // The buttons are reached from the keyboard with Shift+Tab or Ctrl+Tab,
  // also the way Windows sends Shift+Tab.
  struct Key {
    const char *name;
    int key;
    Qt::KeyboardModifiers modifiers;
    QString text;
  };
  const Key keys[] = {{"Shift+Tab", Qt::Key_Backtab, Qt::ShiftModifier, QString()},
                      {"Shift+Tab as Windows sends it", Qt::Key_Backtab, Qt::ShiftModifier, "\t"},
                      {"Ctrl+Tab", Qt::Key_Tab, Qt::ControlModifier, "\t"}};
  for (const auto &key : keys) {
    INFO(key.name);
    showWithFocus(dialog, *edit);
    REQUIRE(edit->hasFocus());
    keyPress(*edit, key.key, key.modifiers, key.text);
    CHECK(dialog.getText() == content);
    QWidget *reached = QApplication::focusWidget();
    REQUIRE(reached);
    CHECK(buttons->isAncestorOf(reached));
  }
}

TEST_CASE("The MText tool options load their text as plain text",
          "[text][mtext][issue3032][gui]") {
  // QTextEdit::setText() guesses the format: this text looks like HTML to it,
  // and came back as "a c d".
  SourceFonts fonts;
  lc::test::ActionFixture<LC_ActionDrawMText> fixture;
  auto *action = fixture.m_action.get();
  action->reset();
  const QString content = QStringLiteral("a <b> c    d\n\tx");
  action->setText(content);
  LC_MTextOptionsWidget widget;
  widget.setAction(action, true);
  auto *edit = widget.findChild<QTextEdit *>("teText");
  REQUIRE(edit);
  CHECK(edit->toPlainText() == content);
  CHECK(action->getText() == content);
}

TEST_CASE("The MText tool options type a tab and are left with Shift+Tab and Ctrl+Tab",
          "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  lc::test::ActionFixture<LC_ActionDrawMText> fixture;
  auto *action = fixture.m_action.get();
  action->reset();
  action->setText("ABC");
  LC_MTextOptionsWidget widget;
  widget.setAction(action, true);
  auto *edit = widget.findChild<QTextEdit *>("teText");
  REQUIRE(edit);
  CHECK_FALSE(edit->tabChangesFocus());
  showWithFocus(widget, *edit);
  REQUIRE(edit->hasFocus());

  typeTab(*edit);
  CHECK(edit->toPlainText() == "\tABC");
  CHECK(action->getText() == "\tABC");

  struct Key {
    const char *name;
    int key;
    Qt::KeyboardModifiers modifiers;
    QString text;
  };
  // Windows sends Shift+Tab with a tab as its text; X11 and macOS do not.
  const Key keys[] = {{"Shift+Tab", Qt::Key_Backtab, Qt::ShiftModifier, QString()},
                      {"Shift+Tab as Windows sends it", Qt::Key_Backtab, Qt::ShiftModifier, "\t"},
                      {"Ctrl+Tab", Qt::Key_Tab, Qt::ControlModifier, "\t"},
                      {"Control+Tab on macOS", Qt::Key_Tab, Qt::MetaModifier, "\t"}};
  for (const auto &key : keys) {
    INFO(key.name);
    showWithFocus(widget, *edit);
    REQUIRE(edit->hasFocus());
    keyPress(*edit, key.key, key.modifiers, key.text);
    CHECK(edit->toPlainText() == "\tABC");
    CHECK(action->getText() == "\tABC");
    CHECK_FALSE(edit->hasFocus());
    QWidget *reached = QApplication::focusWidget();
    REQUIRE(reached);
    CHECK(widget.isAncestorOf(reached));
  }
}

TEST_CASE("MText dialogs keep blanks and tabs and line breaks",
          "[text][mtext][issue3032][gui]") {
  SourceFonts fonts;
  LC_GraphicViewport viewport;
  const QString content = QStringLiteral("  A\tB\n    C  ");
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  {
    RS_MText created(nullptr, leftToRightData(QString()));
    QG_DlgMText dialog(nullptr, &viewport, &created, true);
    auto *edit = dialog.findChild<QTextEdit *>("teText");
    REQUIRE(edit);
    edit->setPlainText(content);
    dialog.updateEntity();
    CHECK(created.getText() == content);

    const QString path = directory.filePath("content.txt");
    dialog.save(path);
    edit->clear();
    dialog.load(path);
    CHECK(edit->toPlainText() == content);
    dialog.accept();
  }
  {
    // An accepted dialog remembers its text for the next new MText.
    RS_MText next(nullptr, leftToRightData(QString()));
    QG_DlgMText dialog(nullptr, &viewport, &next, true);
    const RejectAtEnd rejectAtEnd{dialog};
    auto *edit = dialog.findChild<QTextEdit *>("teText");
    REQUIRE(edit);
    CHECK(edit->toPlainText() == content);
  }
}
