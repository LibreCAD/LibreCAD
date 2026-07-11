#ifndef PROXYGRAPHICDECODER_H
#define PROXYGRAPHICDECODER_H

#include <string>
#include <vector>
#include "../drw_base.h"

class DRW_Interface;
class DRW_Entity;

//! Decoder for cached "proxy graphics".
/*!
 *  Custom / proxy entities (STDPART2D, AEC_WALL/WINDOW/DOOR, ACAD_TABLE, …)
 *  carry a self-contained primitive stream (circles / arcs / polylines / text)
 *  so that any reader can render them without understanding the owning class.
 *  libdxfrw captures the raw bytes into DRW_Entity::proxyGraphics; this class
 *  turns them into real DRW_* primitives emitted through DRW_Interface.
 *
 *  Ported from ezdxf src/ezdxf/proxygraphic.py (opcode table + the byte-aligned
 *  little-endian ByteStream framing) and tools/binarydata.py (ByteStream).
 *  @author libdxfrw
 */
class DRW_ProxyGraphicDecoder {
public:
    //! Decode @p bytes and emit each recovered primitive through @p iface,
    //! inheriting layer / space / handle from @p parent (the owning custom
    //! entity) and the decoder's accumulated colour / layer / linetype /
    //! lineweight draw-state.  @p layerNames / @p ltypeNames are the file's
    //! layer / linetype names in storage order — the index space the proxy's
    //! ATTRIBUTE_LAYER (16) / ATTRIBUTE_LINETYPE (18) opcodes reference; pass
    //! empty to leave those attributes inherited from @p parent.  Never throws;
    //! a malformed stream simply stops decoding.  Returns the number of
    //! primitives emitted (for telemetry / tests).
    static int decode(const std::string &bytes, DRW::Version version,
                      DRW_Interface &iface, const DRW_Entity &parent,
                      const std::vector<std::string> &layerNames = {},
                      const std::vector<std::string> &ltypeNames = {});
};

#endif // PROXYGRAPHICDECODER_H
