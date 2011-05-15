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
#ifndef DOC_PLUGIN_INTERFACE_H
#define DOC_PLUGIN_INTERFACE_H

#include <QObject>

#include "document_interface.h"
#include "rs_graphic.h"

class Doc_plugin_interface : public Document_Interface

{
public:
    Doc_plugin_interface(RS_Graphic *d);
//    ~Doc_plugin_interface(){};
    void addPoint(QPointF *start);
    void addLine(QPointF *start, QPointF *end);
    void addText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va);

    void setLayer(QString name);
    QString getCurrentLayer();

private:
    RS_Graphic *doc;
};

#endif // DOC_PLUGIN_INTERFACE_H
