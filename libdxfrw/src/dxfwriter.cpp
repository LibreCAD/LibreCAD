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

#include <stdlib.h>
#include <fstream>
#include <string>
#include "dxfwriter.h"

#ifdef DRW_DBG
#include <iostream> //for debug
#define DBG(a) std::cerr << a
#else
#define DBG(a)
#endif

//RLZ TODO change std::endl to x0D x0A (13 10)
/*bool dxfWriter::readRec(int *codeData, bool skip) {
//    std::string text;
    int code;

#ifdef DRW_DBG
    count = count+2; //DBG
#endif

    if (!readCode(&code))
        return false;
    *codeData = code;

    if (code < 10)
        readString();
    else if (code < 60)
        readDouble();
    else if (code < 80)
        readInt();
    else if (code > 89 && code < 100) //TODO this is an int 32b
        readInt32();
    else if (code == 100 || code == 102 || code == 105)
        readString();
    else if (code > 109 && code < 150) //skip not used at the v2012
        readDouble();
    else if (code > 159 && code < 170) //skip not used at the v2012
        readInt64();
    else if (code < 180)
        readInt();
    else if (code > 209 && code < 240) //skip not used at the v2012
        readDouble();
    else if (code > 269 && code < 290) //skip not used at the v2012
        readInt();
    else if (code < 300) //TODO this is a boolean indicator, int in Binary?
        readBool();
    else if (code < 370)
        readString();
    else if (code < 390)
        readInt();
    else if (code < 400)
        readString();
    else if (code < 410)
        readInt();
    else if (code < 420)
        readString();
    else if (code < 430) //TODO this is an int 32b
        readInt32();
    else if (code < 440)
        readString();
    else if (code < 450) //TODO this is an int 32b
        readInt32();
    else if (code < 460) //TODO this is long??
        readInt();
    else if (code < 470) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 481)
        readString();
    else if (code > 998 && code < 1009) //skip not used at the v2012
        readString();
    else if (code < 1060) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 1071)
        readInt();
    else if (code == 1071) //TODO this is an int 32b
        readInt32();
    else if (skip)
        //skip safely this dxf entry ( ok for ascii dxf)
        readString();
    else
        //break in binary files because the conduct is unpredictable
        return false;

    return (filestr->good());
}*/

bool dxfWriterBinary::writeString(int code, std::string text) {
    char bufcode[2];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    filestr->write(bufcode, 2);
    *filestr << text << '\0';
    return (filestr->good());
}

/*bool dxfWriterBinary::readCode(int *code) {
    unsigned short *int16p;
    char buffer[2];
    filestr->read(buffer,2);
    int16p = (unsigned short *) buffer;
//exist a 32bits int (code 90) with 2 bytes???
    if ((*code == 90) && (*int16p>2000)){
        DBG(*code); DBG(" de 16bits\n");
        filestr->seekg(-4, std::ios_base::cur);
        filestr->read(buffer,2);
        int16p = (unsigned short *) buffer;
    }
    *code = *int16p;
    DBG(*code); DBG("\n");

    return (filestr->good());
}*/

/*bool dxfWriterBinary::readString() {
    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");
    return (filestr->good());
}*/

/*bool dxfWriterBinary::readString(std::string *text) {
    std::getline(*filestr, *text, '\0');
    DBG(*text); DBG("\n");
    return (filestr->good());
}*/

bool dxfWriterBinary::writeInt16(int code, int data) {
    char bufcode[2];
    char buffer[2];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    buffer[0] =data & 0xFF;
    buffer[1] =data  >> 8;
    filestr->write(bufcode, 2);
    filestr->write(buffer, 2);
    return (filestr->good());
}

bool dxfWriterBinary::writeInt32(int code, int data) {
    char bufcode[2];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    filestr->write(bufcode, 2);

    char buffer[4];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    bufcode[1] =code  >> 16;
    bufcode[1] =code  >> 24;
/*    for (int i=0; i<4; i++) {
        buffer[i] =0;
    }
    *buffer = data;*/
    filestr->write(buffer, 4);
    return (filestr->good());
}

bool dxfWriterBinary::writeInt64(int code, unsigned long long int data) {
    char bufcode[2];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    filestr->write(bufcode, 2);

/*    char buffer[8];
    for (int i=0; i<8; i++) {
        buffer[i] =0;
    }
    *buffer = data;
    filestr->write(buffer, 8);*/
    return (filestr->good());
}

bool dxfWriterBinary::writeDouble(int code, double data) {
    char bufcode[2];
    char buffer[8];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    filestr->write(bufcode, 2);

    unsigned char *val;
    val = (unsigned char *) &data;
    for (int i=0; i<8; i++) {
        buffer[i] =val[i];
    }
    filestr->write(buffer, 8);
    return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
    char buffer[1];
    char bufcode[2];
    bufcode[0] =code & 0xFF;
    bufcode[1] =code  >> 8;
    filestr->write(bufcode, 2);
    buffer[0] = data;
    filestr->write(buffer, 1);
    return (filestr->good());
}

bool dxfWriterAscii::writeString(int code, std::string text) {
    *filestr << code << std::endl << text << std::endl ;
    /*    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");*/
    return (filestr->good());
}

/*bool dxfWriterAscii::readCode(int *code) {
    std::string text;
    std::getline(*filestr, text);
    *code = atoi(text.c_str());
    DBG(*code); DBG("\n");
    return (filestr->good());
}*/
/*bool dxfWriterAscii::readString(std::string *text) {
    std::getline(*filestr, *text);
    if (text->at(text->size()-1) == '\r')
        text->erase(text->size()-1);
    return (filestr->good());
}*/

/*bool dxfWriterAscii::readString() {
    std::getline(*filestr, strData);
    if (strData.at(strData.size()-1) == '\r')
        strData.erase(strData.size()-1);
    DBG(strData); DBG("\n");
    return (filestr->good());
}*/

bool dxfWriterAscii::writeInt16(int code, int data) {
//    *filestr << code << "\r\n" << data << "\r\n";
    *filestr << code << std::endl << data << std::endl;
    return (filestr->good());
}

bool dxfWriterAscii::writeInt32(int code, int data) {
    return writeInt16(code, data);
}

bool dxfWriterAscii::writeInt64(int code, unsigned long long int data) {
    *filestr << code << std::endl << data << std::endl;
    return (filestr->good());
}

bool dxfWriterAscii::writeDouble(int code, double data) {
    *filestr << code << std::endl << data << std::endl;
    return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::writeBool(int code, bool data) {
    *filestr << code << std::endl << data << std::endl;
    return (filestr->good());
}

