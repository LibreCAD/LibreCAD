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

#ifndef DXFWRITER_H
#define DXFWRITER_H

//#include <string>

//class std::ifstream;

class dxfWriter {
public:
    dxfWriter(std::ofstream *stream){filestr = stream; /*count =0;*/}
    virtual ~dxfWriter(){}
    virtual bool writeString(int code, std::string text) = 0;
/*    virtual bool readCode(int *code) = 0; //return true if sucesful (not EOF)
    virtual bool readString(std::string *text) = 0;
    virtual bool readString() = 0;*/
//    bool readRec(int *code, bool skip);
    virtual bool writeInt16(int code, int data) = 0;
    virtual bool writeInt32(int code, int data) = 0;
    virtual bool writeInt64(int code, unsigned long long int data) = 0;
    virtual bool writeDouble(int code, double data) = 0;
    virtual bool writeBool(int code, bool data) = 0;
/*    std::string getString() {return strData;}
    double getDouble() {return doubleData;}
    int getInt32() {return intData;}
    int getInt64() {return int64;}
    bool getBool() {return intData;}
#ifdef DRW_DBG
    int count;//DBG
#endif*/
protected:
    std::ofstream *filestr;
//    std::string strData;
//    double doubleData;
//    signed short intData; //16 bits integer
//    unsigned long long int int64; //64 bits integer
};

class dxfWriterBinary : public dxfWriter {
public:
    dxfWriterBinary(std::ofstream *stream):dxfWriter(stream){ }
    virtual ~dxfWriterBinary() {}
    virtual bool writeString(int code, std::string text);
/*    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();*/
    virtual bool writeInt16(int code, int data);
    virtual bool writeInt32(int code, int data);
    virtual bool writeInt64(int code, unsigned long long int data);
    virtual bool writeDouble(int code, double data);
    virtual bool writeBool(int code, bool data);
private:
//    std::ifstream *filestr;
};

class dxfWriterAscii : public dxfWriter {
public:
    dxfWriterAscii(std::ofstream *stream):dxfWriter(stream){ }
    virtual ~dxfWriterAscii(){}
    virtual bool writeString(int code, std::string text);
/*    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();*/
    virtual bool writeInt16(int code, int data);
    virtual bool writeInt32(int code, int data);
    virtual bool writeInt64(int code, unsigned long long int data);
    virtual bool writeDouble(int code, double data);
    virtual bool writeBool(int code, bool data);
private:
//    std::ifstream *filestr;
};

#endif // DXFWRITER_H
