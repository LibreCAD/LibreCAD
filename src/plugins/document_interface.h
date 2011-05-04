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
#ifndef DOCUMENT_INTERFACE_H
#define DOCUMENT_INTERFACE_H

#include <QPointF>

namespace DPI {
    /**
     * Vertical alignments.
     */
    enum VAlign {
        VAlignTop,      /**< Top. */
        VAlignMiddle,   /**< Middle */
        VAlignBottom    /**< Bottom */
    };

    /**
     * Horizontal alignments.
     */
    enum HAlign {
        HAlignLeft,     /**< Left */
        HAlignCenter,   /**< Centered */
        HAlignRight     /**< Right */
    };
}
/**
 * Interface for comunicate plugins.
 *
 * @author Rallaz
 */
class Document_Interface
{
public:
    virtual ~Document_Interface() {}
    virtual void addPoint(QPointF *start) = 0;
    virtual void addLine(QPointF *start, QPointF *end) = 0;
    virtual void addText(QString txt, QString sty, QPointF *start, double height,
                double angle, DPI::HAlign ha,  DPI::VAlign va) = 0;

    virtual void setLayer(QString name) = 0;
    virtual QString getCurrentLayer() = 0;
};


#endif // DOCUMENT_INTERFACE_H
