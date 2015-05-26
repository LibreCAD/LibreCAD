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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <cstdio>

#include "dl_writer_ascii.h"
#include "dl_exception.h"


/**
 * Closes the output file.
 */
void DL_WriterA::close() const {
    m_ofile.close();
}


/**
 * @retval true Opening file has failed.
 * @retval false Otherwise.
 */
bool DL_WriterA::openFailed() const {
	return m_ofile.fail();
}



/**
 * Writes a real (double) variable to the DXF file.
 *
 * @param gc Group code.
 * @param value Double value
 */
void DL_WriterA::dxfReal(int gc, double value) const {
    char str[256];
    snprintf(str, 256,"%.16lf", value);
	
	// fix for german locale:
	strReplace(str, ',', '.');

    // Cut away those zeros at the end:
    bool dot = false;
    int end = -1;
    for (unsigned int i=0; i<strlen(str); ++i) {
        if (str[i]=='.') {
            dot = true;
            end = i+2;
            continue;
        } else if (dot && str[i]!='0') {
            end = i+1;
        }
    }
    if (end>0 && end<(int)strlen(str)) {
        str[end] = '\0';
    }

    dxfString(gc, str);
    m_ofile.flush();
}



/**
 * Writes an int variable to the DXF file.
 *
 * @param gc Group code.
 * @param value Int value
 */
void DL_WriterA::dxfInt(int gc, int value) const {
    m_ofile << (gc<10 ? "  " : (gc<100 ? " " : "")) << gc << "\n"
    << value << "\n";
}



/**
 * Writes a hex int variable to the DXF file.
 *
 * @param gc Group code.
 * @param value Int value
 */
void DL_WriterA::dxfHex(int gc, int value) const {
    char str[12];
    snprintf(str,12, "%0X", value);
    dxfString(gc, str);
}



/**
 * Writes a string variable to the DXF file.
 *
 * @param gc Group code.
 * @param value String
 */
void DL_WriterA::dxfString(int gc, const char* value) const {
    if (value==NULL) {
#ifndef __GCC2x__
        //throw DL_NullStrExc();
#endif
    }
    m_ofile << (gc<10 ? "  " : (gc<100 ? " " : "")) << gc << "\n"
    << value << "\n";
}



void DL_WriterA::dxfString(int gc, const string& value) const {
    m_ofile << (gc<10 ? "  " : (gc<100 ? " " : "")) << gc << "\n"
    << value << "\n";
}


/**
 * Replaces every occurence of src with dest in the null terminated str.
 */
void DL_WriterA::strReplace(char* str, char src, char dest) {
	size_t i;
	for	(i=0; i<strlen(str); i++) {
		if (str[i]==src) {
			str[i] = dest;
		}
	}
}

