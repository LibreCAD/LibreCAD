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
#include "rs_mtext.h"

#include "rs_fontlist.h"
#include "rs_insert.h"

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



/**
 * Sets a new text. The entities representing the
 * text are updated.
 */
void RS_MText::setText(const QString& t) {
    data.text = t;

    // handle some special flags embedded in the text:
    if (data.text.left(4)=="\\A0;") {
        data.text = data.text.mid(4);
        data.valign = RS2::VAlignBottom;
    } else if (data.text.left(4)=="\\A1;") {
        data.text = data.text.mid(4);
        data.valign = RS2::VAlignMiddle;
    } else if (data.text.left(4)=="\\A2;") {
        data.text = data.text.mid(4);
        data.valign = RS2::VAlignTop;
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
    if (data.valign==RS2::VAlignTop) {
        if (data.halign==RS2::HAlignLeft) {
            return 1;
        } else if (data.halign==RS2::HAlignCenter) {
            return 2;
        } else if (data.halign==RS2::HAlignRight) {
            return 3;
        }
    } else if (data.valign==RS2::VAlignMiddle) {
        if (data.halign==RS2::HAlignLeft) {
            return 4;
        } else if (data.halign==RS2::HAlignCenter) {
            return 5;
        } else if (data.halign==RS2::HAlignRight) {
            return 6;
        }
    } else if (data.valign==RS2::VAlignBottom) {
        if (data.halign==RS2::HAlignLeft) {
            return 7;
        } else if (data.halign==RS2::HAlignCenter) {
            return 8;
        } else if (data.halign==RS2::HAlignRight) {
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
        data.halign = RS2::HAlignLeft;
        break;
    case 2:
        data.halign = RS2::HAlignCenter;
        break;
    case 0:
        data.halign = RS2::HAlignRight;
        break;
    }

    switch ((int)ceil(a/3.0)) {
    default:
    case 1:
        data.valign = RS2::VAlignTop;
        break;
    case 2:
        data.valign = RS2::VAlignMiddle;
        break;
    case 3:
        data.valign = RS2::VAlignBottom;
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
void RS_MText::update() {

    RS_DEBUG->print("RS_Text::update");

    clear();

    if (isUndone()) {
        return;
    }

    usedTextWidth = 0.0;
    usedTextHeight = 0.0;

    RS_Font* font = RS_FONTLIST->requestFont(data.style);

    if (font==NULL) {
        return;
    }

    RS_Vector letterPos = RS_Vector(0.0, -9.0);
    RS_Vector letterSpace = RS_Vector(font->getLetterSpacing(), 0.0);
    RS_Vector space = RS_Vector(font->getWordSpacing(), 0.0);
    int lineCounter = 0;

    // Every single text line gets stored in this entity container
    //  so we can move the whole line around easely:
    RS_EntityContainer* oneLine = new RS_EntityContainer(this);

    // First every text line is created with
    //   alignement: top left
    //   angle: 0
    //   height: 9.0
    // Rotation, scaling and centering is done later

    // For every letter:
    for (int i=0; i<(int)data.text.length(); ++i) {
        bool handled = false;
        switch (data.text.at(i).unicode()) {
        case 0x0A:
            // line feed:
            updateAddLine(oneLine, lineCounter++);
            oneLine = new RS_EntityContainer(this);
            letterPos = RS_Vector(0.0, -9.0);
            handled = true;
            break;

        case 0x20:
            // Space:
            letterPos+=space;
            handled = true;
            break;

        case 0x5C: {
                // code (e.g. \S, \P, ..)
                i++;
                int ch = data.text.at(i).unicode();
                switch (ch) {
                case 'P':
                    updateAddLine(oneLine, lineCounter++);
                    oneLine = new RS_EntityContainer(this);
                    letterPos = RS_Vector(0.0, -9.0);
                    handled = true;
                    break;
                    case 'f':
                    case 'F':
                    //font change
                    // \f{symbol} changes font to symbol
                    // \f{} sets font to standard
                {
                    i++;
                    if(data.text.at(i).unicode()!='{') {
                        i--;
                        continue;
                    }
                    int j=data.text.indexOf('}',i);
                    if(j>i){
                        //
                        QString fontName;
                        if(j==i+1)
                            fontName="standard";
                        else
                            fontName=data.text.mid(i+1,j-i-1);
                        RS_Font* fontNew = RS_FONTLIST->requestFont(
                                    fontName
                                    );
                        if(fontNew != NULL) {
                            font=fontNew;
                        }
                        if(font==NULL) font = RS_FONTLIST->requestFont("standard");
                        i=j;
                    }
                }
                        continue;

                case 'S': {
                        QString up;
                        QString dw;
                        //letterPos += letterSpace;

                        // get upper string:
                        i++;
                        while (data.text.at(i).unicode()!='^' &&
                                                       //data.text.at(i).unicode()!='/' &&
                                                       data.text.at(i).unicode()!='\\' &&
                                                       //data.text.at(i).unicode()!='#' &&
                                i<(int)data.text.length()) {
                            up += data.text.at(i);
                            i++;
                        }

                        i++;

                                                if (data.text.at(i-1).unicode()=='^' &&
                                                     data.text.at(i).unicode()==' ') {
                                                        i++;
                                                }

                        // get lower string:
                        while (data.text.at(i).unicode()!=';' &&
                                i<(int)data.text.length()) {
                            dw += data.text.at(i);
                            i++;
                        }

                        // add texts:
                        RS_MText* upper =
                            new RS_MText(
                                oneLine,
                                RS_MTextData(letterPos + RS_Vector(0.0,9.0),
                                            4.0, 100.0, RS2::VAlignTop,
                                                                                        RS2::HAlignLeft,
                                            RS2::LeftToRight, RS2::Exact,
                                            1.0, up, data.style,
                                            0.0, RS2::Update));
                                                upper->setLayer(NULL);
                        upper->setPen(RS_Pen(RS2::FlagInvalid));
                                                oneLine->addEntity(upper);

                        RS_MText* lower =
                            new RS_MText(
                                oneLine,
                                RS_MTextData(letterPos+RS_Vector(0.0,4.0),
                                            4.0, 100.0, RS2::VAlignTop,
                                                                                        RS2::HAlignLeft,
                                            RS2::LeftToRight, RS2::Exact,
                                            1.0, dw, data.style,
                                            0.0, RS2::Update));
                                                lower->setLayer(NULL);
                        lower->setPen(RS_Pen(RS2::FlagInvalid));
                        oneLine->addEntity(lower);

                        // move cursor:
                        upper->calculateBorders();
                        lower->calculateBorders();

                        double w1 = upper->getSize().x;
                        double w2 = lower->getSize().x;

                        if (w1>w2) {
                            letterPos += RS_Vector(w1, 0.0);
                        } else {
                            letterPos += RS_Vector(w2, 0.0);
                        }
                        letterPos += letterSpace;
                    }
                    handled = true;
                    break;

                default:
                    i--;
                    break;
                }
            }
            //if char is not handled continue in default: statement
            if (handled)
                break;

        default: {
                // One Letter:
                QString letterText = QString(data.text.at(i));
                if (font->findLetter(letterText) == NULL) {
                    RS_DEBUG->print("RS_Text::update: missing font for letter( %s ), replaced it with QChar(0xfffd)",qPrintable(letterText));
                    letterText = QChar(0xfffd);
                }
//                if (font->findLetter(QString(data.text.at(i))) != NULL) {

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

                                        // until 2.0.4.5:
                    //letterWidth = RS_Vector(letter->getSize().x, 0.0);
                                        // from 2.0.4.6:
                    letterWidth = RS_Vector(letter->getMax().x-letterPos.x, 0.0);
                    if (letterWidth.x < 0)
                        letterWidth.x = -letterSpace.x;

                    oneLine->addEntity(letter);

                    // next letter position:
                    letterPos += letterWidth;
                    letterPos += letterSpace;
//                }
            }
            break;
        }
    }

    updateAddLine(oneLine, lineCounter);
    usedTextHeight -= data.height*data.lineSpacingFactor*1.6
                      - data.height;
    forcedCalculateBorders();

    RS_DEBUG->print("RS_Text::update: OK");
}



/**
 * Used internally by update() to add a text line created with
 * default values and alignment to this text container.
 *
 * @param textLine The text line.
 * @param lineCounter Line number.
 */
void RS_MText::updateAddLine(RS_EntityContainer* textLine, int lineCounter) {
    RS_DEBUG->print("RS_Text::updateAddLine: width: %f", textLine->getSize().x);

        //textLine->forcedCalculateBorders();
    //RS_DEBUG->print("RS_Text::updateAddLine: width 2: %f", textLine->getSize().x);

    // Move to correct line position:
    textLine->move(RS_Vector(0.0, -9.0 * lineCounter
                             * data.lineSpacingFactor * 1.6));

    if( ! RS_EntityContainer::autoUpdateBorders) {
        //only update borders when needed
        textLine->forcedCalculateBorders();
    }
    RS_Vector textSize = textLine->getSize();

        RS_DEBUG->print("RS_Text::updateAddLine: width 2: %f", textSize.x);

    // Horizontal Align:
    switch (data.halign) {
    case RS2::HAlignCenter:
                RS_DEBUG->print("RS_Text::updateAddLine: move by: %f", -textSize.x/2.0);
        textLine->move(RS_Vector(-textSize.x/2.0, 0.0));
        break;

    case RS2::HAlignRight:
        textLine->move(RS_Vector(-textSize.x, 0.0));
        break;

    default:
        break;
    }

    // Vertical Align:
    double vSize = getNumberOfLines()*9.0*data.lineSpacingFactor*1.6
                   - (9.0*data.lineSpacingFactor*1.6 - 9.0);

    switch (data.valign) {
    case RS2::VAlignMiddle:
        textLine->move(RS_Vector(0.0, vSize/2.0));
        break;

    case RS2::VAlignBottom:
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

    usedTextHeight += data.height*data.lineSpacingFactor*1.6;

    // Rotate:
    textLine->rotate(RS_Vector(0.0,0.0), data.angle);

    // Move:
    textLine->move(data.insertionPoint);
    textLine->setPen(RS_Pen(RS2::FlagInvalid));
    textLine->setLayer(NULL);
    textLine->forcedCalculateBorders();

    addEntity(textLine);
}



RS_VectorSolutions RS_MText::getRefPoints() {
        RS_VectorSolutions ret(data.insertionPoint);
        return ret;
}


RS_Vector RS_MText::getNearestRef(const RS_Vector& coord,
                                     double* dist) {

        //return getRefPoints().getClosest(coord, dist);
        return RS_Entity::getNearestRef(coord, dist);
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

    RS_Vector vec;
    vec.setPolar(1.0, data.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle = vec.angle();

    bool corr;
    data.angle = RS_Math::makeAngleReadable(data.angle, readable, &corr);

    if (corr) {
        if (data.halign==RS2::HAlignLeft) {
            data.halign=RS2::HAlignRight;
        } else if (data.halign==RS2::HAlignRight) {
            data.halign=RS2::HAlignLeft;
        }
    } else {
        if (data.valign==RS2::VAlignTop) {
            data.valign=RS2::VAlignBottom;
        } else if (data.valign==RS2::VAlignBottom) {
            data.valign=RS2::VAlignTop;
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
