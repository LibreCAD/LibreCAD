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
#include "rs_text.h"

Doc_plugin_interface::Doc_plugin_interface(RS_Graphic *d)
{
    doc =d;
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

void Doc_plugin_interface::setLayer(QString name){
    RS_Layer *lay = new RS_Layer(name);
    doc->addLayer(lay);
}

QString Doc_plugin_interface::getCurrentLayer(){
    return doc->getActiveLayer()->getName();
}
