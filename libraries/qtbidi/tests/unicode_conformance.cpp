/*
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#include "qt_bidi.h"
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

namespace {
std::vector<int> numbers(const QString &field, int base = 10) {
    std::vector<int> values;
    for (const auto &token : field.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts))
        values.push_back(token == "x" ? -1 : token.toInt(nullptr, base));
    return values;
}

bool matches(const std::vector<int> &scalars, int direction, int base,
             const std::vector<int> &expectedLevels, const std::vector<int> &expectedOrder) {
    if (scalars.size() != expectedLevels.size()) return false;
    QString text;
    std::vector<int> units, starts;
    for (int scalar : scalars) {
        starts.push_back(text.size());
        const char32_t value = scalar;
        text += QString::fromUcs4(&value, 1);
        while (units.size() < size_t(text.size())) units.push_back(starts.size() - 1);
    }
    const auto resolved = lc::qtbidi::resolve(text, static_cast<Qt::LayoutDirection>(direction));
    if (base >= 0 && resolved.base != base) return false;
    for (size_t i = 0; i < starts.size(); ++i)
        if (expectedLevels[i] >= 0 && resolved.levels[starts[i]] != expectedLevels[i]) return false;
    std::vector<int> order;
    std::vector<bool> emitted(starts.size(), false);
    for (int unit : lc::qtbidi::reorder(resolved.levels)) {
        const int scalar = units[unit];
        if (!emitted[scalar] && expectedLevels[scalar] >= 0) order.push_back(scalar);
        emitted[scalar] = true;
    }
    return order == expectedOrder;
}

bool run(const QString &path, bool characterTests) {
    QFile input(path);
    QTextStream out(stdout);
    if (!input.open(QIODevice::ReadOnly)) {
        out << "Cannot open " << path << '\n';
        return false;
    }
    const QStringList names {"L", "R", "EN", "ES", "ET", "AN", "CS", "B", "S", "WS", "ON",
        "LRE", "LRO", "AL", "RLE", "RLO", "PDF", "NSM", "BN", "LRI", "RLI", "FSI", "PDI"};
    const std::vector<int> representatives {0x41, 0x5d0, 0x31, 0x2b, 0x24, 0x661, 0x2c, 0x2029,
        0x9, 0x20, 0x21, 0x202a, 0x202d, 0x627, 0x202b, 0x202e, 0x202c, 0x300, 0x200b,
        0x2066, 0x2067, 0x2068, 0x2069};
    QTextStream stream(&input);
    std::vector<int> levels, order;
    int lineNumber = 0, cases = 0, failures = 0;
    while (!stream.atEnd()) {
        const auto line = stream.readLine().section('#', 0, 0).trimmed();
        ++lineNumber;
        if (line.isEmpty()) continue;
        if (line.startsWith("@Levels:")) { levels = numbers(line.section(':', 1)); continue; }
        if (line.startsWith("@Reorder:")) { order = numbers(line.section(':', 1)); continue; }
        if (line.startsWith('@')) continue;
        const auto fields = line.split(';');
        auto check = [&](const std::vector<int> &scalars, int direction, int base,
                         const std::vector<int> &expectedLevels, const std::vector<int> &expectedOrder) {
            ++cases;
            if (!matches(scalars, direction, base, expectedLevels, expectedOrder)) {
                if (++failures <= 10)
                    out << "Mismatch at line " << lineNumber << ", direction " << direction << '\n';
            }
        };
        if (characterTests && fields.size() == 5) {
            check(numbers(fields[0], 16), fields[1].toInt(), fields[2].toInt(),
                  numbers(fields[3]), numbers(fields[4]));
        } else if (!characterTests && fields.size() == 2) {
            std::vector<int> scalars;
            for (const auto &name : fields[0].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
                const auto type = names.indexOf(name);
                if (type < 0) return false;
                scalars.push_back(representatives[type]);
            }
            const int bits = fields[1].trimmed().toInt(nullptr, 16);
            for (int direction = 0; direction < 3; ++direction)
                if (bits & (direction == 2 ? 1 : direction == 0 ? 2 : 4))
                    check(scalars, direction, -1, levels, order);
        } else {
            out << "Invalid corpus record at line " << lineNumber << '\n';
            return false;
        }
    }
    out << path << ": " << cases << " cases, " << failures << " failures\n";
    return cases > 0 && failures == 0;
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    if (argc != 3) {
        QTextStream(stderr) << "Usage: librecad_bidi_conformance BidiTest.txt BidiCharacterTest.txt\n";
        return 2;
    }
    const bool types = run(QString::fromLocal8Bit(argv[1]), false);
    const bool characters = run(QString::fromLocal8Bit(argv[2]), true);
    return types && characters ? 0 : 1;
}
