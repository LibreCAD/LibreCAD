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

#include "rs_mtext.h"
#include "lc_textbidi.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pen.h"

namespace {
// Every line is laid out at this height and scaled to the height of the text
// afterwards.
constexpr double g_layoutHeight = 9.0;
// Tab stops of a paragraph that sets none: four text heights apart.
constexpr double g_defaultTabInterval = 4.0 * g_layoutHeight;
// A tab in a right-to-left line advances by this many word spaces. Every
// release has drawn the "^I" of a DXF file as four spaces.
constexpr double g_rightToLeftTabSpaces = 4.0;
// A paragraph command that is longer than this is taken for text.
constexpr qsizetype g_maxParagraphCommandLength = 256;
// A font command or a stack that is longer than this is taken for text.
constexpr qsizetype g_maxInlineCommandLength = 256;

/**
 * Skips a decimal number such as "-3", "1.5" or ".5" at @p pos.
 */
bool skipNumber(const QString& text, qsizetype& pos, const qsizetype end) {
    qsizetype p = pos;
    if (p < end && (text.at(p) == QChar('-') || text.at(p) == QChar('+'))) {
        ++p;
    }
    int digits = 0;
    const auto skipDigits = [&] {
        while (p < end && text.at(p) >= QChar('0') && text.at(p) <= QChar('9')) {
            ++p;
            ++digits;
        }
    };
    skipDigits();
    if (p < end && text.at(p) == QChar('.')) {
        ++p;
        skipDigits();
    }
    if (digits == 0) {
        return false;
    }
    pos = p;
    return true;
}

/**
 * Skips the value of a paragraph property: a number, or "*" for the initial
 * value.
 */
bool skipPropertyValue(const QString& text, qsizetype& pos, const qsizetype end) {
    if (pos < end && text.at(pos) == QChar('*')) {
        ++pos;
        return true;
    }
    return skipNumber(text, pos, end);
}

/**
 * Finds the end of an MTEXT paragraph command: "\p", properties, ";". An
 * example is "\pi-12,l12,t12;", which AutoCAD writes for a numbered list.
 *
 * Such a command is not drawn. It is not applied either. Its values are
 * lengths: drawing units in the files AutoCAD writes, multiples of the text
 * height by other accounts, and they would have to follow the entity when it is
 * scaled. Until that is settled a line is laid out as if the command were not
 * there.
 *
 * LibreCAD users type backslashes as ordinary text, so only a command that
 * follows the grammar from its "p" to its ";" is accepted. Anything else, such
 * as the "\p" in "C:\plans\a.dxf", stays text.
 *
 * @param text the content of the MText
 * @param start the index of the "p"
 * @return the index of the closing ";", or -1 if this is not a paragraph
 * command
 */
qsizetype paragraphCommandEnd(const QString& text, const qsizetype start) {
    // The ";" is looked for nearby only: text with many "\p" and no ";" must
    // not be searched to its end each time.
    const qsizetype closing = QStringView(text).mid(start + 1, g_maxParagraphCommandLength).indexOf(QChar(';'));
    if (closing < 0) {
        return -1;
    }
    const qsizetype end = start + 1 + closing;
    // "x" alone and "t" without stops say nothing: they are more likely text.
    bool hasProperty = false;
    qsizetype pos = start + 1;
    while (pos < end) {
        const char16_t key = text.at(pos++).unicode();
        switch (key) {
            case ',': // between two properties
            case 'x': // announces an alignment or tab stops
                break;

            case 'i': // first line, on top of the left indentation
            case 'l': // left indentation
            case 'r': // right indentation
            case 'b': // space before the paragraph
            case 'a': // space after the paragraph
                if (!skipPropertyValue(text, pos, end)) {
                    return -1;
                }
                hasProperty = true;
                break;

            case 'q': // alignment: left, center, right, justified, distributed
                if (pos >= end || !QStringLiteral("lcrjd*").contains(text.at(pos))) {
                    return -1;
                }
                ++pos;
                hasProperty = true;
                break;

            case 's': // line spacing: multiple, exact, at least
                if (pos < end && QStringLiteral("mea").contains(text.at(pos))) {
                    ++pos;
                }
                if (!skipPropertyValue(text, pos, end)) {
                    return -1;
                }
                hasProperty = true;
                break;

            case 't': // tab stops: the rest of the command
                while (pos < end) {
                    const char16_t kind = text.at(pos).unicode();
                    if (kind == ',') {
                        ++pos;
                        continue;
                    }
                    if (kind == 'z' || kind == '*') {
                        // no stops
                        ++pos;
                    }
                    else {
                        if (kind == 'c' || kind == 'r' || kind == 'd') {
                            // a centered, right or decimal stop
                            ++pos;
                        }
                        if (!skipNumber(text, pos, end)) {
                            return -1;
                        }
                    }
                    hasProperty = true;
                }
                break;

            default:
                return -1;
        }
    }
    return hasProperty ? end : -1;
}

/**
 * Finds the ";" that closes the arguments of a font command, such as the one
 * of "\fArial|b0|i0|c0|p34;". The arguments are on one line and hold neither
 * a command nor a brace.
 *
 * @param text the content of the MText
 * @param start the index of the first character after the "f" or "F"
 * @return the index of the closing ";", or -1 if this is not a font command
 */
qsizetype fontCommandEnd(const QString& text, const qsizetype start) {
    const qsizetype limit = std::min(text.size(), start + g_maxInlineCommandLength);
    for (qsizetype pos = start; pos < limit; ++pos) {
        switch (text.at(pos).unicode()) {
            case ';':
                return pos;
            case '\\':
            case '{':
            case '}':
            case 0x0A:
            case 0x0D:
                return -1;
            default:
                break;
        }
    }
    return -1;
}

/**
 * Finds the end of an MTEXT command that takes one value: a height "\H2.5x;",
 * a width "\W0.8x;", a tracking "\T1.2;", an oblique angle "\Q15;", a colour
 * "\C1;" or "\c255;", an alignment "\A1;".
 *
 * As with a paragraph command, only one that follows the grammar from its
 * letter to its ";" is accepted: the "\H" of "C:\Home" stays text.
 *
 * @param text the content of the MText
 * @param start the index of the letter
 * @return the index of the closing ";", or -1 if this is not such a command
 */
qsizetype valueCommandEnd(const QString& text, const qsizetype start) {
    const qsizetype end = text.size();
    qsizetype pos = start + 1;
    switch (text.at(start).unicode()) {
        case 'H': // height
        case 'W': // width factor
        case 'T': // tracking
            if (!skipNumber(text, pos, end)) {
                return -1;
            }
            // "x": a multiple of the current value
            if (pos < end && text.at(pos) == QChar('x')) {
                ++pos;
            }
            break;

        case 'Q': // oblique angle
            if (!skipNumber(text, pos, end)) {
                return -1;
            }
            break;

        case 'C': // colour by index
        case 'c': { // colour by value
            const qsizetype digits = pos;
            while (pos < end && text.at(pos) >= QChar('0') && text.at(pos) <= QChar('9')) {
                ++pos;
            }
            if (pos == digits) {
                return -1;
            }
            break;
        }

        case 'A': // alignment in the line: bottom, middle, top
            if (pos >= end || text.at(pos) < QChar('0') || text.at(pos) > QChar('2')) {
                return -1;
            }
            ++pos;
            break;

        default:
            return -1;
    }
    return pos < end && text.at(pos) == QChar(';') ? pos : -1;
}

/**
 * Reads the stack of a "\S" command, such as "1^2;", "1/2;" or "1#2;", as the
 * text it was typed as: both parts with their separator between them. A
 * backslash makes the next character an ordinary one.
 *
 * A stack has a separator; without one, as in "\Share;", this is text.
 *
 * @param text the content of the MText
 * @param start the index of the first character after the "S"
 * @param typed receives the stack as plain text
 * @return the index of the closing ";", or -1 if this is not a stack
 */
qsizetype stackCommandEnd(const QString& text, const qsizetype start, QString& typed) {
    const qsizetype limit = std::min(text.size(), start + g_maxInlineCommandLength);
    QString stack;
    bool separated = false;
    for (qsizetype pos = start; pos < limit; ++pos) {
        char16_t c = text.at(pos).unicode();
        if (c == '\\' && pos + 1 < limit) {
            c = text.at(++pos).unicode();
        }
        else if (c == ';') {
            if (!separated) {
                return -1;
            }
            typed = stack;
            return pos;
        }
        else if (c == '^' || c == '/' || c == '#') {
            separated = true;
        }
        if (c == 0x0A || c == 0x0D) {
            return -1;
        }
        stack.append(c < 0x20 ? QChar(' ') : QChar(c));
    }
    return -1;
}

/**
 * @return the stop that a tab at @p distance from the start of the line
 * advances to. Text that ends on a stop goes to the next one.
 */
double nextTabStop(const double distance) {
    // Text that ends on a stop within rounding is on that stop.
    const double position = distance + 1e-9 * g_defaultTabInterval;
    const double stop = (std::floor(position / g_defaultTabInterval) + 1.0) * g_defaultTabInterval;
    // Beyond the precision of a double two stops cannot be told apart.
    return stop > distance ? stop : distance + g_defaultTabInterval;
}
}

std::vector<int>
RS_MText::computeBidiVisualOrder(const QString &text,
                                 Qt::LayoutDirection baseDirection) {
    return lc::textbidi::visualOrder(text, baseDirection);
}

RS_MText::LC_TextLine* RS_MText::LC_TextLine::clone() const {
    return new LC_TextLine(*this);
}

const RS_Vector& RS_MText::LC_TextLine::getTextSize() const {
    return m_textSize;
}

void RS_MText::LC_TextLine::setTextSize(const RS_Vector& v) {
    m_textSize = v;
}

const RS_Vector& RS_MText::LC_TextLine::getLeftBottomCorner() const {
    return m_leftBottomCorner;
}

void RS_MText::LC_TextLine::setLeftBottomCorner(const RS_Vector& v) {
    m_leftBottomCorner = v;
}

const RS_Vector& RS_MText::LC_TextLine::getBaselineStart() const {
    return m_baselineStart;
}

void RS_MText::LC_TextLine::setBaselineStart(const RS_Vector& v) {
    m_baselineStart = v;
}

const RS_Vector& RS_MText::LC_TextLine::getBaselineEnd() const {
    return m_baselineEnd;
}

void RS_MText::LC_TextLine::setBaselineEnd(const RS_Vector& v) {
    m_baselineEnd = v;
}

void RS_MText::LC_TextLine::moveBaseline(const RS_Vector& offset) {
    m_baselineStart.move(offset);
    m_baselineEnd.move(offset);
    m_leftBottomCorner.move(offset);
}

RS_MTextData::RS_MTextData(const RS_Vector& insertionPoint, const double height, const double width, const VAlign valign,
                           const HAlign halign, const MTextDrawingDirection drawingDirection,
                           const MTextLineSpacingStyle lineSpacingStyle, const double lineSpacingFactor, const QString& text,
                           const QString& style, const double angle, const RS2::UpdateMode updateMode)
    : insertionPoint(insertionPoint), height(height), width(width), valign(valign), halign(halign),
      drawingDirection(drawingDirection), lineSpacingStyle(lineSpacingStyle), lineSpacingFactor(lineSpacingFactor), text(text),
      style(style), angle(angle), updateMode(updateMode) {
}

std::ostream& operator<<(std::ostream& os, const RS_MTextData& td) {
    os << "(" << td.insertionPoint << ',' << td.height << ',' << td.width << ',' << td.valign << ',' << td.halign << ',' << td.
        drawingDirection << ',' << td.lineSpacingStyle << ',' << td.lineSpacingFactor << ',' << td.text.toLatin1().data() << ',' << td.style
       .toLatin1().data() << ',' << td.angle << ',' << td.updateMode << ',' << ")";
    return os;
}

/**
 * Constructor.
 */
RS_MText::RS_MText(RS_EntityContainer* parent, const RS_MTextData& d)
    : RS_EntityContainer(parent), m_data(d) {
    setStyle(m_data.style);
    applyText(m_data.text, true);
}

RS_Entity* RS_MText::clone() const {
    return new RS_MText(*this);
}

// fixme - test concept for using UI proxies for heavy entities on modification operation (rotate, scale etc).
// potentially, it might be either expanded further or removed.
class RS_MTextProxy : public RS_EntityContainer {
public:
    explicit RS_MTextProxy(const RS_MText& parent) : RS_EntityContainer(parent) {
    }
};

RS_Entity* RS_MText::cloneProxy() const {
    auto* proxy = new RS_EntityContainer();
    proxy->setOwner(true);
    for (RS_Entity* entity : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr && line->hasValidBorders()) {
            const RS_Vector& start = line->getBaselineStart();
            const RS_Vector& end = line->getBaselineEnd();
            const auto l = new RS_Line(proxy, start, end);
            proxy->addEntity(l);
        }
    }
    proxy->forcedCalculateBorders();
    return proxy;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_MText::setText(QString t) {
    applyText(t, false);
}

LC_TextEntityState::Error RS_MText::applyText(const QString &input, bool constructor,
                                             const LC_TextEntityState *preparedState) {
    const QString original = input.isNull() ? QString{} : QString(input.constData(), input.size());
    const bool unchanged = !preparedState && m_data.textState.initialized() && original == m_data.textState.compatibilityText();
    auto text = original;
    auto valign = m_data.valign;

    // handle some special flags embedded in the text:
    if (!unchanged && text.left(4) == R"(\A0;)") {
        text = text.mid(4);
        valign = RS_MTextData::VABottom;
    }
    else if (!unchanged && text.left(4) == R"(\A1;)") {
        text = text.mid(4);
        valign = RS_MTextData::VAMiddle;
    }
    else if (!unchanged && text.left(4) == R"(\A2;)") {
        text = text.mid(4);
        valign = RS_MTextData::VATop;
    }
    auto next = preparedState ? *preparedState : m_data.textState;
    if (!preparedState) {
        auto failure = next.acceptLegacyText(original, text, LC_TextDocument::Syntax::MText, constructor);
        if (failure) {
            m_textStateError = failure;
            return failure;
        }
    }
    m_data.text = std::move(text);
    m_data.valign = valign;
    m_data.textState = std::move(next);
    m_textStateError = {};

    if (m_data.updateMode == RS2::Update) {
        update();
    }
    return {};
}

LC_TextEntityState::Error RS_MText::importText(const LC_TextEntityState::Import &input, const QString &compatibilityInput) {
    if (input.convention.syntax != LC_TextDocument::Syntax::MText)
        return {LC_TextEntityState::ErrorCode::InvalidConvention, QStringLiteral("MTEXT requires an MTEXT source convention")};
    auto source = input;
    if (!source.compatibilityStyle) source.compatibilityStyle = m_data.style;
    auto next = m_data.textState;
    auto failure = next.initializeImport(source, compatibilityInput);
    if (failure) return failure;
    const auto finalText = next.compatibilityText();
    failure = next.acceptLegacyText(compatibilityInput, finalText, LC_TextDocument::Syntax::MText, true);
    if (failure) return failure;
    return applyText(compatibilityInput, false, &next);
}

LC_TextEntityState::Error RS_MText::setCanonicalText(const QString &source, const QString &compatibilityInput,
                                                    const LC_TextEntityState::Convention &convention) {
    if (convention.syntax != LC_TextDocument::Syntax::MText)
        return {LC_TextEntityState::ErrorCode::InvalidConvention, QStringLiteral("MTEXT requires an MTEXT source convention")};
    auto next = m_data.textState;
    const auto failure = next.editSource(source, compatibilityInput, convention);
    if (failure) return failure;
    return applyText(compatibilityInput, false, &next);
}

void RS_MText::setDrawingDirection(RS_MTextData::MTextDrawingDirection direction) {
    if (m_data.drawingDirection == direction) {
        return;
    }
    m_data.drawingDirection = direction;
    m_data.legacyRtlLayout = false;
    if (m_data.updateMode == RS2::Update) { // fixme - why update is only on some setters?
        update();
    }
}

/**
 * Gets the alignment as an int.
 *
 * @return  1: top left ... 9: bottom right
 */
int RS_MText::getAlignment() const {
    if (m_data.valign == RS_MTextData::VATop) {
        if (m_data.halign == RS_MTextData::HALeft) {
            return 1;
        }
        if (m_data.halign == RS_MTextData::HACenter) {
            return 2;
        }
        if (m_data.halign == RS_MTextData::HARight) {
            return 3;
        }
    }
    else if (m_data.valign == RS_MTextData::VAMiddle) {
        if (m_data.halign == RS_MTextData::HALeft) {
            return 4;
        }
        if (m_data.halign == RS_MTextData::HACenter) {
            return 5;
        }
        if (m_data.halign == RS_MTextData::HARight) {
            return 6;
        }
    }
    else if (m_data.valign == RS_MTextData::VABottom) {
        if (m_data.halign == RS_MTextData::HALeft) {
            return 7;
        }
        if (m_data.halign == RS_MTextData::HACenter) {
            return 8;
        }
        if (m_data.halign == RS_MTextData::HARight) {
            return 9;
        }
    }

    return 1;
}

/**
 * Sets the alignment from an int.
 *
 * @param a 1: top left ... 9: bottom right
 */
void RS_MText::setAlignment(const int a) {
    switch (a % 3) {
        case 1:
            m_data.halign = RS_MTextData::HALeft;
            break;
        case 2:
            m_data.halign = RS_MTextData::HACenter;
            break;
        case 0:
            m_data.halign = RS_MTextData::HARight;
            break;
        default:
            m_data.halign = RS_MTextData::HALeft;
            break;
    }

    const int valign = static_cast<int>(ceil(a / 3.0));
    switch (valign) {
        case 1:
            m_data.valign = RS_MTextData::VATop;
            break;
        case 2:
            m_data.valign = RS_MTextData::VAMiddle;
            break;
        case 3:
            m_data.valign = RS_MTextData::VABottom;
            break;
        default:
            m_data.valign = RS_MTextData::VATop;
            break;
    }
}

/**
 * @return Number of lines in this text entity.
 */
int RS_MText::getNumberOfLines() const {
    return 1 + std::count_if(m_data.text.cbegin(), m_data.text.cend(), [](QChar c) {
        return c.unicode() == 0xA;
    });
}

/**
 * The content of an MText on one line, without its codes.
 *
 * @p text is MTEXT content the way RS_FilterDXFRW::toNativeString() leaves it:
 * line breaks, tabs and the special characters are the characters themselves,
 * every other code is still spelled out. What is typed in LibreCAD is taken
 * the same way, with "\P", "\~" and "%%d" still as codes.
 */
QString RS_MText::toPlainLine(const QString& text) {
    QString plain;
    plain.reserve(text.size());
    // A run of line breaks is one space, and only between two pieces of text.
    bool lineBreak = false;
    const auto append = [&plain, &lineBreak](const QChar c) {
        if (lineBreak && !plain.isEmpty()) {
            plain.append(QChar(' '));
        }
        lineBreak = false;
        plain.append(c);
    };

    const qsizetype size = text.size();
    for (qsizetype i = 0; i < size; ++i) {
        const char16_t c = text.at(i).unicode();
        if (c == 0x0A || c == 0x0D) {
            lineBreak = true;
            continue;
        }
        if (c < 0x20) {
            // a tab, or a control character that a line has no place for
            append(QChar(' '));
            continue;
        }
        if (c == '{' || c == '}') {
            // the scope of the commands inside
            continue;
        }
        if (c == '%' && i + 2 < size && text.at(i + 1) == QChar('%')) {
            // the codes RS_FilterDXFRW::toNativeString() decodes, typed here
            char16_t special = 0;
            switch (text.at(i + 2).toLower().unicode()) {
                case 'c':
                    special = 0x2300;
                    break;
                case 'd':
                    special = 0x00B0;
                    break;
                case 'p':
                    special = 0x00B1;
                    break;
                default:
                    break;
            }
            if (special != 0) {
                append(QChar(special));
                i += 2;
                continue;
            }
        }
        if (c != '\\') {
            append(QChar(c));
            continue;
        }
        if (i + 1 >= size) {
            // a backslash that ends the text is not drawn either
            break;
        }

        qsizetype commandEnd = -1;
        const char16_t command = text.at(i + 1).unicode();
        switch (command) {
            case '\\':
            case '{':
            case '}':
                append(QChar(command));
                commandEnd = i + 1;
                break;

            case 0x0A:
                // "\\P" in a file is a backslash and a "P"; toNativeString()
                // took the "\P" in it for a line break
                append(QChar('\\'));
                append(QChar('P'));
                commandEnd = i + 1;
                break;

            case 'P': // new paragraph
            case 'N': // new column
                lineBreak = true;
                commandEnd = i + 1;
                break;

            case '~': // a space that does not break
                append(QChar(' '));
                commandEnd = i + 1;
                break;

            case 'L': // underline
            case 'l':
            case 'O': // overline
            case 'o':
            case 'K': // strike through
            case 'k':
            case 'X': // wrap at the dimension line
                commandEnd = i + 1;
                break;

            case 'S': {
                QString typed;
                commandEnd = stackCommandEnd(text, i + 2, typed);
                if (commandEnd >= 0) {
                    for (const QChar part : typed) {
                        append(part);
                    }
                }
                break;
            }

            case 'f':
                if (i + 2 < size && text.at(i + 2) == QChar('{')) {
                    // "\f{name}": the font change of update()
                    const qsizetype closing =
                        QStringView(text).mid(i + 3, g_maxInlineCommandLength).indexOf(QChar('}'));
                    commandEnd = closing < 0 ? i + 2 : i + 3 + closing;
                    break;
                }
                [[fallthrough]];
            case 'F':
                commandEnd = fontCommandEnd(text, i + 2);
                break;

            case 'H':
            case 'W':
            case 'T':
            case 'Q':
            case 'C':
            case 'c':
            case 'A':
                commandEnd = valueCommandEnd(text, i + 1);
                break;

            case 'p':
                commandEnd = paragraphCommandEnd(text, i + 1);
                break;

            default:
                break;
        }
        if (commandEnd < 0) {
            // not a command: the backslash is text, and so is what follows it
            append(QChar('\\'));
            continue;
        }
        i = commandEnd;
    }
    return plain;
}

/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's m_data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_MText::update() {
    RS_DEBUG->print("RS_MText::update");

    clear();
    if (isDeleted()) {
        return;
    }

    m_usedTextWidth = 0.0;
    m_usedTextHeight = 0.0;
    m_hasParagraphCommands = false;

    RS_Font* font{RS_FONTLIST->requestFont(m_data.style)};
    if (nullptr == font) {
        return;
    }

    RS_Vector letterPos{0.0, -9.0};
    const RS_Vector letterSpace{font->getLetterSpacing(), 0.0};
    const double spaceWidth = font->getWordSpacing();

  int lineCounter{0};

    // Every single text line gets stored in this entity container
    // so we can move the whole line around easily:
    auto oneLine{new LC_TextLine(this)};

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // Logical-order pass: process MText format codes and accumulate per-line
  // segments. Each line's segments are then bidi-reordered and emitted via
  // flushBidiLine(); see the comment on LC_BidiSegment in the header.
  std::vector<LC_BidiSegment> segments;

  auto closeLine = [&]() {
    const double leadingAdvance = flushBidiLine(*oneLine, segments, letterSpace, letterPos);
    updateAddLine(oneLine, lineCounter++, leadingAdvance);
    oneLine = new LC_TextLine(this);
    letterPos = RS_Vector(0.0, -9.0);
  };
    for (decltype(m_data.text.length()) i = 0; i < m_data.text.length(); ++i) {
        auto remaining = m_data.text.mid(i);
        // Handle \F not followed by {<codePage>}
        if (remaining.startsWith(R"(\F)") && remaining.indexOf(R"(^\\[Ff]\{[\d\w]*\})") != 0) {
            segments.push_back(
          {LC_BidiSegment::Char, m_data.text.at(i), font, 0.0, {}, {}});
            continue;
        }
        if (remaining.startsWith(R"(\\)")) {
            // Allow escape '\', needed to support "\S" and "\P" in string
            // "\S" is used for super/subscripts
            // "\P" is used to start a new line
            // "\\S" and "\\P" to get literal strings "\S" and "\P"
            segments.push_back(
          {LC_BidiSegment::Char, m_data.text.at(i++), font, 0.0, {}, {}});
            continue;
        }

        switch (m_data.text.at(i).unicode()) {
            case 0x0A:
                // line feed:
                closeLine();
                break;

            case 0x20:
                // Space:
                segments.push_back(
          {LC_BidiSegment::Space, QChar(' '), nullptr, spaceWidth, {}, {}});
                break;

            case 0x09:
                // Tab: advances to the next tab stop, see flushBidiLine()
                segments.push_back(
          {LC_BidiSegment::Tab, QChar('\t'), nullptr, g_rightToLeftTabSpaces * spaceWidth, {}, {}});
                break;

            case 0x5C: {
                bool handled{false};
                // code (e.g. \S, \P, ..)
                ++i;
                if (static_cast<int>(m_data.text.length()) <= i) {
                    continue;
                }
      // `i` is a UTF-16 code-unit index into data.text; index the QString
      // directly. The previous `data.text.toUcs4().at(i)` indexed a UCS-4
      // array with a UTF-16 offset — out of bounds / wrong codepoint once any
      // non-BMP (surrogate-pair) char precedes `i`. All inner cases below are
      // ASCII ('P','f','F','S'), so a BMP code unit is sufficient and a
      // non-BMP unit correctly falls to default (--i).
      char16_t ch{m_data.text.at(i).unicode()};
                switch (ch) {
                    case 'P':
                        closeLine();
                        handled = true;
                        break;

                    case 'p': {
                        // paragraph properties such as "\pi-12,l12,t12;": not drawn
                        const qsizetype commandEnd = paragraphCommandEnd(m_data.text, i);
                        if (commandEnd < 0) {
                            // not a command: drawn like any other text
                            --i;
                            break;
                        }
                        i = commandEnd;
                        m_hasParagraphCommands = true;
                        handled = true;
                        break;
                    } // inner case 'p'

                    case 'f':
                    case 'F': {
                        // font change
                        //  \f{symbol} changes font to symbol
                        //  \f{} sets font to standard
                        ++i;
                        if (i >= m_data.text.size() || '{' != m_data.text.at(i).unicode()) {
                            --i;
                            continue;
                        }

                        const qsizetype j{m_data.text.indexOf('}', i)};
                        if (j > i) {
                            QString fontName = m_data.text.mid(i + 1, j - i - 1);

                            const auto requestedFont = RS_FONTLIST->requestFont(fontName);
                            if (requestedFont != nullptr) {
                                font = requestedFont;
                            }
                            else {
                                font = RS_FONTLIST->requestFont("standard");
                            }
                            i = j;
                        }
                        continue;
                    } // inner case 'f','F'

                    case 'S': {
                        QString upperText;
                        QString lowerText;

                        // get upper string:
                        ++i; // fixme - sand - rework cycles
                        while (static_cast<int>(m_data.text.length()) > i && m_data.text.at(i).unicode() != '^' && m_data.text.at(i).unicode() !=
                            '\\') {
                            upperText += m_data.text.at(i);
                            ++i;
                        }

                        ++i;
                        if (static_cast<int>(m_data.text.length()) > i && '^' == m_data.text.at(i - 1).unicode() && ' ' == m_data.text.at(i).
                            unicode()) {
                            ++i;
                        }

                        // get lower string:
                        while (static_cast<int>(m_data.text.length()) > i && ';' != m_data.text.at(i).unicode()) {
                            lowerText += m_data.text.at(i);
                            ++i;
                        }

        LC_BidiSegment stack;
        stack.kind = LC_BidiSegment::Stack;
        stack.upperText = std::move(upperText);
        stack.lowerText = std::move(lowerText);
        segments.push_back(std::move(stack));
        handled = true;

                        break;
                    } // inner case 'S'

                    default:
                        --i;
                        break;
                } // inner switch (ch)

                if (handled) {
                    break;
                }
            } // outer case 0x5C

            // if char is not handled
            // fall-through
            default: {
                // One Letter:
                segments.push_back(
          {LC_BidiSegment::Char, m_data.text.at(i), font, 0.0, {}, {}});
                break;
            } // outer default
        } // outer switch (m_data.text.at(i).unicode())
    } // for (i) loop

  // Flush any trailing segments belonging to the last line.
  const double leadingAdvance = flushBidiLine(*oneLine, segments, letterSpace, letterPos);

  m_usedTextHeight -=
          m_data.height * m_data.lineSpacingFactor * 5.0 / 3.0 - m_data.height;

    updateAddLine(oneLine, lineCounter, leadingAdvance);

    alignVertically();
    RS_DEBUG->print("RS_MText::update: OK");
}

/**
 * Reorder @p segments via the Unicode Bidirectional Algorithm and emit each
 * one (letter / space / tab / super-sub stack) into @p oneLine in visual L→R
 * order. Cursor advances use LFF metrics; bidi only determines visual order.
 * Clears @p segments on return.
 *
 * @return for a line that flows from the left, the advance of the spaces and
 * tabs in front of its first ink. Zero for a right-to-left line, whose blanks
 * on the left are the trailing ones, and for a line without ink.
 */
double RS_MText::flushBidiLine(LC_TextLine &oneLine,
                               std::vector<LC_BidiSegment> &segments,
                               const RS_Vector &letterSpace,
                               RS_Vector &letterPosition) {
  if (segments.empty()) {
    return 0.0;
  }

  // Build a plain-text string for the bidi pass. Stacks become U+FFFC (Object
  // Replacement Character), which has bidi class ON (Other Neutral) so it
  // gets resolved to the surrounding context like punctuation.
  QString plainText;
  plainText.reserve(static_cast<int>(segments.size()));
  for (const auto &seg : segments) {
    switch (seg.kind) {
    case LC_BidiSegment::Char:
      plainText.append(seg.codepoint);
      break;
    case LC_BidiSegment::Space:
      plainText.append(QChar(' '));
      break;
    case LC_BidiSegment::Tab:
      // One code unit, like every other segment: a cluster index is a segment
      // index. A tab is a segment separator for bidi, so the fields on its two
      // sides keep their logical order along the direction of the line.
      plainText.append(QChar('\t'));
      break;
    case LC_BidiSegment::Stack:
      plainText.append(QChar(0xFFFC));
      break;
    }
  }

  const auto direction = m_data.drawingDirection == RS_MTextData::RightToLeft
      ? Qt::RightToLeft : Qt::LeftToRight;
  // A line flows from the left unless the text is right-to-left. Only such a
  // line is indented and has tab stops: in a right-to-left line the blanks on
  // the visual left are the trailing ones, and a stop would have to be measured
  // from the far end of the line.
  const bool flowsFromLeft = direction == Qt::LeftToRight;
  const bool leftAttached = m_data.halign != RS_MTextData::HACenter
      && m_data.halign != RS_MTextData::HARight;

  // The ink laid out so far, and the advance of the spaces and tabs in front
  // of it. A stack without ink moves the pen as well, but is no indentation.
  bool hasInk = false;
  double inkMin = 0.0;
  double inkMax = 0.0;
  double leadingAdvance = 0.0;
  // Where the edge that tab stops are measured from is, as the pen sees it.
  // A left-attached line ends up with its ink the leading advance from the
  // insertion point (see updateAddLine()), so that is where its stops count
  // from. A centered or right-attached line is anchored on its ink, and its
  // leading blanks are without effect: its stops count from its first ink.
  const auto stopOrigin = [&]() {
    if (!hasInk) {
      return letterPosition.x - leadingAdvance;
    }
    return leftAttached ? inkMin - leadingAdvance : inkMin;
  };
  // The ink behind a tab starts on the stop exactly, as the ink of a line
  // starts on the edge of the line: the left bearing of a letter shifts
  // neither. Otherwise the same stop would be at different places in two lines
  // that begin with different letters. What a tab opens is a field: its first
  // child, where its ink belongs, and its ink so far.
  bool tabPending = false;
  bool inTabField = false;
  unsigned fieldFirst = 0;
  double fieldStart = 0.0;
  double fieldInkMin = 0.0;
  double fieldInkMax = 0.0;
  double inkMaxBeforeField = 0.0;

  for (const auto &cluster : lc::textbidi::visualClusters(
           plainText, direction,
           m_data.legacyRtlLayout && direction == Qt::RightToLeft)) {
    RS_Vector clusterOrigin = letterPosition;
    for (int offset = 0; offset < cluster.length; ++offset) {
      const int logIdx = cluster.start + offset;
      const auto &seg = segments[logIdx];
      const unsigned firstAdded = oneLine.count();
      switch (seg.kind) {
      case LC_BidiSegment::Char: {
        char32_t scalar = seg.codepoint.unicode();
        if (seg.codepoint.isHighSurrogate() &&
            offset + 1 < cluster.length &&
            segments[logIdx + 1].codepoint.isLowSurrogate()) {
          scalar = QChar::surrogateToUcs4(
              seg.codepoint, segments[logIdx + 1].codepoint);
          ++offset;
        }
        const auto category = QChar::category(scalar);
        if (category == QChar::Other_Format ||
            category == QChar::Other_Control) {
          break;
        }
        RS_Font *segFont = seg.font;
        if (segFont == nullptr) {
          // Fallback: should not normally happen, but stay defensive.
          segFont = RS_FONTLIST->requestFont(m_data.style);
        }
        if (segFont != nullptr) {
          if (cluster.rightToLeft)
            scalar = QChar::mirroredChar(scalar);
          const auto glyph = QString::fromUcs4(&scalar, 1);
          if (category == QChar::Mark_NonSpacing ||
              category == QChar::Mark_Enclosing) {
            auto markPosition = clusterOrigin;
            addLetter(oneLine, glyph, *segFont, letterSpace, markPosition);
          } else {
            addLetter(oneLine, glyph, *segFont, letterSpace, letterPosition);
          }
        }
        break;
      }
      case LC_BidiSegment::Space:
        letterPosition.x += seg.width;
        if (!hasInk) {
          leadingAdvance += seg.width;
        } else if (tabPending) {
          fieldStart += seg.width;
        }
        break;
      case LC_BidiSegment::Tab: {
        if (!flowsFromLeft) {
          letterPosition.x += seg.width;
          break;
        }
        const double origin = stopOrigin();
        // The pen of a font without letter spacing can be left of the ink.
        const double reached = hasInk ? std::max(letterPosition.x, inkMax) : letterPosition.x;
        const double stop = nextTabStop(reached - origin);
        if (!std::isfinite(stop)) {
          break;
        }
        letterPosition.x = stop + origin;
        if (!hasInk) {
          // in front of all ink the tab is part of the indentation of the line
          leadingAdvance = stop;
        } else {
          tabPending = true;
          inTabField = false;
          fieldFirst = oneLine.count();
          fieldStart = letterPosition.x;
          inkMaxBeforeField = inkMax;
        }
        break;
      }
      case LC_BidiSegment::Stack: {
        // The parts are measured from where they start, so that blanks in front
        // of a numerator widen the stack rather than push it into what follows.
        double upperWidth = 0.0;
        if (!seg.upperText.isEmpty()) {
          RS_MText *upper = createUpperLower(
              seg.upperText, m_data, letterPosition + RS_Vector{0., 9.});
          oneLine.addEntity(upper);
          upper->reparent(&oneLine);
          upperWidth = upper->getUsedTextWidth();
        }
        double lowerWidth = 0.0;
        if (!seg.lowerText.isEmpty()) {
          RS_MText *lower = createUpperLower(
              seg.lowerText, m_data, letterPosition + RS_Vector{0., 4.});
          oneLine.addEntity(lower);
          lower->reparent(&oneLine);
          lowerWidth = lower->getUsedTextWidth();
        }
        letterPosition.x += std::max(upperWidth, lowerWidth);
        letterPosition += letterSpace;
        break;
      }
      }

      // The ink of what this segment added to the line.
      bool addedInk = false;
      double addedMin = 0.0;
      double addedMax = 0.0;
      for (unsigned index = firstAdded; index < oneLine.count(); ++index) {
        const RS_Entity *added = oneLine.entityAt(static_cast<int>(index));
        if (added == nullptr || !added->hasValidBorders()) {
          continue;
        }
        addedMin = addedInk ? std::min(addedMin, added->getMin().x) : added->getMin().x;
        addedMax = addedInk ? std::max(addedMax, added->getMax().x) : added->getMax().x;
        addedInk = true;
      }
      if (!addedInk) {
        continue;
      }
      if (tabPending) {
        // the first ink of the field
        tabPending = false;
        inTabField = true;
        fieldInkMin = addedMin;
        fieldInkMax = addedMax;
      } else if (inTabField) {
        fieldInkMin = std::min(fieldInkMin, addedMin);
        fieldInkMax = std::max(fieldInkMax, addedMax);
      }
      if (inTabField && fieldInkMin != fieldStart) {
        // Put the leftmost ink of the field on its start. That is the bearing
        // of its first letter, and more when a later letter reaches further
        // left, which happens in a font whose spacing is all in its bearings.
        const RS_Vector shift{fieldStart - fieldInkMin, 0.0};
        for (unsigned index = fieldFirst; index < oneLine.count(); ++index) {
          if (RS_Entity *placed = oneLine.entityAt(static_cast<int>(index))) {
            placed->move(shift);
          }
        }
        letterPosition.x += shift.x;
        clusterOrigin.x += shift.x;
        addedMin += shift.x;
        addedMax += shift.x;
        fieldInkMin = fieldStart;
        fieldInkMax += shift.x;
      }
      if (!hasInk) {
        hasInk = true;
        inkMin = addedMin;
        inkMax = addedMax;
      } else if (inTabField) {
        // a field is right of everything in front of its tab
        inkMax = std::max(inkMaxBeforeField, fieldInkMax);
      } else {
        inkMin = std::min(inkMin, addedMin);
        inkMax = std::max(inkMax, addedMax);
      }
    }
  }

  segments.clear();
  return flowsFromLeft && hasInk ? leadingAdvance : 0.0;
}

void RS_MText::alignVertically() {
    // Vertical Align:
    switch (m_data.valign) {
        case RS_MTextData::VATop: {
            // no change
            break;
        }
        case RS_MTextData::VAMiddle: {
            RS_EntityContainer::move({0., 0.5 * m_usedTextHeight});
            break;
        }
        case RS_MTextData::VABottom: {
            // adjust corners for text lines
            RS_EntityContainer::move({0., m_usedTextHeight});
            break;
        }
        default: LC_ERR << __func__ << "(): line " << __LINE__ << ": invalid Invalid RS_MText::VAlign=" << m_data.valign;
            break;
    }
    for (RS_Entity* e : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(e);
        if (line != nullptr && line->hasValidBorders()) {
            auto corner = RS_Vector(line->getMin().rotate(m_data.insertionPoint, m_data.angle));
            RS_Vector size = line->getSize();
            line->setLeftBottomCorner(corner);
            line->setTextSize(size);
        }
    }
    rotateLinesRefs();

    RS_EntityContainer::rotate(m_data.insertionPoint, m_data.angle);

    forcedCalculateBorders();
}

void RS_MText::rotateLinesRefs() const {
    for (RS_Entity* e : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(e);
        if (line != nullptr && line->hasValidBorders()) {
            RS_Vector corner = line->getLeftBottomCorner();
            const RS_Vector size = line->getTextSize();
            RS_Vector shiftVector(size.y / 2, 0);
            shiftVector.rotate(m_data.angle + M_PI_2);

            RS_Vector baselineStart = corner + shiftVector;
            RS_Vector baselineEnd = corner + shiftVector + RS_Vector::polar(size.x, m_data.angle);

            //            baselineStart.rotate(m_data.insertionPoint, m_data.angle);
            line->setBaselineStart(baselineStart);

            //            baselineEnd.rotate(m_data.insertionPoint, m_data.angle);
            line->setBaselineEnd(baselineEnd);
        }
    }
}

/**
 * Used internally by update() to add a letter to one line
 *
 * @param oneLine
 * @param letter
 * @param font
 * @param letterSpace
 * @param letterPosition
 * @param oneLine the current entity container
 * @param letter the letter to add
 * @param font the font to use
 * @param letterSpace the letter width to use
 * @param letterPosition the current letter position; will be updated
 * after addition
 *
 */
void RS_MText::addLetter(LC_TextLine& oneLine, QString letterText, RS_Font& font, const RS_Vector& letterSpace, RS_Vector& letterPosition) {
    if (nullptr == font.findLetter(letterText)) {
        RS_DEBUG->print("RS_MText::update: missing font for letter( %s ), replaced " "it with QChar(0xfffd)", qPrintable(letterText));
        letterText = QChar(0xfffd);
    }

    LC_LOG << "RS_MText::update: insert a letter at pos:(" << letterPosition.x << ", " << letterPosition.y << ")";

    const RS_InsertData d(letterText, letterPosition, RS_Vector(1.0, 1.0), 0.0, 1, 1,
                    RS_Vector(0.0, 0.0), font.getLetterList(), RS2::NoUpdate);

    const auto letterEntity{new RS_Insert(this, d)};
    letterEntity->setPen(RS_Pen(RS2::FlagInvalid));
    letterEntity->setLayer(nullptr);
    letterEntity->update();
    letterEntity->forcedCalculateBorders();

    // Add spacing, if the font is actually wider than word spacing
    double actualWidth = letterEntity->getMax().x - letterEntity->getMin().x;
    if (actualWidth >= font.getWordSpacing() + RS_TOLERANCE) {
      const double letterSpacing = std::max(1., letterSpace.x);
      const double wordSpacing = font.getWordSpacing();
        actualWidth = wordSpacing + letterSpacing - std::fmod(actualWidth - wordSpacing, letterSpacing);
    }
    LC_LOG << __LINE__ << ": actualWidth: " << actualWidth;

    letterPosition.x += actualWidth;

    oneLine.addEntity(letterEntity);

    // next letter position:
    letterPosition += letterSpace;
}

RS_MText* RS_MText::createUpperLower(QString text, const RS_MTextData& data, const RS_Vector& position) {
    RS_MTextData childData(position, 4.0, 100.0, RS_MTextData::VATop,
                          RS_MTextData::HALeft, data.drawingDirection,
                          RS_MTextData::Exact, 1.0, text, data.style, 0.0,
                          RS2::Update);
    childData.legacyRtlLayout = data.legacyRtlLayout;
    auto* line = new RS_MText(nullptr, childData);
    line->setLayer(nullptr);
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->calculateBorders();
    return line;
}

/**
 * Used internally by update() to add a text line created with
 * default values and alignment to this text container.
 *
 * @param textLine The text line.
 * @param lineCounter Line number.
 * @param leadingAdvance What flushBidiLine() returned for the line: how far
 * its first ink is from the edge a left-attached line starts at, in the units
 * the line was laid out in.
 *
 * @return  distance over the text base-line
 */
double RS_MText::updateAddLine(LC_TextLine* textLine, const int lineCounter, const double leadingAdvance) {
    constexpr double ls = 5.0 / 3.0;

    RS_DEBUG->print("RS_MText::updateAddLine: width: %f", textLine->getSize().x);

    // textLine->forcedCalculateBorders();
    // RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f",
    // textLine->getSize().x);

    // Scale:
    const double scale = m_data.height / 9.0;
    textLine->scale(RS_Vector{0., 0.}, RS_Vector(scale, scale));

    textLine->forcedCalculateBorders();

    // Width the line uses, measured over its ink.
    double lineWidth = textLine->getSize().x;

    // Horizontal Align:
    if (textLine->hasValidBorders()) {
      switch (m_data.halign) {
        case RS_MTextData::HACenter:
            textLine->move(RS_Vector{-0.5 * (textLine->getMin().x + textLine->getMax().x), 0.});
            break;

        case RS_MTextData::HARight:
            textLine->move(RS_Vector{-textLine->getMax().x, 0.});
            break;

        default:
            // The ink of an unindented line starts at the insertion point,
            // whatever the left bearing of its first letter is. Indentation,
            // leading spaces and leading tabs move the ink on from there, and
            // count for the width. A line without ink has nothing to move.
            if (textLine->hasValidBorders()) {
                const double indentation = std::abs(scale) * leadingAdvance;
                textLine->move(RS_Vector{indentation - textLine->getMin().x, 0.});
                lineWidth += indentation;
            }
            break;
        }
    }

    // Update actual text size (before rotating, after scaling!):
    if (lineWidth > m_usedTextWidth) {
        m_usedTextWidth = lineWidth;
    }

    m_usedTextHeight += m_data.height * m_data.lineSpacingFactor * ls;

    // Gets the distance over text base-line (before rotating, after scaling!):
    const double textTail = textLine->hasValidBorders() ? textLine->getMin().y : 0.0;

    // Move:
    textLine->move(m_data.insertionPoint + RS_Vector{0., -m_data.height * lineCounter * m_data.lineSpacingFactor * ls});
    // Rotate:
    // textLine->rotate(m_data.insertionPoint, m_data.angle);

    textLine->setPen(RS_Pen(RS2::FlagInvalid));
    textLine->setLayer(nullptr);
    textLine->forcedCalculateBorders();

    addEntity(textLine);
    return textTail;
}

RS_Vector RS_MText::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = m_data.insertionPoint.distanceTo(coord);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_MText*>(this);
    }
    return m_data.insertionPoint;
}

RS_VectorSolutions RS_MText::getRefPoints() const {
    return {m_data.insertionPoint};
}

RS_Vector RS_MText::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    return RS_Entity::doGetNearestRef(coord, dist);
}

RS_Vector RS_MText::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_Entity::doGetNearestSelectedRef(coord, dist);
}

void RS_MText::moveRef([[maybe_unused]] const RS_Vector& ref, const RS_Vector& offset) {
    move(offset);
}

void RS_MText::moveSelectedRef([[maybe_unused]] const RS_Vector& ref, const RS_Vector& offset) {
    move(offset);
}

void RS_MText::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    m_data.insertionPoint.move(offset);
    //    update();
    for (RS_Entity* e : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(e);
        if (line != nullptr) {
            line->moveBaseline(offset);
        }
    }
    forcedCalculateBorders();
}

void RS_MText::rotate(const RS_Vector& center, const double angle) {
    RS_EntityContainer::rotate(center, angle);
    m_data.insertionPoint.rotate(center, angle);

    //    update();
    //    calculateBorders();

    for (RS_Entity* e : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(e);
        if (line != nullptr) {
            RS_Vector corner = line->getLeftBottomCorner();
            //            corner.rotate(oldInsertionPoint, -m_data.angle);
            corner.rotate(center, angle);
            line->setLeftBottomCorner(corner);
        }
    }

    m_data.angle = RS_Math::correctAngle(m_data.angle + angle);

    rotateLinesRefs();
}

// fixme - sand - check whether this function is actually used
void RS_MText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.angle = RS_Math::correctAngle(m_data.angle + angleVector.angle());
    /*   RS_Vector lineRotationVector(angleVector.angle());
       for (RS_Entity *e: std::as_const(entities)) {
           auto line = dynamic_cast<LC_TextLine *> (e);
           if (line != nullptr) {
               RS_Vector corner = line->getLeftBottomCorner();
               corner.rotate(center, lineRotationVector);
           }
       }
       m_data.angle = RS_Math::correctAngle(m_data.angle + angleVector.angle());
       rotateLinesRefs();
       */
    update();
}

void RS_MText::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.insertionPoint.scale(center, factor);
    m_data.width *= factor.x;
    m_data.height *= factor.x;
    update();
}

void RS_MText::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.insertionPoint.mirror(axisPoint1, axisPoint2);
    // double ang = axisPoint1.angleTo(axisPoint2);
    const bool readable = RS_Math::isAngleReadable(m_data.angle);

    RS_Vector vec = RS_Vector::polar(1.0, m_data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    m_data.angle = vec.angle();

    bool corr = false;
    m_data.angle = RS_Math::makeAngleReadable(m_data.angle, readable, &corr);

    if (corr) {
        if (m_data.halign == RS_MTextData::HALeft) {
            m_data.halign = RS_MTextData::HARight;
        }
        else if (m_data.halign == RS_MTextData::HARight) {
            m_data.halign = RS_MTextData::HALeft;
        }
    }
    else {
        if (m_data.valign == RS_MTextData::VATop) {
            m_data.valign = RS_MTextData::VABottom;
        }
        else if (m_data.valign == RS_MTextData::VABottom) {
            m_data.valign = RS_MTextData::VATop;
        }
    }
    update();
}

bool RS_MText::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) const {
    return false;
}

/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_MText::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    }
}

/**
 * Dumps the point's m_data to stdout.
 */
std::ostream& operator<<(std::ostream& os, const RS_MText& p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}

#define DEBUG_LINE_POINTS_

void RS_MText::draw(RS_Painter* painter) {
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
#endif

    const bool drawAsDraft = painter->isTextLineNotRenderable(getHeight());
    if (drawAsDraft) {
        drawDraft(painter);
        return;
    }

    for (RS_Entity* entity : std::as_const(*this)) {
        painter->drawAsChild(entity);

#ifdef DEBUG_LINE_POINTS
        auto line = dynamic_cast<LC_TextLine*>(entity); if (line != nullptr) {
            painter->drawRect(view->toGui(line->getBaselineStart()) - 2, view->toGui(line->getBaselineStart()) + 2);
            painter->drawRect(view->toGui(line->getLeftBottomCorner()) - 4, view->toGui(line->getLeftBottomCorner()) + 4);
        }
#endif
    }
}

void RS_MText::drawDraft(RS_Painter* painter) {
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(painter->toGui(getMin()), painter->toGui(getMax()));
#endif
    for (RS_Entity* entity : std::as_const(*this)) {
        const auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr && line->hasValidBorders()) {
            painter->drawLineWCS(line->getBaselineStart(), line->getBaselineEnd());
        }
    }
}
