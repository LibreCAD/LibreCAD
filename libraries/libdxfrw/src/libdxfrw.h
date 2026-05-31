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

#include <memory>
#include <string>
#include <unordered_map>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_header.h"
#include "drw_interface.h"


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
        auto it = lineTypeNameMap.find(static_cast<duint32>(handle));
        return (it != lineTypeNameMap.end()) ? it->second : std::string();
    }
    /** Returns block-record name for a given DXF handle, or empty string if not found. */
    std::string resolveBlockRecordName(duint32 handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.name : std::string();
    }
    /** Returns block-record insertion units for a given DXF handle, or 0 if unknown. */
    int resolveBlockRecordInsUnits(duint32 handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.insUnits : 0;
    }
    std::unordered_map<duint32, std::string> lineTypeNameMap;
    std::unordered_map<duint32, BlockRecordInfo> blockRecordMap;
};

class dxfRW {
public:
    dxfRW(const char* name);
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
    bool processTables();
    bool processBlocks();
    bool processBlock();
    bool processEntities(bool isblock);
    bool processObjects();
    bool processDetailViewStyle();
    bool processSectionViewStyle();
    bool processBreakData();
    bool processBreakPointRef();

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
    bool processSpline();
    bool process3dface();
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

//    bool writeHeader();
    bool writeEntity(DRW_Entity *ent);
    bool writeArcDimension(DRW_DimArc *d);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();
    bool writeExtData(const std::vector<DRW_Variant*> &ed);
    /* Entity-flavoured overload: entities own extData via shared_ptr, table
     * records own raw pointers. Same DXF codes, different storage. */
    bool writeExtData(const std::vector<std::shared_ptr<DRW_Variant>> &ed);
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
    int entCount;
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
