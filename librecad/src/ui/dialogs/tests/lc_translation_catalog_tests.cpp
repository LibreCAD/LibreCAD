// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 librecad.org
// Copyright (C) 2026 Dongxu Li (github.com/dxli)

// Issue #3235: the Draw Fast tool, its group in the Options dialog and the Polar
// Snap tooltip showed English text in every language, and a translator could not
// find those strings in the catalogs, because the catalogs had not been refreshed
// with `lupdate` since the strings were added. A string that is absent from a
// catalog can neither be translated nor looked up, so the catalogs that lupdate
// maintains (the TRANSLATIONS list of librecad/src/src.pro) must list them.
//
// The catalogs are read from the source tree, not from the compiled .qm files:
// lrelease leaves out untranslated messages, which is exactly the state a new
// string is in until a translator has worked on it.

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

#include <iterator>
#include <string>

namespace {

struct ExpectedString {
    const char* m_context; // the class whose tr() or form shows the string
    const char* m_source;  // the English text
    const char* m_file;    // where the program has it, relative to the source tree
};

constexpr const char* ACTION_FACTORY = "librecad/src/ui/main/init/lc_actionfactory.cpp";
constexpr const char* DRAW_FAST = "librecad/src/actions/drawing/draw/line/lc_actiondrawlinedirect.cpp";
constexpr const char* OPTIONS_FORM = "librecad/src/ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.ui";

// The strings the issue names (the Draw Fast and Polar Snap tooltips, the check box
// that hides backup files, and the Draw Fast defaults group), with the rest of the
// Draw Fast prompts and the soft angle snap group that sit next to them.
const ExpectedString EXPECTED[] = {
    {"LC_ActionFactory", "Draw Fast", ACTION_FACTORY},
    {"LC_ActionFactory", "Polar Snap", ACTION_FACTORY},

    {"QG_DlgOptionsGeneral", "Hide backup and autosave files", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Draw Fast Defaults", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Opening marker depth:", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral",
     "Length of the perpendicular tick marks drawn on each side of a wall opening "
     "(Draw Fast 'o' / 'opening' command).",
     OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Window offset width:", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral",
     "Distance between the wall line and the inner window line in Draw Fast window mode "
     "('w' / 'window' command).",
     OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Door thickness:", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral",
     "Thickness of the door slab in Draw Fast door mode. Set to 0 for a single-line door leaf.",
     OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Door swing angle:", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral",
     "Arc sweep angle for the door swing in Draw Fast door mode. 90 degrees draws a "
     "quarter-circle; 45 degrees draws a shallower arc.",
     OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Door design:", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral",
     "Door symbol style. 'Swing only' draws the leaf and arc. 'Swing with opening line' "
     "also draws a straight line across the full door opening.",
     OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Swing only", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Swing with opening line", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Soft Angle Snap", OPTIONS_FORM},
    {"QG_DlgOptionsGeneral", "Soft Snap sensitivity:", OPTIONS_FORM},

    {"LC_ActionDrawLineDirect", "Specify first point", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Specify next point or type distance", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Cannot undo: Begin of history reached", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Cannot open: draw at least one wall segment first.", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Type opening width", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Click to choose marker side", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Cannot place window: draw at least one wall segment first.", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Type window width", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Click to choose window direction", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Cannot place door: draw at least one wall segment first.", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Type door width", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Move to the swing side and desired hinge, then click", DRAW_FAST},
    {"LC_ActionDrawLineDirect", "Click near hinge end", DRAW_FAST},
};

QString sourceTreePath(const QString& relative) {
    return QStringLiteral(LIBRECAD_SOURCE_DIR "/") + relative;
}

// One catalog entry as a set member: the context and the source text, which is all
// QCoreApplication::translate() looks a message up by (the disambiguation comment
// is empty for these strings).
QString entryKey(const QString& context, const QString& source) {
    return context + QChar(0x1f) + source;
}

// True when the catalog's <TS> element names a target language. lupdate leaves a
// catalog without one alone ("won't be updated: it does not specify any target
// languages"); the regional Spanish catalogs (librecad_es_bo.ts and the like) are
// such old-format files, so no refresh can reach them.
bool hasTargetLanguage(const QString& name) {
    QFile file(sourceTreePath(QStringLiteral("librecad/ts/") + name));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            return xml.name() == u"TS" && !xml.attributes().value(QStringLiteral("language")).isEmpty();
        }
    }
    return false;
}

// The catalogs that `lupdate` keeps up to date: the files TRANSLATIONS lists in
// librecad/src/src.pro that name a target language, as names relative to the
// librecad/ts directory.
QStringList maintainedCatalogs() {
    QFile pro(sourceTreePath(QStringLiteral("librecad/src/src.pro")));
    if (!pro.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QStringList lines = QString::fromUtf8(pro.readAll()).split('\n');
    static const QRegularExpression entry(QStringLiteral(R"(^\s*\.\./ts/(\w+\.ts)\s*(\\)?\s*$)"));

    QStringList names;
    bool inList = false;
    for (const QString& line : lines) {
        if (!inList) {
            inList = line.trimmed().startsWith(QStringLiteral("TRANSLATIONS"));
            continue;
        }
        const QRegularExpressionMatch match = entry.match(line);
        if (!match.hasMatch()) {
            break;
        }
        const bool last = match.captured(2).isEmpty();
        if (hasTargetLanguage(match.captured(1))) {
            names << match.captured(1);
        }
        if (last) {
            break;
        }
    }
    return names;
}

// Every message of a catalog that is not marked vanished or obsolete.
QSet<QString> catalogEntries(const QString& name) {
    QSet<QString> entries;
    QFile file(sourceTreePath(QStringLiteral("librecad/ts/") + name));
    if (!file.open(QIODevice::ReadOnly)) {
        return entries;
    }
    QXmlStreamReader xml(&file);
    QString context;
    QString source;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }
        const QStringView tag = xml.name();
        if (tag == u"message") {
            source.clear();
        }
        else if (tag == u"name") {
            // only a context has a <name>
            context = xml.readElementText(QXmlStreamReader::SkipChildElements);
        }
        else if (tag == u"source") {
            source = xml.readElementText(QXmlStreamReader::SkipChildElements);
        }
        else if (tag == u"translation") {
            // <source> comes before <translation>, so the entry is complete here; a
            // plural message has <numerusform> children, which the loop passes over.
            const QStringView type = xml.attributes().value(QStringLiteral("type"));
            if (type != u"vanished" && type != u"obsolete") {
                entries.insert(entryKey(context, source));
            }
        }
    }
    return entries;
}

// The text of every <string> of a Qt Designer form that uic hands to tr().
QSet<QString> formStrings(const QString& path) {
    QSet<QString> strings;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return strings;
    }
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == u"string"
            && xml.attributes().value(QStringLiteral("notr")) != u"true") {
            strings.insert(xml.readElementText(QXmlStreamReader::SkipChildElements));
        }
    }
    return strings;
}

// True when the program has the string: as a <string> of the form, or as a literal
// of the C++ file.
bool programHasString(const ExpectedString& expected) {
    const QString path = sourceTreePath(QString::fromUtf8(expected.m_file));
    const QString text = QString::fromUtf8(expected.m_source);
    if (path.endsWith(QStringLiteral(".ui"))) {
        return formStrings(path).contains(text);
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return QString::fromUtf8(file.readAll()).contains('"' + text + '"');
}

} // namespace

TEST_CASE("The strings of issue 3235 are in the program", "[translation][catalog]") {
    // Keeps the list above honest: a reworded or moved string has to be updated here,
    // or the catalog check below would pass for a string nobody shows any more.
    for (const ExpectedString& expected : EXPECTED) {
        INFO(expected.m_context << ": \"" << expected.m_source << "\" in " << expected.m_file);
        CHECK(programHasString(expected));
    }
}

TEST_CASE("The maintained catalogs are found", "[translation][catalog]") {
    const QStringList names = maintainedCatalogs();

    // 49 catalogs when this test was written; a drop means src.pro changed shape and
    // the next test would pass for want of catalogs to check.
    REQUIRE(names.size() >= 40);
    for (const QString& name : names) {
        INFO(name.toStdString());
        CHECK(QFile::exists(sourceTreePath(QStringLiteral("librecad/ts/") + name)));
    }
}

TEST_CASE("Every maintained catalog lists the strings of issue 3235", "[translation][catalog]") {
    const QStringList names = maintainedCatalogs();
    REQUIRE_FALSE(names.isEmpty());

    for (const QString& name : names) {
        const QSet<QString> entries = catalogEntries(name);
        REQUIRE_FALSE(entries.isEmpty());

        QStringList missing;
        for (const ExpectedString& expected : EXPECTED) {
            const QString context = QString::fromUtf8(expected.m_context);
            const QString source = QString::fromUtf8(expected.m_source);
            if (!entries.contains(entryKey(context, source))) {
                missing << QStringLiteral("%1: \"%2\"").arg(context, source);
            }
        }
        INFO(name.toStdString() << " lacks " << missing.size() << " of "
             << std::size(EXPECTED) << " strings: " << missing.join(QStringLiteral("; ")).toStdString());
        CHECK(missing.isEmpty());
    }
}
