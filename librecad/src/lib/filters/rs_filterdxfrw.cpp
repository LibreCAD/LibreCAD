/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
**  Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
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
**********************************************************************/


#include "rs_filterdxfrw.h"

#include <stdio.h>
//#include <map>

#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_leader.h"
#include "rs_spline.h"
#include "rs_system.h"

#include <QStringList>

#include <qtextcodec.h>


/**
 * Default constructor.
 *
 */
RS_FilterDXFRW::RS_FilterDXFRW()
    :RS_FilterInterface(),DRW_Interface() {

    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW()");

    currentContainer = NULL;
    graphic = NULL;
// Init hash to change the QCAD "normal" style to the more correct ISO-3059
// or draftsight symbol (AR*.shx) to sy*.lff
    fontList["normal"] = "iso";
    fontList["normallatin1"] = "iso";
    fontList["normallatin2"] = "iso";
    fontList["arastro"] = "syastro";
    fontList["armap"] = "symap";
    fontList["math"] = "symath";
    fontList["armeteo"] = "symeteo";
    fontList["armusic"] = "symusic";


    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW(): OK");
}

/**
 * Destructor.
 */
RS_FilterDXFRW::~RS_FilterDXFRW() {
    RS_DEBUG->print("RS_FilterDXFRW::~RS_FilterDXFRW(): OK");
}



/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterDXFRW::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("RS_FilterDXFRW::fileImport");

    RS_DEBUG->print("DXFRW Filter: importing file '%s'...", (const char*)QFile::encodeName(file));

    graphic = &g;
    currentContainer = graphic;
    this->file = file;
    // add some variables that need to be there for DXF drawings:
    graphic->addVariable("$DIMSTYLE", "Standard", 2);
    dimStyle = "Standard";
    codePage = "ANSI_1252";
    textStyle = "Standard";

    dxf = new dxfRW(QFile::encodeName(file));

    RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file");
    bool success = dxf->read(this, true);
    RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file: OK");
    //graphic->setAutoUpdateBorders(true);

    if (success==false) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "Cannot open DXF file '%s'.", (const char*)QFile::encodeName(file));
        return false;
    }

    RS_DEBUG->print("RS_FilterDXFRW::fileImport: updating inserts");
    graphic->updateInserts();

    RS_DEBUG->print("RS_FilterDXFRW::fileImport OK");

    delete dxf;
    return true;
}

/**
 * Implementation of the method which handles layers.
 */
void RS_FilterDXFRW::addLayer(const DRW_Layer &data) {
    RS_DEBUG->print("RS_FilterDXF::addLayer");
    RS_DEBUG->print("  adding layer: %s", data.name.c_str());

    RS_DEBUG->print("RS_FilterDXF::addLayer: creating layer");

    QString name = QString::fromUtf8(data.name.c_str());
    if (name != "0" && graphic->findLayer(name)!=NULL) {
        return;
    }
    RS_Layer* layer = new RS_Layer(name);
    RS_DEBUG->print("RS_FilterDXF::addLayer: set pen");
    layer->setPen(attributesToPen(&data));

    RS_DEBUG->print("RS_FilterDXF::addLayer: flags");
    if (data.flags&0x01) {
        layer->freeze(true);
    }
    if (data.flags&0x04) {
        layer->lock(true);
    }
    //help layer doesn't appear in printing
    layer->setHelpLayer(! data.plotF);

    RS_DEBUG->print("RS_FilterDXF::addLayer: add layer to graphic");
    graphic->addLayer(layer);
    RS_DEBUG->print("RS_FilterDXF::addLayer: OK");
}

/**
 * Implementation of the method which handles blocks.
 *
 * @todo Adding blocks to blocks (stack for currentContainer)
 */
void RS_FilterDXFRW::addBlock(const DRW_Block& data) {

    RS_DEBUG->print("RS_FilterDXF::addBlock");

    RS_DEBUG->print("  adding block: %s", data.name.c_str());
/*TODO correct handle of model-space*/

    QString name = QString::fromUtf8(data.name.c_str());
    QString mid = name.mid(1,11);
// Prevent special blocks (paper_space, model_space) from being added:
    if (mid.toLower() != "paper_space" && mid.toLower() != "model_space") {

            RS_Vector bp(data.basePoint.x, data.basePoint.y);
            RS_Block* block =
                new RS_Block(graphic, RS_BlockData(name, bp, false ));
            //block->setFlags(flags);

            if (graphic->addBlock(block)) {
                currentContainer = block;
            }
    }
}



/**
 * Implementation of the method which closes blocks.
 */
void RS_FilterDXFRW::endBlock() {
    if (currentContainer->rtti() == RS2::EntityBlock) {
        RS_Block *bk = (RS_Block *)currentContainer;
        //remove unnamed blocks *D only if version != R12
        if (version!=1009) {
            if (bk->getName().startsWith("*D") )
                graphic->removeBlock(bk);
        }
    }
    currentContainer = graphic;
}



/**
 * Implementation of the method which handles point entities.
 */
void RS_FilterDXFRW::addPoint(const DRW_Point& data) {
    RS_Vector v(data.basePoint.x, data.basePoint.y);

    RS_Point* entity = new RS_Point(currentContainer,
                                    RS_PointData(v));
    setEntityAttributes(entity, &data);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addLine(const DRW_Line& data) {
    RS_DEBUG->print("RS_FilterDXF::addLine");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addLine: create line");

    if (currentContainer==NULL) {
        RS_DEBUG->print("RS_FilterDXF::addLine: currentContainer is NULL");
    }

    RS_Line* entity = new RS_Line(currentContainer,
                                  RS_LineData(v1, v2));
    RS_DEBUG->print("RS_FilterDXF::addLine: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addLine: add entity");

    currentContainer->addEntity(entity);

    RS_DEBUG->print("RS_FilterDXF::addLine: OK");
}


/**
 * Implementation of the method which handles ray entities.
 */
void RS_FilterDXFRW::addRay(const DRW_Ray& data) {
    RS_DEBUG->print("RS_FilterDXF::addRay");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.basePoint.x+data.secPoint.x, data.basePoint.y+data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addRay: create line");

    if (currentContainer==NULL) {
        RS_DEBUG->print("RS_FilterDXF::addRay: currentContainer is NULL");
    }

    RS_Line* entity = new RS_Line(currentContainer,
                                  RS_LineData(v1, v2));
    RS_DEBUG->print("RS_FilterDXF::addRay: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addRay: add entity");

    currentContainer->addEntity(entity);

    RS_DEBUG->print("RS_FilterDXF::addRay: OK");
}


/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addXline(const DRW_Xline& data) {
    RS_DEBUG->print("RS_FilterDXF::addXline");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.basePoint.x+data.secPoint.x, data.basePoint.y+data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addXline: create line");

    if (currentContainer==NULL) {
        RS_DEBUG->print("RS_FilterDXF::addXline: currentContainer is NULL");
    }

    RS_Line* entity = new RS_Line(currentContainer,
                                  RS_LineData(v1, v2));
    RS_DEBUG->print("RS_FilterDXF::addXline: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addXline: add entity");

    currentContainer->addEntity(entity);

    RS_DEBUG->print("RS_FilterDXF::addXline: OK");
}



/**
 * Implementation of the method which handles circle entities.
 */
void RS_FilterDXFRW::addCircle(const DRW_Circle& data) {
    RS_DEBUG->print("RS_FilterDXF::addCircle");

    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_CircleData d(v, data.radious);
    RS_Circle* entity = new RS_Circle(currentContainer, d);
    setEntityAttributes(entity, &data);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles arc entities.
 *
 * @param angle1 Start angle in deg (!)
 * @param angle2 End angle in deg (!)
 */
void RS_FilterDXFRW::addArc(const DRW_Arc& data) {
    RS_DEBUG->print("RS_FilterDXF::addArc");
    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_ArcData d(v, data.radious,
                 data.staangle/ARAD,
                 data.endangle/ARAD,
                 false);
    RS_Arc* entity = new RS_Arc(currentContainer, d);
    setEntityAttributes(entity, &data);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles ellipse entities.
 *
 * @param angle1 Start angle in rad (!)
 * @param angle2 End angle in rad (!)
 */
void RS_FilterDXFRW::addEllipse(const DRW_Ellipse& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addEllipse");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);
    double ang2 = data.endparam;
    if ( fabs(ang2- 6.28318530718) < 1.0e-10 && fabs(data.staparam) < 1.0e-10 )
        ang2 = 0.0;
    RS_EllipseData ed(v1, v2, data.ratio, data.staparam,
                                    ang2, false);
    RS_Ellipse* entity = new RS_Ellipse(currentContainer, ed);
    setEntityAttributes(entity, &data);

    currentContainer->addEntity(entity);
}


/**
 * Implementation of the method which handles trace entities.
 */
void RS_FilterDXFRW::addTrace(const DRW_Trace& data) {
    RS_Solid* entity;
    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);
    RS_Vector v3(data.thirdPoint.x, data.thirdPoint.y);
    RS_Vector v4(data.fourPoint.x, data.fourPoint.y);
    if (v3 == v4)
        entity = new RS_Solid(currentContainer, RS_SolidData(v1, v2, v3));
    else
        entity = new RS_Solid(currentContainer, RS_SolidData(v1, v2, v3,v4));

    setEntityAttributes(entity, &data);
    currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles solid entities.
 */
void RS_FilterDXFRW::addSolid(const DRW_Solid& data) {
    addTrace(data);
}

/**
 * Implementation of the method which handles lightweight polyline entities.
 */
void RS_FilterDXFRW::addLWPolyline(const DRW_LWPolyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addLWPolyline");
    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      data.flags&0x1);
    RS_Polyline *polyline = new RS_Polyline(currentContainer, d);
    setEntityAttributes(polyline, &data);

    for (unsigned int i=0; i<data.vertlist.size(); i++) {
        DRW_Vertex2D *vert = data.vertlist.at(i);
        RS_Vector v(vert->x, vert->y);
        polyline->addVertex(v, vert->bulge);
    }

    currentContainer->addEntity(polyline);
}


/**
 * Implementation of the method which handles polyline entities.
 */
void RS_FilterDXFRW::addPolyline(const DRW_Polyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addPolyline");
    if ( data.flags&0x10)
        return; //the polyline is a polygon mesh, not handled

    if ( data.flags&0x40)
        return; //the polyline is a poliface mesh, TODO convert

    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      data.flags&0x1);
    RS_Polyline *polyline = new RS_Polyline(currentContainer, d);
    setEntityAttributes(polyline, &data);
    for (unsigned int i=0; i<data.vertlist.size(); i++) {
        DRW_Vertex *vert = data.vertlist.at(i);
        RS_Vector v(vert->basePoint.x, vert->basePoint.y);
        polyline->addVertex(v, vert->bulge);
    }

    currentContainer->addEntity(polyline);
}


/**
 * Implementation of the method which handles splines.
 */
void RS_FilterDXFRW::addSpline(const DRW_Spline* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addSpline: degree: %d", data->degree);

        RS_Spline* spline;
        if (data->degree>=1 && data->degree<=3) {
        RS_SplineData d(data->degree, ((data->flags&0x1)==0x1));
        spline = new RS_Spline(currentContainer, d);
        setEntityAttributes(spline, data);

        currentContainer->addEntity(spline);
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::addSpline: Invalid degree for spline: %d. "
                        "Accepted values are 1..3.", data->degree);
        return;
    }
    for (unsigned int i=0; i<data->controllist.size(); i++) {
        DRW_Coord *vert = data->controllist.at(i);
        RS_Vector v(vert->x, vert->y);
        spline->addControlPoint(v);
    }
    spline->update();
}


/**
 * Implementation of the method which handles inserts.
 */
void RS_FilterDXFRW::addInsert(const DRW_Insert& data) {

    RS_DEBUG->print("RS_FilterDXF::addInsert");

    RS_Vector ip(data.basePoint.x, data.basePoint.y);
    RS_Vector sc(data.xscale, data.yscale);
    RS_Vector sp(data.colspace, data.rowspace);

    //cout << "Insert: " << name << " " << ip << " " << cols << "/" << rows << endl;

    RS_InsertData d( QString::fromUtf8(data.name.c_str()),
                    ip, sc, data.angle/ARAD,
                    data.colcount, data.rowcount,
                    sp, NULL, RS2::NoUpdate);
    RS_Insert* entity = new RS_Insert(currentContainer, d);
    setEntityAttributes(entity, &data);
    RS_DEBUG->print("  id: %d", entity->getId());
//    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * multi texts (MTEXT).
 */
void RS_FilterDXFRW::addMText(const DRW_MText& data) {
    RS_DEBUG->print("RS_FilterDXF::addMText: %s", data.text.c_str());

    RS_MTextData::VAlign valign;
    RS_MTextData::HAlign halign;
    RS_MTextData::MTextDrawingDirection dir;
    RS_MTextData::MTextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data.style.c_str());
    sty=sty.toLower();

    if (data.textgen<=3) {
        valign=RS_MTextData::VATop;
    } else if (data.textgen<=6) {
        valign=RS_MTextData::VAMiddle;
    } else {
        valign=RS_MTextData::VABottom;
    }

    if (data.textgen%3==1) {
        halign=RS_MTextData::HALeft;
    } else if (data.textgen%3==2) {
        halign=RS_MTextData::HACenter;
    } else {
        halign=RS_MTextData::HARight;
    }

    if (data.alignH==1) {
        dir = RS_MTextData::LeftToRight;
    } else if (data.alignH==3) {
        dir = RS_MTextData::TopToBottom;
    } else {
        dir = RS_MTextData::ByStyle;
    }

    if (data.alignV==1) {
        lss = RS_MTextData::AtLeast;
    } else {
        lss = RS_MTextData::Exact;
    }

    QString mtext = toNativeString(QString::fromUtf8(data.text.c_str()));
    // use default style for the drawing:
    if (sty.isEmpty()) {
        // japanese, cyrillic:
        if (codePage=="ANSI_932" || codePage=="ANSI_1251") {
            sty = "Unicode";
        } else {
            sty = textStyle;
        }
    } else {
        sty = fontList.value(sty, sty);
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(mtext);

    RS_MTextData d(RS_Vector(data.basePoint.x, data.basePoint.y), data.height, data.widthscale,
                  valign, halign,
                  dir, lss,
                  data.interlin,
                  mtext, sty, data.angle*M_PI/180,
                  RS2::NoUpdate);
    RS_MText* entity = new RS_MText(currentContainer, d);

    setEntityAttributes(entity, &data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * texts (TEXT).
 */
void RS_FilterDXFRW::addText(const DRW_Text& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addText");
    RS_Vector refPoint = RS_Vector(data.basePoint.x, data.basePoint.y);;
    RS_Vector secPoint = RS_Vector(data.secPoint.x, data.secPoint.y);;
    double angle = data.angle;

    if (data.alignV !=0 || data.alignH ==DRW_Text::HMiddle){
            secPoint = RS_Vector(data.basePoint.x, data.basePoint.y);;
            refPoint = RS_Vector(data.secPoint.x, data.secPoint.y);;
    }

    RS_TextData::VAlign valign = (RS_TextData::VAlign)data.alignV;
    RS_TextData::HAlign halign = (RS_TextData::HAlign)data.alignH;
    RS_TextData::TextGeneration dir;
    QString sty = QString::fromUtf8(data.style.c_str());
    sty=sty.toLower();

    if (data.textgen==2) {
        dir = RS_TextData::Backward;
    } else if (data.textgen==4) {
        dir = RS_TextData::UpsideDown;
    } else {
        dir = RS_TextData::None;
    }

    QString mtext = toNativeString(QString::fromUtf8(data.text.c_str()));
    // use default style for the drawing:
    if (sty.isEmpty()) {
        // japanese, cyrillic:
        if (codePage=="ANSI_932" || codePage=="ANSI_1251") {
            sty = "Unicode";
        } else {
            sty = textStyle;
        }
    } else {
        sty = fontList.value(sty, sty);
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(mtext);

    RS_TextData d(refPoint, secPoint, data.height, data.widthscale,
                  valign, halign, dir,
                  mtext, sty, angle*M_PI/180,
                  RS2::NoUpdate);
    RS_Text* entity = new RS_Text(currentContainer, d);

    setEntityAttributes(entity, &data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * dimensions (DIMENSION).
 */
RS_DimensionData RS_FilterDXFRW::convDimensionData(const  DRW_Dimension* data) {

    DRW_Coord crd = data->getDefPoint();
    RS_Vector defP(crd.x, crd.y);
    crd = data->getTextPoint();
    RS_Vector midP(crd.x, crd.y);
    RS_MTextData::VAlign valign;
    RS_MTextData::HAlign halign;
    RS_MTextData::MTextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data->getStyle().c_str());

    QString t; //= data.text;

    // middlepoint of text can be 0/0 which is considered to be invalid (!):
    //  0/0 because older QCad versions save the middle of the text as 0/0
    //  althought they didn't suport saving of the middle of the text.
    if (fabs(crd.x)<1.0e-6 && fabs(crd.y)<1.0e-6) {
        midP = RS_Vector(false);
    }

    if (data->getAlign()<=3) {
        valign=RS_MTextData::VATop;
    } else if (data->getAlign()<=6) {
        valign=RS_MTextData::VAMiddle;
    } else {
        valign=RS_MTextData::VABottom;
    }

    if (data->getAlign()%3==1) {
        halign=RS_MTextData::HALeft;
    } else if (data->getAlign()%3==2) {
        halign=RS_MTextData::HACenter;
    } else {
        halign=RS_MTextData::HARight;
    }

    if (data->getTextLineStyle()==1) {
        lss = RS_MTextData::AtLeast;
    } else {
        lss = RS_MTextData::Exact;
    }

    t = QString::fromUtf8(data->getText().c_str());

    if (sty.isEmpty()) {
        sty = dimStyle;
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(t);

    // data needed to add the actual dimension entity
    return RS_DimensionData(defP, midP,
                            valign, halign,
                            lss,
                            data->getTextLineFactor(),
                            t, sty, data->getDir());
}



/**
 * Implementation of the method which handles
 * aligned dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAlign(const DRW_DimAligned *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAligned");

    RS_DimensionData dimensionData = convDimensionData((DRW_Dimension*)data);

    RS_Vector ext1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector ext2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimAlignedData d(ext1, ext2);

    RS_DimAligned* entity = new RS_DimAligned(currentContainer,
                            dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * linear dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimLinear(const DRW_DimLinear *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLinear");

    RS_DimensionData dimensionData = convDimensionData((DRW_Dimension*)data);

    RS_Vector dxt1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector dxt2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimLinearData d(dxt1, dxt2, RS_Math::deg2rad(data->getAngle()),
                       RS_Math::deg2rad(data->getOblique()));

    RS_DimLinear* entity = new RS_DimLinear(currentContainer,
                                            dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * radial dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimRadial(const DRW_DimRadial* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimRadial");

    RS_DimensionData dimensionData = convDimensionData((DRW_Dimension*)data);
    RS_Vector dp(data->getDiameterPoint().x, data->getDiameterPoint().y);

    RS_DimRadialData d(dp, data->getLeaderLength());

    RS_DimRadial* entity = new RS_DimRadial(currentContainer,
                                            dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * diametric dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimDiametric(const DRW_DimDiametric* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimDiametric");

    RS_DimensionData dimensionData = convDimensionData((DRW_Dimension*)data);
    RS_Vector dp(data->getDiameter1Point().x, data->getDiameter1Point().y);

    RS_DimDiametricData d(dp, data->getLeaderLength());

    RS_DimDiametric* entity = new RS_DimDiametric(currentContainer,
                              dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular(const DRW_DimAngular* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine1().x, data->getFirstLine1().y);
    RS_Vector dp2(data->getFirstLine2().x, data->getFirstLine2().y);
    RS_Vector dp3(data->getSecondLine1().x, data->getSecondLine1().y);
    RS_Vector dp4(data->getDimPoint().x, data->getDimPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    RS_DimAngular* entity = new RS_DimAngular(currentContainer,
                            dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular3P(const DRW_DimAngular3p* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular3P");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine().x, data->getFirstLine().y);
    RS_Vector dp2(data->getSecondLine().x, data->getSecondLine().y);
    RS_Vector dp3(data->getVertexPoint().x, data->getVertexPoint().y);
    RS_Vector dp4 = dimensionData.definitionPoint;
    dimensionData.definitionPoint = RS_Vector(data->getVertexPoint().x, data->getVertexPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    RS_DimAngular* entity = new RS_DimAngular(currentContainer,
                            dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    currentContainer->addEntity(entity);
}



void RS_FilterDXFRW::addDimOrdinate(const DRW_DimOrdinate* /*data*/) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) not yet implemented");
}


/**
 * Implementation of the method which handles leader entities.
 */
void RS_FilterDXFRW::addLeader(const DRW_Leader *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLeader");
    RS_LeaderData d(data->arrow!=0);
    RS_Leader* leader = new RS_Leader(currentContainer, d);
    setEntityAttributes(leader, data);

    currentContainer->addEntity(leader);

    for (unsigned int i=0; i<data->vertexlist.size(); i++) {
        DRW_Coord *vert = data->vertexlist.at(i);
        RS_Vector v(vert->x, vert->y);
        leader->addVertex(v);
    }
}



/**
 * Implementation of the method which handles hatch entities.
 */
void RS_FilterDXFRW::addHatch(const DRW_Hatch *data) {
    RS_DEBUG->print("RS_FilterDXF::addHatch()");
    RS_Hatch* hatch;
    RS_EntityContainer* hatchLoop;

    hatch = new RS_Hatch(currentContainer,
                         RS_HatchData(data->solid, data->scale, data->angle,
                                      QString::fromUtf8(data->name.c_str())));
    setEntityAttributes(hatch, data);
    currentContainer->appendEntity(hatch);

    for (unsigned int i=0; i < data->looplist.size(); i++) {
        DRW_HatchLoop *loop = data->looplist.at(i);
        if ((loop->type & 32) == 32) continue;
        hatchLoop = new RS_EntityContainer(hatch);
        hatchLoop->setLayer(NULL);
        hatch->addEntity(hatchLoop);

        RS_Entity* e = NULL;
        if ((loop->type & 2) == 2){   //polyline, convert to lines & arcs
            DRW_LWPolyline *pline = (DRW_LWPolyline *)loop->objlist.at(0);
            RS_Polyline *polyline = new RS_Polyline(NULL,
                    RS_PolylineData(RS_Vector(false), RS_Vector(false), pline->flags) );
            for (unsigned int j=0; j < pline->vertlist.size(); j++) {
                    DRW_Vertex2D *vert = pline->vertlist.at(j);
                    polyline->addVertex(RS_Vector(vert->x, vert->y), vert->bulge);
            }
            for (RS_Entity* e=polyline->firstEntity(); e!=NULL;
                    e=polyline->nextEntity()) {
                RS_Entity* tmp = e->clone();
                tmp->reparent(hatchLoop);
                tmp->setLayer(NULL);
                hatchLoop->addEntity(tmp);
            }
            delete polyline;

        } else {
            for (unsigned int j=0; j<loop->objlist.size(); j++) {
                e = NULL;
                DRW_Entity *ent = loop->objlist.at(j);
                switch (ent->eType) {
                case DRW::LINE: {
                    DRW_Line *e2 = (DRW_Line *)ent;
                    e = new RS_Line(hatchLoop,
                                    RS_LineData(RS_Vector(e2->basePoint.x, e2->basePoint.y),
                                                RS_Vector(e2->secPoint.x, e2->secPoint.y)));
                    break;
                }
                case DRW::ARC: {
                    DRW_Arc *e2 = (DRW_Arc *)ent;
                    if (e2->isccw && e2->staangle<1.0e-6 && e2->endangle>360-1.0e-6) {
                        e = new RS_Circle(hatchLoop,
                                          RS_CircleData(RS_Vector(e2->basePoint.x, e2->basePoint.y),
                                                        e2->radious));
                    } else {

                        if (e2->isccw) {
                            e = new RS_Arc(hatchLoop,
                                        RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                   RS_Math::correctAngle(RS_Math::deg2rad(e2->staangle)),
                                                   RS_Math::correctAngle(RS_Math::deg2rad(e2->endangle)),
                                                   false));
                        } else {
                            e = new RS_Arc(hatchLoop,
                                        RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                   RS_Math::correctAngle(2*M_PI-RS_Math::deg2rad(e2->staangle)),
                                                   RS_Math::correctAngle(2*M_PI-RS_Math::deg2rad(e2->endangle)),
                                                   true));
                        }
                    }
                    break;
                }
                default:
                    break;
                }
                if (e!=NULL) {
                    e->setLayer(NULL);
                    hatchLoop->addEntity(e);
                }
            }
        }

    }

    RS_DEBUG->print("hatch->update()");
    if (hatch->validate()) {
        hatch->update();
    } else {
        graphic->removeEntity(hatch);
        RS_DEBUG->print(RS_Debug::D_ERROR,
                    "RS_FilterDXFRW::endEntity(): updating hatch failed: invalid hatch area");
    }
}


/**
 * Implementation of the method which handles image entities.
 */
void RS_FilterDXFRW::addImage(const DRW_Image *data) {
    RS_DEBUG->print("RS_FilterDXF::addImage");

    RS_Vector ip(data->basePoint.x, data->basePoint.y);
    RS_Vector uv(data->secPoint.x, data->secPoint.y);
    RS_Vector vv(data->vx, data->vy);
    RS_Vector size(data->sizeu, data->sizev);

    RS_Image* image = new RS_Image( currentContainer,
            RS_ImageData(QString(data->ref.c_str()).toInt(NULL, 16),
                         ip, uv, vv, size,
                         QString(""), data->brightness,
                         data->contrast, data->fade));

    setEntityAttributes(image, data);
    currentContainer->appendEntity(image);
}



/**
 * Implementation of the method which links image entities to image files.
 */
void RS_FilterDXFRW::linkImage(const DRW_ImageDef *data) {
    RS_DEBUG->print("RS_FilterDXFRW::linkImage");

    int handle = QString(data->handle.c_str()).toInt(NULL, 16);
    QString sfile(QString::fromUtf8(data->name.c_str()));
    QFileInfo fiDxf(file);
    QFileInfo fiBitmap(sfile);

    // try to find the image file:

    // first: absolute path:
    if (!fiBitmap.exists()) {
        RS_DEBUG->print("File %s doesn't exist.",
                        (const char*)QFile::encodeName(sfile));
        // try relative path:
        QString f1 = fiDxf.absolutePath() + "/" + sfile;
        if (QFileInfo(f1).exists()) {
            sfile = f1;
        } else {
            RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f1));
            // try drawing path:
            QString f2 = fiDxf.absolutePath() + "/" + fiBitmap.fileName();
            if (QFileInfo(f2).exists()) {
                sfile = f2;
            } else {
                RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f2));
            }
        }
    }

    // Also link images in subcontainers (e.g. inserts):
    for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
            e!=NULL; e=graphic->nextEntity(RS2::ResolveNone)) {
        if (e->rtti()==RS2::EntityImage) {
            RS_Image* img = (RS_Image*)e;
            if (img->getHandle()==handle) {
                img->setFile(sfile);
                RS_DEBUG->print("image found: %s", (const char*)QFile::encodeName(img->getFile()));
                img->update();
            }
        }
    }

    // update images in blocks:
    for (uint i=0; i<graphic->countBlocks(); ++i) {
        RS_Block* b = graphic->blockAt(i);
        for (RS_Entity* e=b->firstEntity(RS2::ResolveNone);
                e!=NULL; e=b->nextEntity(RS2::ResolveNone)) {
            if (e->rtti()==RS2::EntityImage) {
                RS_Image* img = (RS_Image*)e;
                if (img->getHandle()==handle) {
                    img->setFile(sfile);
                    RS_DEBUG->print("image in block found: %s",
                                    (const char*)QFile::encodeName(img->getFile()));
                    img->update();
                }
            }
        }
    }
    RS_DEBUG->print("linking image: OK");
}

using std::map;
/**
 * Sets the header variables from the DXF file.
 */
void RS_FilterDXFRW::addHeader(const DRW_Header* data){
    RS_Graphic* container = NULL;
    if (currentContainer->rtti()==RS2::EntityGraphic) {
        container = (RS_Graphic*)currentContainer;
    } else return;

    map<std::string,DRW_Variant *>::const_iterator it;
    for ( it=data->vars.begin() ; it != data->vars.end(); it++ ){
        QString key = QString::fromStdString((*it).first);
        DRW_Variant *var = (*it).second;
        switch (var->type) {
        case DRW_Variant::COORD:
            container->addVariable(key,
#ifdef  RS_VECTOR2D
            RS_Vector(var->content.v->x, var->content.v->y), var->code);
#else
            RS_Vector(var->content.v->x, var->content.v->y, var->content.v->z), var->code);
#endif
            break;
        case DRW_Variant::STRING:
            container->addVariable(key, QString::fromUtf8(var->content.s->c_str()), var->code);
            break;
        case DRW_Variant::INTEGER:
            container->addVariable(key, var->content.i, var->code);
            break;
        case DRW_Variant::DOUBLE:
            container->addVariable(key, var->content.d, var->code);
            break;
        default:
            break;
        }

    }
    codePage = graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252");
    textStyle = graphic->getVariableString("$TEXTSTYLE", "Standard");
    dimStyle = graphic->getVariableString("$DIMSTYLE", "Standard");

    QString acadver = versionStr = graphic->getVariableString("$ACADVER", "");
    acadver.replace(QRegExp("[a-zA-Z]"), "");
    bool ok;
    version=acadver.toInt(&ok);
    if (!ok) { version = 1021;}
}


/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the DXF file that will be written.
 */
bool RS_FilterDXFRW::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) {

    RS_DEBUG->print("RS_FilterDXFDW::fileExport: exporting file '%s'...",
                    (const char*)QFile::encodeName(file));
    RS_DEBUG->print("RS_FilterDXFDW::fileExport: file type '%d'", (int)type);

    this->graphic = &g;

    // check if we can write to that directory:
#ifndef Q_OS_WIN

    QString path = QFileInfo(file).absolutePath();
    if (QFileInfo(path).isWritable()==false) {
        RS_DEBUG->print("RS_FilterDXFRW::fileExport: can't write file: "
                        "no permission");
        return false;
    }
    //
#endif

    // set version for DXF filter:
    DRW::Version exportVersion;
    if (type==RS2::FormatDXFRW12) {
        exportVersion = DRW::AC1009;
        version = 1009;
    } else if (type==RS2::FormatDXFRW14) {
        exportVersion = DRW::AC1014;
        version = 1014;
    } else if (type==RS2::FormatDXFRW2000) {
        exportVersion = DRW::AC1015;
        version = 1015;
    } else if (type==RS2::FormatDXFRW2004) {
        exportVersion = DRW::AC1018;
        version = 1018;
    } else {
        exportVersion = DRW::AC1021;
        version = 1021;
    }

    dxf = new dxfRW(QFile::encodeName(file));
    bool success = dxf->write(this, exportVersion, false); //ascii
//    bool success = dxf->write(this, exportVersion, true); //binary

    if (!success) {
        RS_DEBUG->print("RS_FilterDXFDW::fileExport: can't write file");
        return false;
    }
/*RLZ pte*/
/*    RS_DEBUG->print("writing tables...");
    dw->sectionTables();
    // VPORT:
    dxf.writeVPort(*dw);
    dw->tableEnd();

    // STYLE:
    RS_DEBUG->print("writing styles...");
    dxf.writeStyle(*dw);

    // VIEW:
    RS_DEBUG->print("writing views...");
    dxf.writeView(*dw);

    // UCS:
    RS_DEBUG->print("writing ucs...");
    dxf.writeUcs(*dw);

    // Appid:
    RS_DEBUG->print("writing appid...");
    dw->tableAppid(1);
    writeAppid(*dw, "ACAD");
    dw->tableEnd();
*/
    return success;
}

/**
 * Writes blocks (just the definition, not the entities in it).
 */
void RS_FilterDXFRW::writeBlockRecords(){
    RS_Block *blk;
    for (uint i = 0; i < graphic->countBlocks(); i++) {
        blk = graphic->blockAt(i);
        RS_DEBUG->print("writing block record: %s", (const char*)blk->getName().toLocal8Bit());
        dxf->writeBlockRecord(blk->getName().toUtf8().data());
    }
}

void RS_FilterDXFRW::writeBlocks() {
    RS_Block *blk;

    if (version==1009) {
        int dimNum = 0, hatchNum= 0;
        QString prefix, sufix;
        //check for existing *D?? or  *U??
        for (uint i = 0; i < graphic->countBlocks(); i++) {
            blk = graphic->blockAt(i);
            prefix = blk->getName().left(2).toUpper();
            sufix = blk->getName().mid(2);
            if (prefix == "*D") {
                if (sufix.toInt() > dimNum) dimNum = sufix.toInt();
            } else if (prefix == "*U") {
                if (sufix.toInt() > hatchNum) hatchNum = sufix.toInt();
            }
        }
        for (RS_Entity *e = graphic->firstEntity(RS2::ResolveNone);
             e != NULL; e = graphic->nextEntity(RS2::ResolveNone)) {
            if ( !(e->getFlag(RS2::FlagUndone)) ) {
                switch (e->rtti()) {
                case RS2::EntityDimLinear:
                case RS2::EntityDimAligned:
                case RS2::EntityDimAngular:
                case RS2::EntityDimRadial:
                case RS2::EntityDimDiametric:
                case RS2::EntityDimLeader:
                    prefix = "*D" + ++dimNum;
                    noNameBlock[e] = prefix;
                    break;
                case RS2::EntityHatch:
                    if ( !((RS_Hatch*)e)->isSolid() ) {
                        prefix = "*U" + ++hatchNum;
                        noNameBlock[e] = prefix;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        QHash<RS_Entity*, QString>::const_iterator it = noNameBlock.constBegin();
        while (it != noNameBlock.constEnd()) {
            DRW_Block block;
            block.name = it.value().toStdString();
            block.basePoint.x = 0.0;
            block.basePoint.y = 0.0;
    #ifndef  RS_VECTOR2D
            block.basePoint.z = 0.0;
    #endif
            dxf->writeBlock(&block);
            RS_EntityContainer *ct = (RS_EntityContainer *)it.key();
            for (RS_Entity* e=ct->firstEntity(RS2::ResolveNone);
                    e!=NULL; e=ct->nextEntity(RS2::ResolveNone)) {
                writeEntity(e);
            }
            ++it;
        }
    } //end version R12

    for (uint i = 0; i < graphic->countBlocks(); i++) {
        blk = graphic->blockAt(i);
        RS_DEBUG->print("writing block: %s", (const char*)blk->getName().toLocal8Bit());

        DRW_Block block;
        block.name = blk->getName().toUtf8().data();
        block.basePoint.x = blk->getBasePoint().x;
        block.basePoint.y = blk->getBasePoint().y;
#ifndef  RS_VECTOR2D
        block.basePoint.z = blk->getBasePoint().z;
#endif
        dxf->writeBlock(&block);
        for (RS_Entity* e=blk->firstEntity(RS2::ResolveNone);
                e!=NULL; e=blk->nextEntity(RS2::ResolveNone)) {
            writeEntity(e);
        }
    }
}


void RS_FilterDXFRW::writeHeader(DRW_Header& data){
    DRW_Variant *curr;
/*TODO $ISOMETRICGRID == $SNAPSTYLE and "GRID on/off" not handled because is part of
 active vport to save is required read/write VPORT table */
    QHash<QString, RS_Variable>vars = graphic->getVariableDict();
    QHash<QString, RS_Variable>::iterator it = vars.begin();
    if (!vars.contains ( "$DWGCODEPAGE" )) {
//RLZ: TODO verify this
        codePage = RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit());
//        RS_Variable v( QString(RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit())),0 );
        vars.insert(QString("$DWGCODEPAGE"), RS_Variable(codePage, 0) );
    }

    while (it != vars.end()) {
        curr = new DRW_Variant();

            switch (it.value().getType()) {
            case RS2::VariableInt:
                curr->addInt(it.value().getInt());
                curr->code = it.value().getCode();
                break;
            case RS2::VariableDouble:
                curr->addDouble(it.value().getDouble());
                curr->code = it.value().getCode();
                break;
            case RS2::VariableString:
                curr->addString(it.value().getString().toUtf8().data() );
                curr->code = it.value().getCode();
                break;
            case RS2::VariableVector:
                curr->addCoord(new DRW_Coord());
                curr->setCoordX(it.value().getVector().x);
                curr->setCoordY(it.value().getVector().y);
#ifndef  RS_VECTOR2D
                curr->setCoordZ(it.value().getVector().z);
#endif
                curr->code = it.value().getCode();
                break;
            default:
                break;
            }
            data.vars[it.key().toStdString()] =curr;
            ++it;
    }
}

void RS_FilterDXFRW::writeLTypes(){
    DRW_LType ltype;
    // Standard linetypes for LibreCAD / AutoCAD
    ltype.name = "CONTINUOUS";
    ltype.desc = "Solid line";
    dxf->writeLineType(&ltype);
    ltype.name = "ByLayer";
    dxf->writeLineType(&ltype);
    ltype.name = "ByBlock";
    dxf->writeLineType(&ltype);

    ltype.name = "DOT";
    ltype.desc = "Dot . . . . . . . . . . . . . . . . . . . . . .";
    ltype.size = 2;
    ltype.length = 6.35;
    ltype.path.push_back(0.0);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DOT2";
    ltype.desc = "Dot (.5x) .....................................";
    ltype.size = 2;
    ltype.length = 3.175;
    ltype.path.push_back(0.0);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DOTX2";
    ltype.desc = "Dot (2x) .  .  .  .  .  .  .  .  .  .  .  .  .";
    ltype.size = 2;
    ltype.length = 12.7;
    ltype.path.push_back(0.0);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHED";
    ltype.desc = "Dot . . . . . . . . . . . . . . . . . . . . . .";
    ltype.size = 2;
    ltype.length = 19.05;
    ltype.path.push_back(12.7);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHED2";
    ltype.desc = "Dashed (.5x) _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _";
    ltype.size = 2;
    ltype.length = 9.525;
    ltype.path.push_back(6.35);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHEDX2";
    ltype.desc = "Dashed (2x) ____  ____  ____  ____  ____  ___";
    ltype.size = 2;
    ltype.length = 38.1;
    ltype.path.push_back(25.4);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHDOT";
    ltype.desc = "Dash dot __ . __ . __ . __ . __ . __ . __ . __";
    ltype.size = 4;
    ltype.length = 25.4;
    ltype.path.push_back(12.7);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHDOT2";
    ltype.desc = "Dash dot (.5x) _._._._._._._._._._._._._._._.";
    ltype.size = 4;
    ltype.length = 12.7;
    ltype.path.push_back(6.35);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DASHDOTX2";
    ltype.desc = "Dash dot (2x) ____  .  ____  .  ____  .  ___";
    ltype.size = 4;
    ltype.length = 50.8;
    ltype.path.push_back(25.4);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DIVIDE";
    ltype.desc = "Divide ____ . . ____ . . ____ . . ____ . . ____";
    ltype.size = 6;
    ltype.length = 31.75;
    ltype.path.push_back(12.7);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DIVIDE2";
    ltype.desc = "Divide (.5x) __..__..__..__..__..__..__..__.._";
    ltype.size = 6;
    ltype.length = 15.875;
    ltype.path.push_back(6.35);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "DIVIDEX2";
    ltype.desc = "Divide (2x) ________  .  .  ________  .  .  _";
    ltype.size = 6;
    ltype.length = 63.5;
    ltype.path.push_back(25.4);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "BORDER";
    ltype.desc = "Border __ __ . __ __ . __ __ . __ __ . __ __ .";
    ltype.size = 6;
    ltype.length = 44.45;
    ltype.path.push_back(12.7);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(12.7);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "BORDER2";
    ltype.desc = "Border (.5x) __.__.__.__.__.__.__.__.__.__.__.";
    ltype.size = 6;
    ltype.length = 22.225;
    ltype.path.push_back(6.35);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(6.35);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "BORDERX2";
    ltype.desc = "Border (2x) ____  ____  .  ____  ____  .  ___";
    ltype.size = 6;
    ltype.length = 88.9;
    ltype.path.push_back(25.4);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(25.4);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(0.0);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "CENTER";
    ltype.desc = "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____";
    ltype.size = 4;
    ltype.length = 50.8;
    ltype.path.push_back(31.75);
    ltype.path.push_back(-6.35);
    ltype.path.push_back(6.35);
    ltype.path.push_back(-6.35);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "CENTER2";
    ltype.desc = "Center (.5x) ___ _ ___ _ ___ _ ___ _ ___ _ ___";
    ltype.size = 4;
    ltype.length = 28.575;
    ltype.path.push_back(19.05);
    ltype.path.push_back(-3.175);
    ltype.path.push_back(3.175);
    ltype.path.push_back(-3.175);
    dxf->writeLineType(&ltype);

    ltype.path.clear();
    ltype.name = "CENTERX2";
    ltype.desc = "Center (2x) ________  __  ________  __  _____";
    ltype.size = 4;
    ltype.length = 101.6;
    ltype.path.push_back(63.5);
    ltype.path.push_back(-12.7);
    ltype.path.push_back(12.7);
    ltype.path.push_back(-12.7);
    dxf->writeLineType(&ltype);
}

void RS_FilterDXFRW::writeLayers(){
    DRW_Layer lay;
    RS_LayerList* ll = graphic->getLayerList();
    for (unsigned int i = 0; i < ll->count(); i++) {
        RS_Layer* l = ll->at(i);
        RS_Pen pen = l->getPen();
        lay.name = l->getName().toUtf8().data();
        lay.color = colorToNumber(pen.getColor());
        lay.lWeight = widthToNumber(pen.getWidth());
        lay.lineType = lineTypeToName(pen.getLineType()).toStdString();
        lay.plotF = ! l->isHelpLayer(); // a help layer should not appear in print
//        lay.lineType = lineType.toStdString(); //.toLatin1().data();
        dxf->writeLayer(&lay);
    }
}

void RS_FilterDXFRW::writeTextstyles(){
//    DRW_Textstyle ts;
//    dxf->writeTextstyle(&ts);
}

void RS_FilterDXFRW::writeVports(){
//    DRW_Vport vp;
//    dxf->writeVport(&vp);
}


void RS_FilterDXFRW::writeDimstyles(){
    DRW_Dimstyle dsty;
    dsty.name = "Standard";
    dsty.dimasz = graphic->getVariableDouble("$DIMASZ", 2.5);
    dsty.dimexe = graphic->getVariableDouble("$DIMEXE", 1.25);
    dsty.dimexo = graphic->getVariableDouble("$DIMEXO", 0.625);
    dsty.dimgap = graphic->getVariableDouble("$DIMGAP", 0.625);
    dsty.dimtxt = graphic->getVariableDouble("$DIMTXT", 2.5);
    dxf->writeDimstyle(&dsty);
}

void RS_FilterDXFRW::writeEntities(){
    for (RS_Entity *e = graphic->firstEntity(RS2::ResolveNone);
         e != NULL; e = graphic->nextEntity(RS2::ResolveNone)) {
        if ( !(e->getFlag(RS2::FlagUndone)) ) {
            writeEntity(e);
        }
    }
}

void RS_FilterDXFRW::writeEntity(RS_Entity* e){
    switch (e->rtti()) {
    case RS2::EntityPoint:
        writePoint((RS_Point*)e);
        break;
    case RS2::EntityLine:
        writeLine((RS_Line*)e);
        break;
    case RS2::EntityCircle:
        writeCircle((RS_Circle*)e);
        break;
    case RS2::EntityArc:
        writeArc((RS_Arc*)e);
        break;
    case RS2::EntitySolid:
        writeSolid((RS_Solid*)e);
        break;
    case RS2::EntityEllipse:
        writeEllipse((RS_Ellipse*)e);
        break;
    case RS2::EntityPolyline:
        writeLWPolyline((RS_Polyline*)e);
        break;
    case RS2::EntitySpline:
        writeSpline((RS_Spline*)e);
        break;
//    case RS2::EntityVertex:
//        break;
    case RS2::EntityInsert:
        writeInsert((RS_Insert*)e);
        break;
    case RS2::EntityMText:
        writeMText((RS_MText*)e);
        break;
    case RS2::EntityText:
        writeText((RS_Text*)e);
        break;
    case RS2::EntityDimLinear:
    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimRadial:
    case RS2::EntityDimDiametric:
        writeDimension((RS_Dimension*)e);
        break;
    case RS2::EntityDimLeader:
        writeLeader((RS_Leader*)e);
        break;
    case RS2::EntityHatch:
        writeHatch((RS_Hatch*)e);
        break;
    case RS2::EntityImage:
        writeImage((RS_Image*)e);
        break;
    default:
        break;
    }
}


/**
 * Writes the given Point entity to the file.
 */
void RS_FilterDXFRW::writePoint(RS_Point* p) {
    DRW_Point point;
    getEntityAttributes(&point, p);
    point.basePoint.x = p->getStartpoint().x;
    point.basePoint.y = p->getStartpoint().y;
    dxf->writePoint(&point);
}


/**
 * Writes the given Line( entity to the file.
 */
void RS_FilterDXFRW::writeLine(RS_Line* l) {
    DRW_Line line;
    getEntityAttributes(&line, l);
    line.basePoint.x = l->getStartpoint().x;
    line.basePoint.y = l->getStartpoint().y;
    line.secPoint.x = l->getEndpoint().x;
    line.secPoint.y = l->getEndpoint().y;
    dxf->writeLine(&line);
}


/**
 * Writes the given circle entity to the file.
 */
void RS_FilterDXFRW::writeCircle(RS_Circle* c) {
    DRW_Circle circle;
    getEntityAttributes(&circle, c);
    circle.basePoint.x = c->getCenter().x;
    circle.basePoint.y = c->getCenter().y;
    circle.radious = c->getRadius();
    dxf->writeCircle(&circle);
}


/**
 * Writes the given arc entity to the file.
 */
void RS_FilterDXFRW::writeArc(RS_Arc* a) {
    DRW_Arc arc;
    getEntityAttributes(&arc, a);
    arc.basePoint.x = a->getCenter().x;
    arc.basePoint.y = a->getCenter().y;
    arc.radious = a->getRadius();
    if (a->isReversed()) {
        arc.staangle = a->getAngle2()*ARAD;
        arc.endangle = a->getAngle1()*ARAD;
    } else {
        arc.staangle = a->getAngle1()*ARAD;
        arc.endangle = a->getAngle2()*ARAD;
    }
    dxf->writeArc(&arc);
}


/**
 * Writes the given polyline entity to the file as lwpolyline.
 */
void RS_FilterDXFRW::writeLWPolyline(RS_Polyline* l) {
    // version 12 are old style polyline
    if (version==1009) {
        writePolyline(l);
        return;
    }
    DRW_LWPolyline pol;
    RS_Entity* currEntity = 0;
    RS_Entity* nextEntity = 0;
    RS_AtomicEntity* ae = NULL;
    double bulge=0.0;

    for (RS_Entity* e=l->firstEntity(RS2::ResolveNone);
         e!=NULL; e=nextEntity) {

        currEntity = e;
        nextEntity = l->nextEntity(RS2::ResolveNone);

        if (!e->isAtomic()) {
            continue;
        }
        ae = (RS_AtomicEntity*)e;

        // Write vertex:
            if (e->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)e)->getBulge();
            } else
                bulge = 0.0;
            pol.addVertex( DRW_Vertex2D(ae->getStartpoint().x,
                                      ae->getStartpoint().y, bulge));
    }
    if (l->isClosed()) {
        pol.flags = 1;
    } else {
        ae = (RS_AtomicEntity*)currEntity;
        if (ae->rtti()==RS2::EntityArc) {
            bulge = ((RS_Arc*)ae)->getBulge();
        }
        pol.addVertex( DRW_Vertex2D(ae->getEndpoint().x,
                                  ae->getEndpoint().y, bulge));
    }
    pol.vertexnum = pol.vertlist.size();
    getEntityAttributes(&pol, l);
    dxf->writeLWPolyline(&pol);
}

/**
 * Writes the given polyline entity to the file (old style).
 */
void RS_FilterDXFRW::writePolyline(RS_Polyline* p) {
    DRW_Polyline pol;
    RS_Entity* currEntity = 0;
    RS_Entity* nextEntity = 0;
    RS_AtomicEntity* ae = NULL;
    double bulge=0.0;

    for (RS_Entity* e=p->firstEntity(RS2::ResolveNone);
         e!=NULL; e=nextEntity) {

        currEntity = e;
        nextEntity = p->nextEntity(RS2::ResolveNone);

        if (!e->isAtomic()) {
            continue;
        }
        ae = (RS_AtomicEntity*)e;

        // Write vertex:
            if (e->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)e)->getBulge();
            } else
                bulge = 0.0;
            pol.addVertex( DRW_Vertex(ae->getStartpoint().x,
                                      ae->getStartpoint().y, 0.0, bulge));
    }
    if (p->isClosed()) {
        pol.flags = 1;
    } else {
        ae = (RS_AtomicEntity*)currEntity;
        if (ae->rtti()==RS2::EntityArc) {
            bulge = ((RS_Arc*)ae)->getBulge();
        }
        pol.addVertex( DRW_Vertex(ae->getEndpoint().x,
                                  ae->getEndpoint().y, 0.0, bulge));
    }
    getEntityAttributes(&pol, p);
    dxf->writePolyline(&pol);
}



/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXFRW::writeSpline(RS_Spline *s) {

    if (s->getNumberOfControlPoints() < s->getDegree()+1) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_FilterDXF::writeSpline: "
                        "Discarding spline: not enough control points given.");
        return;
    }

    // version 12 do not support Spline write as polyline
    if (version==1009) {
        DRW_Polyline pol;
        RS_Entity* e;
        for (e=s->firstEntity(RS2::ResolveNone);
             e!=NULL; e=s->nextEntity(RS2::ResolveNone)) {
            pol.addVertex( DRW_Vertex(e->getStartpoint().x,
                                      e->getStartpoint().y, 0.0, 0.0));
        }
        if (s->isClosed()) {
            pol.flags = 1;
        } else {
            pol.addVertex( DRW_Vertex(e->getEndpoint().x,
                                      e->getEndpoint().y, 0.0, 0.0));
        }
        getEntityAttributes(&pol, s);
        dxf->writePolyline(&pol);
        return;
    }

    DRW_Spline sp;

    if (s->isClosed())
        sp.flags = 11;
    else
        sp.flags = 8;
    sp.ncontrol = s->getNumberOfControlPoints();
    sp.degree = s->getDegree();
    sp.nknots = sp.ncontrol + sp.degree + 1;

    // write spline knots:
    int k = sp.degree+1;
    for (int i=1; i<=sp.nknots; i++) {
        if (i<=k) {
            sp.knotslist.push_back(0.0);
        } else if (i<=sp.nknots-k) {
            sp.knotslist.push_back(1.0/(sp.nknots-2*k+1) * (i-k));
        } else {
            sp.knotslist.push_back(1.0);
        }
    }

    // write spline control points:
    QList<RS_Vector> cp = s->getControlPoints();
    for (int i = 0; i < cp.size(); ++i) {
        DRW_Coord *controlpoint = new DRW_Coord();
        sp.controllist.push_back(controlpoint);
        controlpoint->x = cp.at(i).x;
        controlpoint->y = cp.at(i).y;
     }
    getEntityAttributes(&sp, s);
    dxf->writeSpline(&sp);

}


/**
 * Writes the given Ellipse entity to the file.
 */
void RS_FilterDXFRW::writeEllipse(RS_Ellipse* s) {
// version 12 do not support Ellipse but are
// converted in polyline by library
    DRW_Ellipse el;
    getEntityAttributes(&el, s);
    el.basePoint.x = s->getCenter().x;
    el.basePoint.y = s->getCenter().y;
    el.secPoint.x = s->getMajorP().x;
    el.secPoint.y = s->getMajorP().y;
    el.ratio = s->getRatio();
    if (s->isReversed()) {
        el.staparam = s->getAngle2();
        el.endparam = s->getAngle1();
    } else {
        el.staparam = s->getAngle1();
        el.endparam = s->getAngle2();
    }
    dxf->writeEllipse(&el);
}

/**
 * Writes the given block insert entity to the file.
 */
void RS_FilterDXFRW::writeInsert(RS_Insert* i) {
    DRW_Insert in;
    getEntityAttributes(&in, i);
    in.basePoint.x = i->getInsertionPoint().x;
    in.basePoint.y = i->getInsertionPoint().y;
#ifndef  RS_VECTOR2D
    in.basePoint.z = i->getInsertionPoint().z;
#endif
    in.name = i->getName().toUtf8().data();
    in.xscale = i->getScale().x;
    in.yscale = i->getScale().y;
#ifndef  RS_VECTOR2D
    in.zscale = i->getScale().z;
#endif
    in.angle = RS_Math::rad2deg(i->getAngle());
    in.colcount = i->getCols();
    in.rowcount = i->getRows();
    in.colspace = i->getSpacing().x;
    in.rowspace =i->getSpacing().y;
    dxf->writeInsert(&in);
}


/**
 * Writes the given mText entity to the file.
 */
void RS_FilterDXFRW::writeMText(RS_MText* t) {
    DRW_Text *text;
    DRW_Text txt1;
    DRW_MText txt2;

    if (version==1009)
        text = &txt1;
    else
        text = &txt2;

    getEntityAttributes(text, t);
    text->basePoint.x = t->getInsertionPoint().x;
    text->basePoint.y = t->getInsertionPoint().y;
    text->height = t->getHeight();
    text->angle = t->getAngle()*180/M_PI;
    text->style = t->getStyle().toStdString();

    if (version==1009) {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->alignH =DRW_Text::HLeft;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->alignH =DRW_Text::HCenter;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->alignH = DRW_Text::HRight;
        }
        if (t->getVAlign()==RS_MTextData::VATop) {
            text->alignV = DRW_Text::VTop;
        } else if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->alignV = DRW_Text::VMiddle;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->alignV = DRW_Text::VBaseLine;
        }
        QStringList txtList = t->getText().split('\n',QString::KeepEmptyParts);
        double dist = t->getLineSpacingFactor()*1.6*t->getHeight();//RLZ: the correct valie are 5/3 instead 1.6
        bool setSec = false;
        if (text->alignH != DRW_Text::HLeft || text->alignV != DRW_Text::VBaseLine) {
            text->secPoint.x = t->getInsertionPoint().x;
            text->secPoint.y = t->getInsertionPoint().y;
            setSec = true;
        }
        if (text->alignV == DRW_Text::VTop)
            dist = dist * -1;
        for (int i=0; i<txtList.size();++i){
            if (!txtList.at(i).isEmpty()) {
                text->text = toDxfString(txtList.at(i)).toUtf8().data();
                RS_Vector inc = t->getInsertionPoint();
                inc.setPolar(dist*i, t->getAngle()+M_PI_2);
                if (setSec) {
                    text->secPoint.x += inc.x;
                    text->secPoint.y += inc.y;
                } else {
                    text->basePoint.x += inc.x;
                    text->basePoint.y += inc.y;
                }
                dxf->writeText(text);
            }
        }
    } else {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->textgen =1;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->textgen =2;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->textgen = 3;
        }
        if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->textgen += 3;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->textgen += 6;
        }
        if (t->getDrawingDirection() == RS_MTextData::LeftToRight)
            text->alignH = (DRW_Text::HAlign)1;
        else if (t->getDrawingDirection() == RS_MTextData::TopToBottom)
            text->alignH = (DRW_Text::HAlign)3;
        else text->alignH = (DRW_Text::HAlign)5;
        if (t->getLineSpacingFactor() == RS_MTextData::AtLeast)
            text->alignV = (DRW_Text::VAlign)1;
        else text->alignV = (DRW_Text::VAlign)2;

        text->text = toDxfString(t->getText()).toUtf8().data();
        //        text->widthscale =t->getWidth();
        text->widthscale =t->getUsedTextWidth(); //getSize().x;

        dxf->writeMText((DRW_MText*)text);
    }
}

/**
 * Writes the given Text entity to the file.
 */
void RS_FilterDXFRW::writeText(RS_Text* t){
    DRW_Text *text;
    DRW_Text txt1;
    text = &txt1;

    getEntityAttributes(text, t);
    text->basePoint.x = t->getInsertionPoint().x;
    text->basePoint.y = t->getInsertionPoint().y;
    text->height = t->getHeight();
    text->angle = t->getAngle()*180/M_PI;
    text->style = t->getStyle().toStdString();
    text->alignH =(DRW_Text::HAlign)t->getHAlign();
    text->alignV =(DRW_Text::VAlign)t->getVAlign();

    if (text->alignV != DRW_Text::VBaseLine || text->alignH == DRW_Text::HMiddle) {
        text->secPoint.x = t->getInsertionPoint().x;
        text->secPoint.y = t->getInsertionPoint().y;
    }

    if (text->alignH == DRW_Text::HAligned || text->alignH != DRW_Text::HFit) {
        text->basePoint.x = t->getSecondPoint().x;
        text->basePoint.y = t->getSecondPoint().y;
    }

    if (!t->getText().isEmpty()) {
        text->text = toDxfString(t->getText()).toUtf8().data();
        dxf->writeText(text);
    }
}

/**
 * Writes the given dimension entity to the file.
 */
void RS_FilterDXFRW::writeDimension(RS_Dimension* d) {

    // version 12 are inserts of *D blocks
    if (version==1009) {
        if (noNameBlock.contains(d)) {
            DRW_Insert in;
            getEntityAttributes(&in, d);
            in.basePoint.x = in.basePoint.y = 0.0;
#ifndef  RS_VECTOR2D
            in.basePoint.z = 0.0;
#endif
            in.name = noNameBlock.value(d).toUtf8().data();
            in.xscale = in.yscale = 1.0;
#ifndef  RS_VECTOR2D
            in.zscale = 1.0;
#endif
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            dxf->writeInsert(&in);
        }
        return;
    }

    DRW_Dimension* dim;
    int attachmentPoint=1;
    if (d->getHAlign()==RS_MTextData::HALeft) {
        attachmentPoint=1;
    } else if (d->getHAlign()==RS_MTextData::HACenter) {
        attachmentPoint=2;
    } else if (d->getHAlign()==RS_MTextData::HARight) {
        attachmentPoint=3;
    }
    if (d->getVAlign()==RS_MTextData::VATop) {
        attachmentPoint+=0;
    } else if (d->getVAlign()==RS_MTextData::VAMiddle) {
        attachmentPoint+=3;
    } else if (d->getVAlign()==RS_MTextData::VABottom) {
        attachmentPoint+=6;
    }

    switch (d->rtti()) {
    case RS2::EntityDimAligned: {
        RS_DimAligned* da = (RS_DimAligned*)d;
        DRW_DimAligned * dd = new DRW_DimAligned();
        dim = dd ;
        dim->type = 1 +32;
        dd->setDef1Point(DRW_Coord (da->getExtensionPoint1().x, da->getExtensionPoint1().y, 0.0));
        dd->setDef2Point(DRW_Coord (da->getExtensionPoint2().x, da->getExtensionPoint2().y, 0.0));
        break; }
    case RS2::EntityDimDiametric: {
        RS_DimDiametric* dr = (RS_DimDiametric*)d;
        DRW_DimDiametric * dd = new DRW_DimDiametric();
        dim = dd ;
        dim->type = 3+32;
        dd->setDiameter1Point(DRW_Coord (dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
        dd->setLeaderLength(dr->getLeader());
        break; }
    case RS2::EntityDimRadial: {
        RS_DimRadial* dr = (RS_DimRadial*)d;
        DRW_DimRadial * dd = new DRW_DimRadial();
        dim = dd ;
        dim->type = 4+32;
        dd->setDiameterPoint(DRW_Coord (dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
        dd->setLeaderLength(dr->getLeader());
        break; }
    case RS2::EntityDimAngular: {
        RS_DimAngular* da = (RS_DimAngular*)d;
        if (da->getDefinitionPoint3() == da->getData().definitionPoint) {
            DRW_DimAngular3p * dd = new DRW_DimAngular3p();
            dim = dd ;
            dim->type = 5+32;
            dd->setFirstLine(DRW_Coord (da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //13
            dd->setSecondLine(DRW_Coord (da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //14
            dd->SetVertexPoint(DRW_Coord (da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //15
            dd->setDimPoint(DRW_Coord (da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //10
        } else {
            DRW_DimAngular * dd = new DRW_DimAngular();
            dim = dd ;
            dim->type = 2+32;
            dd->setFirstLine1(DRW_Coord (da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, 0.0)); //13
            dd->setFirstLine2(DRW_Coord (da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, 0.0)); //14
            dd->setSecondLine1(DRW_Coord (da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, 0.0)); //15
            dd->setDimPoint(DRW_Coord (da->getDefinitionPoint4().x, da->getDefinitionPoint4().y, 0.0)); //16
        }
        break; }
    default: { //default to DimLinear
        RS_DimLinear* dl = (RS_DimLinear*)d;
        DRW_DimLinear * dd = new DRW_DimLinear();
        dim = dd ;
        dim->type = 0+32;
        dd->setDef1Point(DRW_Coord (dl->getExtensionPoint1().x, dl->getExtensionPoint1().y, 0.0));
        dd->setDef2Point(DRW_Coord (dl->getExtensionPoint2().x, dl->getExtensionPoint2().y, 0.0));
        dd->setAngle( RS_Math::rad2deg(dl->getAngle()) );
        dd->setOblique(dl->getOblique());
        break; }
    }
    getEntityAttributes(dim, d);
    dim->setDefPoint(DRW_Coord(d->getDefinitionPoint().x, d->getDefinitionPoint().y, 0));
    dim->setTextPoint(DRW_Coord(d->getMiddleOfText().x, d->getMiddleOfText().y, 0));
    dim->setStyle (d->getStyle().toUtf8().data());
    dim->setAlign (attachmentPoint);
    dim->setTextLineStyle(d->getLineSpacingStyle());
    dim->setText (toDxfString(d->getText()).toUtf8().data());
    dim->setTextLineFactor(d->getLineSpacingFactor());
    //   void setName(const string s) {name = s;}

    dxf->writeDimension(dim);
    delete dim;
}


/**
 * Writes the given leader entity to the file.
 */
void RS_FilterDXFRW::writeLeader(RS_Leader* l) {
    if (l->count()<=0)
        RS_DEBUG->print(RS_Debug::D_WARNING, "dropping leader with no vertices");

    DRW_Leader leader;
    getEntityAttributes(&leader, l);
    leader.style = "Standard";
    leader.arrow = l->hasArrowHead();
    leader.leadertype = 0;
    leader.flag = 3;
    leader.hookline = 0;
    leader.hookflag = 0;
    leader.textheight = 1;
    leader.textwidth = 10;
    leader.vertnum = l->count();
    RS_Line* li =NULL;
    for (RS_Entity* v=l->firstEntity(RS2::ResolveNone);
            v!=NULL;   v=l->nextEntity(RS2::ResolveNone)) {
        if (v->rtti()==RS2::EntityLine) {
            li = (RS_Line*)v;
            leader.vertexlist.push_back(new DRW_Coord(li->getStartpoint().x, li->getStartpoint().y, 0.0));
        }
    }
    if (li != NULL) {
        leader.vertexlist.push_back(new DRW_Coord(li->getEndpoint().x, li->getEndpoint().y, 0.0));
    }
    dxf->writeLeader(&leader);
}


/**
 * Writes the given hatch entity to the file.
 */
void RS_FilterDXFRW::writeHatch(RS_Hatch * h) {
    // version 12 are inserts of *U blocks
    if (version==1009) {
        if (noNameBlock.contains(h)) {
            DRW_Insert in;
            getEntityAttributes(&in, h);
            in.basePoint.x = in.basePoint.y = 0.0;
#ifndef  RS_VECTOR2D
            in.basePoint.z = 0.0;
#endif
            in.name = noNameBlock.value(h).toUtf8().data();
            in.xscale = in.yscale = 1.0;
#ifndef  RS_VECTOR2D
            in.zscale = 1.0;
#endif
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            dxf->writeInsert(&in);
        }
        return;
    }

    bool writeIt = true;
    if (h->countLoops()>0) {
        // check if all of the loops contain entities:
        for (RS_Entity* l=h->firstEntity(RS2::ResolveNone);
                l!=NULL;
                l=h->nextEntity(RS2::ResolveNone)) {

            if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
                if (l->count()==0) {
                    writeIt = false;
                }
            }
        }
    } else {
        writeIt = false;
    }

    if (!writeIt) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::writeHatch: Dropping Hatch");
        return;
    }

    DRW_Hatch ha;
    getEntityAttributes(&ha, h);
    ha.solid = h->isSolid();
    ha.scale = h->getScale();
    ha.angle = h->getAngle();
    ha.name = h->getPattern().toUtf8().data();
    ha.loopsnum = h->countLoops();

    for (RS_Entity* l=h->firstEntity(RS2::ResolveNone);
         l!=NULL;
         l=h->nextEntity(RS2::ResolveNone)) {

        // Write hatch loops:
        if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
            RS_EntityContainer* loop = (RS_EntityContainer*)l;
            DRW_HatchLoop *lData = new DRW_HatchLoop(0);

            for (RS_Entity* ed=loop->firstEntity(RS2::ResolveNone);
                 ed!=NULL;
                 ed=loop->nextEntity(RS2::ResolveNone)) {

                // Write hatch loop edges:
                if (ed->rtti()==RS2::EntityLine) {
                    RS_Line* ln = (RS_Line*)ed;
                    DRW_Line *line = new DRW_Line();
                    line->basePoint.x = ln->getStartpoint().x;
                    line->basePoint.y = ln->getStartpoint().y;
                    line->secPoint.x = ln->getEndpoint().x;
                    line->secPoint.y = ln->getEndpoint().y;
                    lData->objlist.push_back(line);
                } else if (ed->rtti()==RS2::EntityArc) {
                    RS_Arc* ar = (RS_Arc*)ed;
                    DRW_Arc *arc = new DRW_Arc();
                    arc->basePoint.x = ar->getStartpoint().x;
                    arc->basePoint.y = ar->getStartpoint().y;
                    arc->radious = ar->getRadius();
                    if (!ar->isReversed()) {
                        arc->staangle = ar->getAngle1();
                        arc->endangle = ar->getAngle2();
                        arc->isccw = true;
                    } else {
                        arc->staangle = 2*M_PI-ar->getAngle1();
                        arc->endangle = 2*M_PI-ar->getAngle2();
                        arc->isccw = false;
                    }
                    lData->objlist.push_back(arc);
                } else if (ed->rtti()==RS2::EntityCircle) {
                    RS_Circle* ci = (RS_Circle*)ed;
                    DRW_Arc *arc= new DRW_Arc();
                    arc->basePoint.x = ci->getStartpoint().x;
                    arc->basePoint.y = ci->getStartpoint().y;
                    arc->radious = ci->getRadius();
                    arc->staangle = 0.0;
                    arc->endangle = 2*M_PI;
                    arc->isccw = true;
                    lData->objlist.push_back(arc);
                }
            }
            lData->update(); //change to DRW_HatchLoop
            ha.appendLoop(lData);
        }
    }
    dxf->writeHatch(&ha);
}


/**
 * Writes the given Solid entity to the file.
 */
void RS_FilterDXFRW::writeSolid(RS_Solid* s) {
    RS_SolidData data;
    DRW_Solid solid;
    RS_Vector corner;
    getEntityAttributes(&solid, s);
    corner = s->getCorner(0);
    solid.basePoint.x = corner.x;
    solid.basePoint.y = corner.y;
    corner = s->getCorner(1);
    solid.secPoint.x = corner.x;
    solid.secPoint.y = corner.y;
    corner = s->getCorner(2);
    solid.thirdPoint.x = corner.x;
    solid.thirdPoint.y = corner.y;
    if (s->isTriangle()) {
        solid.fourPoint.x = solid.thirdPoint.x;
        solid.fourPoint.y = solid.thirdPoint.y;
    } else {
        corner = s->getCorner(3);
        solid.fourPoint.x = corner.x;
        solid.fourPoint.y = corner.y;
    }
    dxf->writeSolid(&solid);
}


void RS_FilterDXFRW::writeImage(RS_Image * i) {
    DRW_Image image;
    getEntityAttributes(&image, i);

    image.basePoint.x = i->getInsertionPoint().x;
    image.basePoint.y = i->getInsertionPoint().y;
    image.secPoint.x = i->getUVector().x;
    image.secPoint.y = i->getUVector().y;
    image.vx = i->getVVector().x;
    image.vy = i->getVVector().y;
    image.sizeu = i->getWidth();
    image.sizev = i->getHeight();
    image.brightness = i->getBrightness();
    image.contrast = i->getContrast();
    image.fade = i->getFade();

    DRW_ImageDef *imgDef = dxf->writeImage(&image, i->getFile().toStdString());
    if (imgDef != NULL) {
        imgDef->loaded = 1;
        imgDef->u = i->getData().size.x;
        imgDef->v = i->getData().size.y;
        imgDef->up = 1;
        imgDef->vp = 1;
        imgDef->resolution = 0;
    }
}



/*void RS_FilterDXFRW::writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                                        const DRW_Entity& attrib) {
    QString blkName;
    blkName = "__CE";

    // Creating an unique ID from the element ID
    int tmp, c=1; // tmp = temporary var c = counter var
    tmp = con->getId();

    while (true) {
        tmp = tmp/c;
        blkName.append((char) tmp %10 + 48);
        c *= 10;
        if (tmp < 10) {
            break;
        }
    }

    //Block definition
    dw.sectionTables();
    dxf.writeBlockRecord(dw);
    dw.dxfString(  0, "BLOCK_RECORD");

    dw.handle();
    dw.dxfHex(330, 1);
    dw.dxfString(100, "AcDbSymbolTableRecord");
    dw.dxfString(100, "AcDbBlockTableRecord");
    dw.dxfString(  2, blkName.toLatin1().data());
    dw.dxfHex(340, 0);
    dw.dxfString(0, "ENDTAB");

    //Block creation
    RS_BlockData blkdata(blkName, RS_Vector(0,0), false);

    RS_Block* blk = new RS_Block(graphic, blkdata);

    for (RS_Entity* e1 = con->firstEntity(); e1 != NULL;
            e1 = con->nextEntity() ) {
        blk->addEntity(e1);
    }
    writeBlock(dw, blk);
    //delete e1;
}*/



/**
 * Writes the atomic entities of the given cotnainer to the file.
 */
/*void RS_FilterDXFRW::writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c,
                                       const DRW_Entity& attrib,
                                       RS2::ResolveLevel level) {

    for (RS_Entity* e=c->firstEntity(level);
            e!=NULL;
            e=c->nextEntity(level)) {

        writeEntity(dw, e, attrib);
    }
}*/


/**
 * Sets the entities attributes according to the attributes
 * that come from a DXF file.
 */
void RS_FilterDXFRW::setEntityAttributes(RS_Entity* entity,
                                       const DRW_Entity* attrib) {
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes");

    RS_Pen pen;
    pen.setColor(Qt::black);
    pen.setLineType(RS2::SolidLine);
    QString layName = toNativeString(QString::fromUtf8(attrib->layer.c_str()));

    // Layer: add layer in case it doesn't exist:
    if (graphic->findLayer(layName)==NULL) {
        DRW_Layer lay;
        lay.name = attrib->layer;
        addLayer(lay);
    }
    entity->setLayer(layName);

    // Color:
    pen.setColor(numberToColor(attrib->color));

    // Linetype:
    pen.setLineType(nameToLineType( QString::fromUtf8(attrib->lineType.c_str()) ));

    // Width:
    pen.setWidth(numberToWidth(attrib->lWeight));

    entity->setPen(pen);
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes: OK");
}



/**
 * Gets the entities attributes as a DL_Attributes object.
 */
void RS_FilterDXFRW::getEntityAttributes(DRW_Entity* ent, const RS_Entity* entity) {
//DRW_Entity RS_FilterDXFRW::getEntityAttributes(RS_Entity* /*entity*/) {

    // Layer:
    RS_Layer* layer = entity->getLayer();
    QString layerName;
    if (layer!=NULL) {
        layerName = layer->getName();
    } else {
        layerName = "0";
    }

    RS_Pen pen = entity->getPen(false);

    // Color:
    int color = colorToNumber(pen.getColor());
    //printf("Color is: %s -> %d\n", pen.getColor().name().toLatin1().data(), color);

    // Linetype:
    QString lineType = lineTypeToName(pen.getLineType());

    // Width:
    int width = widthToNumber(pen.getWidth());

    ent->layer = toDxfString(layerName).toUtf8().data();
    ent->color = color;
    ent->lWeight = width;
    ent->lineType = lineType.toUtf8().data();
}



/**
 * @return Pen with the same attributes as 'attrib'.
 */
RS_Pen RS_FilterDXFRW::attributesToPen(const DRW_Layer* att) const {

    RS_Pen pen(numberToColor(att->color),
               numberToWidth(att->lWeight),
               nameToLineType(QString::fromUtf8(att->lineType.c_str())) );
    return pen;
}



/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the dxflib documentation for details.
 *
 * @param num Color number.
 * @param comp Compatibility with older QCad versions (1.5.3 and older)
 */
RS_Color RS_FilterDXFRW::numberToColor(int num, bool comp) {
    // Compatibility with QCad 1.5.3 and older:
    if (comp) {
        switch(num) {
        case 0:
            return Qt::black;
            break;
        case 1:
            return Qt::darkBlue;
            break;
        case 2:
            return Qt::darkGreen;
            break;
        case 3:
            return Qt::darkCyan;
            break;
        case 4:
            return Qt::darkRed;
            break;
        case 5:
            return Qt::darkMagenta;
            break;
        case 6:
            return Qt::darkYellow;
            break;
        case 7:
            return Qt::lightGray;
            break;
        case 8:
            return Qt::darkGray;
            break;
        case 9:
            return Qt::blue;
            break;
        case 10:
            return Qt::green;
            break;
        case 11:
            return Qt::cyan;
            break;
        case 12:
            return Qt::red;
            break;
        case 13:
            return Qt::magenta;
            break;
        case 14:
            return Qt::yellow;
            break;
        case 15:
            return Qt::black;
            break;
        default:
            break;
        }
    } else {
        if (num==0) {
            return RS_Color(RS2::FlagByBlock);
        } else if (num==256) {
            return RS_Color(RS2::FlagByLayer);
        } else if (num<=255 && num>=0) {
            return RS_Color((int)(DRW::dxfColors[num][0]*255),
                            (int)(DRW::dxfColors[num][1]*255),
                            (int)(DRW::dxfColors[num][2]*255));
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_FilterDXF::numberToColor: Invalid color number given.");
            return RS_Color(RS2::FlagByLayer);
        }
    }
    return RS_Color();
}



/**
 * Converts a color into a color number in the DXF palette.
 * The color that fits best is chosen.
 */
int RS_FilterDXFRW::colorToNumber(const RS_Color& col) {

    //printf("Searching color for %s\n", col.name().toLatin1().data());

    // Special color BYBLOCK:
    if (col.getFlag(RS2::FlagByBlock)) {
        return 0;
    }

    // Special color BYLAYER
    else if (col.getFlag(RS2::FlagByLayer)) {
        return 256;
    }

    // Special color black is not in the table but white represents both
    // black and white
    else if (col.red()==0 && col.green()==0 && col.blue()==0) {
        return 7;
    }

    // All other colors
    else {
        int num=0;
        int diff=255*3;  // smallest difference to a color in the table found so far

        // Run through the whole table and compare
        for (int i=1; i<=255; i++) {
            int d = abs(col.red()-(int)(DRW::dxfColors[i][0]*255))
                    + abs(col.green()-(int)(DRW::dxfColors[i][1]*255))
                    + abs(col.blue()-(int)(DRW::dxfColors[i][2]*255));

            if (d<diff) {
                /*
                printf("color %f,%f,%f is closer\n",
                       dxfColors[i][0],
                       dxfColors[i][1],
                       dxfColors[i][2]);
                */
                diff = d;
                num = i;
                if (d==0) {
                    break;
                }
            }
        }
        //printf("  Found: %d, diff: %d\n", num, diff);
        return num;
    }
}

void RS_FilterDXFRW::add3dFace(const DRW_3Dface& data) {
    RS_DEBUG->print("RS_FilterDXFRW::add3dFace");
    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      !data.invisibleflag);
    RS_Polyline *polyline = new RS_Polyline(currentContainer, d);
    setEntityAttributes(polyline, &data);
    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);
    RS_Vector v3(data.thirdPoint.x, data.thirdPoint.y);
    RS_Vector v4(data.fourPoint.x, data.fourPoint.y);

    polyline->addVertex(v1, 0.0);
    polyline->addVertex(v2, 0.0);
    polyline->addVertex(v3, 0.0);
    polyline->addVertex(v4, 0.0);

    currentContainer->addEntity(polyline);
}

void RS_FilterDXFRW::addComment(const char*) {
    RS_DEBUG->print("RS_FilterDXF::addComment(const char*) not yet implemented.");
}


/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType RS_FilterDXFRW::nameToLineType(const QString& name) {

    QString uName = name.toUpper();

    // Standard linetypes for QCad II / AutoCAD
    if (uName.isEmpty() || uName=="BYLAYER") {
        return RS2::LineByLayer;

    } else if (uName=="BYBLOCK") {
        return RS2::LineByBlock;

    } else if (uName=="CONTINUOUS" || uName=="ACAD_ISO01W100") {
        return RS2::SolidLine;

    } else if (uName=="ACAD_ISO07W100" || uName=="DOT") {
        return RS2::DotLine;

    } else if (uName=="DOT2") {
        return RS2::DotLine2;

    } else if (uName=="DOTX2") {
        return RS2::DotLineX2;


    } else if (uName=="ACAD_ISO02W100" || uName=="ACAD_ISO03W100" ||
               uName=="DASHED" || uName=="HIDDEN") {
        return RS2::DashLine;

    } else if (uName=="DASHED2" || uName=="HIDDEN2") {
        return RS2::DashLine2;

    } else if (uName=="DASHEDX2" || uName=="HIDDENX2") {
        return RS2::DashLineX2;


    } else if (uName=="ACAD_ISO10W100" ||
               uName=="DASHDOT") {
        return RS2::DashDotLine;

    } else if (uName=="DASHDOT2") {
        return RS2::DashDotLine2;

    } else if (uName=="ACAD_ISO04W100" ||
               uName=="DASHDOTX2") {
        return RS2::DashDotLineX2;


    } else if (uName=="ACAD_ISO12W100" || uName=="DIVIDE") {
        return RS2::DivideLine;

    } else if (uName=="DIVIDE2") {
        return RS2::DivideLine2;

    } else if (uName=="ACAD_ISO05W100" || uName=="DIVIDEX2") {
        return RS2::DivideLineX2;


    } else if (uName=="CENTER") {
        return RS2::CenterLine;

    } else if (uName=="CENTER2") {
        return RS2::CenterLine2;

    } else if (uName=="CENTERX2") {
        return RS2::CenterLineX2;


    } else if (uName=="BORDER") {
        return RS2::BorderLine;

    } else if (uName=="BORDER2") {
        return RS2::BorderLine2;

    } else if (uName=="BORDERX2") {
        return RS2::BorderLineX2;
    }

    return RS2::SolidLine;
}



/**
 * Converts a RS_LineType into a name for a line type.
 */
QString RS_FilterDXFRW::lineTypeToName(RS2::LineType lineType) {

    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {

    case RS2::SolidLine:
        return "CONTINUOUS";
        break;

    case RS2::DotLine:
        return "DOT";
        break;
    case RS2::DotLine2:
        return "DOT2";
        break;
    case RS2::DotLineX2:
        return "DOTX2";
        break;

    case RS2::DashLine:
        return "DASHED";
        break;
    case RS2::DashLine2:
        return "DASHED2";
        break;
    case RS2::DashLineX2:
        return "DASHEDX2";
        break;

    case RS2::DashDotLine:
        return "DASHDOT";
        break;
    case RS2::DashDotLine2:
        return "DASHDOT2";
        break;
    case RS2::DashDotLineX2:
        return "DASHDOTX2";
        break;

    case RS2::DivideLine:
        return "DIVIDE";
        break;
    case RS2::DivideLine2:
        return "DIVIDE2";
        break;
    case RS2::DivideLineX2:
        return "DIVIDEX2";
        break;

    case RS2::CenterLine:
        return "CENTER";
        break;
    case RS2::CenterLine2:
        return "CENTER2";
        break;
    case RS2::CenterLineX2:
        return "CENTERX2";
        break;

    case RS2::BorderLine:
        return "BORDER";
        break;
    case RS2::BorderLine2:
        return "BORDER2";
        break;
    case RS2::BorderLineX2:
        return "BORDERX2";
        break;

    case RS2::LineByLayer:
        return "ByLayer";
        break;
    case RS2::LineByBlock:
        return "ByBlock";
        break;
    default:
        break;
    }

    return "CONTINUOUS";
}



/**
 * Converts a RS_LineType into a name for a line type.
 */
/*QString RS_FilterDXFRW::lineTypeToDescription(RS2::LineType lineType) {

    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
    case RS2::SolidLine:
        return "Solid line";
    case RS2::DotLine:
        return "ISO Dashed __ __ __ __ __ __ __ __ __ __ _";
    case RS2::DashLine:
        return "ISO Dashed with Distance __    __    __    _";
    case RS2::DashDotLine:
        return "ISO Long Dashed Dotted ____ . ____ . __";
    case RS2::DashDotDotLine:
        return "ISO Long Dashed Double Dotted ____ .. __";
    case RS2::LineByLayer:
        return "";
    case RS2::LineByBlock:
        return "";
    default:
        break;
    }

    return "CONTINUOUS";
}*/



/**
 * Converts a line width number (e.g. 1) into a RS2::LineWidth.
 */
RS2::LineWidth RS_FilterDXFRW::numberToWidth(int num) {
    switch (num) {
    case -1:
        return RS2::WidthByLayer;
        break;
    case -2:
        return RS2::WidthByBlock;
        break;
    case -3:
        return RS2::WidthDefault;
        break;
    default:
        if (num<3) {
            return RS2::Width00;
        } else if (num<7) {
            return RS2::Width01;
        } else if (num<11) {
            return RS2::Width02;
        } else if (num<14) {
            return RS2::Width03;
        } else if (num<16) {
            return RS2::Width04;
        } else if (num<19) {
            return RS2::Width05;
        } else if (num<22) {
            return RS2::Width06;
        } else if (num<27) {
            return RS2::Width07;
        } else if (num<32) {
            return RS2::Width08;
        } else if (num<37) {
            return RS2::Width09;
        } else if (num<45) {
            return RS2::Width10;
        } else if (num<52) {
            return RS2::Width11;
        } else if (num<57) {
            return RS2::Width12;
        } else if (num<65) {
            return RS2::Width13;
        } else if (num<75) {
            return RS2::Width14;
        } else if (num<85) {
            return RS2::Width15;
        } else if (num<95) {
            return RS2::Width16;
        } else if (num<103) {
            return RS2::Width17;
        } else if (num<112) {
            return RS2::Width18;
        } else if (num<130) {
            return RS2::Width19;
        } else if (num<149) {
            return RS2::Width20;
        } else if (num<180) {
            return RS2::Width21;
        } else if (num<205) {
            return RS2::Width22;
        } else {
            return RS2::Width23;
        }
        break;
    }
    return (RS2::LineWidth)num;
}



/**
 * Converts a RS2::LineWidth into an int width.
 */
int RS_FilterDXFRW::widthToNumber(RS2::LineWidth width) {
    switch (width) {
    case RS2::WidthByLayer:
        return -1;
        break;
    case RS2::WidthByBlock:
        return -2;
        break;
    case RS2::WidthDefault:
        return -3;
        break;
    default:
        return (int)width;
        break;
    }
    return (int)width;
}



/**
 * Converts a native unicode string into a DXF encoded string.
 *
 * DXF endoding includes the following special sequences:
 * - %%%c for a diameter sign
 * - %%%d for a degree sign
 * - %%%p for a plus/minus sign
 */
QString RS_FilterDXFRW::toDxfString(const QString& str) {
    QString res = "";
    int j=0;
    for (int i=0; i<str.length(); ++i) {
        int c = str.at(i).unicode();
        if (c>175 || c<11){
            res.append(str.mid(j,i-j));
            j=i;

            switch (c) {
            case 0x0A:
                res+="\\P";
                break;
                // diameter:
            case 0x2205://RLZ: Empty_set, diameter is 0x2300 need to add in all fonts
                res+="%%C";
                break;
                // degree:
            case 0x00B0:
                res+="%%D";
                break;
                // plus/minus
            case 0x00B1:
                res+="%%P";
                break;
            default:
                j--;
                break;
            }
            j++;
        }

    }
    res.append(str.mid(j));
    return res;
}



/**
 * Converts a DXF encoded string into a native Unicode string.
 */
QString RS_FilterDXFRW::toNativeString(const QString& data) {
    QString res;

    // Ignore font tags:
    int j = 0;
    for (int i=0; i<data.length(); ++i) {
        if (data.at(i).unicode() == 0x7B){ //is '{' ?
            if (data.at(i+1).unicode() == 0x5c && data.at(i+2).unicode() == 0x66){ //is "\f" ?
                //found font tag, append parsed part
                res.append(data.mid(j,i-j));
                //skip to ';'
                for (int k=i+3; k<data.length(); ++k) {
                    if (data.at(k).unicode() == 0x3B) {
                        i = j = ++k;
                        break;
                    }
                }
                //add to '}'
                for (int k=i; k<data.length(); ++k) {
                    if (data.at(k).unicode() == 0x7D) {
                        res.append(data.mid(i,k-i));
                        i = j = ++k;
                        break;
                    }
                }

            }
        }
    }
    res.append(data.mid(j));

    // Line feed:
    res = res.replace(QRegExp("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegExp("\\\\~"), " ");
    // diameter:
    res = res.replace(QRegExp("%%[cC]"), QChar(0x2205));//RLZ: Empty_set, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegExp("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegExp("%%[pP]"), QChar(0x00B1));

    return res;
}



/**
 * Converts the given number from a DXF file into an AngleFormat enum.
 *
 * @param num $DIMAUNIT from DXF (0: decimal deg, 1: deg/min/sec, 2: gradians,
 *                                3: radians, 4: surveyor's units)
 *
 * @ret Matching AngleFormat enum value.
 */
RS2::AngleFormat RS_FilterDXFRW::numberToAngleFormat(int num) {

    RS2::AngleFormat af;

    switch (num) {
    default:
    case 0:
        af = RS2::DegreesDecimal;
        break;
    case 1:
        af = RS2::DegreesMinutesSeconds;
        break;
    case 2:
        af = RS2::Gradians;
        break;
    case 3:
        af = RS2::Radians;
        break;
    case 4:
        af = RS2::Surveyors;
        break;
    }

    return af;
}


/**
 * Converts AngleFormat enum to DXF number.
 */
int RS_FilterDXFRW::angleFormatToNumber(RS2::AngleFormat af) {

    int num;

    switch (af) {
    default:
    case RS2::DegreesDecimal:
        num = 0;
        break;
    case RS2::DegreesMinutesSeconds:
        num = 1;
        break;
    case RS2::Gradians:
        num = 2;
        break;
    case RS2::Radians:
        num = 3;
        break;
    case RS2::Surveyors:
        num = 4;
        break;
    }

    return num;
}



/**
 * converts a DXF unit setting (e.g. INSUNITS) to a unit enum.
 */
RS2::Unit RS_FilterDXFRW::numberToUnit(int num) {
    switch (num) {
    default:
    case  0:
        return RS2::None;
        break;
    case  1:
        return RS2::Inch;
        break;
    case  2:
        return RS2::Foot;
        break;
    case  3:
        return RS2::Mile;
        break;
    case  4:
        return RS2::Millimeter;
        break;
    case  5:
        return RS2::Centimeter;
        break;
    case  6:
        return RS2::Meter;
        break;
    case  7:
        return RS2::Kilometer;
        break;
    case  8:
        return RS2::Microinch;
        break;
    case  9:
        return RS2::Mil;
        break;
    case 10:
        return RS2::Yard;
        break;
    case 11:
        return RS2::Angstrom;
        break;
    case 12:
        return RS2::Nanometer;
        break;
    case 13:
        return RS2::Micron;
        break;
    case 14:
        return RS2::Decimeter;
        break;
    case 15:
        return RS2::Decameter;
        break;
    case 16:
        return RS2::Hectometer;
        break;
    case 17:
        return RS2::Gigameter;
        break;
    case 18:
        return RS2::Astro;
        break;
    case 19:
        return RS2::Lightyear;
        break;
    case 20:
        return RS2::Parsec;
        break;
    }

    return RS2::None;
}



/**
 * Converst a unit enum into a DXF unit number e.g. for INSUNITS.
 */
int RS_FilterDXFRW::unitToNumber(RS2::Unit unit) {
    switch (unit) {
    default:
    case RS2::None:
        return  0;
        break;
    case RS2::Inch:
        return  1;
        break;
    case RS2::Foot:
        return  2;
        break;
    case RS2::Mile:
        return  3;
        break;
    case RS2::Millimeter:
        return  4;
        break;
    case RS2::Centimeter:
        return  5;
        break;
    case RS2::Meter:
        return  6;
        break;
    case RS2::Kilometer:
        return  7;
        break;
    case RS2::Microinch:
        return  8;
        break;
    case RS2::Mil:
        return  9;
        break;
    case RS2::Yard:
        return 10;
        break;
    case RS2::Angstrom:
        return 11;
        break;
    case RS2::Nanometer:
        return 12;
        break;
    case RS2::Micron:
        return 13;
        break;
    case RS2::Decimeter:
        return 14;
        break;
    case RS2::Decameter:
        return 15;
        break;
    case RS2::Hectometer:
        return 16;
        break;
    case RS2::Gigameter:
        return 17;
        break;
    case RS2::Astro:
        return 18;
        break;
    case RS2::Lightyear:
        return 19;
        break;
    case RS2::Parsec:
        return 20;
        break;
    }

    return 0;
}



/**
 * Checks if the given variable is two-dimensional (e.g. $LIMMIN).
 */
bool RS_FilterDXFRW::isVariableTwoDimensional(const QString& var) {
    if (var=="$LIMMIN" ||
            var=="$LIMMAX" ||
            var=="$PLIMMIN" ||
            var=="$PLIMMAX" ||
            var=="$GRIDUNIT" ||
            var=="$VIEWCTR") {

        return true;
    } else {
        return false;
    }
}


// EOF

