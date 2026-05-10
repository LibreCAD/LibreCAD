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
    objHandle(duint32 t, duint32 h, duint32 l)
        :type{t}
        ,handle{h}
        ,loc{l}
    {}
    duint32 type{0};
    duint32 handle{0};
    duint32 loc{0};
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
    dwgPageInfo(duint64 i, duint64 ad, duint64 sz){
        Id=i; address=ad; size=sz;
    }
    duint64 Id;
    duint64 address; //in file stream, for rd18, rd21
    duint64 size; //in file stream, for rd18, rd21
    duint64 dataSize; //for rd18, rd21
    duint64 startOffset; //for rd18, rd21
    duint64 cSize; //compressed page size, for rd21
    duint64 uSize; //uncompressed page size, for rd21
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
    dint32 Id{-1}; //section Id, 2000-   rd15 rd18
    std::string name; //section name rd18
    duint32 compressed{1};//is compressed? 1=no, 2=yes rd18, rd21(encoding)
    duint32 encrypted{0};//encrypted (doc: 0=no, 1=yes, 2=unkn) on read: objects 0 and encrypted yes rd18
    std::unordered_map<duint32, dwgPageInfo >pages;//index, size, offset
    duint64 size;//size of section,  2000- rd15, rd18, rd21 (data size)
    duint64 pageCount{0}; //number of pages (dwgPageInfo) in section rd18, rd21
    duint64 maxSize; //max decompressed size (needed??) rd18 rd21
    duint64 address; //address (seek) , 2000-
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
    std::list<duint32>handlesList;
};


class dwgReader {
    friend class dwgR;
public:
    dwgReader(std::ifstream *stream, dwgR *p)
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
    std::string findTableName(DRW::TTYPE table, dint32 handle);

    void setCodePage(const std::string &c){decoder.setCodePage(c, false);}
    std::string getCodePage(){ return decoder.getCodePage();}
    bool readDwgHeader(DRW_Header& hdr, dwgBuffer *buf, dwgBuffer *hBuf);
    bool readDwgHandles(dwgBuffer *dbuf, duint64 offset, duint64 size);
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
    std::unordered_map<duint32, objHandle>ObjectMap;
    std::unordered_map<duint32, objHandle>objObjectMap; //stores the objects & entities not read in readDwgEntities
    std::unordered_map<duint32, objHandle>remainingMap; //stores the objects & entities not read in all processes, for debug only
    std::unordered_map<duint32, DRW_LType*> ltypemap;
    std::unordered_map<duint32, DRW_Layer*> layermap;
    std::unordered_map<duint32, DRW_Block*> blockmap;
    std::unordered_map<duint32, DRW_Textstyle*> stylemap;
    std::unordered_map<duint32, DRW_Dimstyle*> dimstylemap;
    std::unordered_map<duint32, DRW_Vport*> vportmap;
    std::unordered_map<duint32, DRW_Block_Record*> blockRecordmap;

    /// Resolved DBCOLOR (AcDbColor) lookup, populated as the OBJECTS section
    /// is decoded.  Key: handle of the AcDbColor object.  Value: pair of
    /// (24-bit RGB, display name) — entities referencing this handle (via
    /// ENC flag 0x40) get color24 + colorName patched from this map after
    /// their parseDwg returns.  Names are formatted as "BOOK$ENTRY" when a
    /// book name is present, otherwise just the entry name.
    std::unordered_map<duint32, std::pair<dint32, std::string>> dbColorMap;
    /// MLINESTYLE handle → style name. Populated as MLINESTYLE objects are
    /// parsed; consumed by the entryParse template hook to stamp
    /// styleName onto each MLINE entity post-parse (DXF code 2 / DWG 340
    /// resolves to a name only after the OBJECTS section is read).
    std::unordered_map<duint32, std::string> mlineStyleNameMap;
    std::unordered_map<duint32, DRW_AppId*> appIdmap;
    std::unordered_map<duint32, DRW_View*> viewmap;
    std::unordered_map<duint32, DRW_UCS*> ucsmap;

    // Buffers for ATTRIB attached-attlist routing in processDwgEntity.
    // m_pendingInserts: INSERT entities awaiting their ATTRIB children + SEQEND
    //                   before being dispatched to addInsert.  Keyed by INSERT handle.
    // m_orphanAttribs:  ATTRIB entities seen before their owning INSERT.
    //                   Keyed by parent (INSERT) handle.
    std::unordered_map<duint32, DRW_Insert> m_pendingInserts;
    std::unordered_map<duint32, std::vector<std::shared_ptr<DRW_Attrib>>> m_orphanAttribs;
//    duint32 currBlock;
    duint8 maintenanceVersion{0};

protected:
    std::unique_ptr<dwgBuffer> fileBuf;
    dwgR *parent{nullptr};
    DRW::Version version{DRW::UNKNOWNV};

//seeker (position) for the beginning sentinel of the image data (R13 to R15)
    duint32 previewImagePos;

//sections map
    std::unordered_map<int, dwgSectionInfo >sections;
    std::unordered_map<duint32, DRW_Class*> classesmap;

protected:
    DRW_TextCodec decoder;

protected:
//    duint32 blockCtrl;
    duint32 nextEntLink{0};
    duint32 prevEntLink{0};

private:
    template <class T>
    bool entryParse(T &e, dwgBuffer &buff, duint32 bs, bool &ret) {
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
