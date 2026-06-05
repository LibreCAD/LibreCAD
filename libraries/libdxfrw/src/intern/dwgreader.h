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

#ifndef DWGREADER_H
#define DWGREADER_H

#include <unordered_map>
#include <list>
#include <memory>
#include <vector>
#include "drw_textcodec.h"
#include "dwgutil.h"
#include "dwgbuffer.h"
#include "../libdwgr.h"
#include "../drw_entities.h"

class objHandle{
public:
    objHandle()=default;
    objHandle(std::uint32_t t, std::uint32_t h, std::uint32_t l)
        :type{t}
        ,handle{h}
        ,loc{l}
    {}
    std::uint32_t type{0};
    std::uint32_t handle{0};
    std::uint32_t loc{0};
};

//until 2000 = 2000-
//since 2004 except 2007 = 2004+
// 2007 = 2007
// pages of section
/* 2000-: No pages, only sections
 * 2004+: Id, page number (index)
 *        size, size of page in file stream
 *        address, address in file stream
 *        dataSize, data size for this page
 *        startOffset, start offset for this page
 *        cSize, compressed size of data
 *        uSize, uncompressed size of data
 * 2007: page Id, pageCount & pages
 *       size, size in file
 *       dataSize
 *       startOffset, start position in decompressed data stream
 *       cSize, compressed size of data
 *       uSize, uncompressed size of data
 *       address, address in file stream
 * */
class dwgPageInfo {
public:
    dwgPageInfo()=default;
    dwgPageInfo(std::uint64_t i, std::uint64_t ad, std::uint64_t sz){
        Id=i; address=ad; size=sz;
    }
    std::uint64_t Id{0};
    std::uint64_t address{0}; //in file stream, for rd18, rd21
    std::uint64_t size{0}; //in file stream, for rd18, rd21
    std::uint64_t dataSize{0}; //for rd18, rd21
    std::uint64_t startOffset{0}; //for rd18, rd21
    std::uint64_t cSize{0}; //compressed page size, for rd21
    std::uint64_t uSize{0}; //uncompressed page size, for rd21
};

// sections of file
/* 2000-: No pages, only section Id, size  & address in file
 * 2004+: Id, Section Id
 *        size, total size of uncompressed data
 *        pageCount & pages, number of pages in section
 *        maxSize, max decompressed Size per page
 *        compressed, (1 = no, 2 = yes, normally 2)
 *        encrypted, (0 = no, 1 = yes, 2 = unknown)
 *        name, read & stored but not used
 * 2007: same as 2004+ except encoding, saved in compressed field
 * */
class dwgSectionInfo {
public:
    dwgSectionInfo()=default;
    std::int32_t Id{-1}; //section Id, 2000-   rd15 rd18
    std::string name; //section name rd18
    std::uint32_t compressed{1};//is compressed? 1=no, 2=yes rd18, rd21(encoding)
    std::uint32_t encrypted{0};//encrypted (doc: 0=no, 1=yes, 2=unkn) on read: objects 0 and encrypted yes rd18
    std::unordered_map<std::uint32_t, dwgPageInfo >pages;//index, size, offset
    std::uint64_t size{0};//size of section,  2000- rd15, rd18, rd21 (data size)
    std::uint64_t pageCount{0}; //number of pages (dwgPageInfo) in section rd18, rd21
    std::uint64_t maxSize{0}; //max decompressed size (needed??) rd18 rd21
    std::uint64_t address{0}; //address (seek) , 2000-
};


//! Class to handle dwg obj control entries
/*!
*  Class to handle dwg obj control entries
*  @author Rallaz
*/
class DRW_ObjControl : public DRW_TableEntry {
public:
    DRW_ObjControl() { reset();}

    // hmm-- is DRW_TableEntry::reset() intended to be virtual??
    void reset(){
    }
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    std::list<std::uint32_t>handlesList;
};


class dwgReader {
    // friend the real class (not the legacy `using dwgR = dwgRW;` alias —
    // C++ does not allow `friend class <typedef-name>;`).
    friend class dwgRW;
public:
    dwgReader(std::ifstream *stream, dwgRW *p)
       :fileBuf{ new dwgBuffer(stream) }
       ,parent{p}
    {
        decoder.setVersion(DRW::AC1021, false);//default 2007 in utf8(no convert)
        decoder.setCodePage("UTF-16", false);
//        blockCtrl=0; //RLZ: temporary
//        blockCtrl=layerCtrl=styleCtrl=linetypeCtrl=viewCtrl=0;
//        ucsCtrl=vportCtrl=appidCtrl=dimstyleCtrl=vpEntHeaderCtrl=0;
    }
    virtual ~dwgReader();

protected:
    virtual bool readMetaData() = 0;
    virtual bool readPreview(){return false;}
    virtual bool readFileHeader() = 0;
    virtual bool readDwgHeader(DRW_Header& hdr)=0;
    virtual bool readDwgClasses() = 0;
    virtual bool readDwgHandles() = 0;
    virtual bool readDwgTables(DRW_Header& hdr)=0;
    virtual bool readDwgBlocks(DRW_Interface& intfa) = 0;
    virtual bool readDwgEntities(DRW_Interface& intfa) = 0;
    virtual bool readDwgObjects(DRW_Interface& intfa) = 0;

    virtual bool readDwgEntity(dwgBuffer *dbuf, objHandle& obj, DRW_Interface& intfa);
    bool readDwgObject(dwgBuffer *dbuf, objHandle& obj, DRW_Interface& intfa);
    void parseAttribs(DRW_Entity* e);
    std::string findTableName(DRW::TTYPE table, std::int32_t handle);

    void setCodePage(const std::string &c){decoder.setCodePage(c, false);}
    std::string getCodePage(){ return decoder.getCodePage();}
    bool readDwgHeader(DRW_Header& hdr, dwgBuffer *buf, dwgBuffer *hBuf);
    bool readDwgHandles(dwgBuffer *dbuf, std::uint64_t offset, std::uint64_t size);
    bool readDwgTables(DRW_Header& hdr, dwgBuffer *dbuf);
    bool checkSentinel(dwgBuffer *buf, enum secEnum::DWGSection, bool start);

    bool readDwgBlocks(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readDwgEntities(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readDwgObjects(DRW_Interface& intfa, dwgBuffer *dbuf);
    bool readPlineVertex(DRW_Polyline& pline, dwgBuffer *dbuf);
    // Walk a block_record's child entities in firstEH..lastEH chain (pre-2004)
    // or entMap order (2004+) and dispatch each via readDwgEntity.  Used for
    // both named blocks (entities go into the active block) and modelspace /
    // paperspace (called post-endBlock so entities go into the interface's
    // modelspace container).
    bool walkBlockRecordEntities(DRW_Block_Record* bkr, dwgBuffer *dbuf, DRW_Interface& intfa);

public:
    std::unordered_map<std::uint32_t, objHandle>ObjectMap;
    std::unordered_map<std::uint32_t, objHandle>objObjectMap; //stores the objects & entities not read in readDwgEntities
    std::unordered_map<std::uint32_t, objHandle>remainingMap; //stores the objects & entities not read in all processes, for debug only
    std::unordered_map<std::uint32_t, DRW_LType*> ltypemap;
    std::unordered_map<std::uint32_t, DRW_Layer*> layermap;
    std::unordered_map<std::uint32_t, DRW_Block*> blockmap;
    std::unordered_map<std::uint32_t, DRW_Textstyle*> stylemap;
    std::unordered_map<std::uint32_t, DRW_Dimstyle*> dimstylemap;
    std::unordered_map<std::uint32_t, DRW_Vport*> vportmap;
    std::unordered_map<std::uint32_t, DRW_Block_Record*> blockRecordmap;

    /// Resolved DBCOLOR (AcDbColor) lookup, populated as the OBJECTS section
    /// is decoded.  Key: handle of the AcDbColor object.  Value: pair of
    /// (24-bit RGB, display name) — entities referencing this handle (via
    /// ENC flag 0x40) get color24 + colorName patched from this map after
    /// their parseDwg returns.  Names are formatted as "BOOK$ENTRY" when a
    /// book name is present, otherwise just the entry name.
    std::unordered_map<std::uint32_t, std::pair<std::int32_t, std::string>> dbColorMap;
    /// MLINESTYLE handle → style name. Populated as MLINESTYLE objects are
    /// parsed; consumed by the entryParse template hook to stamp
    /// styleName onto each MLINE entity post-parse (DXF code 2 / DWG 340
    /// resolves to a name only after the OBJECTS section is read).
    std::unordered_map<std::uint32_t, std::string> mlineStyleNameMap;

    /// SCALE (AcDbScale) handle → entry. Populated as SCALE objects are
    /// parsed in the OBJECTS section. Foundation for per-viewport-scale
    /// resolution: annotation-scaled MLEADER/MTEXT/DIMENSION entities
    /// reference these handles via their AcDbAnnotScaleObjectContextData
    /// chain.  scaleFactor() == drawingUnits / paperUnits.
    std::unordered_map<std::uint32_t, DRW_Scale> scaleMap;

    /// Per-entity parseDwg failures accumulated across readDwgBlocks /
    /// readDwgEntities / walkBlockRecordEntities / readPlineVertex.
    /// These are reported as warnings — they do not fail the section.
    /// Section-level (structural) failures still propagate via the
    /// bool return from each section method.
    size_t m_entityParseFailures = 0;
    /// Per-object parseDwg failures accumulated in readDwgObjects. Mirrors
    /// m_entityParseFailures — non-fatal warnings tracked for caller reporting.
    size_t m_objectParseFailures = 0;
    /// R13/R15 CLASSES-section CRC mismatches (warn-only — a mismatch no longer
    /// fails the import, which previously discarded the whole drawing). Non-fatal
    /// diagnostic, surfaced via dwgRW::getClassesCrcMismatch().
    size_t m_classesCrcMismatch = 0;
    /// Custom-class entities (oType >= 500, recName not in our hardcoded
    /// dwgType map) that fell through readDwgEntity's default branch and
    /// got stuffed into objObjectMap.  Keyed by the DXF recName (eg
    /// "STDPART2D", "ACDBVISUALSTYLE", "ACMBOMROW").  These are
    /// vendor-extension entities — typically AutoCAD Mechanical / Civil
    /// proxy-capable graphics — whose geometry never reaches the
    /// renderer.  Surface to the user so they know what's missing.
    std::unordered_map<std::string, size_t> m_skippedCustomClasses;
    /// OBJECTS-section records that libdxfrw still cannot decode. Unlike
    /// m_skippedCustomClasses, this also includes non-graphical metadata such
    /// as reactors, filters, TABLECONTENT, dynamic-block graphs, etc.
    std::unordered_map<std::string, size_t> m_skippedUnsupportedObjects;
    std::unordered_map<std::uint32_t, DRW_AppId*> appIdmap;
    std::unordered_map<std::uint32_t, DRW_View*> viewmap;
    std::unordered_map<std::uint32_t, DRW_UCS*> ucsmap;

    // Buffers for ATTRIB attached-attlist routing in processDwgEntity.
    // m_pendingInserts: INSERT entities awaiting their ATTRIB children + SEQEND
    //                   before being dispatched to addInsert.  Keyed by INSERT handle.
    // m_orphanAttribs:  ATTRIB entities seen before their owning INSERT.
    //                   Keyed by parent (INSERT) handle.
    std::unordered_map<std::uint32_t, DRW_Insert> m_pendingInserts;
    std::unordered_map<std::uint32_t, std::vector<std::shared_ptr<DRW_Attrib>>> m_orphanAttribs;
//    std::uint32_t currBlock;
    std::uint8_t maintenanceVersion{0};

protected:
    std::unique_ptr<dwgBuffer> fileBuf;
    dwgRW *parent{nullptr};
    DRW::Version version{DRW::UNKNOWNV};

//seeker (position) for the beginning sentinel of the image data (R13 to R15)
    std::uint32_t previewImagePos;

//sections map
    std::unordered_map<int, dwgSectionInfo >sections;
    std::unordered_map<std::uint32_t, DRW_Class*> classesmap;

protected:
    DRW_TextCodec decoder;

protected:
//    std::uint32_t blockCtrl;
    std::uint32_t nextEntLink{0};
    std::uint32_t prevEntLink{0};

private:
    template <class T>
    bool entryParse(T &e, dwgBuffer &buff, std::uint32_t bs, bool &ret) {
        ret = e.parseDwg( version, &buff, bs);
        if (ret) {
            parseAttribs(&e);
            nextEntLink = e.nextEntLink;
            prevEntLink = e.prevEntLink;
            // Resolve AcDbColor reference (ENC flag 0x40) against the
            // OBJECTS-section DBCOLOR map populated by readDwgObject.
            // Patches color24 and colorName onto the entity for downstream
            // filters (DXF code 420 / 430).  Non-entity Ts that lack these
            // fields are still accepted because every T derives from
            // DRW_Entity which exposes them.
            if (e.acDbColorHandle != 0) {
                auto it = dbColorMap.find(e.acDbColorHandle);
                if (it != dbColorMap.end()) {
                    // color24 patched only if not already inline-set by
                    // ENC RGB (flag 0x80 path). Inline ENC name (flags
                    // 0x41/0x42) takes precedence over the DBCOLOR target's
                    // name — libreDWG model: these are entity-level
                    // overrides distinct from the DBCOLOR's own name.
                    if (it->second.first >= 0 && e.color24 == -1)
                        e.color24 = it->second.first;
                    if (!it->second.second.empty() && e.colorName.empty())
                        e.colorName = it->second.second;
                }
            }
        }

        return ret;
    }

};



#endif // DWGREADER_H
