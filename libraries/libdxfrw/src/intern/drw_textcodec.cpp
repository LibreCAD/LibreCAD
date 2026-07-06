#include "drw_textcodec.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include "../drw_base.h"
#include "drw_cptables.h"
#include "drw_cptable932.h"
#include "drw_cptable936.h"
#include "drw_cptable949.h"
#include "drw_cptable950.h"

DRW_TextCodec::DRW_TextCodec()
    : version{DRW::AC1021}
    , conv( new DRW_Converter(nullptr, 0) )
{
}

DRW_TextCodec::~DRW_TextCodec() = default;

void DRW_TextCodec::setVersion(DRW::Version v, bool dxfFormat){
    switch (v)
    {
        case DRW::UNKNOWNV:
        case DRW::MC00:
        case DRW::AC12:
        case DRW::AC14:
        case DRW::AC150:
        case DRW::AC210:
        case DRW::AC1002:
        case DRW::AC1003:
        case DRW::AC1004:
            // unhandled?
            break;

        case DRW::AC1006:
        case DRW::AC1009:
        {
            version = DRW::AC1009;
            cp = "ANSI_1252";
            setCodePage( cp, dxfFormat);
            break;
        }

        case DRW::AC1012:
        case DRW::AC1014:
        case DRW::AC1015:
        case DRW::AC1018:
        {
            version = DRW::AC1015;
//            if (cp.empty()) { //codepage not set, initialize ??
                cp = "ANSI_1252";
                setCodePage( cp, dxfFormat);
//          }
            break;
        }

        case DRW::AC1021:
        case DRW::AC1024:
        case DRW::AC1027:
        case DRW::AC1032:
        {
            version = DRW::AC1021;
            if (dxfFormat) {
                cp = "UTF-8";
                setCodePage(cp, dxfFormat);
            } else {
                if (cp.empty() || cp == "ANSI_1252") {
                    cp = "UTF-16";
                    setCodePage(cp, dxfFormat);
                }
            }
            break;
        }
    }
}

void DRW_TextCodec::setVersion(const std::string &v, bool dxfFormat){
    version = DRW::UNKNOWNV;
    for (const auto& [verName, verId] : DRW::dwgVersionStrings)
    {
        if ( v == verName ) {
            version = verId;
            setVersion( verId, dxfFormat);
            break;
        }
    }
}

void DRW_TextCodec::setCodePage(const std::string &c, bool dxfFormat){
    cp = correctCodePage(c);
    conv.reset();
    if (cp == "ANSI_874")
        conv.reset( new DRW_ConvTable(DRW_Table874, CPLENGTHCOMMON) );
    else if (cp == "ANSI_932")
        conv.reset( new DRW_Conv932Table() );
    else if (cp == "ANSI_936")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table936, DRW_LeadTable936,
                                     DRW_DoubleTable936, CPLENGTH936) );
    else if (cp == "ANSI_949")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table949, DRW_LeadTable949,
                                     DRW_DoubleTable949, CPLENGTH949) );
    else if (cp == "ANSI_950")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table950, DRW_LeadTable950,
                                     DRW_DoubleTable950, CPLENGTH950) );
    else if (cp == "ANSI_1250")
        conv.reset( new DRW_ConvTable(DRW_Table1250, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1251")
        conv.reset( new DRW_ConvTable(DRW_Table1251, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1253")
        conv.reset( new DRW_ConvTable(DRW_Table1253, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1254")
        conv.reset( new DRW_ConvTable(DRW_Table1254, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1255")
        conv.reset( new DRW_ConvTable(DRW_Table1255, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1256")
        conv.reset( new DRW_ConvTable(DRW_Table1256, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1257")
        conv.reset( new DRW_ConvTable(DRW_Table1257, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1258")
        conv.reset( new DRW_ConvTable(DRW_Table1258, CPLENGTHCOMMON) );
    else if (cp == "UTF-8") {
        cp = "ANSI_1252";
        conv.reset( new DRW_Converter(nullptr, 0) );
    } else if (cp == "UTF-16") {
        conv.reset( new DRW_ConvUTF16() );
    } else if (dxfFormat) {
        conv.reset( new DRW_Converter(nullptr, 0) );
    } else {
        conv.reset( new DRW_ConvTable(DRW_Table1252, CPLENGTHCOMMON) );
    }
}

std::string DRW_TextCodec::toUtf8(std::string_view s) {
    return conv->toUtf8(s);
}

std::string DRW_TextCodec::fromUtf8(std::string_view s) {
    return conv->fromUtf8(s);
}

std::string DRW_Converter::toUtf8(std::string_view s) {
    std::string result;
    int j = 0;
    unsigned int i= 0;
    for (i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c < 0x80) { //ascii check for \U+???? or \M+cXXXX
            if (c == '\\' && i+6 < s.length() && s.at(i+1) == 'U' && s.at(i+2) == '+') {
                result += s.substr(j,i-j);
                result += encodeText(std::string{s.substr(i,7)});
                i +=6;
                j = i+1;
            } else if (c == '\\' && i+7 < s.length() && s.at(i+1) == 'M' && s.at(i+2) == '+') {
                std::string mif = encodeMifText(std::string{s.substr(i, 8)});
                if (!mif.empty()) {
                    result += s.substr(j, i-j);
                    result += mif;
                    i += 7;
                    j = i+1;
                }
            }
        } else if (c < 0xE0 ) {//2 bits
            i++;
        } else if (c < 0xF0 ) {//3 bits
            i +=2;
        } else if (c < 0xF8 ) {//4 bits
            i +=3;
        }
    }
    result += s.substr(j);

    return result;
}

std::string DRW_ConvTable::fromUtf8(std::string_view s) {
    std::string result;
    bool notFound;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            notFound = true;
            for (int k=0; k<cpLength; k++){
                if(table[k] == code) {
                    result += CPOFFSET + k; //translate from table
                    notFound = false;
                    break;
                }
            }
            if (notFound)
                result += decodeText(code);
        }
    }
    result += s.substr(j);

    return result;
}

std::string DRW_ConvTable::toUtf8(std::string_view s) {
    std::string res;
    for ( auto it=s.begin() ; it < s.end(); ++it ) {
        unsigned char c = *it;
        // \U+ and \M+ escapes are 7-bit ASCII anchors; valid even when an
        // adjacent table byte is high. Test the backslash branch regardless
        // of `c < 0x80` so an escape immediately following a table-mapped
        // byte is still detected.
        if (c == '\\') {
            if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+') {
                res += encodeText(std::string(it, it+7));
                it += 6;
                continue;
            }
            if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                std::string mif = encodeMifText(std::string(it, it+8));
                if (!mif.empty()) {
                    res += mif;
                    it += 7;
                    continue;
                }
            }
            res += c; // literal backslash
        } else if (c < 0x80) {
            res += c; // ascii char write
        } else {
            res += encodeNum(table[c-0x80]); // translate from table
        }
    } //end for

    return res;
}

std::string DRW_Converter::encodeText(const std::string &stmp){
    int code;
#if defined(__APPLE__)
    int Succeeded = sscanf (&( stmp.substr(3,4)[0]), "%x", &code );
    if ( !Succeeded || Succeeded == EOF )
        code = 0;
#else
    std::istringstream sd(stmp.substr(3,4));
    sd >> std::hex >> code;
#endif
    return encodeNum(code);
}

std::string DRW_Converter::encodeMifText(const std::string &tok){
    // tok layout: "\M+cXXXX" — selector at index 3, 4 hex digits at [4..8).
    if (tok.size() < 8 || tok[0] != '\\' || tok[1] != 'M' || tok[2] != '+')
        return std::string{};
    const char* cpName = nullptr;
    switch (tok[3]) {
        case '1': cpName = "ANSI_932"; break; // Shift-JIS
        case '2': cpName = "ANSI_950"; break; // Big5
        case '3': cpName = "ANSI_949"; break; // EUC-KR
        case '4': cpName = "ANSI_1252"; break; // Johab — fallback (no codec)
        case '5': cpName = "ANSI_936"; break; // GB18030/GBK
        default:  return std::string{};
    }
    int code = 0;
#if defined(__APPLE__)
    int succeeded = sscanf(&(tok.substr(4,4)[0]), "%x", &code);
    if (!succeeded || succeeded == EOF)
        return std::string{};
#else
    std::istringstream sd(tok.substr(4, 4));
    sd >> std::hex >> code;
    if (!sd) return std::string{};
#endif
    if (code <= 0) return std::string{};
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    codec.setCodePage(cpName, /*dxfFormat=*/false);
    std::string raw;
    raw.push_back(static_cast<char>((code >> 8) & 0xFF));
    raw.push_back(static_cast<char>(code & 0xFF));
    return codec.toUtf8(raw);
}

std::string DRW_Converter::decodeText(int c){
    std::string res = "\\U+";
    std::string num;
#if defined(__APPLE__)
    std::string str(16, '\0');
    snprintf (&(str[0]), 16, "%04X", c );
    num = str;
#else
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << c;
    ss >> num;
#endif
    res += num;
    return res;
}

std::string DRW_Converter::encodeNum(int c){
    unsigned char ret[5];
    if (c < 128) { // 0-7F US-ASCII 7 bits
        ret[0] = c;
        ret[1] = 0;
    } else if (c < 0x800) { //80-07FF 2 bytes
        ret[0] = 0xC0 | (c >> 6);
        ret[1] = 0x80 | (c & 0x3f);
        ret[2] = 0;
    } else if (c< 0x10000) { //800-FFFF 3 bytes
        ret[0] = 0xe0 | (c >> 12);
        ret[1] = 0x80 | ((c >> 6) & 0x3f);
        ret[2] = 0x80 | (c & 0x3f);
        ret[3] = 0;
    } else { //10000-10FFFF 4 bytes
        ret[0] = 0xf0 | (c >> 18);
        ret[1] = 0x80 | ((c >> 12) & 0x3f);
        ret[2] = 0x80 | ((c >> 6) & 0x3f);
        ret[3] = 0x80 | (c & 0x3f);
        ret[4] = 0;
    }
    return std::string(reinterpret_cast<char*>(ret));
}

/** 's' is a string with at least 4 bytes length
** returned 'b' is byte length of encoded char: 2,3 or 4
**/
int DRW_Converter::decodeNum(const std::string &s, int *b){
    int code= 0;
    unsigned char c = s.at(0);
    if ( (c& 0xE0)  == 0xC0) { //2 bytes
        code = ( c&0x1F)<<6;
        code = (s.at(1) &0x3F) | code;
        *b = 2;
    } else if ( (c& 0xF0)  == 0xE0) { //3 bytes
        code = ( c&0x0F)<<12;
        code = ((s.at(1) &0x3F)<<6) | code;
        code = (s.at(2) &0x3F) | code;
        *b = 3;
    } else if ( (c& 0xF8)  == 0xF0) { //4 bytes
        code = ( c&0x07)<<18;
        code = ((s.at(1) &0x3F)<<12) | code;
        code = ((s.at(2) &0x3F)<<6) | code;
        code = (s.at(3) &0x3F) | code;
        *b = 4;
    }

    return code;
}


std::string DRW_ConvDBCSTable::fromUtf8(std::string_view s) {
    std::string result;
    bool notFound;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            notFound = true;
                for (int k=0; k<cpLength; k++){
                    if(doubleTable[k][1] == code) {
                        int data = doubleTable[k][0];
                        char d[3];
                        d[0] = data >> 8;
                        d[1] = data & 0xFF;
                        d[2]= '\0';
                        result += d; //translate from table
                        notFound = false;
                        break;
                    }
                }
            if (notFound)
                result += decodeText(code);
        } //direct conversion
    }
    result += s.substr(j);

    return result;
}

std::string DRW_ConvDBCSTable::toUtf8(std::string_view s) {
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        if (c < 0x80) {
            notFound = false;
            // check for \U+ or \M+ encoded text (both are ASCII anchors;
            // backslash 0x5C is never a DBCS lead byte)
            if (c == '\\') {
                if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+')  {
                    res += encodeText(std::string(it, it+7));
                    it +=6;
                } else if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                    std::string mif = encodeMifText(std::string(it, it+8));
                    if (!mif.empty()) {
                        res += mif;
                        it += 7;
                    } else {
                        res += c;
                    }
                } else {
                    res +=c; //no \U+/\M+ encoded text write
                }
            } else
                res +=c; //c!='\' ascii char write
        } else if(c == 0x80 ){//1 byte table
            notFound = false;
            res += encodeNum(0x20AC);//euro sign
        } else {//2 bytes
            ++it;
            int code = (c << 8) | static_cast<unsigned char >(*it);
            int sta = leadTable[c-0x81];
            int end = leadTable[c-0x80];
            for (int k=sta; k<end; k++){
                if(doubleTable[k][0] == code) {
                    res += encodeNum(doubleTable[k][1]); //translate from table
                    notFound = false;
                    break;
                }
            }
        }
        //not found
        if (notFound) res += encodeNum(NOTFOUND936);
    } //end for

    return res;
}

DRW_Conv932Table::DRW_Conv932Table()
    :DRW_Converter(DRW_Table932, CPLENGTH932) {
}

std::string DRW_Conv932Table::fromUtf8(std::string_view s) {
    std::string result;
    bool notFound;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            notFound = true;
            // 1 byte table
            if (code > 0xff60 && code < 0xFFA0) {
                result += code - CPOFFSET932; //translate from table
                notFound = false;
            }
            if (notFound && ( code<0xF8 || (code>0x390 && code<0x542) ||
                    (code>0x200F && code<0x9FA1) || code>0xF928 )) {
                for (int k=0; k<cpLength; k++){
                    if(DRW_DoubleTable932[k][1] == code) {
                        int data = DRW_DoubleTable932[k][0];
                        char d[3];
                        d[0] = data >> 8;
                        d[1] = data & 0xFF;
                        d[2]= '\0';
                        result += d; //translate from table
                        notFound = false;
                        break;
                    }
                }
            }
            if (notFound)
                result += decodeText(code);
        } //direct conversion
    }
    result += s.substr(j);

    return result;
}

std::string DRW_Conv932Table::toUtf8(std::string_view s) {
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        if (c < 0x80) {
            notFound = false;
            // check for \U+ or \M+ encoded text (SJIS lead bytes are
            // 0x81-0x9F and 0xE0-0xFC; backslash 0x5C is always ASCII)
            if (c == '\\') {
                if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+')  {
                    res += encodeText(std::string(it, it+7));
                    it +=6;
                } else if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                    std::string mif = encodeMifText(std::string(it, it+8));
                    if (!mif.empty()) {
                        res += mif;
                        it += 7;
                    } else {
                        res += c;
                    }
                } else {
                    res +=c; //no \U+/\M+ encoded text write
                }
            } else
                res +=c; //c!='\' ascii char write
        } else if(c > 0xA0 && c < 0xE0 ){//1 byte table
            notFound = false;
            res += encodeNum(c + CPOFFSET932); //translate from table
        } else {//2 bytes
            ++it;
            int code = (c << 8) | static_cast<unsigned char>(*it);
            int sta=0;
            int end=0;
            if (c > 0x80 && c < 0xA0) {
                sta = DRW_LeadTable932[c-0x81];
                end = DRW_LeadTable932[c-0x80];
            } else if (c > 0xDF && c < 0xFD){
                sta = DRW_LeadTable932[c-0xC1];
                end = DRW_LeadTable932[c-0xC0];
            }
            if (end > 0) {
                for (int k=sta; k<end; k++){
                    if(DRW_DoubleTable932[k][0] == code) {
                        res += encodeNum(DRW_DoubleTable932[k][1]); //translate from table
                        notFound = false;
                        break;
                    }
                }
            }
        }
        //not found
        if (notFound) res += encodeNum(NOTFOUND932);
    } //end for

    return res;
}

std::string DRW_ConvUTF16::fromUtf8(std::string_view s){
    DRW_UNUSED(s);
    //RLZ: to be written (only needed for write dwg 2007+)
    return std::string();
}

std::string DRW_ConvUTF16::toUtf8(std::string_view s){
    // Decode UTF-16LE bytes to UTF-8. Combines surrogate pairs
    // 0xD800..0xDBFF + 0xDC00..0xDFFF into astral codepoints; an unpaired
    // half (high without matching low, or stray low) emits U+FFFD.
    std::string res;
    const size_t n = s.size() & ~static_cast<size_t>(1);
    for (size_t i = 0; i < n; i += 2) {
        std::uint32_t ch =
            static_cast<std::uint8_t>(s[i]) |
            (static_cast<std::uint8_t>(s[i + 1]) << 8);
        if (ch >= 0xD800 && ch <= 0xDBFF) { // high surrogate
            if (i + 3 < n) {
                std::uint16_t lo =
                    static_cast<std::uint8_t>(s[i + 2]) |
                    (static_cast<std::uint8_t>(s[i + 3]) << 8);
                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                    ch = 0x10000u + ((ch - 0xD800u) << 10) + (lo - 0xDC00u);
                    i += 2; // consume the low surrogate
                } else {
                    ch = 0xFFFD; // unpaired high
                }
            } else {
                ch = 0xFFFD; // truncated high at end
            }
        } else if (ch >= 0xDC00 && ch <= 0xDFFF) { // stray low surrogate
            ch = 0xFFFD;
        }
        res += encodeNum(static_cast<int>(ch));
    }
    return res;
}

std::string DRW_TextCodec::correctCodePage(const std::string& s) {
    //stringstream cause crash in OS/X, bug#3597944
    std::string cp=s;
    transform(cp.begin(), cp.end(), cp.begin(), toupper);
    //Latin/Thai
    if (cp=="ANSI_874" || cp=="CP874" || cp=="ISO8859-11" || cp=="TIS-620") {
        return "ANSI_874";
        //Central Europe and Eastern Europe
    } else if (cp=="ANSI_1250" || cp=="CP1250" || cp=="ISO8859-2") {
        return "ANSI_1250";
        //Cyrillic script
    } else if (cp=="ANSI_1251" || cp=="CP1251" || cp=="ISO8859-5" || cp=="KOI8-R" ||
               cp=="KOI8-U" || cp=="IBM 866") {
        return "ANSI_1251";
        //Western Europe
    } else if (cp=="ANSI_1252" || cp=="CP1252" || cp=="LATIN1" || cp=="ISO-8859-1" ||
               cp=="CP819" || cp=="CSISO" || cp=="IBM819" || cp=="ISO_8859-1" || cp=="APPLE ROMAN" ||
               cp=="ISO8859-1" || cp=="ISO8859-15" || cp=="ISO-IR-100" || cp=="L1" || cp=="IBM 850") {
        return "ANSI_1252";
        //Greek
    } else if (cp=="ANSI_1253" || cp=="CP1253" || cp=="ISO8859-7") {
        return "ANSI_1253";
        //Turkish
    } else if (cp=="ANSI_1254" || cp=="CP1254" || cp=="ISO8859-9" || cp=="ISO8859-3") {
        return "ANSI_1254";
        //Hebrew
    } else if (cp=="ANSI_1255" || cp=="CP1255" || cp=="ISO8859-8") {
        return "ANSI_1255";
        //Arabic
    } else if (cp=="ANSI_1256" || cp=="CP1256" || cp=="ISO8859-6") {
        return "ANSI_1256";
        //Baltic
    } else if (cp=="ANSI_1257" || cp=="CP1257" || cp=="ISO8859-4" || cp=="ISO8859-10" || cp=="ISO8859-13") {
        return "ANSI_1257";
        //Vietnamese
    } else if (cp=="ANSI_1258" || cp=="CP1258") {
        return "ANSI_1258";

        //Japanese
    } else if (cp=="ANSI_932" || cp=="SHIFT-JIS" || cp=="SHIFT_JIS" || cp=="CSSHIFTJIS" ||
               cp=="CSWINDOWS31J" || cp=="MS_KANJI" || cp=="X-MS-CP932" || cp=="X-SJIS" ||
               cp=="EUCJP" || cp=="EUC-JP" || cp=="CSEUCPKDFMTJAPANESE" || cp=="X-EUC" ||
               cp=="X-EUC-JP" || cp=="JIS7") {
        return "ANSI_932";
        //Chinese PRC GBK (XGB) simplified
    } else if (cp=="ANSI_936" || cp=="GBK" || cp=="GB2312" || cp=="CHINESE" || cp=="CN-GB" ||
               cp=="CSGB2312" || cp=="CSGB231280" || cp=="CSISO58BG231280" ||
               cp=="GB_2312-80" || cp=="GB231280" || cp=="GB2312-80" ||
               cp=="ISO-IR-58" || cp=="GB18030") {
        return "ANSI_936";
        //Korean
    } else if (cp=="ANSI_949" || cp=="EUCKR") {
        return "ANSI_949";
        //Chinese Big5 (Taiwan, Hong Kong SAR)
    } else if (cp=="ANSI_950" || cp=="BIG5" || cp=="CN-BIG5" || cp=="CSBIG5" ||
               cp=="X-X-BIG5" || cp=="BIG5-HKSCS") {
        return "ANSI_950";

//celtic
/*    } else if (cp=="ISO8859-14") {
       return "ISO8859-14";
    } else if (cp=="TSCII") {
        return "TSCII"; //tamil
    }*/

    } else if (cp=="UTF-8" || cp=="UTF8" || cp=="UTF8-BIT") {
        return "UTF-8";
    } else if (cp=="UTF-16" || cp=="UTF16" || cp=="UTF16-BIT") {
        return "UTF-16";
    }

    return "ANSI_1252";
}
