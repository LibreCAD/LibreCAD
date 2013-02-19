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
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_image.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_polyline.h"
#include "rs_ellipse.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetselect.h"
#include "intern/qc_actiongetent.h"

#if QT_VERSION < 0x040500
#include "emu_qt45.h"
#endif

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


convLTW Converter;


Plugin_Entity::Plugin_Entity(RS_Entity* ent, Doc_plugin_interface* d) {
    entity = ent;
    hasContainer = true;
    dpi = d;
}

/*RS_EntityContainer* parent,
                 const RS_LineData& d*/
Plugin_Entity::Plugin_Entity(RS_EntityContainer* parent, enum DPI::ETYPE type){
    hasContainer = false;
    dpi = NULL;
    entity = NULL;
    switch (type) {
    case DPI::POINT:
        entity = new RS_Point(parent, RS_PointData(RS_Vector(0,0)));
        break;
    case DPI::LINE:
        entity = new RS_Line(parent, RS_LineData());
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
        entity = new RS_Ellipse(parent, RS_EllipseData(RS_Vector(0,0), RS_Vector(0,0),0.0,0.0,0.0,false));
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
    if (entity == NULL) return;
    RS2::EntityType et = entity->rtti();
    data->insert(DPI::EID, (qulonglong)entity->getId());
    data->insert(DPI::LAYER, entity->getLayer()->getName() );
    data->insert(DPI::LTYPE, Converter.lt2str(entity->getPen().getLineType()) );
    data->insert(DPI::LWIDTH, Converter.lw2str(entity->getPen().getWidth()) );
    data->insert(DPI::COLOR, entity->getPen().getColor() );
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
    if (entity == NULL) return;
    RS_Entity *ec= entity;
    if(hasContainer && dpi!=NULL) {
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
        QColor color = hash.take(DPI::COLOR).value<QColor>();
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
    if(hasContainer && dpi!=NULL)
        this->dpi->updateEntity(entity, ec);
}

void Plugin_Entity::getPolylineData(QList<Plug_VertexData> *data){
    if (entity == NULL) return;
    RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) return;
    RS_Polyline *l = static_cast<RS_Polyline*>(entity);

    RS_Entity* nextEntity = 0;
    RS_AtomicEntity* ae = NULL;
    RS_Entity* v = l->firstEntity(RS2::ResolveNone);
    double bulge=0.0;
//bad polyline without vertex
    if (v == NULL) return;

//First polyline vertex
    if (v->rtti() == RS2::EntityArc) {
        bulge = ((RS_Arc*)v)->getBulge();
    }
    ae = (RS_AtomicEntity*)v;
    data->append(Plug_VertexData(QPointF(ae->getStartpoint().x,
                                         ae->getStartpoint().y),bulge));

    for (v=l->firstEntity(RS2::ResolveNone); v!=NULL; v=nextEntity) {
        nextEntity = l->nextEntity(RS2::ResolveNone);
        bulge = 0.0;
        if (!v->isAtomic()) {
            continue;
        }
        ae = (RS_AtomicEntity*)v;

        if (nextEntity!=NULL) {
            if (nextEntity->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)nextEntity)->getBulge();
            }
        }

        if (l->isClosed()==false || nextEntity!=NULL) {
            data->append(Plug_VertexData(QPointF(ae->getEndpoint().x,
                                         ae->getEndpoint().y),bulge));
        }
    }

}

void Plugin_Entity::updatePolylineData(QList<Plug_VertexData> *data){
    if (entity == NULL) return;
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

Doc_plugin_interface::Doc_plugin_interface(RS_Graphic *d, RS_GraphicView* gv, QWidget* parent){
    doc =d;
    gView =gv;
    main = parent;
    haveUndo = false;
}

Doc_plugin_interface::~Doc_plugin_interface(){
    if (haveUndo) {
        doc->endUndoCycle();
    }
}

bool Doc_plugin_interface::addToUndo(RS_Entity* current, RS_Entity* modified){
    if (doc!=NULL) {
        doc->addEntity(modified);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        if (current->isSelected())
            current->setSelected(false);
        current->changeUndoState();
        doc->addUndoable(current);
        doc->addUndoable(modified);
        return true;
    } else
        RS_DEBUG->print("Doc_plugin_interface::addToUndo: currentContainer is NULL");
    return false;
}

void Doc_plugin_interface::updateView(){
    doc->setSelected(false);
    gView->redraw();
}

void Doc_plugin_interface::addPoint(QPointF *start){

    RS_Vector v1(start->x(), start->y());
    if (doc!=NULL) {
        RS_Point* entity = new RS_Point(doc, RS_PointData(v1));
        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addPoint: currentContainer is NULL");
}

void Doc_plugin_interface::addLine(QPointF *start, QPointF *end){

    RS_Vector v1(start->x(), start->y());
    RS_Vector v2(end->x(), end->y());
    if (doc!=NULL) {
        RS_Line* entity = new RS_Line(doc, RS_LineData(v1, v2));
        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is NULL");
}

void Doc_plugin_interface::addMText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va){

    RS_Vector v1(start->x(), start->y());
    if (doc!=NULL) {
        double width = 100.0;

        RS_MTextData::VAlign valign = static_cast <RS_MTextData::VAlign>(va);
        RS_MTextData::HAlign halign = static_cast <RS_MTextData::HAlign>(ha);
        RS_MTextData d(v1, height, width, valign, halign,
                  RS_MTextData::ByStyle, RS_MTextData::Exact, 0.0,
                  txt, sty, angle, RS2::Update);
        RS_MText* entity = new RS_MText(doc, d);

        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addMtext: currentContainer is NULL");
}

void Doc_plugin_interface::addText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va){

    RS_Vector v1(start->x(), start->y());
    if (doc!=NULL) {
        double width = 1.0;

        RS_TextData::VAlign valign = static_cast <RS_TextData::VAlign>(va);
        RS_TextData::HAlign halign = static_cast <RS_TextData::HAlign>(ha);
        RS_TextData d(v1, v1, height, width, valign, halign,
                  RS_TextData::None, txt, sty, angle, RS2::Update);
        RS_Text* entity = new RS_Text(doc, d);

        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addText: currentContainer is NULL");
}

void Doc_plugin_interface::addCircle(QPointF *start, qreal radius){
    if (doc!=NULL) {
        RS_Vector v(start->x(), start->y());
        RS_CircleData d(v, radius);
        RS_Circle* entity = new RS_Circle(doc, d);

        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addCircle: currentContainer is NULL");
}

void Doc_plugin_interface::addArc(QPointF *start, qreal radius, qreal a1, qreal a2){
    if (doc!=NULL) {
        RS_Vector v(start->x(), start->y());
        RS_ArcData d(v, radius,
                 a1/ARAD,
                 a2/ARAD,
                 false);
        RS_Arc* entity = new RS_Arc(doc, d);
        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addArc: currentContainer is NULL");
}

void Doc_plugin_interface::addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2){
    if (doc!=NULL) {
        RS_Vector v1(start->x(), start->y());
        RS_Vector v2(end->x(), end->y());

        RS_EllipseData ed(v1, v2, ratio,
                      a1, a2, false);
        RS_Ellipse* entity = new RS_Ellipse(doc, ed);

        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addEllipse: currentContainer is NULL");
}

void Doc_plugin_interface::addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                                    int w, int h, QString name, int br, int con, int fade){
    if (doc!=NULL) {
        RS_Vector ip(start->x(), start->y());
        RS_Vector uv(uvr->x(), uvr->y());
        RS_Vector vv(vvr->x(), vvr->y());
        RS_Vector size(w, h);

        RS_Image* image =
            new RS_Image(
                doc,
                RS_ImageData(handle /*QString(data.ref.c_str()).toInt(NULL, 16)*/,
                         ip, uv, vv,
                         size,
                         name,
                         br,
                         con,
                         fade));

        doc->addEntity(image);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(image);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addImage: currentContainer is NULL");
}

void Doc_plugin_interface::addInsert(QString name, QPointF ins, QPointF scale, qreal rot){
    if (doc!=NULL) {
        RS_Vector ip(ins.x(), ins.y());
        RS_Vector sp(scale.x(), scale.y());

        RS_InsertData id(name, ip, sp, rot, 1, 1, RS_Vector(0.0, 0.0));
        RS_Insert* entity = new RS_Insert(doc, id);

        doc->addEntity(entity);
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        doc->addUndoable(entity);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addInsert: currentContainer is NULL");
}

/*TODO RLZ: add undo support in this method*/
QString Doc_plugin_interface::addBlockfromFromdisk(QString fullName){
    if (fullName.isEmpty() || doc==NULL)
        return NULL;
    RS_BlockList* blockList = doc->getBlockList();
    if (blockList==NULL)
        return NULL;

    QFileInfo fi(fullName);
    QString s = fi.completeBaseName();

    QString name = s;
    if(blockList->find(name)){
        for (int i=0; i<1e5; ++i) {
            name = QString("%1-%2").arg(s).arg(i);
            if (blockList->find(name)==NULL) {
                break;
            }
        }
    }

    if (fi.isReadable()) {
        RS_BlockData d(name, RS_Vector(0,0), false);
        RS_Block *b = new RS_Block(doc, d);
        RS_Graphic g;
        if (!g.open(fi.absoluteFilePath(), RS2::FormatUnknown)) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "Doc_plugin_interface::addBlockfromFromdisk: Cannot open file: %s");
            delete b;
            return NULL;
        }
        RS_LayerList* ll = g.getLayerList();
        for (int i = 0; i<ll->count(); i++){
            RS_Layer* nl = ll->at(i)->clone();
            doc->addLayer(nl);
        }
        RS_BlockList* bl = g.getBlockList();
        for (int i = 0; i<bl->count(); i++){
            RS_Block* nb = (RS_Block*)bl->at(i)->clone();
            doc->addBlock(nb);
        }
        for (int i = 0; i<g.count(); i++){
            RS_Entity* e = g.entityAt(i)->clone();
            e->reparent(b);
            b->addEntity(e);
        }
        doc->addBlock(b);
        return name;

    } else {
        return NULL;
    }
}

void Doc_plugin_interface::addEntity(Plug_Entity *handle){
    if (doc!=NULL) {
        RS_Entity *ent = (reinterpret_cast<Plugin_Entity*>(handle))->getEnt();
        if (ent != NULL) {
            doc->addEntity(ent);
            if (!haveUndo) {
                doc->startUndoCycle();
                haveUndo = true;
            }
            doc->addUndoable(ent);
        }
    } else
        RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is NULL");
}

/*newEntity not added into graphic, then not needed undo support*/
Plug_Entity *Doc_plugin_interface::newEntity( enum DPI::ETYPE type){
    Plugin_Entity *e = new Plugin_Entity(doc, type);
    if( !(e->isValid()) ) {
        delete e;
        return NULL;
    }
    return  reinterpret_cast<Plug_Entity*>(e);
}

/*TODO RLZ: add undo support in this method*/
void Doc_plugin_interface::removeEntity(Plug_Entity *ent){
    RS_Entity *e = (reinterpret_cast<Plugin_Entity*>(ent))->getEnt();
    if (doc!=NULL && e!=NULL) {
        if (!haveUndo) {
            doc->startUndoCycle();
            haveUndo = true;
        }
        e->setSelected(false);
        e->changeUndoState();
        doc->addUndoable(e);

        gView->redraw(RS2::RedrawDrawing);
    }
}

void Doc_plugin_interface::updateEntity(RS_Entity *org, RS_Entity *newe){
    if (doc!=NULL) {
            if (!haveUndo) {
                doc->startUndoCycle();
                haveUndo = true;
            }
            doc->addEntity(newe);
            doc->addUndoable(newe);
            doc->addUndoable(org);
            org->setUndoState(true);
    } else
        RS_DEBUG->print("Doc_plugin_interface::addEntity: currentContainer is NULL");
}

/*TODO RLZ: add undo support in the remaining methods*/
void Doc_plugin_interface::setLayer(QString name){
    RS_LayerList* listLay = doc->getLayerList();
    RS_Layer *lay = listLay->find(name);
    if (lay == NULL) {
        lay = new RS_Layer(name);
        doc->addLayer(lay);
    }
    listLay->activate(lay, true);
}

QString Doc_plugin_interface::getCurrentLayer(){
    return doc->getActiveLayer()->getName();
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
    RS_Layer* layer = doc->findLayer(name);
    if (layer != NULL) {
        doc->removeLayer(layer);
        return true;
    }
    return false;
}

bool Doc_plugin_interface::getPoint(QPointF *point, const QString& mesage, QPointF *base){
    bool status = false;
    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *gView);
    if (a!=NULL) {
        if (!(mesage.isEmpty()) ) a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        if (base) a->setBasepoint(base);
        QEventLoop ev;
        while (gView->getCurrentAction() ==a)
        {
            ev.processEvents (QEventLoop::ExcludeSocketNotifiers);
        }
        if (a->isCompleted() ){
        a->getPoint(point);
        status = true;}
//RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        gView->killAllActions();
    }
    return status;
}

Plug_Entity *Doc_plugin_interface::getEnt(const QString& mesage){
    QC_ActionGetEnt* a = new QC_ActionGetEnt(*doc, *gView);
    if (a!=NULL) {
        if (!(mesage.isEmpty()) )
            a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        QEventLoop ev;
        while ( !a->isCompleted())
        {
            ev.processEvents ();
        }
    }
    Plug_Entity *e = reinterpret_cast<Plug_Entity*>(a->getSelected(this));
    gView->killAllActions();
    return e;
}

bool Doc_plugin_interface::getSelect(QList<Plug_Entity *> *sel, const QString& mesage){
    bool status = false;
    QC_ActionGetSelect* a = new QC_ActionGetSelect(*doc, *gView);
    if (a!=NULL) {
        if (!(mesage.isEmpty()) )
            a->setMesage(mesage);
        gView->killAllActions();
        gView->setCurrentAction(a);
        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
        }
    }
//    check if a are cancelled by the user issue #349
    RS_EventHandler* eh = gView->getEventHandler();
    if (eh!=NULL && eh->isValid(a) ) {
        a->getSelected(sel, this);
        status = true;
    }
    gView->killAllActions();
    return status;

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

#if QT_VERSION < 0x040500
    int data = emu_qt45_QInputDialog_getInt(main, tit, msg, 0, -2147483647, 2147483647, 1, &ok);
#else
    int data = QInputDialog::getInt(main, tit, msg, 0, -2147483647, 2147483647, 1, &ok);
#endif

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

    double data = QInputDialog::getDouble(main, tit, msg, 0, -2147483647, 2147483647, 4, &ok);
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

    QString text = QInputDialog::getText(main, tit,msg, QLineEdit::Normal,
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
        pr = doc->getLinearPrecision();

    switch (units){
    case 0:
        lf = doc->getLinearFormat();
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
    default:
        lf = RS2::Decimal;
    }

    QString msg = RS_Units::formatLinear(num,RS2::None,lf,pr);
    return msg;
}

