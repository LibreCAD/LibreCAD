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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_header.h"
#include "drw_interface.h"


class dxfReader;
class dxfWriter;

using DRW_TableEntryFunc = std::function<void(DRW_TableEntry*)>;
using DRW_EntityFunc = std::function<void(DRW_Entity*)>;
using DRW_ParseableFunc = std::function<void(DRW_ParseableEntity*)>;

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
    void writeHeader();
    bool writeLineType(DRW_LType *ent);
    bool writeLineTypeGenerics(DRW_LType* ent, int handle);
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
    bool writeBlockRecord(std::string name);
    bool writeBlock(DRW_Block *ent);
    bool writeInsert(DRW_Insert *ent);
    bool writeMText(DRW_MText *ent);
    bool writeMLine(DRW_MLine *ent);
    bool writeUnderlay(DRW_Underlay *ent);
    bool writeText(DRW_Text *ent);
    bool writeHatch(DRW_Hatch *ent);
    bool writeViewport(DRW_Viewport *ent);
    DRW_ImageDef *writeImage(DRW_Image *ent, std::string name);
    bool writeWipeout(DRW_Image *ent);
    bool writeMultiLeader(DRW_MLeader *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeDimension(DRW_Dimension *ent);
    bool writeEntityExtData(DRW_Entity* ent);
    void writeViewPortTable();
    void writeLayerTable();
    void writeLineTypeTable();
    void writeStyleTable();
    void writeUCSTable();
    void writeViewTable();
    void writeAppIdTable();
    void writeBlockRecordTable();
    void writeDimStyleTable();
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
    bool processBlockRecord();
    bool processTables();
    bool processBlocks();
    bool processBlock();
    bool processEntities(bool isblock);
    bool doProcessEntity(DRW_Entity& ent, DRW_EntityFunc applyFunc);
    bool doProcessParseable(DRW_ParseableEntity& ent, DRW_ParseableFunc applyFunc, DRW::error sectionError = DRW::BAD_READ_ENTITIES);
    bool processObjects();

    bool processLType();
    bool processLayer();
    bool doProcessTableEntry(const std::string &sectionName, DRW_TableEntry& entry,
                         DRW_TableEntryFunc applyFunc, bool reuseEntity = true);
    bool processDimStyle(std::vector<DRW_Dimstyle> &styles);
    bool processTextStyle();
    bool processVports();
    bool processAppId();
    bool processView();
    bool processUCS();

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
    bool processLWPolyline();
    bool processPolyline();
    bool processVertex(DRW_Polyline* pl);
    bool processTolerance();
    bool processText();
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
    bool processArcDimension();
    bool processDimension();
    bool processLeader();
    bool processPlotSettings();

//    bool writeHeader();
    bool writeEntity(DRW_Entity *ent);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();
    bool writeExtData(const std::vector<DRW_Variant*> &ed);
    /* Entity-flavoured overload: entities own extData via shared_ptr, table
     * records own raw pointers. Same DXF codes, different storage. */
    bool writeExtData(const std::vector<std::shared_ptr<DRW_Variant>> &ed);
    /*use version from dwgutil.h*/
    std::string toHexStr(int n) const;//RLZ removeme
    bool writeAppData(const std::list<std::list<DRW_Variant>> &appData);

    bool setError(DRW::error lastError);

    inline bool writeString(int code, const std::string &text) const;
    inline bool writeDouble(int code, double d) const;
    inline bool writeDoubleOpt(int code, double d) const;
    inline bool writeUtf8String(int code, const std::string &text) const;
    inline bool writeUtf8Caps(int code, const std::string& text) const;
    inline bool writeHandle(int code, int handle) const;
    inline bool writeInt16(int code, int val) const;
    inline bool writeInt32(int code, int val) const;
    inline bool writeBool(int code, bool val) const;
    inline bool readRec(int *codeData) const;

    inline std::string getString() const;
    inline void writeSectionStart(const std::string& name);
    inline void writeSectionEnd();
    inline void writeSymTypeRecord(const std::string& typeName);
    inline void writeSubClass(const std::string& typeName);
    inline void writeSubClassOpt(const std::string& typeName);
    inline void writeTableName(const std::string& name);
    inline void writeDXFName(const std::string& name);
    void writeName(const std::string& name);
    inline void writeTableEnd();
    inline void writeSymTable();
    inline void writeCoord(int startCode, const DRW_Coord& coord);
    void writeTableStart(const std::string& name, std::string handle, int maxEntriesNumber, int handleCode=5);
    void writeVar(const std::string &name, int defaultValue, int varCode  = 70);
    void writeVarExp(const std::string& name, int value, int varCode);
    void writeVarOpt(const std::string& name, int varCode);
    void writeVar(const std::string &name, double defaultValue, int varCode = 40);
    void writeVar(const std::string &name, const std::string &defaultValue="", int varCode = 1);
    void writeVar(const std::string& name, int startCode, const DRW_Coord& defaultCoord);
    void writeVar2D(const std::string& name, int startCode, const DRW_Coord& defaultCoord);
    void writeVar2DOpt(const std::string& name, int startCode);
    bool writeDouble(int code, DRW_Dimstyle* ent, const std::string& name);
    bool writeInt16(int code, DRW_Dimstyle* ent, const std::string& name);
    bool writeUtf8String(int code, DRW_Dimstyle* ent, const std::string& name);

    void setVersion(DRW::Version v);

    DRW::Version version;
    bool afterAC1009 {false};
    bool afterAC1012 {false};
    bool afterAC1014 {false};
    bool afterAC1015 {false};
    bool afterAC1018 {false};
    DRW::error error {DRW::BAD_NONE};
    std::string fileName;
    std::string codePage;
    bool binFile;
    std::unique_ptr<dxfReader> reader;
    std::unique_ptr<dxfWriter> writer;
    DRW_Interface *iface = nullptr;
    DRW_Header header;
//    int section;
    std::string nextentity;
    int entCount = 0;
    bool wlayer0 = false;
    bool dimstyleStd = false;
    bool applyExt = false;
    bool writingBlock;
    int elParts;  /*!< parts number when convert ellipse to polyline */
    std::unordered_map<std::string,int> blockMap;
    std::unordered_map<std::string,int> textStyleMap;
    std::vector<DRW_ImageDef*> imageDef;  /*!< imageDef list */

    int currHandle;

    DRW_ParsingContext m_readingContext;
    DRW_WritingContext m_writingContext;
};


#endif
