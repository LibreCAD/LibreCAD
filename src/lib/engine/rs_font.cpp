/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <iostream>
//#include <values.h>

#include "rs_color.h"
#include "rs_file.h"
#include "rs_fileinfo.h"
#include "rs_fontchar.h"
#include "rs_math.h"
#include "rs_regexp.h"
#include "rs_string.h"
#include "rs_system.h"
#include "rs_textstream.h"


/**
 * Constructor.
 *
 * @param owner true if the font owns the letters (blocks). Otherwise 
 *              the letters will be deleted when the font is deleted.
 */
RS_Font::RS_Font(const RS_String& fileName, bool owner)
        :	letterList(owner) {
    this->fileName = fileName;
	encoding = "";
    loaded = false;
    letterSpacing = 3.0;
    wordSpacing = 6.75;
    lineSpacingFactor = 1.0;
}



/**
 * Loads the font into memory.
 *
 * @retval true font was already loaded or is loaded now.
 * @retval false font could not be loaded.
 */
bool RS_Font::loadFont() {
    RS_DEBUG->print("RS_Font::loadFont");

    if (loaded) {
        return true;
    }

    RS_String path;

    // Search for the appropriate font if we have only the name of the font:
    if (!fileName.lower().contains(".cxf")) {
        RS_StringList fonts = RS_SYSTEM->getFontList();
        RS_FileInfo file;
        for (RS_StringList::Iterator it = fonts.begin();
                it!=fonts.end();
                it++) {

            if (RS_FileInfo(*it).baseName().lower()==fileName.lower()) {
                path = *it;
                break;
            }
        }
    }

    // We have the full path of the font:
    else {
        path = fileName;
    }

    // No font paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Font::loadFont: No fonts available.");
        return false;
    }

    // Open cxf file:
    RS_File f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Font::loadFont: Cannot open font file: %s", 
			path.latin1());
        return false;
    } else {
        RS_DEBUG->print("RS_Font::loadFont: "
			"Successfully opened font file: %s", 
			path.latin1());
    }

    RS_TextStream ts(&f);
    RS_String line;

    // Read line by line until we find a new letter:
    while (!f.atEnd()) {
        line = ts.readLine();

        if (line.isEmpty()) 
            continue;

        // Read font settings:
        if (line.at(0)=='#') {
            RS_StringList lst =
                RS_StringList::split(':', line.right(line.length()-1));
            RS_StringList::Iterator it3 = lst.begin();

			// RVT_PORT sometimes it happens that the size is < 2
			if (lst.size()<2) 
				continue;
			
            RS_String identifier = (*it3).stripWhiteSpace();
            it3++;
            RS_String value = (*it3).stripWhiteSpace();

            if (identifier.lower()=="letterspacing") {
                letterSpacing = value.toDouble();
            } else if (identifier.lower()=="wordspacing") {
                wordSpacing = value.toDouble();
            } else if (identifier.lower()=="linespacingfactor") {
                lineSpacingFactor = value.toDouble();
            } else if (identifier.lower()=="author") {
                authors.append(value);
            } else if (identifier.lower()=="name") {
               	names.append(value);
            } else if (identifier.lower()=="encoding") {
				ts.setCodec(RS_TextCodec::codecForName(value));
				encoding = value;
            }
        }

        // Add another letter to this font:
        else if (line.at(0)=='[') {

            // uniode character:
            RS_Char ch;

            // read unicode:
            RS_RegExp regexp("[0-9A-Fa-f]{4,4}");
            regexp.search(line);
            RS_String cap = regexp.cap();
            if (!cap.isNull()) {
                int uCode = cap.toInt(NULL, 16);
                ch = RS_Char(uCode);
            }

            // read UTF8 (LibreCAD 1 compatibility)
            else if (line.find(']')>=3) {
                int i = line.find(']');
                RS_String mid = line.mid(1, i-1);
                ch = RS_String::fromUtf8(mid.latin1()).at(0);
            }

            // read normal ascii character:
            else {
                ch = line.at(1);
            }

            // create new letter:
            RS_FontChar* letter =
                new RS_FontChar(NULL, ch, RS_Vector(0.0, 0.0));
				
            // Read entities of this letter:
            RS_String coordsStr;
            RS_StringList coords;
            RS_StringList::Iterator it2;
            do {
                line = ts.readLine();

				if (line.isEmpty()) {
					continue;
				}
				
                coordsStr = line.right(line.length()-2);
                coords = RS_StringList::split(',', coordsStr);
                it2 = coords.begin();

                // Line:
                if (line.at(0)=='L') {
                    double x1 = (*it2++).toDouble();
                    double y1 = (*it2++).toDouble();
                    double x2 = (*it2++).toDouble();
                    double y2 = (*it2).toDouble();

                    RS_LineData ld(RS_Vector(x1, y1), RS_Vector(x2, y2));
                    RS_Line* line = new RS_Line(letter, ld);
                    line->setPen(RS_Pen(RS2::FlagInvalid));
                    line->setLayer(NULL);
                    letter->addEntity(line);
                }

                // Arc:
                else if (line.at(0)=='A') {
                    double cx = (*it2++).toDouble();
                    double cy = (*it2++).toDouble();
                    double r = (*it2++).toDouble();
                    double a1 = (*it2++).toDouble()/ARAD;
                    double a2 = (*it2).toDouble()/ARAD;
                    bool reversed = (line.at(1)=='R');

                    RS_ArcData ad(RS_Vector(cx,cy),
                                  r, a1, a2, reversed);
                    RS_Arc* arc = new RS_Arc(letter, ad);
                    arc->setPen(RS_Pen(RS2::FlagInvalid));
                    arc->setLayer(NULL);
                    letter->addEntity(arc);
                }
            } while (!line.isEmpty());

            letter->calculateBorders();
            letterList.add(letter);
        }
    }

    f.close();
    loaded = true;
	
    RS_DEBUG->print("RS_Font::loadFont OK");

    return true;
}


/**
 * Dumps the fonts data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Font& f) {
    os << " Font file name: " << f.getFileName().latin1() << "\n";
    //<< (RS_BlockList&)f << "\n";
    return os;
}

