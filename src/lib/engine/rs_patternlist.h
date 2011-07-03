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


#ifndef RS_PATTERNLIST_H
#define RS_PATTERNLIST_H


#include "rs_pattern.h"
#include "rs_entity.h"

#define RS_PATTERNLIST RS_PatternList::instance()

/**
 * The global list of patterns. This is implemented as a singleton.
 * Use RS_PatternList::instance() to get a pointer to the object.
 *
 * @author Andrew Mustun
 */
class RS_PatternList {
protected:
    RS_PatternList();

public:
    /**
     * @return Instance to the unique pattern list.
     */
    static RS_PatternList* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_PatternList();
        }
        return uniqueInstance;
    }

    virtual ~RS_PatternList() {clearPatterns();}

    void init();

    void clearPatterns();
    int countPatterns() {
        return patterns.count();
    }
    virtual void removePattern(RS_Pattern* pattern);
    RS_Pattern* requestPattern(const QString& name);
    //! @return a const iterator for the pattern list.
    QListIterator<RS_Pattern *> getIteretor(){
        return QListIterator<RS_Pattern *>(patterns);
    }

        bool contains(const QString& name);

    //void addPatternListListener(RS_PatternListListener* listener);

    friend std::ostream& operator << (std::ostream& os, RS_PatternList& l);

    //static bool test();

protected:
    static RS_PatternList* uniqueInstance;

private:
    //! patterns in the graphic
    QList<RS_Pattern*> patterns;
    //! List of registered PatternListListeners
    //QList<RS_PatternListListener> patternListListeners;
}
;

#endif
