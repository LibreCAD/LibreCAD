/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_OBJECTS_H
#define DRW_OBJECTS_H


#include <string>
#include <vector>
#include <map>
#include "drw_base.h"

class dxfReader;
class dxfWriter;

using std::string;

namespace DRW {

//! Table entries type.
     enum TTYPE {
         UNKNOWNT,
         LTYPE,
         LAYER,
         STYLE,
         DIMSTYLE,
         VPORT,
         BLOCK_RECORD
     };


}

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
class DRW_TableEntry {
public:
    //initializes default values
    DRW_TableEntry() {
        tType = DRW::UNKNOWNT;
        flags = 0;
    }

protected:
    void parseCode(int code, dxfReader *reader);

public:
    enum DRW::TTYPE tType;  /*!< enum: entity type, code 0 */
    string handle;                       /*!< entity identifier, code 5 */
    string handleBlock;              /*!< Soft-pointer ID/handle to owner BLOCK_RECORD object, code 330 */
    UTF8STRING name;           /*!< entry name, code 2 */
    int flags;                               /*!< Flags relevant to entry, code 70 */
};


//! Class to handle dimstyle entries
/*!
*  Class to handle dim style symbol table entries
*  @author Rallaz
*/
class DRW_Dimstyle : public DRW_TableEntry {
public:
    DRW_Dimstyle() {
        tType = DRW::DIMSTYLE;
        dimasz = dimtxt = dimexe = 0.18;
        dimexo = 0.0625;
        dimgap = dimcen = 0.09;
        dimtxsty = "Standard";
        dimscale = dimlfac = dimtfac = 1.0;
        dimdli = 0.38;
        dimrnd = dimdle = dimtp = dimtm = dimtsz = dimtvp = 0.0;
        dimaltf = 25.4;
        dimtol = dimlim = dimse1 = dimse2 = dimtad = dimzin = 0;
        dimtih = dimtoh = dimtolj = 1;
        dimalt = dimtofl = dimsah = dimtix = dimsoxd =0;
        dimaltd = dimunit = dimaltu = dimalttd = dimlunit = 2;
        dimclrd = dimclre = dimclrt = dimjust = dimupt = 0;
        dimazin = dimaltz = dimaltttz = dimtzin = dimfrac = 0;
        dimadec = dimaunit = dimsd1 = dimsd2 = dimtmove = 0;
        dimaltrnd = 0.0;
        dimdec = dimtdec = 4;
        dimfit = dimatfit = 3;
        dimdsep = '.';
        dimlwd = dimlwe = -2;
    }

    void parseCode(int code, dxfReader *reader);

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
    UTF8STRING dimtxsty;      /*!< code 340 R13+ */
    UTF8STRING dimldrblk;     /*!< code 341 V2000+ */
    int dimlwd;               /*!< code 371 V2000+ */
    int dimlwe;               /*!< code 372 V2000+ */
};


//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*  @author Rallaz
*/
/*TODO: handle complex lineType*/
class DRW_LType : public DRW_TableEntry {
public:
    DRW_LType() {
        tType = DRW::LTYPE;
        desc = "";
        size = 0;
        length = 0.0;
        pathIdx = 0;
/*        color = 256; // default BYLAYER (256)
        plotF = true; // default TRUE (plot yes)
        lWeight = -1; // default BYLAYER (-1)*/
//        align = 65; //always 65
    }

    void parseCode(int code, dxfReader *reader);
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
public:
    DRW_Layer() {
        tType = DRW::LAYER;
        lineType = "CONTINUOUS";
        color = 7; // default BYLAYER (256)
        plotF = true; // default TRUE (plot yes)
        lWeight = -3; // default BYDEFAULT (-3)
    }

    void parseCode(int code, dxfReader *reader);

public:
    UTF8STRING lineType;           /*!< line type, code 6 */
    int color;                 /*!< layer color, code 62 */
    bool plotF;                 /*!< Plot flag, code 290 */
    int lWeight;               /*!< layer lineweight, code 370 */
    string handlePlotS;        /*!< Hard-pointer ID/handle of plotstyle, code 390 */
    string handlePlotM;        /*!< Hard-pointer ID/handle of materialstyle, code 347 */
};

//! Class to handle text style entries
/*!
*  Class to handle text style symbol table entries
*  @author Rallaz
*/
class DRW_Textstyle : public DRW_TableEntry {
public:
    DRW_Textstyle() {
        tType = DRW::STYLE;
        height = oblique = 0.0;
        width = lastHeight = 1.0;
        font="txt";
        genFlag = 0; //2= X mirror, 4= Y mirror
    }

    void parseCode(int code, dxfReader *reader);

public:
    double height;          /*!< Fixed text height (0 not set), code 40 */
    double width;           /*!< Width factor, code 41 */
    double oblique;         /*!< Oblique angle, code 50 */
    int genFlag;            /*!< Text generation flags, code 71 */
    double lastHeight;      /*!< Last height used, code 42 */
    UTF8STRING font;        /*!< primary font file name, code 3 */
    UTF8STRING bigFont;     /*!< bigfont file name or blank if none, code 4 */
    UTF8STRING fontFamily;  /*!< ttf font family, italic and bold flags, code 1071 */
};

//! Class to handle vport entries
/*!
*  Class to handle vport symbol table entries
*  @author Rallaz
*/
class DRW_Vport : public DRW_TableEntry {
public:
    DRW_Vport() {
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
    }

    void parseCode(int code, dxfReader *reader);

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
};


//! Class to handle imagedef entries
/*!
*  Class to handle image definitions object entries
*  @author Rallaz
*/
class DRW_ImageDef {
public:
    DRW_ImageDef() {
        version = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    string handle;            /*!< entity identifier, code 5 */
    UTF8STRING name;              /*!< File name of image, code 1 */
    int version;              /*!< class version, code 90, 0=R14 version */
    double u;                 /*!< image size in pixels U value, code 10 */
    double v;                 /*!< image size in pixels V value, code 20 */
    double up;                /*!< default size of one pixel U value, code 11 */
    double vp;                /*!< default size of one pixel V value, code 12 really is 21*/
    int loaded;               /*!< image is loaded flag, code 280, 0=unloaded, 1=loaded */
    int resolution;           /*!< resolution units, code 281, 0=no, 2=centimeters, 5=inch */

    std::map<string,string> reactors;
};


//! Class to handle header entries
/*!
*  Class to handle layer symbol table entries
*  @author Rallaz
*/
class DRW_Header {
public:
    DRW_Header() {
    }
    ~DRW_Header() {
        vars.clear();
    }

    void parseCode(int code, dxfReader *reader);
    void write(dxfWriter *writer, DRW::Version ver);
private:
    bool getDouble(string key, double *varDouble);
    bool getInt(string key, int *varInt);
    bool getStr(string key, string *varStr);
    bool getCoord(string key, DRW_Coord *varStr);

public:
    std::map<string,DRW_Variant*> vars;
private:
    string name;
    DRW_Variant *curr;
};

namespace DRW {

// Extended color palette:
// The first entry is only for direct indexing starting with [1]
// Color 1 is red (1,0,0)
const double dxfColors[][3] = {
                                  {0,0,0},                // unused
                                  {1,0,0},                // 1 red
                                  {1,1,0},                // 2 yellow
                                  {0,1,0},                // 3 green
                                  {0,1,1},
                                  {0,0,1},
                                  {1,0,1},
                                  {1,1,1},                // black or white
                                  {0.5,0.5,0.5},
                                  {0.75,0.75,0.75},
                                  {1,0,0},                // 10
                                  {1,0.5,0.5},
                                  {0.65,0,0},
                                  {0.65,0.325,0.325},
                                  {0.5,0,0},
                                  {0.5,0.25,0.25},
                                  {0.3,0,0},
                                  {0.3,0.15,0.15},
                                  {0.15,0,0},
                                  {0.15,0.075,0.075},
                                  {1,0.25,0},             // 20
                                  {1,0.625,0.5},
                                  {0.65,0.1625,0},
                                  {0.65,0.4063,0.325},
                                  {0.5,0.125,0},
                                  {0.5,0.3125,0.25},
                                  {0.3,0.075,0},
                                  {0.3,0.1875,0.15},
                                  {0.15,0.0375,0},
                                  {0.15,0.0938,0.075},
                                  {1,0.5,0},              // 30
                                  {1,0.75,0.5},
                                  {0.65,0.325,0},
                                  {0.65,0.4875,0.325},
                                  {0.5,0.25,0},
                                  {0.5,0.375,0.25},
                                  {0.3,0.15,0},
                                  {0.3,0.225,0.15},
                                  {0.15,0.075,0},
                                  {0.15,0.1125,0.075},
                                  {1,0.75,0},             // 40
                                  {1,0.875,0.5},
                                  {0.65,0.4875,0},
                                  {0.65,0.5688,0.325},
                                  {0.5,0.375,0},
                                  {0.5,0.4375,0.25},
                                  {0.3,0.225,0},
                                  {0.3,0.2625,0.15},
                                  {0.15,0.1125,0},
                                  {0.15,0.1313,0.075},
                                  {1,1,0},                // 50
                                  {1,1,0.5},
                                  {0.65,0.65,0},
                                  {0.65,0.65,0.325},
                                  {0.5,0.5,0},
                                  {0.5,0.5,0.25},
                                  {0.3,0.3,0},
                                  {0.3,0.3,0.15},
                                  {0.15,0.15,0},
                                  {0.15,0.15,0.075},
                                  {0.75,1,0},             // 60
                                  {0.875,1,0.5},
                                  {0.4875,0.65,0},
                                  {0.5688,0.65,0.325},
                                  {0.375,0.5,0},
                                  {0.4375,0.5,0.25},
                                  {0.225,0.3,0},
                                  {0.2625,0.3,0.15},
                                  {0.1125,0.15,0},
                                  {0.1313,0.15,0.075},
                                  {0.5,1,0},              // 70
                                  {0.75,1,0.5},
                                  {0.325,0.65,0},
                                  {0.4875,0.65,0.325},
                                  {0.25,0.5,0},
                                  {0.375,0.5,0.25},
                                  {0.15,0.3,0},
                                  {0.225,0.3,0.15},
                                  {0.075,0.15,0},
                                  {0.1125,0.15,0.075},
                                  {0.25,1,0},             // 80
                                  {0.625,1,0.5},
                                  {0.1625,0.65,0},
                                  {0.4063,0.65,0.325},
                                  {0.125,0.5,0},
                                  {0.3125,0.5,0.25},
                                  {0.075,0.3,0},
                                  {0.1875,0.3,0.15},
                                  {0.0375,0.15,0},
                                  {0.0938,0.15,0.075},
                                  {0,1,0},                // 90
                                  {0.5,1,0.5},
                                  {0,0.65,0},
                                  {0.325,0.65,0.325},
                                  {0,0.5,0},
                                  {0.25,0.5,0.25},
                                  {0,0.3,0},
                                  {0.15,0.3,0.15},
                                  {0,0.15,0},
                                  {0.075,0.15,0.075},
                                  {0,1,0.25},             // 100
                                  {0.5,1,0.625},
                                  {0,0.65,0.1625},
                                  {0.325,0.65,0.4063},
                                  {0,0.5,0.125},
                                  {0.25,0.5,0.3125},
                                  {0,0.3,0.075},
                                  {0.15,0.3,0.1875},
                                  {0,0.15,0.0375},
                                  {0.075,0.15,0.0938},
                                  {0,1,0.5},              // 110
                                  {0.5,1,0.75},
                                  {0,0.65,0.325},
                                  {0.325,0.65,0.4875},
                                  {0,0.5,0.25},
                                  {0.25,0.5,0.375},
                                  {0,0.3,0.15},
                                  {0.15,0.3,0.225},
                                  {0,0.15,0.075},
                                  {0.075,0.15,0.1125},
                                  {0,1,0.75},             // 120
                                  {0.5,1,0.875},
                                  {0,0.65,0.4875},
                                  {0.325,0.65,0.5688},
                                  {0,0.5,0.375},
                                  {0.25,0.5,0.4375},
                                  {0,0.3,0.225},
                                  {0.15,0.3,0.2625},
                                  {0,0.15,0.1125},
                                  {0.075,0.15,0.1313},
                                  {0,1,1},                // 130
                                  {0.5,1,1},
                                  {0,0.65,0.65},
                                  {0.325,0.65,0.65},
                                  {0,0.5,0.5},
                                  {0.25,0.5,0.5},
                                  {0,0.3,0.3},
                                  {0.15,0.3,0.3},
                                  {0,0.15,0.15},
                                  {0.075,0.15,0.15},
                                  {0,0.75,1},             // 140
                                  {0.5,0.875,1},
                                  {0,0.4875,0.65},
                                  {0.325,0.5688,0.65},
                                  {0,0.375,0.5},
                                  {0.25,0.4375,0.5},
                                  {0,0.225,0.3},
                                  {0.15,0.2625,0.3},
                                  {0,0.1125,0.15},
                                  {0.075,0.1313,0.15},
                                  {0,0.5,1},              // 150
                                  {0.5,0.75,1},
                                  {0,0.325,0.65},
                                  {0.325,0.4875,0.65},
                                  {0,0.25,0.5},
                                  {0.25,0.375,0.5},
                                  {0,0.15,0.3},
                                  {0.15,0.225,0.3},
                                  {0,0.075,0.15},
                                  {0.075,0.1125,0.15},
                                  {0,0.25,1},             // 160
                                  {0.5,0.625,1},
                                  {0,0.1625,0.65},
                                  {0.325,0.4063,0.65},
                                  {0,0.125,0.5},
                                  {0.25,0.3125,0.5},
                                  {0,0.075,0.3},
                                  {0.15,0.1875,0.3},
                                  {0,0.0375,0.15},
                                  {0.075,0.0938,0.15},
                                  {0,0,1},                // 170
                                  {0.5,0.5,1},
                                  {0,0,0.65},
                                  {0.325,0.325,0.65},
                                  {0,0,0.5},
                                  {0.25,0.25,0.5},
                                  {0,0,0.3},
                                  {0.15,0.15,0.3},
                                  {0,0,0.15},
                                  {0.075,0.075,0.15},
                                  {0.25,0,1},             // 180
                                  {0.625,0.5,1},
                                  {0.1625,0,0.65},
                                  {0.4063,0.325,0.65},
                                  {0.125,0,0.5},
                                  {0.3125,0.25,0.5},
                                  {0.075,0,0.3},
                                  {0.1875,0.15,0.3},
                                  {0.0375,0,0.15},
                                  {0.0938,0.075,0.15},
                                  {0.5,0,1},              // 190
                                  {0.75,0.5,1},
                                  {0.325,0,0.65},
                                  {0.4875,0.325,0.65},
                                  {0.25,0,0.5},
                                  {0.375,0.25,0.5},
                                  {0.15,0,0.3},
                                  {0.225,0.15,0.3},
                                  {0.075,0,0.15},
                                  {0.1125,0.075,0.15},
                                  {0.75,0,1},             // 200
                                  {0.875,0.5,1},
                                  {0.4875,0,0.65},
                                  {0.5688,0.325,0.65},
                                  {0.375,0,0.5},
                                  {0.4375,0.25,0.5},
                                  {0.225,0,0.3},
                                  {0.2625,0.15,0.3},
                                  {0.1125,0,0.15},
                                  {0.1313,0.075,0.15},
                                  {1,0,1},                // 210
                                  {1,0.5,1},
                                  {0.65,0,0.65},
                                  {0.65,0.325,0.65},
                                  {0.5,0,0.5},
                                  {0.5,0.25,0.5},
                                  {0.3,0,0.3},
                                  {0.3,0.15,0.3},
                                  {0.15,0,0.15},
                                  {0.15,0.075,0.15},
                                  {1,0,0.75},             // 220
                                  {1,0.5,0.875},
                                  {0.65,0,0.4875},
                                  {0.65,0.325,0.5688},
                                  {0.5,0,0.375},
                                  {0.5,0.25,0.4375},
                                  {0.3,0,0.225},
                                  {0.3,0.15,0.2625},
                                  {0.15,0,0.1125},
                                  {0.15,0.075,0.1313},
                                  {1,0,0.5},              // 230
                                  {1,0.5,0.75},
                                  {0.65,0,0.325},
                                  {0.65,0.325,0.4875},
                                  {0.5,0,0.25},
                                  {0.5,0.25,0.375},
                                  {0.3,0,0.15},
                                  {0.3,0.15,0.225},
                                  {0.15,0,0.075},
                                  {0.15,0.075,0.1125},
                                  {1,0,0.25},             // 240
                                  {1,0.5,0.625},
                                  {0.65,0,0.1625},
                                  {0.65,0.325,0.4063},
                                  {0.5,0,0.125},
                                  {0.5,0.25,0.3125},
                                  {0.3,0,0.075},
                                  {0.3,0.15,0.1875},
                                  {0.15,0,0.0375},
                                  {0.15,0.075,0.0938},
                                  {0.33,0.33,0.33},       // 250
                                  {0.464,0.464,0.464},
                                  {0.598,0.598,0.598},
                                  {0.732,0.732,0.732},
                                  {0.866,0.866,0.866},
                                  {1,1,1}                 // 255
                              } ;
}

#endif

// EOF

