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

#ifndef RS_ACTIONDRAWLINEBISECTOR_H
#define RS_ACTIONDRAWLINEBISECTOR_H

#include "rs_previewactioninterface.h"

class RS_Line;

/**
 * This action class can handle user events to draw bisectors.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineBisector:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    explicit RS_ActionDrawLineBisector(LC_ActionContext *actionContext);
    ~RS_ActionDrawLineBisector() override;
    void init(int status) override;
    QStringList getAvailableCommands() override;
    void setLength(double l);
    double getLength() const;
    void setNumber(int n);
    int getNumber() const;
    void setStatus(int status) override;

protected:
    enum Status {
        SetLine1 = InitialActionStatus,     /**< Choose the 1st line. */
        SetLine2,     /**< Choose the 2nd line. */
        SetLength,    /**< Set length in command line. */
        SetNumber     /**< Set number in command line. */
    };

    /** Closest bisector. */
    RS_Line *m_bisector = nullptr;
    /** First chosen entity */
    RS_Line *m_line1 = nullptr;
    /** Second chosen entity */
    RS_Line *m_line2 = nullptr;
    /** Length of the bisector. */
    double m_length = 0.;
    /** Number of bisectors to create. */
    int m_numberToCreate = 0;
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    /** Last status before entering length or number. */
    Status m_lastStatus = SetLine1;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void setFirstLine(RS_Entity* en);
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};
#endif
