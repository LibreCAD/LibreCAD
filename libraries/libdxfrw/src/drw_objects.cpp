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
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        break;
    }
}

//! Class to handle dimstyle entries
/*!
*  Class to handle ldim style symbol table entries
*  @author Rallaz
*/
void DRW_Dimstyle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 105:
        handle = reader->getString();
        break;
    case 3:
        dimpost = reader->getUtf8String();
        break;
    case 4:
        dimapost = reader->getUtf8String();
        break;
    case 5:
        dimblk = reader->getUtf8String();
        break;
    case 6:
        dimblk1 = reader->getUtf8String();
        break;
    case 7:
        dimblk2 = reader->getUtf8String();
        break;
    case 40:
        dimscale = reader->getDouble();
        break;
    case 41:
        dimasz = reader->getDouble();
        break;
    case 42:
        dimexo = reader->getDouble();
        break;
    case 43:
        dimdli = reader->getDouble();
        break;
    case 44:
        dimexe = reader->getDouble();
        break;
    case 45:
        dimrnd = reader->getDouble();
        break;
    case 46:
        dimdle = reader->getDouble();
        break;
    case 47:
        dimtp = reader->getDouble();
        break;
    case 48:
        dimtm = reader->getDouble();
        break;
    case 140:
        dimtxt = reader->getDouble();
        break;
    case 141:
        dimcen = reader->getDouble();
        break;
    case 142:
        dimtsz = reader->getDouble();
        break;
    case 143:
        dimaltf = reader->getDouble();
        break;
    case 144:
        dimlfac = reader->getDouble();
        break;
    case 145:
        dimtvp = reader->getDouble();
        break;
    case 146:
        dimtfac = reader->getDouble();
        break;
    case 147:
        dimgap = reader->getDouble();
        break;
    case 148:
        dimaltrnd = reader->getDouble();
        break;
    case 71:
        dimtol = reader->getInt32();
        break;
    case 72:
        dimlim = reader->getInt32();
        break;
    case 73:
        dimtih = reader->getInt32();
        break;
    case 74:
        dimtoh = reader->getInt32();
        break;
    case 75:
        dimse1 = reader->getInt32();
        break;
    case 76:
        dimse2 = reader->getInt32();
        break;
    case 77:
        dimtad = reader->getInt32();
        break;
    case 78:
        dimzin = reader->getInt32();
        break;
    case 79:
        dimazin = reader->getInt32();
        break;
    case 170:
        dimalt = reader->getInt32();
        break;
    case 171:
        dimaltd = reader->getInt32();
        break;
    case 172:
        dimtofl = reader->getInt32();
        break;
    case 173:
        dimsah = reader->getInt32();
        break;
    case 174:
        dimtix = reader->getInt32();
        break;
    case 175:
        dimsoxd = reader->getInt32();
        break;
    case 176:
        dimclrd = reader->getInt32();
        break;
    case 177:
        dimclre = reader->getInt32();
        break;
    case 178:
        dimclrt = reader->getInt32();
        break;
    case 179:
        dimadec = reader->getInt32();
        break;
    case 340:
        dimtxsty = reader->getString();
        break;
    default:
        DRW_TableEntry::parseCode(code, reader);
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
        desc = reader->getUtf8String();
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
        lineType = reader->getUtf8String();
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
        name = reader->getUtf8String();
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
    case 21:
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
        curr->addString(reader->getUtf8String());
        if (name =="$ACADVER")
            reader->setVersion(curr->content.s);
        curr->code = code;
        break;
    case 2:
        curr->addString(reader->getUtf8String());
        curr->code = code;
        break;
    case 3:
        curr->addString(reader->getUtf8String());
        if (name =="$DWGCODEPAGE") {
            reader->setCodePage(curr->content.s);
            curr->addString(reader->getCodePage());
        }
        curr->code = code;
        break;
    case 6:
        curr->addString(reader->getUtf8String());
        curr->code = code;
        break;
    case 7:
        curr->addString(reader->getUtf8String());
        curr->code = code;
        break;
    case 8:
        curr->addString(reader->getUtf8String());
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
        curr->addString(reader->getUtf8String());
        curr->code = code;
        break;
    default:
        break;
    }
}

void DRW_Header::write(dxfWriter *writer, DRW::Version ver){
/*RLZ: TODO complete all vars to AC1024*/
    double varDouble;
    int varInt;
    std::string varStr;
    DRW_Coord varCoord;
    writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER");
    switch (ver) {
    case DRW::AC1006: //unsupported version acad 10
    case DRW::AC1009: //acad 11 & 12
        varStr = "AC1009";
        break;
    case DRW::AC1012: //unsupported version acad 13
    case DRW::AC1014: //acad 14
        varStr = "AC1014";
        break;
    case DRW::AC1015: //acad 2000
        varStr = "AC1015";
        break;
    case DRW::AC1018: //acad 2004
        varStr = "AC1018";
        break;
/*    case DRW::AC1021: //acad 2007
        varStr = "AC1021";
        break;*/
    case DRW::AC1024: //acad 2010
        varStr = "AC1024";
        break;
    default: //acad 2007 default version
        varStr = "AC1021";
        break;
    }
    writer->writeString(1, varStr);
    writer->setVersion(&varStr);

    if (ver > DRW::AC1012) {
        writer->writeString(9, "$HANDSEED");
//RLZ        dxfHex(5, 0xFFFF);
        writer->writeString(5, "20000");
    }
    if (!getStr("$DWGCODEPAGE", &varStr)) {
        varStr = "ANSI_1252";
    }
    writer->writeString(9, "$DWGCODEPAGE");
    writer->setCodePage(&varStr);
    writer->writeString(3, writer->getCodePage() );

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
    if (getStr("$DIMSTYLE", &varStr)) {
        writer->writeString(9, "$DIMSTYLE");
        if (ver > DRW::AC1012) {
            writer->writeUtf8Caps(2, varStr);
        } else {
            writer->writeUtf8String(2, varStr);
        }
    }
    if (getDouble("$DIMTSZ", &varDouble)) {
        writer->writeString(9, "$DIMTSZ");
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
