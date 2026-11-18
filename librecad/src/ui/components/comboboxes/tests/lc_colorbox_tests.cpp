/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
**********************************************************************/

// The color combo keeps the colors picked with its color dialog in the settings,
// under keys that do not depend on the language, and tells its rows apart by
// what they are, not by their (translated) text: a picked color loaded from the
// settings is a picked color in every language.

#include <optional>

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QColor>
#include <QString>
#include <QTranslator>

#include "lc_actiontestsupport.h"
#include "qg_colorbox.h"
#include "rs_color.h"
#include "rs_settings.h"

namespace {

// Translates every string of the color combo, as any non-English catalog would.
class PseudoTranslator : public QTranslator {
public:
    QString translate(const char* context, const char* sourceText, const char* /*disambiguation*/ = nullptr,
                      int /*n*/ = -1) const override {
        const QString ctx = QString::fromLatin1(context);
        if (ctx == QLatin1String("QG_ColorBox") || ctx == QLatin1String("QObject")) {
            return QStringLiteral("~") + QString::fromUtf8(sourceText);
        }
        return {};
    }
    bool isEmpty() const override { return false; }
};

class InstalledTranslator {
public:
    InstalledTranslator() { REQUIRE(QApplication::installTranslator(&m_translator)); }
    ~InstalledTranslator() { QApplication::removeTranslator(&m_translator); }
private:
    PseudoTranslator m_translator;
};

// A combo whose color dialog has just returned a color.
class PickingColorBox : public QG_ColorBox {
public:
    PickingColorBox() : QG_ColorBox(true, true) {}
    void pick(const QColor& color) { addCustomColor(RS_Color(color)); }
};

void clearPickedColors() {
    LC_GROUP_GUARD("ColorBox");
    for (int i = 0; i < 8; ++i) {
        LC_REMOVE(QStringLiteral("/CustomColor%1").arg(i));
    }
}

std::optional<QColor> storedColor(int index) {
    LC_GROUP_GUARD("ColorBox");
    const QString key = QStringLiteral("/CustomColor%1").arg(index);
    if (LC_GET_STR(key).isEmpty()) {
        return std::nullopt;
    }
    return QColor(static_cast<QRgb>(LC_GET_COLOR(key, 0)));
}

bool hasColor(const QG_ColorBox& box, const QColor& color) {
    for (int i = 0; i < box.count(); ++i) {
        if (box.itemData(i).value<QColor>() == color) {
            return true;
        }
    }
    return false;
}

int rowsLabelled(const QG_ColorBox& box, const QString& text) {
    int rows = 0;
    for (int i = 0; i < box.count(); ++i) {
        rows += box.itemText(i) == text ? 1 : 0;
    }
    return rows;
}

// Colors no named row of the combo is near.
const QColor first{11, 22, 33};
const QColor second{44, 155, 66};
const QColor third{177, 88, 199};
const QColor fourth{210, 120, 30};

} // namespace

TEST_CASE("Picked colors are kept across sessions in a translated UI", "[gui][color][combo][translation]") {
    (void)lc::test::application();
    clearPickedColors();
    InstalledTranslator translator;

    {
        PickingColorBox session;
        session.pick(first);
    }
    {
        // the first color comes back from the settings, then a second one is picked
        PickingColorBox session;
        REQUIRE(hasColor(session, first));
        session.pick(second);
    }
    PickingColorBox later;
    CHECK(hasColor(later, first));
    CHECK(hasColor(later, second));
    // every picked row carries the translated label, wherever it came from
    CHECK(rowsLabelled(later, QStringLiteral("~Custom Picked")) == 2);
    CHECK(rowsLabelled(later, QStringLiteral("Custom Picked")) == 0);
    clearPickedColors();
}

TEST_CASE("Without picked colors the list has no picked rows", "[gui][color][combo]") {
    (void)lc::test::application();
    clearPickedColors();
    QG_ColorBox box(true, true);
    CHECK(rowsLabelled(box, QStringLiteral("Custom Picked")) == 0);
}

TEST_CASE("Picked colors are stored under language-independent keys", "[gui][color][combo][translation]") {
    (void)lc::test::application();
    clearPickedColors();
    InstalledTranslator translator;
    {
        PickingColorBox session;
        session.pick(first);
    }
    CHECK(storedColor(0) == std::optional<QColor>(first));
    CHECK_FALSE(storedColor(1).has_value());
    clearPickedColors();
}

TEST_CASE("The newest of more picked colors than are kept survives a restart", "[gui][color][combo]") {
    (void)lc::test::application();
    clearPickedColors();
    {
        PickingColorBox session;
        session.pick(first);
        session.pick(second);
        session.pick(third);
        session.pick(fourth);
        CHECK(rowsLabelled(session, QStringLiteral("Custom Picked")) == 3);
    }
    PickingColorBox later;
    CHECK_FALSE(hasColor(later, first));
    CHECK(hasColor(later, second));
    CHECK(hasColor(later, third));
    CHECK(hasColor(later, fourth));
    clearPickedColors();
}

TEST_CASE("A color outside the list gets one translated temporary row", "[gui][color][combo][translation]") {
    (void)lc::test::application();
    clearPickedColors();
    InstalledTranslator translator;
    QG_ColorBox box(true, true);
    box.setColor(RS_Color(first));
    box.setColor(RS_Color(second));
    CHECK(rowsLabelled(box, QStringLiteral("~User Color")) == 1);
    CHECK_FALSE(hasColor(box, first));
    CHECK(hasColor(box, second));
    CHECK(box.getColor() == RS_Color(second));
}
