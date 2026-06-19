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

#ifndef LIBDXFRW_H
#define LIBDXFRW_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_classes.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "handle_allocator.h"


class dxfReader;
class dxfWriter;

/** Holds per-read-session name-resolution tables populated during DXF/DWG parsing. */
class DRW_ParsingContext {
public:
    struct BlockRecordInfo {
        std::string name;
        int insUnits = 0;
    };

    DRW_ParsingContext() = default;
    /** Returns line-type name for a given DXF handle, or empty string if not found. */
    std::string resolveLineTypeName(int handle) const {
        auto it = lineTypeNameMap.find(static_cast<std::uint32_t>(handle));
        return (it != lineTypeNameMap.end()) ? it->second : std::string();
    }
    /** Returns block-record name for a given DXF handle, or empty string if not found. */
    std::string resolveBlockRecordName(std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.name : std::string();
    }
    /** Returns block-record insertion units for a given DXF handle, or 0 if unknown. */
    int resolveBlockRecordInsUnits(std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.insUnits : 0;
    }
    std::unordered_map<std::uint32_t, std::string> lineTypeNameMap;
    std::unordered_map<std::uint32_t, BlockRecordInfo> blockRecordMap;
};

class dxfRW {
public:
    dxfRW(const char* name);
    dxfRW(const dxfRW&) = delete;
    dxfRW& operator=(const dxfRW&) = delete;
    dxfRW(dxfRW&&) = delete;
    dxfRW& operator=(dxfRW&&) = delete;
    ~dxfRW();
    void setDebug(DRW::DebugLevel lvl);
    /// reads the file specified in constructor
    /*!
     * An interface must be provided. It is used by the class to signal various
     * components being added.
     * @param interface_ the interface to use
     * @param ext should the extrusion be applied to convert in 2D?
     * @return true for success
     */
    bool read(DRW_Interface *interface_, bool ext);
    bool readAscii(DRW_Interface *interface_, bool ext, std::string& content);
    void setBinary(bool b) {binFile = b;}

    bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);
    bool writeLineType(DRW_LType *ent);
    bool writeLayer(DRW_Layer *ent);
    bool writeDimstyle(DRW_Dimstyle *ent);
    bool writeTextstyle(DRW_Textstyle *ent);
    bool writeVport(DRW_Vport *ent);
    bool writeView(DRW_View *ent);
    bool writeUCS(DRW_UCS *ent);
    bool writeAppId(DRW_AppId *ent);
    bool writePoint(DRW_Point *ent);
    bool writeLine(DRW_Line *ent);
    bool writeRay(DRW_Ray *ent);
    bool writeXline(DRW_Xline *ent);
    bool writeCircle(DRW_Circle *ent);
    bool writeArc(DRW_Arc *ent);
    bool writeEllipse(DRW_Ellipse *ent);
    bool writeTrace(DRW_Trace *ent);
    bool writeSolid(DRW_Solid *ent);
    bool write3dface(DRW_3Dface *ent);
    bool writeLWPolyline(DRW_LWPolyline *ent);
    bool writePolyline(DRW_Polyline *ent);
    bool writeSpline(DRW_Spline *ent);
    bool writeBlockRecord(std::string name, int insUnits = 0);
    bool writeBlock(DRW_Block *ent);
    bool writeInsert(DRW_Insert *ent);
    bool writeAttrib(DRW_Attrib *ent);
    bool writeMText(DRW_MText *ent);
    bool writeMLine(DRW_MLine *ent);
    bool writeUnderlay(DRW_Underlay *ent);
    bool writeText(DRW_Text *ent);
    bool writeTolerance(DRW_Tolerance *ent);
    bool writeHatch(DRW_Hatch *ent);
    bool writeViewport(DRW_Viewport *ent);
    DRW_ImageDef *writeImage(DRW_Image *ent, std::string name);
    bool writeWipeout(DRW_Image *ent);
    bool writeMultiLeader(DRW_MLeader *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeDimension(DRW_Dimension *ent);
    void setEllipseParts(int parts){elParts = parts;} /*!< set parts number when convert ellipse to polyline */
    bool writePlotSettings(DRW_PlotSettings *ent);
    /*!< F4 — typed DXF emitters for the routed data-only OBJECTS the DWG reader
     * populates only into typed metadata (SUN/SCALE/DICTIONARYVAR/
     * RASTERVARIABLES). The DXF group-code shape is the inverse of each type's
     * parseCode, cross-checked against ezdxf 1.4.4. The filter pulls these from
     * dwgAdvancedMetadata() on the DWG->DXF path (DXF->DXF preserves them via the
     * raw net; the filter dedups by handle to avoid a double-emit). Each emits the
     * verbatim code-5 handle and a 330 owner; a matching CLASS record must be
     * registered (dxfClassForRecordName has SUN/SCALE/DICTIONARYVAR/
     * RASTERVARIABLES). */
    bool writeSun(DRW_Sun *ent);
    bool writeScale(DRW_Scale *ent);
    bool writeDictionaryVar(DRW_DictionaryVar *ent);
    bool writeRasterVariables(DRW_RasterVariables *ent);
    /*!< MLINESTYLE is a FIXED built-in (no CLASS record); DWG read populates
     * only typed metadata, so the filter emits it typed on DWG->DXF, deduped vs
     * the raw net by handle. */
    bool writeMLineStyle(DRW_MLineStyle *ent);
    /*!< WIPEOUTVARIABLES (custom class — CLASS registered) DWG->DXF typed emit;
     * deduped vs the raw net by handle like the other data-only OBJECTS. */
    bool writeWipeoutVariables(DRW_WipeoutVariables *ent);
    bool writeRawDxfObject(DRW_RawDxfObject *obj);
    /*!< Mark a specific code-5 handle as in-use so the minted-handle stream
     * (m_handleAllocator.next()) never re-issues it. Mirrors
     * dwgWriter::reserveHandle. The filter calls this for every verbatim handle
     * preserved in the raw-passthrough net (rawDxfObjects/rawDxfEntities) before
     * write(), so a re-emitted raw OBJECT/ENTITY cannot collide with either a
     * freshly-minted handle or a fixed-low structural handle. */
    void reserveHandle(std::uint32_t h) { m_handleAllocator.reserve(h); }
    /*!< High-water mark of the handle allocator (one past the largest handle
     * reserved or minted so far). Used to populate $HANDSEED. Mirrors
     * dwgWriter::highWaterHandle. */
    std::uint32_t highWaterHandle() const { return m_handleAllocator.current(); }
    /*!< Register the CLASS records to emit in the DXF CLASSES section (custom,
     * non-fixed object classes actually present in the output). The filter
     * builds this from the raw-net objects before write(); empty by default. */
    void setDxfClasses(const std::vector<DRW_Class> &classes) { m_dxfClasses = classes; }
    /*!< Canonical DXF CLASS metadata (recName/className/appName/proxyFlag/
     * wasaProxyFlag/entityFlag, per ezdxf REQUIRED_CLASSES) for the known
     * custom-class OBJECTS that the raw net round-trips. Returns false for
     * fixed/built-in or unknown record names. instanceCount is left 0 for the
     * caller to fill. */
    static bool dxfClassForRecordName(const std::string &recName, DRW_Class &out);
    /*!< Register extra (name, hex-handle) entries to splice into the regenerated
     * root NamedObjectsDictionary (handle C) so raw-net-routed named dictionaries
     * are reachable from the root and not pruned as orphans. The filter builds
     * this from the source root dict before write(); empty by default. */
    void setRootDictEntries(const std::vector<std::pair<std::string, std::string>> &entries) {
        m_rootDictEntries = entries;
    }
    /*!< Register named-dictionary OBJECTS to emit verbatim in the regenerated
     * OBJECTS section (DXF write path only). Each carries its source code-5
     * handle, parent (330) owner, and entry (name -> child handle) list. The
     * filter builds these from dwgAdvancedMetadata().dictionaries() on the
     * DWG->DXF path so the named dictionaries that were previously only
     * referenced from the root C dict (via setRootDictEntries) actually exist
     * as reachable objects with a valid owner — clearing the INVALID_OWNER_HANDLE
     * fixes ezdxf applies for the dangling 350 references. Empty by default. */
    void setNamedDictObjects(const std::vector<DRW_Dictionary> &dicts) {
        m_namedDictObjects = dicts;
    }
    /*!< Register GROUP objects to typed-emit in the regenerated OBJECTS section
     * (DXF write path only). Each carries its name/description/flags and the
     * member entity SOURCE handles (DRW_Group::m_entityHandles). The codec mints
     * a fresh group handle, injects the (name, minted-handle) entry into the
     * ACAD_GROUP D dict, and emits the GROUP with 340 references resolved through
     * the writeEntity source->minted map (members absent from the map — consumed
     * or filtered entities — are skipped, never emitted as a dangling 340).
     * Empty by default. */
    void setGroups(const std::vector<DRW_Group> &groups) {
        m_groups = groups;
    }
    /*!< Allocate a fresh, collision-free code-5 handle from the codec's
     * allocator. The filter uses this AFTER reserving every raw + fixed handle
     * to remap the small minority of raw objects whose ORIGINAL handle coincides
     * with one of the codec's fixed structural literals (LAYER 0x10, LTYPE
     * 0x14-0x16, BLOCK/ENDBLK/BLOCK_RECORD 0x1C-0x21, ...). Those handles cannot
     * be preserved verbatim because the codec emits its own table/block record at
     * the same literal; remapping the raw object (and rewriting every reference
     * to it via setHandleRemap) is the only collision-free resolution. */
    std::uint32_t allocHandle() { return m_handleAllocator.next(); }
    /*!< Format a handle as the codec's canonical code-5 hex string (uppercase,
     * no leading zeros), so a caller can build a 350/330 reference string that
     * byte-matches the re-emitted handle (e.g. a remapped root-dict entry). */
    std::string toHexStrHandle(std::uint32_t h) { return toHexStr(static_cast<int>(h)); }
    /*!< Register a handle-remap applied by writeRawDxfObject to every raw
     * object/entity it emits: the object's own code-5/105 handle and every
     * handle-reference group (codes 320-369, 1005, plus 102-group reactor 330s)
     * whose value is a remapped handle is rewritten to the new handle. Empty by
     * default (raw handles preserved verbatim). Keys/values are numeric handles. */
    void setHandleRemap(const std::map<std::uint32_t, std::uint32_t> &remap) {
        m_handleRemap = remap;
    }

    DRW::Version getVersion() const;
    DRW::error getError() const;

    int getBlockRecordHandleToWrite(const std::string& blockName) const;
    int getTextStyleHandle(const std::string& styleName) const;
    DRW_ParsingContext* getReadingContext() { return &m_readingContext; }
    DRW_WritingContext* getWritingContext() { return &m_writingContext; }

private:
    /// used by read() to parse the content of the file
    bool processDxf();
    bool processHeader();
    bool processClasses();
    bool processTables();
    bool processBlocks();
    bool processBlock();
    bool processEntities(bool isblock);
    bool processObjects();
    bool processDetailViewStyle();
    bool processSectionViewStyle();
    bool processBreakData();
    bool processBreakPointRef();
    bool processMaterial();
    bool processGeoData();
    bool processVisualStyle();
    bool processImageDefReactor();
    bool processSpatialFilter();
    bool processTableStyle();
    bool processMLeaderStyle();
    bool processSortEntsTable();
    bool processDimAssoc();
    bool processBackground();
    bool processPointCloudDef();
    bool processSunStudy();
    bool processRenderSettings();

    bool processLType();
    bool processLayer();
    bool processDimStyle();
    bool processTextStyle();
    bool processVports();
    bool processAppId();
    bool processView();
    bool processUCS();
    bool processBlockRecord();

    bool processPoint();
    bool processLine();
    bool processRay();
    bool processXline();
    bool processCircle();
    bool processArc();
    bool processEllipse();
    bool processTrace();
    bool processSolid();
    bool processInsert();
    bool processAttrib(DRW_Insert* insert);
    bool processLWPolyline();
    bool processPolyline();
    bool processVertex(DRW_Polyline* pl);
    bool processText();
    bool processTolerance();
    bool processMText();
    bool processMLine();
    bool processUnderlay(const std::string& kind);
    bool processHatch();
    bool processMPolygon();
    bool processSpline();
    bool process3dface();
    bool processMesh();
    bool processViewport();
    bool processImage();
    bool processImageDef();
    bool processWipeout();
    bool processMultiLeader();
    bool processDimension();
    bool processArcDimension();
    bool processLeader();
    bool processPlotSettings();
    bool processGroup();
    bool processDictionary();
    bool processScale();
    bool processMLineStyle();
    bool processDictionaryVar();
    bool processDictionaryWithDefault();
    bool processRasterVariables();
    bool processSun();
    bool processLayout();
    bool processWipeoutVariables();
    bool processRawObject();
    bool processRawEntity();
    /*!< Append the current DXF record (already read by reader->readRec) to a
     * raw-passthrough carrier as a correctly-TYPED DRW_Variant, classifying the
     * value by DXF code range (numeric codes leave reader->strData stale AND
     * clobber reader->type to STRING, so neither getString() nor type can be
     * trusted for them — that was the A1/A4 capture bug). ASCII-DXF only. Also
     * latches code 5 -> handle and code 330 -> parentHandle. */
    void captureRawGroup(DRW_RawDxfObject &obj, int code);

//    bool writeHeader();
    /// Reserve the DXF codec's fixed structural code-5 literals (table heads,
    /// mandatory table records, BLOCK_RECORDs, the *Model/*Paper BLOCK+ENDBLK,
    /// and the root dict "C" / ACAD_GROUP "D") in m_handleAllocator before the
    /// body is streamed. These DIFFER from the DWG seedReserved() set. After
    /// this, the first next() yields FIRSTHANDLE (0x30) exactly as the legacy
    /// ++entCount did, so a fresh write (empty raw net) is byte-identical.
    void seedReservedDxf();
    // captureSourceHandle=false on the VERTEX/SEQEND parent re-entries
    // (writePolyline/writeInsert) so they do not pollute the source->minted map.
    bool writeEntity(DRW_Entity *ent, bool captureSourceHandle = true);
    bool writeArcDimension(DRW_DimArc *d);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();
    bool writeExtData(const std::vector<DRW_Variant*> &ed);
    /* Entity-flavoured overload: entities own extData via shared_ptr, table
     * records own raw pointers. Same DXF codes, different storage. */
    bool writeExtData(const std::vector<std::shared_ptr<DRW_Variant>> &ed);
    /*!< F4 — emit a 330 owner handle for a typed data-only OBJECT (the record's
     * parentHandle when nonzero, else root dict "C" so it is reachable and not
     * pruned as an orphan); no-op pre-R2000 (DXF has no OBJECTS 330 then). */
    void writeObjectOwner(std::uint32_t parentHandle);
    /*use version from dwgutil.h*/
    std::string toHexStr(int n);//RLZ removeme
    bool writeAppData(const std::list<std::list<DRW_Variant>> &appData);

    bool setError(const DRW::error lastError);

private:
    DRW::Version version;
    DRW::error error {DRW::BAD_NONE};
    std::string fileName;
    std::string codePage;
    bool binFile;
    std::unique_ptr<dxfReader> reader;
    std::unique_ptr<dxfWriter> writer;
    DRW_Interface *iface;
    DRW_Header header;
//    int section;
    std::string nextentity;
    /// Mints monotonic, collision-free code-5 handles for the DXF write path.
    /// Seeded with the codec's fixed structural literals (seedReservedDxf) plus
    /// every raw-net handle the filter reserves before write(); next() then
    /// skips that whole set, so a minted handle can never duplicate a fixed-low
    /// or preserved-raw handle. Replaces the old `int entCount` + handle floor.
    HandleAllocator m_handleAllocator;
    std::vector<DRW_Class> m_dxfClasses;
    std::vector<std::pair<std::string, std::string>> m_rootDictEntries;
    /// Named-dictionary OBJECTS to emit verbatim in writeObjects (DXF path).
    /// Populated via setNamedDictObjects; empty by default so a fresh write is
    /// byte-identical.
    std::vector<DRW_Dictionary> m_namedDictObjects;
    /// GROUP objects to typed-emit in writeObjects (DXF path). Populated via
    /// setGroups; empty by default so a fresh write is byte-identical.
    std::vector<DRW_Group> m_groups;
    /// Numeric handle -> replacement handle, applied by writeRawDxfObject to the
    /// few raw objects whose original handle collides with a fixed structural
    /// literal. Empty by default (raw handles emitted verbatim).
    std::map<std::uint32_t, std::uint32_t> m_handleRemap;
    bool wlayer0;
    bool dimstyleStd;
    bool applyExt;
    bool writingBlock;
    int elParts;  /*!< parts number when convert ellipse to polyline */
    std::unordered_map<std::string,int> blockMap;
    std::unordered_map<std::string,int> textStyleMap;
    std::vector<DRW_ImageDef*> imageDef;  /*!< imageDef list */

    int currHandle;

    DRW_ParsingContext m_readingContext;
    DRW_WritingContext m_writingContext;
};


#endif // LIBDXFRW_H
