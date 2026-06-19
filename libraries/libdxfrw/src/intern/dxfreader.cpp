/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sstream>
#include <locale>
#include "dxfreader.h"
#include "drw_textcodec.h"
#include "drw_dbg.h"

bool dxfReader::readRec(int *codeData) {
//    std::string text;
    int code;

    if (!readCode(&code))
        return false;
    *codeData = code;

    bool valueOk = true;
    if (code < 10)
        valueOk = readString();
    else if (code < 60)
        valueOk = readDouble();
    else if (code < 80)
        valueOk = readInt16();
    else if (code > 89 && code < 100) //TODO this is an int 32b
        valueOk = readInt32();
    else if (code == 100 || code == 101 || code == 102 || code == 105)
        valueOk = readString(); //101 = "Embedded Object" marker (string, not int16)
    else if (code > 109 && code < 150) //skip not used at the v2012
        valueOk = readDouble();
    else if (code > 159 && code < 170) //skip not used at the v2012
        valueOk = readInt64();
    else if (code < 180)
        valueOk = readInt16();
    else if (code > 209 && code < 240) //skip not used at the v2012
        valueOk = readDouble();
    else if (code > 269 && code < 290) //skip not used at the v2012
        valueOk = readInt16();
    else if (code < 300) //TODO this is a boolean indicator, int in Binary?
        valueOk = readBool();
    else if (code < 310)
        valueOk = readString();
    else if (code < 320)
        valueOk = readBinary();
    else if (code < 370)
        valueOk = readString();
    else if (code < 390)
        valueOk = readInt16();
    else if (code < 400)
        valueOk = readString();
    else if (code < 410)
        valueOk = readInt16();
    else if (code < 420)
        valueOk = readString();
    else if (code < 430) //TODO this is an int 32b
        valueOk = readInt32();
    else if (code < 440)
        valueOk = readString();
    else if (code < 450) //TODO this is an int 32b
        valueOk = readInt32();
    else if (code < 460) //TODO this is long??
        valueOk = readInt32();
    else if (code < 470) //TODO this is a floating point double precision??
        valueOk = readDouble();
    else if (code <= 481)
        valueOk = readString();
    else if( 999 == code && m_bIgnoreComments) {
        readString();
        return readRec( codeData);
    }
    else if (code == 1004)
        valueOk = readBinary();
    else if (code > 998 && code < 1009) //skip not used at the v2012
        valueOk = readString();
    else if (code < 1060) //TODO this is a floating point double precision??
        valueOk = readDouble();
    else if (code < 1071)
        valueOk = readInt16();
    else if (code == 1071) //TODO this is an int 32b
        valueOk = readInt32();
    else if (skip)
        //skip safely this dxf entry ( ok for ascii dxf)
        valueOk = readString();
    else
        //break in binary files because the conduct is unpredictable
        return false;

    return valueOk && (filestr->good());
}
int dxfReader::getHandleString(){
    int res;
#if defined(__APPLE__)
    int Succeeded = sscanf ( strData.c_str(), "%x", &res );
    if ( !Succeeded || Succeeded == EOF )
        res = 0;
#else
    std::istringstream Convert(strData);
    if ( !(Convert >> std::hex >>res) )
        res = 0;
#endif
    return res;
}

bool dxfReaderBinary::readCode(int *code) {
    unsigned char buffer[2] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 2);
    if (!filestr->good())
        return false;
    *code = static_cast<int>(buffer[0])
        | (static_cast<int>(buffer[1]) << 8);
    DRW_DBG(*code); DRW_DBG("\n");

    return true;
}

bool dxfReaderBinary::readString() {
    type = STRING;
    std::getline(*filestr, strData, '\0');
    DRW_DBG(strData); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readString(std::string *text) {
    type = STRING;
    std::getline(*filestr, *text, '\0');
    DRW_DBG(*text); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readBinary() {
    type = BINARY;
    unsigned char chunklen {0};

    filestr->read( reinterpret_cast<char *>(&chunklen), 1);
    if (!filestr->good())
        return false;
    // Capture the chunk bytes as an upper-hex string — the canonical ASCII form
    // of binary codes (310-319/1004) — so getString() returns the real data.
    // Previously this seeked past the chunk and never wrote strData, so any
    // binary group on a binary read (typed entity OR raw-net object) re-emitted
    // a STALE strData (the previous record's value). Same net stream advance.
    strData.clear();
    strData.reserve(static_cast<std::size_t>(chunklen) * 2);
    static const char hex[] = "0123456789ABCDEF";
    for (unsigned i = 0; i < chunklen; ++i) {
        char b = 0;
        filestr->read(&b, 1);
        const unsigned char u = static_cast<unsigned char>(b);
        strData.push_back(hex[(u >> 4) & 0xF]);
        strData.push_back(hex[u & 0xF]);
    }
    DRW_DBG( chunklen); DRW_DBG( " byte(s) binary data read\n");

    return (filestr->good());
}

bool dxfReaderBinary::readInt16() {
    type = INT32;
    unsigned char buffer[2] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 2);
    if (!filestr->good())
        return false;
    intData = static_cast<std::int16_t>((static_cast<unsigned char>(buffer[1]) << 8)
                                  | static_cast<unsigned char>(buffer[0]));
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt32() {
    type = INT32;
    unsigned char buffer[4] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 4);
    if (!filestr->good())
        return false;
    std::uint32_t value = static_cast<std::uint32_t>(buffer[0])
        | (static_cast<std::uint32_t>(buffer[1]) << 8)
        | (static_cast<std::uint32_t>(buffer[2]) << 16)
        | (static_cast<std::uint32_t>(buffer[3]) << 24);
    intData = static_cast<std::int32_t>(value);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt64() {
    type = INT64;
    unsigned char buffer[8] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 8);
    if (!filestr->good())
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    int64 = value;
    DRW_DBG(int64); DRW_DBG(" int64\n");
    return true;
}

bool dxfReaderBinary::readDouble() {
    type = DOUBLE;
    unsigned char buffer[8] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 8);
    if (!filestr->good())
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    std::memcpy(&doubleData, &value, sizeof(doubleData));
    DRW_DBG(doubleData); DRW_DBG("\n");
    return true;
}

//saved as int or add a bool member??
bool dxfReaderBinary::readBool() {
    type = BOOL;
    unsigned char buffer[1] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 1);
    if (!filestr->good())
        return false;
    intData = static_cast<int>(buffer[0]);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderAscii::readCode(int *code) {
    std::string text;
    std::getline(*filestr, text);
    if (!filestr->good())
        return false;
    if (!text.empty() && text.at(text.size()-1) == '\r')
        text.erase(text.size()-1);
    char *end = nullptr;
    errno = 0;
    long parsed = std::strtol(text.c_str(), &end, 10);
    while (end != nullptr && *end != '\0'
           && std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (end == text.c_str() || end == nullptr || *end != '\0'
        || errno == ERANGE || parsed < 0 || parsed > 1071) {
        return false;
    }
    *code = static_cast<int>(parsed);
    DRW_DBG(*code); DRW_DBG("\n");
    return true;
}
bool dxfReaderAscii::readString(std::string *text) {
    type = STRING;
    std::getline(*filestr, *text);
    if (!text->empty() && text->at(text->size()-1) == '\r')
        text->erase(text->size()-1);
    return (filestr->good());
}

bool dxfReaderAscii::readString() {
    type = STRING;
    std::getline(*filestr, strData);
    if (!strData.empty() && strData.at(strData.size()-1) == '\r')
        strData.erase(strData.size()-1);
    DRW_DBG(strData); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderAscii::readBinary() {
    return readString();
}

bool dxfReaderAscii::readInt16() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        long parsed = std::strtol(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE) {
            return false;
        }
        type = INT32;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    } else
        return false;
}

bool dxfReaderAscii::readInt32() {
    type = INT32;
    return readInt16();
}

bool dxfReaderAscii::readInt64() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE) {
            return false;
        }
        type = INT64;
        int64 = parsed;
        DRW_DBG(int64); DRW_DBG(" int64\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readDouble() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        double parsed = std::strtod(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE) {
            DRW_DBG("dxfReaderAscii::readDouble(): reading double error: ");
            DRW_DBG(text);
            DRW_DBG('\n');
            return false;
        }
        type = DOUBLE;
        doubleData = parsed;
        DRW_DBG(doubleData); DRW_DBG('\n');
        return true;
    } else
        return false;
}

//saved as int or add a bool member??
bool dxfReaderAscii::readBool() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        long parsed = std::strtol(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE) {
            return false;
        }
        type = BOOL;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    } else
        return false;
}
