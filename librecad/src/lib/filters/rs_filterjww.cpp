/****************************************************************************
** $Id: rs_filterjww.cpp,v 1.1.1.2 2010/02/08 11:58:24 zeronemo2007 Exp $
**
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file is part of the qcadlib Library project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid qcadlib Professional Edition licenses may use
** this file in accordance with the qcadlib Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ribbonsoft.com for further details.
**
** Contact info@ribbonsoft.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include <QRegularExpression>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QByteArray>
#include <QStringConverter>
#else
#include <QTextCodec>
#endif

#include <QFile>
#include <QFileInfo>

#include "dl_attributes.h"
#include "dl_codes.h"
#include "dl_writer_ascii.h"
#include "lc_containertraverser.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_filterjww.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_system.h"
#include "rs_math.h"
#include "rs_debug.h"

namespace {
QString decodeWithCodePage(const std::string& text, const QString& encoding)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto qEncoding = QStringConverter::encodingForName(encoding.toLatin1());
    if (qEncoding) {
        QStringDecoder decoder{*qEncoding};
        return decoder(QByteArray::fromRawData(text.data(), qsizetype(text.size())));
    }
#else
    QTextCodec* codec = QTextCodec::codecForName(encoding.toLatin1());
    if (codec != nullptr) {
        return codec->toUnicode(text.c_str());
    }
#endif
    // The requested codec is unavailable. JWW is a Japanese format whose
    // file bytes are Shift-JIS — falling through to `fromStdString` (which
    // is `fromUtf8` on Qt 6) would mojibake any non-ASCII content. Try
    // Shift-JIS explicitly before giving up.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (auto sjis = QStringConverter::encodingForName("Shift-JIS")) {
        QStringDecoder dec{*sjis};
        return dec(QByteArray::fromRawData(text.data(), qsizetype(text.size())));
    }
#else
    if (QTextCodec* sjis = QTextCodec::codecForName("Shift-JIS")) {
        return sjis->toUnicode(text.c_str());
    }
#endif
    return QString::fromUtf8(text.data(), qsizetype(text.size()));
}
}

/**
 * Default constructor.
 *
 */
RS_FilterJWW::RS_FilterJWW()
                :RS_FilterInterface() {

        RS_DEBUG->print("RS_FilterJWW::RS_FilterJWW()");

        mtext = "";
		polyline = nullptr;
		leader = nullptr;
		hatch = nullptr;
		hatchLoop = nullptr;
		currentContainer = nullptr;
		graphic = nullptr;
		spline = nullptr;
		splinePoints = nullptr;
        //exportVersion = DL_Codes::VER_2002;
        //systemVariables.setAutoDelete(true);
        RS_DEBUG->print("RS_FilterJWW::RS_FilterJWW(): OK");
}

/**
 * Destructor.
 */
RS_FilterJWW::~RS_FilterJWW() {
        RS_DEBUG->print("RS_FilterJWW::~RS_FilterJWW()");
        RS_DEBUG->print("RS_FilterJWW::~RS_FilterJWW(): OK");
}



/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterJWW::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
        RS_DEBUG->print("RS_FilterJWW::fileImport");
        //RS_DEBUG->timestamp();

        RS_DEBUG->print("JWW Filter: importing file '%s'...", (const char*)QFile::encodeName(file));

        graphic = &g;
        currentContainer = graphic;
        this->file = file;

        RS_DEBUG->print("graphic->countLayers(): %d", graphic->countLayers());

        //graphic->setAutoUpdateBorders(false);
        RS_DEBUG->print("RS_FilterJWW::fileImport: reading file");
        bool success = jww.in((const char*)QFile::encodeName(file), this);
        RS_DEBUG->print("RS_FilterJWW::fileImport: reading file: OK");
        //graphic->setAutoUpdateBorders(true);

        if (success==false) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                                "Cannot open JWW file '%s'.", (const char*)QFile::encodeName(file));
                return false;
        }

        RS_DEBUG->print("RS_FilterJWW::fileImport: adding variables");

        // add some variables that need to be there for JWW drawings:
        if (graphic->getVariableString("$DIMSTYLE", "").isEmpty()) {
                RS_DEBUG->print("RS_FilterJWW::fileImport: adding DIMSTYLE");
                graphic->addVariable("$DIMSTYLE", "Standard", 2);
                RS_DEBUG->print("RS_FilterJWW::fileImport: adding DIMSTYLE: OK");
        }
        RS_DEBUG->print("RS_FilterJWW::fileImport: adding variables: OK");

        RS_DEBUG->print("RS_FilterJWW::fileImport: updating inserts");
        graphic->updateInserts();
        RS_DEBUG->print("RS_FilterJWW::fileImport: updating inserts: OK");

        RS_DEBUG->print("RS_FilterJWW::fileImport OK");
        //RS_DEBUG->timestamp();

        return true;
}



/**
 * Implementation of the method which handles layers.
 */
void RS_FilterJWW::addLayer(const DL_LayerData& data) {
        RS_DEBUG->print("RS_FilterJWW::addLayer");
        RS_DEBUG->print("  adding layer: %s", data.name.c_str());

        RS_DEBUG->print("RS_FilterJWW::addLayer: creating layer");
////////////////////2006/06/05
        RS_Layer* layer = new RS_Layer(toNativeString(data.name.c_str(),getDXFEncoding()));
////////////////////
        RS_DEBUG->print("RS_FilterJWW::addLayer: set pen");
        layer->setPen(attributesToPen(attributes));
        //layer->setFlags(data.flags&0x07);

        RS_DEBUG->print("RS_FilterJWW::addLayer: flags");
        if (data.flags&0x01) {
                layer->freeze(true);
        }
        if (data.flags&0x04) {
                layer->lock(true);
        }

        RS_DEBUG->print("RS_FilterJWW::addLayer: add layer to graphic");
        graphic->addLayer(layer);
        RS_DEBUG->print("RS_FilterJWW::addLayer: OK");
}



/**
 * Implementation of the method which handles blocks.
 *
 * @todo Adding blocks to blocks (stack for currentContainer)
 */
void RS_FilterJWW::addBlock(const DL_BlockData& data) {

        RS_DEBUG->print("RS_FilterJWW::addBlock");

        RS_DEBUG->print("  adding block: %s", data.name.c_str());


        // Prevent special blocks (paper_space, model_space) from being added:
        if (QString(data.name.c_str()).toLower()!="*paper_space0" &&
                        QString(data.name.c_str()).toLower()!="*paper_space" &&
                        QString(data.name.c_str()).toLower()!="*model_space" &&
                        QString(data.name.c_str()).toLower()!="$paper_space0" &&
                        QString(data.name.c_str()).toLower()!="$paper_space" &&
                        QString(data.name.c_str()).toLower()!="$model_space") {

#ifndef RS_NO_COMPLEX_ENTITIES
                if (QString(data.name.c_str()).startsWith("__CE")) {
                        RS_EntityContainer* ec = new RS_EntityContainer();
                        ec->setLayer("0");
                        currentContainer = ec;
                        graphic->addEntity(ec);
                        //currentContainer->setLayer(graphic->findLayer("0"));
                }
                else {
#endif
                        RS_Vector bp(data.bpx, data.bpy);
//////////////////////////////2006/06/05
                        QString enc = RS_System::getEncoding(
                                                                variables.getString("$DWGCODEPAGE", "ANSI_1252"));
                        // get the codec for Japanese
                        QString blName = decodeWithCodePage(data.name, enc);
//////////////////////////////
                        RS_Block* block =
                                new RS_Block(graphic,
                                                         RS_BlockData(blName, bp, false));
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
void RS_FilterJWW::endBlock() {
        currentContainer = graphic;
}



/**
 * Implementation of the method which handles point entities.
 */
void RS_FilterJWW::addPoint(const DL_PointData& data) {
        RS_Vector v(data.x, data.y);

        RS_Point* entity = new RS_Point(currentContainer,
                                                                        RS_PointData(v));
        setEntityAttributes(entity, attributes);

        currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterJWW::addLine(const DL_LineData& data) {

    if (nullptr == currentContainer) {
        RS_DEBUG->print("RS_FilterJWW::addLine: currentContainer is nullptr");
        return;
    }

        RS_DEBUG->print("RS_FilterJWW::addLine");

        RS_Vector v1(data.x1, data.y1);
        RS_Vector v2(data.x2, data.y2);

        RS_DEBUG->print("RS_FilterJWW::addLine: create line");

        RS_Line* entity = new RS_Line{currentContainer, {v1, v2}};
        RS_DEBUG->print("RS_FilterJWW::addLine: set attributes");
        setEntityAttributes(entity, attributes);

        RS_DEBUG->print("RS_FilterJWW::addLine: add entity");

        currentContainer->addEntity(entity);

        RS_DEBUG->print("RS_FilterJWW::addLine: OK");
}



/**
 * Implementation of the method which handles arc entities.
 *
 * @param angle1 Start angle in deg (!)
 * @param angle2 End angle in deg (!)
 */
void RS_FilterJWW::addArc(const DL_ArcData& data) {
        RS_DEBUG->print("RS_FilterJWW::addArc");
        //printf("LINE	 (%12.6f, %12.6f, %12.6f) (%12.6f, %12.6f, %12.6f)\n",
        //	   p1[0], p1[1], p1[2],
        //	   p2[0], p2[1], p2[2]);
        RS_Vector v(data.cx, data.cy);
        RS_ArcData d(v, data.radius,
								 RS_Math::deg2rad(data.angle1),
								 RS_Math::deg2rad(data.angle2),
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
void RS_FilterJWW::addEllipse(const DL_EllipseData& data) {
        RS_DEBUG->print("RS_FilterJWW::addEllipse");

		RS_Vector v1{data.cx, data.cy};
		RS_Vector v2{data.mx, data.my};

		RS_Ellipse* entity = new RS_Ellipse{currentContainer,
		{v1, v2,
				data.ratio,
				data.angle1, data.angle2,
				false}
};
        setEntityAttributes(entity, attributes);

        currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles circle entities.
 */
void RS_FilterJWW::addCircle(const DL_CircleData& data) {
        RS_DEBUG->print("RS_FilterJWW::addCircle");
        //printf("LINE	 (%12.6f, %12.6f, %12.6f) (%12.6f, %12.6f, %12.6f)\n",
        //	   p1[0], p1[1], p1[2],
        //	   p2[0], p2[1], p2[2]);

		RS_Circle* entity = new RS_Circle(currentContainer, {{data.cx, data.cy}, data.radius});
        setEntityAttributes(entity, attributes);

        currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles polyline entities.
 */
void RS_FilterJWW::addPolyline(const DL_PolylineData& data) {
        RS_DEBUG->print("RS_FilterJWW::addPolyline");
        //RS_DEBUG->print("RS_FilterJWW::addPolyline()");
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
void RS_FilterJWW::addVertex(const DL_VertexData& data) {
        RS_DEBUG->print("RS_FilterJWW::addVertex(): %f/%f bulge: %f",
                                        data.x, data.y, data.bulge);

        RS_Vector v(data.x, data.y);

        if (polyline) {
                polyline->addVertex(v, data.bulge);
        }
}



/**
 * Implementation of the method which handles splines.
 */
void RS_FilterJWW::addSpline(const DL_SplineData& data) {
        RS_DEBUG->print("RS_FilterJWW::addSpline: degree: %d", data.degree);

	if(data.degree == 2)
	{
		LC_SplinePointsData d(((data.flags&0x1)==0x1), true);
		splinePoints = new LC_SplinePoints(currentContainer, d);
		setEntityAttributes(splinePoints, attributes);
		currentContainer->addEntity(splinePoints);
		spline = nullptr;
		return;
	}

        if (data.degree>=1 && data.degree<=3) {
                RS_SplineData d(data.degree, ((data.flags&0x1)==0x1));
                spline = new RS_Spline(currentContainer, d);
                setEntityAttributes(spline, attributes);

                currentContainer->addEntity(spline);
				splinePoints = nullptr;
        } else {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterJWW::addSpline: Invalid degree for spline: %d. "
                        "Accepted values are 1..3.", data.degree);
        }
}


/**
 * Implementation of the method which handles spline control points.
 */
void RS_FilterJWW::addControlPoint(const DL_ControlPointData& data) {
        RS_DEBUG->print("RS_FilterJWW::addControlPoint: %f/%f", data.x, data.y);

        RS_Vector v(data.x, data.y);

        if (spline) {
                spline->addControlPoint(v);
                spline->update();
        }
        else if (splinePoints) {
                splinePoints->addControlPoint(v);
                splinePoints->update();
        }
}



/**
 * Implementation of the method which handles inserts.
 */
void RS_FilterJWW::addInsert(const DL_InsertData& data) {

        RS_DEBUG->print("RS_FilterJWW::addInsert");

        if (QString(data.name.c_str()).left(3)=="A$C") {
                return;
        }

        RS_Vector ip(data.ipx, data.ipy);
        RS_Vector sc(data.sx, data.sy);
        RS_Vector sp(data.colSp, data.rowSp);

        //cout << "Insert: " << name << " " << ip << " " << cols << "/" << rows << endl;

        RS_InsertData d(data.name.c_str(),
										ip, sc, RS_Math::deg2rad(data.angle),
                                        data.cols, data.rows,
                                        sp,
										nullptr,
                                        RS2::NoUpdate);
        RS_Insert* entity = new RS_Insert(currentContainer, d);
        setEntityAttributes(entity, attributes);
        RS_DEBUG->Log() << "  id: " << entity->getId();
        //entity->update();
        currentContainer->addEntity(entity);
}



/**
 * Implementation of the method which handles text
 * chunks for MText entities.
 */
/**
 * Implementation of the method which handles text
 * chunks for MText entities.
 */
void RS_FilterJWW::addMTextChunk(const char* text) {
    RS_DEBUG->print("RS_FilterJWW::addMTextChunk: %s", text);
    mtext+=text;
}

/*
 * get the encoding of the DXF files,
 * Acad versions >= 2007 are UTF-8, others in ANSI_1252
 */
QString RS_FilterJWW::getDXFEncoding() {

    QString acadver=variables.getString("$ACADVER", "");
    acadver.replace(QRegularExpression("[a-zA-Z]"), "");
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
 * Implementation of the method which handles
 * multi texts (MTEXT).
 */
void RS_FilterJWW::addMText(const DL_MTextData& data) {
        RS_DEBUG->print("RS_FilterJWW::addMText: %s", data.text.c_str());

        RS_Vector ip(data.ipx, data.ipy);
        RS_MTextData::VAlign valign;
        RS_MTextData::HAlign halign;
        RS_MTextData::MTextDrawingDirection dir;
        RS_MTextData::MTextLineSpacingStyle lss;
        QString sty = data.style.c_str();

        if (data.attachmentPoint<=3) {
                valign=RS_MTextData::VATop;
        } else if (data.attachmentPoint<=6) {
                valign=RS_MTextData::VAMiddle;
        } else {
                valign=RS_MTextData::VABottom;
        }

        if (data.attachmentPoint%3==1) {
                halign=RS_MTextData::HALeft;
        } else if (data.attachmentPoint%3==2) {
                halign=RS_MTextData::HACenter;
        } else {
                halign=RS_MTextData::HARight;
        }

        if (data.drawingDirection==1) {
                dir = RS_MTextData::LeftToRight;
        } else if (data.drawingDirection==3) {
                dir = RS_MTextData::TopToBottom;
        } else {
                dir = RS_MTextData::ByStyle;
        }

        if (data.lineSpacingStyle==1) {
                lss = RS_MTextData::AtLeast;
        } else {
                lss = RS_MTextData::Exact;
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
        }

        RS_DEBUG->print("Text as unicode:");
        RS_DEBUG->printUnicode(mtext);

        RS_MTextData d(ip, data.height, data.width,
                                  valign, halign,
                                  dir, lss,
                                  data.lineSpacingFactor,
                                  mtext, sty, data.angle,
                                  RS2::NoUpdate);
        RS_MText* entity = new RS_MText(currentContainer, d);

        setEntityAttributes(entity, attributes);
        entity->update();
        currentContainer->addEntity(entity);

        mtext = "";
}



/**
 * Implementation of the method which handles
 * texts (TEXT).
 */
void RS_FilterJWW::addText(const DL_TextData& data) {
        RS_DEBUG->print("RS_FilterJWW::addText");
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
                                 refPoint.z,
                                 data.height, width,
                                 attachmentPoint,
                                 drawingDirection,
                                 RS_MTextData::Exact,
                                 1.0,
                                 data.text.c_str(), data.style,
                                 angle));
}



/**
 * Implementation of the method which handles
 * dimensions (DIMENSION).
 */
RS_DimensionData RS_FilterJWW::convDimensionData(
        const DL_DimensionData& data) {

        RS_Vector defP(data.dpx, data.dpy);
        RS_Vector midP(data.mpx, data.mpy);
        RS_MTextData::VAlign valign;
        RS_MTextData::HAlign halign;
        RS_MTextData::MTextLineSpacingStyle lss;
        QString sty = data.style.c_str();
        QString t; //= data.text;

        // middlepoint of text can be 0/0 which is considered to be invalid (!):
        //  0/0 because older QCad versions save the middle of the text as 0/0
        //  although they didn't support saving of the middle of the text.
        if (fabs(data.mpx)<1.0e-6 && fabs(data.mpy)<1.0e-6) {
                midP = RS_Vector(false);
        }

        if (data.attachmentPoint<=3) {
                valign=RS_MTextData::VATop;
        } else if (data.attachmentPoint<=6) {
                valign=RS_MTextData::VAMiddle;
        } else {
                valign=RS_MTextData::VABottom;
        }

        if (data.attachmentPoint%3==1) {
                halign=RS_MTextData::HALeft;
        } else if (data.attachmentPoint%3==2) {
                halign=RS_MTextData::HACenter;
        } else {
                halign=RS_MTextData::HARight;
        }

        if (data.lineSpacingStyle==1) {
                lss = RS_MTextData::AtLeast;
        } else {
                lss = RS_MTextData::Exact;
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
                                                        t, sty, data.angle, 0.0, true, nullptr, false, false);
}



/**
 * Implementation of the method which handles
 * aligned dimensions (DIMENSION).
 */
void RS_FilterJWW::addDimAlign(const DL_DimensionData& data,
                                                           const DL_DimAlignedData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimAligned");

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
void RS_FilterJWW::addDimLinear(const DL_DimensionData& data,
                                                                const DL_DimLinearData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimLinear");

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
void RS_FilterJWW::addDimRadial(const DL_DimensionData& data,
                                                                const DL_DimRadialData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimRadial");

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
void RS_FilterJWW::addDimDiametric(const DL_DimensionData& data,
                                                                   const DL_DimDiametricData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimDiametric");

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
void RS_FilterJWW::addDimAngular(const DL_DimensionData& data,
                                                                 const DL_DimAngularData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimAngular");

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
void RS_FilterJWW::addDimAngular3P(const DL_DimensionData& data,
                                                                   const DL_DimAngular3PData& edata) {
        RS_DEBUG->print("RS_FilterJWW::addDimAngular3P");

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
void RS_FilterJWW::addLeader(const DL_LeaderData& data) {
        RS_DEBUG->print("RS_FilterJWW::addDimLeader");
        //RS_DEBUG->print("RS_FilterJWW::addPolyline()");
        RS_LeaderData d(data.arrowHeadFlag==1, "");
        leader = new RS_Leader(currentContainer, d);
        setEntityAttributes(leader, attributes);

        currentContainer->addEntity(leader);
}



/**
 * Implementation of the method which handles leader vertices.
 */
void RS_FilterJWW::addLeaderVertex(const DL_LeaderVertexData& data) {
        RS_DEBUG->print("RS_FilterJWW::addLeaderVertex");
        //RS_DEBUG->print("RS_FilterJWW::addVertex() bulge: %f", bulge);

        RS_Vector v(data.x, data.y);

        if (leader) {
                leader->addVertex(v);
        }
}



/**
 * Implementation of the method which handles hatch entities.
 */
void RS_FilterJWW::addHatch(const DL_HatchData& data) {
        RS_DEBUG->print("RS_FilterJWW::addHatch()");

        hatch = new RS_Hatch(currentContainer,
                                                 RS_HatchData(data.solid,
                                                                          data.scale,
                                                                          data.angle,
                                                                          QString(data.pattern.c_str())));
        setEntityAttributes(hatch, attributes);

        currentContainer->addEntity(hatch);
}



/**
 * Implementation of the method which handles hatch loops.
 */
void RS_FilterJWW::addHatchLoop(const DL_HatchLoopData& /*data*/) {
        RS_DEBUG->print("RS_FilterJWW::addHatchLoop()");
        if (hatch) {
                hatchLoop = new RS_EntityContainer(hatch);
				hatchLoop->setLayer(nullptr);
                hatch->addEntity(hatchLoop);
        }
}



/**
 * Implementation of the method which handles hatch edge entities.
 */
void RS_FilterJWW::addHatchEdge(const DL_HatchEdgeData& data) {
        RS_DEBUG->print("RS_FilterJWW::addHatchEdge()");

        if (hatchLoop) {
				RS_Entity* e = nullptr;
                switch (data.type) {
                case 1:
                        RS_DEBUG->print("RS_FilterJWW::addHatchEdge(): "
                                                        "line: %f,%f %f,%f",
                                                        data.x1, data.y1, data.x2, data.y2);
						e = new RS_Line{hatchLoop, {{data.x1, data.y1},
						{data.x2, data.y2}}};
                        break;
                case 2:
                        if (data.ccw && data.angle1<1.0e-6 && data.angle2>2*M_PI-1.0e-6) {
                                e = new RS_Circle(hatchLoop,
								{{data.cx, data.cy}, data.radius});
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

                if (e) {
						e->setLayer(nullptr);
                        hatchLoop->addEntity(e);
                }
        }
}



/**
 * Implementation of the method which handles image entities.
 */
void RS_FilterJWW::addImage(const DL_ImageData& data) {
        RS_DEBUG->print("RS_FilterJWW::addImage");

        RS_Vector ip(data.ipx, data.ipy);
        RS_Vector uv(data.ux, data.uy);
        RS_Vector vv(data.vx, data.vy);
        RS_Vector size(data.width, data.height);

        RS_Image* image =
                new RS_Image(
                        currentContainer,
						RS_ImageData(QString(data.ref.c_str()).toInt(nullptr, 16),
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
void RS_FilterJWW::linkImage(const DL_ImageDefData& data) {
        RS_DEBUG->print("RS_FilterJWW::linkImage");

		int handle = QString(data.ref.c_str()).toInt(nullptr, 16);
        QString sfile(data.file.c_str());
        QFileInfo fiDxf(file);
        QFileInfo fiBitmap(sfile);

        // try to find the image file:

        // first: absolute path:
        if (!fiBitmap.exists()) {
                RS_DEBUG->print("File %s doesn't exist.",
                                                (const char*)QFile::encodeName(sfile));
                // try relative path:
                QString f1 = fiDxf.path() + "/" + sfile;
                if (QFileInfo(f1).exists()) {
                        sfile = f1;
                } else {
                        RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f1));
                        // try drawing path:
                        QString f2 = fiDxf.path() + "/" + fiBitmap.fileName();
                        if (QFileInfo(f2).exists()) {
                                sfile = f2;
                        } else {
                                RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f2));
                        }
                }
        }

        // Also link images in subcontainers (e.g. inserts):
        for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
                        e; e=graphic->nextEntity(RS2::ResolveNone)) {
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
        for (unsigned i=0; i<graphic->countBlocks(); ++i) {
                RS_Block* b = graphic->blockAt(i);
                for (RS_Entity* e=b->firstEntity(RS2::ResolveNone);
                                e; e=b->nextEntity(RS2::ResolveNone)) {
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
void RS_FilterJWW::endEntity() {
        RS_DEBUG->print("RS_FilterJWW::endEntity");

        if (hatch) {

                RS_DEBUG->print("hatch->update()");

                if (hatch->validate()) {
                        hatch->update();
                } else {
                        graphic->removeEntity(hatch);
                        RS_DEBUG->print(RS_Debug::D_ERROR,
                                                        "RS_FilterJWW::endEntity(): updating hatch failed: invalid hatch area");
                }
				hatch=nullptr;
        }
}

void RS_FilterJWW::add3dFace(const DL_3dFaceData& /*data*/) {
    RS_DEBUG->print("RS_FilterDXF::add3dFace(const DL_3dFaceData& data) not yet implemented");
}
void RS_FilterJWW::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) {
    RS_DEBUG->print("RS_FilterDXF::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) not yet implemented");
}
void RS_FilterJWW::addComment(const char*) {
    RS_DEBUG->print("RS_FilterDXF::addComment(const char*) not yet implemented.");
}

/**
 * Sets a vector variable from the JWW file.
 */
void RS_FilterJWW::setVariableVector(const char* key,
                                                                         double v1, double v2, double v3, int code) {
        RS_DEBUG->print("RS_FilterJWW::setVariableVector");

        // update document's variable list:
        if (currentContainer->rtti()==RS2::EntityGraphic) {
                ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                                RS_Vector(v1, v2, v3), code);
        }
}



/**
 * Sets a string variable from the JWW file.
 */
void RS_FilterJWW::setVariableString(const char* key,
                                                                         const char* value, int code) {
        RS_DEBUG->print("RS_FilterJWW::setVariableString");

        // update local JWW variable list:
        variables.add(QString(key), QString(value), code);

        // update document's variable list:
        if (currentContainer->rtti()==RS2::EntityGraphic) {
                ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                                QString(value), code);
        }
}



/**
 * Sets an int variable from the JWW file.
 */
void RS_FilterJWW::setVariableInt(const char* key, int value, int code) {
        RS_DEBUG->print("RS_FilterJWW::setVariableInt");

        // update document's variable list:
        if (currentContainer->rtti()==RS2::EntityGraphic) {
                ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                                value, code);
        }
}



/**
 * Sets a double variable from the JWW file.
 */
void RS_FilterJWW::setVariableDouble(const char* key, double value, int code) {
        RS_DEBUG->print("RS_FilterJWW::setVariableDouble");

        // update document's variable list:
        if (currentContainer->rtti()==RS2::EntityGraphic) {
                ((RS_Graphic*)currentContainer)->addVariable(QString(key),
                                value, code);
        }

}


/**
 * Sets the entities attributes according to the attributes
 * that come from a JWW file.
 */
void RS_FilterJWW::setEntityAttributes(RS_Entity* entity,
                                                                           const DL_Attributes& attrib) {
        RS_DEBUG->print("RS_FilterJWW::setEntityAttributes");

        RS_Pen pen;
        pen.setColor(Qt::black);
        pen.setLineType(RS2::SolidLine);

        // Layer:
        if (attrib.getLayer().empty()) {
                entity->setLayer("0");
        } else {
//-------------------------
                //2007-02-24 added
                QString enc = RS_System::getEncoding(
                                                        variables.getString("$DWGCODEPAGE", "ANSI_1252"));
                // get the codec for Japanese
                QString lName = decodeWithCodePage(attrib.getLayer(), enc);
				if (!graphic->findLayer(lName)) {
                        addLayer(DL_LayerData(attrib.getLayer(), 0));
                }
                entity->setLayer(lName);
//-------------------------
                // add layer in case it doesn't exist:
/*		if (graphic->findLayer(attrib.getLayer().c_str())==nullptr) {
                        addLayer(DL_LayerData(attrib.getLayer(), 0));
                }
                entity->setLayer(attrib.getLayer().c_str());
*/
        }

        // Color:
        pen.setColor(numberToColor(attrib.getColor()));

        // Linetype:
        pen.setLineType(nameToLineType(attrib.getLineType().c_str()));

        // Width:
        pen.setWidth(numberToWidth(attrib.getWidth()));

        entity->setPen(pen);
        RS_DEBUG->print("RS_FilterJWW::setEntityAttributes: OK");
}



/**
 * @return Pen with the same attributes as 'attrib'.
 */
RS_Pen RS_FilterJWW::attributesToPen(const DL_Attributes& attrib) const {

        /*
        printf("converting Color %d to %s\n",
           attrib.getColor(), numberToColor(attrib.getColor()).name().toLatin1().constData());
        */

        RS_Pen pen(numberToColor(attrib.getColor()),
                           numberToWidth(attrib.getWidth()),
                           nameToLineType(attrib.getLineType().c_str()));
        return pen;
}



/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the jwwlib documentation for details.
 *
 * @param num Color number.
 * @param comp Compatibility with older QCad versions (1.5.3 and older)
 */
RS_Color RS_FilterJWW::numberToColor(int num, bool comp) {
        // Compatibility with QCad 1.5.3 and older:
        if (comp) {
                switch(num) {
                case 0:
                        return QColor(Qt::black);
                        break;
                case 1:
                        return QColor(Qt::darkBlue);
                        break;
                case 2:
                        return QColor(Qt::darkGreen);
                        break;
                case 3:
                        return QColor(Qt::darkCyan);
                        break;
                case 4:
                        return QColor(Qt::darkRed);
                        break;
                case 5:
                        return QColor(Qt::darkMagenta);
                        break;
                case 6:
                        return QColor(Qt::darkYellow);
                        break;
                case 7:
                        return QColor(Qt::lightGray);
                        break;
                case 8:
                        return QColor(Qt::darkGray);
                        break;
                case 9:
                        return QColor(Qt::blue);
                        break;
                case 10:
                        return QColor(Qt::green);
                        break;
                case 11:
                        return QColor(Qt::cyan);
                        break;
                case 12:
                        return QColor(Qt::red);
                        break;
                case 13:
                        return QColor(Qt::magenta);
                        break;
                case 14:
                        return QColor(Qt::yellow);
                        break;
                case 15:
                        return QColor(Qt::black);
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
                                "RS_FilterJWW::numberToColor: Invalid color number given.");
                        return RS_Color(RS2::FlagByLayer);
                }
        }
        return RS_Color();
}



/**
 * Converts a color into a color number in the JWW palette.
 * The color that fits best is chosen.
 */
int RS_FilterJWW::colorToNumber(const RS_Color& col) {

        //printf("Searching color for %s\n", col.name().toLatin1().constData());

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
                                           jwwColors[i][0],
                                           jwwColors[i][1],
                                           jwwColors[i][2]);
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



/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType RS_FilterJWW::nameToLineType(const QString& name) {

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
                           uName=="DASHED") {
                return RS2::DashLine;

        } else if (uName=="DASHED2") {
                return RS2::DashLine2;

        } else if (uName=="DASHEDX2") {
                return RS2::DashLineX2;

        } else if (uName=="HIDDEN") {
                return RS2::HiddenLine;

        } else if (uName=="HIDDEN2") {
                return RS2::HiddenLine2;

        } else if (uName=="HIDDENX2") {
                return RS2::HiddenLineX2;


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

        } else if (uName=="ACAD_ISO09W100" || uName=="PHANTOM") {
                return RS2::PhantomLine;

        } else if (uName=="PHANTOM2") {
                return RS2::PhantomLine2;

        } else if (uName=="PHANTOMX2") {
                return RS2::PhantomLineX2;


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
QString RS_FilterJWW::lineTypeToName(RS2::LineType lineType) {

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

        case RS2::HiddenLine:
                return "HIDDEN";
                break;
        case RS2::HiddenLine2:
                return "HIDDEN2";
                break;
        case RS2::HiddenLineX2:
                return "HIDDENX2";
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

        case RS2::PhantomLine:
                return "PHANTOM";
                break;
        case RS2::PhantomLine2:
                return "PHANTOM2";
                break;
        case RS2::PhantomLineX2:
                return "PHANTOMX2";
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
/*QString RS_FilterJWW::lineTypeToDescription(RS2::LineType lineType) {

        // Standard linetypes for QCad II / AutoCAD
        switch (lineType) {
        case RS2::SolidLine:
                return "Solid line";
        case RS2::DotLine:
                return "ISO Dashed __ __ __ __ __ __ __ __ __ __ _";
        case RS2::DashLine:
                return "ISO Dashed with Distance __	__	__	_";
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
RS2::LineWidth RS_FilterJWW::numberToWidth(int num) {
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
int RS_FilterJWW::widthToNumber(RS2::LineWidth width) {
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
 * Converts a native unicode string into a JWW encoded string.
 *
 * JWW endoding includes the following special sequences:
 * - %%%c for a diameter sign
 * - %%%d for a degree sign
 * - %%%p for a plus/minus sign
 */
QString RS_FilterJWW::toDxfString(const QString& string) {
        /*
           QString res = string;
           // Line feed:
           res = res.replace(RS_RegExp("\\n"), "\\P");
           // Space:
           res = res.replace(RS_RegExp(" "), "\\~");
           // diameter:
           res = res.replace(QChar(0x2205), "%%c");
           // degree:
           res = res.replace(QChar(0x00B0), "%%d");
           // plus/minus
           res = res.replace(QChar(0x00B1), "%%p");
        */

        QString res = "";

        for (int i=0; i<string.length(); ++i) {
                int c = string.at(i).unicode();
                switch (c) {
                case 0x0A:
                        res+="\\P";
                        break;
                case 0x20:
                        res+="\\~";
                        break;
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
QString RS_FilterJWW::toNativeString(const char* data, const QString& encoding) {
    QString res = QString(data);

    /*	- If the given string doesn't contain any unicode characters, we pass
     *	  the string through a textcoder.
     *	--------------------------------------------------------------------- */
    if (!res.contains("\\U+")) {
        res = decodeWithCodePage(data, encoding);
    }

    // Line feed:
    res = res.replace(QRegularExpression("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegularExpression("\\\\~"), " ");
    // diameter:
    res = res.replace(QRegularExpression("%%c"), QChar(0x2205));
    // degree:
    res = res.replace(QRegularExpression("%%d"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegularExpression("%%p"), QChar(0x00B1));

    // Unicode characters:
    QString cap = "";
    int uCode = 0;
    bool ok = false;
    do {
        QRegularExpression regexp("\\\\U\\+[0-9A-Fa-f]{4,4}");
        QRegularExpressionMatch match=regexp.match(res);
        if (match.hasMatch()) {
            uCode = match.captured(0).right(4).toInt(&ok, 16);
            // workaround for Qt 3.0.x:
            res.replace(QRegularExpression("\\\\U\\+" + cap.right(4)), QChar(uCode));
            // for Qt 3.1:
            //res.replace(cap, QChar(uCode));
        }
    }
    while (!cap.isNull());

    // ASCII code:
    cap = "";
//    uCode = 0;
    ok = false;
    do {
        QRegularExpression regexp("%%[0-9]{3,3}");
        QRegularExpressionMatch match = regexp.match(res);
        if (match.hasMatch()) {
            uCode = match.captured(0).right(3).toInt(&ok, 10);
            // workaround for Qt 3.0.x:
            res.replace(QRegularExpression("%%" + cap.right(3)), QChar(uCode));
            // for Qt 3.1:
            //res.replace(cap, QChar(uCode));
        }
    }
    while (!cap.isNull());

    // Ignore font tags:
    res = res.replace(QRegularExpression("\\\\f[0-9A-Za-z| ]{0,};"), "");

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
 * Converts the given number from a JWW file into an AngleFormat enum.
 *
 * @param num $DIMAUNIT from JWW (0: decimal deg, 1: deg/min/sec, 2: gradians,
 *								3: radians, 4: surveyor's units)
 *
 * @ret Matching AngleFormat enum value.
 */
RS2::AngleFormat RS_FilterJWW::numberToAngleFormat(int num) {

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
 * Converts AngleFormat enum to JWW number.
 */
int RS_FilterJWW::angleFormatToNumber(RS2::AngleFormat af) {

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
 * converts a JWW unit setting (e.g. INSUNITS) to a unit enum.
 */
RS2::Unit RS_FilterJWW::numberToUnit(int num) {
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
 * Converts a unit enum into a JWW unit number e.g. for INSUNITS.
 */
int RS_FilterJWW::unitToNumber(RS2::Unit unit) {
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
bool RS_FilterJWW::isVariableTwoDimensional(const QString& var) {
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
