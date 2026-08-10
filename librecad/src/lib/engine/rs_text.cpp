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
#include "rs_font.h"
#include "rs_text.h"

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

RS_TextData::RS_TextData(const RS_Vector& _insertionPoint,
						 const RS_Vector& _secondPoint,
						 double _height,
						 double _widthRel,
						 VAlign _valign,
						 HAlign _halign,
						 TextGeneration _textGeneration,
						 const QString& _text,
						 const QString& _style,
						 double _angle,
						 RS2::UpdateMode _updateMode):
	insertionPoint(_insertionPoint)
  ,secondPoint(_secondPoint)
  ,height(_height)
  ,widthRel(_widthRel)
  ,valign(_valign)
  ,halign(_halign)
  ,textGeneration(_textGeneration)
  ,text(_text)
  ,style(_style)
  ,angle(_angle)
  ,updateMode(_updateMode)
{
}


std::ostream& operator << (std::ostream& os, const RS_TextData& td) {
	os << "("
	   <<td.insertionPoint<<','
	  <<td.secondPoint<<','
	 <<td.height<<','
	<<td.widthRel<<','
	<<td.valign<<','
	<<td.halign<<','
	<<td.textGeneration<<','
	<<td.text.toLatin1().data() <<','
	<<td.style.toLatin1().data()<<','
	<<td.angle<<','
	<<td.updateMode<<','
	<<")";
	return os;
}

/**
 * Constructor.
 */
RS_Text::RS_Text(RS_EntityContainer* parent,
                 const RS_TextData& d)
        : RS_EntityContainer(parent), data(d) {

    usedTextHeight = 0.0;
    usedTextWidth = 0.0;
    setText(data.text);
}

RS_Entity* RS_Text::clone() const{
	RS_Text* t = new RS_Text(*this);
	t->setOwner(isOwner());
	t->initId();
	t->detach();
	return t;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_Text::setText(const QString& t) {
    data.text = t;

    // handle some special flags embedded in the text:
    if (data.text.left(4)=="\\A0;") {
        data.text = data.text.mid(4);
        data.valign = RS_TextData::VABottom;
    } else if (data.text.left(4)=="\\A1;") {
        data.text = data.text.mid(4);
        data.valign = RS_TextData::VAMiddle;
    } else if (data.text.left(4)=="\\A2;") {
        data.text = data.text.mid(4);
        data.valign = RS_TextData::VATop;
    }

    if (data.updateMode==RS2::Update) {
        update();
        //calculateBorders();
    }
}



/**
 * Gets the alignment as an int.
 *
 * @return  1: top left ... 9: bottom right
 */
//RLZ: bad function, this is MText style align
int RS_Text::getAlignment() {
    if (data.valign==RS_TextData::VATop) {
        if (data.halign==RS_TextData::HALeft) {
            return 1;
        } else if (data.halign==RS_TextData::HACenter) {
            return 2;
        } else if (data.halign==RS_TextData::HARight) {
            return 3;
        }
    } else if (data.valign==RS_TextData::VAMiddle) {
        if (data.halign==RS_TextData::HALeft) {
            return 4;
        } else if (data.halign==RS_TextData::HACenter) {
            return 5;
        } else if (data.halign==RS_TextData::HARight) {
            return 6;
        }
    } else if (data.valign==RS_TextData::VABaseline) {
        if (data.halign==RS_TextData::HALeft) {
            return 7;
        } else if (data.halign==RS_TextData::HACenter) {
            return 8;
        } else if (data.halign==RS_TextData::HARight) {
            return 9;
        }
    } else if (data.valign==RS_TextData::VABottom) {
        if (data.halign==RS_TextData::HALeft) {
            return 10;
        } else if (data.halign==RS_TextData::HACenter) {
            return 11;
        } else if (data.halign==RS_TextData::HARight) {
            return 12;
        }
    }
    if (data.halign==RS_TextData::HAFit) {
        return 13;
    } else if (data.halign==RS_TextData::HAAligned) {
        return 14;
    } else if (data.halign==RS_TextData::HAMiddle) {
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
void RS_Text::setAlignment(int a) {
    switch (a%3) {
    default:
    case 1:
        data.halign = RS_TextData::HALeft;
        break;
    case 2:
        data.halign = RS_TextData::HACenter;
        break;
    case 0:
        data.halign = RS_TextData::HARight;
        break;
    }

    switch ((int)ceil(a/3.0)) {
    default:
    case 1:
        data.valign = RS_TextData::VATop;
        break;
    case 2:
        data.valign = RS_TextData::VAMiddle;
        break;
    case 3:
        data.valign = RS_TextData::VABaseline;
        break;
    case 4:
        data.valign = RS_TextData::VABottom;
        break;
    }
    if (a > 12) {
        data.valign = RS_TextData::VABaseline;
        if (a == 13) {
            data.halign = RS_TextData::HAFit;
        } else if (a == 14) {
            data.halign = RS_TextData::HAAligned;
        } else if (a == 15) {
            data.halign = RS_TextData::HAMiddle;
        }
    }

}



/**
 * @return Number of lines in this text entity.
 */
int RS_Text::getNumberOfLines() {
    int c=1;

    for (int i=0; i<(int)data.text.length(); ++i) {
        if (data.text.at(i).unicode()==0x0A) {
            c++;
        }
    }

    return c;
}




/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_Text::update() {

    RS_DEBUG->print("RS_Text::update");

    clear();

    if (isUndone()) {
        return;
    }

    usedTextWidth = 0.0;
    usedTextHeight = 0.0;

    RS_Font* font = RS_FONTLIST->requestFont(data.style);

    if (font==NULL) {
        // No font at all: the text cannot be turned into letters. Keep a
        // data-derived extent so the entity still has a location and can be
        // drawn as a placeholder box rather than vanishing silently.
        applyFallbackBorders();
        return;
    }

    RS_Vector letterPos = RS_Vector(0.0, -9.0);
    RS_Vector letterSpace = RS_Vector(font->getLetterSpacing(), 0.0);
    RS_Vector space = RS_Vector(font->getWordSpacing(), 0.0);

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // For every letter:
    for (int i=0; i<(int)data.text.length(); ++i) {
        // Space:
        if (data.text.at(i).unicode() == 0x20) {
            letterPos+=space;
        } else {
            // One Letter:
            QString letterText = QString(data.text.at(i));
            if (font->findLetter(letterText) == NULL) {
                RS_DEBUG->print("RS_Text::update: missing font for letter( %s ), replaced it with QChar(0xfffd)",qPrintable(letterText));
                letterText = QChar(0xfffd);
            }
            RS_DEBUG->print("RS_Text::update: insert a "
                            "letter at pos: %f/%f", letterPos.x, letterPos.y);

            RS_InsertData d(letterText,
                            letterPos,
                            RS_Vector(1.0, 1.0),
                            0.0,
                            1,1, RS_Vector(0.0,0.0),
                            font->getLetterList(), RS2::NoUpdate);

            RS_Insert* letter = new RS_Insert(this, d);
            RS_Vector letterWidth;
            letter->setPen(RS_Pen(RS2::FlagInvalid));
            letter->setLayer(NULL);
            letter->update();
            letter->forcedCalculateBorders();

            letterWidth = RS_Vector(letter->getMax().x-letterPos.x, 0.0);
            if (letterWidth.x < 0)
                letterWidth.x = -letterSpace.x;

//            oneLine->addEntity(letter);
            addEntity(letter);

            // next letter position:
            letterPos += letterWidth;
            letterPos += letterSpace;
        }
    }

    if( ! RS_EntityContainer::autoUpdateBorders) {
        //only update borders when needed
        forcedCalculateBorders();
    }
    RS_Vector textSize = getSize();

    if (isEmpty() || textSize.x <= RS_TOLERANCE || !std::isfinite(textSize.x)) {
        // Either the string produced no letters at all (empty or whitespace-only
        // text), or the letters carry no geometry - which is what happens when
        // the font loaded but its letter list is empty, leaving every letter an
        // empty insert. Both cases are the same thing as far as drawing goes, so
        // normalise to an empty container with a data-derived extent. Stopping
        // here also protects the HAAligned/HAFit branch below, which divides by
        // textSize.x and would otherwise corrupt data.height.
        clear();
        applyFallbackBorders();
        return;
    }

    RS_DEBUG->print("RS_Text::updateAddLine: width 2: %f", textSize.x);

    // Vertical Align:
    double vSize = 9.0;
    //HAAligned, HAFit, HAMiddle require VABaseline
    if (data.halign == RS_TextData::HAAligned
            || data.halign == RS_TextData::HAFit
            || data.halign == RS_TextData::HAMiddle) {
        data.valign = RS_TextData::VABaseline;
    }
    RS_Vector offset(0.0, 0.0);
    switch (data.valign) {
    case RS_TextData::VAMiddle:
        offset.move(RS_Vector(0.0, vSize/2.0));
        break;

    case RS_TextData::VABottom:
        offset.move(RS_Vector(0.0, vSize+3));
        break;

    case RS_TextData::VABaseline:
        offset.move(RS_Vector(0.0, vSize));
        break;

    default:
        break;
    }

    // Horizontal Align:
    switch (data.halign) {
    case RS_TextData::HAMiddle:{
        offset.move(RS_Vector(-textSize.x/2.0, -(vSize + textSize.y/2.0 + getMin().y) ));
        break;}
    case RS_TextData::HACenter:
        RS_DEBUG->print("RS_Text::updateAddLine: move by: %f", -textSize.x/2.0);
        offset.move(RS_Vector(-textSize.x/2.0, 0.0));
        break;
    case RS_TextData::HARight:
        offset.move(RS_Vector(-textSize.x, 0.0));
        break;

    default:
        break;
    }

    if (data.halign!=RS_TextData::HAAligned && data.halign!=RS_TextData::HAFit){
        data.secondPoint = RS_Vector(offset.x, offset.y - vSize);
    }
    RS_EntityContainer::move(offset);


    // Scale:
    if (data.halign==RS_TextData::HAAligned){
        double dist = data.insertionPoint.distanceTo(data.secondPoint)/textSize.x;
        data.height = vSize*dist;
        RS_EntityContainer::scale(RS_Vector(0.0,0.0),
                        RS_Vector(dist, dist));
    } else if (data.halign==RS_TextData::HAFit){
        double dist = data.insertionPoint.distanceTo(data.secondPoint)/textSize.x;
        RS_EntityContainer::scale(RS_Vector(0.0,0.0),
                        RS_Vector(dist, data.height/9.0));
    } else {
        RS_EntityContainer::scale(RS_Vector(0.0,0.0),
                        RS_Vector(data.height*data.widthRel/9.0, data.height/9.0));
        data.secondPoint.scale(RS_Vector(0.0,0.0),
                               RS_Vector(data.height*data.widthRel/9.0, data.height/9.0));
    }

    forcedCalculateBorders();

    // Update actual text size (before rotating, after scaling!):
    usedTextWidth = getSize().x;
    usedTextHeight = data.height;

    // Rotate:
    if (data.halign==RS_TextData::HAAligned || data.halign==RS_TextData::HAFit){
        double angle = data.insertionPoint.angleTo(data.secondPoint);
        data.angle = angle;
    } else {
        data.secondPoint.rotate(RS_Vector(0.0,0.0), data.angle);
        data.secondPoint.move(data.insertionPoint);
    }
    RS_EntityContainer::rotate(RS_Vector(0.0,0.0), data.angle);

    // Move to insertion point:
    RS_EntityContainer::move(data.insertionPoint);

    forcedCalculateBorders();

    RS_DEBUG->print("RS_Text::update: OK");
}


/**
 * Nominal advance width of one glyph cell, as a fraction of the text height.
 * The LFF letters are drawn on a 9-unit grid and a typical cell including the
 * letter spacing is about 7.5 units wide.
 */
static constexpr double RS_TEXT_NOMINAL_ASPECT = 7.5 / 9.0;

void RS_Text::applyFallbackBorders() {
    // Derive the extent from the text data. Falling back to the container
    // borders is not an option here: RS_EntityContainer::calculateBorders()
    // collapses an empty container to 0/0, which would place the placeholder at
    // the drawing origin instead of where the text actually is.
    double height = data.height;
    if (!(height > RS_TOLERANCE) || !std::isfinite(height)) {
        height = 1.0;
    }
    double widthRel = data.widthRel;
    if (!(widthRel > RS_TOLERANCE) || !std::isfinite(widthRel)) {
        widthRel = 1.0;
    }
    // An empty string still gets a one-character box so that the entity stays
    // visible and selectable.
    int chars = std::max(1, static_cast<int>(data.text.length()));
    double width = chars * height * widthRel * RS_TEXT_NOMINAL_ASPECT;

    // Local box, baseline-left at the origin.
    double left = 0.0;
    double bottom = 0.0;
    switch (data.halign) {
    case RS_TextData::HACenter:
    case RS_TextData::HAMiddle:
        left = -width / 2.0;
        break;
    case RS_TextData::HARight:
        left = -width;
        break;
    default:
        break;
    }
    switch (data.valign) {
    case RS_TextData::VAMiddle:
        bottom = -height / 2.0;
        break;
    case RS_TextData::VATop:
        bottom = -height;
        break;
    default:
        break;
    }

    RS_Vector corners[4] = {
        {left,         bottom},
        {left + width, bottom},
        {left + width, bottom + height},
        {left,         bottom + height}
    };

    resetBorders();
    for (RS_Vector corner: corners) {
        corner.rotate(RS_Vector(0.0, 0.0), data.angle);
        corner.move(data.insertionPoint);
        minV = RS_Vector::minimum(corner, minV);
        maxV = RS_Vector::maximum(corner, maxV);
    }
}

void RS_Text::calculateBorders() {
    RS_EntityContainer::calculateBorders();
    // No letters, or letters whose geometry collapsed to a point: in both cases
    // the base class has just left the borders at 0/0, which is neither the
    // right size nor the right place. Fall back to the data-derived extent.
    if (isEmpty()
            || (maxV.x - minV.x <= RS_TOLERANCE && maxV.y - minV.y <= RS_TOLERANCE)) {
        applyFallbackBorders();
    }
}

RS_Vector RS_Text::getNearestEndpoint(const RS_Vector& coord, double* dist)const {
	if (dist) {
        *dist = data.insertionPoint.distanceTo(coord);
    }
    return data.insertionPoint;
}

RS_VectorSolutions RS_Text::getRefPoints() const{
	RS_VectorSolutions ret({data.insertionPoint, data.secondPoint});
	return ret;
}

void RS_Text::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    data.insertionPoint.move(offset);
    data.secondPoint.move(offset);
//    update();
}



void RS_Text::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.secondPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angle);
//    update();
}
void RS_Text::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.secondPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
//    update();
}



void RS_Text::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.insertionPoint.scale(center, factor);
    data.secondPoint.scale(center, factor);
    data.height*=factor.x;
    update();
}



void RS_Text::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    bool readable = RS_Math::isAngleReadable(data.angle);

	RS_Vector vec = RS_Vector::polar(1.0, data.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle = vec.angle();

    bool corr;
    data.angle = RS_Math::makeAngleReadable(data.angle, readable, &corr);

    if (corr) {
        data.insertionPoint.mirror(axisPoint1, axisPoint2);
        data.secondPoint.mirror(axisPoint1, axisPoint2);
        if (data.halign==RS_TextData::HALeft) {
            data.halign=RS_TextData::HARight;
        } else if (data.halign==RS_TextData::HARight) {
            data.halign=RS_TextData::HALeft;
        } else if (data.halign==RS_TextData::HAFit || data.halign==RS_TextData::HAAligned) {
            RS_Vector tmp = data.insertionPoint;
            data.insertionPoint = data.secondPoint;
            data.secondPoint = tmp;
        }
    } else {
        RS_Vector minP = RS_Vector(getMin().x, getMax().y);
        minP = minP.mirror(axisPoint1, axisPoint2);
        double mirrAngle = axisPoint1.angleTo(axisPoint2)*2.0;
        data.insertionPoint.move(minP - getMin());
        data.secondPoint.move(minP - getMin());
        data.insertionPoint.rotate(minP, mirrAngle);
        data.secondPoint.rotate(minP, mirrAngle);
    }
    update();
}



bool RS_Text::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) {
    return false;
}



/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_Text::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Text& p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}


void RS_Text::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/)
{
    if (!(painter && view)) {
        return;
    }

    // No letters could be generated (missing font, or a string that produces no
    // glyphs). Draw the placeholder box in every mode, including print preview
    // and printing: silently omitting the entity would hide from the user that
    // the drawing contains text which cannot be rendered.
    if (isEmpty())
    {
        painter->drawPlaceholderRect(view->toGui(getMin()), view->toGui(getMax()));
        return;
    }

    if (!view->isPrintPreview() && !view->isPrinting())
    {
        if (view->isPanning() || view->toGuiDY(getHeight()) < 4)
        {
            // Performance substitution for glyphs that do exist, so it keeps
            // the entity's own pen - including a deliberate NoPen.
            painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
            return;
        }
    }

    foreach (auto e, entities)
    {
        view->drawEntity(painter, e);
    }
}

