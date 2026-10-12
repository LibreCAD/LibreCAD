// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Leading spaces indent a left-aligned MText (issue #3032). Positions are
// compared to the same text without the spaces, so that no glyph outline is
// copied in here.

#include <cmath>
#include <vector>

#include <QtTest>
#include <QtWidgets>

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

struct InkSpan {
    QString name;
    double left = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

RS_MTextData textData(const QString& text, RS_MTextData::HAlign halign = RS_MTextData::HALeft,
                      RS_MTextData::MTextDrawingDirection direction = RS_MTextData::LeftToRight,
                      double height = 9.0, RS_MTextData::VAlign valign = RS_MTextData::VATop)
{
    return {RS_Vector(0.0, 0.0), height, 100.0, valign, halign, direction, RS_MTextData::Exact,
            1.0, text, "standard", 0.0, RS2::Update};
}

// What one line of the text draws, in drawing order.
std::vector<InkSpan> inkOf(RS_MText& text, int index = 0)
{
    std::vector<InkSpan> spans;
    auto* line = dynamic_cast<RS_EntityContainer*>(text.entityAt(index));
    if (line == nullptr)
        return spans;
    for (RS_Entity* entity : *line) {
        const QString name = entity->rtti() == RS2::EntityInsert
            ? static_cast<RS_Insert*>(entity)->getName() : QStringLiteral("<stack>");
        spans.push_back({name, entity->getMin().x, entity->getMax().x, entity->getMin().y});
    }
    return spans;
}

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

// True if a line draws what a line of control draws, dx further right.
bool sameInk(RS_MText& text, RS_MText& control, double dx = 0.0, int index = 0)
{
    const auto ink = inkOf(text, index);
    const auto expected = inkOf(control, index);
    if (ink.empty() || ink.size() != expected.size())
        return false;
    for (std::size_t i = 0; i < ink.size(); ++i) {
        if (ink[i].name != expected[i].name || !near(ink[i].left, expected[i].left + dx)
            || !near(ink[i].right, expected[i].right + dx) || !near(ink[i].bottom, expected[i].bottom))
            return false;
    }
    return true;
}

double wordSpacing(double height = 9.0)
{
    return RS_FONTLIST->requestFont("standard")->getWordSpacing() * height / 9.0;
}

} // namespace

class LC_MTextIndentationTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Settings are only read, under a name of their own.
        QCoreApplication::setOrganizationName("LibreCAD-MTextIndentationTests");
        QCoreApplication::setApplicationName("layout");
        QCoreApplication::setApplicationVersion("2.2.1");
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        RS_SYSTEM->init("LibreCAD", "2.2.1", "librecad", QString(LIBRECAD_TEST_SOURCE_DIR) + "/unix");
        RS_FONTLIST->init();
        // The build puts the font where an application that is run in place looks for it.
        QVERIFY2(RS_FONTLIST->requestFont("standard") != nullptr,
                 "resources/fonts/standard.lff is expected next to the test");
    }

    void leadingSpacesIndentALeftAlignedLine()
    {
        for (const double height : {9.0, 2.5}) {
            RS_MText control(nullptr, textData("ABC", RS_MTextData::HALeft, RS_MTextData::LeftToRight, height));
            QVERIFY(near(control.getMin().x, 0.0));
            for (const int count : {1, 4}) {
                const double indentation = count * wordSpacing(height);
                RS_MText indented(nullptr, textData(QString(count, ' ') + "ABC", RS_MTextData::HALeft,
                                                    RS_MTextData::LeftToRight, height));
                QVERIFY(sameInk(indented, control, indentation));
                QVERIFY(near(indented.getMin().x, indentation));
                // The width counts from the insertion point: it is what a file declares.
                QVERIFY(near(indented.getUsedTextWidth(), control.getUsedTextWidth() + indentation));
                QCOMPARE(indented.getUsedTextHeight(), control.getUsedTextHeight());
                QCOMPARE(indented.getText(), QString(count, ' ') + "ABC");
            }
        }
        for (const auto valign : {RS_MTextData::VATop, RS_MTextData::VAMiddle, RS_MTextData::VABottom}) {
            RS_MText control(nullptr, textData("ABC", RS_MTextData::HALeft, RS_MTextData::LeftToRight, 9.0, valign));
            RS_MText indented(nullptr, textData("    ABC", RS_MTextData::HALeft, RS_MTextData::LeftToRight, 9.0, valign));
            QVERIFY(sameInk(indented, control, 4 * wordSpacing()));
        }
    }

    void indentationBelongsToOneLineAndToItsLeadingSpaces()
    {
        RS_MText control(nullptr, textData("ABC\nDEF"));
        for (const QString& lineBreak : {QStringLiteral("\n"), QStringLiteral("\\P")}) {
            RS_MText text(nullptr, textData("ABC" + lineBreak + "    DEF"));
            QCOMPARE(text.count(), 2u);
            QVERIFY(sameInk(text, control, 0.0, 0));
            QVERIFY(sameInk(text, control, 4 * wordSpacing(), 1));
        }
        RS_MText plain(nullptr, textData("ABC"));
        RS_MText trailing(nullptr, textData("ABC    "));
        QVERIFY(sameInk(trailing, plain));
        QVERIFY(near(trailing.getUsedTextWidth(), plain.getUsedTextWidth()));
        RS_MText words(nullptr, textData("A B"));
        RS_MText wider(nullptr, textData("  A   B"));
        QCOMPARE(inkOf(wider).size(), std::size_t(2));
        QVERIFY(near(inkOf(wider)[0].left, inkOf(words)[0].left + 2 * wordSpacing()));
        QVERIFY(near(inkOf(wider)[1].left, inkOf(words)[1].left + 4 * wordSpacing()));
    }

    void otherAlignmentsAndRightToLeftTextStayWhereTheyAre()
    {
        for (const auto halign : {RS_MTextData::HACenter, RS_MTextData::HARight}) {
            RS_MText plain(nullptr, textData("ABC", halign));
            RS_MText blanks(nullptr, textData("    ABC  ", halign));
            QVERIFY(sameInk(blanks, plain));
            QVERIFY(near(blanks.getUsedTextWidth(), plain.getUsedTextWidth()));
        }
        // In a right-to-left line the first letter is at the right end.
        RS_MText plain(nullptr, textData("ABC", RS_MTextData::HALeft, RS_MTextData::RightToLeft));
        for (const QString& content : {QStringLiteral("    ABC"), QStringLiteral("ABC    "), QStringLiteral("  ABC  ")}) {
            RS_MText blanks(nullptr, textData(content, RS_MTextData::HALeft, RS_MTextData::RightToLeft));
            QVERIFY2(sameInk(blanks, plain), qPrintable(content));
            QVERIFY(near(blanks.getUsedTextWidth(), plain.getUsedTextWidth()));
        }
    }

    void stackedTextMakesRoomForASpaceInFrontOfAPart()
    {
        // A part is drawn at height 4 in the frame of the line.
        const double indentation = wordSpacing(4.0);
        RS_MText plain(nullptr, textData("X\\S1^2;Y"));
        RS_MText blank(nullptr, textData("X\\S 1^ 2;Y"));
        const auto plainInk = inkOf(plain); // X, numerator, denominator, Y
        const auto blankInk = inkOf(blank);
        QCOMPARE(plainInk.size(), std::size_t(4));
        QCOMPARE(blankInk.size(), std::size_t(4));
        QVERIFY(near(blankInk[1].left, plainInk[1].left + indentation));
        // The space after "^" belongs to the divider.
        QVERIFY(near(blankInk[2].left, plainInk[2].left));
        // What follows keeps its distance from the part that reaches furthest.
        QVERIFY(near(blankInk[3].left - blankInk[1].right, plainInk[3].left - plainInk[2].right));
        QVERIFY(blankInk[1].right < blankInk[3].left);
    }
};

QTEST_MAIN(LC_MTextIndentationTests)
#include "mtext_indentation_tests.moc"
