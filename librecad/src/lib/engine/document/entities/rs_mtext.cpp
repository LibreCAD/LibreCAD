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


 #include <iostream>

#include <QChar>

#include "rs_mtext.h"
#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pen.h"

// Implementation of the Unicode Bidirectional Algorithm (UAX #9), simplified
// for paragraphs without explicit embedding/override controls or isolates —
// which covers all DXF/DWG MText input. Per-character bidi class comes from
// QChar::direction() (Qt's Unicode tables), so we still rely on Qt's data
// even though we don't go through QTextLayout (which depends on the font
// database and would silently degrade for scripts that lack a fallback font
// on the host system).
std::vector<int> RS_MText::computeBidiVisualOrder(
    const QString &text, Qt::LayoutDirection baseDirection) {
    std::vector<int> result;
    const int n = text.size();
    if (n == 0) {
        return result;
    }

    const int baseLevel = (baseDirection == Qt::RightToLeft) ? 1 : 0;

    // ---------- Step 1: assign embedding levels per character ----------
    // Levels: 0 = LTR, 1 = RTL, 2 = numeric (LTR within RTL context).
    // Strong directionality directly determines levels for L / R / AL.
    // Numeric types (EN, AN) get the +2 bump (LTR digits inside RTL get
    // visually rendered LTR but "after" the surrounding RTL context).
    // Neutrals/weaks are filled in below from surrounding strong context.
    constexpr int LVL_UNKNOWN = -1;
    std::vector<int> level(n, LVL_UNKNOWN);
    for (int i = 0; i < n; ++i) {
        switch (text.at(i).direction()) {
        case QChar::DirL:
            level[i] = 0;
            break;
        case QChar::DirR:
        case QChar::DirAL:
            level[i] = 1;
            break;
        case QChar::DirEN:
        case QChar::DirAN:
            // Digits are LTR by themselves but get bumped one level above
            // base if base is LTR (level 2), or above the next-level RTL
            // context (also level 2). Per UAX#9, EN/AN consistently get
            // even levels >= base+2 in mixed paragraphs.
            level[i] = (baseLevel == 1) ? 2 : 2;
            break;
        default:
            // Neutrals (whitespace, punctuation, paragraph/segment separators,
            // boundary-neutrals, NSM, BN, etc.) — resolved in pass 2.
            level[i] = LVL_UNKNOWN;
            break;
        }
    }

    // ---------- Step 2: resolve neutrals via surrounding strong context ----
    // A run of unresolved characters takes the level of the strong character
    // before it if that matches the strong character after it; otherwise it
    // takes the paragraph base level. This collapses UAX#9 rules N0/N1/N2
    // into one pass, sufficient for our MText inputs.
    int i = 0;
    while (i < n) {
        if (level[i] != LVL_UNKNOWN) { ++i; continue; }
        int runStart = i;
        while (i < n && level[i] == LVL_UNKNOWN) ++i;
        const int runEnd = i;  // exclusive

        int prevLevel = baseLevel;
        for (int k = runStart - 1; k >= 0; --k) {
            if (level[k] != LVL_UNKNOWN && level[k] < 2) {
                prevLevel = level[k]; break;
            }
        }
        int nextLevel = baseLevel;
        for (int k = runEnd; k < n; ++k) {
            if (level[k] != LVL_UNKNOWN && level[k] < 2) {
                nextLevel = level[k]; break;
            }
        }
        const int neutralLevel = (prevLevel == nextLevel) ? prevLevel : baseLevel;
        for (int k = runStart; k < runEnd; ++k) {
            level[k] = neutralLevel;
        }
    }

    // ---------- Step 3: apply L2 — reverse maximal runs of equal level ----
    // Build the visual order by iterating from the highest level down.
    result.reserve(n);
    for (int k = 0; k < n; ++k) result.push_back(k);

    // Find max level present.
    int maxLevel = 0;
    for (int k = 0; k < n; ++k) {
        if (level[k] > maxLevel) maxLevel = level[k];
    }
    // Per UAX#9 L2: reverse runs of level >= L for L from maxLevel down to
    // 1 (the lowest odd level) inclusive — regardless of base direction.
    // For LTR base this leaves L runs untouched and flips RTL/numeric runs;
    // for RTL base it additionally reverses the entire paragraph at L=1, so
    // strong-LTR runs end up in the right place within an RTL frame.
    for (int L = maxLevel; L >= 1; --L) {
        int j = 0;
        while (j < n) {
            if (level[result[j]] >= L) {
                int runStart = j;
                while (j < n && level[result[j]] >= L) ++j;
                std::reverse(result.begin() + runStart, result.begin() + j);
            } else {
                ++j;
            }
        }
    }
    return result;
}

RS_MText::LC_TextLine *RS_MText::LC_TextLine::clone() const {
    auto *ec = new LC_TextLine(getParent(), isOwner());
    if (isOwner()) {
        for (const RS_Entity *entity: *this)
            if (entity != nullptr)
                ec->push_back(entity->clone());
    } else {
        ec->clear();
        std::copy(cbegin(), cend(), std::back_inserter(*ec));
    }
    ec->detach();
    ec->setTextSize(textSize);
    ec->setLeftBottomCorner(leftBottomCorner);
    ec->setBaselineStart(baselineStart);
    ec->setBaselineEnd(baselineEnd);
    return ec;
}

const RS_Vector &RS_MText::LC_TextLine::getTextSize() {
    return textSize;
}

void RS_MText::LC_TextLine::setTextSize(const RS_Vector &textSize) {
    this->textSize = textSize;
}

const RS_Vector &RS_MText::LC_TextLine::getLeftBottomCorner() const {
    return leftBottomCorner;
}

void RS_MText::LC_TextLine::setLeftBottomCorner(const RS_Vector leftBottomCorner) {
    this->leftBottomCorner = leftBottomCorner;
}

const RS_Vector &RS_MText::LC_TextLine::getBaselineStart() const {
    return baselineStart;
}

void RS_MText::LC_TextLine::setBaselineStart(const RS_Vector &baselineStart) {
    this->baselineStart = baselineStart;
}

const RS_Vector &RS_MText::LC_TextLine::getBaselineEnd() const {
    return baselineEnd;
}

void RS_MText::LC_TextLine::setBaselineEnd(const RS_Vector &baselineEnd) {
    this->baselineEnd = baselineEnd;
}

void RS_MText::LC_TextLine::moveBaseline(const RS_Vector &offset) {
    baselineStart.move(offset);
    baselineEnd.move(offset);
    leftBottomCorner.move(offset);
}

RS_MTextData::RS_MTextData(const RS_Vector &_insertionPoint, double _height,
                           double _width, VAlign _valign, HAlign _halign,
                           MTextDrawingDirection _drawingDirection,
                           MTextLineSpacingStyle _lineSpacingStyle,
                           double _lineSpacingFactor, const QString &_text,
                           const QString &_style, double _angle,
                           RS2::UpdateMode _updateMode)
    : insertionPoint(_insertionPoint), height(_height), width(_width),
      valign(_valign), halign(_halign), drawingDirection(_drawingDirection),
      lineSpacingStyle(_lineSpacingStyle),
      lineSpacingFactor(_lineSpacingFactor), text(_text), style(_style),
      angle(_angle), updateMode(_updateMode) {}

std::ostream &operator<<(std::ostream &os, const RS_MTextData &td) {
    os << "(" << td.insertionPoint << ',' << td.height << ',' << td.width << ','
       << td.valign << ',' << td.halign << ',' << td.drawingDirection << ','
       << td.lineSpacingStyle << ',' << td.lineSpacingFactor << ','
       << td.text.toLatin1().data() << ',' << td.style.toLatin1().data() << ','
       << td.angle << ',' << td.updateMode << ',' << ")";
    return os;
}

/**
 * Constructor.
 */
RS_MText::RS_MText(RS_EntityContainer *parent, const RS_MTextData &d)
    : RS_EntityContainer(parent), data(d) {
    setText(data.text);
}

RS_Entity *RS_MText::clone() const {
    auto *t = new RS_MText(*this);
    t->setOwner(isOwner());
    t->detach();
    return t;
}

// fixme - test concept for using UI proxies for heavy entities on modification operation (rotate, scale etc).
// potentially, it might be either expanded further or removed.
class RS_MTextProxy:public RS_EntityContainer{
public:
    RS_MTextProxy(const RS_MText &parent):RS_EntityContainer(parent) {
    }
};

RS_Entity *RS_MText::cloneProxy() const {
    auto* proxy = new RS_EntityContainer();
    proxy->setOwner(true);
    for (RS_Entity *entity: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr && line->count() > 0) {
            const RS_Vector &start = line->getBaselineStart();
            const RS_Vector &end = line->getBaselineEnd();
            auto line = new RS_Line(proxy, start, end);
            proxy->addEntity(line);
        }
    }
    return proxy;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_MText::setText(QString t) {
    data.text = std::move(t);

    // handle some special flags embedded in the text:
    if (data.text.left(4) == R"(\A0;)") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VABottom;
    } else if (data.text.left(4) == R"(\A1;)") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VAMiddle;
    } else if (data.text.left(4) == R"(\A2;)") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VATop;
    }

    if (data.updateMode == RS2::Update) {
        update();
    }
}

/**
 * Gets the alignment as an int.
 *
 * @return  1: top left ... 9: bottom right
 */
int RS_MText::getAlignment() {
    if (data.valign == RS_MTextData::VATop) {
        if (data.halign == RS_MTextData::HALeft) {
            return 1;
        } else if (data.halign == RS_MTextData::HACenter) {
            return 2;
        } else if (data.halign == RS_MTextData::HARight) {
            return 3;
        }
    } else if (data.valign == RS_MTextData::VAMiddle) {
        if (data.halign == RS_MTextData::HALeft) {
            return 4;
        } else if (data.halign == RS_MTextData::HACenter) {
            return 5;
        } else if (data.halign == RS_MTextData::HARight) {
            return 6;
        }
    } else if (data.valign == RS_MTextData::VABottom) {
        if (data.halign == RS_MTextData::HALeft) {
            return 7;
        } else if (data.halign == RS_MTextData::HACenter) {
            return 8;
        } else if (data.halign == RS_MTextData::HARight) {
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
void RS_MText::setAlignment(int a) {
    switch (a % 3) {
        default:
        case 1:
            data.halign = RS_MTextData::HALeft;
            break;
        case 2:
            data.halign = RS_MTextData::HACenter;
            break;
        case 0:
            data.halign = RS_MTextData::HARight;
            break;
    }

    switch ((int)ceil(a / 3.0)) {
        default:
        case 1:
            data.valign = RS_MTextData::VATop;
            break;
        case 2:
            data.valign = RS_MTextData::VAMiddle;
            break;
        case 3:
            data.valign = RS_MTextData::VABottom;
            break;
    }
}

/**
 * @return Number of lines in this text entity.
 */
int RS_MText::getNumberOfLines() {
    return 1 + std::count_if(data.text.cbegin(), data.text.cend(),
                             [](QChar c) { return c.unicode() == 0xA; });
}

/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_MText::update() {
  RS_DEBUG->print("RS_MText::update");

  clear();
  if (isUndone()) {
    return;
  }

  usedTextWidth = 0.0;
  usedTextHeight = 0.0;

  RS_Font *font{RS_FONTLIST->requestFont(data.style)};
  if (nullptr == font) {
    return;
  }

  // letterSpace and space are always treated as positive advances here; the
  // bidi pass already produced visual L→R order, so the cursor unconditionally
  // moves rightward during emission.
  RS_Vector letterPos{0.0, -9.0};
  const RS_Vector letterSpace{font->getLetterSpacing(), 0.0};
  const double spaceWidth = font->getWordSpacing();

  int lineCounter{0};

  // Every single text line gets stored in this entity container
  // so we can move the whole line around easily:
  LC_TextLine *oneLine{new LC_TextLine(this)};

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
    flushBidiLine(*oneLine, segments, letterSpace, letterPos);
    updateAddLine(oneLine, lineCounter++);
    oneLine = new LC_TextLine(this);
    letterPos = RS_Vector(0.0, -9.0);
  };

  for (decltype(data.text.length()) i = 0; i < data.text.length(); ++i) {
    // Handle \F not followed by {<codePage>}
    if (data.text.mid(i).startsWith(R"(\F)") &&
        data.text.mid(i).indexOf(R"(^\\[Ff]\{[\d\w]*\})") != 0) {
      segments.push_back({LC_BidiSegment::Char, data.text.at(i), font, 0.0, {}, {}});
      continue;
    } else if (data.text.mid(i).startsWith(R"(\\)")) {
      // Allow escape '\', needed to support "\S" and "\P" in string
      // "\S" is used for super/subscripts
      // "\P" is used to start a new line
      // "\\S" and "\\P" to get literal strings "\S" and "\P"
      segments.push_back({LC_BidiSegment::Char, data.text.at(i++), font, 0.0, {}, {}});
      continue;
    }

    bool handled{false};

    switch (data.text.at(i).unicode()) {
    case 0x0A:
      // line feed:
      closeLine();
      break;

    case 0x20:
      // Space:
      segments.push_back({LC_BidiSegment::Space, QChar(' '), nullptr, spaceWidth, {}, {}});
      break;

    case 0x5C: {
      // code (e.g. \S, \P, ..)
      ++i;
      if (static_cast<int>(data.text.length()) <= i) {
        continue;
      }
      std::uint32_t ch{data.text.toUcs4().at(i)};
      switch (ch) {
      case 'P':
        closeLine();
        handled = true;
        break;

      case 'f':
      case 'F': {
        // font change
        //  \f{symbol} changes font to symbol
        //  \f{} sets font to standard
        ++i;
        if ('{' != data.text.at(i).unicode()) {
          --i;
          continue;
        }

        qsizetype j{data.text.indexOf('}', i)};
        if (j > i) {
          QString fontName = data.text.mid(i + 1, j - i - 1);

          RS_Font *fontNew{RS_FONTLIST->requestFont(fontName)};
          if (nullptr != fontNew) {
            font = fontNew;
          }
          if (nullptr == font) {
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
        ++i;
        while (static_cast<int>(data.text.length()) > i &&
               data.text.at(i).unicode() != '^' &&
               data.text.at(i).unicode() != '\\') {
          upperText += data.text.at(i);
          ++i;
        }

        ++i;
        if (static_cast<int>(data.text.length()) > i &&
            '^' == data.text.at(i - 1).unicode() &&
            ' ' == data.text.at(i).unicode()) {
          ++i;
        }

        // get lower string:
        while (static_cast<int>(data.text.length()) > i &&
               ';' != data.text.at(i).unicode()) {
          lowerText += data.text.at(i);
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

      if (handled)
        break;
    } // outer case 0x5C

    // if char is not handled
    // fall-through
    default: {
      // One Letter:
      segments.push_back({LC_BidiSegment::Char, data.text.at(i), font, 0.0, {}, {}});
      break;
    } // outer default
    } // outer switch (data.text.at(i).unicode())
  }   // for (i) loop

  // Flush any trailing segments belonging to the last line.
  flushBidiLine(*oneLine, segments, letterSpace, letterPos);

  usedTextHeight -=
          data.height * data.lineSpacingFactor * 5.0 / 3.0 - data.height;

  updateAddLine(oneLine, lineCounter);

  alignVertically();
  RS_DEBUG->print("RS_MText::update: OK");
}

/**
 * Reorder @p segments via the Unicode Bidirectional Algorithm and emit each
 * one (letter / space / super-sub stack) into @p oneLine in visual L→R order.
 * Cursor advances are computed from @c .lff metrics, not from QTextLayout's
 * Qt-font metrics — QTextLayout is used only for the logical→visual
 * permutation. Clears @p segments on return.
 */
void RS_MText::flushBidiLine(LC_TextLine &oneLine,
                              std::vector<LC_BidiSegment> &segments,
                              const RS_Vector &letterSpace,
                              RS_Vector &letterPosition) {
  if (segments.empty()) {
    return;
  }

  // Build a plain-text string for the bidi pass. Stacks become U+FFFC (Object
  // Replacement Character), which has bidi class ON (Other Neutral) so it
  // gets resolved to the surrounding context like punctuation.
  QString plainText;
  plainText.reserve(static_cast<int>(segments.size()));
  for (const auto &seg : segments) {
    switch (seg.kind) {
    case LC_BidiSegment::Char:  plainText.append(seg.codepoint); break;
    case LC_BidiSegment::Space: plainText.append(QChar(' ')); break;
    case LC_BidiSegment::Stack: plainText.append(QChar(0xFFFC)); break;
    }
  }

  const Qt::LayoutDirection baseDir =
      (data.drawingDirection == RS_MTextData::RightToLeft)
          ? Qt::RightToLeft
          : Qt::LeftToRight;
  const std::vector<int> visualOrder = computeBidiVisualOrder(plainText, baseDir);

  for (int logIdx : visualOrder) {
    const auto &seg = segments[logIdx];
    switch (seg.kind) {
    case LC_BidiSegment::Char: {
      RS_Font *segFont = seg.font;
      if (segFont == nullptr) {
        // Fallback: should not normally happen, but stay defensive.
        segFont = RS_FONTLIST->requestFont(data.style);
      }
      if (segFont != nullptr) {
        addLetter(oneLine, seg.codepoint, *segFont, letterSpace, letterPosition);
      }
      break;
    }
    case LC_BidiSegment::Space:
      letterPosition.x += seg.width;
      break;
    case LC_BidiSegment::Stack: {
      double upperWidth = 0.0;
      if (!seg.upperText.isEmpty()) {
        RS_MText *upper = createUpperLower(
            seg.upperText, data, letterPosition + RS_Vector{0., 9.});
        oneLine.addEntity(upper);
        upper->reparent(&oneLine);
        upperWidth = upper->getSize().x;
      }
      double lowerWidth = 0.0;
      if (!seg.lowerText.isEmpty()) {
        RS_MText *lower = createUpperLower(
            seg.lowerText, data, letterPosition + RS_Vector{0., 4.});
        oneLine.addEntity(lower);
        lower->reparent(&oneLine);
        lowerWidth = lower->getSize().x;
      }
      letterPosition.x += std::max(upperWidth, lowerWidth);
      letterPosition += letterSpace;
      break;
    }
    }
  }

  segments.clear();
}

void RS_MText::alignVertically(){
    // Vertical Align:
    switch (data.valign) {
        case RS_MTextData::VATop: {
            // no change
            break;
        }
        case RS_MTextData::VAMiddle: {
            RS_EntityContainer::move({0., 0.5 * usedTextHeight});
            break;
        }
        case RS_MTextData::VABottom: {
            // adjust corners for text lines
            RS_EntityContainer::move({0., usedTextHeight});
            break;
        }
        default:
            LC_ERR<<__func__<<"(): line "<<__LINE__<<": invalid Invalid RS_MText::VAlign="<<data.valign;
            break;
    }
    for (RS_Entity *e: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            RS_Vector corner =  RS_Vector(line->getMin().rotate(data.insertionPoint, data.angle));
            RS_Vector size = line->getSize();
            line->setLeftBottomCorner(corner);
            line->setTextSize(size);
        }
    }
    rotateLinesRefs();

    RS_EntityContainer::rotate(data.insertionPoint, data.angle);

    forcedCalculateBorders();
}

void RS_MText::rotateLinesRefs() const {
    for (RS_Entity *e: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            RS_Vector corner = line->getLeftBottomCorner();
            RS_Vector size = line->getTextSize();
            RS_Vector shiftVector(size.y/2, 0);
            shiftVector.rotate(data.angle+M_PI_2);

            RS_Vector baselineStart = corner + shiftVector;
            RS_Vector baselineEnd = corner+shiftVector + RS_Vector::polar(size.x, data.angle);

//            baselineStart.rotate(data.insertionPoint, data.angle);
            line->setBaselineStart(baselineStart);

//            baselineEnd.rotate(data.insertionPoint, data.angle);
            line->setBaselineEnd(baselineEnd);
        }
    }
}

/**
 * Used internally by update() to add a letter to one line
 *
 * @param RS_EntityContainer& oneLine the current entity container
 * @param QChar letter the letter to add
 * @param RS_Font& font the font to use
 * @param const RS_Vector& letterSpace the letter width to use
 * @param RS_Vector& letterPosition the current letter position; will be updated
 * after addition
 *
 */
void RS_MText::addLetter(LC_TextLine &oneLine, QChar letter,
                         RS_Font &font, const RS_Vector &letterSpace,
                         RS_Vector &letterPosition) {
    QString letterText{QString(letter)};
    if (nullptr == font.findLetter(letterText)) {
        RS_DEBUG->print("RS_MText::update: missing font for letter( %s ), replaced "
                        "it with QChar(0xfffd)",
                        qPrintable(letterText));
        letterText = QChar(0xfffd);
    }

    LC_LOG << "RS_MText::update: insert a letter at pos:(" << letterPosition.x
           << ", " << letterPosition.y << ")";

    RS_InsertData d(letterText, letterPosition, RS_Vector(1.0, 1.0), 0.0, 1, 1,
                    RS_Vector(0.0, 0.0), font.getLetterList(), RS2::NoUpdate);

    RS_Insert *letterEntity{new RS_Insert(this, d)};
    letterEntity->setPen(RS_Pen(RS2::FlagInvalid));
    letterEntity->setLayer(nullptr);
    letterEntity->update();
    letterEntity->forcedCalculateBorders();

    // Add spacing, if the font is actually wider than word spacing
    double actualWidth = letterEntity->getMax().x - letterEntity->getMin().x;
    if (actualWidth >= font.getWordSpacing() + RS_TOLERANCE) {
        double letterSpacing = std::max(1., letterSpace.x);
        double wordSpacing = font.getWordSpacing();
        actualWidth = wordSpacing + letterSpacing - std::fmod(actualWidth - wordSpacing, letterSpacing);
    }
    LC_LOG<<__LINE__<<": actualWidth: "<<actualWidth;

    letterPosition.x += actualWidth;

    oneLine.addEntity(letterEntity);

    // next letter position:
    letterPosition += letterSpace;
}

RS_MText *RS_MText::createUpperLower(QString text, const RS_MTextData &data,
                                     const RS_Vector &position) {
    auto *line = new RS_MText(
        nullptr, {position, 4.0, 100.0, RS_MTextData::VATop, RS_MTextData::HALeft,
                  data.drawingDirection, RS_MTextData::Exact, 1.0,
                  std::move(text), data.style, 0.0, RS2::Update});
    line->setLayer(nullptr);
    line->setPen({RS2::FlagInvalid});
    line->calculateBorders();
    return line;
}

/**
 * Used internally by update() to add a text line created with
 * default values and alignment to this text container.
 *
 * @param textLine The text line.
 * @param lineCounter Line number.
 *
 * @return  distance over the text base-line
 */
double RS_MText::updateAddLine(LC_TextLine *textLine, int lineCounter) {
    constexpr double ls = 5.0 / 3.0;

    RS_DEBUG->print("RS_MText::updateAddLine: width: %f", textLine->getSize().x);

    // textLine->forcedCalculateBorders();
    // RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f",
    // textLine->getSize().x);

    // Scale:
    double scale = data.height / 9.0;
    textLine->scale(RS_Vector{0., 0.},
                    RS_Vector(scale, scale));

    textLine->forcedCalculateBorders();

    // Horizontal Align:
    switch (data.halign) {
        case RS_MTextData::HACenter:
            textLine->move(RS_Vector{-0.5 * (textLine->getMin().x + textLine->getMax().x), 0.});
            break;

        case RS_MTextData::HARight:
            textLine->move(RS_Vector{- textLine->getMax().x, 0.});
            break;

        default:
            textLine->move(RS_Vector{- textLine->getMin().x, 0.});
            break;
    }

    // Update actual text size (before rotating, after scaling!):
    if (textLine->getSize().x > usedTextWidth) {
        usedTextWidth = textLine->getSize().x;
    }

    usedTextHeight += data.height * data.lineSpacingFactor * ls;

    // Gets the distance over text base-line (before rotating, after scaling!):
    double textTail = textLine->getMin().y;

    // Move:
    textLine->move(data.insertionPoint + RS_Vector{0., -data.height * lineCounter * data.lineSpacingFactor * ls});
    // Rotate:
    // textLine->rotate(data.insertionPoint, data.angle);

    textLine->setPen(RS_Pen(RS2::FlagInvalid));
    textLine->setLayer(nullptr);
    textLine->forcedCalculateBorders();

    addEntity(textLine);
    return textTail;
}

RS_Vector RS_MText::getNearestEndpoint(const RS_Vector &coord,
                                       double *dist) const {
    if (dist) {
        *dist = data.insertionPoint.distanceTo(coord);
    }
    return data.insertionPoint;
}

RS_VectorSolutions RS_MText::getRefPoints() const {
    return {data.insertionPoint};
}

RS_Vector RS_MText::getNearestRef(const RS_Vector &coord, double *dist) const {
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_MText::getNearestSelectedRef(const RS_Vector &coord, double *dist) const {
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

void RS_MText::moveRef([[maybe_unused]]const RS_Vector &ref, const RS_Vector &offset) {
    move(offset);
}

void RS_MText::moveSelectedRef([[maybe_unused]]const RS_Vector &ref, const RS_Vector &offset) {
    move(offset);
}

void RS_MText::move(const RS_Vector &offset) {
    RS_EntityContainer::move(offset);
    data.insertionPoint.move(offset);
    //    update();
    for (RS_Entity* e: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            line->moveBaseline(offset);
        }
    }
    forcedCalculateBorders();
}

void RS_MText::rotate(const RS_Vector &center, double angle) {
    RS_EntityContainer::rotate(center, angle);
    data.insertionPoint.rotate(center, angle);

//    update();
//    calculateBorders();

    for (RS_Entity *e: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            RS_Vector corner = line->getLeftBottomCorner();
//            corner.rotate(oldInsertionPoint, -data.angle);
            corner.rotate(center, angle);
            line->setLeftBottomCorner(corner);
        }
    }

    data.angle = RS_Math::correctAngle(data.angle + angle);

    rotateLinesRefs();
}

// fixme - sand - check whether this function is actually used
void RS_MText::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle + angleVector.angle());
 /*   RS_Vector lineRotationVector(angleVector.angle());
    for (RS_Entity *e: std::as_const(entities)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            RS_Vector corner = line->getLeftBottomCorner();
            corner.rotate(center, lineRotationVector);
        }
    }
    data.angle = RS_Math::correctAngle(data.angle + angleVector.angle());
    rotateLinesRefs();
    */
    update();
}

void RS_MText::scale(const RS_Vector &center, const RS_Vector &factor) {
    data.insertionPoint.scale(center, factor);
    data.width *= factor.x;
    data.height *= factor.x;
    update();
}

void RS_MText::mirror(const RS_Vector &axisPoint1,
                      const RS_Vector &axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    // double ang = axisPoint1.angleTo(axisPoint2);
    bool readable = RS_Math::isAngleReadable(data.angle);

    RS_Vector vec = RS_Vector::polar(1.0, data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    data.angle = vec.angle();

    bool corr = false;
    data.angle = RS_Math::makeAngleReadable(data.angle, readable, &corr);

    if (corr) {
        if (data.halign == RS_MTextData::HALeft) {
            data.halign = RS_MTextData::HARight;
        } else if (data.halign == RS_MTextData::HARight) {
            data.halign = RS_MTextData::HALeft;
        }
    } else {
        if (data.valign == RS_MTextData::VATop) {
            data.valign = RS_MTextData::VABottom;
        } else if (data.valign == RS_MTextData::VABottom) {
            data.valign = RS_MTextData::VATop;
        }
    }
    update();
}

bool RS_MText::hasEndpointsWithinWindow(const RS_Vector & /*v1*/,
                                        const RS_Vector & /*v2*/) const
{
    return false;
}

/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_MText::stretch(const RS_Vector &firstCorner,
                       const RS_Vector &secondCorner, const RS_Vector &offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
        getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream &operator<<(std::ostream &os, const RS_MText &p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}

#define DEBUG_LINE_POINTS_

void RS_MText::draw(RS_Painter *painter) {
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
#endif

    bool drawAsDraft = painter->isTextLineNotRenderable(getHeight());
    if (drawAsDraft){
        drawDraft(painter);
        return;
    }

    for (RS_Entity *entity: std::as_const(*this)) {
        painter->drawAsChild(entity);

#ifdef DEBUG_LINE_POINTS
        auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr) {
            painter->drawRect(view->toGui(line->getBaselineStart())-2, view->toGui(line->getBaselineStart())+2);
            painter->drawRect(view->toGui(line->getLeftBottomCorner())-4, view->toGui(line->getLeftBottomCorner())+4);
        }
#endif
    }
}

void RS_MText::drawDraft(RS_Painter *painter) {
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(painter->toGui(getMin()), painter->toGui(getMax()));
#endif
    for (RS_Entity *entity: std::as_const(*this)) {
        auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr && line->count() > 0) {
            painter->drawLineWCS(line->getBaselineStart(), line->getBaselineEnd());
        }
    }
}
