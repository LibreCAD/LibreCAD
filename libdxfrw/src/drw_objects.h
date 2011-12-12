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

class dxfReader;

using std::string;

namespace DRW {

//! Table entries type.
     enum TTYPE {
         UNKNOWNT,
         LTYPE,
         LAYER,
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
    string name;                         /*!< entry name, code 2 */
    int flags;                               /*!< Flags relevant to entry, code 70 */
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
    string desc;           /*!< descriptive string, code 3 */
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
        lineType = "BYLAYER";
        color = 256; // default BYLAYER (256)
        plotF = true; // default TRUE (plot yes)
        lWeight = -1; // default BYLAYER (-1)
    }

    void parseCode(int code, dxfReader *reader);

public:
    string lineType;           /*!< line type, code 6 */
    int color;                 /*!< entity color, code 62 */
    bool plotF;                 /*!< Plot flag, code 290 */
    int lWeight;               /*!< entity lineweight, code 370 */
    string handlePlotS;        /*!< Hard-pointer ID/handle of plotstyle, code 390 */
    string handlePlotM;        /*!< Hard-pointer ID/handle of materialstyle, code 347 */
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

    void parseCode(int code, dxfReader *reader);

public:
    string version;
    string codepage;
private:
    string name;
    string data;
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

