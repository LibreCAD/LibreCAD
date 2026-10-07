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
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QTemporaryDir>
#include <QTextEdit>

#include "lc_graphicviewport.h"
#include "lc_mtextpropertieseditingwidget.h"
#include "lc_textbidi.h"
#include "lc_textpropertieseditingwidget.h"
#include "qg_dlg_mtext.h"
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
  const QString input = QString::fromUtf8("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d") +
                        QStringLiteral(" 12.5");
  RS_MText mtext(nullptr, mtextData(input));
  LC_GraphicViewport viewport;
  LC_MTextPropertiesEditingWidget editor(nullptr);
  editor.setGraphicViewport(&viewport);
  editor.setEntity(&mtext);
  auto *edit = editor.findChild<QPlainTextEdit *>("teText");
  REQUIRE(edit);
  auto cursor = edit->textCursor();
  cursor.setPosition(1);
  cursor.setPosition(3, QTextCursor::KeepAnchor);
  edit->setTextCursor(cursor);
  for (const char *button : {"rbLeftToRight", "rbRightToLeft"}) {
    auto *radio = editor.findChild<QRadioButton *>(button);
    REQUIRE(radio);
    radio->setChecked(true);
    CHECK(edit->toPlainText() == input);
    CHECK(mtext.getText() == input);
    CHECK(edit->textCursor().anchor() == 1);
    CHECK(edit->textCursor().position() == 3);
  }

  QG_DlgMText dialog(nullptr, &viewport, &mtext, false);
  auto *dialogEdit = dialog.findChild<QTextEdit *>("teText");
  REQUIRE(dialogEdit);
  REQUIRE(dialogEdit->toPlainText() == input);
  dialog.findChild<QRadioButton *>("rbLeftToRight")->setChecked(true);
  dialog.findChild<QRadioButton *>("rbRightToLeft")->setChecked(true);
  dialog.updateEntity();
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
  dialog.load(textPath);
  CHECK(dialogEdit->toPlainText() == input);
  dialogEdit->setPlainText("<b>" + input + "</b>");
  dialog.updateEntity();
  CHECK(mtext.getText() == "<b>" + input + "</b>");
  dialog.reject();

  RS_TextData data;
  data.text = input;
  data.style = "iso3098";
  data.drawingDirection = RS_TextData::RightToLeft;
  RS_Text text(nullptr, data);
  LC_TextPropertiesEditingWidget textEditor(nullptr);
  textEditor.setGraphicViewport(&viewport);
  textEditor.setEntity(&text);
  auto *lineEdit = textEditor.findChild<QLineEdit *>("leText");
  REQUIRE(lineEdit);
  REQUIRE(lineEdit->text() == input);
  lineEdit->setSelection(1, 2);
  for (const char *button : {"rbAuto", "rbLeftToRight", "rbRightToLeft"}) {
    textEditor.findChild<QRadioButton *>(button)->setChecked(true);
    CHECK(lineEdit->text() == input);
    CHECK(lineEdit->selectionStart() == 1);
    CHECK(lineEdit->selectedText().size() == 2);
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
