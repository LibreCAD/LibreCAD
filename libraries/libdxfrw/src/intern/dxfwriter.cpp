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

#include <cstdlib>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <algorithm>
#include "dxfwriter.h"

namespace {

bool isAsciiDxfCode(int code) {
    return code >= 0 && code <= 1071;
}

bool isBinaryDxfCode(int code) {
    return isAsciiDxfCode(code) && code != 999;
}

bool isSafeAsciiDxfString(const std::string& value) {
    return value.find('\r') == std::string::npos
        && value.find('\n') == std::string::npos
        && value.find('\0') == std::string::npos;
}

bool isSafeBinaryDxfString(const std::string& value) {
    return value.find('\0') == std::string::npos;
}

bool isDxfInt16Value(int value) {
    return value >= static_cast<int>(std::numeric_limits<std::int16_t>::min())
        && value <= static_cast<int>(std::numeric_limits<std::uint16_t>::max());
}

bool writeAsciiDxfInteger(std::ostream *stream, int code, std::int64_t value) {
    if (stream == nullptr || !isAsciiDxfCode(code))
        return false;
    stream->width(3);
    *stream << std::right << code << '\n';
    stream->width(5);
    *stream << value << '\n';
    return stream->good();
}

bool writeBinaryDxfCode(std::ostream *stream, int code) {
    if (stream == nullptr || !isBinaryDxfCode(code))
        return false;
    const std::uint16_t value = static_cast<std::uint16_t>(code);
    const char bytes[] = {
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU)
    };
    stream->write(bytes, sizeof(bytes));
    return stream->good();
}

bool writeBinaryDxfR12Code(std::ostream *stream, int code) {
    if (stream == nullptr || code < 0 || code > 1071)
        return false;
    if (code <= 254) {
        const char byte = static_cast<char>(code);
        stream->write(&byte, 1);
        return stream->good();
    }
    if (code < 1000)
        return false;
    const char escape = static_cast<char>(0xFF);
    stream->write(&escape, 1);
    return writeBinaryDxfCode(stream, code);
}

bool writeLittleEndian(std::ostream *stream, std::uint64_t value,
                       std::size_t byteCount) {
    if (stream == nullptr || byteCount > sizeof(value))
        return false;
    char bytes[sizeof(value)] = {};
    for (std::size_t i = 0; i < byteCount; ++i) {
        bytes[i] = static_cast<char>((value >> (i * 8U)) & 0xFFU);
    }
    stream->write(bytes, static_cast<std::streamsize>(byteCount));
    return stream->good();
}

} // namespace

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

bool dxfWriter::writeUtf8String(int code, std::string text) {
    std::string t = encoder.fromUtf8(text);
    return writeString(code, t);
}

bool dxfWriter::writeUtf8Caps(int code, std::string text) {
    std::string strname = text;
    std::transform(strname.begin(), strname.end(), strname.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::toupper(ch));
                   });
    std::string t = encoder.fromUtf8(strname);
    return writeString(code, t);
}

bool dxfWriterBinary::writeString(int code, std::string text) {
    if ((code >= 310 && code <= 319) || code == 1004) {
        if ((text.size() % 2) != 0 || text.size() / 2 > 127)
            return recordWriteResult(false);
        auto hexValue = [](char ch) -> int {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= '0' && c <= '9')
                return c - '0';
            c = static_cast<unsigned char>(std::toupper(c));
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        };
        for (char ch : text) {
            if (hexValue(ch) < 0)
                return recordWriteResult(false);
        }

        if (!writeCode(code))
            return recordWriteResult(false);
        const unsigned char chunkLen = static_cast<unsigned char>(text.size() / 2);
        filestr->write(reinterpret_cast<const char*>(&chunkLen), 1);
        for (std::size_t i = 0; i < text.size(); i += 2) {
            int hi = hexValue(text[i]);
            int lo = hexValue(text[i + 1]);
            unsigned char value = static_cast<unsigned char>((hi << 4) | lo);
            filestr->write(reinterpret_cast<const char*>(&value), 1);
        }
        return recordWriteResult(filestr->good());
    }
    if (!isSafeBinaryDxfString(text) || !writeCode(code))
        return recordWriteResult(false);
    *filestr << text << '\0';
    return recordWriteResult(filestr->good());
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
    if (!isDxfInt16Value(data))
        return recordWriteResult(false);
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint16_t>(data), sizeof(std::uint16_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt32(int code, int data) {
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint32_t>(data), sizeof(std::uint32_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt64(int code, std::int64_t data) {
    const std::uint64_t bits = static_cast<std::uint64_t>(data);
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeDouble(int code, double data) {
    if (!std::isfinite(data))
        return recordWriteResult(false);
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(data), "DXF doubles require 64 bits");
    std::memcpy(&bits, &data, sizeof(bits));
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, data ? 1U : 0U, 1);
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeCode(int code) {
    return writeBinaryDxfCode(filestr, code);
}

bool dxfWriterBinaryR12::writeCode(int code) {
    return writeBinaryDxfR12Code(filestr, code);
}

dxfWriterAscii::dxfWriterAscii(std::ostream *stream):dxfWriter(stream){
    filestr->precision(16);
}

bool dxfWriterAscii::writeString(int code, std::string text) {
    if (!isAsciiDxfCode(code) || !isSafeAsciiDxfString(text))
        return recordWriteResult(false);
//    *filestr << code << std::endl << text << std::endl ;
    filestr->width(3);
    *filestr << std::right << code << '\n';
    filestr->width(0);
    *filestr << std::left << text << '\n';
    /*    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");*/
    return recordWriteResult(filestr->good());
}

bool dxfWriterAscii::writeInt16(int code, int data) {
    if (!isDxfInt16Value(data))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt32(int code, int data) {
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt64(int code, std::int64_t data) {
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeDouble(int code, double data) {
    if (!isAsciiDxfCode(code) || !std::isfinite(data))
        return recordWriteResult(false);
//    std::streamsize prec = filestr->precision();
//    filestr->precision(12);
//    *filestr << code << std::endl << data << std::endl;
    filestr->width(3);
    *filestr << std::right << code << '\n';
    *filestr << data << '\n';
//    filestr->precision(prec);
    return recordWriteResult(filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::writeBool(int code, bool data) {
    if (!isAsciiDxfCode(code))
        return recordWriteResult(false);
    *filestr << code << '\n' << data << '\n';
    return recordWriteResult(filestr->good());
}
