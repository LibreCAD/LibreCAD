#ifndef LC_ORTHOGONALGRID_H
#define LC_ORTHOGONALGRID_H

#include "lc_lattice.h"
#include "lc_metagrid.h"

class LC_OrthogonalGrid
{
public:
    LC_OrthogonalGrid();



  void init(const RS_Vector &viewZero, const RS_Vector &viewSize, RS_Vector const &metaGridWidth, RS_Vector const &gridWidth, bool metaGridVisible);


protected:
    LC_Lattice* lattice;
};

#endif // LC_ORTHOGONALGRID_H
