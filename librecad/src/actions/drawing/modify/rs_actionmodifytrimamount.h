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

class RS_AtomicEntity;
/**
 * This action class can handle user events to trim entities by a given
 * amount.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyTrimAmount:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionModifyTrimAmount(LC_ActionContext *actionContext);
    ~RS_ActionModifyTrimAmount() override;
    void init(int status) override;
    QStringList getAvailableCommands() override;
    double getDistance() const{return m_distance;}
    void setDistance(double d){m_distance = d;}
    bool isDistanceTotalLength() const{return m_distanceIsTotalLength;}
    void setDistanceIsTotalLength(bool on){m_distanceIsTotalLength = on;}
    void setSymmetricDistance(bool val){m_symmetricDistance = val;};
    bool isSymmetricDistance() const {return m_symmetricDistance;};
protected:
    /**
 * Action States.
 */
    enum Status {
        ChooseTrimEntity      /**< Choosing the entity to trim. */
    };

    RS_AtomicEntity *m_trimEntity = nullptr;
    std::unique_ptr<RS_Vector> m_trimCoord;
    double m_distance = 0.;
    bool m_distanceIsTotalLength = false;
    bool m_symmetricDistance = false;

    double determineDistance(const RS_AtomicEntity *e) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};
#endif
