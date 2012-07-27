#ifndef SHAPEFILE_H_INCLUDED
#define SHAPEFILE_H_INCLUDED

/******************************************************************************
 * $Id: shapefil.h,v 1.52 2011-12-11 22:26:46 fwarmerdam Exp $
 *
 * Project:  Shapelib
 * Purpose:  Primary include file for Shapelib.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see LICENSE.LGPL).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: shapefil.h,v $
 * Revision 1.52  2011-12-11 22:26:46  fwarmerdam
 * upgrade .qix access code to use SAHooks (gdal #3365)
 *
 * Revision 1.51  2011-07-24 05:59:25  fwarmerdam
 * minimize use of CPLError in favor of SAHooks.Error()
 *
 * Revision 1.50  2011-05-13 17:35:17  fwarmerdam
 * added DBFReorderFields() and DBFAlterFields() functions (from Even)
 *
 * Revision 1.49  2011-04-16 14:38:21  fwarmerdam
 * avoid warnings with gcc on SHP_CVSID
 *
 * Revision 1.48  2010-08-27 23:42:52  fwarmerdam
 * add SHPAPI_CALL attribute in code
 *
 * Revision 1.47  2010-01-28 11:34:34  fwarmerdam
 * handle the shape file length limits more gracefully (#3236)
 *
 * Revision 1.46  2008-11-12 14:28:15  fwarmerdam
 * DBFCreateField() now works on files with records
 *
 * Revision 1.45  2008/11/11 17:47:10  fwarmerdam
 * added DBFDeleteField() function
 *
 * Revision 1.44  2008/01/16 20:05:19  bram
 * Add file hooks that accept UTF-8 encoded filenames on some platforms.  Use SASetupUtf8Hooks
 *  tosetup the hooks and check SHPAPI_UTF8_HOOKS for its availability.  Currently, this
 *  is only available on the Windows platform that decodes the UTF-8 filenames to wide
 *  character strings and feeds them to _wfopen and _wremove.
 *
 * Revision 1.43  2008/01/10 16:35:30  fwarmerdam
 * avoid _ prefix on #defined symbols (bug 1840)
 *
 * Revision 1.42  2007/12/18 18:28:14  bram
 * - create hook for client specific atof (bugzilla ticket 1615)
 * - check for NULL handle before closing cpCPG file, and close after reading.
 *
 * Revision 1.41  2007/12/15 20:25:32  bram
 * dbfopen.c now reads the Code Page information from the DBF file, and exports
 * this information as a string through the DBFGetCodePage function.  This is 
 * either the number from the LDID header field ("LDID/<number>") or as the 
 * content of an accompanying .CPG file.  When creating a DBF file, the code can
 * be set using DBFCreateEx.
 *
 * Revision 1.40  2007/12/06 07:00:25  fwarmerdam
 * dbfopen now using SAHooks for fileio
 *
 * Revision 1.39  2007/12/04 20:37:56  fwarmerdam
 * preliminary implementation of hooks api for io and errors
 *
 * Revision 1.38  2007/11/21 22:39:56  fwarmerdam
 * close shx file in readonly mode (GDAL #1956)
 *
 * Revision 1.37  2007/10/27 03:31:14  fwarmerdam
 * limit default depth of tree to 12 levels (gdal ticket #1594)
 *
 * Revision 1.36  2007/09/10 23:33:15  fwarmerdam
 * Upstreamed support for visibility flag in SHPAPI_CALL for the needs
 * of GDAL (gdal ticket #1810).
 *
 * Revision 1.35  2007/09/03 19:48:10  fwarmerdam
 * move DBFReadAttribute() static dDoubleField into dbfinfo
 *
 * Revision 1.34  2006/06/17 15:33:32  fwarmerdam
 * added pszWorkField - bug 1202 (rso)
 *
 * Revision 1.33  2006/02/15 01:14:30  fwarmerdam
 * added DBFAddNativeFieldType
 *
 * Revision 1.32  2006/01/26 15:07:32  fwarmerdam
 * add bMeasureIsUsed flag from Craig Bruce: Bug 1249
 *
 * Revision 1.31  2006/01/05 01:27:27  fwarmerdam
 * added dbf deletion mark/fetch
 *
 * Revision 1.30  2005/01/03 22:30:13  fwarmerdam
 * added support for saved quadtrees
 *
 * Revision 1.29  2004/09/26 20:09:35  fwarmerdam
 * avoid rcsid warnings
 *
 * Revision 1.28  2003/12/29 06:02:18  fwarmerdam
 * added cpl_error.h option
 *
 * Revision 1.27  2003/04/21 18:30:37  warmerda
 * added header write/update public methods
 *
 * Revision 1.26  2002/09/29 00:00:08  warmerda
 * added FTLogical and logical attribute read/write calls
 *
 * Revision 1.25  2002/05/07 13:46:30  warmerda
 * added DBFWriteAttributeDirectly().
 *
 * Revision 1.24  2002/04/10 16:59:54  warmerda
 * added SHPRewindObject
 *
 * Revision 1.23  2002/01/15 14:36:07  warmerda
 * updated email address
 *
 * Revision 1.22  2002/01/15 14:32:00  warmerda
 * try to improve SHPAPI_CALL docs
 */

#include <stdio.h>

#ifdef USE_DBMALLOC
#include <dbmalloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                        Configuration options.                        */
/************************************************************************/

/* -------------------------------------------------------------------- */
/*      Should the DBFReadStringAttribute() strip leading and           */
/*      trailing white space?                                           */
/* -------------------------------------------------------------------- */
#define TRIM_DBF_WHITESPACE

/* -------------------------------------------------------------------- */
/*      Should we write measure values to the Multipatch object?        */
/*      Reportedly ArcView crashes if we do write it, so for now it     */
/*      is disabled.                                                    */
/* -------------------------------------------------------------------- */
#define DISABLE_MULTIPATCH_MEASURE
    
/* -------------------------------------------------------------------- */
/*      SHPAPI_CALL                                                     */
/*                                                                      */
/*      The following two macros are present to allow forcing           */
/*      various calling conventions on the Shapelib API.                */
/*                                                                      */
/*      To force __stdcall conventions (needed to call Shapelib         */
/*      from Visual Basic and/or Dephi I believe) the makefile could    */
/*      be modified to define:                                          */
/*                                                                      */
/*        /DSHPAPI_CALL=__stdcall                                       */
/*                                                                      */
/*      If it is desired to force export of the Shapelib API without    */
/*      using the shapelib.def file, use the following definition.      */
/*                                                                      */
/*        /DSHAPELIB_DLLEXPORT                                          */
/*                                                                      */
/*      To get both at once it will be necessary to hack this           */
/*      include file to define:                                         */
/*                                                                      */
/*        #define SHPAPI_CALL __declspec(dllexport) __stdcall           */
/*        #define SHPAPI_CALL1 __declspec(dllexport) * __stdcall        */
/*                                                                      */
/*      The complexity of the situtation is partly caused by the        */
/*      peculiar requirement of Visual C++ that __stdcall appear        */
/*      after any "*"'s in the return value of a function while the     */
/*      __declspec(dllexport) must appear before them.                  */
/* -------------------------------------------------------------------- */

#ifdef SHAPELIB_DLLEXPORT
#  define SHPAPI_CALL __declspec(dllexport)
#  define SHPAPI_CALL1(x)  __declspec(dllexport) x
#endif

#ifndef SHPAPI_CALL
#  if defined(USE_GCC_VISIBILITY_FLAG)
#    define SHPAPI_CALL     __attribute__ ((visibility("default")))
#    define SHPAPI_CALL1(x) __attribute__ ((visibility("default")))     x
#  else
#    define SHPAPI_CALL
#  endif
#endif

#ifndef SHPAPI_CALL1
#  define SHPAPI_CALL1(x)      x SHPAPI_CALL
#endif
    
/* -------------------------------------------------------------------- */
/*      Macros for controlling CVSID and ensuring they don't appear     */
/*      as unreferenced variables resulting in lots of warnings.        */
/* -------------------------------------------------------------------- */
#ifndef DISABLE_CVSID
#  if defined(__GNUC__) && __GNUC__ >= 4
#    define SHP_CVSID(string)     static char cpl_cvsid[] __attribute__((used)) = string;
#  else
#    define SHP_CVSID(string)     static char cpl_cvsid[] = string; \
static char *cvsid_aw() { return( cvsid_aw() ? ((char *) NULL) : cpl_cvsid ); }
#  endif
#else
#  define SHP_CVSID(string)
#endif

/* -------------------------------------------------------------------- */
/*      On some platforms, additional file IO hooks are defined that    */
/*      UTF-8 encoded filenames Unicode filenames                       */
/* -------------------------------------------------------------------- */
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#	define SHPAPI_WINDOWS
#	define SHPAPI_UTF8_HOOKS
#endif

/* -------------------------------------------------------------------- */
/*      IO/Error hook functions.                                        */
/* -------------------------------------------------------------------- */
typedef int *SAFile;

#ifndef SAOffset
typedef unsigned long SAOffset;
#endif

typedef struct {
    SAFile     (*FOpen) ( const char *filename, const char *access);
    SAOffset   (*FRead) ( void *p, SAOffset size, SAOffset nmemb, SAFile file);
    SAOffset   (*FWrite)( void *p, SAOffset size, SAOffset nmemb, SAFile file);
    SAOffset   (*FSeek) ( SAFile file, SAOffset offset, int whence );
    SAOffset   (*FTell) ( SAFile file );
    int        (*FFlush)( SAFile file );
    int        (*FClose)( SAFile file );
    int        (*Remove) ( const char *filename );

    void       (*Error) ( const char *message );
    double     (*Atof)  ( const char *str );
} SAHooks;

void SHPAPI_CALL SASetupDefaultHooks( SAHooks *psHooks );
#ifdef SHPAPI_UTF8_HOOKS
void SHPAPI_CALL SASetupUtf8Hooks( SAHooks *psHooks );
#endif

/************************************************************************/
/*                             SHP Support.                             */
/************************************************************************/
typedef	struct
{
    SAHooks sHooks;

    SAFile      fpSHP;
    SAFile 	fpSHX;

    int		nShapeType;				/* SHPT_* */
    
    unsigned int 	nFileSize;				/* SHP file */

    int         nRecords;
    int		nMaxRecords;
    unsigned int		*panRecOffset;
    unsigned int		*panRecSize;

    double	adBoundsMin[4];
    double	adBoundsMax[4];

    int		bUpdated;

    unsigned char *pabyRec;
    int         nBufSize;
} SHPInfo;

typedef SHPInfo * SHPHandle;

/* -------------------------------------------------------------------- */
/*      Shape types (nSHPType)                                          */
/* -------------------------------------------------------------------- */
#define SHPT_NULL	0
#define SHPT_POINT	1
#define SHPT_ARC	3
#define SHPT_POLYGON	5
#define SHPT_MULTIPOINT	8
#define SHPT_POINTZ	11
#define SHPT_ARCZ	13
#define SHPT_POLYGONZ	15
#define SHPT_MULTIPOINTZ 18
#define SHPT_POINTM	21
#define SHPT_ARCM	23
#define SHPT_POLYGONM	25
#define SHPT_MULTIPOINTM 28
#define SHPT_MULTIPATCH 31


/* -------------------------------------------------------------------- */
/*      Part types - everything but SHPT_MULTIPATCH just uses           */
/*      SHPP_RING.                                                      */
/* -------------------------------------------------------------------- */

#define SHPP_TRISTRIP	0
#define SHPP_TRIFAN	1
#define SHPP_OUTERRING	2
#define SHPP_INNERRING	3
#define SHPP_FIRSTRING	4
#define SHPP_RING	5

/* -------------------------------------------------------------------- */
/*      SHPObject - represents on shape (without attributes) read       */
/*      from the .shp file.                                             */
/* -------------------------------------------------------------------- */
typedef struct
{
    int		nSHPType;

    int		nShapeId; /* -1 is unknown/unassigned */

    int		nParts;
    int		*panPartStart;
    int		*panPartType;
    
    int		nVertices;
    double	*padfX;
    double	*padfY;
    double	*padfZ;
    double	*padfM;

    double	dfXMin;
    double	dfYMin;
    double	dfZMin;
    double	dfMMin;

    double	dfXMax;
    double	dfYMax;
    double	dfZMax;
    double	dfMMax;

    int		bMeasureIsUsed;
} SHPObject;

/* -------------------------------------------------------------------- */
/*      SHP API Prototypes                                              */
/* -------------------------------------------------------------------- */

/* If pszAccess is read-only, the fpSHX field of the returned structure */
/* will be NULL as it is not necessary to keep the SHX file open */
SHPHandle SHPAPI_CALL
      SHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle SHPAPI_CALL
      SHPOpenLL( const char *pszShapeFile, const char *pszAccess, 
                 SAHooks *psHooks );
SHPHandle SHPAPI_CALL
      SHPCreate( const char * pszShapeFile, int nShapeType );
SHPHandle SHPAPI_CALL
      SHPCreateLL( const char * pszShapeFile, int nShapeType,
                   SAHooks *psHooks );
void SHPAPI_CALL
      SHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType,
                  double * padfMinBound, double * padfMaxBound );

SHPObject SHPAPI_CALL1(*)
      SHPReadObject( SHPHandle hSHP, int iShape );
int SHPAPI_CALL
      SHPWriteObject( SHPHandle hSHP, int iShape, SHPObject * psObject );

void SHPAPI_CALL
      SHPDestroyObject( SHPObject * psObject );
void SHPAPI_CALL
      SHPComputeExtents( SHPObject * psObject );
SHPObject SHPAPI_CALL1(*)
      SHPCreateObject( int nSHPType, int nShapeId, int nParts, 
                       const int * panPartStart, const int * panPartType,
                       int nVertices, 
                       const double * padfX, const double * padfY,
                       const double * padfZ, const double * padfM );
SHPObject SHPAPI_CALL1(*)
      SHPCreateSimpleObject( int nSHPType, int nVertices,
                             const double * padfX, 
                             const double * padfY, 
                             const double * padfZ );

int SHPAPI_CALL
      SHPRewindObject( SHPHandle hSHP, SHPObject * psObject );

void SHPAPI_CALL SHPClose( SHPHandle hSHP );
void SHPAPI_CALL SHPWriteHeader( SHPHandle hSHP );

const char SHPAPI_CALL1(*)
      SHPTypeName( int nSHPType );
const char SHPAPI_CALL1(*)
      SHPPartTypeName( int nPartType );

/* -------------------------------------------------------------------- */
/*      Shape quadtree indexing API.                                    */
/* -------------------------------------------------------------------- */

/* this can be two or four for binary or quad tree */
#define MAX_SUBNODE	4

/* upper limit of tree levels for automatic estimation */
#define MAX_DEFAULT_TREE_DEPTH 12

typedef struct shape_tree_node
{
    /* region covered by this node */
    double	adfBoundsMin[4];
    double	adfBoundsMax[4];

    /* list of shapes stored at this node.  The papsShapeObj pointers
       or the whole list can be NULL */
    int		nShapeCount;
    int		*panShapeIds;
    SHPObject   **papsShapeObj;

    int		nSubNodes;
    struct shape_tree_node *apsSubNode[MAX_SUBNODE];
    
} SHPTreeNode;

typedef struct
{
    SHPHandle   hSHP;
    
    int		nMaxDepth;
    int		nDimension;
    int         nTotalCount;
    
    SHPTreeNode	*psRoot;
} SHPTree;

SHPTree SHPAPI_CALL1(*)
      SHPCreateTree( SHPHandle hSHP, int nDimension, int nMaxDepth,
                     double *padfBoundsMin, double *padfBoundsMax );
void    SHPAPI_CALL
      SHPDestroyTree( SHPTree * hTree );

int	SHPAPI_CALL
      SHPWriteTree( SHPTree *hTree, const char * pszFilename );

int	SHPAPI_CALL
      SHPTreeAddShapeId( SHPTree * hTree, SHPObject * psObject );
int	SHPAPI_CALL
      SHPTreeRemoveShapeId( SHPTree * hTree, int nShapeId );

void 	SHPAPI_CALL
      SHPTreeTrimExtraNodes( SHPTree * hTree );

int    SHPAPI_CALL1(*)
      SHPTreeFindLikelyShapes( SHPTree * hTree,
                               double * padfBoundsMin,
                               double * padfBoundsMax,
                               int * );
int     SHPAPI_CALL
      SHPCheckBoundsOverlap( double *, double *, double *, double *, int );

int SHPAPI_CALL1(*) 
SHPSearchDiskTree( FILE *fp, 
                   double *padfBoundsMin, double *padfBoundsMax,
                   int *pnShapeCount );


typedef struct SHPDiskTreeInfo* SHPTreeDiskHandle;

SHPTreeDiskHandle SHPAPI_CALL
    SHPOpenDiskTree( const char* pszQIXFilename,
                     SAHooks *psHooks );

void SHPAPI_CALL
    SHPCloseDiskTree( SHPTreeDiskHandle hDiskTree );

int SHPAPI_CALL1(*) 
SHPSearchDiskTreeEx( SHPTreeDiskHandle hDiskTree, 
                   double *padfBoundsMin, double *padfBoundsMax,
                   int *pnShapeCount );

int SHPAPI_CALL
    SHPWriteTreeLL(SHPTree *hTree, const char *pszFilename, SAHooks *psHooks );

/************************************************************************/
/*                             DBF Support.                             */
/************************************************************************/
typedef	struct
{
    SAHooks sHooks;

    SAFile	fp;

    int         nRecords;

    int		nRecordLength;
    int		nHeaderLength;
    int		nFields;
    int		*panFieldOffset;
    int		*panFieldSize;
    int		*panFieldDecimals;
    char	*pachFieldType;

    char	*pszHeader;

    int		nCurrentRecord;
    int		bCurrentRecordModified;
    char	*pszCurrentRecord;

    int         nWorkFieldLength;
    char        *pszWorkField;
    
    int		bNoHeader;
    int		bUpdated;

    double      dfDoubleField;

    int         iLanguageDriver;
    char        *pszCodePage;
} DBFInfo;

typedef DBFInfo * DBFHandle;

typedef enum {
  FTString,
  FTInteger,
  FTDouble,
  FTLogical,
  FTInvalid
} DBFFieldType;

#define XBASE_FLDHDR_SZ       32


DBFHandle SHPAPI_CALL
      DBFOpen( const char * pszDBFFile, const char * pszAccess );
DBFHandle SHPAPI_CALL
      DBFOpenLL( const char * pszDBFFile, const char * pszAccess,
                 SAHooks *psHooks );
DBFHandle SHPAPI_CALL
      DBFCreate( const char * pszDBFFile );
DBFHandle SHPAPI_CALL
      DBFCreateEx( const char * pszDBFFile, const char * pszCodePage );
DBFHandle SHPAPI_CALL
      DBFCreateLL( const char * pszDBFFile, const char * pszCodePage, SAHooks *psHooks );

int	SHPAPI_CALL
      DBFGetFieldCount( DBFHandle psDBF );
int	SHPAPI_CALL
      DBFGetRecordCount( DBFHandle psDBF );
int	SHPAPI_CALL
      DBFAddField( DBFHandle hDBF, const char * pszFieldName,
                   DBFFieldType eType, int nWidth, int nDecimals );

int	SHPAPI_CALL
      DBFAddNativeFieldType( DBFHandle hDBF, const char * pszFieldName,
                             char chType, int nWidth, int nDecimals );

int	SHPAPI_CALL
      DBFDeleteField( DBFHandle hDBF, int iField );

int SHPAPI_CALL
      DBFReorderFields( DBFHandle psDBF, int* panMap );

int SHPAPI_CALL
      DBFAlterFieldDefn( DBFHandle psDBF, int iField, const char * pszFieldName,
                         char chType, int nWidth, int nDecimals );

DBFFieldType SHPAPI_CALL
      DBFGetFieldInfo( DBFHandle psDBF, int iField, 
                       char * pszFieldName, int * pnWidth, int * pnDecimals );

int SHPAPI_CALL
      DBFGetFieldIndex(DBFHandle psDBF, const char *pszFieldName);

int 	SHPAPI_CALL
      DBFReadIntegerAttribute( DBFHandle hDBF, int iShape, int iField );
double 	SHPAPI_CALL
      DBFReadDoubleAttribute( DBFHandle hDBF, int iShape, int iField );
const char SHPAPI_CALL1(*)
      DBFReadStringAttribute( DBFHandle hDBF, int iShape, int iField );
const char SHPAPI_CALL1(*)
      DBFReadLogicalAttribute( DBFHandle hDBF, int iShape, int iField );
int     SHPAPI_CALL
      DBFIsAttributeNULL( DBFHandle hDBF, int iShape, int iField );

int SHPAPI_CALL
      DBFWriteIntegerAttribute( DBFHandle hDBF, int iShape, int iField, 
                                int nFieldValue );
int SHPAPI_CALL
      DBFWriteDoubleAttribute( DBFHandle hDBF, int iShape, int iField,
                               double dFieldValue );
int SHPAPI_CALL
      DBFWriteStringAttribute( DBFHandle hDBF, int iShape, int iField,
                               const char * pszFieldValue );
int SHPAPI_CALL
     DBFWriteNULLAttribute( DBFHandle hDBF, int iShape, int iField );

int SHPAPI_CALL
     DBFWriteLogicalAttribute( DBFHandle hDBF, int iShape, int iField,
			       const char lFieldValue);
int SHPAPI_CALL
     DBFWriteAttributeDirectly(DBFHandle psDBF, int hEntity, int iField,
                               void * pValue );
const char SHPAPI_CALL1(*)
      DBFReadTuple(DBFHandle psDBF, int hEntity );
int SHPAPI_CALL
      DBFWriteTuple(DBFHandle psDBF, int hEntity, void * pRawTuple );

int SHPAPI_CALL DBFIsRecordDeleted( DBFHandle psDBF, int iShape );
int SHPAPI_CALL DBFMarkRecordDeleted( DBFHandle psDBF, int iShape, 
                                      int bIsDeleted );

DBFHandle SHPAPI_CALL
      DBFCloneEmpty(DBFHandle psDBF, const char * pszFilename );
 
void	SHPAPI_CALL
      DBFClose( DBFHandle hDBF );
void    SHPAPI_CALL
      DBFUpdateHeader( DBFHandle hDBF );
char    SHPAPI_CALL
      DBFGetNativeFieldType( DBFHandle hDBF, int iField );

const char SHPAPI_CALL1(*)
      DBFGetCodePage(DBFHandle psDBF );

#ifdef __cplusplus
}
#endif

#endif /* ndef SHAPEFILE_H_INCLUDED */
