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


#ifndef RS_PATTERNLIST_H
#define RS_PATTERNLIST_H

#include<map>
#include<memory>

class RS_Pattern;
class QString;

#define RS_PATTERNLIST RS_PatternList::instance()

/**
 * The global list of patterns. This is implemented as a singleton.
 * Use RS_PatternList::instance() to get a pointer to the object.
 *
 * @author Andrew Mustun
 */
class RS_PatternList {
	using PTN_MAP = std::map<QString, std::unique_ptr<RS_Pattern>>;
	RS_PatternList() = default;

public:
    /**
     * @return Instance to the unique pattern list.
     */
	static RS_PatternList* instance();

	~RS_PatternList();
	RS_PatternList(RS_PatternList const&) = delete;
	RS_PatternList& operator = (RS_PatternList const&) = delete;
	RS_PatternList(RS_PatternList &&) = delete;
	RS_PatternList& operator = (RS_PatternList &&) = delete;

	void init();

	int countPatterns() const {
		return patterns.size();
    }

	//! \{ range based loop support
	PTN_MAP::iterator begin() {
		return patterns.begin();
	}
	PTN_MAP::const_iterator begin() const{
		return patterns.begin();
	}
	PTN_MAP::iterator end() {
		return patterns.end();
	}
	PTN_MAP::const_iterator end() const{
		return patterns.end();
	}
	//! \}

	RS_Pattern* requestPattern(const QString& name);

	bool contains(const QString& name) const;

    friend std::ostream& operator << (std::ostream& os, RS_PatternList& l);


private:
    //! patterns in the graphic
	PTN_MAP patterns;
};

#endif
