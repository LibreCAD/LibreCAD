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

#include <iostream>
#include <math.h>
#include "drw_objects.h"
#include "dxfreader.h"
#include "dxfwriter.h"

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

void DRW_ImageDef::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        name = reader->getString();
        break;
    case 5:
        handle = reader->getString();
        break;
    case 10:
        u = reader->getDouble();
        break;
    case 20:
        v = reader->getDouble();
        break;
    case 11:
        up = reader->getDouble();
        break;
    case 12:
        vp = reader->getDouble();
        break;
    case 280:
        loaded = reader->getInt32();
        break;
    case 281:
        resolution = reader->getInt32();
        break;
    default:
        break;
    }
}


void DRW_Header::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 9:
        curr = new DRW_Variant();
        name = reader->getString();
        vars[name]=curr;
        break;
    case 1:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 2:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 3:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 6:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 7:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 8:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    case 10:
        curr->addCoord(new DRW_Coord());
        curr->setCoordX(reader->getDouble());
        curr->code = code;
        break;
    case 20:
        curr->setCoordY(reader->getDouble());
        break;
    case 30:
        curr->setCoordZ(reader->getDouble());
        curr->code = code;
        break;
    case 40:
        curr->addDouble(reader->getDouble());
        curr->code = code;
        break;
    case 50:
        curr->addDouble(reader->getDouble());
        curr->code = code;
        break;
    case 62:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 70:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 280:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 290:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 370:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 380:
        curr->addInt(reader->getInt32());
        curr->code = code;
        break;
    case 390:
        curr->addString(reader->getString());
        curr->code = code;
        break;
    default:
        break;
    }
}

void DRW_Header::write(dxfWriter *writer, DRW::Version ver){
/*TODO complete all vars to AC1024*/
    double varDouble;
    int varInt;
    std::string varStr;
    DRW_Coord varCoord;
    writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER");
    switch (ver) {
    case DRW::AC1009: //unsupported version
    case DRW::AC1012:
        writer->writeString(1, "AC1012");
        break;
    case DRW::AC1014:
        writer->writeString(1, "AC1014");
        break;
    case DRW::AC1018:
        writer->writeString(1, "AC1018");
        break;
    case DRW::AC1021:
        writer->writeString(1, "AC1018");
        break;
    case DRW::AC1024:
        writer->writeString(1, "AC1018");
        break;
//    case DRW::AC1015:
//acad2000 default version
    default:
        writer->writeString(1, "AC1015");
        break;
    }

    if (ver > DRW::AC1014) {
        writer->writeString(9, "$HANDSEED");
//RLZ        dxfHex(5, 0xFFFF);
        writer->writeString(5, "20000");
    }

    if (getDouble("$DIMASZ", &varDouble)) {
        writer->writeString(9, "$DIMASZ");
        writer->writeDouble(40, varDouble);
    }
    if (getDouble("$DIMEXE", &varDouble)) {
        writer->writeString(9, "$DIMEXE");
        writer->writeDouble(40, varDouble);
    }
    if (getDouble("$DIMEXO", &varDouble)) {
        writer->writeString(9, "$DIMEXO");
        writer->writeDouble(40, varDouble);
    }
    if (getDouble("$DIMGAP", &varDouble)) {
        writer->writeString(9, "$DIMGAP");
        writer->writeDouble(40, varDouble);
    }
    if (getDouble("$DIMTXT", &varDouble)) {
        writer->writeString(9, "$DIMTXT");
        writer->writeDouble(40, varDouble);
    }
    if (getCoord("$INSBASE", &varCoord)) {
        writer->writeString(9, "$INSBASE");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
        writer->writeDouble(30, varCoord.z);
    }
    if (getCoord("$EXTMIN", &varCoord)) {
        writer->writeString(9, "$EXTMIN");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
        writer->writeDouble(30, varCoord.z);
    }
    if (getCoord("$EXTMAX", &varCoord)) {
        writer->writeString(9, "$EXTMAX");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
        writer->writeDouble(30, varCoord.z);
    }
    if (getCoord("$LIMMIN", &varCoord)) {
        writer->writeString(9, "$LIMMIN");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
    }
    if (getCoord("$LIMMAX", &varCoord)) {
        writer->writeString(9, "$LIMMAX");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
    }
    if (getInt("$ORTHOMODE", &varInt)) {
        writer->writeString(9, "$ORTHOMODE");
        writer->writeInt16(70, varInt);
    }
    if (getStr("$DWGCODEPAGE", &varStr)) {
        writer->writeString(9, "$DWGCODEPAGE");
        writer->writeString(3, varStr);
    }
    if (getCoord("$PLIMMIN", &varCoord)) {
        writer->writeString(9, "$PLIMMIN");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
    }
    if (getCoord("$PLIMMAX", &varCoord)) {
        writer->writeString(9, "$PLIMMAX");
        writer->writeDouble(10, varCoord.x);
        writer->writeDouble(20, varCoord.y);
    }
    if (ver > DRW::AC1014) {
        if (getInt("$INSUNITS", &varInt)) {
            writer->writeString(9, "$INSUNITS");
            writer->writeInt16(70, varInt);
        }
    }

    std::map<std::string,DRW_Variant *>::const_iterator it;
    for ( it=vars.begin() ; it != vars.end(); it++ ){
//        QString key = QString::fromStdString((*it).first);
        std::cerr << (*it).first << std::endl;
    }

}

bool DRW_Header::getDouble(string key, double *varDouble){
    bool result = false;
    std::map<std::string,DRW_Variant *>::iterator it;
    it=vars.find( key);
    if (it != vars.end()) {
        DRW_Variant *var = (*it).second;
        if (var->type == DRW_Variant::DOUBLE) {
            *varDouble = var->content.d;
            result = true;
        }
        vars.erase (it);
    }
    return result;
}

bool DRW_Header::getInt(string key, int *varInt){
    bool result = false;
    std::map<std::string,DRW_Variant *>::iterator it;
    it=vars.find( key);
    if (it != vars.end()) {
        DRW_Variant *var = (*it).second;
        if (var->type == DRW_Variant::INTEGER) {
            *varInt = var->content.i;
            result = true;
        }
        vars.erase (it);
    }
    return result;
}

bool DRW_Header::getStr(string key, std::string *varStr){
    bool result = false;
    std::map<std::string,DRW_Variant *>::iterator it;
    it=vars.find( key);
    if (it != vars.end()) {
        DRW_Variant *var = (*it).second;
        if (var->type == DRW_Variant::STRING) {
            *varStr = *var->content.s;
            result = true;
        }
        vars.erase (it);
    }
    return result;
}

bool DRW_Header::getCoord(string key, DRW_Coord *varCoord){
    bool result = false;
    std::map<std::string,DRW_Variant *>::iterator it;
    it=vars.find( key);
    if (it != vars.end()) {
        DRW_Variant *var = (*it).second;
        if (var->type == DRW_Variant::COORD) {
            *varCoord = *var->content.v;
            result = true;
        }
        vars.erase (it);
    }
    return result;
}
