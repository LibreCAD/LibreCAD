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

#ifndef DWGBUFFER_H
#define DWGBUFFER_H

#include <fstream>
#include <sstream>
#include <memory>
#include "../drw_base.h"

class DRW_Coord;
class DRW_TextCodec;

class dwgBasicStream{
protected:
    dwgBasicStream() = default;
public:
    virtual ~dwgBasicStream() = default;
    virtual bool read(std::uint8_t* s, std::uint64_t n) = 0;
    virtual std::uint64_t size() const = 0;
    virtual std::uint64_t getPos() const = 0;
    virtual bool setPos(std::uint64_t p) = 0;
    virtual bool good() const = 0;
    virtual dwgBasicStream* clone() const = 0;
};

class dwgFileStream: public dwgBasicStream{
public:
    explicit dwgFileStream(std::ifstream *s)
        :stream{s}
    {
        stream->seekg (0, std::ios::end);
        sz = stream->tellg();
        stream->seekg(0, std::ios_base::beg);
    }
    bool read(std::uint8_t* s, std::uint64_t n) override;
    std::uint64_t size() const override{return sz;}
    std::uint64_t getPos() const override{return stream->tellg();}
    bool setPos(std::uint64_t p) override;
    bool good() const override{return stream->good();}
    dwgBasicStream* clone() const override{return new dwgFileStream(stream);}
private:
    std::ifstream *stream{nullptr};
    std::uint64_t sz{0};
};

class dwgCharStream: public dwgBasicStream{
public:
    dwgCharStream(std::uint8_t *buf, std::uint64_t s)
        :stream{buf}
        ,sz{s}
    {}
    bool read(std::uint8_t* s, std::uint64_t n) override;
    std::uint64_t size() const override {return sz;}
    std::uint64_t getPos() const override {return pos;}
    bool setPos(std::uint64_t p) override;
    bool good() const override {return isOk;}
    dwgBasicStream* clone() const override {return new dwgCharStream(stream, sz);}
private:
    std::uint8_t *stream{nullptr};
    std::uint64_t sz{0};
    std::uint64_t pos{0};
    bool isOk{true};
};

class dwgBuffer {
public:
    dwgBuffer(std::ifstream *stream, DRW_TextCodec *decoder = nullptr);
    dwgBuffer(std::uint8_t *buf, std::uint64_t size, DRW_TextCodec *decoder= nullptr);
    dwgBuffer( const dwgBuffer& org );
    dwgBuffer& operator=( const dwgBuffer& org );
    virtual ~dwgBuffer() = default;
    std::uint64_t size() const {return filestr->size();}
    bool setPosition(std::uint64_t pos);
    std::uint64_t getPosition() const;
    void resetPosition(){setPosition(0); setBitPos(0);}
    void setBitPos(std::uint8_t pos);
    std::uint8_t getBitPos() const {return bitPos;}
    bool moveBitPos(std::int32_t size);
    void setVariableTextByteLength(bool enabled) { variableTextByteLength = enabled; }

    std::uint8_t getBit();  //B
    bool getBoolBit();  //B as bool
    std::uint8_t get2Bits(); //BB
    std::uint8_t get3Bits(); //3B
    std::uint16_t getBitShort(); //BS
    std::int16_t getSBitShort(); //BS
    std::int32_t getBitLong(); //BL
    std::uint64_t getBitLongLong();  //BLL (R24)
    double getBitDouble(); //BD
    //2BD => call BD 2 times
    DRW_Coord get3BitDouble(); //3BD
    std::uint8_t getRawChar8();  //RC
    std::uint16_t getRawShort16();  //RS
    double getRawDouble(); //RD
    std::uint32_t getRawLong32();   //RL
    std::uint64_t getRawLong64();   //RLL
    DRW_Coord get2RawDouble(); //2RD
    //3RD => call RD 3 times
    std::uint32_t getUModularChar(); //UMC, unsigned for offsets in 1015
    std::int32_t getModularChar(); //MC
    std::int32_t getModularShort(); //MS
    dwgHandle getHandle(); //H
    dwgHandle getOffsetHandle(std::uint32_t href); //H converted to hard
    UTF8STRING getVariableText(DRW::Version v, bool nullTerm = true); //TV => call TU for 2007+ or T for previous versions
    UTF8STRING getCP8Text(); //T 8 bit text converted from codepage to utf8
    UTF8STRING getUCSText(bool nullTerm = true); //TU unicode 16 bit (UCS) text converted to utf8
    UTF8STRING getUCSStr(std::uint16_t ts);

    std::uint16_t getObjType(DRW::Version v);  //OT

    //X, U, SN,

    DRW_Coord getExtrusion(bool b_R2000_style); //BE
    double getDefaultDouble(double d); //DD
    double getThickness(bool b_R2000_style);//BT
    //3DD
    /// Read a CMC (Complex Material Color) field per ODA spec.
    /// Returns the indexed color value (or sentinel for ByLayer/ByBlock/RGB).
    /// If rgb24 is non-null, also populates the 24-bit RGB component for
    /// type-0xC2 (true color) entries; otherwise leaves rgb24 untouched.
    /// CMC color decoder. R15 and earlier: directly BS as ACIS. R2004+:
    /// BS index, BL rgb-with-method, RC method byte, optional name + book
    /// name from str_dat (R2007+) or dat (earlier).  See libreDWG
    /// bits.c:3704-3743 for the canonical layout.
    ///
    /// @param v        version
    /// @param rgb24    out: 24-bit RGB for type 0xC2 (true color)
    /// @param strBuf   string buffer for R2007+ name/book reads (separate
    ///                 string area).  If null, falls back to *this — wrong
    ///                 for R2007+ but matches the historical behavior.
    /// @param outName  out: color name (when method byte bit 1 set)
    /// @param outBookName out: book name (when method byte bit 2 set)
    std::uint32_t getCmColor(DRW::Version v,
                       std::int32_t* rgb24 = nullptr,
                       dwgBuffer* strBuf = nullptr,
                       UTF8STRING* outName = nullptr,
                       UTF8STRING* outBookName = nullptr); //CMC
    std::uint32_t getEnColor(DRW::Version v); //ENC
    //TC

    std::uint16_t getBERawShort16();  //RS big-endian order

    bool isGood() const {return filestr->good();}
    bool getBytes(std::uint8_t *buf, std::uint64_t size);
    int numRemainingBytes() const {return (maxSize- filestr->getPos());}

    std::uint16_t crc8(std::uint16_t dx,std::int32_t start,std::int32_t end);
    std::uint32_t crc32(std::uint32_t seed,std::int32_t start,std::int32_t end);

//    std::uint8_t getCurrByte(){return currByte;}
    DRW_TextCodec *decoder{nullptr};

    /// Side-channel: getEnColor() sets this true when the ENC field's flag
    /// byte has bit 0x40 (AcDbColor reference). Caller (DRW_Entity::parseDwg)
    /// captures it immediately, then DRW_Entity::parseDwgEntHandle consumes
    /// the corresponding offset handle at the START of the handle stream
    /// (libreDWG common_entity_data.spec:454-459 — read from hdl_dat).
    bool lastEnColorHadDbColorRef{false};

    /// Side-channel: ENC inline color/book names. Populated by getEnColor()
    /// when flags & 0x41 == 0x41 (color name) or flags & 0x42 == 0x42
    /// (book name). libreDWG common_entity_data.spec:468-475 reads these as
    /// 8-bit TV from the data stream (FIELD_TV — deliberately 8-bit even on
    /// R2007+; spec quirk verified against real files). Caller in
    /// DRW_Entity::parseDwg captures these and populates entity.colorName
    /// as "BOOK$ENTRY"; the inline ENC name takes precedence over the
    /// dbColorMap-resolved DBCOLOR name (entity-level override).
    UTF8STRING lastEnColorName;
    UTF8STRING lastEnColorBookName;

    /// Side-channel: ENC alpha-raw (DXF code 440). Populated by getEnColor()
    /// when flag 0x20 is set. libreDWG common_entity_data.spec:439 — the
    /// BL alpha_raw packs alpha_type in the high byte (0=ByLayer, 1=ByBlock,
    /// 3=explicit alpha) and alpha 0..255 in the low byte. Stored raw;
    /// DRW_Entity::parseDwg copies into transparency for the filter to
    /// interpret.  0 = "not set" (default for entities without flag 0x20).
    std::uint32_t lastEnColorAlphaRaw{0};

private:
    std::unique_ptr<dwgBasicStream> filestr;
    std::uint64_t maxSize{0};
    std::uint8_t currByte{0};
    std::uint8_t bitPos{0};
    bool variableTextByteLength{false};

    UTF8STRING get8bitStr();
    UTF8STRING get16bitStr(std::uint16_t textSize, bool nullTerm = true);
};

#endif // DWGBUFFER_H
