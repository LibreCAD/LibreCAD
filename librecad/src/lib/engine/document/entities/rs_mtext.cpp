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

#include <iostream>

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pen.h"

RS_MText::LC_TextLine* RS_MText::LC_TextLine::clone() const {
    auto* ec = new LC_TextLine(getParent(), isOwner());
    if (isOwner()) {
        for (const RS_Entity* entity : *this) {
            if (entity != nullptr) {
                ec->push_back(entity->clone());
            }
        }
    }
    else {
        ec->clear();
        std::copy(cbegin(), cend(), std::back_inserter(*ec));
    }
    ec->detach();
    ec->setTextSize(m_textSize);
    ec->setLeftBottomCorner(m_leftBottomCorner);
    ec->setBaselineStart(m_baselineStart);
    ec->setBaselineEnd(m_baselineEnd);
    return ec;
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
    RS_MText::setText(m_data.text);
}

RS_Entity* RS_MText::clone() const {
    auto* t = new RS_MText(*this);
    t->setOwner(isOwner());
    t->detach();
    return t;
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
        if (line != nullptr && line->count() > 0) {
            const RS_Vector& start = line->getBaselineStart();
            const RS_Vector& end = line->getBaselineEnd();
            const auto l = new RS_Line(proxy, start, end);
            proxy->addEntity(l);
        }
    }
    return proxy;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_MText::setText(QString t) {
    m_data.text = std::move(t);

    // handle some special flags embedded in the text:
    if (m_data.text.left(4) == R"(\A0;)") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_MTextData::VABottom;
    }
    else if (m_data.text.left(4) == R"(\A1;)") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_MTextData::VAMiddle;
    }
    else if (m_data.text.left(4) == R"(\A2;)") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_MTextData::VATop;
    }

    if (m_data.updateMode == RS2::Update) {
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

    RS_Font* font{RS_FONTLIST->requestFont(m_data.style)};
    if (nullptr == font) {
        return;
    }

    RS_Vector letterPos{0.0, -9.0};
    RS_Vector letterSpace{font->getLetterSpacing(), 0.0};
    RS_Vector space{font->getWordSpacing(), 0.0};

    // Support right-to-left lext layout direction
    if (m_data.drawingDirection == RS_MTextData::RightToLeft) {
        letterSpace.x = -letterSpace.x;
        space.x = -space.x;
    }
    int lineCounter{0};

    // Every single text line gets stored in this entity container
    // so we can move the whole line around easily:
    auto oneLine{new LC_TextLine(this)};

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // For every letter:
    for (decltype(m_data.text.length()) i = 0; i < m_data.text.length(); ++i) {
        auto remaining = m_data.text.mid(i);
        // Handle \F not followed by {<codePage>}
        if (remaining.startsWith(R"(\F)") && remaining.indexOf(R"(^\\[Ff]\{[\d\w]*\})") != 0) {
            addLetter(*oneLine, m_data.text.at(i), *font, letterSpace, letterPos);
            continue;
        }
        if (remaining.startsWith(R"(\\)")) {
            // Allow escape '\', needed to support "\S" and "\P" in string
            // "\S" is used for super/subscripts
            // "\P" is used to start a new line
            // "\\S" and "\\P" to get literal strings "\S" and "\P"
            addLetter(*oneLine, m_data.text.at(i++), *font, letterSpace, letterPos);
            continue;
        }

        switch (m_data.text.at(i).unicode()) {
            case 0x0A:
                // line feed:
                updateAddLine(oneLine, lineCounter++);
                oneLine = new LC_TextLine(this);
                letterPos = RS_Vector(0.0, -9.0);
                break;

            case 0x20:
                // Space:
                letterPos += space;
                break;

            case 0x5C: {
                bool handled{false};
                // code (e.g. \S, \P, ..)
                ++i;
                if (static_cast<int>(m_data.text.length()) <= i) {
                    continue;
                }
                const std::uint32_t ch{m_data.text.toUcs4().at(i)};
                switch (ch) {
                    case 'P':
                        updateAddLine(oneLine, lineCounter++);
                        oneLine = new LC_TextLine(this);
                        letterPos = RS_Vector(0.0, -9.0);
                        handled = true;
                        break;

                    case 'f':
                    case 'F': {
                        // font change
                        //  \f{symbol} changes font to symbol
                        //  \f{} sets font to standard
                        ++i;
                        if ('{' != m_data.text.at(i).unicode()) {
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

                        // add texts:
                        double upperWidth{0.0};
                        if (!upperText.isEmpty()) {
                            RS_MText* upper = createUpperLower(upperText, m_data, letterPos + RS_Vector{0., 9.});
                            oneLine->addEntity(upper);
                            upper->reparent(oneLine);
                            upperWidth = upper->getSize().x;
                        }

                        double lowerWidth{0.0};
                        if (!lowerText.isEmpty()) {
                            RS_MText* lower = createUpperLower(lowerText, m_data, letterPos + RS_Vector{0.0, 4.0});
                            oneLine->addEntity(lower);
                            lower->reparent(oneLine);
                            lowerWidth = lower->getSize().x;
                        }

                        letterPos.x += std::copysign(std::max(upperWidth, lowerWidth), letterSpace.x);
                        letterPos += letterSpace;
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
                addLetter(*oneLine, m_data.text.at(i), *font, letterSpace, letterPos);
                break;
            } // outer default
        } // outer switch (m_data.text.at(i).unicode())
    } // for (i) loop

    m_usedTextHeight -= m_data.height * m_data.lineSpacingFactor * 5.0 / 3.0 - m_data.height;

    updateAddLine(oneLine, lineCounter);

    alignVertically();
    RS_DEBUG->print("RS_MText::update: OK");
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
        if (line != nullptr) {
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
        if (line != nullptr) {
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
void RS_MText::addLetter(LC_TextLine& oneLine, const QChar letter, RS_Font& font, const RS_Vector& letterSpace, RS_Vector& letterPosition) {
    auto letterText{QString(letter)};
    if (nullptr == font.findLetter(letterText)) {
        RS_DEBUG->print("RS_MText::update: missing font for letter( %s ), replaced " "it with QChar(0xfffd)", qPrintable(letterText));
        letterText = QChar(0xfffd);
    }

    LC_LOG << "RS_MText::update: insert a letter at pos:(" << letterPosition.x << ", " << letterPosition.y << ")";

    // adjust for right-to-left text: letter position start from the right
    const bool righToLeft = std::signbit(letterSpace.x);

    const RS_InsertData d(letterText, letterPosition, RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0), font.getLetterList(),
                          RS2::NoUpdate);

    const auto letterEntity{new RS_Insert(this, d)};
    letterEntity->setPen(RS_Pen(RS2::FlagInvalid));
    letterEntity->setLayer(nullptr);
    letterEntity->update();
    letterEntity->forcedCalculateBorders();

    // Add spacing, if the font is actually wider than word spacing
    double actualWidth = letterEntity->getMax().x - letterEntity->getMin().x;
    if (actualWidth >= font.getWordSpacing() + RS_TOLERANCE) {
        const double letterSpacing = std::max(1., std::abs(letterSpace.x));
        const double wordSpacing = font.getWordSpacing();
        actualWidth = wordSpacing + letterSpacing - std::fmod(actualWidth - wordSpacing, letterSpacing);
    }
    LC_LOG << __LINE__ << ": actualWidth: " << actualWidth;

    RS_Vector letterWidth = {actualWidth, 0.};
    // right-to-left text support
    letterWidth.x = std::copysign(letterWidth.x, letterSpace.x);

    letterPosition += letterWidth;

    // For right-to-left text, need to align the current position with the right edge
    if (righToLeft) {
        letterEntity->move(letterWidth);
    }

    oneLine.addEntity(letterEntity);

    // next letter position:
    letterPosition += letterSpace;
}

RS_MText* RS_MText::createUpperLower(QString text, const RS_MTextData& data, const RS_Vector& position) {
    auto* line = new RS_MText(nullptr, {
                                  position,
                                  4.0,
                                  100.0,
                                  RS_MTextData::VATop,
                                  RS_MTextData::HALeft,
                                  data.drawingDirection,
                                  RS_MTextData::Exact,
                                  1.0,
                                  std::move(text),
                                  data.style,
                                  0.0,
                                  RS2::Update
                              });
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
 *
 * @return  distance over the text base-line
 */
double RS_MText::updateAddLine(LC_TextLine* textLine, const int lineCounter) {
    constexpr double ls = 5.0 / 3.0;

    RS_DEBUG->print("RS_MText::updateAddLine: width: %f", textLine->getSize().x);

    // textLine->forcedCalculateBorders();
    // RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f",
    // textLine->getSize().x);

    // Scale:
    const double scale = m_data.height / 9.0;
    textLine->scale(RS_Vector{0., 0.}, RS_Vector(scale, scale));

    textLine->forcedCalculateBorders();

    // Horizontal Align:
    switch (m_data.halign) {
        case RS_MTextData::HACenter:
            textLine->move(RS_Vector{-0.5 * (textLine->getMin().x + textLine->getMax().x), 0.});
            break;

        case RS_MTextData::HARight:
            textLine->move(RS_Vector{-textLine->getMax().x, 0.});
            break;

        default:
            textLine->move(RS_Vector{-textLine->getMin().x, 0.});
            break;
    }

    // Update actual text size (before rotating, after scaling!):
    if (textLine->getSize().x > m_usedTextWidth) {
        m_usedTextWidth = textLine->getSize().x;
    }

    m_usedTextHeight += m_data.height * m_data.lineSpacingFactor * ls;

    // Gets the distance over text base-line (before rotating, after scaling!):
    const double textTail = textLine->getMin().y;

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
        if (line != nullptr && line->count() > 0) {
            painter->drawLineWCS(line->getBaselineStart(), line->getBaselineEnd());
        }
    }
}
