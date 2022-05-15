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


#include "libdwgr.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include "intern/drw_dbg.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgreader.h"
#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"

#define FIRSTHANDLE 48

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dwgR::dwgR(const char* name)
    : fileName{ name }
{
    DRW_DBGSL(DRW_dbg::Level::None);
}

dwgR::~dwgR() = default;

void dwgR::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

/*reads metadata and loads image preview*/
bool dwgR::getPreview(){
    bool isOk = false;

    std::ifstream filestr;
    isOk = openFile(&filestr);
    if (!isOk)
        return false;

    isOk = reader->readMetaData();
    if (isOk) {
        isOk = reader->readPreview();
    } else
        error = DRW::BAD_READ_METADATA;

    filestr.close();
    if (reader) {
        reader.reset();
    }
    return isOk;
}

bool dwgR::testReader(){
    bool isOk = false;

    std::ifstream filestr;
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open() || !filestr.good() ){
        error = DRW::BAD_OPEN;
        return isOk;
    }

    dwgBuffer fileBuf(&filestr);
    duint8 *tmpStrData = new duint8[fileBuf.size()];
    fileBuf.getBytes(tmpStrData, fileBuf.size());
    dwgBuffer dataBuf(tmpStrData, fileBuf.size());
    fileBuf.setPosition(0);
    DRW_DBG("\ndwgR::testReader filebuf size: ");DRW_DBG(fileBuf.size());
    DRW_DBG("\ndwgR::testReader dataBuf size: ");DRW_DBG(dataBuf.size());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    fileBuf.setBitPos(4);
    dataBuf.setBitPos(4);
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    fileBuf.setBitPos(6);
    dataBuf.setBitPos(6);
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    fileBuf.setBitPos(0);
    dataBuf.setBitPos(0);
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());

    delete[]tmpStrData;
    filestr.close();
    DRW_DBG("\n\n");
    return isOk;
}

/*start reading dwg file header and, if can read it, continue reading all*/
bool dwgR::read(DRW_Interface *interface_, bool ext){
    bool isOk = false;
    applyExt = ext;
    iface = interface_;

//testReader();return false;

    std::ifstream filestr;
    isOk = openFile(&filestr);
    if (!isOk)
        return false;

    isOk = reader->readMetaData();
    if (isOk) {
        isOk = reader->readFileHeader();
        if (isOk) {
            isOk = processDwg();
        }
        else {
            error = DRW::BAD_READ_FILE_HEADER;
        }
    }
    else {
        error = DRW::BAD_READ_METADATA;
    }

    filestr.close();
    if (reader) {
        reader.reset();
    }

    return isOk;
}

/**
 * Factory method which creates a reader for the specified DWG version.
 *
 * \returns nullptr if version is not supported.
*/
std::unique_ptr<dwgReader> dwgR::createReaderForVersion(DRW::Version version, std::ifstream *stream, dwgR *p )
{
    switch ( version ) {
       // unsupported
       case DRW::UNKNOWNV:
       case DRW::MC00:
       case DRW::AC12:
       case DRW::AC14:
       case DRW::AC150:
       case DRW::AC210:
       case DRW::AC1002:
       case DRW::AC1003:
       case DRW::AC1004:
       case DRW::AC1006:
       case DRW::AC1009:
           break;

       case DRW::AC1012:
       case DRW::AC1014:
       case DRW::AC1015:
           return std::unique_ptr< dwgReader >( new dwgReader15( stream, p) );

       case DRW::AC1018:
           return std::unique_ptr< dwgReader >( new dwgReader18( stream, p) );

       case DRW::AC1021:
           return std::unique_ptr< dwgReader >( new dwgReader21( stream, p) );

       case DRW::AC1024:
           return std::unique_ptr< dwgReader >( new dwgReader24( stream, p) );

       case DRW::AC1027:
           return std::unique_ptr< dwgReader >( new dwgReader27( stream, p) );

       // unsupported
       case DRW::AC1032:
           break;
    }
    return nullptr;
}

/* Open the file and stores it in filestr, install the correct reader version.
 * If fail opening file, error are set as DRW::BAD_OPEN
 * If not are DWG or are unsupported version, error are set as DRW::BAD_VERSION
 * and closes filestr.
 * Return true on succeed or false on fail
*/
bool dwgR::openFile(std::ifstream *filestr){
    bool isOk = false;
    DRW_DBG("dwgR::read 1\n");
    filestr->open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr->is_open() || !filestr->good() ){
        error = DRW::BAD_OPEN;
        return isOk;
    }

    char line[7];
    filestr->read (line, 6);
    line[6]='\0';
    DRW_DBG("dwgR::read 2\n");
    DRW_DBG("dwgR::read line version: ");
    DRW_DBG(line);
    DRW_DBG("\n");

    // check version line against known version strings
    version = DRW::UNKNOWNV;
    for ( auto it = DRW::dwgVersionStrings.begin(); it != DRW::dwgVersionStrings.end(); ++it )
    {
        if ( strcmp( line, it->first ) == 0 ) {
            version = it->second;
            break;
        }
    }

    reader = createReaderForVersion( version, filestr, this );

    if (!reader) {
        error = DRW::BAD_VERSION;
        filestr->close();
    } else
        isOk = true;

    return isOk;
}

/********* Reader Process *********/

bool dwgR::processDwg() {
    DRW_DBG("dwgR::processDwg() start processing dwg\n");
    bool ret;
    bool ret2;
    DRW_Header hdr;
    ret = reader->readDwgHeader(hdr);
    if (!ret) {
        error = DRW::BAD_READ_HEADER;
    }

    ret2 = reader->readDwgClasses();
    if (ret && !ret2) {
        error = DRW::BAD_READ_CLASSES;
        ret = ret2;
    }

    ret2 = reader->readDwgHandles();
    if (ret && !ret2) {
        error = DRW::BAD_READ_HANDLES;
        ret = ret2;
    }

    ret2 = reader->readDwgTables(hdr);
    if (ret && !ret2) {
        error = DRW::BAD_READ_TABLES;
        ret = ret2;
    }

    iface->addHeader(&hdr);

    for (auto it=reader->ltypemap.begin(); it!=reader->ltypemap.end(); ++it) {
        DRW_LType *lt = it->second;
        iface->addLType(const_cast<DRW_LType&>(*lt) );
    }
    for (auto it=reader->layermap.begin(); it!=reader->layermap.end(); ++it) {
        DRW_Layer *ly = it->second;
        iface->addLayer(const_cast<DRW_Layer&>(*ly));
    }

    for (auto it=reader->stylemap.begin(); it!=reader->stylemap.end(); ++it) {
        DRW_Textstyle *ly = it->second;
        iface->addTextStyle(const_cast<DRW_Textstyle&>(*ly));
    }

    for (auto it=reader->dimstylemap.begin(); it!=reader->dimstylemap.end(); ++it) {
        DRW_Dimstyle *ly = it->second;
        iface->addDimStyle(const_cast<DRW_Dimstyle&>(*ly));
    }

    for (auto it=reader->vportmap.begin(); it!=reader->vportmap.end(); ++it) {
        DRW_Vport *ly = it->second;
        iface->addVport(const_cast<DRW_Vport&>(*ly));
    }

    for (auto it=reader->appIdmap.begin(); it!=reader->appIdmap.end(); ++it) {
        DRW_AppId *ly = it->second;
        iface->addAppId(const_cast<DRW_AppId&>(*ly));
    }

    ret2 = reader->readDwgBlocks(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_BLOCKS;
        ret = ret2;
    }

    ret2 = reader->readDwgEntities(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_ENTITIES;
        ret = ret2;
    }

    ret2 = reader->readDwgObjects(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_OBJECTS;
        ret = ret2;
    }

    return ret;
}
