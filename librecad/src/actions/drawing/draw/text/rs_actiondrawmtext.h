
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

#ifndef RS_ACTIONDRAWMTEXT_H
#define RS_ACTIONDRAWMTEXT_H

#include "rs_previewactioninterface.h"

struct RS_MTextData;

/**
 * This action class can handle user events to draw texts.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawMText:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDrawMText(LC_ActionContext *actionContext);
    ~RS_ActionDrawMText() override;
    void init(int status) override;
    void reset();
    void preparePreview();
    QStringList getAvailableCommands() override;
    void setText(const QString &t);
    QString getText();
    void setUcsAngleDegrees(double a);
    double getUcsAngleDegrees();
protected:
    /**
    * Action States.
    */
    enum Status {
        ShowDialog,           /**< Showing the text dialog. */
        SetPos,               /**< Setting the position. */
        SetText               /**< Setting the text in the command line. */
    };
    std::unique_ptr<RS_MTextData> m_mtextData;
    //RS_Text* text;
    std::unique_ptr<RS_Vector> m_pos;
    bool m_textChanged = false;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
};
#endif
