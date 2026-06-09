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

class LC_PrintPreviewOptionsWidget;

/**
 * Default action for print preview.
 *
 * @author Andrew Mustun
 */
class RS_ActionPrintPreview : public RS_ActionInterface {
    Q_OBJECT
public:
    explicit RS_ActionPrintPreview(LC_ActionContext *actionContext);
    ~RS_ActionPrintPreview() override;

    void init(int status) override;
    void resume() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    QStringList getAvailableCommands() override;

    void center() const;
    void zoomToPage() const;
    void fit() const;
    bool setScale(double newScale, bool autoZoom = true) const;
    double getScale() const;
    void printWarning(const QString& s) const;
    void calcPagesNum(bool multiplePages);
    bool isLineWidthScaling() const;
    void setLineWidthScaling(bool state) const;
    void setBlackWhite(bool bw) const;
    bool isBlackWhite() const;

    RS2::Unit getUnit() const;
    void setPaperScaleFixed(bool fixed) const;
    bool isPaperScaleFixed() const;

    int getPagesNumHorizontal() const;
    void setPagesNumHorizontal(int pagesCount);
    int getPagesNumVertical() const;
    void setPagesNumVertical(int pagesCount);

    void invokeSettingsDialog() const;
    bool isPortrait() const;
    void setPaperOrientation(bool portrait) const;

    QStringList getStandardPrintScales() const;
    bool isUseImperialScales() const;

    QStringList readCustomRatios(bool metric, int maxCount);
    void saveCustomRatios(const QStringList& scales, int startIndex);
protected:
    /**
    * Action States.
    */
    enum Status {
        Neutral,
        Moving
    };
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    bool m_hasOptions = false;
    bool m_bPaperOffset = false;

    void updateActionPrompt() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    QString getAdditionalHelpMessage() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void zoomPageExWithBorder(int borderSize) const;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
};
#endif
