/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <math.h>
#include "drw_objects.h"
#include "dxfreader.h"

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
void DRW_TableEntry::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getString();
        break;
    case 330:
        handleBlock = reader->getString();
        break;
    case 2:
        name = reader->getString();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        break;
    }
}

//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*  @author Rallaz
*/
void DRW_LType::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 3:
        desc = reader->getString();
        break;
    case 73:
        size = reader->getInt32();
        path.reserve(size);
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 49:
        path.push_back(reader->getDouble());
        pathIdx++;
        break;
/*    case 74:
        haveShape = reader->getInt32();
        break;*/
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}

//! Update line type
/*!
*  Update the size and length of line type acording to the path
*  @author Rallaz
*/
/*TODO: control max length permited */
void DRW_LType::update(){
    double d =0;
    size = path.size();
    for (int i = 0;  i< size; i++){
        d += fabs(path.at(i));
    }
    length = d;
}

//! Class to handle layer entries
/*!
*  Class to handle layer symbol table entries
*  @author Rallaz
*/
void DRW_Layer::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 6:
        lineType = reader->getString();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 290:
        plotF = reader->getBool();
        break;
    case 370:
        lWeight = reader->getInt32();
        break;
    case 390:
        handlePlotS = reader->getString();
        break;
    case 347:
        handlePlotM = reader->getString();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
        break;
    }
}
