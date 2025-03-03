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

#ifndef RS_ACTIONDIMDIAMETRIC_H
#define RS_ACTIONDIMDIAMETRIC_H

#include <memory>

#include "rs_actiondimension.h"
#include "rs_dimdiametric.h"
#include "lc_actioncircledimbase.h"

struct RS_DimDiametricData;

/**
 * This action class can handle user events to draw diametric dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimDiametric:public LC_ActionCircleDimBase {
Q_OBJECT
public:
    RS_ActionDimDiametric(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDimDiametric() override;
protected:
    /** Data of new dimension */
    std::unique_ptr<RS_DimDiametricData> edata;
    void reset() override;
    RS_Dimension *createDim(RS_EntityContainer *parent) const override;
    RS_Vector preparePreview(RS_Entity *en, RS_Vector &position, bool forcePosition) override;
};
#endif
