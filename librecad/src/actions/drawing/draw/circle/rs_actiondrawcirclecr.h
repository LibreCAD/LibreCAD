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

#ifndef RS_ACTIONDRAWCIRCLECR_H
#define RS_ACTIONDRAWCIRCLECR_H

#include "rs_previewactioninterface.h"
#include "lc_actiondrawcirclebase.h"

struct RS_CircleData;

/**
 * This action class can handle user events to draw 
 * simple arcs with the center, radius, start- and endangle given.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawCircleCR:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleCR(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawCircleCR() override;
    void reset() override;
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    QStringList getAvailableCommands() override;
    double getRadius() const;
    void setRadius(double val);
protected:
    /**
     * Action States.
     */
    enum Status {
        SetCenter,         /**< Setting the center point. */
        SetRadius          /**< Setting radius in command line. */
    };
    LC_ActionOptionsWidget* createOptionsWidget() override;
    /**
     * Circle data defined so far.
     */
    std::unique_ptr<RS_CircleData> data;
    bool setRadiusStr(const QString &sr) ;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};
#endif
