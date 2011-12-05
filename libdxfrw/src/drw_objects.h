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


#endif

// EOF

