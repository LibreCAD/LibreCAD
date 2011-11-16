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

#ifndef DXFREADER_H
#define DXFREADER_H

//#include <string>

//class std::ifstream;

class dxfReader {
public:
    dxfReader(std::ifstream *stream){filestr = stream; count =0;}
    virtual ~dxfReader(){}
    virtual bool readCode(int *code) = 0; //return true if sucesful (not EOF)
    virtual bool readString(std::string *text) = 0;
    virtual bool readString() = 0;
    bool readRec(int *code, bool skip);
    virtual bool readInt() = 0;
    virtual bool readInt32() = 0;
    virtual bool readInt64() = 0;
    virtual bool readDouble() = 0;
    virtual bool readBool() = 0;
    std::string getString() {return strData;}
    double getDouble() {return doubleData;}
    int getInt32() {return intData;}
    int getInt64() {return int64;}
    bool getBool() {return intData;}
#ifdef DRW_DBG
    int count;//DBG
#endif
protected:
    std::ifstream *filestr;
    std::string strData;
    double doubleData;
    signed short intData; //16 bits integer
    unsigned long long int int64; //64 bits integer
};

class dxfReaderBinary : public dxfReader {
public:
    dxfReaderBinary(std::ifstream *stream):dxfReader(stream){ }
    virtual ~dxfReaderBinary() {}
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readInt();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readDouble();
    virtual bool readBool();
private:
//    std::ifstream *filestr;
};

class dxfReaderAscii : public dxfReader {
public:
    dxfReaderAscii(std::ifstream *stream):dxfReader(stream){ }
    virtual ~dxfReaderAscii(){}
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readInt();
    virtual bool readDouble();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readBool();
private:
//    std::ifstream *filestr;
};

#endif // DXFREADER_H
