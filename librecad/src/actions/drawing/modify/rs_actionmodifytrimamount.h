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

#ifndef RS_ACTIONMODIFYTRIMAMOUNT_H
#define RS_ACTIONMODIFYTRIMAMOUNT_H

#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to trim entities by a given
 * amount.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyTrimAmount:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionModifyTrimAmount(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionModifyTrimAmount() override;
    void init(int status) override;
    void trigger() override;
    QStringList getAvailableCommands() override;
    double getDistance() const{return distance;}
    void setDistance(double d){distance = d;}
    bool isDistanceTotalLength() const{return distanceIsTotalLength;}
    void setDistanceIsTotalLength(bool on){distanceIsTotalLength = on;}
    void setSymmetricDistance(bool val){symmetricDistance = val;};
    bool isSymmetricDistance() const {return symmetricDistance;};
    void mouseMoveEvent(QMouseEvent *event) override;

protected:
    /**
 * Action States.
 */
    enum Status {
        ChooseTrimEntity      /**< Choosing the entity to trim. */
    };

    RS_AtomicEntity *trimEntity = nullptr;
    std::unique_ptr<RS_Vector> trimCoord;
    double distance = 0.;
    bool distanceIsTotalLength = false;
    bool symmetricDistance = false;
    /**
     * Commands
     */
    /*
    QString cmdDistance;
    QString cmdDistance2;
    QString cmdDistance3;
    */
    double determineDistance(const RS_AtomicEntity *e) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
};
#endif
