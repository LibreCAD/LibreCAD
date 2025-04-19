/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#ifndef QC_ACTIONGETPOINT_H
#define QC_ACTIONGETPOINT_H

#include "rs_previewactioninterface.h"

class QPointF;

/**
 * This action class can handle user events to get a point from plugin.
 *
 * @author  Rallaz
 */
class QC_ActionGetPoint : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    QC_ActionGetPoint(LC_ActionContext *actionContext);
    ~QC_ActionGetPoint();

    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void getPoint(QPointF *point);
    void setBasepoint(QPointF* basepoint);
    void setMessage(QString msg);
    bool isCompleted(){return m_completed;}
    bool wasCanceled(){return m_canceled;}
protected:
    bool m_canceled;
    bool m_completed;
    bool m_setTargetPoint;
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};
#endif
