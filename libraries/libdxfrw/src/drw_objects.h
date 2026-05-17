/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD.org                                           **
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
         MLEADERSTYLE,
         DBCOLOR,
         VISUALSTYLE,
         UNDERLAYDEFINITION,
         SCALE
     };

//pending VP_ENT_HDR, GROUP, LONG_TRANSACTION, XRECORD,
//ACDBPLACEHOLDER, VBA_PROJECT, ACAD_TABLE, CELLSTYLEMAP, DICTIONARYVAR,
//DICTIONARYWDFLT, FIELD, IDBUFFER, IMAGEDEF, IMAGEDEFREACTOR, LAYER_INDEX,
//MATERIAL, PLACEHOLDER, PLOTSETTINGS, RASTERVARIABLES, SORTENTSTABLE,
//SPATIAL_INDEX, SPATIAL_FILTER, TABLEGEOMETRY, TABLESTYLES,
}

class dwgBufferW;

#define SETOBJFRIENDS  friend class dxfRW; \
                       friend class dwgReader; \
                       friend class dwgWriter15;

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
class DRW_TableEntry {
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
        xDictFlag {e.xDictFlag},
        numReactors {e.numReactors},
        curr {nullptr}
    {
        for (std::vector<DRW_Variant *>::const_iterator it = e.extData.begin(); it != e.extData.end(); ++it) {
            DRW_Variant *src = *it;
            DRW_Variant *dst = new DRW_Variant( *src);
            extData.push_back( dst);
            if (src == e.curr) {
                curr = dst;
            }
        }
    }

protected:
    virtual bool parseCode(int code, const std::unique_ptr<dxfReader>& reader);
    virtual bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) = 0;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer* strBuf, duint32 bs=0);
    void reset() {
        flags = 0;
        for (std::vector<DRW_Variant*>::iterator it = extData.begin(); it != extData.end(); ++it) {
            delete *it;
        }
        extData.clear();
        curr = nullptr;
    }

public:
    enum DRW::TTYPE tType {DRW::UNKNOWNT};  /*!< enum: entity type, code 0 */
    duint32         handle {0};             /*!< entity identifier, code 5 */
    int             parentHandle {0};       /*!< Soft-pointer ID/handle to owner object, code 330 */
    UTF8STRING      name;                   /*!< entry name, code 2 */
    int             flags {0};              /*!< Flags relevant to entry, code 70 */
    std::vector<DRW_Variant*> extData;      /*!< FIFO list of extended data, codes 1000 to 1071*/

    //***** dwg parse ********/
protected:
    dint16  oType {0};
    duint8  xDictFlag {0};
    dint32  numReactors {0};
    duint32 objSize {0};    //RL 32bits object data size in bits

private:
    DRW_Variant* curr {nullptr};
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
        for (auto& kv : vars) delete kv.second;
    }

    void add(const std::string& key, int code, int value) { vars[key] = new DRW_Variant(code, value); }
    void add(const std::string& key, int code, double value) { vars[key] = new DRW_Variant(code, value); }
    void add(const std::string& key, int code, std::string value) { vars[key] = new DRW_Variant(code, UTF8STRING(value)); }
    DRW_Variant* get(const std::string& key) const {
        auto it = vars.find(key);
        return (it != vars.end()) ? it->second : nullptr;
    }

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
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
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
    std::map<std::string, DRW_Variant*> vars; /*!< extra/override variables written after standard fields */
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
    void update();

public:
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
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
        firstEH = lastEH = DRW::NoHandle;
        DRW_TableEntry::reset();
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
//Note:    int DRW_TableEntry::flags; contains code 70 of block
    int insUnits;             /*!< block insertion units, code 70 of block_record*/
    DRW_Coord basePoint;      /*!<  block insertion base point dwg only */
    UTF8STRING xrefPath;      /*!< Xref path name for XREF block_records (DWG: parsed from BLOCK_HEADER, DXF: code 1) */
protected:
    //dwg parser
private:
    duint32 block;   //handle for block entity
    duint32 endBlock;//handle for end block entity
    duint32 firstEH; //handle of first entity, only in pre-2004
    duint32 lastEH;  //handle of last entity, only in pre-2004
    std::vector<duint32>entMap;
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
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
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
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
    duint32 visualStyleHandle = 0; /*!< R2007+ visual-style ref (DWG-only) */
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

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
        marginLeft = 0.0;
        marginBottom = 0.0;
        marginRight = 0.0;
        marginTop = 0.0;
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
    UTF8STRING plotViewName;/*!< Plot view name, code 6 */
    double marginLeft;      /*!< Size, in millimeters, of unprintable margin on left side of paper, code 40 */
    double marginBottom;    /*!< Size, in millimeters, of unprintable margin on bottom side of paper, code 41 */
    double marginRight;     /*!< Size, in millimeters, of unprintable margin on right side of paper, code 42 */
    double marginTop;       /*!< Size, in millimeters, of unprintable margin on top side of paper, code 43 */
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
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

public:
    DRW_Coord origin;           /*!< UCS origin, codes 10/20/30 */
    DRW_Coord xAxisDirection;   /*!< UCS X-axis direction, codes 11/21/31 */
    DRW_Coord yAxisDirection;   /*!< UCS Y-axis direction, codes 12/22/32 */
    DRW_Coord orthoOrigin;      /*!< Origin for orthographic UCS, codes 13/23/33 */
    double elevation;           /*!< Elevation, code 146 */
    int orthoType;              /*!< Orthographic type, code 71 (0 none, 1 Top, ...) */
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
        DRW_TableEntry::reset();
    }

protected:
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) override;
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;

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
    duint32 namedUCS_ID;                /*!< Handle of named UCS table record, code 345 */
    duint32 baseUCS_ID;                 /*!< Handle of base UCS table record, code 346 */
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
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
};

//! Class to handle Dictionary (ODA spec sec 19.4.30 fixed type 42)
/*!
*  Minimal carrier for named-object dictionaries; full entry-list parsing
*  pending sample-validated implementation.
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
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    int cloning;     /*!< duplicate-record handling (BS) */
    int hardOwner;   /*!< hard-owner flag (RC, R2007+) */
    //future: std::vector<std::pair<UTF8STRING, duint32>> entries; per ODA 19.4.30
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
        layoutFlags = 0;
        tabOrder = 0;
        name.clear();
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    int layoutFlags;   /*!< layout flags (BS) */
    int tabOrder;      /*!< tab order (BL) */
    //future: minLim/maxLim, insertionBase, ucsOrigin, etc. per ODA 19.4.85
};

//! One parallel line element within a MLineStyle (ODA §19.4.73).
struct DRW_MLineElement {
    double offset = 0.0;        /*!< BD — perpendicular offset from centerline */
    int    color  = 256;        /*!< CMC index — 256 = ByLayer */
    int    color24 = -1;        /*!< true-color RGB (-1 = none) */
    duint32 linetypeHandle = 0; /*!< H — linetype object reference */
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
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
    DRW_MLeaderStyle() {
        tType = DRW::MLEADERSTYLE;
    }
protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    /* Per §20.4.87.  Names follow the spec column for traceability. */
    duint16 styleVersion = 2;          /*!< code 179, R2010+ */
    duint16 contentType = 2;           /*!< code 170: 0=None,1=Block,2=MText,3=Tolerance */
    duint16 drawMLeaderOrder = 0;      /*!< code 171: 0=content first,1=leader head first */
    duint16 drawLeaderOrder = 0;       /*!< code 172 */
    dint32 maxLeaderPoints = 0;        /*!< code 90 */
    double firstSegmentAngle = 0.0;    /*!< code 40 (radians) */
    double secondSegmentAngle = 0.0;   /*!< code 41 (radians) */
    duint16 leaderType = 1;            /*!< code 173 */
    int leaderColor = 0;               /*!< code 91 (CMC) */
    dwgHandle leaderLineTypeHandle{};  /*!< code 340 (handle stream) */
    dint32 leaderLineWeight = 0;       /*!< code 92 */
    bool landingEnabled = true;        /*!< code 290 */
    double landingGap = 0.0;           /*!< code 42 */
    bool autoIncludeLanding = true;    /*!< code 291 */
    double landingDistance = 0.0;      /*!< code 43 */
    UTF8STRING description;            /*!< code 3 */
    dwgHandle arrowHeadBlockHandle{};  /*!< code 341 */
    double arrowHeadSize = 0.0;        /*!< code 44 */
    UTF8STRING textDefault;            /*!< code 300 */
    dwgHandle textStyleHandle{};       /*!< code 342 */
    duint16 leftAttachment = 0;        /*!< code 174 */
    duint16 rightAttachment = 0;       /*!< code 178 */
    duint16 textAngleType = 0;         /*!< code 175 (R2010+) */
    duint16 textAlignmentType = 0;     /*!< code 176 */
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
    duint16 blockConnectionType = 0;   /*!< code 177 */
    double scaleFactor = 1.0;          /*!< code 142 */
    bool propertyChanged = false;      /*!< code 295 */
    bool isAnnotative = false;         /*!< code 296 */
    double breakSize = 0.0;            /*!< code 143 */
    /* R2010+ */
    duint16 attachmentDirection = 0;   /*!< code 271 */
    duint16 topAttachment = 0;         /*!< code 273 */
    duint16 bottomAttachment = 0;      /*!< code 272 */
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    int rgb = -1;            /*!< 24-bit RGB value, code 420 (or -1 if not RGB type) */
    duint16 colorIndex = 0;  /*!< ACI fallback when method=0xC3, code 62 */
    duint8 colorMethod = 0;  /*!< 0xC0 ByLayer / 0xC1 ByBlock / 0xC2 RGB / 0xC3 ACI */
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
    DRW_Scale() {
        tType = DRW::SCALE;
    }

    double scaleFactor() const {
        return paperUnits == 0.0 ? 1.0 : drawingUnits / paperUnits;
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    duint16 flag = 0;            /*!< always 0, code 70 */
    double  paperUnits = 1.0;    /*!< numerator,  code 140 */
    double  drawingUnits = 1.0;  /*!< denominator, code 141 */
    bool    isUnitScale = false; /*!< true for the 1:1 entry, code 290 */
    // name (inherited from DRW_TableEntry) carries the user-visible label, code 300
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    UTF8STRING desc;       /*!< description (TV in DWG) */
    duint16 type = 0;      /*!< visual-style type code (BS in DWG) */
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
    bool parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs=0) override;
public:
    Kind kind = PDF;
    UTF8STRING filename;
    UTF8STRING sheetName;
};

/** Holds per-write-session maps populated during DXF writing. */
class DRW_WritingContext {
public:
    DRW_WritingContext() = default;
    std::vector<std::pair<std::string, int>> lineTypesMap; /*!< uppercase name -> handle */
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

