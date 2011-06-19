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

#include <iostream>
#include <QTextStream>
#include <QTextCodec>

#include "rs_fontchar.h"
#include "rs_system.h"

/**
 * Constructor.
 *
 * @param owner true if the font owns the letters (blocks). Otherwise 
 *              the letters will be deleted when the font is deleted.
 */
RS_Font::RS_Font(const QString& fileName, bool owner)
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

    QString path;

    // Search for the appropriate font if we have only the name of the font:
    if (!fileName.toLower().contains(".cxf")) {
        QStringList fonts = RS_SYSTEM->getFontList();
        QFileInfo file;
        for (QStringList::Iterator it = fonts.begin();
                it!=fonts.end();
                it++) {

            if (QFileInfo(*it).baseName().toLower()==fileName.toLower()) {
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
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Font::loadFont: Cannot open font file: %s", 
                        path.toLatin1().data());
        return false;
    } else {
        RS_DEBUG->print("RS_Font::loadFont: "
			"Successfully opened font file: %s", 
                        path.toLatin1().data());
    }

    QTextStream ts(&f);
    QString line;

    // Read line by line until we find a new letter:
    while (!ts.atEnd()) {
        line = ts.readLine();

        if (line.isEmpty()) 
            continue;

        // Read font settings:
        if (line.at(0)=='#') {
            QStringList lst =
                ( line.right(line.length()-1) ).split(':', QString::SkipEmptyParts);
            QStringList::Iterator it3 = lst.begin();

			// RVT_PORT sometimes it happens that the size is < 2
			if (lst.size()<2) 
				continue;
			
            QString identifier = (*it3).trimmed();
            it3++;
            QString value = (*it3).trimmed();

            if (identifier.toLower()=="letterspacing") {
                letterSpacing = value.toDouble();
            } else if (identifier.toLower()=="wordspacing") {
                wordSpacing = value.toDouble();
            } else if (identifier.toLower()=="linespacingfactor") {
                lineSpacingFactor = value.toDouble();
            } else if (identifier.toLower()=="author") {
                authors.append(value);
            } else if (identifier.toLower()=="name") {
               	names.append(value);
            } else if (identifier.toLower()=="encoding") {
                                ts.setCodec(QTextCodec::codecForName(value.toLatin1()));
				encoding = value;
            }
        }

        // Add another letter to this font:
        else if (line.at(0)=='[') {

            // uniode character:
            QChar ch;

            // read unicode:
            QRegExp regexp("[0-9A-Fa-f]{4,4}");
            regexp.indexIn(line);
            QString cap = regexp.cap();
            if (!cap.isNull()) {
                int uCode = cap.toInt(NULL, 16);
                ch = QChar(uCode);
            }

            // read UTF8 (LibreCAD 1 compatibility)
            else if (line.indexOf(']')>=3) {
                int i = line.indexOf(']');
                QString mid = line.mid(1, i-1);
                ch = QString::fromUtf8(mid.toLatin1()).at(0);
            }

            // read normal ascii character:
            else {
                ch = line.at(1);
            }

            // create new letter:
            RS_FontChar* letter =
                new RS_FontChar(NULL, ch, RS_Vector(0.0, 0.0));
				
            // Read entities of this letter:
            QString coordsStr;
            QStringList coords;
            QStringList::Iterator it2;
            do {
                line = ts.readLine();

				if (line.isEmpty()) {
					continue;
				}
				
                coordsStr = line.right(line.length()-2);
//                coords = QStringList::split(',', coordsStr);
                coords = coordsStr.split(',', QString::SkipEmptyParts);
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
    os << " Font file name: " << f.getFileName().toLatin1().data() << "\n";
    //<< (RS_BlockList&)f << "\n";
    return os;
}

