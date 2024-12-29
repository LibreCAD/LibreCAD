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

#include <cmath>
#include <iostream>

#include "rs_mtext.h"

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_graphicview.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_line.h"
#include "rs_painter.h"


RS_MText::LC_TextLine *RS_MText::LC_TextLine::clone() const {
    auto *ec = new LC_TextLine(getParent(), isOwner());
    if (isOwner()) {
        for (const auto *entity: entities)
            if (entity != nullptr)
                ec->entities.push_back(entity->clone());
    } else {
        ec->entities = entities;
    }
    ec->detach();
    ec->initId();
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
    t->initId();
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

RS_Entity *RS_MText::cloneProxy(RS_GraphicView* view) const {
    if (view->isDrawTextsAsDraftForPreview()) {
        auto* proxy = new RS_EntityContainer();
        proxy->setOwner(true);
        for (RS_Entity *entity: std::as_const(entities)) {
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
    else{
        return clone();
    }
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

  RS_Vector letterPos{0.0, -9.0};
  RS_Vector letterSpace{font->getLetterSpacing(), 0.0};
  RS_Vector space{font->getWordSpacing(), 0.0};

  // Support right-to-left lext layout direction
  if (data.drawingDirection == RS_MTextData::RightToLeft) {
    letterSpace.x = -letterSpace.x;
    space.x = -space.x;
  }
  int lineCounter{0};

  // Every single text line gets stored in this entity container
  // so we can move the whole line around easily:
  LC_TextLine *oneLine{new LC_TextLine(this)};

  // First every text line is created with
  //   alignment: top left
  //   angle: 0
  //   height: 9.0
  // Rotation, scaling and centering is done later

  // For every letter:
  for (decltype(data.text.length()) i = 0; i < data.text.length(); ++i) {
    // Handle \F not followed by {<codePage>}
    if (data.text.mid(i).startsWith(R"(\F)") &&
        data.text.mid(i).indexOf(R"(^\\[Ff]\{[\d\w]*\})") != 0) {
      addLetter(*oneLine, data.text.at(i), *font, letterSpace, letterPos);
      continue;
    } else if (data.text.mid(i).startsWith(R"(\\)")) {
      // Allow escape '\', needed to support "\S" and "\P" in string
      // "\S" is used for super/subscripts
      // "\P" is used to start a new line
      // "\\S" and "\\P" to get literal strings "\S" and "\P"
      addLetter(*oneLine, data.text.at(i++), *font, letterSpace, letterPos);
      continue;
    }

    bool handled{false};

    switch (data.text.at(i).unicode()) {
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
      // code (e.g. \S, \P, ..)
      ++i;
      if (static_cast<int>(data.text.length()) <= i) {
        continue;
      }
      std::uint32_t ch{data.text.toUcs4().at(i)};
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

        // add texts:
        double upperWidth{0.0};
        if (!upperText.isEmpty()) {
          RS_MText *upper =
              createUpperLower(upperText, data, letterPos + RS_Vector{0., 9.});
          oneLine->addEntity(upper);
          upper->reparent(oneLine);
          upperWidth = upper->getSize().x;
        }

        double lowerWidth{0.0};
        if (!lowerText.isEmpty()) {
          RS_MText *lower = createUpperLower(lowerText, data,
                                             letterPos + RS_Vector{0.0, 4.0});
          oneLine->addEntity(lower);
          lower->reparent(oneLine);
          lowerWidth = lower->getSize().x;
        }

        letterPos.x +=
            std::copysign(std::max(upperWidth, lowerWidth), letterSpace.x);
        letterPos += letterSpace;
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
      addLetter(*oneLine, data.text.at(i), *font, letterSpace, letterPos);
      break;
    } // outer default
    } // outer switch (data.text.at(i).unicode())
  }   // for (i) loop

  usedTextHeight -=
          data.height * data.lineSpacingFactor * 5.0 / 3.0 - data.height;

  updateAddLine(oneLine, lineCounter);

  alignVertically();
  RS_DEBUG->print("RS_MText::update: OK");
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
    for (RS_Entity *e: std::as_const(entities)) {
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
    for (RS_Entity *e: std::as_const(entities)) {
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

    // adjust for right-to-left text: letter position start from the right
    bool righToLeft = std::signbit(letterSpace.x);

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
        actualWidth = font.getWordSpacing() + std::ceil((actualWidth - font.getWordSpacing())/std::abs(letterSpace.x)) * std::abs(letterSpace.x);
    }

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
    for (RS_Entity* e: std::as_const(entities)) {
        auto line = dynamic_cast<LC_TextLine *> (e);
        if (line != nullptr) {
            line->moveBaseline(offset);
        }
    }
    forcedCalculateBorders();
}

void RS_MText::rotate(const RS_Vector &center, const double &angle) {
    RS_Vector oldInsertionPoint{data.insertionPoint};
    RS_EntityContainer::rotate(center, angle);
    data.insertionPoint.rotate(center, angle);

//    update();
//    calculateBorders();

    for (RS_Entity *e: std::as_const(entities)) {
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
                                        const RS_Vector & /*v2*/) {
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

void RS_MText::draw(RS_Painter *painter, RS_GraphicView *view,
                    double & patternOffset) {
    /*if (!(painter && view))
        return;*/
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
#endif
//    if (!view->isPrintPreview() && !view->isPrinting()) {
        if (/*view->isPanning() || */view->toGuiDY(getHeight()) < view->getMinRenderableTextHeightInPx()) {
            drawDraft(painter, view, patternOffset);
            return;
        }
//    }

    for (RS_Entity *entity: std::as_const(entities)) {
        entity->drawAsChild(painter, view, patternOffset);

#ifdef DEBUG_LINE_POINTS
        auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr) {
            painter->drawRect(view->toGui(line->getBaselineStart())-2, view->toGui(line->getBaselineStart())+2);
            painter->drawRect(view->toGui(line->getLeftBottomCorner())-4, view->toGui(line->getLeftBottomCorner())+4);
        }
#endif
    }
}

void RS_MText::drawDraft(RS_Painter *painter, RS_GraphicView *view, [[maybe_unused]] double &patternOffset) {
#ifdef DEBUG_LINE_POINTS
    painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
#endif
    for (RS_Entity *entity: std::as_const(entities)) {
        auto line = dynamic_cast<LC_TextLine*>(entity);
        if (line != nullptr && line->count() > 0) {
            const RS_Vector &start = view->toGui(line->getBaselineStart());
            const RS_Vector &end = view->toGui(line->getBaselineEnd());
            painter->drawLine(start, end);
    }
  }
}
