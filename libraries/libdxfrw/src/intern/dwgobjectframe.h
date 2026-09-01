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

#ifndef DWGOBJECTFRAME_H
#define DWGOBJECTFRAME_H

#include <cstdint>
#include <vector>

#include "dwgbuffer.h"

/**
 * A validated R13+ DWG entity/object frame.
 *
 * The frame header is a modular-short body size, followed by the R2010+
 * handle-bit size, the body bytes, and a CRC16.  Keeping this decoding in one
 * place prevents entity, object, and child readers from disagreeing about
 * bounds or CRC coverage.  `readAt()` leaves the buffer at the frame start on
 * failure, so callers cannot accidentally continue with a partially decoded
 * record.
 */
class DwgObjectFrame {
public:
    static constexpr std::uint64_t MaxBodySize = 256ULL * 1024ULL * 1024ULL;

    bool readAt(dwgBuffer& buffer, DRW::Version version, std::uint64_t offset);

    std::uint64_t offset() const { return m_offset; }
    /// R2010+ MC prefix: number of bits occupied by the handle stream.
    /// Existing parseDwg APIs accept this raw value and derive their own body
    /// boundary from it.
    std::uint32_t bodyBitSize() const { return m_handleStreamBitSize; }
    /// Bit offset at which the trailing handle stream begins.
    std::uint32_t objectDataBitSize() const { return m_objectDataBitSize; }
    std::uint32_t handleStreamBitSize() const { return m_handleStreamBitSize; }
    std::vector<std::uint8_t>& body() { return m_body; }
    const std::vector<std::uint8_t>& body() const { return m_body; }

private:
    std::uint64_t m_offset{0};
    std::uint32_t m_objectDataBitSize{0};
    std::uint32_t m_handleStreamBitSize{0};
    std::vector<std::uint8_t> m_body;
};

#endif // DWGOBJECTFRAME_H
