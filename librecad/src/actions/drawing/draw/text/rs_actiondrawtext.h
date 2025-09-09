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

#ifndef RS_ACTIONDRAWTEXT_H
#define RS_ACTIONDRAWTEXT_H

#include "rs_previewactioninterface.h"

struct RS_TextData;

/**
 * This action class can handle user events to draw texts.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawText : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDrawText(LC_ActionContext *actionContext);
    ~RS_ActionDrawText() override;
    void init(int status) override;
    void reset();
    void preparePreview();
    QStringList getAvailableCommands() override;
    void setText(const QString &t);
    const QString &getText() const;
    void setUcsAngleDegrees(double a);
    double getUcsAngleDegrees() const;
protected:
    // fixme - sand - cmd -  add support of entering angle as part of command line
    /**
 * Action States.
 */
    enum Status {
        ShowDialog = InitialActionStatus,           /**< Showing the text dialog. */
        SetPos,               /**< Setting the position. */
        SetSecPos,            /**< Setting the second point for aligned of fit text. */
        SetText               /**< Setting the text in the command line. */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    std::unique_ptr<RS_TextData> m_textData;
    double m_ucsBasicAngleDegrees = 0.0;
    bool m_textChanged = false;
    bool m_snappedToRelZero = false;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
};
#endif
