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

#ifndef RS_ACTIONMODIFYMIRROR_H
#define RS_ACTIONMODIFYMIRROR_H

#include "lc_actionmodifybase.h"

struct RS_MirrorData;

/**
 * This action class can handle user events to mirror entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyMirror:public LC_ActionModifyBase {
    Q_OBJECT
public:
    RS_ActionModifyMirror(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionModifyMirror() override;
    void trigger() override;
    bool isMirrorToExistingLine() const {return mirrorToExistingLine;};
    void setMirrorToExistingLine(bool value);
protected:
    /**
    * Action States.
    */
    enum Status {
        SetAxisPoint1,    /**< Setting the 1st point of the axis. */
        SetAxisPoint2,    /**< Setting the 2nd point of the axis. */
        ShowDialog,        /**< Showing the options dialog. */
    };
    struct Points;
    std::unique_ptr<Points> pPoints;
    bool mirrorToExistingLine;
    void previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2);
    void showOptionsAndTrigger();
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseMoveEventSelected(QMouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void doTrigger();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
};
#endif
