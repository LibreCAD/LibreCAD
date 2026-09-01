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

#ifndef DXFREADER_H
#define DXFREADER_H

#include <cstdint>
#include <unordered_set>

#include "drw_textcodec.h"

class dxfReader {
public:
    enum TYPE {
        STRING,
        INT32,
        INT64,
        DOUBLE,
        BOOL,
        BINARY,
        INVALID
    };
    enum TYPE type;
public:
    dxfReader(std::istream *stream){
        filestr = stream;
        type = INVALID;
    }
    virtual ~dxfReader() = default;
    bool readRec(int *code);

    std::string getString() {return strData;}
    const std::string& getRawValue() const { return rawData; }
    // Convert a validated hexadecimal handle string representable by the
    // legacy 32-bit object model. Typed records must use this form.
    std::uint32_t getHandleString();
    bool isValidHandleString() const;
    // Raw DXF carriers may retain a syntactically valid DWG-width handle
    // lexeme (one to sixteen hexadecimal digits) without narrowing it.
    bool isValidHandleLexeme() const;
    // Admit one code-5 self handle for this read session. References use
    // other group codes and are intentionally not registered here.
    bool registerSelfHandle();
    void setAllowWideHandleLexemes(bool allow) {
        m_allowWideHandleLexemes = allow;
    }
    bool allowsWideHandleLexemes() const {
        return m_allowWideHandleLexemes;
    }
    std::string toUtf8String(std::string t) {return decoder.toUtf8(t);}
    std::string getUtf8String() {return decoder.toUtf8(strData);}
    double getDouble() {return doubleData;}
    int getInt32() {return intData;}
    std::int64_t getInt64() {return int64;}
    bool getBool() { return (intData==0) ? false : true;}
    int getVersion(){return decoder.getVersion();}
    void setVersion(const std::string &v, bool dxfFormat){decoder.setVersion(v, dxfFormat);}
    void setCodePage(const std::string &c){decoder.setCodePage(c, true);}
    std::string getCodePage(){ return decoder.getCodePage();}
    DRW::Version getSourceVersion() const { return decoder.getSourceVersion(); }
    bool hasSourceVersion() const { return decoder.hasSourceVersion(); }
    void setIgnoreComments(const bool bValue) {m_bIgnoreComments = bValue;}

protected:
    virtual bool readCode(int *code) = 0; //return true if successful (not EOF)
    virtual bool readString(std::string *text) = 0;
    virtual bool readString() = 0;
    virtual bool readBinary() = 0;
    virtual bool readInt16() = 0;
    virtual bool readInt32() = 0;
    virtual bool readInt64() = 0;
    virtual bool readDouble() = 0;
    virtual bool readBool() = 0;

protected:
    std::istream *filestr;
    std::string strData;
    // Source value spelling for ASCII records; binary readers leave this as
    // the decoded representation and callers must use typed values instead.
    std::string rawData;
    double doubleData = 0.0;
    signed int intData = 0; //32 bits integer
    std::int64_t int64 = 0; // signed 64-bit integer (DXF codes 160-169)
    bool skip = false; //set to true for ascii dxf, false for binary
private:
    DRW_TextCodec decoder;
    bool m_bIgnoreComments {false};
    bool m_allowWideHandleLexemes {false};
    std::unordered_set<std::uint64_t> m_selfHandles;
    std::uint64_t m_currentSelfHandle {0};
    bool m_currentSelfHandleRegistered {false};
};

class dxfReaderBinary : public dxfReader {
public:
    dxfReaderBinary(std::istream *stream):dxfReader(stream){skip = false; }
    virtual ~dxfReaderBinary() = default;
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readBinary();
    virtual bool readInt16();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readDouble();
    virtual bool readBool();
};

// Pre-R13 (R12/AC1009) binary DXF uses 1-byte group codes instead of the
// 2-byte little-endian codes of R13+. Only readCode differs; every value
// reader (string/double/int/...) is identical, so inherit them all.
class dxfReaderBinaryR12 : public dxfReaderBinary {
public:
    dxfReaderBinaryR12(std::istream *stream):dxfReaderBinary(stream){}
    virtual ~dxfReaderBinaryR12() = default;
    virtual bool readCode(int *code) override;
};

class dxfReaderAscii : public dxfReader {
public:
    dxfReaderAscii(std::istream *stream):dxfReader(stream){skip = true; }
    virtual ~dxfReaderAscii() = default;
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readBinary();
    virtual bool readInt16();
    virtual bool readDouble();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readBool();
};

#endif // DXFREADER_H
