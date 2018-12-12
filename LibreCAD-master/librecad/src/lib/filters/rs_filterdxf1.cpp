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

#include<cstdlib>
#include "rs_filterdxfrw.h"
#include "rs_filterdxf1.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_font.h"
#include "rs_information.h"
#include "rs_utility.h"
#include "rs_system.h"
#include "rs_dimlinear.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimradial.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_point.h"
#include "rs_math.h"
#include "rs_debug.h"


/**
 * Default constructor.
 */
RS_FilterDXF1::RS_FilterDXF1()
		:RS_FilterInterface()
		, graphic(nullptr)
{
	RS_DEBUG->print("Setting up DXF 1 filter...");
}

/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param graphic The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterDXF1::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("DXF1 Filter: importing file '%s'...", file.toLatin1().data());

	this->graphic = &g;

    fPointer=0;
    fBuf=0;
    fBufP=0;
    fSize=0;
    dosFile=false;
    name = file;

    if(readFileInBuffer()) {
        separateBuf();
        return readFromBuffer();
    }
	
    return false;
}

bool RS_FilterDXF1::fileExport(RS_Graphic& /*g*/, const QString& /*file*/,
	RS2::FormatType /*type*/) {
	RS_DEBUG->print(RS_Debug::D_WARNING,
					"Exporting of QCad 1.x file not implemented");
	return false;
}

/**
 * Reads a dxf1 file from buffer.
 */
bool RS_FilterDXF1::readFromBuffer() {
    RS_DEBUG->print( "\nDXF: Read from buffer" );

    bool      ret;                    // returned value
    QString   dxfLine;                // A line in the dxf file
    QString   dxfCode;                // A Code in the dxf file as string
    int       code=-1;                // Dxf-code as number
    double    vx1=0.0, vy1=0.0;       // Start point
    double    vx2=0.0, vy2=0.0;       // End point
    double    vcx=0.0, vcy=0.0;       // Centre
    double    vcr=0.0;                // Radius
    double    va1=0.0, va2=0.0;       // Start / End Angle
    //double    vab=0.0,                // Bulge
    //           vpx=0.0, vpy=0.0;       // First Polyline point
    //double    ax=0.0, ay=0.0;         // Current coordinate
    //bool      plClose=false;          // Polyline closed-flag
    QString lastLayer;              // Last used layer name (test adding only
    //   if the new layer!=lastLayer)
    //int       currentLayerNum=0;      // Current layer number
    RS_Layer* currentLayer=0;         // Pointer to current layer
    //QList<RGraphic> blockList;        // List of blocks
    //blockList.setAutoDelete( true );
    //bool      oldColorNumbers=false;  // use old color numbers (qcad<1.5.3)
    RS_Pen pen;

    ///if(!add) graphic->clearLayers();

    //graphic->addLayer(DEF_DEFAULTLAYER);

    //RS_DEBUG->print( "\nDefault layer added" );

    // Loaded graphics without unit information: load as unit less:
    //graphic->setUnit( None );

    RS_DEBUG->print( "\nUnit set" );

    resetBufP();

    if(fBuf) {

        RS_DEBUG->print( "\nBuffer OK" );
        RS_DEBUG->print( "\nBuffer: " );
        RS_DEBUG->print( fBuf );

        do {
            dxfLine=getBufLine();
            pen = RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer);

            RS_DEBUG->print( "\ndxfLine: " );
            RS_DEBUG->print( dxfLine.toLatin1().data() );

            // $-Setting in the header of DXF found
            // RVT_PORT changed all occurenses of if (dxfline && ....) to if (dxfline.size() ......)
            if( dxfLine.size() &&
                    dxfLine[0]=='$' ) {


                // Units:
                //
                if( dxfLine=="$INSUNITS" ) {
					dxfCode=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==70 ) {
							dxfLine=getBufLine() ;
                            if( dxfLine.size() ) {
                                graphic->addVariable("$INSUNITS", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case  0: graphic->setUnit( RS2::None );       break;
                                              case  1: graphic->setUnit( RS2::Inch );       break;
                                              case  2: graphic->setUnit( RS2::Foot );       break;
                                              case  3: graphic->setUnit( RS2::Mile );       break;
                                              case  4: graphic->setUnit( RS2::Millimeter ); break;
                                              case  5: graphic->setUnit( RS2::Centimeter ); break;
                                              case  6: graphic->setUnit( RS2::Meter );      break;
                                              case  7: graphic->setUnit( RS2::Kilometer );  break;
                                              case  8: graphic->setUnit( RS2::Microinch );  break;
                                              case  9: graphic->setUnit( RS2::Mil );        break;
                                              case 10: graphic->setUnit( RS2::Yard );       break;
                                              case 11: graphic->setUnit( RS2::Angstrom );   break;
                                              case 12: graphic->setUnit( RS2::Nanometer );  break;
                                              case 13: graphic->setUnit( RS2::Micron );     break;
                                              case 14: graphic->setUnit( RS2::Decimeter );  break;
                                              case 15: graphic->setUnit( RS2::Decameter );  break;
                                              case 16: graphic->setUnit( RS2::Hectometer ); break;
                                              case 17: graphic->setUnit( RS2::Gigameter );  break;
                                              case 18: graphic->setUnit( RS2::Astro );      break;
                                              case 19: graphic->setUnit( RS2::Lightyear );  break;
                                              case 20: graphic->setUnit( RS2::Parsec );     break;
                                            }

                                            graphic->setDimensionUnit( graphic->getUnit() );
                                            //graphic->setGridUnit( graphic->getUnit() );
                                */
                            }
                        }
                    }
                }

                // Dimension Units:
                //
                else if( dxfLine=="$DIMALT" ) {
					dxfCode=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==70 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMALT", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case  0: graphic->setDimensionUnit( RS2::None );       break;
                                              case  1: graphic->setDimensionUnit( RS2::Inch );       break;
                                              case  2: graphic->setDimensionUnit( RS2::Foot );       break;
                                              case  3: graphic->setDimensionUnit( RS2::Mile );       break;
                                              case  4: graphic->setDimensionUnit( RS2::Millimeter ); break;
                                              case  5: graphic->setDimensionUnit( RS2::Centimeter ); break;
                                              case  6: graphic->setDimensionUnit( RS2::Meter );      break;
                                              case  7: graphic->setDimensionUnit( RS2::Kilometer );  break;
                                              case  8: graphic->setDimensionUnit( RS2::Microinch );  break;
                                              case  9: graphic->setDimensionUnit( RS2::Mil );        break;
                                              case 10: graphic->setDimensionUnit( RS2::Yard );       break;
                                              case 11: graphic->setDimensionUnit( RS2::Angstrom );   break;
                                              case 12: graphic->setDimensionUnit( RS2::Nanometer );  break;
                                              case 13: graphic->setDimensionUnit( RS2::Micron );     break;
                                              case 14: graphic->setDimensionUnit( RS2::Decimeter );  break;
                                              case 15: graphic->setDimensionUnit( RS2::Decameter );  break;
                                              case 16: graphic->setDimensionUnit( RS2::Hectometer ); break;
                                              case 17: graphic->setDimensionUnit( RS2::Gigameter );  break;
                                              case 18: graphic->setDimensionUnit( RS2::Astro );      break;
                                              case 19: graphic->setDimensionUnit( RS2::Lightyear );  break;
                                              case 20: graphic->setDimensionUnit( RS2::Parsec );     break;
                                            }
                                */
                            }
                        }
                    }
                }

                // Dimension Format:
                //
                /*else if( dxfLine=="$DIMLUNIT" ) {
                  if(dxfCode=getBufLine()) {
                    if( dxfCode.toInt()==70 ) {
                      if( dxfLine=getBufLine() ) {
                        switch( dxfLine.toInt() ) {
                          case 1: graphic->setDimensionFormat( Scientific ); break;
                          case 2:
                          case 3: graphic->setDimensionFormat( Decimal ); break;
                          case 4:
                          case 5: graphic->setDimensionFormat( Fractional ); break;
                          default: break;
                        }
                      }
                    }
                  }
            }*/

                // Dimension Arrow Size:
                //
                else if( dxfLine=="$DIMASZ" ) {
					dxfCode=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==40 ) {
							dxfLine=getBufLine() ;
                            if( dxfLine.size()) {
                                graphic->addVariable("$DIMASZ", dxfLine, 40);
                                //graphic->setDimensionArrowSize( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                // Dimension Scale:
                //
                /*
                else if( dxfLine=="$DIMSCALE" ) {
                  if(dxfCode=getBufLine()) {
                    if( dxfCode.toInt()==40 ) {
                      if( dxfLine=getBufLine() ) {
                        graphic->setDimensionScale( dxfLine.toDouble() );
                      }
                    }
                  }
            }
                */

                // Dimension Text Height:
                //







                else if( dxfLine=="$DIMTXT" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==40 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMTXT", dxfLine, 40);
                                //graphic->setDimensionTextHeight( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                // Dimension exactness:
                //







                else if( dxfLine=="$DIMRND" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==40 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMRND", dxfLine, 40);
                                //if( dxfLine.toDouble()>0.000001 ) {
                                //graphic->setDimensionExactness( dxfLine.toDouble() );
                            }
                            //}
                        }
                    }
                }

                // Dimension over length:
                //







                else if( dxfLine=="$DIMEXE" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==40 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMEXE", dxfLine, 40);
                                //graphic->setDimensionOverLength( dxfLine.toDouble() );
                            }
                        }
                    }
                }

                // Dimension under length:
                //







                else if( dxfLine=="$DIMEXO" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==40 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMEXO", dxfLine, 40);
                                //graphic->setDimensionUnderLength( dxfLine.toDouble() );
                            }
                        }
                    }
                }


                // Angle dimension format:
                //







                else if( dxfLine=="$DIMAUNIT" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==70 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMAUNIT", dxfLine, 70);
                                /*
                                            switch( dxfLine.toInt() ) {
                                              case 0: graphic->setAngleDimensionFormat( DecimalDegrees ); break;
                                              case 1: graphic->setAngleDimensionFormat( DegreesMinutesSeconds ); break;
                                              case 2: graphic->setAngleDimensionFormat( Gradians ); break;
                                              case 3: graphic->setAngleDimensionFormat( Radians ); break;
                                              case 4: graphic->setAngleDimensionFormat( Surveyor ); break;
                                              default: break;
                                            }
                                */
                            }
                        }
                    }
                }

                // Angle dimension exactness:
                //
                else if( dxfLine=="$DIMADEC" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==70 ) {
							dxfLine=getBufLine();
                            if( dxfLine.size() ) {
                                graphic->addVariable("$DIMADEC", dxfLine, 70);
                                //graphic->setAngleDimensionExactness( RS_Math::pow(0.1, dxfLine.toInt()) );
                            }
                        }
                    }
                }

                // Grid x/y:
                //
                else if( dxfLine=="$GRIDUNIT" ) {
					dxfLine=getBufLine();
                    if(dxfCode.size()) {
                        if( dxfCode.toInt()==10 ) {
							dxfLine=getBufLine();
                            if (dxfLine.size()) {
                                double x = atof(dxfLine.toLatin1().data());
								dxfLine=getBufLine();
                                if (dxfLine.size()) {
                                    double y = atof(dxfLine.toLatin1().data());

                                    graphic->addVariable("$GRIDUNIT", RS_Vector(x,y), 10);
                                }
                            }
                        }
                    }
                }
                /*
                            double gx=dxfLine.toDouble();
                            if (gx<0.0001) gx=0.0001;
                            graphic->setMinGridX(gx);
                            graphic->setGridFormat( Fractional );

                            for( double q=0.00000001; q<=100000.0; q*=10.0 ) {
                              if( mtCompFloat(gx, q, q/1000.0) ) {
                                graphic->setGridFormat( Decimal );
                                break;
                              }
                            }

                          }
                        }
                      }
                      if(dxfCode=getBufLine()) {
                        if( dxfCode.toInt()==20 ) {
                          if( dxfLine=getBufLine() ) {
                            double gy=dxfLine.toDouble();
                            if (gy<0.0001) gy=0.0001;
                            graphic->setMinGridY(gy);
                          }
                        }
                      }
                */

                // Page limits min x/y:
                //
                /*else if( dxfLine=="$PLIMMIN" ) {
                  if(dxfCode=getBufLine()) {
                    if( dxfCode.toInt()==10 ) {
                      if( dxfLine=getBufLine() ) {
                        graphic->setPageOriginX( dxfLine.toDouble() );
                      }
                    }
                  }
                  if(dxfCode=getBufLine()) {
                    if( dxfCode.toInt()==20 ) {
                      if( dxfLine=getBufLine() ) {
                        graphic->setPageOriginY( dxfLine.toDouble() );
                      }
                    }
                  }
            }
                */

                // Page limits min x/y:
                //
                /*
                      else if( dxfLine=="$PLIMMAX" ) {
                        if(dxfCode=getBufLine()) {
                          if( dxfCode.toInt()==10 ) {
                            if( dxfLine=getBufLine() ) {
                              graphic->setPageSizeX( dxfLine.toDouble() - graphic->getPageOriginX() );
                            }
                          }
                        }
                        if(dxfCode=getBufLine()) {
                          if( dxfCode.toInt()==20 ) {
                            if( dxfLine=getBufLine() ) {
                              graphic->setPageSizeY( dxfLine.toDouble() - graphic->getPageOriginY() );
                            }
                          }
                        }
                      }
                */

                // Paper space scale:
                //
                /*
                      else if( dxfLine=="$PSVPSCALE" ) {
                        if(dxfCode=getBufLine()) {
                          if( dxfCode.toInt()==40 ) {
                            if( dxfLine=getBufLine() ) {
                              graphic->setPaperSpace( dxfLine.toDouble() );
                            }
                          }
                        }
                      }
                */

            }

            // Entity
            //
            else if(dxfLine.size() &&
                    dxfLine[0]>='A' && dxfLine[0]<='Z') {

                if(dxfLine=="EOF") {
                    // End of file reached
                    //
                }

                // ------
                // Layer:
                // ------
                else if(dxfLine=="LAYER") {
                    currentLayer=0;
                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine.size()) {
                                switch(code) {
                                case  2:  // Layer name
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->addLayer(new RS_Layer(dxfLine));
                                    graphic->activateLayer(dxfLine);
                                    currentLayer = graphic->getActiveLayer();
                                    lastLayer=dxfLine;
                                    break;
                                case 70:  // Visibility
                                    /*
                                    if(dxfLine.toInt()&5) {
                                      if(currentLayerNum>=0 && currentLayerNum<DEF_MAXLAYERS) {
                                        graphic->layer[currentLayerNum].DelFlag(Y_VISIBLE);
                                      }
                                }
                                    */
                                    break;
                                case  6:  // style
                                    //if(currentLayer)
                                    //currentLayer->setStyle( graphic->nameToStyle(dxfLine) );
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case 39:  // Thickness
                                    //if(currentLayer) currentLayer->setWidth(dxfLine.toInt());
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    //if(currentLayer) {
                                    //	currentLayer->setColor( graphic->numberToColor(dxfLine.toInt(), !oldColorNumbers));
                                    //}
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
                    if (currentLayer) {
                        currentLayer->setPen(pen);
                    }
                    //graphic->setStyle("CONTINUOUS");
                    //graphic->setWidth(0);
                    //graphic->setColor(0, false);
                }

                // ------
                // Point:
                // ------
                else if(dxfLine=="POINT") {
                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine.size()) {
                                switch(code) {
                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;
                                case 10:  // X1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vx1 = dxfLine.toDouble();
                                    break;
                                case 20:  // Y1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vy1 = dxfLine.toDouble();
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
                    graphic->setActivePen(pen);
                    graphic->addEntity(new RS_Point(graphic,
                                                    RS_PointData(RS_Vector(vx1, vy1))));
                }

                // -----
                // Line:
                // -----
                else if(dxfLine=="LINE") {
                    do {
                        dxfCode=getBufLine();

                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {

                            dxfLine=getBufLine();

                            if(dxfLine.size()) {
                                switch(code) {
                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;
                                case 10:  // X1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vx1 = dxfLine.toDouble();
                                    break;
                                case 20:  // Y1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vy1 = dxfLine.toDouble();
                                    break;
                                case 11:  // X2
                                    dxfLine.replace( QRegExp(","), "." );
                                    vx2 = dxfLine.toDouble();
                                    break;
                                case 21:  // Y2
                                    dxfLine.replace( QRegExp(","), "." );
                                    vy2 = dxfLine.toDouble();
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);

                    //if(!mtCompFloat(vx1, vx2) || !mtCompFloat(vy1, vy2)) {
                    //graphic->addLine(vx1, vy1, vx2, vy2, currentLayerNum, add);
                    graphic->setActivePen(pen);
					graphic->addEntity(new RS_Line{graphic,
												   {vx1, vy1}, {vx2, vy2}});
                    //}
                }


                // ----
                // Arc:
                // ----
                else if(dxfLine=="ARC") {
                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine.size()) {
                                switch(code) {
                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;
                                case 10:  // Centre X
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcx = dxfLine.toDouble();
                                    break;
                                case 20:  // Centre Y
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcy = dxfLine.toDouble();
                                    break;
                                case 40:  // Radius
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcr = dxfLine.toDouble();
                                    break;
                                case 50:  // Start Angle
                                    dxfLine.replace( QRegExp(","), "." );
                                    va1 = RS_Math::correctAngle(dxfLine.toDouble()/ARAD);
                                    break;
                                case 51:  // End Angle
                                    dxfLine.replace( QRegExp(","), "." );
                                    va2 = RS_Math::correctAngle(dxfLine.toDouble()/ARAD);
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
                    //if(vcr>0.0 && !mtCompFloat(va1, va2)) {
                    //  graphic->addArc(vcx, vcy, vcr, va1, va2, false, currentLayerNum, add);
                    //}
                    graphic->setActivePen(pen);
                    graphic->addEntity(new RS_Arc(graphic,
                                                  RS_ArcData(RS_Vector(vcx, vcy),
                                                             vcr, va1, va2, false)));
                }

                // -------
                // Circle:
                // -------
                else if(dxfLine=="CIRCLE") {
                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine.size()) {
                                switch(code) {
                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;
                                case 10:  // Centre X
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcx = dxfLine.toDouble();
                                    break;
                                case 20:  // Centre Y
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcy = dxfLine.toDouble();
                                    break;
                                case 40:  // Radius
                                    dxfLine.replace( QRegExp(","), "." );
                                    vcr = dxfLine.toDouble();
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
                    /*if(vcr>0.0) {
                      graphic->addCircle(vcx, vcy, vcr, 0.0, 360.0, false, currentLayerNum, add);
                }*/
                    graphic->setActivePen(pen);
                    graphic->addEntity(new RS_Circle(graphic,
					{{vcx, vcy}, vcr}));
                }


                // ------
                // Hatch:
                // ------
                /*
                if(dxfLine=="HATCH") {
                  do {
                    dxfCode=getBufLine();
                    if(dxfCode) code=dxfCode.toInt();
                    if(dxfCode && code!=0) {
                      dxfLine=getBufLine();
                      if(dxfLine) {
                        switch(code) {
                          case  8:  // Layer
                          //  if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                              graphic->activateLayer(dxfLine);
                              //lastLayer=dxfLine;
                            //}
                            break;
                          case 10:  // X1
                            vx1 = dxfLine.toDouble();
                            break;
                          case 20:  // Y1
                            vy1 = dxfLine.toDouble();
                            //graphic->Vec[vc].CreatePoint(vy1, vx1, currentLayerNum);
                            //if(vc<vElements-1) ++vc;
                            break;
                          case 11:  // X2
                            vx2 = dxfLine.toDouble();
                            break;
                          case 21:  // Y2
                            vy2 = dxfLine.toDouble();
                            //graphic->Vec[vc].CreatePoint(vy2, vx2, currentLayerNum);
                            //if(vc<vElements-1) ++vc;
                            break;
                          default:
                            break;
                        }
                      }
                    }
                  }while(dxfCode && code!=0);
                  / *
                  if(!mt.CompFloat(vx1, vx2) || !mt.CompFloat(vy1, vy2)) {
                    graphic->Vec[vc].CreateLine(vx1, vy1, vx2, vy2, currentLayerNum);
                    if(vc<vElements-1) ++vc;
                  }
                  if(++updProgress==1000) {
                    np->getStateWin()->UpdateProgressBar((int)(pcFact*vc)+25);
                    updProgress=0;
                  }
                  * /
            }
                */


                // -----
                // Text:
                // -----
                else if(dxfLine=="TEXT") {

                    QString vtext;          // the text
                    char  vtextStyle[256];  // text style (normal_ro, cursive_ri, normal_st, ...)
                    double vheight=10.0;     // text height
                    double vtextAng=0.0;     // text angle
                    //double vradius=0.0;      // text radius
                    //double vletterspace=2.0; // Text letter space
                    //double vwordspace=6.0;   // Text wordspace
                    QString vfont;         // font "normal", "cursive", ...
                    RS_MTextData::HAlign vhalign=RS_MTextData::HALeft;
                    // alignment (0=left, 1=center, 2=right)
                    //int   vattachement=7;   // 1=top left, 2, 3, 4, 5, 6, 7, 8, 9=bottom right
                    //unsigned  vfl=0;            // special flags
//RLZ: unused                    bool  codeSeven=false;  // Have we found a code seven?

                    vtextStyle[0] = '\0';
                    vfont="normal";

                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size())
                            code=dxfCode.toInt();
                        if(dxfCode.size() && code!=0) {
                            if(code!=1 && code!=3 && code!=7)
                                dxfLine=getBufLine();
                            if(dxfLine.size() || code==1 || code==3 || code==7) {

                                switch(code) {

                                case  1:  // Text itself
                                    vtext=getBufLine();
                                    strDecodeDxfString(vtext);
                                    break;

                                case  3:  // Text parts (always 250 chars)
                                    vtext=getBufLine();
                                    break;

                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;

                                case  7:  
								    // Text style (normal_ro#50.0, 
									//    cursive_ri#20.0, normal_st)
                                    qstrncpy(vtextStyle, getBufLine().toLatin1().data(), 249);

                                    // get font typ:
                                    //
                                    {
                                        char dummy[256];
                                        sscanf(vtextStyle, "%[^_#\n]", dummy);
                                        vfont=dummy;
                                    }

                                    // get text style:
                                    //
                                    /*
                                                               if(strstr(vtextStyle, "_ro"))
                                                                   vfl=vfl|E_ROUNDOUT;
                                                               else if(strstr(vtextStyle, "_ri"))
                                                                   vfl=vfl|E_ROUNDIN;
                                                               else
                                                                   vfl=vfl|E_STRAIGHT;
                                    */
									

                                    /*if(strstr(vtextStyle, "_fix")) {
                                        vfl=vfl|E_FIXEDWIDTH;
                                }*/

                                    // get radius, letterspace, wordspace:
                                    //
                                    {
                                        char *ptr;  // pointer to value
                                        ptr = strchr(vtextStyle, '#');
                                        if(ptr) {
                                            // Parse radius
                                            /*if(vfl&E_ROUNDOUT || vfl&E_ROUNDIN) {
                                                ++ptr;
                                                if(ptr[0]) {
                                                    sscanf(ptr, "%lf", &vradius);
                                                }
                                                ptr = strchr(ptr, '#');
                                        }*/
                                            /*if(ptr) {
                                                // Parse letter space:
                                                ++ptr;
                                                if(ptr[0]) {
                                                    sscanf(ptr, "%lf", &vletterspace);
                                                }
                                                // Parse word space:
                                                ptr = strchr(ptr, '#');
                                                if(ptr) {
                                                    ++ptr;
                                                    if(ptr[0]) {
                                                        sscanf(ptr, "%lf", &vwordspace);
                                                    }
                                                }
                                        }*/
                                        }
                                    }
//RLZ: unused                                    codeSeven=true;
                                    break;

                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;

                                case 10:  // X1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vx1 = dxfLine.toDouble();
                                    break;
                                case 20:  // Y1
                                    dxfLine.replace( QRegExp(","), "." );
                                    vy1 = dxfLine.toDouble();
                                    break;
                                case 40:  // height
                                    dxfLine.replace( QRegExp(","), "." );
                                    vheight = dxfLine.toDouble();
                                    /*if(!codeSeven) {
                                        vletterspace = vheight*0.2;
                                        vwordspace = vheight*0.6;
                                }*/
                                    break;
                                case 50:  // angle
                                    dxfLine.replace( QRegExp(","), "." );
                                    vtextAng = dxfLine.toDouble() / ARAD;
                                    break;
                                case 72:  {// alignment
                                        //if(!mtext) {
                                        int v = dxfLine.toInt();
                                        if(v==1)
                                            vhalign = RS_MTextData::HACenter;
                                        else if(v==2)
                                            vhalign = RS_MTextData::HARight;
                                        else
                                            vhalign = RS_MTextData::HALeft;
                                        //}
                                    }
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
                    char* i=strchr(vtextStyle, '#');
                    if (i) {
                        i[0] = '\0';
                    }
                    graphic->addEntity(
                        new RS_MText(graphic,
                                    RS_MTextData(
                                        RS_Vector(vx1, vy1),
                                        vheight,
                                        100.0,
                                        RS_MTextData::VABottom,
                                        vhalign,
                                        RS_MTextData::LeftToRight,
                                        RS_MTextData::Exact,
                                        1.0,
                                        vtext,
                                        vtextStyle,
                                        vtextAng
                                    )
                                   )
                    );
                }

                // ----------
                // Dimension:
                // ----------
                else if(dxfLine=="DIMENSION") {
                    int typ=1;
                    double v10=0.0, v20=0.0;
                    double v13=0.0, v23=0.0;
                    double v14=0.0, v24=0.0;
                    double v15=0.0, v25=0.0;
                    double v16=0.0, v26=0.0;
                    double v40=0.0, v50=0.0;
                    QString dimText;
                    do {
                        dxfCode=getBufLine();
                        if(dxfCode.size()) {
                            code=dxfCode.toInt();
                        }
                        if(dxfCode.size() && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine.size()) {
                                switch(code) {
                                case  1:  // Text (if any)
                                    dimText=dxfLine;

                                    // Mend unproper savings of older versions:
                                    if(dimText==" " || dimText==";;")
                                        dimText="";

                                    //else dimText.replace(QRegExp("%%c"), "¯");
                                    else
                                        strDecodeDxfString(dimText);
                                    break;
                                case  6:  // style
                                    pen.setLineType(RS_FilterDXFRW::nameToLineType(dxfLine));
                                    break;
                                case  8:  // Layer
                                    //if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                    //}
                                    break;
                                case 10:  // line position x
                                    dxfLine.replace( QRegExp(","), "." );
                                    v10 = dxfLine.toDouble();
                                    break;
                                case 20:  // line position y
                                    dxfLine.replace( QRegExp(","), "." );
                                    v20 = dxfLine.toDouble();
                                    break;
                                case 13:  // X1
                                    dxfLine.replace( QRegExp(","), "." );
                                    v13 = dxfLine.toDouble();
                                    break;
                                case 23:  // Y1
                                    dxfLine.replace( QRegExp(","), "." );
                                    v23 = dxfLine.toDouble();
                                    break;
                                case 14:  // X2
                                    dxfLine.replace( QRegExp(","), "." );
                                    v14 = dxfLine.toDouble();
                                    break;
                                case 24:  // Y2
                                    dxfLine.replace( QRegExp(","), "." );
                                    v24 = dxfLine.toDouble();
                                    break;
                                case 15:  // X3
                                    dxfLine.replace( QRegExp(","), "." );
                                    v15 = dxfLine.toDouble();
                                    break;
                                case 25:  // Y3
                                    dxfLine.replace( QRegExp(","), "." );
                                    v25 = dxfLine.toDouble();
                                    break;
                                case 16:  // X4
                                    dxfLine.replace( QRegExp(","), "." );
                                    v16 = dxfLine.toDouble();
                                    break;
                                case 26:  // Y4
                                    dxfLine.replace( QRegExp(","), "." );
                                    v26 = dxfLine.toDouble();
                                    break;
                                case 40:
                                    dxfLine.replace( QRegExp(","), "." );
                                    v40 = dxfLine.toDouble();
                                    break;
                                case 50:
                                    dxfLine.replace( QRegExp(","), "." );
                                    v50 = dxfLine.toDouble();
                                    break;
                                case 70:  // Typ
                                    typ = dxfLine.toInt();
                                    break;
                                case 39:  // Thickness
                                    pen.setWidth(numberToWidth(dxfLine.toInt()));
                                    break;
                                case 62:  // Color
                                    pen.setColor(RS_FilterDXFRW::numberToColor(dxfLine.toInt()));
                                    break;

                                default:
                                    break;
                                }
                            }
                        }
                    } while(dxfCode.size() && code!=0);
					
                    //double dist;

                    // Remove Bit values:
                    if(typ>=128) {
                        typ-=128;   // Location of Text
					}
                    if(typ>= 64) {
                        typ-= 64;   // Ordinate
					}

                    switch(typ) {
                        // Horiz. / vert.:
                    case 0: {
                            RS_DimLinear* d =
                                new RS_DimLinear(
                                    graphic,
                                    RS_DimensionData(
                                        RS_Vector(v10, v20),
                                        RS_Vector(0.0, 0.0),
                                        RS_MTextData::VABottom,
                                        RS_MTextData::HACenter,
                                        RS_MTextData::Exact,
                                        1.0,
                                        dimText,
                                        "ISO-25",
                                        0.0
                                    ),
                                    RS_DimLinearData(
                                        RS_Vector(v13, v23),
                                        RS_Vector(v14, v24),
                                        v50/ARAD,
                                        0.0
                                    )
                                );
                            d->update();
                            graphic->addEntity(d);
                        }
                        break;

                        // Aligned:
                    case 1: {
                            double angle =
                                RS_Vector(v13, v23).angleTo(RS_Vector(v10,v20));
                            double dist =
                                RS_Vector(v13, v23).distanceTo(RS_Vector(v10,v20));

							RS_Vector defP = RS_Vector::polar(dist, angle);
                            defP+=RS_Vector(v14, v24);

                            RS_DimAligned* d =
                                new RS_DimAligned(
                                    graphic,
                                    RS_DimensionData(
                                        defP,
                                        RS_Vector(0.0, 0.0),
                                        RS_MTextData::VABottom,
                                        RS_MTextData::HACenter,
                                        RS_MTextData::Exact,
                                        1.0,
                                        dimText,
                                        "ISO-25",
                                        0.0
                                    ),
                                    RS_DimAlignedData(
                                        RS_Vector(v13, v23),
                                        RS_Vector(v14, v24)
                                    )
                                );
                            d->update();
                            graphic->addEntity(d);
                        }
                        break;

                        // Angle:
                    case 2: {
							RS_Line tl1{{v13, v23}, {v14, v24}};
							RS_Line tl2{{v10, v20}, {v15, v25}};

                            //bool inters=false;
                            //tmpEl1.getIntersection(&tmpEl2,
                            //                       &inters, &vcx, &vcy, 0,0,0,0, false);
							RS_VectorSolutions const& s = RS_Information::getIntersection(
                                    &tl1, &tl2, false);

                            if (s.get(0).valid) {
                                vcx = s.get(0).x;
                                vcy = s.get(0).y;
                                //vcr = RS_Vector(vcx, vcy).distanceTo(v16, v26);

                                /*if(RS_Vector(vcx,vcy).distanceTo(v13,v23)<vcr) {
                                    va1 = tl1.getAngle1();
                            } else {
                                    va1 = tl2.getAngle2();
                            }

                                if(RS_Vector(vcx,vcy).distanceTo(v10,v20)<vcr) {
                                    va2 = tl2.getAngle1();
                            } else {
                                    va2 = tl2.getAngle2();
                            }
                                */
                                /*
                                graphic->addDimension(vcx, vcy, va1, va2,
                                               mtGetDistance(vcx, vcy, v13, v23),
                                               mtGetDistance(vcx, vcy, v10, v20),
                                                      vcr,
                                                      E_ROUNDOUT,
                                                      currentLayerNum,
                                                      add);
                                */
                                //RS_Vector dp4;
                                //dp4.setPolar();
                                RS_DimAngular* d =
                                    new RS_DimAngular(
                                        graphic,
                                        RS_DimensionData(
                                            RS_Vector(v10, v20),
                                            RS_Vector(0.0, 0.0),
                                            RS_MTextData::VABottom,
                                            RS_MTextData::HACenter,
                                            RS_MTextData::Exact,
                                            1.0,
                                            dimText,
                                            "ISO-25",
                                            0.0
                                        ),
                                        RS_DimAngularData(
                                            RS_Vector(v13, v23),
                                            RS_Vector(vcx, vcy),
                                            RS_Vector(vcx, vcy),
                                            RS_Vector(v16, v26)
                                        )
                                    );
                                d->update();
                                graphic->addEntity(d);
                            }
                        }
                        break;

                        // Radius:
                    case 4: {
                            /*
                                                graphic->addDimension(v10, v20, v15, v25,
                                                                    0.0, 0.0,
                                                                                      v40,
                                                                                      E_STRAIGHT|E_RADIUS,
                                                                                      currentLayerNum,
                                                                                      add);
                            									*/

                            double ang =
                                RS_Vector(v10, v20)
                                .angleTo(RS_Vector(v15, v25));
							RS_Vector v2 = RS_Vector::polar(v40, ang);
                            RS_DimRadial* d =
                                new RS_DimRadial(
                                    graphic,
                                    RS_DimensionData(
                                        RS_Vector(v10, v20),
                                        RS_Vector(0.0, 0.0),
                                        RS_MTextData::VABottom,
                                        RS_MTextData::HACenter,
                                        RS_MTextData::Exact,
                                        1.0,
                                        dimText,
                                        "ISO-25",
                                        0.0
                                    ),
                                    RS_DimRadialData(
                                        RS_Vector(v10, v20) + v2,
                                        0.0
                                    )
                                );
                            d->update();
                            graphic->addEntity(d);
                        }
                        break;

                        // Arrow:
                    case 7: {
                            /*
                                           graphic->addDimension(v13, v23, v14, v24,
                                                                 0.0, 0.0, 0.0,
                                                                 E_STRAIGHT|E_ARROW,
                                                                 currentLayerNum,
                                                                 add);
                            */
							/*
                            double ang =
                                RS_Vector(v10, v20)
                                .angleTo(RS_Vector(v15, v25));
                            RS_Vector v2;
                            v2.setPolar(v40, ang);
                            RS_DimDiametric* d =
                                new RS_DimDiametric(
                                    graphic,
                                    RS_DimensionData(
                                        RS_Vector(v10, v20),
                                        RS_Vector(0.0, 0.0),
                                        RS2::VAlignBottom,
                                        RS2::HAlignCenter,
                                        RS2::Exact,
                                        1.0,
                                        dimText,
                                        "ISO-25",
                                        0.0
                                    ),
                                    RS_DimDiametricData(
                                        RS_Vector(v10, v20) + v2,
                                        0.0
                                    )
                                );
                            d->update();
                            graphic->addEntity(d);
							*/
							RS_LeaderData data(true);
							RS_Leader* d = 
								new RS_Leader(graphic, data);
							d->addVertex(RS_Vector(v14, v24));
							d->addVertex(RS_Vector(v10, v20));
							d->update();
							graphic->addEntity(d);
                        }
                        break;
                    }
                    //graphic->elementCurrent()->setText(dimText);
                }



                // ---------
                // Hatching:
                // ---------
                /*
                      else if(dxfLine=="HATCH") {
                        QString patternName="45";
                        double patternScale=1.0;
                        //int numPaths=1;
                        //int numEdges=1;
                        int nextObjectTyp=T_LINE;
                        double v10=0.0, v20=0.0,
                              v11=0.0, v21=0.0,
                              v40=0.0, v50=0.0,
                              v51=0.0;
                        do {
                          dxfCode=getBufLine();
                          if(dxfCode) code=dxfCode.toInt();
                          if(dxfCode && code!=0) {
                            dxfLine=getBufLine();
                            if(dxfLine) {
                              switch(code) {
                                case  2:
                                  patternName = dxfLine;
                                  break;
                                case  6:  // style
                                  pen.setLineType(RS_FilterDXF::nameToLineType(dxfLine));
                                  break;
                                case  8:  // Layer
                                //  if(dxfLine!=lastLayer) {
									if (dxfLine=="(null)" || dxfLine=="default") {
										dxfLine = "0";
									}
                                    graphic->activateLayer(dxfLine);
                                    //lastLayer=dxfLine;
                                  //}
                                  break;
                                case 10:  // Start point/center of boundary line/arc
                                  dxfLine.replace( QRegExp(","), "." );
                                  v10=dxfLine.toDouble();
                                  break;
                                case 20:  // Start point/center of boundary line/arc
                                  dxfLine.replace( QRegExp(","), "." );
                                  v20=dxfLine.toDouble();
                                  break;
                                case 11:  // End point of boundary line
                                  dxfLine.replace( QRegExp(","), "." );
                                  v11=dxfLine.toDouble();
                                  break;
                                case 21:  // End point of boundary line
                                  dxfLine.replace( QRegExp(","), "." );
                                  v21=dxfLine.toDouble();
                                  if(nextObjectTyp==T_LINE) {
                                    int elnu=graphic->addLine(v10, v20, v11, v21, currentLayerNum, add);
                                    graphic->elementAt(elnu)->setFlag(E_TAGGED);
                                  }
                                  break;
                                case 40:  // Radius of boundary entity
                                  dxfLine.replace( QRegExp(","), "." );
                                  v40=dxfLine.toDouble();
                                  break;
                                case 50:  // Start angle
                                  dxfLine.replace( QRegExp(","), "." );
                                  v50=dxfLine.toDouble();
                                  break;
                                case 51:  // End angle
                                  dxfLine.replace( QRegExp(","), "." );
                                  v51=dxfLine.toDouble();
                                  break;
                                case 73:  // Counterclockwise?
                                  if(nextObjectTyp==T_ARC) {
                                    int elnu;
                                    if( mtCompFloat( v50, 0.0 ) && mtCompFloat( v51, 0.0 ) ) {
                                      elnu=graphic->addCircle(v10, v20, v40, 0.0, 360.0, (bool)dxfLine.toInt(), currentLayerNum, add);
                                    }
                                    else {
                                      elnu=graphic->addArc(v10, v20, v40, v50, v51, (bool)dxfLine.toInt(), currentLayerNum, add);
                                    }
                                    graphic->elementAt(elnu)->setFlag(E_TAGGED);
                                    //newEl = new RElement( graphic );
                                    //newEl->createArc(v10, v20, v40, v50, v51, (bool)dxfLine.toInt());
                                    //boundaryList.append(newEl);
                                  }
                                  break;
                                case 41:  // Scale
                                  dxfLine.replace( QRegExp(","), "." );
                                  patternScale=dxfLine.toDouble();
                                  break;
                                case 52:  // Angle

                                  break;
                                case 70:  // Solid (=1) or pattern (=0)

                                  break;
                                case 39:  // Thickness
                                  pen.setWidth(RS_FilterDXF::numberToWidth(dxfLine.toInt()));
                                  break;
                                case 62:  // Color
                                  pen.setColor(RS_FilterDXF::numberToColor(dxfLine.toInt()));
                                  break;
                                case 91:  // Number of boundary paths (loops)
                                  //numPaths=dxfLine.toInt();
                                  break;
                                case 92:  // Typ of boundary

                                  break;
                                case 93:  // Number of edges in this boundary
                                  //numEdges=dxfLine.toInt();
                                  break;
                                case 72:  // Edge typ
                                  switch(dxfLine.toInt()) {
                                    case 1: nextObjectTyp=T_LINE; break;
                                    case 2: nextObjectTyp=T_ARC;  break;
                                    default: break;
                                  }
                                  break;

                                default:
                                  break;
                              }
                            }
                          }
                        }while(dxfCode && code!=0);

                        graphic->addHatching(patternScale,
                                             patternName,
                                             currentLayerNum,
                                             add);

                        graphic->editDelete(false);

                      }
                */

            }
        }
        while(dxfLine.size() && dxfLine!="EOF");

        //graphic->terminateAction();

        //graphic->debugElements();

        ret=true;
    } else {
        ret=false;
    }

    return ret;
}






/**
 * Resets  the whole object
 *   (base class too)
 */
void RS_FilterDXF1::reset() {
    file.reset();

    delBuffer();
    fBufP=0;
    fSize=0;
    if(fPointer) {
        fclose(fPointer);
        fPointer=0;
    }
}



/**
 * Reset buffer pointer to the beginning of the buffer:
 */
void RS_FilterDXF1::resetBufP() {
    fBufP=0;
}



/**
 * Set buffer pointer to the given index:
 */
void RS_FilterDXF1::setBufP(int _fBufP) {
    if(_fBufP<(int)fSize) {
        fBufP = _fBufP;
    }
}


/**
 * delete buffer:
 */
void RS_FilterDXF1::delBuffer() {
    if(fBuf) {
        delete[] fBuf;
        fBuf=0;
    }
}



/**
 * Remove any 13-characters in the buffer:
 */
void RS_FilterDXF1::dos2unix() {
    char *src = fBuf, *dst = fBuf;

    if (!fBuf)
        return;

    while (*src != '\0') {
        if (*src == '\r') {
            dosFile = true;
        } else {
            *dst++ = *src;
        }
        src++;
    }

    *dst = '\0';
}


// Get next line in the buffer:
//   and overread ALL separators
//
// return:  -Null-string: end of buffer
//          -String which is the next line in buffer
//
QString RS_FilterDXF1::getBufLine() {
    char *ret;
    QString str;

    if (fBufP >= (int)fSize)
        return QString::null;

    ret = &fBuf[fBufP];

    // Skip empty lines
    /*if (*ret == '\0' && noEmptyLines) {
        while (++fBufP < (int)fSize && fBuf[fBufP] == '\0')
            ;
        if (fBufP >= (int)fSize)
            return QString::null;
        ret = &fBuf[fBufP];
}*/

    // Move fBufP pointer to the next line
    while (fBufP < (int)fSize && fBuf[fBufP++] != '\0')
        ;

    str = QString::fromLocal8Bit(ret).simplified();

    if (str.isNull()) {
        return "";
    } else {
        return str;
    }
}




// Get next line in the buffer:
//   and overread ALL separators
//
// return:  -Null-string: end of buffer
//          -String which is the next line in buffer
//
char* RS_FilterDXF1::getBufLineCh() {
    char *ret;

    if (fBufP >= (int)fSize)
        return 0;

    ret = &fBuf[fBufP];

    // Skip empty lines
    /*if (*ret == '\0' && noEmptyLines) {
        while (++fBufP < (int)fSize && fBuf[fBufP] == '\0')
            ;
        if (fBufP >= (int)fSize)
            return 0;
        ret = &fBuf[fBufP];
}*/

    // Move fBufP pointer to the next line
    while (fBufP < (int)fSize && fBuf[fBufP++] != '\0')
        ;

    return ret;
}



// Copy buffer from a given string:
//
void RS_FilterDXF1::copyBufFrom(const char* _buf) {
    if(_buf) {
        fBuf = new char[strlen(_buf)+16];
        strcpy(fBuf, _buf);
    }
}



// Go to the next '_lstr'-line in buffer:
//
// return: true:  line found
//         false: end of buffer
//
bool RS_FilterDXF1::gotoBufLine(char* _lstr) {
    QString l;
    do {
        l=getBufLine();
    } while(!l.isNull() && l!=_lstr);

    if(!l.isNull())
        return true;
    return false;
}



// Goto next line where the string _lstr appears:
//
// return: true:  string in line found
//         false: end of buffer
//
//
bool RS_FilterDXF1::gotoBufLineString(char* _lstr) {
    QString l;
    do {
        l=getBufLine();
    } while(!l.isNull() && l.contains(_lstr));

    if(!l.isNull())
        return true;
    return false;
}



// Replace bynary Bytes (<32) by an other (given) byte:
//
void RS_FilterDXF1::replaceBinaryBytesBy(char _c) {
    int bc;

    for(bc=0; bc<(int)fSize; ++bc) {
        if(fBuf[bc]<32 && fBuf[bc]>=0) {
            fBuf[bc] = _c;
        }
    }
}



// Separate buffer (change chars sc1 and sc2 in '\0'
//
void RS_FilterDXF1::separateBuf(char _c1,
                                char _c2,
                                char _c3,
                                char _c4) {
    int bc;

    for(bc=0; bc<(int)fSize; ++bc) {
        if(fBuf[bc]==_c1 || fBuf[bc]==_c2 ||
                fBuf[bc]==_c3 || fBuf[bc]==_c4    ) {
            fBuf[bc] = '\0';
        }
    }
}



// remove comment between '_fc' and '_lc'
//   comments get replaced by '\0'
//
void RS_FilterDXF1::removeComment(char _fc, char _lc) {
    bool rem=false;   // Are we removing currrently?
    int bc;           // counter

    for(bc=0; bc<(int)fSize; ++bc) {
        if(fBuf[bc]==_fc)
            rem=true;
        if(fBuf[bc]==_lc) {
            fBuf[bc]='\0';
            rem=false;
        }
        if(rem)
            fBuf[bc]='\0';
    }
}



// Read file '_name' in buffer (buf)
//
// '_bNum' : Max number of Bytes
//         : -1: All
// return: true: successful
//         false: file not found
//
bool RS_FilterDXF1::readFileInBuffer(char* _name, int _bNum) {
    file.setFileName(_name);
    return readFileInBuffer(_bNum);
}



// Read file in buffer (buf)
//
// 'bNum' : Max number of Bytes
//        : -1: All
// return: true: successful
//         false: file not found
//
bool RS_FilterDXF1::readFileInBuffer(int _bNum) {
    fPointer = fopen(name.toLatin1().data(), "rb");//RLZ verify with locales
    if(fPointer) {
        if(file.open(fPointer, QIODevice::ReadOnly)) {
            fSize=file.size();
            if(_bNum==-1)
                _bNum=fSize;

            fBuf = new char[_bNum+16];

            file.read(fBuf, _bNum);
            fBuf[_bNum] = '\0';
            file.close();
        }
        fclose(fPointer);

        // Convert 13/10 to 10
        dos2unix();
		fPointer=nullptr;

        return true;
    }
    return false;
}



// Decode a DXF string to the C-convention (special character \P is a \n)
//
void RS_FilterDXF1::strDecodeDxfString(QString& str) {
    if (str.isEmpty())
        return;
    str.replace(QRegExp("%%c"), QChar(0xF8)); // Diameter
    str.replace(QRegExp("%%d"), QChar(0xB0)); // Degree
    str.replace(QRegExp("%%p"), QChar(0xB1)); // Plus/minus
    str.replace(QRegExp("\\\\[pP]"), QChar('\n'));
}




// Compare two double values:
//
// return: true: values are equal
//         false: values are not equal
//
bool RS_FilterDXF1::mtCompFloat(double _v1, double _v2, double _tol) {
    double delta = _v2-_v1;

    if(delta>-_tol && delta<_tol)
        return true;
    else
        return false;
}

/**
 * Converts a line width number (e.g. 1) into a RS2::LineWidth.
 */
RS2::LineWidth RS_FilterDXF1::numberToWidth(int num) {
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
int RS_FilterDXF1::widthToNumber(RS2::LineWidth width) {
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


// EOF











