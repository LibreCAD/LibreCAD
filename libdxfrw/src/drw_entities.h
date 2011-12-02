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

#ifndef DRW_ENTITIES_H
#define DRW_ENTITIES_H


#include <string>

class dxfReader;

using std::string;

namespace DRW {
//! Version numbers for the DXF Format.
enum Version {
    AC1006,       /*!< R10. */
    AC1009,       /*!< R11 & R12. */
    AC1012,       /*!< R13. */
    AC1014,       /*!< R14. */
    AC1015,       /*!< ACAD 2000. */
    AC1018,       /*!< ACAD 2004. */
    AC1021,       /*!< ACAD 2007. */
    AC1024        /*!< ACAD 2010. */
};

//! Vertical alignments.
    enum VAlign {
        VAlignTop,      /*!< Top. */
        VAlignMiddle,   /*!< Middle */
        VAlignBottom    /*!< Bottom */
    };

    //! Horizontal alignments.
    enum HAlign {
        HAlignLeft,     /*!< Left */
        HAlignCenter,   /*!< Centered */
        HAlignRight     /*!< Right */
    };


   //! Entity's type.
    enum ETYPE {
        POINT,
        LINE,
        CIRCLE,
        ARC,
        ELLIPSE,
        TRACE,
        SOLID,
        BLOCK,
        INSERT,
        POLYLINE,
        SPLINE,
        HATCH,
        TEXT,
        IMAGE,
        DIMLEADER,
        DIMALIGNED,
        DIMLINEAR,
        DIMRADIAL,
        DIMDIAMETRIC,
        DIMANGULAR,
        OVERLAYBOX,
        CONSTRUCTIONLINE,
        UNKNOWN
    };

    enum LWEIGHT {
        L0=0,
        L1,
        L2,
        L3,
        L4,
        L5,
        L6,
        L7
    };

}

//! Base class for entities
/*!
*  Base class for entities
*  @author Rallaz
*/
class DRW_Entity {
public:
    //initializes default values
    DRW_Entity() {
        eType = DRW::UNKNOWN;
        lineType = "BYLAYER";
        color = 256; // default BYLAYER (256)
        ltypeScale = 1.0;
        visible = true;
        layer = "0";
        lWeight = -1; // default BYLAYER (-1)
        space = 0; // default ModelSpace (0)
    }

protected:
    void parseCode(int code, dxfReader *reader);

public:
    enum DRW::ETYPE eType;     /*!< enum: entity type, code 0 */
    string handle;             /*!< entity identifier, code 5 */
    string handleBlock;        /*!< Soft-pointer ID/handle to owner BLOCK_RECORD object, code 330 */
    string layer;              /*!< layer name, code 8 */
    string lineType;           /*!< line type, code 6 */
    int color;                 /*!< entity color, code 62 */
    //RLZ: TODO as integer or enum??
    int lWeight;               /*!< entity lineweight, code 370 */
//    enum DRW::LWEIGHT lWeight; /*!< entity lineweight, code 370 */
    double ltypeScale;         /*!< linetype scale, code 48 */
    bool visible;              /*!< entity visibility, code 60 */
    int color24;               /*!< 24-bit color, code 420 */
    string colorName;          /*!< color name, code 430 */
    int space;                  /*!< space indicator 0 = model, 1 paper , code 67*/
};


//! Class to handle point entity
/*!
*  Class to handle point entity
*  @author Rallaz
*/
class DRW_Point : public DRW_Entity {
public:
    DRW_Point() {
        eType = DRW::POINT;
        z = ex = ey = 0;
        ez = 1;
        thickness = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double x;                 /*!< x coordinate, code 10 */
    double y;                 /*!< y coordinate, code 20 */
    double z;                 /*!< z coordinate, code 30 */
    double thickness;         /*!< thickness, code 39 */
    double ex;                /*!< x extrusion coordinate, code 210 */
    double ey;                /*!< y extrusion coordinate, code 220 */
    double ez;                /*!< z extrusion coordinate, code 230 */
};

//! Class to handle line entity
/*!
*  Class to handle line entity
*  @author Rallaz
*/
class DRW_Line : public DRW_Point {
public:
    DRW_Line() {
        eType = DRW::LINE;
        bz = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double bx;                 /*!< x coordinate, code 11 */
    double by;                 /*!< y coordinate, code 21 */
    double bz;                 /*!< z coordinate, code 31 */
};

//! Class to handle circle entity
/*!
*  Class to handle circle entity
*  @author Rallaz
*/
class DRW_Circle : public DRW_Point {
public:
    DRW_Circle() {
        eType = DRW::CIRCLE;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double radious;                 /*!< radius, code 40 */
};

//! Class to handle arc entity
/*!
*  Class to handle arc entity
*  @author Rallaz
*/
class DRW_Arc : public DRW_Circle {
public:
    DRW_Arc() {
        eType = DRW::ARC;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double staangle;               /*!< x coordinate, code 50 */
    double endangle;               /*!< x coordinate, code 51 */
};

//! Class to handle ellipse entity
/*!
*  Class to handle ellipse and elliptic arc entity
*  @author Rallaz
*/
class DRW_Ellipse : public DRW_Line {
public:
    DRW_Ellipse() {
        eType = DRW::ELLIPSE;
        bz = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double ratio;           /*!< ratio, code 40 */
    double staparam;      /*!< start parameter, code 41 */
    double endparam;     /*!< z coordinate, code 42 */
};

//! Class to handle trace entity
/*!
*  Class to handle trace entity
*  @author Rallaz
*/
class DRW_Trace : public DRW_Line {
public:
    DRW_Trace() {
        eType = DRW::TRACE;
        cz = 0;
        dz = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double cx;                 /*!< x coordinate, code 12 */
    double cy;                 /*!< y coordinate, code 22 */
    double cz;                 /*!< z coordinate, code 32 */
    double dx;                 /*!< x coordinate, code 13 */
    double dy;                 /*!< y coordinate, code 23 */
    double dz;                 /*!< z coordinate, code 33 */
};

//! Class to handle solid entity
/*!
*  Class to handle solid entity
*  @author Rallaz
*/
class DRW_Solid : public DRW_Trace {
public:
    DRW_Solid() {
        eType = DRW::SOLID;
    }

    void parseCode(int code, dxfReader *reader);
};

//! Class to handle block entries
/*!
*  Class to handle block entries
*  @author Rallaz
*/
class DRW_Block : public DRW_Point {
public:
    DRW_Block() {
        eType = DRW::BLOCK;
        layer = "0";
        flags = 0;
        name = "caca";
    }

    void parseCode(int code, dxfReader *reader);

public:
    string name;             /*!< block name, code 2 */
    int flags;                   /*!< block type, code 70 */
};


//! Class to handle insert entries
/*!
*  Class to handle insert entries
*  @author Rallaz
*/
class DRW_Insert : public DRW_Point {
public:
    DRW_Insert() {
        eType = DRW::INSERT;
        xscale = 1;
        yscale = 1;
        zscale = 1;
        angle = 0;
        colcount = 1;
        rowcount = 1;
        colspace = 0;
        rowspace = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    string name;             /*!< block name, code 2 */
    double xscale;           /*!< x scale factor, code 41 */
    double yscale;           /*!< y scale factor, code 42 */
    double zscale;           /*!< z scale factor, code 43 */
    double angle;            /*!< rotation angle, code 50 */
    int colcount;            /*!< column count, code 70 */
    int rowcount;            /*!< row count, code 71 */
    double colspace;         /*!< column space, code 44 */
    double rowspace;         /*!< row space, code 45 */
};


#endif

// EOF

