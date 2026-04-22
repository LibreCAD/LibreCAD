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

#include "rs_font.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <iostream>

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_fontchar.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_system.h"

class RS_FontChar;

namespace {
    // Encode a unicode character from its hexdecimal string
    // "0x20" is encoded to the character '0'
    QString charFromHex(const QString& hexCode) {
        bool okay = false;
        const char32_t ucsCode = hexCode.toUInt(&okay, 16);
        //  REPLACEMENT CHARACTER for unicode
        constexpr char32_t invalidCode = 0xFFFD;
        return (okay) ? QString::fromUcs4(&ucsCode, 1) : QString::fromUcs4(&invalidCode, 1);
    }

    // Extract the unicode char from LFF font line
    std::pair<QString, bool> extractFontChar(const QString& line) {
        // read unicode:
        static QRegularExpression regexp("[0-9A-Fa-f]{1,5}");
        const QRegularExpressionMatch match = regexp.match(line);
        if (!match.hasMatch()) {
            return {};
        }

        const QString cap = match.captured(0);
        bool okay = false;
        const std::uint32_t code = cap.toUInt(&okay, 16);

        if (!okay) {
            LC_ERR << __func__ << "() line " << __LINE__ << ": invalid font code in " << line;
            return {};
        }
        const char32_t ucsCode{static_cast<char32_t>(code)};
        return {QString::fromUcs4(&ucsCode, 1), true};
    }
}

/**
 * Constructor.
 *
 * @param fileName
 * @param owner true if the font owns the letters (blocks). Otherwise
 *              the letters will be deleted when the font is deleted.
 */
RS_Font::RS_Font(const QString& fileName, const bool owner)
    : m_letterList(owner), m_fileName(fileName), m_fileLicense("unknown") {
    m_loaded = false;
    m_letterSpacing = 3.0;
    m_wordSpacing = 6.75;
    m_lineSpacingFactor = 1.0;
    m_rawLffFontList.clear();
}

/**
 * Loads the font into memory.
 *
 * @retval true font was already loaded or is loaded now.
 * @retval false font could not be loaded.
 */
bool RS_Font::loadFont() {
    RS_DEBUG->print("RS_Font::loadFont");

    if (m_loaded) {
        return true;
    }

    QString path;

    // Search for the appropriate font if we have only the name of the font:
    if (!m_fileName.contains(".cxf", Qt::CaseInsensitive) && !m_fileName.contains(".lff", Qt::CaseInsensitive)) {
        QStringList fonts = RS_SYSTEM->getNewFontList();
        fonts.append(RS_SYSTEM->getFontList());

        for (const QString& font : std::as_const(fonts)) {
            if (QFileInfo(font).baseName().toLower() == m_fileName.toLower()) {
                path = font;
                break;
            }
        }
    }

    // We have the full path of the font:
    else {
        path = m_fileName;
    }

    // No font paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Font::loadFont: No fonts available.");
        return false;
    }

    // Open cxf file:
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        LC_LOG(RS_Debug::D_WARNING) << "RS_Font::loadFont: Cannot open font file: " << path;
        return false;
    }
    LC_LOG(RS_Debug::D_WARNING) << "RS_Font::loadFont: Successfully opened font file: " << path;
    f.close();

    if (path.contains(".cxf")) {
        readCXF(path);
    }
    if (path.contains(".lff")) {
        readLFF(path);
    }

    const RS_Block* bk = m_letterList.find(QChar(0xfffd));
    if (bk == nullptr) {
        // create new letter:
        const auto letter = new RS_FontChar(nullptr, QChar(0xfffd), RS_Vector(0.0, 0.0));
        auto* pline = new RS_Polyline(letter, RS_PolylineData());
        pline->setPen(RS_Pen(RS2::FlagInvalid));
        pline->setLayer(nullptr);
        pline->addVertex(RS_Vector(1, 0), 0);
        pline->addVertex(RS_Vector(0, 2), 0);
        pline->addVertex(RS_Vector(1, 4), 0);
        pline->addVertex(RS_Vector(2, 2), 0);
        pline->addVertex(RS_Vector(1, 0), 0);
        letter->addEntity(pline);
        letter->calculateBorders();
        m_letterList.add(letter);
    }

    m_loaded = true;

    RS_DEBUG->print("RS_Font::loadFont OK");

    return true;
}

void RS_Font::readCXF(const QString& path) {
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    QTextStream ts(&f);

    // Read line by line until we find a new letter:
    while (!ts.atEnd()) {
        QString line = ts.readLine();

        if (line.isEmpty()) {
            continue;
        }

        // Read font settings:
        if (line.at(0) == '#') {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList lst = (line.right(line.length() - 1)).split(':', Qt::SkipEmptyParts);
#else
            QStringList lst = (line.right(line.length() - 1)).split(':', QString::SkipEmptyParts);
#endif
            QStringList::Iterator it3 = lst.begin();

            // RVT_PORT sometimes it happens that the size is < 2
            if (lst.size() < 2) {
                continue;
            }

            QString identifier = it3->trimmed();
            it3++;
            QString value = it3->trimmed();

            if (identifier.toLower() == "letterspacing") {
                m_letterSpacing = value.toDouble();
            }
            else if (identifier.toLower() == "wordspacing") {
                m_wordSpacing = value.toDouble();
            }
            else if (identifier.toLower() == "linespacingfactor") {
                m_lineSpacingFactor = value.toDouble();
            }
            else if (identifier.toLower() == "author") {
                m_authors.append(value);
            }
            else if (identifier.toLower() == "name") {
                m_names.append(value);
            }
            else if (identifier.toLower() == "encoding") {
                ts.setEncoding(QStringConverter::encodingForName(value.toLatin1()).value());
                m_encoding = value;
            }
        }

        // Add another letter to this font:
        else if (line.at(0) == '[') {
            // uniode character:
            QString ch;

            // read unicode:
            QRegularExpression regexp("[0-9A-Fa-f]{4,4}");
            QRegularExpressionMatch match = regexp.match(line);
            if (match.hasMatch()) {
                ch = charFromHex(match.captured(0));
            }

            // read UTF8 (LibreCAD 1 compatibility)
            else if (line.indexOf(']') >= 3) {
                const int i = line.indexOf(']');
                QString mid = line.mid(1, i - 1);
                ch = QString::fromUtf8(mid.toLatin1()).first(1);
            }

            // read normal ascii character:
            else {
                ch = line.first(1);
            }

            // create new letter:
            const auto letter = new RS_FontChar(nullptr, ch, RS_Vector(0.0, 0.0));

            // Read entities of this letter:
            QString coordsStr;
            QStringList coords;
            QStringList::Iterator it2;
            do {
                line = ts.readLine();

                if (line.isEmpty()) {
                    continue;
                }

                coordsStr = line.right(line.length() - 2);
                //                coords = QStringList::split(',', coordsStr);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                coords = coordsStr.split(',', Qt::SkipEmptyParts);
#else
                coords = coordsStr.split(',', QString::SkipEmptyParts);
#endif
                it2 = coords.begin();

                // Line:
                if (line.at(0) == 'L') {
                    double x1 = (it2++)->toDouble();
                    double y1 = (it2++)->toDouble();
                    double x2 = (it2++)->toDouble();
                    double y2 = it2->toDouble();

                    const auto l = new RS_Line{letter, {{x1, y1}, {x2, y2}}};
                    l->setPen(RS_Pen(RS2::FlagInvalid));
                    l->setLayer(nullptr);
                    letter->addEntity(l);
                }

                // Arc:
                else if (line.at(0) == 'A') {
                    const double cx = (it2++)->toDouble();
                    const double cy = (it2++)->toDouble();
                    const double r = (it2++)->toDouble();
                    const double a1 = RS_Math::deg2rad((it2++)->toDouble());
                    const double a2 = RS_Math::deg2rad(it2->toDouble());
                    const bool reversed = line.at(1) == 'R';

                    RS_ArcData ad(RS_Vector(cx, cy), r, a1, a2, reversed);
                    const auto arc = new RS_Arc(letter, ad);
                    arc->setPen(RS_Pen(RS2::FlagInvalid));
                    arc->setLayer(nullptr);
                    letter->addEntity(arc);
                }
            }
            while (!line.isEmpty());

            if (letter->isEmpty()) {
                delete letter;
            }
            else {
                letter->calculateBorders();
                m_letterList.add(letter);
            }
        }
    }
}

QString letterNameToHexUnicodeCode(const QString& originalName) {
    QString uCode;
    uCode.setNum(originalName.at(0).unicode(), 16);
    while (uCode.length() < 4) {
        uCode = "0" + uCode;
    }
    return QString("[%1] %2").arg(uCode).arg(originalName.at(0));
}

void RS_Font::readLFF(const QString& path) {
    QFile f(path);
    m_encoding = "UTF-8";
    f.open(QIODevice::ReadOnly);
    QTextStream ts(&f);

    // Read line by line until we find a new letter:
    while (!ts.atEnd()) {
        QString line = ts.readLine();

        if (line.isEmpty()) {
            continue;
        }

        // Read font settings:
        if (line.at(0) == '#') {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList lst = line.remove(0, 1).split(':', Qt::SkipEmptyParts);
#else
            QStringList lst = line.remove(0, 1).split(':', QString::SkipEmptyParts);
#endif
            //if size is < 2 is a comentary not parameter
            if (lst.size() < 2) {
                continue;
            }

            QString identifier = lst.at(0).trimmed();
            QString value = lst.at(1).trimmed();

            if (identifier.toLower() == "letterspacing") {
                m_letterSpacing = value.toDouble();
            }
            else if (identifier.toLower() == "wordspacing") {
                m_wordSpacing = value.toDouble();
            }
            else if (identifier.toLower() == "linespacingfactor") {
                m_lineSpacingFactor = value.toDouble();
            }
            else if (identifier.toLower() == "author") {
                m_authors.append(value);
            }
            else if (identifier.toLower() == "name") {
                m_names.append(value);
            }
            else if (identifier.toLower() == "license") {
                m_fileLicense = value;
            }
            else if (identifier.toLower() == "encoding") {
                ts.setEncoding(QStringConverter::encodingForName(value.toLatin1()).value());
                m_encoding = value;
            }
            else if (identifier.toLower() == "created") {
                m_fileCreate = value;
            }
        }

        // Add another letter to this font:
        else if (line.at(0) == '[') {
            // uniode character:
            const auto [ch, okay] = extractFontChar(line);
            if (!okay) {
                LC_LOG(RS_Debug::D_WARNING) << "Ignoring code from LFF font file: " << line;
                continue;
            }

            // fixme - sand - restore char name encoding later (as name is renamed anyway later at RS_FilterLFF::fileImport
            // QString letterName = letterNameToHexUnicodeCode(ch);
            QString letterName = ch;

            QStringList fontData;
            do {
                line = ts.readLine();
                if (line.isEmpty()) {
                    break;
                }
                fontData.push_back(line);
            }
            while (true);
            if (!fontData.isEmpty() // valid data
                && !m_rawLffFontList.contains(letterName)) {
                // ignore duplicates
                m_rawLffFontList[letterName] = fontData;
            }
        }
    }
}

void RS_Font::generateAllFonts() {
    for (const QString& key : m_rawLffFontList.keys()) {
        generateLffFont(key);
    }
}

RS_Block* RS_Font::generateLffFont(const QString& key) {
    if (key.isEmpty()) {
        LC_ERR << __LINE__ << " " << __func__ << "(" << key << "): empty key";
    }

    if (!m_rawLffFontList.contains(key)) {
        LC_ERR << QString{"RS_Font::generateLffFont([%1]) : can not find the letter in LFF file %2"}.arg(key.at(0)).arg(m_fileName);
        return nullptr;
    }

    // create new letter:
    auto letter = std::make_unique<RS_FontChar>(nullptr, key, RS_Vector(0.0, 0.0)); // fixme - sand - rework, plain pointer is ok here

    // Read entities of this letter:
    QStringList fontData = m_rawLffFontList[key];

    while (!fontData.isEmpty()) {
        QString line = fontData.takeFirst();

        if (line.isEmpty()) {
            continue;
        }

        // Defined char:
        if (line.at(0) == 'C') {
            line.remove(0, 1);
            auto ch = charFromHex(line);
            if (ch == key) {
                // recursion, a character can't include itself
                const auto uCode = line.toUInt(nullptr, 16);
                LC_ERR << QString{"RS_Font::generateLffFont([%1]) : recursion, ignore this character from %2"}.arg(uCode, 4, 16).arg(
                    m_fileName);
                return nullptr;
            }

            const RS_Block* bk = m_letterList.find(ch);
            if (nullptr == bk) {
                if (!m_rawLffFontList.contains(ch)) {
                    LC_ERR << QString{"RS_Font::generateLffFont([%1]) : can not find the letter C%04X in LFF file %2"}.arg(QChar(key.at(0)))
                       .arg(m_fileName);
                    return nullptr;
                }
                generateLffFont(ch);
                bk = m_letterList.find(ch);
            }
            if (nullptr != bk) {
                RS_Entity* bkClone = bk->clone();
                bkClone->setPen(RS_Pen(RS2::FlagInvalid));
                bkClone->setLayer(nullptr);
                letter->addEntity(bkClone);
            }
        }
        //sequence:
        else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList vertex = line.split(';', Qt::SkipEmptyParts);
#else
            QStringList vertex = line.split(';', QString::SkipEmptyParts);
#endif
            //at least is required two vertex
            if (vertex.size() < 2) {
                continue;
            }
            const auto pline = new RS_Polyline(letter.get(), RS_PolylineData());
            pline->setPen(RS_Pen(RS2::FlagInvalid));
            pline->setLayer(nullptr);
            foreach(const QString& point, vertex) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                QStringList coords = point.split(',', Qt::SkipEmptyParts);
#else
                QStringList coords = point.split(',', QString::SkipEmptyParts);
#endif
                //at least X,Y is required
                const double x1 = coords.at(0).toDouble();
                // Issue #2045, if y-coordinate is missing, default to 0
                const double y1 = coords.size() >= 2 ? coords.at(1).toDouble() : 0.;
                //check presence of bulge
                double bulge = 0;
                if (coords.size() >= 3 && coords.at(2).at(0) == QChar('A')) {
                    QString bulgeStr = coords.at(2);
                    bulge = bulgeStr.remove(0, 1).toDouble();
                }
                pline->setNextBulge(bulge);
                pline->addVertex(RS_Vector(x1, y1), bulge);
            }
            letter->addEntity(pline);
        }
    }

    if (!letter->isEmpty()) {
        letter->calculateBorders();
        m_letterList.add(letter.get());
        const auto ret = letter.get();
        letter.release();
        return ret;
    }
    return nullptr;
}

RS_Block* RS_Font::findLetter(const QString& name) {
    RS_Block* ret = m_letterList.find(name);
    return (ret != nullptr) ? ret : generateLffFont(name);
}

/**
 * Dumps the fonts data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Font& f) {
    os << " Font file name: " << f.getFileName().toLatin1().data() << "\n";
    //<< (RS_BlockList&)f << "\n";
    return os;
}

unsigned RS_Font::countLetters() const {
    return m_letterList.count();
}

RS_Block* RS_Font::letterAt(const unsigned i) {
    return m_letterList.at(i);
}
