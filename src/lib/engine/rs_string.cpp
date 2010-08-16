/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#include "rs_string.h"

#include <iostream>


RS_String RS_StringCompat::replace(const RS_String& str,
                                   RS_Char c1, RS_Char c2) {

    RS_String ret = str;

    for (uint i=0; i<ret.length(); ++i) {
        if (ret.at(i)==c1) {
            ret.ref(i) = c2;
        }
    }

    return ret;
}

RS_String RS_StringCompat::replace(const RS_String& str,
                                   const RS_String& s1, const RS_String& s2) {

    if (s1.isEmpty()) {
        return str;
    }

    RS_String ret = "";

    for (uint i=0; i<str.length(); ++i) {
        if (str.mid(i, s1.length())==s1) {
            ret += s2;
			i += s1.length()-1;
        } else {
            ret += str.at(i);
        }
    }

    return ret;
}

void RS_StringCompat::test() {
    RS_String res;
    RS_String s1 = "abcdefg";
    res = RS_StringCompat::replace(s1, 'a', 'A');
    assert(res=="Abcdefg");
    res = RS_StringCompat::replace(s1, 'b', 'B');
    assert(res=="aBcdefg");
    res = RS_StringCompat::replace(s1, 'g', 'G');
    assert(res=="abcdefG");

    res = RS_StringCompat::replace(s1, "", "blah");
    assert(res=="abcdefg");
    res = RS_StringCompat::replace(s1, "ab", "AB");
    assert(res=="ABcdefg");
    res = RS_StringCompat::replace(s1, "def", "DEF");
    assert(res=="abcDEFg");
    res = RS_StringCompat::replace(s1, "g", "G");
    assert(res=="abcdefG");
    res = RS_StringCompat::replace(s1, "fg", "FG");
    assert(res=="abcdeFG");

    s1 = "a";
    res = RS_StringCompat::replace(s1, "a", "ABC");
    assert(res=="ABC");
    s1 = "ab";
    res = RS_StringCompat::replace(s1, "ab", "");
    assert(res=="");
}
