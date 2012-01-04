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
#include <vector>
#include "drw_base.h"

class dxfReader;

using std::string;

namespace DRW {

//! Vertical alignments.
    enum VAlign {
        VAlignBaseLine =0,  /*!< Top. */
        VAlignBottom,           /*!< Bottom */
        VAlignMiddle,            /*!< Middle */
        VAlignTop                  /*!< Top. */
    };

    //! Horizontal alignments.
    enum HAlign {
        HAlignLeft = 0,  /*!< Left */
        HAlignCenter,     /*!< Centered */
        HRight,               /*!< Right */
        HAligned,            /*!< Right */
        HAlignMiddle,     /*!< middle */
        HAlignFit             /*!< fit into point */
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
        LWPOLYLINE,
        POLYLINE,
        VERTEX,
        SPLINE,
        HATCH,
        TEXT,
        MTEXT,
        E3DFACE,
        IMAGE,
        LEADER,
        DIMENSION,
        DIMALIGNED,
        DIMLINEAR,
        DIMRADIAL,
        DIMDIAMETRIC,
        DIMANGULAR,
        DIMANGULAR3P,
        DIMORDINATE,
//        OVERLAYBOX,
//        CONSTRUCTIONLINE,
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
        haveExtrusion = false;
    }

    virtual void applyExtrusion() = 0;
protected:
    void parseCode(int code, dxfReader *reader);
    void calculateAxis(DRW_Coord extPoint);
    void extrudePoint(DRW_Coord extPoint, DRW_Coord *point);

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
    int space;                 /*!< space indicator 0 = model, 1 paper , code 67*/
    bool haveExtrusion;        /*!< set to true if the entity have extrusion*/
private:
    DRW_Coord extAxisX;
    DRW_Coord extAxisY;
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
        basePoint.z = extPoint.x = extPoint.y = 0;
        extPoint.z = 1;
        thickness = 0;
    }

    virtual void applyExtrusion(){}

    void parseCode(int code, dxfReader *reader);

public:
    DRW_Coord basePoint;      /*!<  base point, code 10, 20 & 30 */
    double thickness;         /*!< thickness, code 39 */
    DRW_Coord extPoint;       /*!<  Dir extrusion normal vector, code 210, 220 & 230 */
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
        secPoint.z = 0;
    }

    virtual void applyExtrusion(){}
    void parseCode(int code, dxfReader *reader);

public:
    DRW_Coord secPoint;        /*!< second point, code 11, 21 & 31 */
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

    virtual void applyExtrusion();
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
        isccw = 1;
    }

    virtual void applyExtrusion(){DRW_Circle::applyExtrusion();}
    void parseCode(int code, dxfReader *reader);

public:
    double staangle;               /*!< x coordinate, code 50 */
    double endangle;               /*!< x coordinate, code 51 */
    double isccw;                  /*!< is counter clockwise arc?, only used in hatch, code 73 */
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
        thirdPoint.z = 0;
        fourPoint.z = 0;
    }

    virtual void applyExtrusion();
    void parseCode(int code, dxfReader *reader);

public:
    DRW_Coord thirdPoint;        /*!< third point, code 12, 22 & 32 */
    DRW_Coord fourPoint;        /*!< four point, code 13, 23 & 33 */
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

//! Class to handle 3dface entity
/*!
*  Class to handle 3dface entity
*  @author Rallaz
*/
class DRW_3Dface : public DRW_Trace {
public:
    DRW_3Dface() {
        eType = DRW::E3DFACE;
        invisibleflag = 0;
    }

    virtual void applyExtrusion(){}
    void parseCode(int code, dxfReader *reader);

public:
    int invisibleflag;       /*!< invisible edge flag, code 70 */

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

    virtual void applyExtrusion(){}
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

    virtual void applyExtrusion(){DRW_Point::applyExtrusion();}
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

//! Class to handle lwpolyline entity
/*!
*  Class to handle lwpolyline entity
*  @author Rallaz
*/
class DRW_LWPolyline : public DRW_Entity {
public:
    DRW_LWPolyline() {
        eType = DRW::LWPOLYLINE;
        width = 0;
        elevation = flags = 0;
        extPoint.x = extPoint.y = 0;
        extPoint.z = 1;
        vertex = NULL;
    }
    ~DRW_LWPolyline() {
        while (!vertlist.empty()) {
           vertlist.pop_back();
         }
    }
    virtual void applyExtrusion();
    void addVertex (DRW_Vertex2D v) {
        DRW_Vertex2D *vert = new DRW_Vertex2D();
        vert->x = v.x;
        vert->y = v.y;
        vert->stawidth = v.stawidth;
        vert->endwidth = v.endwidth;
        vert->bulge = v.bulge;
        vertlist.push_back(vert);
    }
    DRW_Vertex2D *addVertex () {
        DRW_Vertex2D *vert = new DRW_Vertex2D();
        vert->stawidth = 0;
        vert->endwidth = 0;
        vert->bulge = 0;
        vertlist.push_back(vert);
        return vert;
    }

    void parseCode(int code, dxfReader *reader);

public:
    int vertexnum;            /*!< number of vertex, code 90 */
    int flags;                /*!< polyline flag, code 70, default 0 */
    double width;             /*!< constant width, code 43 */
    double elevation;         /*!< elevation, code 38 */
    DRW_Coord extPoint;       /*!<  Dir extrusion normal vector, code 210, 220 & 230 */
    DRW_Vertex2D *vertex;       /*!< current vertex to add data */
    std::vector<DRW_Vertex2D *> vertlist;  /*!< vertex list */
};

//! Class to handle insert entries
/*!
*  Class to handle insert entries
*  @author Rallaz
*/
class DRW_Text : public DRW_Line {
public:
    DRW_Text() {
        eType = DRW::TEXT;
        angle = 0;
        widthscale = 1;
        oblique = 0;
        style = "STANDARD";
        textgen = 0;
        alignH = DRW::HAlignLeft;
        alignV = DRW::VAlignBaseLine;
    }

    virtual void applyExtrusion(){} //RLZ TODO
    void parseCode(int code, dxfReader *reader);

public:
    double height;           /*!< height text, code 40 */
    string text;                 /*!< text string, code 1 */
    double angle;             /*!< rotation angle, code 50 */
    double widthscale;     /*!< width factor, code 41 */
    double oblique;          /*!< oblique angle, code 51 */
    string style;                /*!< stile name, code 7 */
    int textgen;                 /*!< text generation, code 71 */
    enum DRW::HAlign alignH;   /*!< horizontal align, code 72 */
    enum DRW::VAlign alignV;    /*!< vertical align, code 73 */
};

//! Class to handle insert entries
/*!
*  Class to handle insert entries
*  @author Rallaz
*/
class DRW_MText : public DRW_Text {
public:
    DRW_MText() {
        eType = DRW::MTEXT;
        interlin = 1;
        alignV = (DRW::VAlign)2;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double interlin;     /*!< width factor, code 44 */
};

//! Class to handle vertex
/*!
*  Class to handle vertex  for polyline entity
*  @author Rallaz
*/
class DRW_Vertex : public DRW_Point {
public:
    DRW_Vertex() {
        eType = DRW::VERTEX;
        stawidth = endwidth = bulge = 0;
        vindex1 = vindex2 = vindex3 = vindex4 = 0;
        flags = identifier = 0;
    }
    DRW_Vertex(double sx, double sy, double sz, double b) {
        stawidth = endwidth = 0;
        vindex1 = vindex2 = vindex3 = vindex4 = 0;
        flags = identifier = 0;
        basePoint.x = sx;
        basePoint.y =sy;
        basePoint.z =sz;
        bulge = b;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double stawidth;          /*!< Start width, code 40 */
    double endwidth;          /*!< End width, code 41 */
    double bulge;             /*!< bulge, code 42 */

    int flags;                 /*!< vertex flag, code 70, default 0 */
    double tgdir;           /*!< curve fit tangent direction, code 50 */
    int vindex1;             /*!< polyface mesh vertex index, code 71, default 0 */
    int vindex2;             /*!< polyface mesh vertex index, code 72, default 0 */
    int vindex3;             /*!< polyface mesh vertex index, code 73, default 0 */
    int vindex4;             /*!< polyface mesh vertex index, code 74, default 0 */
    int identifier;           /*!< vertex identifier, code 91, default 0 */
};

//! Class to handle polyline entity
/*!
*  Class to handle polyline entity
*  @author Rallaz
*/
class DRW_Polyline : public DRW_Point {
public:
    DRW_Polyline() {
        eType = DRW::POLYLINE;
        flags = defstawidth = defendwidth = 0;
        curvetype = 0;
        vertexcount = facecount = 0;
        smoothM = smoothN = 0;
    }
    ~DRW_Polyline() {
        while (!vertlist.empty()) {
           vertlist.pop_back();
         }
    }
    void addVertex (DRW_Vertex v) {
        DRW_Vertex *vert = new DRW_Vertex();
        vert->basePoint.x = v.basePoint.x;
        vert->basePoint.y = v.basePoint.y;
        vert->basePoint.z = v.basePoint.z;
        vert->stawidth = v.stawidth;
        vert->endwidth = v.endwidth;
        vert->bulge = v.bulge;
        vertlist.push_back(vert);
    }
    void appendVertex (DRW_Vertex *v) {
        vertlist.push_back(v);
    }

    void parseCode(int code, dxfReader *reader);

public:
    int flags;               /*!< polyline flag, code 70, default 0 */
    double defstawidth;      /*!< Start width, code 40, default 0 */
    double defendwidth;      /*!< End width, code 41, default 0 */
    int vertexcount;         /*!< polygon mesh M vertex or  polyface vertex num, code 71, default 0 */
    int facecount;           /*!< polygon mesh N vertex or  polyface face num, code 72, default 0 */
    int smoothM;             /*!< smooth surface M density, code 73, default 0 */
    int smoothN;             /*!< smooth surface M density, code 74, default 0 */
    int curvetype;           /*!< curves & smooth surface type, code 75, default 0 */

    std::vector<DRW_Vertex *> vertlist;  /*!< vertex list */
};


//! Class to handle spline entity
/*!
*  Class to handle spline entity
*  @author Rallaz
*/
class DRW_Spline : public DRW_Entity {
public:
    DRW_Spline() {
        eType = DRW::SPLINE;
        flags = nknots = ncontrol = 0;
        nfit = ex = ey = 0;
        ez = 1;
        tolknot = tolcontrol = tolfit = 0.0000001;

    }
    ~DRW_Spline() {
        while (!controllist.empty()) {
           controllist.pop_back();
        }
        while (!fitlist.empty()) {
           fitlist.pop_back();
        }
    }
    virtual void applyExtrusion(){}

    void parseCode(int code, dxfReader *reader);

public:
    double ex;                /*!< normal vector x coordinate, code 210 */
    double ey;                /*!< normal vector y coordinate, code 220 */
    double ez;                /*!< normal vector z coordinate, code 230 */
    double tgsx;              /*!< start tangent x coordinate, code 12 */
    double tgsy;              /*!< start tangent y coordinate, code 22 */
    double tgsz;              /*!< start tangent z coordinate, code 32 */
    double tgex;              /*!< end tangent x coordinate, code 13 */
    double tgey;              /*!< end tangent y coordinate, code 23 */
    double tgez;              /*!< end tangent z coordinate, code 33 */
    int flags;                /*!< spline flag, code 70 */
    int degree;               /*!< degree of the spline, code 71 */
    int nknots;               /*!< number of knots, code 72, default 0 */
    int ncontrol;             /*!< number of control points, code 73, default 0 */
    int nfit;                 /*!< number of fit points, code 74, default 0 */
    double tolknot;           /*!< knot tolerance, code 42, default 0.0000001 */
    double tolcontrol;        /*!< control point tolerance, code 43, default 0.0000001 */
    double tolfit;            /*!< fit point tolerance, code 44, default 0.0000001 */

    std::vector<double> knotslist;           /*!< knots list, code 40 */
    std::vector<DRW_Coord *> controllist;  /*!< control points list, code 10, 20 & 30 */
    std::vector<DRW_Coord *> fitlist;      /*!< fit points list, code 11, 21 & 31 */

private:
    DRW_Coord *controlpoint;   /*!< current control point to add data */
    DRW_Coord *fitpoint;       /*!< current fit point to add data */
};

//! Class to handle hatch loop
/*!
*  Class to handle hatch loop
*  @author Rallaz
*/
class DRW_HatchLoop {
public:
    DRW_HatchLoop(int t) {
        type = t;
        numedges = 0;
    }

    ~DRW_HatchLoop() {
/*        while (!pollist.empty()) {
           pollist.pop_back();
         }*/
        while (!objlist.empty()) {
           objlist.pop_back();
         }
    }

public:
    int type;               /*!< boundary path type, code 92, polyline=2, default=0 */
    int numedges;           /*!< number of edges (if not a polyline), code 93 */
//TODO: store lwpolylines as entities
//    std::vector<DRW_LWPolyline *> pollist;  /*!< polyline list */
    std::vector<DRW_Entity *> objlist;      /*!< entities list */
};

//! Class to handle hatch entity
/*!
*  Class to handle hatch entity
*  @author Rallaz
*/
//TODO: handle lwpolylines, splines and ellipses
class DRW_Hatch : public DRW_Point {
public:
    DRW_Hatch() {
        eType = DRW::HATCH;
        loopsnum = angle = scale = 0;
        hstyle = basePoint.x = basePoint.y = 0;
        solid = hpattern = 1;
        deflines = doubleflag = 0;
        loop = NULL;
        clearEntities();
    }

    ~DRW_Hatch() {
        while (!looplist.empty()) {
           looplist.pop_back();
         }
    }

    void appendLoop (DRW_HatchLoop *v) {
        looplist.push_back(v);
    }

    virtual void applyExtrusion(){}
    void parseCode(int code, dxfReader *reader);

public:
    string name;               /*!< hatch pattern name, code 2 */
    int solid;                 /*!< solid fill flag, code 70, solid=1, pattern=0 */
    int associative;           /*!< associativity, code 71, associatve=1, non-assoc.=0 */
    int hstyle;                /*!< hatch style, code 75 */
    int hpattern;              /*!< hatch pattern type, code 76 */
    int doubleflag;            /*!< hatch pattern double flag, code 77, double=1, single=0 */
    int loopsnum;              /*!< namber of boundary paths (loops), code 91 */
    double angle;              /*!< hatch pattern angle, code 52 */
    double scale;              /*!< hatch pattern scale, code 41 */
    int deflines;              /*!< number of pattern definition lines, code 78 */

    std::vector<DRW_HatchLoop *> looplist;  /*!< polyline list */

private:
    void clearEntities(){
        pt = line = NULL;
        pline = NULL;
        arc = NULL;
        ellipse = NULL;
        spline = NULL;
        plvert = NULL;
    }

    void addLine() {
        clearEntities();
        if (loop) {
            pt = line = new DRW_Line;
            loop->objlist.push_back(line);
        }
    }

    void addArc() {
        clearEntities();
        if (loop) {
            pt = arc = new DRW_Arc;
            loop->objlist.push_back(arc);
        }
    }

    void addEllipse() {
        clearEntities();
        if (loop) {
            pt = ellipse = new DRW_Ellipse;
            loop->objlist.push_back(ellipse);
        }
    }

    void addSpline() {
        clearEntities();
        if (loop) {
            pt = NULL;
            spline = new DRW_Spline;
            loop->objlist.push_back(spline);
        }
    }

    DRW_HatchLoop *loop;       /*!< current loop to add data */
    DRW_Line *line;
    DRW_Arc *arc;
    DRW_Ellipse *ellipse;
    DRW_Spline *spline;
    DRW_LWPolyline *pline;
    DRW_Point *pt;
    DRW_Vertex2D *plvert;
    bool ispol;
};

//! Class to handle image entity
/*!
*  Class to handle image entity
*  @author Rallaz
*/
class DRW_Image : public DRW_Line {
public:
    DRW_Image() {
        eType = DRW::IMAGE;
        vz = fade = 0;
        brightness = contrast = 50;
    }

    void parseCode(int code, dxfReader *reader);

public:
    string ref;                /*!< Hard reference to imagedef object, code 340 */
    double vx;                 /*!< V-vector of single pixel, x coordinate, code 12 */
    double vy;                 /*!< V-vector of single pixel, y coordinate, code 22 */
    double vz;                 /*!< V-vector of single pixel, z coordinate, code 32 */
    double sizeu;              /*!< image size in pixels, U value, code 13 */
    double sizev;              /*!< image size in pixels, V value, code 23 */
    double dz;                 /*!< z coordinate, code 33 */
    int clip;                  /*!< Clipping state, code 280, 0=off 1=on */
    int brightness;            /*!< Brightness value, code 281, (0-100) default 50 */
    int contrast;              /*!< Brightness value, code 282, (0-100) default 50 */
    int fade;                   /*!< Brightness value, code 283, (0-100) default 0 */

};


//! Class for parse dimension entity
/*!
*  Class for parse dimension entity
*  @author Rallaz
*/
class DRW_DimensionData : public DRW_Line {
public:
    DRW_DimensionData() {
        eType = DRW::DIMENSION;
        linesty = linefactor = 1;
        angle = oblique = 0;
    }

    void parseCode(int code, dxfReader *reader);

public:
    string name;               /*!< Name of the block that contains the entities, code 2 */
    string text;               /*!< Dimension text explicitly entered by the user, code 1 */
    string style;              /*!< Dimension style, code 3 */
    int type;                  /*!< Dimension type, code 70 */
    int align;                 /*!< attachment point, code 71 */
    int linesty;               /*!< Dimension text line spacing style, code 72, default 1 */
    double linefactor;         /*!< Dimension text line spacing factor, code 41, default 1? */
    double rot;                /*!< rotation angle of the dimension text, code 53 */
//    double hdir;               /*!< horizontal direction for the dimension, code 51, default ? */
//protected:
    DRW_Coord clonePoint;      /*!< Insertion point for clones (Baseline & Continue), code 12, 22 & 32 */
    DRW_Coord def1;            /*!< Definition point 1, code 13, 23 & 33 */
    DRW_Coord def2;            /*!< Definition point 2, code 14, 24 & 34 */
    DRW_Coord circlePoint;     /*!< Definition point for center, diameter & radius, code 15, 25 & 35 */
    DRW_Coord arcPoint;        /*!< Point defining dimension arc, x coordinate, code 16, 26 & 36 */
    double angle;              /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
    double oblique;            /*!< oblique angle, code 52 */
    double length;             /*!< Leader length, code 40 */
};


//! Base class for dimension entity
/*!
*  Base class for dimension entity
*  @author Rallaz
*/
class DRW_Dimension : public DRW_Entity {
public:
    DRW_Dimension(DRW_DimensionData d) {
        eType = DRW::DIMENSION;
        dim = d;
        handle = d.handle;
        handleBlock = d.handleBlock;
        layer = d.layer;
        lineType = d.lineType;
        color = d.color;
        lWeight = d.lWeight;
        ltypeScale = d.ltypeScale;
        visible = d.visible;
        color24 = d.color24;
        colorName = d.colorName;
        space = d.space;
    }
    virtual void applyExtrusion(){}

    DRW_Coord getTextPoint() const {return DRW_Coord(dim.secPoint.x, dim.secPoint.y, dim.secPoint.z);} /*!< Middle point of text, code 11, 21 & 31 */
    DRW_Coord getBasePoint() const {return DRW_Coord(dim.basePoint.x, dim.basePoint.y, dim.basePoint.z);}
    DRW_Coord getExtrusion(){return DRW_Coord(dim.extPoint.x, dim.extPoint.y, dim.extPoint.z);} /*!< extrusion, code 210, 220 & 230 */
    string getName(){return dim.name;}                    /*!< Name of the block that contains the entities, code 2 */
    string getText() const {return dim.text;}                    /*!< Dimension text explicitly entered by the user, code 1 */
    string getStyle() const {return dim.style;}                  /*!< Dimension style, code 3 */
    int getType(){ return dim.type;}                      /*!< Dimension type, code 70 */
    int getAlign() const { return dim.align;}                    /*!< attachment point, code 71 */
    int getTextLineStyle() const { return dim.linesty;}          /*!< Dimension text line spacing style, code 72, default 1 */
    double getTextLineFactor() const { return dim.linefactor;}   /*!< Dimension text line spacing factor, code 41, default 1? */
    double getDir() const { return dim.rot;}                     /*!< rotation angle of the dimension text, code 53 */

protected:
    DRW_Coord getBasepoint() const {return DRW_Coord(dim.basePoint.x, dim.basePoint.y, dim.basePoint.z);} /*!< Definition point, code 10, 20 & 30 */
    DRW_Coord getClonepoint() const {return dim.clonePoint;}               /*!< Insertion for clones (Baseline & Continue), 12, 22 & 32 */
    DRW_Coord getDef1point() const {return dim.def1;}                      /*!< Definition point 1, code 13, 23 & 33 */
    DRW_Coord getDef2point() const {return dim.def2;}                      /*!< Definition point 2, code 14, 24 & 34 */
    DRW_Coord getCenArcpoint() const {return dim.circlePoint;}             /*!< Definition point for center, diameter & radius, code 15, 25 & 35 */
    DRW_Coord getArcpoint() const {return dim.arcPoint;}                   /*!< Point defining dimension arc, x coordinate, code 16, 26 & 36 */
    double getangle() const {return dim.angle;}                   /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
    double getoblique() const {return dim.oblique;}                        /*!< oblique angle, code 52 */
    double getleader() const {return dim.length;}                          /*!< Leader length, code 40 */

private:
    DRW_DimensionData dim;
};


//! Class to handle  aligned dimension entity
/*!
*  Class to handle aligned dimension entity
*  @author Rallaz
*/
class DRW_DimAligned : public DRW_Dimension {
public:
    DRW_DimAligned(DRW_DimensionData d): DRW_Dimension(d){
        eType = DRW::DIMALIGNED;
    }

    DRW_Coord getDimPoint(){return getBasepoint();}        /*!< Definition point, code 10, 20 & 30 */
    DRW_Coord getClonePoint(){return getClonepoint();}     /*!< Insertion for clones (Baseline & Continue), 12, 22 & 32 */
    DRW_Coord getDef1Point() const {return getDef1point();}       /*!< Definition point 1, code 13, 23 & 33 */
    DRW_Coord getDef2Point() const {return getDef2point();}       /*!< Definition point 2, code 14, 24 & 34 */
};

//! Class to handle  linear or rotated dimension entity
/*!
*  Class to handle linear or rotated dimension entity
*  @author Rallaz
*/
class DRW_DimLinear : public DRW_DimAligned {
public:
    DRW_DimLinear(DRW_DimensionData d): DRW_DimAligned(d) {
        eType = DRW::DIMLINEAR;
    }

    double getAngle() const {return getangle();}          /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
    double getOblique() const {return getoblique();}      /*!< oblique angle, code 52 */
};

//! Class to handle radial dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*  @author Rallaz
*/
class DRW_DimRadial : public DRW_Dimension {
public:
    DRW_DimRadial(DRW_DimensionData d): DRW_Dimension(d) {
        eType = DRW::DIMRADIAL;
    }

    DRW_Coord getCenterPoint() const {return getBasepoint();}            /*!< center point, code 10, 20 & 30 */
    DRW_Coord getDiameterPoint() const {return getCenArcpoint();}      /*!< Definition point for radius, code 15, 25 & 35 */
    double getLeaderLength() const {return getleader();}                /*!< Leader length, code 40 */
};

//! Class to handle radial dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*  @author Rallaz
*/
class DRW_DimDiametric : public DRW_Dimension {
public:
    DRW_DimDiametric(DRW_DimensionData d): DRW_Dimension(d) {
        eType = DRW::DIMDIAMETRIC;
    }

    DRW_Coord getDiameter1Point() const {return getCenArcpoint();}      /*!< Definition point for diameter, code 15, 25 & 35 */
    DRW_Coord getDiameter2Point() const {return getBasepoint();}         /*!< Opposite diameter point, code 10, 20 & 30 */
    double getLeaderLength() const {return getleader();}                /*!< Leader length, code 40 */
};

//! Class to handle angular dimension entity
/*!
*  Class to handle angular dimension entity
*  @author Rallaz
*/
class DRW_DimAngular : public DRW_DimLinear {
public:
    DRW_DimAngular(DRW_DimensionData d): DRW_DimLinear(d) {
        eType = DRW::DIMANGULAR;
    }

    DRW_Coord getFirstLine1() const {return getDef1point();}         /*!< Definition point line 1-1, code 13, 23 & 33 */
    DRW_Coord getFirstLine2() const {return getDef2point();}         /*!< Definition point line 1-2, code 14, 24 & 34 */
    DRW_Coord getSecondLine1() const {return getCenArcpoint();}      /*!< Definition point line 2-1, code 15, 25 & 35 */
    DRW_Coord getSecondLine2() const {return getBasepoint();}        /*!< Definition point line 2-2, code 10, 20 & 30 */
    DRW_Coord getDimPoint() const {return getArcpoint();}            /*!< Dimension definition point, code 16, 26 & 36 */
};


//! Class to handle angular 3p dimension entity
/*!
*  Class to handle angular 3p dimension entity
*  @author Rallaz
*/
class DRW_DimAngular3p : public DRW_DimLinear {
public:
    DRW_DimAngular3p(DRW_DimensionData d): DRW_DimLinear(d) {
        eType = DRW::DIMANGULAR3P;
    }

    DRW_Coord getVertex() const {return getCenArcpoint();}          /*!< Vertex point, code 15, 25 & 35 */
    DRW_Coord getDimPoint() const {return getBasepoint();}          /*!< Dimension definition point, code 10, 20 & 30 */
    DRW_Coord getFirstLine() const {return getDef1point();}         /*!< Definition point line 1, code 13, 23 & 33 */
    DRW_Coord getSecondLine() const {return getDef2point();}        /*!< Definition point line 2, code 14, 24 & 34 */
};

//! Class to handle angular 3p dimension entity
/*!
*  Class to handle angular 3p dimension entity
*  @author Rallaz
*/
class DRW_DimOrdinate : public DRW_DimLinear {
public:
    DRW_DimOrdinate(DRW_DimensionData d): DRW_DimLinear(d) {
        eType = DRW::DIMORDINATE;
    }

    DRW_Coord getOriginPoint(){return getBasepoint();}       /*!< Origin definition point, code 10, 20 & 30 */
    DRW_Coord getFirstLine(){return getDef1point();}         /*!< Feature location point, code 13, 23 & 33 */
    DRW_Coord getSecondLine(){return getDef2point();}        /*!< Leader end point, code 14, 24 & 34 */
};


//! Class to handle leader entity
/*!
*  Class to handle leader entity
*  @author Rallaz
*/
class DRW_Leader : public DRW_Entity {
public:
    DRW_Leader() {
        eType = DRW::LEADER;
        flag = 3;
        hookflag = vertnum = 0;
        leadertype = extrusionPoint.x = extrusionPoint.y = 0;
        arrow = extrusionPoint.z = 1;
    }
    ~DRW_Leader() {
        while (!vertexlist.empty()) {
           vertexlist.pop_back();
        }
    }

    virtual void applyExtrusion(){}
    void parseCode(int code, dxfReader *reader);

public:
    string style;              /*!< Dimension style name, code 3 */
    int arrow;                 /*!< Arrowhead flag, code 71, 0=Disabled; 1=Enabled */
    int leadertype;            /*!< Leader path type, code 72, 0=Straight line segments; 1=Spline */
    int flag;                  /*!< Leader creation flag, code 73, default 3 */
    int hookline;              /*!< Hook line direction flag, code 74, default 1 */
    int hookflag;              /*!< Hook line flag, code 75 */
    double textheight;         /*!< Text annotation height, code 40 */
    double textwidth;          /*!< Text annotation width, code 41 */
    int vertnum;               /*!< Number of vertices, code 76 */
    int coloruse;              /*!< Color to use if leader's DIMCLRD = BYBLOCK, code 77 */
    string handle;             /*!< Hard reference to associated annotation, code 340 */
    DRW_Coord extrusionPoint;  /*!< Normal vector, code 210, 220 & 230 */
    DRW_Coord horizdir;        /*!< "Horizontal" direction for leader, code 211, 221 & 231 */
    DRW_Coord offsetblock;     /*!< Offset of last leader vertex from block, code 212, 222 & 232 */
    DRW_Coord offsettext;      /*!< Offset of last leader vertex from annotation, code 213, 223 & 233 */

    std::vector<DRW_Coord *> vertexlist;  /*!< vertex points list, code 10, 20 & 30 */

private:
    DRW_Coord *vertexpoint;   /*!< current control point to add data */
};


#endif

// EOF

