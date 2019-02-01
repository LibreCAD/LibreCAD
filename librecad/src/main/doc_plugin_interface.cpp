/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "doc_plugin_interface.h"
#include <QEventLoop>
#include <QList>
#include <QInputDialog>
#include <QFileInfo>
#include "rs_graphicview.h"
#include "rs_actioninterface.h"
#include "rs_eventhandler.h"
#include "rs_actionselect.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_image.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_polyline.h"
#include "rs_ellipse.h"
#include "rs_polyline.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetselect.h"
#include "intern/qc_actiongetent.h"
#include "rs_math.h"
#include "rs_debug.h"
// #include <QDebug>

convLTW::convLTW(){
//    QHash<int, QString> lType;
    lType.insert(RS2::LineByLayer, "BYLAYER");
    lType.insert(RS2::LineByBlock, "BYBLOCK");
    lType.insert(RS2::SolidLine, "SolidLine");
    lType.insert(RS2::DotLine, "DotLine");
    lType.insert(RS2::DotLine2, "DotLine2");
    lType.insert(RS2::DotLineX2, "DotLineX2");
    lType.insert(RS2::DashLine, "DashLine");
    lType.insert(RS2::DashLine2, "DashLine2");
    lType.insert(RS2::DashLineX2, "DashLineX2");
    lType.insert(RS2::DashDotLine, "DashDotLine");
    lType.insert(RS2::DashDotLine2, "DashDotLine2");
    lType.insert(RS2::DashDotLineX2, "DashDotLineX2");
    lType.insert(RS2::DivideLine, "DivideLine");
    lType.insert(RS2::DivideLine2, "DivideLine2");
    lType.insert(RS2::DivideLineX2, "DivideLineX2");
    lType.insert(RS2::CenterLine, "CenterLine");
    lType.insert(RS2::CenterLine2, "CenterLine2");
    lType.insert(RS2::CenterLineX2, "CenterLineX2");
    lType.insert(RS2::BorderLine, "BorderLine");
    lType.insert(RS2::BorderLine2, "BorderLine");
    lType.insert(RS2::BorderLineX2, "BorderLine");

    lWidth.insert(RS2::Width00, "0.00mm");
    lWidth.insert(RS2::Width01, "0.05mm");
    lWidth.insert(RS2::Width02, "0.09mm");
    lWidth.insert(RS2::Width03, "0.13mm");
    lWidth.insert(RS2::Width04, "0.15mm");
    lWidth.insert(RS2::Width05, "0.18mm");
    lWidth.insert(RS2::Width06, "0.20mm");
    lWidth.insert(RS2::Width07, "0.25mm");
    lWidth.insert(RS2::Width08, "0.30mm");
    lWidth.insert(RS2::Width09, "0.35mm");
    lWidth.insert(RS2::Width10, "0.40mm");
    lWidth.insert(RS2::Width11, "0.50mm");
    lWidth.insert(RS2::Width12, "0.53mm");
    lWidth.insert(RS2::Width13, "0.60mm");
    lWidth.insert(RS2::Width14, "0.70mm");
    lWidth.insert(RS2::Width15, "0.80mm");
    lWidth.insert(RS2::Width16, "0.90mm");
    lWidth.insert(RS2::Width17, "1.00mm");
    lWidth.insert(RS2::Width18, "1.06mm");
    lWidth.insert(RS2::Width19, "1.20mm");
    lWidth.insert(RS2::Width20, "1.40mm");
    lWidth.insert(RS2::Width21, "1.58mm");
    lWidth.insert(RS2::Width22, "2.00mm");
    lWidth.insert(RS2::Width23, "2.11mm");
    lWidth.insert(RS2::WidthByLayer, "BYLAYER");
    lWidth.insert(RS2::WidthByBlock, "BYBLOCK");
    lWidth.insert(RS2::WidthDefault, "BYDEFAULT");
}

QString convLTW::lt2str(enum RS2::LineType lt){
    return lType.value(lt, "BYLAYER");
}
QString convLTW::lw2str(enum RS2::LineWidth lw){
    return lWidth.value(lw, "BYDEFAULT");
}
enum RS2::LineType convLTW::str2lt(QString s){
    return lType.key(s, RS2::LineByLayer);
}
enum RS2::LineWidth convLTW::str2lw(QString w){
    return lWidth.key(w, RS2::WidthDefault);
}
QString convLTW::intColor2str(int col){
    switch (col) {
    case -1:
        return "BYLAYER";
        break;
    case -2:
        return "BYBLOCK";
        break;
    default:
        return QString::number(col >> 16) + ", " + QString::number((col >> 8)& 0xFF) + ", " + QString::number(col & 0xFF);
        break;
    }
}


convLTW Converter;


Plugin_Entity::Plugin_Entity(RS_Entity* ent, Doc_plugin_interface* d):
    entity(ent)
  ,hasContainer(true)
  ,dpi(d)
{
}

/*RS_EntityContainer* parent,
                 const RS_LineData& d*/
Plugin_Entity::Plugin_Entity(RS_EntityContainer* parent, enum DPI::ETYPE type){
    hasContainer = false;
	dpi = nullptr;
	entity = nullptr;
    switch (type) {
    case DPI::POINT:
		entity = new RS_Point(parent, RS_PointData(RS_Vector(0,0)));
        break;
    case DPI::LINE:
		entity = new RS_Line{parent, {}, {}};
        break;
/*    case DPI::CONSTRUCTIONLINE:
        entity = new RS_ConstructionLine();
        break;*/
    case DPI::CIRCLE:
        entity = new RS_Circle(parent, RS_CircleData());
        break;
    case DPI::ARC:
        entity = new RS_Arc(parent, RS_ArcData());
        break;
    case DPI::ELLIPSE:
		entity = new RS_Ellipse{parent,
		{{0.,0.}, {0.,0.},0.,0.,0.,false}};
        break;
    case DPI::IMAGE:
        entity = new RS_Image(parent, RS_ImageData());
        break;
/*    case DPI::OVERLAYBOX:
        entity = new RS_OverlayBox();
        break;
    case DPI::SOLID:
        entity = new RS_Solid();
        break;*/
    case DPI::MTEXT:
        entity = new RS_MText(parent, RS_MTextData());
        break;
    case DPI::TEXT:
        entity = new RS_Text(parent, RS_TextData());
        break;
/*    case DPI::INSERT:
        entity = new RS_Insert();
        break;*/
    case DPI::POLYLINE:
        entity = new RS_Polyline(parent, RS_PolylineData());
        break;
/*    case DPI::SPLINE:
        entity = new RS_Spline();
        break;
    case DPI::HATCH:
        entity = new RS_Hatch();
        break;
    case DPI::DIMLEADER:
        entity = new RS_Leader();
        break;
    case DPI::DIMALIGNED:
        entity = new RS_DimAligned();
        break;
    case DPI::DIMLINEAR:
        entity = new RS_DimLinear();
        break;
    case DPI::DIMRADIAL:
        entity = new RS_DimRadial();
        break;
    case DPI::DIMDIAMETRIC:
        entity = new RS_DimDiametric();
        break;
    case DPI::DIMANGULAR:
        entity = new RS_DimAngular();
        break;*/
    default:
        break;
    }
}

Plugin_Entity::~Plugin_Entity() {
    if(!hasContainer)
        delete entity;
}

void Plugin_Entity::getData(QHash<int, QVariant> *data){
	if (!entity) return;
    RS2::EntityType et = entity->rtti();
    data->insert(DPI::EID, (qulonglong)entity->getId());
    data->insert(DPI::LAYER, entity->getLayer()->getName() );
    data->insert(DPI::LTYPE, Converter.lt2str(entity->getPen(false).getLineType()) );
    data->insert(DPI::LWIDTH, Converter.lw2str(entity->getPen(false).getWidth()) );
    data->insert(DPI::COLOR, entity->getPen(false).getColor().toIntColor() );
    data->insert(DPI::VISIBLE, (entity->isVisible()) ? 1 : 0 );
    switch (et) {
    //atomicEntity
    case RS2::EntityLine: {
        data->insert(DPI::ETYPE, DPI::LINE);
        RS_LineData d = static_cast<RS_Line*>(entity)->getData();
        data->insert(DPI::STARTX, d.startpoint.x );
        data->insert(DPI::STARTY, d.startpoint.y );
        data->insert(DPI::ENDX, d.endpoint.x );
        data->insert(DPI::ENDY, d.endpoint.y );
        break;}
    case RS2::EntityPoint: {
        data->insert(DPI::ETYPE, DPI::POINT);
        RS_PointData d = static_cast<RS_Point*>(entity)->getData();
        data->insert(DPI::STARTX, d.pos.x );
        data->insert(DPI::STARTY, d.pos.y );
        break; }
    case RS2::EntityArc: {
        data->insert(DPI::ETYPE, DPI::ARC);
        RS_ArcData d = static_cast<RS_Arc*>(entity)->getData();
        data->insert(DPI::STARTX, d.center.x );
        data->insert(DPI::STARTY, d.center.y );
        data->insert(DPI::RADIUS, d.radius );
        data->insert(DPI::STARTANGLE, d.angle1 );
        data->insert(DPI::ENDANGLE, d.angle2 );
        data->insert(DPI::REVERSED, d.reversed );
        break;}
    case RS2::EntityCircle: {
        data->insert(DPI::ETYPE, DPI::CIRCLE);
        RS_CircleData d = static_cast<RS_Circle*>(entity)->getData();
        data->insert(DPI::STARTX, d.center.x );
        data->insert(DPI::STARTY, d.center.y );
        data->insert(DPI::RADIUS, d.radius );
        break;}
    case RS2::EntityEllipse: {
        data->insert(DPI::ETYPE, DPI::ELLIPSE);
//        RS_EllipseData d = static_cast<RS_Ellipse*>(entity)->getData();
        RS_Ellipse *dd = static_cast<RS_Ellipse*>(entity);
        data->insert(DPI::STARTX, dd->getCenter().x );//10
        data->insert(DPI::STARTY, dd->getCenter().y );//20
        data->insert(DPI::ENDX, dd->getMajorP().x );//11 endpoint major axis x
        data->insert(DPI::ENDY, dd->getMajorP().y );//21 endpoint major axis y
        data->insert(DPI::HEIGHT, dd->getRatio() );//40 major/minor axis ratio
        data->insert(DPI::STARTANGLE, dd->getAngle1() );
        data->insert(DPI::ENDANGLE, dd->getAngle2() );
        data->insert(DPI::REVERSED, dd->isReversed() );
        break;}
    case RS2::EntitySolid: //TODO
        //Only used in dimensions ?
        data->insert(DPI::ETYPE, DPI::SOLID);
        break;
    case RS2::EntityConstructionLine:
        //Unused ?
        data->insert(DPI::ETYPE, DPI::CONSTRUCTIONLINE);
        break;
    case RS2::EntityImage: {
        data->insert(DPI::ETYPE, DPI::IMAGE);
        RS_ImageData d = static_cast<RS_Image*>(entity)->getData();
        data->insert(DPI::STARTX, d.insertionPoint.x );
        data->insert(DPI::STARTY, d.insertionPoint.y );
        data->insert(DPI::ENDX, d.uVector.x );
        data->insert(DPI::ENDY, d.uVector.y );
        data->insert(DPI::VVECTORX, d.vVector.x );
        data->insert(DPI::VVECTORY, d.vVector.y );
        data->insert(DPI::SIZEU, d.size.x );
        data->insert(DPI::SIZEV, d.size.y );
        data->insert(DPI::BLKNAME, d.file );
        break;}
    case RS2::EntityOverlayBox:
        //Unused ?
        data->insert(DPI::ETYPE, DPI::OVERLAYBOX);
        break;
//EntityContainer
    case RS2::EntityInsert: {
        data->insert(DPI::ETYPE, DPI::INSERT);
        RS_InsertData d = static_cast<RS_Insert*>(entity)->getData();
        data->insert(DPI::STARTX, d.insertionPoint.x );
        data->insert(DPI::STARTY, d.insertionPoint.y );
        data->insert(DPI::BLKNAME, d.name );
        data->insert(DPI::STARTANGLE, d.angle );
        data->insert(DPI::XSCALE, d.scaleFactor.x );
        data->insert(DPI::YSCALE, d.scaleFactor.y );
        break;}
    case RS2::EntityMText: {
        data->insert(DPI::ETYPE, DPI::MTEXT);
        RS_MTextData d = static_cast<RS_MText*>(entity)->getData();
        data->insert(DPI::STARTX, d.insertionPoint.x );
        data->insert(DPI::STARTY, d.insertionPoint.y );
        data->insert(DPI::STARTANGLE, d.angle );
        data->insert(DPI::HEIGHT, d.height );
        data->insert(DPI::TEXTCONTENT, d.text );
        break;}
    case RS2::EntityText: {
        data->insert(DPI::ETYPE, DPI::TEXT);
        RS_TextData d = static_cast<RS_Text*>(entity)->getData();
        data->insert(DPI::STARTX, d.insertionPoint.x );
        data->insert(DPI::STARTY, d.insertionPoint.y );
        data->insert(DPI::STARTANGLE, d.angle );
        data->insert(DPI::HEIGHT, d.height );
        data->insert(DPI::TEXTCONTENT, d.text );
        break;}
    case RS2::EntityHatch:
        data->insert(DPI::ETYPE, DPI::HATCH);
        break;
    case RS2::EntitySpline:
        data->insert(DPI::ETYPE, DPI::SPLINE);
        break;
	case RS2::EntitySplinePoints:
		data->insert(DPI::ETYPE, DPI::SPLINEPOINTS);
		break;
    case RS2::EntityPolyline:
        data->insert(DPI::ETYPE, DPI::POLYLINE);
        data->insert(DPI::CLOSEPOLY, static_cast<RS_Polyline*>(entity)->isClosed() );
        break;
    case RS2::EntityVertex:
        data->insert(DPI::ETYPE, DPI::UNKNOWN);
        break;
    case RS2::EntityDimAligned:
        data->insert(DPI::ETYPE, DPI::DIMALIGNED);
        break;
    case RS2::EntityDimLinear:
        data->insert(DPI::ETYPE, DPI::DIMLINEAR);
        break;
    case RS2::EntityDimRadial:
        data->insert(DPI::ETYPE, DPI::DIMRADIAL);
        break;
    case RS2::EntityDimDiametric:
        data->insert(DPI::ETYPE, DPI::DIMDIAMETRIC);
        break;
    case RS2::EntityDimAngular:
        data->insert(DPI::ETYPE, DPI::DIMANGULAR);
        break;
    case RS2::EntityDimLeader:
        data->insert(DPI::ETYPE, DPI::DIMLEADER);
        break;
    case RS2::EntityUnknown:
    default:
        data->insert(DPI::ETYPE, DPI::UNKNOWN);
        break;
    }
}

void Plugin_Entity::updateData(QHash<int, QVariant> *data){
	if (!entity) return;
    RS_Entity *ec= entity;
    if(hasContainer && dpi) {
        ec = entity->clone();
    }
    QHash<int, QVariant> hash = *data;
    QString str;
    RS_Vector vec;
    RS_Pen epen = ec->getPen();
//    double num;
    if (hash.contains(DPI::LAYER)) {
        str = (hash.take(DPI::LAYER)).toString();
        ec->setLayer(str);
    }
    if (hash.contains(DPI::LTYPE)) {
        str = (hash.take(DPI::LTYPE)).toString();
        epen.setLineType( Converter.str2lt(str) );
    }
    if (hash.contains(DPI::LWIDTH)) {
        str = (hash.take(DPI::LWIDTH)).toString();
        epen.setWidth( Converter.str2lw(str) );
    }
    if (hash.contains(DPI::COLOR)) {
        int co = hash.take(DPI::COLOR).toInt();
        RS_Color color;// = hash.take(DPI::COLOR).value<QColor>();
        color.fromIntColor(co);
        epen.setColor(color);
    }
    ec->setPen(epen);

    RS2::EntityType et = ec->rtti();
    switch (et) {
    //atomicEntity
    case RS2::EntityLine: {
        vec = static_cast<RS_Line*>(ec)->getStartpoint();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        static_cast<RS_Line*>(ec)->setStartpoint(vec);
        vec = static_cast<RS_Line*>(ec)->getEndpoint();
        if (hash.contains(DPI::ENDX)) {
            vec.x = (hash.take(DPI::ENDX)).toDouble();
        }
        if (hash.contains(DPI::ENDY)) {
            vec.y = (hash.take(DPI::ENDY)).toDouble();
        }
        static_cast<RS_Line*>(ec)->setEndpoint(vec);
        break;}
    case RS2::EntityPoint: {
        vec = static_cast<RS_Point*>(ec)->getPos();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        static_cast<RS_Point*>(ec)->setPos(vec);
        break; }
    case RS2::EntityArc: {
        RS_Arc *arc = static_cast<RS_Arc*>(ec);
        vec = arc->getCenter();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        arc->setCenter(vec);
        if (hash.contains(DPI::RADIUS)) {
            arc->setRadius( (hash.take(DPI::RADIUS)).toDouble() );
        }
        if (hash.contains(DPI::STARTANGLE)) {
             arc->setAngle1( (hash.take(DPI::STARTANGLE)).toDouble() );
           vec.y = (hash.take(DPI::STARTANGLE)).toDouble();
        }
        if (hash.contains(DPI::ENDANGLE)) {
            arc->setAngle2( (hash.take(DPI::ENDANGLE)).toDouble() );
        }
        break;}
    case RS2::EntityCircle: {
        RS_Circle *cir = static_cast<RS_Circle*>(ec);
        vec = cir->getCenter();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        cir->setCenter(vec);
        if (hash.contains(DPI::RADIUS)) {
            cir->setRadius( (hash.take(DPI::RADIUS)).toDouble() );
        }
        break;}
    case RS2::EntityEllipse: {
        RS_Ellipse *ellipse = static_cast<RS_Ellipse*>(ec);
        vec = ellipse->getCenter();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        ellipse->setCenter(vec);

        vec = ellipse->getMajorP();
        if (hash.contains(DPI::ENDX)) {
            vec.x = (hash.take(DPI::ENDX)).toDouble();
        }
        if (hash.contains(DPI::ENDY)) {
            vec.y = (hash.take(DPI::ENDY)).toDouble();
        }
        ellipse->setMajorP(vec);

        if (hash.contains(DPI::STARTANGLE)) {
            ellipse->setAngle1((hash.take(DPI::STARTANGLE)).toDouble());
        }
        if (hash.contains(DPI::ENDANGLE)) {
            ellipse->setAngle2((hash.take(DPI::ENDANGLE)).toDouble());
        }
        if (hash.contains(DPI::HEIGHT)) {
            ellipse->setRatio((hash.take(DPI::HEIGHT)).toDouble());
        }
        if (hash.contains(DPI::REVERSED)) {
            ellipse->setReversed( (hash.take(DPI::REVERSED)).toBool());
        }
        break;}
    case RS2::EntitySolid: //TODO
        //Only used in dimensions ?
        break;
    case RS2::EntityConstructionLine:
        //Unused ?
        break;
    case RS2::EntityImage: {
        RS_Image *img = static_cast<RS_Image*>(ec);
        vec = img->getInsertionPoint();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        img->setInsertionPoint(vec);
        if (hash.contains(DPI::BLKNAME)) {
            img->setFile( (hash.take(DPI::BLKNAME)).toString() );
        }
        vec = img->getUVector();
        RS_Vector vec2 = img->getVVector();
        RS_Vector vec3(img->getWidth(),img->getHeight());
        if (hash.contains(DPI::ENDX)) {
            vec.x = (hash.take(DPI::ENDX)).toDouble();
        }
        if (hash.contains(DPI::ENDY)) {
            vec.y = (hash.take(DPI::ENDY)).toDouble();
        }
        if (hash.contains(DPI::VVECTORX)) {
            vec2.x = (hash.take(DPI::VVECTORX)).toDouble();
        }
        if (hash.contains(DPI::VVECTORY)) {
            vec2.y = (hash.take(DPI::VVECTORY)).toDouble();
        }
        if (hash.contains(DPI::SIZEU)) {
            vec3.x = (hash.take(DPI::SIZEU)).toDouble();
        }
        if (hash.contains(DPI::SIZEV)) {
            vec3.y = (hash.take(DPI::SIZEV)).toDouble();
        }
        img->updateData(vec3, vec, vec2);
        break;}
    case RS2::EntityOverlayBox:
        //Unused ?
        break;
//EntityContainer
    case RS2::EntityInsert: {
        break;}
    case RS2::EntityMText: {
        RS_MText *txt = static_cast<RS_MText*>(ec);
        bool move = false;
        vec = txt->getInsertionPoint();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
            move = true;
        } else vec.x = 0;
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
            move = true;
        } else vec.y = 0;
        if (move)
            txt->move(vec);
        if (hash.contains(DPI::TEXTCONTENT)) {
            txt->setText( (hash.take(DPI::TEXTCONTENT)).toString() );
        }
        if (hash.contains(DPI::STARTANGLE)) {
            txt->setAngle( (hash.take(DPI::STARTANGLE)).toDouble() );
        }
        if (hash.contains(DPI::HEIGHT)) {
            txt->setHeight( (hash.take(DPI::HEIGHT)).toDouble() );
        }
        break;}
    case RS2::EntityText: {
        RS_Text *txt = static_cast<RS_Text*>(ec);
        bool move = false;
        vec = txt->getInsertionPoint();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
            move = true;
        } else vec.x = 0;
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
            move = true;
        } else vec.y = 0;
        if (move)
            txt->move(vec);
        if (hash.contains(DPI::TEXTCONTENT)) {
            txt->setText( (hash.take(DPI::TEXTCONTENT)).toString() );
        }
        if (hash.contains(DPI::STARTANGLE)) {
            txt->setAngle( (hash.take(DPI::STARTANGLE)).toDouble() );
        }
        if (hash.contains(DPI::HEIGHT)) {
            txt->setHeight( (hash.take(DPI::HEIGHT)).toDouble() );
        }
        break;}
    case RS2::EntityHatch:
        break;
    case RS2::EntitySpline:
        break;
    case RS2::EntityPolyline: {
        RS_Polyline *pl = static_cast<RS_Polyline*>(ec);
        if (hash.take(DPI::CLOSEPOLY).toBool()) {
            pl->setClosed(true);
        }else{
            pl->setClosed(false);
        }
        break;}
    case RS2::EntityVertex:
        break;
    case RS2::EntityDimAligned:
        break;
    case RS2::EntityDimLinear:
        break;
    case RS2::EntityDimRadial:
        break;
    case RS2::EntityDimDiametric:
        break;
    case RS2::EntityDimAngular:
        break;
    case RS2::EntityDimLeader:
        break;
    case RS2::EntityUnknown:
    default:
        break;
    }
    ec->update();
    if(hasContainer && dpi)
        this->dpi->updateEntity(entity, ec);
}

void Plugin_Entity::getPolylineData(QList<Plug_VertexData> *data){
	if (!entity) return;
    RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) return;
    RS_Polyline *l = static_cast<RS_Polyline*>(entity);

    RS_Entity* nextEntity = 0;
	RS_AtomicEntity* ae = nullptr;
    RS_Entity* v = l->firstEntity(RS2::ResolveNone);
    double bulge=0.0;
//bad polyline without vertex
	if (!v) return;

//First polyline vertex
    if (v->rtti() == RS2::EntityArc) {
        bulge = ((RS_Arc*)v)->getBulge();
    }
    ae = (RS_AtomicEntity*)v;
    data->append(Plug_VertexData(QPointF(ae->getStartpoint().x,
                                         ae->getStartpoint().y),bulge));

	for (v=l->firstEntity(RS2::ResolveNone); v; v=nextEntity) {
		nextEntity = l->nextEntity(RS2::ResolveNone);
        bulge = 0.0;
        if (!v->isAtomic()) {
            continue;
        }
        ae = (RS_AtomicEntity*)v;

        if (nextEntity) {
            if (nextEntity->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)nextEntity)->getBulge();
            }
        }

		if (!l->isClosed() || nextEntity) {
            data->append(Plug_VertexData(QPointF(ae->getEndpoint().x,
                                         ae->getEndpoint().y),bulge));
        }
    }

}

void Plugin_Entity::updatePolylineData(QList<Plug_VertexData> *data){
	if (!entity) return;
    RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) return;
    if (data->size()<2) return; //At least two vertex
    RS_Vector vec(false);
    RS_Polyline *pl = static_cast<RS_Polyline*>(entity);
//    vec.x = data->at(0).point.x();
//    vec.y = data->at(0).point.y();
    pl->clear();
    pl->setEndpoint(vec);
    pl->setStartpoint(vec);
    vec.valid = true;
    for (int i = 0; i < data->size(); ++i) {
        vec.x = data->at(i).point.x();
        vec.y = data->at(i).point.y();
        pl->addVertex(vec, data->at(i).bulge );
    }


}

void Plugin_Entity::move(QPointF offset){
    RS_Entity *ne = entity->clone();
    ne->move( RS_Vector(offset.x(), offset.y()) );
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok){
        entity->move( RS_Vector(offset.x(), offset.y()) );
        delete ne;
    } else
        this->entity = ne;
}

void Plugin_Entity::moveRotate(QPointF const& offset, QPointF const& center, double angle)
{
	RS_Entity *ne = entity->clone();
	ne->move( RS_Vector(offset.x(), offset.y()) );
	ne->rotate( RS_Vector(center.x(), center.y()) , angle);
	bool ok = dpi->addToUndo(entity, ne);
	//if doc interface fails to handle for undo only modify original entity
	if (!ok){
		entity->move( RS_Vector(offset.x(), offset.y()) );
		entity->rotate( RS_Vector(center.x(), center.y()) , angle);
		delete ne;
	} else
		this->entity = ne;
}

void Plugin_Entity::rotate(QPointF center, double angle){
    RS_Entity *ne = entity->clone();
    ne->rotate( RS_Vector(center.x(), center.y()) , angle);
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok){
        entity->rotate( RS_Vector(center.x(), center.y()) , angle);
        delete ne;
    } else
        this->entity = ne;
}

void Plugin_Entity::scale(QPointF center, QPointF factor){
    RS_Entity *ne = entity->clone();
    ne->scale( RS_Vector(center.x(), center.y()),
                RS_Vector(factor.x(), factor.y()) );
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok){
        entity->scale( RS_Vector(center.x(), center.y()),
                    RS_Vector(factor.x(), factor.y()) );
        delete ne;
    } else
        this->entity = ne;
}

QString Plugin_Entity::intColor2str(int color){
    return Converter.intColor2str(color);
}

Doc_plugin_interface::Doc_plugin_interface(RS_Document *d, RS_GraphicView* gv, QWidget* parent):
doc(d)
,docGr(doc->getGraphic())
,gView(gv)
,main_window(parent)
{
}

bool Doc_plugin_interface::addToUndo(RS_Entity* current, RS_Entity* modified){
    if (doc) {
        doc->addEntity(modified);
        LC_UndoSection undo(doc);
        if (current->isSelected())
            current->setSelected(false);
        current->changeUndoState();
        undo.addUndoable(current);
        undo.addUndoable(modified);
        return true;
    } else
		RS_DEBUG->print("Doc_plugin_interface::addToUndo: currentContainer is nullptr");
    return false;
}

void Doc_plugin_interface::updateView(){
    doc->setSelected(false);
    gView->getContainer()->calculateBorders();
    gView->redraw();
}

void Doc_plugin_interface::addPoint(QPointF *start){

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        RS_Point* entity = new RS_Point(doc, RS_PointData(v1));
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addPoint: currentContainer is nullptr");
}

void Doc_plugin_interface::addLine(QPointF *start, QPointF *end){

    RS_Vector v1(start->x(), start->y());
    RS_Vector v2(end->x(), end->y());
    if (doc) {
		RS_Line* entity = new RS_Line{doc, v1, v2};
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is nullptr");
}

void Doc_plugin_interface::addMText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va){

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        double width = 100.0;

        RS_MTextData::VAlign valign = static_cast <RS_MTextData::VAlign>(va);
        RS_MTextData::HAlign halign = static_cast <RS_MTextData::HAlign>(ha);
        RS_MTextData d(v1, height, width, valign, halign,
                  RS_MTextData::ByStyle, RS_MTextData::Exact, 0.0,
                  txt, sty, angle, RS2::Update);
        RS_MText* entity = new RS_MText(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addMtext: currentContainer is nullptr");
}

void Doc_plugin_interface::addText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va){

    RS_Vector v1(start->x(), start->y());
    if (doc) {
        double width = 1.0;

        RS_TextData::VAlign valign = static_cast <RS_TextData::VAlign>(va);
        RS_TextData::HAlign halign = static_cast <RS_TextData::HAlign>(ha);
        RS_TextData d(v1, v1, height, width, valign, halign,
                  RS_TextData::None, txt, sty, angle, RS2::Update);
        RS_Text* entity = new RS_Text(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addText: currentContainer is nullptr");
}

void Doc_plugin_interface::addCircle(QPointF *start, qreal radius){
    if (doc) {
        RS_Vector v(start->x(), start->y());
        RS_CircleData d(v, radius);
        RS_Circle* entity = new RS_Circle(doc, d);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addCircle: currentContainer is nullptr");
}

void Doc_plugin_interface::addArc(QPointF *start, qreal radius, qreal a1, qreal a2){
    if (doc) {
        RS_Vector v(start->x(), start->y());
        RS_ArcData d(v, radius,
				 RS_Math::deg2rad(a1),
				 RS_Math::deg2rad(a2),
                 false);
        RS_Arc* entity = new RS_Arc(doc, d);
        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addArc: currentContainer is nullptr");
}

void Doc_plugin_interface::addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2){
    if (doc) {
        RS_Vector v1(start->x(), start->y());
        RS_Vector v2(end->x(), end->y());

		RS_EllipseData ed{v1, v2, ratio, a1, a2, false};
        RS_Ellipse* entity = new RS_Ellipse(doc, ed);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addEllipse: currentContainer is nullptr");
}

void Doc_plugin_interface::addLines(std::vector<QPointF> const& points, bool closed)
{
    if (doc) {
        RS_LineData data;

        LC_UndoSection undo(doc);
        data.endpoint=RS_Vector(points.front().x(), points.front().y());

        for(size_t i=1; i<points.size(); ++i){
            data.startpoint=data.endpoint;
            data.endpoint=RS_Vector(points[i].x(), points[i].y());
            RS_Line* line=new RS_Line(doc, data);
            doc->addEntity(line);
            undo.addUndoable(line);
        }
        if(closed){
            data.startpoint=data.endpoint;
            data.endpoint=RS_Vector(points.front().x(), points.front().y());
            RS_Line* line=new RS_Line(doc, data);
            doc->addEntity(line);
            undo.addUndoable(line);
        }
    } else
		RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
}

void Doc_plugin_interface::addPolyline(std::vector<Plug_VertexData> const& points, bool closed)
{
    if (doc) {
        RS_PolylineData data;
        if(closed)
            data.setFlag(RS2::FlagClosed);
        RS_Polyline* entity = new RS_Polyline(doc, data);

        for(auto const& pt: points){
            entity->addVertex(RS_Vector(pt.point.x(), pt.point.y()), pt.bulge);
        }

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
}

void Doc_plugin_interface::addSplinePoints(std::vector<QPointF> const& points, bool closed)
{
    if (doc) {
        LC_SplinePointsData data(closed, false); //cut = false
        for(auto const& pt: points){
            data.splinePoints.emplace_back(RS_Vector(pt.x(), pt.y()));
        }

        LC_SplinePoints* entity = new LC_SplinePoints(doc, data);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("%s: currentContainer is nullptr", __func__);
}

void Doc_plugin_interface::addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                                    int w, int h, QString name, int br, int con, int fade){
    if (doc) {
        RS_Vector ip(start->x(), start->y());
        RS_Vector uv(uvr->x(), uvr->y());
        RS_Vector vv(vvr->x(), vvr->y());
        RS_Vector size(w, h);

        RS_Image* image =
            new RS_Image(
                doc,
				RS_ImageData(handle /*QString(data.ref.c_str()).toInt(nullptr, 16)*/,
                         ip, uv, vv,
                         size,
                         name,
                         br,
                         con,
                         fade));

        doc->addEntity(image);
        LC_UndoSection undo(doc);
        undo.addUndoable(image);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addImage: currentContainer is nullptr");
}

void Doc_plugin_interface::addInsert(QString name, QPointF ins, QPointF scale, qreal rot){
    if (doc) {
        RS_Vector ip(ins.x(), ins.y());
        RS_Vector sp(scale.x(), scale.y());

        RS_InsertData id(name, ip, sp, rot, 1, 1, RS_Vector(0.0, 0.0));
        RS_Insert* entity = new RS_Insert(doc, id);

        doc->addEntity(entity);
        LC_UndoSection undo(doc);
        undo.addUndoable(entity);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addInsert: currentContainer is nullptr");
}

/*TODO RLZ: add undo support in this method*/
QString Doc_plugin_interface::addBlockfromFromdisk(QString fullName){
	if (fullName.isEmpty() || !doc)
		return nullptr;
    RS_BlockList* blockList = doc->getBlockList();
	if (!blockList)
		return nullptr;

    QFileInfo fi(fullName);
    QString s = fi.completeBaseName();

	QString name = blockList->newName(s);

    if (fi.isReadable()) {
        RS_BlockData d(name, RS_Vector(0,0), false);
        RS_Block *b = new RS_Block(doc, d);
        RS_Graphic g;
        if (!g.open(fi.absoluteFilePath(), RS2::FormatUnknown)) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "Doc_plugin_interface::addBlockfromFromdisk: Cannot open file: %s");
            delete b;
			return nullptr;
        }
        RS_LayerList* ll = g.getLayerList();
        for (unsigned int i = 0; i<ll->count(); i++){
            RS_Layer* nl = ll->at(i)->clone();
            docGr->addLayer(nl);
        }
        RS_BlockList* bl = g.getBlockList();
        for (int i = 0; i<bl->count(); i++){
            RS_Block* nb = (RS_Block*)bl->at(i)->clone();
            docGr->addBlock(nb);
        }
        for (unsigned int i = 0; i<g.count(); i++){
            RS_Entity* e = g.entityAt(i)->clone();
            e->reparent(b);
            b->addEntity(e);
        }
        docGr->addBlock(b);
        return name;

    } else {
		return nullptr;
    }
}

void Doc_plugin_interface::addEntity(Plug_Entity *handle){
    if (doc) {
        RS_Entity *ent = (reinterpret_cast<Plugin_Entity*>(handle))->getEnt();
		if (ent) {
            doc->addEntity(ent);
            LC_UndoSection undo(doc);
            undo.addUndoable(ent);
        }
    } else
		RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is nullptr");
}

/*newEntity not added into graphic, then not needed undo support*/
Plug_Entity *Doc_plugin_interface::newEntity( enum DPI::ETYPE type){
    Plugin_Entity *e = new Plugin_Entity(doc, type);
    if( !(e->isValid()) ) {
        delete e;
		return nullptr;
    }
    return  reinterpret_cast<Plug_Entity*>(e);
}

/*TODO RLZ: add undo support in this method*/
void Doc_plugin_interface::removeEntity(Plug_Entity *ent){
    RS_Entity *e = (reinterpret_cast<Plugin_Entity*>(ent))->getEnt();
    if (doc && e) {
        LC_UndoSection undo(doc);
        e->setSelected(false);
        e->changeUndoState();
        undo.addUndoable(e);

        gView->redraw(RS2::RedrawDrawing);
    }
}

void Doc_plugin_interface::updateEntity(RS_Entity *org, RS_Entity *newe){
    if (doc) {
        LC_UndoSection undo(doc);
        doc->addEntity(newe);
        undo.addUndoable(newe);
        undo.addUndoable(org);
        org->setUndoState(true);
    } else
		RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is nullptr");
}

/*TODO RLZ: add undo support in the remaining methods*/
void Doc_plugin_interface::setLayer(QString name){
    RS_LayerList* listLay = doc->getLayerList();
    RS_Layer *lay = listLay->find(name);
	if (!lay) {
        lay = new RS_Layer(name);
        docGr->addLayer(lay);
    }
    listLay->activate(lay, true);
}

QString Doc_plugin_interface::getCurrentLayer(){
    return docGr->getActiveLayer()->getName();
}

QStringList Doc_plugin_interface::getAllLayer(){
    QStringList listName;
    RS_LayerList* listLay = doc->getLayerList();
    for (unsigned int i = 0; i < listLay->count(); ++i) {
         listName << listLay->at(i)->getName();
     }
    return listName;
}

QStringList Doc_plugin_interface::getAllBlocks(){
    QStringList listName;
    RS_BlockList* listBlk = doc->getBlockList();
    for (int i = 0; i < listBlk->count(); ++i) {
         listName << listBlk->at(i)->getName();
     }
    return listName;
}

bool Doc_plugin_interface::deleteLayer(QString name){
    RS_Layer* layer = docGr->findLayer(name);
	if (layer) {
        docGr->removeLayer(layer);
        return true;
    }
    return false;
}

void Doc_plugin_interface::getCurrentLayerProperties(int *c, DPI::LineWidth *w, DPI::LineType *t){
    RS_Pen pen = docGr->getActiveLayer()->getPen();
    *c = pen.getColor().toIntColor();
//    RS_Color col = pen.getColor();
//    c->setRgb(col.red(), col.green(), col.blue());
    *w = static_cast<DPI::LineWidth>(pen.getWidth());
    *t = static_cast<DPI::LineType>(pen.getLineType());
}

void Doc_plugin_interface::getCurrentLayerProperties(int *c, QString *w, QString *t){
    RS_Pen pen = docGr->getActiveLayer()->getPen();
    *c = pen.getColor().toIntColor();
//    RS_Color col = pen.getColor();
//    c->setRgb(col.red(), col.green(), col.blue());
    w->clear();
    w->append(Converter.lw2str(pen.getWidth()));
    t->clear();
    t->append(Converter.lt2str(pen.getLineType()));
}

void Doc_plugin_interface::setCurrentLayerProperties(int c, DPI::LineWidth w, DPI::LineType t){
    RS_Layer* layer = docGr->getActiveLayer();
	if (layer) {
        RS_Color co;
        co.fromIntColor(c);
        RS_Pen pen(co, static_cast<RS2::LineWidth>(w), static_cast<RS2::LineType>(t));
//        RS_Pen pen(RS_Color(c), static_cast<RS2::LineWidth>(w), static_cast<RS2::LineType>(t));
        layer->setPen(pen);
    }
}

void Doc_plugin_interface::setCurrentLayerProperties(int c, QString const& w,
													 QString const& t){
    RS_Layer* layer = docGr->getActiveLayer();
	if (layer) {
        RS_Color co;
        co.fromIntColor(c);
        RS_Pen pen(co, Converter.str2lw(w), Converter.str2lt(t));
//        RS_Pen pen(RS_Color(c), Converter.str2lw(w), Converter.str2lt(t));
        layer->setPen(pen);
    }
}

bool Doc_plugin_interface::getPoint(QPointF *point, const QString& mesage,
									QPointF *base){
    bool status = false;
    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *gView);
    if (a) {
        if (!(mesage.isEmpty()) ) a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        if (base) a->setBasepoint(base);
        QEventLoop ev;
        while (!a->isCompleted()) {
            ev.processEvents ();
            if (!gView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        gView->killAllActions();
    }
    return status;
}

Plug_Entity *Doc_plugin_interface::getEnt(const QString& mesage){
    QC_ActionGetEnt* a = new QC_ActionGetEnt(*doc, *gView);
    if (a) {
        if (!(mesage.isEmpty()) )
            a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!gView->getEventHandler()->hasAction())
                break;
        }
    }
    Plug_Entity *e = reinterpret_cast<Plug_Entity*>(a->getSelected(this));
    a->finish();
    gView->killAllActions();
    return e;
}

bool Doc_plugin_interface::getSelect(QList<Plug_Entity *> *sel, const QString& mesage){
    bool status = false;
    QC_ActionGetSelect* a = new QC_ActionGetSelect(*doc, *gView);
    if (a) {
        if (!(mesage.isEmpty()) )
            a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!gView->getEventHandler()->hasAction())
                break;
        }
        // qDebug() << "getSelect: passed event loop";
    }
//    check if a are cancelled by the user issue #349
    RS_EventHandler* eh = gView->getEventHandler();
    if (eh && eh->isValid(a) ) {
        a->getSelected(sel, this);
        status = true;
    }
    gView->killAllActions();
    return status;

}

bool Doc_plugin_interface::getAllEntities(QList<Plug_Entity *> *sel, bool visible){
    bool status = false;

	for(auto e: *doc){

        if (e->isVisible() || !visible) {
            Plugin_Entity *pe = new Plugin_Entity(e, this);
            sel->append(reinterpret_cast<Plug_Entity*>(pe));
        }
    }
    status = true;
    return status;
}

bool Doc_plugin_interface::getVariableInt(const QString& key, int *num){
    if( (*num = docGr->getVariableInt(key, 0)) )
        return true;
    else
        return false;
}

bool Doc_plugin_interface::getVariableDouble(const QString& key, double *num){
    if( (*num = docGr->getVariableDouble(key, 0.0)) )
        return true;
    else
        return false;
}

bool Doc_plugin_interface::addVariable(const QString& key, int value, int code){
    docGr->addVariable(key, value, code);
    if (key.startsWith("$DIM"))
        doc->updateDimensions(true);
    return true;
}

bool Doc_plugin_interface::addVariable(const QString& key, double value, int code){
   docGr->addVariable(key, value, code);
   if (key.startsWith("$DIM"))
       doc->updateDimensions(true);
   return true;
}

bool Doc_plugin_interface::getInt(int *num, const QString& mesage, const QString& title){
    bool ok;
    QString msg, tit;
    if ( mesage.isEmpty() )
        msg = QObject::tr("enter an integer number");
    else
        msg = mesage;
    if ( title.isEmpty() )
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    int data = QInputDialog::getInt(main_window, tit, msg, 0, -2147483647, 2147483647, 1, &ok);

    if (ok)
        *num = data;
    return ok;
}
bool Doc_plugin_interface::getReal(qreal *num, const QString& mesage, const QString& title){
    bool ok;
    QString msg, tit;
    if ( mesage.isEmpty() )
        msg = QObject::tr("enter a number");
    else
        msg = mesage;
    if ( title.isEmpty() )
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    double data = QInputDialog::getDouble(main_window, tit, msg, 0, -2147483647, 2147483647, 4, &ok);
    if (ok )
        *num = data;
    return ok;
}
bool Doc_plugin_interface::getString(QString *txt, const QString& mesage, const QString& title){
    bool ok;
    QString msg, tit;
    if ( mesage.isEmpty() )
        msg = QObject::tr("enter text");
    else
        msg = mesage;
    if ( title.isEmpty() )
        tit = QObject::tr("LibreCAD query");
    else
        tit = title;

    QString text = QInputDialog::getText(main_window, tit,msg, QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
        txt->clear();
        txt->append(text);
    }
    return ok;
}

QString Doc_plugin_interface::realToStr(const qreal num, const int units, const int prec){
    RS2::LinearFormat lf;
    int pr = prec;
    if (pr == 0)
        pr = docGr->getLinearPrecision();

    switch (units){
    case 0:
        lf = docGr->getLinearFormat();
        break;
    case 1:
        lf = RS2::Scientific;
        break;
    case 3:
        lf = RS2::Engineering;
        break;
    case 4:
        lf = RS2::Architectural;
        break;
    case 5:
        lf = RS2::Fractional;
        break;
    case 6:
        lf = RS2::ArchitecturalMetric;
        break;
    default:
        lf = RS2::Decimal;
    }

    QString msg = RS_Units::formatLinear(num,RS2::None,lf,pr);
    return msg;
}
