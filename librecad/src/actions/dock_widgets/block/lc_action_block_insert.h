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
#ifndef RS_ACTIONBLOCKSINSERT_H
#define RS_ACTIONBLOCKSINSERT_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class RS_Block;
struct RS_InsertData;

/**
 * This action class can handle user events for inserting blocks into the
 * current drawing.
 *
 * @author Andrew Mustun
 */
class LC_ActionBlockInsert:public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    explicit LC_ActionBlockInsert(LC_ActionContext *actionContext);
    ~LC_ActionBlockInsert() override;
    void init(int status) override;
    void reset();
    QStringList getAvailableCommands() override;
    double getAngle() const;
    void setAngle(double a) const;
    double getFactor() const;
    void setFactor(double f) const;
    int getColumns() const;
    void setColumns(int c) const;
    int getRows() const;
    void setRows(int r) const;
    double getColumnSpacing() const;
    void setColumnSpacing(double cs) const;
    double getRowSpacing() const;
    void setRowSpacing(double rs) const;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetUndefined     = -1, /**< Setting undefined for initialisation. */
        SetTargetPoint   = 0, /**< Setting the reference point. */
        SetAngle         = 1, /**< Setting angle in the command line. */
        SetFactor        = 2, /**< Setting factor in the command line. */
        SetColumns       = 3, /**< Setting columns in the command line. */
        SetRows          = 4, /**< Setting rows in the command line. */
        SetColumnSpacing = 5, /**< Setting column spacing in the command line. */
        SetRowSpacing    = 6 /**< Setting row spacing in the command line. */
    };

    RS_Block *m_block = nullptr;
    std::unique_ptr<RS_InsertData> m_data;
    /** Last status before entering option. */
    Status m_lastStatus = SetUndefined;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateActionPrompt() override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* event) override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;

    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;

    void doSaveOptions() override;
    void doLoadOptions() override;
};
#endif
