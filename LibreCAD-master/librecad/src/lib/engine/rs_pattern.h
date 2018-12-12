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


#ifndef RS_PATTERN_H
#define RS_PATTERN_H

#include "rs_entitycontainer.h"

class RS_PatternList;

/**
 * Patterns are used for hatches. They are stored in a RS_PatternList.
 * Use RS_PatternList to access a pattern.
 *
 * @author Andrew Mustun
 */
class RS_Pattern : public RS_EntityContainer {
public:
    RS_Pattern(const QString& fileName);
	virtual ~RS_Pattern()=default;
	RS2::EntityType rtti() const{
		return RS2::EntityPattern;
	}

    virtual bool loadPattern();
	
    /** @return the fileName of this pattern. */
	QString getFileName() const;

protected:
    //! Pattern file name
    QString fileName;

    //! Is this pattern currently loaded into memory?
    bool loaded;

	
};


#endif
