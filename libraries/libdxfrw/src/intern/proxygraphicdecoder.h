#ifndef PROXYGRAPHICDECODER_H
#define PROXYGRAPHICDECODER_H

#include <cstddef>
#include <string>
#include <vector>
#include "../drw_base.h"

class DRW_Interface;
class DRW_Entity;
class DRW_Arc;
class DRW_Circle;
class DRW_Ellipse;
class DRW_LWPolyline;
class DRW_Mesh;
class DRW_Polyline;
class DRW_Text;

//! The narrow output surface used by the proxy-graphics opcode decoder.
//!
//! Proxy graphics only produces the seven primitive kinds declared here.  It
//! deliberately does not inherit DRW_Interface: a DWG BLOCK reader can stage
//! these values without forwarding unrelated TABLES or OBJECTS callbacks.
class DRW_ProxyGraphicSink {
public:
    virtual ~DRW_ProxyGraphicSink() = default;

    virtual bool addArc(const DRW_Arc& value) = 0;
    virtual bool addCircle(const DRW_Circle& value) = 0;
    virtual bool addEllipse(const DRW_Ellipse& value) = 0;
    virtual bool addLWPolyline(const DRW_LWPolyline& value) = 0;
    virtual bool addMesh(const DRW_Mesh& value) = 0;
    virtual bool addPolyline(const DRW_Polyline& value) = 0;
    virtual bool addText(const DRW_Text& value) = 0;
};

enum class DRW_ProxyGraphicStopReason : unsigned char {
    Complete,
    ShortHeader,
    InvalidChunkSize,
    TruncatedChunk,
    ChunkLimit,
    PrimitiveLimit,
    MatrixDepthLimit,
    ResourceLimit,
    AllocationFailure,
    SinkRefused
};

//! Explicit resource limits for one proxy-graphics stream.
struct DRW_ProxyGraphicLimits {
    std::size_t maxChunkCount {1000000u};
    std::size_t maxPrimitiveCount {1000000u};
    std::size_t maxMatrixDepth {64u};
    std::size_t maxPolylineVertices {1000000u};
    std::size_t maxMeshVertices {1000000u};
    std::size_t maxMeshFaceEntries {4000000u};
};

//! Outcome of scanning or decoding one proxy-graphics stream.
struct DRW_ProxyGraphicDecodeResult {
    std::size_t emittedPrimitiveCount {0};
    std::size_t recognizedPrimitiveChunkCount {0};
    std::size_t skippedUnsupportedChunkCount {0};
    std::size_t consumedByteCount {0};
    DRW_ProxyGraphicStopReason stopReason {
        DRW_ProxyGraphicStopReason::Complete};

    [[nodiscard]] bool completed() const noexcept {
        return stopReason == DRW_ProxyGraphicStopReason::Complete;
    }
};

//! Decoder for cached "proxy graphics".
/*!
 *  Custom / proxy entities (STDPART2D, AEC_WALL/WINDOW/DOOR, ACAD_TABLE, …)
 *  carry a self-contained primitive stream (circles / arcs / polylines / text)
 *  so that any reader can render them without understanding the owning class.
 *  libdxfrw captures the raw bytes into DRW_Entity::proxyGraphics; this class
 *  turns them into real DRW_* primitives emitted through a small sink.
 *
 *  Ported from ezdxf src/ezdxf/proxygraphic.py (opcode table + the byte-aligned
 *  little-endian ByteStream framing) and tools/binarydata.py (ByteStream).
 *  @author libdxfrw
 */
class DRW_ProxyGraphicDecoder {
public:
    //! Scan proxy chunk framing without allocating primitives or invoking a
    //! callback. The recognized primitive count is a safe journal-event upper
    //! bound because each supported opcode emits at most one primitive.
    static DRW_ProxyGraphicDecodeResult inspect(
        const std::string &bytes,
        const DRW_ProxyGraphicLimits &limits = {});

    //! Decode @p bytes and emit each recovered primitive through @p sink,
    //! inheriting layer / space / handle from @p parent (the owning custom
    //! entity) and the decoder's accumulated colour / layer / linetype /
    //! lineweight draw-state.  @p layerNames / @p ltypeNames are the file's
    //! layer / linetype names in storage order — the index space the proxy's
    //! ATTRIBUTE_LAYER (16) / ATTRIBUTE_LINETYPE (18) opcodes reference; pass
    //! empty to leave those attributes inherited from @p parent. A malformed
    //! stream stops decoding and is reported in the result. A sink may decline
    //! an event; exceptions from a sink are intentionally not swallowed.
    static DRW_ProxyGraphicDecodeResult decode(
        const std::string &bytes, DRW::Version version,
        DRW_ProxyGraphicSink &sink, const DRW_Entity &parent,
        const std::vector<std::string> &layerNames = {},
        const std::vector<std::string> &ltypeNames = {},
        const DRW_ProxyGraphicLimits &limits = {});

    //! Compatibility overload for existing direct DXF and immediate DWG
    //! callers. It uses an internal DRW_Interface adapter and returns the
    //! emitted primitive count exactly as older callers expect.
    static int decode(const std::string &bytes, DRW::Version version,
                      DRW_Interface &iface, const DRW_Entity &parent,
                      const std::vector<std::string> &layerNames = {},
                      const std::vector<std::string> &ltypeNames = {});
};

#endif // PROXYGRAPHICDECODER_H
