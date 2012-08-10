/****************************************************************************
** $Id:  main.cpp
** fontconverter a qt console program to convert .jhf to .lff fonts
**
** Copyright (C) 2011 -Rallaz. <rallazz@gmail.comm>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public Licence
** as published by the Free Software Foundation; either version 3
** of the License, or (at your option) any later option.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
**
*****************************************************************************/

/*
lfftransform -scale char infile outfile
    char: char ex-code to use as base like 41 (A)
    the input file is scaled so that the character "char" measures BASE_SCALE heigth

lfftransform -short infile outfile
    short the file based in hex code
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <stdlib.h>
#include <sstream>
#include <math.h>

#define BASE_SCALE 9
#define TOLERANCE_ANGLE 1.0e-8

double correctAngle(double a ){
    return M_PI + remainder(a - M_PI, 2*M_PI);
}

bool isAngleBetween(double a, double a1, double a2) {
    //a1 and a2 almost the same angle
    // the |a2-a1| % (2 pi)=0 means the whole angular range
    if(fabs( remainder(correctAngle(a2 - a1 ) , 2.*M_PI)) < TOLERANCE_ANGLE)
        return true;
    if (  correctAngle(a2 -a1) >= correctAngle(a - a1) - 0.5*TOLERANCE_ANGLE
          || correctAngle(a2 -a1) >= correctAngle(a2 - a) -0.5*TOLERANCE_ANGLE
          ) return true;
    return false;
}

class defVert {
public:
    defVert(double x, double y, bool hB = false, double b= 0.0) {
        xVal = x;
        yVal = y;
        hasBulge = hB;
        bulge = b;
    }
    defVert(std::string data) {
        std::vector<std::string> verts;
        size_t start =0;
        size_t found =0;
        do {
            found=data.find(',', start);
            if (found!=std::string::npos) {
                verts.push_back(data.substr(start, found-start));
                start = ++found;
            } else
                verts.push_back(data.substr(start));
        } while (found!=std::string::npos);

        if (verts.size() > 1) {
            std::istringstream sdX(verts.at(0));
            std::istringstream sdY(verts.at(1));
            sdX >> xVal;
            sdY >> yVal;
        }
        if (verts.size() > 2) {
            hasBulge = true;
            std::istringstream sdB(verts.at(2).substr(1));
            sdB >> bulge;
        } else {
            hasBulge = false;
            bulge = 0.0;
        }
    }
    double distTo(defVert e) const;
    double angleTo(defVert e) const;

    double x() const { return xVal;}
    double y() const { return yVal;}
    void setX(double x){xVal = x;}
    void setY(double y){yVal = y;}
    defVert operator - (const defVert& v) const {
            return defVert(xVal - v.x(), yVal - v.y());}
    defVert operator + (const defVert& v) const {
            return defVert(xVal + v.x(), yVal + v.y());}
    defVert operator / (const double& s) const {
            return defVert(xVal / s, yVal / s);}

    bool hasBulge;
    double bulge;
private:
    double xVal;
    double yVal;
};

double defVert::distTo(defVert e) const{
    double incX = e.x()- xVal;
    double incY = e.y()- yVal;
    return sqrt( pow(incX, 2) + pow(incY, 2) );
}

double defVert::angleTo(defVert e) const{
    defVert pt = e - *this;
    double ang = atan2(pt.y(), pt.x());
    return M_PI + remainder(ang - M_PI, 2*M_PI);
}

class defRect {
public:
    defRect() {xVal = yVal = wVal = hVal = 0.0; valid = false;}
    defRect(double x, double y, double w, double h) {
        xVal = x;
        yVal = y;
        wVal = w;
        hVal = h;
        valid = true;
    }
    defRect(defVert p1, defVert p2) {
        xVal = std::min(p1.x(), p2.x());
        yVal = std::min(p1.y(), p2.y());
        wVal = abs(p1.x() - p2.x());
        hVal = abs(p1.y() - p2.y());
        valid = true;
    }
    bool isValid(){return valid;}
    double x() const { return xVal;}
    double y() const { return yVal;}
    double w() const { return wVal;}
    double h() const { return hVal;}
    defRect united(defRect r) {
        if (!valid) {
            if (r.isValid())
                return defRect(r.x(),r.y(),r.w(),r.h());
            else
                return defRect();
        } else if(!r.isValid())
            return defRect(xVal,yVal,wVal,hVal);

        defVert v = r.bottomRight();
        double w = std::max(xVal+wVal, v.x());
        double h = std::max(yVal+hVal, v.y());
        double x = std::min(xVal, r.x());
        double y = std::min(yVal, r.y());
        return defRect(x,y,w-x,h-y);
    }
    defVert bottomRight() {return defVert(xVal+wVal, yVal+hVal);}
private:
    double xVal;
    double yVal;
    double wVal;
    double hVal;
    bool valid;
};


class defPath {
public:
    defPath() {
        hasContour = false;
    }
    void addString(std::string s);

    std::string getString(){return defStr;}

    void scale (double factor){
        if (vertex.empty()) return;
        std::ostringstream strS(std::ostringstream::out);
        vertex[0].setX( vertex.at(0).x()* factor);
        vertex[0].setY( vertex.at(0).y()* factor);
        strS << vertex.at(0).x() << ',' << vertex.at(0).y();
        for (unsigned int i=1 ; i<vertex.size(); i++) {
            vertex[i].setX( vertex.at(i).x()* factor);
            vertex[i].setY( vertex.at(i).y()* factor);
            strS << ';' << vertex.at(i).x() << ',' << vertex.at(i).y();
            if (vertex.at(i).hasBulge) {
                strS << ",A" << vertex.at(i).bulge;
            }
        }
        defStr = strS.str();
    }

    defRect getRect() {
        if(!hasContour) {
            if (vertex.empty())
                contour = defRect();
            else {
                defRect rect = defRect(vertex.at(0), vertex.at(0));
                if (vertex.size() > 1)
                    for (unsigned int i=1 ; i<vertex.size(); i++) {
                        if(vertex.at(i).hasBulge){ //arc
                            defVert prevV = vertex.at(i-1);
                            defVert currV = vertex.at(i);
                            double alpha = atan(currV.bulge)*4.0;
                            defVert middle = (prevV + currV)/2.0;
                            double dist = prevV.distTo(currV)/2.0;
                            double angle = prevV.angleTo(currV);
                            double radius = fabs(dist / sin(alpha/2.0));
                            double wu = fabs(pow(radius, 2.0) - pow(dist, 2.0));
                            double h = sqrt(wu);
                            if (currV.bulge>0.0)
                                angle+=M_PI/2.0;
                            else
                                angle-=M_PI/2.0;

                            if (fabs(alpha)>M_PI)
                                h*=-1.0;
                            defVert center( h * cos(angle) + middle.x(), h * sin(angle)+ middle.y() );
                            double ang1, ang2;
                            if (currV.bulge>0.0){
                                ang1 = center.angleTo(prevV);
                                ang2 = center.angleTo(currV);
                            } else {
                                ang2 = center.angleTo(prevV);
                                ang1 = center.angleTo(currV);
                            }

                            double minX = std::min(prevV.x(), currV.x());
                            double minY = std::min(prevV.y(), currV.y());
                            double maxX = std::max(prevV.x(), currV.x());
                            double maxY = std::max(prevV.y(), currV.y());

                            if ( isAngleBetween(0.5*M_PI,ang1,ang2) )
                                maxY = center.y() + radius;
                            if ( isAngleBetween(1.5*M_PI,ang1,ang2) )
                                minY = center.y() - radius;
                            if ( isAngleBetween(M_PI,ang1,ang2) )
                                minX = center.x() - radius;
                            if ( isAngleBetween(0.,ang1,ang2) )
                                maxX = center.x() + radius;

                            rect = rect.united( defRect(minX, minY, (maxX - minX), (maxY - minY)) );
                        } else { //line
                            rect = rect.united( defRect(vertex.at(i-1), vertex.at(i)) );
                        }
                    }
                contour = rect;
            }
            hasContour = true;
        }
        return contour;
    }

    void moveH(double d){
        if (vertex.empty()) return;
        std::ostringstream strS(std::ostringstream::out);
        vertex[0].setX( vertex.at(0).x() + d);
        strS << vertex.at(0).x() << ',' << vertex.at(0).y();
        for (unsigned int i=1 ; i<vertex.size(); i++) {
            vertex[i].setX( vertex.at(i).x() + d);
            strS << ';' << vertex.at(i).x() << ',' << vertex.at(i).y();
            if (vertex.at(i).hasBulge) {
                strS << ",A" << vertex.at(i).bulge;
            }
        }
        defStr = strS.str();
    }
    void moveV(double d){
        if (vertex.empty()) return;
        std::ostringstream strS(std::ostringstream::out);
        vertex[0].setY( vertex.at(0).y() + d);
        strS << vertex.at(0).x() << ',' << vertex.at(0).y();
        for (unsigned int i=1 ; i<vertex.size(); i++) {
            vertex[i].setY( vertex.at(i).y() + d);
            strS << ';' << vertex.at(i).x() << ',' << vertex.at(i).y();
            if (vertex.at(i).hasBulge) {
                strS << ",A" << vertex.at(i).bulge;
            }
        }
        defStr = strS.str();
    }

private:
    std::vector <defVert> vertex;
    std::string defStr;
    defRect contour;
    bool hasContour;
};

void defPath::addString(std::string s) {
    defStr = s;
    if (s.at(0) !='C') {
        std::vector<std::string> verts;
        size_t start =0;
        size_t found =0;
        do {
            found=s.find(';', start);
            if (found!=std::string::npos) {
                verts.push_back(s.substr(start, found-start));
                start = ++found;
            } else
                verts.push_back(s.substr(start));
        } while (found!=std::string::npos);

        for (unsigned int i = 0; i < verts.size(); ++i) {
            defVert v(verts.at(i));
            vertex.push_back(v);
        }
    }
    hasContour = false;
}

class defChar {
public:
    defChar(int val, std::string n) {
        charName = n;
        charValue = val;
    }
    ~defChar() {
        while (!paths.empty())
             paths.pop_back();
    }
    void addPath(defPath *p) {
        paths.push_back(p);
    }

    defRect getRect() {
        if (paths.empty())
            return defRect();
        defRect rect = paths.at(0)->getRect();
        if (paths.size() > 1) {
            for (unsigned int i=1 ; i<paths.size(); i++) {
                rect = rect.united( paths.at(i)->getRect() );
            }
        }
        return rect;
    }
    void move (bool toLeft){
        if (paths.empty())
            return;
        defRect rect = paths.at(0)->getRect();
        if (paths.size() > 1) {
            for (unsigned int i=1 ; i<paths.size(); i++) {
                rect = rect.united( paths.at(i)->getRect() );
            }
        }
        if (toLeft) {
            double pos = rect.x();
            for (unsigned int i=0 ; i<paths.size(); i++) {
                paths.at(i)->moveH(-pos);
            }
        }else {
            double pos = rect.y();
            for (unsigned int i=0 ; i<paths.size(); i++) {
                paths.at(i)->moveV(-pos);
            }
        }
    }

    void scale (double factor){
        for (unsigned int i=0 ; i<paths.size(); i++) {
            paths.at(i)->scale(factor);
        }
    }
    std::string getString(){
        std::string strS =charName;
        for (unsigned int i=0 ; i<paths.size(); i++) {
            strS += '\n';
            strS += paths.at(i)->getString();
        }
        return strS;
    }

private:
    std::vector <defPath*> paths;
    int charValue;
    std::string charName;
};


class fontLFF {
public:
    fontLFF(std::string in, std::string out) {
        input = in;
        output =out;
        scaleF = 1;
//        defFont = 65;
    }
    int scale(int def);
    int shortChar();
    int move(std::string dir);

private:
    bool getScale(int defFont);

    std::string input;
    std::string output;
//    int defFont;
    double scaleF;
};


int fontLFF::move(std::string dir){
    bool moveLeft;
    if (dir == "b")
        moveLeft = false;
    else if (dir == "l")
        moveLeft = true;
    else
        return 5;
    std::string line;
    std::ifstream fi(input.c_str(), std::ios::in);
    std::ofstream fo(output.c_str(), std::ios::out | std::ios::trunc);
    if (!fi.is_open())
        return 2;
    if (!fo.is_open())
        return 3;
    while (fi.good()) {
        std::getline(fi, line);
        if (line.empty()) {
            fo << "\n";
            continue;
        }
        if (line.at(0)=='#') {
            fo << line << "\n";
        } else if (line.at(0)=='[') {
            std::istringstream sd(line.substr(1,4));
            int code;
            sd >> std::hex >> code;
            defChar dataChar(code, line);
            do {
                std::getline(fi, line);
                if(line.empty()) {
                    dataChar.move(moveLeft);
                    fo << "\n";
                    std::string data = dataChar.getString();
                    fo << data << "\n";
                    break;
                }
                defPath *pt = new defPath();
                pt->addString(line);
                dataChar.addPath(pt);
            } while(true);
        }
    }//end while parsing file
    fi.close();
    fo << "\n";
    fo.close();

    return 0;
}


int fontLFF::scale(int def){
    if (!getScale(def)) {
        return 4;
    }
    std::string line;
    std::ifstream fi(input.c_str(), std::ios::in);
    std::ofstream fo(output.c_str(), std::ios::out | std::ios::trunc);
    if (!fi.is_open()) {
        return 2;
    }
    if (!fo.is_open()) {
        return 3;
    }
    while (fi.good()) {
        std::getline(fi, line);
        if (line.empty()) {
            fo << "\n";
            continue;
        }
        if (line.at(0)=='#') {
            fo << line << "\n";
        } else if (line.at(0)=='[') {
            std::istringstream sd(line.substr(1,4));
            int code;
            sd >> std::hex >> code;
            defChar dataChar(code, line);
            do {
                std::getline(fi, line);
                if(line.empty()) {
                    dataChar.scale(scaleF);
                    fo << "\n";
                    std::string data = dataChar.getString();
                    fo << data << "\n";
                    break;
                }
                defPath *pt = new defPath();
                pt->addString(line);
                dataChar.addPath(pt);
            } while(true);
        }
    }//end while parsing file
    fi.close();
    fo << "\n";
    fo.close();
    return 0;
}

bool fontLFF::getScale(int defFont){
    std::string line;
    std::ifstream fi(input.c_str(), std::ios::in);
    bool success = false;
    if (!fi.is_open()) {
        return false;
    }
    //find base char
    while (fi.good()) {
        std::getline(fi, line);
        if (line.empty())
            continue;
        if (line.at(0)=='[') {
            std::istringstream sd(line.substr(1,4));
            int code;
            sd >> std::hex >> code;
            if (code == defFont) {
                defChar baseChar(code, line);
                do {
                    std::getline(fi, line);
                    if(line.empty()) break;
                    defPath *pt = new defPath();
                    pt->addString(line);
                    baseChar.addPath(pt);
                } while(true);
                scaleF = BASE_SCALE/baseChar.getRect().bottomRight().y();
                success = true;
                break;
            }
        }
    }
    fi.close();
    return success;
}

int fontLFF::shortChar(){
    std::string line;
    std::ifstream fi(input.c_str(), std::ios::in);
    std::ofstream fo(output.c_str(), std::ios::out | std::ios::trunc);
    std::map<int, defChar*> map;
    if (!fi.is_open()) {
        return 2;
    }
    if (!fo.is_open()) {
        return 3;
    }
    while (fi.good()) {
        std::getline(fi, line);
        if (line.empty()) {
            fo << "\n";
            continue;
        }
        if (line.at(0)=='#') {
            fo << line << "\n";
        } else if (line.at(0)=='[') {
            std::istringstream sd(line.substr(1,4));
            int code;
            sd >> std::hex >> code;
            defChar *dataChar = new defChar(code, line);
            map[code] = dataChar;
            do {
                std::getline(fi, line);
                if(line.empty()) {
                    break;
                }
                defPath *pt = new defPath();
                pt->addString(line);
                dataChar->addPath(pt);
            } while(true);
        }
    }//end while parsing file
    fi.close();

    std::map<int, defChar*>::iterator it;
    for ( it=map.begin() ; it != map.end(); it++ ){
        std::string data = (*it).second->getString();
        fo << "\n";
        fo << data << "\n";
    }
    map.clear();
    fo.close();
    return 0;
}

void usage(){
    std::cout << "usage: lfftransform -scale \"char\" infile outfile\n";
    std::cout << "usage: lfftransform -short infile outfile\n";
    std::cout << "usage: lfftransform -move \"to\" infile outfile\n";
    std::cout << "char: char ex-code to use as base like 41 (A)\n";
    std::cout << "to: valid value are b (bottom) or l (left)\n";
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        usage();
        return 1;
    }
    int success = 1;
    int param = 1;
    std::string option(argv[param++]);
    std::string dir;
    int code;

    if (option == "-scale" && argc == 5) {
        std::istringstream sd(argv[param++]);
        sd >> std::hex >> code;
    } else if (option == "-short" && argc == 4){
    } else if (option == "-move" && argc == 5){
        dir =argv[param++];
    } else {
        usage();
        return 1;
    }
    std::string input = argv[param++];
    std::string output = argv[param];
    fontLFF reader = fontLFF(input, output);
    if (option == "-scale")
        success = reader.scale(code);
    else if (option == "-short")
        success = reader.shortChar();
    else if (option == "-move")
        success = reader.move(dir);

    if (success==2)
        std::cout << "Failed reading input file: " << input << "\n";
    if (success==3)
        std::cout << "Failed writing output file: " << output << "\n";
    if (success==4)
        std::cout << "Font "<< argv[3] << " not found in input file\n";
    if (success==5)
        std::cout << "Bad direction to move, valid value are b or l\n";

    return success;
}

