/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_OBJECTS_H
#define DRW_OBJECTS_H


#include <map>
#include <memory>
#include <string>
#include <vector>
#include "drw_base.h"

class dxfReader;
class dxfWriter;
class dwgBuffer;

namespace DRW {

//! Table entries type.
     enum TTYPE {
         UNKNOWNT,
         LTYPE,
         LAYER,
         STYLE,
         DIMSTYLE,
         VPORT,
         BLOCK_RECORD,
         APPID,
         IMAGEDEF,
         PLOTSETTINGS,
         VIEW,
         UCS,
         MLINESTYLE,
         LAYOUT,
         DICTIONARY,
         DICTIONARYVAR,
         DICTIONARYWDFLT,
         XRECORD,
         FIELD,
         FIELDLIST,
         RASTERVARIABLES,
         SORTENTSTABLE,
         MATERIAL,
         TABLESTYLE,
         TABLECONTENT,
         CELLSTYLEMAP,
         MLEADERSTYLE,
         DBCOLOR,
         VISUALSTYLE,
         UNDERLAYDEFINITION,
         SCALE,
         DIMASSOC,
         EVALUATIONGRAPH,
         DETAILVIEWSTYLE,
         SECTIONVIEWSTYLE,
         BREAKDATA,
         BREAKPOINTREF,
         GROUP,
         IMAGEDEFREACTOR,
         SPATIALFILTER,
         GEODATA,
         TABLEGEOMETRY,
         ACDBPLACEHOLDER,
         SUN,
         ASSOCIATIVEOBJECT,
         ACSHHISTORYOBJECT,
         IDBUFFER,
         LAYERINDEX,
         SPATIALINDEX
     };

//pending VP_ENT_HDR, LONG_TRANSACTION,
//VBA_PROJECT, ACAD_TABLE, PLACEHOLDER,
}

struct DRW_DwgSubrecordRange {
    UTF8STRING m_name;
    std::uint64_t m_startBit = 0;
    std::uint64_t m_bitSize = 0;
    DRW::Version m_version = DRW::UNKNOWNV;
    std::uint32_t m_count = 0;
    bool m_parseComplete = true;
};

class dwgBufferW;

#define SETOBJFRIENDS  friend class dxfRW; \
                       friend class dwgReader; \
                       friend class dwgWriter15; \
                       friend class DrwObjectEncodeTestAccess;

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
class DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_TableEntry() = default;

    virtual~DRW_TableEntry() {
        for (std::vector<DRW_Variant*>::iterator it = extData.begin(); it != extData.end(); ++it) {
            delete *it;
        }

        extData.clear();
    }

    DRW_TableEntry(const DRW_TableEntry& e) :
        tType {e.tType},
        handle {e.handle},
        parentHandle {e.parentHandle},
        name {e.name},
        flags {e.flags},
        reactorHandles {e.reactorHandles},
        xDictHandle {e.xDictHandle},
        xDictFlag {e.xDictFlag},
        numReactors {e.numReactors},
        curr {nullptr}
    {
        // Match copy-assign (which copies these); set in the body to avoid
        // a member-init-order (-Wreorder) constraint.
        oType = e.oType;
        objSize = e.objSize;
        for (std::vector<DRW_Variant *>::const_iterator it = e.extData.begin(); it != e.extData.end(); ++it) {
            DRW_Variant *src = *it;
            DRW_Variant *dst = new DRW_Variant( *src);
            extData.push_back( dst);
            if (src == e.curr) {
                curr = dst;
            }
        }
    }

    // Deep-copy assignment so extData's owned DRW_Variant* are not double-freed
    // (the implicit copy-assign would shallow-copy the pointers). (Phase 3A.0)
    DRW_TableEntry& operator=(const DRW_TableEntry& e) {
        if (this == &e)
            return *this;
        for (DRW_Variant* p : extData)
            delete p;
        extData.clear();
        curr = nullptr;
        tType = e.tType;
        handle = e.handle;
        parentHandle = e.parentHandle;
        name = e.name;
        flags = e.flags;
        reactorHandles = e.reactorHandles;
        xDictHandle = e.xDictHandle;
        xDictFlag = e.xDictFlag;
        numReactors = e.numReactors;
        oType = e.oType;
        objSize = e.objSize;
        for (const DRW_Variant* src : e.extData) {
            DRW_Variant* dst = new DRW_Variant(*src);
            extData.push_back(dst);
            if (src == e.curr)
                curr = dst;
        }
        return *this;
    }

protected:
    virtual bool parseCode(int code, const std::unique_ptr<dxfReader>& reader);
    virtual bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) = 0;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer* strBuf, std::uint32_t bs=0);
    void reset() {
        flags = 0;
        for (std::vector<DRW_Variant*>::iterator it = extData.begin(); it != extData.end(); ++it) {
            delete *it;
        }
        extData.clear();
        reactorHandles.clear();
        xDictHandle = 0;
        curr = nullptr;
    }

public:
    enum DRW::TTYPE tType {DRW::UNKNOWNT};  /*!< enum: entity type, code 0 */
    std::uint32_t         handle {0};             /*!< entity identifier, code 5 */
    int             parentHandle {0};       /*!< Soft-pointer ID/handle to owner object, code 330 */
    UTF8STRING      name;                   /*!< entry name, code 2 */
    int             flags {0};              /*!< Flags relevant to entry, code 70 */
    std::vector<DRW_Variant*> extData;      /*!< FIFO list of extended data, codes 1000 to 1071*/
    std::vector<std::uint32_t> reactorHandles;    /*!< persisted reactor handles (ODA §19.4.2); DWG round-trip (Phase 2a) */
    std::uint32_t         xDictHandle {0};        /*!< extension-dictionary handle (ODA §19.4.2); DWG round-trip (Phase 2a) */

    //***** dwg parse ********/
protected:
    std::int16_t  oType {0};
    std::uint8_t  xDictFlag {0};
    std::int32_t  numReactors {0};
    std::uint32_t objSize {0};    //RL 32bits object data size in bits

private:
    DRW_Variant* curr {nullptr};
};

//! Raw carrier for a DWG object/entity class that libdxfrw does not model yet.
struct DRW_UnsupportedObject {
    int m_objectType = 0;
    std::uint32_t m_handle = 0;
    std::uint32_t m_bodyBitSize = 0;
    std::uint64_t m_objectOffset = 0;
    std::uint32_t m_objectSize = 0;
    bool m_isEntity = false;
    bool m_isCustomClass = false;
    UTF8STRING m_recordName;
    UTF8STRING m_className;
    std::vector<std::uint8_t> m_rawBytes;
};

//! Lossless carrier for non-object DWG data sections preserved byte-for-byte.
struct DRW_RawDwgSection {
    UTF8STRING m_name;
    DRW::Version m_version = DRW::UNKNOWNV;
    std::vector<std::uint8_t> m_data;
};

//! Lossless DXF passthrough carrier (slice A1) for an OBJECTS-section object that
//! libdxfrw does not model as a typed DXF object. Each (group code, raw text value)
//! pair is preserved verbatim so the object can be re-emitted unchanged on DXF write
//! (the DWG raw carrier above is binary and cannot serve the DXF text path).
struct DRW_RawDxfObject {
    UTF8STRING name;                  /*!< object type name (the code-0 string) */
    std::uint32_t handle = 0;              /*!< code 5 (for dedup vs typed writers) */
    std::uint32_t parentHandle = 0;       /*!< code 330 owner */
    std::vector<DRW_Variant> groups; /*!< every (code,value) pair, value kept as text */
};

//! GROUP object (fixed type 72), carrying a named set of entity handles.
class DRW_Group : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Group() { tType = DRW::GROUP; }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    UTF8STRING m_description;
    bool m_isUnnamed = false;
    bool m_selectable = true;
    std::vector<std::uint32_t> m_entityHandles;
};

//! IMAGEDEF_REACTOR object.  The image relationship is carried by object ownership.
class DRW_ImageDefinitionReactor : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_ImageDefinitionReactor() { tType = DRW::IMAGEDEFREACTOR; }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    std::int32_t m_classVersion = 0;
};

//! SPATIAL_FILTER object used for clipped external references.
class DRW_SpatialFilter : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 507;

    DRW_SpatialFilter() { tType = DRW::SPATIALFILTER; }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::vector<DRW_Coord> m_boundaryPoints;
    DRW_Coord m_normal{0.0, 0.0, 1.0};
    DRW_Coord m_origin{0.0, 0.0, 0.0};
    bool m_displayBoundary = false;
    bool m_clipFrontPlane = false;
    bool m_clipBackPlane = false;
    double m_frontDistance = 0.0;
    double m_backDistance = 0.0;
    std::vector<double> m_inverseInsertTransform;
    std::vector<double> m_insertTransform;
};

struct DRW_GeoMeshPoint {
    DRW_Coord m_source;
    DRW_Coord m_destination;
};

struct DRW_GeoMeshFace {
    std::int32_t m_index1 = 0;
    std::int32_t m_index2 = 0;
    std::int32_t m_index3 = 0;
};

//! GEODATA object carrying drawing geolocation metadata.
class DRW_GeoData : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 506;

    DRW_GeoData() { tType = DRW::GEODATA; }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::int32_t m_version = 0;
    std::uint32_t m_hostBlockHandle = 0;
    std::int16_t m_coordinatesType = 0;
    DRW_Coord m_designPoint;
    DRW_Coord m_referencePoint;
    DRW_Coord m_upDirection{0.0, 0.0, 1.0};
    DRW_Coord m_northDirection{0.0, 1.0, 0.0};
    double m_horizontalUnitScale = 1.0;
    double m_verticalUnitScale = 1.0;
    std::int32_t m_horizontalUnits = 0;
    std::int32_t m_verticalUnits = 0;
    std::int32_t m_scaleEstimationMethod = 0;
    double m_userSpecifiedScaleFactor = 1.0;
    bool m_enableSeaLevelCorrection = false;
    double m_seaLevelElevation = 0.0;
    double m_coordinateProjectionRadius = 0.0;
    UTF8STRING m_coordinateSystemDefinition;
    UTF8STRING m_geoRssTag;
    UTF8STRING m_observationFromTag;
    UTF8STRING m_observationToTag;
    UTF8STRING m_observationCoverageTag;
    std::vector<DRW_GeoMeshPoint> m_points;
    std::vector<DRW_GeoMeshFace> m_faces;
};

struct DRW_TableGeometryContent {
    DRW_Coord m_topLeft;
    DRW_Coord m_center;
    double m_contentWidth = 0.0;
    double m_contentHeight = 0.0;
    double m_width = 0.0;
    double m_height = 0.0;
    std::int32_t m_unknown = 0;
};

struct DRW_TableGeometryCell {
    std::int32_t m_flags = 0;
    double m_widthWithGap = 0.0;
    double m_heightWithGap = 0.0;
    std::uint32_t m_unknownHandle = 0;
    std::vector<DRW_TableGeometryContent> m_contents;
};

//! TABLEGEOMETRY object introduced for table geometry cache data.
class DRW_TableGeometry : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_TableGeometry() { tType = DRW::TABLEGEOMETRY; }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    std::int32_t m_rowCount = 0;
    std::int32_t m_columnCount = 0;
    std::int32_t m_cellCount = 0;
    std::vector<DRW_TableGeometryCell> m_cells;
};


//! Class to handle dimstyle entries
/*!
*  Class to handle dim style symbol table entries
*  @author Rallaz
*/
class DRW_Dimstyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Dimstyle() { reset();}
    ~DRW_Dimstyle() {
        clearVars();
    }

    // Rule-of-Five: vars owns raw DRW_Variant* (deleted in the dtor), so a
    // default shallow copy would double-free. The base DRW_TableEntry copy
    // ctor already deep-copies extData; here we additionally deep-copy vars
    // and copy the POD fields. (Phase 3A.0)
    DRW_Dimstyle(const DRW_Dimstyle& o) : DRW_TableEntry(o) {
        copyPodFrom(o);
        deepCopyVarsFrom(o);
    }
    DRW_Dimstyle& operator=(const DRW_Dimstyle& o) {
        if (this != &o) {
            clearVars();
            DRW_TableEntry::operator=(o);
            copyPodFrom(o);
            deepCopyVarsFrom(o);
        }
        return *this;
    }
    DRW_Dimstyle(DRW_Dimstyle&& o) : DRW_TableEntry(o) {
        copyPodFrom(o);
        vars = std::move(o.vars);
        o.vars.clear();
    }
    DRW_Dimstyle& operator=(DRW_Dimstyle&& o) {
        if (this != &o) {
            clearVars();
            DRW_TableEntry::operator=(o);
            copyPodFrom(o);
            vars = std::move(o.vars);
            o.vars.clear();
        }
        return *this;
    }

    void add(const std::string& key, int code, int value) { delete get(key); vars[key] = new DRW_Variant(code, value); }
    void add(const std::string& key, int code, double value) { delete get(key); vars[key] = new DRW_Variant(code, value); }
    void add(const std::string& key, int code, std::string value) { delete get(key); vars[key] = new DRW_Variant(code, UTF8STRING(value)); }
    DRW_Variant* get(const std::string& key) const {
        auto it = vars.find(key);
        return (it != vars.end()) ? it->second : nullptr;
    }

    // Populate the vars map from the parsed/struct fields so the LibreCAD
    // createDimStyle consumer (which reads $DIM* keys) receives imported
    // values. Idempotent: never clobbers a key already present. (Phase 3A.0)
    void syncStructToVars();

    void reset(){
        tType = DRW::DIMSTYLE;
        dimasz = dimtxt = dimexe = 0.18;
        dimexo = 0.0625;
        dimgap = dimcen = 0.09;
        dimtxsty = "Standard";
        dimscale = dimlfac = dimtfac = dimfxl = 1.0;
        dimdli = 0.38;
        dimrnd = dimdle = dimtp = dimtm = dimtsz = dimtvp = 0.0;
        dimaltf = 25.4;
        dimtol = dimlim = dimse1 = dimse2 = dimtad = dimzin = 0;
        dimtoh = dimtolj = 1;
        dimalt = dimtofl = dimsah = dimtix = dimsoxd = dimfxlon = 0;
        dimaltd = dimunit = dimaltu = dimalttd = dimlunit = 2;
        dimclrd = dimclre = dimclrt = dimjust = dimupt = 0;
        dimazin = dimaltz = dimaltttz = dimtzin = dimfrac = 0;
        dimtih = dimadec = dimaunit = dimsd1 = dimsd2 = dimtmove = 0;
        dimaltrnd = 0.0;
        dimdec = dimtdec = 4;
        dimfit = dimatfit = 3;
        dimdsep = '.';
        dimlwd = dimlwe = -2;
        // Phase 3A.1: R2007+/R2010+ members + handle refs.
        dimjogang = 0.0;
        dimtfill = dimtfillclr = dimarcsym = 0;
        dimtxtdirection = 0;
        dimaltmzf = dimmzf = 1.0;
        dimaltmzs.clear();
        dimmzs.clear();
        dimtxstyH = dimldrblkH = dimblkH = dimblk1H = dimblk2H = dwgHandle{};
        dimltypeH = dimltex1H = dimltex2H = dwgHandle{};
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;

    //V12
    UTF8STRING dimpost;       /*!< code 3 */
    UTF8STRING dimapost;      /*!< code 4 */
/* handle are code 105 */
    UTF8STRING dimblk;        /*!< code 5, code 342 V2000+ */
    UTF8STRING dimblk1;       /*!< code 6, code 343 V2000+ */
    UTF8STRING dimblk2;       /*!< code 7, code 344 V2000+ */
    double dimscale;          /*!< code 40 */
    double dimasz;            /*!< code 41 */
    double dimexo;            /*!< code 42 */
    double dimdli;            /*!< code 43 */
    double dimexe;            /*!< code 44 */
    double dimrnd;            /*!< code 45 */
    double dimdle;            /*!< code 46 */
    double dimtp;             /*!< code 47 */
    double dimtm;             /*!< code 48 */
    double dimfxl;            /*!< code 49 V2007+ */
    double dimtxt;            /*!< code 140 */
    double dimcen;            /*!< code 141 */
    double dimtsz;            /*!< code 142 */
    double dimaltf;           /*!< code 143 */
    double dimlfac;           /*!< code 144 */
    double dimtvp;            /*!< code 145 */
    double dimtfac;           /*!< code 146 */
    double dimgap;            /*!< code 147 */
    double dimaltrnd;         /*!< code 148 V2000+ */
    int dimtol;               /*!< code 71 */
    int dimlim;               /*!< code 72 */
    int dimtih;               /*!< code 73 */
    int dimtoh;               /*!< code 74 */
    int dimse1;               /*!< code 75 */
    int dimse2;               /*!< code 76 */
    int dimtad;               /*!< code 77 */
    int dimzin;               /*!< code 78 */
    int dimazin;              /*!< code 79 V2000+ */
    int dimalt;               /*!< code 170 */
    int dimaltd;              /*!< code 171 */
    int dimtofl;              /*!< code 172 */
    int dimsah;               /*!< code 173 */
    int dimtix;               /*!< code 174 */
    int dimsoxd;              /*!< code 175 */
    int dimclrd;              /*!< code 176 */
    int dimclre;              /*!< code 177 */
    int dimclrt;              /*!< code 178 */
    int dimadec;              /*!< code 179 V2000+ */
    int dimunit;              /*!< code 270 R13+ (obsolete 2000+, use dimlunit & dimfrac) */
    int dimdec;               /*!< code 271 R13+ */
    int dimtdec;              /*!< code 272 R13+ */
    int dimaltu;              /*!< code 273 R13+ */
    int dimalttd;             /*!< code 274 R13+ */
    int dimaunit;             /*!< code 275 R13+ */
    int dimfrac;              /*!< code 276 V2000+ */
    int dimlunit;             /*!< code 277 V2000+ */
    int dimdsep;              /*!< code 278 V2000+ */
    int dimtmove;             /*!< code 279 V2000+ */
    int dimjust;              /*!< code 280 R13+ */
    int dimsd1;               /*!< code 281 R13+ */
    int dimsd2;               /*!< code 282 R13+ */
    int dimtolj;              /*!< code 283 R13+ */
    int dimtzin;              /*!< code 284 R13+ */
    int dimaltz;              /*!< code 285 R13+ */
    int dimaltttz;            /*!< code 286 R13+ */
    int dimfit;               /*!< code 287 R13+  (obsolete 2000+, use dimatfit & dimtmove)*/
    int dimupt;               /*!< code 288 R13+ */
    int dimatfit;             /*!< code 289 V2000+ */
    int dimfxlon;             /*!< code 290 V2007+ */
    UTF8STRING dimtxsty;      /*!< code 340 R13+ */
    UTF8STRING dimldrblk;     /*!< code 341 V2000+ */
    int dimlwd;               /*!< code 371 V2000+ */
    int dimlwe;               /*!< code 372 V2000+ */
    // Phase 3A.1: R2007+ numeric/color members (dwg.spec:4766-4790).
    // dimfxl/dimfxlon already exist above — do not re-add.
    double dimjogang {0.0};   /*!< code 50  BD  V2007+ */
    int dimtfill {0};         /*!< code 69  BS  V2007+ */
    int dimtfillclr {0};      /*!< code 70  CMC V2007+ */
    int dimarcsym {0};        /*!< code 90  BS  V2007+ */
    // Phase 3A.1: R2010+ members (dwg.spec:4842-4849).
    int dimtxtdirection {0};  /*!< code 294 B   V2010+ */
    double dimaltmzf {1.0};   /*!< BD  V2010+ */
    UTF8STRING dimaltmzs;     /*!< T   V2010+ */
    double dimmzf {1.0};      /*!< BD  V2010+ */
    UTF8STRING dimmzs;        /*!< T   V2010+ */
    // Phase 3A.1: resolved handle-stream refs (mirror DRW_Layer::lTypeH).
    dwgHandle dimtxstyH;      /*!< code 340 */
    dwgHandle dimldrblkH;     /*!< code 341 */
    dwgHandle dimblkH;        /*!< code 342 */
    dwgHandle dimblk1H;       /*!< code 343 */
    dwgHandle dimblk2H;       /*!< code 344 */
    dwgHandle dimltypeH;      /*!< code 345 */
    dwgHandle dimltex1H;      /*!< code 346 */
    dwgHandle dimltex2H;      /*!< code 347 */
    std::map<std::string, DRW_Variant*> vars; /*!< extra/override variables written after standard fields */

private:
    void clearVars() {
        for (auto& kv : vars) delete kv.second;
        vars.clear();
    }
    void deepCopyVarsFrom(const DRW_Dimstyle& o) {
        vars.clear();
        for (const auto& kv : o.vars)
            vars[kv.first] = kv.second ? new DRW_Variant(*kv.second) : nullptr;
    }
    // Copy every owned-by-value POD/string field (NOT the base members, NOT
    // vars). New value-type members added in later phases ride this list.
    void copyPodFrom(const DRW_Dimstyle& o) {
        dimpost = o.dimpost; dimapost = o.dimapost;
        dimblk = o.dimblk; dimblk1 = o.dimblk1; dimblk2 = o.dimblk2;
        dimscale = o.dimscale; dimasz = o.dimasz; dimexo = o.dimexo;
        dimdli = o.dimdli; dimexe = o.dimexe; dimrnd = o.dimrnd;
        dimdle = o.dimdle; dimtp = o.dimtp; dimtm = o.dimtm; dimfxl = o.dimfxl;
        dimtxt = o.dimtxt; dimcen = o.dimcen; dimtsz = o.dimtsz;
        dimaltf = o.dimaltf; dimlfac = o.dimlfac; dimtvp = o.dimtvp;
        dimtfac = o.dimtfac; dimgap = o.dimgap; dimaltrnd = o.dimaltrnd;
        dimtol = o.dimtol; dimlim = o.dimlim; dimtih = o.dimtih;
        dimtoh = o.dimtoh; dimse1 = o.dimse1; dimse2 = o.dimse2;
        dimtad = o.dimtad; dimzin = o.dimzin; dimazin = o.dimazin;
        dimalt = o.dimalt; dimaltd = o.dimaltd; dimtofl = o.dimtofl;
        dimsah = o.dimsah; dimtix = o.dimtix; dimsoxd = o.dimsoxd;
        dimclrd = o.dimclrd; dimclre = o.dimclre; dimclrt = o.dimclrt;
        dimadec = o.dimadec; dimunit = o.dimunit; dimdec = o.dimdec;
        dimtdec = o.dimtdec; dimaltu = o.dimaltu; dimalttd = o.dimalttd;
        dimaunit = o.dimaunit; dimfrac = o.dimfrac; dimlunit = o.dimlunit;
        dimdsep = o.dimdsep; dimtmove = o.dimtmove; dimjust = o.dimjust;
        dimsd1 = o.dimsd1; dimsd2 = o.dimsd2; dimtolj = o.dimtolj;
        dimtzin = o.dimtzin; dimaltz = o.dimaltz; dimaltttz = o.dimaltttz;
        dimfit = o.dimfit; dimupt = o.dimupt; dimatfit = o.dimatfit;
        dimfxlon = o.dimfxlon; dimtxsty = o.dimtxsty; dimldrblk = o.dimldrblk;
        dimlwd = o.dimlwd; dimlwe = o.dimlwe;
        // Phase 3A.1 members — all value types, plain member-wise copy.
        dimjogang = o.dimjogang; dimtfill = o.dimtfill;
        dimtfillclr = o.dimtfillclr; dimarcsym = o.dimarcsym;
        dimtxtdirection = o.dimtxtdirection;
        dimaltmzf = o.dimaltmzf; dimaltmzs = o.dimaltmzs;
        dimmzf = o.dimmzf; dimmzs = o.dimmzs;
        dimtxstyH = o.dimtxstyH; dimldrblkH = o.dimldrblkH;
        dimblkH = o.dimblkH; dimblk1H = o.dimblk1H; dimblk2H = o.dimblk2H;
        dimltypeH = o.dimltypeH; dimltex1H = o.dimltex1H; dimltex2H = o.dimltex2H;
    }
};


//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*  @author Rallaz
*/
/*TODO: handle complex lineType*/
class DRW_LType : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_LType() { reset();}

    void reset(){
        tType = DRW::LTYPE;
        desc = "";
        size = 0;
        length = 0.0;
        pathIdx = 0;
        DRW_TableEntry::reset();
    }

public:
    void updateValues(const UTF8STRING &lTypeName, const UTF8STRING &ltDescription, int ltSize, double ltLength, const std::vector<double> &ltPath) {
        reset();
        name = lTypeName;
        desc = ltDescription;
        size = ltSize;
        length = ltLength;
        path.clear();
        for (auto it: ltPath) { path.push_back(it); }
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    void update();

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;
    UTF8STRING desc;           /*!< descriptive string, code 3 */
//    int align;               /*!< align code, always 65 ('A') code 72 */
    int size;                 /*!< element number, code 73 */
    double length;            /*!< total length of pattern, code 40 */
//    int haveShape;      /*!< complex linetype type, code 74 */
    std::vector<double> path;  /*!< trace, point or space length sequence, code 49 */
private:
    int pathIdx;
};


//! Class to handle layer entries
/*!
*  Class to handle layer symbol table entries
*  @author Rallaz
*/
class DRW_Layer : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Layer() { reset();}

    void reset() {
        tType = DRW::LAYER;
        lineType = "CONTINUOUS";
        color = 7; // default BYLAYER (256)
        plotF = true; // default TRUE (plot yes)
        lWeight = DRW_LW_Conv::widthDefault; // default BYDEFAULT (dxf -3, dwg 31)
        color24 = -1; //default -1 not set
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;
    UTF8STRING lineType;            /*!< line type, code 6 */
    int color;                      /*!< layer color, code 62 */
    int color24;                    /*!< 24-bit color, code 420 */
    UTF8STRING colorName;           /*!< color book name, code 430 ("BOOK$ENTRY") */
    bool plotF;                     /*!< Plot flag, code 290 */
    enum DRW_LW_Conv::lineWidth lWeight; /*!< layer lineweight, code 370 */
    std::string handlePlotS;        /*!< Hard-pointer ID/handle of plotstyle, code 390 */
    std::string handleMaterialS;        /*!< Hard-pointer ID/handle of materialstyle, code 347 */
/*only used for read dwg*/
    dwgHandle lTypeH;
};

//! Class to handle block record entries
/*!
*  Class to handle block record table entries
*  @author Rallaz
*/
class DRW_Block_Record : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Block_Record() { reset();}
    void reset() {
        tType = DRW::BLOCK_RECORD;
        flags = 0;
        insUnits = 0;
        description.clear();
        canExplode = true;
        blockScaling = 0;
        layoutHandle = 0;
        firstEH = lastEH = DRW::NoHandle;
        DRW_TableEntry::reset();
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
//Note:    int DRW_TableEntry::flags; contains code 70 of block
    int insUnits;             /*!< block insertion units, code 70 of block_record*/
    DRW_Coord basePoint;      /*!<  block insertion base point dwg only */
    UTF8STRING xrefPath;      /*!< Xref path name for XREF block_records (DWG: parsed from BLOCK_HEADER, DXF: code 1) */
    UTF8STRING description;   /*!< block description (DXF 4); DWG read */
    bool canExplode = true;   /*!< whether the block can be exploded (DXF 280); R2007+ DWG */
    std::uint8_t blockScaling = 0;  /*!< block scaling flag (DXF 281); R2007+ DWG */
    std::uint32_t layoutHandle = 0; /*!< soft ptr to the owning LAYOUT (DXF 340); DWG read */
protected:
    //dwg parser
private:
    std::uint32_t block;   //handle for block entity
    std::uint32_t endBlock;//handle for end block entity
    std::uint32_t firstEH; //handle of first entity, only in pre-2004
    std::uint32_t lastEH;  //handle of last entity, only in pre-2004
    std::vector<std::uint32_t>entMap;
};

//! Class to handle text style entries
/*!
*  Class to handle text style symbol table entries
*  @author Rallaz
*/
class DRW_Textstyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Textstyle() { reset();}

    void reset(){
        tType = DRW::STYLE;
        height = oblique = 0.0;
        width = lastHeight = 1.0;
        font="txt";
        genFlag = 0; //2= X mirror, 4= Y mirror
        fontFamily = 0;
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;
    double height;          /*!< Fixed text height (0 not set), code 40 */
    double width;           /*!< Width factor, code 41 */
    double oblique;         /*!< Oblique angle, code 50 */
    int genFlag;            /*!< Text generation flags, code 71 */
    double lastHeight;      /*!< Last height used, code 42 */
    UTF8STRING font;        /*!< primary font file name, code 3 */
    UTF8STRING bigFont;     /*!< bigfont file name or blank if none, code 4 */
    int fontFamily;         /*!< ttf font family, italic and bold flags, code 1071 */
};

//! Class to handle vport entries
/*!
*  Class to handle vport symbol table entries
*  @author Rallaz
*/
class DRW_Vport : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Vport() { reset();}

    void reset(){
        tType = DRW::VPORT;
        UpperRight.x = UpperRight.y = 1.0;
        snapSpacing.x = snapSpacing.y = 10.0;
        gridSpacing = snapSpacing;
        center.x = 0.651828;
        center.y = -0.16;
        viewDir.z = 1;
        height = 5.13732;
        ratio = 2.4426877;
        lensHeight = 50;
        frontClip = backClip = snapAngle = twistAngle = 0.0;
        viewMode = snap = grid = snapStyle = snapIsopair = 0;
        fastZoom = 1;
        circleZoom = 100;
	        ucsIcon = 3;
	        gridBehavior = 7;
	        visualStyleHandle = 0;
	        m_sunHandle = 0;
            backgroundHandle = 0;
            namedUcsHandle = 0;
            baseUcsHandle = 0;
            ucsOrigin = DRW_Coord(0.0, 0.0, 0.0);
            ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
            ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
            ucsElevation = 0.0;
            ucsOrthoType = 0;
            ucsPerVP = false;
	        DRW_TableEntry::reset();
	    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;
    DRW_Coord lowerLeft;     /*!< Lower left corner, code 10 & 20 */
    DRW_Coord UpperRight;    /*!< Upper right corner, code 11 & 21 */
    DRW_Coord center;        /*!< center point in WCS, code 12 & 22 */
    DRW_Coord snapBase;      /*!< snap base point in DCS, code 13 & 23 */
    DRW_Coord snapSpacing;   /*!< snap Spacing, code 14 & 24 */
    DRW_Coord gridSpacing;   /*!< grid Spacing, code 15 & 25 */
    DRW_Coord viewDir;       /*!< view direction from target point, code 16, 26 & 36 */
    DRW_Coord viewTarget;    /*!< view target point, code 17, 27 & 37 */
    double height;           /*!< view height, code 40 */
    double ratio;            /*!< viewport aspect ratio, code 41 */
    double lensHeight;       /*!< lens height, code 42 */
    double frontClip;        /*!< front clipping plane, code 43 */
    double backClip;         /*!< back clipping plane, code 44 */
    double snapAngle;        /*!< snap rotation angle, code 50 */
    double twistAngle;       /*!< view twist angle, code 51 */
    int viewMode;            /*!< view mode, code 71 */
    int circleZoom;          /*!< circle zoom percent, code 72 */
    int fastZoom;            /*!< fast zoom setting, code 73 */
    int ucsIcon;             /*!< UCSICON setting, code 74 */
    int snap;                /*!< snap on/off, code 75 */
    int grid;                /*!< grid on/off, code 76 */
    int snapStyle;           /*!< snap style, code 77 */
    int snapIsopair;         /*!< snap isopair, code 78 */
    int gridBehavior;        /*!< grid behavior, code 60, undocummented */
    /** code 60, bit coded possible value are
    * bit 1 (1) show out of limits
    * bit 2 (2) adaptive grid
    * bit 3 (4) allow subdivision
    * bit 4 (8) follow dynamic SCP
    **/
	    std::uint32_t visualStyleHandle = 0; /*!< R2007+ visual-style ref (DWG-only) */
	    std::uint32_t m_sunHandle = 0;         /*!< R2007+ SUN hard-owner ref (DWG-only) */
        std::uint32_t backgroundHandle = 0;  /*!< R2007+ background ref (DWG-only) */
        std::uint32_t namedUcsHandle = 0;    /*!< R2000+ named UCS ref (DWG-only) */
        std::uint32_t baseUcsHandle = 0;     /*!< R2000+ base UCS ref (DWG-only) */
        DRW_Coord ucsOrigin;           /*!< R2000+ per-viewport UCS origin (DWG-only) */
        DRW_Coord ucsXAxis;            /*!< R2000+ per-viewport UCS X axis (DWG-only) */
        DRW_Coord ucsYAxis;            /*!< R2000+ per-viewport UCS Y axis (DWG-only) */
        double ucsElevation = 0.0;     /*!< R2000+ per-viewport UCS elevation (DWG-only) */
        int ucsOrthoType = 0;          /*!< R2000+ UCS orthographic view type (DWG-only) */
        bool ucsPerVP = false;         /*!< R2000+ UCS-per-viewport flag (DWG-only) */
};


//! Class to handle imagedef entries
/*!
*  Class to handle image definitions object entries
*  @author Rallaz
*/
class DRW_ImageDef : public DRW_TableEntry {//
    SETOBJFRIENDS
public:
    DRW_ImageDef() {
        reset();
    }

    void reset(){
        tType = DRW::IMAGEDEF;
        imgVersion = 0;
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
//    std::string handle;       /*!< entity identifier, code 5 */
    UTF8STRING name;          /*!< File name of image, code 1 */
    int imgVersion;              /*!< class version, code 90, 0=R14 version */
    double u;                 /*!< image size in pixels U value, code 10 */
    double v;                 /*!< image size in pixels V value, code 20 */
    double up;                /*!< default size of one pixel U value, code 11 */
    double vp;                /*!< default size of one pixel V value, code 12 really is 21*/
    int loaded;               /*!< image is loaded flag, code 280, 0=unloaded, 1=loaded */
    int resolution;           /*!< resolution units, code 281, 0=no, 2=centimeters, 5=inch */

    std::map<std::string,std::string> reactors;
};

//! Class to handle plotsettings entries
/*!
*  Class to handle plot settings object entries
*  @author baranovskiykonstantin@gmail.com
*/
class DRW_PlotSettings : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_PlotSettings() {
        reset();
    }

    void reset(){
        tType = DRW::PLOTSETTINGS;
        // Full plot field set (P4-01) — member names mirror DRW_Layout's plot
        // prefix so the read/encode code can be shared verbatim.
        pageSetupName.clear();
        printerConfig.clear();
        plotLayoutFlags = 0;
        marginLeft = marginBottom = marginRight = marginTop = 0.0;
        paperWidth = paperHeight = 0.0;
        paperSize.clear();
        plotOriginX = plotOriginY = 0.0;
        paperUnits = 0;
        plotRotation = 0;
        plotType = 0;
        windowMinX = windowMinY = windowMaxX = windowMaxY = 0.0;
        plotViewName.clear();
        realWorldUnits = 1.0;
        drawingUnits = 1.0;
        currentStyleSheet.clear();
        scaleType = 0;
        scaleFactor = 1.0;
        paperImageOriginX = paperImageOriginY = 0.0;
        shadePlotMode = 0;
        shadePlotResLevel = 0;
        shadePlotCustomDPI = 0;
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    // Full PlotSettings field set per ODA §20.4.84 (mirrors DRW_Layout's
    // embedded plot prefix; member names kept identical so P4-02's lifted
    // parse/encode is a verbatim copy of the Layout path).
    UTF8STRING pageSetupName;     /*!< code 1, TV */
    UTF8STRING printerConfig;     /*!< code 2, TV */
    int plotLayoutFlags;          /*!< code 70, BS */
    double marginLeft;            /*!< code 40, BD, millimeters */
    double marginBottom;          /*!< code 41, BD */
    double marginRight;           /*!< code 42, BD */
    double marginTop;             /*!< code 43, BD */
    double paperWidth;            /*!< code 44, BD */
    double paperHeight;           /*!< code 45, BD */
    UTF8STRING paperSize;         /*!< code 4, TV */
    double plotOriginX;           /*!< code 46, 2BD */
    double plotOriginY;           /*!< code 47, 2BD */
    int paperUnits;               /*!< code 72, BS */
    int plotRotation;             /*!< code 73, BS */
    int plotType;                 /*!< code 74, BS */
    double windowMinX;            /*!< code 48, 2BD */
    double windowMinY;            /*!< code 49, 2BD */
    double windowMaxX;            /*!< code 140, 2BD */
    double windowMaxY;            /*!< code 141, 2BD */
    UTF8STRING plotViewName;      /*!< code 6, T (R13-R2000 only) */
    double realWorldUnits;        /*!< code 142, BD — numerator of print scale */
    double drawingUnits;          /*!< code 143, BD — denominator of print scale */
    UTF8STRING currentStyleSheet; /*!< code 7, TV */
    int scaleType;                /*!< code 75, BS — standard scale type */
    double scaleFactor;           /*!< code 147, BD */
    double paperImageOriginX;     /*!< code 148, 2BD */
    double paperImageOriginY;     /*!< code 149, 2BD */
    int shadePlotMode;            /*!< code 76, BS (R2004+) */
    int shadePlotResLevel;        /*!< code 77, BS (R2004+) */
    int shadePlotCustomDPI;       /*!< code 78, BS (R2004+) */
};

//! Class to handle UCS entries
/*!
*  Class to handle UCS symbol-table entries (named user coordinate systems).
*/
class DRW_UCS : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_UCS() { reset(); }

    void reset(){
        tType = DRW::UCS;
        origin.x = origin.y = origin.z = 0.0;
        xAxisDirection.x = xAxisDirection.y = xAxisDirection.z = 0.0;
        yAxisDirection.x = yAxisDirection.y = yAxisDirection.z = 0.0;
        orthoOrigin.x = orthoOrigin.y = orthoOrigin.z = 0.0;
        elevation = 0.0;
        orthoType = 0;
        baseUcsHandle = dwgHandle{};
        namedUcsHandle = dwgHandle{};
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    DRW_Coord origin;           /*!< UCS origin, codes 10/20/30 */
    DRW_Coord xAxisDirection;   /*!< UCS X-axis direction, codes 11/21/31 */
    DRW_Coord yAxisDirection;   /*!< UCS Y-axis direction, codes 12/22/32 */
    DRW_Coord orthoOrigin;      /*!< Origin for orthographic UCS, codes 13/23/33 */
    double elevation;           /*!< Elevation, code 146 */
    int orthoType;              /*!< Orthographic type, code 71/79 (0 none, 1 Top, ...) */
    // Phase 4 (P4-04): resolved handle-stream refs (R2000+).
    dwgHandle baseUcsHandle;    /*!< base UCS handle (code 346) */
    dwgHandle namedUcsHandle;   /*!< named UCS handle (code 345) */
};

//! Class to handle VIEW entries
/*!
*  Class to handle VIEW symbol-table entries (named views).
*/
class DRW_View : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_View() { reset(); }

    void reset(){
        tType = DRW::VIEW;
        size.x = size.y = size.z = 0.0;
        center.x = center.y = center.z = 0.0;
        viewDirectionFromTarget.x = viewDirectionFromTarget.y = viewDirectionFromTarget.z = 0.0;
        targetPoint.x = targetPoint.y = targetPoint.z = 0.0;
        lensLen = 0.0;
        frontClippingPlaneOffset = 0.0;
        backClippingPlaneOffset = 0.0;
        twistAngle = 0.0;
        viewMode = 0;
        renderMode = 0;
        hasUCS = false;
        cameraPlottable = false;
        ucsOrigin.x = ucsOrigin.y = ucsOrigin.z = 0.0;
        ucsXAxis.x = ucsXAxis.y = ucsXAxis.z = 0.0;
        ucsYAxis.x = ucsYAxis.y = ucsYAxis.z = 0.0;
        ucsOrthoType = 1;
        ucsElevation = 0.0;
        namedUCS_ID = 0;
        baseUCS_ID = 0;
        m_useDefaultLights = true;
        m_defaultLightingType = 1;
        m_brightness = 0.0;
        m_contrast = 0.0;
        m_ambientColor = 250;
        m_backgroundHandle = 0;
        m_visualStyleHandle = 0;
        m_sunHandle = 0;
        m_liveSectionHandle = 0;
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *hdlBuf = nullptr) const;

public:
    DRW_Coord size;                     /*!< View width/height in DCS, codes 40 & 41 */
    DRW_Coord center;                   /*!< View center point in DCS, codes 10 & 20 */
    DRW_Coord viewDirectionFromTarget;  /*!< View direction from target in WCS, codes 11/21/31 */
    DRW_Coord targetPoint;              /*!< Target point in WCS, codes 12/22/32 */
    double lensLen;                     /*!< Lens length, code 42 */
    double frontClippingPlaneOffset;    /*!< Front clipping plane offset from target, code 43 */
    double backClippingPlaneOffset;     /*!< Back clipping plane offset from target, code 44 */
    double twistAngle;                  /*!< Twist angle, code 50 */
    int viewMode;                       /*!< View mode, code 71 */
    unsigned int renderMode;            /*!< Render mode, code 281 */
    bool hasUCS;                        /*!< 1 if a UCS is associated, code 72 */
    bool cameraPlottable;               /*!< 1 if camera is plottable, code 73 */
    DRW_Coord ucsOrigin;                /*!< UCS origin, codes 110/120/130 */
    DRW_Coord ucsXAxis;                 /*!< UCS X axis, codes 111/121/131 */
    DRW_Coord ucsYAxis;                 /*!< UCS Y axis, codes 112/122/132 */
    int ucsOrthoType;                   /*!< Orthographic type, code 79 */
    double ucsElevation;                /*!< UCS elevation, code 146 */
    std::uint32_t namedUCS_ID;                /*!< Handle of named UCS table record, code 345 */
    std::uint32_t baseUCS_ID;                 /*!< Handle of base UCS table record, code 346 */
    bool m_useDefaultLights = true;     /*!< R2007+ default-lighting flag (DWG-only) */
    std::uint8_t m_defaultLightingType = 1;   /*!< R2007+ default-lighting type (DWG-only) */
    double m_brightness = 0.0;          /*!< R2007+ view brightness (DWG-only) */
    double m_contrast = 0.0;            /*!< R2007+ view contrast (DWG-only) */
    std::uint32_t m_ambientColor = 250;       /*!< R2007+ ambient CMC indexed color (DWG-only) */
    std::uint32_t m_backgroundHandle = 0;     /*!< R2007+ background soft-pointer ref (DWG-only) */
    std::uint32_t m_visualStyleHandle = 0;    /*!< R2007+ visual-style hard-pointer ref (DWG-only) */
    std::uint32_t m_sunHandle = 0;            /*!< R2007+ SUN hard-owner ref (DWG-only) */
    std::uint32_t m_liveSectionHandle = 0;    /*!< R2007+ live-section soft-pointer ref (DWG-only) */
};

//! Class to handle AppId entries
/*!
*  Class to handle AppId symbol table entries
*  @author Rallaz
*/
class DRW_AppId : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_AppId() { reset();}

    void reset(){
        tType = DRW::APPID;
        flags = 0;
        name = "";
        DRW_TableEntry::reset();
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr, dwgBufferW *hdlBuf = nullptr) const;
};

//! Class to handle Dictionary (ODA spec sec 20.4.44 fixed type 42)
/*!
*  Carrier for named-object dictionaries, including entry names and handles.
*/
class DRW_Dictionary : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Dictionary() { reset(); }
    void reset(){
        tType = DRW::DICTIONARY;
        cloning = 0;
        hardOwner = 0;
        name.clear();
        m_entries.clear();
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    struct Entry {
        UTF8STRING m_name;
        std::uint32_t m_handle = 0;
    };
    int cloning;     /*!< duplicate-record handling (BS) */
    int hardOwner;   /*!< hard-owner flag (RC, R2007+) */
    std::vector<Entry> m_entries; /*!< dictionary entry names and item handles */
};

//! Class to handle DICTIONARYWDFLT (AcDbDictionaryWithDefault).
class DRW_DictionaryWithDefault : public DRW_Dictionary {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 513;

    DRW_DictionaryWithDefault() { reset(); }
    void reset(){
        DRW_Dictionary::reset();
        tType = DRW::DICTIONARYWDFLT;
        m_defaultEntryHandle = 0;
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::uint32_t m_defaultEntryHandle = 0;
};

//! Class to handle DICTIONARYVAR (AcDbDictionaryVar).
class DRW_DictionaryVar : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 512;

    DRW_DictionaryVar() { reset(); }
    void reset(){
        tType = DRW::DICTIONARYVAR;
        m_schema = 0;
        m_value.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    int m_schema = 0;       /*!< code 280 */
    UTF8STRING m_value;     /*!< code 1 */
};

//! Class to handle XRECORD (fixed type 0x4f).
class DRW_XRecord : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_XRecord() { reset(); }
    void reset(){
        tType = DRW::XRECORD;
        m_cloning = 0;
        m_values.clear();
        m_handleValues.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    int m_cloning = 0; /*!< duplicate-record handling, code 280 */
    std::vector<DRW_Variant> m_values;
    std::vector<std::pair<int, std::uint32_t>> m_handleValues; /*!< DXF code + object id */
};

//! Value payload used by FIELD and TABLE/TABLECONTENT objects.
struct DRW_CadValue {
    int m_formatFlags = 0;
    int m_dataType = 0;
    std::uint32_t m_dataSize = 0;
    int m_unitType = 0;
    DRW_Variant m_value;
    UTF8STRING m_formatString;
    UTF8STRING m_valueString;
    std::uint32_t m_handle = 0;
    std::vector<std::uint8_t> m_rawData;
};

//! Class to handle FIELD (AcDbField).
class DRW_Field : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 516;

    struct ChildValue {
        UTF8STRING m_key;
        DRW_CadValue m_value;
    };

    DRW_Field() { reset(); }
    void reset(){
        tType = DRW::FIELD;
        m_evaluatorId.clear();
        m_fieldCode.clear();
        m_formatString.clear();
        m_evaluationOptionFlags = 0;
        m_filingOptionFlags = 0;
        m_fieldStateFlags = 0;
        m_evaluationStatusFlags = 0;
        m_evaluationErrorCode = 0;
        m_evaluationErrorMessage.clear();
        m_value = DRW_CadValue();
        m_valueString.clear();
        m_valueStringLength = 0;
        m_childHandles.clear();
        m_objectHandles.clear();
        m_childValues.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    UTF8STRING m_evaluatorId;
    UTF8STRING m_fieldCode;
    UTF8STRING m_formatString;
    int m_evaluationOptionFlags = 0;
    int m_filingOptionFlags = 0;
    int m_fieldStateFlags = 0;
    int m_evaluationStatusFlags = 0;
    int m_evaluationErrorCode = 0;
    UTF8STRING m_evaluationErrorMessage;
    DRW_CadValue m_value;
    UTF8STRING m_valueString;
    int m_valueStringLength = 0;
    std::vector<std::uint32_t> m_childHandles;
    std::vector<std::uint32_t> m_objectHandles;
    std::vector<ChildValue> m_childValues;
};

//! Class to handle FIELDLIST (AcDbFieldList).
class DRW_FieldList : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 515;

    DRW_FieldList() { reset(); }
    void reset(){
        tType = DRW::FIELDLIST;
        m_unknown = 0;
        m_fieldHandles.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    int m_unknown = 0;
    std::vector<std::uint32_t> m_fieldHandles;
};

//! Class to handle RASTERVARIABLES (AcDbRasterVariables).
class DRW_RasterVariables : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 505;

    DRW_RasterVariables() { reset(); }
    void reset(){
        tType = DRW::RASTERVARIABLES;
        m_classVersion = 0;
        m_imageFrame = 0;
        m_imageQuality = 0;
        m_units = 0;
        DRW_TableEntry::reset();
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    int m_classVersion = 0;
    int m_imageFrame = 0;
    int m_imageQuality = 0;
    int m_units = 0;
};

//! Class to handle WIPEOUTVARIABLES (AcDbWipeoutVariables) — the drawing-wide
//! wipeout/image display-frame flag. ODA / libreDWG dwg2.spec WIPEOUTVARIABLES.
class DRW_WipeoutVariables : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_WipeoutVariables() { reset(); }
    void reset(){
        tType = DRW::UNKNOWNT;
        m_displayFrame = 0;
        DRW_TableEntry::reset();
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    std::uint16_t m_displayFrame = 0;  /*!< global display-frame flag, DXF 70 */
};

//! Class to handle SORTENTSTABLE (AcDbSortentsTable).
class DRW_SortEntsTable : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 514;

    DRW_SortEntsTable() { reset(); }
    void reset(){
        tType = DRW::SORTENTSTABLE;
        m_sortHandles.clear();
        m_blockOwnerHandle = 0;
        m_entityHandles.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::vector<std::uint32_t> m_sortHandles;
    std::uint32_t m_blockOwnerHandle = 0;
    std::vector<std::uint32_t> m_entityHandles;
};

//! Class to handle MATERIAL (AcDbMaterial) identity fields.
class DRW_Material : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Material() { reset(); }
    void reset(){
        tType = DRW::MATERIAL;
        m_name.clear();
        m_description.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    UTF8STRING m_name;
    UTF8STRING m_description;
};

struct DRW_TableStyleBorder {
    int m_edgeFlags = 0;
    int m_propertyOverrideFlags = 0;
    int m_borderType = 0;
    int m_color = 0;
    int m_lineWeight = 0;
    std::uint32_t m_lineTypeHandle = 0;
    int m_visible = 0;
    double m_doubleLineSpacing = 0.0;
};

struct DRW_TableStyleContentFormat {
    std::uint32_t m_propertyOverrideFlags = 0;
    std::uint32_t m_propertyFlags = 0;
    int m_valueDataType = 0;
    int m_valueUnitType = 0;
    UTF8STRING m_valueFormatString;
    double m_rotation = 0.0;
    double m_blockScale = 1.0;
    int m_cellAlignment = 0;
    int m_contentColor = 0;
    std::uint32_t m_textStyleHandle = 0;
    double m_textHeight = 0.0;
};

struct DRW_TableStyleCellStyle {
    int m_type = 0;
    bool m_hasData = false;
    std::uint32_t m_propertyOverrideFlags = 0;
    std::uint32_t m_mergeFlags = 0;
    int m_backgroundColor = 0;
    std::uint32_t m_contentLayout = 0;
    DRW_TableStyleContentFormat m_contentFormat;
    int m_marginOverrideFlags = 0;
    double m_verticalMargin = 0.0;
    double m_horizontalMargin = 0.0;
    double m_bottomMargin = 0.0;
    double m_rightMargin = 0.0;
    double m_marginHorizontalSpacing = 0.0;
    double m_marginVerticalSpacing = 0.0;
    int m_id = 0;
    int m_styleClass = 0;
    UTF8STRING m_name;
    std::vector<DRW_TableStyleBorder> m_borders;
};

struct DRW_TableStyleRowStyle {
    std::uint32_t m_textStyleHandle = 0;
    double m_textHeight = 0.0;
    int m_textAlignment = 0;
    int m_textColor = 0;
    int m_fillColor = 0;
    bool m_hasBackgroundColor = false;
    int m_valueDataType = 0;
    int m_valueUnitType = 0;
    UTF8STRING m_valueFormatString;
    std::vector<DRW_TableStyleBorder> m_borders;
};

//! Class to handle TABLESTYLE (AcDbTableStyle) identity/core fields.
class DRW_TableStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_TableStyle() { reset(); }
    void reset(){
        tType = DRW::TABLESTYLE;
        m_name.clear();
        m_flowDirection = 0;
        m_flags = 0;
        m_horizontalCellMargin = 0.0;
        m_verticalCellMargin = 0.0;
        m_titleSuppressed = false;
        m_headerSuppressed = false;
        m_unknownHandle = 0;
        m_tableCellStyle = DRW_TableStyleCellStyle();
        m_rowStyles.clear();
        m_cellStyles.clear();
        m_subrecordRanges.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    UTF8STRING m_name;
    int m_flowDirection = 0;
    int m_flags = 0;
    double m_horizontalCellMargin = 0.0;
    double m_verticalCellMargin = 0.0;
    bool m_titleSuppressed = false;
    bool m_headerSuppressed = false;
    std::uint32_t m_unknownHandle = 0;
    DRW_TableStyleCellStyle m_tableCellStyle;
    std::vector<DRW_TableStyleRowStyle> m_rowStyles;
    std::vector<DRW_TableStyleCellStyle> m_cellStyles;
    std::vector<DRW_DwgSubrecordRange> m_subrecordRanges;
};

//! Class to handle CELLSTYLEMAP (AcDbCellStyleMap).
class DRW_CellStyleMap : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_CellStyleMap() { reset(); }
    void reset(){
        tType = DRW::CELLSTYLEMAP;
        m_cellStyles.clear();
        m_subrecordRanges.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    std::vector<DRW_TableStyleCellStyle> m_cellStyles;
    std::vector<DRW_DwgSubrecordRange> m_subrecordRanges;
};

//! Class to handle Layout (ODA spec sec 19.4.85 fixed type 82)
/*!
*  Minimal carrier for paperspace layouts; full field parsing pending.
*/
class DRW_Layout : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_Layout() { reset(); }
    void reset(){
        tType = DRW::LAYOUT;
        // PlotSettings prefix
        pageSetupName.clear();
        printerConfig.clear();
        plotLayoutFlags = 0;
        marginLeft = marginBottom = marginRight = marginTop = 0.0;
        paperWidth = paperHeight = 0.0;
        paperSize.clear();
        plotOriginX = plotOriginY = 0.0;
        paperUnits = 0;
        plotRotation = 0;
        plotType = 0;
        windowMinX = windowMinY = windowMaxX = windowMaxY = 0.0;
        plotViewName.clear();
        realWorldUnits = 1.0;
        drawingUnits = 1.0;
        currentStyleSheet.clear();
        scaleType = 0;
        scaleFactor = 1.0;
        paperImageOriginX = paperImageOriginY = 0.0;
        shadePlotMode = 0;
        shadePlotResLevel = 0;
        shadePlotCustomDPI = 0;
        // Layout-specific
        name.clear();
        tabOrder = 0;
        layoutFlags = 0;
        ucsOrigin = DRW_Coord();
        limMinX = limMinY = limMaxX = limMaxY = 0.0;
        insPoint = DRW_Coord();
        ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        elevation = 0.0;
        orthoViewType = 0;
        extMin = DRW_Coord();
        extMax = DRW_Coord();
        viewportCount = 0;
        // Handle refs
        plotViewHandle = dwgHandle{};
        visualStyleHandle = dwgHandle{};
        paperSpaceBlockRecordHandle = dwgHandle{};
        lastActiveViewportHandle = dwgHandle{};
        baseUcsHandle = dwgHandle{};
        namedUcsHandle = dwgHandle{};
        viewportHandles.clear();
        m_dxfSubclass = 0;
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
    //! Transient DXF-parse state: which 100-subclass we are in
    //! (0 = AcDbPlotSettings prefix, 1 = AcDbLayout). Disambiguates the
    //! codes 1/70/76/330 that appear in both subclasses.
    int m_dxfSubclass = 0;
public:
    // PlotSettings prefix per ODA §20.4.84 (the LAYOUT object embeds these inline)
    UTF8STRING pageSetupName;     /*!< code 1, TV */
    UTF8STRING printerConfig;     /*!< code 2, TV */
    int plotLayoutFlags;          /*!< code 70, BS */
    double marginLeft;            /*!< code 40, BD, millimeters */
    double marginBottom;          /*!< code 41, BD */
    double marginRight;           /*!< code 42, BD */
    double marginTop;             /*!< code 43, BD */
    double paperWidth;            /*!< code 44, BD */
    double paperHeight;           /*!< code 45, BD */
    UTF8STRING paperSize;         /*!< code 4, TV */
    double plotOriginX;           /*!< code 46, 2BD */
    double plotOriginY;           /*!< code 47, 2BD */
    int paperUnits;               /*!< code 72, BS */
    int plotRotation;             /*!< code 73, BS */
    int plotType;                 /*!< code 74, BS */
    double windowMinX;            /*!< code 48, 2BD */
    double windowMinY;            /*!< code 49, 2BD */
    double windowMaxX;            /*!< code 140, 2BD */
    double windowMaxY;            /*!< code 141, 2BD */
    UTF8STRING plotViewName;      /*!< code 6, T (R13-R2000 only) */
    double realWorldUnits;        /*!< code 142, BD — numerator of print scale */
    double drawingUnits;          /*!< code 143, BD — denominator of print scale */
    UTF8STRING currentStyleSheet; /*!< code 7, TV */
    int scaleType;                /*!< code 75, BS — standard scale type */
    double scaleFactor;           /*!< code 147, BD */
    double paperImageOriginX;     /*!< code 148, 2BD */
    double paperImageOriginY;     /*!< code 149, 2BD */
    int shadePlotMode;            /*!< code 76, BS (R2004+) */
    int shadePlotResLevel;        /*!< code 77, BS (R2004+) */
    int shadePlotCustomDPI;       /*!< code 78, BS (R2004+) */
    // Layout-specific fields
    int layoutFlags;              /*!< code 70, BS */
    int tabOrder;                 /*!< code 71, BL */
    DRW_Coord ucsOrigin;          /*!< code 13, 3BD */
    double limMinX;               /*!< code 10, 2RD */
    double limMinY;
    double limMaxX;               /*!< code 11, 2RD */
    double limMaxY;
    DRW_Coord insPoint;           /*!< code 12, 3BD */
    DRW_Coord ucsXAxis;           /*!< code 16, 3BD */
    DRW_Coord ucsYAxis;           /*!< code 17, 3BD */
    double elevation;             /*!< code 146, BD */
    int orthoViewType;            /*!< code 76, BS — orthographic view type of UCS */
    DRW_Coord extMin;             /*!< code 14, 3BD — layout extent min */
    DRW_Coord extMax;             /*!< code 15, 3BD — layout extent max */
    std::int32_t viewportCount;         /*!< BL — viewport count (R2004+) */
    // Handle refs (the common parentHandle is on DRW_TableEntry)
    dwgHandle plotViewHandle;                 /*!< code 6 (R2004+) */
    dwgHandle visualStyleHandle;              /*!< (R2007+) */
    dwgHandle paperSpaceBlockRecordHandle;    /*!< code 330 */
    dwgHandle lastActiveViewportHandle;       /*!< code 331 */
    dwgHandle baseUcsHandle;                  /*!< code 346 */
    dwgHandle namedUcsHandle;                 /*!< code 345 */
    std::vector<std::uint32_t> viewportHandles;     /*!< viewport handles (R2004+) */
};

//! One parallel line element within a MLineStyle (ODA §19.4.73).
struct DRW_MLineElement {
    double offset = 0.0;        /*!< BD — perpendicular offset from centerline */
    int    color  = 256;        /*!< CMC index — 256 = ByLayer */
    int    color24 = -1;        /*!< true-color RGB (-1 = none) */
    int    linetypeIndex = -1;  /*!< BSd inline lt index PRE-R2018 (DXF 6); -1 = unset.
                                 *   R2018+ uses linetypeHandle instead (0B.4b). */
    std::uint32_t linetypeHandle = 0; /*!< H — linetype object reference */
    UTF8STRING linetype;        /*!< resolved linetype name (DXF 6) */
};

//! Class to handle MLineStyle (ODA spec sec 19.4.73 fixed type 73)
/*!
*  Carrier for multiline styles. Per-line `elements` define each parallel
*  line's offset, color and linetype. The MLINE entity references this
*  style by handle; entries are populated in addMLineStyle on import.
*/
class DRW_MLineStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_MLineStyle() { reset(); }
    void reset(){
        tType = DRW::MLINESTYLE;
        flags = 0;
        startAngle = endAngle = 0.0;
        fillColor = 256;
        name.clear();
        description.clear();
        elements.clear();
    }
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    int flags;                  /*!< style flags (BS) */
    double startAngle;          /*!< start angle (BD) */
    double endAngle;            /*!< end angle (BD) */
    UTF8STRING description;     /*!< description (TV / DXF 3) */
    int fillColor = 256;        /*!< CMC fill color / DXF 62 (256 = ByLayer) */
    std::vector<DRW_MLineElement> elements;
};

//! Class to handle MLEADERSTYLE (ODA spec §20.4.87, AcDbMLeaderStyle).
/*!
 *  MLEADER style dictionary entry.  Lives under the root ACAD_MLEADERSTYLE
 *  dictionary; each MLEADER entity carries a style handle that resolves to
 *  one of these.  Override flags on the MLEADER (BL 90) shadow individual
 *  fields here.
 */
class DRW_MLeaderStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 504;

    DRW_MLeaderStyle() {
        tType = DRW::MLEADERSTYLE;
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;

    /* Per §20.4.87.  Names follow the spec column for traceability. */
    std::uint16_t styleVersion = 2;          /*!< code 179, R2010+ */
    std::uint16_t contentType = 2;           /*!< code 170: 0=None,1=Block,2=MText,3=Tolerance */
    std::uint16_t drawMLeaderOrder = 0;      /*!< code 171: 0=content first,1=leader head first */
    std::uint16_t drawLeaderOrder = 0;       /*!< code 172 */
    std::int32_t maxLeaderPoints = 0;        /*!< code 90 */
    double firstSegmentAngle = 0.0;    /*!< code 40 (radians) */
    double secondSegmentAngle = 0.0;   /*!< code 41 (radians) */
    std::uint16_t leaderType = 1;            /*!< code 173 */
    int leaderColor = 0;               /*!< code 91 (CMC) */
    dwgHandle leaderLineTypeHandle{};  /*!< code 340 (handle stream) */
    std::int32_t leaderLineWeight = 0;       /*!< code 92 */
    bool landingEnabled = true;        /*!< code 290 */
    double landingGap = 0.0;           /*!< code 42 */
    bool autoIncludeLanding = true;    /*!< code 291 */
    double landingDistance = 0.0;      /*!< code 43 */
    UTF8STRING description;            /*!< code 3 */
    dwgHandle arrowHeadBlockHandle{};  /*!< code 341 */
    double arrowHeadSize = 0.0;        /*!< code 44 */
    UTF8STRING textDefault;            /*!< code 300 */
    dwgHandle textStyleHandle{};       /*!< code 342 */
    std::uint16_t leftAttachment = 0;        /*!< code 174 */
    std::uint16_t rightAttachment = 0;       /*!< code 178 */
    std::uint16_t textAngleType = 0;         /*!< code 175 (R2010+) */
    std::uint16_t textAlignmentType = 0;     /*!< code 176 */
    int textColor = 0;                 /*!< code 93 */
    double textHeight = 0.0;           /*!< code 45 */
    bool textFrameEnabled = false;     /*!< code 292 */
    bool alwaysAlignTextLeft = false;  /*!< code 297 */
    double alignSpace = 0.0;           /*!< code 46 */
    dwgHandle blockHandle{};           /*!< code 343 */
    int blockColor = 0;                /*!< code 94 */
    DRW_Coord blockScale{1, 1, 1};     /*!< code 47/49/140 */
    bool blockScaleEnabled = false;    /*!< code 293 */
    double blockRotation = 0.0;        /*!< code 141 (radians) */
    bool blockRotationEnabled = false; /*!< code 294 */
    std::uint16_t blockConnectionType = 0;   /*!< code 177 */
    double scaleFactor = 1.0;          /*!< code 142 */
    bool propertyChanged = false;      /*!< code 295 */
    bool isAnnotative = false;         /*!< code 296 */
    double breakSize = 0.0;            /*!< code 143 */
    /* R2010+ */
    std::uint16_t attachmentDirection = 0;   /*!< code 271 */
    std::uint16_t topAttachment = 0;         /*!< code 273 */
    std::uint16_t bottomAttachment = 0;      /*!< code 272 */
    bool textExtended = false;         /*!< code 298, R2013+ */
};

//! Class to handle DBCOLOR (AcDbColor) — custom-class object §20.4.
/*!
 *  Named/true-color reference target. R2004+ entities with the ENC flag bit
 *  0x40 set carry a hard-pointer handle to one of these instead of an inline
 *  RGB value. Holds the resolved RGB plus the optional book/color names.
 *
 *  Layout (libreDWG dwg2.spec:2404-2408):
 *    - Common entity preamble
 *    - FIELD_CMC color  : §2.7 CMC = BS index, BL rgb (high byte = method),
 *                          RC method-byte, optional TV color name (bit 1),
 *                          optional TV book name (bit 2)
 *    - START_OBJECT_HANDLE_STREAM  (parent dictionary, reactors, xdic)
 */
class DRW_DbColor : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_DbColor() { reset(); }
    void reset(){
        tType = DRW::DBCOLOR;
        rgb = -1;
        colorIndex = 0;
        colorMethod = 0;
        name.clear();
        bookName.clear();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    int rgb = -1;            /*!< 24-bit RGB value, code 420 (or -1 if not RGB type) */
    std::uint16_t colorIndex = 0;  /*!< ACI fallback when method=0xC3, code 62 */
    std::uint8_t colorMethod = 0;  /*!< 0xC0 ByLayer / 0xC1 ByBlock / 0xC2 RGB / 0xC3 ACI */
    UTF8STRING bookName;     /*!< color book name, code 430 prefix */
    // name (inherited from DRW_TableEntry) is the entry name within the book
};

//! Class to handle SCALE (AcDbScale) — annotation-scale entry, custom-class object.
/*!
 *  Lives in the OBJECTS section under the named-object-dictionary
 *  ACAD_SCALELIST. Each entry maps a scale name like "1:48" to a
 *  paper-units / drawing-units ratio that consumers (MTEXT, MLEADER,
 *  DIMENSION, ATTRIB) reference via their AcDbAnnotScaleObjectContextData
 *  per-scale ctx data.  ODA spec §20.4.93; libreDWG dwg2.spec:1195-1203
 *  (DWG_OBJECT (SCALE)).
 *
 *  Body fields after the AcDbScale subclass marker:
 *    BS  flag           always 0
 *    TV  name           e.g., "1:48"
 *    BD  paperUnits     numerator (paper space)
 *    BD  drawingUnits   denominator (drawing/model space)
 *    B   isUnitScale    true when ratio is 1:1
 *
 *  Scale factor used at render time = drawingUnits / paperUnits
 *  (e.g., "1:48" → drawingUnits=48, paperUnits=1 → factor=48).
 */
class DRW_Scale : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 508;

    DRW_Scale() {
        tType = DRW::SCALE;
    }

    double scaleFactor() const {
        return paperUnits == 0.0 ? 1.0 : drawingUnits / paperUnits;
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr) const;
public:
    std::uint16_t flag = 0;            /*!< always 0, code 70 */
    double  paperUnits = 1.0;    /*!< numerator,  code 140 */
    double  drawingUnits = 1.0;  /*!< denominator, code 141 */
    bool    isUnitScale = false; /*!< true for the 1:1 entry, code 290 */
    // name (inherited from DRW_TableEntry) carries the user-visible label, code 300
};

struct DRW_DimensionAssociationOsnapRef {
    UTF8STRING m_className;
    std::uint8_t m_objectOsnapType = 0;
    std::uint32_t m_objectHandle = 0;
};

//! Shell parser for DIMASSOC (AcDbDimAssoc) associative dimension metadata.
class DRW_DimensionAssociation : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_DimensionAssociation() {
        tType = DRW::DIMASSOC;
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    std::uint32_t m_dimensionHandle = 0;
    std::uint32_t m_associativityFlags = 0;
    bool m_isTransSpace = false;
    std::uint8_t m_rotatedDimensionType = 0;
    std::vector<DRW_DimensionAssociationOsnapRef> m_osnapRefs;
};

struct DRW_EvaluationGraphNode {
    std::int32_t m_index = 0;
    std::int32_t m_flags = 0;
    std::int32_t m_nextNodeIndex = 0;
    std::uint32_t m_expressionHandle = 0;
    std::int32_t m_data1 = 0;
    std::int32_t m_data2 = 0;
    std::int32_t m_data3 = 0;
    std::int32_t m_data4 = 0;
};

struct DRW_EvaluationGraphEdge {
    std::int32_t m_value92 = 0;
    std::int32_t m_value93 = 0;
    std::int32_t m_value94 = 0;
    std::int32_t m_value91a = 0;
    std::int32_t m_value91b = 0;
    std::int32_t m_value92a = 0;
    std::int32_t m_value92b = 0;
    std::int32_t m_value92c = 0;
    std::int32_t m_value92d = 0;
    std::int32_t m_value92e = 0;
};

//! Shell parser for ACAD_EVALUATION_GRAPH dynamic/associative metadata.
class DRW_EvaluationGraph : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_EvaluationGraph() {
        tType = DRW::EVALUATIONGRAPH;
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    std::int32_t m_value96 = 0;
    std::int32_t m_value97 = 0;
    std::vector<DRW_EvaluationGraphNode> m_nodes;
    std::vector<DRW_EvaluationGraphEdge> m_edges;
};

//! Minimal ACDBPLACEHOLDER carrier (ODA fixed type 80).
class DRW_AcDbPlaceholder : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_AcDbPlaceholder() { tType = DRW::ACDBPLACEHOLDER; }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *handleBuf = nullptr) const;
};

//! SUN object metadata used by R2007+ view/vport lighting.
class DRW_Sun : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 517;

    DRW_Sun() { tType = DRW::SUN; }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *handleBuf = nullptr) const;

    std::uint32_t m_classVersion = 0;
    bool m_isOn = false;
    std::uint32_t m_color = 0;          /*!< ACI index, DXF code 63 */
    std::int32_t m_color24 = -1;        /*!< 24-bit true color, DXF code 421; -1 = unset */
    double m_intensity = 0.0;
    bool m_hasShadow = false;
    std::int32_t m_julianDay = 0;
    std::int32_t m_milliseconds = 0;
    bool m_isDaylightSavings = false;
    std::uint32_t m_shadowType = 0;
    std::uint16_t m_shadowMapSize = 0;
    std::uint8_t m_shadowSoftness = 0;
};

struct DRW_AssociativeHandleRef {
    bool m_isOwned = false;
    std::uint32_t m_handle = 0;
};

struct DRW_AssociativePrefixStatus {
    enum class Kind {
        AcDbAssocAction,
        AcDbAssocActionParam,
        AcDbAssocDependency,
        AcDbAssocGeomDependency,
        AcDbAssocNetwork,
        AcDbAssocActionBody,
        AcDbEvalExpr,
        AcDbShHistoryNode,
        AcShActionBody
    };

    enum class ParseStatus {
        Complete,
        Partial,
        Missing,
        UnsupportedVersion,
        BoundedCountOverflow
    };

    Kind m_kind = Kind::AcDbAssocAction;
    std::uint64_t m_startBit = 0;
    std::uint64_t m_bitSize = 0;
    std::uint16_t m_classVersion = 0;
    ParseStatus m_status = ParseStatus::Missing;
    size_t m_decodedHandleCount = 0;
    size_t m_decodedValueCount = 0;
    std::int32_t m_decodedCountValue = 0;
    UTF8STRING m_sourceAssumption;
};

//! Shell parser for ACDBASSOC* action/dependency/action-param objects.
class DRW_AssociativeObject : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    explicit DRW_AssociativeObject(const UTF8STRING& recordName = UTF8STRING())
        : m_recordName(recordName) {
        tType = DRW::ASSOCIATIVEOBJECT;
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    UTF8STRING m_recordName;
    std::uint16_t m_classVersion = 0;
    std::int32_t m_geometryStatus = 0;
    std::uint32_t m_owningNetworkHandle = 0;
    std::uint32_t m_actionBodyHandle = 0;
    std::int32_t m_actionIndex = 0;
    std::int32_t m_maxDependencyIndex = 0;
    std::vector<DRW_AssociativeHandleRef> m_dependencies;
    std::vector<DRW_AssociativeHandleRef> m_actions;
    std::vector<std::uint32_t> m_ownedParams;
    std::vector<std::uint32_t> m_ownedActions;
    size_t m_valueParamCount = 0;
    size_t m_ownedParamPrefixCount = 0;
    bool m_valueParamsParsed = false;
    bool m_actionParamPrefixParsed = false;
    bool m_singleDependencyActionParamParsed = false;
    bool m_compoundActionParamParsed = false;
    std::uint32_t m_dependencyHandle = 0;
    std::uint32_t m_readDependencyHandle = 0;
    std::uint32_t m_writeDependencyHandle = 0;
    std::uint32_t m_rNodeHandle = 0;
    std::uint32_t m_dNodeHandle = 0;
    std::int32_t m_status = 0;
    std::uint8_t m_osnapMode = 0;
    double m_parameter = 0.0;
    DRW_Coord m_point;
    std::vector<DRW_AssociativePrefixStatus> m_prefixStatuses;
};

//! Shell parser for ACSH_* history/action objects.
class DRW_AcShHistoryObject : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    explicit DRW_AcShHistoryObject(const UTF8STRING& recordName = UTF8STRING())
        : m_recordName(recordName) {
        tType = DRW::ACSHHISTORYOBJECT;
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;

public:
    UTF8STRING m_recordName;
    std::uint32_t m_major = 0;
    std::uint32_t m_minor = 0;
    std::uint32_t m_ownerHandle = 0;
    std::uint32_t m_historyNodeId = 0;
    bool m_showHistory = false;
    bool m_recordHistory = false;
    DRW_Coord m_direction;
    double m_draftAngle = 0.0;
    double m_startDraftDistance = 0.0;
    double m_endDraftDistance = 0.0;
    double m_scaleFactor = 1.0;
    double m_twistAngle = 0.0;
    double m_alignAngle = 0.0;
    std::vector<std::uint8_t> m_binaryBlob1;
    std::vector<std::uint8_t> m_binaryBlob2;
    std::vector<DRW_AssociativePrefixStatus> m_prefixStatuses;
};

//! Common AcDbModelDocViewStyle header shared by detail/section view styles.
struct DRW_ModelDocViewStyle {
    std::uint16_t m_modelDocClassVersion = 0;   /*!< DXF 70, AcDbModelDocViewStyle */
    UTF8STRING m_description;             /*!< DXF 3 */
    bool m_modifiedForRecompute = false;  /*!< DXF 290 */
    UTF8STRING m_displayName;             /*!< DXF 300, R2018+ */
    std::uint32_t m_viewStyleFlags = 0;         /*!< DXF 90, R2018+ */
};

//! Class to handle DETAILVIEWSTYLE / AcDbDetailViewStyle objects.
class DRW_DetailViewStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_DetailViewStyle() { reset(); }
    void reset();
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    DRW_ModelDocViewStyle m_modelDoc;
    std::uint16_t m_classVersion = 0;
    std::uint32_t m_flags = 0;
    std::uint32_t m_identifierStyleHandle = 0;
    int m_identifierColor = 256;
    double m_identifierHeight = 0.0;
    UTF8STRING m_identifierExcludeCharacters;
    double m_identifierOffset = 0.0;
    std::uint8_t m_identifierPlacement = 0;
    std::uint32_t m_arrowSymbolHandle = 0;
    int m_arrowSymbolColor = 256;
    double m_arrowSymbolSize = 0.0;
    std::uint32_t m_boundaryLineTypeHandle = 0;
    std::int32_t m_boundaryLineWeight = 0;
    int m_boundaryLineColor = 256;
    std::uint32_t m_viewLabelTextStyleHandle = 0;
    int m_viewLabelTextColor = 256;
    double m_viewLabelTextHeight = 0.0;
    std::uint32_t m_viewLabelAttachment = 0;
    double m_viewLabelOffset = 0.0;
    std::uint32_t m_viewLabelAlignment = 0;
    UTF8STRING m_viewLabelPattern;
    std::uint32_t m_connectionLineTypeHandle = 0;
    std::int32_t m_connectionLineWeight = 0;
    int m_connectionLineColor = 256;
    std::uint32_t m_borderLineTypeHandle = 0;
    std::int32_t m_borderLineWeight = 0;
    int m_borderLineColor = 256;
    std::uint8_t m_modelEdge = 0;
private:
    UTF8STRING m_dxfSubclass;
    int m_dxfGroup = -1;
    int m_dxfHandleCount = 0;
    int m_dxfColorCount = 0;
    int m_dxfDoubleCount = 0;
    int m_dxfLongCount = 0;
};

//! Class to handle SECTIONVIEWSTYLE / AcDbSectionViewStyle objects.
class DRW_SectionViewStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_SectionViewStyle() { reset(); }
    void reset();
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    DRW_ModelDocViewStyle m_modelDoc;
    std::uint16_t m_classVersion = 0;
    std::uint32_t m_flags = 0;
    std::uint32_t m_identifierStyleHandle = 0;
    int m_identifierColor = 256;
    double m_identifierHeight = 0.0;
    std::uint32_t m_arrowStartSymbolHandle = 0;
    std::uint32_t m_arrowEndSymbolHandle = 0;
    int m_arrowSymbolColor = 256;
    double m_arrowSymbolSize = 0.0;
    UTF8STRING m_identifierExcludeCharacters;
    std::int32_t m_identifierPosition = 0;
    double m_identifierOffset = 0.0;
    std::int32_t m_arrowPosition = 0;
    double m_arrowSymbolExtensionLength = 0.0;
    std::uint32_t m_planeLineTypeHandle = 0;
    std::int32_t m_planeLineWeight = 0;
    int m_planeLineColor = 256;
    std::uint32_t m_bendLineTypeHandle = 0;
    std::int32_t m_bendLineWeight = 0;
    int m_bendLineColor = 256;
    double m_bendLineLength = 0.0;
    double m_endLineOvershoot = 0.0;
    double m_endLineLength = 0.0;
    std::uint32_t m_viewLabelTextStyleHandle = 0;
    int m_viewLabelTextColor = 256;
    double m_viewLabelTextHeight = 0.0;
    std::uint32_t m_viewLabelAttachment = 0;
    double m_viewLabelOffset = 0.0;
    std::uint32_t m_viewLabelAlignment = 0;
    UTF8STRING m_viewLabelPattern;
    int m_hatchColor = 256;
    int m_hatchBackgroundColor = 257;
    UTF8STRING m_hatchPattern;
    double m_hatchScale = 1.0;
    std::int32_t m_hatchTransparency = 0;
    bool m_unknownB1 = false;
    bool m_unknownB2 = false;
    std::vector<double> m_hatchAngles;
private:
    UTF8STRING m_dxfSubclass;
    int m_dxfGroup = -1;
    int m_dxfHandleCount = 0;
    int m_dxfColorCount = 0;
    int m_dxfDoubleCount = 0;
    int m_dxfLongCount = 0;
    int m_dxfBoolCount = 0;
    std::uint32_t m_dxfExpectedHatchAngles = 0;
};

//! Class to handle BREAKDATA / AcDbBreakData objects.
class DRW_BreakData : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_BreakData() { reset(); }
    void reset();
protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    std::vector<std::uint32_t> m_pointRefHandles;
    std::uint32_t m_dimensionHandle = 0;
private:
    bool m_dxfInBreakData = false;
};

//! Placeholder carrier for BREAKPOINTREF / AcDbBreakPointRef objects.
class DRW_BreakPointRef : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_BreakPointRef() { reset(); }
    void reset() { tType = DRW::BREAKPOINTREF; DRW_TableEntry::reset(); }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
};

//! Class to handle VISUALSTYLE (AcDbVisualStyle) — custom-class object §20.4.95.
/*!
 *  Stub: full ODA spec lists 60+ fields for visual styles, but LibreCAD is 2D
 *  and never consumes them. We only need round-trip identity at the OBJECTS-
 *  section dispatch boundary so the file's class table doesn't drop a phantom
 *  entry. Each object is parsed from a size-bounded buffer so misalignment
 *  within parseDwg can't propagate to neighbors.
 */
class DRW_VisualStyle : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    DRW_VisualStyle() { reset(); }
    void reset() { tType = DRW::VISUALSTYLE; desc.clear(); type = 0; }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    UTF8STRING desc;       /*!< description (TV in DWG) */
    std::uint16_t type = 0;      /*!< visual-style type code (BS in DWG) */
};

//! Class to handle UNDERLAYDEFINITION (AcDb*Definition) — custom-class object.
/*!
 *  Three flavors share one class: PDF, DGN, DWF (Pdf/Dgn/DwfDefinition).
 *  Lives in the OBJECTS section. Each carries a filename + sheet/layout name
 *  that the matching UNDERLAY entity references via its definitionHandle.
 */
class DRW_UnderlayDefinition : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    enum Kind { PDF, DGN, DWF };
    DRW_UnderlayDefinition() { reset(); }
    void reset() {
        tType = DRW::UNDERLAYDEFINITION;
        kind = PDF;
        filename.clear();
        sheetName.clear();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
public:
    Kind kind = PDF;
    UTF8STRING filename;
    UTF8STRING sheetName;
};

//! IDBUFFER (AcDbIdBuffer) — ODA §20.4.79.
/*!
 *  Holds a list of object handles. Used as an opaque selection-set carrier
 *  by xref-clip, filter chains, etc.
 */
class DRW_IDBuffer : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 509;

    DRW_IDBuffer() { reset(); }
    void reset() {
        tType = DRW::IDBUFFER;
        classVersion = 0;
        objIds.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    int classVersion = 0;                /*!< class_version RC, always 0 */
    std::vector<std::uint32_t> objIds;         /*!< object handles (soft pointer) */
};

//! LAYER_INDEX (AcDbLayerIndex) — ODA §20.4.83.
/*!
 *  Layer index — for each layer name records the entry handle to the
 *  matching IDBUFFER (containing the entities on that layer).
 */
struct DRW_LayerIndexEntry {
    int indexLong = 0;
    UTF8STRING name;
    std::uint32_t entryHandle = 0;       /*!< populated from the post-body handle stream */
};

class DRW_LayerIndex : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 510;

    DRW_LayerIndex() { reset(); }
    void reset() {
        tType = DRW::LAYERINDEX;
        timestamp1 = 0;
        timestamp2 = 0;
        entries.clear();
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::uint32_t timestamp1 = 0;          /*!< Julian day BL */
    std::uint32_t timestamp2 = 0;          /*!< milliseconds BL */
    std::vector<DRW_LayerIndexEntry> entries;
};

//! SPATIAL_INDEX (AcDbSpatialIndex) — ODA §20.4.95.
/*!
 *  Spatial index for accelerated entity lookup. Body beyond timestamps is
 *  unspecified ("rest of bits to handles") in the ODA spec; we capture
 *  only the timestamps + common handles. The opaque blob between
 *  timestamps and handles is preserved by raw-byte replay on write.
 */
class DRW_SpatialIndex : public DRW_TableEntry {
    SETOBJFRIENDS
public:
    static constexpr std::uint16_t kDwgClassNum = 511;

    DRW_SpatialIndex() { reset(); }
    void reset() {
        tType = DRW::SPATIALINDEX;
        timestamp1 = 0;
        timestamp2 = 0;
        DRW_TableEntry::reset();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs=0) override;
    bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                   dwgBufferW *strBuf = nullptr,
                   dwgBufferW *handleBuf = nullptr) const;
public:
    std::uint32_t timestamp1 = 0;          /*!< Julian day BL */
    std::uint32_t timestamp2 = 0;          /*!< milliseconds BL */
};

/** Holds per-write-session maps populated during DXF/DWG writing. */
class DRW_WritingContext {
public:
    DRW_WritingContext() = default;
    std::vector<std::pair<std::string, int>> lineTypesMap; /*!< DXF: uppercase name -> handle */
    std::map<std::string, std::uint32_t> ltypeMap;    /*!< DWG: uppercase ltype name -> handle */
    std::map<std::string, std::uint32_t> layerMap;    /*!< DWG: uppercase layer name -> handle */
    std::map<std::string, std::uint32_t> styleMap;    /*!< DWG: uppercase style name -> handle */
    std::map<std::string, std::uint32_t> viewMap;     /*!< DWG: uppercase view name -> handle */
    std::map<std::string, std::uint32_t> vportMap;    /*!< DWG: uppercase vport name -> handle */
    std::map<std::string, std::uint32_t> appidMap;    /*!< DWG: uppercase appid name -> handle */
    std::map<std::string, std::uint32_t> dimstyleMap; /*!< DWG: uppercase dimstyle name -> handle */
    /*!< DXF write: source entity handle -> minted code-5 handle, captured in
     * dxfRW::writeEntity. Lets GROUP-emit (F3) resolve a member's SOURCE handle
     * to the handle actually written. emplace-only: a parent re-entering
     * writeEntity (writePolyline/writeInsert reuse ent->handle) keeps the
     * first-seen real source; stale minted-range keys are never queried. */
    std::map<std::uint32_t, std::uint32_t> sourceHandleToMintedMap;
};

namespace DRW {

// Extended color palette:
// The first entry is only for direct indexing starting with [1]
// Color 1 is red (1,0,0)
const unsigned char dxfColors[][3] = {
    {  0,  0,  0}, // unused
    {255,  0,  0}, // 1 red
    {255,255,  0}, // 2 yellow
    {  0,255,  0}, // 3 green
    {  0,255,255}, // 4 cyan
    {  0,  0,255}, // 5 blue
    {255,  0,255}, // 6 magenta
    {  0,  0,  0}, // 7 black or white
    {128,128,128}, // 8 50% gray
    {192,192,192}, // 9 75% gray
    {255,  0,  0}, // 10
    {255,127,127},
    {204,  0,  0},
    {204,102,102},
    {153,  0,  0},
    {153, 76, 76}, // 15
    {127,  0,  0},
    {127, 63, 63},
    { 76,  0,  0},
    { 76, 38, 38},
    {255, 63,  0}, // 20
    {255,159,127},
    {204, 51,  0},
    {204,127,102},
    {153, 38,  0},
    {153, 95, 76}, // 25
    {127, 31,  0},
    {127, 79, 63},
    { 76, 19,  0},
    { 76, 47, 38},
    {255,127,  0}, // 30
    {255,191,127},
    {204,102,  0},
    {204,153,102},
    {153, 76,  0},
    {153,114, 76}, // 35
    {127, 63,  0},
    {127, 95, 63},
    { 76, 38,  0},
    { 76, 57, 38},
    {255,191,  0}, // 40
    {255,223,127},
    {204,153,  0},
    {204,178,102},
    {153,114,  0},
    {153,133, 76}, // 45
    {127, 95,  0},
    {127,111, 63},
    { 76, 57,  0},
    { 76, 66, 38},
    {255,255,  0}, // 50
    {255,255,127},
    {204,204,  0},
    {204,204,102},
    {153,153,  0},
    {153,153, 76}, // 55
    {127,127,  0},
    {127,127, 63},
    { 76, 76,  0},
    { 76, 76, 38},
    {191,255,  0}, // 60
    {223,255,127},
    {153,204,  0},
    {178,204,102},
    {114,153,  0},
    {133,153, 76}, // 65
    { 95,127,  0},
    {111,127, 63},
    { 57, 76,  0},
    { 66, 76, 38},
    {127,255,  0}, // 70
    {191,255,127},
    {102,204,  0},
    {153,204,102},
    { 76,153,  0},
    {114,153, 76}, // 75
    { 63,127,  0},
    { 95,127, 63},
    { 38, 76,  0},
    { 57, 76, 38},
    { 63,255,  0}, // 80
    {159,255,127},
    { 51,204,  0},
    {127,204,102},
    { 38,153,  0},
    { 95,153, 76}, // 85
    { 31,127,  0},
    { 79,127, 63},
    { 19, 76,  0},
    { 47, 76, 38},
    {  0,255,  0}, // 90
    {127,255,127},
    {  0,204,  0},
    {102,204,102},
    {  0,153,  0},
    { 76,153, 76}, // 95
    {  0,127,  0},
    { 63,127, 63},
    {  0, 76,  0},
    { 38, 76, 38},
    {  0,255, 63}, // 100
    {127,255,159},
    {  0,204, 51},
    {102,204,127},
    {  0,153, 38},
    { 76,153, 95}, // 105
    {  0,127, 31},
    { 63,127, 79},
    {  0, 76, 19},
    { 38, 76, 47},
    {  0,255,127}, // 110
    {127,255,191},
    {  0,204,102},
    {102,204,153},
    {  0,153, 76},
    { 76,153,114}, // 115
    {  0,127, 63},
    { 63,127, 95},
    {  0, 76, 38},
    { 38, 76, 57},
    {  0,255,191}, // 120
    {127,255,223},
    {  0,204,153},
    {102,204,178},
    {  0,153,114},
    { 76,153,133}, // 125
    {  0,127, 95},
    { 63,127,111},
    {  0, 76, 57},
    { 38, 76, 66},
    {  0,255,255}, // 130
    {127,255,255},
    {  0,204,204},
    {102,204,204},
    {  0,153,153},
    { 76,153,153}, // 135
    {  0,127,127},
    { 63,127,127},
    {  0, 76, 76},
    { 38, 76, 76},
    {  0,191,255}, // 140
    {127,223,255},
    {  0,153,204},
    {102,178,204},
    {  0,114,153},
    { 76,133,153}, // 145
    {  0, 95,127},
    { 63,111,127},
    {  0, 57, 76},
    { 38, 66, 76},
    {  0,127,255}, // 150
    {127,191,255},
    {  0,102,204},
    {102,153,204},
    {  0, 76,153},
    { 76,114,153}, // 155
    {  0, 63,127},
    { 63, 95,127},
    {  0, 38, 76},
    { 38, 57, 76},
    {  0, 66,255}, // 160
    {127,159,255},
    {  0, 51,204},
    {102,127,204},
    {  0, 38,153},
    { 76, 95,153}, // 165
    {  0, 31,127},
    { 63, 79,127},
    {  0, 19, 76},
    { 38, 47, 76},
    {  0,  0,255}, // 170
    {127,127,255},
    {  0,  0,204},
    {102,102,204},
    {  0,  0,153},
    { 76, 76,153}, // 175
    {  0,  0,127},
    { 63, 63,127},
    {  0,  0, 76},
    { 38, 38, 76},
    { 63,  0,255}, // 180
    {159,127,255},
    { 50,  0,204},
    {127,102,204},
    { 38,  0,153},
    { 95, 76,153}, // 185
    { 31,  0,127},
    { 79, 63,127},
    { 19,  0, 76},
    { 47, 38, 76},
    {127,  0,255}, // 190
    {191,127,255},
    {102,  0,204},
    {153,102,204},
    { 76,  0,153},
    {114, 76,153}, // 195
    { 63,  0,127},
    { 95, 63,127},
    { 38,  0, 76},
    { 57, 38, 76},
    {191,  0,255}, // 200
    {223,127,255},
    {153,  0,204},
    {178,102,204},
    {114,  0,153},
    {133, 76,153}, // 205
    { 95,  0,127},
    {111, 63,127},
    { 57,  0, 76},
    { 66, 38, 76},
    {255,  0,255}, // 210
    {255,127,255},
    {204,  0,204},
    {204,102,204},
    {153,  0,153},
    {153, 76,153}, // 215
    {127,  0,127},
    {127, 63,127},
    { 76,  0, 76},
    { 76, 38, 76},
    {255,  0,191}, // 220
    {255,127,223},
    {204,  0,153},
    {204,102,178},
    {153,  0,114},
    {153, 76,133}, // 225
    {127,  0, 95},
    {127, 63, 11},
    { 76,  0, 57},
    { 76, 38, 66},
    {255,  0,127}, // 230
    {255,127,191},
    {204,  0,102},
    {204,102,153},
    {153,  0, 76},
    {153, 76,114}, // 235
    {127,  0, 63},
    {127, 63, 95},
    { 76,  0, 38},
    { 76, 38, 57},
    {255,  0, 63}, // 240
    {255,127,159},
    {204,  0, 51},
    {204,102,127},
    {153,  0, 38},
    {153, 76, 95}, // 245
    {127,  0, 31},
    {127, 63, 79},
    { 76,  0, 19},
    { 76, 38, 47},
    { 51, 51, 51}, // 250
    { 91, 91, 91},
    {132,132,132},
    {173,173,173},
    {214,214,214},
    {255,255,255}  // 255
};

}

#endif

// EOF
