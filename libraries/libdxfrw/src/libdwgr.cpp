/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
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
#include <cstring>
#include <sstream>
#include <utility>
#include "intern/drw_dbg.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgreader.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgwriter.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
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
    error = DRW::BAD_NONE;

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
    std::uint8_t *tmpStrData = new std::uint8_t[fileBuf.size()];
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
    error = DRW::BAD_NONE;
    applyExt = ext;
    iface = interface_;
    resetReadDiagnostics();

//testReader();return false;

    std::ifstream filestr;
    isOk = openFile(&filestr);
    if (!isOk)
        return false;

    isOk = readInstalledReader();
    filestr.close();

    return isOk;
}

bool dwgRW::readBuffer(const std::uint8_t *data, std::uint64_t size,
                       DRW_Interface *interface_, bool ext) {
    error = DRW::BAD_NONE;
    applyExt = ext;
    iface = interface_;
    resetReadDiagnostics();

    if (data == nullptr || size < 6) {
        error = DRW::BAD_OPEN;
        return false;
    }

    auto buffer = std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(data), size);
    if (!openBuffer(std::move(buffer)))
        return false;

    return readInstalledReader();
}

bool dwgRW::readInstalledReader() {
    if (!reader) {
        error = DRW::BAD_OPEN;
        return false;
    }

    bool isOk = reader->readMetaData();
    if (isOk) {
        isOk = reader->readFileHeader();
        if (isOk) {
            isOk = processDwg();
        } else {
            error = DRW::BAD_READ_FILE_HEADER;
        }
    } else {
        error = DRW::BAD_READ_METADATA;
    }

    captureReaderDiagnostics();
    reader.reset();
    return isOk;
}

void dwgRW::captureReaderDiagnostics() {
    if (!reader)
        return;

    // Capture per-entity failure count + skipped custom-class breakdown before
    // destroying the reader so the public getters (post-read) can still surface
    // them.
    m_entityParseFailures = reader->m_entityParseFailures;
    m_objectParseFailures = reader->m_objectParseFailures;
    m_classesCrcMismatch = reader->m_classesCrcMismatch;
    m_skippedCustomClasses = reader->m_skippedCustomClasses;
    m_skippedUnsupportedObjects = reader->m_skippedUnsupportedObjects;
    m_decodedProxyPrimitives = reader->m_decodedProxyPrimitives;
    m_layerNameOrder = reader->m_layerNameOrder;
    m_ltypeNameOrder = reader->m_ltypeNameOrder;
    codePage = reader->getCodePage();
}

void dwgRW::resetReadDiagnostics() {
    m_entityParseFailures = 0;
    m_objectParseFailures = 0;
    m_classesCrcMismatch = 0;
    m_skippedCustomClasses.clear();
    m_skippedUnsupportedObjects.clear();
    m_decodedProxyPrimitives = 0;
    m_layerNameOrder.clear();
    m_ltypeNameOrder.clear();
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

size_t dwgRW::getObjectParseFailures() const {
    // Mirrors getEntityParseFailures: prefer the dwgRW-side cache (survives
    // reader.reset()), fall back to the live reader for a mid-read query.
    return reader ? reader->m_objectParseFailures : m_objectParseFailures;
}

size_t dwgRW::getClassesCrcMismatch() const {
    // Non-fatal R13/R15 CLASSES CRC mismatch count (warn-only). Same
    // cache-then-live pattern as the parse-failure getters.
    return reader ? reader->m_classesCrcMismatch : m_classesCrcMismatch;
}

std::unordered_map<std::string, size_t> dwgRW::getSkippedCustomClasses() const {
    return reader ? reader->m_skippedCustomClasses : m_skippedCustomClasses;
}

std::unordered_map<std::string, size_t> dwgRW::getSkippedUnsupportedObjects() const {
    return reader ? reader->m_skippedUnsupportedObjects : m_skippedUnsupportedObjects;
}

void dwgRW::resetWriteSkipCounters() {
    m_writeSkipCounters = {};
}

bool dwgRW::recordWriteResult(WriteSkipKind kind, bool ok) {
    if (ok)
        return true;

    switch (kind) {
    case WriteSkipKind::Entity:
        ++m_writeSkipCounters.entityWrites;
        break;
    case WriteSkipKind::TableRecord:
        ++m_writeSkipCounters.tableRecordWrites;
        break;
    case WriteSkipKind::Object:
        ++m_writeSkipCounters.objectWrites;
        break;
    case WriteSkipKind::ClassRegistration:
        ++m_writeSkipCounters.classRegistrations;
        break;
    case WriteSkipKind::RawObject:
        ++m_writeSkipCounters.rawObjectWrites;
        break;
    case WriteSkipKind::RawSection:
        ++m_writeSkipCounters.rawSectionWrites;
        break;
    case WriteSkipKind::BlockDefinition:
        ++m_writeSkipCounters.blockDefinitions;
        break;
    }
    return false;
}

bool dwgRW::encodeEntityForWrite(DRW_Entity *ent) {
    return recordWriteResult(WriteSkipKind::Entity,
                             writer != nullptr && ent != nullptr
                                 && writer->encodeEntity(ent));
}

bool dwgRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin) {
    // The 'bin' parameter is accepted only for signature symmetry with
    // dxfRW::write — DWG is always binary on disk.
    (void)bin;
    resetWriteSkipCounters();
    if (ver != DRW::AC1015 && ver != DRW::AC1018 &&
        ver != DRW::AC1024 && ver != DRW::AC1027 &&
        ver != DRW::AC1032) {
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

    if (ver == DRW::AC1032)
        writer = std::make_unique<dwgWriter32>(&filestr, &header);
    else if (ver == DRW::AC1027)
        writer = std::make_unique<dwgWriter27>(&filestr, &header);
    else if (ver == DRW::AC1024)
        writer = std::make_unique<dwgWriter24>(&filestr, &header);
    else if (ver == DRW::AC1018)
        writer = std::make_unique<dwgWriter18>(&filestr, &header);
    else
        writer = std::make_unique<dwgWriter15>(&filestr, &header);

    // Seed caller-reserved handles into the writer's HandleAllocator BEFORE
    // any defineBlock()/next() mint (writeBlocks runs below).  Without this a
    // block-record handle minted from 0x30 can collide with a fixed-type
    // OBJECT's preserved low handle → duplicate object-map entry →
    // writeDwgHandles() fails → BAD_OPEN aborts the whole save. (P3 #1)
    for (std::uint32_t h : m_reservedHandles)
        writer->reserveHandle(h);

    iface->writeDwgClasses();

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
              writer->writeDwgClasses();
    if (ok) {
        // Collect user-defined table records before emitting the objects
        // section.  Each iface callback calls back into dwgRW::add*() which
        // forwards to the writer's pending lists.  Order matters: LTypes
        // before Layers so ltype→handle resolution works in emitLayerRecord.
        iface->writeLTypes();
        iface->writeLayers();
        iface->writeTextstyles();
        iface->writeViews();
        iface->writeUCSs();
        iface->writeVports();
        iface->writeDimstyles();
        iface->writeAppId();
        ok = writer->writeDwgObjects();
    }
    if (ok) {
        // Caller-driven object-stream content.  writeBlocks fires
        // first so the caller can `defineBlock(...)` for any user
        // blocks; we then emit BLOCK_CONTROL with the collected user
        // block_record handles + the 2 canonical phantoms.  Only after
        // that can the reader's findTableName resolve INSERT block
        // names.  writeEntities is where modelspace geometry flows;
        // writeObjects is reserved for NOD-dictionary objects (Phase 5).
        iface->writeBlocks();
        ok = writer->emitDeferredBlockControl();
    }
    if (ok) {
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
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLine(DRW_Line *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeCircle(DRW_Circle *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeArc(DRW_Arc *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeEllipse(DRW_Ellipse *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeText(DRW_Text *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeRText(DRW_RText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeArcAlignedText(DRW_ArcAlignedText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLWPolyline(DRW_LWPolyline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeRay(DRW_Ray *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeXline(DRW_Xline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeTrace(DRW_Trace *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSolid(DRW_Solid *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::write3dface(DRW_3Dface *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeInsert(DRW_Insert *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeTable(DRW_Table *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMText(DRW_MText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSpline(DRW_Spline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeHelix(DRW_Helix *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeAttrib(DRW_Attrib *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeAttdef(DRW_Attdef *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeHatch(DRW_Hatch *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMPolygon(DRW_MPolygon *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeDimension(DRW_Dimension *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeTolerance(DRW_Tolerance *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLight(DRW_Light *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMLine(DRW_MLine *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeUnderlay(DRW_Underlay *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    // Entity custom class must be in CLASSES before the body is encoded.
    if (!registerUnderlayEntityClass(ent->kind))
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return encodeEntityForWrite(ent);
}

bool dwgRW::registerUnderlayEntityClass(DRW_Underlay::Kind kind) {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerUnderlayEntityClass(kind));
}

bool dwgRW::writePolyline(DRW_Polyline *ent) {
    if (writer == nullptr || ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    // Pre-allocate the polyline handle BEFORE vertex handles so that
    // readDwgEntities (which iterates ObjectMap in ascending-handle order)
    // processes the polyline first and can consume vertices via readPlineVertex.
    if (ent->handle == 0)
        ent->handle = writer->allocNextHandle();
    else
        writer->reserveHandle(ent->handle);
    const bool isPolyface = (ent->flags & 64) != 0;
    const bool isMesh = (ent->flags & 16) != 0;
    const bool is3D = (ent->flags & 8) != 0;
    // Encode vertices (they receive higher handles than the polyline).
    for (auto& v : ent->vertlist) {
        if (v && v->dwgSubtype() == DRW_Vertex::DwgSubtype::Auto) {
            if (isPolyface) {
                v->setDwgSubtype((v->flags & 64) != 0
                    ? DRW_Vertex::DwgSubtype::Polyface
                    : DRW_Vertex::DwgSubtype::PolyfaceFace);
            } else if (isMesh) {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Mesh);
            } else if (is3D) {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D);
            } else {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
            }
        }
        if (v && !writer->encodeEntity(v.get()))
            return recordWriteResult(WriteSkipKind::Entity, false);
    }
    DRW_SeqEnd seqEnd;
    seqEnd.handle = writer->allocNextHandle();
    ent->setDwgSeqEndHandle(seqEnd.handle);
    if (!writer->encodeEntity(&seqEnd))
        return recordWriteResult(WriteSkipKind::Entity, false);
    return recordWriteResult(WriteSkipKind::Entity, writer->encodeEntity(ent));
}

bool dwgRW::writeLeader(DRW_Leader *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMLeader(DRW_MLeader *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeViewport(DRW_Viewport *ent) {
    return encodeEntityForWrite(ent);
}

// Phase 6.1 — SHAPE passthrough (no native LibreCAD entity).
bool dwgRW::writeShape(DRW_Shape *ent) {
    return encodeEntityForWrite(ent);
}

// Phase 6.2 — OLE2FRAME passthrough (opaque payload preserved by encodeDwg).
bool dwgRW::writeOle2Frame(DRW_Ole2Frame *ent) {
    return encodeEntityForWrite(ent);
}

// Table-record add* methods — forward to dwgWriter15 via dynamic_cast since
// the add*() API lives on dwgWriter15 (all concrete writers derive from it).
// Declared early so entity writers (IMAGE / IMAGEDEF) can reuse it.
static dwgWriter15 *asWriter15(std::unique_ptr<dwgWriter> &w) {
    return dynamic_cast<dwgWriter15 *>(w.get());
}

bool dwgRW::writeMesh(DRW_Mesh *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeWipeout(DRW_Wipeout *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    // Ensure WIPEOUT custom class is present (bootstrap also registers 530).
    if (!registerWipeoutEntityClass())
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeImageDef(DRW_ImageDef *object) {
    auto *w = asWriter15(writer);
    if (w == nullptr || object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeImageDef(*object));
}

bool dwgRW::writeImageDefinitionReactor(DRW_ImageDefinitionReactor *object) {
    auto *w = asWriter15(writer);
    if (w == nullptr || object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object,
                             w->writeImageDefinitionReactor(*object));
}

bool dwgRW::registerImageDefReactorObjectClass(
    DRW_ImageDefinitionReactor *object) {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    const std::uint32_t handle = object != nullptr ? object->handle : 0;
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerImageDefReactorObjectClass(handle));
}

bool dwgRW::writeImage(DRW_Image *ent, const std::string *fileName) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);

    // When a path is provided, emit IMAGEDEF + reactor and wire handles so
    // re-read delivers both the entity frame and linkImage().
    if (fileName != nullptr && !fileName->empty()) {
        auto *w = asWriter15(writer);
        if (w == nullptr)
            return recordWriteResult(WriteSkipKind::Entity, false);

        DRW_ImageDef imageDef;
        imageDef.name = *fileName;
        imageDef.imgVersion = 0;
        imageDef.u = ent->sizeu;
        imageDef.v = ent->sizev;
        imageDef.up = 1.0;
        imageDef.vp = 1.0;
        imageDef.loaded = 1;
        imageDef.resolution = 0;
        imageDef.parentHandle = 0xC;  // named object dictionary default owner
        if (!w->writeImageDef(imageDef))
            return recordWriteResult(WriteSkipKind::Object, false);

        // Allocate the image entity handle before the reactor so the reactor
        // can name the image as its owner when the caller left it unset.
        if (ent->handle == 0)
            ent->handle = w->allocNextHandle();
        else
            w->reserveHandle(ent->handle);

        DRW_ImageDefinitionReactor reactor;
        reactor.m_classVersion = 2;
        reactor.parentHandle = static_cast<int>(ent->handle);
        if (!w->writeImageDefinitionReactor(reactor))
            return recordWriteResult(WriteSkipKind::Object, false);

        ent->ref = imageDef.handle;
        ent->m_imageDefReactorHandle = reactor.handle;
    }

    // IMAGE uses fixed oType 101 — no custom class registration.
    if (ent->m_clipBoundaryType == 0 && ent->clipPath.empty()) {
        // Default full-image rectangle so encodeDwg accepts a fresh IMAGE.
        ent->m_clipBoundaryType = 1;
        ent->clipPath = {DRW_Coord{-0.5, -0.5, 0.0},
                         DRW_Coord{ent->sizeu - 0.5, ent->sizev - 0.5, 0.0}};
    }
    return encodeEntityForWrite(ent);
}

bool dwgRW::writePointCloud(DRW_PointCloud *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writePointCloudEx(DRW_PointCloudEx *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSurface(DRW_Surface *ent) {
    return encodeEntityForWrite(ent);
}

std::uint32_t dwgRW::defineBlock(const std::string& name, const DRW_Coord& basePoint,
                           int insUnits) {
    if (writer == nullptr) {
        (void)recordWriteResult(WriteSkipKind::BlockDefinition, false);
        return 0;
    }
    const std::uint32_t handle = writer->defineBlock(name, basePoint, insUnits);
    (void)recordWriteResult(WriteSkipKind::BlockDefinition, handle != 0);
    return handle;
}

bool dwgRW::beginBlockContent(std::uint32_t blockRecordHandle) {
    return writer != nullptr && writer->beginBlockContent(blockRecordHandle);
}

bool dwgRW::endBlockContent() {
    return writer != nullptr && writer->endBlockContent();
}

bool dwgRW::addLType(DRW_LType *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addLType(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addLayer(DRW_Layer *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addLayer(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addTextstyle(DRW_Textstyle *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addTextstyle(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addUCS(DRW_UCS *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addUcs(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addView(DRW_View *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addView(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addVport(DRW_Vport *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addVport(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addDimstyle(DRW_Dimstyle *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addDimstyle(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}
bool dwgRW::addAppId(DRW_AppId *ent) {
    if (ent == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    w->addAppId(*ent);
    return recordWriteResult(WriteSkipKind::TableRecord, true);
}

bool dwgRW::writeAcDbPlaceholder(DRW_AcDbPlaceholder *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object,
                             w->writeAcDbPlaceholder(*object));
}

bool dwgRW::registerSunObjectClass(DRW_Sun *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSunObjectClass(object->handle));
}

bool dwgRW::writeSun(DRW_Sun *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeSun(*object));
}

bool dwgRW::registerMLeaderStyleObjectClass(DRW_MLeaderStyle *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerMLeaderStyleObjectClass(object->handle));
}

bool dwgRW::writeMLeaderStyle(DRW_MLeaderStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeMLeaderStyle(*object));
}

bool dwgRW::writeDictionary(DRW_Dictionary *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeDictionary(*object));
}

bool dwgRW::writeXRecord(DRW_XRecord *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeXRecord(*object));
}

bool dwgRW::writeLayout(DRW_Layout *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeLayout(*object));
}

bool dwgRW::writeGroup(DRW_Group *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeGroup(*object));
}

bool dwgRW::writeMLineStyle(DRW_MLineStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeMLineStyle(*object));
}

bool dwgRW::registerRasterVariablesObjectClass(DRW_RasterVariables *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerRasterVariablesObjectClass(object->handle));
}

bool dwgRW::writeRasterVariables(DRW_RasterVariables *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeRasterVariables(*object));
}

bool dwgRW::registerWipeoutVariablesObjectClass(DRW_WipeoutVariables *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerWipeoutVariablesObjectClass(object->handle));
}

bool dwgRW::registerWipeoutEntityClass() {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerWipeoutEntityClass());
}

bool dwgRW::writeWipeoutVariables(DRW_WipeoutVariables *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeWipeoutVariables(*object));
}

bool dwgRW::registerGeoDataObjectClass(DRW_GeoData *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerGeoDataObjectClass(object->handle));
}

bool dwgRW::writeGeoData(DRW_GeoData *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeGeoData(*object));
}

bool dwgRW::registerSpatialFilterObjectClass(DRW_SpatialFilter *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSpatialFilterObjectClass(object->handle));
}

bool dwgRW::writeSpatialFilter(DRW_SpatialFilter *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeSpatialFilter(*object));
}

// PR 8d.2a — five small no-storage OBJECTS families.  Same wrapper shape as
// the PR 8d.1b/c/d trio (RasterVariables/GeoData/SpatialFilter).
bool dwgRW::registerScaleObjectClass(DRW_Scale *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerScaleObjectClass(object->handle));
}

bool dwgRW::writeScale(DRW_Scale *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeScale(*object));
}

bool dwgRW::registerIDBufferObjectClass(DRW_IDBuffer *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerIDBufferObjectClass(object->handle));
}

bool dwgRW::writeIDBuffer(DRW_IDBuffer *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeIDBuffer(*object));
}

bool dwgRW::registerLayerIndexObjectClass(DRW_LayerIndex *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerLayerIndexObjectClass(object->handle));
}

bool dwgRW::writeLayerIndex(DRW_LayerIndex *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeLayerIndex(*object));
}

bool dwgRW::registerSpatialIndexObjectClass(DRW_SpatialIndex *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSpatialIndexObjectClass(object->handle));
}

bool dwgRW::writeSpatialIndex(DRW_SpatialIndex *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeSpatialIndex(*object));
}

bool dwgRW::registerDictionaryVarObjectClass(DRW_DictionaryVar *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerDictionaryVarObjectClass(object->handle));
}

bool dwgRW::writeDictionaryVar(DRW_DictionaryVar *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeDictionaryVar(*object));
}

// PR 8d.2b — four larger no-storage OBJECTS families.  Same wrapper shape as
// the PR 8d.2a trio.
bool dwgRW::registerDictionaryWithDefaultObjectClass(DRW_DictionaryWithDefault *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerDictionaryWithDefaultObjectClass(object->handle));
}

bool dwgRW::writeDictionaryWithDefault(DRW_DictionaryWithDefault *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object,
                             w->writeDictionaryWithDefault(*object));
}

bool dwgRW::registerSortEntsTableObjectClass(DRW_SortEntsTable *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSortEntsTableObjectClass(object->handle));
}

bool dwgRW::writeSortEntsTable(DRW_SortEntsTable *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeSortEntsTable(*object));
}

bool dwgRW::registerFieldListObjectClass(DRW_FieldList *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerFieldListObjectClass(object->handle));
}

bool dwgRW::writeFieldList(DRW_FieldList *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeFieldList(*object));
}

bool dwgRW::registerFieldObjectClass(DRW_Field *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerFieldObjectClass(object->handle));
}

bool dwgRW::writeField(DRW_Field *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object, w->writeField(*object));
}

bool dwgRW::registerUnderlayDefinitionObjectClass(
    DRW_UnderlayDefinition *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerUnderlayDefinitionObjectClass(object->kind,
                                                      object->handle));
}

bool dwgRW::writeUnderlayDefinition(DRW_UnderlayDefinition *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return recordWriteResult(WriteSkipKind::Object,
                             w->writeUnderlayDefinition(*object));
}

bool dwgRW::registerRawDwgObjectClass(const DRW_UnsupportedObject *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    const bool registered = writer->registerRawObjectClass(*object);
    if (registered && object->m_handle != 0)
        writer->reserveHandle(object->m_handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration, registered);
}

bool dwgRW::writeRawDwgObject(DRW_UnsupportedObject *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::RawObject, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::RawObject, false);
    return recordWriteResult(WriteSkipKind::RawObject, w->replayRawObject(*object));
}

bool dwgRW::writeRawDwgSection(const DRW_RawDwgSection *section) {
    if (section == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::RawSection, false);
    return recordWriteResult(WriteSkipKind::RawSection,
                             writer->addRawDwgSection(*section));
}

std::unique_ptr<dwgReader> dwgRW::createReaderForVersion(
    DRW::Version version, std::unique_ptr<dwgBuffer> buffer, dwgRW *p)
{
    switch ( version ) {
       // unsupported (no parser exists)
       case DRW::UNKNOWNV:
       case DRW::MC00:
       case DRW::AC12:
       case DRW::AC150:
       case DRW::AC1002:
           break;          // R2.5: same family as R2.6, but no corpus fixture
                           //       to validate -> left rejected for now.
       case DRW::AC14:     // R1.40: dedicated pre-R2.0b reader (different
                           //   container: no @0x14 section pointers / no
                           //   per-record size or CRC; entity stream @0x202).
           return std::unique_ptr< dwgReader >( new dwgReaderR1_40(std::move(buffer), p) );
       case DRW::AC210:    // R2.10, R2.6, R9: same pre-R13 container as R10 (the
       case DRW::AC1003:   //   SINCE(R_2_0b)/PRE(R_10) branch). dwgReaderR11
       case DRW::AC1004:   //   handles them via `version`-gated deltas (1B LTYPE
                           //   handle, 2D LINE/POINT/3DLINE bodies, elevation-
                           //   for-all). Validated vs dwgread.
       case DRW::AC1006:   // R10: 1B LTYPE handle; bodies read 3D unless HAS_ELEVATION.
       case DRW::AC1009:   // R11: 2B LTYPE handle + 2-byte table `used` field.
           return std::unique_ptr< dwgReader >( new dwgReaderR11(std::move(buffer), p) );

       case DRW::AC1012:
       case DRW::AC1014:
       case DRW::AC1015:
           return std::unique_ptr< dwgReader >( new dwgReader15(std::move(buffer), p) );

       case DRW::AC1018:
           return std::unique_ptr< dwgReader >( new dwgReader18(std::move(buffer), p) );

       case DRW::AC1021:
           return std::unique_ptr< dwgReader >( new dwgReader21(std::move(buffer), p) );

       case DRW::AC1024:
           return std::unique_ptr< dwgReader >( new dwgReader24(std::move(buffer), p) );

       case DRW::AC1027:
           return std::unique_ptr< dwgReader >( new dwgReader27(std::move(buffer), p) );

       case DRW::AC1032:
           return std::unique_ptr< dwgReader >( new dwgReader32(std::move(buffer), p) );
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

    auto buffer = std::make_unique<dwgBuffer>(filestr);
    isOk = openBuffer(std::move(buffer));
    if (!isOk)
        filestr->close();

    return isOk;
}

bool dwgRW::openBuffer(std::unique_ptr<dwgBuffer> buffer) {
    bool isOk = false;
    if (!buffer || buffer->size() < 6) {
        error = DRW::BAD_VERSION;
        return false;
    }

    version = sniffVersion(buffer.get());
    reader = createReaderForVersion(version, std::move(buffer), this);

    if (!reader) {
        error = DRW::BAD_VERSION;
    } else
        isOk = true;

    return isOk;
}

DRW::Version dwgRW::sniffVersion(dwgBuffer *buffer) {
    if (buffer == nullptr || buffer->size() < 6)
        return DRW::UNKNOWNV;

    char line[7];
    for (int i = 0; i < 6; ++i)
        line[i] = static_cast<char>(buffer->getRawChar8());
    line[6]='\0';
    DRW_DBG("dwgRW::read 2\n");
    DRW_DBG("dwgRW::read line version: ");
    DRW_DBG(line);
    DRW_DBG("\n");

    // check version line against known version strings
    DRW::Version sniffedVersion = DRW::UNKNOWNV;
    for ( auto it = DRW::dwgVersionStrings.begin(); it != DRW::dwgVersionStrings.end(); ++it )
    {
        if ( std::strncmp( line, it->first, sizeof(line) ) == 0 ) {
            sniffedVersion = it->second;
            break;
        }
    }

    buffer->resetPosition();
    return sniffedVersion;
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

    // readDwgTables/Blocks/Entities/Objects all depend on a valid header,
    // classes map and object handle map from the phases above -- on a
    // corrupted file where one of those already failed (ret is false),
    // running them anyway walks whatever ended up in ObjectMap (empty,
    // partial, or built from garbage offsets) and pays full CRC/decompress
    // cost per bogus "entity"/"object" before each is individually reported
    // as a parse failure. `ret &&` short-circuits the call once ret is
    // false, turning that multi-second futile parse into a fast, clear
    // error return; the already-set error code (from whichever phase failed
    // first) is preserved, matching the existing "first failure wins"
    // pattern below.
    ret2 = ret && reader->readDwgTables(hdr);
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

    ret2 = ret && reader->readDwgBlocks(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_BLOCKS;
        ret = ret2;
    }

    ret2 = ret && reader->readDwgEntities(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_ENTITIES;
        ret = ret2;
    }

    ret2 = ret && reader->readDwgObjects(*iface);
    if (ret && !ret2) {
        error = DRW::BAD_READ_OBJECTS;
        ret = ret2;
    }

    if (ret) {
        for (const DRW_RawDwgSection& section : reader->m_rawDwgSections)
            iface->addRawDwgSection(section);
        for (const DRW_DataStorageSection& storage : reader->m_dataStorageSections)
            iface->addDataStorage(storage);
    }

    return ret;
}
