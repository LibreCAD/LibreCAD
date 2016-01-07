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

#include <QString>
#include "rs_utility.h"


/**
 * Converts a double to a string cutting away unnecessary 0's.
 * e.g. 2.70000  -> 2.7
 */
QString RS_Utility::doubleToString(double value, int precision) {
    QString ret;

    ret.setNum(value, 'f', precision);

    if(ret.contains('.')) {
        // remove trailing zeros:
        while(ret.at(ret.length()-1)=='0') {
            ret.truncate(ret.length()-1);
        }

        // remove trailing .
        if(ret.at(ret.length()-1)=='.') {
            ret.truncate(ret.length()-1);
        }
    }

    return ret;
}

// EOF

