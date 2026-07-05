/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2021 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/


#include "libdxfrw.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <sstream>
#include <cassert>
#include "intern/drw_textcodec.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/drw_dbg.h"
#include "intern/dwgutil.h"
#include "intern/proxygraphicdecoder.h"

#define FIRSTHANDLE 48

namespace {

void writeDxfSplineBody(dxfWriter *writer, DRW_Spline *ent) {
    // Normal vector is optional; omit when it is the default (0,0,1).
    if (ent->normalVec.x != 0.0 || ent->normalVec.y != 0.0 || ent->normalVec.z != 1.0) {
        writer->writeDouble(210, ent->normalVec.x);
        writer->writeDouble(220, ent->normalVec.y);
        writer->writeDouble(230, ent->normalVec.z);
    }
    int flags = ent->flags;
    if (std::any_of(ent->weightlist.begin(), ent->weightlist.end(),
                    [](double weight) { return std::fabs(weight - 1.0) > 1e-12; })) {
        flags |= 0x04;
    }
    writer->writeInt16(70, flags);
    writer->writeInt16(71, ent->degree);
    writer->writeInt16(72, static_cast<int>(ent->knotslist.size()));
    writer->writeInt16(73, static_cast<int>(ent->controllist.size()));
    writer->writeInt16(74, static_cast<int>(ent->fitlist.size()));
    writer->writeDouble(42, ent->tolknot);
    writer->writeDouble(43, ent->tolcontrol);
    writer->writeDouble(44, ent->tolfit);
    for (double k : ent->knotslist)
        writer->writeDouble(40, k);
    // Control points with interleaved weights (when present)
    for (std::size_t i = 0; i < ent->controllist.size(); ++i) {
        const auto& crd = ent->controllist[i];
        writer->writeDouble(10, crd->x);
        writer->writeDouble(20, crd->y);
        writer->writeDouble(30, crd->z);
        if (i < ent->weightlist.size())
            writer->writeDouble(41, ent->weightlist[i]);
    }
    for (const auto& crd : ent->fitlist) {
        writer->writeDouble(11, crd->x);
        writer->writeDouble(21, crd->y);
        writer->writeDouble(31, crd->z);
    }
    // Start/end tangent vectors (fit-point splines, codes 12/22/32 and 13/23/33)
    if (ent->tgStart.x != 0.0 || ent->tgStart.y != 0.0 || ent->tgStart.z != 0.0) {
        writer->writeDouble(12, ent->tgStart.x);
        writer->writeDouble(22, ent->tgStart.y);
        writer->writeDouble(32, ent->tgStart.z);
    }
    if (ent->tgEnd.x != 0.0 || ent->tgEnd.y != 0.0 || ent->tgEnd.z != 0.0) {
        writer->writeDouble(13, ent->tgEnd.x);
        writer->writeDouble(23, ent->tgEnd.y);
        writer->writeDouble(33, ent->tgEnd.z);
    }
}

std::size_t tableColumnCount(const DRW_Table& table) {
    std::size_t columns = table.m_content.m_columns.size();
    for (const auto& row : table.m_content.m_rows)
        columns = std::max(columns, row.m_cells.size());
    return columns;
}

UTF8STRING tableCellText(const DRW_TableCell& cell) {
    for (const DRW_TableCellContent& content : cell.m_contents) {
        if (!content.m_text.empty())
            return content.m_text;
        if (!content.m_value.m_valueString.empty())
            return content.m_value.m_valueString;
        if (content.m_value.m_value.type() == DRW_Variant::STRING)
            return content.m_value.m_value.c_str();
    }
    return UTF8STRING();
}

const char *modelerGeometryDxfName(DRW::ETYPE type) {
    switch (type) {
    case DRW::E3DSOLID:
        return "3DSOLID";
    case DRW::REGION:
        return "REGION";
    case DRW::BODY:
        return "BODY";
    default:
        return nullptr;
    }
}

const char *modelerGeometryDxfSubclass(DRW::ETYPE type) {
    switch (type) {
    case DRW::E3DSOLID:
        return "AcDb3dSolid";
    case DRW::REGION:
        return "AcDbRegion";
    case DRW::BODY:
        return "AcDbBody";
    default:
        return nullptr;
    }
}

bool isTextAcisPayload(const std::vector<std::uint8_t>& data) {
    return std::all_of(data.begin(), data.end(), [](std::uint8_t byte) {
        return byte == '\n' || byte == '\r' || byte == '\t' ||
               (byte >= 0x20 && byte < 0x7f);
    });
}

void writeDxfTextChunks(dxfWriter *writer, const std::vector<std::uint8_t>& data) {
    const std::string text(data.begin(), data.end());
    constexpr std::size_t kChunkSize = 255;
    for (std::size_t off = 0; off < text.size(); off += kChunkSize) {
        const std::size_t n = std::min(kChunkSize, text.size() - off);
        writer->writeString(off == 0 ? 1 : 3, text.substr(off, n));
    }
}

void writeDxfBinaryChunks(dxfWriter *writer, const std::vector<std::uint8_t>& data) {
    static const char hexd[] = "0123456789ABCDEF";
    constexpr std::size_t kChunkBytes = 127;
    for (std::size_t off = 0; off < data.size(); off += kChunkBytes) {
        const std::size_t n = std::min(kChunkBytes, data.size() - off);
        std::string chunk;
        chunk.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t byte = data[off + i];
            chunk.push_back(hexd[byte >> 4]);
            chunk.push_back(hexd[byte & 0x0F]);
        }
        writer->writeString(310, chunk);
    }
}

}

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dxfRW::dxfRW(const char* name){
    DRW_DBGSL(DRW_dbg::Level::None);
    fileName = name;
    applyExt = false;
    elParts = 128; //parts number when convert ellipse to polyline
}
dxfRW::~dxfRW(){
    for (std::vector<DRW_ImageDef*>::iterator it=imageDef.begin(); it!=imageDef.end(); ++it)
        delete *it;

    imageDef.clear();
}

void dxfRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

bool dxfRW::read(DRW_Interface *interface_, bool ext){
    drw_assert(fileName.empty() == false);
    applyExt = ext;
    std::ifstream filestr;
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    DRW_DBG("dxfRW::read 1def\n");
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open()
        || !filestr.good()) {
        return setError(DRW::BAD_OPEN);
    }

    char line[24]{};
    char line2[22] = "AutoCAD Binary DXF\r\n";
    line2[20] = (char)26;
    line2[21] = '\0';
    // Read the 22-byte sentinel plus the first 2 bytes of the group stream so
    // the binary sub-format can be detected before any group is parsed.
    filestr.read (line, 24);
    filestr.close();
    iface = interface_;
    DRW_DBG("dxfRW::read 2\n");
    // `line` is filled by an unterminated read; compare the sentinel by exact
    // length to avoid strcmp reading past the buffer when the sentinel
    // bytes don't include an embedded NUL.
    if (std::memcmp(line, line2, 22) == 0) {
        filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
        binFile = true;
        //skip sentinel
        filestr.seekg (22, std::ios::beg);
        // R12/AC1009 binary uses 1-byte group codes; R13+ uses 2-byte LE. The
        // first group is always code 0 (SECTION): R12 => bytes 00 'S' (byte[1]
        // != 0); R13+ => bytes 00 00. So a non-zero second byte selects the
        // 1-byte reader.
        if (static_cast<unsigned char>(line[23]) != 0)
            reader = std::make_unique<dxfReaderBinaryR12>(&filestr);
        else
            reader = std::make_unique<dxfReaderBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        binFile = false;
        filestr.open (fileName.c_str(), std::ios_base::in);
        reader = std::make_unique<dxfReaderAscii>(&filestr);
    }

    bool isOk {processDxf()};
    filestr.close();
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}

bool dxfRW::readAscii(DRW_Interface *interface_, bool ext, std::string& content) {
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    applyExt = ext;
    iface = interface_;
    std::istringstream strstream(content);
    reader = std::make_unique<dxfReaderAscii>(&strstream);
    bool isOk {processDxf()};
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}

int dxfRW::getBlockRecordHandleToWrite(const std::string& blockName) const {
    auto it = blockMap.find(blockName);
    return (it != blockMap.end()) ? it->second : -1;
}

int dxfRW::getTextStyleHandle(const std::string& styleName) const {
    if (!styleName.empty()) {
        std::string upper = styleName;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        auto it = textStyleMap.find(upper);
        if (it != textStyleMap.end()) return it->second;
    }
    return -1;
}

bool dxfRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin){
    bool isOk = false;
    std::ofstream filestr;
    if (interface_ == nullptr)
        return false;
    version = ver;
    binFile = bin;
    iface = interface_;
    if (binFile) {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
        if (!filestr.is_open() || !filestr.good())
            return false;
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
        writer = std::make_unique<dxfWriterBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::trunc);
        if (!filestr.is_open() || !filestr.good())
            return false;
        writer = std::make_unique<dxfWriterAscii>(&filestr);
        std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
        if (!writer->writeString(999, comm))
            return false;
    }
    DRW_Header header;
    iface->writeHeader(header);
    writer->writeString(0, "SECTION");
    //Reserve the codec's fixed structural code-5 literals (table heads, mandatory
    //records, BLOCK_RECORDs, *Model/*Paper BLOCK+ENDBLK, root dict C / ACAD_GROUP
    //D) so the minted-handle stream (m_handleAllocator.next()) skips them. Any raw
    //code-5 handle preserved by the filter was already reserve()d before write(),
    //so a re-emitted raw OBJECT/ENTITY can collide with neither a minted handle
    //nor a fixed-low structural handle. The first next() yields FIRSTHANDLE (0x30)
    //exactly as the legacy ++entCount did, keeping a fresh write byte-identical.
    seedReservedDxf();
    header.write(writer, version);
    writer->writeString(0, "ENDSEC");
    if (ver > DRW::AC1009) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "CLASSES");
        //Emit a CLASS record for each custom (non-fixed) object class actually
        //present in the output. Without these, AutoCAD/ODA silently drop the
        //corresponding OBJECTS instances (the entry and instance must co-exist).
        //The filter registers them from the raw-net objects before write().
        for (DRW_Class &cls : m_dxfClasses) {
            cls.write(writer.get(), version);
        }
        writer->writeString(0, "ENDSEC");
    }
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writeTables();
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "BLOCKS");
    writeBlocks();
    writer->writeString(0, "ENDSEC");

    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    iface->writeEntities();
    writer->writeString(0, "ENDSEC");

    if (version > DRW::AC1009) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "OBJECTS");
        writeObjects();
        writer->writeString(0, "ENDSEC");
    }
    if (!writer->writeString(0, "EOF")) {
        writer.reset();
        return false;
    }
    // Back-patch $HANDSEED with the final handle high-water mark. The header was
    // streamed first (before any table/block/entity/object handle was minted),
    // so it wrote a fixed-width placeholder and recorded the value-field offset.
    // Now that the whole body is written, m_handleAllocator.current() is one past
    // the largest handle reserved or minted, i.e. strictly above every emitted
    // code-5 handle — exactly what a $HANDSEED needs to be.
    if (header.m_handseedValueOffset != std::streampos(-1)) {
        std::uint32_t seed = highWaterHandle();
        char buf[DRW_Header::kHandseedFieldWidth + 1];
        snprintf(buf, sizeof(buf), "%0*X",
                 DRW_Header::kHandseedFieldWidth, seed);
        std::streampos resume = filestr.tellp();
        filestr.seekp(header.m_handseedValueOffset);
        filestr.write(buf, DRW_Header::kHandseedFieldWidth);
        filestr.seekp(resume);
    }
    filestr.flush();
    isOk = filestr.good();
    filestr.close();
    writer.reset();
    return isOk;
}

void dxfRW::seedReservedDxf() {
    // Fixed structural code-5 handles the codec writes as literals (see
    // writeTables/writeBlocks/writeBlockRecord/writeObjects). These DIFFER from
    // the DWG seedReserved() set — they are the DXF codec's own canonical values
    // and stay verbatim. Reserving them up front lets m_handleAllocator.next()
    // skip them while preserving FIRSTHANDLE (0x30) as the first minted handle.
    static const std::uint32_t fixed[] = {
        0x1,   // BLOCK_RECORD table head
        0x2,   // LAYER table head
        0x3,   // STYLE table head
        0x5,   // LTYPE table head
        0x6,   // VIEW table head
        0x7,   // UCS table head
        0x8,   // VPORT table head
        0x9,   // APPID table head
        0xA,   // DIMSTYLE table head
        0xC,   // NamedObjectsDictionary (root dict)
        0xD,   // ACAD_GROUP dictionary
        0x10,  // LAYER "0"
        0x12,  // APPID "ACAD"
        0x14,  // LTYPE "ByBlock"
        0x15,  // LTYPE "ByLayer"
        0x16,  // LTYPE "Continuous"
        0x1C,  // BLOCK "*Paper_Space"
        0x1D,  // ENDBLK "*Paper_Space"
        0x1E,  // BLOCK_RECORD "*Paper_Space"
        0x1F,  // BLOCK_RECORD "*Model_Space"
        0x20,  // BLOCK "*Model_Space"
        0x21,  // ENDBLK "*Model_Space"
    };
    for (std::uint32_t h : fixed)
        m_handleAllocator.reserve(h);
}

bool dxfRW::writeEntity(DRW_Entity *ent, bool captureSourceHandle) {
    // On entry, ent->handle is a SOURCE-handle key seeded by the filter from
    // RS_Entity::sourceHandle() (getEntityAttributes); it is read by NOTHING
    // before this unconditional mint and is captured here for GROUP 340
    // resolution (F3). Any future pre-mint read of ent->handle is a latent bug.
    const std::uint32_t sourceHandle = ent->handle;
    ent->handle = m_handleAllocator.next();  // unconditional mint (unchanged)
    if (captureSourceHandle && sourceHandle != 0) {
        // emplace (NOT operator[]): keeps the FIRST-seen source->minted mapping.
        // captureSourceHandle is false on the VERTEX/SEQEND parent re-entries from
        // writePolyline/writeInsert, which call writeEntity(ent) AGAIN on the SAME
        // parent whose handle was already minted -- so sourceHandle there is a
        // stale MINTED handle (>= FIRSTHANDLE), not a real source. Recording those
        // would POLLUTE the map: a real source handle (also commonly >= FIRSTHANDLE)
        // can numerically equal a stale minted key, and emplace keeping the
        // first-seen would then SHADOW the genuine mapping -> GROUP 340 (resolved
        // via sourceHandleToMintedMap) would mis-point or drop a member. Gating on
        // the call SITE (not the handle value) is correct because a real source
        // handle is indistinguishable from a minted one by value alone.
        m_writingContext.sourceHandleToMintedMap.emplace(sourceHandle, ent->handle);
    }
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    // R2000+ DXF requires a code-330 owner handle (soft-pointer to the owning
    // BLOCK_RECORD) on every entity. Without it ezdxf/AutoCAD treat the entity
    // as an orphan and emit a recover/audit warning. Resolution priority:
    //   1) ent->parentHandle when explicitly seeded (e.g. raw-replay paths);
    //   2) the active BLOCK_RECORD (currHandle) while writingBlock is true --
    //      writeBlock() latches it for every user block in the BLOCKS section;
    //   3) the fixed Model_Space (0x1F) / Paper_Space (0x1E) BLOCK_RECORD
    //      handles by ent->space, for entities in the ENTITIES section.
    if (version > DRW::AC1014) {
        std::uint32_t ownerHandle = ent->parentHandle;
        if (ownerHandle == 0) {
            if (writingBlock)
                ownerHandle = static_cast<std::uint32_t>(currHandle);
            else
                ownerHandle = (ent->space == DRW::PaperSpace) ? 0x1Eu : 0x1Fu;
        }
        writer->writeString(330, toHexStr(static_cast<int>(ownerHandle)));
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbEntity");
    }
    if (ent->space == 1)
        writer->writeInt16(67, 1);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(8, ent->layer);
        writer->writeUtf8String(6, ent->lineType);
    } else {
        writer->writeUtf8Caps(8, ent->layer);
        writer->writeUtf8Caps(6, ent->lineType);
    }
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        writer->writeInt32(420, ent->color24);
    }
    if (version > DRW::AC1015 && !ent->colorName.empty()) {
        writer->writeUtf8String(430, ent->colorName);
    }
    // linetype scale(48) + visibility(60) — both read by DRW_Entity::parseCode
    // and symmetric on the DWG path; previously dropped on DXF save, so a
    // per-entity linetype scale or an invisible entity was lost on DWG→DXF.
    // 60: 0=visible (default, omitted), 1=invisible. (write-review pass-2 #11)
    if (version > DRW::AC1009 && ent->ltypeScale != 1.0) {
        writer->writeDouble(48, ent->ltypeScale);
    }
    if (version > DRW::AC1009 && !ent->visible) {
        writer->writeInt16(60, 1);
    }
    if (version > DRW::AC1018 && ent->shadow != DRW::CastAndReceieveShadows) {
        writer->writeInt16(284, static_cast<int>(ent->shadow));
    }
    // Material (347) is an R2007+ AcDbEntity field (ezdxf acdb_entity:347 ->
    // DXF2007); emitting it at R2004 is non-conformant.
    if (version > DRW::AC1018 && ent->material != DRW::MaterialByLayer) {
        writer->writeUtf8String(347, toHexStr(static_cast<int>(ent->material)));
    }
    if (version > DRW::AC1014) {
        writer->writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
    }
    // Plot-style handle (390) is likewise R2007+ (ezdxf acdb_entity:390 ->
    // DXF2007).
    if (version > DRW::AC1018 && ent->plotStyle != DRW::DefaultPlotStyle) {
        writer->writeUtf8String(390, toHexStr(ent->plotStyle));
    }
    if (version > DRW::AC1015 && ent->transparency != DRW::Opaque) {
        writer->writeInt32(440, ent->transparency);
    }
    if (version >= DRW::AC1014) {
        writeAppData(ent->appData);
    }
    return true;
}

bool dxfRW::writeAppData(const std::list<std::list<DRW_Variant>>& appData) {
    for(auto group : appData) {
        //Search for application name
        bool found = false;

        for(auto data : group) {
            if(data.code() == 102 && data.type() == DRW_Variant::STRING) {
                writer->writeString(102, "{" + *(data.content.s));
                found = true;
                break;
            }
        }

        if(found) {
            for(auto data : group) {
                if(data.code() == 102) {
                    continue;
                }

                switch(data.type()) {
                    case DRW_Variant::STRING:
                        writer->writeString(data.code(), *(data.content.s));
                        break;

                    case DRW_Variant::INTEGER:
                        writer->writeInt32(data.code(), data.content.i);
                        break;

                    case DRW_Variant::INTEGER64:
                        writer->writeInt64(data.code(), static_cast<std::uint64_t>(data.content.i64));
                        break;

                    case DRW_Variant::DOUBLE:
                        writer->writeDouble(data.code(), data.content.d);
                        break;

                    default:
                        break;
                }
            }

            writer->writeString(102, "}");
        }
    }
    return true;
}

bool dxfRW::writeLineType(DRW_LType *ent){
    std::string strname = ent->name;

    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
//do not write linetypes handled by library
    if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") {
        return true;
    }
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        int handle = static_cast<int>(m_handleAllocator.next());
        writer->writeString(5, toHexStr(handle));
        m_writingContext.lineTypesMap.emplace_back(strname, handle);
        if (version > DRW::AC1012) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeUtf8String(3, ent->desc);
    ent->update();
    writer->writeInt16(72, 65);
    writer->writeInt16(73, ent->size);
    writer->writeDouble(40, ent->length);

    for (unsigned int i = 0;  i< ent->path.size(); i++){
        writer->writeDouble(49, ent->path.at(i));
        if (version > DRW::AC1009) {
            writer->writeInt16(74, 0);
        }
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeLayer(DRW_Layer *ent){
    writer->writeString(0, "LAYER");
    if (!wlayer0 && ent->name == "0") {
        wlayer0 = true;
        if (version > DRW::AC1009) {
            writer->writeString(5, "10");
        }
    } else {
        if (version > DRW::AC1009) {
            writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
        }
    }
    if (version > DRW::AC1012) {
        writer->writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLayerTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        writer->writeInt32(420, ent->color24);
    }
    if (version > DRW::AC1009) {
        writer->writeUtf8String(6, ent->lineType);
        // plot (290), lineweight (370) and plotstyle handle (390) are R2000+
        // LAYER fields (ezdxf acdb_symbol_table_record: all DXF2000); they did
        // not exist in R13/R14, so emitting them there is non-conformant.
        if (version > DRW::AC1014) {
            // Emit the plot flag unconditionally, matching lineweight (370) and
            // plotstyle (390) below and AutoCAD/ezdxf, which always write every
            // R2000+ LAYER field. The previous code wrote 290 only when plotF
            // was false and relied on "absent => true"; that dropped an explicit
            // "290 1" written by a strict external tool on re-save and was
            // inconsistent with the sibling fields. Reading is unaffected:
            // DRW_Layer defaults plotF=true, so legacy files without 290 still
            // load as plot-on.
            writer->writeBool(290, ent->plotF);
            writer->writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
            writer->writeString(390, "F");
        }
    } else
        writer->writeUtf8Caps(6, ent->lineType);
    if (!ent->extData.empty()){
        writeExtData(ent->extData);
    }
//    writer->writeString(347, "10012");
    return true;
}

bool dxfRW::writeTextstyle(DRW_Textstyle *ent){
    writer->writeString(0, "STYLE");
    //stringstream cause crash in OS/X, bug#3597944
    std::string name=ent->name;
    transform(name.begin(), name.end(), name.begin(), toupper);
    if (!dimstyleStd) {
        if (name == "STANDARD"){
            dimstyleStd = true;
        }
    }
    if (version > DRW::AC1009) {
        int handle = static_cast<int>(m_handleAllocator.next());
        writer->writeString(5, toHexStr(handle));
        textStyleMap[name] = handle;
    }

    if (version > DRW::AC1012) {
        writer->writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbTextStyleTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->width);
    writer->writeDouble(50, ent->oblique);
    writer->writeInt16(71, ent->genFlag);
    writer->writeDouble(42, ent->lastHeight);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(3, ent->font);
        writer->writeUtf8String(4, ent->bigFont);
        if (ent->fontFamily != 0)
            writer->writeInt32(1071, ent->fontFamily);
    } else {
        writer->writeUtf8Caps(3, ent->font);
        writer->writeUtf8Caps(4, ent->bigFont);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeVport(DRW_Vport *ent){
    if (!dimstyleStd) {
        ent->name = "*ACTIVE";
        dimstyleStd = true;
    }
    writer->writeString(0, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
        if (version > DRW::AC1012)
            writer->writeString(330, "2");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewportTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(10, ent->lowerLeft.x);
    writer->writeDouble(20, ent->lowerLeft.y);
    writer->writeDouble(11, ent->UpperRight.x);
    writer->writeDouble(21, ent->UpperRight.y);
    writer->writeDouble(12, ent->center.x);
    writer->writeDouble(22, ent->center.y);
    writer->writeDouble(13, ent->snapBase.x);
    writer->writeDouble(23, ent->snapBase.y);
    writer->writeDouble(14, ent->snapSpacing.x);
    writer->writeDouble(24, ent->snapSpacing.y);
    writer->writeDouble(15, ent->gridSpacing.x);
    writer->writeDouble(25, ent->gridSpacing.y);
    writer->writeDouble(16, ent->viewDir.x);
    writer->writeDouble(26, ent->viewDir.y);
    writer->writeDouble(36, ent->viewDir.z);
    writer->writeDouble(17, ent->viewTarget.x);
    writer->writeDouble(27, ent->viewTarget.y);
    writer->writeDouble(37, ent->viewTarget.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->ratio);
    writer->writeDouble(42, ent->lensHeight);
    writer->writeDouble(43, ent->frontClip);
    writer->writeDouble(44, ent->backClip);
    writer->writeDouble(50, ent->snapAngle);
    writer->writeDouble(51, ent->twistAngle);
    writer->writeInt16(71, ent->viewMode);
    writer->writeInt16(72, ent->circleZoom);
    writer->writeInt16(73, ent->fastZoom);
    writer->writeInt16(74, ent->ucsIcon);
    writer->writeInt16(75, ent->snap);
    writer->writeInt16(76, ent->grid);
    writer->writeInt16(77, ent->snapStyle);
    writer->writeInt16(78, ent->snapIsopair);
    if (version > DRW::AC1014) {
        writer->writeInt16(281, 0);
        writer->writeInt16(65, 1);
        writer->writeDouble(110, 0.0);
        writer->writeDouble(120, 0.0);
        writer->writeDouble(130, 0.0);
        writer->writeDouble(111, 1.0);
        writer->writeDouble(121, 0.0);
        writer->writeDouble(131, 0.0);
        writer->writeDouble(112, 0.0);
        writer->writeDouble(122, 1.0);
        writer->writeDouble(132, 0.0);
        writer->writeInt16(79, 0);
        writer->writeDouble(146, 0.0);
        if (version > DRW::AC1018) {
            writer->writeString(348, "10020");
            writer->writeInt16(60, ent->gridBehavior);//v2007 undocummented see DRW_Vport class
            writer->writeInt16(61, 5);
            writer->writeBool(292, 1);
            writer->writeInt16(282, 1);
            writer->writeDouble(141, 0.0);
            writer->writeDouble(142, 0.0);
            writer->writeInt16(63, 250);
            writer->writeInt32(421, 3358443);
        }
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeDimstyle(DRW_Dimstyle *ent){
    writer->writeString(0, "DIMSTYLE");
    if (!dimstyleStd) {
        std::string name = ent->name;
        std::transform(name.begin(), name.end(), name.begin(),::toupper);
        if (name == "STANDARD")
            dimstyleStd = true;
    }
    if (version > DRW::AC1009) {
        writer->writeString(105, toHexStr(static_cast<int>(m_handleAllocator.next())));
    }

    if (version > DRW::AC1012) {
        writer->writeString(330, "A");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbDimStyleTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    if ( version == DRW::AC1009 || !(ent->dimpost.empty()) )
        writer->writeUtf8String(3, ent->dimpost);
    if ( version == DRW::AC1009 || !(ent->dimapost.empty()) )
        writer->writeUtf8String(4, ent->dimapost);
    if ( version == DRW::AC1009 || !(ent->dimblk.empty()) )
        writer->writeUtf8String(5, ent->dimblk);
    if ( version == DRW::AC1009 || !(ent->dimblk1.empty()) )
        writer->writeUtf8String(6, ent->dimblk1);
    if ( version == DRW::AC1009 || !(ent->dimblk2.empty()) )
        writer->writeUtf8String(7, ent->dimblk2);
    // De-dup: the prepareDRWDimStyle* helpers populate ent->vars (via d.add)
    // with the real modified values; the POD members below keep reset() defaults.
    // Emitting both produced DUPLICATE DXF group codes (default + real) that ezdxf
    // flags. Route the POD writes through guards that skip any code already in
    // ent->vars, so the vars loop below emits the single real value. (P3 #6)
    std::set<int> dimVarCodes;
    for (auto& kv : ent->vars)
        if (kv.second) dimVarCodes.insert(kv.second->code());
    auto wD = [&](int code, double val) {
        if (!dimVarCodes.count(code)) writer->writeDouble(code, val); };
    auto wI = [&](int code, int val) {
        if (!dimVarCodes.count(code)) writer->writeInt16(code, val); };
    wD(40, ent->dimscale);
    wD(41, ent->dimasz);
    wD(42, ent->dimexo);
    wD(43, ent->dimdli);
    wD(44, ent->dimexe);
    wD(45, ent->dimrnd);
    wD(46, ent->dimdle);
    wD(47, ent->dimtp);
    wD(48, ent->dimtm);
    if ( version > DRW::AC1018 || ent->dimfxl !=0 )
        wD(49, ent->dimfxl);
    wD(140, ent->dimtxt);
    wD(141, ent->dimcen);
    wD(142, ent->dimtsz);
    wD(143, ent->dimaltf);
    wD(144, ent->dimlfac);
    wD(145, ent->dimtvp);
    wD(146, ent->dimtfac);
    wD(147, ent->dimgap);
    if (version > DRW::AC1014) {
        wD(148, ent->dimaltrnd);
    }
    wI(71, ent->dimtol);
    wI(72, ent->dimlim);
    wI(73, ent->dimtih);
    wI(74, ent->dimtoh);
    wI(75, ent->dimse1);
    wI(76, ent->dimse2);
    wI(77, ent->dimtad);
    wI(78, ent->dimzin);
    if (version > DRW::AC1014) {
        wI(79, ent->dimazin);
    }
    wI(170, ent->dimalt);
    wI(171, ent->dimaltd);
    wI(172, ent->dimtofl);
    wI(173, ent->dimsah);
    wI(174, ent->dimtix);
    wI(175, ent->dimsoxd);
    wI(176, ent->dimclrd);
    wI(177, ent->dimclre);
    wI(178, ent->dimclrt);
    if (version > DRW::AC1014) {
        wI(179, ent->dimadec);
    }
    if (version > DRW::AC1009) {
        if (version < DRW::AC1015)
            wI(270, ent->dimunit);
        wI(271, ent->dimdec);
        wI(272, ent->dimtdec);
        wI(273, ent->dimaltu);
        wI(274, ent->dimalttd);
        wI(275, ent->dimaunit);
    }
    if (version > DRW::AC1014) {
        wI(276, ent->dimfrac);
        wI(277, ent->dimlunit);
        wI(278, ent->dimdsep);
        wI(279, ent->dimtmove);
    }
    if (version > DRW::AC1009) {
        wI(280, ent->dimjust);
        wI(281, ent->dimsd1);
        wI(282, ent->dimsd2);
        wI(283, ent->dimtolj);
        wI(284, ent->dimtzin);
        wI(285, ent->dimaltz);
        wI(286, ent->dimaltttz);
        if (version < DRW::AC1015)
            wI(287, ent->dimfit);
        wI(288, ent->dimupt);
    }
    if (version > DRW::AC1014) {
        wI(289, ent->dimatfit);
    }
    if ( version > DRW::AC1018 && ent->dimfxlon !=0 )
        wI(290, ent->dimfxlon);
    if (version > DRW::AC1009) {
        std::string txstyname = ent->dimtxsty;
        std::transform(txstyname.begin(), txstyname.end(), txstyname.begin(),::toupper);
        if(textStyleMap.count(txstyname) > 0) {
            int txstyHandle = (*(textStyleMap.find(txstyname))).second;
            writer->writeUtf8String(340, toHexStr(txstyHandle));
        }
    }
    if (version > DRW::AC1014) {
        // 341 (DIMLDRBLK handle) is only emitted when the leader block exists,
        // but DIMLWD (371) / DIMLWE (372) are unconditional R2000+ dimstyle
        // fields — they were wrongly dropped when dimldrblk was empty/absent.
        if(blockMap.count(ent->dimldrblk) > 0) {
            int blkHandle = (*(blockMap.find(ent->dimldrblk))).second;
            writer->writeUtf8String(341, toHexStr(blkHandle));
        }
        wI(371, ent->dimlwd);
        wI(372, ent->dimlwe);
    }
    for (auto& kv : ent->vars) {
        DRW_Variant* v = kv.second;
        switch (v->type()) {
            case DRW_Variant::STRING:  writer->writeUtf8String(v->code(), v->c_str()); break;
            case DRW_Variant::INTEGER: writer->writeInt16(v->code(), v->i_val()); break;
            case DRW_Variant::INTEGER64: writer->writeInt64(v->code(), static_cast<std::uint64_t>(v->i64_val())); break;
            case DRW_Variant::DOUBLE:  writer->writeDouble(v->code(), v->d_val()); break;
            default: break;
        }
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeView(DRW_View *ent){
    writer->writeString(0, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
        if (version > DRW::AC1012)
            writer->writeString(330, "6");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(40, ent->size.y);
    writer->writeDouble(10, ent->center.x);
    writer->writeDouble(20, ent->center.y);
    writer->writeDouble(41, ent->size.x);
    writer->writeDouble(11, ent->viewDirectionFromTarget.x);
    writer->writeDouble(21, ent->viewDirectionFromTarget.y);
    writer->writeDouble(31, ent->viewDirectionFromTarget.z);
    writer->writeDouble(12, ent->targetPoint.x);
    writer->writeDouble(22, ent->targetPoint.y);
    writer->writeDouble(32, ent->targetPoint.z);
    writer->writeDouble(42, ent->lensLen);
    writer->writeDouble(43, ent->frontClippingPlaneOffset);
    writer->writeDouble(44, ent->backClippingPlaneOffset);
    writer->writeDouble(50, ent->twistAngle);
    writer->writeInt16(71, ent->viewMode);
    if (version > DRW::AC1009) {
        writer->writeInt16(281, static_cast<int>(ent->renderMode));
        writer->writeBool(72, ent->hasUCS);
        writer->writeBool(73, ent->cameraPlottable);
        if (ent->hasUCS) {
            writer->writeDouble(110, ent->ucsOrigin.x);
            writer->writeDouble(120, ent->ucsOrigin.y);
            writer->writeDouble(130, ent->ucsOrigin.z);
            writer->writeDouble(111, ent->ucsXAxis.x);
            writer->writeDouble(121, ent->ucsXAxis.y);
            writer->writeDouble(131, ent->ucsXAxis.z);
            writer->writeDouble(112, ent->ucsYAxis.x);
            writer->writeDouble(122, ent->ucsYAxis.y);
            writer->writeDouble(132, ent->ucsYAxis.z);
            writer->writeInt16(79, ent->ucsOrthoType);
            writer->writeDouble(146, ent->ucsElevation);
        }
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeUCS(DRW_UCS *ent){
    writer->writeString(0, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
        if (version > DRW::AC1012)
            writer->writeString(330, "7");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbUCSTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(10, ent->origin.x);
    writer->writeDouble(20, ent->origin.y);
    writer->writeDouble(30, ent->origin.z);
    writer->writeDouble(11, ent->xAxisDirection.x);
    writer->writeDouble(21, ent->xAxisDirection.y);
    writer->writeDouble(31, ent->xAxisDirection.z);
    writer->writeDouble(12, ent->yAxisDirection.x);
    writer->writeDouble(22, ent->yAxisDirection.y);
    writer->writeDouble(32, ent->yAxisDirection.z);
    writer->writeInt16(79, 0);
    writer->writeDouble(146, ent->elevation);
    writer->writeDouble(13, ent->orthoOrigin.x);
    writer->writeDouble(23, ent->orthoOrigin.y);
    writer->writeDouble(33, ent->orthoOrigin.z);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeAppId(DRW_AppId *ent){
    std::string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
//do not write mandatory ACAD appId, handled by library
    if (strname == "ACAD")
        return true;
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeInt16(70, ent->flags);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writePoint(DRW_Point *ent) {
    writer->writeString(0, "POINT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbPoint");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    if (ent->xAxisAngle != 0.0)
        writer->writeDouble(50, ent->xAxisAngle * ARAD);  // radians -> DXF degrees (rad * 180/pi)
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent) {
    writer->writeString(0, "LINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbLine");
    }
    // thickness(39) — reader + DWG encoder both preserve it; omitting it
    // flattened thick 2.5D profiles on DXF save.
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    } else {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
    }
    // extrusion(210/220/230) — default 0,0,1; reader consumes it. Omitting it
    // re-imported out-of-plane lines in the WCS XY plane.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeRay(DRW_Ray *ent) {
    writer->writeString(0, "RAY");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbRay");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeXline(DRW_Xline *ent) {
    writer->writeString(0, "XLINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbXline");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeCircle(DRW_Circle *ent) {
    writer->writeString(0, "CIRCLE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    // Extrusion (AcDbCircle subclass) — default 0,0,1. Omitting it flattened
    // non-Z-up circles on DXF export; the reader (DRW_Point::parseCode) already
    // consumes 210/220/230, so this completes the round trip.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeArc(DRW_Arc *ent) {
    writer->writeString(0, "ARC");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    // thickness(39) belongs to the AcDbCircle subclass (precedes the AcDbArc
    // marker); reader + DWG encoder preserve it.
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    // Extrusion belongs to the AcDbCircle subclass, so it must precede the
    // AcDbArc marker. Default 0,0,1; reader consumes 210/220/230.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbArc");
    }
    writer->writeDouble(50, ent->staangle*ARAD);
    writer->writeDouble(51, ent->endangle*ARAD);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent){
    //verify axis/ratio and params for full ellipse
    ent->correctAxis();
    if (version > DRW::AC1009) {
        writer->writeString(0, "ELLIPSE");
        writeEntity(ent);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbEllipse");
        }
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(40, ent->ratio);
        writer->writeDouble(41, ent->staparam);
        writer->writeDouble(42, ent->endparam);
        // extrusion(210/220/230) — a tilted-plane ellipse loses its orientation
        // (and partial arcs can flip sweep) without it; reader + applyExtrusion
        // depend on it. Default 0,0,1.
        DRW_Coord crd = ent->extPoint;
        if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
            writer->writeDouble(210, crd.x);
            writer->writeDouble(220, crd.y);
            writer->writeDouble(230, crd.z);
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else {
        DRW_Polyline pol;
        //RLZ: copy properties
        ent->toPolyline(&pol, elParts);
        writePolyline(&pol);
    }
    return true;
}

bool dxfRW::writeTrace(DRW_Trace *ent){
    writer->writeString(0, "TRACE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    if (ent->thickness != 0.0)
        writer->writeDouble(39, ent->thickness);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeSolid(DRW_Solid *ent){
    writer->writeString(0, "SOLID");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    if (ent->thickness != 0.0)
        writer->writeDouble(39, ent->thickness);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::write3dface(DRW_3Dface *ent){
    writer->writeString(0, "3DFACE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbFace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    writer->writeInt16(70, ent->invisibleflag);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LWPOLYLINE");
        writeEntity(ent);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbPolyline");
        }
        ent->vertexnum = ent->vertlist.size();
        writer->writeInt32(90, ent->vertexnum);
        writer->writeInt16(70, ent->flags);
        writer->writeDouble(43, ent->width);
        if (ent->elevation != 0)
            writer->writeDouble(38, ent->elevation);
        if (ent->thickness != 0)
            writer->writeDouble(39, ent->thickness);
        for (int i = 0;  i< ent->vertexnum; i++){
            auto& v = ent->vertlist.at(i);
            writer->writeDouble(10, v->x);
            writer->writeDouble(20, v->y);
            if (v->stawidth != 0)
                writer->writeDouble(40, v->stawidth);
            if (v->endwidth != 0)
                writer->writeDouble(41, v->endwidth);
            if (v->bulge != 0)
                writer->writeDouble(42, v->bulge);
            if (version > DRW::AC1021 && v->identifier != 0)
                writer->writeInt32(91, v->identifier);
        }
        // extrusion(210/220/230) — UCS/extruded plines flatten to WCS without it
        // (reader reads it, DWG encoder preserves it). Default 0,0,1.
        DRW_Coord crd = ent->extPoint;
        if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
            writer->writeDouble(210, crd.x);
            writer->writeDouble(220, crd.y);
            writer->writeDouble(230, crd.z);
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else {
        //RLZ: TODO convert lwpolyline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writePolyline(DRW_Polyline *ent) {
    writer->writeString(0, "POLYLINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        if (ent->flags & 8 || ent->flags & 16)
            writer->writeString(100, "AcDb3dPolyline");
        else
            writer->writeString(100, "AcDb2dPolyline");
    } else
        writer->writeInt16(66, 1);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeInt16(70, ent->flags);
    if (ent->defstawidth != 0) {
        writer->writeDouble(40, ent->defstawidth);
    }
    if (ent->defendwidth != 0) {
        writer->writeDouble(41, ent->defendwidth);
    }
    if (ent->flags & 16 || ent->flags & 32) {
        writer->writeInt16(71, ent->vertexcount);
        writer->writeInt16(72, ent->facecount);
    }
    if (ent->smoothM != 0) {
        writer->writeInt16(73, ent->smoothM);
    }
    if (ent->smoothN != 0) {
        writer->writeInt16(74, ent->smoothN);
    }
    if (ent->curvetype != 0) {
        writer->writeInt16(75, ent->curvetype);
    }
    DRW_Coord crd  = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);

    int vertexnum = ent->vertlist.size();
    for (int i = 0;  i< vertexnum; i++){
        DRW_Vertex *v = ent->vertlist.at(i).get();
        writer->writeString(0, "VERTEX");
        writeEntity(ent, /*captureSourceHandle=*/false);  // parent re-entry: do not pollute the map
        if (version > DRW::AC1009) {
            // R2000+ requires a type-specific second subclass marker after
            // AcDbVertex (a face record uses ONLY AcDbFaceRecord). Mirrors
            // ezdxf polyline.py vertex classification; without it AutoCAD/ezdxf
            // mis-type 3D/mesh/polyface vertices.
            if ((v->flags & 128) && (v->flags & 64)) {
                writer->writeString(100, "AcDbFaceRecord");
            } else {
                writer->writeString(100, "AcDbVertex");
                if (v->flags & 128)
                    writer->writeString(100, "AcDbPolyFaceMeshVertex");
                else if (ent->flags & 16)
                    writer->writeString(100, "AcDbPolyFaceMeshVertex");
                else if (ent->flags & 32)
                    writer->writeString(100, "AcDbPolygonMeshVertex");
                else if (ent->flags & 8)
                    writer->writeString(100, "AcDb3dPolylineVertex");
                else
                    writer->writeString(100, "AcDb2dVertex");
            }
        }
        if ( (v->flags & 128) && !(v->flags & 64) ) {
            writer->writeDouble(10, 0);
            writer->writeDouble(20, 0);
            writer->writeDouble(30, 0);
        } else {
            writer->writeDouble(10, v->basePoint.x);
            writer->writeDouble(20, v->basePoint.y);
            writer->writeDouble(30, v->basePoint.z);
        }
        if (v->stawidth != 0)
            writer->writeDouble(40, v->stawidth);
        if (v->endwidth != 0)
            writer->writeDouble(41, v->endwidth);
        if (v->bulge != 0)
            writer->writeDouble(42, v->bulge);
        if (v->flags != 0) {
            writer->writeInt16(70, v->flags);
        }
        if (v->flags & 2) {
            writer->writeDouble(50, v->tgdir);
        }
        if ( v->flags & 128 ) {
            if (v->vindex1 != 0) {
                writer->writeInt16(71, v->vindex1);
            }
            if (v->vindex2 != 0) {
                writer->writeInt16(72, v->vindex2);
            }
            if (v->vindex3 != 0) {
                writer->writeInt16(73, v->vindex3);
            }
            if (v->vindex4 != 0) {
                writer->writeInt16(74, v->vindex4);
            }
            if ( !(v->flags & 64) ) {
                writer->writeInt32(91, v->identifier);
            }
        }
    }
    writer->writeString(0, "SEQEND");
    writeEntity(ent, /*captureSourceHandle=*/false);  // parent re-entry: do not pollute the map
    return true;
}

bool dxfRW::writeSpline(DRW_Spline *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "SPLINE");
        writeEntity(ent);
        writer->writeString(100, "AcDbSpline");
        writeDxfSplineBody(writer.get(), ent);
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else {
        //RLZ: TODO convert spline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeHelix(DRW_Helix *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "HELIX");
        writeEntity(ent);
        writer->writeString(100, "AcDbSpline");
        writeDxfSplineBody(writer.get(), ent);
        writer->writeString(100, "AcDbHelix");
        writer->writeInt32(90, ent->m_majorVersion);
        writer->writeInt32(91, ent->m_maintVersion);
        writer->writeDouble(10, ent->axisBasePt.x);
        writer->writeDouble(20, ent->axisBasePt.y);
        writer->writeDouble(30, ent->axisBasePt.z);
        writer->writeDouble(11, ent->startPt.x);
        writer->writeDouble(21, ent->startPt.y);
        writer->writeDouble(31, ent->startPt.z);
        writer->writeDouble(12, ent->axisVector.x);
        writer->writeDouble(22, ent->axisVector.y);
        writer->writeDouble(32, ent->axisVector.z);
        writer->writeDouble(40, ent->radius);
        writer->writeDouble(41, ent->turns);
        writer->writeDouble(42, ent->turnHeight);
        writer->writeBool(290, ent->handedness);
        writer->writeInt16(280, static_cast<int>(ent->constraintType));
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    }
    return true;
}

bool dxfRW::writeHatch(DRW_Hatch *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "HATCH");
        writeEntity(ent);
        writer->writeString(100, "AcDbHatch");
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(70, ent->solid);
        writer->writeInt16(71, ent->associative);
        ent->loopsnum = ent->looplist.size();
        writer->writeInt16(91, ent->loopsnum);
        //write paths data
        for (int i = 0;  i< ent->loopsnum; i++){
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writer->writeInt16(92, loop->type);
            if ( (loop->type & 2) == 2){
                // Polyline boundary path
                DRW_LWPolyline *pl = nullptr;
                if (!loop->objlist.empty())
                    pl = dynamic_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
                const bool hasBulge = pl && std::any_of(
                    pl->vertlist.begin(), pl->vertlist.end(),
                    [](const std::shared_ptr<DRW_Vertex2D>& v){ return v && v->bulge != 0.0; });
                writer->writeInt16(72, hasBulge ? 1 : 0);
                writer->writeInt16(73, pl ? (pl->flags & 1) : 0); // is-closed
                const int nv = pl ? static_cast<int>(pl->vertlist.size()) : 0;
                writer->writeInt16(93, nv);
                for (int v = 0; v < nv; ++v) {
                    const auto &vtx = pl->vertlist.at(v);
                    writer->writeDouble(10, vtx->x);
                    writer->writeDouble(20, vtx->y);
                    if (hasBulge)
                        writer->writeDouble(42, vtx->bulge);
                }
                // Emit source boundary handles (associative hatch) or 0.
                if (!loop->m_boundaryHandles.empty()) {
                    writer->writeInt16(97, static_cast<int>(loop->m_boundaryHandles.size()));
                    for (std::uint32_t h : loop->m_boundaryHandles)
                        writer->writeString(330, toHexStr(static_cast<int>(h)));
                } else {
                    writer->writeInt16(97, 0);
                }
            } else {
                //boundary path
                loop->update();
                writer->writeInt16(93, loop->numedges);
                for (int j = 0; j<loop->numedges; ++j) {
                    switch ( (loop->objlist.at(j))->eType) {
                    case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j).get();
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                        break; }
                    case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j).get();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle*ARAD);
                        writer->writeDouble(51, a->endangle*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::ELLIPSE: {
                        writer->writeInt16(72, 3);
                        DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j).get();
                        a->correctAxis();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(11, a->secPoint.x);
                        writer->writeDouble(21, a->secPoint.y);
                        writer->writeDouble(40, a->ratio);
                        writer->writeDouble(50, a->staparam*ARAD);
                        writer->writeDouble(51, a->endparam*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::SPLINE: {
                        writer->writeInt16(72, 4);
                        DRW_Spline* sp = (DRW_Spline*)loop->objlist.at(j).get();
                        writer->writeInt32(94, sp->degree);
                        const bool rational = (sp->flags & 0x4) != 0;
                        const bool periodic = (sp->flags & 0x2) != 0;
                        writer->writeInt16(73, rational ? 1 : 0);
                        writer->writeInt16(74, periodic ? 1 : 0);
                        writer->writeInt32(95, static_cast<int>(sp->knotslist.size()));
                        writer->writeInt32(96, static_cast<int>(sp->controllist.size()));
                        for (double k : sp->knotslist) {
                            writer->writeDouble(40, k);
                        }
                        for (size_t k = 0; k < sp->controllist.size(); ++k) {
                            const auto& cp = sp->controllist[k];
                            if (!cp) continue;
                            writer->writeDouble(10, cp->x);
                            writer->writeDouble(20, cp->y);
                            if (rational) {
                                double w = (k < sp->weightlist.size()) ? sp->weightlist[k] : 1.0;
                                writer->writeDouble(42, w);
                            }
                        }
                        writer->writeInt32(97, static_cast<int>(sp->fitlist.size()));
                        for (const auto& fp : sp->fitlist) {
                            if (!fp) continue;
                            writer->writeDouble(11, fp->x);
                            writer->writeDouble(21, fp->y);
                        }
                        // start/end tangents (codes 12/22, 13/23)
                        if (sp->tgStart.x != 0.0 || sp->tgStart.y != 0.0) {
                            writer->writeDouble(12, sp->tgStart.x);
                            writer->writeDouble(22, sp->tgStart.y);
                        }
                        if (sp->tgEnd.x != 0.0 || sp->tgEnd.y != 0.0) {
                            writer->writeDouble(13, sp->tgEnd.x);
                            writer->writeDouble(23, sp->tgEnd.y);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
                // Emit source boundary handles (associative hatch) or 0.
                if (!loop->m_boundaryHandles.empty()) {
                    writer->writeInt16(97, static_cast<int>(loop->m_boundaryHandles.size()));
                    for (std::uint32_t h : loop->m_boundaryHandles)
                        writer->writeString(330, toHexStr(static_cast<int>(h)));
                } else {
                    writer->writeInt16(97, 0);
                }
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        if (!ent->solid){
            writer->writeDouble(52, ent->angle);
            writer->writeDouble(41, ent->scale);
            writer->writeInt16(77, ent->doubleflag);
        }
        // code 78 (def-line count) is written for both solid and pattern fills
        const int nDefLines = static_cast<int>(ent->patternLines.size());
        writer->writeInt16(78, nDefLines);
        for (const DRW_Hatch::PatternLine &pl : ent->patternLines) {
            writer->writeDouble(53, pl.angle);
            writer->writeDouble(43, pl.baseX);
            writer->writeDouble(44, pl.baseY);
            writer->writeDouble(45, pl.offsetX);
            writer->writeDouble(46, pl.offsetY);
            writer->writeInt16(79, static_cast<int>(pl.dashList.size()));
            for (double d : pl.dashList)
                writer->writeDouble(49, d);
        }
        if (ent->pixelSize != 0.0)
            writer->writeDouble(47, ent->pixelSize);
        // Seed points (group 98 = count, then 10/20 pairs).
        const int seedCount = static_cast<int>(ent->seedPoints.size());
        writer->writeInt32(98, seedCount);
        for (const DRW_Coord &pt : ent->seedPoints) {
            writer->writeDouble(10, pt.x);
            writer->writeDouble(20, pt.y);
        }
        // Gradient block (R2004+ DXF; codes 450-470 + 463/421/63 per stop).
        if (ent->isGradient) {
            writer->writeInt32(450, ent->isGradient);
            writer->writeInt32(451, ent->gradReserved);
            writer->writeDouble(460, ent->gradAngle);
            writer->writeDouble(461, ent->gradShift);
            writer->writeInt32(452, ent->singleColor);
            writer->writeDouble(462, ent->gradTint);
            writer->writeInt32(453, static_cast<int>(ent->gradColors.size()));
            for (const DRW_Hatch::GradientStop &stop : ent->gradColors) {
                writer->writeDouble(463, stop.value);
                if (stop.aciColor != 0)
                    writer->writeInt16(63, stop.aciColor);
                if (stop.rgb >= 0)
                    writer->writeInt32(421, stop.rgb);
                if (stop.colorMethod != 0)
                    writer->writeInt32(431, stop.colorMethod);
                if (!stop.colorName.empty())
                    writer->writeUtf8String(432, stop.colorName);
                if (!stop.colorBookName.empty())
                    writer->writeUtf8String(433, stop.colorBookName);
            }
            writer->writeUtf8String(470, ent->gradName);
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else {
        //RLZ: TODO verify in acad12
    }
    return true;
}

bool dxfRW::writeMPolygon(DRW_MPolygon *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "MPOLYGON");
        writeEntity(ent);
        writer->writeString(100, "AcDbMPolygon");
        writer->writeInt16(70, ent->solid);
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(71, ent->associative);
        ent->loopsnum = ent->looplist.size();
        writer->writeInt16(91, ent->loopsnum);
        for (int i = 0; i < ent->loopsnum; ++i) {
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writer->writeInt16(92, loop->type);
            if ((loop->type & 2) == 2) {
                DRW_LWPolyline *pl = nullptr;
                if (!loop->objlist.empty())
                    pl = dynamic_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
                const bool hasBulge = pl && std::any_of(
                    pl->vertlist.begin(), pl->vertlist.end(),
                    [](const std::shared_ptr<DRW_Vertex2D>& v){ return v && v->bulge != 0.0; });
                writer->writeInt16(72, hasBulge ? 1 : 0);
                writer->writeInt16(73, pl ? (pl->flags & 1) : 0);
                const int nv = pl ? static_cast<int>(pl->vertlist.size()) : 0;
                writer->writeInt16(93, nv);
                for (int v = 0; v < nv; ++v) {
                    const auto &vtx = pl->vertlist.at(v);
                    writer->writeDouble(10, vtx->x);
                    writer->writeDouble(20, vtx->y);
                    if (hasBulge)
                        writer->writeDouble(42, vtx->bulge);
                }
                writer->writeInt16(97, static_cast<int>(loop->m_boundaryHandles.size()));
                for (std::uint32_t h : loop->m_boundaryHandles)
                    writer->writeString(330, toHexStr(static_cast<int>(h)));
            } else {
                loop->update();
                writer->writeInt16(93, loop->numedges);
                for (int j = 0; j < loop->numedges; ++j) {
                    switch ((loop->objlist.at(j))->eType) {
                    case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j).get();
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                        break; }
                    case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j).get();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle*ARAD);
                        writer->writeDouble(51, a->endangle*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::ELLIPSE: {
                        writer->writeInt16(72, 3);
                        DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j).get();
                        a->correctAxis();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(11, a->secPoint.x);
                        writer->writeDouble(21, a->secPoint.y);
                        writer->writeDouble(40, a->ratio);
                        writer->writeDouble(50, a->staparam*ARAD);
                        writer->writeDouble(51, a->endparam*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::SPLINE: {
                        writer->writeInt16(72, 4);
                        DRW_Spline* sp = (DRW_Spline*)loop->objlist.at(j).get();
                        writer->writeInt32(94, sp->degree);
                        const bool rational = (sp->flags & 0x4) != 0;
                        const bool periodic = (sp->flags & 0x2) != 0;
                        writer->writeInt16(73, rational ? 1 : 0);
                        writer->writeInt16(74, periodic ? 1 : 0);
                        writer->writeInt32(95, static_cast<int>(sp->knotslist.size()));
                        writer->writeInt32(96, static_cast<int>(sp->controllist.size()));
                        for (double k : sp->knotslist)
                            writer->writeDouble(40, k);
                        for (size_t k = 0; k < sp->controllist.size(); ++k) {
                            const auto& cp = sp->controllist[k];
                            if (!cp) continue;
                            writer->writeDouble(10, cp->x);
                            writer->writeDouble(20, cp->y);
                            if (rational) {
                                double w = (k < sp->weightlist.size()) ? sp->weightlist[k] : 1.0;
                                writer->writeDouble(42, w);
                            }
                        }
                        writer->writeInt32(97, static_cast<int>(sp->fitlist.size()));
                        for (const auto& fp : sp->fitlist) {
                            if (!fp) continue;
                            writer->writeDouble(11, fp->x);
                            writer->writeDouble(21, fp->y);
                        }
                        if (sp->tgStart.x != 0.0 || sp->tgStart.y != 0.0) {
                            writer->writeDouble(12, sp->tgStart.x);
                            writer->writeDouble(22, sp->tgStart.y);
                        }
                        if (sp->tgEnd.x != 0.0 || sp->tgEnd.y != 0.0) {
                            writer->writeDouble(13, sp->tgEnd.x);
                            writer->writeDouble(23, sp->tgEnd.y);
                        }
                        break; }
                    default:
                        break;
                    }
                }
                writer->writeInt16(97, static_cast<int>(loop->m_boundaryHandles.size()));
                for (std::uint32_t h : loop->m_boundaryHandles)
                    writer->writeString(330, toHexStr(static_cast<int>(h)));
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        if (!ent->solid) {
            writer->writeDouble(52, ent->angle);
            writer->writeDouble(41, ent->scale);
            writer->writeInt16(77, ent->doubleflag);
        }
        const int nDefLines = static_cast<int>(ent->patternLines.size());
        writer->writeInt16(78, nDefLines);
        for (const DRW_Hatch::PatternLine &pl : ent->patternLines) {
            writer->writeDouble(53, pl.angle);
            writer->writeDouble(43, pl.baseX);
            writer->writeDouble(44, pl.baseY);
            writer->writeDouble(45, pl.offsetX);
            writer->writeDouble(46, pl.offsetY);
            writer->writeInt16(79, static_cast<int>(pl.dashList.size()));
            for (double d : pl.dashList)
                writer->writeDouble(49, d);
        }
        if (ent->fillColorAci != 0)
            writer->writeInt16(63, ent->fillColorAci);
        if (ent->fillColorRgb >= 0)
            writer->writeInt32(421, ent->fillColorRgb);
        if (!ent->fillColorName.empty())
            writer->writeUtf8String(430, ent->fillColorName);
        writer->writeDouble(11, ent->xDirX);
        writer->writeDouble(21, ent->xDirY);
        writer->writeInt32(99, ent->degenerateLoops);
        if (ent->isGradient) {
            writer->writeInt32(450, ent->isGradient);
            writer->writeInt32(451, ent->gradReserved);
            writer->writeDouble(460, ent->gradAngle);
            writer->writeDouble(461, ent->gradShift);
            writer->writeInt32(452, ent->singleColor);
            writer->writeDouble(462, ent->gradTint);
            writer->writeInt32(453, static_cast<int>(ent->gradColors.size()));
            for (const DRW_Hatch::GradientStop &stop : ent->gradColors) {
                writer->writeDouble(463, stop.value);
                if (stop.aciColor != 0)
                    writer->writeInt16(63, stop.aciColor);
                if (stop.rgb >= 0)
                    writer->writeInt32(421, stop.rgb);
                if (stop.colorMethod != 0)
                    writer->writeInt32(431, stop.colorMethod);
                if (!stop.colorName.empty())
                    writer->writeUtf8String(432, stop.colorName);
                if (!stop.colorBookName.empty())
                    writer->writeUtf8String(433, stop.colorBookName);
            }
            writer->writeUtf8String(470, ent->gradName);
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    }
    return true;
}

bool dxfRW::writeLeader(DRW_Leader *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LEADER");
        writeEntity(ent);
        writer->writeString(100, "AcDbLeader");
        writer->writeUtf8String(3, ent->style);
        writer->writeInt16(71, ent->arrow);
        writer->writeInt16(72, ent->leadertype);
        writer->writeInt16(73, ent->flag);
        writer->writeInt16(74, ent->hookline);
        writer->writeInt16(75, ent->hookflag);
        writer->writeDouble(40, ent->textheight);
        writer->writeDouble(41, ent->textwidth);
        writer->writeInt16(76, static_cast<int>(ent->vertexlist.size()));
        for (unsigned int i=0; i<ent->vertexlist.size(); i++) {
            auto vert = ent->vertexlist.at(i);
            writer->writeDouble(10, vert->x);
            writer->writeDouble(20, vert->y);
            writer->writeDouble(30, vert->z);
        }
        // block_color (77): color used when the leader annotation color is
        // BYBLOCK; ezdxf acdb_leader emits it unconditionally (default 7).
        writer->writeInt16(77, ent->coloruse);
        // annotation_handle (340): hard ref to the associated annotation,
        // resolved through the source->minted map; skip when unresolved so we
        // never emit a dangling handle (mirrors the GROUP-340 policy).
        if (ent->annotHandle != 0) {
            auto it = m_writingContext.sourceHandleToMintedMap.find(ent->annotHandle);
            if (it != m_writingContext.sourceHandleToMintedMap.end())
                writer->writeString(340, toHexStr(static_cast<int>(it->second)));
        }
        if (ent->extrusionPoint.x != 0.0 || ent->extrusionPoint.y != 0.0 ||
            ent->extrusionPoint.z != 1.0) {
            writer->writeDouble(210, ent->extrusionPoint.x);
            writer->writeDouble(220, ent->extrusionPoint.y);
            writer->writeDouble(230, ent->extrusionPoint.z);
        }
        // horizontal_direction (211), offset-from-block (212), offset-from-
        // annotation (213) — emitted after 210 per ezdxf order, when non-default.
        if (ent->horizdir.x != 1.0 || ent->horizdir.y != 0.0 || ent->horizdir.z != 0.0) {
            writer->writeDouble(211, ent->horizdir.x);
            writer->writeDouble(221, ent->horizdir.y);
            writer->writeDouble(231, ent->horizdir.z);
        }
        if (ent->offsetblock.x != 0.0 || ent->offsetblock.y != 0.0 || ent->offsetblock.z != 0.0) {
            writer->writeDouble(212, ent->offsetblock.x);
            writer->writeDouble(222, ent->offsetblock.y);
            writer->writeDouble(232, ent->offsetblock.z);
        }
        if (ent->offsettext.x != 0.0 || ent->offsettext.y != 0.0 || ent->offsettext.z != 0.0) {
            writer->writeDouble(213, ent->offsettext.x);
            writer->writeDouble(223, ent->offsettext.y);
            writer->writeDouble(233, ent->offsettext.z);
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}
bool dxfRW::writeArcDimension(DRW_DimArc *d) {
    if (version <= DRW::AC1009)
        return true;
    writer->writeString(0, "ARC_DIMENSION");
    writeEntity(d);
    writer->writeString(100, "AcDbDimension");
    if (version >= DRW::AC1024)
        writer->writeInt16(280, 0);   // AcDbDimension version, 0 = R2010+
    if (!d->getName().empty())
        writer->writeString(2, d->getName());
    writer->writeDouble(10, d->getArcDefPoint().x);
    writer->writeDouble(20, d->getArcDefPoint().y);
    writer->writeDouble(30, d->getArcDefPoint().z);
    writer->writeDouble(11, d->getTextPoint().x);
    writer->writeDouble(21, d->getTextPoint().y);
    writer->writeDouble(31, d->getTextPoint().z);
    // ARC_DIMENSION: subtype 5 in low 3 bits (same as angular3p); preserve high bits
    d->type = (d->type & ~0x07) | 5;
    if (!(d->type & 32)) d->type += 32;
    writer->writeInt16(70, d->type);
    if (!d->getText().empty())
        writer->writeUtf8String(1, d->getText());
    writer->writeInt16(71, d->getAlign());
    if (d->getTextLineStyle() != 1)
        writer->writeInt16(72, d->getTextLineStyle());
    if (d->getTextLineFactor() != 1)
        writer->writeDouble(41, d->getTextLineFactor());
    writer->writeUtf8String(3, d->getStyle());
    if (d->getDir() != 0)
        writer->writeDouble(53, d->getDir());
    writer->writeDouble(210, d->getExtrusion().x);
    writer->writeDouble(220, d->getExtrusion().y);
    writer->writeDouble(230, d->getExtrusion().z);
    writer->writeString(100, "AcDbArcDimension");
    writer->writeDouble(13, d->getExtLine1().x);
    writer->writeDouble(23, d->getExtLine1().y);
    writer->writeDouble(33, d->getExtLine1().z);
    writer->writeDouble(14, d->getExtLine2().x);
    writer->writeDouble(24, d->getExtLine2().y);
    writer->writeDouble(34, d->getExtLine2().z);
    writer->writeDouble(15, d->getArcCenter().x);
    writer->writeDouble(25, d->getArcCenter().y);
    writer->writeDouble(35, d->getArcCenter().z);
    writer->writeInt16(70, d->arcSymbol);
    writer->writeDouble(40, d->arcStartAngle);
    writer->writeDouble(41, d->arcEndAngle);
    writer->writeInt16(71, d->isPartial ? 1 : 0);
    DRW_Coord lp1 = d->hasLeader ? d->getLeaderPt1() : d->getExtLine1();
    DRW_Coord lp2 = d->hasLeader ? d->leaderPt2      : d->getExtLine2();
    writer->writeDouble(16, lp1.x); writer->writeDouble(26, lp1.y); writer->writeDouble(36, lp1.z);
    writer->writeDouble(17, lp2.x); writer->writeDouble(27, lp2.y); writer->writeDouble(37, lp2.z);
    if (!d->extData.empty())
        writeExtData(d->extData);
    return true;
}

bool dxfRW::writeLargeRadialDimension(DRW_DimLargeRadial *d) {
    if (version <= DRW::AC1009)
        return true;
    writer->writeString(0, "LARGE_RADIAL_DIMENSION");
    writeEntity(d);
    writer->writeString(100, "AcDbDimension");
    if (version >= DRW::AC1024)
        writer->writeInt16(280, 0);
    if (!d->getName().empty())
        writer->writeString(2, d->getName());
    writer->writeDouble(10, d->getCenterPoint().x);
    writer->writeDouble(20, d->getCenterPoint().y);
    writer->writeDouble(30, d->getCenterPoint().z);
    writer->writeDouble(11, d->getTextPoint().x);
    writer->writeDouble(21, d->getTextPoint().y);
    writer->writeDouble(31, d->getTextPoint().z);
    int dimType = (d->type & ~0x07) | 4;
    if (!(dimType & 32))
        dimType += 32;
    writer->writeInt16(70, dimType);
    if (!d->getText().empty())
        writer->writeUtf8String(1, d->getText());
    writer->writeInt16(71, d->getAlign());
    if (d->getTextLineStyle() != 1)
        writer->writeInt16(72, d->getTextLineStyle());
    if (d->getTextLineFactor() != 1)
        writer->writeDouble(41, d->getTextLineFactor());
    writer->writeUtf8String(3, d->getStyle());
    if (d->measureValue != 0)
        writer->writeDouble(42, d->measureValue);
    if (d->getDir() != 0)
        writer->writeDouble(53, d->getDir());
    if (d->hdir != 0)
        writer->writeDouble(51, d->hdir);
    if (d->getFlipArrow1())
        writer->writeInt16(74, 1);
    if (d->getFlipArrow2())
        writer->writeInt16(75, 1);
    if (d->genTol)
        writer->writeInt16(76, 1);
    if (d->limGen)
        writer->writeInt16(77, 1);
    if (d->tolPlus != 0)
        writer->writeDouble(43, d->tolPlus);
    if (d->tolMinus != 0)
        writer->writeDouble(44, d->tolMinus);
    if (d->tolScale != 0)
        writer->writeDouble(45, d->tolScale);
    if (d->tolDecimals != 0)
        writer->writeInt16(78, d->tolDecimals);
    if (d->tolAlign != 0)
        writer->writeInt16(79, d->tolAlign);
    if (d->tolZero != 0)
        writer->writeInt16(80, d->tolZero);
    if (d->altTolDecimals != 0)
        writer->writeInt16(81, d->altTolDecimals);
    if (d->altZero != 0)
        writer->writeInt16(82, d->altZero);
    if (d->altTolZero != 0)
        writer->writeInt16(83, d->altTolZero);
    if (d->textMove != 0)
        writer->writeInt16(84, d->textMove);
    writer->writeDouble(210, d->getExtrusion().x);
    writer->writeDouble(220, d->getExtrusion().y);
    writer->writeDouble(230, d->getExtrusion().z);
    writer->writeString(100, "AcDbRadialDimensionLarge");
    writer->writeDouble(13, d->getChordPoint().x);
    writer->writeDouble(23, d->getChordPoint().y);
    writer->writeDouble(33, d->getChordPoint().z);
    writer->writeDouble(14, d->overrideCenterPoint.x);
    writer->writeDouble(24, d->overrideCenterPoint.y);
    writer->writeDouble(34, d->overrideCenterPoint.z);
    writer->writeDouble(15, d->jogPoint.x);
    writer->writeDouble(25, d->jogPoint.y);
    writer->writeDouble(35, d->jogPoint.z);
    writer->writeDouble(40, d->jogAngle);
    if (!d->extData.empty())
        writeExtData(d->extData);
    return true;
}

bool dxfRW::writeDimension(DRW_Dimension *ent) {
    if (ent->eType == DRW::DIMARC)
        return writeArcDimension(static_cast<DRW_DimArc*>(ent));
    if (auto *largeRadial = dynamic_cast<DRW_DimLargeRadial*>(ent))
        return writeLargeRadialDimension(largeRadial);
    if (version > DRW::AC1009) {
        writer->writeString(0, "DIMENSION");
        writeEntity(ent);
        writer->writeString(100, "AcDbDimension");
        if (version >= DRW::AC1024)
            writer->writeInt16(280, 0);   // AcDbDimension version, 0 = R2010+
        if (!ent->getName().empty()){
            writer->writeString(2, ent->getName());
        }
        writer->writeDouble(10, ent->getDefPoint().x);
        writer->writeDouble(20, ent->getDefPoint().y);
        writer->writeDouble(30, ent->getDefPoint().z);
        writer->writeDouble(11, ent->getTextPoint().x);
        writer->writeDouble(21, ent->getTextPoint().y);
        writer->writeDouble(31, ent->getTextPoint().z);
        if ( !(ent->type & 32))
            ent->type = ent->type +32;
        writer->writeInt16(70, ent->type);
        if ( !(ent->getText().empty()) )
            writer->writeUtf8String(1, ent->getText());
        writer->writeInt16(71, ent->getAlign());
        if ( ent->getTextLineStyle() != 1)
            writer->writeInt16(72, ent->getTextLineStyle());
        if ( ent->getTextLineFactor() != 1)
            writer->writeDouble(41, ent->getTextLineFactor());
        writer->writeUtf8String(3, ent->getStyle());
        // Measurement(42), horizontal direction(51) and flip-arrow flags(74/75)
        // — the DXF reader consumes all four (drw_entities.cpp parseCode) and the
        // DWG encoder writes hdir/flipArrow1/flipArrow2; emit them for symmetry.
        if (ent->measureValue != 0)
            writer->writeDouble(42, ent->measureValue);
        if (ent->getDir() != 0)
            writer->writeDouble(53, ent->getDir());
        if (ent->hdir != 0)
            writer->writeDouble(51, ent->hdir);
        if (ent->getFlipArrow1())
            writer->writeInt16(74, 1);
        if (ent->getFlipArrow2())
            writer->writeInt16(75, 1);
        if (ent->genTol)
            writer->writeInt16(76, 1);
        if (ent->limGen)
            writer->writeInt16(77, 1);
        if (ent->tolPlus != 0)
            writer->writeDouble(43, ent->tolPlus);
        if (ent->tolMinus != 0)
            writer->writeDouble(44, ent->tolMinus);
        if (ent->tolScale != 0)
            writer->writeDouble(45, ent->tolScale);
        if (ent->tolDecimals != 0)
            writer->writeInt16(78, ent->tolDecimals);
        if (ent->tolAlign != 0)
            writer->writeInt16(79, ent->tolAlign);
        if (ent->tolZero != 0)
            writer->writeInt16(80, ent->tolZero);
        if (ent->altTolDecimals != 0)
            writer->writeInt16(81, ent->altTolDecimals);
        if (ent->altZero != 0)
            writer->writeInt16(82, ent->altZero);
        if (ent->altTolZero != 0)
            writer->writeInt16(83, ent->altTolZero);
        if (ent->textMove != 0)
            writer->writeInt16(84, ent->textMove);
        writer->writeDouble(210, ent->getExtrusion().x);
        writer->writeDouble(220, ent->getExtrusion().y);
        writer->writeDouble(230, ent->getExtrusion().z);

        switch (ent->eType) {
        case DRW::DIMALIGNED:
        case DRW::DIMLINEAR: {
            DRW_DimAligned * dd = (DRW_DimAligned*)ent;
            writer->writeString(100, "AcDbAlignedDimension");
            DRW_Coord crd = dd->getClonepoint();
            if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
                writer->writeDouble(12, crd.x);
                writer->writeDouble(22, crd.y);
                writer->writeDouble(32, crd.z);
            }
            writer->writeDouble(13, dd->getDef1Point().x);
            writer->writeDouble(23, dd->getDef1Point().y);
            writer->writeDouble(33, dd->getDef1Point().z);
            writer->writeDouble(14, dd->getDef2Point().x);
            writer->writeDouble(24, dd->getDef2Point().y);
            writer->writeDouble(34, dd->getDef2Point().z);
            if (ent->eType == DRW::DIMLINEAR) {
                DRW_DimLinear * dl = (DRW_DimLinear*)ent;
                if (dl->getAngle() != 0)
                    writer->writeDouble(50, dl->getAngle());
                if (dl->getOblique() != 0)
                    writer->writeDouble(52, dl->getOblique());
                writer->writeString(100, "AcDbRotatedDimension");
            }
            break; }
        case DRW::DIMRADIAL: {
            DRW_DimRadial * dd = (DRW_DimRadial*)ent;
            writer->writeString(100, "AcDbRadialDimension");
            writer->writeDouble(15, dd->getDiameterPoint().x);
            writer->writeDouble(25, dd->getDiameterPoint().y);
            writer->writeDouble(35, dd->getDiameterPoint().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMDIAMETRIC: {
            DRW_DimDiametric * dd = (DRW_DimDiametric*)ent;
            writer->writeString(100, "AcDbDiametricDimension");
            writer->writeDouble(15, dd->getDiameter1Point().x);
            writer->writeDouble(25, dd->getDiameter1Point().y);
            writer->writeDouble(35, dd->getDiameter1Point().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMANGULAR: {
            DRW_DimAngular * dd = (DRW_DimAngular*)ent;
            writer->writeString(100, "AcDb2LineAngularDimension");
            writer->writeDouble(13, dd->getFirstLine1().x);
            writer->writeDouble(23, dd->getFirstLine1().y);
            writer->writeDouble(33, dd->getFirstLine1().z);
            writer->writeDouble(14, dd->getFirstLine2().x);
            writer->writeDouble(24, dd->getFirstLine2().y);
            writer->writeDouble(34, dd->getFirstLine2().z);
            writer->writeDouble(15, dd->getSecondLine1().x);
            writer->writeDouble(25, dd->getSecondLine1().y);
            writer->writeDouble(35, dd->getSecondLine1().z);
            writer->writeDouble(16, dd->getDimPoint().x);
            writer->writeDouble(26, dd->getDimPoint().y);
            writer->writeDouble(36, dd->getDimPoint().z);
            break; }
        case DRW::DIMANGULAR3P: {
            DRW_DimAngular3p * dd = (DRW_DimAngular3p*)ent;
            writer->writeString(100, "AcDb3PointAngularDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            writer->writeDouble(15, dd->getVertexPoint().x);
            writer->writeDouble(25, dd->getVertexPoint().y);
            writer->writeDouble(35, dd->getVertexPoint().z);
            break; }
        case DRW::DIMORDINATE: {
            DRW_DimOrdinate * dd = (DRW_DimOrdinate*)ent;
            writer->writeString(100, "AcDbOrdinateDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            break; }
        default:
            break;
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}

bool dxfRW::writeInsert(DRW_Insert *ent){
    const bool hasAttribs = !ent->attlist.empty();
    writer->writeString(0, "INSERT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockReference");
        if (hasAttribs)
            writer->writeInt16(66, 1); //attributes-follow flag
        writer->writeUtf8String(2, ent->name);
    } else {
        if (hasAttribs)
            writer->writeInt16(66, 1);
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(41, ent->xscale);
    writer->writeDouble(42, ent->yscale);
    writer->writeDouble(43, ent->zscale);
    writer->writeDouble(50, (ent->angle)*ARAD); //in dxf angle is writed in degrees
    writer->writeInt16(70, ent->colcount);
    writer->writeInt16(71, ent->rowcount);
    writer->writeDouble(44, ent->colspace);
    writer->writeDouble(45, ent->rowspace);
    // extrusion(210/220/230) — OCS inserts re-import in the wrong plane without
    // it (reader reads it, DWG encoder preserves it). Default 0,0,1.
    if (ent->extPoint.x != 0 || ent->extPoint.y != 0 || ent->extPoint.z != 1) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    //Trailing block attributes + terminating SEQEND (mirrors writePolyline).
    if (hasAttribs) {
        for (const auto &att : ent->attlist) {
            if (att)
                writeAttrib(att.get());
        }
        writer->writeString(0, "SEQEND");
        writeEntity(ent, /*captureSourceHandle=*/false);  // parent re-entry: do not pollute the map
    }
    return true;
}

bool dxfRW::writeTable(DRW_Table *ent){
    writer->writeString(0, "ACAD_TABLE");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockReference");

    const UTF8STRING blockName = ent->name.empty() ? UTF8STRING("*T1") : ent->name;
    writer->writeUtf8String(2, blockName);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->xscale != 1.0)
        writer->writeDouble(41, ent->xscale);
    if (ent->yscale != 1.0)
        writer->writeDouble(42, ent->yscale);
    if (ent->zscale != 1.0)
        writer->writeDouble(43, ent->zscale);
    if (ent->angle != 0.0)
        writer->writeDouble(50, ent->angle * ARAD);
    if (ent->extPoint.x != 0 || ent->extPoint.y != 0 || ent->extPoint.z != 1) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }

    writer->writeString(100, "AcDbTable");
    if (ent->m_tableStyleHandle != 0)
        writer->writeString(342, toHexStr(static_cast<int>(ent->m_tableStyleHandle)));

    DRW_Coord horizontal = ent->m_horizontalDirection;
    if (horizontal.x == 0.0 && horizontal.y == 0.0 && horizontal.z == 0.0)
        horizontal.x = 1.0;
    writer->writeDouble(11, horizontal.x);
    writer->writeDouble(21, horizontal.y);
    writer->writeDouble(31, horizontal.z);

    const std::size_t rows = ent->m_content.m_rows.size();
    const std::size_t columns = tableColumnCount(*ent);
    writer->writeInt32(90, ent->m_valueFlag);
    writer->writeInt32(91, static_cast<int>(rows));
    writer->writeInt32(92, static_cast<int>(columns));
    writer->writeInt32(93, 0);
    writer->writeInt32(94, 0);
    writer->writeInt32(95, 0);
    writer->writeInt32(96, 0);

    for (const auto& row : ent->m_content.m_rows)
        writer->writeDouble(141, row.m_height);
    for (std::size_t column = 0; column < columns; ++column) {
        const double width = column < ent->m_content.m_columns.size()
            ? ent->m_content.m_columns[column].m_width
            : 0.0;
        writer->writeDouble(142, width);
    }

    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            const DRW_TableCell *cell = nullptr;
            if (column < ent->m_content.m_rows[row].m_cells.size())
                cell = &ent->m_content.m_rows[row].m_cells[column];
            const UTF8STRING text = cell == nullptr ? UTF8STRING() : tableCellText(*cell);
            writer->writeInt16(171, 1);
            writer->writeInt16(172, 0);
            writer->writeInt16(173, 0);
            writer->writeInt16(174, 0);
            writer->writeInt16(175, 1);
            writer->writeInt16(176, 1);
            writer->writeInt32(91, 0);
            writer->writeInt16(178, 0);
            writer->writeDouble(145, 0.0);
            writer->writeInt32(92, 0);
            writer->writeUtf8String(301, "CELL_VALUE");
            writer->writeInt32(93, text.empty() ? 3 : 2);
            writer->writeInt32(90, text.empty() ? 0 : 4);
            if (!text.empty())
                writer->writeUtf8String(1, text);
            else
                writer->writeInt32(91, 0);
            writer->writeInt32(94, 0);
            writer->writeUtf8String(300, "");
            writer->writeUtf8String(302, text);
            writer->writeUtf8String(304, "ACVALUE_END");
        }
    }

    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeAttrib(DRW_Attrib *ent){
    if (ent->eType == DRW::ATTDEF) {
        if (auto *attdef = dynamic_cast<DRW_Attdef *>(ent))
            return writeAttdef(attdef);
    }

    writer->writeString(0, "ATTRIB");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbText");
    if (ent->thickness != 0)  // reader + DWG encoder preserve thickness(39)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft)
        writer->writeInt16(72, ent->alignH);
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbAttribute");
    writer->writeUtf8String(2, ent->tag);
    writer->writeInt16(70, ent->attribFlags);
    writer->writeInt16(73, ent->m_fieldLength);
    if (ent->alignV != DRW_Text::VBaseLine)
        writer->writeInt16(74, ent->alignV);
    if (version > DRW::AC1014)
        writer->writeInt16(280, ent->lockPosition ? 1 : 0);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeAttdef(DRW_Attdef *ent){
    writer->writeString(0, "ATTDEF");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbText");
    if (ent->thickness != 0)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft)
        writer->writeInt16(72, ent->alignH);
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbAttributeDefinition");
    writer->writeUtf8String(2, ent->tag);
    writer->writeUtf8String(3, ent->prompt);
    writer->writeInt16(70, ent->attribFlags);
    writer->writeInt16(73, ent->m_fieldLength);
    if (ent->alignV != DRW_Text::VBaseLine)
        writer->writeInt16(74, ent->alignV);
    if (version > DRW::AC1014)
        writer->writeInt16(280, ent->lockPosition ? 1 : 0);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeText(DRW_Text *ent){
    writer->writeString(0, "TEXT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->thickness != 0)  // reader + DWG encoder preserve thickness(39)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft) {
        writer->writeInt16(72, ent->alignH);
    }
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->alignV != DRW_Text::VBaseLine) {
        writer->writeInt16(73, ent->alignV);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

static double arcAlignedDxfValue(const UTF8STRING& value, double fallback) {
    if (value.empty())
        return fallback;
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

bool dxfRW::writeRText(DRW_RText *ent) {
    if (version <= DRW::AC1009)
        return true;
    writer->writeString(0, "RTEXT");
    writeEntity(ent);
    writer->writeString(100, "RText");
    writer->writeUtf8String(7, ent->style.empty() ? "Standard" : ent->style);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(50, ent->angle);
    writer->writeInt32(70, ent->m_rTextFlags);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeArcAlignedText(DRW_ArcAlignedText *ent) {
    if (version <= DRW::AC1009)
        return true;
    writer->writeString(0, "ARCALIGNEDTEXT");
    writeEntity(ent);
    writer->writeString(100, "AcDbArcAlignedText");
    writer->writeUtf8String(1, ent->text);
    if (!ent->m_fontName.empty())
        writer->writeUtf8String(2, ent->m_fontName);
    if (!ent->m_bigFontName.empty())
        writer->writeUtf8String(3, ent->m_bigFontName);
    writer->writeUtf8String(7, ent->style.empty() ? "Standard" : ent->style);
    writer->writeDouble(10, ent->m_center.x);
    writer->writeDouble(20, ent->m_center.y);
    writer->writeDouble(30, ent->m_center.z);
    writer->writeDouble(40, ent->m_radius);
    writer->writeDouble(41, arcAlignedDxfValue(
        ent->m_xScale, ent->widthscale > 0.0 ? ent->widthscale : 1.0));
    writer->writeDouble(42, arcAlignedDxfValue(
        ent->m_textSize, ent->height > 0.0 ? ent->height : 0.0));
    writer->writeDouble(43, arcAlignedDxfValue(ent->m_charSpacing, 1.0));
    writer->writeDouble(44, arcAlignedDxfValue(ent->m_offsetFromArc, 0.0));
    writer->writeDouble(45, arcAlignedDxfValue(ent->m_rightOffset, 0.0));
    writer->writeDouble(46, arcAlignedDxfValue(ent->m_leftOffset, 0.0));
    writer->writeDouble(50, ent->m_startAngle * ARAD);
    writer->writeDouble(51, ent->m_endAngle * ARAD);
    writer->writeInt32(90, ent->m_rawColor);
    writer->writeInt32(77, ent->m_characterSet);
    writer->writeInt32(78, ent->m_pitchAndFamily);
    writer->writeInt32(79, ent->m_isShx);
    writer->writeInt32(74, ent->m_isBold);
    writer->writeInt32(75, ent->m_isItalic);
    writer->writeInt32(76, ent->m_isUnderlined);
    writer->writeInt32(72, ent->m_alignment);
    writer->writeInt32(70, ent->m_isReverse);
    writer->writeInt32(280, ent->m_wizardFlag);
    writer->writeInt32(73, ent->m_textPosition);
    writer->writeInt32(71, ent->m_textDirection);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeTolerance(DRW_Tolerance *ent){
    writer->writeString(0, "TOLERANCE");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbFcf");
    writer->writeUtf8String(3, ent->dimStyleName);
    writer->writeDouble(10, ent->insertionPoint.x);
    writer->writeDouble(20, ent->insertionPoint.y);
    writer->writeDouble(30, ent->insertionPoint.z);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    writer->writeDouble(11, ent->xAxisDirectionVector.x);
    writer->writeDouble(21, ent->xAxisDirectionVector.y);
    writer->writeDouble(31, ent->xAxisDirectionVector.z);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeMLine(DRW_MLine *ent) {
    if (version <= DRW::AC1009) return true;  // MLINE is R13+
    writer->writeString(0, "MLINE");
    writeEntity(ent);
    writer->writeString(100, "AcDbMline");
    writer->writeUtf8String(2, ent->styleName);
    if (ent->styleHandle != 0) {
        writer->writeString(340, toHexStr(static_cast<int>(ent->styleHandle)));
    }
    writer->writeDouble(40, ent->scale);
    writer->writeInt16(70, ent->justification);
    writer->writeInt16(71, ent->openClosed);
    writer->writeInt16(72, static_cast<int>(ent->vertlist.size()));
    writer->writeInt16(73, static_cast<int>(ent->numLines));
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    for (const auto& v : ent->vertlist) {
        writer->writeDouble(11, v.position.x);
        writer->writeDouble(21, v.position.y);
        writer->writeDouble(31, v.position.z);
        writer->writeDouble(12, v.vertexDir.x);
        writer->writeDouble(22, v.vertexDir.y);
        writer->writeDouble(32, v.vertexDir.z);
        writer->writeDouble(13, v.miterDir.x);
        writer->writeDouble(23, v.miterDir.y);
        writer->writeDouble(33, v.miterDir.z);
        for (int li = 0; li < ent->numLines; ++li) {
            const auto& seg = (li < static_cast<int>(v.segParms.size()))
                                  ? v.segParms[li] : std::vector<double>{};
            const auto& fill = (li < static_cast<int>(v.areaFillParms.size()))
                                   ? v.areaFillParms[li] : std::vector<double>{};
            writer->writeInt16(74, static_cast<int>(seg.size()));
            for (double p : seg) writer->writeDouble(41, p);
            writer->writeInt16(75, static_cast<int>(fill.size()));
            for (double p : fill) writer->writeDouble(42, p);
        }
    }
    if (!ent->extData.empty()) {
        writeExtData(ent->extData);
    }
    return true;
}

bool dxfRW::writeUnderlay(DRW_Underlay *ent) {
    if (version <= DRW::AC1009) return true;  // R13+ only
    const char* tag = (ent->kind == DRW_Underlay::DGN) ? "DGNUNDERLAY"
                    : (ent->kind == DRW_Underlay::DWF) ? "DWFUNDERLAY"
                    : "PDFUNDERLAY";
    writer->writeString(0, tag);
    writeEntity(ent);
    writer->writeString(100, "AcDbUnderlayReference");
    if (ent->definitionHandle != 0) {
        writer->writeString(340, toHexStr(static_cast<int>(ent->definitionHandle)));
    }
    writer->writeDouble(10, ent->position.x);
    writer->writeDouble(20, ent->position.y);
    writer->writeDouble(30, ent->position.z);
    if (ent->scale.x != 1.0 || ent->scale.y != 1.0 || ent->scale.z != 1.0) {
        writer->writeDouble(41, ent->scale.x);
        writer->writeDouble(42, ent->scale.y);
        writer->writeDouble(43, ent->scale.z);
    }
    writer->writeDouble(50, ent->rotation);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    writer->writeInt16(280, ent->flags);
    writer->writeInt16(281, ent->contrast);
    writer->writeInt16(282, ent->fade);
    for (const auto& v : ent->clipBoundary) {
        writer->writeDouble(11, v.x);
        writer->writeDouble(21, v.y);
    }
    if (!ent->extData.empty()) {
        writeExtData(ent->extData);
    }
    return true;
}

bool dxfRW::writeUnderlayDefinition(DRW_UnderlayDefinition *ent) {
    if (version <= DRW::AC1009) return true;
    const char* tag = (ent->kind == DRW_UnderlayDefinition::DGN) ? "DGNDEFINITION"
                    : (ent->kind == DRW_UnderlayDefinition::DWF) ? "DWFDEFINITION"
                    : "PDFDEFINITION";
    writer->writeString(0, tag);
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbUnderlayDefinition");
    writer->writeUtf8String(1, ent->filename);
    writer->writeUtf8String(2, ent->sheetName);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeMText(DRW_MText *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "MTEXT");
        writeEntity(ent);
        writer->writeString(100, "AcDbMText");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(40, ent->height);
        writer->writeDouble(41, ent->widthscale);
        writer->writeInt16(71, ent->textgen);
        writer->writeInt16(72, ent->alignH);
        // Chunk on UTF-8 codepoint boundaries so a multi-byte character (or
        // its codepage/DBCS encoding, or a \U+/\M+ escape) is never split
        // across the group-3/group-1 records. The previous code split the
        // post-codec byte string at a fixed 250 bytes, corrupting DBCS pairs
        // and escape sequences. Encoded per-codepoint chunks stay <=250 bytes;
        // for pure-ASCII text the output is byte-identical to the old 250-split.
        const std::string& utf8 = ent->text;
        std::vector<std::string> chunks;
        std::string cur;
        for (std::size_t p = 0; p < utf8.size(); ) {
            unsigned char c = static_cast<unsigned char>(utf8[p]);
            std::size_t cl = (c < 0x80) ? 1 : ((c >> 5) == 0x6) ? 2
                           : ((c >> 4) == 0xE) ? 3 : ((c >> 3) == 0x1E) ? 4 : 1;
            if (p + cl > utf8.size()) cl = utf8.size() - p;
            std::string enc = writer->fromUtf8String(utf8.substr(p, cl));
            if (!cur.empty() && cur.size() + enc.size() > 250) {
                chunks.push_back(cur);
                cur.clear();
            }
            cur += enc;
            p += cl;
        }
        chunks.push_back(cur);  // final (group 1); empty when text is empty
        for (std::size_t k = 0; k + 1 < chunks.size(); ++k)
            writer->writeString(3, chunks[k]);
        writer->writeString(1, chunks.back());
        writer->writeString(7, ent->style);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeDouble(50, ent->angle);
        writer->writeInt16(73, ent->linespacingStyle);  // linespacing style (was: alignV)
        writer->writeDouble(44, ent->interlin);
//RLZ ... 11, 21, 31 needed?
        if (!ent->extData.empty()) {
            writeExtData(ent->extData);
        }
    } else {
        //RLZ: TODO convert mtext in text lines (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeLight(DRW_Light *ent) {
    // AcDbLight is an R2007+ (AC1021+) entity; pre-R2007 DXF has no LIGHT entity,
    // so skip rather than emit something AutoCAD/ezdxf would reject. Lights read
    // from a DWG are carried on LibreCAD's metadata shelf and would otherwise be
    // dropped on DWG->DXF export; this re-emits them (D4 write-path preservation).
    if (version < DRW::AC1021)
        return false;
    writer->writeString(0, "LIGHT");
    writeEntity(ent);
    writer->writeString(100, "AcDbLight");
    writer->writeInt32(90, static_cast<int>(ent->m_classVersion));
    writer->writeUtf8String(1, ent->m_name);
    writer->writeInt16(70, static_cast<int>(ent->m_type));
    writer->writeBool(290, ent->m_status);
    // ACI index in 63; a packed true-color value goes in 421 instead.
    if (ent->m_color < 256)
        writer->writeInt16(63, static_cast<int>(ent->m_color));
    else
        writer->writeInt32(421, static_cast<int>(ent->m_color));
    writer->writeBool(291, ent->m_plotGlyph);
    writer->writeDouble(40, ent->m_intensity);
    writer->writeDouble(10, ent->m_position.x);
    writer->writeDouble(20, ent->m_position.y);
    writer->writeDouble(30, ent->m_position.z);
    writer->writeDouble(11, ent->m_target.x);
    writer->writeDouble(21, ent->m_target.y);
    writer->writeDouble(31, ent->m_target.z);
    writer->writeInt16(72, static_cast<int>(ent->m_attenuationType));
    writer->writeBool(292, ent->m_useAttenuationLimits);
    writer->writeDouble(41, ent->m_attenuationStartLimit);
    writer->writeDouble(42, ent->m_attenuationEndLimit);
    writer->writeDouble(50, ent->m_hotspotAngle);
    writer->writeDouble(51, ent->m_falloffAngle);
    writer->writeBool(293, ent->m_castShadows);
    writer->writeInt16(73, static_cast<int>(ent->m_shadowType));
    writer->writeInt32(91, static_cast<int>(ent->m_shadowMapSize));
    writer->writeInt16(280, static_cast<int>(ent->m_shadowMapSoftness));
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeMesh(DRW_Mesh *ent) {
    if (version <= DRW::AC1009)
        return true;
    writer->writeString(0, "MESH");
    writeEntity(ent);
    writer->writeString(100, "AcDbSubDMesh");
    writer->writeInt16(71, static_cast<int>(ent->version));
    writer->writeInt16(72, ent->blendCrease ? 1 : 0);
    writer->writeInt32(91, ent->subdivisionLevel);
    writer->writeInt32(92, static_cast<int>(ent->vertices.size()));
    for (const DRW_Coord& vertex : ent->vertices) {
        writer->writeDouble(10, vertex.x);
        writer->writeDouble(20, vertex.y);
        writer->writeDouble(30, vertex.z);
    }

    std::int32_t faceStreamCount = 0;
    for (const auto& face : ent->faces)
        faceStreamCount += static_cast<std::int32_t>(face.size() + 1);
    writer->writeInt32(93, faceStreamCount);
    for (const auto& face : ent->faces) {
        writer->writeInt32(90, static_cast<int>(face.size()));
        for (std::int32_t index : face)
            writer->writeInt32(90, index);
    }

    writer->writeInt32(94, static_cast<int>(ent->edges.size()));
    for (const auto& edge : ent->edges) {
        writer->writeInt32(90, edge.first);
        writer->writeInt32(90, edge.second);
    }

    writer->writeInt32(95, static_cast<int>(ent->creases.size()));
    for (double crease : ent->creases)
        writer->writeDouble(140, crease);
    writer->writeInt32(90, 0);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeShape(DRW_Shape *ent) {
    // DXF SHAPE (AcDbShape). The DWG stores only a glyph index; the glyph name
    // lives in the external .shx and is unrecoverable, so group 2 carries the
    // SHAPEFILE/STYLE record name (resolved on read), matching libredwg/ACadSharp.
    // m_rotation/m_oblique are radians (DRW_Shape::parseDwg keeps them un-scaled,
    // unlike DRW_Text) -> convert to DXF degrees.
    writer->writeString(0, "SHAPE");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbShape");
    if (ent->m_thickness != 0.0)
        writer->writeDouble(39, ent->m_thickness);
    writer->writeDouble(10, ent->m_insertionPoint.x);
    writer->writeDouble(20, ent->m_insertionPoint.y);
    writer->writeDouble(30, ent->m_insertionPoint.z);
    writer->writeDouble(40, ent->m_scale);            // size
    if (!ent->m_styleName.empty())
        writer->writeUtf8String(2, ent->m_styleName); // shape (style) name
    writer->writeDouble(50, ent->m_rotation * ARAD);  // radians -> degrees
    if (ent->m_widthFactor != 1.0)
        writer->writeDouble(41, ent->m_widthFactor);
    if (ent->m_oblique != 0.0)
        writer->writeDouble(51, ent->m_oblique * ARAD);
    if (ent->m_extrusion.x != 0.0 || ent->m_extrusion.y != 0.0 || ent->m_extrusion.z != 1.0) {
        writer->writeDouble(210, ent->m_extrusion.x);
        writer->writeDouble(220, ent->m_extrusion.y);
        writer->writeDouble(230, ent->m_extrusion.z);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeOle2Frame(DRW_Ole2Frame *ent) {
    // DXF OLE2FRAME (AcDbOle2Frame). Field order/codes per ACadSharp + dwgread:
    // 70 version, 3 client, 10/11 frame corners, 71 type, 72 mode, 73 (const 3),
    // 90 length, 310 binary (hex chunks), 1 "OLE" trailer. pt1/pt2 were decoded
    // from the OLE payload header on read; the payload is replayed verbatim.
    writer->writeString(0, "OLE2FRAME");
    writeEntity(ent);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbOle2Frame");
    writer->writeInt16(70, static_cast<int>(ent->m_oleVersion));
    writer->writeUtf8String(3, ent->m_oleClient);
    writer->writeDouble(10, ent->m_pt1.x);
    writer->writeDouble(20, ent->m_pt1.y);
    writer->writeDouble(30, ent->m_pt1.z);
    writer->writeDouble(11, ent->m_pt2.x);
    writer->writeDouble(21, ent->m_pt2.y);
    writer->writeDouble(31, ent->m_pt2.z);
    writer->writeInt16(71, static_cast<int>(ent->m_flags));  // OLE object type
    writer->writeInt16(72, static_cast<int>(ent->m_mode));   // tile/paper-space mode
    writer->writeInt16(73, 3);                               // undocumented, always 3
    writer->writeInt32(90, static_cast<int>(ent->m_payloadBytes.size()));
    // group 310: payload as hex, 127 bytes (254 hex chars) per record (AutoCAD/
    // dwgread convention; the binary-DXF writer hex-decodes and re-chunks).
    static const char hexd[] = "0123456789ABCDEF";
    const std::vector<std::uint8_t>& data = ent->m_payloadBytes;
    for (std::size_t off = 0; off < data.size(); off += 127) {
        const std::size_t n = std::min<std::size_t>(127, data.size() - off);
        std::string chunk;
        chunk.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t b = data[off + i];
            chunk.push_back(hexd[b >> 4]);
            chunk.push_back(hexd[b & 0x0F]);
        }
        writer->writeString(310, chunk);
    }
    writer->writeString(1, "OLE");
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeViewport(DRW_Viewport *ent) {
    writer->writeString(0, "VIEWPORT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbViewport");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0)
        writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->pswidth);
    writer->writeDouble(41, ent->psheight);
    writer->writeInt16(68, ent->vpstatus);
    writer->writeInt16(69, ent->vpID);
    writer->writeDouble(12, ent->centerPX);
    writer->writeDouble(22, ent->centerPY);
    if (version > DRW::AC1009) {
        writer->writeDouble(15, ent->gridSpX);
        writer->writeDouble(25, ent->gridSpY);
        writer->writeDouble(46, ent->circleZoom);
        writer->writeInt16(72, ent->majorGridLines);
        writer->writeInt32(90, ent->statusFlags);
        writer->writeUtf8String(1, ent->styleSheet);
        writer->writeInt16(281, ent->renderMode);
        writer->writeInt16(71, ent->ucsAtOrigin ? 1 : 0);
        writer->writeInt16(74, ent->ucsPerViewport ? 1 : 0);
        writer->writeDouble(110, ent->ucsOrigin.x);
        writer->writeDouble(120, ent->ucsOrigin.y);
        writer->writeDouble(130, ent->ucsOrigin.z);
        writer->writeDouble(111, ent->ucsXAxis.x);
        writer->writeDouble(121, ent->ucsXAxis.y);
        writer->writeDouble(131, ent->ucsXAxis.z);
        writer->writeDouble(112, ent->ucsYAxis.x);
        writer->writeDouble(122, ent->ucsYAxis.y);
        writer->writeDouble(132, ent->ucsYAxis.z);
        writer->writeDouble(146, ent->ucsElevation);
        writer->writeInt16(76, ent->ucsOrthographicType);
        writer->writeInt16(148, ent->shadePlotMode);
        writer->writeInt16(292, ent->useDefaultLighting ? 1 : 0);
        writer->writeInt16(282, ent->defaultLightingType);
        writer->writeDouble(451, ent->brightness);
        writer->writeDouble(452, ent->contrast);
        if (ent->ambientColorRgb >= 0)
            writer->writeInt32(421, ent->ambientColorRgb);
        writer->writeInt16(431, ent->ambientColorMethod);
        if (ent->vpHeaderHandle != 0)
            writer->writeString(331, toHexStr(static_cast<int>(ent->vpHeaderHandle)));
        if (ent->clipBoundaryHandle != 0)
            writer->writeString(340, toHexStr(static_cast<int>(ent->clipBoundaryHandle)));
        if (ent->namedUcsHandle != 0)
            writer->writeString(345, toHexStr(static_cast<int>(ent->namedUcsHandle)));
        if (ent->baseUcsHandle != 0)
            writer->writeString(346, toHexStr(static_cast<int>(ent->baseUcsHandle)));
        if (ent->backgroundHandle != 0)
            writer->writeString(347, toHexStr(static_cast<int>(ent->backgroundHandle)));
        if (ent->visualStyleHandle != 0)
            writer->writeString(348, toHexStr(static_cast<int>(ent->visualStyleHandle)));
        if (ent->shadePlotHandle != 0)
            writer->writeString(349, toHexStr(static_cast<int>(ent->shadePlotHandle)));
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

DRW_ImageDef* dxfRW::writeImage(DRW_Image *ent, std::string name){
    if (version > DRW::AC1009) {
        //search if exist imagedef with this mane (image inserted more than 1 time)
        //RLZ: imagedef_reactor seem needed to read in acad
        DRW_ImageDef *id = NULL;
        for (unsigned int i=0; i<imageDef.size(); i++) {
            if (imageDef.at(i)->name == name ) {
                id = imageDef.at(i);
                continue;
            }
        }
        if (id == NULL) {
            id = new DRW_ImageDef();
            imageDef.push_back(id);
            id->handle = m_handleAllocator.next();
        }
        id->name = name;
        std::string idReactor = toHexStr(static_cast<int>(m_handleAllocator.next()));

        writer->writeString(0, "IMAGE");
        writeEntity(ent);
        writer->writeString(100, "AcDbRasterImage");
        writer->writeInt32(90, 0);   // class_version (mandatory; always 0)
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(12, ent->vVector.x);
        writer->writeDouble(22, ent->vVector.y);
        writer->writeDouble(32, ent->vVector.z);
        writer->writeDouble(13, ent->sizeu);
        writer->writeDouble(23, ent->sizev);
        writer->writeString(340, toHexStr(id->handle));
        writer->writeInt16(70, 1);
        writer->writeInt16(280, ent->clip);
        writer->writeInt16(281, ent->brightness);
        writer->writeInt16(282, ent->contrast);
        writer->writeInt16(283, ent->fade);
        writer->writeString(360, idReactor);
        // Clip boundary (ezdxf acdb_raster_image order): type 71, count 91,
        // then 14/24 vertices. A polygonal path (>=3 pts) is type 2; otherwise
        // emit the rectangular default (type 1, two opposite corners in
        // image-pixel coords) so consumers never see count=0.
        if (ent->clipPath.size() >= 3) {
            writer->writeInt16(71, 2);
            writer->writeInt32(91, static_cast<std::int32_t>(ent->clipPath.size()));
            for (const DRW_Coord& v : ent->clipPath) {
                writer->writeDouble(14, v.x);
                writer->writeDouble(24, v.y);
            }
        } else {
            writer->writeInt16(71, 1);
            writer->writeInt32(91, 2);
            writer->writeDouble(14, -0.5);
            writer->writeDouble(24, -0.5);
            writer->writeDouble(14, ent->sizeu - 0.5);
            writer->writeDouble(24, ent->sizev - 0.5);
        }
        if (version >= DRW::AC1024) {
            writer->writeBool(290, ent->clipMode);  // R2010+ clip mode
        }
        if (!ent->extData.empty())
            writeExtData(ent->extData);
        id->reactors[idReactor] = toHexStr(ent->handle);
        return id;
    }
    return NULL; //not exist in acad 12
}

// MULTILEADER DXF write.  Mirrors the entity-level field set captured by
// DRW_MLeader::parseCode.  The CONTEXT_DATA{} block is NOT emitted yet —
// a full faithful round-trip requires walking all roots/leader-lines
// with their control-flow markers (302/304 open, 305/303/301 close);
// follow-up.  For now the entity is written as a recognisable
// AcDbMLeader stub plus its scalar fields; consumers that read it back
// see all the override flags + style fields preserved.
bool dxfRW::writeMultiLeader(DRW_MLeader *ent){
    if (version <= DRW::AC1009) {
        return false;  // not in ACAD R12 / earlier
    }
    writer->writeString(0, "MULTILEADER");
    writeEntity(ent);
    writer->writeString(100, "AcDbMLeader");

    const DRW_MLeaderAnnotContext &ctx = ent->context;
    const bool hasContext =
        ctx.hasTextContents || ctx.hasContentsBlock || !ctx.roots.empty();
    auto writeCoord = [&](int xCode, const DRW_Coord &coord) {
        writer->writeDouble(xCode, coord.x);
        writer->writeDouble(xCode + 10, coord.y);
        writer->writeDouble(xCode + 20, coord.z);
    };
    if (hasContext) {
        writer->writeString(300, "CONTEXT_DATA{");
        writer->writeDouble(40, ctx.overallScale);
        writeCoord(10, ctx.contentBasePoint);
        writer->writeDouble(41, ctx.textHeight);
        writer->writeDouble(140, ctx.arrowHeadSize);
        writer->writeDouble(145, ctx.landingGap);
        writer->writeInt16(174, ctx.styleLeftAttach);
        writer->writeInt16(175, ctx.styleRightAttach);
        writer->writeInt16(176, ctx.textAlignType);
        writer->writeInt16(177, ctx.attachmentType);
        writer->writeBool(290, ctx.hasTextContents);
        if (ctx.hasTextContents) {
            writer->writeUtf8String(304, ctx.textLabel);
            writeCoord(11, ctx.textNormal);
            writeCoord(12, ctx.textLocation);
            writeCoord(13, ctx.textDirection);
            writer->writeDouble(42, ctx.textRotation);
            writer->writeDouble(43, ctx.boundaryWidth);
            writer->writeDouble(44, ctx.boundaryHeight);
            writer->writeDouble(45, ctx.lineSpacingFactor);
            writer->writeInt16(170, ctx.lineSpacingStyle);
            writer->writeInt32(90, ctx.textColor);
            writer->writeInt16(171, ctx.alignment);
            writer->writeInt16(172, ctx.flowDirection);
            writer->writeInt32(91, ctx.bgFillColor);
            writer->writeDouble(141, ctx.bgScaleFactor);
            writer->writeInt32(92, ctx.bgTransparency);
            writer->writeBool(291, ctx.bgFillEnabled);
            writer->writeBool(292, ctx.bgMaskFillOn);
            writer->writeInt16(173, ctx.columnType);
            writer->writeBool(293, ctx.textHeightAuto);
            writer->writeDouble(142, ctx.columnWidth);
            writer->writeDouble(143, ctx.columnGutter);
            writer->writeBool(294, ctx.columnFlowReversed);
            for (double columnSize : ctx.columnSizes)
                writer->writeDouble(144, columnSize);
            writer->writeBool(295, ctx.wordBreak);
        }
        writer->writeBool(296, ctx.hasContentsBlock);
        if (ctx.hasContentsBlock) {
            writeCoord(14, ctx.blockNormal);
            writeCoord(15, ctx.blockLocation);
            writeCoord(16, ctx.blockScale);
            writer->writeDouble(46, ctx.blockRotation);
            writer->writeInt32(93, ctx.blockColor);
        }

        for (const DRW_MLeaderRoot &root : ctx.roots) {
            writer->writeString(302, "LEADER{");
            writer->writeBool(290, root.isContentValid);
            writer->writeBool(291, root.unknown291);
            writeCoord(10, root.connectionPoint);
            writeCoord(11, root.direction);
            writer->writeInt32(90, root.leaderIndex);
            writer->writeDouble(40, root.landingDistance);
            for (const DRW_MLeaderLeaderLine &line : root.leaderLines) {
                writer->writeString(304, "LEADER_LINE{");
                for (const DRW_Coord &point : line.points)
                    writeCoord(10, point);
                writer->writeInt32(90, line.segmentIndex);
                writer->writeInt32(91, line.leaderLineIndex);
                writer->writeInt32(93, line.overrideFlags);
                writer->writeInt16(170, line.leaderType);
                writer->writeInt32(92, line.color);
                writer->writeInt32(171, line.lineWeight);
                writer->writeDouble(40, line.arrowSize);
                writer->writeString(305, "}");
            }
            writer->writeInt16(271, root.attachmentDirection);
            writer->writeString(303, "}");
        }

        writeCoord(110, ctx.basePoint);
        writeCoord(111, ctx.baseDirection);
        writeCoord(112, ctx.baseVertical);
        writer->writeBool(297, ctx.isNormalReversed);
        writer->writeInt16(273, ctx.styleTopAttach);
        writer->writeInt16(272, ctx.styleBottomAttach);
        writer->writeString(301, "}");
    }

    writer->writeInt32(90, ent->overrideFlags);
    writer->writeInt16(170, ent->leaderType);
    writer->writeInt32(91, ent->leaderColor);
    writer->writeInt16(171, ent->leaderLineWeight);
    writer->writeBool(290, ent->landingEnabled);
    writer->writeBool(291, ent->doglegEnabled);
    writer->writeDouble(41, ent->landingDistance);
    writer->writeDouble(42, ent->defaultArrowHeadSize);
    writer->writeInt16(172, ent->styleContentType);
    writer->writeInt16(173, ent->styleLeftAttach);
    writer->writeInt16(95, ent->styleRightAttach);
    writer->writeInt16(174, ent->styleTextAngleType);
    writer->writeInt16(175, ent->unknown175);
    writer->writeInt32(92, ent->styleTextColor);
    writer->writeBool(292, ent->styleTextFrameEnabled);
    writer->writeInt32(93, ent->styleBlockColor);
    writer->writeDouble(43, ent->styleBlockRotation);
    writer->writeInt16(176, ent->styleAttachmentType);
    writer->writeBool(293, ent->isAnnotative);
    writer->writeBool(294, ent->isTextDirectionNegative);
    writer->writeInt16(178, ent->ipeAlign);
    writer->writeInt16(179, ent->justification);
    writer->writeDouble(45, ent->scaleFactor);
    writer->writeInt16(271, ent->attachmentDirection);
    writer->writeInt16(273, ent->styleTopAttach);
    writer->writeInt16(272, ent->styleBottomAttach);
    writer->writeBool(295, ent->leaderExtendedToText);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeWipeout(DRW_Wipeout *ent){
    // WIPEOUT inherits AcDbRasterImage's group codes plus an AcDbWipeout
    // subclass marker carrying the polygon (91 + 14/24) and frame flag (290).
    // No AcDbRasterImageDef is written: WIPEOUT carries no actual raster.
    if (version <= DRW::AC1009) {
        return false; // not in ACAD R12 / earlier
    }
    writer->writeString(0, "WIPEOUT");
    writeEntity(ent);
    writer->writeString(100, "AcDbRasterImage");
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->vVector.x);
    writer->writeDouble(22, ent->vVector.y);
    writer->writeDouble(32, ent->vVector.z);
    writer->writeDouble(13, ent->sizeu);
    writer->writeDouble(23, ent->sizev);
    writer->writeInt16(70, 1);             // image-display flags
    writer->writeInt16(280, ent->clip);    // 1 = clipping enabled
    writer->writeInt16(281, ent->brightness);
    writer->writeInt16(282, ent->contrast);
    writer->writeInt16(283, ent->fade);
    writer->writeString(100, "AcDbWipeout");
    writer->writeInt32(90, 0);             // class version
    // Clip boundary type: honour stored value; WIPEOUT defaults to 2 (polygon).
    writer->writeInt16(71, ent->m_clipBoundaryType != 0 ? ent->m_clipBoundaryType : 2);
    writer->writeInt32(91, static_cast<std::int32_t>(ent->clipPath.size()));
    for (const DRW_Coord& v : ent->clipPath) {
        writer->writeDouble(14, v.x);
        writer->writeDouble(24, v.y);
    }
    // Group 290 is the R2010+ Clip mode (0 = mask outside, 1 = mask inside);
    // this is shared with IMAGE and is NOT a frame-display flag.  WIPEOUTFRAME
    // (whether the polygon outline is drawn) is global, in WIPEOUTVARIABLES.
    writer->writeBool(290, ent->clipMode);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writePointCloud(DRW_PointCloud *ent){
    if (version <= DRW::AC1018) {
        return false;
    }
    writer->writeString(0, "POINTCLOUD");
    writeEntity(ent);
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbPointCloud");
    writer->writeInt32(90, ent->classVersion);
    writer->writeDouble(10, ent->origin.x);
    writer->writeDouble(20, ent->origin.y);
    writer->writeDouble(30, ent->origin.z);
    writer->writeUtf8String(1, ent->savedFilename);
    writer->writeInt32(91, ent->sourceFileCount);
    for (const UTF8STRING& srcFile : ent->sourceFiles) {
        writer->writeUtf8String(300, srcFile);
    }
    writer->writeDouble(11, ent->extentsMin.x);
    writer->writeDouble(21, ent->extentsMin.y);
    writer->writeDouble(31, ent->extentsMin.z);
    writer->writeDouble(12, ent->extentsMax.x);
    writer->writeDouble(22, ent->extentsMax.y);
    writer->writeDouble(32, ent->extentsMax.z);
    writer->writeInt64(92, static_cast<std::int64_t>(ent->pointCount));
    writer->writeUtf8String(2, ent->ucsName);
    writer->writeDouble(13, ent->ucsOrigin.x);
    writer->writeDouble(23, ent->ucsOrigin.y);
    writer->writeDouble(33, ent->ucsOrigin.z);
    writer->writeDouble(14, ent->ucsXDirection.x);
    writer->writeDouble(24, ent->ucsXDirection.y);
    writer->writeDouble(34, ent->ucsXDirection.z);
    writer->writeDouble(15, ent->ucsYDirection.x);
    writer->writeDouble(25, ent->ucsYDirection.y);
    writer->writeDouble(35, ent->ucsYDirection.z);
    writer->writeDouble(16, ent->ucsZDirection.x);
    writer->writeDouble(26, ent->ucsZDirection.y);
    writer->writeDouble(36, ent->ucsZDirection.z);
    writer->writeString(340, toHexStr(static_cast<int>(ent->definitionHandle)));
    writer->writeString(360, toHexStr(static_cast<int>(ent->reactorHandle)));
    writer->writeBool(290, ent->showIntensity);
    writer->writeInt16(280, ent->intensityScheme);
    writer->writeDouble(441, ent->intensityStyle.minIntensity);
    writer->writeDouble(442, ent->intensityStyle.maxIntensity);
    writer->writeDouble(443, ent->intensityStyle.lowThreshold);
    writer->writeDouble(444, ent->intensityStyle.highThreshold);
    writer->writeBool(291, ent->showClipping);
    writer->writeInt32(93, ent->clippingCount);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writePointCloudEx(DRW_PointCloudEx *ent){
    if (version <= DRW::AC1024) {
        return false;
    }
    writer->writeString(0, "POINTCLOUDEX");
    writeEntity(ent);
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbPointCloudEx");
    writer->writeInt32(90, ent->classVersion);
    writer->writeDouble(11, ent->extentsMin.x);
    writer->writeDouble(21, ent->extentsMin.y);
    writer->writeDouble(31, ent->extentsMin.z);
    writer->writeDouble(12, ent->extentsMax.x);
    writer->writeDouble(22, ent->extentsMax.y);
    writer->writeDouble(32, ent->extentsMax.z);
    writer->writeDouble(13, ent->ucsOrigin.x);
    writer->writeDouble(23, ent->ucsOrigin.y);
    writer->writeDouble(33, ent->ucsOrigin.z);
    writer->writeDouble(14, ent->ucsXDirection.x);
    writer->writeDouble(24, ent->ucsXDirection.y);
    writer->writeDouble(34, ent->ucsXDirection.z);
    writer->writeDouble(15, ent->ucsYDirection.x);
    writer->writeDouble(25, ent->ucsYDirection.y);
    writer->writeDouble(35, ent->ucsYDirection.z);
    writer->writeDouble(16, ent->ucsZDirection.x);
    writer->writeDouble(26, ent->ucsZDirection.y);
    writer->writeDouble(36, ent->ucsZDirection.z);
    writer->writeBool(290, ent->isLocked);
    writer->writeString(340, toHexStr(static_cast<int>(ent->definitionHandle)));
    writer->writeString(360, toHexStr(static_cast<int>(ent->reactorHandle)));
    writer->writeUtf8String(1, ent->name);
    writer->writeBool(291, ent->showIntensity);
    writer->writeBool(292, ent->showCropping);
    writer->writeInt32(91, ent->croppingCount);
    writer->writeInt32(92, ent->unknownInt0);
    writer->writeInt32(93, ent->unknownInt1);
    writer->writeInt16(280, ent->stylizationType);
    writer->writeUtf8String(300, ent->intensityColorScheme);
    writer->writeUtf8String(301, ent->currentColorScheme);
    writer->writeUtf8String(302, ent->classificationColorScheme);
    writer->writeDouble(440, ent->elevationMin);
    writer->writeDouble(441, ent->elevationMax);
    writer->writeDouble(442, ent->intensityMin);
    writer->writeDouble(443, ent->intensityMax);
    writer->writeInt16(281, ent->intensityOutOfRangeBehavior);
    writer->writeInt16(282, ent->elevationOutOfRangeBehavior);
    writer->writeBool(293, ent->elevationApplyToFixedRange);
    writer->writeBool(294, ent->intensityAsGradient);
    writer->writeBool(295, ent->elevationAsGradient);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeSurface(DRW_Surface *ent){
    if (version <= DRW::AC1018) {
        return false;
    }
    std::string entType;
    switch (ent->eType) {
    case DRW::PLANESURFACE: entType = "PLANESURFACE"; break;
    case DRW::EXTRUDEDSURFACE: entType = "EXTRUDEDSURFACE"; break;
    case DRW::REVOLVEDSURFACE: entType = "REVOLVEDSURFACE"; break;
    case DRW::SWEPTSURFACE: entType = "SWEPTSURFACE"; break;
    case DRW::LOFTEDSURFACE: entType = "LOFTEDSURFACE"; break;
    case DRW::NURBSURFACE: entType = "NURBSSURFACE"; break;
    default: return false;
    }
    writer->writeString(0, entType);
    writeEntity(ent);
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbSurface");
    writer->writeInt16(70, ent->modelerFormatVersion);
    writer->writeInt16(71, ent->uIsolines);
    writer->writeInt16(72, ent->vIsolines);
    if (!ent->rawAcisData.empty()) {
        writeDxfBinaryChunks(writer.get(), ent->rawAcisData);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeModelerGeometry(DRW_ModelerGeometry *ent) {
    if (version <= DRW::AC1009)
        return false;

    const char *recordName = modelerGeometryDxfName(ent->eType);
    const char *subclassName = modelerGeometryDxfSubclass(ent->eType);
    if (recordName == nullptr || subclassName == nullptr)
        return false;

    writer->writeString(0, recordName);
    writeEntity(ent);
    writer->writeString(100, "AcDbModelerGeometry");
    writer->writeString(100, subclassName);
    writer->writeInt16(70, ent->m_modelerVersion);
    if (ent->m_historyHandle != 0)
        writer->writeString(350, toHexStr(static_cast<int>(ent->m_historyHandle)));
    if (!ent->m_rawBytes.empty()) {
        if (version <= DRW::AC1018 && isTextAcisPayload(ent->m_rawBytes))
            writeDxfTextChunks(writer.get(), ent->m_rawBytes);
        else
            writeDxfBinaryChunks(writer.get(), ent->m_rawBytes);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeBlockRecord(std::string name, int insUnits){
    if (version > DRW::AC1009) {
        writer->writeString(0, "BLOCK_RECORD");
        // Mint the BLOCK_RECORD handle, then reserve the next two for the
        // matching BLOCK (currHandle+1) and ENDBLK (currHandle+2) emitted by
        // writeBlock/writeBlocks, mirroring the legacy "entCount = 2+entCount".
        std::uint32_t blockRecordHandle = m_handleAllocator.next();
        writer->writeString(5, toHexStr(static_cast<int>(blockRecordHandle)));

        blockMap[name] = static_cast<int>(blockRecordHandle);
        m_handleAllocator.reserve(blockRecordHandle + 1);  // BLOCK
        m_handleAllocator.reserve(blockRecordHandle + 2);  // ENDBLK
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeUtf8String(2, name);
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, insUnits);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
    }
    return true;
}

bool dxfRW::writeBlock(DRW_Block *bk){
    if (writingBlock) {
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, toHexStr(currHandle+2));
            if (version > DRW::AC1014) {
                writer->writeString(330, toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbBlockEnd");
        }
    }
    writingBlock = true;
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        currHandle = (*(blockMap.find(bk->name))).second;
        writer->writeString(5, toHexStr(currHandle+1));
        if (version > DRW::AC1014) {
            writer->writeString(330, toHexStr(currHandle));
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeUtf8String(2, bk->name);
    } else
        writer->writeUtf8Caps(2, bk->name);
    writer->writeInt16(70, bk->flags);
    writer->writeDouble(10, bk->basePoint.x);
    writer->writeDouble(20, bk->basePoint.y);
    if (bk->basePoint.z != 0.0) {
        writer->writeDouble(30, bk->basePoint.z);
    }
    if (version > DRW::AC1009)
        writer->writeUtf8String(3, bk->name);
    else
        writer->writeUtf8Caps(3, bk->name);
    if(version >= DRW::AC1014) {
        writeAppData(bk->appData);
    }
    writer->writeString(1, "");

    return true;
}

bool dxfRW::writeTables() {
    writer->writeString(0, "TABLE");
    writer->writeString(2, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, "8");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
/*** VPORT ***/
    dimstyleStd =false;
    iface->writeVports();
    if (!dimstyleStd) {
        DRW_Vport portact;
        portact.name = "*ACTIVE";
        writeVport(&portact);
    }
    writer->writeString(0, "ENDTAB");
/*** LTYPE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "5");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 4); //end table def
//Mandatory linetypes
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "14");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByBlock");
    } else
        writer->writeString(2, "BYBLOCK");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "15");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByLayer");
    } else
        writer->writeString(2, "BYLAYER");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "16");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "Continuous");
    } else {
        writer->writeString(2, "CONTINUOUS");
    }
    writer->writeInt16(70, 0);
    writer->writeString(3, "Solid line");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);
//Application linetypes
    iface->writeLTypes();
    writer->writeString(0, "ENDTAB");
/*** LAYER ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LAYER");
    if (version > DRW::AC1009) {
        writer->writeString(5, "2");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    wlayer0 =false;
    iface->writeLayers();
    if (!wlayer0) {
        DRW_Layer lay0;
        lay0.name = "0";
        writeLayer(&lay0);
    }
    writer->writeString(0, "ENDTAB");
/*** STYLE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "STYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "3");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 3); //end table def
    dimstyleStd =false;
    iface->writeTextstyles();
    if (!dimstyleStd) {
        DRW_Textstyle tsty;
        tsty.name = "Standard";
        writeTextstyle(&tsty);
    }
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, "6");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    iface->writeViews();
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, "7");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    iface->writeUCSs();
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "9");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "12");
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
    }
    writer->writeString(2, "ACAD");
    writer->writeInt16(70, 0);
    iface->writeAppId();
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "A");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    if (version > DRW::AC1014) {
        writer->writeString(100, "AcDbDimStyleTable");
        writer->writeInt16(71, 1); //end table def
    }
    dimstyleStd =false;
    iface->writeDimstyles();
    if (!dimstyleStd) {
        DRW_Dimstyle dsty;
        dsty.name = "Standard";
        writeDimstyle(&dsty);
    }
    writer->writeString(0, "ENDTAB");

    if (version > DRW::AC1009) {
        writer->writeString(0, "TABLE");
        writer->writeString(2, "BLOCK_RECORD");
        writer->writeString(5, "1");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        writer->writeInt16(70, 2); //end table def
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1F");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Model_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1E");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Paper_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
    }
    /* always call writeBlockRecords to iface for prepare unnamed blocks */
    iface->writeBlockRecords();
    if (version > DRW::AC1009) {
        writer->writeString(0, "ENDTAB");
    }
return true;
}

bool dxfRW::writeBlocks() {
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "20");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Model_Space");
    } else
        writer->writeString(2, "$MODEL_SPACE");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009)
        writer->writeString(3, "*Model_Space");
    else
        writer->writeString(3, "$MODEL_SPACE");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "21");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockEnd");

    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1C");
        if (version > DRW::AC1014) {
            // Paper_Space BLOCK (handle 1C) is owned by the Paper_Space
            // BLOCK_RECORD (1E, reserved above), not the nonexistent handle 1B.
            writer->writeString(330, "1E");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Paper_Space");
    } else
        writer->writeString(2, "$PAPER_SPACE");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009)
        writer->writeString(3, "*Paper_Space");
    else
        writer->writeString(3, "$PAPER_SPACE");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1D");
        if (version > DRW::AC1014) {
            // Paper_Space ENDBLK (handle 1D) is owned by the Paper_Space
            // BLOCK_RECORD (1E), not the Model_Space BLOCK_RECORD (1F).
            writer->writeString(330, "1E");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockEnd");
    writingBlock = false;
    iface->writeBlocks();
    if (writingBlock) {
        writingBlock = false;
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, toHexStr(currHandle+2));
//            writer->writeString(5, "1D");
            if (version > DRW::AC1014) {
                writer->writeString(330, toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009)
            writer->writeString(100, "AcDbBlockEnd");
    }
    return true;
}

bool dxfRW::writeObjects() {
    writer->writeString(0, "DICTIONARY");
    std::string imgDictH;
    writer->writeString(5, "C");
    if (version > DRW::AC1014) {
        writer->writeString(330, "0");
    }
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    writer->writeString(3, "ACAD_GROUP");
    writer->writeString(350, "D");
    if (imageDef.size() != 0) {
        writer->writeString(3, "ACAD_IMAGE_DICT");
        imgDictH = toHexStr(static_cast<int>(m_handleAllocator.next()));
        writer->writeString(350, imgDictH);
    }
    //Slice (spine-dicts): re-attach raw-net-routed named dictionaries to the
    //regenerated root NamedObjectsDictionary so they are reachable (not pruned as
    //orphans). The filter populates (name, hex-handle) from the source root dict;
    //each handle matches the verbatim code-5 of a dictionary re-emitted later in
    //this OBJECTS section. ACAD_GROUP / root / C-D collisions are excluded there.
    for (const std::pair<std::string, std::string> &entry : m_rootDictEntries) {
        writer->writeString(3, entry.first);
        writer->writeString(350, entry.second);
    }
    //F3: mint a fresh code-5 handle for each GROUP BEFORE the ACAD_GROUP D dict
    //so the D dict body can list each group as a (name, handle) entry (the GROUP
    //objects themselves are emitted after the named-dict block below, owned by
    //D). Allocator-minted handles skip the reserved/fixed set, so they cannot
    //collide. A group with no resolvable members is still listed/emitted (a valid
    //empty group is harmless); members are filtered at emit time.
    std::vector<std::uint32_t> groupHandles;
    groupHandles.reserve(m_groups.size());
    for (std::size_t i = 0; i < m_groups.size(); ++i)
        groupHandles.push_back(m_handleAllocator.next());
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "D");
    writer->writeString(330, "C");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    for (std::size_t i = 0; i < m_groups.size(); ++i) {
        //Unnamed groups carry a generated "*An" name; named groups carry their
        //real name. The D-dict entry name mirrors the GROUP's name field.
        writer->writeUtf8String(3, m_groups[i].name);
        writer->writeString(350, toHexStr(static_cast<int>(groupHandles[i])));
    }
//write IMAGEDEF_REACTOR
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(0, "IMAGEDEF_REACTOR");
            writer->writeString(5, (*it).first);
            writer->writeString(330, (*it).second);
            writer->writeString(100, "AcDbRasterImageDefReactor");
            writer->writeInt16(90, 2); //version 2=R14 to v2010
            writer->writeString(330, (*it).second);
        }
    }
    if (imageDef.size() != 0) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, imgDictH);
        writer->writeString(330, "C");
        writer->writeString(100, "AcDbDictionary");
        writer->writeInt16(281, 1);
        for (unsigned int i=0; i<imageDef.size(); i++) {
            size_t f1, f2;
            f1 = imageDef.at(i)->name.find_last_of("/\\");
            f2 =imageDef.at(i)->name.find_last_of('.');
            ++f1;
            writer->writeString(3, imageDef.at(i)->name.substr(f1,f2-f1));
            writer->writeString(350, toHexStr(imageDef.at(i)->handle) );
        }
    }
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        writer->writeString(0, "IMAGEDEF");
        writer->writeString(5, toHexStr(id->handle) );
        // Owner 330 = the AcDbRasterImageDef dictionary (imgDictH). A missing
        // owner triggers an INVALID_OWNER_HANDLE audit/repair in ezdxf/AutoCAD.
        // imgDictH is non-empty whenever imageDef is non-empty (set above).
        if (version > DRW::AC1014 && !imgDictH.empty()) {
            writer->writeString(330, imgDictH);
        }
        writer->writeString(102, "{ACAD_REACTORS");
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(330, (*it).first);
        }
        writer->writeString(102, "}");
        writer->writeString(100, "AcDbRasterImageDef");
        writer->writeInt16(90, 0); //version 0=R14 to v2010
        writer->writeUtf8String(1, id->name);
        writer->writeDouble(10, id->u);
        writer->writeDouble(20, id->v);
        writer->writeDouble(11, id->up);
        writer->writeDouble(21, id->vp);
        writer->writeInt16(280, id->loaded);
        writer->writeInt16(281, id->resolution);
    }
    //no more needed imageDef, delete it
    while (!imageDef.empty()) {
       imageDef.pop_back();
    }

    //F4-followup: emit the named-dictionary OBJECTS the filter routed via
    //setNamedDictObjects (DWG->DXF). The root C dict already references these by
    //handle (setRootDictEntries spliced their (name, handle) into C above); this
    //makes them exist as reachable objects with a valid owner, clearing the
    //INVALID_OWNER_HANDLE fixes ezdxf otherwise applies to the dangling 350s.
    for (const DRW_Dictionary &dict : m_namedDictObjects) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, toHexStr(static_cast<int>(dict.handle)));
        writeObjectOwner(dict.parentHandle != 0
                             ? static_cast<std::uint32_t>(dict.parentHandle)
                             : 0);
        writer->writeString(100, "AcDbDictionary");
        // Preserve the full duplicate-record cloning policy (valid values include
        // 0,1,2,3,4,5,11,12,13 — e.g. 12 = keep, sort). parseCode reads the full
        // int (code 281); collapsing nonzero->1 silently rewrote the policy.
        writer->writeInt16(281, dict.cloning);
        for (const DRW_Dictionary::Entry &entry : dict.m_entries) {
            writer->writeUtf8String(3, entry.m_name);
            writer->writeString(350, toHexStr(static_cast<int>(entry.m_handle)));
        }
    }

    //F3: emit each GROUP object, owned by the ACAD_GROUP D dict (which already
    //lists it as a (name, minted-handle) entry above). Member 340 references are
    //resolved through the writeEntity source->minted map; a member whose SOURCE
    //handle was not written (consumed / filtered entity) is SKIPPED — never an
    //emitted dangling 340.
    const auto &srcToMinted = m_writingContext.sourceHandleToMintedMap;
    for (std::size_t i = 0; i < m_groups.size(); ++i) {
        const DRW_Group &grp = m_groups[i];
        writer->writeString(0, "GROUP");
        writer->writeString(5, toHexStr(static_cast<int>(groupHandles[i])));
        writer->writeString(330, "D");
        writer->writeString(100, "AcDbGroup");
        writer->writeUtf8String(300, grp.m_description);
        writer->writeInt16(70, grp.m_isUnnamed ? 1 : 0);
        writer->writeInt16(71, grp.m_selectable ? 1 : 0);
        for (std::uint32_t memberSrc : grp.m_entityHandles) {
            auto it = srcToMinted.find(memberSrc);
            if (it == srcToMinted.end())
                continue;  // member not written -> skip (no dangling 340)
            writer->writeString(340, toHexStr(static_cast<int>(it->second)));
        }
    }

    iface->writeObjects();

    return true;
}

bool dxfRW::writeExtData(
    const std::vector<std::shared_ptr<DRW_Variant>> &ed) {
    // Re-pack as raw pointers so we share the existing implementation. The
    // raw pointers do not own — same lifetime as the shared_ptrs in @p ed.
    std::vector<DRW_Variant*> raw;
    raw.reserve(ed.size());
    for (const auto &sp : ed) {
        if (sp) raw.push_back(sp.get());
    }
    return writeExtData(raw);
}

bool dxfRW::writeExtData(const std::vector<DRW_Variant*> &ed){
    for (std::vector<DRW_Variant*>::const_iterator it=ed.begin(); it!=ed.end(); ++it){
        switch ((*it)->code()) {
        case 1000:
        case 1001:
        case 1002:
        case 1003:
        case 1005:
        {int cc = (*it)->code();
            if ((*it)->type() == DRW_Variant::STRING)
                writer->writeUtf8String(cc, *(*it)->content.s);
//            writer->writeUtf8String((*it)->code, (*it)->content.s);
            break;}
        case 1004:
            // DXF code 1004 is binary chunk data; emitted as a hex-encoded
            // string. Both BINARY (from DWG path) and STRING (from a DXF
            // round-trip that already hex-encoded the bytes) variants are
            // accepted.
            if ((*it)->type() == DRW_Variant::BINARY) {
                const std::vector<std::uint8_t>* bytes = (*it)->binary();
                std::string hex;
                if (bytes != nullptr) {
                    static const char hexDigits[] = "0123456789ABCDEF";
                    hex.reserve(bytes->size() * 2);
                    for (std::uint8_t b : *bytes) {
                        hex.push_back(hexDigits[(b >> 4) & 0xF]);
                        hex.push_back(hexDigits[b & 0xF]);
                    }
                }
                writer->writeUtf8String(1004, hex);
            } else if ((*it)->type() == DRW_Variant::STRING) {
                writer->writeUtf8String(1004, *(*it)->content.s);
            }
            break;
        case 1010:
        case 1011:
        case 1012:
        case 1013:
            if ((*it)->type() == DRW_Variant::COORD) {
                writer->writeDouble((*it)->code(), (*it)->content.v->x);
                writer->writeDouble((*it)->code()+10 , (*it)->content.v->y);
                writer->writeDouble((*it)->code()+20 , (*it)->content.v->z);
            }
            break;
        case 1040:
        case 1041:
        case 1042:
            if ((*it)->type() == DRW_Variant::DOUBLE)
                writer->writeDouble((*it)->code(), (*it)->content.d);
            break;
        case 1070:
            if ((*it)->type() == DRW_Variant::INTEGER)
                writer->writeInt16((*it)->code(), (*it)->content.i);
            break;
        case 1071:
            if ((*it)->type() == DRW_Variant::INTEGER)
                writer->writeInt32((*it)->code(), (*it)->content.i);
            else if ((*it)->type() == DRW_Variant::INTEGER64)
                writer->writeInt32((*it)->code(), static_cast<std::int32_t>((*it)->content.i64));
            break;
        default:
            break;
        }
    }
    return true;
}

/********* Reader Process *********/

bool dxfRW::processDxf() {
    DRW_DBG("dxfRW::processDxf() start processing dxf\n");
    int code {-1};
    bool inSection {false};

    reader->setIgnoreComments( false);
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" code\n");
        /* at this level we should only get:
         999 - Comment
         0 - SECTION or EOF
         2 - section name
         everything else between "2 - section name" and "0 - ENDSEC" is handled in process() methods
        */
        switch (code) {
        case 999: // when DXF was created by libdxfrw, first record is a comment with dxfrw version info
            header.addComment( reader->getString());
            continue;

        case 0:
            // ignore further comments, as libdxfrw doesn't support comments in sections
            reader->setIgnoreComments( true);
            if (!inSection) {
                std::string sectionstr {reader->getString()};

                if ("SECTION" == sectionstr) {
                    DRW_DBG(sectionstr); DRW_DBG(" new section\n");
                    inSection = true;
                    continue;
                }
                if ("EOF" == sectionstr) {
                    return true;  //found EOF terminate
                }
            }
            else {
                // in case SECTION was unknown or not supported
                if ("ENDSEC" == reader->getString()) {
                    inSection = false;
                }
            }
            break;

        case 2:
            if (inSection) {
                bool processed {false};
                std::string sectionname {reader->getString()};

                DRW_DBG(sectionname); DRW_DBG(" process section\n");
                if ("HEADER" == sectionname) {
                    processed = processHeader();
                }
                else if ("CLASSES" == sectionname) {
                    processed = processClasses();
                }
                else if ("TABLES" == sectionname) {
                    processed = processTables();
                }
                else if ("BLOCKS" == sectionname) {
                    processed = processBlocks();
                }
                else if ("ENTITIES" == sectionname) {
                    processed = processEntities(false);
                }
                else if ("OBJECTS" == sectionname) {
                    processed = processObjects();
                }
                else {
                    DRW_DBG("section unknown or not supported\n");
                    continue;
                }

                if (!processed) {
                    DRW_DBG("  failed\n");
                    return setError(DRW::BAD_READ_SECTION);
                }

                inSection = false;
            }
            continue;

        default:
            // landing here means an unknown or not supported SECTION
            inSection = false;
            break;
        }
    }

    if (0 == code && "EOF" == reader->getString()) {
        // in case the final EOF has no newline we end up here!
        // this is caused by filestr->good() which is false for missing newline on EOF
        return true;
    }

    return setError(DRW::BAD_UNKNOWN);
}

/********* Header Section *********/

bool dxfRW::processHeader() {
    DRW_DBG("dxfRW::processHeader\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" processHeader\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
            if (sectionstr == "ENDSEC") {
                iface->addHeader(&header);
                return true;  //found ENDSEC terminate
            }

            DRW_DBG("unexpected 0 code in header!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        if (!header.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_HEADER);
}

/********* Classes Section *********/

bool dxfRW::processClasses() {
    DRW_DBG("dxfRW::processClasses\n");
    int code = 0;
    bool reading = false;
    DRW_Class cls;

    auto finishClass = [&]() -> bool {
        if (!reading)
            return true;
        // AutoCAD legitimately emits internal proxy classes (e.g. DbBEditSession)
        // with an empty code-1 record name; only an empty C++ class name (code 2)
        // is structurally invalid. Rejecting an empty recName aborts the whole read.
        if (cls.className.empty()) {
            DRW_DBG("malformed CLASS record: missing class name\n");
            return false;
        }
        iface->addDxfClass(cls);
        return true;
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" processClasses\n");
        if (code == 0) {
            if (!finishClass())
                return setError(DRW::BAD_CODE_PARSED);
            const std::string sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processClasses\n\n");
            if (sectionstr == "CLASS") {
                reading = true;
                cls = DRW_Class{};
                continue;
            }
            if (sectionstr == "ENDSEC") {
                return true;
            }

            DRW_DBG("unexpected 0 code in classes section!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        if (!reading)
            continue;

        switch (code) {
        case 1:
            cls.recName = reader->getUtf8String();
            break;
        case 2:
            cls.className = reader->getUtf8String();
            break;
        case 3:
            cls.appName = reader->getUtf8String();
            break;
        case 90:
            cls.proxyFlag = reader->getInt32();
            break;
        case 91:
            cls.instanceCount = reader->getInt32();
            break;
        case 280:
            cls.wasaProxyFlag = reader->getInt32();
            break;
        case 281:
            cls.entityFlag = reader->getInt32();
            break;
        default:
            break;
        }
    }

    return setError(DRW::BAD_READ_HEADER);
}

/********* Tables Section *********/

bool dxfRW::processTables() {
    DRW_DBG("dxfRW::processTables\n");
    int code;
    std::string sectionstr;
    bool more = true;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
            if (sectionstr == "TABLE") {
                more = reader->readRec(&code);
                DRW_DBG(code); DRW_DBG("\n");
                if (!more) {
                    return setError(DRW::BAD_READ_TABLES); //wrong dxf file
                }
                if (code == 2) {
                    sectionstr = reader->getString();
                    DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
                //found section, process it
                    if (sectionstr == "LTYPE") {
                        processLType();
                    } else if (sectionstr == "LAYER") {
                        processLayer();
                    } else if (sectionstr == "STYLE") {
                        processTextStyle();
                    } else if (sectionstr == "VPORT") {
                        processVports();
                    } else if (sectionstr == "VIEW") {
                        processView();
                    } else if (sectionstr == "UCS") {
                        processUCS();
                    } else if (sectionstr == "APPID") {
                        processAppId();
                    } else if (sectionstr == "DIMSTYLE") {
                        processDimStyle();
                    } else if (sectionstr == "BLOCK_RECORD") {
                        processBlockRecord();
                    }
                }
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processBlockRecord() {
    DRW_DBG("dxfRW::processBlockRecord\n");
    int code = 0;
    bool reading = false;
    std::uint32_t handle = DRW::NoHandle;
    DRW_ParsingContext::BlockRecordInfo record;

    auto finishRecord = [&]() {
        if (reading && handle != DRW::NoHandle && !record.name.empty()) {
            m_readingContext.blockRecordMap[handle] = record;
        }
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            finishRecord();
            const std::string sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "BLOCK_RECORD") {
                reading = true;
                handle = DRW::NoHandle;
                record = DRW_ParsingContext::BlockRecordInfo{};
            } else if (sectionstr == "ENDTAB") {
                return true;
            } else {
                reading = false;
            }
        } else if (reading) {
            switch (code) {
            case 2:
                record.name = reader->getUtf8String();
                break;
            case 5:
                handle = reader->getHandleString();
                break;
            case 70:
                record.insUnits = reader->getInt32();
                break;
            default:
                break;
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLType() {
    DRW_DBG("dxfRW::processLType\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_LType ltype;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading) {
                ltype.update();
                if (ltype.handle != 0 && !ltype.name.empty()) {
                    m_readingContext.lineTypeNameMap[ltype.handle] = ltype.name;
                }
                iface->addLType(ltype);
            }
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "LTYPE") {
                reading = true;
                ltype.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!ltype.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLayer() {
    DRW_DBG("dxfRW::processLayer\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Layer layer;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addLayer(layer);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "LAYER") {
                reading = true;
                layer.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!layer.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processDimStyle() {
    DRW_DBG("dxfRW::processDimStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Dimstyle dimSty;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading) {
                // Phase 3A.0: populate the vars map from the parsed struct so
                // the LibreCAD createDimStyle consumer (reads $DIM* keys) gets
                // the imported values, not reset() defaults. Copy-free (called
                // on dimSty before it is reset() for the next record).
                dimSty.syncStructToVars();
                iface->addDimStyle(dimSty);
            }
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "DIMSTYLE") {
                reading = true;
                dimSty.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!dimSty.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processTextStyle(){
    DRW_DBG("dxfRW::processTextStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Textstyle TxtSty;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addTextStyle(TxtSty);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "STYLE") {
                reading = true;
                TxtSty.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!TxtSty.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processVports(){
    DRW_DBG("dxfRW::processVports");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Vport vp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addVport(vp);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "VPORT") {
                reading = true;
                vp.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!vp.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processView(){
    DRW_DBG("dxfRW::processView");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_View v;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addView(v);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "VIEW") {
                reading = true;
                v.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;
            }
        } else if (reading) {
            if (!v.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processUCS(){
    DRW_DBG("dxfRW::processUCS");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_UCS u;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addUCS(u);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "UCS") {
                reading = true;
                u.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;
            }
        } else if (reading) {
            if (!u.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processAppId(){
    DRW_DBG("dxfRW::processAppId");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_AppId vp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addAppId(vp);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "APPID") {
                reading = true;
                vp.reset();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!vp.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

/********* Block Section *********/

bool dxfRW::processBlocks() {
    DRW_DBG("dxfRW::processBlocks\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "BLOCK") {
                if (!processBlock())
                    return false;
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_BLOCKS);
}

bool dxfRW::processBlock() {
    DRW_DBG("dxfRW::processBlock");
    int code;
    DRW_Block block;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (block.parentHandle != DRW::NoHandle) {
                const auto recordName = m_readingContext.resolveBlockRecordName(block.parentHandle);
                if (!recordName.empty())
                    block.name = recordName;
                block.insUnits = m_readingContext.resolveBlockRecordInsUnits(block.parentHandle);
            }
            if (block.handle != DRW::NoHandle && !block.name.empty()) {
                m_readingContext.blockRecordMap[block.handle] = {block.name, block.insUnits};
            }
            iface->addBlock(block);
            if (nextentity == "ENDBLK") {
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            } else {
                if (!processEntities(true))
                    return false;
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            }
        }

        if (!block.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_BLOCKS);
}


/********* Entities Section *********/

bool dxfRW::processEntities(bool isblock) {
    DRW_DBG("dxfRW::processEntities\n");
    int code;
    if (!isblock || nextentity.empty()) {
        if (!reader->readRec(&code)){
            return setError(DRW::BAD_READ_ENTITIES);
        }

        if (code == 0) {
            nextentity = reader->getString();
        } else if (!isblock) {
            return setError(DRW::BAD_READ_ENTITIES);  //first record in entities is 0
        }
    }

    bool processed {false};
    do {
        if (nextentity == "ENDSEC" || nextentity == "ENDBLK") {
            return true;  //found ENDSEC or ENDBLK terminate
        }
        else if (nextentity == "POINT") {
            processed = processPoint();
        } else if (nextentity == "LINE") {
            processed = processLine();
        } else if (nextentity == "CIRCLE") {
            processed = processCircle();
        } else if (nextentity == "ARC") {
            processed = processArc();
        } else if (nextentity == "ELLIPSE") {
            processed = processEllipse();
        } else if (nextentity == "TRACE") {
            processed = processTrace();
        } else if (nextentity == "SOLID") {
            processed = processSolid();
        } else if (nextentity == "INSERT") {
            processed = processInsert();
        } else if (nextentity == "ACAD_TABLE") {
            processed = processTable();
        } else if (nextentity == "LWPOLYLINE") {
            processed = processLWPolyline();
        } else if (nextentity == "POLYLINE") {
            processed = processPolyline();
        } else if (nextentity == "TEXT") {
            processed = processText();
        } else if (nextentity == "MTEXT") {
            processed = processMText();
        } else if (nextentity == "RTEXT") {
            processed = processRText();
        } else if (nextentity == "ARCALIGNEDTEXT"
                   || nextentity == "ARC_ALIGNED_TEXT") {
            processed = processArcAlignedText();
        } else if (nextentity == "MLINE") {
            processed = processMLine();
        } else if (nextentity == "PDFUNDERLAY"
                   || nextentity == "DGNUNDERLAY"
                   || nextentity == "DWFUNDERLAY") {
            processed = processUnderlay(nextentity);
        } else if (nextentity == "HATCH") {
            processed = processHatch();
        } else if (nextentity == "MPOLYGON") {
            processed = processMPolygon();
        } else if (nextentity == "SPLINE") {
            processed = processSpline();
        } else if (nextentity == "HELIX") {
            processed = processHelix();
        } else if (nextentity == "3DFACE") {
            processed = process3dface();
        } else if (nextentity == "MESH") {
            processed = processMesh();
        } else if (nextentity == "VIEWPORT") {
            processed = processViewport();
        } else if (nextentity == "IMAGE") {
            processed = processImage();
        } else if (nextentity == "WIPEOUT") {
            processed = processWipeout();
        } else if (nextentity == "POINTCLOUD") {
            processed = processPointCloud();
        } else if (nextentity == "POINTCLOUDEX") {
            processed = processPointCloudEx();
        } else if (nextentity == "PLANESURFACE"
                   || nextentity == "EXTRUDEDSURFACE"
                   || nextentity == "REVOLVEDSURFACE"
                   || nextentity == "SWEPTSURFACE"
                   || nextentity == "LOFTEDSURFACE"
                   || nextentity == "NURBSURFACE") {
            processed = processSurface();
        } else if (nextentity == "3DSOLID"
                   || nextentity == "REGION"
                   || nextentity == "BODY") {
            processed = processModelerGeometry();
        } else if (nextentity == "MULTILEADER") {
            processed = processMultiLeader();
        } else if (nextentity == "DIMENSION") {
            processed = processDimension();
        } else if (nextentity == "ARC_DIMENSION") {
            processed = processArcDimension();
        } else if (nextentity == "LARGE_RADIAL_DIMENSION") {
            processed = processLargeRadialDimension();
        } else if (nextentity == "LEADER") {
            processed = processLeader();
        } else if (nextentity == "RAY") {
            processed = processRay();
        } else if (nextentity == "XLINE") {
            processed = processXline();
        } else if (nextentity == "TOLERANCE") {
            processed = processTolerance();
        } else {
            //Slice A4: capture an unmodeled entity verbatim rather than dropping
            //it (also preserves block ATTDEFs, which have no typed DXF dispatch).
            processed = processRawEntity();
        }
    } while (processed);

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processEllipse() {
    DRW_DBG("dxfRW::processEllipse");
    int code;
    DRW_Ellipse ellipse;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                ellipse.applyExtrusion();
            iface->addEllipse(ellipse);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!ellipse.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTrace() {
    DRW_DBG("dxfRW::processTrace");
    int code;
    DRW_Trace trace;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                trace.applyExtrusion();
            iface->addTrace(trace);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!trace.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSolid() {
    DRW_DBG("dxfRW::processSolid");
    int code;
    DRW_Solid solid;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                solid.applyExtrusion();
            iface->addSolid(solid);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!solid.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMesh() {
    DRW_DBG("dxfRW::processMesh");
    int code;
    DRW_Mesh mesh;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMesh(mesh);
            return true;
        }
        if (!mesh.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::process3dface() {
    DRW_DBG("dxfRW::process3dface");
    int code;
    DRW_3Dface face;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->add3dFace(face);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!face.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processViewport() {
    DRW_DBG("dxfRW::processViewport");
    int code;
    DRW_Viewport vp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addViewport(vp);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!vp.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPoint() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Point point;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addPoint(point);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!point.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLine() {
    DRW_DBG("dxfRW::processLine\n");
    int code;
    DRW_Line line;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addLine(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMLine() {
    DRW_DBG("dxfRW::processMLine\n");
    int code;
    DRW_MLine mline;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLine(&mline);
            return true;
        }
        if (!mline.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processUnderlay(const std::string& kind) {
    DRW_DBG("dxfRW::processUnderlay\n");
    int code;
    DRW_Underlay u;
    if (kind == "DGNUNDERLAY") u.kind = DRW_Underlay::DGN;
    else if (kind == "DWFUNDERLAY") u.kind = DRW_Underlay::DWF;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addUnderlay(&u);
            return true;
        }
        if (!u.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processRay() {
    DRW_DBG("dxfRW::processRay\n");
    int code;
    DRW_Ray line;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addRay(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processXline() {
    DRW_DBG("dxfRW::processXline\n");
    int code;
    DRW_Xline line;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addXline(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processCircle() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Circle circle;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                circle.applyExtrusion();
            iface->addCircle(circle);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!circle.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArc() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Arc arc;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                arc.applyExtrusion();
            iface->addArc(arc);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!arc.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processInsert() {
    DRW_DBG("dxfRW::processInsert");
    int code;
    DRW_Insert insert;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            // Attribute flag (66=1) signals trailing ATTRIB entities; mirror
            // the POLYLINE/VERTEX/SEQEND pattern and gate on the next entity
            // name rather than the flag (some writers omit code 66).
            if (nextentity != "ATTRIB") {
                iface->addInsert(insert);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processAttrib(&insert))  //fills insert.attlist until SEQEND
                return false;
            continue;
        }

        if (!insert.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTable() {
    DRW_DBG("dxfRW::processTable");
    int code;
    DRW_Table table;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity != "ATTRIB") {
                iface->addTable(table);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processAttrib(&table))
                return false;
            continue;
        }

        if (!table.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processAttrib(DRW_Insert *insert) {
    DRW_DBG("dxfRW::processAttrib");
    int code;
    auto att = std::make_shared<DRW_Attrib>();
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity == "SEQEND") {
                insert->attlist.push_back(att);
                return true;  //found SEQEND, no more attribs, terminate
            }
            if (nextentity == "ATTRIB") {
                insert->attlist.push_back(att);
                att = std::make_shared<DRW_Attrib>(); //another attrib
                continue;
            }
            insert->attlist.push_back(att);
            return true;
        }

        if (!att->parseCode(code, reader)) { //members of att are reinitialized here
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLWPolyline() {
    DRW_DBG("dxfRW::processLWPolyline");
    int code;
    DRW_LWPolyline pl;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt)
                pl.applyExtrusion();
            iface->addLWPolyline(pl);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!pl.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPolyline() {
    DRW_DBG("dxfRW::processPolyline");
    int code;
    DRW_Polyline pl;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity != "VERTEX") {
                iface->addPolyline(pl);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processVertex(&pl))
                return false;
            continue;
        }

        if (!pl.parseCode(code, reader)) { //parseCode just initialize the members of pl
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processVertex(DRW_Polyline *pl) {
    DRW_DBG("dxfRW::processVertex");
    int code;
    auto v = std::make_shared<DRW_Vertex>();
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if(0 == code)  {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity == "SEQEND") {
                pl->appendVertex(v);
                return true;  //found SEQEND no more vertex, terminate
            }
            if (nextentity == "VERTEX"){
                pl->appendVertex(v);
                v = std::make_shared<DRW_Vertex>(); //another vertex
                continue;
            }
            pl->appendVertex(v);
            return true;
        }

        if (!v->parseCode(code, reader)) { //the members of v are reinitialized here
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTolerance() {
    DRW_DBG("dxfRW::processTolerance");
    int code;
    DRW_Tolerance tol;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addTolerance(tol);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!tol.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processText() {
    DRW_DBG("dxfRW::processText");
    int code;
    DRW_Text txt;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMText() {
    DRW_DBG("dxfRW::processMText");
    int code;
    DRW_MText txt;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            txt.updateAngle();
            iface->addMText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// RTEXT (Express Tools reactive text) — read-only, mapped onto DRW_Text and
// delivered via addText.  Its DXF group codes are a TEXT subset plus a flags
// long (70); DRW_RText::parseCode handles the flag and delegates the rest.
bool dxfRW::processRText() {
    DRW_DBG("dxfRW::processRText");
    int code;
    DRW_RText txt;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// ARCALIGNEDTEXT (Express Tools arc-aligned text) — read-only, mapped onto
// DRW_Text as a 2D approximation (text at the arc mid-point).  applyArcApprox-
// imation() derives basePoint / angle / height from the arc parameters once all
// group codes are read.
bool dxfRW::processArcAlignedText() {
    DRW_DBG("dxfRW::processArcAlignedText");
    int code;
    DRW_ArcAlignedText txt;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            txt.applyArcApproximation();
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processHatch() {
    DRW_DBG("dxfRW::processHatch");
    int code;
    DRW_Hatch hatch;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addHatch(&hatch);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!hatch.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processSpline() {
    DRW_DBG("dxfRW::processSpline");
    int code;
    DRW_Spline sp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addSpline(&sp);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!sp.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processHelix() {
    DRW_DBG("dxfRW::processHelix");
    int code;
    DRW_Helix helix;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addHelix(&helix);
            return true;
        }

        if (!helix.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processImage() {
    DRW_DBG("dxfRW::processImage");
    int code;
    DRW_Image img;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!img.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// MULTILEADER DXF read.  Captures the entity-level scalar fields via
// DRW_MLeader::parseCode.  Nested CONTEXT_DATA{} / LEADER{} / LEADER_LINE{}
// blocks use control-flow group codes (300/302/304 open + 301/303/305 close)
// — Phase 8 keeps the body capture minimal; Phase 9 / follow-up will wire
// the full nested-block state machine.
bool dxfRW::processMultiLeader() {
    DRW_DBG("dxfRW::processMultiLeader");
    int code;
    DRW_MLeader e;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLeader(&e);
            return true;
        }
        if (!e.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processWipeout() {
    DRW_DBG("dxfRW::processWipeout");
    int code;
    DRW_Wipeout wipeout;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addWipeout(&wipeout);
            return true;
        }

        if (!wipeout.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPointCloud() {
    DRW_DBG("dxfRW::processPointCloud");
    int code;
    DRW_PointCloud pc;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addPointCloud(&pc);
            return true;
        }

        if (!pc.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPointCloudEx() {
    DRW_DBG("dxfRW::processPointCloudEx");
    int code;
    DRW_PointCloudEx pce;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addPointCloudEx(&pce);
            return true;
        }

        if (!pce.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSurface() {
    DRW_DBG("dxfRW::processSurface");
    int code;
    std::unique_ptr<DRW_Surface> surf;
    if (nextentity == "PLANESURFACE") {
        surf = std::make_unique<DRW_PlaneSurface>();
    } else if (nextentity == "EXTRUDEDSURFACE") {
        surf = std::make_unique<DRW_ExtrudedSurface>();
    } else if (nextentity == "REVOLVEDSURFACE") {
        surf = std::make_unique<DRW_RevolvedSurface>();
    } else if (nextentity == "SWEPTSURFACE") {
        surf = std::make_unique<DRW_SweptSurface>();
    } else if (nextentity == "LOFTEDSURFACE") {
        surf = std::make_unique<DRW_LoftedSurface>();
    } else if (nextentity == "NURBSSURFACE") {
        surf = std::make_unique<DRW_NurbsSurface>();
    } else {
        return setError(DRW::BAD_READ_ENTITIES);
    }

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addSurface(surf.get());
            return true;
        }

        if (!surf->parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processModelerGeometry() {
    DRW_DBG("dxfRW::processModelerGeometry");
    int code;
    DRW::ETYPE type = DRW::UNKNOWN;
    if (nextentity == "3DSOLID") {
        type = DRW::E3DSOLID;
    } else if (nextentity == "REGION") {
        type = DRW::REGION;
    } else if (nextentity == "BODY") {
        type = DRW::BODY;
    } else {
        return setError(DRW::BAD_READ_ENTITIES);
    }

    DRW_ModelerGeometry geom(type);
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addModelerGeometry(geom);
            return true;
        }

        if (!geom.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// MPOLYGON (AcDbMPolygon) DXF read.  Boundary loops, solid flag and pattern share
// HATCH's group codes; DRW_MPolygon::parseCode delegates those to DRW_Hatch and
// additionally captures the MPOLYGON-only fill-color / degenerate-count trailer.
// Delivered via addMPolygon (defaults to addHatch, so it renders as a hatch).
bool dxfRW::processMPolygon() {
    DRW_DBG("dxfRW::processMPolygon");
    int code;
    DRW_MPolygon poly;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMPolygon(&poly);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!poly.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processDimension() {
    DRW_DBG("dxfRW::processDimension");
    int code;
    DRW_Dimension dim;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            int type = dim.type & 0x0F;
            switch (type) {
            case 0: {
                DRW_DimLinear d(dim);
                iface->addDimLinear(&d);
                break; }
            case 1: {
                DRW_DimAligned d(dim);
                iface->addDimAlign(&d);
                break; }
            case 2:  {
                DRW_DimAngular d(dim);
                iface->addDimAngular(&d);
                break;}
            case 3: {
                DRW_DimDiametric d(dim);
                iface->addDimDiametric(&d);
                break; }
            case 4: {
                DRW_DimRadial d(dim);
                iface->addDimRadial(&d);
                break; }
            case 5: {
                DRW_DimAngular3p d(dim);
                iface->addDimAngular3P(&d);
                break; }
            case 6: {
                DRW_DimOrdinate d(dim);
                iface->addDimOrdinate(&d);
                break; }
            }
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!dim.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArcDimension() {
    DRW_DBG("dxfRW::processArcDimension");
    int code;
    DRW_DimArc d;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDimArc(&d);
            return true;
        }
        if (!d.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

// LARGE_RADIAL_DIMENSION (AcDbRadialDimensionLarge) is a top-level entity token,
// NOT a DIMENSION subtype, so it gets its own reader.  Delivered via the existing
// addDimRadial callback (DRW_DimLargeRadial is-a DRW_DimRadial).
bool dxfRW::processLargeRadialDimension() {
    DRW_DBG("dxfRW::processLargeRadialDimension");
    int code;
    DRW_DimLargeRadial d;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDimRadial(&d);
            return true;
        }
        if (!d.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLeader() {
    DRW_DBG("dxfRW::processLeader");
    int code;
    DRW_Leader leader;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addLeader(&leader);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!leader.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


/********* Objects Section *********/

bool dxfRW::processObjects() {
    DRW_DBG("dxfRW::processObjects\n");
    int code;
    if (!reader->readRec(&code)
            || 0 != code){
        return setError(DRW::BAD_READ_OBJECTS); //first record in objects must be 0
    }

    bool processed {false};
    nextentity = reader->getString();
    do {
        if ("ENDSEC" == nextentity) {
            return true;  //found ENDSEC terminate
        }

        if ("ACDBDETAILVIEWSTYLE" == nextentity || "DETAILVIEWSTYLE" == nextentity) {
            processed = processDetailViewStyle();
        }
        else if ("ACDBSECTIONVIEWSTYLE" == nextentity || "SECTIONVIEWSTYLE" == nextentity) {
            processed = processSectionViewStyle();
        }
        else if ("BREAKDATA" == nextentity) {
            processed = processBreakData();
        }
        else if ("BREAKPOINTREF" == nextentity) {
            processed = processBreakPointRef();
        }
        else if ("IMAGEDEF" == nextentity) {
            processed = processImageDef();
        }
        else if ("PDFDEFINITION" == nextentity
                 || "DGNDEFINITION" == nextentity
                 || "DWFDEFINITION" == nextentity) {
            processed = processUnderlayDefinition();
        }
        else if ("PLOTSETTINGS" == nextentity) {
            processed = processPlotSettings();
        }
        else if ("GROUP" == nextentity) {
            processed = processGroup();
        }
        else if ("DICTIONARY" == nextentity) {
            processed = processDictionary();
        }
        else if ("SCALE" == nextentity) {
            processed = processScale();
        }
        else if ("MLINESTYLE" == nextentity) {
            processed = processMLineStyle();
        }
        else if ("DICTIONARYVAR" == nextentity) {
            processed = processDictionaryVar();
        }
        else if ("ACDBDICTIONARYWDFLT" == nextentity) {
            processed = processDictionaryWithDefault();
        }
        else if ("RASTERVARIABLES" == nextentity) {
            processed = processRasterVariables();
        }
        else if ("FIELD" == nextentity || "ACDBFIELD" == nextentity) {
            processed = processField();
        }
        else if ("FIELDLIST" == nextentity || "ACDBFIELDLIST" == nextentity) {
            processed = processFieldList();
        }
        else if ("SUN" == nextentity) {
            processed = processSun();
        }
        else if ("LAYOUT" == nextentity) {
            processed = processLayout();
        }
        else if ("WIPEOUTVARIABLES" == nextentity) {
            processed = processWipeoutVariables();
        }
        else if ("MATERIAL" == nextentity) {
            processed = processMaterial();
        }
        else if ("GEODATA" == nextentity) {
            processed = processGeoData();
        }
        else if ("VISUALSTYLE" == nextentity
                 || "ACDB_VISUALSTYLE_CLASS" == nextentity) {
            processed = processVisualStyle();
        }
        else if ("IMAGEDEF_REACTOR" == nextentity) {
            processed = processImageDefReactor();
        }
        else if ("SPATIAL_FILTER" == nextentity) {
            processed = processSpatialFilter();
        }
        else if ("TABLESTYLE" == nextentity) {
            processed = processTableStyle();
        }
        else if ("MLEADERSTYLE" == nextentity) {
            processed = processMLeaderStyle();
        }
        else if ("SORTENTSTABLE" == nextentity) {
            processed = processSortEntsTable();
        }
        else if ("DIMASSOC" == nextentity) {
            processed = processDimAssoc();
        }
        else if ("SOLIDBACKGROUND" == nextentity || "SOLID_BACKGROUND" == nextentity
                 || "GRADIENTBACKGROUND" == nextentity || "GRADIENT_BACKGROUND" == nextentity
                 || "GROUNDPLANEBACKGROUND" == nextentity || "GROUND_PLANE_BACKGROUND" == nextentity
                 || "IMAGEBACKGROUND" == nextentity || "IMAGE_BACKGROUND" == nextentity
                 || "IBLBACKGROUND" == nextentity || "IBL_BACKGROUND" == nextentity
                 || "SKYLIGHTBACKGROUND" == nextentity || "SKYLIGHT_BACKGROUND" == nextentity) {
            processed = processBackground();
        }
        else if ("POINTCLOUDDEFINITION" == nextentity
                 || "POINTCLOUDDEFINITIONEX" == nextentity
                 || "POINTCLOUDDEFREACTOR" == nextentity
                 || "POINTCLOUDDEFREACTOREX" == nextentity) {
            processed = processPointCloudDef();
        }
        else if ("SUNSTUDY" == nextentity) {
            processed = processSunStudy();
        }
        else if ("RENDERSETTINGS" == nextentity || "RENDERGLOBAL" == nextentity
                 || "RENDERENVIRONMENT" == nextentity || "RENDERENTRY" == nextentity
                 || "RAPIDRTRENDERSETTINGS" == nextentity
                 || "MENTALRAYRENDERSETTINGS" == nextentity) {
            processed = processRenderSettings();
        }
        else if ("SECTIONMANAGER" == nextentity || "ACDBSECTIONMANAGER" == nextentity
                 || "SECTION_MANAGER" == nextentity || "SECTIONSETTINGS" == nextentity
                 || "ACDBSECTIONSETTINGS" == nextentity || "SECTION_SETTINGS" == nextentity) {
            processed = processSection();
        }
        else {
            //Slice A1: never silently drop an unmodeled object — capture its
            //group codes verbatim for lossless re-emit instead of skipping.
            processed = processRawObject();
        }
    }
    while (processed);

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDetailViewStyle() {
    DRW_DBG("dxfRW::processDetailViewStyle");
    int code;
    DRW_DetailViewStyle style;
    //Also route to the raw net so this typed-read OBJECT survives DXF->DXF (it
    //has no typed DXF writer). Without it the object is dropped and any extension
    //dictionary it owns is orphaned (dangling 330). CLASS is registered via
    //dxfClassForRecordName(ACDB(DETAIL|SECTION)VIEWSTYLE).
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDetailViewStyle(style);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!style.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processSectionViewStyle() {
    DRW_DBG("dxfRW::processSectionViewStyle");
    int code;
    DRW_SectionViewStyle style;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addSectionViewStyle(style);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!style.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processBreakData() {
    DRW_DBG("dxfRW::processBreakData");
    int code;
    DRW_BreakData data;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addBreakData(data);
            iface->addRawDxfObject(raw);  // else dropped on DXF->DXF (no typed writer)
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// MATERIAL (AcDbMaterial): structured DXF read of name/description (matching the
// DWG parser + dwgTs), plus full raw-net preservation for lossless DXF re-emit
// (the visual-property fields are not modeled, only round-tripped).
bool dxfRW::processMaterial() {
    DRW_DBG("dxfRW::processMaterial");
    int code;
    DRW_Material data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMaterial(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// GEODATA (AcDbGeoData): structured DXF read of the scalar geolocation fields,
// plus full raw-net preservation (the coordinate-mesh lists are round-tripped
// raw only — see DRW_GeoData::parseCode).
bool dxfRW::processGeoData() {
    DRW_DBG("dxfRW::processGeoData");
    int code;
    DRW_GeoData data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addGeoData(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// VISUALSTYLE (AcDbVisualStyle): structured DXF read of description + style type,
// plus full raw-net preservation (the per-property face/edge/display settings are
// round-tripped raw only — matches dwgTs's VISUALSTYLE decode depth).
bool dxfRW::processVisualStyle() {
    DRW_DBG("dxfRW::processVisualStyle");
    int code;
    DRW_VisualStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addVisualStyle(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// IMAGEDEF_REACTOR (AcDbRasterImageDefReactor): structured DXF read of the
// class-version field + raw-net preservation for lossless DXF re-emit.
bool dxfRW::processImageDefReactor() {
    DRW_DBG("dxfRW::processImageDefReactor");
    int code;
    DRW_ImageDefinitionReactor data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addImageDefinitionReactor(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SPATIAL_FILTER (AcDbSpatialFilter): structured DXF read of the clip boundary +
// planes + raw-net preservation.
bool dxfRW::processSpatialFilter() {
    DRW_DBG("dxfRW::processSpatialFilter");
    int code;
    DRW_SpatialFilter data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addSpatialFilter(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// TABLESTYLE (AcDbTableStyle): structured DXF read of the top-level fields +
// raw-net preservation (nested row/cell styles round-tripped raw only).
bool dxfRW::processTableStyle() {
    DRW_DBG("dxfRW::processTableStyle");
    int code;
    DRW_TableStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addTableStyle(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// MLEADERSTYLE (AcDbMLeaderStyle): structured DXF read of the full scalar record
// + raw-net preservation.  Delivered via addMLeaderStyle (pointer callback).
bool dxfRW::processMLeaderStyle() {
    DRW_DBG("dxfRW::processMLeaderStyle");
    int code;
    DRW_MLeaderStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addMLeaderStyle(&data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SORTENTSTABLE (AcDbSortentsTable): structured DXF read of the draw-order map
// (block owner + entity/sort handle pairs) + raw-net preservation.
bool dxfRW::processSortEntsTable() {
    DRW_DBG("dxfRW::processSortEntsTable");
    int code;
    DRW_SortEntsTable data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addSortEntsTable(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// DIMASSOC (AcDbDimAssoc): structured DXF read of the associative-dimension
// metadata (dimension handle, flags, osnap refs) + raw-net preservation.
bool dxfRW::processDimAssoc() {
    DRW_DBG("dxfRW::processDimAssoc");
    int code;
    DRW_DimensionAssociation data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addDimensionAssociation(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDb*Background OBJECTS (solid/gradient/ground-plane/image/IBL/skylight):
// structured DXF read into DRW_Background (kind set from the entity name) + raw-
// net preservation.  DWG stays raw (no DWG parser).  Not rendered by LibreCAD.
bool dxfRW::processBackground() {
    DRW_DBG("dxfRW::processBackground");
    int code;
    DRW_Background data;
    if (nextentity == "GRADIENTBACKGROUND" || nextentity == "GRADIENT_BACKGROUND")
        data.m_kind = DRW_Background::Gradient;
    else if (nextentity == "GROUNDPLANEBACKGROUND" || nextentity == "GROUND_PLANE_BACKGROUND")
        data.m_kind = DRW_Background::GroundPlane;
    else if (nextentity == "IMAGEBACKGROUND" || nextentity == "IMAGE_BACKGROUND")
        data.m_kind = DRW_Background::Image;
    else if (nextentity == "IBLBACKGROUND" || nextentity == "IBL_BACKGROUND")
        data.m_kind = DRW_Background::Ibl;
    else if (nextentity == "SKYLIGHTBACKGROUND" || nextentity == "SKYLIGHT_BACKGROUND")
        data.m_kind = DRW_Background::Skylight;
    else
        data.m_kind = DRW_Background::Solid;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addBackground(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDbPointCloudDef / ...DefEx and reactors: structured DXF read into
// DRW_PointCloudDef (kind from the entity name) + raw-net preservation.
bool dxfRW::processPointCloudDef() {
    DRW_DBG("dxfRW::processPointCloudDef");
    int code;
    DRW_PointCloudDef data;
    if (nextentity == "POINTCLOUDDEFINITIONEX")
        data.m_kind = DRW_PointCloudDef::DefinitionEx;
    else if (nextentity == "POINTCLOUDDEFREACTOR"
             || nextentity == "POINTCLOUDDEFREACTOREX")
        data.m_kind = DRW_PointCloudDef::Reactor;
    else
        data.m_kind = DRW_PointCloudDef::Definition;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addPointCloudDef(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SUNSTUDY (AcDbSunStudy): structured DXF read of the scalar study config +
// raw-net preservation (date/hour lists left raw).
bool dxfRW::processSunStudy() {
    DRW_DBG("dxfRW::processSunStudy");
    int code;
    DRW_SunStudy data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addSunStudy(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDbRenderSettings family (settings/global/environment/entry/mentalray/
// rapidrt): positional DXF capture into DRW_RenderSettings (kind from the entity
// name; vectors finalized into named fields) + raw-net preservation.
bool dxfRW::processRenderSettings() {
    DRW_DBG("dxfRW::processRenderSettings");
    int code;
    DRW_RenderSettings data;
    if (nextentity == "RENDERGLOBAL") data.m_kind = DRW_RenderSettings::Global;
    else if (nextentity == "RENDERENVIRONMENT") data.m_kind = DRW_RenderSettings::Environment;
    else if (nextentity == "RENDERENTRY") data.m_kind = DRW_RenderSettings::Entry;
    else if (nextentity == "RAPIDRTRENDERSETTINGS") data.m_kind = DRW_RenderSettings::RapidRT;
    else if (nextentity == "MENTALRAYRENDERSETTINGS") data.m_kind = DRW_RenderSettings::MentalRay;
    else data.m_kind = DRW_RenderSettings::Settings;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            data.finalize();
            iface->addRenderSettings(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SECTION_MANAGER / SECTION_SETTINGS: structured DXF read into DRW_Section
// (kind from the entity name) + raw-net preservation.
bool dxfRW::processSection() {
    DRW_DBG("dxfRW::processSection");
    int code;
    DRW_Section data;
    if (nextentity == "SECTIONSETTINGS" || nextentity == "ACDBSECTIONSETTINGS"
        || nextentity == "SECTION_SETTINGS")
        data.m_kind = DRW_Section::Settings;
    else
        data.m_kind = DRW_Section::Manager;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            nextentity = reader->getString();
            iface->addSection(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!data.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processBreakPointRef() {
    DRW_DBG("dxfRW::processBreakPointRef");
    int code;
    DRW_BreakPointRef ref;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addBreakPointRef(ref);
            iface->addRawDxfObject(raw);  // else dropped on DXF->DXF (no typed writer)
            return true;
        }
        captureRawGroup(raw, code);
        if (!ref.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processUnderlayDefinition() {
    DRW_DBG("dxfRW::processUnderlayDefinition");
    int code;
    DRW_UnderlayDefinition definition;
    if (nextentity == "DGNDEFINITION")
        definition.kind = DRW_UnderlayDefinition::DGN;
    else if (nextentity == "DWFDEFINITION")
        definition.kind = DRW_UnderlayDefinition::DWF;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->linkUnderlay(&definition);
            iface->addRawDxfObject(raw);
            return true;
        }
        captureRawGroup(raw, code);
        if (!definition.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processImageDef() {
    DRW_DBG("dxfRW::processImageDef");
    int code;
    DRW_ImageDef img;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->linkImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!img.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPlotSettings() {
    DRW_DBG("dxfRW::processPlotSettings");
    int code;
    DRW_PlotSettings ps;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addPlotSettings(&ps);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!ps.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processGroup() {
    DRW_DBG("dxfRW::processGroup");
    int code;
    DRW_Group group;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addGroup(group);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!group.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionary() {
    DRW_DBG("dxfRW::processDictionary");
    int code;
    DRW_Dictionary dict;
    //Route NON-ROOT named dictionaries through the raw net so they round-trip
    //DXF->DXF (re-attached to the regenerated root via setRootDictEntries). The
    //source root dict (330==0) is NOT routed — the codec regenerates it at fixed
    //handle C; re-emitting it would duplicate the NamedObjectsDictionary.
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionary(dict);
            //Skip the root (330==0) and the fixed root/group handles C/D — the
            //codec always regenerates those; routing them would duplicate the
            //NamedObjectsDictionary / ACAD_GROUP dict.
            if (raw.parentHandle != 0 && raw.handle != 0xCu && raw.handle != 0xDu)
                iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!dict.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processScale() {
    DRW_DBG("dxfRW::processScale");
    int code;
    DRW_Scale scale;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addScale(scale);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!scale.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processMLineStyle() {
    DRW_DBG("dxfRW::processMLineStyle");
    int code;
    DRW_MLineStyle style;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLineStyle(style);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!style.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionaryVar() {
    DRW_DBG("dxfRW::processDictionaryVar");
    int code;
    DRW_DictionaryVar var;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionaryVar(var);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!var.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionaryWithDefault() {
    DRW_DBG("dxfRW::processDictionaryWithDefault");
    int code;
    DRW_DictionaryWithDefault dict;
    //Same as processDictionary: route non-root WDFLT dicts (e.g. ACAD_PLOTSTYLENAME)
    //through the raw net; its 340 default points at a raw-net-preserved placeholder.
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionaryWithDefault(dict);
            if (raw.parentHandle != 0 && raw.handle != 0xCu && raw.handle != 0xDu)
                iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!dict.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// Data-only OBJECTS (no inter-object handle refs beyond base-class 5/330) are
// ALSO captured into the raw-passthrough net so the DXF writer re-emits their
// bodies verbatim on a DXF->DXF round-trip. The typed object still populates
// LC_DwgAdvancedMetadata for the DWG write path; the raw net is DXF-write-only
// (the DWG path ignores it), so there is no double-emit. The dictionary/handle
// "spine" types (DICTIONARY/GROUP/LAYOUT/ACDBDICTIONARYWDFLT) are deliberately
// NOT routed here — verbatim re-emit of their handle graph would corrupt the
// regenerated dictionary tree; those await typed DXF writers.
bool dxfRW::processRasterVariables() {
    DRW_DBG("dxfRW::processRasterVariables");
    int code;
    DRW_RasterVariables rv;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addRasterVariables(rv);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!rv.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// FIELD / FIELDLIST (AcDbField / AcDbFieldList).  The DWG read path already
// decodes these (dwgReader OBJECTS dispatch -> DRW_Field / DRW_FieldList ->
// addField / addFieldList); this adds the matching DXF read.  The group-code
// layout below was verified against an ODA-converted FIELD-rich DWG
// (blocks_and_tables_-_imperial.dwg -> DXF, ODA File Converter 27.1.0):
//   1  evaluatorId          90  child-field count       360 child-field handle
//   2  fieldCode            97  object-id count         331 object-id handle
//   3  fieldCode overflow   91  evaluation option flags 300 evaluation error msg
//   92/94/95/96 flags       301 value string           98  value-string length
// After the field-level scalars, per-child-value records begin at a code 6 (data
// key) or 7 (cache key) and each end at code 304 "ACVALUE_END".  Codes that also
// occur INSIDE a value sub-record (90/91/94/140/300) must not clobber the
// field-level scalars, so a "child open" flag routes them to the child value.
bool dxfRW::processField() {
    DRW_DBG("dxfRW::processField");
    int code;
    DRW_Field field;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    bool inSubclass = false;      // set once the AcDbField subclass marker is seen
    bool childOpen = false;       // currently inside a per-child value sub-record
    DRW_Field::ChildValue child;

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (childOpen)
                field.m_childValues.push_back(child);
            iface->addField(field);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);

        if (100 == code) {
            if ("AcDbField" == reader->getString())
                inSubclass = true;
            continue;
        }
        if (!inSubclass) {
            // Common preamble: handle (5) and owner (330).
            if (5 == code)        field.handle = reader->getHandleString();
            else if (330 == code) field.parentHandle = reader->getHandleString();
            continue;
        }

        switch (code) {
        case 1:   field.m_evaluatorId = reader->getUtf8String(); break;
        case 2:   field.m_fieldCode = reader->getUtf8String(); break;
        case 3:   field.m_fieldCode += reader->getUtf8String(); break;
        case 4:   field.m_formatString = reader->getUtf8String(); break;
        case 6:
        case 7:
            if (childOpen)
                field.m_childValues.push_back(child);
            child = DRW_Field::ChildValue();
            child.m_key = reader->getUtf8String();
            childOpen = true;
            break;
        case 304:  // ACVALUE_END terminates a value sub-record
            if (childOpen) {
                field.m_childValues.push_back(child);
                childOpen = false;
            }
            break;
        case 90:
            if (childOpen) child.m_value.m_dataType = reader->getInt32();
            // field-level code 90 is the child-field count -> implicit in m_childHandles
            break;
        case 91:
            if (childOpen) child.m_value.m_value.addInt(91, reader->getInt32());
            else field.m_evaluationOptionFlags = reader->getInt32();
            break;
        case 92:  if (!childOpen) field.m_filingOptionFlags = reader->getInt32(); break;
        case 94:  if (!childOpen) field.m_fieldStateFlags = reader->getInt32(); break;
        case 95:  if (!childOpen) field.m_evaluationStatusFlags = reader->getInt32(); break;
        case 96:  if (!childOpen) field.m_evaluationErrorCode = reader->getInt32(); break;
        case 140: if (childOpen) child.m_value.m_value.addDouble(140, reader->getDouble()); break;
        case 300:
            if (childOpen) child.m_value.m_valueString = reader->getUtf8String();
            else field.m_evaluationErrorMessage = reader->getUtf8String();
            break;
        case 301:
            if (childOpen) child.m_value.m_valueString = reader->getUtf8String();
            else field.m_valueString = reader->getUtf8String();
            break;
        case 98:  if (!childOpen) field.m_valueStringLength = reader->getInt32(); break;
        case 360: {
            int h = reader->getHandleString();
            if (h != 0) field.m_childHandles.push_back(static_cast<std::uint32_t>(h));
            break;
        }
        case 331: {
            int h = reader->getHandleString();
            if (h != 0) field.m_objectHandles.push_back(static_cast<std::uint32_t>(h));
            break;
        }
        default: break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// FIELDLIST (AcDbIdSet / AcDbFieldList): num_fields (90) + an "unknown" bool
// (290) + a soft-pointer per field.  The OBJECTS common preamble already eats
// the first 330 (owner); every 330 after the AcDbIdSet subclass marker is a
// field handle.
bool dxfRW::processFieldList() {
    DRW_DBG("dxfRW::processFieldList");
    int code;
    DRW_FieldList list;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    bool inSet = false;  // set once the AcDbIdSet / AcDbFieldList marker is seen
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addFieldList(list);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);

        if (100 == code) {
            const std::string sub = reader->getString();
            if ("AcDbIdSet" == sub || "AcDbFieldList" == sub)
                inSet = true;
            continue;
        }

        switch (code) {
        case 5:   list.handle = reader->getHandleString(); break;
        case 90:  break;  // num_fields == m_fieldHandles.size()
        case 290: list.m_unknown = reader->getInt32(); break;
        case 330: {
            const int h = reader->getHandleString();
            if (!inSet)
                list.parentHandle = h;
            else if (h != 0)
                list.m_fieldHandles.push_back(static_cast<std::uint32_t>(h));
            break;
        }
        default: break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processSun() {
    DRW_DBG("dxfRW::processSun");
    int code;
    DRW_Sun sun;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addSun(sun);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!sun.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processLayout() {
    DRW_DBG("dxfRW::processLayout");
    int code;
    DRW_Layout layout;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addLayout(layout);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!layout.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processWipeoutVariables() {
    DRW_DBG("dxfRW::processWipeoutVariables");
    int code;
    DRW_WipeoutVariables wv;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addWipeoutVariables(wv);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        captureRawGroup(raw, code);
        if (!wv.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

//Slice A1: lossless passthrough for an OBJECTS-section object libdxfrw does not
//model as a typed DXF object. Captures every (code,value) pair verbatim (value
//kept as raw text, which round-trips exactly for ASCII DXF) so the object can be
//re-emitted unchanged once the DXF object-write spine (A2) consumes it.
namespace {
enum class RawValType { Str, Int, Int64, Dbl };
//Mirror dxfReader::readRec's code->reader dispatch (intern/dxfreader.cpp) so a
//raw-captured group value is taken from the matching typed getter. readRec parses
//numeric codes into the typed members (intData/int64/doubleData) and leaves
//strData STALE, so getString() is wrong for them. The reader's public `type` is
//ALSO unreliable here: each numeric reader sets `type` then calls readString(&t)
//which resets it to STRING — hence we classify by code range, not reader->type.
RawValType classifyDxfCode(int code) {
    if (code < 10) return RawValType::Str;
    else if (code < 60) return RawValType::Dbl;
    else if (code < 80) return RawValType::Int;             // int16
    else if (code > 89 && code < 100) return RawValType::Int;  // int32
    else if (code == 100 || code == 102 || code == 105) return RawValType::Str;
    else if (code > 109 && code < 150) return RawValType::Dbl;
    else if (code > 159 && code < 170) return RawValType::Int64;
    else if (code < 180) return RawValType::Int;
    else if (code > 209 && code < 240) return RawValType::Dbl;
    else if (code > 269 && code < 290) return RawValType::Int;
    else if (code < 300) return RawValType::Int;            // readBool -> intData
    else if (code < 310) return RawValType::Str;
    else if (code < 320) return RawValType::Str;            // readBinary -> string
    else if (code < 370) return RawValType::Str;            // incl. 330/340/350/360
    else if (code < 390) return RawValType::Int;
    else if (code < 400) return RawValType::Str;
    else if (code < 410) return RawValType::Int;
    else if (code < 420) return RawValType::Str;
    else if (code < 430) return RawValType::Int;
    else if (code < 440) return RawValType::Str;
    else if (code < 450) return RawValType::Int;
    else if (code < 460) return RawValType::Int;
    else if (code < 470) return RawValType::Dbl;
    else if (code <= 481) return RawValType::Str;
    else if (code == 1004) return RawValType::Str;
    else if (code > 998 && code < 1009) return RawValType::Str;
    else if (code < 1060) return RawValType::Dbl;
    else if (code < 1071) return RawValType::Int;
    else if (code == 1071) return RawValType::Int;
    return RawValType::Str;
}
}  // namespace

//Capture the current DXF record into a raw-passthrough carrier as a correctly
//TYPED DRW_Variant (see classifyDxfCode above for why getString()/reader->type
//cannot be trusted for numeric codes — that was the A1/A4 capture bug). The write
//side (writeRawDxfObject) re-emits each variant type, so a typed capture
//round-trips numeric values. ASCII-DXF only; the raw net contract is ASCII (see
//processRawObject). Also latches code 5 -> handle and code 330 -> parentHandle.
void dxfRW::captureRawGroup(DRW_RawDxfObject &obj, int code) {
    switch (classifyDxfCode(code)) {
    case RawValType::Int:
        obj.groups.emplace_back(code, static_cast<std::int32_t>(reader->getInt32()));
        break;
    case RawValType::Int64:
        obj.groups.emplace_back(code, static_cast<std::int64_t>(reader->getInt64()));
        break;
    case RawValType::Dbl:
        obj.groups.emplace_back(code, reader->getDouble());
        break;
    case RawValType::Str:
    default:
        obj.groups.emplace_back(code, reader->getString());
        break;
    }
    if (5 == code && obj.handle == 0) {
        obj.handle = reader->getHandleString();
    } else if (330 == code) {
        // Latch the OWNER 330 only — the one OUTSIDE any 102 {ACAD_REACTORS/
        // ACAD_XDICTIONARY} control group. Reactor 330s live at 102-group depth
        // >= 1; the prior code latched the LAST 330 unconditionally, so an object
        // whose only 330s are reactors (no owner 330) took a reactor handle as
        // its owner. Compute the depth from the groups captured so far (the
        // current 330 is the last element) and latch only at depth 0.
        int depth = 0;
        for (std::size_t i = 0; i + 1 < obj.groups.size(); ++i) {
            if (obj.groups[i].code() == 102
                && obj.groups[i].type() == DRW_Variant::STRING) {
                const char *v = obj.groups[i].c_str();
                if (v && v[0] == '{') ++depth;
                else if (v && v[0] == '}') --depth;
            }
        }
        if (depth == 0 && obj.parentHandle == 0)
            obj.parentHandle = reader->getHandleString();
    }
}

bool dxfRW::processRawObject() {
    DRW_DBG("dxfRW::processRawObject");
    int code;
    DRW_RawDxfObject obj;
    obj.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addRawDxfObject(obj);
            return true;  //found new entity or ENDSEC, terminate
        }
        captureRawGroup(obj, code);
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

//Slice A4: lossless passthrough for an unmodeled entity in the ENTITIES section
//or inside a BLOCK (including standalone ATTDEF). Same verbatim capture as
//processRawObject but reports via addRawDxfEntity / the entities error code.
bool dxfRW::processRawEntity() {
    DRW_DBG("dxfRW::processRawEntity");
    int code;
    DRW_RawDxfObject ent;
    ent.name = nextentity;
    // Accumulate any cached proxy graphics (codes 92/160 + 310) via the common
    // entity parser so we can decode it into render primitives — same path as
    // the DWG reader.  proxyHost is a throwaway carrier; the raw object is still
    // delivered verbatim for round-trip.
    DRW_Point proxyHost;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (proxyHost.proxyGraphics.size() >= 16)
                DRW_ProxyGraphicDecoder::decode(proxyHost.proxyGraphics, version,
                                                *iface, proxyHost);
            iface->addRawDxfEntity(ent);
            return true;  //found new entity, ENDSEC or ENDBLK, terminate
        }
        proxyHost.parseCode(code, reader); // captures layer + proxyGraphics
        captureRawGroup(ent, code);
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

//Slice A2: re-emit a raw-captured object (from processRawObject) verbatim. The
//A1/A4 capture stores every value as STRING, so the raw text round-trips exactly
//for ASCII DXF; the other variant arms are handled defensively.
// True for DXF group codes whose STRING value is a handle reference that the
// codec's m_handleRemap may need to rewrite: the self handle (5/105), the
// soft/hard pointer & owner ranges (320-369), the hard-pointer ranges
// (390-399 and 480-481), and xdata handles (1005). Codes outside these ranges
// (e.g. text strings, layer names) are never rewritten, so a numeric-looking
// non-handle value can never be mistaken for a handle.
static bool dxfIsHandleRefCode(int code) {
    return code == 5 || code == 105 || code == 1005 ||
           (code >= 320 && code <= 369) ||
           (code >= 390 && code <= 399) ||
           (code >= 480 && code <= 481);
}

bool dxfRW::writeRawDxfObject(DRW_RawDxfObject *obj) {
    writer->writeString(0, obj->name);
    for (const DRW_Variant &v : obj->groups) {
        // Apply the structural-collision handle remap to handle-reference codes.
        // The value is a hex handle string; if it names a remapped handle, emit
        // the replacement so this object (and every reference to a remapped
        // object) stays internally consistent.
        if (!m_handleRemap.empty() && v.type() == DRW_Variant::STRING &&
            dxfIsHandleRefCode(v.code())) {
            std::string s = v.c_str();
            char *end = nullptr;
            unsigned long parsed = std::strtoul(s.c_str(), &end, 16);
            if (end != s.c_str() && end != nullptr && *end == '\0') {
                auto it = m_handleRemap.find(static_cast<std::uint32_t>(parsed));
                if (it != m_handleRemap.end()) {
                    writer->writeString(
                        v.code(), toHexStr(static_cast<int>(it->second)));
                    continue;
                }
            }
        }
        switch (v.type()) {
        case DRW_Variant::STRING:
            writer->writeString(v.code(), std::string(v.c_str()));
            break;
        case DRW_Variant::INTEGER:
            writer->writeInt32(v.code(), v.i_val());
            break;
        case DRW_Variant::INTEGER64:
            writer->writeInt64(v.code(), v.i64_val());
            break;
        case DRW_Variant::DOUBLE:
            writer->writeDouble(v.code(), v.d_val());
            break;
        default:
            break;
        }
    }
    return true;
}

//Slice A3: canonical DXF CLASS metadata for the custom-class OBJECTS the raw net
//round-trips — both the routed data-only types (SUN/SCALE/...) and common
//unmodeled OBJECTS captured verbatim (MATERIAL/VISUALSTYLE/...). Values are the
//DXF-authoritative ezdxf CLASS_DEFINITIONS tuple {className(2), appName(3),
//flags(90), wasaProxy(280), isEntity(281)}. Every entry is a NON-fixed object
//class (ezdxf lists only classes that need a CLASS), so registering one can never
//mislabel a fixed built-in (DICTIONARY/GROUP/LAYOUT/MLINESTYLE are absent here);
//emission is instance-driven, so an entry that is never present never fires.
//instanceCount (91) is left 0 for the caller to fill. entityFlag 0 = object.
//Arbitrary/proprietary objects not in this table still round-trip losslessly
//LibreCAD<->LibreCAD but get no CLASS (a heuristic proxy is a deliberate TODO,
//since distinguishing a custom class from an unmodeled fixed type is unsafe).
bool dxfRW::dxfClassForRecordName(const std::string &recName, DRW_Class &out) {
    struct Entry { const char *rec; const char *cls; const char *app; int flag; int isEntity; };
    static const Entry table[] = {
        // Routed data-only OBJECTS (also captured into the raw net on read).
        {"SUN",              "AcDbSun",                 "SCENEOE",           1153, 0},
        {"SCALE",            "AcDbScale",               "ObjectDBX Classes", 1153, 0},
        {"DICTIONARYVAR",    "AcDbDictionaryVar",       "ObjectDBX Classes", 0, 0},
        {"RASTERVARIABLES",  "AcDbRasterVariables",     "ISM",               0, 0},
        {"WIPEOUTVARIABLES", "AcDbWipeoutVariables",    "WipeOut",           0, 0},
        {"PDFDEFINITION",    "AcDbPdfDefinition",       "ObjectDBX Classes", 1153, 0},
        {"DGNDEFINITION",    "AcDbDgnDefinition",       "ObjectDBX Classes", 1153, 0},
        {"DWFDEFINITION",    "AcDbDwfDefinition",       "ObjectDBX Classes", 1153, 0},
        // Common unmodeled custom OBJECTS that reach the raw net verbatim.
        {"MATERIAL",         "AcDbMaterial",            "ObjectDBX Classes", 1153, 0},
        {"VISUALSTYLE",      "AcDbVisualStyle",         "ObjectDBX Classes", 4095, 0},
        {"TABLESTYLE",       "AcDbTableStyle",          "ObjectDBX Classes", 4095, 0},
        {"MLEADERSTYLE",     "AcDbMLeaderStyle", "ACDB_MLEADERSTYLE_CLASS", 4095, 0},
        {"ACDBDETAILVIEWSTYLE",  "AcDbDetailViewStyle",  "ObjectDBX Classes", 1025, 0},
        {"DETAILVIEWSTYLE",      "AcDbDetailViewStyle",  "ObjectDBX Classes", 1025, 0},
        {"ACDBSECTIONVIEWSTYLE", "AcDbSectionViewStyle", "ObjectDBX Classes", 1025, 0},
        {"SECTIONVIEWSTYLE",     "AcDbSectionViewStyle", "ObjectDBX Classes", 1025, 0},
        {"ACDBPLACEHOLDER",  "AcDbPlaceHolder",         "ObjectDBX Classes", 0, 0},
        {"CELLSTYLEMAP",     "AcDbCellStyleMap",        "ObjectDBX Classes", 1152, 0},
        {"FIELD",            "AcDbField",               "ObjectDBX Classes", 1152, 0},
        {"FIELDLIST",        "AcDbFieldList",           "ObjectDBX Classes", 1152, 0},
        {"GEODATA",          "AcDbGeoData",             "ObjectDBX Classes", 4095, 0},
        {"SPATIAL_FILTER",   "AcDbSpatialFilter",       "ObjectDBX Classes", 0, 0},
        {"SORTENTSTABLE",    "AcDbSortentsTable",       "ObjectDBX Classes", 0, 0},
        {"IDBUFFER",         "AcDbIdBuffer",            "ObjectDBX Classes", 0, 0},
        {"LAYER_INDEX",      "AcDbLayerIndex",          "ObjectDBX Classes", 0, 0},
        {"SPATIAL_INDEX",    "AcDbSpatialIndex",        "ObjectDBX Classes", 0, 0},
        {"DIMASSOC",         "AcDbDimAssoc",            "AcDbDimAssoc",      0, 0},
        // Custom ENTITIES (isEntity=1). Typed direct writers and raw-net replay
        // both need these CLASS records; without them AutoCAD/ODA prune the
        // entities on load.
        {"ACAD_TABLE",       "AcDbTable",               "ObjectDBX Classes", 1025, 1},
        {"HELIX",            "AcDbHelix",               "ObjectDBX Classes", 4095, 1},
        {"MESH",             "AcDbSubDMesh",            "SCENEOE",           1025, 1},
        {"RTEXT",            "AcDbRText",               "EXPRESS",           1025, 1},
        {"ARCALIGNEDTEXT",   "AcDbArcAlignedText",      "EXPRESS",           1025, 1},
        {"MPOLYGON",         "AcDbMPolygon",            "AcMPolygonObj15",   1025, 1},
        {"LARGE_RADIAL_DIMENSION", "AcDbRadialDimensionLarge", "ACAD",       1025, 1},
        {"PDFUNDERLAY",      "AcDbPdfReference",        "ObjectDBX Classes", 4095, 1},
        {"DGNUNDERLAY",      "AcDbDgnReference",        "ObjectDBX Classes", 4095, 1},
        {"DWFUNDERLAY",      "AcDbDwfReference",        "ObjectDBX Classes", 4095, 1},
        {"SURFACE",          "AcDbSurface",             "ObjectDBX Classes", 4095, 1},
        {"EXTRUDEDSURFACE",  "AcDbExtrudedSurface",     "ObjectDBX Classes", 4095, 1},
        {"LOFTEDSURFACE",    "AcDbLoftedSurface",       "ObjectDBX Classes", 0, 1},
        {"REVOLVEDSURFACE",  "AcDbRevolvedSurface",     "ObjectDBX Classes", 0, 1},
        {"SWEPTSURFACE",     "AcDbSweptSurface",        "ObjectDBX Classes", 0, 1},
        {"PLANESURFACE",     "AcDbPlaneSurface",        "ObjectDBX Classes", 4095, 1},
        {"NURBSSURFACE",     "AcDbNurbSurface",         "ObjectDBX Classes", 4095, 1},
        {"POINTCLOUD",       "AcDbPointCloud",          "ObjectDBX Classes", 4095, 1},
        {"POINTCLOUDEX",     "AcDbPointCloudEx",        "ObjectDBX Classes", 4095, 1},
    };
    for (const Entry &e : table) {
        if (recName == e.rec) {
            out.recName = e.rec;
            out.className = e.cls;
            out.appName = e.app;
            out.proxyFlag = e.flag;
            out.wasaProxyFlag = 0;
            out.entityFlag = e.isEntity;
            out.instanceCount = 0;
            return true;
        }
    }
    return false;
}

bool dxfRW::writePlotSettings(DRW_PlotSettings *ent) {
    writer->writeString(0, "PLOTSETTINGS");
    writer->writeString(5, toHexStr(static_cast<int>(m_handleAllocator.next())));
    if (version > DRW::AC1014) {
        writer->writeString(330, "C");  //owner: root dict (avoids ownerless prune)
    }
    writer->writeString(100, "AcDbPlotSettings");
    writePlotSettingsFields(ent);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

void dxfRW::writePlotSettingsFields(const DRW_PlotSettings *ent) {
    // Full AcDbPlotSettings field set in ezdxf layout.py order. Previously only
    // 6/40/41/42/43 were emitted, so page size, margins, plot window, scale,
    // rotation, units and shade-plot settings were all lost on export.
    writer->writeUtf8String(1, ent->pageSetupName);
    writer->writeUtf8String(2, ent->printerConfig);
    writer->writeUtf8String(4, ent->paperSize);
    writer->writeUtf8String(6, ent->plotViewName);
    writer->writeDouble(40, ent->marginLeft);
    writer->writeDouble(41, ent->marginBottom);
    writer->writeDouble(42, ent->marginRight);
    writer->writeDouble(43, ent->marginTop);
    writer->writeDouble(44, ent->paperWidth);
    writer->writeDouble(45, ent->paperHeight);
    writer->writeDouble(46, ent->plotOriginX);
    writer->writeDouble(47, ent->plotOriginY);
    writer->writeDouble(48, ent->windowMinX);
    writer->writeDouble(49, ent->windowMinY);
    writer->writeDouble(140, ent->windowMaxX);
    writer->writeDouble(141, ent->windowMaxY);
    writer->writeDouble(142, ent->realWorldUnits);
    writer->writeDouble(143, ent->drawingUnits);
    writer->writeInt16(70, ent->plotLayoutFlags);
    writer->writeInt16(72, ent->paperUnits);
    writer->writeInt16(73, ent->plotRotation);
    writer->writeInt16(74, ent->plotType);
    writer->writeUtf8String(7, ent->currentStyleSheet);
    writer->writeInt16(75, ent->scaleType);
    if (version > DRW::AC1015) {   // shade-plot settings are R2004+
        writer->writeInt16(76, ent->shadePlotMode);
        writer->writeInt16(77, ent->shadePlotResLevel);
        writer->writeInt16(78, ent->shadePlotCustomDPI);
    }
    writer->writeDouble(147, ent->scaleFactor);
    writer->writeDouble(148, ent->paperImageOriginX);
    writer->writeDouble(149, ent->paperImageOriginY);
}

// LAYOUT is a fixed built-in OBJECT. It embeds the same AcDbPlotSettings prefix
// as PLOTSETTINGS, followed by AcDbLayout fields.
bool dxfRW::writeLayout(DRW_Layout *ent) {
    const std::uint32_t handle = ent->handle != 0
        ? static_cast<std::uint32_t>(ent->handle)
        : m_handleAllocator.next();
    writer->writeString(0, "LAYOUT");
    writer->writeString(5, toHexStr(static_cast<int>(handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbPlotSettings");
    DRW_PlotSettings plotSettings;
    plotSettings.pageSetupName = ent->pageSetupName;
    plotSettings.printerConfig = ent->printerConfig;
    plotSettings.plotLayoutFlags = ent->plotLayoutFlags;
    plotSettings.marginLeft = ent->marginLeft;
    plotSettings.marginBottom = ent->marginBottom;
    plotSettings.marginRight = ent->marginRight;
    plotSettings.marginTop = ent->marginTop;
    plotSettings.paperWidth = ent->paperWidth;
    plotSettings.paperHeight = ent->paperHeight;
    plotSettings.paperSize = ent->paperSize;
    plotSettings.plotOriginX = ent->plotOriginX;
    plotSettings.plotOriginY = ent->plotOriginY;
    plotSettings.paperUnits = ent->paperUnits;
    plotSettings.plotRotation = ent->plotRotation;
    plotSettings.plotType = ent->plotType;
    plotSettings.windowMinX = ent->windowMinX;
    plotSettings.windowMinY = ent->windowMinY;
    plotSettings.windowMaxX = ent->windowMaxX;
    plotSettings.windowMaxY = ent->windowMaxY;
    plotSettings.plotViewName = ent->plotViewName;
    plotSettings.realWorldUnits = ent->realWorldUnits;
    plotSettings.drawingUnits = ent->drawingUnits;
    plotSettings.currentStyleSheet = ent->currentStyleSheet;
    plotSettings.scaleType = ent->scaleType;
    plotSettings.scaleFactor = ent->scaleFactor;
    plotSettings.paperImageOriginX = ent->paperImageOriginX;
    plotSettings.paperImageOriginY = ent->paperImageOriginY;
    plotSettings.shadePlotMode = ent->shadePlotMode;
    plotSettings.shadePlotResLevel = ent->shadePlotResLevel;
    plotSettings.shadePlotCustomDPI = ent->shadePlotCustomDPI;
    writePlotSettingsFields(&plotSettings);

    writer->writeString(100, "AcDbLayout");
    writer->writeUtf8String(1, ent->name);
    writer->writeInt16(70, ent->layoutFlags);
    writer->writeInt32(71, ent->tabOrder);
    writer->writeDouble(10, ent->limMinX);
    writer->writeDouble(20, ent->limMinY);
    writer->writeDouble(11, ent->limMaxX);
    writer->writeDouble(21, ent->limMaxY);
    writer->writeDouble(12, ent->insPoint.x);
    writer->writeDouble(22, ent->insPoint.y);
    writer->writeDouble(32, ent->insPoint.z);
    writer->writeDouble(14, ent->extMin.x);
    writer->writeDouble(24, ent->extMin.y);
    writer->writeDouble(34, ent->extMin.z);
    writer->writeDouble(15, ent->extMax.x);
    writer->writeDouble(25, ent->extMax.y);
    writer->writeDouble(35, ent->extMax.z);
    writer->writeDouble(146, ent->elevation);
    writer->writeDouble(13, ent->ucsOrigin.x);
    writer->writeDouble(23, ent->ucsOrigin.y);
    writer->writeDouble(33, ent->ucsOrigin.z);
    writer->writeDouble(16, ent->ucsXAxis.x);
    writer->writeDouble(26, ent->ucsXAxis.y);
    writer->writeDouble(36, ent->ucsXAxis.z);
    writer->writeDouble(17, ent->ucsYAxis.x);
    writer->writeDouble(27, ent->ucsYAxis.y);
    writer->writeDouble(37, ent->ucsYAxis.z);
    writer->writeInt16(76, ent->orthoViewType);
    if (ent->paperSpaceBlockRecordHandle.ref != 0)
        writer->writeString(
            330, toHexStr(static_cast<int>(ent->paperSpaceBlockRecordHandle.ref)));
    if (ent->lastActiveViewportHandle.ref != 0)
        writer->writeString(
            331, toHexStr(static_cast<int>(ent->lastActiveViewportHandle.ref)));
    if (ent->namedUcsHandle.ref != 0)
        writer->writeString(
            345, toHexStr(static_cast<int>(ent->namedUcsHandle.ref)));
    if (ent->baseUcsHandle.ref != 0)
        writer->writeString(
            346, toHexStr(static_cast<int>(ent->baseUcsHandle.ref)));
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

//F4: typed DXF emitters for the routed data-only OBJECTS that the DWG reader
//populates only into typed metadata (NOT the DXF raw net). On DWG->DXF the filter
//pulls each from dwgAdvancedMetadata().suns()/scales()/dictionaryVars()/
//rasterVariables() and calls these so the object is present in the output (DXF->DXF
//already preserves them via the raw net; the filter dedups by handle to avoid a
//double-emit). The group-code shape is the inverse of each type's parseCode
//(DRW_Sun/Scale/DictionaryVar/RasterVariables::parseCode in drw_objects.cpp),
//cross-checked field-for-field against ezdxf 1.4.4. Each emits the verbatim
//code-5 handle (reserved by the filter's pre-write pass) and a 330 owner (the
//record's parentHandle when known, else root dict "C" to avoid an ownerless
//prune). The matching CLASS record is registered by the filter via
//dxfClassForRecordName.

//helper: emit 330 owner as a hex handle (record parentHandle when nonzero, else
//root dict "C" so the object is reachable and not pruned as an orphan).
void dxfRW::writeObjectOwner(std::uint32_t parentHandle) {
    if (version <= DRW::AC1014)
        return;  //pre-R2000 DXF has no 330 owner handles in OBJECTS
    if (parentHandle != 0)
        writer->writeString(330, toHexStr(static_cast<int>(parentHandle)));
    else
        writer->writeString(330, "C");
}

bool dxfRW::writeSun(DRW_Sun *ent) {
    writer->writeString(0, "SUN");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbSun");
    writer->writeInt32(90, static_cast<int>(ent->m_classVersion));
    writer->writeBool(290, ent->m_isOn);
    writer->writeInt16(63, static_cast<int>(ent->m_color));
    if (version > DRW::AC1015 && ent->m_color24 >= 0)
        writer->writeInt32(421, ent->m_color24);  // 24-bit true color (R2004+)
    writer->writeDouble(40, ent->m_intensity);
    writer->writeBool(291, ent->m_hasShadow);
    writer->writeInt32(91, ent->m_julianDay);
    writer->writeInt32(92, ent->m_milliseconds);
    writer->writeBool(292, ent->m_isDaylightSavings);
    writer->writeInt16(70, static_cast<int>(ent->m_shadowType));
    writer->writeInt16(71, static_cast<int>(ent->m_shadowMapSize));
    writer->writeInt16(280, static_cast<int>(ent->m_shadowSoftness));
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeScale(DRW_Scale *ent) {
    writer->writeString(0, "SCALE");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbScale");
    writer->writeInt16(70, static_cast<int>(ent->flag));
    writer->writeUtf8String(300, ent->name);
    writer->writeDouble(140, ent->paperUnits);
    writer->writeDouble(141, ent->drawingUnits);
    writer->writeBool(290, ent->isUnitScale);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeDictionaryVar(DRW_DictionaryVar *ent) {
    writer->writeString(0, "DICTIONARYVAR");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    //DICTIONARYVAR uses the literal subclass marker "DictionaryVariables"
    //(NOT "AcDbDictionaryVar"); confirmed against ezdxf 1.4.4.
    writer->writeString(100, "DictionaryVariables");
    writer->writeInt16(280, static_cast<int>(ent->m_schema));
    writer->writeUtf8String(1, ent->m_value);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

bool dxfRW::writeRasterVariables(DRW_RasterVariables *ent) {
    writer->writeString(0, "RASTERVARIABLES");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbRasterVariables");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeInt16(70, ent->m_imageFrame);
    writer->writeInt16(71, ent->m_imageQuality);
    writer->writeInt16(72, ent->m_units);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// GEODATA (AcDbGeoData, custom class). Inverse of DRW_GeoData::parseCode for
// scalar geolocation fields and mesh point/face lists.
bool dxfRW::writeGeoData(DRW_GeoData *ent) {
    writer->writeString(0, "GEODATA");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbGeoData");
    writer->writeInt32(90, ent->m_version != 0 ? ent->m_version : 3);
    if (ent->m_hostBlockHandle != 0)
        writer->writeString(330, toHexStr(static_cast<int>(ent->m_hostBlockHandle)));
    writer->writeInt16(70, ent->m_coordinatesType);
    writer->writeDouble(10, ent->m_designPoint.x);
    writer->writeDouble(20, ent->m_designPoint.y);
    writer->writeDouble(30, ent->m_designPoint.z);
    writer->writeDouble(11, ent->m_referencePoint.x);
    writer->writeDouble(21, ent->m_referencePoint.y);
    writer->writeDouble(31, ent->m_referencePoint.z);
    writer->writeDouble(40, ent->m_horizontalUnitScale);
    writer->writeDouble(41, ent->m_verticalUnitScale);
    writer->writeInt32(91, ent->m_horizontalUnits);
    writer->writeInt32(92, ent->m_verticalUnits);
    writer->writeDouble(210, ent->m_upDirection.x);
    writer->writeDouble(220, ent->m_upDirection.y);
    writer->writeDouble(230, ent->m_upDirection.z);
    writer->writeDouble(12, ent->m_northDirection.x);
    writer->writeDouble(22, ent->m_northDirection.y);
    writer->writeInt32(95, ent->m_scaleEstimationMethod);
    writer->writeDouble(141, ent->m_userSpecifiedScaleFactor);
    writer->writeBool(294, ent->m_enableSeaLevelCorrection);
    writer->writeDouble(142, ent->m_seaLevelElevation);
    writer->writeDouble(143, ent->m_coordinateProjectionRadius);

    std::string definition = ent->m_coordinateSystemDefinition;
    for (std::string::size_type pos = definition.find('\n');
         pos != std::string::npos;
         pos = definition.find('\n', pos + 2)) {
        definition.replace(pos, 1, "^J");
    }
    std::string::size_type offset = 0;
    while (definition.size() - offset > 255) {
        writer->writeUtf8String(303, definition.substr(offset, 255));
        offset += 255;
    }
    writer->writeUtf8String(301, definition.substr(offset));
    writer->writeUtf8String(302, ent->m_geoRssTag);
    writer->writeUtf8String(305, ent->m_observationFromTag);
    writer->writeUtf8String(306, ent->m_observationToTag);
    writer->writeUtf8String(307, ent->m_observationCoverageTag);

    writer->writeInt32(93, static_cast<int>(ent->m_points.size()));
    for (const DRW_GeoMeshPoint &point : ent->m_points) {
        writer->writeDouble(13, point.m_source.x);
        writer->writeDouble(23, point.m_source.y);
        writer->writeDouble(14, point.m_destination.x);
        writer->writeDouble(24, point.m_destination.y);
    }
    writer->writeInt32(96, static_cast<int>(ent->m_faces.size()));
    for (const DRW_GeoMeshFace &face : ent->m_faces) {
        writer->writeInt32(97, face.m_index1);
        writer->writeInt32(98, face.m_index2);
        writer->writeInt32(99, face.m_index3);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// SPATIAL_FILTER (AcDbSpatialFilter, custom class). Emits the clip boundary,
// normal/origin, clip flags/distances, and boundary-relative transforms.
bool dxfRW::writeSpatialFilter(DRW_SpatialFilter *ent) {
    writer->writeString(0, "SPATIAL_FILTER");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbFilter");
    writer->writeString(100, "AcDbSpatialFilter");
    writer->writeInt16(70, static_cast<int>(ent->m_boundaryPoints.size()));
    for (const DRW_Coord &point : ent->m_boundaryPoints) {
        writer->writeDouble(10, point.x);
        writer->writeDouble(20, point.y);
    }
    writer->writeDouble(210, ent->m_normal.x);
    writer->writeDouble(220, ent->m_normal.y);
    writer->writeDouble(230, ent->m_normal.z);
    writer->writeDouble(11, ent->m_origin.x);
    writer->writeDouble(21, ent->m_origin.y);
    writer->writeDouble(31, ent->m_origin.z);
    writer->writeInt16(71, ent->m_displayBoundary ? 1 : 0);
    writer->writeInt16(72, ent->m_clipFrontPlane ? 1 : 0);
    if (ent->m_clipFrontPlane)
        writer->writeDouble(40, ent->m_frontDistance);
    writer->writeInt16(73, ent->m_clipBackPlane ? 1 : 0);
    if (ent->m_clipBackPlane)
        writer->writeDouble(41, ent->m_backDistance);

    auto writeMatrix12 = [this](const std::vector<double> &matrix) {
        for (std::size_t i = 0; i < 12; ++i)
            writer->writeDouble(40, i < matrix.size() ? matrix[i] : 0.0);
    };
    writeMatrix12(ent->m_inverseInsertTransform);
    writeMatrix12(ent->m_insertTransform);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// SORTENTSTABLE (AcDbSortentsTable, custom class). Entity handles are remapped
// through the source->minted map when this is emitted after ENTITIES; direct unit
// writers with an empty map preserve the caller-provided handles verbatim.
bool dxfRW::writeSortEntsTable(DRW_SortEntsTable *ent) {
    writer->writeString(0, "SORTENTSTABLE");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbSortentsTable");
    const std::uint32_t blockOwner = ent->m_blockOwnerHandle != 0
        ? ent->m_blockOwnerHandle
        : 0x1Fu;
    writer->writeString(330, toHexStr(static_cast<int>(blockOwner)));

    const auto &srcToMinted = m_writingContext.sourceHandleToMintedMap;
    const bool remapEntities = !srcToMinted.empty();
    auto resolveEntity = [&](std::uint32_t source, std::uint32_t &resolved) {
        if (source == 0)
            return false;
        if (!remapEntities) {
            resolved = source;
            return true;
        }
        auto it = srcToMinted.find(source);
        if (it == srcToMinted.end())
            return false;
        resolved = it->second;
        return true;
    };
    auto resolveSort = [&](std::uint32_t source, std::uint32_t fallback) {
        if (source == 0)
            return fallback;
        if (!remapEntities)
            return source;
        auto it = srcToMinted.find(source);
        return it != srcToMinted.end() ? it->second : source;
    };

    const std::size_t entryCount = std::max(
        ent->m_entityHandles.size(), ent->m_sortHandles.size());
    for (std::size_t i = 0; i < entryCount; ++i) {
        const std::uint32_t entitySource =
            i < ent->m_entityHandles.size() ? ent->m_entityHandles[i] : 0;
        std::uint32_t entityHandle = 0;
        if (!resolveEntity(entitySource, entityHandle))
            continue;
        const std::uint32_t sortSource =
            i < ent->m_sortHandles.size() ? ent->m_sortHandles[i] : entitySource;
        const std::uint32_t sortHandle = resolveSort(sortSource, entityHandle);
        writer->writeString(331, toHexStr(static_cast<int>(entityHandle)));
        writer->writeString(5, toHexStr(static_cast<int>(sortHandle)));
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// FIELD (AcDbField, custom class). Emits the scalar field state, child/object
// references, cached value string, and child value records preserved by the
// typed FIELD model.
bool dxfRW::writeField(DRW_Field *ent) {
    writer->writeString(0, "FIELD");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbField");
    writer->writeUtf8String(1, ent->m_evaluatorId);
    writer->writeUtf8String(2, ent->m_fieldCode);
    if (!ent->m_formatString.empty())
        writer->writeUtf8String(4, ent->m_formatString);
    writer->writeInt32(90, static_cast<int>(ent->m_childHandles.size()));
    for (std::uint32_t child : ent->m_childHandles)
        writer->writeString(360, toHexStr(static_cast<int>(child)));
    writer->writeInt32(97, static_cast<int>(ent->m_objectHandles.size()));
    for (std::uint32_t object : ent->m_objectHandles)
        writer->writeString(331, toHexStr(static_cast<int>(object)));
    writer->writeInt32(91, ent->m_evaluationOptionFlags);
    writer->writeInt32(92, ent->m_filingOptionFlags);
    writer->writeInt32(94, ent->m_fieldStateFlags);
    writer->writeInt32(95, ent->m_evaluationStatusFlags);
    writer->writeInt32(96, ent->m_evaluationErrorCode);
    writer->writeUtf8String(300, ent->m_evaluationErrorMessage);
    writer->writeInt32(93, static_cast<int>(ent->m_childValues.size()));

    auto writeChildValue = [this](const DRW_Field::ChildValue &child) {
        writer->writeUtf8String(6, child.m_key);
        writer->writeInt32(93, child.m_value.m_formatFlags);
        writer->writeInt32(90, child.m_value.m_dataType);
        switch (child.m_value.m_dataType) {
        case 0:
        case 1:
            writer->writeInt32(91, child.m_value.m_value.i_val());
            break;
        case 2:
            writer->writeDouble(140, child.m_value.m_value.d_val());
            break;
        case 4:
        case 512: {
            std::string valueText = child.m_value.m_valueString;
            if (valueText.empty()
                && child.m_value.m_value.type() == DRW_Variant::STRING
                && child.m_value.m_value.c_str() != nullptr) {
                valueText = child.m_value.m_value.c_str();
            }
            writer->writeUtf8String(300, valueText);
            break;
        }
        case 64:
            if (child.m_value.m_handle != 0)
                writer->writeString(
                    330, toHexStr(static_cast<int>(child.m_value.m_handle)));
            break;
        default:
            break;
        }
        writer->writeInt32(94, child.m_value.m_unitType);
        writer->writeUtf8String(302, child.m_value.m_formatString);
        writer->writeUtf8String(304, "ACVALUE_END");
    };
    for (const DRW_Field::ChildValue &child : ent->m_childValues)
        writeChildValue(child);

    writer->writeUtf8String(301, ent->m_valueString);
    writer->writeInt32(98, ent->m_valueStringLength);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// FIELDLIST (AcDbIdSet / AcDbFieldList, custom class).
bool dxfRW::writeFieldList(DRW_FieldList *ent) {
    writer->writeString(0, "FIELDLIST");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbIdSet");
    writer->writeInt32(90, static_cast<int>(ent->m_fieldHandles.size()));
    writer->writeBool(290, ent->m_unknown != 0);
    for (std::uint32_t field : ent->m_fieldHandles)
        writer->writeString(330, toHexStr(static_cast<int>(field)));
    writer->writeString(100, "AcDbFieldList");
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

// MLEADERSTYLE (AcDbMLeaderStyle, custom class). Inverse of
// DRW_MLeaderStyle::parseCode: common table-entry name/flags, scalar style
// fields, and handle references 340-343.
bool dxfRW::writeMLeaderStyle(DRW_MLeaderStyle *ent) {
    if (version < DRW::AC1021)
        return false;
    writer->writeString(0, "MLEADERSTYLE");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbMLeaderStyle");
    writer->writeUtf8String(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeInt32(179, ent->styleVersion);
    writer->writeInt32(170, ent->contentType);
    writer->writeInt32(171, ent->drawMLeaderOrder);
    writer->writeInt32(172, ent->drawLeaderOrder);
    writer->writeInt32(90, ent->maxLeaderPoints);
    writer->writeDouble(40, ent->firstSegmentAngle);
    writer->writeDouble(41, ent->secondSegmentAngle);
    writer->writeInt32(173, ent->leaderType);
    writer->writeInt32(91, ent->leaderColor);
    if (ent->leaderLineTypeHandle.ref != 0)
        writer->writeString(340, toHexStr(static_cast<int>(ent->leaderLineTypeHandle.ref)));
    writer->writeInt32(92, ent->leaderLineWeight);
    writer->writeBool(290, ent->landingEnabled);
    writer->writeDouble(42, ent->landingGap);
    writer->writeBool(291, ent->autoIncludeLanding);
    writer->writeDouble(43, ent->landingDistance);
    writer->writeUtf8String(3, ent->description);
    if (ent->arrowHeadBlockHandle.ref != 0)
        writer->writeString(341, toHexStr(static_cast<int>(ent->arrowHeadBlockHandle.ref)));
    writer->writeDouble(44, ent->arrowHeadSize);
    writer->writeUtf8String(300, ent->textDefault);
    if (ent->textStyleHandle.ref != 0)
        writer->writeString(342, toHexStr(static_cast<int>(ent->textStyleHandle.ref)));
    writer->writeInt32(174, ent->leftAttachment);
    writer->writeInt32(178, ent->rightAttachment);
    writer->writeInt32(175, ent->textAngleType);
    writer->writeInt32(176, ent->textAlignmentType);
    writer->writeInt32(93, ent->textColor);
    writer->writeDouble(45, ent->textHeight);
    writer->writeBool(292, ent->textFrameEnabled);
    writer->writeBool(297, ent->alwaysAlignTextLeft);
    writer->writeDouble(46, ent->alignSpace);
    if (ent->blockHandle.ref != 0)
        writer->writeString(343, toHexStr(static_cast<int>(ent->blockHandle.ref)));
    writer->writeInt32(94, ent->blockColor);
    writer->writeDouble(47, ent->blockScale.x);
    writer->writeDouble(49, ent->blockScale.y);
    writer->writeDouble(140, ent->blockScale.z);
    writer->writeBool(293, ent->blockScaleEnabled);
    writer->writeDouble(141, ent->blockRotation);
    writer->writeBool(294, ent->blockRotationEnabled);
    writer->writeInt32(177, ent->blockConnectionType);
    writer->writeDouble(142, ent->scaleFactor);
    writer->writeBool(295, ent->propertyChanged);
    writer->writeBool(296, ent->isAnnotative);
    writer->writeDouble(143, ent->breakSize);
    writer->writeInt32(271, ent->attachmentDirection);
    writer->writeInt32(273, ent->topAttachment);
    writer->writeInt32(272, ent->bottomAttachment);
    writer->writeBool(298, ent->textExtended);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

//MLINESTYLE is a FIXED built-in (no CLASS record). The group-code shape is the
//inverse of DRW_MLineStyle::parseCode (drw_objects.cpp): name 2, flags 70,
//description 3, fill color 62 (before any element), start/end angle 51/52,
//element count 71, then per element offset 49 / color 62 / linetype 6.
//Cross-checked field-for-field against ezdxf 1.4.4 (AcDbMlineStyle).
bool dxfRW::writeMLineStyle(DRW_MLineStyle *ent) {
    writer->writeString(0, "MLINESTYLE");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbMlineStyle");
    writer->writeUtf8String(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeUtf8String(3, ent->description);
    writer->writeInt16(62, ent->fillColor);
    writer->writeDouble(51, ent->startAngle);
    writer->writeDouble(52, ent->endAngle);
    writer->writeInt16(71, static_cast<int>(ent->elements.size()));
    for (const DRW_MLineElement &el : ent->elements) {
        writer->writeDouble(49, el.offset);
        writer->writeInt16(62, el.color);
        writer->writeUtf8String(6, el.linetype.empty() ? "BYLAYER" : el.linetype);
    }
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

//WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class). Inverse of
//DRW_WipeoutVariables::parseCode: only the global display-frame flag (DXF 70).
bool dxfRW::writeWipeoutVariables(DRW_WipeoutVariables *ent) {
    writer->writeString(0, "WIPEOUTVARIABLES");
    writer->writeString(5, toHexStr(static_cast<int>(ent->handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbWipeoutVariables");
    writer->writeInt16(70, ent->m_displayFrame);
    if (!ent->extData.empty())
        writeExtData(ent->extData);
    return true;
}

/** utility function
 * convert a int to string in hex
 **/
std::string dxfRW::toHexStr(int n){
#if defined(__APPLE__)
    char buffer[9]= {'\0'};
    snprintf(buffer,9, "%X", n);
    return std::string(buffer);
#else
    std::ostringstream Convert;
    Convert << std::uppercase << std::hex << n;
    return Convert.str();
#endif
}

DRW::Version dxfRW::getVersion() const {
    return version;
}

DRW::error dxfRW::getError() const
{
    return error;
}

bool dxfRW::setError(const DRW::error lastError)
{
    error = lastError;
    return (DRW::BAD_NONE == error);
}
