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

#ifndef RS_ACTIONDRAWPOLYLINE_H
#define RS_ACTIONDRAWPOLYLINE_H

#include <memory>
#include "rs_previewactioninterface.h"
#include "rs_polyline.h"

namespace mu {
    class Parser;
}

class RS_EntityContainer;
class RS_GraphicView;



/**
 * This action class can handle user events to draw 
 * simple lines with the start- and endpoint given.
 *
 * @author Andrew Mustun
*/
class RS_ActionDrawPolyline : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    enum SegmentMode {
        Line = 0,
        Tangential = 1,
        TanRad = 2,
        // TanAng,
        // TanRadAng,
        Ang = 3,
        // RadAngEndp,
        // RadAngCenp
    };

    RS_ActionDrawPolyline(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~RS_ActionDrawPolyline() override;

    void reset();

    void init(int status) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent *e) override;
    QStringList getAvailableCommands() override;
    void close();

    virtual void undo();
    void setMode(SegmentMode m);
    int getMode() const;
    void setRadius(double r);
    double getRadius() const;
    void setAngle(double a);
    double getAngle() const;
    void setReversed(bool c);
    bool isReversed() const;
    double solveBulge(const RS_Vector& mouse);
protected:
    /**
            * Action States.
            */
    enum Status {
        SetStartpoint, /*  Setting the startpoint.  */
        SetNextPoint,  /*  Setting the endpoint.    */
    };

    RS_Polyline*& getPolyline() const;
    QList<RS_Vector>& getHistory() const;
    QList<double>& getBHistory() const;
    RS_Vector& getPoint() const;
    RS_Vector& getStart() const;
    RS_PolylineData& getData() const;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    double m_radius = 0.;
    double m_angle = 0.;
    SegmentMode m_mode{};
    int m_reversed = 1;
    bool m_calculatedSegment = false;

    struct Points {

/**
 * Line data defined so far.
 */
        RS_PolylineData data;
        RS_ArcData arc_data;
        /**
      * Polyline entity we're working on.
      */
        RS_Polyline* polyline;

        /**
      * last point.
      */
        RS_Vector point;
        RS_Vector calculatedEndpoint;
        /**
      * Start point of the series of lines. Used for close function.
      */
        RS_Vector start;

        /**
      * Point history (for undo)
      */
        QList<RS_Vector> history;

        /**
      * Bulge history (for undo)
      */
        QList<double> bHistory;
        QString equation;
    };

    std::unique_ptr<Points> pPoints;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    QString prepareCommand(RS_CommandEvent *e) const override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
private:
    void drawEquation(int numberOfPolylines);
    void setParserExpression(const QString& expression);
    bool getPlottingX(QString command, double& x);
    std::unique_ptr<mu::Parser> m_muParserObject;

    double startPointX = 0.;
    double endPointX = 0.;
    double startPointY = 0.;
    double endPointY = 0.;
    bool shiftX = false;
    bool equationSettingOn = false;
    bool startPointSettingOn = false;
    bool endPointSettingOn = false;
    bool stepSizeSettingOn = false;
    bool shiftY = false;
};
#endif
