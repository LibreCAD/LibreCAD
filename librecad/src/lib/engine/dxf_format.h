/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2022 P. Winters (polly.winters1@gmail.com)
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


#ifndef DXF_FORMAT_H
#define DXF_FORMAT_H

/**
 * Constants relating to the DXF file format.
 * Eg. Group codes for objects and entities that appear in DXF files.
 *
 * @author Polly Winters
 */

/*	Group code for comments  */
#define DXF_FORMAT_GC_Comment		999					/* Each comment starts with 999 followed by a line of arbitrary comment text */

/*	Sections of the DXF file  */
#define DXF_FORMAT_GC_Section		0					/* Each section starts with 0 followed by keyword SECTION */
#define DXF_FORMAT_Section_Keyword				"SECTION"			/* Section keyword */

#define DXF_FORMAT_GC_SectionName	2					/* Name of section is given by 2 followed by the section namestring */
#define DXF_FORMAT_SectionName_Header			"HEADER"			/* Header section name string */
#define DXF_FORMAT_SectionName_Classes			"CLASSES"			/* Classes section name string */
#define DXF_FORMAT_SectionName_Tables			"TABLES"			/* Tables section name string */
#define DXF_FORMAT_SectionName_Blocks			"BLOCKS"			/* Blocks section name string */
#define DXF_FORMAT_SectionName_Entities			"ENTITIES"			/* Entities section name string */
#define DXF_FORMAT_SectionName_Objects			"OBJECTS"			/* Objects section name string */
#define DXF_FORMAT_SectionName_Thumbnailimage	"THUMBNAILIMAGE"	/* Thumbnailimage section name string */

#define DXF_FORMAT_GC_EndSec		0					/* Each section ends with 0 followed by keyword ENDSEC */
#define DXF_FORMAT_EndSec_Keyword				"ENDSEC"			/* End of section keyword */

/*	HEADER section. The header section is introduced by section namestring = HEADER.
	The section contains a series of variable names and the value(s) of the variables. 
*/

/* Name of variable is given by 9 followed by the variable namestring */
#define DXF_FORMAT_GC_VarName		9

/* Maintenance version number (should be ignored) - 9, $ACADMAINTVER, 70, <integer> */
#define DXF_FORMAT_GC_AcadMaintVer	70

/* AutoCAD drawing database version number - 9, $ACADVER, 1, <string> */
#define DXF_FORMAT_GC_AcadVer		1
#define DXF_FORMAT_Ver_Acad2000			"AC1015"		/* $ACADVER value string, AutoCAD 2000 */ 
#define DXF_FORMAT_Ver_Acad2004			"AC1018"		/* $ACADVER value string, AutoCAD 2004 */ 
#define DXF_FORMAT_Ver_Acad2007			"AC1021"		/* $ACADVER value string, AutoCAD 2007 */ 
#define DXF_FORMAT_Ver_Acad2010			"AC1024"		/* $ACADVER value string, AutoCAD 2010 */ 

/* Angle 0 direction - 9, $ANGBASE, 50, <angle 0 direction, double> */
#define DXF_FORMAT_GC_AngBase		50

/* Attribute visibility - 9, $ATTMODE, 70, <integer> */
#define DXF_FORMAT_GC_AttMode		70
//0 = None, 1 = Normal, 2 = All

/* Units format for angles - 9, $AUNITS, 70, <integer> */
#define DXF_FORMAT_GC_AUnits		70

/* Units precision for angles - 9, $AUPREC, 70, <integer> */
#define DXF_FORMAT_GC_AUPrec		70

/* Current entity color number - 9, $CECOLOR, 62, <integer> */
#define DXF_FORMAT_GC_CEColor		62
//0 = BYBLOCK
//1 - 255 = colour number
//256 = BYLAYER

/* Current entity linetype scale - 9, $CELTSCALE, 40, <double> */
#define DXF_FORMAT_GC_CELTScale		40

/* Entity linetype name, or BYBLOCK or BYLAYER - 9, $CELTYPE, 6, <string> */
#define DXF_FORMAT_GC_CELType		6

/* Lineweight of new objects - 9, $CELWEIGHT, 370, <integer> */
#define DXF_FORMAT_GC_CELWeight		370

/* Plotstyle handle of new objects; if CEPSNTYPE is 3, then this value indicates the handle
   - 9, $CEPSNID, 390, <string> */
#define DXF_FORMAT_GC_CEPSNID		390

/* Plot style type of new objects - 9, $CEPSNTYPE, 380, <integer> */
#define DXF_FORMAT_GC_CEPSNType		380
//0 = Plot style by layer
//1 = Plot style by block
//2 = Plot style by dictionary default
//3 = Plot style by object ID/handle

/* First chamfer distance - 9, $CHAMFERA, 40, <double> */
#define DXF_FORMAT_GC_ChamferA		40

/* Second chamfer distance - 9, $CHAMFERB, 40, <double> */
#define DXF_FORMAT_GC_ChamferB		40

/* Chamfer length - 9, $CHAMFERC, 40, <double> */
#define DXF_FORMAT_GC_ChamferC		40

/* Chamfer angle - 9, $CHAMFERD, 40, <double> */
#define DXF_FORMAT_GC_ChamferD		40

/* Current layer name - 9, $CLAYER, 8, <string> */
#define DXF_FORMAT_GC_CLayer		8

/* Current multiline justification - 9, $CMLJUST, 70, <integer> */
#define DXF_FORMAT_GC_CMLJust		70
//0 = Top
//1 = Middle
//2 = Bottom

/* Current multiline scale - 9, $CMLSCALE, 40, <double> */
#define DXF_FORMAT_GC_CMLScale		40

/* Current multiline style name - 9, $CMLSTYLE, 2, <string> */
#define DXF_FORMAT_GC_CMLStyle		2

/* Shadow mode for a 3D object - 9, $CSHADOW, 280, <integer> */
#define DXF_FORMAT_GC_CShadow		280
//0 = Casts and receives shadows
//1 = Casts shadows
//2 = Receives shadows
//3 = Ignores shadows
//Note: Starting with AutoCAD 2016-based products, this variable is obsolete but still supported for backwards compatibility.

/* Number of precision places displayed in angular dimensions - 9, $DIMADEC, 70, <integer> */
#define DXF_FORMAT_GC_DimADec		70

/* Alternate unit dimensioning performed if nonzero - 9, $DIMALT, 70, <integer> */
#define DXF_FORMAT_GC_DimAlt		70

/* Alternate unit decimal places - 9, $DIMALTD, 70, <integer> */
#define DXF_FORMAT_GC_DimAltD		70

/* Alternate unit scale factor - 9, $DIMALTF, 40, <double> */
#define DXF_FORMAT_GC_DimAltF		40

/* Determines rounding of alternate units - 9, $DIMALTRND, 40, <double> */
#define DXF_FORMAT_GC_DimAltRnd		40

/* Number of decimal places for tolerance values of an alternate units dimension
   - 9, $DIMALTTD, 70, <integer> */
#define DXF_FORMAT_GC_DimAltTD		70

/* Controls suppression of zeros for alternate tolerance values - 9, $DIMALTTZ, 70, <integer> */
#define DXF_FORMAT_GC_DimAltTZ		70
//0 = Suppresses zero feet and precisely zero inches
//1 = Includes zero feet and precisely zero inches
//2 = Includes zero feet and suppresses zero inches
//3 = Includes zero inches and suppresses zero feet
//To suppress leading or trailing zeros, add the following values to one of the preceding values:
//4 = Suppresses leading zeros
//8 = Suppresses trailing zeros

/* Units format for alternate units of all dimension style family members except angular
   - 9, $DIMALTU, 70, <integer> */
#define DXF_FORMAT_GC_DimAltU		70
//1 = Scientific
//2 = Decimal
//3 = Engineering
//4 = Architectural (stacked)
//5 = Fractional (stacked)
//6 = Architectural
//7 = Fractional
//8 = Operating system defines the decimal separator and number grouping symbols

/* Controls suppression of zeros for alternate unit dimension values - 9, $DIMALTZ, 70, <integer> */
#define DXF_FORMAT_GC_DimAltZ		70
//0 = Suppresses zero feet and precisely zero inches
//1 = Includes zero feet and precisely zero inches
//2 = Includes zero feet and suppresses zero inches
//3 = Includes zero inches and suppresses zero feet
//4 = Suppresses leading zeros in decimal dimensions
//8 = Suppresses trailing zeros in decimal dimensions
//12 = Suppresses both leading and trailing zeros

/* Alternate dimensioning suffix - 9, $DIMAPOST, 1, <string> */
#define DXF_FORMAT_GC_DimAPost		1

/* Associative dimensioning control - 9, $DIMASO, 70, <integer> 
   Note: Obsolete; see $DIMASSOC */
#define DXF_FORMAT_GC_DimAso		70
//1 = Create associative dimensioning
//0 = Draw individual entities

/* Controls the associativity of dimension objects - 9, $DIMASSOC, 280, <integer> */
#define DXF_FORMAT_GC_DimAssoc		280
//0 = Creates exploded dimensions; there is no association between elements of the dimension, and the lines, arcs, arrowheads, and text of a dimension are drawn as separate objects
//1 = Creates non-associative dimension objects; the elements of the dimension are formed into a single object, and if the definition point on the object moves, then the dimension value is updated
//2 = Creates associative dimension objects; the elements of the dimension are formed into a single object and one or more definition points of the dimension are coupled with association points on geometric objects

/* Dimensioning arrow size - 9, $DIMASZ, 40, <double> */
#define DXF_FORMAT_GC_DimASz		40

/* Controls dimension text and arrow placement when space is not sufficient to place both within the extension lines
   - 9, $DIMATFIT, 70, <integer> */
#define DXF_FORMAT_GC_DimATFit		70
//0 = Places both text and arrows outside extension lines
//1 = Moves arrows first, then text
//2 = Moves text first, then arrows
//3 = Moves either text or arrows, whichever fits best
//AutoCAD adds a leader to moved dimension text when DIMTMOVE is set to 1

/* Angle format for angular dimensions - 9, $DIMAUNIT, 70, <integer> */
#define DXF_FORMAT_GC_DimAUnit		70
//0 = Decimal degrees
//1 = Degrees/minutes/seconds
//2 = Gradians
//3 = Radians
//4 = Surveyors units

/* Controls suppression of zeros for angular dimensions - 9, $DIMAZIN, 70, <integer> */
#define DXF_FORMAT_GC_DimAZIn		70
//0 = Displays all leading and trailing zeros
//1 = Suppresses leading zeros in decimal dimensions
//2 = Suppresses trailing zeros in decimal dimensions
//3 = Suppresses leading and trailing zeros

/* Arrow block name - 9, $DIMBLK, 1, <string> */
#define DXF_FORMAT_GC_DimBlk		1

/* First arrow block name - 9, $DIMBLK1, 1, <string> */
#define DXF_FORMAT_GC_DimBlk1		1

/* Second arrow block name - 9, $DIMBLK2, 1, <string> */
#define DXF_FORMAT_GC_DimBlk2		1

/* Size of center mark/lines - 9, $DIMCEN, 40, <double> */
#define DXF_FORMAT_GC_DimCen		40

/* Dimension line color - 9, $DIMCLRD, 70, <integer> */
#define DXF_FORMAT_GC_DimClrD		70
//0 = BYBLOCK
//1 - 255 = colour number
//256 = BYLAYER

/* Dimension extension line color - 9, $DIMCLRE, 70, <integer> */
#define DXF_FORMAT_GC_DimClrE		70
//0 = BYBLOCK
//1 - 255 = colour number
//256 = BYLAYER

/* Dimension text color - 9, $DIMCLRT, 70, <integer> */
#define DXF_FORMAT_GC_DimClrT		70
//0 = BYBLOCK
//1 - 255 = colour number
//256 = BYLAYER

/* Number of decimal places for the tolerance values of a primary units dimension
   - 9, $DIMDEC, 70, <integer> */
#define DXF_FORMAT_GC_DimDec		70

/* Dimension line extension - 9, $DIMDLE, 40, <double> */
#define DXF_FORMAT_GC_DimDLE		40

/* Dimension line increment - 9, $DIMDLI, 40, <double> */
#define DXF_FORMAT_GC_DimDLI		40

/* Single-character decimal separator used when creating dimensions whose unit format is decimal
   - 9, $DIMDSEP, 70, <integer> */
#define DXF_FORMAT_GC_DimDSep		70

/* Extension line extension - 9, $DIMEXE, 40, <double> */
#define DXF_FORMAT_GC_DimEXE		40

/* Extension line offset - 9, $DIMEXO, 40, <double> */
#define DXF_FORMAT_GC_DimExO		40

/* Scale factor used to calculate the height of text for dimension fractions and tolerances. 
   AutoCAD multiplies DIMTXT by DIMTFAC to set the fractional or tolerance text height.
   - 9, $DIMFAC, 40, <double> */
#define DXF_FORMAT_GC_DimFac		40

/* Dimension line gap - 9, $DIMGAP, 40, <double> */
#define DXF_FORMAT_GC_DimGap		40

/* Horizontal dimension text position - 9, $DIMJUST, 70, <integer> */
#define DXF_FORMAT_GC_DimJust		70
//0 = Above dimension line and center-justified between extension lines
//1 = Above dimension line and next to first extension line
//2 = Above dimension line and next to second extension line
//3 = Above and center-justified to first extension line
//4 = Above and center-justified to second extension line

/* Arrow block name for leaders - 9, $DIMLDRBLK, 1, <string> */
#define DXF_FORMAT_GC_DimLdrBlk		1

/* Linear measurements scale factor - 9, $DIMLFAC, 40, <double> */
#define DXF_FORMAT_GC_DimLFac		40

/* Dimension limits generated if nonzero - 9, $DIMLIM, 70, <integer> */
#define DXF_FORMAT_GC_DimLim		70

/* Sets units for all dimension types except Angular - 9, $DIMLUNIT, 70, <integer> */
#define DXF_FORMAT_GC_DimLUnit		70
//1 = Scientific
//2 = Decimal
//3 = Engineering
//4 = Architectural
//5 = Fractional
//6 = Operating system

/* Dimension line lineweight - 9, $DIMLWD, 70, <integer> */
#define DXF_FORMAT_GC_DimLWD		70
//-3 = Standard
//-2 = ByLayer
//-1 = ByBlock
//0-211 = an integer representing 100th of mm

/* Extension line lineweight - 9, $DIMLWE, 70, <integer> */
#define DXF_FORMAT_GC_DimLWE		70
//-3 = Standard
//-2 = ByLayer
//-1 = ByBlock
//0-211 = an integer representing 100th of mm

/* General dimensioning suffix - 9, $DIMPOST, 1, <string> */
#define DXF_FORMAT_GC_DimPost		1

/* Rounding value for dimension distances - 9, $DIMRND, 40, <double> */
#define DXF_FORMAT_GC_DimRnd		40

/* Use separate arrow blocks if nonzero - 9, $DIMSAH, 70, <integer> */
#define DXF_FORMAT_GC_DimSAH		70

/* Overall dimensioning scale factor - 9, $DIMSCALE, 40, <double> */
#define DXF_FORMAT_GC_DimScale		40

/* Suppression of first extension line - 9, $DIMSD1, 70, <integer> */
#define DXF_FORMAT_GC_DimSD1		70
//0 = Not suppressed
//1 = Suppressed

/* Suppression of second extension line - 9, $DIMSD2, 70, <integer> */
#define DXF_FORMAT_GC_DimSD2		70
//0 = Not suppressed
//1 = Suppressed

/* First extension line suppressed if nonzero - 9, $DIMSE1, 70, <integer> */
#define DXF_FORMAT_GC_DimSE1		70

/* Second extension line suppressed if nonzero - 9, $DIMSE2, 70, <integer> */
#define DXF_FORMAT_GC_DimSE2		70

/* Dimensions recompute control - 9, $DIMSHO, 70, <integer> */
#define DXF_FORMAT_GC_DimSho		70
//1 = Recompute dimensions while dragging
//0 = Drag original image

/* Suppress outside-extensions dimension lines if nonzero - 9, $DIMSOXD, 70, <integer> */
#define DXF_FORMAT_GC_DimSOXD		70

/* Dimension style name - 9, $DIMSTYLE, 2, <string> */
#define DXF_FORMAT_GC_DimStyle		2

/* Text above dimension line if nonzero - 9, $DIMTAD, 70, <integer> */
#define DXF_FORMAT_GC_DimTAD		70

/* Number of decimal places to display the tolerance values - 9, $DIMTDEC, 70, <integer> */
#define DXF_FORMAT_GC_DimTDec		70

/* Dimension tolerance display scale factor - 9, $DIMTFAC, 40, <double> */
#define DXF_FORMAT_GC_DimTFac		40

/* Text inside horizontal if nonzero - 9, $DIMTIH, 70, <integer> */
#define DXF_FORMAT_GC_DimTIH		70

/* Force text inside extensions if nonzero - 9, $DIMTIX, 70, <integer> */
#define DXF_FORMAT_GC_DimTIX		70

/* Minus tolerance - 9, $DIMTM, 40, <double> */
#define DXF_FORMAT_GC_DimTM			40

/* Dimension text movement rules - 9, $DIMTMOVE, 70, <integer> */
#define DXF_FORMAT_GC_DimTMove		70
//0 = Moves the dimension line with dimension text
//1 = Adds a leader when dimension text is moved
//2 = Allows text to be moved freely without a leader

/* If text is outside the extension lines, dimension lines are forced between the extension lines if nonzero
   - 9, $DIMTOFL, 70, <integer> */
#define DXF_FORMAT_GC_DimTOFL		70

/* Text outside horizontal if nonzero - 9, $DIMTOH, 70, <integer> */
#define DXF_FORMAT_GC_DimTOH		70

/* Dimension tolerances generated if nonzero - 9, $DIMTOL, 70, <integer> */
#define DXF_FORMAT_GC_DimTol		70

/* Vertical justification for tolerance values - 9, $DIMTOLJ, 70, <integer> */
#define DXF_FORMAT_GC_DimTolJ		70
//0 = Top
//1 = Middle
//2 = Bottom

/* Plus tolerance - 9, $DIMTP, 40, <double> */
#define DXF_FORMAT_GC_DimTP			40

/* Dimensioning tick size - 9, $DIMTSZ, 40, <double> */
#define DXF_FORMAT_GC_DimTSz		40
//0 = Draws arrowheads
//>0 = Draws oblique strokes instead of arrowheads

/* Text vertical position - 9, $DIMTVP, 40, <double> */
#define DXF_FORMAT_GC_DimTVP		40

/* Dimension text style - 9, $DIMTXSTY, 7, <string> */
#define DXF_FORMAT_GC_DimTxSty		7

/* Dimensioning text height - 9, $DIMTXT, 40, <double> */
#define DXF_FORMAT_GC_DimTxt		40

/* Controls suppression of zeros for tolerance values - 9, $DIMTZIN, 70, <integer> */
#define DXF_FORMAT_GC_DimTZIn		70
//0 = Suppresses zero feet and precisely zero inches
//1 = Includes zero feet and precisely zero inches
//2 = Includes zero feet and suppresses zero inches
//3 = Includes zero inches and suppresses zero feet
//4 = Suppresses leading zeros in decimal dimensions
//8 = Suppresses trailing zeros in decimal dimensions
//12 = Suppresses both leading and trailing zeros

/* Cursor functionality for user-positioned text - 9, $DIMUPT, 70, <integer> */
#define DXF_FORMAT_GC_DimUPT		70
//0 = Controls only the dimension line location
//1 = Controls the text position as well as the dimension line location

/* Controls suppression of zeros for primary unit values - 9, $DIMZIN, 70, <integer> */
#define DXF_FORMAT_GC_DimZIn		70
//0 = Suppresses zero feet and precisely zero inches
//1 = Includes zero feet and precisely zero inches
//2 = Includes zero feet and suppresses zero inches
//3 = Includes zero inches and suppresses zero feet
//4 = Suppresses leading zeros in decimal dimensions
//8 = Suppresses trailing zeros in decimal dimensions
//12 = Suppresses both leading and trailing zeros

/* Controls the display of silhouette curves of body objects in Wireframe mode
   - 9, $DISPSILH, 70, <integer> */
#define DXF_FORMAT_GC_DispSilh		70
//0 = Off
//1 = On

/* Hard-pointer ID to visual style while creating 3D solid primitives. The default value is NULL.
   - 9, $349, xx, <hex string representing object ID> */
#define DXF_FORMAT_GC_DragVS		349

/* Drawing code page; set to the system code page when a new drawing is created, but not otherwise maintained by AutoCAD
   - 9, $DWGCODEPAGE, 3, <string> */
#define DXF_FORMAT_GC_DwgCodePage	3

/* Current elevation set by ELEV command - 9, $ELEVATION, 40, <double> */
#define DXF_FORMAT_GC_Elevation		40

/* Lineweight endcaps setting for new objects - 9, $ENDCAPS, 280, <integer> */
#define DXF_FORMAT_GC_Endcaps		280
//0 = None
//1 = Round
//2 = Angle
//3 = Square

/* Drawing extents upper-right corner (in WCS) - 9, $EXTMAX, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_ExtMax_X		10
#define DXF_FORMAT_GC_ExtMax_Y		20
#define DXF_FORMAT_GC_ExtMax_Z		30

/* Drawing extents lower-left corner (in WCS) - 9, $EXTMIN, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_ExtMin_X		10
#define DXF_FORMAT_GC_ExtMin_Y		20
#define DXF_FORMAT_GC_ExtMin_Z		30

/* Controls symbol table naming - 9, $EXTNAMES, 290, <boolean> */
#define DXF_FORMAT_GC_ExtNames		290
//0 = Release 14 compatibility. Limits names to 31 characters in length. Names can include the letters A to Z, the numerals 0 to 9, and the special characters dollar sign ($), underscore (_), and hyphen (-).
//1 = AutoCAD 2000. Names can be up to 255 characters in length, and can include the letters A to Z, the numerals 0 to 9, spaces, and any special characters not used for other purposes by Microsoft Windows and AutoCAD

/* Fillet radius - 9, $FILLETRAD, 40, <double> */
#define DXF_FORMAT_GC_FilletRad		40

/* Fill mode on if nonzero - 9, $FILLMODE, 70, <integer> */
#define DXF_FORMAT_GC_FillMode		70

/* Set at creation time, uniquely identifies a particular drawing - 9, $FINGERPRINTGUID, 2, <string> */
#define DXF_FORMAT_GC_FingerPrintGUID	2

/* Specifies a gap to be displayed where an object is hidden by another object; the value is specified
   as a percent of one unit and is independent of the zoom level. A haloed line is shortened at the 
   point where it is hidden when HIDE or the Hidden option of SHADEMODE is used
   - 9, $HALOGAP, 280, <integer> */
#define DXF_FORMAT_GC_HaloGap		280

/* Next available handle - 9, $HANDSEED, 5, <hex digit string> */
#define DXF_FORMAT_GCHandSeed		5

/* Specifies HIDETEXT system variable - 9, $HIDETEXT, 290, <boolean> */
#define DXF_FORMAT_GC_HideText		290
//0 = HIDE ignores text objects when producing the hidden view
//1 = HIDE does not ignore text objects

/* Path for all relative hyperlinks in the drawing. If null, the drawing path is used.
   - 9, $HYPERLINKBASE, 1, <string> */
#define DXF_FORMAT_GC_HyperlinkBase		1

/* Controls whether layer and spatial indexes are created and saved in drawing files
   - 9, $INDEXCTL, 280, <integer> */
#define DXF_FORMAT_GC_IndexCtl		280
//0 = No indexes are created
//1 = Layer index is created
//2 = Spatial index is created
//3 = Layer and spatial indexes are created

/*  Insertion base set by BASE command (in WCS) - 9, $INSBASE, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_InsBase_X		10
#define DXF_FORMAT_GC_InsBase_Y		20
#define DXF_FORMAT_GC_InsBase_Z		30

/* Default drawing units for AutoCAD DesignCenter blocks - 9, $INSUNITS, 70, <integer> */
#define DXF_FORMAT_GC_InsUnits		70
//0 = Unitless
//1 = Inches
//2 = Feet
//3 = Miles
//4 = Millimeters
//5 = Centimeters
//6 = Meters
//7 = Kilometers
//8 = Microinches
//9 = Mils
//10 = Yards
//11 = Angstroms
//12 = Nanometers
//13 = Microns
//14 = Decimeters
//15 = Decameters
//16 = Hectometers
//17 = Gigameters
//18 = Astronomical units
//19 = Light years
//20 = Parsecs
//21 = US Survey Feet
//22 = US Survey Inch
//23 = US Survey Yard
//24 = US Survey Mile

/* Represents the ACI color index of the "interference objects" created during the INTERFERE command. Default value is 1.
   - 9, $INTERFERECOLOR, 62, <integer> */
#define DXF_FORMAT_GC_InterfereColor		62

/* Hard-pointer ID to the visual style for interference objects. Default visual style is Conceptual.
   - 9, $INTERFEREOBJVS, 345, <hex string> */
#define DXF_FORMAT_GC_InterfereObjVS		345

/* Hard-pointer ID to the visual style for the viewport during interference checking. Default visual style is 3d Wireframe.
   - 9, $INTERFEREVPVS, 346, <hex string> */
#define DXF_FORMAT_GC_InterfereVPVS			346

/* Specifies the entity color of intersection polylines - 9, $INTERSECTIONCOLOR, 70, <integer> */
#define DXF_FORMAT_GC_IntersectionColor		70
//Values 1-255 designate an AutoCAD color index (ACI)
//0 = Color BYBLOCK
//256 = Color BYLAYER
//257 = Color BYENTITY

/* Specifies the display of intersection polylines - 9, $INTERSECTIONDISPLAY, 290, <boolean> */
#define DXF_FORMAT_GC_IntersectionDisplay	290
//0 = Turns off the display of intersection polylines
//1 = Turns on the display of intersection polylines

/* Lineweight joint setting for new objects - 9, $JOINSTYLE, 280, <integer> */
#define DXF_FORMAT_GC_JoinStyle		280
//0=None
//1= Round
//2 = Angle
//3 = Flat

/* Nonzero if limits checking is on - 9, $LIMCHECK, 70, <integer> */
#define DXF_FORMAT_GC_Limcheck		70

/* XY drawing limits upper-right corner (in WCS) - 9, $LIMMAX, 10, <double X>, 20, <double Y> */
#define DXF_FORMAT_GC_LimMax_X		10
#define DXF_FORMAT_GC_LimMax_Y		20

/* XY drawing limits lower-left corner (in WCS) - 9, $LIMMIN, 10, <double X>, 20, <double Y> */
#define DXF_FORMAT_GC_LimMin_X		10
#define DXF_FORMAT_GC_LimMin_Y		20

/* Global linetype scale - 9, $LTSCALE, 40, <double> */
#define DXF_FORMAT_GC_LTScale		40

/* Units format for coordinates and distances - 9, $LUNITS, 70, <integer> */
#define DXF_FORMAT_GC_LUnits		70

/* Units precision for coordinates and distances - 9, $LUPREC, 70, <integer> */
#define DXF_FORMAT_GC_LUPrec		70

/* Controls the display of lineweights on the Model or Layout tab - 9, $LWDISPLAY, 290, <boolean> */
#define DXF_FORMAT_GC_LWDisplay		290
//0 = Lineweight is not displayed
//1 = Lineweight is displayed

/* Sets maximum number of viewports to be regenerated - 9, $MAXACTVP, 70, <integer> */
#define DXF_FORMAT_GC_MaxActVP		70

/* Sets drawing units - 9, $MEASUREMENT, 70, <integer> */
#define DXF_FORMAT_GC_Measurement	70
//0 = English
//1 = Metric

/* Name of menu file - 9, $MENU, 1, <string> */
#define DXF_FORMAT_GC_Menu			1

/* Mirror text if nonzero - 9, $MIRRTEXT, 70, <integer> */
#define DXF_FORMAT_GC_MirrText		70

/* Specifies the color of obscured lines. An obscured line is a hidden line made visible by 
   changing its color and linetype and is visible only when the HIDE or SHADEMODE command 
   is used. The OBSCUREDCOLOR setting is visible only if the OBSCUREDLTYPE is turned ON by
   setting it to a value other than 0
   - 9, $OBSCOLOR, 70, <integer> */
#define DXF_FORMAT_GC_ObsColor		70
//0 and 256 = Entity color
//1-255 = An AutoCAD color index (ACI)

/* Specifies the linetype of obscured lines. Obscured linetypes are independent of zoom level,
   unlike regular AutoCAD linetypes. Value 0 turns off display of obscured lines and is the
   default. Linetype values are defined as follows.
   - 9, $OBSLTYPE, 280, <integer> */
#define DXF_FORMAT_GC_ObsLType		280
//0 = Off
//1 = Solid
//2 = Dashed
//3 = Dotted
//4 = Short Dash
//5 = Medium Dash
//6 = Long Dash
//7 = Double Short Dash
//8 = Double Medium Dash
//9 = Double Long Dash
//10 = Medium Long Dash
//11 = Sparse Dot

/* Ortho mode on if nonzero - 9, $ORTHOMODE, 70, <integer> */
#define DXF_FORMAT_GC_OrthoMode		70

/* Point display mode - 9, $PDMODE, 70, <integer>
   The mode values consist of a centre mark style, optionally plus a surrounding circle
   or square or both.                                                                   */
#define DXF_FORMAT_GC_PDMode		70
#define DXF_FORMAT_PDMode_CentreDot			0		/* . to mark centre of point */
#define DXF_FORMAT_PDMode_CentreBlank		1		/* no centre mark for point */
#define DXF_FORMAT_PDMode_CentrePlus		2		/* + to mark centre of point */
#define DXF_FORMAT_PDMode_CentreCross		3		/* X to mark centre of point */
#define DXF_FORMAT_PDMode_CentreTick		4		/* ' to mark centre of point */

#define DXF_FORMAT_PDMode_EncloseCircle(centre)			(centre|32)
#define DXF_FORMAT_PDMode_EncloseSquare(centre)			(centre|64)
#define DXF_FORMAT_PDMode_EncloseCircleSquare(centre)	(centre|96)

#define DXF_FORMAT_PDMode_getCentre(mode)				(mode&7)
#define DXF_FORMAT_PDMode_hasEncloseCircle(mode)		(mode&32)
#define DXF_FORMAT_PDMode_hasEncloseSquare(mode)		(mode&64)

/* Point display size - 9, $PDSIZE, 40, <double>
   Size values are interpreted as follows:
      0 - Creates a point at 5 percent of the viewport height
     >0 - Specifies an absolute size in current drawing units
     <0 - Specifies a percentage of the viewport size             */
#define DXF_FORMAT_GC_PDSize		40

#define DXF_FORMAT_PDSize_Absolute(size)		(size)
#define DXF_FORMAT_PDSize_Percent(size)			(- size)

#define DXF_FORMAT_PDSize_isAbsolute(size)		(size>=0)
#define DXF_FORMAT_PDSize_isPercent(size)		(size<0)

/* Current paper space elevation - 9, $PELEVATION, 40, <double> */
#define DXF_FORMAT_GC_PElevation	40

/* Maximum extents for paper space - 9, $PEXTMAX, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PExtMax_X		10
#define DXF_FORMAT_GC_PExtMax_Y		20
#define DXF_FORMAT_GC_PExtMax_Z		30

/* Minimum extents for paper space - 9, $PEXTMIN, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PExtMin_X		10
#define DXF_FORMAT_GC_PExtMin_Y		20
#define DXF_FORMAT_GC_PExtMin_Z		30

/* Paper space insertion base point - 9, $PINSBASE, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PInsBase_X	10
#define DXF_FORMAT_GC_PInsBase_Y	20
#define DXF_FORMAT_GC_PInsBase_Z	30

/* Limits checking in paper space when nonzero - 9, $PLIMCHECK, 70, <integer> */
#define DXF_FORMAT_GC_PLimCheck		70

/* Maximum X and Y limits in paper space - 9, $PLIMMAX, 10, <double X>, 20, <double Y> */
#define DXF_FORMAT_GC_PLimMax_X		10
#define DXF_FORMAT_GC_PLimMax_Y		20

/* Minimum X and Y limits in paper space - 9, $PLIMMIN, 10, <double X>, 20, <double Y> */
#define DXF_FORMAT_GC_PLimMin_X		10
#define DXF_FORMAT_GC_PLimMin_Y		20

/* Governs the generation of linetype patterns around the vertices of a 2D polyline
   - 9, $PLINEGEN, 70, <integer> */
#define DXF_FORMAT_GC_PLineGen		70
//1 = Linetype is generated in a continuous pattern around vertices of the polyline
//0 = Each segment of the polyline starts and ends with a dash

/* Default polyline width - 9, $PLINEWID, 40, <double> */
#define DXF_FORMAT_GC_PLineWid		40

/* Assigns a project name to the current drawing. Used when an external reference or
   image is not found on its original path. The project name points to a section in
   the registry that can contain one or more search paths for each project name defined.
   Project names and their search directories are created from the Files tab of the 
   Options dialog box.
   - 9, $PROJECTNAME, 1, <string> */
#define DXF_FORMAT_GC_ProjectName	1

/* Controls the saving of proxy object images - 9, $PROXYGRAPHICS, 70, <integer> */
#define DXF_FORMAT_GC_ProxyGraphics		70

/* Controls paper space linetype scaling - 9, $PSLTSCALE, 70, <integer> */
#define DXF_FORMAT_GC_PSLTScale		70
//1 = No special linetype scaling
//0 = Viewport scaling governs linetype scaling

/* Indicates whether the current drawing is in a Color-Dependent or Named Plot Style mode
   - 9, $PSTYLEMODE, 290, <boolean> */
#define DXF_FORMAT_GC_PStyleMode	290
//0 = Uses named plot style tables in the current drawing
//1 = Uses color-dependent plot style tables in the current drawing

/* View scale factor for new viewports - 9, $PSVPSCALE, 40, <double> */
#define DXF_FORMAT_GC_PSVPScale		40
//0 = Scaled to fit
//>0 = Scale factor (a positive real value)

/* Name of the UCS that defines the origin and orientation of orthographic UCS settings (paper space only)
   - 9, $PUCSBASE, 2, <string> */
#define DXF_FORMAT_GC_PUCSBase		2

/* Current paper space UCS name - 9, $PUCSNAME, 2, <string> */
#define DXF_FORMAT_GC_PUCSName		2

/* Current paper space UCS origin - 9, $PUCSORG, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrg_X		10
#define DXF_FORMAT_GC_PUCSOrg_Y		20
#define DXF_FORMAT_GC_PUCSOrg_Z		30

/* Point which becomes the new UCS origin after changing paper space UCS to BACK when PUCSBASE is set to WORLD
   - 9, $PUCSORGBACK, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgBack_X		10
#define DXF_FORMAT_GC_PUCSOrgBack_Y		20
#define DXF_FORMAT_GC_PUCSOrgBack_Z		30

/* Point which becomes the new UCS origin after changing paper space UCS to BOTTOM when PUCSBASE is set to WORLD
   - 9, $PUCSORGBOTTOM, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgBottom_X	10
#define DXF_FORMAT_GC_PUCSOrgBottom_Y	20
#define DXF_FORMAT_GC_PUCSOrgBottom_Z	30

/* Point which becomes the new UCS origin after changing paper space UCS to FRONT when PUCSBASE is set to WORLD
   - 9, $PUCSORGFRONT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgFront_X	10
#define DXF_FORMAT_GC_PUCSOrgFront_Y	20
#define DXF_FORMAT_GC_PUCSOrgFront_Z	30

/* Point which becomes the new UCS origin after changing paper space UCS to LEFT when PUCSBASE is set to WORLD
   - 9, $PUCSORGLEFT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgLeft_X		10
#define DXF_FORMAT_GC_PUCSOrgLeft_Y		20
#define DXF_FORMAT_GC_PUCSOrgLeft_Z		30

/* Point which becomes the new UCS origin after changing paper space UCS to RIGHT when PUCSBASE is set to WORLD
  - 9, $PUCSORGRIGHT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgRight_X	10
#define DXF_FORMAT_GC_PUCSOrgRight_Y	20
#define DXF_FORMAT_GC_PUCSOrgRight_Z	30

/* Point which becomes the new UCS origin after changing paper space UCS to TOP when PUCSBASE is set to WORLD
   - 9, $PUCSORGTOP, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSOrgTop_X		10
#define DXF_FORMAT_GC_PUCSOrgTop_Y		20
#define DXF_FORMAT_GC_PUCSOrgTop_Z		30

/* If paper space UCS is orthographic (PUCSORTHOVIEW not equal to 0), this is the name of
   the UCS that the orthographic UCS is relative to. If blank, UCS is relative to WORLD
   - 9, $PUCSORTHOREF, 2, <string> */
#define DXF_FORMAT_GC_PUCSOrthoRef		2

/* Orthographic view type of paper space UCS - 9, $PUCSORTHOVIEW, 70, <integer> */
#define DXF_FORMAT_GC_PUCSorthoView		70
//0 = UCS is not orthographic
//1 = Top
//2 = Bottom
//3 = Front
//4 = Back
//5 = Left
//6 = Right

/* Current paper space UCS X axis - 9, $PUCSXDIR, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSXDir_X		10
#define DXF_FORMAT_GC_PUCSXDir_Y		20
#define DXF_FORMAT_GC_PUCSXDir_Z		30

/* Current paper space UCS Y axis - 9, $PUCSYDIR, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_PUCSYDir_X		10
#define DXF_FORMAT_GC_PUCSYDir_Y		20
#define DXF_FORMAT_GC_PUCSYDir_Z		30

/* Quick Text mode on if nonzero - 9, $QTEXTMODE, 70, <integer> */
#define DXF_FORMAT_GC_QTextMode		70

/* REGENAUTO mode on if nonzero - 9, $REGENMODE, 70, <integer> */
#define DXF_FORMAT_GC_RegenMode		70

/* Controls the shading of edges - 9, $SHADEDGE, 70, <integer> */
#define DXF_FORMAT_GC_ShadeEdge		70
//0 = Faces shaded, edges not highlighted
//1 = Faces shaded, edges highlighted in black
//2 = Faces not filled, edges in entity color
//3 = Faces in entity color, edges in black

/* Percent ambient/diffuse light - 9, $SHADEDIF, 70, <integer> */
#define DXF_FORMAT_GC_ShadeDif		70
//range 1-100; default 70

/* Location of the ground shadow plane. This is a Z axis ordinate
   - 9, $SHADOWPLANELOCATION, 40, <double> */
#define DXF_FORMAT_GC_ShadowPlaneLocation	40

/* Sketch record increment - 9, $SKETCHINC, 40, <double> */
#define DXF_FORMAT_GC_SketchInc		40

/* Determines the object type created by the SKETCH command - 9, $SKPOLY, 70, <integer> */
#define DXF_FORMAT_GC_SKPoly		70
//0 = Generates lines
//1 = Generates polylines
//2 = Generates splines

/* Controls the object sorting methods; accessible from the Options dialog box User 
   Preferences tab. SORTENTS uses bitcodes.
   - 9, $SORTENTS, 280, <integer> */
#define DXF_FORMAT_GC_SortEnts		280
//0 = Disables SORTENTS
//1 = Sorts for object selection
//2 = Sorts for object snap
//4 = Sorts for redraws; obsolete
//8 = Sorts for MSLIDE command slide creation; obsolete
//16 = Sorts for REGEN commands
//32 = Sorts for plotting
//64 = Sorts for PostScript output; obsolete

/* Number of line segments per spline patch - 9, $SPLINESEGS, 70, < integer> */
#define DXF_FORMAT_GC_SplineSegs	70

/* Spline curve type for PEDIT Spline - 9, $SPLINETYPE, 70, <integer> */
#define DXF_FORMAT_GC_SplineType	70

/* Number of mesh tabulations in first direction - 9, $SURFTAB1, 70, <integer> */
#define DXF_FORMAT_GC_SurfTab1		70

/* Number of mesh tabulations in second direction - 9, $SURFTAB2, 70, <integer> */
#define DXF_FORMAT_GC_SurfTab2		70

/* Surface type for PEDIT Smooth - 9, $SURFTYPE, 70, <integer> */
#define DXF_FORMAT_GC_SurfType		70

/* Surface density (for PEDIT Smooth) in M direction - 9, $SURFU, 70, <integer> */
#define DXF_FORMAT_GC_SurfU			70

/* Surface density (for PEDIT Smooth) in N direction - 9, $SURFV, 70, <integer> */
#define DXF_FORMAT_GC_SurfV			70

/* Local date/time of drawing creation (see Special Handling of Date/Time Variables)
   - 9, $TDCREATE, 40, <double> */
#define DXF_FORMAT_GC_TDCreate		40

/* Cumulative editing time for this drawing (see Special Handling of Date/Time Variables)
   - 9, $TDINDWG, 40, <double> */
#define DXF_FORMAT_GC_TDInDwg		40

/* Universal date/time the drawing was created (see Special Handling of Date/Time Variables)
   - 9, $TDUCREATE, 40, <double> */
#define DXF_FORMAT_GC_TDUCreate		40

/* Local date/time of last drawing update (see Special Handling of Date/Time Variables)
   - 9, $TDUPDATE, 40, <double> */
#define DXF_FORMAT_GC_TDUpdate		40

/* User-elapsed timer - 9, $TDUSRTIMER, 40, <double> */
#define DXF_FORMAT_GC_TDUsrTimer	40

/* Universal date/time of the last update/save (see Special Handling of Date/Time Variables)
   - 9, $TDUUPDATE, 40, <double> */
#define DXF_FORMAT_GC_TDUUpdate		40

/* Default text height - 9, $TEXTSIZE, 40, <double> */
#define DXF_FORMAT_GC_TextSize		40

/* Current text style name - 9, $TEXTSTYLE, 7, <string> */
#define DXF_FORMAT_GC_TextStyle		7

/* Current thickness set by ELEV command - 9, $THICKNESS, 40, <double> */
#define DXF_FORMAT_GC_Thickness		40

/* 1 for previous release compatibility mode 0 otherwise - 9, $TILEMODE, 70, <integer> */
#define DXF_FORMAT_GC_TileMode		70

/* Default trace width - 9, $TRACEWID, 40, <double> */
#define DXF_FORMAT_GC_TraceWid		40

/* Specifies the maximum depth of the spatial index - 9, $TREEDEPTH, 70, <integer> */
#define DXF_FORMAT_GC_TreeDepth		70

/* Name of the UCS that defines the origin and orientation of orthographic UCS settings
   - 9, $UCSBASE, 2, <string> */
#define DXF_FORMAT_GC_UCSBase		2

/* Name of current UCS - 9, $UCSNAME, 2, <string> */
#define DXF_FORMAT_GC_UCSName		2

/* Origin of current UCS (in WCS) - 9, $UCSORG, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrg_X		10
#define DXF_FORMAT_GC_UCSOrg_Y		20
#define DXF_FORMAT_GC_UCSOrg_Z		30

/* Point which becomes the new UCS origin after changing model space UCS to BACK when UCSBASE is set to WORLD
   - 9, $UCSORGBACK, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgBack_X		10
#define DXF_FORMAT_GC_UCSOrgBack_Y		20
#define DXF_FORMAT_GC_UCSOrgBack_Z		30

/* Point which becomes the new UCS origin after changing model space UCS to BOTTOM when UCSBASE is set to WORLD
   - 9, $UCSORGBOTTOM, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgBottom_X	10
#define DXF_FORMAT_GC_UCSOrgBottom_Y	20
#define DXF_FORMAT_GC_UCSOrgBottom_Z	30

/* Point which becomes the new UCS origin after changing model space UCS to FRONT when UCSBASE is set to WORLD
   - 9, $UCSORGFRONT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgFront_X		10
#define DXF_FORMAT_GC_UCSOrgFront_Y		20
#define DXF_FORMAT_GC_UCSOrgFront_Z		30

/* Point which becomes the new UCS origin after changing model space UCS to LEFT when UCSBASE is set to WORLD
   - 9, $UCSORGLEFT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgLeft_X		10
#define DXF_FORMAT_GC_UCSOrgLeft_Y		20
#define DXF_FORMAT_GC_UCSOrgLeft_Z		30

/* Point which becomes the new UCS origin after changing model space UCS to RIGHT when UCSBASE is set to WORLD
   - 9, UCSORGRIGHT, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgRight_X		10
#define DXF_FORMAT_GC_UCSOrgRight_Y		20
#define DXF_FORMAT_GC_UCSOrgRight_Z		30

/* Point which becomes the new UCS origin after changing model space UCS to TOP when UCSBASE is set to WORLD
   - 9, $UCSORGTOP, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSOrgTop_X		10
#define DXF_FORMAT_GC_UCSOrgTop_Y		20
#define DXF_FORMAT_GC_UCSOrgTop_Z		30

/* If model space UCS is orthographic (UCSORTHOVIEW not equal to 0), this is the name of 
   the UCS that the orthographic UCS is relative to. If blank, UCS is relative to WORLD.
   - 9, $UCSORTHOREF, 2, <string> */
#define DXF_FORMAT_GC_UCSOrthoRef		2

/* Orthographic view type of model space UCS - 9, $UCSORTHOVIEW, 70, <integer> */
#define DXF_FORMAT_GC_UCSOrthoView		70
//0 = UCS is not orthographic
//1 = Top
//2 = Bottom
//3 = Front
//4 = Back
//5 = Left
//6 = Right

/* Direction of the current UCS X axis (in WCS) - 9, $UCSXDIR, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSXDir_X		10
#define DXF_FORMAT_GC_UCSXDir_Y		20
#define DXF_FORMAT_GC_UCSXDir_Z		30

/* Direction of the current UCS Y axis (in WCS) - 9, $UCSYDIR, 10, <double X>, 20, <double Y>, 30, <double Z> */
#define DXF_FORMAT_GC_UCSYDir_X		10
#define DXF_FORMAT_GC_UCSYDir_Y		20
#define DXF_FORMAT_GC_UCSYDir_Z		30

/* Low bit set = Display fractions, feet-and-inches, and surveyors angles in input format
   - 9, $UNITMODE, 70, <integer> */
#define DXF_FORMAT_GC_UnitMode		70

/* Five integer variables intended for use by third-party developers - 9, $USERIx, 70, <integer> */
#define DXF_FORMAT_GC_UserI1		70
#define DXF_FORMAT_GC_UserI2		70
#define DXF_FORMAT_GC_UserI3		70
#define DXF_FORMAT_GC_UserI4		70
#define DXF_FORMAT_GC_UserI5		70

/* Five real variables intended for use by third-party developers - 9, $USERRx, 40, <double> */
#define DXF_FORMAT_GC_UserR1		40
#define DXF_FORMAT_GC_UserR2		40
#define DXF_FORMAT_GC_UserR3		40
#define DXF_FORMAT_GC_UserR4		40
#define DXF_FORMAT_GC_UserR5		40

/* Controls the user timer for the drawing - 9, $USRTIMER, 70, <integer> */
#define DXF_FORMAT_GC_UsrTimer		70
//0 = Timer off
//1 = Timer on

/* Uniquely identifies a particular version of a drawing. Updated when the drawing is modified.
   - 9, $VERSIONGUID, 2, <string> */
#define DXF_FORMAT_GC_VersionGUID	2

/* Controls the properties of xref-dependent layers - 9, $VISRETAIN, 70, <integer> */
#define DXF_FORMAT_GC_VisRetain		70
//0 = Don't retain xref-dependent visibility settings
//1 = Retain xref-dependent visibility settings

/* Determines whether input for the DVIEW and VPOINT command evaluated as relative to the WCS or current UCS
   - 9, $WORLDVIEW, 70, <integer> */
#define DXF_FORMAT_GC_WorldView		70
//0 = Don't change UCS
//1 = Set UCS to WCS during DVIEW/VPOINT

/* Controls the visibility of xref clipping boundaries - 9, $XCLIPFRAME, 290, <boolean> */
#define DXF_FORMAT_GC_XClipFrame	290
//0 = Clipping boundary is not visible
//1 = Clipping boundary is visible

/* Controls whether the current drawing can be edited in-place when being referenced by another drawing
   - 9, $XEDIT, 290, <boolean> */
#define DXF_FORMAT_GC_XEdit			290
//0 = Can't use in-place reference editing
//1 = Can use in-place reference editing

#endif

