/****************************************************************************
** $Id: dl_jww.cpp,v 1.1.1.2 2010/02/08 11:58:29 zeronemo2007 Exp $
**
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file is part of the dxflib project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid dxflib Professional Edition licenses may use 
** this file in accordance with the dxflib Commercial License
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

#include "dl_jww.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <cassert>
#include <cmath>


//#include "dl_attributes.h"
//#include "dl_codes.h"
#include "dl_creationinterface.h"
//#include "dl_writer_ascii.h"

//#undef DEBUG
#define SKIP_MOJI
#ifdef DEBUG
#include "rs_debug.h"
#endif

#define  ArraySize(arr)  (sizeof(arr)/sizeof(arr[0]))

/*
#ifdef	_WINDOWS
#include "iconv/iconv.h"
#else
#include "utf8/utf8cv.h"
#include "UnicodeF.h"
#endif
*/

//static const char* UTF8_CES = "UTF-8";
//static const char* SHIFTJIS_CES = "SJIS";
//static const char* EUCJP_CES = "EUC-JP";

/*        black = 250,
        green = 3,
        red = 1,
        brown = 15,
        yellow = 2,
        cyan = 4,
        magenta = 6,
        gray = 8,
        blue = 5,
        l_blue = 163,
        l_green = 121,
        l_cyan = 131,
        l_red = 23,
        l_magenta = 221,
        l_gray = 252,
        white = 7,
        bylayer = 256,
        byblock = 0
*/
static	int	colTable[] = {
	250,	//RS_Color(0x00, 0x00, 0x00)
	4,		//RS_Color(0x00, 0xC0, 0xC0)
	256,	//RS_Color(0x00, 0x00, 0x00)
	3,		//RS_Color(0x00, 0xC0, 0x00)
	2,		//RS_Color(0xC0, 0xC0, 0x00)
	6,		//RS_Color(0xC0, 0x00, 0xC0)
	5,		//RS_Color(0x00, 0x00, 0xFF)
	131,	//RS_Color(0x00, 0x80, 0x80)
	221,	//RS_Color(0xFF, 0x00, 0x80)
	8,		//RS_Color(0xC0, 0xC0, 0xC0)
	23,		//RS_Color(0xFF, 0x80, 0xFF)
	6,		//RS_Color(0xFF, 0, 0xFF)
	1,		//RS_Color(0xFF, 0, 0)
	7,		//RS_Color(0xFF, 0xFF, 0xFF)
	252,	//RS_Color(0x80, 0x80, 0x80)
	256		//RS_Color(0, 0, 0)
};

static	string	lTable[] = {
	"CONTINUOUS", //RS2::SolidLine
	"CONTINUOUS", //RS2::SolidLine
	"CONTINUOUS", //RS2::SolidLine
//	"DOT", //RS2::DotLine
//	"DOT2", //RS2::DotLine2
//	"DOTX2", //RS2::DotLineX2
	"DASHED", //RS2::DashLine
	"DASHED2", //RS2::DashLine2
	"DASHEDX2", //RS2::DashLineX2
//	"DASHDOT", //RS2::DashDotLine
//	"DASHDOT2", //RS2::DashDotLine2
//	"DASHDOTX2", //RS2::DashDotLineX2
//	"DIVIDE", //RS2::DivideLine
//	"DIVIDE2", //RS2::DivideLine2
//	"DIVIDEX2", //RS2::DivideLineX2
	"CENTER", //RS2::CenterLine
	"CENTER2", //RS2::CenterLine2
//	"CENTERX2", //RS2::CenterLineX2
	"BORDER", //RS2::BorderLine
	"BORDER2", //RS2::BorderLine2
	"BORDERX2", //RS2::BorderLineX2
	"ByLayer", //RS2::LineByLayer
	"ByBlock",//RS2::LineByBlock
};

static	string HEX[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

static double Deg(double ang)
{
	return ang / M_PI * 180.0;
}

/**
 * Default constructor.
 */
DL_Jww::DL_Jww() {
}



/**
 * Destructor.
 */
DL_Jww::~DL_Jww() {
}

void DL_Jww::CreateSen(DL_CreationInterface* creationInterface, CDataSen& DSen)
{
	string lName = HEX[DSen.m_nGLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DSen.m_nGLayer] + "-" +
													HEX[DSen.m_nLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DSen.m_nLayer];
	// add layer
	creationInterface->addLayer(DL_LayerData(lName,0));
//#ifdef	DEBUG
if(DSen.m_nPenStyle > ArraySize(lTable)-1)
	std::cout << "線種番号 " << (WORD)DSen.m_nPenStyle << std::endl;   //線種番号
if(DSen.m_nPenColor > ArraySize(colTable)-1)
	std::cout << "線色番号 " << (WORD)DSen.m_nPenColor << std::endl;   //線色番号
if(DSen.m_nPenWidth > 26)
	std::cout << "線色幅 " << (WORD)DSen.m_nPenWidth << std::endl;//線色幅
//#endif
	int width;
	if(DSen.m_nPenWidth > 26)
		width = 0;
	else
		width = DSen.m_nPenWidth;
	int color = colTable[DSen.m_nPenColor > ArraySize(colTable)-1 ? ArraySize(colTable)-1 : DSen.m_nPenColor];
	attrib = DL_Attributes(values[8],	  // layer
			       color,	      // color
			       width,	      // width
			       lTable[DSen.m_nPenStyle > ArraySize(lTable)-1 ? ArraySize(lTable)-1 : DSen.m_nPenStyle]);	  // linetype
	creationInterface->setAttributes(attrib);

	creationInterface->setExtrusion(0.0, 0.0, 1.0, 0.0 );
	// correct some impossible attributes for layers:
/*
	attrib = creationInterface->getAttributes();
	if (attrib.getColor()==256 || attrib.getColor()==0) {
		attrib.setColor(7);
	}
	if (attrib.getWidth()<0) {
		attrib.setWidth(1);
	}
	if (!strcasecmp(attrib.getLineType().c_str(), "BYLAYER") ||
			!strcasecmp(attrib.getLineType().c_str(), "BYBLOCK")) {
		attrib.setLineType("CONTINUOUS");
	}
*/
	DL_LineData d(DSen.m_start.x,
				DSen.m_start.y,
				0.0,
				DSen.m_end.x,
				DSen.m_end.y,
				0.0);

	creationInterface->addLine(d);

#ifdef FINISHED
	RS_LineData data(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0));
	RS_Line* line;

	data.startpoint = RS_Vector(DSen.m_start.x, DSen.m_start.y);
	data.endpoint = RS_Vector(DSen.m_end.x, DSen.m_end.y);
	line = new RS_Line(graphic, data);
	RS2::LineType ltype = lTable[DSen.m_nPenStyle];
	RS_Color col = colTable[DSen.m_nPenColor];
	RS2::LineWidth lw = lWidth[DSen.m_nPenWidth>26 ? 0 : DSen.m_nPenWidth];
	line->setPen(RS_Pen(col, lw, ltype));
			//画層設定
//画層
// m_nGLayer-m_nLayer
//_0-0_ から_0-F_
//	 ...
//_F-0_ から_F-F_
	RS_String lName = HEX[DSen.m_nGLayer > 0x0f ? 0: DSen.m_nGLayer] + "-" +
		HEX[DSen.m_nLayer > 0x0f ? 0: DSen.m_nLayer];
	if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
#ifdef DEBUG
cout << jwdoc->vSen[i].m_nGLayer << " " << jwdoc->vSen[i].m_nLayer << endl;
std::cout << lName.ascii() << std::endl;
#endif
		RS_Layer* layer = new RS_Layer(lName);
		graphic->addLayer(layer);
	}
	line->setLayer(lName);
#ifdef	DEBUG
std::cout << "線種番号 " << (WORD)DSen.m_nPenStyle << std::endl;   //線種番号
std::cout << "線色番号 " << (WORD)DSen.m_nPenColor << std::endl;   //線色番号
std::cout << "線色幅 " << (WORD)DSen.m_nPenWidth << std::endl;//線色幅
#endif

	// add the line to the graphic
	graphic->addEntity(line);
	std::cout << *line;
#endif
}

void DL_Jww::CreateEnko(DL_CreationInterface* creationInterface, CDataEnko& DEnko)
{
	string lName = HEX[DEnko.m_nGLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DEnko.m_nGLayer] + "-" +
													HEX[DEnko.m_nLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DEnko.m_nLayer];

	// add layer
	creationInterface->addLayer(DL_LayerData(lName,0));

	int width;
	if(DEnko.m_nPenWidth > 26)
		width = 0;
	else
		width = DEnko.m_nPenWidth;
	int color = colTable[DEnko.m_nPenColor > ArraySize(colTable)-1 ? ArraySize(colTable)-1 : DEnko.m_nPenColor];
	attrib = DL_Attributes(values[8],	  // layer
			       color,	      // color
			       width,	      // width
			       lTable[DEnko.m_nPenStyle > ArraySize(lTable)-1 ? ArraySize(lTable)-1 : DEnko.m_nPenStyle]);	  // linetype
	creationInterface->setAttributes(attrib);

	creationInterface->setExtrusion(0.0, 0.0, 1.0, 0.0 );

	double angle1, angle2;
	//正円
	if(DEnko.m_bZenEnFlg){
		if(DEnko.m_dHenpeiRitsu == 1.0){
			DL_CircleData d(DEnko.m_start.x, DEnko.m_start.y, 0.0, DEnko.m_dHankei);
			creationInterface->addCircle(d);
		}else{
			double angle1, angle2;
			if(DEnko.m_radEnkoKaku > 0.0){
				angle1 = DEnko.m_radKaishiKaku;
				angle2 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
			}else{
				angle1 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
				angle2 = DEnko.m_radKaishiKaku;
			}
			angle1 = angle1 - floor(angle1 / (M_PI * 2.0)) * M_PI * 2.0;
			angle2 = angle2 - floor(angle2 / (M_PI * 2.0)) * M_PI * 2.0;
			if( angle2 <= angle1 )
				angle1 = angle1 - M_PI * 2.0;
			//楕円
			DL_EllipseData d(DEnko.m_start.x, DEnko.m_start.y, 0.0,
							DEnko.m_dHankei * cos(DEnko.m_radKatamukiKaku), DEnko.m_dHankei * sin(DEnko.m_radKatamukiKaku), 0.0,
							DEnko.m_dHenpeiRitsu,
							angle1, angle2);

			creationInterface->addEllipse(d);
		}
	}else{
		if(DEnko.m_dHenpeiRitsu == 1.0){
			//円弧
			if(DEnko.m_radEnkoKaku > 0.0){
				angle1 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku;
				angle2 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku + DEnko.m_radEnkoKaku;
			}else{
				angle1 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku + DEnko.m_radEnkoKaku;
				angle2 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku;
			}
			angle1 = angle1 - floor(angle1 / (M_PI * 2.0)) * M_PI * 2.0;
			angle2 = angle2 - floor(angle2 / (M_PI * 2.0)) * M_PI * 2.0;
			if( angle2 <= angle1 )
				angle1 = angle1 - M_PI * 2.0;
			DL_ArcData d(DEnko.m_start.x, DEnko.m_start.y, 0.0,
					DEnko.m_dHankei,
					Deg(angle1),
					Deg(angle2));

			creationInterface->addArc(d);
		}else{
			double angle1, angle2;
			if(DEnko.m_radEnkoKaku > 0.0){
				angle1 = DEnko.m_radKaishiKaku;
				angle2 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
			}else{
				angle1 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
				angle2 = DEnko.m_radKaishiKaku;
			}
			angle1 = angle1 - floor(angle1 / (M_PI * 2.0)) * M_PI * 2.0;
			angle2 = angle2 - floor(angle2 / (M_PI * 2.0)) * M_PI * 2.0;
			if( angle2 <= angle1 )
				angle1 = angle1 - M_PI * 2.0;
			//楕円
			DL_EllipseData d(DEnko.m_start.x, DEnko.m_start.y, 0.0,
							DEnko.m_dHankei * cos(DEnko.m_radKatamukiKaku), DEnko.m_dHankei * sin(DEnko.m_radKatamukiKaku), 0.0,
							DEnko.m_dHenpeiRitsu,
							angle1, angle2);

			creationInterface->addEllipse(d);
		}
	}
#ifdef FINISHED
	RS_CircleData data1(RS_Vector(0.0, 0.0), 0.0);
	RS_Circle* circle;
	RS_ArcData arc_data(RS_Vector(0.0, 0.0), 0.0, 0.0, 0.0, false);
	RS_Arc* arc;
	RS_Ellipse* elps;
	//正円
	if(DEnko.m_bZenEnFlg){
		if(DEnko.m_dHenpeiRitsu == 1.0){
			data1.center = RS_Vector(DEnko.m_start.x, DEnko.m_start.y);
			data1.radius = DEnko.m_dHankei;
			circle = new RS_Circle(graphic, data1);
			RS2::LineType ltype = lTable[DEnko.m_nPenStyle];
			RS_Color col = colTable[DEnko.m_nPenColor];
			RS2::LineWidth lw = lWidth[DEnko.m_nPenWidth > 26 ? 0 : DEnko.m_nPenWidth];//RS2::Width12
			circle->setPen(RS_Pen(col, lw, ltype));

			RS_String lName = HEX[DEnko.m_nGLayer > 0x0f ? 0:DEnko.m_nGLayer] + "-" +
				HEX[DEnko.m_nLayer > 0x0f ? 0: DEnko.m_nLayer];
			if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
#ifdef DEBUG
std::cout << lName.ascii() << std::endl;
#endif
				RS_Layer* layer = new RS_Layer(lName);
				graphic->addLayer(layer);
			}
			circle->setLayer(lName);
			// add the line to the graphic
			graphic->addEntity(circle);
#ifdef DEBUG
std::cout << *circle;
#endif
		}else{
			//楕円
			double angle1, angle2;
			if(DEnko.m_radEnkoKaku > 0.0){
				angle1 = DEnko.m_radKaishiKaku;
				angle2 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
			}else{
				angle1 = DEnko.m_radKaishiKaku + DEnko.m_radEnkoKaku;
				angle2 = DEnko.m_radKaishiKaku;
			}
			angle1 = angle1 - floor(angle1 / (M_PI * 2.0)) * M_PI * 2.0;
			angle2 = angle2 - floor(angle2 / (M_PI * 2.0)) * M_PI * 2.0;
			if( angle2 <= angle1 )
				angle1 = angle1 - M_PI * 2.0;
			RS_EllipseData elps_data(RS_Vector(DEnko.m_start.x, DEnko.m_start.y),
									RS_Vector(DEnko.m_dHankei * cos(DEnko.m_radKatamukiKaku), DEnko.m_dHankei * sin(DEnko.m_radKatamukiKaku)),
									DEnko.m_dHenpeiRitsu,
									angle1, angle2, false);
			elps = new RS_Ellipse(graphic, elps_data);
			RS2::LineType ltype = lTable[DEnko.m_nPenStyle];
			RS_Color col = colTable[DEnko.m_nPenColor];
			RS2::LineWidth lw = lWidth[DEnko.m_nPenWidth > 26 ? 0 : DEnko.m_nPenWidth];//RS2::Width12
			elps->setPen(RS_Pen(col, lw, ltype));

			RS_String lName = HEX[DEnko.m_nGLayer > 0x0f ? 0:DEnko.m_nGLayer] + "-" +
				HEX[DEnko.m_nLayer > 0x0f ? 0: DEnko.m_nLayer];
			if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
				RS_Layer* layer = new RS_Layer(lName);
				graphic->addLayer(layer);
			}
			elps->setLayer(lName);
			// add the line to the graphic
			graphic->addEntity(elps);
		}
	}else{
		//円弧
		arc_data.center = RS_Vector(DEnko.m_start.x, DEnko.m_start.y);
		arc_data.radius = DEnko.m_dHankei;
		if(DEnko.m_radEnkoKaku > 0.0){
			arc_data.angle1 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku;
			arc_data.angle2 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku + DEnko.m_radEnkoKaku;
		}else{
			arc_data.angle1 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku + DEnko.m_radEnkoKaku;
			arc_data.angle2 = DEnko.m_radKaishiKaku + DEnko.m_radKatamukiKaku;
		}
		if( arc_data.angle2 <= arc_data.angle1 )
			arc_data.angle1 = arc_data.angle1 - M_PI * 2.0;
		arc_data.angle1 = Deg(arc_data.angle1);
		arc_data.angle2 = Deg(arc_data.angle2);
		arc_data.reversed = false;
		arc = new RS_Arc(graphic, arc_data);
		RS2::LineType ltype = lTable[DEnko.m_nPenStyle];
		RS_Color col = colTable[DEnko.m_nPenColor];
		RS2::LineWidth lw = lWidth[DEnko.m_nPenWidth > 26 ? 0 : DEnko.m_nPenWidth];//RS2::Width12
		arc->setPen(RS_Pen(col, lw, ltype));

		RS_String lName = HEX[DEnko.m_nGLayer > 0x0f ? 0:DEnko.m_nGLayer] + "-" +
			HEX[DEnko.m_nLayer > 0x0f ? 0: DEnko.m_nLayer];
		if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
#ifdef DEBUG
std::cout << lName.ascii() << std::endl;
#endif
			RS_Layer* layer = new RS_Layer(lName);
			graphic->addLayer(layer);
		}
		arc->setLayer(lName);
		// add the line to the graphic
		graphic->addEntity(arc);
	}
#endif
}

void DL_Jww::CreateTen(DL_CreationInterface* creationInterface, CDataTen& DTen)
{
	string lName = HEX[DTen.m_nGLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DTen.m_nGLayer] + "-" +
													HEX[DTen.m_nLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DTen.m_nLayer];

	// add layer
	creationInterface->addLayer(DL_LayerData(lName,0));
	int width;
	if(DTen.m_nPenWidth > 26)
		width = 0;
	else
		width = DTen.m_nPenWidth;
	int color = colTable[DTen.m_nPenColor > ArraySize(colTable)-1 ? ArraySize(colTable)-1 : DTen.m_nPenColor];
	attrib = DL_Attributes(values[8],	  // layer
			       color,	      // color
			       width,	      // width
			       lTable[DTen.m_nPenStyle > ArraySize(lTable)-1 ? ArraySize(lTable)-1 : DTen.m_nPenStyle]);	  // linetype
	creationInterface->setAttributes(attrib);

	creationInterface->setExtrusion(0.0, 0.0, 1.0, 0.0 );

	DL_PointData d(DTen.m_start.x, DTen.m_start.y, 0.0);
	creationInterface->addPoint(d);
#ifdef FINISHED
	RS_PointData data2(RS_Vector(0.0, 0.0));
	RS_Point*	point;

	data2.pos = RS_Vector(DTen.m_start.x, DTen.m_start.y);
	point = new RS_Point(graphic, data2);
	RS2::LineType ltype = lTable[DTen.m_nPenStyle];
	RS_Color col = colTable[DTen.m_nPenColor];
	RS2::LineWidth lw = lWidth[DTen.m_nPenWidth > 26 ? 0 :DTen.m_nPenWidth];//RS2::Width12
	point->setPen(RS_Pen(col, RS2::Width23, ltype));
	//画層設定
	RS_String lName = HEX[DTen.m_nGLayer > 0x0f ? 0 : DTen.m_nGLayer] + "-" + 
		HEX[DTen.m_nLayer > 0x0f ? 0: DTen.m_nLayer];
	if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
#ifdef DEBUG
std::cout << lName.ascii() << std::endl;
#endif
		RS_Layer* layer = new RS_Layer(lName);
		graphic->addLayer(layer);
	}
	point->setLayer(lName);
	// add the line to the graphic
	graphic->addEntity(point);
#ifdef DEBUG
std::cout << *point;
#endif
#endif
}

void DL_Jww::CreateMoji(DL_CreationInterface* creationInterface, CDataMoji& DMoji)
{
	string lName = HEX[DMoji.m_nGLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DMoji.m_nGLayer] + "-" +
													HEX[DMoji.m_nLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DMoji.m_nLayer];

	// add layer
	creationInterface->addLayer(DL_LayerData(lName,0));

	int width;
	if(DMoji.m_nPenWidth > 26)
		width = 0;
	else
		width = DMoji.m_nPenWidth;
	int color = colTable[DMoji.m_nPenColor > ArraySize(colTable)-1 ? ArraySize(colTable)-1 : DMoji.m_nPenColor];
	attrib = DL_Attributes(values[8],	  // layer
			       color,	      // color
			       width,	      // width
			       lTable[DMoji.m_nPenStyle > ArraySize(lTable)-1 ? ArraySize(lTable)-1 : DMoji.m_nPenStyle]);	  // linetype
	creationInterface->setAttributes(attrib);

	creationInterface->setExtrusion(0.0, 0.0, 1.0, 0.0 );

	DL_TextData d(
		// insertion point
		DMoji.m_start.x, DMoji.m_start.y, 0.0,
		// alignment point
		0.0, 0.0, 0.0,
		// height
		DMoji.m_dSizeY,
		// x scale
		1.0,
		// generation flags
		0,
		// h just
		0,
		// v just
		0,
		// text
		DMoji.m_string,
		// style
		string("japanese"),
		// angle
		DMoji.m_degKakudo / 180.0 * M_PI);

	creationInterface->addText(d);
#ifdef FINISHED
	QTextCodec* codec = QTextCodec::codecForName("SJIS");
	RS_TextData data3(RS_Vector(0.0, 0.0),
				10,//double height,
				10,//double width,
				RS2::VAlignMiddle,
				RS2::HAlignCenter,
				RS2::LeftToRight,
				RS2::AtLeast,
				1.0,//double lineSpacingFactor,
				RS_String(""),//const RS_String& text,
				RS_String(""),
				0.0,//double angle,
				RS2::Update);
	RS_Text*	text;
	data3.insertionPoint = RS_Vector(DMoji.m_start.x, DMoji.m_start.y);
	data3.height = DMoji.m_dSizeY;
	data3.width = DMoji.m_dSizeX;
	data3.valign = RS2::VAlignBottom;//VAlignMiddle;
	data3.halign = RS2::HAlignLeft;//HAlignCenter;
	data3.drawingDirection = RS2::LeftToRight;
	data3.lineSpacingStyle = RS2::Exact;
	data3.lineSpacingFactor = DMoji.m_dKankaku;//1.0;
	//コード変換
	size_t left = DMoji.m_string.length();
	char* sjis = (char *)DMoji.m_string.c_str();
	char buf[200];
	//memset(buf, NULL, 1000);
	char* p = buf;
	size_t bufleft = 200;
#ifdef _WINDOWS
	// エンコーディングの変換：iconvを使う場合
	iconv_t cd = iconv_open(UTF8_CES, SHIFTJIS_CES);
#ifdef	DEBUG
printf("sjis = %x, p = %x\n", sjis, p);
#endif
	size_t r = iconv(cd, (const char **)(&sjis), &left, &p, &bufleft);//const_cast<char**>
#ifdef	DEBUG
printf("sjis = %x, p = %x\n", sjis, p);
printf("sjis = %x %x %x %x, p = %x %x %x %x\n", sjis[0],sjis[1],sjis[2],sjis[3], buf[0],buf[1],buf[2],buf[3]);
printf("r = %d, left = %d, bufleft = %d\n", r, left, bufleft);
#endif
	*p = '\0';
	iconv_close(cd);
#else
//	int ires = SJIS2UTF8N(sjis,buf,bufleft);
	int nBytesOut;
	strcpy(buf,(const char *)CUnicodeF::sjis_to_euc((const unsigned char *)sjis/*, &nBytesOut*/));
//	QTextCodec* codec = QTextCodec::codecForName("eucJP");
//	data3.text = codec->toUnicode(buf);
#endif
//	data3.text = codec->toUnicode(sjis);
	data3.text = RS_String::fromUtf8(buf);
	data3.style = RS_String("japanese-euc");
	data3.angle = DMoji.m_degKakudo / 180.0 * M_PI;
#ifdef DEBUG
RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
#endif
	data3.updateMode = RS2::Update;
	//DWORD m_nMojiShu;//文字種(斜体文字は20000、ボールド体は10000を加えた数値)

	text = new RS_Text(graphic, data3);
	RS2::LineType ltype = lTable[DMoji.m_nPenStyle];
	RS_Color col = colTable[DMoji.m_nPenColor];
	RS2::LineWidth lw = lWidth[DMoji.m_nPenWidth > 26 ? 0 : DMoji.m_nPenWidth];//RS2::Width12
	text->setPen(RS_Pen(col, RS2::Width08, ltype));

	//画層設定
	RS_String lName = HEX[DMoji.m_nGLayer > 0x0f ? 0: DMoji.m_nGLayer] + "-" +
		HEX[DMoji.m_nLayer > 0x0f ? 0 : DMoji.m_nLayer];
	if( graphic->findLayer(lName) == (RS_Layer*)NULL ){
#ifdef DEBUG
std::cout << lName.ascii() << std::endl;
#endif
		RS_Layer* layer = new RS_Layer(lName);
		graphic->addLayer(layer);
	}
	text->setLayer(lName);
	// add the line to the graphic
	graphic->addEntity(text);
#ifdef DEBUG
std::cout << data3.height << "  " << data3.width << std::endl;
std::cout << *text;
#endif
#endif
}

void DL_Jww::CreateSolid(DL_CreationInterface* /*creationInterface*/, CDataSolid& /*DSolid*/)
{
}

void DL_Jww::CreateSunpou(DL_CreationInterface* creationInterface, CDataSunpou& DSunpou)
{
	string lName = HEX[DSunpou.m_nGLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DSunpou.m_nGLayer] + "-" +
													HEX[DSunpou.m_nLayer > ArraySize(HEX)-1 ? ArraySize(HEX)-1: DSunpou.m_nLayer];

	// add layer
	creationInterface->addLayer(DL_LayerData(lName,0));
	int width;
	if(DSunpou.m_nPenWidth > 26)
		width = 0;
	else
		width = DSunpou.m_nPenWidth;
	int color = colTable[DSunpou.m_nPenColor > ArraySize(colTable)-1 ? ArraySize(colTable)-1 : DSunpou.m_nPenColor];
	attrib = DL_Attributes(values[8],	  // layer
			       color,	      // color
			       width,	      // width
			       lTable[DSunpou.m_nPenStyle > ArraySize(lTable)-1 ? ArraySize(lTable)-1 : DSunpou.m_nPenStyle]);	  // linetype
	creationInterface->setAttributes(attrib);

	creationInterface->setExtrusion(0.0, 0.0, 1.0, 0.0 );

	CreateSen(creationInterface, DSunpou.m_Sen);	//線分メンバ
	CreateMoji(creationInterface, DSunpou.m_Moji);	//文字メンバ
#ifdef FINISHED
//	if(DSunpou.nOldVersionSave >=420){
////	WORD m_bSxfMode;	//SXFのモード
//		CreateSen(graphic, DSunpou.m_SenHo1);	//補助線1
//		CreateSen(graphic, DSunpou.m_SenHo2);	//補助線2
//		CreateTen(graphic, DSunpou.m_Ten1);	//矢印（点)1
//		CreateTen(graphic, DSunpou.m_Ten2);	//矢印（点)2
//		CreateTen(graphic, DSunpou.m_TenHo1);	//基準点1
//		CreateTen(graphic, DSunpou.m_TenHo2);	//基準点2
//	}
#endif
}

void DL_Jww::CreateBlock(DL_CreationInterface* /*creationInterface*/, CDataBlock& /*DBlock*/)
{
#ifdef FINISHED
/*	int BlockSize=jwdoc->pBlockList->getBlockListCount();
	for(int i=0; i < BlockSize; i++ )
	{
		int Count=jwdoc->pBlockList->GetDataListCount(i);
		for( int j=0 ; j < Count; j++)
		{
			switch(jwdoc->pBlockList->GetDataType(i,j))
			{
			case	Sen :
				CreateSen(graphic, jwdoc->pBlockList->GetCDataSen(i,j));
				break;
			case	Enko:
				CreateEnko(graphic, jwdoc->pBlockList->GetCDataEnko(i,j));
				break;
			case	Ten:
				CreateTen(graphic, jwdoc->pBlockList->GetCDataTen(i,j));
				break;
			case	Moji:
				CreateMoji(graphic, jwdoc->pBlockList->GetCDataMoji(i,j));
				break;
			case	Solid:
				CreateSolid(graphic, jwdoc->pBlockList->GetCDataSolid(i,j));
				break;
			case	Sunpou:
				CreateSunpou(graphic, jwdoc->pBlockList->GetCDataSunpou(i,j));
				break;
			case	Block:
				CreateBlock(graphic, jwdoc->pBlockList->GetCDataBlock(i,j));
				break;
			}
		}
	}
*/
#endif
}

/**
 * @brief Reads the given file and calls the appropriate functions in
 * the given creation interface for every entity found in the file.
 *
 * @param file Input
 *		Path and name of file to read
 * @param creationInterface
 *		Pointer to the class which takes care of the entities in the file.
 *
 * @retval true If \p file could be opened.
 * @retval false If \p file could not be opened.
 */
bool DL_Jww::in(const string& file, DL_CreationInterface* creationInterface) {
	//JWWファイル読み取り
	string ofile("");
	JWWDocument* jwdoc = new JWWDocument((std::string&)file, ofile);
	if(!jwdoc->Read())
		return false;
	//DXF変数設定
	creationInterface->setVariableString("$DWGCODEPAGE", "SJIS", 7);
	creationInterface->setVariableString("$TEXTSTYLE", "japanese", 7);
	//線分データ
	for( unsigned int i = 0; i < jwdoc->vSen.size(); i++ )
		CreateSen(creationInterface, jwdoc->vSen[i]);
	//円弧データ
	for( unsigned int i = 0; i < jwdoc->vEnko.size(); i++ )
		CreateEnko(creationInterface, jwdoc->vEnko[i]);
	//点データ
	for( unsigned int i = 0; i < jwdoc->vTen.size(); i++ )
		CreateTen(creationInterface, jwdoc->vTen[i]);
	//文字データ
	for( unsigned int i = 0; i < jwdoc->vMoji.size(); i++ )
		CreateMoji(creationInterface, jwdoc->vMoji[i]);
	//寸法
    for(unsigned int i=0 ; i < jwdoc->vSunpou.size(); i++ )
		CreateSunpou(creationInterface, jwdoc->vSunpou[i]);
	//ソリッド
    for(unsigned int i=0 ; i < jwdoc->vSolid.size(); i++ )
		CreateSolid(creationInterface, jwdoc->vSolid[i]);
	//部品
    for(unsigned int i=0 ; i < jwdoc->vBlock.size(); i++)
		CreateBlock(creationInterface, jwdoc->vBlock[i]);
	delete jwdoc;

	return true;
}

/**
 * Processes a group (pair of group code and value).
 *
 * @param creationInterface Handle to class that creates entities and
 * other CAD data from DXF group codes
 *
 * @param groupCode Constant indicating the data type of the group.
 * @param groupValue The data value.
 *
 * @retval true if done processing current entity and new entity begun
 * @retval false if not done processing current entity
*/
bool DL_Jww::processJwwGroup(DL_CreationInterface* /*creationInterface*/,
                             int /*groupCode*/, const char */*groupValue*/) {
	return true;
}



/**
 * Adds a variable from the DXF file.
 */
void DL_Jww::addSetting(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a layer that was read from the file via the creation interface.
 */
void DL_Jww::addLayer(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a block that was read from the file via the creation interface.
 */
void DL_Jww::addBlock(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Ends a block that was read from the file via the creation interface.
 */
void DL_Jww::endBlock(DL_CreationInterface* creationInterface) {
	creationInterface->endBlock();
}



/**
 * Adds a point entity that was read from the file via the creation interface.
 */
void DL_Jww::addPoint(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a line entity that was read from the file via the creation interface.
 */
void DL_Jww::addLine(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a polyline entity that was read from the file via the creation interface.
 */
void DL_Jww::addPolyline(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a polyline vertex entity that was read from the file 
 * via the creation interface.
 */
void DL_Jww::addVertex(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a spline entity that was read from the file via the creation interface.
 */
void DL_Jww::addSpline(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a knot to the previously added spline. 
 */
/*
void DL_Jww::addKnot(DL_CreationInterface* creationInterface) {
   std::cout << "DL_Jww::addKnot\n";
}
*/



/**
 * Adds a control point to the previously added spline. 
 */
/*
void DL_Jww::addControlPoint(DL_CreationInterface* creationInterface) {
	std::cout << "DL_Jww::addControlPoint\n";
}
*/



/**
 * Adds an arc entity that was read from the file via the creation interface.
 */
void DL_Jww::addArc(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a circle entity that was read from the file via the creation interface.
 */
void DL_Jww::addCircle(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an ellipse entity that was read from the file via the creation interface.
 */
void DL_Jww::addEllipse(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an insert entity that was read from the file via the creation interface.
 */
void DL_Jww::addInsert(DL_CreationInterface* /*creationInterface*/) {
}


/**
 * Adds a trace entity (4 edge closed polyline) that was read from the file via the creation interface.
 *
 * @author AHM
 */
void DL_Jww::addTrace(DL_CreationInterface* /*creationInterface*/) {
}

/**
 * Adds a solid entity (filled trace) that was read from the file via the creation interface.
 * 
 * @author AHM
 */
void DL_Jww::addSolid(DL_CreationInterface* /*creationInterface*/) {
}


/**
 * Adds an MText entity that was read from the file via the creation interface.
 */
void DL_Jww::addMText(DL_CreationInterface* /*creationInterface*/) {
}

/**
 * Handles additional MText data.
 */
bool DL_Jww::handleMTextData(DL_CreationInterface* /*creationInterface*/) {
	return true;
}



/**
 * Handles additional polyline data.
 */
bool DL_Jww::handleLWPolylineData(DL_CreationInterface* /*creationInterface*/) {
	return true;
}



/**
 * Handles additional spline data.
 */
bool DL_Jww::handleSplineData(DL_CreationInterface* /*creationInterface*/) {
	return true;
}



/**
 * Handles additional leader data.
 */
bool DL_Jww::handleLeaderData(DL_CreationInterface* /*creationInterface*/) {
	return true;
}



/**
 * Handles additional hatch data.
 */
bool DL_Jww::handleHatchData(DL_CreationInterface* /*creationInterface*/) {
	return true;
}




/**
 * Adds an text entity that was read from the file via the creation interface.
 */
void DL_Jww::addText(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an attrib entity that was read from the file via the creation interface.
 * @todo add attrib instead of normal text
 */
void DL_Jww::addAttrib(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * @return dimension data from current values.
 */
DL_DimensionData DL_Jww::getDimData() {
    // tin-pot@gmx.net 2011-12-29: make compiler happy.
    DL_DimensionData dummy(0.,0.,0.,0.,0.,0.,0,0,0,0.,"DUMMY","",0.);
    return dummy;
}



/**
 * Adds a linear dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimLinear(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an aligned dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimAligned(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a radial dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimRadial(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a diametric dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimDiametric(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an angular dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimAngular(DL_CreationInterface* /*creationInterface*/) {
}


/**
 * Adds an angular dimension entity that was read from the file via the creation interface.
 */
void DL_Jww::addDimAngular3P(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a leader entity that was read from the file via the creation interface.
 */
void DL_Jww::addLeader(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds a hatch entity that was read from the file via the creation interface.
 */
void DL_Jww::addHatch(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an image entity that was read from the file via the creation interface.
 */
void DL_Jww::addImage(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Adds an image definition that was read from the file via the creation interface.
 */
void DL_Jww::addImageDef(DL_CreationInterface* /*creationInterface*/) {
}



/**
 * Ends some special entities like hatches or old style polylines.
 */
void DL_Jww::endEntity(DL_CreationInterface* /*creationInterface*/) {
}


/**
 * Ends a sequence and notifies the creation interface.
 */
void DL_Jww::endSequence(DL_CreationInterface* /*creationInterface*/) {
}


/**
 * @brief Opens the given file for writing and returns a pointer
 * to the dxf writer. This pointer needs to be passed on to other
 * writing functions.
 *
 * @param file Full path of the file to open.
 *
 * @return Pointer to an ascii dxf writer object.
 */
DL_WriterA* DL_Jww::out(const char* /*file*/, DL_Codes::version /*version*/) {
#ifdef DEBUG
//	jwdoc->Save();
#endif
    return NULL; // tin-pot@gmx.net 2011-12-29: Make compiler happy.
}



/**
 * @brief Writes a DXF header to the file currently opened 
 * by the given DXF writer object.
 */
void DL_Jww::writeHeader(DL_WriterA& /*dw*/) {
}




/**
 * Writes a point entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writePoint(DL_WriterA& /*dw*/,
                        const DL_PointData& /*data*/,
                        const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a line entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeLine(DL_WriterA& /*dw*/,
                       const DL_LineData& /*data*/,
                       const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a polyline entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 * @see writeVertex
 */
void DL_Jww::writePolyline(DL_WriterA& /*dw*/,
                           const DL_PolylineData& /*data*/,
                           const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a single vertex of a polyline to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeVertex(DL_WriterA& /*dw*/,
                         const DL_VertexData& /*data*/) {
}

	
	
/**
 * Writes the polyline end. Only needed for DXF R12.
 */
void DL_Jww::writePolylineEnd(DL_WriterA& /*dw*/) {
}


/**
 * Writes a spline entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 * @see writeControlPoint
 */
void DL_Jww::writeSpline(DL_WriterA& /*dw*/,
                         const DL_SplineData& /*data*/,
                         const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a single control point of a spline to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeControlPoint(DL_WriterA& /*dw*/,
                               const DL_ControlPointData& /*data*/) {
}



/**
 * Writes a single knot of a spline to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeKnot(DL_WriterA& /*dw*/,
                       const DL_KnotData& /*data*/) {
}



/**
 * Writes a circle entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeCircle(DL_WriterA& /*dw*/,
                         const DL_CircleData& /*data*/,
                         const DL_Attributes& /*attrib*/) {
}



/**
 * Writes an arc entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeArc(DL_WriterA& /*dw*/,
                      const DL_ArcData& /*data*/,
                      const DL_Attributes& /*attrib*/) {
}



/**
 * Writes an ellipse entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeEllipse(DL_WriterA& /*dw*/,
                          const DL_EllipseData& /*data*/,
                          const DL_Attributes& /*attrib*/) {
}



/**
 * Writes an insert to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeInsert(DL_WriterA& /*dw*/,
                         const DL_InsertData& /*data*/,
                         const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a multi text entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeMText(DL_WriterA& /*dw*/,
                        const DL_MTextData& /*data*/,
                        const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a text entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeText(DL_WriterA& /*dw*/,
                       const DL_TextData& /*data*/,
                       const DL_Attributes& /*attrib*/) {
}


/**
 * Writes an aligned dimension entity to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific aligned dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimAligned(DL_WriterA& /*dw*/,
                             const DL_DimensionData& /*data*/,
                             const DL_DimAlignedData& /*edata*/,
                             const DL_Attributes& /*attrib*/) {
}

/**
 * Writes a linear dimension entity to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific linear dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimLinear(DL_WriterA& /*dw*/,
                            const DL_DimensionData& /*data*/,
                            const DL_DimLinearData& /*edata*/,
                            const DL_Attributes& /*attrib*/) {
}

/**
 * Writes a radial dimension entity to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific radial dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimRadial(DL_WriterA& /*dw*/,
                            const DL_DimensionData& /*data*/,
                            const DL_DimRadialData& /*edata*/,
                            const DL_Attributes& /*attrib*/) {
}

/**
 * Writes a diametric dimension entity to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific diametric dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimDiametric(DL_WriterA& /*dw*/,
                               const DL_DimensionData& /*data*/,
                               const DL_DimDiametricData& /*edata*/,
                               const DL_Attributes& /*attrib*/) {
}



/**
 * Writes an angular dimension entity to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific angular dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimAngular(DL_WriterA& /*dw*/,
                             const DL_DimensionData& /*data*/,
                             const DL_DimAngularData& /*edata*/,
                             const DL_Attributes& /*attrib*/) {
}

/**
 * Writes an angular dimension entity (3 points version) to the file.
 *
 * @param dw DXF writer
 * @param data Generic dimension data for from the file
 * @param data Specific angular dimension data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeDimAngular3P(DL_WriterA& /*dw*/,
                               const DL_DimensionData& /*data*/,
                               const DL_DimAngular3PData& /*edata*/,
                               const DL_Attributes& /*attrib*/) {
}

/**
 * Writes a leader entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 * @see writeVertex
 */
void DL_Jww::writeLeader(DL_WriterA& /*dw*/,
                         const DL_LeaderData& /*data*/,
                         const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a single vertex of a leader to the file.
 *
 * @param dw DXF writer
 * @param data Entity data
 */
void DL_Jww::writeLeaderVertex(DL_WriterA& /*dw*/,
                               const DL_LeaderVertexData& /*data*/) {
}



/**
 * Writes the beginning of a hatch entity to the file.
 * This must be followed by one or more writeHatchLoop()
 * calls and a writeHatch2() call.
 *
 * @param dw DXF writer
 * @param data Entity data.
 * @param attrib Attributes
 */
void DL_Jww::writeHatch1(DL_WriterA& /*dw*/,
                         const DL_HatchData& /*data*/,
                         const DL_Attributes& /*attrib*/) {
}



/**
 * Writes the end of a hatch entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data.
 * @param attrib Attributes
 */
void DL_Jww::writeHatch2(DL_WriterA& /*dw*/,
                         const DL_HatchData& /*data*/,
						 const DL_Attributes& /*attrib*/) {
}



/**
 * Writes the beginning of a hatch loop to the file. This
 * must happen after writing the beginning of a hatch entity.
 *
 * @param dw DXF writer
 * @param data Entity data.
 * @param attrib Attributes
 */
void DL_Jww::writeHatchLoop1(DL_WriterA& /*dw*/,
                             const DL_HatchLoopData& /*data*/) {
}



/**
 * Writes the end of a hatch loop to the file.
 *
 * @param dw DXF writer
 * @param data Entity data.
 * @param attrib Attributes
 */
void DL_Jww::writeHatchLoop2(DL_WriterA& /*dw*/,
							 const DL_HatchLoopData& /*data*/) {

}


/**
 * Writes the beginning of a hatch entity to the file.
 *
 * @param dw DXF writer
 * @param data Entity data.
 * @param attrib Attributes
 */
void DL_Jww::writeHatchEdge(DL_WriterA& /*dw*/,
                            const DL_HatchEdgeData& /*data*/) {
}



/**
 * Writes an image entity.
 *
 * @return IMAGEDEF handle. Needed for the IMAGEDEF counterpart.
 */
int DL_Jww::writeImage(DL_WriterA& /*dw*/,
                       const DL_ImageData& /*data*/,
                       const DL_Attributes& /*attrib*/) {

    return -1; // tin-pot@gmx.net 2011-12-29: Make compiler happy.
}



/**
 * Writes an image definiition entity.
 */
void DL_Jww::writeImageDef(DL_WriterA& /*dw*/,
                           int /*handle*/,
                           const DL_ImageData& /*data*/) {
}


/**
 * Writes a layer to the file. Layers are stored in the 
 * tables section of a DXF file.
 *
 * @param dw DXF writer
 * @param data Entity data from the file
 * @param attrib Attributes
 */
void DL_Jww::writeLayer(DL_WriterA& /*dw*/,
                        const DL_LayerData& /*data*/,
                        const DL_Attributes& /*attrib*/) {
}



/**
 * Writes a line type to the file. Line types are stored in the 
 * tables section of a DXF file.
 */
void DL_Jww::writeLineType(DL_WriterA& /*dw*/,
                           const DL_LineTypeData& /*data*/) {
}

/**
 * Writes the APPID section to the DXF file.
 *
 * @param name Application name
 */
void DL_Jww::writeAppid(DL_WriterA& /*dw*/, const string& /*name*/) {
}



/**
 * Writes a block's definition (no entities) to the DXF file.
 */
void DL_Jww::writeBlock(DL_WriterA& /*dw*/, const DL_BlockData& /*data*/) {
}



/**
 * Writes a block end.
 *
 * @param name Block name
 */
void DL_Jww::writeEndBlock(DL_WriterA& /*dw*/, const string& /*name*/) {
}



/**
 * Writes a viewport section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked VPORT section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeVPort(DL_WriterA& /*dw*/) {
}



/**
 * Writes a style section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked STYLE section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeStyle(DL_WriterA& /*dw*/) {
}



/**
 * Writes a view section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked VIEW section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeView(DL_WriterA& /*dw*/) {
}



/**
 * Writes a ucs section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked UCS section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeUcs(DL_WriterA& /*dw*/) {
}

/**
 * Writes a dimstyle section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked DIMSTYLE section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeDimStyle(DL_WriterA& /*dw*/,
                    double /*dimasz*/, double /*dimexe*/, double /*dimexo*/,
                       double /*dimgap*/, double /*dimtxt*/) {
}



/**
 * Writes a blockrecord section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked BLOCKRECORD section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeBlockRecord(DL_WriterA& /*dw*/) {
}

/**
 * Writes a single block record with the given name.
 */
void DL_Jww::writeBlockRecord(DL_WriterA& /*dw*/, const string& /*name*/) {
}



/**
 * Writes a objects section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked OBJECTS section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeObjects(DL_WriterA& /*dw*/) {
}


/**
 * Writes the end of the objects section. This section is needed in VER_R13.
 * Note that this method currently only writes a faked OBJECTS section
 * to make the file readable by Aut*cad.
 */
void DL_Jww::writeObjectsEnd(DL_WriterA& /*dw*/) {
}



/**
 * Checks if the given variable is known by the given DXF version.
 */
bool DL_Jww::checkVariable(const char* /*var*/, DL_Codes::version /*version*/) {
	return true;
}



/**
 * @returns the library version as int (4 bytes, each byte one version number).
 * e.g. if str = "2.0.2.0" getLibVersion returns 0x02000200
 */
int DL_Jww::getLibVersion(const char* /*str*/) {
    return -1; // tin-pot@gmx.net 2011-12-29: Make compiler happy.
}
