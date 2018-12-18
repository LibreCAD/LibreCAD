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

#include<iostream>
#include<cmath>
#include "rs_font.h"
#include "rs_mtext.h"

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

RS_MTextData::RS_MTextData(const RS_Vector& _insertionPoint,
			double _height,
			double _width,
			VAlign _valign,
			HAlign _halign,
			MTextDrawingDirection _drawingDirection,
			MTextLineSpacingStyle _lineSpacingStyle,
			double _lineSpacingFactor,
			const QString& _text,
			const QString& _style,
			double _angle,
			RS2::UpdateMode _updateMode):
	insertionPoint(_insertionPoint)
	,height(_height)
	,width(_width)
	,valign(_valign)
	,halign(_halign)
	,drawingDirection(_drawingDirection)
	,lineSpacingStyle(_lineSpacingStyle)
	,lineSpacingFactor(_lineSpacingFactor)
	,text(_text)
	,style(_style)
	,angle(_angle)
	,updateMode(_updateMode)
{
}

std::ostream& operator << (std::ostream& os, const RS_MTextData& td) {
	os << "("
	   <<td.insertionPoint<<','
	  <<td.height<<','
	 <<td.width<<','
	<<td.valign<<','
	<<td.halign<<','
	<<td.drawingDirection<<','
	<<td.lineSpacingStyle<<','
	<<td.lineSpacingFactor<<','
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
RS_MText::RS_MText(RS_EntityContainer* parent,
                 const RS_MTextData& d)
        : RS_EntityContainer(parent), data(d) {

    usedTextHeight = 0.0;
    usedTextWidth = 0.0;
    setText(data.text);
}

RS_Entity* RS_MText::clone() const{
	RS_MText* t = new RS_MText(*this);
	t->setOwner(isOwner());
	t->initId();
	t->detach();
	return t;
}

/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_MText::setText(const QString& t) {
    data.text = t;

    // handle some special flags embedded in the text:
    if (data.text.left(4)=="\\A0;") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VABottom;
    } else if (data.text.left(4)=="\\A1;") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VAMiddle;
    } else if (data.text.left(4)=="\\A2;") {
        data.text = data.text.mid(4);
        data.valign = RS_MTextData::VATop;
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
int RS_MText::getAlignment() {
    if (data.valign==RS_MTextData::VATop) {
        if (data.halign==RS_MTextData::HALeft) {
            return 1;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 2;
        } else if (data.halign==RS_MTextData::HARight) {
            return 3;
        }
    } else if (data.valign==RS_MTextData::VAMiddle) {
        if (data.halign==RS_MTextData::HALeft) {
            return 4;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 5;
        } else if (data.halign==RS_MTextData::HARight) {
            return 6;
        }
    } else if (data.valign==RS_MTextData::VABottom) {
        if (data.halign==RS_MTextData::HALeft) {
            return 7;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 8;
        } else if (data.halign==RS_MTextData::HARight) {
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
    switch (a%3) {
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

    switch ((int)ceil(a/3.0)) {
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
void RS_MText::update()
{
    RS_DEBUG->print("RS_MText::update");

    clear();
    if (isUndone()) {
        return;
    }

    usedTextWidth = 0.0;
    usedTextHeight = 0.0;

    RS_Font* font {RS_FONTLIST->requestFont( data.style)};
    if (nullptr == font) {
        return;
    }

    RS_Vector letterPos {RS_Vector( 0.0, -9.0)};
    RS_Vector letterSpace {RS_Vector( font->getLetterSpacing(), 0.0)};
    RS_Vector space {RS_Vector( font->getWordSpacing(), 0.0)};
    int lineCounter {0};

    // Every single text line gets stored in this entity container
    // so we can move the whole line around easely:
    RS_EntityContainer* oneLine {new RS_EntityContainer(this)};

    // First every text line is created with
    //   alignment: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // For every letter:
    for (int i = 0; i < static_cast<int>(data.text.length()); ++i) {
        bool handled {false};

        switch (data.text.at(i).unicode()) {
        case 0x0A:
            // line feed:
            updateAddLine( oneLine, lineCounter++);
            oneLine = new RS_EntityContainer(this);
            letterPos = RS_Vector( 0.0, -9.0);
            break;

        case 0x20:
            // Space:
            letterPos += space;
            break;

        case 0x5C: {
            // code (e.g. \S, \P, ..)
            ++i;
            int ch {data.text.at(i).unicode()};
            switch (ch) {
            case 'P':
                updateAddLine( oneLine, lineCounter++);
                oneLine = new RS_EntityContainer(this);
                letterPos = RS_Vector( 0.0, -9.0);
                handled = true;
                break;

            case 'f':
            case 'F': {
                //font change
                // \f{symbol} changes font to symbol
                // \f{} sets font to standard
                ++i;
                if ('{' != data.text.at(i).unicode()) {
                    --i;
                    continue;
                }

                int j {data.text.indexOf( '}', i)};
                if (j > i) {
                    QString fontName;
                    if (i + 1 == j) {
                        fontName = "standard";
                    }
                    else {
                        fontName = data.text.mid( i + 1, j - i - 1);
                    }

                    RS_Font* fontNew {RS_FONTLIST->requestFont( fontName)};
                    if (nullptr != fontNew) {
                        font = fontNew;
                    }
                    if (nullptr == font) {
                        font = RS_FONTLIST->requestFont( "standard");
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
                while (data.text.at(i).unicode()!='^'
                       && data.text.at(i).unicode()!='\\'
                       && i < static_cast<int>(data.text.length()) ) {
                    upperText += data.text.at(i);
                    ++i;
                }

                ++i;

                if ('^' == data.text.at(i - 1).unicode()
                    && ' ' == data.text.at(i).unicode() ) {
                    ++i;
                }

                // get lower string:
                while (';' != data.text.at(i).unicode()
                       && static_cast<int>(data.text.length()) > i) {
                    lowerText += data.text.at(i);
                    ++i;
                }

                // add texts:
                double upperWidth {0.0};
                if (! upperText.isEmpty()) {
                    RS_MText* upper { new RS_MText( oneLine,
                                                    RS_MTextData( letterPos + RS_Vector( 0.0, 9.0),
                                                                  4.0,
                                                                  100.0,
                                                                  RS_MTextData::VATop,
                                                                  RS_MTextData::HALeft,
                                                                  RS_MTextData::LeftToRight,
                                                                  RS_MTextData::Exact,
                                                                  1.0,
                                                                  upperText,
                                                                  data.style,
                                                                  0.0,
                                                                  RS2::Update)) };
                    upper->setLayer( nullptr);
                    upper->setPen( RS_Pen( RS2::FlagInvalid));
                    upper->calculateBorders();
                    oneLine->addEntity(upper);
                    upperWidth = upper->getSize().x;
                }

                double lowerWidth {0.0};
                if (! lowerText.isEmpty()) {
                    RS_MText* lower { new RS_MText( oneLine,
                                                    RS_MTextData( letterPos + RS_Vector( 0.0, 4.0),
                                                                  4.0,
                                                                  100.0,
                                                                  RS_MTextData::VATop,
                                                                  RS_MTextData::HALeft,
                                                                  RS_MTextData::LeftToRight,
                                                                  RS_MTextData::Exact,
                                                                  1.0,
                                                                  lowerText,
                                                                  data.style,
                                                                  0.0,
                                                                  RS2::Update)) };
                    lower->setLayer( nullptr);
                    lower->setPen( RS_Pen( RS2::FlagInvalid));
                    lower->calculateBorders();
                    oneLine->addEntity(lower);
                    lowerWidth = lower->getSize().x;
                }

                if (upperWidth > lowerWidth) {
                    letterPos += RS_Vector( upperWidth, 0.0);
                }
                else {
                    letterPos += RS_Vector( lowerWidth, 0.0);
                }
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
            QString letterText {QString(data.text.at(i))};
            if (nullptr == font->findLetter( letterText)) {
                RS_DEBUG->print("RS_MText::update: missing font for letter( %s ), replaced it with QChar(0xfffd)",
                                qPrintable( letterText));
                letterText = QChar( 0xfffd);
            }

            RS_DEBUG->print("RS_MText::update: insert a letter at pos: %f/%f", letterPos.x, letterPos.y);

            RS_InsertData d( letterText,
                             letterPos,
                             RS_Vector( 1.0, 1.0),
                             0.0,
                             1,
                             1,
                             RS_Vector( 0.0, 0.0),
                             font->getLetterList(),
                             RS2::NoUpdate);

            RS_Insert* letter {new RS_Insert(this, d)};
            RS_Vector letterWidth;
            letter->setPen( RS_Pen( RS2::FlagInvalid));
            letter->setLayer( nullptr);
            letter->update();
            letter->forcedCalculateBorders();

            letterWidth = RS_Vector( letter->getMax().x - letterPos.x, 0.0);
            if (0 > letterWidth.x) {
                letterWidth.x = -letterSpace.x;
            }

            oneLine->addEntity( letter);

            // next letter position:
            letterPos += letterWidth;
            letterPos += letterSpace;

            break;
        } // outer default
        } // outer switch (data.text.at(i).unicode())
    } // for (i) loop

    double tt {updateAddLine( oneLine, lineCounter)};
    if (RS_MTextData::VABottom == data.valign) {
        RS_Vector ot {RS_Vector( 0.0, -tt).rotate( data.angle)};
        RS_EntityContainer::move( ot);
    }

    usedTextHeight -= data.height * data.lineSpacingFactor * 5.0 / 3.0 - data.height;
    forcedCalculateBorders();

    RS_DEBUG->print("RS_MText::update: OK");
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
double RS_MText::updateAddLine(RS_EntityContainer* textLine, int lineCounter) {
    double ls =5.0/3.0;

    RS_DEBUG->print("RS_MText::updateAddLine: width: %f", textLine->getSize().x);

        //textLine->forcedCalculateBorders();
    //RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f", textLine->getSize().x);

    // Move to correct line position:
    textLine->move(RS_Vector(0.0, -9.0 * lineCounter
                             * data.lineSpacingFactor * ls));

    if( ! RS_EntityContainer::autoUpdateBorders) {
        //only update borders when needed
        textLine->forcedCalculateBorders();
    }
    RS_Vector textSize = textLine->getSize();

        RS_DEBUG->print("RS_MText::updateAddLine: width 2: %f", textSize.x);

    // Horizontal Align:
    switch (data.halign) {
    case RS_MTextData::HACenter:
                RS_DEBUG->print("RS_MText::updateAddLine: move by: %f", -textSize.x/2.0);
        textLine->move(RS_Vector(-textSize.x/2.0, 0.0));
        break;

    case RS_MTextData::HARight:
        textLine->move(RS_Vector(-textSize.x, 0.0));
        break;

    default:
        break;
    }

    // Vertical Align:
    double vSize = getNumberOfLines()*9.0*data.lineSpacingFactor*ls
                   - (9.0*data.lineSpacingFactor*ls - 9.0);

    switch (data.valign) {
    case RS_MTextData::VAMiddle:
        textLine->move(RS_Vector(0.0, vSize/2.0));
        break;

    case RS_MTextData::VABottom:
        textLine->move(RS_Vector(0.0, vSize));
        break;

    default:
        break;
    }

    // Scale:
    textLine->scale(RS_Vector(0.0,0.0),
                    RS_Vector(data.height/9.0, data.height/9.0));

    textLine->forcedCalculateBorders();

    // Update actual text size (before rotating, after scaling!):
    if (textLine->getSize().x>usedTextWidth) {
        usedTextWidth = textLine->getSize().x;
    }

    usedTextHeight += data.height*data.lineSpacingFactor*ls;

    // Gets the distance over text base-line (before rotating, after scaling!):
    double textTail = textLine->getMin().y;

    // Rotate:
    textLine->rotate(RS_Vector(0.0,0.0), data.angle);

    // Move:
    textLine->move(data.insertionPoint);
    textLine->setPen(RS_Pen(RS2::FlagInvalid));
    textLine->setLayer(NULL);
    textLine->forcedCalculateBorders();

    addEntity(textLine);
    return textTail;
}


RS_Vector RS_MText::getNearestEndpoint(const RS_Vector& coord, double* dist)const {
    if (dist) {
        *dist = data.insertionPoint.distanceTo(coord);
    }
    return data.insertionPoint;
}


RS_VectorSolutions RS_MText::getRefPoints() const{
		return RS_VectorSolutions({data.insertionPoint});
}

void RS_MText::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    data.insertionPoint.move(offset);
//    update();
}



void RS_MText::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angle);
//    update();
}
void RS_MText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
//    update();
}



void RS_MText::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.insertionPoint.scale(center, factor);
    data.width*=factor.x;
    data.height*=factor.x;
    update();
}



void RS_MText::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    //double ang = axisPoint1.angleTo(axisPoint2);
    bool readable = RS_Math::isAngleReadable(data.angle);

	RS_Vector vec = RS_Vector::polar(1.0, data.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle = vec.angle();

    bool corr;
    data.angle = RS_Math::makeAngleReadable(data.angle, readable, &corr);

    if (corr) {
        if (data.halign==RS_MTextData::HALeft) {
            data.halign=RS_MTextData::HARight;
        } else if (data.halign==RS_MTextData::HARight) {
            data.halign=RS_MTextData::HALeft;
        }
    } else {
        if (data.valign==RS_MTextData::VATop) {
            data.valign=RS_MTextData::VABottom;
        } else if (data.valign==RS_MTextData::VABottom) {
            data.valign=RS_MTextData::VATop;
        }
    }
    update();
}



bool RS_MText::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) {
    return false;
}



/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_MText::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_MText& p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}

void RS_MText::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/)
{
    if (!(painter && view)) {
        return;
    }

    if (!view->isPrintPreview() && !view->isPrinting())
    {
        if (view->isPanning() || view->toGuiDY(getHeight()) < 4)
        {
            painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
            return;
        }
    }

    foreach (auto e, entities)
    {
        view->drawEntity(painter, e);
    }
}
