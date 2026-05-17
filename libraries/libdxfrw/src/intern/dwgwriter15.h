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

#include "dwgwriter.h"

class DRW_Entity;

/// R2000 (AC1015) concrete DWG writer.  Structural mirror of
/// [dwgReader15](dwgreader15.h).
///
/// Section emission order matches LibreDWG (encode.c §3144 ff):
///   HEADER → CLASSES → OBJECTS body → HANDLES → 2NDHEADER.
/// AUXHEADER / TEMPLATE / PREVIEW are optional and emit only if
/// the input document carries them (v1: PREVIEW only, as an empty
/// stub; AUXHEADER + TEMPLATE deferred to v2).
class dwgWriter15 : public dwgWriter {
public:
    dwgWriter15(std::ofstream *stream, DRW_Header *header)
        : dwgWriter(stream, header) {}

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
                        const DRW_Coord& basePoint) override;
    bool emitDeferredBlockControl() override;

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
    void finishObject();

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
                         duint32 blockHandle, duint32 endBlockHandle);

    /// Phase 4d helper: emit a Block entity at `handle`.  `isEnd=true`
    /// suppresses the name field and emits an ENDBLK (oType=5) rather
    /// than a BLOCK (oType=4).
    void emitBlockEntity(duint32 handle, const std::string& name,
                         bool isEnd);

private:
    /// File offset of the first section-locator record byte.  Used by
    /// `finalize()` to back-patch addresses + sizes.  Set during
    /// `writeFileHeaderStub`.
    duint32 m_recordsOffset {0};

    /// Number of section-locator records emitted (typically 5 for an
    /// empty document — HEADER, CLASSES, HANDLES, UNKNOWN, AUXHEADER —
    /// or 6 if TEMPLATE present).  v1 emits 5.
    duint8 m_numSections {5};

    /// Per-section-frame helper: emit BEGIN sentinel + 4-byte RL size
    /// placeholder, mark the start offset.  Returns the byte offset of
    /// the placeholder RL so `endSentinelSection` can patch it.
    size_t beginSentinelSection(const duint8 (&beginSentinel)[16]);

    /// Per-section-frame helper: emit END sentinel + CRC16 LE over the
    /// section bytes between begin sentinel and end sentinel.  Patches
    /// the RL size at `sizeOffset` with the actual payload size.
    void endSentinelSection(size_t sectionStart, size_t sizeOffset,
                            const duint8 (&endSentinel)[16]);

    /// Scratch buffer for the in-flight object body.  Cleared at every
    /// `beginObject` call.
    dwgBufferW m_objectBody;

    /// Handle of the in-flight object (set by `beginObject`, cleared
    /// at `finishObject`).  Used to record the (handle, offset) pair.
    duint32 m_currentHandle {0};

    /// Object-map collector.  Each entry is `(handle, byte-offset of
    /// the object's MS prefix in m_buf)`.  Sorted by handle in
    /// `writeDwgHandles` before page emission for monotonic deltas.
    std::vector<std::pair<duint32, duint32>> m_objectMap;

    /// Block_Record handles for user-defined blocks (from defineBlock).
    /// Consumed by emitDeferredBlockControl to populate BLOCK_CONTROL's
    /// numEntries + child handle list.  +2 phantom handles for
    /// MODEL_SPACE and PAPER_SPACE are added on top.
    std::vector<duint32> m_userBlockRecordHandles;
};

#endif // DWGWRITER15_H
