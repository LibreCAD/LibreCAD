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

#include "rs_constructionline.h"
#include "rs_dimlinear.h"

/**
 * Constructor.
 *
 * @param angle Initial angle in rad.
 * @param fixedAngle true: The user can't change the angle.
 *                   false: The user can change the angle in a option widget.
 */
RS_ActionDimLinear::RS_ActionDimLinear(LC_ActionContext *actionContext,double angle,bool _fixedAngle, RS2::ActionType type)
    :LC_ActionDimLinearBase("Draw linear dimensions", actionContext, type),
     edata(std::make_unique<RS_DimLinearData>(RS_Vector(0., 0.), RS_Vector(0., 0.), angle, 0.)),
     fixedAngle(_fixedAngle), lastStatus(SetExtPoint1){
    setUcsAngleDegrees(angle);
    updateOptions();
    reset();
}

RS_ActionDimLinear::~RS_ActionDimLinear() = default;

void RS_ActionDimLinear::reset(){
    RS_ActionDimension::reset();

    double angleDeg = fixedAngle ? ucsBasisAngleDegrees : 0; // keep selected angle

    *edata = {{}, {}, toWorldAngleFromUCSBasisDegrees(angleDeg), toWorldAngle(0.0)};
}

void RS_ActionDimLinear::preparePreview(){
    double angle = getDimAngle();
    RS_Vector dirV = RS_Vector::polar(100., angle + M_PI_2);
    RS_ConstructionLine cl(nullptr,RS_ConstructionLineData(edata->extensionPoint2,edata->extensionPoint2 + dirV));
    data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}

RS_Entity *RS_ActionDimLinear::createDim(RS_EntityContainer* parent){
    edata->angle = getDimAngle();
    edata->oblique =  toWorldAngle(0.0);
    auto *dim = new RS_DimLinear(parent, *data, *edata);
    return dim;
}

double RS_ActionDimLinear::getUcsAngleDegrees() const{
    return ucsBasisAngleDegrees;
}

void RS_ActionDimLinear::setUcsAngleDegrees(double ucsRelAngleDegrees){
    ucsBasisAngleDegrees = ucsRelAngleDegrees;
}

bool RS_ActionDimLinear::hasFixedAngle() const{
    return fixedAngle;
}

void RS_ActionDimLinear::onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetAngle:{
            if (isZero){
                setUcsAngleDegrees(0);
                updateOptions();
                setStatus(lastStatus);
                return;
            }
            break;
        }
        default:
            break;
    }
    LC_ActionDimLinearBase::onCoordinateEvent(status, isZero, pos);
}

bool RS_ActionDimLinear::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetText: {
            setText(c);
            updateOptions();
            enableCoordinateInput();
            setStatus(lastStatus);
            accept = true;
            break;
        }
        case SetAngle: {
            double ucsBasisAngleDeg;
            bool ok = parseToUCSBasisAngle(c, ucsBasisAngleDeg);
            if (ok){
                accept = true;
                ucsBasisAngleDegrees = ucsBasisAngleDeg;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        default:
            lastStatus = (Status) getStatus();
            deletePreview();
            if (checkCommand("text", c)){
                disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            } else if (!fixedAngle && (checkCommand("angle", c))){
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
            if (!fixedAngle){
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
    return edata->extensionPoint1;
}

RS_Vector RS_ActionDimLinear::getExtensionPoint2(){
    return edata->extensionPoint2;
}

double RS_ActionDimLinear::getDimAngle(){
// Todo - sand - option for dim behaviour?
// This is good place for optional behavior for Hor/Vert dims.
// Implementation below created dims orthogonal to X and Y axises. 0 in UCS Abs values, ignore basis there??
// however, if translate angles using toWorldAngleFromUCSBasis(), dims will be orthogonal to angles basis.
// not sure whether this is necessary so far, yet still - an option may be added to the options widget.

    switch (m_actionType){
        case RS2::ActionDimLinearHor:{
            return toWorldAngle(0);
        }
        case RS2::ActionDimLinearVer:{
            return toWorldAngle(M_PI_2);
        }
        default:{
            return toWorldAngleFromUCSBasisDegrees(ucsBasisAngleDegrees);
        }
    }
}

void RS_ActionDimLinear::setExtensionPoint1(RS_Vector p){
    edata->extensionPoint1 = p;
}

void RS_ActionDimLinear::setExtensionPoint2(RS_Vector p){
    edata->extensionPoint2 = p;
}
