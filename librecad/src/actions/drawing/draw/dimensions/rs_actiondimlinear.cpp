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

#include "rs_actiondimlinear.h"

#include "lc_linemath.h"
#include "rs_constructionline.h"
#include "rs_dimlinear.h"

/**
 * Constructor.
 *
 * @param actionContext
 * @param angle Initial angle in rad.
 * @param fixedAngle
 * @param type
 * @param fixedAngle true: The user can't change the angle.
 *                   false: The user can change the angle in a option widget.
 */
RS_ActionDimLinear::RS_ActionDimLinear(LC_ActionContext *actionContext,double angle, const bool fixedAngle, const RS2::ActionType type)
    :LC_ActionDimLinearBase("Draw linear dimensions", actionContext, RS2::EntityDimLinear, type),
     m_edata(std::make_unique<RS_DimLinearData>(RS_Vector(0., 0.), RS_Vector(0., 0.), angle, 0.)),
     m_fixedAngle{fixedAngle}{
    setUcsAngleDegrees(angle);
    updateOptions();
    RS_ActionDimLinear::reset();
}

RS_ActionDimLinear::~RS_ActionDimLinear() = default;

void RS_ActionDimLinear::reset(){
    RS_ActionDimension::reset();

    const double angleDeg = m_fixedAngle ? m_ucsBasisAngleDegrees : 0; // keep selected angle

    *m_edata = {{}, {}, toWorldAngleFromUCSBasisDegrees(angleDeg), toWorldAngle(0.0)};
}

void RS_ActionDimLinear::preparePreview(const bool alternateMode){
    const double angle = getDimAngle(alternateMode);
    const RS_Vector dirV = RS_Vector::polar(100., angle + M_PI_2);
    const RS_ConstructionLine cl(nullptr,RS_ConstructionLineData(m_edata->extensionPoint2,m_edata->extensionPoint2 + dirV));
    m_dimensionData->definitionPoint = cl.getNearestPointOnEntity(m_dimensionData->definitionPoint);
}

RS_Entity *RS_ActionDimLinear::createDim(RS_EntityContainer* parent){
    m_edata->angle = getDimAngle(m_alternateDimDirection);
    m_edata->obliqueAngle =  toWorldAngle(0.0);
    auto *dim = new RS_DimLinear(parent, *m_dimensionData, *m_edata);
    return dim;
}

double RS_ActionDimLinear::getUcsAngleDegrees() const{
    return m_ucsBasisAngleDegrees;
}

void RS_ActionDimLinear::setUcsAngleDegrees(const double ucsRelAngleDegrees){
    m_ucsBasisAngleDegrees = ucsRelAngleDegrees;
}

bool RS_ActionDimLinear::hasFixedAngle() const{
    return m_fixedAngle;
}

void RS_ActionDimLinear::onCoordinateEvent(const int status, const bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetAngle:{
            if (isZero){
                setUcsAngleDegrees(0);
                updateOptions();
                setStatus(m_lastStatus);
                return;
            }
            break;
        }
        default:
            break;
    }
    LC_ActionDimLinearBase::onCoordinateEvent(status, isZero, pos);
}

bool RS_ActionDimLinear::doProcessCommand(const int status, const QString &command) {
    bool accept = false;
    switch (status) {
        case SetText: {
            setText(command);
            updateOptions();
            enableCoordinateInput();
            setStatus(m_lastStatus);
            accept = true;
            break;
        }
        case SetAngle: {
            double ucsBasisAngleDeg;
            const bool ok = parseToUCSBasisAngle(command, ucsBasisAngleDeg);
            if (ok){
                accept = true;
                m_ucsBasisAngleDegrees = ucsBasisAngleDeg;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        default:
            m_lastStatus = static_cast<Status>(getStatus());
            deletePreview();
            if (checkCommand("text", command)){
                disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            } else if (!m_fixedAngle && checkCommand("angle", command)){
                setStatus(SetAngle);
                accept = true;
            }
            break;
    }
    return accept;
}

QStringList RS_ActionDimLinear::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetExtPoint1:
        case SetExtPoint2:
        case SetDefPoint: {
            cmd += command("text");
            if (!m_fixedAngle){
                cmd += command("angle");
            }
            break;
        }
        default:
            break;
    }
    return cmd;
}

RS_Vector RS_ActionDimLinear::getExtensionPoint1(){
    return m_edata->extensionPoint1;
}

RS_Vector RS_ActionDimLinear::getExtensionPoint2(){
    return m_edata->extensionPoint2;
}

double RS_ActionDimLinear::getDimAngle(const bool alternateMode){
// Todo - sand - option for dim behaviour?
// This is good place for optional behavior for Hor/Vert dims.
// Implementation below created dims orthogonal to X and Y axises. 0 in UCS Abs values, ignore basis there??
// however, if translate angles using toWorldAngleFromUCSBasis(), dims will be orthogonal to angles basis.
// not sure whether this is necessary so far, yet still - an option may be added to the options widget.

    switch (m_actionType){
        case RS2::ActionDimLinearHor:{
            const double angle = alternateMode ? M_PI_2: 0;
            return toWorldAngle(angle);
        }
        case RS2::ActionDimLinearVer:{
            const double angle = alternateMode ? 0: M_PI_2;
            return toWorldAngle(angle);
        }
        default:{
            double angleDegrees = m_ucsBasisAngleDegrees;
            if (alternateMode) {
                if (!LC_LineMath::isMeaningfulAngle(m_ucsBasisAngleDegrees)) {
                    angleDegrees = 90.0;
                }
                else if (!LC_LineMath::isMeaningfulAngle(m_ucsBasisAngleDegrees - 90.0)) {
                    angleDegrees = 0.0;
                }
            }
            return toWorldAngleFromUCSBasisDegrees(angleDegrees);
        }
    }
}

bool RS_ActionDimLinear::checkMaySwitchDimDirection() const {
    bool maySwitchDirection = m_actionType == RS2::ActionDimLinearHor || m_actionType == RS2::ActionDimLinearVer;
    if (!maySwitchDirection) {
        maySwitchDirection  = !LC_LineMath::isMeaningfulAngle(m_ucsBasisAngleDegrees) || !LC_LineMath::isMeaningfulAngle(m_ucsBasisAngleDegrees - M_PI_2);
    }
    return maySwitchDirection;
}

void RS_ActionDimLinear::updateMouseButtonHintForExtPoint2() {
    const bool maySwitchDirection = checkMaySwitchDimDirection();
    if (maySwitchDirection) {
        updatePromptTRBack(tr("Specify second extension line origin"), MOD_SHIFT_AND_CTRL( MSG_REL_ZERO, tr("Switch Direction")));
    }
    else {
        LC_ActionDimLinearBase::updateMouseButtonHintForExtPoint2();
    }
}

void RS_ActionDimLinear::updateMouseButtonHintForDefPoint() {
    const bool maySwitchDirection = checkMaySwitchDimDirection();
    if (maySwitchDirection) {
        updatePromptTRBack(tr("Specify dimension line location"), MOD_SHIFT_AND_CTRL(tr("Snap to Adjacent Dim"), tr("Switch Direction")));
    }
    else {
        LC_ActionDimLinearBase::updateMouseButtonHintForDefPoint();
    }
}

void RS_ActionDimLinear::setExtensionPoint1(const RS_Vector& p){
    m_edata->extensionPoint1 = p;
}

void RS_ActionDimLinear::setExtensionPoint2(const RS_Vector& p){
    m_edata->extensionPoint2 = p;
}
