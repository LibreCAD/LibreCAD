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

#ifndef DRW_BASE_H
#define DRW_BASE_H


#include <string>
#include <cmath>

using std::string;


//! Class to handle 3D coordinate point
/*!
*  Class to handle 3D coordinate point
*  @author Rallaz
*/
class DRW_Coord {
public:
    DRW_Coord() { z = 0; }
    DRW_Coord(double ix, double iy, double iz) {
        x = ix; y = iy; z = iz;
    }

     DRW_Coord operator = (const DRW_Coord& data) {
        x = data.x;  y = data.y;  z = data.z;
        return *this;
    }
/*!< convert to unitary vector */
    void unitize(){
        double dist;
        dist = sqrt(x*x + y*y + z*z);
        if (dist > 0.0) {
            x= x/dist;
            y= y/dist;
            z= z/dist;
        }
    }

public:
    double x;
    double y;
    double z;
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

public:
    double x;                 /*!< x coordinate, code 10 */
    double y;                 /*!< y coordinate, code 20 */
    double stawidth;          /*!< Start width, code 40 */
    double endwidth;          /*!< End width, code 41 */
    double bulge;             /*!< bulge, code 42 */
};


//! Class to handle header vars
/*!
*  Class to handle header vars
*  @author Rallaz
*/
class DRW_Variant {
public:
    enum TYPE {
        STRING,
        INTEGER,
        DOUBLE,
        COORD,
        INVALID
    };

    DRW_Variant() {
        type = INVALID;
    }
    ~DRW_Variant() {
        if (type == COORD)
            delete content.v;
    }
    enum TYPE type;

    void addString(string s) {data = s; content.s = &data; setType(STRING);}
    void addInt(int i) {content.i = i; setType(INTEGER);}
    void addDouble(double d) {content.d = d; setType(DOUBLE);}
    void addCoord(DRW_Coord *v) {content.v = v; setType(COORD);}
    void setType(enum TYPE t) { if (type == COORD) delete content.v; type = t;}
    void setCoordX(double d) { if (type == COORD) content.v->x = d;}
    void setCoordY(double d) { if (type == COORD) content.v->y = d;}
    void setCoordZ(double d) { if (type == COORD) content.v->z = d;}

private:
    typedef union {
        string *s;
        int i;
        double d;
        DRW_Coord *v;
    } DRW_VarContent;

public:
    DRW_VarContent content;

public:
    int code;
    string version;
    string codepage;
private:
//    DRW_VarContent content;
    string data;
};



#endif

// EOF

