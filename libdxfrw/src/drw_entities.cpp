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

#include "drw_entities.h"
#include "dxfreader.h"


//! Base class for entities
/*!
*  Base class for entities
*  @author Rallaz
*/
void DRW_Entity::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getString();
        break;
    case 330:
        handleBlock = reader->getString();
        break;
    case 8:
        layer = reader->getString();
        break;
    case 6:
        lineType = reader->getString();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 370:
//        lWeight = (DRW::LWEIGHT)reader->getInt32();
//RLZ: TODO as integer or enum??
        lWeight = reader->getInt32();
        break;
    case 48:
        ltypeScale = reader->getDouble();
        break;
    case 60:
        visible = reader->getBool();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    case 430:
        colorName = reader->getString();
        break;
    default:
        break;
    }
}

void DRW_Point::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10:
        x = reader->getDouble();
        break;
    case 20:
        y = reader->getDouble();
        break;
    case 30:
        z = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 210:
        ex = reader->getDouble();
        break;
    case 220:
        ey = reader->getDouble();
        break;
    case 230:
        ez = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Line::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 11:
        bx = reader->getDouble();
        break;
    case 21:
        by = reader->getDouble();
        break;
    case 31:
        bz = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Circle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        radious = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Arc::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 50:
        staangle = reader->getDouble();
        break;
    case 51:
        endangle = reader->getDouble();
        break;
    default:
        DRW_Circle::parseCode(code, reader);
        break;
    }
}

void DRW_Ellipse::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        ratio = reader->getDouble();
        break;
    case 41:
        staparam = reader->getDouble();
        break;
    case 42:
        endparam = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Trace::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 12:
        cx = reader->getDouble();
        break;
    case 22:
        cy = reader->getDouble();
        break;
    case 32:
        cz = reader->getDouble();
        break;
    case 13:
        dx = reader->getDouble();
        break;
    case 23:
        dy = reader->getDouble();
        break;
    case 33:
        dz = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Solid::parseCode(int code, dxfReader *reader){
        DRW_Trace::parseCode(code, reader);
}

