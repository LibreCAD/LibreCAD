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

#include "rs_text.h"

#include<iostream>

#include "rs_debug.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_painter.h"
#include "rs_pen.h"

namespace {
/**
 * Resolve the UAX#9 base direction for an RS_Text update pass. Honors an
 * explicit @c LeftToRight / @c RightToLeft setting when set; for @c ByContent
 * scans the string for the first strong-directional character and uses that
 * (UAX#9 P-rules), defaulting to LTR when there is no strong character.
 */
Qt::LayoutDirection
resolveTextBaseDirection(const QString &text,
                         RS_TextData::DrawingDirection setting) {
  if (setting == RS_TextData::LeftToRight)
    return Qt::LeftToRight;
  if (setting == RS_TextData::RightToLeft)
    return Qt::RightToLeft;
  for (int i = 0; i < text.size(); ++i) {
    const QChar::Direction d = text.at(i).direction();
    if (d == QChar::DirL)
      return Qt::LeftToRight;
    if (d == QChar::DirR || d == QChar::DirAL)
      return Qt::RightToLeft;
  }
  return Qt::LeftToRight;
}
} // namespace

class RS_Font;

RS_TextData::RS_TextData(const RS_Vector& insertionPoint, const RS_Vector& secondPoint, const double height, const double widthRel,
                         const VAlign valign, const HAlign halign, const TextGeneration textGeneration, const QString& text,
                         const QString& style, const double angle, const RS2::UpdateMode updateMode) : insertionPoint(insertionPoint),
    secondPoint(secondPoint), height(height), widthRel(widthRel), valign(valign), halign(halign), textGeneration(textGeneration),
    text(text), style(style), angle(angle), updateMode(updateMode) {
}

std::ostream& operator <<(std::ostream& os, const RS_TextData& td) {
    os << "(" << td.insertionPoint << ',' << td.secondPoint << ',' << td.height << ',' << td.widthRel << ',' << td.valign << ',' << td.
        halign << ',' << td.textGeneration << ',' << td.text.toLatin1().data() << ',' << td.style.toLatin1().data() << ',' << td.angle <<
        ',' << td.updateMode << ',' << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Text::RS_Text(RS_EntityContainer* parent, const RS_TextData& d)
    : RS_EntityContainer(parent), m_data(d) {
    m_usedTextHeight = 0.0;
    m_usedTextWidth = 0.0;
    RS_Text::setText(m_data.text);
}

RS_Entity* RS_Text::clone() const {
    const auto t = new RS_Text(*this);
    t->setOwner(isOwner());
    t->detach();
    return t;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_Text::setText(const QString& t) {
    m_data.text = t;

    // handle some special flags embedded in the text:
    if (m_data.text.left(4) == "\\A0;") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_TextData::VABottom;
    }
    else if (m_data.text.left(4) == "\\A1;") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_TextData::VAMiddle;
    }
    else if (m_data.text.left(4) == "\\A2;") {
        m_data.text = m_data.text.mid(4);
        m_data.valign = RS_TextData::VATop;
    }
    if (m_data.updateMode == RS2::Update) {
        update();
        //calculateBorders();
    }
}

void RS_Text::setDrawingDirection(RS_TextData::DrawingDirection direction) {
  if (data.drawingDirection == direction)
    return;
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
//RLZ: bad function, this is MText style align
int RS_Text::getAlignment() const {
    if (m_data.valign == RS_TextData::VATop) {
        if (m_data.halign == RS_TextData::HALeft) {
            return 1;
        }
        if (m_data.halign == RS_TextData::HACenter) {
            return 2;
        }
        if (m_data.halign == RS_TextData::HARight) {
            return 3;
        }
    }
    else if (m_data.valign == RS_TextData::VAMiddle) {
        if (m_data.halign == RS_TextData::HALeft) {
            return 4;
        }
        if (m_data.halign == RS_TextData::HACenter) {
            return 5;
        }
        if (m_data.halign == RS_TextData::HARight) {
            return 6;
        }
    }
    else if (m_data.valign == RS_TextData::VABaseline) {
        if (m_data.halign == RS_TextData::HALeft) {
            return 7;
        }
        if (m_data.halign == RS_TextData::HACenter) {
            return 8;
        }
        if (m_data.halign == RS_TextData::HARight) {
            return 9;
        }
    }
    else if (m_data.valign == RS_TextData::VABottom) {
        if (m_data.halign == RS_TextData::HALeft) {
            return 10;
        }
        if (m_data.halign == RS_TextData::HACenter) {
            return 11;
        }
        if (m_data.halign == RS_TextData::HARight) {
            return 12;
        }
    }
    if (m_data.halign == RS_TextData::HAFit) {
        return 13;
    }
    if (m_data.halign == RS_TextData::HAAligned) {
        return 14;
    }
    if (m_data.halign == RS_TextData::HAMiddle) {
        return 15;
    }
    return 1;
}

/**
 * Sets the alignment from an int.
 *
 * @param a 1: top left ... 9: bottom right
 */
//RLZ: bad function, this is MText style align
void RS_Text::setAlignment(const int a) {
    switch (a % 3) {
        case 1:
            m_data.halign = RS_TextData::HALeft;
            break;
        case 2:
            m_data.halign = RS_TextData::HACenter;
            break;
        case 0:
            m_data.halign = RS_TextData::HARight;
            break;
        default: {
            m_data.halign = RS_TextData::HALeft;
            break;
        }
    }

    const int valign = static_cast<int>(ceil(a / 3.0));
    switch (valign) {
        case 1:
            m_data.valign = RS_TextData::VATop;
            break;
        case 2:
            m_data.valign = RS_TextData::VAMiddle;
            break;
        case 3:
            m_data.valign = RS_TextData::VABaseline;
            break;
        case 4:
            m_data.valign = RS_TextData::VABottom;
            break;
        default:
            m_data.valign = RS_TextData::VATop;
            break;
    }
    if (a > 12) {
        m_data.valign = RS_TextData::VABaseline;
        if (a == 13) {
            m_data.halign = RS_TextData::HAFit;
        }
        else if (a == 14) {
            m_data.halign = RS_TextData::HAAligned;
        }
        else if (a == 15) {
            m_data.halign = RS_TextData::HAMiddle;
        }
    }
    if (data.updateMode == RS2::Update) {  // fixme - sand - why it's only there? that's logic should be in all setter? or it's artefact?
      update();
    }
}

/**
 * @return Number of lines in this text entity.
 */
int RS_Text::getNumberOfLines() const {
    int c = 1;
    for (qsizetype i = 0; i < m_data.text.length(); ++i) {
        if (m_data.text.at(i).unicode() == 0x0A) {
            c++;
        }
    }
    return c;
}

/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's m_data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_Text::update() {
    RS_DEBUG->print("RS_Text::update");

    clear();

    if (isDeleted()) {
        return;
    }

    m_usedTextWidth = 0.0;
    m_usedTextHeight = 0.0;

    RS_Font* font = RS_FONTLIST->requestFont(m_data.style);

    if (font == nullptr) {
        return;
    }

    auto letterPos = RS_Vector(0.0, -9.0);
    const auto letterSpace = RS_Vector(font->getLetterSpacing(), 0.0);
    const auto space = RS_Vector(font->getWordSpacing(), 0.0);

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // Visual ordering depends on the drawingDirection setting:
    //   * RightToLeft: pure positional reversal — matches AutoCAD semantics
    //     and the editor mirror. UAX#9 alone leaves EN digits direction-
    //     immune, so widget and canvas would diverge for "1234" otherwise.
    //   * ByContent (and the other settings): UAX#9 with first-strong base
    //     detection so embedded strong-RTL runs (Hebrew/Arabic) still
    //     display in correct visual order.
    std::vector<int> visual;
    if (m_data.drawingDirection == RS_TextData::RightToLeft) {
      visual.resize(m_data.text.size());
      for (int i = 0; i < m_data.text.size(); ++i) {
        visual[i] = m_data.text.size() - 1 - i;
      }
    } else {
      const Qt::LayoutDirection baseDir =
          resolveTextBaseDirection(m_data.text, m_data.drawingDirection);
      visual = RS_MText::computeBidiVisualOrder(m_data.text, baseDir); // fixme - sand - it's better to use separate utility
    }

    for (int logIdx : visual) {
      const QChar ch = m_data.text.at(logIdx);
      // Space:
      if (ch.unicode() == 0x20) {
        letterPos += space;
      } else {
        // One Letter:
        QString letterText = QString(ch);
        if (font->findLetter(letterText) == nullptr) {
          RS_DEBUG->print("RS_Text::update: missing font for letter( %s ), "
                          "replaced it with QChar(0xfffd)",
                          qPrintable(letterText));
          letterText = QChar(0xfffd);
        }
        RS_DEBUG->print("RS_Text::update: insert a "
                        "letter at pos: %f/%f",
                        letterPos.x, letterPos.y);

            RS_InsertData d(letterText, letterPos, RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0), font->getLetterList(),
                            RS2::NoUpdate);

            auto* letter = new RS_Insert(this, d);
            letter->setPen(RS_Pen(RS2::FlagInvalid));
            letter->setLayer(nullptr);
            letter->update();
            letter->forcedCalculateBorders();

        auto letterWidth = RS_Vector(letter->getMax().x - letterPos.x, 0.0);
            if (letterWidth.x < 0) {
                letterWidth.x = -letterSpace.x;
            }
            addEntity(letter);

            // next letter position:
            letterPos += letterWidth;
            letterPos += letterSpace;
        }
    }

    if (!getAutoUpdateBorders()) {
        //only update borders when needed
        forcedCalculateBorders();
    }
    const RS_Vector textSize = getSize();

    RS_DEBUG->print("RS_Text::updateAddLine: width 2: %f", textSize.x);

    // Vertical Align:
    constexpr double vSize = 9.0;
    //HAAligned, HAFit, HAMiddle require VABaseline
    if (m_data.halign == RS_TextData::HAAligned || m_data.halign == RS_TextData::HAFit || m_data.halign == RS_TextData::HAMiddle) {
        m_data.valign = RS_TextData::VABaseline;
    }
    RS_Vector offset(0.0, 0.0);
    switch (m_data.valign) {
        case RS_TextData::VAMiddle: {
            offset.move(RS_Vector(0.0, vSize / 2.0));
            break;
        }
        case RS_TextData::VABottom: {
            offset.move(RS_Vector(0.0, vSize + 3));
            break;
        }
        case RS_TextData::VABaseline: {
            offset.move(RS_Vector(0.0, vSize));
            break;
        }
        default:
            break;
    }

    // Horizontal Align:
    switch (m_data.halign) {
        case RS_TextData::HAMiddle: {
            offset.move(RS_Vector(-textSize.x / 2.0, -(vSize + (textSize.y / 2.0) + getMin().y)));
            break;
        }
        case RS_TextData::HACenter: {
            RS_DEBUG->print("RS_Text::updateAddLine: move by: %f", -textSize.x / 2.0);
            offset.move(RS_Vector(-textSize.x / 2.0, 0.0));
            break;
        }
        case RS_TextData::HARight: {
            offset.move(RS_Vector(-textSize.x, 0.0));
            break;
        }
        default:
            break;
    }

    if (m_data.halign != RS_TextData::HAAligned && m_data.halign != RS_TextData::HAFit) {
        m_data.secondPoint = RS_Vector(offset.x, offset.y - vSize);
    }
    RS_EntityContainer::move(offset);

    // Scale:
    if (m_data.halign == RS_TextData::HAAligned) {
        const double dist = m_data.insertionPoint.distanceTo(m_data.secondPoint) / textSize.x;
        m_data.height = vSize * dist;
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0), RS_Vector(dist, dist));
    }
    else if (m_data.halign == RS_TextData::HAFit) {
        const double dist = m_data.insertionPoint.distanceTo(m_data.secondPoint) / textSize.x;
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0), RS_Vector(dist, m_data.height / 9.0));
    }
    else {
        RS_EntityContainer::scale(RS_Vector(0.0, 0.0), RS_Vector(m_data.height * m_data.widthRel / 9.0, m_data.height / 9.0));
        m_data.secondPoint.scale(RS_Vector(0.0, 0.0), RS_Vector(m_data.height * m_data.widthRel / 9.0, m_data.height / 9.0));
    }

    forcedCalculateBorders();

    // Update actual text size (before rotating, after scaling!):
    m_usedTextWidth = getSize().x;
    m_usedTextHeight = m_data.height;

    // Rotate:
    if (m_data.halign == RS_TextData::HAAligned || m_data.halign == RS_TextData::HAFit) {
        const double angle = m_data.insertionPoint.angleTo(m_data.secondPoint);
        m_data.angle = angle;
    }
    else {
        m_data.secondPoint.rotate(RS_Vector(0.0, 0.0), m_data.angle);
        m_data.secondPoint.move(m_data.insertionPoint);
    }
    RS_EntityContainer::rotate(RS_Vector(0.0, 0.0), m_data.angle);

    // Move to insertion point:
    RS_EntityContainer::move(m_data.insertionPoint);

    updateBaselinePoints();

    forcedCalculateBorders();

    RS_DEBUG->print("RS_Text::update: OK");
}

void RS_Text::updateBaselinePoints() {
    const RS_Vector shiftVector = RS_Vector::polar(m_usedTextHeight / 2, m_data.angle + M_PI_2);
    m_baselineStartPoint = m_data.secondPoint + shiftVector;
    m_baselineEndPoint = m_baselineStartPoint + RS_Vector::polar(m_usedTextWidth, m_data.angle);
}

RS_Vector RS_Text::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = m_data.insertionPoint.distanceTo(coord);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Text*>(this);
    }
    return m_data.insertionPoint;
}

RS_VectorSolutions RS_Text::getRefPoints() const {
    RS_VectorSolutions ret({m_data.insertionPoint, m_data.secondPoint});
    return ret;
}

RS_Vector RS_Text::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    return RS_Entity::doGetNearestRef(coord, dist);
}

RS_Vector RS_Text::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_Entity::doGetNearestSelectedRef(coord, dist);
}

void RS_Text::moveRef([[maybe_unused]] const RS_Vector& ref, const RS_Vector& offset) {
    move(offset);
}

void RS_Text::moveSelectedRef([[maybe_unused]] const RS_Vector& ref, const RS_Vector& offset) {
    //    RS_EntityContainer::moveSelectedRef(ref, offset);
    move(offset);
}

void RS_Text::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    m_data.insertionPoint.move(offset);
    m_data.secondPoint.move(offset);
    updateBaselinePoints();
    forcedCalculateBorders();
    //    update();
}

void RS_Text::rotate(const RS_Vector& center, const double angle) {
    const RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.secondPoint.rotate(center, angleVector);
    m_data.angle = RS_Math::correctAngle(m_data.angle + angle);
    updateBaselinePoints();
    //    update();
}

void RS_Text::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.secondPoint.rotate(center, angleVector);
    m_data.angle = RS_Math::correctAngle(m_data.angle + angleVector.angle());
    updateBaselinePoints();
    //    update();
}

void RS_Text::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.insertionPoint.scale(center, factor);
    m_data.secondPoint.scale(center, factor);
    m_data.height *= factor.x;
    update();
}

void RS_Text::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    const bool readable = RS_Math::isAngleReadable(m_data.angle);

    RS_Vector vec = RS_Vector::polar(1.0, m_data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    m_data.angle = vec.angle();

    bool corr = false;
    m_data.angle = RS_Math::makeAngleReadable(m_data.angle, readable, &corr);

    if (corr) {
        m_data.insertionPoint.mirror(axisPoint1, axisPoint2);
        m_data.secondPoint.mirror(axisPoint1, axisPoint2);
        if (m_data.halign == RS_TextData::HALeft) {
            m_data.halign = RS_TextData::HARight;
        }
        else if (m_data.halign == RS_TextData::HARight) {
            m_data.halign = RS_TextData::HALeft;
        }
        else if (m_data.halign == RS_TextData::HAFit || m_data.halign == RS_TextData::HAAligned) {
            const RS_Vector tmp = m_data.insertionPoint;
            m_data.insertionPoint = m_data.secondPoint;
            m_data.secondPoint = tmp;
        }
    }
    else {
        RS_Vector minP{getMin().x, getMax().y};
        minP = minP.mirror(axisPoint1, axisPoint2);
        const double mirrAngle = axisPoint1.angleTo(axisPoint2) * 2.0;
        m_data.insertionPoint.move(minP - getMin());
        m_data.secondPoint.move(minP - getMin());
        m_data.insertionPoint.rotate(minP, mirrAngle);
        m_data.secondPoint.rotate(minP, mirrAngle);
    }
    update();
}

bool RS_Text::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) const {
    return false;
}

/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_Text::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    }
}

/**
 * Dumps the point's m_data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Text& p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}

RS_Entity* RS_Text::cloneProxy() const {
    return new RS_Line(nullptr, m_baselineStartPoint, m_baselineEndPoint);
}

void RS_Text::drawDraft(RS_Painter* painter) {
    painter->drawLineWCS(m_baselineStartPoint, m_baselineEndPoint);
}

void RS_Text::draw(RS_Painter* painter) {
    const bool drawAsDraft = painter->isTextLineNotRenderable(getHeight());
    if (drawAsDraft) {
        drawDraft(painter);
        return;
    }

    for (RS_Entity* e : *this) {
        painter->drawAsChild(e);
    }
}
