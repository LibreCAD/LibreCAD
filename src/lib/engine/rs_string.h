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


#ifndef RS_STRING_H
#define RS_STRING_H

#include <assert.h>

#include <qstring.h>

#define STR(x)   #x
#define XSTR(x)  STR(x)

#define RS_String QString
#define RS_Char QChar

class RS_StringCompat {
public:
    static RS_String replace(const RS_String& str, RS_Char c1, RS_Char c2);
    static RS_String replace(const RS_String& str, 
		const RS_String& s1, const RS_String& s2);
	static void test();
};

#endif
