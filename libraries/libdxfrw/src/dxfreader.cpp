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
#include <sstream>
#include "dxfreader.h"
#include "drw_textcodec.h"

#ifdef DRW_DBG
#include <iostream> //for debug
#define DBG(a) std::cerr << a
#else
#define DBG(a)
#endif

bool dxfReader::readRec(int *codeData, bool skip) {
//    std::string text;
    int code;

#ifdef DRW_DBG
    count = count+2; //DBG
/*    if (count > 10250)
        DBG("line 10256");*/
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
}

bool dxfReaderBinary::readCode(int *code) {
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
}

bool dxfReaderBinary::readString() {
    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readString(std::string *text) {
    std::getline(*filestr, *text, '\0');
    DBG(*text); DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readInt() {
    char buffer[2];
    filestr->read(buffer,2);
    intData = (int)((buffer[1] << 8) | buffer[0]);
    DBG(intData); DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readInt32() {
    unsigned int *int32p;
    char buffer[4];
    filestr->read(buffer,4);
    int32p = (unsigned int *) buffer;
    intData = *int32p;
    DBG(intData); DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readInt64() {
    unsigned long long int *int64p; //64 bits integer pointer
    char buffer[8];
    filestr->read(buffer,8);
    int64p = (unsigned long long int *) buffer;
    int64 = *int64p;
    DBG(int64); DBG(" int64\n");
    return (filestr->good());
}

bool dxfReaderBinary::readDouble() {
    double *result;
    char buffer[8];
    filestr->read(buffer,8);
    result = (double *) buffer;
    doubleData = *result;
    DBG(doubleData); DBG("\n");
    return (filestr->good());
}

//saved as int or add a bool member??
bool dxfReaderBinary::readBool() {
    char buffer[1];
    filestr->read(buffer,1);
    intData = (int)(buffer[0]);
    DBG(intData); DBG("\n");
    return (filestr->good());
}

bool dxfReaderAscii::readCode(int *code) {
    std::string text;
    std::getline(*filestr, text);
    *code = atoi(text.c_str());
    DBG(*code); DBG("\n");
    return (filestr->good());
}
bool dxfReaderAscii::readString(std::string *text) {
    std::getline(*filestr, *text);
    if(text->size()>0) {
        if (text->at(text->size()-1) == '\r')
            text->erase(text->size()-1);
    }
    return (filestr->good());
}

bool dxfReaderAscii::readString() {
    std::getline(*filestr, strData);
    if (!strData.empty() && strData.at(strData.size()-1) == '\r')
        strData.erase(strData.size()-1);
    DBG(strData); DBG("\n");
    return (filestr->good());
}

bool dxfReaderAscii::readInt() {
    std::string text;
    if (readString(&text)){
        intData = atoi(text.c_str());
        DBG(intData); DBG("\n");
        return true;
    } else
        return false;
}

bool dxfReaderAscii::readInt32() {
    return readInt();
}

bool dxfReaderAscii::readInt64() {
    return readInt();
}

bool dxfReaderAscii::readDouble() {
    std::string text;
    if (readString(&text)){
        std::istringstream sd(text);
        sd >> doubleData;
        DBG(doubleData); DBG("\n");
        return true;
    } else
        return false;
}

//saved as int or add a bool member??
bool dxfReaderAscii::readBool() {
    std::string text;
    if (readString(&text)){
        intData = atoi(text.c_str());
        DBG(intData); DBG("\n");
        return true;
    } else
        return false;
}

