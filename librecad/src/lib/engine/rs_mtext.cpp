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

#include <algorithm>
#include <cmath>
#include <iostream>

#include "rs_mtext.h"

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_graphicview.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_painter.h"

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
  RS_MText *t = new RS_MText(*this);
  t->setOwner(isOwner());
  t->initId();
  t->detach();
  return t;
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

void RS_MText::setDrawingDirection(
    RS_MTextData::MTextDrawingDirection direction) {
  if (data.drawingDirection == direction) return;
  data.drawingDirection = direction;
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
    // No font at all: the text cannot be turned into letters. Keep a
    // data-derived extent so the entity still has a location and can be drawn
    // as a placeholder box rather than vanishing silently.
    applyFallbackBorders();
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
  RS_EntityContainer *oneLine{new RS_EntityContainer(this)};

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
      oneLine = new RS_EntityContainer(this);
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
        oneLine = new RS_EntityContainer(this);
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

  forcedCalculateBorders();
  if (isEmpty() || getSize().x <= RS_TOLERANCE || !std::isfinite(getSize().x)) {
    // Either the string produced no letters at all (empty or whitespace-only
    // text), or the letters carry no geometry - which is what happens when the
    // font loaded but its letter list is empty, leaving every letter an empty
    // insert. Normalise both to an empty container with a data-derived extent.
    clear();
    applyFallbackBorders();
    RS_DEBUG->print("RS_MText::update: no letters generated");
    return;
  }

  alignVertically();
  RS_DEBUG->print("RS_MText::update: OK");
}

/**
 * Nominal advance width of one glyph cell, as a fraction of the text height.
 * The LFF letters are drawn on a 9-unit grid and a typical cell including the
 * letter spacing is about 7.5 units wide.
 */
static constexpr double RS_MTEXT_NOMINAL_ASPECT = 7.5 / 9.0;

void RS_MText::applyFallbackBorders() {
  // Derive the extent from the text data. Falling back to the container borders
  // is not an option here: RS_EntityContainer::calculateBorders() collapses an
  // empty container to 0/0, which would place the placeholder at the drawing
  // origin instead of where the text actually is.
  double height = data.height;
  if (!(height > RS_TOLERANCE) || !std::isfinite(height)) {
    height = 1.0;
  }

  // Longest line drives the width; the number of lines drives the height.
  const QStringList lines = data.text.split('\n');
  int columns = 1;
  for (const QString &line : lines) {
    columns = std::max(columns, static_cast<int>(line.length()));
  }
  int rows = std::max(1, static_cast<int>(lines.size()));

  double width = columns * height * RS_MTEXT_NOMINAL_ASPECT;
  if (data.width > RS_TOLERANCE && std::isfinite(data.width)) {
    // A reference rectangle width was given, so the text wraps within it.
    width = std::min(width, data.width);
  }
  double totalHeight = rows * height;

  // Local box, top-left at the origin: RS_MText is laid out downwards from the
  // insertion point (VATop is the no-op case in alignVertically()).
  double left = 0.0;
  double top = 0.0;
  switch (data.halign) {
  case RS_MTextData::HACenter:
    left = -width / 2.0;
    break;
  case RS_MTextData::HARight:
    left = -width;
    break;
  default:
    break;
  }
  switch (data.valign) {
  case RS_MTextData::VAMiddle:
    top = totalHeight / 2.0;
    break;
  case RS_MTextData::VABottom:
    top = totalHeight;
    break;
  default:
    break;
  }

  RS_Vector corners[4] = {{left, top},
                          {left + width, top},
                          {left + width, top - totalHeight},
                          {left, top - totalHeight}};

  resetBorders();
  for (RS_Vector corner : corners) {
    corner.rotate(RS_Vector(0.0, 0.0), data.angle);
    corner.move(data.insertionPoint);
    minV = RS_Vector::minimum(corner, minV);
    maxV = RS_Vector::maximum(corner, maxV);
  }
}

void RS_MText::calculateBorders() {
  RS_EntityContainer::calculateBorders();
  // No letters, or letters whose geometry collapsed to a point: in both cases
  // the base class has just left the borders at 0/0, which is neither the right
  // size nor the right place. Fall back to the data-derived extent.
  if (isEmpty() ||
      (maxV.x - minV.x <= RS_TOLERANCE && maxV.y - minV.y <= RS_TOLERANCE)) {
    applyFallbackBorders();
  }
}

void RS_MText::alignVertically()
{
    // Vertical Align:

    switch (data.valign) {
    case RS_MTextData::VATop:
        // no change
        break;

    case RS_MTextData::VAMiddle:
        RS_EntityContainer::move({0., 0.5 * usedTextHeight});
      break;

    case RS_MTextData::VABottom:
        RS_EntityContainer::move({0., usedTextHeight});
      break;

    default:
        LC_ERR<<__func__<<"(): line "<<__LINE__<<": invalid Invalid RS_MText::VAlign="<<data.valign;
      break;
    }
    RS_EntityContainer::rotate(data.insertionPoint, data.angle);
    forcedCalculateBorders();
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
void RS_MText::addLetter(RS_EntityContainer &oneLine, QChar letter,
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
      const double spacing = std::max(1., letterSpace.x);
      actualWidth = font.getWordSpacing() + std::ceil((actualWidth - font.getWordSpacing())/spacing) * spacing;
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
  RS_MText *line = new RS_MText(
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
double RS_MText::updateAddLine(RS_EntityContainer *textLine, int lineCounter) {
  constexpr double ls = 5.0 / 3.0;

  RS_DEBUG->print("RS_MText::updateAddLine: width: %f", textLine->getSize().x);

  // textLine->forcedCalculateBorders();
  // RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f",
  // textLine->getSize().x);

  // Scale:
  textLine->scale(RS_Vector{0., 0.},
                  RS_Vector(data.height / 9.0, data.height / 9.0));

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

void RS_MText::move(const RS_Vector &offset) {
  RS_EntityContainer::move(offset);
  data.insertionPoint.move(offset);
  //    update();
}

void RS_MText::rotate(const RS_Vector &center, const double &angle) {
  RS_Vector angleVector(angle);
  RS_EntityContainer::rotate(center, angleVector);
  data.insertionPoint.rotate(center, angleVector);
  data.angle = RS_Math::correctAngle(data.angle + angle);
  //    update();
}
void RS_MText::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  RS_EntityContainer::rotate(center, angleVector);
  data.insertionPoint.rotate(center, angleVector);
  data.angle = RS_Math::correctAngle(data.angle + angleVector.angle());
  //    update();
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

void RS_MText::draw(RS_Painter *painter, RS_GraphicView *view,
                    double & /*patternOffset*/) {
  if (!(painter && view))
    return;

  // No letters could be generated (missing font, or a string that produces no
  // glyphs). Draw the placeholder box in every mode, including print preview and
  // printing: silently omitting the entity would hide from the user that the
  // drawing contains text which cannot be rendered.
  if (isEmpty()) {
    painter->drawPlaceholderRect(view->toGui(getMin()), view->toGui(getMax()));
    return;
  }

  if (!view->isPrintPreview() && !view->isPrinting()) {
    if (view->isPanning() || view->toGuiDY(getHeight()) < 4) {
      // Performance substitution for glyphs that do exist, so it keeps the
      // entity's own pen - including a deliberate NoPen.
      painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
      return;
    }
  }

  double patternOffset = 0.0;

  foreach (RS_Entity *entity, entities)
    entity->draw(painter, view, patternOffset);
}
