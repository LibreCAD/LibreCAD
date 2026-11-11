/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org
 Copyright (C) 2026 Dongxu Li (github.com/dxli)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_linfile.h"

#include <cmath>
#include <cstddef>
#include <set>
#include <utility>

#include <QChar>
#include <QStringConverter>
#include <QStringList>

#include "lc_linetypenames.h"

namespace {

// A byte order mark is metadata, not content.
const QByteArray& utf8Bom() {
    static const QByteArray bom = QByteArray::fromHex("EFBBBF");
    return bom;
}

QString decode(const QByteArray& data) {
    QByteArray bytes = data;
    if (bytes.startsWith(utf8Bom())) {
        bytes.remove(0, utf8Bom().size());
    }
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder.decode(bytes);
    // A file that is not valid UTF-8 comes from an older 8-bit code page, and
    // Latin-1 maps every byte, so no byte is lost.
    if (decoder.hasError()) {
        return QString::fromLatin1(bytes);
    }
    return text;
}

bool isPatternLine(const QStringList& fields) {
    return !fields.isEmpty() &&
           fields.first().trimmed().compare(QStringLiteral("A"), Qt::CaseInsensitive) == 0;
}

// The canonical spelling of a built-in row is LibreCAD's own, and a file does
// not replace it. An alias such as ACAD_ISO02W100 is a line type a drawing
// may hold of its own, so a file may bring it.
bool isBuiltInName(const QString& name) {
    const QString key = LC_LineTypeNames::foldName(name);
    if (key == QStringLiteral("BYLAYER") || key == QStringLiteral("BYBLOCK")) {
        return true;
    }
    if (!LC_LineTypeNames::isKnownName(name)) {
        return false;
    }
    const RS2::LineType type = LC_LineTypeNames::nameToLineType(name);
    return key ==
           LC_LineTypeNames::foldName(LC_LineTypeNames::lineTypeToName(type));
}

void appendIssue(LC_LinFile::ReadResult& result, LC_LinFile::Issue::Kind kind,
                 int line, const QString& text) {
    LC_LinFile::Issue issue;
    issue.kind = kind;
    issue.line = line;
    issue.text = text;
    result.issues.push_back(issue);
}

// The shortest of 15 or 17 significant digits that reads back to the same
// double: a drawing's own values survive a round trip, 0.1 stays 0.1.
QString formatNumber(double value) {
    if (value == 0.0) {
        return QStringLiteral("0"); // -0 has no other meaning here
    }
    QString text = QString::number(value, 'g', 15);
    bool ok = false;
    const double back = text.toDouble(&ok);
    if (!ok || back != value) {
        text = QString::number(value, 'g', 17);
    }
    return text;
}

// canonicalDashes() reports run lengths; a line type file carries the sign.
std::vector<double> signedRuns(const std::vector<double>& runs) {
    std::vector<double> pattern;
    pattern.reserve(runs.size());
    for (std::size_t i = 0; i < runs.size(); ++i) {
        pattern.push_back(i % 2 == 0 ? runs[i] : -runs[i]);
    }
    return pattern;
}

// A control character would end the line early or split the record.
QString cleanDescription(const QString& description) {
    QString clean = description;
    for (QChar& c : clean) {
        const char16_t value = c.unicode();
        if (value <= 0x1Fu || value == 0x7Fu) {
            c = QLatin1Char(' ');
        }
    }
    return clean;
}

} // namespace

LC_LinFile::ReadResult LC_LinFile::read(const QByteArray& data, double scale) {
    ReadResult result;
    // A scale that is not a positive finite number carries no unit conversion.
    const double factor = (std::isfinite(scale) && scale > 0.0) ? scale : 1.0;
    const QStringList lines = decode(data).split(QLatin1Char('\n'));

    struct Pending {
        bool valid = false;
        int line = 0;
        QString name;
        QString description;
    } pending;

    // A "*" line without its "A," line is the record that never completed.
    const auto reportMissing = [&result, &pending]() {
        if (!pending.valid) {
            return;
        }
        appendIssue(result, Issue::Kind::MissingPattern, pending.line, pending.name);
        pending.valid = false;
    };

    std::set<QString> keys;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i);
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        line = line.trimmed();
        const int lineNumber = i + 1;
        if (line.isEmpty() || line.startsWith(QLatin1Char(';'))) {
            continue;
        }
        if (line.startsWith(QLatin1Char('*'))) {
            reportMissing();
            const QString rest = line.mid(1);
            const int comma = rest.indexOf(QLatin1Char(','));
            pending.valid = true;
            pending.line = lineNumber;
            if (comma < 0) {
                pending.name = rest.trimmed();
                pending.description.clear();
            } else {
                pending.name = rest.left(comma).trimmed();
                pending.description = rest.mid(comma + 1).trimmed();
            }
            continue;
        }
        const QStringList fields = line.split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (!isPatternLine(fields)) {
            appendIssue(result, Issue::Kind::UnknownLine, lineNumber, line);
            continue;
        }
        if (!pending.valid) {
            appendIssue(result, Issue::Kind::OrphanPattern, lineNumber, line);
            continue;
        }

        // A complete record: the name is checked before the pattern, and one
        // record yields one issue.
        const Pending record = pending;
        pending.valid = false;

        if (LC_LineType::checkName(record.name) != LC_LineType::NameCheck::Ok) {
            appendIssue(result, Issue::Kind::InvalidName, record.line, record.name);
            continue;
        }
        if (isBuiltInName(record.name)) {
            appendIssue(result, Issue::Kind::BuiltInName, record.line, record.name);
            continue;
        }
        const QString key = LC_LineTypeNames::foldName(record.name);
        if (keys.find(key) != keys.end()) {
            appendIssue(result, Issue::Kind::DuplicateName, record.line, record.name);
            continue;
        }

        // A bracketed element is a text or a shape this engine cannot draw,
        // and it is not a number whatever the rest of the line holds.
        if (line.contains(QLatin1Char('['))) {
            QString element = line;
            for (const QString& field : fields) {
                if (field.contains(QLatin1Char('['))) {
                    element = field.trimmed();
                    break;
                }
            }
            appendIssue(result, Issue::Kind::ComplexElement, lineNumber, element);
            continue;
        }
        const int elementCount = static_cast<int>(fields.size()) - 1;
        if (elementCount > kMaxElements) {
            appendIssue(result, Issue::Kind::TooManyElements, record.line, record.name);
            continue;
        }

        std::vector<double> pattern;
        pattern.reserve(static_cast<std::size_t>(elementCount > 0 ? elementCount : 0));
        bool badNumber = false;
        QString badText;
        for (int j = 1; j < fields.size(); ++j) {
            const QString element = fields.at(j).trimmed();
            bool ok = false;
            // QString::toDouble() reads the C locale whatever the application's.
            const double value = element.toDouble(&ok);
            if (!ok || !std::isfinite(value)) {
                badNumber = true;
                badText = element;
                break;
            }
            pattern.push_back(value);
        }
        if (badNumber) {
            appendIssue(result, Issue::Kind::BadNumber, lineNumber, badText);
            continue;
        }
        if (!pattern.empty() && pattern.front() < 0.0) {
            appendIssue(result, Issue::Kind::FirstElementNotDash, lineNumber,
                        fields.at(1).trimmed());
            continue;
        }
        for (double& value : pattern) {
            value *= factor;
        }
        if (LC_LineTypeNames::canonicalDashes(pattern).empty()) {
            appendIssue(result, Issue::Kind::CannotDraw, record.line, record.name);
            continue;
        }

        LC_LineType entry(record.name);
        entry.description = record.description;
        entry.pattern = std::move(pattern);
        entry.origin = LC_LineType::Origin::Imported;
        entry.hasImportedRecord = false;
        keys.insert(key);
        result.entries.push_back(std::move(entry));
    }

    reportMissing();
    return result;
}

QByteArray LC_LinFile::write(const std::vector<LC_LineType>& entries,
                             std::vector<Issue>* issues) {
    QByteArray out;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const LC_LineType& entry = entries[i];
        const auto report = [issues, i](Issue::Kind kind, const QString& text) {
            if (issues == nullptr) {
                return;
            }
            Issue issue;
            issue.kind = kind;
            issue.line = static_cast<int>(i) + 1;
            issue.text = text;
            issues->push_back(issue);
        };
        const QString name = entry.name.trimmed();
        if (LC_LineType::checkName(entry.name) != LC_LineType::NameCheck::Ok) {
            report(Issue::Kind::InvalidName, name);
            continue;
        }
        // A drawing can hold an element that is not a finite number; a file
        // cannot carry one, so the canonical form is built from the finite
        // elements.
        bool hasNonFinite = false;
        std::vector<double> finite;
        finite.reserve(entry.pattern.size());
        for (const double value : entry.pattern) {
            if (std::isfinite(value)) {
                finite.push_back(value);
            } else {
                hasNonFinite = true;
            }
        }
        const std::vector<double>& source = hasNonFinite ? finite : entry.pattern;
        const std::vector<double> dashes =
            LC_LineTypeNames::canonicalDashes(source);
        if (dashes.empty()) {
            report(Issue::Kind::CannotDraw, name);
            continue;
        }
        if (entry.pattern.size() > static_cast<std::size_t>(kMaxElements)) {
            report(Issue::Kind::TooManyElements, name);
            continue;
        }
        // The format wants a dash or a dot first and no non-finite number. A
        // drawing may hold either; such a list is written as the same line
        // type started at its first dash. Any other list is written as it is.
        const bool startsWithGap =
            !entry.pattern.empty() && entry.pattern.front() < 0.0;
        std::vector<double> elements;
        if (startsWithGap || hasNonFinite) {
            elements = signedRuns(dashes);
        } else {
            elements = entry.pattern;
        }

        QString header = QStringLiteral("*") + name;
        // A description of blanks is no description, and the reader trims.
        const QString description = cleanDescription(entry.description).trimmed();
        if (!description.isEmpty()) {
            header += QLatin1Char(',');
            header += description;
        }
        out += header.toUtf8();
        out += '\n';

        QString patternLine = QStringLiteral("A");
        for (const double value : elements) {
            patternLine += QLatin1Char(',');
            patternLine += formatNumber(value);
        }
        out += patternLine.toUtf8();
        out += '\n';
    }
    return out;
}
