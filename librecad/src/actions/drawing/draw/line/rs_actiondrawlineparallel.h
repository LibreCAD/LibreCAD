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

#ifndef RS_ACTIONDRAWLINEPARALLEL_H
#define RS_ACTIONDRAWLINEPARALLEL_H

#include "lc_latecompletionrequestor.h"
#include "rs_previewactioninterface.h"

class RS_Vector;
/**
 * This action class can handle user events to draw parallel 
 * lines, arcs and circles.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineParallel:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDrawLineParallel(LC_ActionContext *actionContext, RS2::ActionType actionType);
    ~RS_ActionDrawLineParallel() override;
    QStringList getAvailableCommands() override;
    double getDistance() const;
    void setDistance(double d);
    int getNumber() const;
    void setNumber(int n);
private:
    // fixme - why no possibility to set distance via command line?
    enum Status {
        SetEntity = InitialActionStatus,    /**< Choose original entity. */
//SetDistance,  /**< Setting distance in the command line. */
        SetNumber     /**< Setting number in the command line. */
//SetThrough     /**< Setting a point for the parallel to go through. */
    };

    /** Closest parallel. */
    RS_Entity *m_parallel = nullptr;
    /** Distance of the parallel. */
    double m_distance = 0.;
    /** Number of parallels. */
    int m_numberToCreate = 0;
    /** Coordinate of the mouse. */
    std::unique_ptr<RS_Vector> m_coord;
    /** Original entity. */
    RS_Entity *m_entity = nullptr;
protected:
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
private:
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
};
#endif
