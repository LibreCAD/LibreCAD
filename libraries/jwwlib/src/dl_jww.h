/****************************************************************************
** $Id: dl_jww.h,v 1.1.1.2 2010/02/08 11:58:29 zeronemo2007 Exp $
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

#ifndef DL_JWW_H
#define DL_JWW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __GCC2x__
#include <sstream>
#endif

#include "dl_attributes.h"
#include "dl_codes.h"
#include "dl_entities.h"
#include "dl_writer_ascii.h"

#include "jwwdoc.h"

class DL_CreationInterface;
class DL_WriterA;

#define DL_VERSION     "2.0.4.8"

#define DL_UNKNOWN               0
#define DL_LAYER                10
#define DL_BLOCK                11
#define DL_ENDBLK               12
#define DL_LINETYPE             13
#define DL_SETTING              50
#define DL_ENTITY_POINT        100
#define DL_ENTITY_LINE         101
#define DL_ENTITY_POLYLINE     102
#define DL_ENTITY_LWPOLYLINE   103
#define DL_ENTITY_VERTEX       104
#define DL_ENTITY_SPLINE       105
#define DL_ENTITY_KNOT         106
#define DL_ENTITY_CONTROLPOINT 107
#define DL_ENTITY_ARC          108
#define DL_ENTITY_CIRCLE       109
#define DL_ENTITY_ELLIPSE      110
#define DL_ENTITY_INSERT       111
#define DL_ENTITY_TEXT         112
#define DL_ENTITY_MTEXT        113
#define DL_ENTITY_DIMENSION    114
#define DL_ENTITY_LEADER       115
#define DL_ENTITY_HATCH        116
#define DL_ENTITY_ATTRIB       117
#define DL_ENTITY_IMAGE        118
#define DL_ENTITY_IMAGEDEF     119
#define DL_ENTITY_TRACE        120
#define DL_ENTITY_SOLID        121
#define DL_ENTITY_SEQEND       122

/**
 * Reading of JWW files.
 *
 * in() reads a JWW file through JWWDocument (jwwdoc.h) and calls methods
 * from the interface DL_CreationInterface to add the entities to the
 * container provided by the user of the library. There is no writer.
 *
 * @author Andrew Mustun
 */
class DL_Jww {
public:
    DL_Jww();
    ~DL_Jww();

    bool in(const string& file,
            DL_CreationInterface* creationInterface);
    bool readJwwGroups(FILE* fp,
                       DL_CreationInterface* creationInterface,
					   int* errorCounter = nullptr);

    bool processJwwGroup(DL_CreationInterface* creationInterface,
                         int groupCode, const char* groupValue);

	void addSetting(DL_CreationInterface* creationInterface);
    void addLayer(DL_CreationInterface* creationInterface);
    void addBlock(DL_CreationInterface* creationInterface);
    void endBlock(DL_CreationInterface* creationInterface);

    void addPoint(DL_CreationInterface* creationInterface) ;
    void addLine(DL_CreationInterface* creationInterface) ;

    void addPolyline(DL_CreationInterface* creationInterface) ;
    void addVertex(DL_CreationInterface* creationInterface) ;

    void addSpline(DL_CreationInterface* creationInterface) ;
    //void addKnot(DL_CreationInterface* creationInterface);
    //void addControlPoint(DL_CreationInterface* creationInterface);

    void addArc(DL_CreationInterface* creationInterface) ;
    void addCircle(DL_CreationInterface* creationInterface) ;
    void addEllipse(DL_CreationInterface* creationInterface) ;
    void addInsert(DL_CreationInterface* creationInterface);

    void addTrace(DL_CreationInterface* creationInterface) ;
    void addSolid(DL_CreationInterface* creationInterface) ;

    void addMText(DL_CreationInterface* creationInterface);
	bool handleMTextData(DL_CreationInterface* creationInterface) ;
	bool handleLWPolylineData(DL_CreationInterface* creationInterface);
	bool handleSplineData(DL_CreationInterface* creationInterface);
	bool handleLeaderData(DL_CreationInterface* creationInterface);
	bool handleHatchData(DL_CreationInterface* creationInterface);

    void addText(DL_CreationInterface* creationInterface);
    void addAttrib(DL_CreationInterface* creationInterface);
    DL_DimensionData getDimData() const;
    void addDimLinear(DL_CreationInterface* creationInterface);
    void addDimAligned(DL_CreationInterface* creationInterface);
    void addDimRadial(DL_CreationInterface* creationInterface);
    void addDimDiametric(DL_CreationInterface* creationInterface);
    void addDimAngular(DL_CreationInterface* creationInterface);
    void addDimAngular3P(DL_CreationInterface* creationInterface);
    void addLeader(DL_CreationInterface* creationInterface) const;
    void addHatch(DL_CreationInterface* creationInterface);
    void addImage(DL_CreationInterface* creationInterface);
    void addImageDef(DL_CreationInterface* creationInterface);

	void endEntity(DL_CreationInterface* creationInterface);

    void endSequence(DL_CreationInterface* creationInterface);

	int  stringToInt(const char* s, bool* ok= nullptr) ;


    /**
     * Converts the given string into a double or returns the given
     * default valud (def) if value is NULL or empty.
     */
    static double toReal(const char* value, double def=0.0) {
       if (value && value[0] != '\0') {
            double ret;
            if (strchr(value, ',') != nullptr) {
               auto tmp = new char[strnlen(value, 20)+1];
               strncpy(tmp, value, 20);
               DL_WriterA::strReplace(tmp, ',', '.');
               ret = atof(tmp);
      		   delete[] tmp;
            }
            else {
               ret = atof(value);
            }
			return ret;
        }
       return def;
    }
    /**
     * Converts the given string into an int or returns the given
     * default valud (def) if value is NULL or empty.
     */
    static int toInt(const char* value, int def=0) {
        if (value && value[0] != '\0') {
            return atoi(value);
        }
        return def;
    }
    /**
     * Converts the given string into a string or returns the given
     * default valud (def) if value is NULL or empty.
     */
    static const char* toString(const char* value, const char* def="") {
        if (value && value[0] != '\0') {
            return value;
        }
        return def;
    }

	int getLibVersion(const char* str);

	void CreateSen(DL_CreationInterface* creationInterface, CDataSen& DSen);
	void CreateEnko(DL_CreationInterface* creationInterface, CDataEnko& DEnko);
	void CreateTen(DL_CreationInterface* creationInterface, CDataTen& DTen);
	void CreateMoji(DL_CreationInterface* creationInterface, CDataMoji& DMoji);
	void CreateSolid(DL_CreationInterface* creationInterface, CDataSolid& DSolid);
	void CreateSunpou(DL_CreationInterface* creationInterface, CDataSunpou& DSunpou);
	void CreateBlock(DL_CreationInterface* creationInterface, CDataBlock& DBlock);

private:
    unsigned long styleHandleStd;

	string polylineLayer;
    double* vertices;
    int maxVertices;
    int vertexIndex;
	
    double* knots;
    int maxKnots;
    int knotIndex;
	
    double* controlPoints;
    int maxControlPoints;
    int controlPointIndex;

    double* leaderVertices;
    int maxLeaderVertices;
    int leaderVertexIndex;

    // array of hatch loops
    DL_HatchLoopData* hatchLoops;
    int maxHatchLoops;
    int hatchLoopIndex;
    // array in format [loop#][edge#]
    DL_HatchEdgeData** hatchEdges;
    int* maxHatchEdges;
    int* hatchEdgeIndex;
    bool dropEdges;

    // Bulge for the next vertex.
    double bulge;

    // Only the useful part of the group code
    char groupCodeTmp[DL_DXF_MAXLINE+1];
    // ...same as integer
    unsigned int groupCode;
    // Only the useful part of the group value
    char groupValue[DL_DXF_MAXLINE+1];
    // Current entity type
    int currentEntity;
    // Value of the current setting
    char settingValue[DL_DXF_MAXLINE+1];
    // Key of the current setting (e.g. "$ACADVER")
    char settingKey[DL_DXF_MAXLINE+1];
    // Stores the group codes
    char values[DL_DXF_MAXGROUPCODE][DL_DXF_MAXLINE+1];
    // First call of this method. We initialize all group values in
    //  the first call.
    bool firstCall;
    // Attributes of the current entity (layer, color, width, line type)
    DL_Attributes attrib;
	// library version. hex: 0x20003001 = 2.0.3.1
	int libVersion;
};

#endif
