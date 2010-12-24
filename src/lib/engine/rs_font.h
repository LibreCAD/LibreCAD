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


#ifndef RS_FONT_H
#define RS_FONT_H

#include <iostream>

#include "rs_blocklist.h"
#include "rs_string.h"
#include "rs_stringlist.h"


/**
 * Class for representing a font. This is implemented as a RS_Graphic
 * with a name (the font name) and several blocks, one for each letter
 * in the font.
 *
 * @author Andrew Mustun
 */
class RS_Font {
public:
    RS_Font(const RS_String& name, bool owner=true);
    //RS_Font(const char* name);

    /** @return the fileName of this font. */
    RS_String getFileName() const {
        return fileName;
    }
	
    /** @return the encoding of this font. */
    RS_String getEncoding() const {
        return encoding;
    }
	
    /** @return the alternative names of this font. */
    const RS_StringList& getNames() const {
        return names;
    }
	
    /** @return the author(s) of this font. */
    const RS_StringList& getAuthors() const {
        return authors;
    }

    /** @return Default letter spacing for this font */
    double getLetterSpacing() {
        return letterSpacing;
    }

    /** @return Default word spacing for this font */
    double getWordSpacing() {
        return wordSpacing;
    }

    /** @return Default line spacing factor for this font */
    double getLineSpacingFactor() {
        return lineSpacingFactor;
    }

    bool loadFont();

	// Wrappers for block list (letters) functions
	RS_BlockList* getLetterList() {
		return &letterList;
	}
    RS_Block* findLetter(const RS_String& name) {
		return letterList.find(name);
	}
    uint countLetters() {
        return letterList.count();
    }
    RS_Block* letterAt(uint i) {
		return letterList.at(i);
	}

    friend std::ostream& operator << (std::ostream& os, const RS_Font& l);

    friend class RS_FontList;

private:
	//! block list (letters)
	RS_BlockList letterList;

    //! Font file name
    RS_String fileName;
	
    //! Font encoding (see docu for QTextCodec)
    RS_String encoding;

	//! Font names
	RS_StringList names;
	
	//! Authors
	RS_StringList authors;

    //! Is this font currently loaded into memory?
    bool loaded;

    //! Default letter spacing for this font
    double letterSpacing;

    //! Default word spacing for this font
    double wordSpacing;

    //! Default line spacing factor for this font
    double lineSpacingFactor;
};

#endif

