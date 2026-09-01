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

#include "dwgobjectframe.h"

#include <limits>

#include "drw_reserve.h"

bool DwgObjectFrame::readAt(dwgBuffer& buffer, DRW::Version version,
                            std::uint64_t offset) {
    m_offset = offset;
    m_objectDataBitSize = 0;
    m_handleStreamBitSize = 0;
    m_body.clear();

    const auto restoreFrameCursor = [&]() {
        // Keep malformed probes non-poisoning, but honor the frame-reader
        // contract when the requested frame offset is representable.
        if (offset <= buffer.size()) {
            buffer.setPosition(offset);
            buffer.setBitPos(0);
        }
    };
    const auto fail = [&]() {
        m_objectDataBitSize = 0;
        m_handleStreamBitSize = 0;
        m_body.clear();
        restoreFrameCursor();
        return false;
    };

    // Parse on an independent cursor. A truncated field marks a dwgBuffer's
    // invalid state; publishing that state would make a later retry fail even
    // when the source bytes are repaired or another frame is selected.
    dwgBuffer probe = buffer.forkIndependent();
    if (offset > probe.size() || !probe.setPosition(offset)) {
        return fail();
    }

    const std::int32_t encodedSize = probe.getModularShort();
    if (!probe.isGood() || encodedSize < 0) {
        return fail();
    }

    if (version > DRW::AC1021) {
        const std::uint64_t handleStreamBitSize = probe.getUModularChar();
        if (handleStreamBitSize > std::numeric_limits<std::uint32_t>::max())
            return fail();
        m_handleStreamBitSize = static_cast<std::uint32_t>(handleStreamBitSize);
    }
    if (!probe.isGood())
        return fail();

    const std::uint64_t bodyStart = probe.getPosition();
    const std::uint64_t bodySize = static_cast<std::uint64_t>(encodedSize);
    constexpr std::uint64_t crcSize = sizeof(std::uint16_t);
    if (bodyStart > buffer.size()
        || bodySize > buffer.size() - bodyStart
        || crcSize > buffer.size() - bodyStart - bodySize
        || bodySize > MaxBodySize
        || bodySize > std::numeric_limits<std::size_t>::max()) {
        return fail();
    }

    const std::uint64_t bodyEnd = bodyStart + bodySize;
    // R2010+ stores the handle stream bit count separately from the total
    // frame body. It cannot describe more bits than the body contains.
    const std::uint64_t totalBodyBits = bodySize * 8;
    // A non-empty frame must retain at least one bit for the object-data
    // portion.  A full-body handle stream cannot carry the common prologue and
    // is therefore structurally invalid even though the arithmetic itself is
    // representable.
    if (m_handleStreamBitSize > totalBodyBits
        || (bodySize != 0 && m_handleStreamBitSize == totalBodyBits)) {
        return fail();
    }
    m_objectDataBitSize = static_cast<std::uint32_t>(
        totalBodyBits - m_handleStreamBitSize);
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        || bodyEnd > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
        return fail();
    }

    if (bodySize > static_cast<std::uint64_t>(std::numeric_limits<int>::max())
        || !DRW::resize(m_body, static_cast<int>(bodySize))) {
        return fail();
    }
    if (!probe.getBytes(m_body.data(), bodySize)) {
        return fail();
    }

    const std::uint16_t storedCrc = probe.getRawShort16();
    if (!probe.isGood()) {
        return fail();
    }

    const std::uint16_t calculatedCrc = probe.crc8(
        0xC0C1,
        static_cast<std::int32_t>(offset),
        static_cast<std::int32_t>(bodyEnd));
    if (storedCrc != calculatedCrc) {
        return fail();
    }

    if (!buffer.setPosition(probe.getPosition())) {
        return fail();
    }
    buffer.setBitPos(probe.getBitPos());
    if (buffer.getBitPos() != probe.getBitPos()) {
        return fail();
    }
    return true;
}
