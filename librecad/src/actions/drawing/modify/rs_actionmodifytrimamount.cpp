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

#include "rs_actionmodifytrimamount.h"

#include "qg_trimamountoptions.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_modification.h"

// fixme - sand - add support of selection of the endpoint that is fixed during length increase/descrease.
// currently, start point is fixed (and so the length is applied to endpoint only). However, it will be
// handy to let the user to select that too.
// fixme - sand - separate to line and arc lengthen, that will let to use different options in settings.

namespace {
    //list of entity types supported by current action. In general, entities that has proper implementation of trimStartpoint() and trimEndpoint()
    // might be supported. For now, we'll support Line and Arc
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc/*, RS2::EntityParabola,RS2::EntityEllipse*/};
}

RS_ActionModifyTrimAmount::RS_ActionModifyTrimAmount(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Trim Entity by a given amount",actionContext, RS2::ActionModifyTrimAmount)
    , m_trimEntity(nullptr), m_trimCoord(new RS_Vector{}), m_distance(0.0), m_distanceIsTotalLength(false){
}

RS_ActionModifyTrimAmount::~RS_ActionModifyTrimAmount() = default;

void RS_ActionModifyTrimAmount::init(int status) {
    RS_PreviewActionInterface::init(status);

    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
    m_snapMode.snapOnEntity = true;
}

// fixme - check if negative total length is larger than the overall length of the entity
void RS_ActionModifyTrimAmount::doTrigger() {
    RS_DEBUG->print("RS_ActionModifyTrimAmount::trigger()");
    RS_Modification m(*m_container, m_viewport, true);
    double dist = determineDistance(m_trimEntity);

    bool trimStart;
    bool trimEnd;
    bool trimBoth = m_symmetricDistance && !m_distanceIsTotalLength;
    m.trimAmount(*m_trimCoord, m_trimEntity, dist, trimBoth, trimStart, trimEnd);

    m_trimEntity = nullptr;
    setStatus(ChooseTrimEntity);
}

bool RS_ActionModifyTrimAmount::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if ("length" == tag) {
        setDistance(distance);
        return true;
    }
    return false;
}

double RS_ActionModifyTrimAmount::determineDistance(const RS_AtomicEntity *e) const{
    double d;
    if (m_distanceIsTotalLength){
        //the distance is taken as the new total length
        d = fabs(m_distance) - e->getLength();
    } else {
        d = m_distance;
    }
    return d;
}

void RS_ActionModifyTrimAmount::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Vector coord =  e->graphPoint;
    auto en = catchAndDescribe(e, g_enTypeList, RS2::ResolveNone);
    deleteSnapper();
    if (isAtomic(en)) {
        highlightHover(en);
        auto* atomic = static_cast<RS_AtomicEntity*>(en);
        RS_Modification m(*m_container, m_viewport, false);
        double dist = determineDistance(atomic);
        bool trimBoth = m_symmetricDistance && !m_distanceIsTotalLength;
        bool trimStart;
        bool trimEnd;
        auto trimmed = m.trimAmount(coord, atomic, dist, trimBoth, trimStart, trimEnd, true);
        if (trimmed != nullptr) {
            double originalLen = atomic->getLength();
            double trimmedLen = trimmed->getLength();
            bool increased = originalLen < trimmedLen;
            if (increased) {
                previewEntity(trimmed);
            }

            if (m_showRefEntitiesOnPreview) {
                RS_Arc* atomicArc = nullptr;
                RS_Arc* trimmedArc = nullptr;
                bool entityIsArc = isArc(atomic);
                if (entityIsArc) {
                    atomicArc = dynamic_cast<RS_Arc*>(atomic);
                    trimmedArc = dynamic_cast<RS_Arc*>(trimmed);
                }

                if (trimStart) {
                    const RS_Vector& originalStart = atomic->getStartpoint();
                    const RS_Vector& trimmedStart = trimmed->getStartpoint();
                    previewRefSelectablePoint(trimmedStart);
                    previewRefPoint(originalStart);
                    if (!increased) {
                        if (isLine(atomic)) { // fixme - support of polyline
                            previewRefLine(originalStart, trimmedStart);
                        }
                        else if (entityIsArc) {
                            const RS_ArcData& arcData = RS_ArcData(atomicArc->getCenter(), atomicArc->getRadius(),
                                                                   atomicArc->getAngle1(),
                                                                   trimmedArc->getAngle1(),
                                                                   atomicArc->isReversed());
                            previewRefArc(arcData);
                        }
                    }
                }

                if (trimEnd) {
                    const RS_Vector& originalEnd = atomic->getEndpoint();
                    const RS_Vector& trimmedEnd = trimmed->getEndpoint();
                    previewRefSelectablePoint(trimmedEnd);
                    previewRefPoint(originalEnd);
                    if (!increased) {
                        if (isLine(atomic)) { // fixme - support of polyline
                            previewRefLine(originalEnd, trimmedEnd);
                        }
                        else if (entityIsArc) {
                            const RS_ArcData& arcData = RS_ArcData(atomicArc->getCenter(), atomicArc->getRadius(),
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

void RS_ActionModifyTrimAmount::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseTrimEntity: {
            *m_trimCoord = e->graphPoint;
            RS_Entity* en = catchEntityByEvent(e, g_enTypeList, RS2::ResolveNone);
            if (en == nullptr){
                commandMessage(tr("No entity found."));
            }
            else if (en->isAtomic()){
                m_trimEntity = dynamic_cast<RS_AtomicEntity *>(en);
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
    invalidateSnapSpot();
}

void RS_ActionModifyTrimAmount::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
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
                m_distance = d;
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
            updateMouseWidgetTRBack(tr("Select line/arc to trim OR enter length value:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyTrimAmount::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
