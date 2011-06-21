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


#include "rs_patternlist.h"

#include "rs_stringlist.h"
#include "rs_system.h"

RS_PatternList* RS_PatternList::uniqueInstance = NULL;



/**
 * Default constructor.
 */
RS_PatternList::RS_PatternList() {
    patterns.setAutoDelete(true);
    //patternListListeners.setAutoDelete(false);
}



/**
 * Initializes the pattern list by creating empty RS_Pattern 
 * objects, one for each pattern that could be found.
 */
void RS_PatternList::init() {
    RS_DEBUG->print("RS_PatternList::initPatterns");

    RS_StringList list = RS_SYSTEM->getPatternList();
    RS_Pattern* pattern;

	patterns.clear();

    for (RS_StringList::Iterator it = list.begin();
            it != list.end(); ++it) {
        RS_DEBUG->print("pattern: %s:", (*it).latin1());

        QFileInfo fi(*it);
        pattern = new RS_Pattern(fi.baseName().toLower());
        patterns.append(pattern);

        RS_DEBUG->print("base: %s", pattern->getFileName().latin1());
    }
}



/**
 * Removes all patterns in the patternlist.
 */
void RS_PatternList::clearPatterns() {
    patterns.clear();
}



/**
 * Removes a pattern from the list.
 * Listeners are notified after the pattern was removed from 
 * the list but before it gets deleted.
 */
void RS_PatternList::removePattern(RS_Pattern* pattern) {
    RS_DEBUG->print("RS_PatternList::removePattern()");

    // here the pattern is removed from the list but not deleted
    patterns.remove(pattern);

    //for (uint i=0; i<patternListListeners.count(); ++i) {
    //    RS_PatternListListener* l = patternListListeners.at(i);
    //    l->patternRemoved(pattern);
    //}
}



/**
 * @return Pointer to the pattern with the given name or
 * \p NULL if no such pattern was found. The pattern will be loaded into
 * memory if it's not already.
 */
RS_Pattern* RS_PatternList::requestPattern(const RS_String& name) {
    RS_DEBUG->print("RS_PatternList::requestPattern %s", name.latin1());

    RS_String name2 = name.lower();
    RS_Pattern* foundPattern = NULL;

    RS_DEBUG->print("name2: %s", name2.latin1());

    // Search our list of available patterns:
    for (RS_Pattern* p=patterns.first();
            p!=NULL;
            p=patterns.next()) {

        if (p->getFileName()==name2) {
            // Make sure this pattern is loaded into memory:
            p->loadPattern();
            foundPattern = p;
            break;
        }
    }

    //if (foundPattern==NULL && name!="standard") {
    //    foundPattern = requestPattern("standard");
    //}

    return foundPattern;
}

	
bool RS_PatternList::contains(const RS_String& name) {
    RS_String name2 = name.lower();

    // Search our list of available patterns:
    for (RS_Pattern* p=patterns.first();
            p!=NULL;
            p=patterns.next()) {

        if (p->getFileName()==name2) {
			return true;
		}
	}

	return false;
}


/**
 * Dumps the patterns to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_PatternList& l) {

    os << "Patternlist: \n";
    for (RS_Pattern* p=l.firstPattern();
            p!=NULL;
            p=l.nextPattern()) {

        os << *p << "\n";
    }

    return os;
}

