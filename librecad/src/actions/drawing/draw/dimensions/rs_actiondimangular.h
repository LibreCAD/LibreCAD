/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_ACTIONDIMANGULAR_H
#define RS_ACTIONDIMANGULAR_H
#include "rs_actiondimension.h"

struct RS_DimAngularData;

/**
 * This action class can handle user events to draw angular dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimAngular : public RS_ActionDimension{
    Q_OBJECT
public:
    RS_ActionDimAngular(LC_ActionContext *actionContext);
    ~RS_ActionDimAngular() override;
    QStringList getAvailableCommands() override;
protected:
    enum Status {
        SetLine1 = InitialActionStatus,      ///< Choose 1st line
        SetLine2,      ///< Choose 2nd line
        SetPos,        ///< Choose position
        SetText        ///< Setting text label in console
    };
    RS_Line*     m_line1 = nullptr;                          ///< 1st chosen line
    RS_Line*     m_line2 = nullptr;                          ///< 2nd chosen line
    RS_Vector   m_click1;                         ///< 1st click pos
    RS_Vector   m_click2;                         ///< 2nd click pos
    RS_Vector   m_center;                         ///< Center of arc
    std::unique_ptr<RS_DimAngularData> m_edata;   ///< Data of new dimension
    Status      m_lastStatus = SetLine1;                     ///< Last status before entering text
    std::vector<double> m_angles;                 ///< Array to sort line angles
    int         m_quadrantOffset {0};             ///< Offset on starting determineQuadrant
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    void reset() override;
    RS_LineData justify( RS_Line* line, const RS_Vector &click);
    void lineOrder(const RS_Vector &dimPos, RS_LineData& ld1, RS_LineData& ld2);
    int determineQuadrant(const double angle);
    bool setData(const RS_Vector& dimPos, const bool calcCenter = false);
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void setFirstLine(RS_Entity* en, const RS_Vector& pos);
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    void doTrigger() override;
};
#endif
