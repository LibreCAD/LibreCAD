/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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


#include "rs_filterdxf.h"

#include <stdio.h>

#include "dl_attributes.h"
#include "dl_codes.h"
#include "dl_writer_ascii.h"

#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_leader.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_system.h"

#include <QStringList>

#include <qtextcodec.h>
#include "rs_debug.h"

/**
 * Default constructor.
 *
 */
RS_FilterDXF::RS_FilterDXF()
        :RS_FilterInterface() {

    RS_DEBUG->print("RS_FilterDXF::RS_FilterDXF()");

    mtext = "";
    polyline = NULL;
    leader = NULL;
    hatch = NULL;
    hatchLoop = NULL;
    currentContainer = NULL;
    graphic = NULL;
	spline = NULL;
	splinePoints = NULL;
    //exportVersion = DL_Codes::VER_2002;
    //systemVariables.setAutoDelete(true);
    RS_DEBUG->print("RS_FilterDXF::RS_FilterDXF(): OK");
}



/**
 * Destructor.
 */
RS_FilterDXF::~RS_FilterDXF() {
    RS_DEBUG->print("RS_FilterDXF::~RS_FilterDXF()");
    RS_DEBUG->print("RS_FilterDXF::~RS_FilterDXF(): OK");
}



/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterDXF::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("RS_FilterDXF::fileImport");
    //RS_DEBUG->timestamp();

    RS_DEBUG->print("DXF Filter: importing file '%s'...", (const char*)QFile::encodeName(file));

    variables.clear();
    graphic = &g;
    currentContainer = graphic;
    this->file = file;

    RS_DEBUG->print("graphic->countLayers(): %d", graphic->countLayers());

    //graphic->setAutoUpdateBorders(false);
    RS_DEBUG->print("RS_FilterDXF::fileImport: reading file");
    bool success = dxf.in((const char*)QFile::encodeName(file), this);
    RS_DEBUG->print("RS_FilterDXF::fileImport: reading file: OK");
    //graphic->setAutoUpdateBorders(true);

    if (success==false) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "Cannot open DXF file '%s'.", (const char*)QFile::encodeName(file));
        return false;
    }

    RS_DEBUG->print("RS_FilterDXF::fileImport: adding variables");

    // add some variables that need to be there for DXF drawings:
    if (graphic->getVariableString("$DIMSTYLE", "").isEmpty()) {
        RS_DEBUG->print("RS_FilterDXF::fileImport: adding DIMSTYLE");
        graphic->addVariable("$DIMSTYLE", "Standard", 2);
        RS_DEBUG->print("RS_FilterDXF::fileImport: adding DIMSTYLE: OK");
    }
    RS_DEBUG->print("RS_FilterDXF::fileImport: adding variables: OK");

    RS_DEBUG->print("RS_FilterDXF::fileImport: updating inserts");
    graphic->updateInserts();
    RS_DEBUG->print("RS_FilterDXF::fileImport: updating inserts: OK");

    RS_DEBUG->print("RS_FilterDXF::fileImport OK");
    //RS_DEBUG->timestamp();

    return true;
}

/*
 * get the encoding of the DXF files,
 * Acad versions >= 2007 are UTF-8, others in ANSI_1252
 */
QString RS_FilterDXF::getDXFEncoding() {

    QString acadver=variables.getString("$ACADVER", "");
    acadver.replace(QRegExp("[a-zA-Z]"), "");
    bool ok;
    int version=acadver.toInt(&ok);

    // >= ACAD2007
    if (ok && version >= 1021) {
        return RS_System::getEncoding("UTF-8");
    }

    // < ACAD2007
    QString codePage=variables.getString("$DWGCODEPAGE", "ANSI_1252");
    return RS_System::getEncoding(codePage);
}


/**
 * Implementation of the method which handles layers.
 */
void RS_FilterDXF::addLayer(const DL_LayerData& data) {
    RS_DEBUG->print("RS_FilterDXF::addLayer");
    RS_DEBUG->print("  adding layer: %s", data.name.c_str());

    RS_DEBUG->print("RS_FilterDXF::addLayer: creating layer");

    RS_Layer* layer = new RS_Layer(toNativeString(data.name.c_str(),getDXFEncoding()));
    RS_DEBUG->print("RS_FilterDXF::addLayer: set pen");
    layer->setPen(attributesToPen(attributes));
    layer->setConstructionLayer(! data.plotF); //plotF is used to indicate whether the plot is plotted in printing
    //layer->setFlags(data.flags&0x07);

    RS_DEBUG->print("RS_FilterDXF::addLayer: flags");
    if (data.flags&0x01) {
        layer->freeze(true);
    }
    if (data.flags&0x04) {
        layer->lock(true);
    }


    RS_DEBUG->print("RS_FilterDXF::addLayer: add layer to graphic");
    graphic->addLayer(layer);
    RS_DEBUG->print("RS_FilterDXF::addLayer: OK");
}

/**
 * Implementation of the method which handles blocks.
 *
 * @todo Adding blocks to blocks (stack for currentContainer)
 */
void RS_FilterDXF::addBlock(const DL_BlockData& data) {

    RS_DEBUG->print("RS_FilterDXF::addBlock");

    RS_DEBUG->print("  adding block: %s", data.name.c_str());


    // Prevent special blocks (paper_space, model_space) from being added:
    if (QString(QString::fromUtf8(data.name.c_str())).toLower()!="*paper_space0" &&
            QString(QString::fromUtf8(data.name.c_str())).toLower()!="*paper_space" &&
            QString(QString::fromUtf8(data.name.c_str())).toLower()!="*model_space" &&
            QString(QString::fromUtf8(data.name.c_str())).toLower()!="$paper_space0" &&
            QString(QString::fromUtf8(data.name.c_str())).toLower()!="$paper_space" &&
            QString(QString::fromUtf8(data.name.c_str())).toLower()!="$model_space") {

#ifndef RS_NO_COMPLEX_ENTITIES
        if (QString(QString::fromUtf8(data.name.c_str())).startsWith("__CE")) {
            RS_EntityContainer* ec = new RS_EntityContainer();
            ec->setLayer("0");
            currentContainer = ec;
            graphic->addEntity(ec);
            //currentContainer->setLayer(graphic->findLayer("0"));
        }
        else {
#endif
            RS_Vector bp(data.bpx, data.bpy);
            RS_Block* block =
                new RS_Block(graphic,
                             RS_BlockData(QString::fromUtf8(data.name.c_str()), bp, false));
            //block->setFlags(flags);

            if (graphic->addBlock(block)) {
                currentContainer = block;
            }
#ifndef RS_NO_COMPLEX_ENTITIES

        }
#endif

    }
}



/**
 * Implementation of the method which closes blocks.
 */
void RS_FilterDXF::endBlock() {
    currentContainer = graphic;
}



/**
 * Implementation of the method which handles point entities.
 */
void RS_FilterDXF::addPoint(const DL_PointData& data) {
    RS_Vector v(data.x, data.y);

    RS_Point* entity = new RS_Point(currentContainer,
                                    RS_PointData(v));
    setEntityAttributes(entity, attributes);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXF::addLine(const DL_LineData& data) {
    RS_DEBUG->print("RS_FilterDXF::addLine");

    RS_Vector v1(data.x1, data.y1);
    RS_Vector v2(data.x2, data.y2);

    RS_DEBUG->print("RS_FilterDXF::addLine: create line");

    if (currentContainer==NULL) {
        RS_DEBUG->print("RS_FilterDXF::addLine: currentContainer is NULL");
    }

    RS_Line* entity = new RS_Line(currentContainer,
                                  RS_LineData(v1, v2));
    RS_DEBUG->print("RS_FilterDXF::addLine: set attributes");
    setEntityAttributes(entity, attributes);

    RS_DEBUG->print("RS_FilterDXF::addLine: add entity");

    currentContainer->addEntity(entity);

    RS_DEBUG->print("RS_FilterDXF::addLine: OK");
}



/**
 * Implementation of the method which handles arc entities.
 *
 * @param angle1 Start angle in deg (!)
 * @param angle2 End angle in deg (!)
 */
void RS_FilterDXF::addArc(const DL_ArcData& data) {
    RS_DEBUG->print("RS_FilterDXF::addArc");
    //printf("LINE     (%12.6f, %12.6f, %12.6f) (%12.6f, %12.6f, %12.6f)\n",
    //       p1[0], p1[1], p1[2],
    //       p2[0], p2[1], p2[2]);
    RS_Vector v(data.cx, data.cy);
    RS_ArcData d(v, data.radius,
                 data.angle1/ARAD,
                 data.angle2/ARAD,
                 false);
    RS_Arc* entity = new RS_Arc(currentContainer, d);
    setEntityAttributes(entity, attributes);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles ellipse entities.
 *
 * @param angle1 Start angle in rad (!)
 * @param angle2 End angle in rad (!)
 */
void RS_FilterDXF::addEllipse(const DL_EllipseData& data) {
    RS_DEBUG->print("RS_FilterDXF::addEllipse");

    RS_Vector v1(data.cx, data.cy);
    RS_Vector v2(data.mx, data.my);

    RS_EllipseData ed(v1, v2,
                      data.ratio,
                      data.angle1,
                      data.angle2,
                      false);
    RS_Ellipse* entity = new RS_Ellipse(currentContainer, ed);
    setEntityAttributes(entity, attributes);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles circle entities.
 */
void RS_FilterDXF::addCircle(const DL_CircleData& data) {
    RS_DEBUG->print("RS_FilterDXF::addCircle");
    //printf("LINE     (%12.6f, %12.6f, %12.6f) (%12.6f, %12.6f, %12.6f)\n",
    //       p1[0], p1[1], p1[2],
    //       p2[0], p2[1], p2[2]);

    RS_Vector v(data.cx, data.cy);
    RS_CircleData d(v, data.radius);
    RS_Circle* entity = new RS_Circle(currentContainer, d);
    setEntityAttributes(entity, attributes);

    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles polyline entities.
 */
void RS_FilterDXF::addPolyline(const DL_PolylineData& data) {
    RS_DEBUG->print("RS_FilterDXF::addPolyline");
    //RS_DEBUG->print("RS_FilterDXF::addPolyline()");
    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      data.flags&0x1);
    polyline = new RS_Polyline(currentContainer, d);
    setEntityAttributes(polyline, attributes);

    currentContainer->addEntity(polyline);
}



/**
 * Implementation of the method which handles polyline vertices.
 */
void RS_FilterDXF::addVertex(const DL_VertexData& data) {
    RS_DEBUG->print("RS_FilterDXF::addVertex(): %f/%f bulge: %f",
                    data.x, data.y, data.bulge);

    RS_Vector v(data.x, data.y);

    if (polyline!=NULL) {
        polyline->addVertex(v, data.bulge);
    }
}



/**
 * Implementation of the method which handles splines.
 */
void RS_FilterDXF::addSpline(const DL_SplineData& data) {
    RS_DEBUG->print("RS_FilterDXF::addSpline: degree: %d", data.degree);

	if(data.degree == 2)
	{
		RS_SplinePointsData d(((data.flags&0x1)==0x1), true);
		splinePoints = new RS_SplinePoints(currentContainer, d);
        setEntityAttributes(spline, attributes);
		currentContainer->addEntity(spline);
		spline = NULL;
		return;
	}

    if (data.degree>=1 && data.degree<=3) {
        RS_SplineData d(data.degree, ((data.flags&0x1)==0x1));
        spline = new RS_Spline(currentContainer, d);
        setEntityAttributes(spline, attributes);

        currentContainer->addEntity(spline);
		splinePoints = NULL;
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::addSpline: Invalid degree for spline: %d. "
                        "Accepted values are 1..3.", data.degree);
    }
}


/**
 * Implementation of the method which handles spline control points.
 */
void RS_FilterDXF::addControlPoint(const DL_ControlPointData& data) {
    RS_DEBUG->print("RS_FilterDXF::addControlPoint: %f/%f", data.x, data.y);

    RS_Vector v(data.x, data.y);

    if (spline!=NULL) {
        spline->addControlPoint(v);
        spline->update();
    }
    else if (splinePoints != NULL) {
        splinePoints->addControlPoint(v);
        splinePoints->update();
    }
}



/**
 * Implementation of the method which handles inserts.
 */
void RS_FilterDXF::addInsert(const DL_InsertData& data) {

    RS_DEBUG->print("RS_FilterDXF::addInsert");

    if (QString(data.name.c_str()).left(3)=="A$C") {
        return;
    }

    RS_Vector ip(data.ipx, data.ipy);
    RS_Vector sc(data.sx, data.sy);
    RS_Vector sp(data.colSp, data.rowSp);

    //cout << "Insert: " << name << " " << ip << " " << cols << "/" << rows << endl;

    RS_InsertData d(data.name.c_str(),
                    ip, sc, data.angle/ARAD,
                    data.cols, data.rows,
                    sp,
                    NULL,
                    RS2::NoUpdate);
    RS_Insert* entity = new RS_Insert(currentContainer, d);
    setEntityAttributes(entity, attributes);
    RS_DEBUG->print("  id: %d", entity->getId());
    //entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles text
 * chunks for MText entities.
 */
void RS_FilterDXF::addMTextChunk(const char* text) {
    RS_DEBUG->print("RS_FilterDXF::addMTextChunk: %s", text);
    mtext+=text;

}



/**
 * Implementation of the method which handles
 * multi texts (MTEXT).
 */
void RS_FilterDXF::addMText(const DL_MTextData& data) {
    RS_DEBUG->print("RS_FilterDXF::addMText: %s", data.text.c_str());

    RS_Vector ip(data.ipx, data.ipy);
    RS2::VAlign valign;
    RS2::HAlign halign;
    RS2::TextDrawingDirection dir;
    RS2::TextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data.style.c_str());
    sty=sty.toLower();

    if (data.attachmentPoint<=3) {
        valign=RS2::VAlignTop;
    } else if (data.attachmentPoint<=6) {
        valign=RS2::VAlignMiddle;
    } else {
        valign=RS2::VAlignBottom;
    }

    if (data.attachmentPoint%3==1) {
        halign=RS2::HAlignLeft;
    } else if (data.attachmentPoint%3==2) {
        halign=RS2::HAlignCenter;
    } else {
        halign=RS2::HAlignRight;
    }

    if (data.drawingDirection==1) {
        dir = RS2::LeftToRight;
    } else if (data.drawingDirection==3) {
        dir = RS2::TopToBottom;
    } else {
        dir = RS2::ByStyle;
    }

    if (data.lineSpacingStyle==1) {
        lss = RS2::AtLeast;
    } else {
        lss = RS2::Exact;
    }

    mtext+=data.text.c_str();
    mtext = toNativeString(mtext.toLocal8Bit().data(), getDXFEncoding());

    // use default style for the drawing:
    if (sty.isEmpty()) {
        // japanese, cyrillic:
        QString codepage = variables.getString("$DWGCODEPAGE", "ANSI_1252");
        if (codepage=="ANSI_932" || codepage=="ANSI_1251") {
            sty = "Unicode";
        } else {
            sty = variables.getString("$TEXTSTYLE", "Standard");
        }
    } else {
        // Change the QCAD "normal" style to the more correct ISO-3059
        if  (sty=="normal" || sty=="normallatin1" || sty=="normallatin2") {
            sty="iso";
        }
    }


    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(mtext);

    RS_TextData d(ip, data.height, data.width,
                  valign, halign,
                  dir, lss,
                  data.lineSpacingFactor,
                  mtext, sty, data.angle,
                  RS2::NoUpdate);
    RS_Text* entity = new RS_Text(currentContainer, d);

    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);

    mtext = "";
}



/**
 * Implementation of the method which handles
 * texts (TEXT).
 */
void RS_FilterDXF::addText(const DL_TextData& data) {
    RS_DEBUG->print("RS_FilterDXF::addText");
    int attachmentPoint;
    RS_Vector refPoint;
    double angle = data.angle;

    // TODO: check, maybe implement a separate TEXT instead of using MTEXT

    // baseline has 5 vertical alignment modes:
    if (data.vJustification!=0 || data.hJustification!=0) {
        switch (data.hJustification) {
        default:
        case 0: // left aligned
            attachmentPoint = 1;
            refPoint = RS_Vector(data.apx, data.apy);
            break;
        case 1: // centered
            attachmentPoint = 2;
            refPoint = RS_Vector(data.apx, data.apy);
            break;
        case 2: // right aligned
            attachmentPoint = 3;
            refPoint = RS_Vector(data.apx, data.apy);
            break;
        case 3: // aligned (TODO)
            attachmentPoint = 2;
            refPoint = RS_Vector((data.ipx+data.apx)/2.0,
                                 (data.ipy+data.apy)/2.0);
            angle =
                RS_Vector(data.ipx, data.ipy).angleTo(
                    RS_Vector(data.apx, data.apy));
            break;
        case 4: // Middle (TODO)
            attachmentPoint = 2;
            refPoint = RS_Vector(data.apx, data.apy);
            break;
        case 5: // fit (TODO)
            attachmentPoint = 2;
            refPoint = RS_Vector((data.ipx+data.apx)/2.0,
                                 (data.ipy+data.apy)/2.0);
            angle =
                RS_Vector(data.ipx, data.ipy).angleTo(
                    RS_Vector(data.apx, data.apy));
            break;
        }

        switch (data.vJustification) {
        default:
        case 0: // baseline
        case 1: // bottom
            attachmentPoint += 6;
            break;

        case 2: // middle
            attachmentPoint += 3;
            break;

        case 3: // top
            break;
        }
    } else {
        //attachmentPoint = (data.hJustification+1)+(3-data.vJustification)*3;
        attachmentPoint = 7;
        refPoint = RS_Vector(data.ipx, data.ipy);
    }

    int drawingDirection = 5;
    double width = 100.0;

    mtext = "";
    addMText(DL_MTextData(
                 refPoint.x,
                 refPoint.y,
#ifdef  RS_VECTOR2D
                 0.,
#else
                 refPoint.z,
#endif
                 data.height, width,
                 attachmentPoint,
                 drawingDirection,
                 RS2::Exact,
                 1.0,
                 data.text.c_str(), data.style,
                 angle));
}



/**
 * Implementation of the method which handles
 * dimensions (DIMENSION).
 */
RS_DimensionData RS_FilterDXF::convDimensionData(
    const DL_DimensionData& data) {

    RS_Vector defP(data.dpx, data.dpy);
    RS_Vector midP(data.mpx, data.mpy);
    RS2::VAlign valign;
    RS2::HAlign halign;
    RS2::TextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data.style.c_str());

    QString t; //= data.text;

    // middlepoint of text can be 0/0 which is considered to be invalid (!):
    //  0/0 because older QCad versions save the middle of the text as 0/0
    //  although they didn't support saving of the middle of the text.
    if (fabs(data.mpx)<1.0e-6 && fabs(data.mpy)<1.0e-6) {
        midP = RS_Vector(false);
    }

    if (data.attachmentPoint<=3) {
        valign=RS2::VAlignTop;
    } else if (data.attachmentPoint<=6) {
        valign=RS2::VAlignMiddle;
    } else {
        valign=RS2::VAlignBottom;
    }

    if (data.attachmentPoint%3==1) {
        halign=RS2::HAlignLeft;
    } else if (data.attachmentPoint%3==2) {
        halign=RS2::HAlignCenter;
    } else {
        halign=RS2::HAlignRight;
    }

    if (data.lineSpacingStyle==1) {
        lss = RS2::AtLeast;
    } else {
        lss = RS2::Exact;
    }

    t = toNativeString(data.text.c_str(), getDXFEncoding());

    if (sty.isEmpty()) {
        sty = variables.getString("$DIMSTYLE", "Standard");
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(t);

    // data needed to add the actual dimension entity
    return RS_DimensionData(defP, midP,
                            valign, halign,
                            lss,
                            data.lineSpacingFactor,
                            t, sty, data.angle);
}



/**
 * Implementation of the method which handles
 * aligned dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimAlign(const DL_DimensionData& data,
                               const DL_DimAlignedData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimAligned");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector ext1(edata.epx1, edata.epy1);
    RS_Vector ext2(edata.epx2, edata.epy2);

    RS_DimAlignedData d(ext1, ext2);

    RS_DimAligned* entity = new RS_DimAligned(currentContainer,
                            dimensionData, d);
    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * linear dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimLinear(const DL_DimensionData& data,
                                const DL_DimLinearData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimLinear");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector dxt1(edata.dpx1, edata.dpy1);
    RS_Vector dxt2(edata.dpx2, edata.dpy2);

    RS_DimLinearData d(dxt1, dxt2, RS_Math::deg2rad(edata.angle),
                       RS_Math::deg2rad(edata.oblique));

    RS_DimLinear* entity = new RS_DimLinear(currentContainer,
                                            dimensionData, d);
    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * radial dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimRadial(const DL_DimensionData& data,
                                const DL_DimRadialData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimRadial");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(edata.dpx, edata.dpy);

    RS_DimRadialData d(dp, edata.leader);

    RS_DimRadial* entity = new RS_DimRadial(currentContainer,
                                            dimensionData, d);

    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * diametric dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimDiametric(const DL_DimensionData& data,
                                   const DL_DimDiametricData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimDiametric");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(edata.dpx, edata.dpy);

    RS_DimDiametricData d(dp, edata.leader);

    RS_DimDiametric* entity = new RS_DimDiametric(currentContainer,
                              dimensionData, d);

    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimAngular(const DL_DimensionData& data,
                                 const DL_DimAngularData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimAngular");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(edata.dpx1, edata.dpy1);
    RS_Vector dp2(edata.dpx2, edata.dpy2);
    RS_Vector dp3(edata.dpx3, edata.dpy3);
    RS_Vector dp4(edata.dpx4, edata.dpy4);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    RS_DimAngular* entity = new RS_DimAngular(currentContainer,
                            dimensionData, d);

    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXF::addDimAngular3P(const DL_DimensionData& data,
                                   const DL_DimAngular3PData& edata) {
    RS_DEBUG->print("RS_FilterDXF::addDimAngular3P");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(edata.dpx3, edata.dpy3);
    RS_Vector dp2(edata.dpx1, edata.dpy1);
    RS_Vector dp3(edata.dpx3, edata.dpy3);
    RS_Vector dp4 = dimensionData.definitionPoint;
    dimensionData.definitionPoint = RS_Vector(edata.dpx2, edata.dpy2);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    RS_DimAngular* entity = new RS_DimAngular(currentContainer,
                            dimensionData, d);

    setEntityAttributes(entity, attributes);
    entity->update();
    currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles leader entities.
 */
void RS_FilterDXF::addLeader(const DL_LeaderData& data) {
    RS_DEBUG->print("RS_FilterDXF::addDimLeader");
    //RS_DEBUG->print("RS_FilterDXF::addPolyline()");
    RS_LeaderData d(data.arrowHeadFlag==1);
    leader = new RS_Leader(currentContainer, d);
    setEntityAttributes(leader, attributes);

    currentContainer->addEntity(leader);
}



/**
 * Implementation of the method which handles leader vertices.
 */
void RS_FilterDXF::addLeaderVertex(const DL_LeaderVertexData& data) {
    RS_DEBUG->print("RS_FilterDXF::addLeaderVertex");
    //RS_DEBUG->print("RS_FilterDXF::addVertex() bulge: %f", bulge);

    RS_Vector v(data.x, data.y);

    if (leader!=NULL) {
        leader->addVertex(v);
    }
}



/**
 * Implementation of the method which handles hatch entities.
 */
void RS_FilterDXF::addHatch(const DL_HatchData& data) {
    RS_DEBUG->print("RS_FilterDXF::addHatch()");

    hatch = new RS_Hatch(currentContainer,
                         RS_HatchData(data.solid,
                                      data.scale,
                                      data.angle,
                                      QString(QString::fromUtf8(data.pattern.c_str()))));
    setEntityAttributes(hatch, attributes);
    omitHatchLoop = false;

    currentContainer->addEntity(hatch);
}



/**
 * Implementation of the method which handles hatch loops.
 */
void RS_FilterDXF::addHatchLoop(const DL_HatchLoopData& data) {
    RS_DEBUG->print("RS_FilterDXF::addHatchLoop()");
    if ( (data.pathType & 32) == 32)
        omitHatchLoop = true;
    else
        omitHatchLoop = false;
    if (hatch!=NULL && !omitHatchLoop) {
        hatchLoop = new RS_EntityContainer(hatch);
        hatchLoop->setLayer(NULL);
        hatch->addEntity(hatchLoop);
    }
}



/**
 * Implementation of the method which handles hatch edge entities.
 */
void RS_FilterDXF::addHatchEdge(const DL_HatchEdgeData& data) {
    RS_DEBUG->print("RS_FilterDXF::addHatchEdge()");

    if (hatchLoop!=NULL && !omitHatchLoop) {
        RS_Entity* e = NULL;
        switch (data.type) {
        case 1:
            RS_DEBUG->print("RS_FilterDXF::addHatchEdge(): "
                            "line: %f,%f %f,%f",
                            data.x1, data.y1, data.x2, data.y2);
            e = new RS_Line(hatchLoop,
                            RS_LineData(RS_Vector(data.x1, data.y1),
                                        RS_Vector(data.x2, data.y2)));
            break;
        case 2:
            if (data.ccw && data.angle1<1.0e-6 && data.angle2>2*M_PI-1.0e-6) {
                e = new RS_Circle(hatchLoop,
                                  RS_CircleData(RS_Vector(data.cx, data.cy),
                                                data.radius));
            } else {
                if (data.ccw) {
                    e = new RS_Arc(
                            hatchLoop,
                            RS_ArcData(RS_Vector(data.cx, data.cy),
                                       data.radius,
                                       RS_Math::correctAngle(data.angle1),
                                       RS_Math::correctAngle(data.angle2),
                                       false));
                } else {
                    e = new RS_Arc(
                            hatchLoop,
                            RS_ArcData(RS_Vector(data.cx, data.cy),
                                       data.radius,
                                       RS_Math::correctAngle(2*M_PI-data.angle1),
                                       RS_Math::correctAngle(2*M_PI-data.angle2),
                                       true));
                }
            }
            break;
        default:
            break;
        }

        if (e!=NULL) {
            e->setLayer(NULL);
            hatchLoop->addEntity(e);
        }
    }
}



/**
 * Implementation of the method which handles image entities.
 */
void RS_FilterDXF::addImage(const DL_ImageData& data) {
    RS_DEBUG->print("RS_FilterDXF::addImage");

    RS_Vector ip(data.ipx, data.ipy);
    RS_Vector uv(data.ux, data.uy);
    RS_Vector vv(data.vx, data.vy);
    RS_Vector size(data.width, data.height);

    RS_Image* image =
        new RS_Image(
            currentContainer,
            RS_ImageData(QString(data.ref.c_str()).toInt(NULL, 16),
                         ip, uv, vv,
                         size,
                         QString(""),
                         data.brightness,
                         data.contrast,
                         data.fade));

    setEntityAttributes(image, attributes);
    currentContainer->addEntity(image);
}



/**
 * Implementation of the method which links image entities to image files.
 */
void RS_FilterDXF::linkImage(const DL_ImageDefData& data) {
    RS_DEBUG->print("RS_FilterDXF::linkImage");

    int handle = QString(data.ref.c_str()).toInt(NULL, 16);
    QString sfile(QString::fromUtf8(data.file.c_str()));
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



/**
 * Finishes a hatch entity.
 */
void RS_FilterDXF::endEntity() {
    RS_DEBUG->print("RS_FilterDXF::endEntity");

    if (hatch!=NULL) {

        RS_DEBUG->print("hatch->update()");

        if (hatch->validate()) {
            hatch->update();
        } else {
            graphic->removeEntity(hatch);
            RS_DEBUG->print(RS_Debug::D_ERROR,
                            "RS_FilterDXF::endEntity(): updating hatch failed: invalid hatch area");
        }
        hatch=NULL;
    }
}



/**
 * Sets a vector variable from the DXF file.
 */
void RS_FilterDXF::setVariableVector(const char* key,
                                     double v1, double v2, double v3, int code) {
    RS_DEBUG->print("RS_FilterDXF::setVariableVector");

    // update document's variable list:
    if (currentContainer->rtti()==RS2::EntityGraphic) {
        ((RS_Graphic*)currentContainer)->addVariable(QString(key),
#ifdef  RS_VECTOR2D
                RS_Vector(v1, v2), code);
#else
                RS_Vector(v1, v2, v3), code);
#endif
    }
}



/**
 * Sets a string variable from the DXF file.
 */
void RS_FilterDXF::setVariableString(const char* key,
                                     const char* value, int code) {
    RS_DEBUG->print("RS_FilterDXF::setVariableString");

    // update local DXF variable list:
    variables.add(QString(key), QString(value), code);

    // update document's variable list:
    if (currentContainer->rtti()==RS2::EntityGraphic) {
        ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                QString(value), code);
    }
}



/**
 * Sets an int variable from the DXF file.
 */
void RS_FilterDXF::setVariableInt(const char* key, int value, int code) {
    RS_DEBUG->print("RS_FilterDXF::setVariableInt");

    // update document's variable list:
    if (currentContainer->rtti()==RS2::EntityGraphic) {
        ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                value, code);
    }
}



/**
 * Sets a double variable from the DXF file.
 */
void RS_FilterDXF::setVariableDouble(const char* key, double value, int code) {
    RS_DEBUG->print("RS_FilterDXF::setVariableDouble");

    // update document's variable list:
    if (currentContainer->rtti()==RS2::EntityGraphic) {
        ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                value, code);
    }

}



/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the DXF file that will be written.
 */
bool RS_FilterDXF::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) {

    RS_DEBUG->print("RS_FilterDXF::fileExport: exporting file '%s'...",
                    (const char*)QFile::encodeName(file));
    RS_DEBUG->print("RS_FilterDXF::fileExport: file type '%d'", (int)type);

    this->graphic = &g;

    // check if we can write to that directory:
#ifndef Q_OS_WIN

    QString path = QFileInfo(file).absolutePath();
    if (QFileInfo(path).isWritable()==false) {
        RS_DEBUG->print("RS_FilterDXF::fileExport: can't write file: "
                        "no permission");
        return false;
    }
    //
#endif

    // set version for DXF filter:
    DL_Codes::version exportVersion;
    if (type==RS2::FormatDXFOLD12) {
        exportVersion = DL_Codes::AC1009;
    } else {
        exportVersion = DL_Codes::AC1015;
    }

    //DL_WriterA* dw = dxf.out(file, VER_R12);
    DL_WriterA* dw = dxf.out((const char*)QFile::encodeName(file), exportVersion);

    if (dw==NULL) {
        RS_DEBUG->print("RS_FilterDXF::fileExport: can't write file");
        return false;
    }

    // Header
    RS_DEBUG->print("writing headers...");
    dxf.writeHeader(*dw);

    // Variables
    RS_DEBUG->print("writing variables...");
    writeVariables(*dw);

    // Section TABLES
    RS_DEBUG->print("writing tables...");
    dw->sectionTables();

    // VPORT:
    dxf.writeVPort(*dw);

    // Line types:
    RS_DEBUG->print("writing line types...");
    int numLT = (int)RS2::BorderLineX2-(int)RS2::LineByBlock;
    if (type==RS2::FormatDXFOLD12) {
        numLT-=2;
    }
    dw->tableLineTypes(numLT);
    for (int t=(int)RS2::LineByBlock; t<=(int)RS2::BorderLineX2; ++t) {
        if ((RS2::LineType)t!=RS2::NoPen) {
            writeLineType(*dw, (RS2::LineType)t);
        }
    }
    dw->tableEnd();

    // Layers:
    RS_DEBUG->print("writing layers...");
    dw->tableLayers(graphic->countLayers());
    for (uint i=0; i<graphic->countLayers(); ++i) {
        RS_Layer* l = graphic->layerAt(i);
        writeLayer(*dw, l);
    }
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

    // DIMSTYLE:
    RS_DEBUG->print("writing dim styles...");
    dxf.writeDimStyle(*dw,
                      graphic->getVariableDouble("$DIMASZ", 2.5),
                      graphic->getVariableDouble("$DIMEXE", 1.25),
                      graphic->getVariableDouble("$DIMEXO", 0.625),
                      graphic->getVariableDouble("$DIMGAP", 0.625),
                      graphic->getVariableDouble("$DIMTXT", 2.5));

    // BLOCK_RECORD:
    if (type==RS2::FormatDXFOLD) {
        RS_DEBUG->print("writing block records...");
        dxf.writeBlockRecord(*dw);

        for (uint i=0; i<graphic->countBlocks(); ++i) {
            RS_Block* blk = graphic->blockAt(i);
                        dxf.writeBlockRecord(*dw,
                            std::string(blk->getName().toLocal8Bit()));
                        /*
                        // v2.0.4.9..:
            //writeBlock(*dw, blk);
            dw->dxfString(  0, "BLOCK_RECORD");
            //dw.dxfHex(5, 0x1F);
            dw->handle();
            dw->dxfHex(330, 1);
            dw->dxfString(100, "AcDbSymbolTableRecord");
            dw->dxfString(100, "AcDbBlockTableRecord");
            dw->dxfString(  2, blk->getName().toLocal8Bit());
            dw->dxfHex(340, 0);
                        */
        }
        dw->tableEnd();
    }

    // end of tables:
    RS_DEBUG->print("writing end of section TABLES...");
    dw->sectionEnd();


    // Section BLOCKS:
    RS_DEBUG->print("writing blocks...");
    dw->sectionBlocks();

    if (type==RS2::FormatDXFOLD) {
        RS_Block b1(graphic, RS_BlockData("*Model_Space",
                                          RS_Vector(0.0,0.0), false));
        writeBlock(*dw, &b1);
        RS_Block b2(graphic, RS_BlockData("*Paper_Space",
                                          RS_Vector(0.0,0.0), false));
        writeBlock(*dw, &b2);
        RS_Block b3(graphic, RS_BlockData("*Paper_Space0",
                                          RS_Vector(0.0,0.0), false));
        writeBlock(*dw, &b3);
    }

    for (uint i=0; i<graphic->countBlocks(); ++i) {
        RS_Block* blk = graphic->blockAt(i);

        // Save block if it's not a model or paper space:
        // Careful: other blocks with * / $ exist
        //if (blk->getName().at(0)!='*' &&
        //        blk->getName().at(0)!='$') {
        writeBlock(*dw, blk);
        //}
    }
    dw->sectionEnd();


    // Section ENTITIES:
    RS_DEBUG->print("writing section ENTITIES...");
    dw->sectionEntities();
    for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=graphic->nextEntity(RS2::ResolveNone)) {

        writeEntity(*dw, e);
    }
    RS_DEBUG->print("writing end of section ENTITIES...");
    dw->sectionEnd();

    if (type==RS2::FormatDXFOLD) {
        RS_DEBUG->print("writing section OBJECTS...");
        dxf.writeObjects(*dw);

        // IMAGEDEF's from images in entities and images in blocks
        QStringList written;
        for (uint i=0; i<graphic->countBlocks(); ++i) {
            RS_Block* block = graphic->blockAt(i);
            for (RS_Entity* e=block->firstEntity(RS2::ResolveAll);
                    e!=NULL;
                    e=block->nextEntity(RS2::ResolveAll)) {

                if (e->rtti()==RS2::EntityImage) {
                    RS_Image* img = ((RS_Image*)e);
                    if (written.contains(file)==0 && img->getHandle()!=0) {
                        writeImageDef(*dw, img);
                        written.append(img->getFile());
                    }
                }
            }
        }
        for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
                e!=NULL;
                e=graphic->nextEntity(RS2::ResolveNone)) {

            if (e->rtti()==RS2::EntityImage) {
                RS_Image* img = ((RS_Image*)e);
                if (written.contains(file)==0 && img->getHandle()!=0) {
                    writeImageDef(*dw, img);
                    written.append(img->getFile());
                }
            }
        }
        RS_DEBUG->print("writing end of section OBJECTS...");
        dxf.writeObjectsEnd(*dw);
    }

    RS_DEBUG->print("writing EOF...");
    dw->dxfEOF();


    RS_DEBUG->print("close..");
    dw->close();

    delete dw;

    // check if file was actually written (strange world of windoze xp):
    if (QFileInfo(file).exists()==false) {
        RS_DEBUG->print("RS_FilterDXF::fileExport: file could not be written");
        return false;
    }

    return true;
}



/**
 * Writes all known variable settings to the DXF file.
 */
void RS_FilterDXF::writeVariables(DL_WriterA& dw) {
    QHash<QString, RS_Variable>vars = graphic->getVariableDict();
    QHash<QString, RS_Variable>::iterator it = vars.begin();
    while (it != vars.end()) {
        // exclude variables that are not known to DXF 12:
        if (!DL_Dxf::checkVariable(it.key().toLatin1(), dxf.getVersion())) {
            continue;
        }

        if (it.key()!="$ACADVER" && it.key()!="$HANDSEED") {

            dw.dxfString(9, it.key().toLatin1());
            switch (it.value().getType()) {
            case RS2::VariableVoid:
                break;
            case RS2::VariableInt:
                dw.dxfInt(it.value().getCode(), it.value().getInt());
                break;
            case RS2::VariableDouble:
                dw.dxfReal(it.value().getCode(), it.value().getDouble());
                break;
            case RS2::VariableString:
                dw.dxfString(it.value().getCode(),
                             it.value().getString().toLatin1());
                break;
            case RS2::VariableVector:
                dw.dxfReal(it.value().getCode(),
                           it.value().getVector().x);
                dw.dxfReal(it.value().getCode()+10,
                           it.value().getVector().y);
                if ( isVariableTwoDimensional(it.key()) == false) {
                    dw.dxfReal(it.value().getCode()+20,
#ifdef  RS_VECTOR2D
                               0.);
#else
                               it.value().getVector().z);
#endif
                }
                break;
            }
        }
        ++it;
    }
    dw.sectionEnd();
}



/**
 * Writes one layer to the DXF file.
 *
 * @todo Add support for unicode layer names
 */
void RS_FilterDXF::writeLayer(DL_WriterA& dw, RS_Layer* l) {
    if (l==NULL) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::writeLayer: layer is NULL");
        return;
    }

    RS_DEBUG->print("RS_FilterDXF::writeLayer %s", l->getName().toLatin1().data());

    dxf.writeLayer(
        dw,
        DL_LayerData(toDxfString(l->getName()).toStdString(),  //RLZ: verify layername whit locales
                     l->isFrozen() + (l->isLocked()<<2), ! l->isConstructionLayer()),
        DL_Attributes(std::string(""),
                      colorToNumber(l->getPen().getColor()),
                      widthToNumber(l->getPen().getWidth()),
                      (const char*)lineTypeToName(
                          l->getPen().getLineType()).toLocal8Bit()));

    RS_DEBUG->print("RS_FilterDXF::writeLayer end");
}



/**
 * Writes a line type to the DXF file.
 */
void RS_FilterDXF::writeLineType(DL_WriterA& dw, RS2::LineType t) {
    dxf.writeLineType(
        dw,
        DL_LineTypeData((const char*)lineTypeToName(t).toLocal8Bit(), 0));
}



/**
 * Writes an application id to the DXF file.
 *
 * @param appid Application ID (e.g. "LibreCAD").
 */
void RS_FilterDXF::writeAppid(DL_WriterA& dw, const char* appid) {
    dxf.writeAppid(dw, appid);
}



/**
 * Writes a block (just the definition, not the entities in it).
 */
void RS_FilterDXF::writeBlock(DL_WriterA& dw, RS_Block* blk) {
    if (blk==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::writeBlock: Block is NULL");
        return;
    }

    RS_DEBUG->print("writing block: %s", (const char*)blk->getName().toLocal8Bit());

    dxf.writeBlock(dw,
                   DL_BlockData((const char*)blk->getName().toLocal8Bit(), 0,
                                blk->getBasePoint().x,
                                blk->getBasePoint().y,
#ifdef  RS_VECTOR2D
                                0.));
#else
                                blk->getBasePoint().z));
#endif
    for (RS_Entity* e=blk->firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=blk->nextEntity(RS2::ResolveNone)) {
        writeEntity(dw, e);
    }
    dxf.writeEndBlock(dw, (const char*)blk->getName().toLocal8Bit());
}



/**
 * Writes the given entity to the DXF file.
 */
void RS_FilterDXF::writeEntity(DL_WriterA& dw, RS_Entity* e) {
    writeEntity(dw, e, getEntityAttributes(e));
}


/**
 * Writes the given entity to the DXF file.
 */
void RS_FilterDXF::writeEntity(DL_WriterA& dw, RS_Entity* e,
                               const DL_Attributes& attrib) {

    if (e==NULL || e->getFlag(RS2::FlagUndone)) {
        return;
    }
    RS_DEBUG->print("writing Entity");

    switch (e->rtti()) {
    case RS2::EntityPoint:
        writePoint(dw, (RS_Point*)e, attrib);
        break;
    case RS2::EntityLine:
        writeLine(dw, (RS_Line*)e, attrib);
        break;
    case RS2::EntityPolyline:
        writePolyline(dw, (RS_Polyline*)e, attrib);
        break;
    case RS2::EntitySpline:
        writeSpline(dw, (RS_Spline*)e, attrib);
        break;
    case RS2::EntitySplinePoints:
        writeSplinePoints(dw, (LC_SplinePoints*)e, attrib);
        break;
    case RS2::EntityVertex:
        break;
    case RS2::EntityCircle:
        writeCircle(dw, (RS_Circle*)e, attrib);
        break;
    case RS2::EntityArc:
        writeArc(dw, (RS_Arc*)e, attrib);
        break;
    case RS2::EntityEllipse:
        writeEllipse(dw, (RS_Ellipse*)e, attrib);
        break;
    case RS2::EntityInsert:
        writeInsert(dw, (RS_Insert*)e, attrib);
        break;
    case RS2::EntityText:
        writeText(dw, (RS_Text*)e, attrib);
        break;

    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimLinear:
    case RS2::EntityDimRadial:
    case RS2::EntityDimDiametric:
        writeDimension(dw, (RS_Dimension*)e, attrib);
        break;
    case RS2::EntityDimLeader:
        writeLeader(dw, (RS_Leader*)e, attrib);
        break;
    case RS2::EntityHatch:
        writeHatch(dw, (RS_Hatch*)e, attrib);
        break;
    case RS2::EntityImage:
        writeImage(dw, (RS_Image*)e, attrib);
        break;
    case RS2::EntitySolid:
        writeSolid(dw, (RS_Solid*)e, attrib);
        break;

#ifndef RS_NO_COMPLEX_ENTITIES

    case RS2::EntityContainer:
        writeEntityContainer(dw, (RS_EntityContainer*)e, attrib);
        break;
#endif

    default:
        break;
    }
}



/**
 * Writes the given Point entity to the file.
 */
void RS_FilterDXF::writePoint(DL_WriterA& dw, RS_Point* p,
                              const DL_Attributes& attrib) {
    dxf.writePoint(
        dw,
        DL_PointData(p->getPos().x,
                     p->getPos().y,
                     0.0),
        attrib);
}


/**
 * Writes the given Line( entity to the file.
 */
void RS_FilterDXF::writeLine(DL_WriterA& dw, RS_Line* l,
                             const DL_Attributes& attrib) {
    dxf.writeLine(
        dw,
        DL_LineData(l->getStartpoint().x,
                    l->getStartpoint().y,
                    0.0,
                    l->getEndpoint().x,
                    l->getEndpoint().y,
                    0.0),
        attrib);
}



/**
 * Writes the given polyline entity to the file.
 */
void RS_FilterDXF::writePolyline(DL_WriterA& dw,
                                 RS_Polyline* l,
                                 const DL_Attributes& attrib) {

        int count = l->count();
        if (l->isClosed()==false) {
                count++;
        }

        dxf.writePolyline(
        dw,
        DL_PolylineData(count,
                        0, 0,
                        l->isClosed()*0x1),
        attrib);
    bool first = true;
    RS_Entity* nextEntity = 0;
    RS_AtomicEntity* ae = NULL;
        RS_Entity* lastEntity = l->lastEntity(RS2::ResolveNone);
    for (RS_Entity* v=l->firstEntity(RS2::ResolveNone);
            v!=NULL;
            v=nextEntity) {

        nextEntity = l->nextEntity(RS2::ResolveNone);

        if (!v->isAtomic()) {
            continue;
        }

        ae = (RS_AtomicEntity*)v;
        double bulge=0.0;

                // Write vertex:
        if (first) {
            if (v->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)v)->getBulge();
            }
            dxf.writeVertex(dw,
                            DL_VertexData(ae->getStartpoint().x,
                                          ae->getStartpoint().y,
                                          0.0,
                                                                                  bulge));
            first = false;
        }

        //if (dxf.getVersion()==VER_R12) {
            if (nextEntity!=NULL) {
                if (nextEntity->rtti()==RS2::EntityArc) {
                    bulge = ((RS_Arc*)nextEntity)->getBulge();
                }
                                else {
                                        bulge = 0.0;
                                }
            }
        /*} else {
            if (v->rtti()==RS2::EntityArc) {
                bulge = ((RS_Arc*)v)->getBulge();
            }
        }*/


                if (l->isClosed()==false || v!=lastEntity) {
                dxf.writeVertex(dw,
                        DL_VertexData(ae->getEndpoint().x,
                                      ae->getEndpoint().y,
                                      0.0,
                                      bulge));
                }
    }
    dxf.writePolylineEnd(dw);
}



/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXF::writeSpline(DL_WriterA& dw,
                               RS_Spline* s,
                               const DL_Attributes& attrib) {

    // split spline into atomic entities for DXF R12:
    if (dxf.getVersion()==VER_R12) {
        writeAtomicEntities(dw, s, attrib, RS2::ResolveNone);
        return;
    }

    if (s->getNumberOfControlPoints() < s->getDegree()+1) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_FilterDXF::writeSpline: "
                        "Discarding spline: not enough control points given.");
        return;
    }

    // Number of control points:
    int numCtrl = s->getNumberOfControlPoints();

    // Number of knots (= number of control points + spline degree + 1)
    int numKnots = numCtrl + s->getDegree() + 1;

    int flags;
    if (s->isClosed()) {
        flags = 11;
    } else {
        flags = 8;
    }

    // write spline header:
    dxf.writeSpline(
        dw,
        DL_SplineData(s->getDegree(),
                      numKnots,
                      numCtrl,
                      flags),
        attrib);

    // write spline knots:
    QList<RS_Vector> cp = s->getControlPoints();
    QList<RS_Vector>::iterator it;

    int k = s->getDegree()+1;
    DL_KnotData kd;
    for (int i=1; i<=numKnots; i++) {
        if (i<=k) {
            kd = DL_KnotData(0.0);
        } else if (i<=numKnots-k) {
            kd = DL_KnotData(1.0/(numKnots-2*k+1) * (i-k));
        } else {
            kd = DL_KnotData(1.0);
        }
        dxf.writeKnot(dw,
                      kd);
    }

    // write spline control points:
    for (it = cp.begin(); it!=cp.end(); ++it) {
        dxf.writeControlPoint(dw,
                              DL_ControlPointData((*it).x,
                                                  (*it).y,
                                                  0.0));
    }
}



/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXF::writeSplinePoints(DL_WriterA& dw,
	LC_SplinePoints* s, const DL_Attributes& attrib)
{
	// split spline into atomic entities for DXF R12:
	if(dxf.getVersion() == VER_R12)
	{
		QList<RS_Vector> sp = s->getStrokePoints();
        dxf.writePolyline(dw, DL_PolylineData(sp.size, 0, 0, s->isClosed()*0x1), attrib);
		for(int i = 0; i < sp.count(); i++)
		{
			dxf.writeVertex(dw, DL_VertexData(sp.at(i).x, sp.at(i).y, 0.0, 0.0));
		}
		dxf.writePolylineEnd(dw);
		return;
	}

	// Number of control points:
	int numCtrl = s->getNumberOfControlPoints();
	QList<RS_Vector> cp = s->getControlPoints();

	if(numCtrl < 3)
	{
		if(numCtrl > 1)
		{
			dxf.writeLine(dw, cp.at(0).x, cp.at(0).y, 0.0,
				cp.at(1).x, cp.at(1).y, 0.0), attrib);
		}
		return;
	}

	// Number of knots (= number of control points + spline degree + 1)
	int numKnots = numCtrl + 3;

	int flags;
	if(s->isClosed()) flags = 11;
	else flags = 8;

	// write spline header:
	dxf.writeSpline(dw, DL_SplineData(2, numKnots, numCtrl, flags), attrib);

	// write spline knots:
	int k = 3;
	DL_KnotData kd;
	for(int i = 1; i <= numKnots; i++) 
	{
		if(i <= 3) kd = DL_KnotData(0.0);
		else if(i <= numCtrl)
			kd = DL_KnotData((i - 3.0)/(numCtrl - 2.0));
		else kd = DL_KnotData(1.0);
		dxf.writeKnot(dw, kd);
	}

	// write spline control points:
	QList<RS_Vector>::iterator it;
	for(it = cp.begin(); it != cp.end(); it++)
	{
		dxf.writeControlPoint(dw, DL_ControlPointData((*it).x, (*it).y, 0.0));
	}
}



/**
 * Writes the given circle entity to the file.
 */
void RS_FilterDXF::writeCircle(DL_WriterA& dw, RS_Circle* c,
                               const DL_Attributes& attrib) {
    dxf.writeCircle(
        dw,
        DL_CircleData(c->getCenter().x,
                      c->getCenter().y,
                      0.0,
                      c->getRadius()),
        attrib);

}



void RS_FilterDXF::writeArc(DL_WriterA& dw, RS_Arc* a,
                            const DL_Attributes& attrib) {
    double a1, a2;
    if (a->isReversed()) {
        a1 = a->getAngle2()*ARAD;
        a2 = a->getAngle1()*ARAD;
    } else {
        a1 = a->getAngle1()*ARAD;
        a2 = a->getAngle2()*ARAD;
    }
    dxf.writeArc(
        dw,
        DL_ArcData(a->getCenter().x,
                   a->getCenter().y,
                   0.0,
                   a->getRadius(),
                   a1, a2),
        attrib);
}


void RS_FilterDXF::writeEllipse(DL_WriterA& dw, RS_Ellipse* s,
                                const DL_Attributes& attrib) {
    if (s->isReversed()) {
        dxf.writeEllipse(
            dw,
            DL_EllipseData(s->getCenter().x,
                           s->getCenter().y,
                           0.0,
                           s->getMajorP().x,
                           s->getMajorP().y,
                           0.0,
                           s->getRatio(),
                           s->getAngle2(),
                           s->getAngle1()),
            attrib);
    } else {
        dxf.writeEllipse(
            dw,
            DL_EllipseData(s->getCenter().x,
                           s->getCenter().y,
                           0.0,
                           s->getMajorP().x,
                           s->getMajorP().y,
                           0.0,
                           s->getRatio(),
                           s->getAngle1(),
                           s->getAngle2()),
            attrib);
    }
}



void RS_FilterDXF::writeInsert(DL_WriterA& dw, RS_Insert* i,
                               const DL_Attributes& attrib) {
    dxf.writeInsert(
        dw,
                DL_InsertData(i->getName().toLatin1().data(),
                      i->getInsertionPoint().x,
                      i->getInsertionPoint().y,
                      0.0,
                      i->getScale().x,
                      i->getScale().y,
                      0.0,
                      i->getAngle()*ARAD,
                      i->getCols(), i->getRows(),
                      i->getSpacing().x,
                      i->getSpacing().y),
        attrib);
}


void RS_FilterDXF::writeText(DL_WriterA& dw, RS_Text* t,
                             const DL_Attributes& attrib) {

    if (dxf.getVersion()==VER_R12) {
        int hJust=0;
        int vJust=0;
        if (t->getHAlign()==RS2::HAlignLeft) {
            hJust=0;
        } else if (t->getHAlign()==RS2::HAlignCenter) {
            hJust=1;
        } else if (t->getHAlign()==RS2::HAlignRight) {
            hJust=2;
        }
        if (t->getVAlign()==RS2::VAlignTop) {
            vJust=3;
        } else if (t->getVAlign()==RS2::VAlignMiddle) {
            vJust=2;
        } else if (t->getVAlign()==RS2::VAlignBottom) {
            vJust=1;
        }
        dxf.writeText(
            dw,
            DL_TextData(t->getInsertionPoint().x,
                        t->getInsertionPoint().y,
                        0.0,
                        t->getInsertionPoint().x,
                        t->getInsertionPoint().y,
                        0.0,
                        t->getHeight(),
                        1.0,
                        0,
                        hJust, vJust,
                        (const char*)toDxfString(
                            t->getText()).toLocal8Bit(),
                        (const char*)t->getStyle().toLocal8Bit(),
                        t->getAngle()),
            attrib);

    } else {
        int attachmentPoint=1;
        if (t->getHAlign()==RS2::HAlignLeft) {
            attachmentPoint=1;
        } else if (t->getHAlign()==RS2::HAlignCenter) {
            attachmentPoint=2;
        } else if (t->getHAlign()==RS2::HAlignRight) {
            attachmentPoint=3;
        }
        if (t->getVAlign()==RS2::VAlignTop) {
            attachmentPoint+=0;
        } else if (t->getVAlign()==RS2::VAlignMiddle) {
            attachmentPoint+=3;
        } else if (t->getVAlign()==RS2::VAlignBottom) {
            attachmentPoint+=6;
        }

        dxf.writeMText(
            dw,
            DL_MTextData(t->getInsertionPoint().x,
                         t->getInsertionPoint().y,
                         0.0,
                         t->getHeight(),
                         t->getWidth(),
                         attachmentPoint,
                         t->getDrawingDirection(),
                         t->getLineSpacingStyle(),
                         t->getLineSpacingFactor(),
                         (const char*)toDxfString(
                             t->getText()).toLocal8Bit(),
                         (const char*)t->getStyle().toLocal8Bit(),
                         t->getAngle()),
            attrib);
    }
}



void RS_FilterDXF::writeDimension(DL_WriterA& dw, RS_Dimension* d,
                                  const DL_Attributes& attrib) {

    // split hatch into atomic entities:
    if (dxf.getVersion()==VER_R12) {
        writeAtomicEntities(dw, d, attrib, RS2::ResolveNone);
        return;
    }

    int type;
    int attachmentPoint=1;
    if (d->getHAlign()==RS2::HAlignLeft) {
        attachmentPoint=1;
    } else if (d->getHAlign()==RS2::HAlignCenter) {
        attachmentPoint=2;
    } else if (d->getHAlign()==RS2::HAlignRight) {
        attachmentPoint=3;
    }
    if (d->getVAlign()==RS2::VAlignTop) {
        attachmentPoint+=0;
    } else if (d->getVAlign()==RS2::VAlignMiddle) {
        attachmentPoint+=3;
    } else if (d->getVAlign()==RS2::VAlignBottom) {
        attachmentPoint+=6;
    }

    switch (d->rtti()) {
    case RS2::EntityDimAligned:
        type = 1;
        break;
    case RS2::EntityDimLinear:
        type = 0;
        break;
    case RS2::EntityDimRadial:
        type = 4;
        break;
    case RS2::EntityDimDiametric:
        type = 3;
        break;
    default:
        type = 0;
        break;
    }

    DL_DimensionData dimData(d->getDefinitionPoint().x,
                             d->getDefinitionPoint().y,
                             0.0,
                             d->getMiddleOfText().x,
                             d->getMiddleOfText().y,
                             0.0,
                             type,
                             attachmentPoint,
                             d->getLineSpacingStyle(),
                             d->getLineSpacingFactor(),
                             (const char*)toDxfString(
                                 d->getText()).toLocal8Bit(),
                             (const char*)d->getStyle().toLocal8Bit(),
                             d->getAngle());

    if (d->rtti()==RS2::EntityDimAligned) {
        RS_DimAligned* da = (RS_DimAligned*)d;

        DL_DimAlignedData dimAlignedData(da->getExtensionPoint1().x,
                                         da->getExtensionPoint1().y,
                                         0.0,
                                         da->getExtensionPoint2().x,
                                         da->getExtensionPoint2().y,
                                         0.0);

        dxf.writeDimAligned(dw, dimData, dimAlignedData, attrib);
    } else if (d->rtti()==RS2::EntityDimLinear) {
        RS_DimLinear* dl = (RS_DimLinear*)d;

        DL_DimLinearData dimLinearData(dl->getExtensionPoint1().x,
                                       dl->getExtensionPoint1().y,
                                       0.0,
                                       dl->getExtensionPoint2().x,
                                       dl->getExtensionPoint2().y,
                                       0.0,
                                       dl->getAngle(),
                                       dl->getOblique());

        dxf.writeDimLinear(dw, dimData, dimLinearData, attrib);
    } else if (d->rtti()==RS2::EntityDimRadial) {
        RS_DimRadial* dr = (RS_DimRadial*)d;

        DL_DimRadialData dimRadialData(dr->getDefinitionPoint().x,
                                       dr->getDefinitionPoint().y,
                                       0.0,
                                       dr->getLeader());

        dxf.writeDimRadial(dw, dimData, dimRadialData, attrib);
    } else if (d->rtti()==RS2::EntityDimDiametric) {
        RS_DimDiametric* dr = (RS_DimDiametric*)d;

        DL_DimDiametricData dimDiametricData(dr->getDefinitionPoint().x,
                                             dr->getDefinitionPoint().y,
                                             0.0,
                                             dr->getLeader());

        dxf.writeDimDiametric(dw, dimData, dimDiametricData, attrib);
    } else if (d->rtti()==RS2::EntityDimAngular) {
        RS_DimAngular* da = (RS_DimAngular*)d;

        DL_DimAngularData dimAngularData(da->getDefinitionPoint1().x,
                                         da->getDefinitionPoint1().y,
                                         0.0,
                                         da->getDefinitionPoint2().x,
                                         da->getDefinitionPoint2().y,
                                         0.0,
                                         da->getDefinitionPoint3().x,
                                         da->getDefinitionPoint3().y,
                                         0.0,
                                         da->getDefinitionPoint4().x,
                                         da->getDefinitionPoint4().y,
                                         0.0);

        dxf.writeDimAngular(dw, dimData, dimAngularData, attrib);
    }

}


void RS_FilterDXF::writeLeader(DL_WriterA& dw, RS_Leader* l,
                               const DL_Attributes& attrib) {
    if (l->count()>0) {
        dxf.writeLeader(
            dw,
            DL_LeaderData(l->hasArrowHead(),
                          0,
                          3,
                          0,
                          0,
                          1.0,
                          10.0,
                          l->count()),
            attrib);
        bool first = true;
        for (RS_Entity* v=l->firstEntity(RS2::ResolveNone);
                v!=NULL;
                v=l->nextEntity(RS2::ResolveNone)) {

            // Write line verties:
            if (v->rtti()==RS2::EntityLine) {
                RS_Line* l = (RS_Line*)v;
                if (first) {
                    dxf.writeLeaderVertex(
                        dw,
                        DL_LeaderVertexData(l->getStartpoint().x,
                                            l->getStartpoint().y,
                                            0.0));
                    first = false;
                }
                dxf.writeLeaderVertex(
                    dw,
                    DL_LeaderVertexData(l->getEndpoint().x,
                                        l->getEndpoint().y,
                                        0.0));
            }
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "dropping leader with no vertices");
    }
}


void RS_FilterDXF::writeHatch(DL_WriterA& dw, RS_Hatch* h,
                              const DL_Attributes& attrib) {

    // split hatch into atomic entities:
    if (dxf.getVersion()==VER_R12) {
        writeAtomicEntities(dw, h, attrib, RS2::ResolveAll);
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
    } else {
        DL_HatchData data(h->countLoops(),
                          h->isSolid(),
                          h->getScale(),
                          h->getAngle(),
                          (const char*)h->getPattern().toLocal8Bit());
        dxf.writeHatch1(dw, data, attrib);

        for (RS_Entity* l=h->firstEntity(RS2::ResolveNone);
                l!=NULL;
                l=h->nextEntity(RS2::ResolveNone)) {

            // Write hatch loops:
            if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
                RS_EntityContainer* loop = (RS_EntityContainer*)l;
                DL_HatchLoopData lData(loop->count());
                dxf.writeHatchLoop1(dw, lData);

                for (RS_Entity* ed=loop->firstEntity(RS2::ResolveNone);
                        ed!=NULL;
                        ed=loop->nextEntity(RS2::ResolveNone)) {

                    // Write hatch loop edges:
                    if (ed->rtti()==RS2::EntityLine) {
                        RS_Line* ln = (RS_Line*)ed;
                        dxf.writeHatchEdge(
                            dw,
                            DL_HatchEdgeData(ln->getStartpoint().x,
                                             ln->getStartpoint().y,
                                             ln->getEndpoint().x,
                                             ln->getEndpoint().y));
                    } else if (ed->rtti()==RS2::EntityArc) {
                        RS_Arc* ar = (RS_Arc*)ed;
                        if (!ar->isReversed()) {
                            dxf.writeHatchEdge(
                                dw,
                                DL_HatchEdgeData(ar->getCenter().x,
                                                 ar->getCenter().y,
                                                 ar->getRadius(),
                                                 ar->getAngle1(),
                                                 ar->getAngle2(),
                                                 true));
                        } else {
                            dxf.writeHatchEdge(
                                dw,
                                DL_HatchEdgeData(ar->getCenter().x,
                                                 ar->getCenter().y,
                                                 ar->getRadius(),
                                                 2*M_PI-ar->getAngle1(),
                                                 2*M_PI-ar->getAngle2(),
                                                 false));
                        }
                    } else if (ed->rtti()==RS2::EntityCircle) {
                        RS_Circle* ci = (RS_Circle*)ed;
                        dxf.writeHatchEdge(
                            dw,
                            DL_HatchEdgeData(ci->getCenter().x,
                                             ci->getCenter().y,
                                             ci->getRadius(),
                                             0.0,
                                             2*M_PI,
                                             true));
                    }
                }
                dxf.writeHatchLoop2(dw, lData);
            }
        }
        dxf.writeHatch2(dw, data, attrib);
    }

}



void RS_FilterDXF::writeSolid(DL_WriterA& dw, RS_Solid* s,
                              const DL_Attributes& attrib) {

    // split solid into line entities:
    //if (dxf.getVersion()==VER_R12) {
        for (int i=0; i<3; ++i) {
            dxf.writeLine(
                dw,
                DL_LineData(s->getCorner(i).x,
                            s->getCorner(i).y,
                            0.0,
                            s->getCorner((i+1)%3).x,
                            s->getCorner((i+1)%3).y,
                            0.0),
                attrib);
        }
        //return;
    //}
}


void RS_FilterDXF::writeImage(DL_WriterA& dw, RS_Image* i,
                              const DL_Attributes& attrib) {
    int handle = dxf.writeImage(
                     dw,
                     DL_ImageData(std::string(""),
                                  i->getInsertionPoint().x,
                                  i->getInsertionPoint().y,
                                  0.0,
                                  i->getUVector().x,
                                  i->getUVector().y,
                                  0.0,
                                  i->getVVector().x,
                                  i->getVVector().y,
                                  0.0,
                                  i->getWidth(),
                                  i->getHeight(),
                                  i->getBrightness(),
                                  i->getContrast(),
                                  i->getFade()),
                     attrib);
    i->setHandle(handle);
}



void RS_FilterDXF::writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                                        const DL_Attributes& /*attrib*/) {
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

}



/**
 * Writes the atomic entities of the given cotnainer to the file.
 */
void RS_FilterDXF::writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c,
                                       const DL_Attributes& attrib,
                                       RS2::ResolveLevel level) {

    for (RS_Entity* e=c->firstEntity(level);
            e!=NULL;
            e=c->nextEntity(level)) {

        writeEntity(dw, e, attrib);
    }
}

/**
 * Writes an IMAGEDEF object into an OBJECT section.
 */
void RS_FilterDXF::writeImageDef(DL_WriterA& dw, RS_Image* i) {
    if (i==NULL || i->getFlag(RS2::FlagUndone)) {
        return;
    }

    dxf.writeImageDef(
        dw,
        i->getHandle(),
        DL_ImageData((const char*)i->getFile().toLocal8Bit(),
                     i->getInsertionPoint().x,
                     i->getInsertionPoint().y,
                     0.0,
                     i->getUVector().x,
                     i->getUVector().y,
                     0.0,
                     i->getVVector().x,
                     i->getVVector().y,
                     0.0,
                     i->getWidth(),
                     i->getHeight(),
                     i->getBrightness(),
                     i->getContrast(),
                     i->getFade()));
}



/**
 * Sets the entities attributes according to the attributes
 * that come from a DXF file.
 */
void RS_FilterDXF::setEntityAttributes(RS_Entity* entity,
                                       const DL_Attributes& attrib) {
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes");

    RS_Pen pen;
    pen.setColor(Qt::black);
    pen.setLineType(RS2::SolidLine);

    // Layer:
    if (attrib.getLayer().empty()) {
        entity->setLayer("0");
    } else {
        // add layer in case it doesn't exist:

        if (graphic->findLayer(QString::fromUtf8(attrib.getLayer().c_str()))==NULL) {
            addLayer(DL_LayerData(attrib.getLayer(), 0));
        }
        entity->setLayer(QString::fromUtf8(attrib.getLayer().c_str()));
    }

    // Color:
    pen.setColor(numberToColor(attrib.getColor()));

    // Linetype:
    pen.setLineType(nameToLineType(attrib.getLineType().c_str()));

    // Width:
    pen.setWidth(numberToWidth(attrib.getWidth()));

    entity->setPen(pen);
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes: OK");
}



/**
 * Gets the entities attributes as a DL_Attributes object.
 */
DL_Attributes RS_FilterDXF::getEntityAttributes(RS_Entity* entity) {

    // Layer:
    RS_Layer* layer = entity->getLayer();
    QString layerName;
    if (layer!=NULL) {
        layerName = layer->getName();
    } else {
        layerName = "NULL";
    }

    RS_Pen pen = entity->getPen(false);

    // Color:
    int color = colorToNumber(pen.getColor());
    //printf("Color is: %s -> %d\n", pen.getColor().name().toLatin1().data(), color);

    // Linetype:
    QString lineType = lineTypeToName(pen.getLineType());

    // Width:
    int width = widthToNumber(pen.getWidth());

    DL_Attributes attrib(toDxfString(layerName).toStdString(),
                         color,
                         width,
                         lineType.toLatin1().data());

    return attrib;
}



/**
 * @return Pen with the same attributes as 'attrib'.
 */
RS_Pen RS_FilterDXF::attributesToPen(const DL_Attributes& attrib) const {

    /*
    printf("converting Color %d to %s\n",
       attrib.getColor(), numberToColor(attrib.getColor()).name().toLatin1().data());
    */

    RS_Pen pen(numberToColor(attrib.getColor()),
               numberToWidth(attrib.getWidth()),
               nameToLineType(attrib.getLineType().c_str()));
    return pen;
}



/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the dxflib documentation for details.
 *
 * @param num Color number.
 * @param comp Compatibility with older QCad versions (1.5.3 and older)
 */
RS_Color RS_FilterDXF::numberToColor(int num, bool comp) {
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
            return RS_Color((int)(dxfColors[num][0]*255),
                            (int)(dxfColors[num][1]*255),
                            (int)(dxfColors[num][2]*255));
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
int RS_FilterDXF::colorToNumber(const RS_Color& col) {

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
            int d = abs(col.red()-(int)(dxfColors[i][0]*255))
                    + abs(col.green()-(int)(dxfColors[i][1]*255))
                    + abs(col.blue()-(int)(dxfColors[i][2]*255));

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

void RS_FilterDXF::add3dFace(const DL_3dFaceData& data) {
    RS_DEBUG->print("RS_FilterDXF::add3dFace(const DL_3dFaceData& data) not yet implemented");
}
void RS_FilterDXF::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) {
    RS_DEBUG->print("RS_FilterDXF::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) not yet implemented");
}
void RS_FilterDXF::addComment(const char*) {
    RS_DEBUG->print("RS_FilterDXF::addComment(const char*) not yet implemented.");
}


/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType RS_FilterDXF::nameToLineType(const QString& name) {

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
QString RS_FilterDXF::lineTypeToName(RS2::LineType lineType) {

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
/*QString RS_FilterDXF::lineTypeToDescription(RS2::LineType lineType) {

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
RS2::LineWidth RS_FilterDXF::numberToWidth(int num) {
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
int RS_FilterDXF::widthToNumber(RS2::LineWidth width) {
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
QString RS_FilterDXF::toDxfString(const QString& string) {
    QString res = "";

    for (uint i=0; i<string.length(); ++i) {
        int c = string.at(i).unicode();
        switch (c) {
        case 0x0A:
            res+="\\P";
            break;
// RVT Space can stay space, verified with DraftSpace
//        case 0x20:
//            res+="\\~";
//            break;
            // diameter:
        case 0x2205:
            res+="%%c";
            break;
            // degree:
        case 0x00B0:
            res+="%%d";
            break;
            // plus/minus
        case 0x00B1:
            res+="%%p";
            break;
        default:
            if (c>127) {
                QString hex;
                hex = QString("%1").arg(c, 4, 16);

                hex = hex.replace(' ', '0');

                res+=QString("\\U+%1").arg(hex);
            } else {
                res+=string.at(i);
            }
            break;
        }
    }

    return res;
}



/**
 * Converts a DXF encoded string into a native Unicode string.
 */
QString RS_FilterDXF::toNativeString(const char* data, const QString& encoding) {
    QString res = QString(data);

    /*	- If the given string doesn't contain any unicode characters, we pass
     *	  the string through a textcoder.
     *	--------------------------------------------------------------------- */
    if (!res.contains("\\U+")) {
        QTextCodec *codec = QTextCodec::codecForName(encoding.toAscii());
        if (codec)
            res = codec->toUnicode(data);
    }

    // Line feed:
    res = res.replace(QRegExp("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegExp("\\\\~"), " ");
    // diameter:
    res = res.replace(QRegExp("%%c"), QChar(0x2205));
    // degree:
    res = res.replace(QRegExp("%%d"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegExp("%%p"), QChar(0x00B1));

    // Unicode characters:
    QString cap = "";
    int uCode = 0;
    bool ok = false;
    do {
        QRegExp regexp("\\\\U\\+[0-9A-Fa-f]{4,4}");
        regexp.indexIn(res);
        cap = regexp.cap();
        if (!cap.isNull()) {
            uCode = cap.right(4).toInt(&ok, 16);
            // workaround for Qt 3.0.x:
            res.replace(QRegExp("\\\\U\\+" + cap.right(4)), QChar(uCode));
            // for Qt 3.1:
            //res.replace(cap, QChar(uCode));
        }
    }
    while (!cap.isNull());

    // ASCII code:
    cap = "";
    uCode = 0;
    ok = false;
    do {
        QRegExp regexp("%%[0-9]{3,3}");
        regexp.indexIn(res);
        cap = regexp.cap();
        if (!cap.isNull()) {
            uCode = cap.right(3).toInt(&ok, 10);
            // workaround for Qt 3.0.x:
            res.replace(QRegExp("%%" + cap.right(3)), QChar(uCode));
            // for Qt 3.1:
            //res.replace(cap, QChar(uCode));
        }
    }
    while (!cap.isNull());

    // Ignore font tags:
    res = res.replace(QRegExp("\\\\f[0-9A-Za-z| ]{0,};"), "");

    // Ignore {}
    res = res.replace("\\{", "#curly#");
    res = res.replace("{", "");
    res = res.replace("#curly#", "{");

    res = res.replace("\\}", "#curly#");
    res = res.replace("}", "");
    res = res.replace("#curly#", "}");

    RS_DEBUG->print("RS_FilterDXF::toNativeString:");
    RS_DEBUG->printUnicode(res);
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
RS2::AngleFormat RS_FilterDXF::numberToAngleFormat(int num) {

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
int RS_FilterDXF::angleFormatToNumber(RS2::AngleFormat af) {

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
RS2::Unit RS_FilterDXF::numberToUnit(int num) {
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
int RS_FilterDXF::unitToNumber(RS2::Unit unit) {
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
bool RS_FilterDXF::isVariableTwoDimensional(const QString& var) {
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

