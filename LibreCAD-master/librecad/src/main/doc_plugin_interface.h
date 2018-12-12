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
#ifndef DOC_PLUGIN_INTERFACE_H
#define DOC_PLUGIN_INTERFACE_H

#include <QObject>

#include "document_interface.h"
#include "rs_graphic.h"

class Doc_plugin_interface;

class convLTW
{
public:
    convLTW();
    QString lt2str(enum RS2::LineType lt);
    QString lw2str(enum RS2::LineWidth lw);
    QString intColor2str(int col);
    enum RS2::LineType str2lt(QString s);
    enum RS2::LineWidth str2lw(QString w);
private:
    QHash<RS2::LineType, QString> lType;
    QHash<RS2::LineWidth, QString> lWidth;
};

class Plugin_Entity
{
public:
    Plugin_Entity(RS_Entity* ent, Doc_plugin_interface* d);
    Plugin_Entity(RS_EntityContainer* parent, enum DPI::ETYPE type);
    virtual ~Plugin_Entity();
    bool isValid(){if (entity) return true; else return false;}
    RS_Entity* getEnt() {return entity;}
    virtual void getData(QHash<int, QVariant> *data);
    virtual void updateData(QHash<int, QVariant> *data);
    virtual void getPolylineData(QList<Plug_VertexData> *data);
    virtual void updatePolylineData(QList<Plug_VertexData> *data);

	virtual void move(QPointF offset);
	virtual void moveRotate(QPointF const& offset, QPointF const& center, double angle);
	virtual void rotate(QPointF center, double angle);
    virtual void scale(QPointF center, QPointF factor);
    virtual QString intColor2str(int color);
private:
    RS_Entity* entity;
    bool hasContainer;
    Doc_plugin_interface* dpi;
};

class Doc_plugin_interface : public Document_Interface
{
public:
    Doc_plugin_interface(RS_Document *d, RS_GraphicView* gv, QWidget* parent);
    void updateView();
    void addPoint(QPointF *start);
    void addLine(QPointF *start, QPointF *end);
    void addMText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va);
    void addText(QString txt, QString sty, QPointF *start,
            double height, double angle, DPI::HAlign ha,  DPI::VAlign va);

    void addCircle(QPointF *start, qreal radius);
    void addArc(QPointF *start, qreal radius, qreal a1, qreal a2);
    void addEllipse(QPointF *start, QPointF *end, qreal ratio, qreal a1, qreal a2);
    virtual void addLines(std::vector<QPointF> const& points, bool closed=false);
    virtual void addPolyline(std::vector<Plug_VertexData> const& points, bool closed=false);
    virtual void addSplinePoints(std::vector<QPointF> const& points, bool closed=false);
    void addImage(int handle, QPointF *start, QPointF *uvr, QPointF *vvr,
                  int w, int h, QString name, int br, int con, int fade);
    void addInsert(QString name, QPointF ins, QPointF scale, qreal rot);
    QString addBlockfromFromdisk(QString fullName);
    void addEntity(Plug_Entity *handle);
    Plug_Entity *newEntity( enum DPI::ETYPE type);
    void removeEntity(Plug_Entity *ent);
    void updateEntity(RS_Entity *org, RS_Entity *newe);

    void setLayer(QString name);
    QString getCurrentLayer();
    QStringList getAllLayer();
    QStringList getAllBlocks();
    bool deleteLayer(QString name);

    void getCurrentLayerProperties(int *c, DPI::LineWidth *w, DPI::LineType *t);
    void getCurrentLayerProperties(int *c, QString *w, QString *t);
    void setCurrentLayerProperties(int c, DPI::LineWidth w, DPI::LineType t);
	void setCurrentLayerProperties(int c, QString const& w, QString const& t);

    bool getPoint(QPointF *point, const QString& mesage, QPointF *base);
    Plug_Entity *getEnt(const QString& mesage);
    bool getSelect(QList<Plug_Entity *> *sel, const QString& mesage);
    bool getAllEntities(QList<Plug_Entity *> *sel, bool visible = false);

    bool getVariableInt(const QString& key, int *num);
    bool getVariableDouble(const QString& key, double *num);
    bool addVariable(const QString& key, int value, int code=70);
    bool addVariable(const QString& key, double value, int code=40);

    bool getInt(int *num, const QString& mesage, const QString& title);
    bool getReal(qreal *num, const QString& mesage, const QString& title);
    bool getString(QString *txt, const QString& mesage, const QString& title);
    QString realToStr(const qreal num, const int units = 0, const int prec = 0);

    //method to handle undo in Plugin_Entity 
    bool addToUndo(RS_Entity* current, RS_Entity* modified);
private:
    RS_Document *doc;
    RS_Graphic *docGr;
    RS_GraphicView *gView;
    QWidget* main_window;
};

/*void addArc(QPointF *start);			->Without start
void addCircle(QPointF *start);			->Without start
more...
*/
#endif // DOC_PLUGIN_INTERFACE_H
