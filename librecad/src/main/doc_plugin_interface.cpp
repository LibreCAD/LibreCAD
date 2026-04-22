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
#include <QFileInfo>
#include <QInputDialog>
#include <QList>

#include "lc_action_select_single.h"
#include "lc_actioncontext.h"
#include "lc_containertraverser.h"
#include "lc_documentsstorage.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_units.h"
#include "intern/qc_actiongetent.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetselect.h"

convLTW::convLTW() {
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

QString convLTW::lt2str(const RS2::LineType lt) const {
    return lType.value(lt, "BYLAYER");
}

QString convLTW::lw2str(const RS2::LineWidth lw) const {
    return lWidth.value(lw, "BYDEFAULT");
}

RS2::LineType convLTW::str2lt(const QString& s) const {
    return lType.key(s, RS2::LineByLayer);
}

RS2::LineWidth convLTW::str2lw(const QString& w) const {
    return lWidth.key(w, RS2::WidthDefault);
}

QString convLTW::intColor2str(const int col) {
    switch (col) {
        case -1:
            return "BYLAYER";
        case -2:
            return "BYBLOCK";
        default:
            return QString::number(col >> 16) + ", " + QString::number((col >> 8) & 0xFF) + ", " + QString::number(col & 0xFF);
    }
}

convLTW Converter;

Plugin_Entity::Plugin_Entity(RS_Entity* ent, Doc_plugin_interface* d) : entity(ent), hasContainer(true), dpi(d) {
}

/*RS_EntityContainer* parent,
                 const RS_LineData& d*/
Plugin_Entity::Plugin_Entity(RS_EntityContainer* parent, const enum DPI::ETYPE type) {
    hasContainer = false;
    dpi = nullptr;
    entity = nullptr;
    switch (type) {
        case DPI::POINT:
            entity = new RS_Point(parent, RS_PointData(RS_Vector(0, 0)));
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
            entity = new RS_Ellipse{parent, {{0., 0.}, {0., 0.}, 0., 0., 0., false}};
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
    if (!hasContainer) {
        delete entity;
    }
}

RS2::EntityType Plugin_Entity::getEntityType() {
    return entity->rtti();
}

void Plugin_Entity::getData(QHash<int, QVariant>* data) {
    if (entity == nullptr) {
        return;
    }
    RS2::EntityType et = entity->rtti();
    data->insert(DPI::EID, entity->getId());
    data->insert(DPI::LAYER, entity->getLayer()->getName());
    auto pen = entity->getPen(false);
    data->insert(DPI::LTYPE, Converter.lt2str(pen.getLineType()));
    data->insert(DPI::LWIDTH, Converter.lw2str(pen.getWidth()));
    data->insert(DPI::COLOR, pen.getColor().toIntColor());
    data->insert(DPI::VISIBLE, entity->isVisible() ? 1 : 0);
    switch (et) {
        //atomicEntity
        case RS2::EntityLine: {
            data->insert(DPI::ETYPE, DPI::LINE);
            auto d = static_cast<RS_Line*>(entity)->getData();
            data->insert(DPI::STARTX, d.startpoint.x);
            data->insert(DPI::STARTY, d.startpoint.y);
            data->insert(DPI::ENDX, d.endpoint.x);
            data->insert(DPI::ENDY, d.endpoint.y);
            break;
        }
        case RS2::EntityPoint: {
            data->insert(DPI::ETYPE, DPI::POINT);
            auto d = static_cast<RS_Point*>(entity)->getData();
            data->insert(DPI::STARTX, d.pos.x);
            data->insert(DPI::STARTY, d.pos.y);
            break;
        }
        case RS2::EntityArc: {
            data->insert(DPI::ETYPE, DPI::ARC);
            auto d = static_cast<RS_Arc*>(entity)->getData();
            data->insert(DPI::STARTX, d.center.x);
            data->insert(DPI::STARTY, d.center.y);
            data->insert(DPI::RADIUS, d.radius);
            data->insert(DPI::STARTANGLE, d.angle1);
            data->insert(DPI::ENDANGLE, d.angle2);
            data->insert(DPI::REVERSED, d.reversed);
            break;
        }
        case RS2::EntityCircle: {
            data->insert(DPI::ETYPE, DPI::CIRCLE);
            auto d = static_cast<RS_Circle*>(entity)->getData();
            data->insert(DPI::STARTX, d.center.x);
            data->insert(DPI::STARTY, d.center.y);
            data->insert(DPI::RADIUS, d.radius);
            break;
        }
        case RS2::EntityEllipse: {
            data->insert(DPI::ETYPE, DPI::ELLIPSE);
            //        RS_EllipseData d = static_cast<RS_Ellipse*>(entity)->getData();
            auto* dd = static_cast<RS_Ellipse*>(entity);
            data->insert(DPI::STARTX, dd->getCenter().x); //10
            data->insert(DPI::STARTY, dd->getCenter().y); //20
            data->insert(DPI::ENDX, dd->getMajorP().x); //11 endpoint major axis x
            data->insert(DPI::ENDY, dd->getMajorP().y); //21 endpoint major axis y
            data->insert(DPI::HEIGHT, dd->getRatio()); //40 major/minor axis ratio
            data->insert(DPI::STARTANGLE, dd->getAngle1());
            data->insert(DPI::ENDANGLE, dd->getAngle2());
            data->insert(DPI::REVERSED, dd->isReversed());
            break;
        }
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
            auto d = static_cast<RS_Image*>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::ENDX, d.uVector.x);
            data->insert(DPI::ENDY, d.uVector.y);
            data->insert(DPI::VVECTORX, d.vVector.x);
            data->insert(DPI::VVECTORY, d.vVector.y);
            data->insert(DPI::SIZEU, d.size.x);
            data->insert(DPI::SIZEV, d.size.y);
            data->insert(DPI::BLKNAME, d.file);
            break;
        }
        case RS2::EntityOverlayBox:
            //Unused ?
            data->insert(DPI::ETYPE, DPI::OVERLAYBOX);
            break;
        //EntityContainer
        case RS2::EntityInsert: {
            data->insert(DPI::ETYPE, DPI::INSERT);
            auto d = static_cast<RS_Insert*>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::BLKNAME, d.name);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::XSCALE, d.scaleFactor.x);
            data->insert(DPI::YSCALE, d.scaleFactor.y);
            break;
        }
        case RS2::EntityMText: {
            data->insert(DPI::ETYPE, DPI::MTEXT);
            auto d = static_cast<RS_MText*>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::HEIGHT, d.height);
            data->insert(DPI::TEXTCONTENT, d.text);
            break;
        }
        case RS2::EntityText: {
            data->insert(DPI::ETYPE, DPI::TEXT);
            auto d = static_cast<RS_Text*>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::HEIGHT, d.height);
            data->insert(DPI::TEXTCONTENT, d.text);
            break;
        }
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
            data->insert(DPI::CLOSEPOLY, static_cast<RS_Polyline*>(entity)->isClosed());
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
        case RS2::EntityDimOrdinate:
            data->insert(DPI::ETYPE, DPI::DIMORDINATE);
            break;
        case RS2::EntityTolerance:
            data->insert(DPI::ETYPE, DPI::TOLERANCE);
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
        case RS2::EntityUnknown: default:
            data->insert(DPI::ETYPE, DPI::UNKNOWN);
            break;
    }
}

void Plugin_Entity::updateData(QHash<int, QVariant>* data) {
    if (entity == nullptr) {
        return;
    }
    RS_Entity* ec = entity;
    if (hasContainer && (dpi != nullptr)) {
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
        epen.setLineType(Converter.str2lt(str));
    }
    if (hash.contains(DPI::LWIDTH)) {
        str = (hash.take(DPI::LWIDTH)).toString();
        epen.setWidth(Converter.str2lw(str));
    }
    if (hash.contains(DPI::COLOR)) {
        int co = hash.take(DPI::COLOR).toInt();
        RS_Color color; // = hash.take(DPI::COLOR).value<QColor>();
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
            break;
        }
        case RS2::EntityPoint: {
            vec = static_cast<RS_Point*>(ec)->getPos();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            static_cast<RS_Point*>(ec)->setPos(vec);
            break;
        }
        case RS2::EntityArc: {
            auto* arc = static_cast<RS_Arc*>(ec);
            vec = arc->getCenter();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            arc->setCenter(vec);
            if (hash.contains(DPI::RADIUS)) {
                arc->setRadius((hash.take(DPI::RADIUS)).toDouble());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                arc->setAngle1((hash.take(DPI::STARTANGLE)).toDouble());
                arc->calculateBorders();
                vec.y = (hash.take(DPI::STARTANGLE)).toDouble();
            }
            if (hash.contains(DPI::ENDANGLE)) {
                arc->setAngle2((hash.take(DPI::ENDANGLE)).toDouble());
                arc->calculateBorders();
            }
            break;
        }
        case RS2::EntityCircle: {
            auto* cir = static_cast<RS_Circle*>(ec);
            vec = cir->getCenter();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            cir->setCenter(vec);
            if (hash.contains(DPI::RADIUS)) {
                cir->setRadius((hash.take(DPI::RADIUS)).toDouble());
            }
            break;
        }
        case RS2::EntityEllipse: {
            auto* ellipse = static_cast<RS_Ellipse*>(ec);
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
                ellipse->setReversed((hash.take(DPI::REVERSED)).toBool());
            }
            break;
        }
        case RS2::EntitySolid: //TODO
            //Only used in dimensions ?
            break;
        case RS2::EntityConstructionLine:
            //Unused ?
            break;
        case RS2::EntityImage: {
            auto* img = static_cast<RS_Image*>(ec);
            vec = img->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            img->setInsertionPoint(vec);
            if (hash.contains(DPI::BLKNAME)) {
                img->setFile((hash.take(DPI::BLKNAME)).toString());
            }
            vec = img->getUVector();
            RS_Vector vec2 = img->getVVector();
            RS_Vector vec3(img->getWidth(), img->getHeight());
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
            break;
        }
        case RS2::EntityOverlayBox:
            //Unused ?
            break;
        //EntityContainer
        case RS2::EntityInsert: {
            break;
        }
        case RS2::EntityMText: {
            auto txt = static_cast<RS_MText*>(ec);
            bool move = false;
            vec = txt->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
                move = true;
            }
            else {
                vec.x = 0;
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
                move = true;
            }
            else {
                vec.y = 0;
            }
            if (move) {
                txt->move(vec);
            }
            if (hash.contains(DPI::TEXTCONTENT)) {
                txt->setText((hash.take(DPI::TEXTCONTENT)).toString());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                txt->setAngle((hash.take(DPI::STARTANGLE)).toDouble());
            }
            if (hash.contains(DPI::HEIGHT)) {
                txt->setHeight((hash.take(DPI::HEIGHT)).toDouble());
            }
            break;
        }
        case RS2::EntityText: {
            auto txt = static_cast<RS_Text*>(ec);
            bool move = false;
            vec = txt->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
                move = true;
            }
            else {
                vec.x = 0;
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
                move = true;
            }
            else {
                vec.y = 0;
            }
            if (move) {
                txt->move(vec);
            }
            if (hash.contains(DPI::TEXTCONTENT)) {
                txt->setText((hash.take(DPI::TEXTCONTENT)).toString());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                txt->setAngle((hash.take(DPI::STARTANGLE)).toDouble());
            }
            if (hash.contains(DPI::HEIGHT)) {
                txt->setHeight((hash.take(DPI::HEIGHT)).toDouble());
            }
            break;
        }
        case RS2::EntityHatch:
            break;
        case RS2::EntitySpline:
            break;
        case RS2::EntityPolyline: {
            auto* pl = static_cast<RS_Polyline*>(ec);
            if (hash.take(DPI::CLOSEPOLY).toBool()) {
                pl->setClosed(true);
            }
            else {
                pl->setClosed(false);
            }
            break;
        }
        case RS2::EntityVertex:
            break;
        case RS2::EntityDimAligned:
            break;
        case RS2::EntityDimLinear:
            break;
        case RS2::EntityTolerance:
            break;
        case RS2::EntityDimOrdinate:
            break;
        case RS2::EntityDimRadial:
            break;
        case RS2::EntityDimDiametric:
            break;
        case RS2::EntityDimAngular:
            break;
        case RS2::EntityDimLeader:
            break;
        case RS2::EntityUnknown: default:
            break;
    }
    ec->update();
    if (hasContainer && (dpi != nullptr)) {
        this->dpi->updateEntity(entity, ec);
    }
}

void Plugin_Entity::getPolylineData(QList<Plug_VertexData>* data) {
    if (entity == nullptr) {
        return;
    }
    const RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) {
        return;
    }
    const auto* l = static_cast<RS_Polyline*>(entity);

    RS_Entity* nextEntity = nullptr;
    const RS_AtomicEntity* ae = nullptr;
    RS_Entity* v = l->firstEntity(RS2::ResolveNone);
    double bulge = 0.0;
    //bad polyline without vertex
    if (v == nullptr) {
        return;
    }

    //First polyline vertex
    if (v->rtti() == RS2::EntityArc) {
        bulge = static_cast<RS_Arc*>(v)->getBulge();
    }
    ae = static_cast<RS_AtomicEntity*>(v);
    data->append(Plug_VertexData(QPointF(ae->getStartpoint().x, ae->getStartpoint().y), bulge));

    lc::LC_ContainerTraverser traverser{*l, RS2::ResolveNone};
    for (v = traverser.first(); v != nullptr; v = traverser.next()) {
        nextEntity = traverser.next();
        bulge = 0.0;
        if (!v->isAtomic()) {
            continue;
        }
        ae = static_cast<RS_AtomicEntity*>(v);

        if (nextEntity != nullptr) {
            if (nextEntity->rtti() == RS2::EntityArc) {
                bulge = static_cast<RS_Arc*>(nextEntity)->getBulge();
            }
        }

        if (!l->isClosed() || (nextEntity != nullptr)) {
            data->append(Plug_VertexData(QPointF(ae->getEndpoint().x, ae->getEndpoint().y), bulge));
        }
    }
}

void Plugin_Entity::updatePolylineData(QList<Plug_VertexData>* data) {
    if (entity == nullptr) {
        return;
    }
    const RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) {
        return;
    }
    if (data->size() < 2){
        return; //At least two vertex
    }
    RS_Vector vec(false);
    const auto pl = static_cast<RS_Polyline*>(entity);
    //    vec.x = data->at(0).point.x();
    //    vec.y = data->at(0).point.y();
    pl->clear();
    pl->setEndpoint(vec);
    pl->setStartpoint(vec);
    vec.valid = true;
    for (const auto& plugVertexData : *data) {
        vec.x = plugVertexData.point.x();
        vec.y = plugVertexData.point.y();
        pl->addVertex(vec, plugVertexData.bulge);
    }
}

void Plugin_Entity::move(const QPointF offset, const DPI::Disposition disp) {
    RS_Entity* ne = entity->clone();
    ne->move(RS_Vector(offset.x(), offset.y()));
    const bool ok = dpi->addToUndo(entity, ne, disp);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->move(RS_Vector(offset.x(), offset.y()));
        delete ne;
    }
    else {
        this->entity = ne;
    }
}

void Plugin_Entity::moveRotate(const QPointF& offset, const QPointF& center, const double angle, const DPI::Disposition disp) {
    RS_Entity* ne = entity->clone();
    ne->move(RS_Vector(offset.x(), offset.y()));
    ne->rotate(RS_Vector(center.x(), center.y()), angle);
    const bool ok = dpi->addToUndo(entity, ne, disp);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->move(RS_Vector(offset.x(), offset.y()));
        entity->rotate(RS_Vector(center.x(), center.y()), angle);
        delete ne;
    }
    else {
        this->entity = ne;
    }
}

void Plugin_Entity::rotate(const QPointF center, const double angle, const DPI::Disposition disp) {
    RS_Entity* ne = entity->clone();
    ne->rotate(RS_Vector(center.x(), center.y()), angle);
    const bool ok = dpi->addToUndo(entity, ne, disp);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->rotate(RS_Vector(center.x(), center.y()), angle);
        delete ne;
    }
    else {
        this->entity = ne;
    }
}

void Plugin_Entity::scale(const QPointF center, const QPointF factor, const DPI::Disposition disp) {
    RS_Entity* ne = entity->clone();
    ne->scale(RS_Vector(center.x(), center.y()), RS_Vector(factor.x(), factor.y()));
    const bool ok = dpi->addToUndo(entity, ne, disp);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->scale(RS_Vector(center.x(), center.y()), RS_Vector(factor.x(), factor.y()));
        delete ne;
    }
    else {
        this->entity = ne;
    }
}

QString Plugin_Entity::intColor2str(const int color) {
    return Converter.intColor2str(color);
}

Doc_plugin_interface::Doc_plugin_interface(LC_ActionContext* actionContext, QWidget* parent) : m_document(actionContext->getDocument()),
    m_docGr(m_document->getGraphic()), m_graphicView(actionContext->getGraphicView()), m_mainWindow(parent), m_actionContext{actionContext} {
    Q_ASSERT(m_document != nullptr && m_graphicView != nullptr);
    m_viewport = m_graphicView->getViewPort();
}

bool Doc_plugin_interface::addToUndo(RS_Entity* current, RS_Entity* modified, const DPI::Disposition how) const {
    m_document->addEntity(modified);
    const LC_UndoSection undo(m_document, m_viewport);
    current->clearSelectionFlag();
    if (how == DPI::DELETE_ORIGINAL) {
        undo.undoableDelete(current);
    }
    undo.undoableAdd(modified);
    return true;
}

void Doc_plugin_interface::updateView() {
    m_document->setSelectionFlag(false);
    m_graphicView->getDocument()->calculateBorders();
    m_graphicView->redraw();
}

void Doc_plugin_interface::addPoint(QPointF* start) {
    const RS_Vector v1(start->x(), start->y());
    auto* entity = new RS_Point(m_document, RS_PointData(v1));
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addLine(QPointF* start, QPointF* end) {
    const RS_Vector v1(start->x(), start->y());
    const RS_Vector v2(end->x(), end->y());
    const auto entity = new RS_Line{m_document, v1, v2};
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addMText(const QString& txt, const QString& sty, const QPointF* start, const double height, const double angle, DPI::HAlign ha,
                                    DPI::VAlign va) const {
    const RS_Vector v1(start->x(), start->y());

    constexpr double width = 100.0;

    const auto valign = static_cast<RS_MTextData::VAlign>(va);
    const auto halign = static_cast<RS_MTextData::HAlign>(ha);
    const RS_MTextData d(v1, height, width, valign, halign, RS_MTextData::ByStyle, RS_MTextData::Exact, 0.0, txt, sty, angle, RS2::Update);
    const auto entity = new RS_MText(m_document, d);
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addText(const QString txt, const QString sty, QPointF* start, const double height, const double angle, DPI::HAlign ha, DPI::VAlign va) {
    const RS_Vector v1(start->x(), start->y());
    constexpr double width = 1.0;
    const auto valign = static_cast<RS_TextData::VAlign>(va);
    const auto halign = static_cast<RS_TextData::HAlign>(ha);
    const RS_TextData d(v1, v1, height, width, valign, halign, RS_TextData::None, txt, sty, angle, RS2::Update);
    const auto entity = new RS_Text(m_document, d);
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addCircle(QPointF* start, const qreal radius) {
    const RS_Vector v(start->x(), start->y());
    const RS_CircleData d(v, radius);
    const auto entity = new RS_Circle(m_document, d);
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addArc(QPointF* start, const qreal radius, const qreal a1, const qreal a2) {
    const RS_Vector v(start->x(), start->y());
    const RS_ArcData d(v, radius, RS_Math::deg2rad(a1), RS_Math::deg2rad(a2), false);
    const auto entity = new RS_Arc(m_document, d);
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addEllipse(QPointF* start, QPointF* end, const qreal ratio, const qreal a1, const qreal a2) {
    const RS_Vector v1(start->x(), start->y());
    const RS_Vector v2(end->x(), end->y());

    const RS_EllipseData ed{v1, v2, ratio, a1, a2, false};
    const auto entity = new RS_Ellipse(m_document, ed);

    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addLines(const std::vector<QPointF>& points, const bool closed) {
    RS_LineData data;

    const LC_UndoSection undo(m_document, m_viewport);
    data.endpoint = RS_Vector(points.front().x(), points.front().y());

    for (size_t i = 1; i < points.size(); ++i) {
        data.startpoint = data.endpoint;
        data.endpoint = RS_Vector(points[i].x(), points[i].y());
        auto* line = new RS_Line(m_document, data);
        undo.undoableAdd(line);
    }
    if (closed) {
        data.startpoint = data.endpoint;
        data.endpoint = RS_Vector(points.front().x(), points.front().y());
        auto* line = new RS_Line(m_document, data);
        undo.undoableAdd(line);
    }
}

void Doc_plugin_interface::addPolyline(const std::vector<Plug_VertexData>& points, const bool closed) {
    RS_PolylineData data;
    if (closed) {
        data.setFlag(RS2::FlagClosed);
    }
    auto* entity = new RS_Polyline(m_document, data);
    for (const auto& pt : points) {
        entity->addVertex(RS_Vector(pt.point.x(), pt.point.y()), pt.bulge);
    }
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addSplinePoints(const std::vector<QPointF>& points, const bool closed) {
    LC_SplinePointsData data(closed, false); //cut = false
    for (const auto& pt : points) {
        data.splinePoints.emplace_back(RS_Vector(pt.x(), pt.y()));
    }
    auto* entity = new LC_SplinePoints(m_document, data);
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

void Doc_plugin_interface::addImage(const int handle, QPointF* start, QPointF* uvr, QPointF* vvr, const int w, const int h, const QString name, const int br, const int con, const int fade) {
    const RS_Vector ip(start->x(), start->y());
    const RS_Vector uv(uvr->x(), uvr->y());
    const RS_Vector vv(vvr->x(), vvr->y());
    const RS_Vector size(w, h);

    auto* image = new RS_Image(m_document, RS_ImageData(handle /*QString(data.ref.c_str()).toInt(nullptr, 16)*/, ip, uv, vv, size, name, br,
                                                        con, fade));

    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(image);
}

void Doc_plugin_interface::addInsert(const QString name, const QPointF ins, const QPointF scale, const qreal rot) {
    const RS_Vector ip(ins.x(), ins.y());
    const RS_Vector sp(scale.x(), scale.y());

    const RS_InsertData id(name, ip, sp, rot, 1, 1, RS_Vector(0.0, 0.0));
    auto* entity = new RS_Insert(m_document, id);

    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableAdd(entity);
}

/*TODO RLZ: add undo support in this method*/
QString Doc_plugin_interface::addBlockfromFromdisk(const QString fullName) {
    if (fullName.isEmpty() || (m_document == nullptr)) {
        return nullptr;
    }
    RS_BlockList* blockList = m_document->getBlockList();
    if (blockList == nullptr) {
        return nullptr;
    }

    const QFileInfo fi(fullName);

    if (fi.isReadable()) {
        const QString s = fi.completeBaseName();
        QString name = blockList->newName(s);
        const RS_BlockData d(name, RS_Vector(0, 0), false);
        auto* b = new RS_Block(m_document, d);
        RS_Graphic g;
        const LC_DocumentsStorage storage;
        if (!storage.loadDocument(&g, fi.absoluteFilePath(), RS2::FormatUnknown)) {
            // if (!g.open(fi.absoluteFilePath(), RS2::FormatUnknown)) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "Doc_plugin_interface::addBlockfromFromdisk: Cannot open file: %s",
                            fullName.toStdString().c_str());
            delete b;
            return nullptr;
        }
        const RS_LayerList* ll = g.getLayerList();
        for (unsigned int i = 0; i < ll->count(); i++) {
            RS_Layer* nl = ll->at(i)->clone();
            m_docGr->addLayer(nl);
        }
        RS_BlockList* bl = g.getBlockList();
        for (int i = 0; i < bl->count(); i++) {
            auto* nb = static_cast<RS_Block*>(bl->at(i)->clone());
            m_docGr->addBlock(nb);
        }
        for (unsigned int i = 0; i < g.count(); i++) {
            RS_Entity* e = g.entityAt(i)->clone();
            e->reparent(b);
            b->addEntity(e);
        }
        m_docGr->addBlock(b);
        return name;
    }
    return nullptr;
}

void Doc_plugin_interface::addEntity(Plug_Entity* handle) {
    RS_Entity* ent = reinterpret_cast<Plugin_Entity*>(handle)->getEnt();
    if (ent != nullptr) {
        const LC_UndoSection undo(m_document, m_viewport);
        undo.undoableAdd(ent);
    }
}

/*newEntity not added into graphic, then not needed undo support*/
Plug_Entity* Doc_plugin_interface::newEntity(const enum DPI::ETYPE type) {
    auto* e = new Plugin_Entity(m_document, type);
    if (!e->isValid()) {
        delete e;
        return nullptr;
    }
    return reinterpret_cast<Plug_Entity*>(e);
}

/*TODO RLZ: add undo support in this method*/
void Doc_plugin_interface::removeEntity(Plug_Entity* ent) {
    RS_Entity* e = reinterpret_cast<Plugin_Entity*>(ent)->getEnt();
    if (e != nullptr) {
        const LC_UndoSection undo(m_document, m_viewport);
        undo.undoableDelete(e);
        m_graphicView->redraw(RS2::RedrawDrawing);
    }
}

void Doc_plugin_interface::updateEntity(RS_Entity* original, RS_Entity* clone) const {
    const LC_UndoSection undo(m_document, m_viewport);
    undo.undoableReplace(original, clone);
}

/*TODO RLZ: add undo support in the remaining methods*/
void Doc_plugin_interface::setLayer(const QString name) {
    RS_LayerList* listLay = m_document->getLayerList();
    RS_Layer* lay = listLay->find(name);
    if (lay == nullptr) {
        lay = new RS_Layer(name);
        m_docGr->addLayer(lay);
    }
    RS_Graphic* graphic = m_document->getGraphic();
    if (graphic != nullptr) {
        graphic->activateLayer(lay, true);
    }
}

QString Doc_plugin_interface::getCurrentLayer() {
    return m_docGr->getActiveLayer()->getName();
}

QStringList Doc_plugin_interface::getAllLayer() {
    QStringList listName;
    const RS_LayerList* listLay = m_document->getLayerList();
    for (unsigned int i = 0; i < listLay->count(); ++i) {
        listName << listLay->at(i)->getName();
    }
    return listName;
}

QStringList Doc_plugin_interface::getAllBlocks() {
    QStringList listName;
    RS_BlockList* listBlk = m_document->getBlockList();
    for (int i = 0; i < listBlk->count(); ++i) {
        listName << listBlk->at(i)->getName();
    }
    return listName;
}

bool Doc_plugin_interface::deleteLayer(const QString name) {
    RS_Layer* layer = m_docGr->findLayer(name);
    if (layer != nullptr) {
        m_docGr->removeLayer(layer);
        return true;
    }
    return false;
}

void Doc_plugin_interface::getCurrentLayerProperties(int* c, DPI::LineWidth* w, DPI::LineType* t) {
    const RS_Pen pen = m_docGr->getActiveLayer()->getPen();
    *c = pen.getColor().toIntColor();
    //    RS_Color col = pen.getColor();
    //    c->setRgb(col.red(), col.green(), col.blue());
    *w = static_cast<DPI::LineWidth>(pen.getWidth());
    *t = static_cast<DPI::LineType>(pen.getLineType());
}

void Doc_plugin_interface::getCurrentLayerProperties(int* c, QString* w, QString* t) {
    const RS_Pen pen = m_docGr->getActiveLayer()->getPen();
    *c = pen.getColor().toIntColor();
    w->clear();
    w->append(Converter.lw2str(pen.getWidth()));
    t->clear();
    t->append(Converter.lt2str(pen.getLineType()));
}

void Doc_plugin_interface::setCurrentLayerProperties(const int c, DPI::LineWidth w, DPI::LineType t) {
    RS_Layer* layer = m_docGr->getActiveLayer();
    if (layer != nullptr) {
        RS_Color co;
        co.fromIntColor(c);
        const RS_Pen pen(co, static_cast<RS2::LineWidth>(w), static_cast<RS2::LineType>(t));
        layer->setPen(pen);
    }
}

void Doc_plugin_interface::setCurrentLayerProperties(const int c, const QString& w, const QString& t) {
    RS_Layer* layer = m_docGr->getActiveLayer();
    if (layer != nullptr) {
        RS_Color co;
        co.fromIntColor(c);
        const RS_Pen pen(co, Converter.str2lw(w), Converter.str2lt(t));
        layer->setPen(pen);
    }
}

bool Doc_plugin_interface::getPoint(QPointF* point, const QString& message, QPointF* base) {
    bool status = false;

    const auto a = std::make_shared<QC_ActionGetPoint>(m_actionContext);
    if (a != nullptr) {
        if (!message.isEmpty()) {
            a->setMessage(message);
        }
        m_graphicView->killAllActions();
        m_graphicView->setCurrentAction(a);
        if (base != nullptr) {
            a->setBasepoint(base);
        }
        QEventLoop ev;
        while (!a->isCompleted()) {
            ev.processEvents();
            if (!m_graphicView->hasAction()) {
                break;
            }
        }
        if (a->isCompleted() && !a->wasCanceled()) {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        m_graphicView->killAllActions();
    }
    return status;
}

Plug_Entity* Doc_plugin_interface::getEnt(const QString& message) {
    const auto a = std::make_shared<QC_ActionGetEnt>(m_actionContext);
    if (!message.isEmpty()) {
        a->setMessage(message);
    }
    m_graphicView->killAllActions();
    m_graphicView->setCurrentAction(a);
    QEventLoop ev;
    while (!a->isCompleted()) {
        ev.processEvents();
        if (!m_graphicView->hasAction()) {
            break;
        }
    }
    auto* e = reinterpret_cast<Plug_Entity*>(a->getSelected(this));
    m_graphicView->killAllActions();
    return e;
}

bool Doc_plugin_interface::performSelect(RS2::EntityType typeToSelect, const QString& message, QList<Plug_Entity*>* sel, const bool clearSel) {
    const auto a = std::make_shared<QC_ActionGetSelect>(typeToSelect, m_actionContext);
    if (a == nullptr || sel == nullptr || m_graphicView == nullptr) {
        return false;
    }
    if (clearSel){
        sel->clear(); // Optional: Reset list before collection
    }
    if (!message.isEmpty()) {
        a->setMessage(message);
        RS_DIALOGFACTORY->commandMessage(message);
    }
    const auto inner = typeToSelect == RS2::EntityType::EntityUnknown
                     ? std::make_shared<LC_ActionSelectSingle>(m_actionContext, a.get())
                     : std::make_shared<LC_ActionSelectSingle>(typeToSelect, m_actionContext, a.get());
    if (inner == nullptr) {
        return false; // Rare shared_ptr fail
    }
    m_graphicView->killAllActions();
    m_graphicView->setCurrentAction(inner);
    if (!m_graphicView->hasAction()) {
        // Robustness: Verify set succeeded
        m_graphicView->killAllActions();
        return false;
    }
    inner->init(0);
    a->init(0);
    QEventLoop ev;
    while (!a->isCompleted()) {
        ev.processEvents();
        if (!m_graphicView->hasAction()) {
            break;
        }
    }
    const bool completed = a->isCompleted();
    m_graphicView->killAllActions(); // Always cleanup
    if (completed) {
        a->getSelected(sel, this);
        return !sel->isEmpty();
    }
    return false;
}

bool Doc_plugin_interface::getSelect(QList<Plug_Entity*>* sel, const QString& message) {
    return performSelect(RS2::EntityType::EntityUnknown, message, sel, false);
}

bool Doc_plugin_interface::getSelectByType(QList<Plug_Entity*>* sel, const enum DPI::ETYPE type, const QString& message) {
    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;
    if (type == DPI::LINE) {
        typeToSelect = RS2::EntityType::EntityLine;
    }
    else if (type == DPI::POINT) {
        typeToSelect = RS2::EntityType::EntityPoint;
    }
    else if (type == DPI::POLYLINE) {
        typeToSelect = RS2::EntityType::EntityPolyline;
    }
    else {
        //Unhandled case
    }

    m_graphicView->setTypeToSelect(typeToSelect);
    const bool status = performSelect(typeToSelect, message, sel, false);
    m_graphicView->setTypeToSelect(RS2::EntityType::EntityUnknown);
    return status;
}

bool Doc_plugin_interface::getAllEntities(QList<Plug_Entity*>* sel, const bool visible) {
    bool status = false;

    for (const auto e : *m_document) {
        if (e->isVisible() || !visible) {
            auto* pe = new Plugin_Entity(e, this);
            sel->append(reinterpret_cast<Plug_Entity*>(pe));
        }
    }
    status = true;
    return status;
}

void Doc_plugin_interface::unselectEntities() {
    const auto a = new QC_ActionGetSelect(m_actionContext);
    a->unselectEntities();
}

bool Doc_plugin_interface::getVariableInt(const QString& key, int* num) {
    if ((*num = m_docGr->getVariableInt(key, 0))) {
        return true;
    }
    return false;
}

bool Doc_plugin_interface::getVariableDouble(const QString& key, double* num) {
    *num = m_docGr->getVariableDouble(key, 0.0); // fixme - sand - review the logic there
    if (*num) {
        return true;
    }
    return false;
}

bool Doc_plugin_interface::addVariable(const QString& key, const int value, const int code) {
    m_docGr->addVariable(key, value, code);
    if (key.startsWith("$DIM")) {
        m_document->updateDimensions(true);
    }
    return true;
}

bool Doc_plugin_interface::addVariable(const QString& key, const double value, const int code) {
    m_docGr->addVariable(key, value, code);
    if (key.startsWith("$DIM")) {
        m_document->updateDimensions(true);
    }
    return true;
}

bool Doc_plugin_interface::getInt(int* num, const QString& message, const QString& title) {
    bool ok = false;
    QString msg, tit;
    if (message.isEmpty()) {
        msg = QObject::tr("enter an integer number");
    }
    else {
        msg = message;
    }
    if (title.isEmpty()) {
        tit = QObject::tr("LibreCAD query");
    }
    else {
        tit = title;
    }
    const int data = QInputDialog::getInt(m_mainWindow, tit, msg, 0, -2147483647, 2147483647, 1, &ok);
    if (ok) {
        *num = data;
    }
    return ok;
}

bool Doc_plugin_interface::getReal(qreal* num, const QString& message, const QString& title) {
    bool ok = false;
    QString msg, tit;
    if (message.isEmpty()) {
        msg = QObject::tr("enter a number");
    }
    else {
        msg = message;
    }
    if (title.isEmpty()) {
        tit = QObject::tr("LibreCAD query");
    }
    else {
        tit = title;
    }

    const double data = QInputDialog::getDouble(m_mainWindow, tit, msg, 0, -2147483647, 2147483647, 4, &ok);
    if (ok) {
        *num = data;
    }
    return ok;
}

bool Doc_plugin_interface::getString(QString* txt, const QString& message, const QString& title) {
    bool ok = false;
    QString msg, tit;
    if (message.isEmpty()) {
        msg = QObject::tr("enter text");
    }
    else {
        msg = message;
    }
    if (title.isEmpty()) {
        tit = QObject::tr("LibreCAD query");
    }
    else {
        tit = title;
    }

    const QString text = QInputDialog::getText(m_mainWindow, tit, msg, QLineEdit::Normal, QString(), &ok);
    if (ok && !text.isEmpty()) {
        txt->clear();
        txt->append(text);
    }
    return ok;
}

QString Doc_plugin_interface::realToStr(const qreal num, const int units, const int prec) {
    RS2::LinearFormat lf;
    int pr = prec;
    if (pr == 0) {
        pr = m_docGr->getLinearPrecision();
    }

    switch (units) {
        case 0:
            lf = m_docGr->getLinearFormat();
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

    QString msg = RS_Units::formatLinear(num, RS2::None, lf, pr);
    return msg;
}
