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

#ifndef RS_ACTIONDRAWHATCH_H
#define RS_ACTIONDRAWHATCH_H

#include "rs_previewactioninterface.h"
#include "lc_actionpreselectionawarebase.h"

struct RS_HatchData;

/**
 * This action class can handle user events to draw hatches.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawHatch : public LC_ActionPreSelectionAwareBase {
Q_OBJECT
public:
    RS_ActionDrawHatch(RS_EntityContainer& container,
                       RS_GraphicView& graphicView);
    ~RS_ActionDrawHatch() override;
    void init(int status) override;
    void setShowArea(bool s);
protected:
    /**
     * Action States.
     */
    enum Status {
        ShowDialog           /**< Showing the hatch dialog. */
    };
    std::unique_ptr<RS_HatchData> data;
    bool m_bShowArea{true};
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool keepSelected) override;
    bool isAllowTriggerOnEmptySelection() override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void doSelectEntity(RS_Entity* entityToSelect, bool selectContour) const override;
    bool isEntityAllowedToSelect(RS_Entity *ent) const override;
};
#endif
