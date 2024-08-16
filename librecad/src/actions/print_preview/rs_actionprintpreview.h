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

#ifndef RS_ACTIONPRINTPREVIEW_H
#define RS_ACTIONPRINTPREVIEW_H

#include <memory>

#include "rs_actioninterface.h"

class QG_PrintPreviewOptions;

/**
 * Default action for print preview.
 *
 * @author Andrew Mustun
 */
class RS_ActionPrintPreview : public RS_ActionInterface {
    Q_OBJECT
public:
    RS_ActionPrintPreview(RS_EntityContainer& container,
                          RS_GraphicView& graphicView);
    ~RS_ActionPrintPreview() override;

    void init(int status) override;
    void resume() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    QStringList getAvailableCommands() override;

    void center();
    void zoomToPage();
    void fit();
    bool setScale(double f, bool autoZoom = true);
    double getScale() const;
    void printWarning(const QString& s);
    void calcPagesNum(bool multiplePages);
    bool isLineWidthScaling();
    void setLineWidthScaling(bool state);
    void setBlackWhite(bool bw);
    bool isBlackWhite();

    RS2::Unit getUnit();
    void setPaperScaleFixed(bool fixed);
    bool isPaperScaleFixed();

    int getPagesNumHorizontal();
    void setPagesNumHorizontal(int pagesCount);
    int getPagesNumVertical();
    void setPagesNumVertical(int pagesCount);

    void invokeSettingsDialog();
    bool isPortrait();
    void setPaperOrientation(bool portrait);
protected:
    /**
    * Action States.
    */
    enum Status {
        Neutral,
        Moving
    };
    struct Points;
    std::unique_ptr<Points> pPoints;
    std::unique_ptr<QG_PrintPreviewOptions> m_option;
    bool hasOptions = false;
    bool m_bPaperOffset = false;

    void updateMouseButtonHints() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    QString getAdditionalHelpMessage() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void zoomPageExWithBorder(int borderSize);
    LC_ActionOptionsWidget* createOptionsWidget() override;
};
#endif
