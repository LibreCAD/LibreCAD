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
        isccw = 1;
    }

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

//! Class to handle vertex
/*!
*  Class to handle vertex for lwpolyline entity
*  @author Rallaz
*/
class DRW_Vertex2D {
public:
    DRW_Vertex2D() {
//        eType = DRW::LWPOLYLINE;
        stawidth = endwidth = bulge = 0;
    }
    DRW_Vertex2D(double sx, double sy, double b) {
        stawidth = endwidth = 0;
        x = sx;
        y =sy;
        bulge = b;
    }

//    void parseCode(int code, dxfReader *reader);

public:
    double x;                 /*!< x coordinate, code 10 */
    double y;                 /*!< y coordinate, code 20 */
//    double z;                 /*!< z coordinate, code 30 */
    double stawidth;          /*!< Start width, code 40 */
    double endwidth;          /*!< End width, code 41 */
    double bulge;             /*!< bulge, code 42 */
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
        width = ex = ey = 0;
        ez = 1;
        elevation = flags = 0;
        vertex = NULL;
    }
    ~DRW_LWPolyline() {
        while (!vertlist.empty()) {
           vertlist.pop_back();
         }
    }
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
    double ex;                /*!< x extrusion coordinate, code 210 */
    double ey;                /*!< y extrusion coordinate, code 220 */
    double ez;                /*!< z extrusion coordinate, code 230 */
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
/*        angle = 0;
        widthscale = 1;
        oblique = 0;
        style = "STANDARD";
        textgen = 0;
        alignH = DRW::HAlignLeft;*/
        alignV = (DRW::VAlign)2;
    }

    void parseCode(int code, dxfReader *reader);

public:
//    double height;           /*!< height text, code 40 */
//    string text;                 /*!< text string, code 1 */
//    double angle;             /*!< rotation angle, code 50 */
//    double widthscale;     /*!< width factor, code 41 */
//    double oblique;          /*!< oblique angle, code 51 */
//    string style;                /*!< stile name, code 7 */
//    int textgen;                 /*!< text generation, code 71 */
//    enum DRW::HAlign alignH;   /*!< horizontal align, code 72 */
//    enum DRW::VAlign alignV;    /*!< vertical align, code 73 */
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
        flags = identifier = z = 0;
    }
    DRW_Vertex(double sx, double sy, double sz, double b) {
        stawidth = endwidth = 0;
        vindex1 = vindex2 = vindex3 = vindex4 = 0;
        flags = identifier = 0;
        x = sx;
        y =sy;
        z =sz;
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
        curvetype = ex = ey = 0;
        ez = 1;
        vertexcount = facecount = 0;
        smoothM = smoothN = 0;
//        vertex = NULL;
    }
    ~DRW_Polyline() {
        while (!vertlist.empty()) {
           vertlist.pop_back();
         }
    }
    void addVertex (DRW_Vertex v) {
        DRW_Vertex *vert = new DRW_Vertex();
        vert->x = v.x;
        vert->y = v.y;
        vert->z = v.z;
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
    int flags;                     /*!< polyline flag, code 70, default 0 */
    double defstawidth;   /*!< Start width, code 40, default 0 */
    double defendwidth;  /*!< End width, code 41, default 0 */
    int vertexcount;          /*!< polygon mesh M vertex or  polyface vertex num, code 71, default 0 */
    int facecount;             /*!< polygon mesh N vertex or  polyface face num, code 72, default 0 */
    int smoothM;             /*!< smooth surface M density, code 73, default 0 */
    int smoothN;             /*!< smooth surface M density, code 74, default 0 */
    int curvetype;            /*!< curves & smooth surface type, code 75, default 0 */

//    DRW_Vertex *vertex;       /*!< current vertex to add data */
    std::vector<DRW_Vertex *> vertlist;  /*!< vertex list */
};

//! Class to handle spline point
/*!
*  Class to handle spline point
*  @author Rallaz
*/
class DRW_SpPoint {
public:
    DRW_SpPoint() {
        z = 0;
    }
    ~DRW_SpPoint() {}

//    void parseCode(int code, dxfReader *reader);

public:
    double x;
    double y;
    double z;
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
    std::vector<DRW_SpPoint *> controllist;  /*!< control points list, code 10, 20 & 30 */
    std::vector<DRW_SpPoint *> fitlist;      /*!< fit points list, code 11, 21 & 31 */

private:
    DRW_SpPoint *controlpoint;   /*!< current control point to add data */
    DRW_SpPoint *fitpoint;       /*!< current fit point to add data */
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
        hstyle = ex = ey = x = y = 0;
        ez = solid = hpattern = 1;
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


//! Base class for dimension entity
/*!
*  Base class for dimension entity
*  @author Rallaz
*/
class DRW_Dimension : public DRW_Line {
public:
    DRW_Dimension() {
        eType = DRW::DIMENSION;
        linesty = linefactor = 1;
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

};


//! Class to handle  aligned, linear or rotated dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*  @author Rallaz
*/
class DRW_DimLinear : public DRW_Dimension {
public:
    DRW_DimLinear() {
//Note: the type is defined in code 100 (AcDbAlignedDimension only: DIMALIGNED
//        AcDbAlignedDimension and AcDbRotatedDimension: DIMLINEAR
        eType = DRW::DIMALIGNED;
//        eType = DRW::DIMLINEAR;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double dx1;                /*!< Definition point 1, x coordinate, code 13 */
    double dy1;                /*!< Definition point 1, y coordinate, code 23 */
    double dz1;                /*!< Definition point 1, z coordinate, code 33 */
    double dx2;                /*!< Definition point 2, x coordinate, code 14 */
    double dy2;                /*!< Definition point 2, y coordinate, code 24 */
    double dz2;                /*!< Definition point 2, z coordinate, code 34 */
    double angle;              /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
    double rotline;            /*!< oblique angle, code 52 */
};

//! Class to handle radial dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*  @author Rallaz
*/
class DRW_DimRadial : public DRW_Dimension {
public:
    DRW_DimRadial() {
        //Note: the type is defined in code 100 (AcDbRadialDimension: DIMRADIAL
        //                                    AcDbDiametricDimension: DIMDIAMETRIC
        eType = DRW::DIMRADIAL;
//        eType = DRW::DIMDIAMETRIC;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double cenx;                /*!< Definition point for diameter, x coordinate, code 15 */
    double ceny;                /*!< Definition point for diameter, code 25 */
    double cenz;                /*!< Definition point for diameter, code 35 */
    double length;            /*!< Leader length, code 40 */
};

//! Class to handle angular dimension entity
/*!
*  Class to handle angular dimension entity
*  @author Rallaz
*/
class DRW_DimAngular : public DRW_DimLinear {
public:
    DRW_DimAngular() {
        eType = DRW::DIMANGULAR;
    }

    void parseCode(int code, dxfReader *reader);

public:
    double cenx;                /*!< Definition point for center, x coordinate, code 15 */
    double ceny;                /*!< Definition point for center, code 25 */
    double cenz;                /*!< Definition point for center, code 35 */
    double arcx;                /*!< Point defining dimension arc, x coordinate, code 16 */
    double arcy;                /*!< Point defining dimension arc, code 26 */
    double arcz;                /*!< Point defining dimension arc, code 36 */

};



#endif

// EOF

