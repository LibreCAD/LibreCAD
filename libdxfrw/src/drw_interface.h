/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_INTERFACE_H
#define DRW_INTERFACE_H

#include <string.h>

#include "drw_entities.h"
#include "drw_objects.h"
//#include "dl_extrusion.h"

/**
 * Abstract class (interface) for comunicate dxfReader with the application.
 * Inherit your class which takes care of the entities in the 
 * processed DXF file from this interface. 
 *
 * @author Rallaz
 */
class DRW_Interface {
public:
    DRW_Interface() {
//        extrusion = new DL_Extrusion;
    }
    virtual ~DRW_Interface() {
//        delete extrusion;
    }

    /** Called when header is parsed.  */
    virtual void addHeader(const DRW_Header* data) = 0;

    /** Called for every line Type.  */
    virtual void addLType(const DRW_LType& data) = 0;
    /** Called for every layer. */
    virtual void addLayer(const DRW_Layer& data) = 0;

    /**
     * Called for every block. Note: all entities added after this
     * command go into this block until endBlock() is called.
    *
     * @see endBlock()
     */
    virtual void addBlock(const DRW_Block& data) = 0;

    /** Called to end the current block */
    virtual void endBlock() = 0;

    /** Called for every point */
    virtual void addPoint(const DRW_Point& data) = 0;

    /** Called for every line */
    virtual void addLine(const DRW_Line& data) = 0;

    /** Called for every arc */
    virtual void addArc(const DRW_Arc& data) = 0;

    /** Called for every circle */
    virtual void addCircle(const DRW_Circle& data) = 0;

    /** Called for every ellipse */
    virtual void addEllipse(const DRW_Ellipse& data) = 0;

    /** Called for every lwpolyline */
    virtual void addLWPolyline(const DRW_LWPolyline& data) = 0;

    /** Called for every polyline start */
    virtual void addPolyline(const DRW_Polyline& data) = 0;

    /** Called for every spline */
    virtual void addSpline(const DRW_Spline* data) = 0;
	
	/** Called for every spline knot value */
    virtual void addKnot(const DRW_Entity& data) = 0;

    /** Called for every insert. */
    virtual void addInsert(const DRW_Insert& data) = 0;
    
    /** Called for every trace start */
    virtual void addTrace(const DRW_Trace& data) = 0;
    
    /** Called for every 3dface start */
    virtual void add3dFace(const DRW_3Dface& data) = 0;

    /** Called for every solid start */
    virtual void addSolid(const DRW_Solid& data) = 0;


    /** Called for every Multi Text entity. */
    virtual void addMText(const DRW_MText& data) = 0;

    /** Called for every Text entity. */
    virtual void addText(const DRW_Text& data) = 0;

    /**
     * Called for every aligned dimension entity. 
     */
    virtual void addDimAlign(const DRW_DimAligned *data) = 0;
    /**
     * Called for every linear or rotated dimension entity. 
     */
    virtual void addDimLinear(const DRW_DimLinear *data) = 0;

	/**
     * Called for every radial dimension entity. 
     */
    virtual void addDimRadial(const DRW_DimRadial *data) = 0;

	/**
     * Called for every diametric dimension entity. 
     */
    virtual void addDimDiametric(const DRW_DimDiametric *data) = 0;

	/**
     * Called for every angular dimension (2 lines version) entity. 
     */
    virtual void addDimAngular(const DRW_DimAngular *data) = 0;

	/**
     * Called for every angular dimension (3 points version) entity. 
     */
    virtual void addDimAngular3P(const DRW_DimAngular3p *data) = 0;
	
    /**
     * Called for every ordinate dimension entity. 
     */
    virtual void addDimOrdinate(const DRW_DimOrdinate *data) = 0;
    
    /** 
	 * Called for every leader start. 
	 */
    virtual void addLeader(const DRW_Leader *data) = 0;
	
	/** 
	 * Called for every hatch entity. 
	 */
    virtual void addHatch(const DRW_Hatch *data) = 0;
	
	/** 
	 * Called for every image entity. 
	 */
    virtual void addImage(const DRW_Image *data) = 0;

	/**
	 * Called for every image definition.
	 */
        virtual void linkImage(const DRW_ImageDef *data) = 0;

    /**
     * Called for every comment in the DXF file (code 999).
     */
    virtual void addComment(const char* comment) = 0;

    /** Sets the current attributes for entities. */
/*    void setExtrusion(double dx, double dy, double dz, double elevation) {
        extrusion->setDirection(dx, dy, dz);
		extrusion->setElevation(elevation);
    }*/

    /** @return the current attributes used for new entities. */
//    DL_Extrusion* getExtrusion() {
//        return extrusion;
//    }

    virtual void writeHeader() = 0;
    virtual void writeEntities() = 0;
    virtual void writeLTypes() = 0;
    virtual void writeLayers() = 0;

protected:
//    DL_Attributes attributes;
//    DL_Extrusion *extrusion;
};

#endif
