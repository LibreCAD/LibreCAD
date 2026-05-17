/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD.org                                           **
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
#include "intern/dwgwriter.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"

#define FIRSTHANDLE 48

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dwgRW::dwgRW(const char* name)
    : fileName{ name }
{
    DRW_DBGSL(DRW_dbg::Level::None);
}

dwgRW::~dwgRW() = default;

void dwgRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

/*reads metadata and loads image preview*/
bool dwgRW::getPreview(){
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

bool dwgRW::testReader(){
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
    DRW_DBG("\ndwgRW::testReader filebuf size: ");DRW_DBG(fileBuf.size());
    DRW_DBG("\ndwgRW::testReader dataBuf size: ");DRW_DBG(dataBuf.size());
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
bool dwgRW::read(DRW_Interface *interface_, bool ext){
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
        // Capture per-entity failure count + skipped custom-class breakdown
        // before destroying the reader so the public getters (post-read) can
        // still surface them.
        m_entityParseFailures = reader->m_entityParseFailures;
        m_skippedCustomClasses = reader->m_skippedCustomClasses;
        reader.reset();
    }

    return isOk;
}

/**
 * Factory method which creates a reader for the specified DWG version.
 *
 * \returns nullptr if version is not supported.
*/
size_t dwgRW::getEntityParseFailures() const {
    // Prefer the dwgRW-side cache (survives reader.reset() at end of
    // read()). Fall back to live reader for the unusual case of a
    // caller querying mid-read.
    return reader ? reader->m_entityParseFailures : m_entityParseFailures;
}

std::unordered_map<std::string, size_t> dwgRW::getSkippedCustomClasses() const {
    return reader ? reader->m_skippedCustomClasses : m_skippedCustomClasses;
}

bool dwgRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin) {
    // The 'bin' parameter is accepted only for signature symmetry with
    // dxfRW::write — DWG is always binary on disk.
    (void)bin;
    if (ver != DRW::AC1015) {
        error = DRW::BAD_VERSION;
        return false;
    }
    if (interface_ == nullptr) {
        error = DRW::BAD_OPEN;
        return false;
    }
    iface = interface_;
    version = ver;
    error = DRW::BAD_NONE;

    std::ofstream filestr(fileName.c_str(),
                          std::ios_base::out | std::ios_base::binary |
                          std::ios_base::trunc);
    if (!filestr.is_open() || !filestr.good()) {
        error = DRW::BAD_OPEN;
        return false;
    }

    // Let the caller populate the header vars first.  Mirror of
    // dxfRW::write at libdxfrw.cpp:152-153.  The iface is allowed to
    // ignore the callback — in that case `header` keeps its default
    // (empty) state and the encoder emits per-var defaults.
    iface->writeHeader(header);

    writer = std::make_unique<dwgWriter15>(&filestr, &header);

    // If the caller did not set HANDSEED explicitly, seed it from the
    // writer's HandleAllocator high-water mark.  A null HANDSEED is
    // legal but causes AutoCAD to mark the file modified on first open.
    if (header.getHandSeed() == 0) {
        header.setHandSeed(writer->highWaterHandle());
    }

    // Section emit order (mirror of dxfRW::write).  Per-section helpers
    // emit framing; the iface callbacks drive caller-side enumeration
    // of entities/blocks/objects into the object stream between
    // writeDwgObjects (control objects + table records) and
    // writeDwgHandles (object map).
    bool ok = writer->writeFileHeaderStub() &&
              writer->writeDwgHeader() &&
              writer->writeDwgClasses() &&
              writer->writeDwgObjects();
    if (ok) {
        // Caller-driven object-stream content.  writeBlocks fires
        // first so the caller can `defineBlock(...)` for any user
        // blocks; we then emit BLOCK_CONTROL with the collected user
        // block_record handles + the 2 canonical phantoms.  Only after
        // that can the reader's findTableName resolve INSERT block
        // names.  writeEntities is where modelspace geometry flows;
        // writeObjects is reserved for NOD-dictionary objects (Phase 5).
        iface->writeBlocks();
        writer->emitDeferredBlockControl();
        iface->writeEntities();
        iface->writeObjects();
    }
    ok = ok &&
         writer->writeDwgHandles() &&
         writer->writeSecondHeader() &&
         writer->finalize();
    writer.reset();
    filestr.close();
    if (!ok) error = DRW::BAD_OPEN;
    return ok;
}

// Per-entity write API — invoked from the caller's `writeEntities`
// iface callback.  Each forwards to the writer's `encodeEntity` (a
// virtual on the base `dwgWriter`).  Returns false if the writer isn't
// ready (e.g., caller invoked outside `writeEntities`).
bool dwgRW::writePoint(DRW_Point *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeLine(DRW_Line *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeCircle(DRW_Circle *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeArc(DRW_Arc *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeEllipse(DRW_Ellipse *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeText(DRW_Text *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeLWPolyline(DRW_LWPolyline *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeRay(DRW_Ray *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeXline(DRW_Xline *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeTrace(DRW_Trace *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeSolid(DRW_Solid *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::write3dface(DRW_3Dface *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeInsert(DRW_Insert *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeMText(DRW_MText *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeSpline(DRW_Spline *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeAttrib(DRW_Attrib *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeAttdef(DRW_Attdef *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeHatch(DRW_Hatch *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

bool dwgRW::writeDimension(DRW_Dimension *ent) {
    if (writer == nullptr || ent == nullptr) return false;
    return writer->encodeEntity(ent);
}

duint32 dwgRW::defineBlock(const std::string& name, const DRW_Coord& basePoint) {
    if (writer == nullptr) return 0;
    return writer->defineBlock(name, basePoint);
}

std::unique_ptr<dwgReader> dwgRW::createReaderForVersion(DRW::Version version, std::ifstream *stream, dwgRW *p )
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

       case DRW::AC1032:
           return std::unique_ptr< dwgReader >( new dwgReader32( stream, p) );
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
bool dwgRW::openFile(std::ifstream *filestr){
    bool isOk = false;
    DRW_DBG("dwgRW::read 1\n");
    filestr->open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr->is_open() || !filestr->good() ){
        error = DRW::BAD_OPEN;
        return isOk;
    }

    char line[7];
    filestr->read (line, 6);
    line[6]='\0';
    DRW_DBG("dwgRW::read 2\n");
    DRW_DBG("dwgRW::read line version: ");
    DRW_DBG(line);
    DRW_DBG("\n");

    // check version line against known version strings
    version = DRW::UNKNOWNV;
    for ( auto it = DRW::dwgVersionStrings.begin(); it != DRW::dwgVersionStrings.end(); ++it )
    {
        if ( std::strncmp( line, it->first, sizeof(line) ) == 0 ) {
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

bool dwgRW::processDwg() {
    DRW_DBG("dwgRW::processDwg() start processing dwg\n");
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

    for (auto it=reader->viewmap.begin(); it!=reader->viewmap.end(); ++it) {
        DRW_View *vw = it->second;
        iface->addView(const_cast<DRW_View&>(*vw));
    }

    for (auto it=reader->ucsmap.begin(); it!=reader->ucsmap.end(); ++it) {
        DRW_UCS *u = it->second;
        iface->addUCS(const_cast<DRW_UCS&>(*u));
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
