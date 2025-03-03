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

#include<cmath>


#include <QMouseEvent>

#include "rs_actionmodifytrimamount.h"
#include "rs_atomicentity.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "qg_trimamountoptions.h"

namespace {

    //list of entity types supported by current action. In general, entities that has proper implementation of trimStartpoint() and trimEndpoint()
    // might be supported. For now, we'll support Line and Arc
    const auto enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc/*, RS2::EntityParabola,RS2::EntityEllipse*/};
}

RS_ActionModifyTrimAmount::RS_ActionModifyTrimAmount(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Trim Entity by a given amount",
                               container, graphicView), trimEntity(nullptr), trimCoord(new RS_Vector{}), distance(0.0), distanceIsTotalLength(false){
    actionType = RS2::ActionModifyTrimAmount;
}

RS_ActionModifyTrimAmount::~RS_ActionModifyTrimAmount() = default;

void RS_ActionModifyTrimAmount::init(int status) {
    RS_PreviewActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
    snapMode.snapOnEntity = true;
}

// fixme - check if negative total length is larger than the overall length of the entity

void RS_ActionModifyTrimAmount::trigger(){

    RS_DEBUG->print("RS_ActionModifyTrimAmount::trigger()");

    if (trimEntity && trimEntity->isAtomic()){

        RS_Modification m(*container, graphicView, true);
        auto* e = dynamic_cast<RS_AtomicEntity *>(trimEntity);
        double dist = determineDistance(e);

        bool trimStart;
        bool trimEnd;

        bool trimBoth = symmetricDistance && !distanceIsTotalLength;

        m.trimAmount(*trimCoord, e, dist, trimBoth, trimStart, trimEnd);

        trimEntity = nullptr;
        setStatus(ChooseTrimEntity);

        updateSelectionWidget();

        graphicView->redraw();
    }
}

double RS_ActionModifyTrimAmount::determineDistance(const RS_AtomicEntity *e) const{
    double d;
    if (distanceIsTotalLength){
        //the distance is taken as the new total length
        d = fabs(distance) - e->getLength();
    } else {
        d = distance;
    }
    return d;
}

void RS_ActionModifyTrimAmount::mouseMoveEvent(QMouseEvent *e){
    snapPoint(e);
    RS_Vector coord =  toGraph(e);
    auto en = catchEntity(e, enTypeList, RS2::ResolveNone);
    deletePreview();
    deleteHighlights();
    if (en != nullptr){
        if (en->isAtomic()){
            highlightHover(en);
            auto* atomic = reinterpret_cast<RS_AtomicEntity *>(en);
            RS_Modification m(*container, nullptr, false);
            double dist = determineDistance(atomic);
            bool trimBoth = symmetricDistance && !distanceIsTotalLength;
            bool trimStart;
            bool trimEnd;
            auto trimmed = m.trimAmount(coord, atomic, dist, trimBoth, trimStart, trimEnd, true);
            if (trimmed != nullptr){
                double originalLen = atomic->getLength();
                double trimmedLen = trimmed->getLength();
                bool increased = originalLen < trimmedLen;
                if (increased){
                    previewEntity(trimmed);
                }

                if (showRefEntitiesOnPreview) {
                    RS_Arc *atomicArc = nullptr;
                    RS_Arc *trimmedArc = nullptr;
                    bool entityIsArc = isArc(atomic);
                    if (entityIsArc) {
                        atomicArc = dynamic_cast<RS_Arc *>(atomic);
                        trimmedArc = dynamic_cast<RS_Arc *>(trimmed);
                    }


                    if (trimStart) {
                        const RS_Vector &originalStart = atomic->getStartpoint();
                        const RS_Vector &trimmedStart = trimmed->getStartpoint();
                        previewRefSelectablePoint(trimmedStart);
                        previewRefPoint(originalStart);
                        if (!increased) {
                            if (isLine(atomic)) {
                                previewRefLine(originalStart, trimmedStart);
                            } else if (entityIsArc) {
                                const RS_ArcData &arcData = RS_ArcData(atomicArc->getCenter(), atomicArc->getRadius(),
                                                                       atomicArc->getAngle1(),
                                                                       trimmedArc->getAngle1(),
                                                                       atomicArc->isReversed());
                                previewRefArc(arcData);
                            }
                        }
                    }

                    if (trimEnd) {
                        const RS_Vector &originalEnd = atomic->getEndpoint();
                        const RS_Vector &trimmedEnd = trimmed->getEndpoint();
                        previewRefSelectablePoint(trimmedEnd);
                        previewRefPoint(originalEnd);
                        if (!increased) {
                            if (isLine(atomic)) {
                                previewRefLine(originalEnd, trimmedEnd);
                            } else if (entityIsArc) {
                                const RS_ArcData &arcData = RS_ArcData(atomicArc->getCenter(), atomicArc->getRadius(),
                                                                       atomicArc->getAngle2(),
                                                                       trimmedArc->getAngle2(),
                                                                       atomicArc->isReversed());
                                previewRefArc(arcData);
                            }
                        }
                    }
                }
            }
        }
    }
    drawPreview();
    drawHighlights();
}

void RS_ActionModifyTrimAmount::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case ChooseTrimEntity: {
            *trimCoord = toGraph(e);
            RS_Entity* en = catchEntity(e, enTypeList, RS2::ResolveNone);
            if (en == nullptr){
                commandMessage(tr("No entity found."));
            }
            else if (en->isAtomic()){
                trimEntity = dynamic_cast<RS_AtomicEntity *>(en);
                trigger();
            }
            else {
                commandMessage(tr("The chosen Entity is not an atomic entity or cannot be trimmed."));
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyTrimAmount::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    initPrevious(status);
}

// fixme - support for other options via command line (currently only length may be set) (???)
bool RS_ActionModifyTrimAmount::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case ChooseTrimEntity: {
            bool ok;
            double d = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                distance = d;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            // fixme - should we allow change status for invalid input?
            updateOptions();
            setStatus(ChooseTrimEntity);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionModifyTrimAmount::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case ChooseTrimEntity:
            break;
        default:
            break;
    }
    return cmd;
}

LC_ActionOptionsWidget* RS_ActionModifyTrimAmount::createOptionsWidget(){
    return new QG_TrimAmountOptions();
}

void RS_ActionModifyTrimAmount::updateMouseButtonHints() {
    switch (getStatus()) {
        case ChooseTrimEntity:
            updateMouseWidgetTRBack(tr("Select line, arc, ellipse or parabola to trim or enter distance:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyTrimAmount::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
