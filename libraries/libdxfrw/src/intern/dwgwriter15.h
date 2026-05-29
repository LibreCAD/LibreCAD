/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGWRITER15_H
#define DWGWRITER15_H

#include <initializer_list>
#include <utility>
#include <vector>

#include "dwgwriter.h"
#include "../drw_objects.h"

class DRW_Entity;

/// Section record indices used in m_sectionOffsets / m_sectionSizes.
namespace recno {
    constexpr duint8 HEADER    = 0;
    constexpr duint8 CLASSES   = 1;
    constexpr duint8 HANDLES   = 2;
    constexpr duint8 UNKNOWN   = 3;
    constexpr duint8 TEMPLATE  = 4;
    constexpr duint8 AUXHEADER = 5;
}

/// R2000 (AC1015) concrete DWG writer.  Structural mirror of
/// [dwgReader15](dwgreader15.h).
///
/// Section emission order matches LibreDWG (encode.c §3144 ff):
///   HEADER → CLASSES → OBJECTS body → HANDLES → 2NDHEADER.
/// AUXHEADER / TEMPLATE / PREVIEW are optional and emit only if
/// the input document carries them.  Fresh R2000 writes include the
/// AcDb:AuxHeader locator and stream so section layout matches
/// ACadSharp/ODA expectations.
class dwgWriter15 : public dwgWriter {
public:
    dwgWriter15(std::ofstream *stream, DRW_Header *header)
        : dwgWriter(stream, header) {
        // Pre-seed with canonical reserved handles so add*() deduplication
        // works before writeDwgObjects() fires.  Values mirror reservedHandle::*
        // in dwgwriter15.cpp (0x0F-0x16 are the R2000 fixed-table handles).
        m_writingCtx.ltypeMap    = {{"CONTINUOUS", 0x11u},
                                    {"BYLAYER",    0x10u},
                                    {"BYBLOCK",    0x0Fu}};
        m_writingCtx.layerMap    = {{"0",        0x12u}};
        m_writingCtx.styleMap    = {{"STANDARD", 0x13u}};
        m_writingCtx.viewMap     = {};
        m_writingCtx.appidMap    = {{"ACAD",     0x14u}};
        m_writingCtx.dimstyleMap = {{"STANDARD", 0x15u}};
        m_writingCtx.vportMap    = {{"*ACTIVE",  0x16u}};
    }

    bool writeFileHeaderStub() override;
    bool writeDwgHeader() override;
    bool writeDwgClasses() override;
    bool writeDwgObjects() override;
    bool writeDwgHandles() override;
    bool writeSecondHeader() override;
    bool finalize() override;

    /// Encode a single entity into the object stream.  See
    /// dwgWriter::encodeEntity for the contract.
    bool encodeEntity(DRW_Entity *ent) override;

    duint32 defineBlock(const std::string& name,
                        const DRW_Coord& basePoint,
                        int insUnits = 0) override;
    bool emitDeferredBlockControl() override;

    /// Accept a user-defined table record for deferred emission in
    /// writeDwgObjects().  Normalises the name to upper-case, deduplicates
    /// against standard entries, allocates a handle, updates m_writingCtx.
    void addLType(const DRW_LType& lt);
    void addLayer(const DRW_Layer& lay);
    void addTextstyle(const DRW_Textstyle& ts);
    void addView(const DRW_View& view);
    void addVport(const DRW_Vport& vp);
    void addDimstyle(const DRW_Dimstyle& ds);
    void addAppId(const DRW_AppId& ai);
    bool replayRawObject(const DRW_UnsupportedObject& object);
    bool writeAcDbPlaceholder(const DRW_AcDbPlaceholder& placeholder);
    bool writeSun(const DRW_Sun& sun);
    bool writeMLeaderStyle(const DRW_MLeaderStyle& style);
    bool writeDictionary(const DRW_Dictionary& dictionary);
    bool writeXRecord(const DRW_XRecord& xrecord);

protected:
    /// Begin a new object in the object stream (the unsentinel'd byte
    /// region between CLASSES and HANDLES).  Records the offset of
    /// `m_buf` at frame start, captures `handle` for the eventual
    /// `m_objectMap` entry, and returns a reference to the scratch
    /// body buffer the caller writes into.  `m_objectBody` is cleared
    /// at every call so callers can use it directly without prep.
    dwgBufferW& beginObject(duint32 handle);

    /// Finish the current object: byte-align the body, emit the
    /// per-object frame (MS objectSize + body bytes + RS CRC16 LE) to
    /// `m_buf`, record the (handle, offset) pair in `m_objectMap`.
    /// CRC is over the MS prefix + body bytes per LibreDWG convention.
    virtual void finishObject();

    /// Phase 3d helper: emit one control object at `handle` into the
    /// object stream.  `numEntries` is the BL value emitted to the
    /// body (does NOT include the +2 phantom adjustment the reader
    /// applies to BLOCK_CONTROL / LTYPE_CONTROL).  `childHandles` are
    /// the offset-handle entries that get walked by the reader's
    /// `for (int i=0; i<numEntries; i++)` loop after the +2 phantom
    /// adjustment — so for LTYPE_CONTROL with a real CONTINUOUS entry
    /// you pass numEntries=1 and childHandles={BYBLOCK, BYLAYER,
    /// CONTINUOUS} (the +2 phantoms are part of the same offset-handle
    /// sequence on the wire).  Handles are emitted as absolute hard
    /// pointers (code 4) so the reader's `getOffsetHandle` returns
    /// them as-is.
    void emitControlObject(duint16 oType, duint32 handle, duint32 numEntries,
                           std::initializer_list<duint32> childHandles);
    void emitControlObject(duint16 oType, duint32 handle, duint32 numEntries,
                           const std::vector<duint32>& childHandles);

    /// Phase 3e helper: emit a minimum-stub table record at `handle`.
    /// Body is preamble + name only.  The reader parses the name
    /// correctly and stores the record in its `*map` table; trailing
    /// fields read as zeros/null when the buffer runs out (silent
    /// per-record warning, not a section failure).  Sufficient to
    /// flip `ltypemap.count("CONTINUOUS") == 1` etc., to make Phase 3's
    /// "the reader reports the standard tables" milestone hold.
    void emitTableRecord(duint16 oType, duint32 handle,
                         const std::string& name);

    /// Phase 4d helper: emit a full Block_Record at `handle` with the
    /// `block`/`endBlock` handles pointing at DRW_Block entities the
    /// caller has already emitted.  Empty body — firstEH/lastEH=0,
    /// no inserts, no layout.  Needed so `readDwgBlocks` can resolve
    /// the BLOCK_CONTROL `+2` phantom handles (0x17, 0x18) without
    /// failing the block walk.
    void emitBlockRecord(duint32 handle, const std::string& name,
                         duint32 blockHandle, duint32 endBlockHandle,
                         int insUnits = 0);

    /// Phase 4d helper: emit a Block entity at `handle`.  `isEnd=true`
    /// suppresses the name field and emits an ENDBLK (oType=5) rather
    /// than a BLOCK (oType=4).
    void emitBlockEntity(duint32 handle, const std::string& name,
                         bool isEnd);

    /// Full table-record emitters — preamble + encodeDwg + finishObject.
    void emitLtypeRecord(duint32 handle, const DRW_LType& lt);
    void emitLayerRecord(duint32 handle, const DRW_Layer& lay);
    void emitStyleRecord(duint32 handle, const DRW_Textstyle& ts);
    void emitViewRecord(duint32 handle, const DRW_View& view);
    void emitVportRecord(duint32 handle, const DRW_Vport& vp);
    void emitAppIdRecord(duint32 handle, const DRW_AppId& ai);
    void emitDimstyleRecord(duint32 handle, const DRW_Dimstyle& ds);
    void emitAcDbPlaceholderObject(duint32 handle,
                                   const DRW_AcDbPlaceholder& placeholder);
    void emitSunObject(duint32 handle, const DRW_Sun& sun);
    void emitMLeaderStyleObject(duint32 handle, const DRW_MLeaderStyle& style);
    void emitDictionaryObject(duint32 handle, const DRW_Dictionary& dictionary);
    void emitXRecordObject(duint32 handle, const DRW_XRecord& xrecord);

protected:
    /// Populate m_header's ctrl-handle fields with canonical reserved values
    /// where they are still zero (caller may have pre-filled them on read).
    /// Called from writeDwgHeader (and overrides) so the HEADER section's
    /// control-handle block references the right objects.
    void initHeaderControlHandles();

    /// Per-section-frame helper: emit BEGIN sentinel + 4-byte RL size
    /// placeholder, mark the start offset.  Returns the byte offset of
    /// the placeholder RL so `endSentinelSection` can patch it.
    size_t beginSentinelSection(const duint8 (&beginSentinel)[16]);

    /// Per-section-frame helper: emit END sentinel + CRC16 LE over the
    /// section bytes between begin sentinel and end sentinel.  Patches
    /// the RL size at `sizeOffset` with the actual payload size.
    void endSentinelSection(size_t sectionStart, size_t sizeOffset,
                            const duint8 (&endSentinel)[16]);

protected:
    /// Scratch buffer for the in-flight object body (DATA section).
    /// Cleared at every `beginObject` call.
    dwgBufferW m_objectBody;

    /// Scratch buffer for string fields (AC1024+ only — strings are
    /// written into the tail of the data section before the handle
    /// section).  Ignored by dwgWriter15/18 finishObject.
    dwgBufferW m_objectStrings;

    /// Scratch buffer for handle fields (AC1024+ only — handles are
    /// written into a separate handle section after the data section).
    /// Ignored by dwgWriter15/18 finishObject.
    dwgBufferW m_objectHandles;

    /// Handle of the in-flight object (set by `beginObject`, cleared
    /// at `finishObject`).  Used to record the (handle, offset) pair.
    duint32 m_currentHandle {0};

    /// Object-map collector.  Each entry is `(handle, byte-offset of
    /// the object's MS prefix in m_buf)`.  Sorted by handle in
    /// `writeDwgHandles` before page emission for monotonic deltas.
    std::vector<std::pair<duint32, duint32>> m_objectMap;

    /// Writing context: maps normalised (upper-case) table record names
    /// to their allocated DWG handles.  Pre-seeded with standard entries;
    /// extended by add*() as user records are registered.  Used by
    /// encodeEntity() to resolve layer/ltype names to handles.
    DRW_WritingContext m_writingCtx;

    /// Pending user-defined table records.  Populated by add*() during
    /// the table-callback phase; consumed by writeDwgObjects().
    std::vector<std::pair<duint32, DRW_LType>>     m_pendingLTypes;
    std::vector<std::pair<duint32, DRW_Layer>>     m_pendingLayers;
    std::vector<std::pair<duint32, DRW_Textstyle>> m_pendingStyles;
    std::vector<std::pair<duint32, DRW_View>>      m_pendingViews;
    std::vector<std::pair<duint32, DRW_Vport>>     m_pendingVports;
    std::vector<std::pair<duint32, DRW_Dimstyle>>  m_pendingDimstyles;
    std::vector<std::pair<duint32, DRW_AppId>>     m_pendingAppIds;

private:
    /// File offset of the first section-locator record byte.  Used by
    /// `finalize()` to back-patch addresses + sizes.  Set during
    /// `writeFileHeaderStub`.
    duint32 m_recordsOffset {0};

    /// Number of section-locator records emitted.  R2000 writes the
    /// canonical HEADER, CLASSES, HANDLES, ObjFreeSpace, Template, and
    /// AuxHeader records.
    duint8 m_numSections {6};

    /// Block_Record handles for user-defined blocks (from defineBlock).
    /// Consumed by emitDeferredBlockControl to populate BLOCK_CONTROL's
    /// numEntries + child handle list.  +2 phantom handles for
    /// MODEL_SPACE and PAPER_SPACE are added on top.
    std::vector<duint32> m_userBlockRecordHandles;
};

#endif // DWGWRITER15_H
