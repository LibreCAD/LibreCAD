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

#include <cstdlib>
#include "drw_entities.h"
#include "dxfreader.h"


//! Calculate arbitary axis
/*!
*   Calculate arbitary axis for aplly extrusions
*  @author Rallaz
*/
void DRW_Entity::calculateAxis(DRW_Coord extPoint){
    if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625) {
        extAxisX.x = extPoint.z;
        extAxisX.y = 0;
        extAxisX.z = -extPoint.x;
    } else {
        extAxisX.x = -extPoint.y;
        extAxisX.y = extPoint.x;
        extAxisX.z = 0;
    }
    extAxisX.unitize();
    extAxisY.x = (extPoint.y * extAxisX.z) - (extAxisX.y * extPoint.z);
    extAxisY.y = (extPoint.z * extAxisX.x) - (extAxisX.z * extPoint.x);
    extAxisY.z = (extPoint.x * extAxisX.y) - (extAxisX.x * extPoint.y);
    extAxisY.unitize();
}
//! Extrude a point using arbitary axis
/*!
*   apply extrusion in a point using arbitary axis (previous calculated)
*  @author Rallaz
*/
void DRW_Entity::extrudePoint(DRW_Coord extPoint, DRW_Coord *point){
    double px, py, pz;
    px = (extAxisX.x*point->x)+(extAxisY.x*point->y)+(extPoint.x*point->z);
    py = (extAxisX.y*point->x)+(extAxisY.y*point->y)+(extPoint.y*point->z);
    pz = (extAxisX.z*point->x)+(extAxisY.z*point->y)+(extPoint.z*point->z);

    point->x = px;
    point->y = py;
    point->z = pz;
}

void DRW_Entity::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getString();
        break;
    case 330:
        handleBlock = reader->getString();
        break;
    case 8:
        layer = reader->getUtf8String();
        break;
    case 6:
        lineType = reader->getUtf8String();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 370:
//        lWeight = (DRW::LWEIGHT)reader->getInt32();
//RLZ: TODO as integer or enum??
        lWeight = reader->getInt32();
        break;
    case 48:
        ltypeScale = reader->getDouble();
        break;
    case 60:
        visible = reader->getBool();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    case 430:
        colorName = reader->getString();
        break;
    case 67:
        space = reader->getInt32();
        break;
    default:
        break;
    }
}

void DRW_Point::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Line::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 11:
        secPoint.x = reader->getDouble();
        break;
    case 21:
        secPoint.y = reader->getDouble();
        break;
    case 31:
        secPoint.z = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Circle::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
    }
}

void DRW_Circle::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        radious = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Arc::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 50:
        staangle = reader->getDouble();
        break;
    case 51:
        endangle = reader->getDouble();
        break;
    default:
        DRW_Circle::parseCode(code, reader);
        break;
    }
}

void DRW_Ellipse::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        ratio = reader->getDouble();
        break;
    case 41:
        staparam = reader->getDouble();
        break;
    case 42:
        endparam = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Ellipse::toPolyline(DRW_Polyline *pol){
    double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
    double cosCurr, sinCurr;
    radMajor = sqrt(secPoint.x*secPoint.x + secPoint.y*secPoint.y);
    radMinor = radMajor*ratio;
    incAngle = atan2(secPoint.y, secPoint.x);
    cosRot = cos(incAngle);
    sinRot = sin(incAngle);
    incAngle = M_PI/64;
    curAngle = staparam;
    int i = curAngle/incAngle;
    do {
        if (curAngle > endparam) {
            curAngle = endparam;
            i = 130;
        }
        cosCurr = cos(curAngle);
        sinCurr = sin(curAngle);
        double x = basePoint.x + (cosCurr*cosRot*radMajor) - (sinCurr*sinRot*radMinor);
        double y = basePoint.y + (cosCurr*sinRot*radMajor) + (sinCurr*cosRot*radMinor);
        pol->addVertex( DRW_Vertex(x, y, 0.0, 0.0));
        curAngle = (++i)*incAngle;
    } while (i<128);
    if ( fabs(endparam - 6.28318530718) < 1.0e-10){
        pol->flags = 1;
    }
    pol->layer = this->layer;
    pol->lineType = this->lineType;
    pol->color = this->color;
    pol->lWeight = this->lWeight;
}

void DRW_Trace::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
        extrudePoint(extPoint, &secPoint);
        extrudePoint(extPoint, &thirdPoint);
        extrudePoint(extPoint, &fourPoint);
    }
}

void DRW_Trace::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 12:
        thirdPoint.x = reader->getDouble();
        break;
    case 22:
        thirdPoint.y = reader->getDouble();
        break;
    case 32:
        thirdPoint.z = reader->getDouble();
        break;
    case 13:
        fourPoint.x = reader->getDouble();
        break;
    case 23:
        fourPoint.y = reader->getDouble();
        break;
    case 33:
        fourPoint.z = reader->getDouble();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Solid::parseCode(int code, dxfReader *reader){
        DRW_Trace::parseCode(code, reader);
}

void DRW_3Dface::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        invisibleflag = reader->getInt32();
        break;
    default:
        DRW_Trace::parseCode(code, reader);
        break;
    }
}

void DRW_Block::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Insert::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 41:
        xscale = reader->getDouble();
        break;
    case 42:
        yscale = reader->getDouble();
        break;
    case 43:
        zscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 70:
        colcount = reader->getInt32();
        break;
    case 71:
        rowcount = reader->getInt32();
        break;
    case 44:
        colspace = reader->getDouble();
        break;
    case 45:
        rowspace = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_LWPolyline::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        for (unsigned int i=0; i<vertlist.size(); i++) {
            DRW_Vertex2D *vert = vertlist.at(i);
            DRW_Coord v(vert->x, vert->y, elevation);
            extrudePoint(extPoint, &v);
            vert->x = v.x;
            vert->y = v.y;
        }
    }
}

void DRW_LWPolyline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 10: {
        vertex = new DRW_Vertex2D();
        vertlist.push_back(vertex);
        vertex->x = reader->getDouble();
        break; }
    case 20:
        if(vertex != NULL)
            vertex->y = reader->getDouble();
        break;
    case 40:
        if(vertex != NULL)
            vertex->stawidth = reader->getDouble();
        break;
    case 41:
        if(vertex != NULL)
            vertex->endwidth = reader->getDouble();
        break;
    case 42:
        if(vertex != NULL)
            vertex->bulge = reader->getDouble();
        break;
    case 38:
        elevation = reader->getDouble();
        break;
    case 43:
        width = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 90:
        vertexnum = reader->getInt32();
        vertlist.reserve(vertexnum);
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Text::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        widthscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 51:
        oblique = reader->getDouble();
        break;
    case 71:
        textgen = reader->getInt32();
        break;
    case 72:
        alignH = (HAlign)reader->getInt32();
        break;
    case 73:
        alignV = (VAlign)reader->getInt32();
        break;
    case 1:
        text = reader->getUtf8String();
        break;
    case 7:
        style = reader->getUtf8String();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_MText::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        text += reader->getString();
        text = reader->toUtf8String(text);
        break;
    case 11:
        haveXAxis = true;
        DRW_Text::parseCode(code, reader);
        break;
    case 3:
        text += reader->getString();
        break;
    case 44:
        interlin = reader->getDouble();
        break;
    default:
        DRW_Text::parseCode(code, reader);
        break;
    }
}

void DRW_MText::updateAngle(){
    if (haveXAxis) {
            angle = atan2(secPoint.y, secPoint.x)*180/M_PI;
    }
}

void DRW_Polyline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        defstawidth = reader->getDouble();
        break;
    case 41:
        defendwidth = reader->getDouble();
        break;
    case 71:
        vertexcount = reader->getInt32();
        break;
    case 72:
        facecount = reader->getInt32();
        break;
    case 73:
        smoothM = reader->getInt32();
        break;
    case 74:
        smoothN = reader->getInt32();
        break;
    case 75:
        curvetype = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Vertex::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        stawidth = reader->getDouble();
        break;
    case 41:
        endwidth = reader->getDouble();
        break;
    case 42:
        bulge = reader->getDouble();
        break;
    case 50:
        tgdir = reader->getDouble();
        break;
    case 71:
        vindex1 = reader->getInt32();
        break;
    case 72:
        vindex2 = reader->getInt32();
        break;
    case 73:
        vindex3 = reader->getInt32();
        break;
    case 74:
        vindex4 = reader->getInt32();
        break;
    case 91:
        identifier = reader->getInt32();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Hatch::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        solid = reader->getInt32();
        break;
    case 71:
        associative = reader->getInt32();
        break;
    case 72:        /*edge type*/
        if (ispol){ //if is polyline is a as_bulge flag
            break;
        } else if (reader->getInt32() == 1){ //line
            addLine();
        } else if (reader->getInt32() == 2){ //arc
            addArc();
        } else if (reader->getInt32() == 3){ //elliptic arc
            addEllipse();
        } else if (reader->getInt32() == 4){ //spline
            addSpline();
        }
        break;
    case 10:
        if (pt) pt->basePoint.x = reader->getDouble();
        else if (pline) {
            plvert = pline->addVertex();
            plvert->x = reader->getDouble();
        }
        break;
    case 20:
        if (pt) pt->basePoint.y = reader->getDouble();
        else if (plvert) plvert ->y = reader->getDouble();
        break;
    case 11:
        if (line) line->secPoint.x = reader->getDouble();
        else if (ellipse) ellipse->secPoint.x = reader->getDouble();
        break;
    case 21:
        if (line) line->secPoint.y = reader->getDouble();
        else if (ellipse) ellipse->secPoint.y = reader->getDouble();
        break;
    case 40:
        if (arc) arc->radious = reader->getDouble();
        else if (ellipse) ellipse->ratio = reader->getDouble();
        break;
    case 41:
        scale = reader->getDouble();
        break;
    case 42:
        if (plvert) plvert ->bulge = reader->getDouble();
        break;
    case 50:
        if (arc) arc->staangle = reader->getDouble();
        else if (ellipse) ellipse->staparam = reader->getDouble();
        break;
    case 51:
        if (arc) arc->endangle = reader->getDouble();
        else if (ellipse) ellipse->endparam = reader->getDouble();
        break;
    case 52:
        angle = reader->getDouble();
        break;
    case 73:
        if (arc) arc->isccw = reader->getInt32();
        else if (pline) pline->flags = reader->getInt32();
        break;
    case 75:
        hstyle = reader->getInt32();
        break;
    case 76:
        hpattern = reader->getInt32();
        break;
    case 77:
        doubleflag = reader->getInt32();
        break;
    case 78:
        deflines = reader->getInt32();
        break;
    case 91:
        loopsnum = reader->getInt32();
        looplist.reserve(loopsnum);
        break;
    case 92:
        loop = new DRW_HatchLoop(reader->getInt32());
        looplist.push_back(loop);
        if (reader->getInt32() & 2) {
            ispol = true;
            clearEntities();
            pline = new DRW_LWPolyline;
            loop->objlist.push_back(pline);
        } else ispol = false;
        break;
    case 93:
        if (pline) pline->vertexnum = reader->getInt32();
        else loop->numedges = reader->getInt32();//aqui reserve
        break;
    case 98: //seed points ??
        clearEntities();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}

void DRW_Spline::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 210:
        ex = reader->getDouble();
        break;
    case 220:
        ey = reader->getDouble();
        break;
    case 230:
        ez = reader->getDouble();
        break;
    case 12:
        tgsx = reader->getDouble();
        break;
    case 22:
        tgsy = reader->getDouble();
        break;
    case 32:
        tgsz = reader->getDouble();
        break;
    case 13:
        tgex = reader->getDouble();
        break;
    case 23:
        tgey = reader->getDouble();
        break;
    case 33:
        tgez = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 71:
        degree = reader->getInt32();
        break;
    case 72:
        nknots = reader->getInt32();
        break;
    case 73:
        ncontrol = reader->getInt32();
        break;
    case 74:
        nfit = reader->getInt32();
        break;
    case 42:
        tolknot = reader->getDouble();
        break;
    case 43:
        tolcontrol = reader->getDouble();
        break;
    case 44:
        tolfit = reader->getDouble();
        break;
    case 10: {
        controlpoint = new DRW_Coord();
        controllist.push_back(controlpoint);
        controlpoint->x = reader->getDouble();
        break; }
    case 20:
        if(controlpoint != NULL)
            controlpoint->y = reader->getDouble();
        break;
    case 30:
        if(controlpoint != NULL)
            controlpoint->z = reader->getDouble();
        break;
    case 11: {
        fitpoint = new DRW_Coord();
        fitlist.push_back(fitpoint);
        fitpoint->x = reader->getDouble();
        break; }
    case 21:
        if(fitpoint != NULL)
            fitpoint->y = reader->getDouble();
        break;
    case 31:
        if(fitpoint != NULL)
            fitpoint->z = reader->getDouble();
        break;
    case 40:
        knotslist.push_back(reader->getDouble());
        break;
//    case 41:
//        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Image::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 12:
        vx = reader->getDouble();
        break;
    case 22:
        vy = reader->getDouble();
        break;
    case 32:
        vz = reader->getDouble();
        break;
    case 13:
        sizeu = reader->getDouble();
        break;
    case 23:
        sizev = reader->getDouble();
        break;
    case 340:
        ref = reader->getString();
        break;
    case 280:
        clip = reader->getInt32();
        break;
    case 281:
        brightness = reader->getInt32();
        break;
    case 282:
        contrast = reader->getInt32();
        break;
    case 283:
        fade = reader->getInt32();
        break;
    default:
        DRW_Line::parseCode(code, reader);
        break;
    }
}

void DRW_Dimension::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 2:
        name = reader->getString();
        break;
    case 3:
        style = reader->getUtf8String();
        break;
    case 70:
        type = reader->getInt32();
        break;
    case 71:
        align = reader->getInt32();
        break;
    case 72:
        linesty = reader->getInt32();
        break;
    case 10:
        defPoint.x = reader->getDouble();
        break;
    case 20:
        defPoint.y = reader->getDouble();
        break;
    case 30:
        defPoint.z = reader->getDouble();
        break;
    case 11:
        textPoint.x = reader->getDouble();
        break;
    case 21:
        textPoint.y = reader->getDouble();
        break;
    case 31:
        textPoint.z = reader->getDouble();
        break;
    case 12:
        clonePoint.x = reader->getDouble();
        break;
    case 22:
        clonePoint.y = reader->getDouble();
        break;
    case 32:
        clonePoint.z = reader->getDouble();
        break;
    case 13:
        def1.x = reader->getDouble();
        break;
    case 23:
        def1.y = reader->getDouble();
        break;
    case 33:
        def1.z = reader->getDouble();
        break;
    case 14:
        def2.x = reader->getDouble();
        break;
    case 24:
        def2.y = reader->getDouble();
        break;
    case 34:
        def2.z = reader->getDouble();
        break;
    case 15:
        circlePoint.x = reader->getDouble();
        break;
    case 25:
        circlePoint.y = reader->getDouble();
        break;
    case 35:
        circlePoint.z = reader->getDouble();
        break;
    case 16:
        arcPoint.x = reader->getDouble();
        break;
    case 26:
        arcPoint.y = reader->getDouble();
        break;
    case 36:
        arcPoint.z = reader->getDouble();
        break;
    case 41:
        linefactor = reader->getDouble();
        break;
    case 53:
        rot = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 52:
        oblique = reader->getDouble();
        break;
    case 40:
        length = reader->getDouble();
        break;
/*    case 51:
        hdir = reader->getDouble();
        break;*/
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Leader::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 3:
        style = reader->getUtf8String();
        break;
    case 71:
        arrow = reader->getInt32();
        break;
    case 72:
        leadertype = reader->getInt32();
        break;
    case 73:
        flag = reader->getInt32();
        break;
    case 74:
        hookline = reader->getInt32();
        break;
    case 75:
        hookflag = reader->getInt32();
        break;
    case 76:
        vertnum = reader->getInt32();
        break;
    case 77:
        coloruse = reader->getInt32();
        break;
    case 40:
        textheight = reader->getDouble();
        break;
    case 41:
        textwidth = reader->getDouble();
        break;
    case 10: {
        vertexpoint = new DRW_Coord();
        vertexlist.push_back(vertexpoint);
        vertexpoint->x = reader->getDouble();
        break; }
    case 20:
        if(vertexpoint != NULL)
            vertexpoint->y = reader->getDouble();
        break;
    case 30:
        if(vertexpoint != NULL)
            vertexpoint->z = reader->getDouble();
        break;
    case 340:
        handle = reader->getString();
        break;
    case 210:
        extrusionPoint.x = reader->getDouble();
        break;
    case 220:
        extrusionPoint.y = reader->getDouble();
        break;
    case 230:
        extrusionPoint.z = reader->getDouble();
        break;
    case 211:
        horizdir.x = reader->getDouble();
        break;
    case 221:
        horizdir.y = reader->getDouble();
        break;
    case 231:
        horizdir.z = reader->getDouble();
        break;
    case 212:
        offsetblock.x = reader->getDouble();
        break;
    case 222:
        offsetblock.y = reader->getDouble();
        break;
    case 232:
        offsetblock.z = reader->getDouble();
        break;
    case 213:
        offsettext.x = reader->getDouble();
        break;
    case 223:
        offsettext.y = reader->getDouble();
        break;
    case 233:
        offsettext.z = reader->getDouble();
        break;
    default:
        DRW_Entity::parseCode(code, reader);
        break;
    }
}

void DRW_Viewport::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 40:
        pswidth = reader->getDouble();
        break;
    case 41:
        psheight = reader->getDouble();
        break;
    case 68:
        vpstatus = reader->getInt32();
        break;
    case 69:
        vpID = reader->getInt32();
        break;
    case 12: {
        centerPX = reader->getDouble();
        break; }
    case 22:
        centerPY = reader->getDouble();
        break;
    default:
        DRW_Point::parseCode(code, reader);
        break;
    }
}
