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
protected:
    void createOptionsWidget() override;
public:
    /**
     * Action States.
     */
    enum Status {
        ShowDialog,           /**< Showing the text dialog. */
        SetPos,               /**< Setting the position. */
        SetText               /**< Setting the text in the command line. */
    };

public:
    RS_ActionDrawMText(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawMText() override;
    void init(int status = 0) override;
    void reset();
    void trigger() override;
    void preparePreview();
    void mouseMoveEvent(QMouseEvent *e) override;
    void coordinateEvent(RS_CoordinateEvent *e) override;
    QStringList getAvailableCommands() override;
    void updateMouseButtonHints() override;
    void setText(const QString &t);
    QString getText();
    void setAngle(double a);
    double getAngle();
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) override;
    void mouseRightButtonReleaseEvent(int status, QMouseEvent *e) override;

    bool doProcessCommand(int status, const QString &command) override;

private:
    std::unique_ptr<RS_MTextData> data;
    //RS_Text* text;
    std::unique_ptr<RS_Vector> pos;
    bool textChanged = false;
};

#endif
