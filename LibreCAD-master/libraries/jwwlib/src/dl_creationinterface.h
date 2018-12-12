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

#ifndef DL_CREATIONINTERFACE_H
#define DL_CREATIONINTERFACE_H

#include <cstring>

#include "dl_attributes.h"
#include "dl_codes.h"
#include "dl_entities.h"
#include "dl_extrusion.h"

/**
 * Abstract class (interface) for the creation of new entities.
 * Inherit your class which takes care of the entities in the 
 * processed DXF file from this interface. 
 *
 * Double arrays passed to your implementation contain 3 double 
 * values for x, y, z coordinates unless stated differently.
 *
 * @author Andrew Mustun
 */
class DL_CreationInterface {
public:
    DL_CreationInterface() {
        extrusion = new DL_Extrusion;
    }
    virtual ~DL_CreationInterface() {
        delete extrusion;
    }

    /**
     * Called for every layer.
     */
    virtual void addLayer(const DL_LayerData& data) = 0;

    /**
     * Called for every block. Note: all entities added after this
     * command go into this block until endBlock() is called.
    *
     * @see endBlock()
     */
    virtual void addBlock(const DL_BlockData& data) = 0;

    /** Called to end the current block */
    virtual void endBlock() = 0;

    /** Called for every point */
    virtual void addPoint(const DL_PointData& data) = 0;

    /** Called for every line */
    virtual void addLine(const DL_LineData& data) = 0;

    /** Called for every arc */
    virtual void addArc(const DL_ArcData& data) = 0;

    /** Called for every circle */
    virtual void addCircle(const DL_CircleData& data) = 0;

    /** Called for every ellipse */
    virtual void addEllipse(const DL_EllipseData& data) = 0;

    /** Called for every polyline start */
    virtual void addPolyline(const DL_PolylineData& data) = 0;

    /** Called for every polyline vertex */
    virtual void addVertex(const DL_VertexData& data) = 0;
	
	/** Called for every spline */
    virtual void addSpline(const DL_SplineData& data) = 0;
	
	/** Called for every spline control point */
    virtual void addControlPoint(const DL_ControlPointData& data) = 0;
	
	/** Called for every spline knot value */
    virtual void addKnot(const DL_KnotData& data) = 0;

    /** Called for every insert. */
    virtual void addInsert(const DL_InsertData& data) = 0;
    
    /** Called for every trace start */
    virtual void addTrace(const DL_TraceData& data) = 0;
    
    /** Called for every 3dface start */
    virtual void add3dFace(const DL_3dFaceData& data) = 0;

    /** Called for every solid start */
    virtual void addSolid(const DL_SolidData& data) = 0;


    /** Called for every Multi Text entity. */
    virtual void addMText(const DL_MTextData& data) = 0;

    /**
     * Called for additional text chunks for MTEXT entities.
     * The chunks come at 250 character in size each. Note that 
     * those chunks come <b>before</b> the actual MTEXT entity.
     */
    virtual void addMTextChunk(const char* text) = 0;

    /** Called for every Text entity. */
    virtual void addText(const DL_TextData& data) = 0;

    /**
     * Called for every aligned dimension entity. 
     */
    virtual void addDimAlign(const DL_DimensionData& data,
                             const DL_DimAlignedData& edata) = 0;
    /**
     * Called for every linear or rotated dimension entity. 
     */
    virtual void addDimLinear(const DL_DimensionData& data,
                              const DL_DimLinearData& edata) = 0;

	/**
     * Called for every radial dimension entity. 
     */
    virtual void addDimRadial(const DL_DimensionData& data,
                              const DL_DimRadialData& edata) = 0;

	/**
     * Called for every diametric dimension entity. 
     */
    virtual void addDimDiametric(const DL_DimensionData& data,
                              const DL_DimDiametricData& edata) = 0;

	/**
     * Called for every angular dimension (2 lines version) entity. 
     */
    virtual void addDimAngular(const DL_DimensionData& data,
                              const DL_DimAngularData& edata) = 0;

	/**
     * Called for every angular dimension (3 points version) entity. 
     */
    virtual void addDimAngular3P(const DL_DimensionData& data,
                              const DL_DimAngular3PData& edata) = 0;
	
    /**
     * Called for every ordinate dimension entity. 
     */
    virtual void addDimOrdinate(const DL_DimensionData& data,
                             const DL_DimOrdinateData& edata) = 0;
    
    /** 
	 * Called for every leader start. 
	 */
    virtual void addLeader(const DL_LeaderData& data) = 0;
	
	/** 
	 * Called for every leader vertex 
	 */
    virtual void addLeaderVertex(const DL_LeaderVertexData& data) = 0;
	
	/** 
	 * Called for every hatch entity. 
	 */
    virtual void addHatch(const DL_HatchData& data) = 0;
	
	/** 
	 * Called for every image entity. 
	 */
    virtual void addImage(const DL_ImageData& data) = 0;

	/**
	 * Called for every image definition.
	 */
	virtual void linkImage(const DL_ImageDefData& data) = 0;

	/** 
	 * Called for every hatch loop. 
	 */
    virtual void addHatchLoop(const DL_HatchLoopData& data) = 0;

	/** 
	 * Called for every hatch edge entity. 
	 */
    virtual void addHatchEdge(const DL_HatchEdgeData& data) = 0;
	
	/** 
	 * Called after an entity has been completed.  
	 */
    virtual void endEntity() = 0;
    
    /**
     * Called for every comment in the DXF file (code 999).
     */
    virtual void addComment(const char* comment) = 0;

    /**
     * Called for every vector variable in the DXF file (e.g. "$EXTMIN").
     */
    virtual void setVariableVector(const char* key, 
	               double v1, double v2, double v3, int code) = 0;
	
    /**
     * Called for every string variable in the DXF file (e.g. "$ACADVER").
     */
    virtual void setVariableString(const char* key, const char* value, int code) = 0;
	
    /**
     * Called for every int variable in the DXF file (e.g. "$ACADMAINTVER").
     */
    virtual void setVariableInt(const char* key, int value, int code) = 0;
	
    /**
     * Called for every double variable in the DXF file (e.g. "$DIMEXO").
     */
    virtual void setVariableDouble(const char* key, double value, int code) = 0;
	
     /**
      * Called when a SEQEND occurs (when a POLYLINE or ATTRIB is done)
      */
     virtual void endSequence() = 0;

    /** Sets the current attributes for entities. */
    void setAttributes(const DL_Attributes& attrib) {
        attributes = attrib;
    }

    /** @return the current attributes used for new entities. */
    DL_Attributes getAttributes() {
        return attributes;
    }

    /** Sets the current attributes for entities. */
    void setExtrusion(double dx, double dy, double dz, double elevation) {
        extrusion->setDirection(dx, dy, dz);
		extrusion->setElevation(elevation);
    }

    /** @return the current attributes used for new entities. */
    DL_Extrusion* getExtrusion() {
        return extrusion;
    }

protected:
    DL_Attributes attributes;
    DL_Extrusion *extrusion;
};

#endif
