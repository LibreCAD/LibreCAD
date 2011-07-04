/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
**
** This program is free software; you can redistribute it and/or modify
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
#include "rs_graphicview.h"
#include "rs_actioninterface.h"
#include "rs_actionselect.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetselect.h"
#include "intern/qc_actiongetent.h"

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


Plugin_Entity::Plugin_Entity(RS_Entity* ent) {
    entity = ent;
    hasContainer = true;
}

/*RS_EntityContainer* parent,
                 const RS_LineData& d*/
Plugin_Entity::Plugin_Entity(RS_EntityContainer* parent, enum DPI::ETYPE type){
    hasContainer = false;
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
/*    case DPI::ELLIPSE:
        entity = new RS_Ellipse(parent, RS_EllipseData());
        break;*/
    case DPI::IMAGE:
        entity = new RS_Image(parent, RS_ImageData());
        break;
/*    case DPI::OVERLAYBOX:
        entity = new RS_OverlayBox();
        break;
    case DPI::SOLID:
        entity = new RS_Solid();
        break;*/
    case DPI::TEXT:
        entity = new RS_Text(parent, RS_TextData());
        break;
/*    case DPI::INSERT:
        entity = new RS_Insert();
        break;
    case DPI::POLYLINE:
        entity = new RS_Polyline();
        break;
    case DPI::SPLINE:
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
        break;}
    case RS2::EntityCircle: {
        data->insert(DPI::ETYPE, DPI::CIRCLE);
        RS_CircleData d = static_cast<RS_Circle*>(entity)->getData();
        data->insert(DPI::STARTX, d.center.x );
        data->insert(DPI::STARTY, d.center.y );
        data->insert(DPI::RADIUS, d.radius );
        break;}
    case RS2::EntityEllipse: { //TODO
        data->insert(DPI::ETYPE, DPI::ELLIPSE);
//        RS_EllipseData d = static_cast<RS_Ellipse*>(entity)->getData();
        RS_Ellipse *dd = static_cast<RS_Ellipse*>(entity);
        data->insert(DPI::STARTX, dd->getCenter().x );//10
        data->insert(DPI::STARTY, dd->getCenter().y );//20
        data->insert(DPI::ENDX, dd->getMajorP().x );//11 pto final eje mayor
        data->insert(DPI::ENDY, dd->getMajorP().y );//21 pto final eje mayor
        data->insert(DPI::HEIGHT, dd->getRatio() );//40 ratio eje menor/mayor
        break;}
    case RS2::EntitySolid:
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
    case RS2::EntityText: {
        data->insert(DPI::ETYPE, DPI::TEXT);
        RS_TextData d = static_cast<RS_Text*>(entity)->getData();
        data->insert(DPI::STARTX, d.insertionPoint.x );
        data->insert(DPI::STARTY, d.insertionPoint.y );
        data->insert(DPI::STARTANGLE, d.angle );
        data->insert(DPI::HEIGHT, d.height );
        break;}
    case RS2::EntityHatch:
        data->insert(DPI::ETYPE, DPI::HATCH);
        break;
    case RS2::EntitySpline:
        data->insert(DPI::ETYPE, DPI::SPLINE);
        break;
    case RS2::EntityPolyline:
        data->insert(DPI::ETYPE, DPI::POLYLINE);
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
    QHash<int, QVariant> hash = *data;
    QString str;
    RS_Vector vec;
    RS_Pen epen = entity->getPen();
//    double num;
    if (hash.contains(DPI::LAYER)) {
        str = (hash.take(DPI::LAYER)).toString();
        entity->setLayer(str);
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
    entity->setPen(epen);

    RS2::EntityType et = entity->rtti();
    switch (et) {
    //atomicEntity
    case RS2::EntityLine: {
        vec = static_cast<RS_Line*>(entity)->getStartpoint();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        static_cast<RS_Line*>(entity)->setStartpoint(vec);
        vec = static_cast<RS_Line*>(entity)->getEndpoint();
        if (hash.contains(DPI::ENDX)) {
            vec.x = (hash.take(DPI::ENDX)).toDouble();
        }
        if (hash.contains(DPI::ENDY)) {
            vec.y = (hash.take(DPI::ENDY)).toDouble();
        }
        static_cast<RS_Line*>(entity)->setEndpoint(vec);
        break;}
    case RS2::EntityPoint: {
        vec = static_cast<RS_Point*>(entity)->getPos();
        if (hash.contains(DPI::STARTX)) {
            vec.x = (hash.take(DPI::STARTX)).toDouble();
        }
        if (hash.contains(DPI::STARTY)) {
            vec.y = (hash.take(DPI::STARTY)).toDouble();
        }
        static_cast<RS_Point*>(entity)->setPos(vec);
        break; }
    case RS2::EntityArc: {
        RS_Arc *arc = static_cast<RS_Arc*>(entity);
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
        RS_Circle *cir = static_cast<RS_Circle*>(entity);
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
    case RS2::EntityEllipse: { //TODO
        break;}
    case RS2::EntitySolid:
        //Only used in dimensions ?
        break;
    case RS2::EntityConstructionLine:
        //Unused ?
        break;
    case RS2::EntityImage: {
        break;}
    case RS2::EntityOverlayBox:
        //Unused ?
        break;
//EntityContainer
    case RS2::EntityInsert: {
        break;}
    case RS2::EntityText: {
        break;}
    case RS2::EntityHatch:
        break;
    case RS2::EntitySpline:
        break;
    case RS2::EntityPolyline:
        break;
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
}


void Plugin_Entity::move(QPointF offset){
    entity->move( RS_Vector(offset.x(), offset.y()) );
}
void Plugin_Entity::rotate(QPointF center, double angle){
    entity->rotate( RS_Vector(center.x(), center.y()) , angle);
}
void Plugin_Entity::scale(QPointF center, QPointF factor){
    entity->scale( RS_Vector(center.x(), center.y()),
                RS_Vector(factor.x(), factor.y()) );
}


Doc_plugin_interface::Doc_plugin_interface(RS_Graphic *d, RS_GraphicView* gv, QWidget* parent)
{
    doc =d;
    gView =gv;
    main = parent;
}

void Doc_plugin_interface::updateView(){
    doc->setSelected(false);
    gView->redraw();
}

void Doc_plugin_interface::addPoint(QPointF *start){

    RS_Vector v1(start->x(), start->y());
    if (doc==NULL) {
        RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is NULL");
    }

    RS_Point* entity = new RS_Point(doc, RS_PointData(v1));
//    setEntityAttributes(entity, attributes);
    doc->addEntity(entity);
}

void Doc_plugin_interface::addLine(QPointF *start, QPointF *end){

    RS_Vector v1(start->x(), start->y());
    RS_Vector v2(end->x(), end->y());
    if (doc==NULL) {
        RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is NULL");
    }

    RS_Line* entity = new RS_Line(doc, RS_LineData(v1, v2));
//    setEntityAttributes(entity, attributes);
    doc->addEntity(entity);
}

void Doc_plugin_interface::addText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va){

    RS_Vector v1(start->x(), start->y());
    if (doc==NULL) {
        RS_DEBUG->print("Doc_plugin_interface::addLine: currentContainer is NULL");
    }
    double width = 100.0;

    RS2::VAlign valign = static_cast <RS2::VAlign>(va);
    RS2::HAlign halign = static_cast <RS2::HAlign>(ha);
    RS_TextData d(v1, height, width, valign, halign,
                  RS2::ByStyle, RS2::Exact, 0.0,
                  txt, sty, angle, RS2::Update);
    RS_Text* entity = new RS_Text(doc, d);

//    setEntityAttributes(entity, attributes);
    doc->addEntity(entity);
}

void Doc_plugin_interface::addCircle(QPointF *start, qreal radius){
    RS_DEBUG->print("RS_FilterDXF::addCircle");

    RS_Vector v(start->x(), start->y());
    RS_CircleData d(v, radius);
    RS_Circle* entity = new RS_Circle(doc, d);
//    setEntityAttributes(entity, attributes);
    doc->addEntity(entity);
}
void Doc_plugin_interface::addArc(QPointF *start, qreal radius, qreal a1, qreal a2){
    RS_Vector v(start->x(), start->y());
    RS_ArcData d(v, radius,
                 a1/ARAD,
                 a2/ARAD,
                 false);
    RS_Arc* entity = new RS_Arc(doc, d);
//    setEntityAttributes(entity, attributes);
    doc->addEntity(entity);
}
void Doc_plugin_interface::addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2){

/*    RS_Vector v1(data.cx, data.cy);
    RS_Vector v2(data.mx, data.my);*/
    RS_Vector v1(start->x(), start->y());
    RS_Vector v2(end->x(), end->y());

    RS_EllipseData ed(v1, v2, ratio,
                      a1, a2, false);
    RS_Ellipse* entity = new RS_Ellipse(doc, ed);
//    setEntityAttributes(entity, attributes);

    doc->addEntity(entity);
}

void Doc_plugin_interface::addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                                    int w, int h, QString name, int br, int con, int fade){
    RS_Vector ip(start->x(), start->y());
    RS_Vector uv(uvr->x(), uvr->y());
    RS_Vector vv(vvr->x(), vvr->y());
    RS_Vector size(w, h);

    RS_Image* image =
        new RS_Image(
            doc,
            RS_ImageData(handle /*RS_String(data.ref.c_str()).toInt(NULL, 16)*/,
                         ip, uv, vv,
                         size,
                         name,
                         br,
                         con,
                         fade));

//    setEntityAttributes(image, attributes);
    doc->addEntity(image);
}

void Doc_plugin_interface::addEntity(Plug_Entity *handle){
    RS_Entity *ent = (reinterpret_cast<Plugin_Entity*>(handle))->getEnt();
    if (ent != NULL)
        doc->addEntity(ent);
}
Plug_Entity *Doc_plugin_interface::newEntity( enum DPI::ETYPE type){
    Plugin_Entity *e = new Plugin_Entity(doc, type);
    if( !(e->isValid()) ) {
        delete e;
        return NULL;
    }
    return  reinterpret_cast<Plug_Entity*>(e);
}

void Doc_plugin_interface::removeEntity(Plug_Entity *ent){
    RS_Entity *e = (reinterpret_cast<Plugin_Entity*>(ent))->getEnt();
    if (doc!=NULL) {
        doc->startUndoCycle();
        if (e!=NULL) {
            e->setSelected(false);
            e->changeUndoState();
            doc->addUndoable(e);
        }
        doc->endUndoCycle();
        gView->redraw(RS2::RedrawDrawing);
    }
}

void Doc_plugin_interface::setLayer(QString name){
    RS_Layer *lay = new RS_Layer(name);
    doc->addLayer(lay);
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
        a->getPoint(point);
        status = true;
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
    Plug_Entity *e = reinterpret_cast<Plug_Entity*>(a->getSelected());
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
//    QList<Plug_Entity *> *se = new QList<Plug_Entity *>();
    a->getSelected(sel);
    status = true;
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

    int data = QInputDialog::getInt(main, tit, msg, 0, -2147483647, 2147483647, 1, &ok);
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
