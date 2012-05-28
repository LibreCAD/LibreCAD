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
#ifndef DOCUMENT_INTERFACE_H
#define DOCUMENT_INTERFACE_H

#include <QPointF>
#include <QHash>
#include <QString>

namespace DPI {
    //! Vertical alignments.
    enum VAlign {
        VAlignTop,      /*!< Top. */
        VAlignMiddle,   /*!< Middle */
        VAlignBottom    /*!< Bottom */
    };

    //! Horizontal alignments.
    enum HAlign {
        HAlignLeft,     /*!< Left */
        HAlignCenter,   /*!< Centered */
        HAlignRight     /*!< Right */
    };


   //! Entity's type.
    enum ETYPE {
        POINT,
        LINE,
        CONSTRUCTIONLINE,
        CIRCLE,
        ARC,
        ELLIPSE,
        IMAGE,
        OVERLAYBOX,
        SOLID,/*end atomicEntity, start entityContainer*/
        TEXT,
        INSERT,
        POLYLINE,
        SPLINE,
        HATCH,
        DIMLEADER,
        DIMALIGNED,
        DIMLINEAR,
        DIMRADIAL,
        DIMDIAMETRIC,
        DIMANGULAR,
        UNKNOWN
    };

    //! Entity's data (dxf like).

    enum EDATA {
        ETYPE=0,        /*!< enum: entity type */
        TEXTCONTENT=1,  /*!< QString: text string */
        BLKNAME=2,      /*!< QString: block name */
        EID=5,          /*!< qulonglong: entity identifier */
        LTYPE=6,        /*!< QString: line type */
        TXTSTYLE=7,     /*!< QString: text style */
        LAYER=8,        /*!< QString: layer */
        STARTX=10,      /*!< double: start x coordinate */
        ENDX=11,        /*!< double: end x coordinate */
        STARTY=20,      /*!< double: start y coordinate */
        ENDY=21,        /*!< double: end y coordinate */
        STARTZ=30,      /*!< double: start z coordinate always 0 */
        ENDZ=30,        /*!< double: end z coordinate always 0 */
        LWIDTH=38,      /*!< QString: line width */  //verify number
        RADIUS=39,      /*!< double: radius */
        HEIGHT=40,      /*!< double: text heigt or ellipse ratio*/
        XSCALE=41,      /*!< double: x insert scale */
        YSCALE=42,      /*!< double: y insert scale */
        ZSCALE=43,      /*!< double: z insert scale always 1? */
        LTSCALE=48,     /*!< line type scale (not in LibreCAD) */
        STARTANGLE=50,  /*!< double: arc start angle or rotation angle for insert and text */
        ENDANGLE=51,    /*!< double: arc end angle */
        COLOR=62,       /*!< QColor: entity color */
        CLOSEPOLY=70,   /*!< int: closed polyline 0=open, 1=closed */
        TXTALIGNH=72,   /*!< enum: horizontal alignment for text */
        TXTALIGNV=73,   /*!< enum: vertical alignment for text */
        REVERSED=291 /*!< bool: true if arc is reversed (clockwise) */
    };

}

class Plug_VertexData
{
public:
    Plug_VertexData(QPointF p, double b){
        point = p;
        bulge = b;
    }
    QPointF point;
    double bulge;
};

//! Wrapper for acces entities from plugins.
 /*!
 *  Wrapper class for create, acces and modify entities from plugins.
 *  TODO: terminate acces function -> getData()
 *        terminate implementation of modify function -> updateData()
 *        terminate implementation of create function (ctor called from document)
 *           can't create entities:
 *           - atomic = CONSTRUCTIONLINE, OVERLAYBOX, SOLID,
 *           - container = INSERT, POLYLINE, SPLINE, HATCH, DIMLEADER,
 *             DIMALIGNED, DIMLINEAR, DIMRADIAL, DIMDIAMETRIC, DIMANGULAR,
 *        verify destructor if needed or not to delete entity
 * @author Rallaz
 */
class Plug_Entity
{
public:
    virtual ~Plug_Entity() {}

    //! Obtain the entity data.
    /*!
    * The data is a QHash with the EDATA keys relevant to the type of entity
    *  \param data pointer to a QHash<int, QVariant> to store the entity data.
    */
    virtual void getData(QHash<int, QVariant> *data) = 0;

    //! Update the entity data.
    /*!
    * The data is a QHash with the EDATA keys relevant to the type of entity
    *  \param data pointer to a QHash<int, QVariant> with the entity data.
    */
    virtual void updateData(QHash<int, QVariant> *data) = 0;

    //! Obtain the polyline list of vertex.
    /*!
    * The data is a QList to store a Plug_VertexData for heach vertex form the polyline
    *  \param data pointer to a QList<Plug_VertexData> to store the vertex list.
    */
    virtual void getPolylineData(QList<Plug_VertexData> *data) = 0;

    //! Update the polyline list of vertex.
    /*!
    * The data is a QList of Plug_VertexData with the coordinates of each vertex to the
    * polyline the cuurent vertex are removed and replaced for the new list.
    *  \param data pointer to a QList<Plug_VertexData> with the coordinates vertex's.
    */
    virtual void updatePolylineData(QList<Plug_VertexData> *data) = 0;

    //! Move the entity.
    /*!
    *  \param offset move the entity by the given QPointF.
    */
    virtual void move(QPointF offset) = 0;

    //! rotate the entity.
    /*!
    *  \param center center of rotation.
    *  \param angle angle to rotate.
    */
    virtual void rotate(QPointF center, double angle) = 0;

    //! Scale the entity.
    /*!
    *  \param center base point for scale.
    *  \param factor scale factor.
    */
    virtual void scale(QPointF center, QPointF factor) = 0;

};

//! Interface for comunicate plugins.
 /*!
 * Class for comunicate plugins with document (drawing).
 * entities whitout add*() function:
 * atomic = CONSTRUCTIONLINE, OVERLAYBOX, SOLID,
 * container = INSERT, POLYLINE, SPLINE, HATCH, DIMLEADER,
 * DIMALIGNED, DIMLINEAR, DIMRADIAL, DIMDIAMETRIC, DIMANGULAR,
 * TODO: memory assignation and cleanup in plugin
 *   added----    implementation of function -> deleteEntity()
 * @author Rallaz
 */
class Document_Interface
{
public:
    virtual ~Document_Interface() {}
    //! Force to update the graphic view.
    /*! Force to update the graphic view.
    */
    virtual void updateView() = 0;

    //! Add point entity to current document.
    /*! Add point entity to current document with current attributes.
    *  \param start point coordinate.
    */
    virtual void addPoint(QPointF *start) = 0;

    //! Add line entity to current document.
    /*! Add line entity to current document with current attributes.
    *  \param start start point coordinate.
    *  \param end end point coordinate.
    */
    virtual void addLine(QPointF *start, QPointF *end) = 0;

    //! Add text entity to current document.
    /*! Add text entity to current document with current attributes
    *  \param txt a QString with text content
    *  \param sty a QString with text style name
    *  \param start insertion point coordinate
    *  \param height height of text
    *  \param angle rotation angle of text
    *  \param ha horizontal alignement of text
    *  \param va vertical alignement of text
    */
    virtual void addText(QString txt, QString sty, QPointF *start, double height,
                double angle, DPI::HAlign ha,  DPI::VAlign va) = 0;

    //! Add circle entity to current document.
    /*! Add circle entity to current document with current attributes.
    *  \param start center point coordinate.
    *  \param radius radius for circle.
    */
    virtual void addCircle(QPointF *start, qreal radius) = 0;

    //! Add arc entity to current document.
    /*! Add arc of circumference entity to current document with current attributes.
    *  \param start center point coordinate.
    *  \param radius radius for circle.
    */
    virtual void addArc(QPointF *start, qreal radius, qreal a1, qreal a2) = 0;

    //! Add ellipse entity to current document.
    /*! Add ellipse entity to current document with current attributes.
    *  \param start center point coordinate.
    *  \param radius radius for arc.
    */
    virtual void addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2) = 0;

    //! Add image entity to current document.
    /*! Add image entity to current document with current attributes.
    *  \param start start point coordinate.
    *  \param end end point coordinate.
    */
    virtual void addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                  int w, int h, QString name, int br, int con, int fade) = 0;

    //! Add insert entity to current document.
    /*! Add a block insert entity to current document with current attributes.
    *  \param name name of block to insert.
    *  \param ins insertion point coordinate.
    *  \param scale x,y scale factor.
    *  \param rot rotation angle.
    */
    virtual void addInsert(QString name, QPointF ins, QPointF scale, qreal rot) = 0;

    //! Add block definition from disk to current document.
    /*! Add block definition from disk to current document.
    *  \param fullName path+name of dxf file to add.
    *  \return name of created block or NULL if fail.
    */
    virtual QString addBlockfromFromdisk(QString fullName) = 0;

    //! Add a entity to current document.
    /*! Add a entity to current document with the data sets with Plug_Entity.updateData().
    *  \param handle a pointer to Plug_Entity.
    */
    virtual void addEntity(Plug_Entity *handle) = 0;

    //! Create a new Plug_Entity.
    /*! Create a new Plug_Entity of type ETYPE with default data.
    * sets the data with Plug_Entity.updateData().
    * if fail (unknoun or unhandled entity) return NULL
    *  \param type an DPI::ETYPE type.
    *  \return handle to pointer of Plug_Entity with type DPI::ETYPE or NULL if fail.
    */
    virtual Plug_Entity *newEntity(enum DPI::ETYPE type) = 0;

    //! Remove a Entity.
    /*! Remove a entity from current drawing.
    *  \param ent handle to pointer of Plug_Entity.
    */
    virtual void removeEntity(Plug_Entity *ent) = 0;

    //! Set the current layer in current document.
    /*! Set the current layer in current document, if not exist create it.
    *  \param name a QString with the name of the layer.
    */
    virtual void setLayer(QString name) = 0;

    //! Get the current layer in current document.
    /*! Get the current layer in current document.
    *  \return The name of the current layer.
    */
    virtual QString getCurrentLayer() = 0;

    //! Gets the layers list in current document.
    /*! Gets the list of names of all layers in current document.
    *  \return A list with the name of all layers in document.
    */
    virtual QStringList getAllLayer() = 0;

    //! Gets the blocks list in current document.
    /*! Gets the list of names of all blocks in current document.
    *  \return A list with the name of all blocks in document.
    */
    virtual QStringList getAllBlocks() = 0;

    //! Delete a layer in current document.
    /*! Delete the layer "name" in current document if it exist.
    *  \return The name of the current layer.
    */
    virtual bool deleteLayer(QString name) = 0;


    //! Gets a point.
    /*! Prompt message or an default message to the user asking for a point.
    *   If base is present draw a line from base to cursor position.
    * \param point a pointer to QPointF to store the obtained point.
    * \param mesage an optional QString with prompt message.
    * \param base visual helper point, if present.
    * \return true if succes.
    * \return false if fail, i.e. user cancel.
    */
    virtual bool getPoint(QPointF *point, const QString& mesage = "", QPointF *base = 0) = 0;

    //! Select a entity.
    /*! Prompt message or a default message to the user asking for a sigle selection.
    * You can delete the Plug_Entity wen no more needed.
    * \param mesage an optional QString with prompt message.
    * \return a Plug_Entity handle the selected entity or NULL.
    */
    virtual Plug_Entity *getEnt(const QString& mesage = "") = 0;

    //! Gets a entities selection.
    /*! Prompt message or an default message to the user asking for a selection.
    * You can delete all, the Plug_Entity and the returned QList wen no more needed.
    * \param sel a QList of poiters to Plug_Entity handled the selected entites.
    * \param mesage an optional QString with prompt message.
    * \return true if succes.
    * \return false if fail, i.e. user cancel.
    */
    virtual bool getSelect(QList<Plug_Entity *> *sel, const QString& mesage = "") = 0;

    virtual bool getInt(int *num, const QString& mesage = "", const QString& title = "") = 0;
    virtual bool getReal(qreal *num, const QString& mesage = "", const QString& title = "") = 0;
    virtual bool getString(QString *txt, const QString& mesage = "", const QString& title = "") = 0;

};


#endif // DOCUMENT_INTERFACE_H
