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


#include "libdxfrw.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <functional>

#include "intern/drw_textcodec.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/drw_dbg.h"
#include "intern/dwgutil.h"

#define FIRSTHANDLE 48


dxfRW::dxfRW(const char* name){
    DRW_DBGSL(DRW_dbg::Level::None);
    fileName = name;
    // fixme - initialize
    // reader = nullptr;
    // writer = nullptr;
    applyExt = false;
    elParts = 128; //parts number when convert ellipse to polyline
}
dxfRW::~dxfRW(){
    for (auto it=imageDef.begin(); it!=imageDef.end(); ++it)
        delete *it;
    }
    imageDef.clear();
}

void dxfRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

#define ERR_UNKNOWN return setError(DRW::BAD_UNKNOWN);
#define ERR_TABLES return setError(DRW::BAD_READ_TABLES);
#define ERR_BAD_CODE  return setError( DRW::BAD_CODE_PARSED);


bool dxfRW::readAscii(DRW_Interface *interface_, bool ext, std::string& content) {
    if (nullptr == interface_) {
        ERR_UNKNOWN;
    }
    applyExt = ext;
    std::istringstream filestr(content);
    iface = interface_;

    reader = new dxfReaderAscii(&filestr);
    bool isOk {processDxf()};
    setVersion((DRW::Version) reader->getVersion());
    reader.reset();
    return isOk;
}

bool dxfRW::read(DRW_Interface *interface_, bool ext){
    drw_assert(fileName.empty() == false);
    applyExt = ext;
    std::ifstream filestr;
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    DRW_DBG("dxfRW::read 1def\n");
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open()
        || !filestr.good()) {
        return setError(DRW::BAD_OPEN);
    }

    char line[22];
    char line2[22] = "AutoCAD Binary DXF\r\n";
    line2[20] = (char)26;
    line2[21] = '\0';
    filestr.read (line, 22);
    filestr.close();
    iface = interface_;
    DRW_DBG("dxfRW::read 2\n");
    // `line` is filled by an unterminated 22-byte read; compare by exact
    // length to avoid strcmp reading past the buffer when the sentinel
    // bytes don't include an embedded NUL.
    if (std::memcmp(line, line2, sizeof(line)) == 0) {
        filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
        binFile = true;
        //skip sentinel
        filestr.seekg (22, std::ios::beg);
        reader = std::make_unique<dxfReaderBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        binFile = false;
        filestr.open (fileName.c_str(), std::ios_base::in);
        reader = std::make_unique<dxfReaderAscii>(&filestr);
    }

    bool isOk {processDxf()};
    filestr.close();
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}

bool dxfRW::readAscii(DRW_Interface *interface_, bool ext, std::string& content) {
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    applyExt = ext;
    iface = interface_;
    std::istringstream strstream(content);
    reader = std::make_unique<dxfReaderAscii>(&strstream);
    bool isOk {processDxf()};
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}

int dxfRW::getBlockRecordHandleToWrite(const std::string& blockName) const {
    auto it = blockMap.find(blockName);
    return (it != blockMap.end()) ? it->second : -1;
}

int dxfRW::getTextStyleHandle(const std::string& styleName) const {
    if (!styleName.empty()) {
        std::string upper = styleName;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        auto it = textStyleMap.find(upper);
        if (it != textStyleMap.end()) return it->second;
    }
    return -1;
}

bool dxfRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin){
    bool isOk = false;
    std::ofstream filestr;
    setVersion(ver);
    binFile = bin;
    iface = interface_;
    if (binFile) {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
        writer = std::make_unique<dxfWriterBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        filestr.open (fileName.c_str(), std::ios_base::out | std::ios::trunc);
        writer = std::make_unique<dxfWriterAscii>(&filestr);
        std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
        writeString(999, comm);
    }
    // DRW_Header header;
    iface->writeHeader(header);
    writeString(0, "SECTION");
    entCount = FIRSTHANDLE;
    // header.write(writer, version);
    writeHeader();
    writeSectionEnd();

    if (afterAC1009) {
        writeSectionStart("CLASSES");
        writeSectionEnd();
    }
    writeSectionStart("TABLES");
    writeTables();
    writeSectionEnd();

    writeSectionStart("BLOCKS");
    writeBlocks();
    writeSectionEnd();

    writeSectionStart("ENTITIES");
    iface->writeEntities();
    writeSectionEnd();

    if (afterAC1009) {
        writeSectionStart("OBJECTS");
        writeObjects();
        writeSectionEnd();
    }
    writeName("EOF");

    filestr.flush();
    filestr.close();
    isOk = true;
    writer.reset();
    return isOk;
}


void dxfRW::writeHeader() {
    /*RLZ: TODO complete all vars to AC1024*/

    int varInt;
    std::string varStr;


    writeString(2, "HEADER");
    writeString(9, "$ACADVER");

    switch (version) {
        case DRW::AC1006: //unsupported version acad 10
        case DRW::AC1009: //acad 11 & 12
            varStr = "AC1009";
            break;
        case DRW::AC1012: //unsupported version acad 13
        case DRW::AC1014: //acad 14
            varStr = "AC1014";
            break;
        case DRW::AC1015: //acad 2000
            varStr = "AC1015";
            break;
        case DRW::AC1018: //acad 2004
            varStr = "AC1018";
            break;
            /*    case DRW::AC1021: //acad 2007
                    varStr = "AC1021";
                    break;*/
        case DRW::AC1024: //acad 2010
            varStr = "AC1024";
            break;
        case DRW::AC1027: //acad 2013
            varStr = "AC1027";
            break;
        default: //acad 2007 default version
            varStr = "AC1021";
            break;
    }
    writeString(1, varStr);
    writer->setVersion(varStr, true);

    header.getStr("$ACADVER", &varStr);
    header.getStr("$ACADMAINTVER", &varStr);

    if (!header.getStr("$DWGCODEPAGE", &varStr)) {
        varStr = "ANSI_1252";
    }
    writeString(9, "$DWGCODEPAGE");
    writer->setCodePage(varStr);
    writeString(3, writer->getCodePage());

    DRW_Coord zeroCoord{0.0, 0.0, 0.0};
    DRW_Coord maxCoord{
        1.0000000000000000E+020,
        1.0000000000000000E+020, 1.0000000000000000E+020
        };


    writeVar("$TITLE", "");
    writeVar("$SUBJECT", "");
    writeVar("$AUTHOR", "");
    writeVar("$KEYWORDS", "");
    writeVar("$COMMENTS", "");


    DRW_Coord xVectorCoord(1.0, 0.0, 0.0);
    DRW_Coord yVectorCoord(0.0, 1.0, 0.0);

    writeVar("$INSBASE", 10, zeroCoord);
    writeVar("$EXTMIN", 10,  maxCoord);
    writeVar("$EXTMAX", 10,  maxCoord);

    writeVar2D("$LIMMIN", 10, zeroCoord);
    writeVar2D("$LIMMAX", 10, {420.0,297.0, 0.0});

    writeVar("$ORTHOMODE", 0);
    writeVar("$REGENMODE", 1);
    writeVar("$FILLMODE", 1);
    writeVar("$QTEXTMODE", 0);
    writeVar("$MIRRTEXT", 0);

    if (version == DRW::AC1009){
        writeVar("$DRAGMODE", 2, 70);
    }

    writeVar("$LTSCALE", 1.0, 40);
    if (version == DRW::AC1009){
        writeVar("$OSMODE", 0, 70);
    }

    writeVar("$ATTMODE", 0, 70);
    writeVar("$TEXTSIZE", 2.5, 40);
    writeVar("$TRACEWID", 15.68, 40);
    writeVar("$TEXTSTYLE", "STANDARD", 7);
    writeVar("$CLAYER", "0", 8); // todo - store current layer
    writeVar("$CELTYPE", "BYLAYER", 6);
    writeVar("$CECOLOR", 256, 62);
    if (afterAC1009){
        writeVar("$CELTSCALE", 1.0, 40);
        writeVar("$DISPSILH", 9, 70);
    }

    // dimvars
    writeVar("$DIMSCALE", 2.5);
    writeVar("$DIMASZ", 2.5);
    writeVar("$DIMEXO", 0.625);
    writeVar("$DIMDLI", 3.75);
    writeVar("$DIMRND", 0.0);
    writeVar("$DIMDLE", 0.0);
    writeVar("$DIMEXE", 1.25);
    writeVar("$DIMTP", 0.0);
    writeVar("$DIMTM", 0.0);
    writeVar("$DIMTXT", 2.5);
    writeVar("$DIMCEN", 2.5);
    writeVar("$DIMTSZ", 0.0);
    writeVar("$DIMTOL", 0);
    writeVar("$DIMLIM", 0);
    writeVar("$DIMTIH", 0);
    writeVar("$DIMTOH", 0);
    writeVar("$DIMSE1", 0);
    writeVar("$DIMSE2", 0);
    writeVar("$DIMTAD", 1);
    writeVar("$DIMZIN", 8);

    writeVar("$DIMBLK", "");

    writeVar("$DIMASO", 1);
    writeVar("$DIMSHO", 1);
    writeVar( "$DIMPOST", "");
    writeVar( "$DIMAPOST", "");

    writeVar("$DIMALT", 0);
    writeVar("$DIMALTD", 3);
    writeVar("$DIMALTF", 0.03937);
    writeVar("$DIMLFAC", 1.0);
    writeVar("$DIMTOFL", 1);
    writeVar("$DIMTVP", 0.0);
    writeVar("$DIMTIX", 0);
    writeVar("$DIMSOXD", 0);
    writeVar("$DIMSAH", 0);

    writeVar("$DIMBLK1", "");
    writeVar("$DIMBLK2", "");
    writeVar("$DIMSTYLE", "STANDARD", 2);

    writeVar("$DIMCLRD", 0);
    writeVar("$DIMCLRE", 0);
    writeVar("$DIMCLRT", 0);
    writeVar("$DIMTFAC", 1.0);
    writeVar("$DIMGAP", 0.625);
    //post r12 dim vars
    if (afterAC1009) {
        writeVar("$DIMJUST", 0);
        writeVar("$DIMSD1", 0);
        writeVar("$DIMSD2", 0);
        writeVar("$DIMTOLJ", 0);
        writeVar("$DIMTZIN", 8);
        writeVar("$DIMALTZ", 0);
        writeVar("$DIMALTTZ", 0);
        writeVar("$DIMUPT", 0);
        writeVar("$DIMDEC", 2);
        writeVar("$DIMTDEC", 2);
        writeVar("$DIMALTU", 2);
        writeVar("$DIMALTTD", 3);

        writeVar("$DIMTXSTY", "STANDARD", 7);

        writeVar("$DIMAUNIT", 0);
        writeVar("$DIMADEC", 0);
        writeVar("$DIMALTRND", 0.0);
        writeVar("$DIMAZIN", 0);
        writeVar("$DIMDSEP", 44);
        writeVar("$DIMATFIT", 3);
        writeVar("$DIMFRAC", 0);

        writeVar("$DIMLDRBLK", "STANDARD");

        //verify if exist "$DIMLUNIT" or obsolete "$DIMUNIT" (pre v2000)
        int varInt;
        if ( !header.getInt("$DIMLUNIT", &varInt) ){
            if (!header.getInt("$DIMUNIT", &varInt))
                varInt = 2;
        }
        //verify valid values from 1 to 6
        if (varInt < 1 || varInt > 6) {
            varInt = 2;
        }
        if (afterAC1014) {
            writeVarExp("$DIMLUNIT", varInt, 70);
        } else {
            writeVarExp("$DIMUNIT", varInt, 70);
        }

        writeVar("$DIMLWD", -2);
        writeVar("$DIMLWE", -2);
        writeVar("$DIMTMOVE", 0);

        if (afterAC1018) {// and post v2004 dim vars
            writeVar("$DIMFXL", 1.0);
            writeVar("$DIMFXLON", 0);
            writeVar("$DIMJOGANG", 0.7854);
            writeVar("$DIMTFILL", 0);
            writeVar("$DIMTFILLCLR", 0);
            writeVar("$DIMARCSYM", 0);

            writeVar("$DIMLTYPE", "", 6);
            writeVar("$DIMLTEX1", "", 6);
            writeVar("$DIMLTEX2", "", 6);
            if (version > DRW::AC1021) {// and post v2007 dim vars
                writeVar("$DIMTXTDIRECTION", 0);
            }
        }// end post v2004 dim vars
    }//end post r12 dim vars

    writeVar("$LUNITS", 2, 70);
    writeVar("$LUPREC", 4, 70);
    writeVar("$SKETCHINC", 1.0, 40);
    writeVar("$FILLETRAD", 0.0, 40);
    writeVar("$AUNITS", 0, 70);
    writeVar("$AUPREC", 2, 70);
    writeVar("$MENU", ".", 1);

    writeVar("$ELEVATION", 0.0, 40);
    writeVar("$PELEVATION", 0.0, 40);
    writeVar("$THICKNESS", 0.0, 40);
    writeVar("$LIMCHECK", 0, 70);
    if (version < DRW::AC1015) {
        writeVar("$BLIPMODE", 0, 70);
    }

    writeVar("$CHAMFERA", 0.0, 40);
    writeVar("$CHAMFERB", 0.0, 40);
    if (afterAC1009) {
        writeVar("$CHAMFERC", 0.0, 40);
        writeVar("$CHAMFERD", 0.0, 40);
    }

    writeVar("$SKPOLY", 0, 70);
    //rlz: todo, times
    writeVar("$USRTIMER", 1, 70);
    writeVar("$ANGBASE", 0.0, 50);
    writeVar("$ANGDIR", 0, 70);
    writeVar("$PDMODE", 34, 70);

    writeVar("$PDSIZE", 0.0, 40);
    writeVar("$PLINEWID", 0.0, 40);

    if (afterAC1012) {
        writeVar("$COORDS", 2, 70);
    }

    writeVar("$SPLFRAME", 0, 70);
    writeVar("$SPLINETYPE", 2, 70);
    writeVar("$SPLINESEGS", 8, 70);
    if (version < DRW::AC1012) {
        writeVar("$ATTDIA", 1, 70);
        writeVar("$ATTREQ", 1, 70);
        writeVar("$HANDLING", 1, 70);
    }
    writeString(9, "$HANDSEED");
    //RLZ        dxfHex(5, 0xFFFF);
    writer->writeString(5, "20000");

    writeVar("$SURFTAB1", 6, 70);
    writeVar("$SURFTAB2", 6, 70);
    writeVar("$SURFTYPE", 6, 70);
    writeVar("$SURFU", 6, 70);
    writeVar("$SURFV", 6, 70);
    if (afterAC1009) {
        writeVar("$UCSBASE", "", 2);
    }

    writeVar("$UCSNAME", "", 2);
    writeVar("$UCSORG", 10, zeroCoord);
    writeVar("$UCSXDIR", 10, xVectorCoord);
    writeVar("$UCSYDIR", 10, yVectorCoord);

    if (afterAC1009) { //begin post r12 UCS vars
        writeVar("$UCSORTHOREF", "", 2);
        writeVar("$UCSORTHOVIEW", 0, 70);
        writeVar("$UCSORGTOP", 10, zeroCoord);
        writeVar("$UCSORGBOTTOM", 10, zeroCoord);
        writeVar("$UCSORGLEFT", 10, zeroCoord);
        writeVar("$UCSORGRIGHT", 10, zeroCoord);
        writeVar("$UCSORGFRONT", 10, zeroCoord);
        writeVar("$UCSORGBACK", 10, zeroCoord);
        writeVar("$PUCSBASE", "", 2);
    } //end post r12 UCS vars

    writeVar("$PUCSNAME", "", 2);
    writeVar("$PUCSORG", 10, zeroCoord);
    writeVar("$PUCSXDIR", 10, xVectorCoord);
    writeVar("$PUCSYDIR", 10, yVectorCoord);

    if (afterAC1009) { //begin post r12 PUCS vars
        writeVar("$PUCSORTHOREF", "", 2);
        writeVar("$PUCSORTHOVIEW", 0, 70);
        writeVar("$PUCSORGTOP", 10, zeroCoord);
        writeVar("$PUCSORGBOTTOM", 10, zeroCoord);
        writeVar("$PUCSORGLEFT", 10, zeroCoord);
        writeVar("$PUCSORGRIGHT", 10, zeroCoord);
        writeVar("$PUCSORGFRONT", 10, zeroCoord);
        writeVar("$PUCSORGBACK", 10, zeroCoord);
    } //end post r12 PUCS vars

    writeVar("$USERI1", 0, 70);
    writeVar("$USERI2", 0, 70);
    writeVar("$USERI3", 0, 70);
    writeVar("$USERI4", 0, 70);
    writeVar("$USERI5", 0, 70);

    writeVar("$USERR1", 0.0, 40);
    writeVar("$USERR2", 0.0, 40);
    writeVar("$USERR3", 0.0, 40);
    writeVar("$USERR4", 0.0, 40);
    writeVar("$USERR5", 0.0, 40);

    writeVar("$WORLDVIEW", 1, 70);
    writeVar("$SHADEDGE", 3, 70);
    writeVar("$SHADEDIF", 70, 70);
    writeVar("$TILEMODE", 1, 70);
    writeVar("$MAXACTVP", 64, 70);

    if (afterAC1009) { //begin post r12 PUCS vars
        writeVar("$PINSBASE", 10, zeroCoord);
    }

    writeVar("$PLIMCHECK", 0, 70);
    writeVar("$PEXTMIN", 10, zeroCoord);
    writeVar("$PEXTMAX", 10, zeroCoord);

    /* RLZ: moved to active VPORT, but can write in header if present*/
    writeVarOpt("$GRIDMODE", 70);
    writeVarOpt("$SNAPSTYLE", 70);

    writeVar2DOpt("$GRIDUNIT", 10);
    writeVar2DOpt("$VIEWCTR", 10);

    /* RLZ: moved to active VPORT, but can write in header if present*/

    writeVar2D("$PLIMMIN", 10, zeroCoord);
    writeVar2D("$PLIMMAX", 10, {297.0, 210.0, 0.0});
    writeVar("$UNITMODE", 0, 70);
    writeVar("$VISRETAIN", 1, 70);
    writeVar("$PLINEGEN", 0, 70);
    writeVar("$PSLTSCALE", 1, 70);

    if (afterAC1009){//start port r12 vars
        writeVar("$TREEDEPTH", 3020, 70);
        writeVar("$CMLSTYLE", "Standard", 2);
        writeVar("$CMLJUST", 0, 70);
        writeVar("$CMLSCALE", 20.0, 40);
        writeVar("$PROXYGRAPHICS", 1, 70);

        int insunits {DRW_Header::Units::None};
        header.getInt("$INSUNITS", &insunits);     // get $INSUNITS now to evaluate $MEASUREMENT
        header.getInt("$MEASUREMENT", &varInt);    // just remove the variable from list

        writeVarExp("$MEASUREMENT", DRW_Header::measurement( insunits), 70);

        writeVar("$CELWEIGHT", -1, 370);
        writeVar("$ENDCAPS", 0, 280);
        writeVar("$JOINSTYLE", 0, 280);
        writeVar("$LWDISPLAY", 0, 290);
        if (afterAC1014) {
            writeVarExp("$INSUNITS", insunits, 70);  // already fetched above for $MEASUREMENT
        }
        writeVar("$HYPERLINKBASE", "", 1);
        writeVar("$STYLESHEET", "", 1);
        writeVar("$XEDIT", 1, 290);
        writeVar("$CEPSNTYPE", 0, 380);
        writeVar("$PSTYLEMODE", 1, 290);

        //RLZ: here $FINGERPRINTGUID and $VERSIONGUID, do not add?

        writeVar("$EXTNAMES", 1, 290);
        writeVar("$PSVPSCALE", 0.0, 40);
        writeVar("$OLESTARTUP", 0, 290);
    }
    if (afterAC1018) {// and post v2007 vars
        writeVar("$CAMERADISPLAY", 0, 290);
        writeVar("$LENSLENGTH", 50.0, 40);
        writeVar("$CAMERAHEIGHT", 0.0, 40);
        writeVar("$STEPSPERSEC", 2.0, 40);
        writeVar("$STEPSIZE", 50.0, 40);
        writeVar("$3DDWFPREC", 2.0, 40);
        writeVar("$PSOLWIDTH", 5.0, 40);
        writeVar("$PSOLHEIGHT", 80.0, 40);
        writeVar("$LOFTANG1", M_PI_2, 40);
        writeVar("$LOFTANG2", M_PI_2, 40);
        writeVar("$LOFTMAG1", 0.0, 40);
        writeVar("$LOFTMAG2", 0.0, 40);
        writeVar("$LOFTPARAM", 7, 70);
        writeVar("$LOFTNORMALS", 1, 280);

        writeVar("$LATITUDE", 1.0);
        writeVar("$LONGITUDE", 1.0);
        writeVar("$NORTHDIRECTION", 0.0);

        //$CMATERIAL is a handle

        writeVar("$TIMEZONE", -8000);
        writeVar("$LIGHTGLYPHDISPLAY", 1, 280);
        writeVar("$TILEMODELIGHTSYNCH", 1, 280);
        writeVar("$SOLIDHIST", 1, 280);
        writeVar("$SHOWHIST", 1, 280);
        writeVar("$DWFFRAME", 2, 280);
        writeVar("$DGNFRAME", 0, 280);

        writeVar("$REALWORLDSCALE", 1, 290);
        writeVar("$INTERFERECOLOR", 1, 62);

        //$INTERFEREOBJVS is a handle
        //$INTERFEREVPVS is a handle

        writeVar("$CSHADOW", 0, 280);
        writeVar("$SHADOWPLANELOCATION", 0.0, 40);
    }

    for (auto it :header.customVars) {
        std::string key = it.first;
        DRW_Variant* var = it.second;
        auto val = var->content.s;
        if (!val->empty()) {
            writeString(9, "$CUSTOMPROPERTYTAG");
            writeString(1, key);
            writeString(9, "$CUSTOMPROPERTY");
            writeString(1, *val);
        }
    }
}

bool dxfRW::writeEntity(DRW_Entity *ent) {
    ent->handle = ++entCount;
    writeHandle(5, ent->handle);
    writeSubClassOpt("Entity");

    if (ent->space == 1) {
        writeInt16(67, 1);
    }
    if (afterAC1009) {
        writeUtf8String(8, ent->layer);
        writeUtf8String(6, ent->lineType);
    } else {
        writeUtf8Caps(8, ent->layer);
        writeUtf8Caps(6, ent->lineType);
    }

    writeInt16(62, ent->color);
    if (afterAC1015 && ent->color24 >= 0) {
        writeInt32(420, ent->color24);
    }
    if (afterAC1015 && !ent->colorName.empty()) {
        writeUtf8String(430, ent->colorName);
    }
    if (version > DRW::AC1018 && ent->shadow != DRW::CastAndReceieveShadows) {
        writeInt16(284, static_cast<int>(ent->shadow));
    }
    if (afterAC1015 && ent->material != DRW::MaterialByLayer) {
        writeUtf8String(347, toHexStr(static_cast<int>(ent->material)));
    }
    if (version > DRW::AC1014) {
        writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
    }
    if (afterAC1015 && ent->plotStyle != DRW::DefaultPlotStyle) {
        writeUtf8String(390, toHexStr(ent->plotStyle));
    }
    if (afterAC1015 && ent->transparency != DRW::Opaque) {
        writeInt32(440, ent->transparency);
    }
    if (version >= DRW::AC1014) {
        writeAppData(ent->appData);
    }
    return true;
}

bool dxfRW::writeAppData(const std::list<std::list<DRW_Variant>>& appData) {
    for(const auto& group : appData) {
        //Search for application name
        bool found = false;

        for(const auto& data : group) {
            if(data.code() == 102 && data.type() == DRW_Variant::STRING) {
                writeString(102, "{" + *(data.content.s));
                found = true;
                break;
            }
        }

        if (found) {
            for (const auto& data : group) {
                if (data.code() == 102) {
                    continue;
                }
                switch (data.type()) {
                    case DRW_Variant::STRING:
                        writeString(data.code(), *(data.content.s));
                        break;

                    case DRW_Variant::INTEGER:
                        writeInt32(data.code(), data.content.i);
                        break;

                    case DRW_Variant::DOUBLE:
                        writeDouble(data.code(), data.content.i);
                        break;

                    default:
                        break;
                }
            }

            writeString(102, "}");
        }
    }
    return true;
}

bool dxfRW::writeLineTypeGenerics(DRW_LType *ent, int handle) {
    writeName("LTYPE");
    std::string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
    if (afterAC1009) {
        auto text = toHexStr(handle);
        writeString(5, text);
        if (afterAC1012) {
            writeString(330, "5");
        }
        writeSymTypeRecord("Linetype");
        writeUtf8String(2, ent->name);
        m_writingContext.lineTypesMap.emplace_back(std::pair<std::string, int>(strname, handle));
    } else {
        writeUtf8Caps(2, strname);
    }
    return true;
}

bool dxfRW::writeLineType(DRW_LType *ent){
    std::string strname = ent->name;

    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
    //do not write linetypes handled by library
    if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") {
        return true;
    }
    writeLineTypeGenerics(ent, ++entCount);
    
    writeInt16(70, ent->flags);
    writeUtf8String(3, ent->desc);
    ent->update();
    writeInt16(72, 65);
    writeInt16(73, ent->size);
    writeDouble(40, ent->length);

    size_t size = ent->path.size();
    for (unsigned int i = 0;  i< size; i++){
        writeDouble(49, ent->path.at(i));
        if (afterAC1009) {
            writeInt16(74, 0);
        }
    }
    return true;
}

bool dxfRW::writeLayer(DRW_Layer *ent){
    writeName("LAYER");
    if (!wlayer0 && ent->name == "0") {
        wlayer0 = true;
        if (afterAC1009) {
            writeString(5, "10");
        }
    } else {
        if (afterAC1009) {
            writeString(5, toHexStr(++entCount));
        }
    }
    if (afterAC1012) {
        writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLayerTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);
    writeInt16(62, ent->color);
    if (afterAC1015 && ent->color24 >= 0) {
        writeInt32(420, ent->color24);
    }
    if (afterAC1009) {
        writeUtf8String(6, ent->lineType);
        if (!ent->plotF) {
            writeBool(290, ent->plotF);
        }
        writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
        writeString(390, "F");
    } else {
        writeUtf8Caps(6, ent->lineType);
    }
    if (!ent->extData.empty()){
        writeExtData(ent->extData);
    }
//    writeString(347, "10012");
    return true;
}

bool dxfRW::writeUCS(DRW_UCS* ent){
    writeName("UCS");
    if (afterAC1009) {
        writeString(5, toHexStr(++entCount));
        if (afterAC1012) {
            writeString(330, "7");
            // 	Soft-pointer ID/handle to owner object, fixme - check whether id is fixed as for layers?
        }
        writeSymTypeRecord("UCS");
        writeUtf8String(2, ent->name);
    }
    else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);
    writeCoord(10, ent->origin);
    writeCoord(11, ent->xAxisDirection);
    writeCoord(12, ent->yAxisDirection);

    writeInt16(79, 0);
    //ID/handle of base UCS if this is an orthographic. This code is not present if the 79 code is 0.
    // If this code is not present and 79 code is non-zero, then base UCS is assumed to be WORLD
    // fixme - review this. Probably we'll skip support of it?
    // writeInt16(346, 0);

    writeDouble(146, ent->elevation);
    writeDouble(71, ent->orthoType);

    writeCoord(13, ent->orthoOrigin);
    return true;
}

bool dxfRW::writeView(DRW_View *ent){
    writeName("VIEW");
    if (afterAC1009) {
        writeString(5, toHexStr(++entCount));
        if (afterAC1012) {
            writeString(330, "6");
            // 	Soft-pointer ID/handle to owner object, fixme - check whether id is fixed as for layers?
        }
        writeSymTypeRecord("View");
        writeUtf8String(2, ent->name);
    }
    else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);

    writeDouble(40, ent->size.y);
    writeDouble(10, ent->center.x);
    writeDouble(20, ent->center.y);
    writeDouble(41, ent->size.x);

    writeCoord(11, ent->viewDirectionFromTarget);
    writeCoord(12, ent->targetPoint);

    writeDouble(42, ent->lensLen);
    writeDouble(43, ent->frontClippingPlaneOffset);
    writeDouble(44, ent->backClippingPlaneOffset);
    writeDouble(50, ent->twistAngle);
    writeInt16(71, ent->viewMode);
    writeInt16(281, ent->renderMode);


    if (afterAC1009) {
        writeInt16(281, static_cast<int>(ent->renderMode));
        writeBool(72, ent->hasUCS);
        writeBool(73, ent->cameraPlottable);
        /*
         * fixme - investigate deep whether we should support these attributes
        writeString(332, "42"); // Soft-pointer ID/handle to background object (optional)
        writeString(334, "42"); // Soft-pointer ID/handle to live section object (optional)
        writeString(348, "42"); // Hard-pointer ID/handle to visual style object (optional)
        */

        if (ent->hasUCS){
            writeCoord(110, ent->ucsOrigin);
            writeCoord(111, ent->ucsXAxis);
            writeCoord(112, ent->ucsYAxis);
            writeInt16(79, ent->ucsOrthoType);
            writeDouble(146, ent->ucsElevation);

            /*
              * fixme - investigate deep whether we should support these attributes
             //ID/handle of AcDbUCSTableRecord if UCS is a named UCS. If not present, then UCS is unnamed (appears only if code 72 is set to 1)
             writeString(345, "42");
             // Soft-pointer ID/handle to live section object (optional)
             writeString(346, "42");
             */
        }
    }
    return true;
}


bool dxfRW::writeTextstyle(DRW_Textstyle *ent){
    writeName("STYLE");
    //stringstream cause crash in OS/X, bug#3597944
    std::string name=ent->name;
    transform(name.begin(), name.end(), name.begin(), toupper);
    if (!dimstyleStd) {
        if (name == "STANDARD"){
            dimstyleStd = true;
        }
    }
    if (afterAC1009) {
        writeString(5, toHexStr(++entCount));
        m_writingContext.textStyleMap[name] = entCount;
        if (afterAC1012) {
            writeString(330, "2");
        }
        writeSymTypeRecord("TextStyle");
        writeUtf8String(2, ent->name);
    } else {
        writer->writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);
    writeDouble(40, ent->height);
    writeDouble(41, ent->width);
    writeDouble(50, ent->oblique);
    writeInt16(71, ent->genFlag);
    writeDouble(42, ent->lastHeight);
    if (afterAC1009) {
        writeUtf8String(3, ent->font);
        writeUtf8String(4, ent->bigFont);
        if (ent->fontFamily != 0) {
            writeInt32(1071, ent->fontFamily);
        }
    } else {
        writeUtf8Caps(3, ent->font);
        writeUtf8Caps(4, ent->bigFont);
    }
    return true;
}

bool dxfRW::writeVport(DRW_Vport *ent){
    if (!dimstyleStd) {
        ent->name = "*ACTIVE";
        dimstyleStd = true;
    }
    writeName( "VPORT");
    if (afterAC1009) {
        writeString(5, toHexStr(++entCount));
        if (afterAC1012) {
            writeString(330, "2");
        }
        writeSymTypeRecord("Viewport");
        writeUtf8String(2, ent->name);
    }
    else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);
    writeDouble(10, ent->lowerLeft.x);
    writeDouble(20, ent->lowerLeft.y);
    writeDouble(11, ent->UpperRight.x);
    writeDouble(21, ent->UpperRight.y);
    writeDouble(12, ent->center.x);
    writeDouble(22, ent->center.y);
    writeDouble(13, ent->snapBase.x);
    writeDouble(23, ent->snapBase.y);
    writeDouble(14, ent->snapSpacing.x);
    writeDouble(24, ent->snapSpacing.y);
    writeDouble(15, ent->gridSpacing.x);
    writeDouble(25, ent->gridSpacing.y);
    writeCoord(16, ent->viewDir);
    writeCoord(17, ent->viewTarget);
    writeDouble(40, ent->height);
    writeDouble(41, ent->ratio);
    writeDouble(42, ent->lensHeight);
    writeDouble(43, ent->frontClip);
    writeDouble(44, ent->backClip);
    writeDouble(50, ent->snapAngle);
    writeDouble(51, ent->twistAngle);
    writeInt16(71, ent->viewMode);
    writeInt16(72, ent->circleZoom);
    writeInt16(73, ent->fastZoom);
    writeInt16(74, ent->ucsIcon);
    writeInt16(75, ent->snap);
    writeInt16(76, ent->grid);
    writeInt16(77, ent->snapStyle);
    writeInt16(78, ent->snapIsopair);
    if (afterAC1014) {
        writeInt16(281, 0);
        writeInt16(65, 1);
        writeDouble(110, 0.0); // fixme - write as coords
        writeDouble(120, 0.0);
        writeDouble(130, 0.0);
        writeDouble(111, 1.0);
        writeDouble(121, 0.0);
        writeDouble(131, 0.0);
        writeDouble(112, 0.0);
        writeDouble(122, 1.0);
        writeDouble(132, 0.0);
        writeInt16(79, 0);
        writeDouble(146, 0.0);
        if (afterAC1018) {
            writeString(348, "10020");
            writeInt16(60, ent->gridBehavior);//v2007 undocummented see DRW_Vport class
            writeInt16(61, 5);
            writeBool(292, 1);
            writeInt16(282, 1);
            writeDouble(141, 0.0);
            writeDouble(142, 0.0);
            writeInt16(63, 250);
            writeInt32(421, 3358443);
        }
    }
    return true;
}

bool dxfRW::writeDouble(int code, DRW_Dimstyle* ent, const std::string& name) {
    double val;
    if (ent->get(name, val)) {
        writeDouble(code, val);
    }
    return true;
}

bool dxfRW::writeInt16(int code, DRW_Dimstyle* ent, const std::string& name) {
    int val;
    if (ent->get(name, val)) {
        writeInt16(code, val);
    }
    return true;
}

bool dxfRW::writeUtf8String(int code, DRW_Dimstyle* ent, const std::string& name) {
    std::string  val;
    if (ent->get(name, val)) {
        writeUtf8String(code, val);
    }
    return true;
}

bool dxfRW::writeDimstyle(DRW_Dimstyle *ent) {
    writeName("DIMSTYLE");
    // if (!dimstyleStd) {
    //     std::string name = ent->name;
    //     std::transform(name.begin(), name.end(), name.begin(),::toupper);
    //     if (name == "STANDARD")
    //         dimstyleStd = true;
    // }
    if (afterAC1009) {
        writeString(105, toHexStr(++entCount));
        if (afterAC1012) {
            writeString(330, "A");
        }
        writeSymTypeRecord("DimStyle");
        writeUtf8String(2, ent->name);
    }
    else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);

    //DRW_Dimstyle::ValueHolder var;

    writeUtf8String(3, ent, "$DIMPOST");
    writeUtf8String(4, ent, "$DIMAPOST");

    writeUtf8String(5, ent, "$DIMBLK");
    writeUtf8String(6, ent, "$DIMBLK1");
    writeUtf8String(7, ent, "$DIMBLK2");


    writeDouble(40, ent, "$DIMSCALE");
    writeDouble(41, ent, "$DIMASZ");
    writeDouble(42, ent, "$DIMEXO");
    writeDouble(43, ent, "$DIMDLI");
    writeDouble(44, ent, "$DIMEXE");
    writeDouble(45, ent, "$DIMRND");
    writeDouble(46, ent, "$DIMDLE");
    writeDouble(47, ent, "$DIMTP");
    writeDouble(48, ent, "$DIMTM");

    if (afterAC1018) {
        writeDouble(49, ent, "$DIMFXL");

        writeInt16(69, ent, "$DIMTFILL");
        writeInt16(70, ent, "$DIMTFILLCLR");
    }

    writeDouble(140, ent, "$DIMTXT");
    writeDouble(141, ent, "$DIMCEN");
    writeDouble(142, ent, "$DIMTSZ");
    writeDouble(143, ent, "$DIMALTF");
    writeDouble(144, ent, "$DIMLFAC");
    writeDouble(145, ent, "$DIMTVP");
    writeDouble(146, ent, "$DIMTFAC");
    writeDouble(147, ent, "$DIMGAP");

    if (afterAC1014) {
        writeDouble(148, ent, "$DIMALTRND");
    }
    writeInt16(71, ent, "$DIMTOL");
    writeInt16(72, ent, "$DIMLIM");
    writeInt16(72, ent, "$DIMLIM");
    writeInt16(73, ent, "$DIMTIH");
    writeInt16(74, ent, "$DIMTOH");
    writeInt16(75, ent, "$DIMSE1");
    writeInt16(76, ent, "$DIMSE2");
    writeInt16(77, ent, "$DIMTAD");
    writeInt16(78, ent, "$DIMZIN");

    if (afterAC1014) {
        writeInt16(79, ent, "$DIMAZIN");
    }
    writeInt16(170, ent, "$DIMALT");
    writeInt16(171, ent, "$DIMALTD");
    writeInt16(172, ent, "$DIMTOFL");
    writeInt16(173, ent, "$DIMSAH");
    writeInt16(174, ent, "$DIMTIX");
    writeInt16(175, ent, "$DIMSOXD");
    writeInt16(176, ent, "$DIMCLRD");
    writeInt16(177, ent, "$DIMCLRE");
    writeInt16(178, ent, "$DIMCLRT");

    if (afterAC1014) {
        writeInt16(179, ent, "$DIMADEC");
    }
    if (afterAC1009) {
        if (version < DRW::AC1015) {
            writeInt16(270, ent, "$DIMLUNIT"); // obsolete dimunit...
        }
        writeInt16(271, ent, "$DIMDEC");
        writeInt16(272, ent, "$DIMTDEC");
        writeInt16(273, ent, "$DIMALTU");
        writeInt16(274, ent, "$DIMALTTD");
        writeInt16(275, ent, "$DIMAUNIT");
    }
    if (afterAC1014) {
        writeInt16(276, ent, "$DIMFRAC");
        writeInt16(277, ent, "$DIMLUNIT");
        writeInt16(278, ent, "$DIMDSEP");
        writeInt16(279, ent, "$DIMTMOVE");
    }
    if (afterAC1009) {
        writeInt16(280, ent, "$DIMJUST");
        writeInt16(281, ent, "$DIMSD1");
        writeInt16(282, ent, "$DIMSD2");
        writeInt16(283, ent, "$DIMTOLJ");
        writeInt16(284, ent, "$DIMTZIN");
        writeInt16(285, ent, "$DIMALTZ");
        writeInt16(286, ent, "$DIMALTTZ");

        if (version < DRW::AC1015) {
            writeInt16(287, ent, "$DIMATFIT");
        }
        writeInt16(288, ent, "$DIMUPT");
    }
    if (afterAC1014) {
        writeInt16(289, ent, "$DIMATFIT");
    }
    if (afterAC1018) {
        writeInt16(290, ent, "$DIMFXLON");
    }
    if (afterAC1009) {
        writeInt16(292, ent, "$DIMTXTDIRECTION"); // fixme - which version?
        writeUtf8String(340, ent, "$DIMTXSTY");
    }
    if (afterAC1014) {
        writeUtf8String(341, ent, "_$DIMLDRBLK"); // block id for arrow

        writeUtf8String(342, ent, "_$DIMBLK"); // block id for arrow
        writeUtf8String(343, ent, "_$DIMBLK1"); // block id for arrow
        writeUtf8String(344, ent, "_$DIMBLK2"); // block id for arrow

        writeUtf8String(345, ent, "$DIMLTYPE"); // ref to linetype...

        // writeUtf8String(346, ent, "$DIMLTYPE"); // enum linetype name
        writeUtf8String(347, ent, "$DIMLTEX1"); // block it for arrow
        writeUtf8String(348, ent, "$DIMLTEX2"); // block it for arrow

        writeInt16(371, ent, "$DIMLWD");
        writeInt16(372, ent, "$DIMLWE");
        writeInt16(90, ent, "$DIMARCSYM");
    }

    if (!ent->extData.empty()) {
        writeExtData(ent->extData);
    }
    return true;
}


bool dxfRW::writeAppId(DRW_AppId *ent){
    std::string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
    //do not write mandatory ACAD appId, handled by library
    if (strname == "ACAD") {
        return true;
    }
    writeName("APPID");
    if (afterAC1009) {
        writeString(5, toHexStr(++entCount));
        if (afterAC1014) {
            writeString(330, "9");
        }
        writeSymTypeRecord("RegApp");
        writeUtf8String(2, ent->name);
    } else {
        writeUtf8Caps(2, ent->name);
    }
    writeInt16(70, ent->flags);
    return true;
}

bool dxfRW::writePoint(DRW_Point *ent) {
    writeName( "POINT");
    writeEntity(ent);
    writeSubClassOpt("Point");
    writeCoord(10, ent->basePoint);
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent) {
    writeName("LINE");
    writeEntity(ent);
    writeSubClassOpt("Line");
    writeCoord(10, ent->basePoint);
    writeCoord(11, ent->secPoint);
    return true;
}

bool dxfRW::writeRay(DRW_Ray *ent) {
    writeName("RAY");
    writeEntity(ent);
    writeSubClassOpt("Ray");

    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writeCoord(10, ent->basePoint);
    writeCoord(11, crd);
    return true;
}

bool dxfRW::writeXline(DRW_Xline *ent) {
    writeName("XLINE");
    writeEntity(ent);
    writeSubClassOpt("Xline");
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writeCoord(10, ent->basePoint);
    writeCoord(11, crd);
    return true;
}

bool dxfRW::writeCircle(DRW_Circle *ent) {
    writeName("CIRCLE");
    writeEntity(ent);
    writeSubClassOpt("Circle");
    writeCoord(10, ent->basePoint);
    writeDouble(40, ent->radious);
    return true;
}

bool dxfRW::writeArc(DRW_Arc *ent) {
    writeName("ARC");
    writeEntity(ent);

    writeSubClassOpt("Circle");
    writeCoord(10, ent->basePoint);
    writeDouble(40, ent->radious);

    writeSubClassOpt("Arc");
    writeDouble(50, ent->staangle*ARAD);
    writeDouble(51, ent->endangle*ARAD);
    return true;
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent){
    //verify axis/ratio and params for full ellipse
    ent->correctAxis();
    if (afterAC1009) {
        writeName("ELLIPSE");
        writeEntity(ent);
        writeSubClassOpt("Ellipse");
        writeCoord(10, ent->basePoint);
        writeCoord(11, ent->secPoint);
        writeDouble(40, ent->ratio);
        writeDouble(41, ent->staparam);
        writeDouble(42, ent->endparam);
    } else {
        DRW_Polyline pol;
        //RLZ: copy properties
        ent->toPolyline(&pol, elParts);
        writePolyline(&pol);
    }
    return true;
}

bool dxfRW::writeTrace(DRW_Trace *ent){
    writeName("TRACE");
    writeEntity(ent);
    writeSubClassOpt("Trace");
    writeCoord(10, ent->basePoint);
    writeCoord(11, ent->secPoint);
    writeCoord(12, ent->thirdPoint);
    writeCoord(13, ent->fourPoint);
    return true;
}

bool dxfRW::writeSolid(DRW_Solid *ent){
    writeName("SOLID");
    writeEntity(ent);
    writeSubClassOpt("Trace");
    writeCoord(10, ent->basePoint);
    writeCoord(11, ent->secPoint);
    writeCoord(12, ent->thirdPoint);
    writeCoord(13, ent->fourPoint);
    return true;
}

bool dxfRW::write3dface(DRW_3Dface *ent){
    writeName( "3DFACE");
    writeEntity(ent);
    writeSubClassOpt("Face");
    writeCoord(10, ent->basePoint);
    writeCoord(11, ent->secPoint);
    writeCoord(12, ent->thirdPoint);
    writeCoord(13, ent->fourPoint);
    writeInt16(70, ent->invisibleflag);
    return true;
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent){
    if (afterAC1009) {
        writeName("LWPOLYLINE");
        writeEntity(ent);
        writeSubClassOpt("Polyline");
        ent->vertexnum = ent->vertlist.size();
        writeInt32(90, ent->vertexnum);
        writeInt16(70, ent->flags);
        writeDouble(43, ent->width);
        writeDoubleOpt(38, ent->elevation);
        writeDoubleOpt(39, ent->thickness);

        for (int i = 0;  i< ent->vertexnum; i++){
            auto& v = ent->vertlist.at(i);
            writeDouble(10, v->x);
            writeDouble(20, v->y);
            writeDoubleOpt(40, v->stawidth);
            writeDoubleOpt(41, v->endwidth);
            writeDoubleOpt(42, v->bulge);
        }
    } else {
        //RLZ: TODO convert lwpolyline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writePolyline(DRW_Polyline *ent) {
    writeName("POLYLINE");
    writeEntity(ent);
    if (afterAC1009) {
        if (ent->flags & 8 || ent->flags & 16) {
            writeSubClass("3dPolyline");
        }
        else {
            writeSubClass("2dPolyline");
        }
    } else {
        writeInt16(66, 1);
    }
    DRW_Coord coord(0, 0, ent->basePoint.z);
    writeCoord(10, coord);

    writeDoubleOpt(39, ent->thickness);

    writeInt16(70, ent->flags);
    writeDoubleOpt(40, ent->defstawidth);
    writeDoubleOpt(41, ent->defendwidth);

    if (ent->flags & 16 || ent->flags & 32) {
        writeInt16(71, ent->vertexcount);
        writeInt16(72, ent->facecount);
    }
    if (ent->smoothM != 0) {
        writeInt16(73, ent->smoothM);
    }
    if (ent->smoothN != 0) {
        writeInt16(74, ent->smoothN);
    }
    if (ent->curvetype != 0) {
        writeInt16(75, ent->curvetype);
    }
    DRW_Coord crd  = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writeCoord(210, crd);
    }

    DRW_Coord zero(0,0,0);

    int vertexnum = ent->vertlist.size();
    for (int i = 0;  i< vertexnum; i++){
        DRW_Vertex *v = ent->vertlist.at(i).get();
        writeName( "VERTEX");
        writeEntity(ent);
        writeSubClassOpt("Vertex");
        if ( (v->flags & 128) && !(v->flags & 64) ) {
            writeCoord(10,  zero);
        } else {
            writeCoord(10,  v->basePoint);
        }
        writeDoubleOpt(40, v->stawidth);
        writeDoubleOpt(41, v->endwidth);
        writeDoubleOpt(42, v->bulge);

        if (v->flags != 0) {
            writeInt16(70, ent->flags);
        }
        if (v->flags & 2) {
            writeDouble(50, v->tgdir);
        }
        if ( v->flags & 128 ) {
            if (v->vindex1 != 0) {
                writeInt16(71, v->vindex1);
            }
            if (v->vindex2 != 0) {
                writeInt16(72, v->vindex2);
            }
            if (v->vindex3 != 0) {
                writeInt16(73, v->vindex3);
            }
            if (v->vindex4 != 0) {
                writeInt16(74, v->vindex4);
            }
            if ( !(v->flags & 64) ) {
                writeInt32(91, v->identifier);
            }
        }
    }
    writeName("SEQEND");
    writeEntity(ent);
    return true;
}

bool dxfRW::writeSpline(DRW_Spline *ent){
    if (afterAC1009) {
        writeName("SPLINE");
        writeEntity(ent);
        writeSubClassOpt("Spline");
        writeCoord(210, ent->normalVec);

        writeInt16(70, ent->flags);
        writeInt16(71, ent->degree);
        writeInt16(72, ent->nknots);
        writeInt16(73, ent->ncontrol);
        writeInt16(74, ent->nfit);
        writeDouble(42, ent->tolknot);
        writeDouble(43, ent->tolcontrol);
        writeDouble(44, ent->tolfit);
        //RLZ: warning check if nknots are correct and ncontrol
        for (int i = 0;  i< ent->nknots; i++){
            writeDouble(40, ent->knotslist.at(i));
        }
        size_t size = ent->weightlist.size();
        for (std::size_t i = 0; i< size; i++) {
            writeDouble(41, ent->weightlist.at(i));
        }
        for (int i = 0;  i< ent->ncontrol; i++){
            auto crd = ent->controllist.at(i);
            writeCoord(10, *crd);
        }
        //fit points: required for splinepoints / fit-point-driven splines
        for (int i = 0;  i< ent->nfit; i++){
            auto crd = ent->fitlist.at(i);
            writeCoord(11, *crd);
        }
    } else {
        //RLZ: TODO convert spline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeHatch(DRW_Hatch *ent){
    if (afterAC1009) {
        writeName("HATCH");
        writeEntity(ent);
        writeSubClassOpt("Hatch");
        writeDouble(10, 0.0);
        writeDouble(20, 0.0);
        writeDouble(30, ent->basePoint.z);

        writeCoord(210, ent->extPoint);

        writeString(2, ent->name);
        writeInt16(70, ent->solid);
        writeInt16(71, ent->associative);
        ent->loopsnum = ent->looplist.size();
        writeInt16(91, ent->loopsnum);
        //write paths data
        for (int i = 0;  i< ent->loopsnum; i++){
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writeInt16(92, loop->type);
            if ((loop->type & 2) == 2) {
                //RLZ: polyline boundary writeme
            }
            else {
                //boundary path
                loop->update();
                writeInt16(93, loop->numedges);
                for (int j = 0; j<loop->numedges; ++j) {
                    switch ((loop->objlist.at(j))->eType) {
                        case DRW::LINE: {
                            writeInt16(72, 1);
                            auto l = static_cast<DRW_Line*>(loop->objlist.at(j).get());
                            writeDouble(10, l->basePoint.x); // fixme - sand - short version of vector write!!
                            writeDouble(20, l->basePoint.y);
                            writeDouble(11, l->secPoint.x); // fixme - sand - short version of vector write!!
                            writeDouble(21, l->secPoint.y);
                            break;
                        }
                        case DRW::ARC: {
                            writeInt16(72, 2);
                            auto a = static_cast<DRW_Arc*>(loop->objlist.at(j).get());
                            writeDouble(10, a->basePoint.x); // fixme - sand - short version of vector write!!
                            writeDouble(20, a->basePoint.y);
                            writeDouble(40, a->radious);
                            writeDouble(50, a->staangle * ARAD);
                            writeDouble(51, a->endangle * ARAD);
                            writeInt16(73, a->isccw);
                            break;
                        }
                        case DRW::ELLIPSE: {
                            writeInt16(72, 3);
                            auto a = static_cast<DRW_Ellipse*>(loop->objlist.at(j).get());
                            a->correctAxis();
                            writeDouble(10, a->basePoint.x); // fixme - sand - short version of vector write!!
                            writeDouble(20, a->basePoint.y);
                            writeDouble(11, a->secPoint.x); // fixme - sand - short version of vector write!!
                            writeDouble(21, a->secPoint.y);
                            writeDouble(40, a->ratio);
                            writeDouble(50, a->staparam * ARAD);
                            writeDouble(51, a->endparam * ARAD);
                            writeInt16(73, a->isccw);
                            break;
                        }
                        case DRW::SPLINE:{
                            writer->writeInt16(72, 4);
                            DRW_Spline* sp = (DRW_Spline*)loop->objlist.at(j).get();
                            writer->writeInt32(94, sp->degree);
                            const bool rational = (sp->flags & 0x4) != 0;
                            const bool periodic = (sp->flags & 0x2) != 0;
                            writer->writeInt16(73, rational ? 1 : 0);
                            writer->writeInt16(74, periodic ? 1 : 0);
                            writer->writeInt32(95, static_cast<int>(sp->knotslist.size()));
                            writer->writeInt32(96, static_cast<int>(sp->controllist.size()));
                            for (double k : sp->knotslist) {
                                writer->writeDouble(40, k);
                            }
                            for (size_t k = 0; k < sp->controllist.size(); ++k) {
                                const auto& cp = sp->controllist[k];
                                if (!cp) continue;
                                writer->writeDouble(10, cp->x);
                                writer->writeDouble(20, cp->y);
                                if (rational) {
                                    double w = (k < sp->weightlist.size()) ? sp->weightlist[k] : 1.0;
                                    writer->writeDouble(42, w);
                                }
                            }
                            writer->writeInt32(97, static_cast<int>(sp->fitlist.size()));
                            for (const auto& fp : sp->fitlist) {
                                if (!fp) continue;
                                writer->writeDouble(11, fp->x);
                                writer->writeDouble(21, fp->y);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                writeInt16(97, 0);
            }
        }
        writeInt16(75, ent->hstyle);
        writeInt16(76, ent->hpattern);
        if (!ent->solid){
            writeDouble(52, ent->angle);
            writeDouble(41, ent->scale);
            writeInt16(77, ent->doubleflag);
            writeInt16(78, ent->deflines);
        }
        // Seed points (group 98 = count, then 10/20 pairs).
        const int seedCount = static_cast<int>(ent->seedPoints.size());
        writeInt32(98, seedCount);
        for (const DRW_Coord &pt : ent->seedPoints) {
            writeDouble(10, pt.x);
            writeDouble(20, pt.y);
        }
        // Gradient block (R2004+ DXF; codes 450-470 + 463/421/63 per stop).
        if (ent->isGradient) {
            writeInt32(450, ent->isGradient);
            writeInt32(451, ent->gradReserved);
            writeDouble(460, ent->gradAngle);
            writeDouble(461, ent->gradShift);
            writeInt32(452, ent->singleColor);
            writeDouble(462, ent->gradTint);
            writeInt32(453, static_cast<int>(ent->gradColors.size()));
            for (const DRW_Hatch::GradientStop &stop : ent->gradColors) {
                writeDouble(463, stop.value);
                writeInt32(421, stop.rgb);
                if (stop.aciColor != 0)
                    writeInt32(63, stop.aciColor);
            }
            writeUtf8String(470, ent->gradName);
        }
    } else {
        //RLZ: TODO verify in acad12
    }
    return true;
}

bool dxfRW::writeLeader(DRW_Leader *ent){
    if (afterAC1009) {
        writeName("LEADER");
        writeEntity(ent);
        writeSubClass("Leader");
        writeUtf8String(3, ent->style);
        writeInt16(71, ent->arrow);
        writeInt16(72, ent->leadertype);
        writeInt16(73, ent->flag);
        writeInt16(74, ent->hookline);
        writeInt16(75, ent->hookflag);
        writeDouble(40, ent->textheight);
        writeDouble(41, ent->textwidth);
        writeDouble(76, ent->vertnum); // fixme - review why twice?? and double instead of int
        size_t size = ent->vertexlist.size();
        writeDouble(76, size);
        for (unsigned int i=0; i<size; i++) {
            auto vert = ent->vertexlist.at(i);
            writeCoord(10, *vert);
        }
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}
bool dxfRW::writeDimension(DRW_Dimension *ent) {
    if (afterAC1009) {
        writeName("DIMENSION");
        writeEntity(ent);
        writeSubClass("Dimension");
        if (!ent->getName().empty()){
            writeString(2, ent->getName());  // write name??
        }

        writeCoord(10, ent->getDefPoint());
        writeCoord(11, ent->getTextPoint());

        if (!(ent->type & 32)) {
            // fixme - sand - ordinate type support !!!??
            ent->type = ent->type +32;
        }
        writeInt16(70, ent->type);
        if (!(ent->getText().empty()) ) {
            writeUtf8String(1, ent->getText());
        }
        writeInt16(71, ent->getAlign());
        if (ent->getTextLineStyle() != 1) {
            writeInt16(72, ent->getTextLineStyle());
        }
        if (ent->getTextLineFactor() != 1){
            writeDouble(41, ent->getTextLineFactor());
        }
        writeUtf8String(3, ent->getStyle());
        if (ent->getTextLineFactor() != 0){
            writeDouble(53, ent->getDir());
        }

        writeDoubleOpt(51, ent->getHDir());
        writeCoord(210, ent->getExtrusion());

        if (ent->getFlipArrow1()) {
            writeBool(74,true);
        }
        if (ent->getFlipArrow2()) {
            writeBool(75,true);
        }

        switch (ent->eType) {
            case DRW::DIMALIGNED:
            case DRW::DIMLINEAR: {
                auto* dd = static_cast<DRW_DimAligned*>(ent);
                writeSubClass("AlignedDimension");
                DRW_Coord crd = dd->getClonepoint();
                if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
                    writeCoord(12, crd);
                }
                writeCoord(13, dd->getDef1Point());
                writeCoord(14, dd->getDef2Point());

                if (ent->eType == DRW::DIMLINEAR) {
                    auto dl = static_cast<DRW_DimLinear*>(ent);
                    writeSubClass("RotatedDimension");
                    // writeDouble(50, dl->getAngle());
                    writeDoubleOpt(50, dl->getAngle());
                    // writeDouble(52, dl->getOblique());
                    writeDoubleOpt(52, dl->getOblique());
                }
                break;
            }
            case DRW::DIMRADIAL: {
                auto* dd = static_cast<DRW_DimRadial*>(ent);
                writeSubClass("RadialDimension");
                writeCoord(15, dd->getDiameterPoint());
                writeDouble(40, dd->getLeaderLength());
                break;
            }
            case DRW::DIMDIAMETRIC: {
                auto* dd = static_cast<DRW_DimDiametric*>(ent);
                writeSubClass("DiametricDimension");
                writeCoord(15, dd->getDiameter1Point());
                writeDouble(40, dd->getLeaderLength());
                break;
            }
            case DRW::DIMANGULAR: {
                auto* dd = static_cast<DRW_DimAngular*>(ent);
                writeSubClass("2LineAngularDimension");
                writeCoord(13, dd->getFirstLine1());
                writeCoord(14, dd->getFirstLine2());
                writeCoord(15, dd->getSecondLine1());
                writeCoord(16, dd->getDimPoint());
                break;
            }
            case DRW::DIMANGULAR3P: {
                auto* dd = static_cast<DRW_DimAngular3p*>(ent);
                writeCoord(13, dd->getFirstLine());
                writeCoord(14, dd->getSecondLine());
                writeCoord(15, dd->getVertexPoint());
                break;
            }
            case DRW::DIMORDINATE: {
                auto* dd = static_cast<DRW_DimOrdinate*>(ent);
                writeSubClass("OrdinateDimension");
                writeCoord(13, dd->getFirstLine());
                writeCoord(14, dd->getSecondLine());
                break;
            }
            default:
                break;
        }

        writeEntityExtData(ent);
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}

bool dxfRW::writeEntityExtData(DRW_Entity* ent) {
    for (auto r: ent->extData) {
        auto type = r->type();
        switch (type) {
            case DRW_Variant::INTEGER: {
                writeInt16(r->code(), r->i_val());
                break;
            }
            case DRW_Variant::DOUBLE: {
                writeDouble(r->code(), r->d_val());
                break;
            }
            case DRW_Variant::STRING: {
                writeString(r->code(), r->c_str());
                break;
            }
            case DRW_Variant::COORD: {
                // fixme - sand - ext data - process writing ext data coordinate???
                // writeString(r->code(), r->setCoordX());
                break;
            }
            default:
                break;
        }
    }
    return true;
}

bool dxfRW::writeInsert(DRW_Insert *ent){
    writeName("INSERT");
    writeEntity(ent);
    if (afterAC1009) {
        writeSubClass("BlockReference");
        writeUtf8String(2, ent->name);
    } else {
        writeUtf8Caps(2, ent->name);
    }
    writeCoord(10, ent->basePoint);
    writeDouble(41, ent->xscale);
    writeDouble(42, ent->yscale);
    writeDouble(43, ent->zscale);
    writeDouble(50, (ent->angle)*ARAD); //in dxf angle is writed in degrees
    writeInt16(70, ent->colcount);
    writeInt16(71, ent->rowcount);
    writeDouble(44, ent->colspace);
    writeDouble(45, ent->rowspace);
    return true;
}

bool dxfRW::writeText(DRW_Text *ent) {
    writeName("TEXT");
    writeEntity(ent);
    writeSubClassOpt("Text");

    //    writeDouble(39, ent->thickness);
    writeCoord(10, ent->basePoint);

    writeDouble(40, ent->height);
    writeUtf8String(1, ent->text);
    writeDouble(50, ent->angle);
    writeDouble(41, ent->widthscale);
    writeDouble(51, ent->oblique);
    if (afterAC1009) {
        writeUtf8String(7, ent->style);
    }
    else {
        writeUtf8Caps(7, ent->style);
    }
    writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft) {
        writeInt16(72, ent->alignH);
    }
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writeCoord(11, ent->secPoint);
    }
    writeCoord(210, ent->extPoint);
    if (afterAC1009) {
        writeSubClass("Text");
    }
    if (ent->alignV != DRW_Text::VBaseLine) {
        writeInt16(73, ent->alignV);
    }
    return true;
}

// fixme - sand - rework to shorter form
bool dxfRW::writeMLine(DRW_MLine *ent) {
    if (version <= DRW::AC1009) return true;  // MLINE is R13+
    writer->writeString(0, "MLINE");
    writeEntity(ent);
    writer->writeString(100, "AcDbMline");
    writer->writeUtf8String(2, ent->styleName);
    if (ent->styleHandle != 0) {
        writer->writeString(340, toHexStr(static_cast<int>(ent->styleHandle)));
    }
    writer->writeDouble(40, ent->scale);
    writer->writeInt16(70, ent->justification);
    writer->writeInt16(71, ent->openClosed);
    writer->writeInt16(72, static_cast<int>(ent->vertlist.size()));
    writer->writeInt16(73, static_cast<int>(ent->numLines));
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    for (const auto& v : ent->vertlist) {
        writer->writeDouble(11, v.position.x);
        writer->writeDouble(21, v.position.y);
        writer->writeDouble(31, v.position.z);
        writer->writeDouble(12, v.vertexDir.x);
        writer->writeDouble(22, v.vertexDir.y);
        writer->writeDouble(32, v.vertexDir.z);
        writer->writeDouble(13, v.miterDir.x);
        writer->writeDouble(23, v.miterDir.y);
        writer->writeDouble(33, v.miterDir.z);
        for (int li = 0; li < ent->numLines; ++li) {
            const auto& seg = (li < static_cast<int>(v.segParms.size()))
                                  ? v.segParms[li] : std::vector<double>{};
            const auto& fill = (li < static_cast<int>(v.areaFillParms.size()))
                                   ? v.areaFillParms[li] : std::vector<double>{};
            writer->writeInt16(74, static_cast<int>(seg.size()));
            for (double p : seg) writer->writeDouble(41, p);
            writer->writeInt16(75, static_cast<int>(fill.size()));
            for (double p : fill) writer->writeDouble(42, p);
        }
    }
    if (!ent->extData.empty()) {
        writeExtData(ent->extData);
    }
    return true;
}

// fixme - sand - rework to shorter form
bool dxfRW::writeUnderlay(DRW_Underlay *ent) {
    if (version <= DRW::AC1009) return true;  // R13+ only
    const char* tag = (ent->kind == DRW_Underlay::DGN) ? "DGNUNDERLAY"
                    : (ent->kind == DRW_Underlay::DWF) ? "DWFUNDERLAY"
                    : "PDFUNDERLAY";
    writer->writeString(0, tag);
    writeEntity(ent);
    writer->writeString(100, "AcDbUnderlayReference");
    if (ent->definitionHandle != 0) {
        writer->writeString(340, toHexStr(static_cast<int>(ent->definitionHandle)));
    }
    writer->writeDouble(10, ent->position.x);
    writer->writeDouble(20, ent->position.y);
    writer->writeDouble(30, ent->position.z);
    if (ent->scale.x != 1.0 || ent->scale.y != 1.0 || ent->scale.z != 1.0) {
        writer->writeDouble(41, ent->scale.x);
        writer->writeDouble(42, ent->scale.y);
        writer->writeDouble(43, ent->scale.z);
    }
    writer->writeDouble(50, ent->rotation);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    writer->writeInt16(280, ent->flags);
    writer->writeInt16(281, ent->contrast);
    writer->writeInt16(282, ent->fade);
    for (const auto& v : ent->clipBoundary) {
        writer->writeDouble(11, v.x);
        writer->writeDouble(21, v.y);
    }
    if (!ent->extData.empty()) {
        writeExtData(ent->extData);
    }
    return true;
}

bool dxfRW::writeMText(DRW_MText *ent){
    if (afterAC1009) {
        writeName("MTEXT");
        writeEntity(ent);
        writeSubClass("MText");
        writeCoord(10, ent->basePoint);
        writeDouble(40, ent->height);
        writeDouble(41, ent->widthscale);
        writeInt16(71, ent->textgen);
        writeInt16(72, ent->alignH);
        std::string text = writer->fromUtf8String(ent->text);

        int i;
        for(i =0; (text.size()-i) > 250; ) {
            writeString(3, text.substr(i, 250));
            i +=250;
        }
        writeString(1, text.substr(i));
        writeString(7, ent->style);
        writeCoord(210, ent->extPoint);
        writeDouble(50, ent->angle);
        writeInt16(73, ent->alignV);
        writeDouble(44, ent->interlin);
//RLZ ... 11, 21, 31 needed?
        if (!ent->extData.empty()) {
            writeExtData(ent->extData);
        }
    } else {
        //RLZ: TODO convert mtext in text lines (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeViewport(DRW_Viewport *ent) {
    writeName( "VIEWPORT");
    writeEntity(ent);
    writeSubClassOpt("Viewport");

    writeCoord(10, ent->basePoint);
    writeDouble(40, ent->pswidth);
    writeDouble(41, ent->psheight);
    writeInt16(68, ent->vpstatus);
    writeInt16(69, ent->vpID);
    writeDouble(12, ent->centerPX);//RLZ: verify if exist in V12
    writeDouble(22, ent->centerPY);//RLZ: verify if exist in V12
    return true;
}

DRW_ImageDef* dxfRW::writeImage(DRW_Image *ent, std::string name){
    if (afterAC1009) {
        //search if exist imagedef with this mane (image inserted more than 1 time)
        //RLZ: imagedef_reactor seem needed to read in acad
        DRW_ImageDef *id = nullptr;
        size_t size = imageDef.size();
        for (unsigned int i=0; i<size; i++) {
            auto image_def = imageDef.at(i);
            if (image_def->name == name ) {
                id = image_def;
                continue; // fixme sand - why it is so?
            }
        }
        if (id == nullptr) {
            id = new DRW_ImageDef();
            imageDef.push_back(id);
            id->handle = ++entCount;
        }
        id->name = name;
        std::string idReactor = toHexStr(++entCount);

        writeName("IMAGE");
        writeEntity(ent);
        writeSubClass("RasterImage");
        writeCoord(10, ent->basePoint);
        writeCoord(11, ent->secPoint);
        writeCoord(12, ent->vVector);
        writeDouble(13, ent->sizeu);
        writeDouble(23, ent->sizev);
        writeString(340, toHexStr(id->handle));
        writeInt16(70, 1);
        writeInt16(280, ent->clip);
        writeInt16(281, ent->brightness);
        writeInt16(282, ent->contrast);
        writeInt16(283, ent->fade);
        writeString(360, idReactor);
        id->reactors[idReactor] = toHexStr(ent->handle);
        return id;
    }
    return nullptr; //not exist in acad 12
}


// fixme - sand - to shorter form
// MULTILEADER DXF write.  Mirrors the entity-level field set captured by
// DRW_MLeader::parseCode.  The CONTEXT_DATA{} block is NOT emitted yet —
// a full faithful round-trip requires walking all roots/leader-lines
// with their control-flow markers (302/304 open, 305/303/301 close);
// follow-up.  For now the entity is written as a recognisable
// AcDbMLeader stub plus its scalar fields; consumers that read it back
// see all the override flags + style fields preserved.
bool dxfRW::writeMultiLeader(DRW_MLeader *ent){
    if (version <= DRW::AC1009) {
        return false;  // not in ACAD R12 / earlier
    }
    writer->writeString(0, "MULTILEADER");
    writeEntity(ent);
    writer->writeString(100, "AcDbMLeader");
    writer->writeInt32(90, ent->overrideFlags);
    writer->writeInt16(170, ent->leaderType);
    writer->writeInt32(91, ent->leaderColor);
    writer->writeInt32(171, ent->leaderLineWeight);
    writer->writeBool(290, ent->landingEnabled);
    writer->writeBool(291, ent->doglegEnabled);
    writer->writeDouble(41, ent->landingDistance);
    writer->writeDouble(42, ent->defaultArrowHeadSize);
    writer->writeInt16(172, ent->styleContentType);
    writer->writeInt16(173, ent->styleLeftAttach);
    writer->writeInt16(95, ent->styleRightAttach);
    writer->writeInt16(174, ent->styleTextAngleType);
    writer->writeInt16(175, ent->unknown175);
    writer->writeInt32(92, ent->styleTextColor);
    writer->writeBool(292, ent->styleTextFrameEnabled);
    writer->writeInt32(93, ent->styleBlockColor);
    writer->writeDouble(43, ent->styleBlockRotation);
    writer->writeInt16(176, ent->styleAttachmentType);
    writer->writeBool(293, ent->isAnnotative);
    writer->writeBool(294, ent->isTextDirectionNegative);
    writer->writeInt16(178, ent->ipeAlign);
    writer->writeInt16(179, ent->justification);
    writer->writeDouble(45, ent->scaleFactor);
    writer->writeInt16(271, ent->attachmentDirection);
    writer->writeInt16(273, ent->styleTopAttach);
    writer->writeInt16(272, ent->styleBottomAttach);
    writer->writeBool(295, ent->leaderExtendedToText);
    return true;
}

bool dxfRW::writeWipeout(DRW_Image *ent){
    // WIPEOUT inherits AcDbRasterImage's group codes plus an AcDbWipeout
    // subclass marker carrying the polygon (91 + 14/24) and frame flag (290).
    // No AcDbRasterImageDef is written: WIPEOUT carries no actual raster.
    if (version <= DRW::AC1009) {
        return false; // not in ACAD R12 / earlier
    }
    writer->writeString(0, "WIPEOUT");
    writeEntity(ent);
    writer->writeString(100, "AcDbRasterImage");
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->vVector.x);
    writer->writeDouble(22, ent->vVector.y);
    writer->writeDouble(32, ent->vVector.z);
    writer->writeDouble(13, ent->sizeu);
    writer->writeDouble(23, ent->sizev);
    writer->writeInt16(70, 1);             // image-display flags
    writer->writeInt16(280, ent->clip);    // 1 = clipping enabled
    writer->writeInt16(281, ent->brightness);
    writer->writeInt16(282, ent->contrast);
    writer->writeInt16(283, ent->fade);
    writer->writeString(100, "AcDbWipeout");
    writer->writeInt32(90, 0);             // class version
    writer->writeInt32(91, static_cast<dint32>(ent->clipPath.size()));
    for (const DRW_Coord& v : ent->clipPath) {
        writer->writeDouble(14, v.x);
        writer->writeDouble(24, v.y);
    }
    // Group 290 is the R2010+ Clip mode (0 = mask outside, 1 = mask inside);
    // this is shared with IMAGE and is NOT a frame-display flag.  WIPEOUTFRAME
    // (whether the polygon outline is drawn) is global, in WIPEOUTVARIABLES.
    writer->writeBool(290, ent->clipMode);
    return true;
}

bool dxfRW::writeBlockRecord(std::string name){
    if (afterAC1009) {
        writeName("BLOCK_RECORD");
        writeString(5, toHexStr(++entCount)); // Handle (all except DIMSTYLE)

        m_writingContext.blockMap[name] = entCount;
        entCount = 2+entCount;//reserve 2 for BLOCK & ENDBLOCK
        if (afterAC1014) {
            writeString(330, "1"); // Soft-pointer ID/handle to owner object
        }
        writeSymTypeRecord("Block");
        writeUtf8String(2, name); //Block name
        if (afterAC1018) {
            //    writeInt16(340, 22); //  Hard-pointer ID/handle to associated LAYOUT object
            writeInt16(70, 0); // Block insertion units.
            writeInt16(280, 1); // Block explodability
            writeInt16(281, 0); // Block scalability
        }
    }
    return true;
}

bool dxfRW::writeBlock(DRW_Block *bk){
    if (writingBlock) {
        writeName("ENDBLK");
        if (afterAC1009) {
            writeString(5, toHexStr(currHandle+2));
            if (afterAC1014) {
                writeString(330, toHexStr(currHandle));
            }
            writeSubClass("Entity");
        }
        writeString(8, "0");
        writeSubClassOpt("BlockEnd");
    }
    writingBlock = true;
    writeName("BLOCK");
    if (afterAC1009) {
        currHandle = (*(m_writingContext.blockMap.find(bk->name))).second;
        writeString(5, toHexStr(currHandle+1));
        if (afterAC1014) {
            writeString(330, toHexStr(currHandle));
        }
        writeSubClass("Entity");
    }
    writeString(8, "0");
    if (afterAC1009) {
        writeSubClass("BlockBegin");
        writeUtf8String(2, bk->name);
    } else {
        writeUtf8Caps(2, bk->name);
    }
    writeInt16(70, bk->flags);
    writeCoord(10, bk->basePoint);

    if (afterAC1009) {
        writeUtf8String(3, bk->name);
    }
    else {
        writeUtf8Caps(3, bk->name);
    }
    if(afterAC1014) {
        writeAppData(bk->appData);
    }
    writeString(1, "");
    return true;
}

void dxfRW::writeViewPortTable() {
    writeTableStart("VPORT", "8", 1);
    dimstyleStd =false;  // fixme - sand - review this..
    iface->writeVports();
    if (!dimstyleStd) {
        DRW_Vport portact;
        portact.name = "*ACTIVE";
        writeVport(&portact);
    }
    writeTableEnd();
}

void dxfRW::writeLayerTable() {
    writeTableStart("LAYER", "2", 1);
    wlayer0 = false;
    iface->writeLayers();
    if (!wlayer0) {
        DRW_Layer lay0;
        lay0.name = "0";
        writeLayer(&lay0);
    }
    writeTableEnd();
}

void dxfRW::writeLineTypeTable() {
    writeTableStart("LTYPE", "5", 4);
    //Mandatory linetypes
    DRW_LType lt;
    lt.reset();
    lt.name = "ByBlock";
    writeLineTypeGenerics(&lt, 20);
    writeInt16(70, 0);
    writeString(3, "");
    writeInt16(72, 65);
    writeInt16(73, 0);
    writeDouble(40, 0.0);

    lt.name = "ByLayer";
    writeLineTypeGenerics(&lt, 21);

    writeInt16(70, 0);
    writeString(3, "");
    writeInt16(72, 65);
    writeInt16(73, 0);
    writeDouble(40, 0.0);

    lt.name = "Continuous";
    writeLineTypeGenerics(&lt, 22);

    writeInt16(70, 0);
    writeString(3, "Solid line");
    writeInt16(72, 65);
    writeInt16(73, 0);
    writeDouble(40, 0.0);
    //Application linetypes
    iface->writeLTypes();
    writeTableEnd();
}

void dxfRW::writeStyleTable() {
    writeTableStart("STYLE", "3", 3);
    dimstyleStd =false;
    iface->writeTextstyles();
    if (!dimstyleStd) {
        DRW_Textstyle tsty;
        tsty.name = "Standard";
        writeTextstyle(&tsty);
    }
    writeTableEnd();
}

// fixme - review
void dxfRW::writeUCSTable() {
    // writeTableStart("UCS", "7", 0);
    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, "7");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    iface->writeUCSs();
    writeTableEnd();
}

// fixme - review
void dxfRW::writeViewTable() {
    // writeTableStart("VIEW", "6", 0);
    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, "6");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    iface->writeViews();
    writeTableEnd();
}


// fixme - sand - compare and review this
void dxfRW::writeAppIdTable() {
    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "9");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "12");
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
    }

    writeString(2, "ACAD");
    writeInt16(70, 0);
    iface->writeAppId();
    writeTableEnd();
}

void dxfRW::writeBlockRecordTable() {
    if (afterAC1009) {
        writeTableName("BLOCK_RECORD");
        writeString(5, "1");
        if (afterAC1014) {
            writeString(330, "0");
        }
        writeSymTable();
        writeInt16(70, 2); //end table def
        writeName("BLOCK_RECORD");
        writeString(5, "1F");
        if (afterAC1014) {
            writeString(330, "1");
        }
        writeSymTypeRecord("Block");
        writeString(2, "*Model_Space");
        if (afterAC1018) {
            //    writeInt16(340, 22);
            writeInt16(70, 0);
            writeInt16(280, 1);
            writeInt16(281, 0);
        }
        writeName("BLOCK_RECORD");
        writeString(5, "1E");
        if (afterAC1014) {
            writeString(330, "1");
        }

        writeSymTypeRecord("Block");
        writeString(2, "*Paper_Space");
        if (afterAC1018) {
            //    writeInt16(340, 22);
            writeInt16(70, 0);
            writeInt16(280, 1);
            writeInt16(281, 0);
        }
    }
    /* always call writeBlockRecords to iface for prepare unnamed blocks */
    iface->writeBlockRecords();
    if (afterAC1009) {
        writeTableEnd();
    }
}

void dxfRW::writeDimStyleTable() {
    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "A");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    if (version > DRW::AC1014) {
        writer->writeString(100, "AcDbDimStyleTable");
        writer->writeInt16(71, 1); //end table def
    }
    dimstyleStd =false;
    iface->writeDimstyles();
    if (!dimstyleStd) {
        DRW_Dimstyle dsty;
        dsty.name = "Standard";
        writeDimstyle(&dsty);
    }
    writer->writeString(0, "ENDTAB");
}

bool dxfRW::writeTables() {
    writeViewPortTable();
    writeLineTypeTable();
    writeLayerTable();
    writeStyleTable();
    writeUCSTable();
    writeViewTable();
    writeAppIdTable();
    writeBlockRecordTable();
    writeDimStyleTable();
    return true;
}

bool dxfRW::writeBlocks() {
    writeName("BLOCK");
    if (afterAC1009) {
        writeString(5, "20");
        if (afterAC1014) {
            writeString(330, "1F");
        }
        writeSubClass("Entity");
    }
    writeString(8, "0");
    if (afterAC1009) {
        writeSubClass("BlockBegin");
        writeString(2, "*Model_Space");
    } else
        writeString(2, "$MODEL_SPACE");
    writeInt16(70, 0);
    // fixme - sand - use zero
    writeDouble(10, 0.0);
    writeDouble(20, 0.0);
    writeDouble(30, 0.0);
    if (afterAC1009)
        writeString(3, "*Model_Space");
    else
        writeString(3, "$MODEL_SPACE");
    writeString(1, "");
    writeName("ENDBLK");
    if (afterAC1009) {
        writeString(5, "21");
        if (afterAC1014) {
            writeString(330, "1F");
        }
        writeSubClass("Entity");
    }
    writeString(8, "0");
    if (afterAC1009)
        writeSubClass("BlockEnd");

    writeName("BLOCK");
    if (afterAC1009) {
        writeString(5, "1C");
        if (afterAC1014) {
            writeString(330, "1B");
        }
        writeSubClass("Entity");
    }
    writeString(8, "0");
    if (afterAC1009) {
        writeSubClass("BlockBegin");
        writeString(2, "*Paper_Space");
    } else
        writeString(2, "$PAPER_SPACE");
    writeInt16(70, 0);
    writeDouble(10, 0.0);
    writeDouble(20, 0.0);
    writeDouble(30, 0.0);
    if (afterAC1009)
        writeString(3, "*Paper_Space");
    else
        writeString(3, "$PAPER_SPACE");
    writeString(1, "");
    writeName( "ENDBLK");
    if (afterAC1009) {
        writeString(5, "1D");
        if (afterAC1014) {
            writeString(330, "1F");
        }
        writeSubClass("Entity");
    }
    writeString(8, "0");
    if (afterAC1009)
        writeSubClass("BlockEnd");
    writingBlock = false;
    iface->writeBlocks();
    if (writingBlock) {
        writingBlock = false;
        writeName( "ENDBLK");
        if (afterAC1009) {
            writeString(5, toHexStr(currHandle+2));
//            writeString(5, "1D");
            if (afterAC1014) {
                writeString(330, toHexStr(currHandle));
            }
            writeSubClass("Entity");
        }
        writeString(8, "0");
        if (afterAC1009)
            writeSubClass("BlockEnd");
    }
    return true;
}

bool dxfRW::writeObjects() {
    writeName("DICTIONARY");
    std::string imgDictH;
    writeString(5, "C");
    if (afterAC1014) {
        writeString(330, "0");
    }
    writeSubClass("Dictionary");
    writeInt16(281, 1);
    writeString(3, "ACAD_GROUP");
    writeString(350, "D");
    if (!imageDef.empty()) {
        writeString(3, "ACAD_IMAGE_DICT");
        imgDictH = toHexStr(++entCount);
        writeString(350, imgDictH);
    }
    writeName("DICTIONARY");
    writeString(5, "D");
    writeString(330, "C");
    writeSubClass("Dictionary");
    writeInt16(281, 1);
//write IMAGEDEF_REACTOR
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writeName( "IMAGEDEF_REACTOR");
            writeString(5, (*it).first);
            writeString(330, (*it).second);
            writeSubClass("RasterImageDefReactor");
            writeInt16(90, 2); //version 2=R14 to v2010
            writeString(330, (*it).second);
        }
    }
    if (!imageDef.empty()) {
        writeName("DICTIONARY");
        writeString(5, imgDictH);
        writeString(330, "C");
        writeSubClass("Dictionary");
        writeInt16(281, 1);
        for (unsigned int i=0; i<imageDef.size(); i++) {
            size_t f1, f2;
            f1 = imageDef.at(i)->name.find_last_of("/\\");
            f2 =imageDef.at(i)->name.find_last_of('.');
            ++f1;
            writeString(3, imageDef.at(i)->name.substr(f1,f2-f1));
            writeString(350, toHexStr(imageDef.at(i)->handle) );
        }
    }
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        writeName("IMAGEDEF");
        writeString(5, toHexStr(id->handle) );
        if (afterAC1014) {
//            writeString(330, "0"); handle to DICTIONARY
        }
        writeString(102, "{ACAD_REACTORS");
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writeString(330, (*it).first);
        }
        writeString(102, "}");
        writeSubClass("RasterImageDef");
        writeInt16(90, 0); //version 0=R14 to v2010
        writeUtf8String(1, id->name);
        writeDouble(10, id->u);
        writeDouble(20, id->v);
        writeDouble(11, id->up);
        writeDouble(21, id->vp);
        writeInt16(280, id->loaded);
        writeInt16(281, id->resolution);
    }
    //no more needed imageDef, delete it
    while (!imageDef.empty()) {
       imageDef.pop_back();
    }

    iface->writeObjects();

    return true;
}

bool dxfRW::writeExtData( // fixme - this is version from master!
    const std::vector<std::shared_ptr<DRW_Variant>> &ed) {
    // Re-pack as raw pointers so we share the existing implementation. The
    // raw pointers do not own — same lifetime as the shared_ptrs in @p ed.
    std::vector<DRW_Variant*> raw;
    raw.reserve(ed.size());
    for (const auto &sp : ed) {
        if (sp) raw.push_back(sp.get());
    }
    return writeExtData(raw);
}

bool dxfRW::writeExtData(const std::vector<DRW_Variant*> &ed){
    for (auto it=ed.begin(); it!=ed.end(); ++it){
        switch ((*it)->code()) {
            case 1000:
            case 1001:
            case 1002:
            case 1003:
            case 1005: {
                int cc = (*it)->code();
                if ((*it)->type() == DRW_Variant::STRING)
                    writeUtf8String(cc, *(*it)->content.s);
                //            writeUtf8String((*it)->code, (*it)->content.s);
                break;
            }
            case 1004: {
                // DXF code 1004 is binary chunk data; emitted as a hex-encoded
                // string. Both BINARY (from DWG path) and STRING (from a DXF
                // round-trip that already hex-encoded the bytes) variants are
                // accepted.
                if ((*it)->type() == DRW_Variant::BINARY) {
                    const std::vector<duint8>* bytes = (*it)->binary();
                    std::string hex;
                    if (bytes != nullptr) {
                        static const char hexDigits[] = "0123456789ABCDEF";
                        hex.reserve(bytes->size() * 2);
                        for (duint8 b : *bytes) {
                            hex.push_back(hexDigits[(b >> 4) & 0xF]);
                            hex.push_back(hexDigits[b & 0xF]);
                        }
                    }
                    writer->writeUtf8String(1004, hex);
                } else if ((*it)->type() == DRW_Variant::STRING) {
                    writer->writeUtf8String(1004, *(*it)->content.s);
                }
                break;
            }
            case 1010:
            case 1011:
            case 1012:
            case 1013: {
                if ((*it)->type() == DRW_Variant::COORD) {
                    writeDouble((*it)->code(), (*it)->content.v->x);
                    writeDouble((*it)->code() + 10, (*it)->content.v->y);
                    writeDouble((*it)->code() + 20, (*it)->content.v->z);
                }
                break;
            }
            case 1040:
            case 1041:
            case 1042: {
                if ((*it)->type() == DRW_Variant::DOUBLE) {
                    writeDouble((*it)->code(), (*it)->content.d);
                }
                break;
            }
            case 1070: {
                if ((*it)->type() == DRW_Variant::INTEGER) {
                    writeInt16((*it)->code(), (*it)->content.i);
                }
                break;
            }
            case 1071: {
                if ((*it)->type() == DRW_Variant::INTEGER) {
                    writeInt32((*it)->code(), (*it)->content.i);
                }
                break;
            }
            default:
                break;
        }
    }
    return true;
}

/********* Reader Process *********/

bool dxfRW::processDxf() {
    DRW_DBG("dxfRW::processDxf() start processing dxf\n");
    int code {-1};
    bool inSection {false};

    reader->setIgnoreComments( false);
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" code\n");
        /* at this level we should only get:
         999 - Comment
         0 - SECTION or EOF
         2 - section name
         everything else between "2 - section name" and "0 - ENDSEC" is handled in process() methods
        */
        switch (code) {
            case 999: {
                // when DXF was created by libdxfrw, first record is a comment with dxfrw version info
                header.addComment(getString());
                continue;
            }
            case 0: {
                // ignore further comments, as libdxfrw doesn't support comments in sections
                reader->setIgnoreComments(true);
                if (!inSection) {
                    std::string sectionstr{getString()};

                    if ("SECTION" == sectionstr) {
                        DRW_DBG(sectionstr);
                        DRW_DBG(" new section\n");
                        inSection = true;
                        continue;
                    }
                    if ("EOF" == sectionstr) {
                        return true; //found EOF terminate
                    }
                }
                else {
                    // in case SECTION was unknown or not supported
                    if ("ENDSEC" == getString()) {
                        inSection = false;
                    }
                }
                break;
            }
            case 2: {
                if (inSection) {
                    bool processed{false};
                    std::string sectionname{getString()};

                    DRW_DBG(sectionname);
                    DRW_DBG(" process section\n");
                    if ("HEADER" == sectionname) {
                        processed = processHeader();
                    }
                    else if ("TABLES" == sectionname) {
                        processed = processTables();
                    }
                    else if ("BLOCKS" == sectionname) {
                        processed = processBlocks();
                    }
                    else if ("ENTITIES" == sectionname) {
                        processed = processEntities(false);
                    }
                    else if ("OBJECTS" == sectionname) {
                        processed = processObjects();
                    }
                    else {
                        //TODO handle CLASSES
                        DRW_DBG(sectionname);
                        DRW_DBG(" section unknown or not supported\n");
                        continue;
                    }

                    if (!processed) {
                        DRW_DBG("  failed\n");
                        return setError(DRW::BAD_READ_SECTION);
                    }

                    inSection = false;
                }
                continue;
            }
            default:
                // landing here means an unknown or not supported SECTION
                inSection = false;
                break;
        }
    }

    if (0 == code && "EOF" == getString()) {
        // in case the final EOF has no newline we end up here!
        // this is caused by filestr->good() which is false for missing newline on EOF
        return true;
    }

    return setError(DRW::BAD_UNKNOWN);
}

/********* Header Section *********/

bool dxfRW::processHeader() {
    DRW_DBG("dxfRW::processHeader\n");
    int code;
    std::string sectionstr;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" processHeader\n");
        if (code == 0) {
            sectionstr = getString();
            DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
            if (sectionstr == "ENDSEC") {
                iface->addHeader(&header);
                return true;  //found ENDSEC terminate
            }

            DRW_DBG("unexpected 0 code in header!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        if (!header.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_HEADER);
}

/********* Tables Section *********/

bool dxfRW::processTables() {
    DRW_DBG("dxfRW::processTables\n");
    int code;
    std::string sectionstr;
    bool more = true;

    std::vector<DRW_Dimstyle> dimstyles;


    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            sectionstr = getString();
            DRW_DBG(sectionstr); DRW_DBG(" processTables\n\n");
            if (sectionstr == "TABLE") {
                more = readRec(&code);
                DRW_DBG(code); DRW_DBG("\n");
                if (!more) {
                    return setError(DRW::BAD_READ_TABLES); //wrong dxf file
                }
                if (code == 2) {
                    sectionstr = getString();
                    DRW_DBG(sectionstr); DRW_DBG(" processTables\n\n");
                //found section, process it
                    if (sectionstr == "LTYPE") {
                        processLType();
                    } else if (sectionstr == "LAYER") {
                        processLayer();
                    } else if (sectionstr == "STYLE") {
                        processTextStyle();
                    } else if (sectionstr == "VPORT") {
                        processVports();
                    } else if (sectionstr == "VIEW") {
                       processView();
                    } else if (sectionstr == "UCS") {
                        processUCS();
                    } else if (sectionstr == "APPID") {
                        processAppId();
                    } else if (sectionstr == "DIMSTYLE") {
                        processDimStyle(dimstyles);
                    } else if (sectionstr == "BLOCK_RECORD") {
                        processBlockRecord();
                    }
                }
            } else if (sectionstr == "ENDSEC") {
                for (auto it=dimstyles.begin(); it!=dimstyles.end(); ++it) {
                    it->resolveRefs(m_readingContext);
                    iface->addDimStyle(*it);
                }

                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processBlockRecord() {
    DRW_DBG("dxfRW::processBlockRecord");
    DRW_Block_Record blockRecord;
    return doProcessTableEntry("BLOCK_RECORD", blockRecord, [this](DRW_TableEntry* r){
        auto br = static_cast<DRW_Block_Record*>(r);
        duint32 handle = br->handle;
        m_readingContext.blockRecordMap[handle] = br;
    }, false);
}

bool dxfRW::processUCS(){
    DRW_DBG("dxfRW::processUCS\n");
    DRW_UCS ucs;
    return doProcessTableEntry("UCS", ucs, [this](DRW_TableEntry* e){
       auto ent = static_cast<DRW_UCS*>(e);
       iface->addUCS(*ent);
   });
}

bool dxfRW::processView() {
    DRW_DBG("dxfRW::processView\n");
    DRW_View view;
    return doProcessTableEntry("VIEW", view, [this](DRW_TableEntry* e){
       auto ent = static_cast<DRW_View*>(e);
       iface->addView(*ent);
   });
}

bool dxfRW::processLType() {
    DRW_DBG("dxfRW::processLType\n");
    DRW_LType ltype;
    return doProcessTableEntry("LTYPE", ltype, [this](DRW_TableEntry* l){
        auto ltp = static_cast<DRW_LType*>(l);
        ltp->update();
        int handle = ltp->handle;
        m_readingContext.lineTypeMap[handle] = ltp;
        iface->addLType(*ltp);
    }, false);
}

bool dxfRW::processLayer() {
    DRW_DBG("dxfRW::processLayer\n");
    DRW_Layer layer;
    return doProcessTableEntry("LAYER", layer, [this](DRW_TableEntry* l){
        auto layer = static_cast<DRW_Layer*>(l);
        iface->addLayer(*layer);
    });
}

bool dxfRW::processDimStyle(std::vector<DRW_Dimstyle>& styles) {
    DRW_DBG("dxfRW::processDimStyle");
    DRW_Dimstyle dimSty;
    return doProcessTableEntry("DIMSTYLE", dimSty, [&styles](DRW_TableEntry* e){
       auto ent = static_cast<DRW_Dimstyle*>(e);
       styles.push_back(*ent);
   }, false);
}

bool dxfRW::doProcessTableEntry(const std::string &sectionName, DRW_TableEntry& startEntry, DRW_TableEntryFunc applyFunc,
    bool reuseEntity) {
    DRW_DBG("dxfRW::processLayer\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_TableEntry* entry;
    if (reuseEntity) {
        entry = &startEntry;
    }
    else {
        entry = startEntry.newInstance();
    }
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading) {
                applyFunc(entry);
            }
            sectionstr = getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == sectionName) {
                reading = true;
                if (reuseEntity) {
                    entry->reset();
                }
                else {
                    entry = entry->newInstance();
                }
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!entry->parseCode(code, reader)) {
                 if (!reuseEntity) {
                     delete entry;
                 }
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processTextStyle(){
    DRW_DBG("dxfRW::processTextStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    auto* textStyle = new DRW_Textstyle;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading) {
                m_readingContext.textStyles[textStyle->handle] = textStyle;
                iface->addTextStyle(*textStyle);
            }
            sectionstr = getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "STYLE") {
                reading = true;
                textStyle = new DRW_Textstyle();
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading) {
            if (!textStyle->parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processVports(){
    DRW_DBG("dxfRW::processVports");
    DRW_Vport vp;
    return doProcessTableEntry("VPORT", vp, [this](DRW_TableEntry* e){
      auto ent = static_cast<DRW_Vport*>(e);
      iface->addVport(*ent);
    });
}

bool dxfRW::processAppId(){
    DRW_DBG("dxfRW::processAppId");
    DRW_AppId vp;
    return doProcessTableEntry("APPID", vp, [this](DRW_TableEntry* e){
     auto ent = static_cast<DRW_AppId*>(e);
     iface->addAppId(*ent);
   });
}

/********* Block Section *********/

bool dxfRW::processBlocks() {
    DRW_DBG("dxfRW::processBlocks\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (sectionstr == "BLOCK") {
                processBlock();
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_BLOCKS);
}

bool dxfRW::processBlock() {
    DRW_DBG("dxfRW::processBlock");
    DRW_Block block;
    return doProcessParseable(block, [this](DRW_ParseableEntity* e)
    {
        auto ent = static_cast<DRW_Block*>(e);
        iface->addBlock(*ent);
            if (nextentity == "ENDBLK") {  //found ENDBLK, terminate
                iface->endBlock();
            } else {
                processEntities(true);
                iface->endBlock();
            }
    },DRW::BAD_READ_BLOCKS);
}

/********* Entities Section *********/

bool dxfRW::processEntities(bool isblock) {
    DRW_DBG("dxfRW::processEntities\n");
    int code;
    if (!readRec(&code)){
        return setError(DRW::BAD_READ_ENTITIES);
    }

    if (code == 0) {
        nextentity = getString();
    } else if (!isblock) {
        return setError(DRW::BAD_READ_ENTITIES);  //first record in entities is 0
    }

    bool processed {false};
    do {
        if (nextentity == "ENDSEC" || nextentity == "ENDBLK") {
            return true;  //found ENDSEC or ENDBLK terminate
        }
        if (nextentity == "LINE") {
            processed = processLine();
        }  else if (nextentity == "CIRCLE") {
            processed = processCircle();
        } else if (nextentity == "ARC") {
            processed = processArc();
        } else if (nextentity == "INSERT") {
            processed = processInsert();
        } else if (nextentity == "LWPOLYLINE") {
            processed = processLWPolyline();
        } else if (nextentity == "POLYLINE") {
            processed = processPolyline();
        } else if (nextentity == "TEXT") {
            processed = processText();
        } else if (nextentity == "MTEXT") {
            processed = processMText();
        } else if (nextentity == "DIMENSION") {
            processed = processDimension();
        } else if (nextentity == "MLINE") {
            processed = processMLine();
        } else if (nextentity == "PDFUNDERLAY"
                   || nextentity == "DGNUNDERLAY"
                   || nextentity == "DWFUNDERLAY") {
            processed = processUnderlay(nextentity);
        } else if (nextentity == "HATCH") {
            processed = processHatch();
        } else if (nextentity == "TOLERANCE") {
            processed = processTolerance();
        } else if (nextentity == "SOLID") {
            processed = processSolid();
        }
        else if (nextentity == "SPLINE") {
            processed = processSpline();
        }else if (nextentity == "LEADER") {
            processed = processLeader();
        } else if (nextentity == "ELLIPSE") {
            processed = processEllipse();
        } else if (nextentity == "VIEWPORT") {
            processed = processViewport();
        } else if (nextentity == "IMAGE") {
            processed = processImage();
        } else if (nextentity == "POINT") {
            processed = processPoint();
        } else if (nextentity == "XLINE") {
            processed = processXline();
        } else if (nextentity == "WIPEOUT") {
            processed = processWipeout();
        } else if (nextentity == "MULTILEADER") {
            processed = processMultiLeader();
        } else if (nextentity == "3DFACE") {
            processed = process3dface();
        } else if (nextentity == "RAY") {
            processed = processRay();
        } else if (nextentity == "TRACE") {
            processed = processTrace();
        }
        else if (nextentity == "ARC_DIMENSION") {
            processed = processArcDimension();
        } else {
            if (!readRec(&code)) {
                return setError(DRW::BAD_READ_ENTITIES); //end of file without ENDSEC
            }
            if (code == 0) {
                nextentity = getString();
            }
            processed = true;
        }
    } while (processed);

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::doProcessEntity(DRW_Entity& ent, DRW_EntityFunc applyFunc) {
    int code;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (applyExt) {
                ent.applyExtrusion();
            }
            applyFunc(&ent);
            return true;  //found new entity or ENDSEC, terminate
        }
        if (!ent.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::doProcessParseable(DRW_ParseableEntity &ent, DRW_ParseableFunc applyFunc, const DRW::error sectionError) {
    int code;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            applyFunc(&ent);
            return true;  //found new entity or ENDSEC, terminate
        }
        if (!ent.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(sectionError);
}

bool dxfRW::processEllipse() {
    DRW_DBG("dxfRW::processEllipse");
    DRW_Ellipse ellipse;
    return doProcessEntity(ellipse, [this](DRW_Entity* e){
        auto ent = static_cast<DRW_Ellipse*>(e);
        if (applyExt)
            ent->applyExtrusion();
        iface->addEllipse(*ent);
    });
}

bool dxfRW::processTrace() {
    DRW_DBG("dxfRW::processTrace");
    DRW_Trace trace;
    return doProcessEntity(trace,[this](DRW_Entity* e){
        auto ent = static_cast<DRW_Trace*>(e);
        if (applyExt) {
            ent->applyExtrusion();
        }
        iface->addTrace(*ent);
    });
}

bool dxfRW::processSolid() {
    DRW_DBG("dxfRW::processSolid");
    DRW_Solid solid;
    return doProcessEntity(solid,[this](DRW_Entity* e){
        auto ent = static_cast<DRW_Solid*>(e);
        if (applyExt) {
            ent->applyExtrusion();
        }
        iface->addSolid(*ent);
    });
}

bool dxfRW::process3dface() {
    DRW_DBG("dxfRW::process3dface");
    DRW_3Dface face;
    return doProcessParseable(face,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_3Dface*>(e);
        iface->add3dFace(*ent);
    });
}

bool dxfRW::processViewport() {
    DRW_DBG("dxfRW::processViewport");
    DRW_Viewport vp;
    return doProcessParseable(vp,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Viewport*>(e);
        iface->addViewport(*ent);
    });
}

bool dxfRW::processPoint() {
    DRW_DBG("dxfRW::processPoint\n");
    DRW_Point point;
    return doProcessParseable(point,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Point*>(e);
        iface->addPoint(*ent);
    });
}

bool dxfRW::processLine() {
    DRW_DBG("dxfRW::processLine\n");
    DRW_Line line;
    return doProcessParseable(line, [this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Line*>(e);
        iface->addLine(*ent);
    });
}
// fixme- sand - rework to DRW_ParseableEntity
bool dxfRW::processMLine() {
    DRW_DBG("dxfRW::processMLine\n");
    int code;
    DRW_MLine mline;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLine(&mline);
            return true;
        }
        if (!mline.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processUnderlay(const std::string& kind) {
    DRW_DBG("dxfRW::processUnderlay\n");
    int code;
    DRW_Underlay u;
    if (kind == "DGNUNDERLAY") u.kind = DRW_Underlay::DGN;
    else if (kind == "DWFUNDERLAY") u.kind = DRW_Underlay::DWF;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addUnderlay(&u);
            return true;
        }
        if (!u.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processRay() {
    DRW_DBG("dxfRW::processRay\n");
    DRW_Ray line;
    return doProcessParseable(line,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Ray*>(e);
        iface->addRay(*ent);
    });
}

bool dxfRW::processXline() {
    DRW_DBG("dxfRW::processXline\n");
    DRW_Xline line;
    return doProcessParseable(line,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Xline*>(e);
        iface->addXline(*ent);
    });
}

bool dxfRW::processCircle() {
    DRW_DBG("dxfRW::processPoint\n");
    DRW_Circle circle;
    return doProcessEntity(circle, [this](DRW_Entity* e){
        auto ent = static_cast<DRW_Circle*>(e);
        if (applyExt) {
            ent->applyExtrusion();
        }
        iface->addCircle(*ent);
    });
}

bool dxfRW::processArc() {
    DRW_DBG("dxfRW::processPoint\n");
    DRW_Arc arc;
    return doProcessEntity(arc, [this](DRW_Entity* e){
        auto ent = static_cast<DRW_Arc*>(e);
        if (applyExt) {
            ent->applyExtrusion();
        }
        iface->addArc(*ent);
    });
}

bool dxfRW::processInsert() {
    DRW_DBG("dxfRW::processInsert");
    DRW_Insert insert;
    return doProcessParseable(insert,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Insert*>(e);
       iface->addInsert(*ent);
   });
}

bool dxfRW::processLWPolyline() {
    DRW_DBG("dxfRW::processLWPolyline");
    DRW_LWPolyline pl;
    return doProcessEntity(pl, [this](DRW_Entity* e){
        auto ent = static_cast<DRW_LWPolyline*>(e);
        if (applyExt) {
            ent->applyExtrusion();
        }
        iface->addLWPolyline(*ent);
    });
}

bool dxfRW::processPolyline() {
    DRW_DBG("dxfRW::processPolyline");
    int code;
    DRW_Polyline pl;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity != "VERTEX") {
                iface->addPolyline(pl);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processVertex(&pl)) {
                return false;
            }
        }

        if (!pl.parseCode(code, reader)) { //parseCode just initialize the members of pl
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processVertex(DRW_Polyline *pl) {
    DRW_DBG("dxfRW::processVertex");
    int code;
    auto v = std::make_shared<DRW_Vertex>();
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if(0 == code)  {
            pl->appendVertex(v);
            nextentity = getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (nextentity == "SEQEND") {
                return true;  //found SEQEND no more vertex, terminate
            }
            if (nextentity == "VERTEX"){
                v = std::make_shared<DRW_Vertex>(); //another vertex
            }
        }

        if (!v->parseCode(code, reader)) { //the members of v are reinitialized here
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTolerance() {
    DRW_DBG("dxfRW::processTolerance\n");
    DRW_Tolerance tol;
    return doProcessParseable(tol,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Tolerance*>(e);
       iface->addTolerance(*ent);
   });
}

bool dxfRW::processText() {
    DRW_DBG("dxfRW::processText");
    DRW_Text txt;
    return doProcessParseable(txt,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Text*>(e);
       iface->addText(*ent);
   });
}

bool dxfRW::processMText() {
    DRW_DBG("dxfRW::processMText");
    DRW_MText txt;
    return doProcessParseable(txt,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_MText*>(e);
       ent->updateAngle();
       iface->addMText(*ent);
   });
}

bool dxfRW::processHatch() {
    DRW_DBG("dxfRW::processHatch");
    DRW_Hatch hatch;
    return doProcessParseable(hatch,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Hatch*>(e);
       iface->addHatch(ent);
   });
}

bool dxfRW::processSpline() {
    DRW_DBG("dxfRW::processSpline");
    DRW_Spline sp;
    return doProcessParseable(sp,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Spline*>(e);
       iface->addSpline(ent);
   });
}

bool dxfRW::processImage() {
    DRW_DBG("dxfRW::processImage");
    DRW_Image img;
    return doProcessParseable(img,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Image*>(e);
       iface->addImage(ent);
   });
}

// MULTILEADER DXF read.  Captures the entity-level scalar fields via
// DRW_MLeader::parseCode.  Nested CONTEXT_DATA{} / LEADER{} / LEADER_LINE{}
// blocks use control-flow group codes (300/302/304 open + 301/303/305 close)
// — Phase 8 keeps the body capture minimal; Phase 9 / follow-up will wire
// the full nested-block state machine.
bool dxfRW::processMultiLeader() {
    DRW_DBG("dxfRW::processMultiLeader");
    int code;
    DRW_MLeader e;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLeader(&e);
            return true;
        }
        if (!e.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArcDimension() {
    DRW_DBG("dxfRW::processArcDimension");
    int code;
    DRW_ArcDimension dim;
    while (readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            // fixme - sand - restore ARCDimension
            // iface->addArcDimension(&dim);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!dim.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

// fixme - sand - rework to DRW_ParseableEntity

// MULTILEADER DXF read.  Captures the entity-level scalar fields via
// DRW_MLeader::parseCode.  Nested CONTEXT_DATA{} / LEADER{} / LEADER_LINE{}
// blocks use control-flow group codes (300/302/304 open + 301/303/305 close)
// — Phase 8 keeps the body capture minimal; Phase 9 / follow-up will wire
// the full nested-block state machine.
bool dxfRW::processMultiLeader() {
    DRW_DBG("dxfRW::processMultiLeader");
    int code;
    DRW_MLeader e;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLeader(&e);
            return true;
        }
        if (!e.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}
bool dxfRW::processWipeout() {
    // WIPEOUT shares DRW_Image's group codes (subclass marker AcDbRasterImage)
    // plus AcDbWipeout-specific codes 91/14/24/290 already handled by
    // DRW_Image::parseCode.  Differs from processImage only in the callback.
    DRW_DBG("dxfRW::processWipeout");
    int code;
    DRW_Image img;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            nextentity = reader->getString();
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addWipeout(&img);
            return true;
        }

        if (!img.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processDimension() {
    DRW_DBG("dxfRW::processDimension");
    DRW_Dimension dim;
    return doProcessParseable(dim,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_Dimension*>(e);
        int type = ent->type & 0x0F;
        switch (type) {
            case 0: {
                DRW_DimLinear d(*ent);
                iface->addDimLinear(&d);
                break;
            }
            case 1: {
                DRW_DimAligned d(*ent);
                iface->addDimAlign(&d);
                break;
            }
            case 2: {
                DRW_DimAngular d(*ent);
                iface->addDimAngular(&d);
                break;
            }
            case 3: {
                DRW_DimDiametric d(*ent);
                iface->addDimDiametric(&d);
                break;
            }
            case 4: {
                DRW_DimRadial d(*ent);
                iface->addDimRadial(&d);
                break;
            }
            case 5: {
                DRW_DimAngular3p d(*ent);
                iface->addDimAngular3P(&d);
                break;
            }
            case 6: {
                DRW_DimOrdinate d(*ent);
                iface->addDimOrdinate(&d);
                break;
            }
            default:
                break;
        }
    });
}

bool dxfRW::processLeader() {
    DRW_DBG("dxfRW::processLeader");
    DRW_Leader leader;
    return doProcessParseable(leader,[this](DRW_ParseableEntity* e){
        auto ent = static_cast<DRW_Leader*>(e);
        iface->addLeader(ent);
        return true;
    });
}

/********* Objects Section *********/

bool dxfRW::processObjects() {
    DRW_DBG("dxfRW::processObjects\n");
    int code;
    if (!readRec(&code) || 0 != code){
        return setError(DRW::BAD_READ_OBJECTS); //first record in objects must be 0
    }

    bool processed {false};
    nextentity = getString();
    do {
        if ("ENDSEC" == nextentity) {
            return true;  //found ENDSEC terminate
        }
        if ("IMAGEDEF" == nextentity) {
            processed = processImageDef();
        }
        else if ("PLOTSETTINGS" == nextentity) {
            processed = processPlotSettings();
        }
        else {
            if (!readRec(&code)) {
                return setError(DRW::BAD_READ_OBJECTS); //end of file without ENDSEC
            }
            if (code == 0) {
                nextentity = getString();
            }
            processed = true;
        }
    }
    while (processed);

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processImageDef() {
    DRW_DBG("dxfRW::processImageDef");
    DRW_ImageDef img;
    return doProcessParseable(img,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_ImageDef*>(e);
       iface->linkImage(ent);
    });
}

bool dxfRW::processPlotSettings() {
    DRW_DBG("dxfRW::processPlotSettings");
    DRW_PlotSettings ps;
    return doProcessParseable(ps,[this](DRW_ParseableEntity* e){
       auto ent = static_cast<DRW_PlotSettings*>(e);
       iface->addPlotSettings(ent);
    });
}

bool dxfRW::writePlotSettings(DRW_PlotSettings *ent) {
    writeName("PLOTSETTINGS");
    // writeUtf8String(4, ent->plotViewName);
    writeString(5, toHexStr(++entCount));
    writeSubClass("AcDbPlotSettings");
    writeUtf8String(6, ent->plotViewName);
    writeUtf8String(7, ent->currentStyleName);
    writeDouble(40, ent->marginLeftMM);
    writeDouble(41, ent->marginBottomMM);
    writeDouble(42, ent->marginRightMM);
    writeDouble(43, ent->marginTopMM);
    writeDouble(44, ent->paperWidthMM);
    writeDouble(45, ent->paperHeightMM);
    writeDouble(46, ent->originOffsetXMM);
    writeDouble(47, ent->originOffsetYMM);
    writeDouble(48, ent->plotWindowLowerLeftX);
    writeDouble(49, ent->plotWindowLowerLeftY);
    writeDouble(140, ent->plotWindowUpperRightX);
    writeDouble(141, ent->plotWindowUpperRightY);
    writeDouble(142, ent->customPrintScalePaperUnitsNumerator);
    writeDouble(143, ent->customPrintScaleDrawingUnitsDenominator);
    writeInt32(70, ent->plotLayoutFlag);
    writeInt32(72, ent->plotPaperUnits);
    writeInt32(73, ent->plotRotation);
    writeInt32(74, ent->plotType);
    writeInt32(75, ent->standardScaleType);
    writeInt32(76, ent->shadePlotMode);
    writeInt32(77, ent->shadePlotResolutionMode);
    writeInt32(78, ent->shadePlotCustomDPI);
    writeDouble(147, ent->standardScaleFactor);
    writeDouble(148, ent->paperImageOriginX);
    writeDouble(149, ent->paperImageOriginY);
    return true;
}

/** utility function
 * convert a int to string in hex
 **/
std::string dxfRW::toHexStr(int n){
#if defined(__APPLE__)
    char buffer[9]= {'\0'};
    snprintf(buffer,9, "%X", n);
    return std::string(buffer);
#else
    std::ostringstream Convert;
    Convert << std::uppercase << std::hex << n;
    return Convert.str();
#endif
}

int dxfRW::getBlockRecordHandleToWrite(const std::string &blockName) const {
    // fixme - sand - so far, the exact match of block it expected!
    // check whether case insensitive search is needed. If it is, the code should be like below...
    // std::string txstyname = blockName;
    // std::transform(txstyname.begin(), txstyname.end(), txstyname.begin(),::toupper);
    // if(blockMap.count(blockName) > 0) {
    //    auto pair = blockMap.find(blockName);

    if(m_writingContext.blockMap.count(blockName) > 0) {
        auto pair = m_writingContext.blockMap.find(blockName);
        int blkHandle = pair->second;
        return blkHandle;
    }
    return -1;
}

int dxfRW::getTextStyleHandle(const std::string &styleName) const {
    if (!styleName.empty()) {
        auto name = styleName;
        std::transform(name.begin(), name.end(), name.begin(), toupper);
        if(m_writingContext.textStyleMap.count(name) > 0) {
            auto pair = m_writingContext.textStyleMap.find(name);
            int blkHandle = pair->second;
            return blkHandle;
        }
    }
    return -1;
}

DRW::Version dxfRW::getVersion() const {
    return version;
}

DRW::error dxfRW::getError() const{
    return error;
}

// fixme - sand - add more informative errors collecting...
bool dxfRW::setError(const DRW::error lastError){
    error = lastError;
    return (DRW::BAD_NONE == error);
}

void dxfRW::setVersion(DRW::Version v) {
    version = v;
    afterAC1009 = v > DRW::AC1009;
    afterAC1012 = v > DRW::AC1012;
    afterAC1014 = v > DRW::AC1014;
    afterAC1015 = v > DRW::AC1015;
    afterAC1018 = v > DRW::AC1018;
}

bool dxfRW::writeString(int code, const std::string &text) const {
    return writer->writeString(code, text);
}

bool dxfRW::writeDouble(int code, double d) const {
    return writer->writeDouble(code, d);
}

bool dxfRW::writeDoubleOpt(int code, double d) const {
    if (d != 0.0) {
        return writer->writeDouble(code, d);
    }
    return true;
}

bool dxfRW::writeUtf8String(int code, const std::string &text) const {
    return writer->writeUtf8String(code, text);
}

bool dxfRW::writeUtf8Caps(int code, const std::string &text) const {
    return writer->writeUtf8Caps(code, text);
}

bool dxfRW::writeHandle(int code, int handle) const {
    return writer->writeString(code, toHexStr(handle));
}

bool dxfRW::writeInt16(int code, int val) const {
    return writer->writeInt16(code, val);
}

bool dxfRW::writeInt32(int code, int val) const {
    return writer->writeInt32(code, val);
}

bool dxfRW::writeBool(int code, bool val) const {
    return writer->writeBool(code, val);
}

bool dxfRW::readRec(int* codeData) const {
    return reader->readRec(codeData);
}

std::string dxfRW::getString() const {
    return reader->getString();
}

void dxfRW::writeSectionStart(const std::string &name) {
    writeString(0, "SECTION");
    writeString(2, name);
}

void dxfRW::writeSectionEnd() {
    writeString(0, "ENDSEC");
}

void dxfRW::writeSymTypeRecord(const std::string &typeName) {
    writeString(100, "AcDbSymbolTableRecord");
    writeString(100, "AcDb"+typeName+"TableRecord");
}

void dxfRW::writeSubClass(const std::string &typeName) {
    writeString(100, "AcDb"+typeName);
}

void dxfRW::writeSubClassOpt(const std::string& name) {
    if (afterAC1009) {
        writeSubClass(name);
    }
}

void dxfRW::writeTableStart(const std::string &name, const std::string handle, int maxEntriesNumber, int handleCode) {
    writeTableName(name);
    if (afterAC1009) {
        writeString(handleCode, handle);
        if (afterAC1014) {
            writeString(330, "0");
        }
        writeSymTable();
    }
    writeInt16(70, maxEntriesNumber); //end table def
}

void dxfRW::writeTableName(const std::string &name) {
    writeString(0, "TABLE");
    writeString(2, name);
}

void dxfRW::writeName(const std::string &name) {
    writeString(0, name);
}

void dxfRW::writeTableEnd() {
    writeString(0, "ENDTAB");
}
void dxfRW::writeSymTable() {
    writeString(100, "AcDbSymbolTable");
}

void dxfRW::writeCoord(int startCode, const DRW_Coord& coord) {
    writeDouble(startCode, coord.x);
    writeDouble(startCode + 10, coord.y);
    writeDoubleOpt(startCode + 20, coord.z);
}
// code 40
void dxfRW::writeVar( const std::string &name, double defaultValue, int varCode) {
    double varDouble;
    writeString(9, name);
    if (header.getDouble(name, &varDouble)) {
        writeDouble(varCode, varDouble);
    }
    else {
        writeDouble(varCode, defaultValue);
    }
}

// varcode 70
void dxfRW::writeVar( const std::string &name, int defaultValue, int varCode) {
    int varInt;
    writeString(9, name);
    if (header.getInt(name, &varInt)) {
        writeInt16(varCode, varInt);
    }
    else {
        writeInt16(varCode, defaultValue);
    }
}

void dxfRW::writeVarExp( const std::string &name, int value, int varCode) {
    writeString(9, name);
    writeInt16(varCode, value);
}

void dxfRW::writeVarOpt(const std::string& name, int varCode) {
    int varInt;
    if (header.getInt(name, &varInt)) {
        writeString(9, name);
        writeInt16(varCode, varInt);
    }
}

// varcode 1
void dxfRW::writeVar(const std::string &name, const  std::string &defaultValue, int varCode) {
    std::string varStr;
    writeString(9, name);
    if (header.getStr(name, &varStr)) {
        if (version == DRW::AC1009) {
            writeUtf8Caps(varCode, varStr);
        }
        else {
            writeUtf8String(varCode, varStr);
        }
    }
    else {
        writeString(varCode, defaultValue);
    }
}

void dxfRW::writeVar(const std::string& name, int startCode,const DRW_Coord& defaultCoord) {
    writer->writeString(9, name);
    DRW_Coord varCoord;
    if (header.getCoord(name, &varCoord)) {
        writer->writeDouble(startCode, varCoord.x);
        writer->writeDouble(startCode + 10, varCoord.y);
        writer->writeDouble(startCode + 20, varCoord.z);
    } else {
        writer->writeDouble(startCode, defaultCoord.x);
        writer->writeDouble(startCode + 10,defaultCoord.y);
        writer->writeDouble(startCode + 20,defaultCoord.z);
    }
}

void dxfRW::writeVar2D(const std::string& name, int startCode, const DRW_Coord& defaultCoord) {
    writer->writeString(9, name);
    DRW_Coord varCoord;
    if (header.getCoord(name, &varCoord)) {
        writer->writeDouble(startCode, varCoord.x);
        writer->writeDouble(startCode + 10, varCoord.y);
    } else {
        writer->writeDouble(startCode, defaultCoord.x);
        writer->writeDouble(startCode + 10,defaultCoord.y);
    }
}

void dxfRW::writeVar2DOpt(const std::string& name, int startCode) {
    DRW_Coord varCoord;
    if (header.getCoord(name, &varCoord)) {
        writer->writeString(9, name);
        writer->writeDouble(startCode, varCoord.x);
        writer->writeDouble(startCode + 10, varCoord.y);
    }
}
