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

#ifndef RS_ACTIONLIBRARYINSERT_H
#define RS_ACTIONLIBRARYINSERT_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_creation.h"
#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events for inserting library items 
 * (or any other DXF files) into the current drawing (as block/insert).
 *
 * @author Andrew Mustun
 */
class LC_ActionBlockLibraryInsert : public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionBlockLibraryInsert(LC_ActionContext *actionContext);
    ~LC_ActionBlockLibraryInsert() override;
    void init(int status) override;
    void reset() const;
    QStringList getAvailableCommands() override;
    void setFile(const QString& file) const;
    double getAngle() const;
    void setAngle(double a) const;
    double getFactor() const;
    void setFactor(double f) const;
/*int getColumns() {
 return data.cols;
}

void setColumns(int c) {
 data.cols = c;
}

int getRows() {
 return data.rows;
}

void setRows(int r) {
 data.rows = r;
}

double getColumnSpacing() {
 return data.spacing.x;
}

void setColumnSpacing(double cs) {
 data.spacing.x = cs;
}

double getRowSpacing() {
 return data.spacing.y;
}

void setRowSpacing(double rs) {
 data.spacing.y = rs;
}*/

protected:
    /**
   * Action States.
   */
    enum Status {
        SetTargetPoint,    /**< Setting the reference point. */
        SetAngle,          /**< Setting angle in the command line. */
        SetFactor          /**< Setting factor in the command line. */
//SetColumns,        /**< Setting columns in the command line. */
//SetRows,           /**< Setting rows in the command line. */
//SetColumnSpacing,  /**< Setting column spacing in the command line. */
//SetRowSpacing      /**< Setting row spacing in the command line. */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    struct RS_LibraryInsertData;

/** Last status before entering option. */
    Status m_lastStatus = SetTargetPoint;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateActionPrompt() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
};
#endif
