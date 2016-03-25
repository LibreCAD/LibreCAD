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

#include "rs_actiondimaligned.h"

#include "rs_dimension.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"


RS_ActionDimension::RS_ActionDimension(const char* name,
									   RS_EntityContainer& container,
									   RS_GraphicView& graphicView)
	:RS_PreviewActionInterface(name,
							   container, graphicView) {

	reset();
}



RS_ActionDimension::~RS_ActionDimension() = default;



void RS_ActionDimension::reset() {
	data.reset(new RS_DimensionData(RS_Vector(false),
									RS_Vector(false),
									RS_MTextData::VAMiddle,
									RS_MTextData::HACenter,
									RS_MTextData::Exact,
									1.0,
									"",
									"Standard",
									0.0)
			   );
	diameter = false;
}



void RS_ActionDimension::init(int status) {
	RS_PreviewActionInterface::init(status);
	//reset();
}



void RS_ActionDimension::hideOptions() {
	RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDimension::showOptions() {
	RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimension::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::SelectCursor);
}

bool RS_ActionDimension::isDimensionAction(RS2::ActionType type) {
	switch(type){
	case RS2::ActionDimAligned:
	case RS2::ActionDimLinear:
	case RS2::ActionDimLinearVer:
	case RS2::ActionDimLinearHor:
	case RS2::ActionDimAngular:
	case RS2::ActionDimDiametric:
	case RS2::ActionDimRadial:
		return true;
	default:
		return false;
	}
}


QString RS_ActionDimension::getText() const {
	if (!data->text.isEmpty()) {
		return data->text;
	}

	QString l = label;

	if (l.isEmpty() &&
			(diameter==true || !tol1.isEmpty() || !tol2.isEmpty())) {
		l = "<>";
	}

	if (diameter==true) {
		l = QChar(0x2205) + l;
	}

	if (!tol1.isEmpty() || !tol2.isEmpty()) {
		l += QString("\\S%1\\%2;").arg(tol1).arg(tol2);
	}

	return l;
}

void RS_ActionDimension::setText(const QString& t) {
	data->text = t;
}

const QString& RS_ActionDimension::getLabel() const{
	return label;
}

void RS_ActionDimension::setLabel(const QString& t) {
	//data->text = t;
	label = t;
}
const QString& RS_ActionDimension::getTol1() const{
	return tol1;
}
void RS_ActionDimension::setTol1(const QString& t) {
	tol1 = t;
}
const QString& RS_ActionDimension::getTol2() const{
	return tol2;
}
void RS_ActionDimension::setTol2(const QString& t) {
	tol2 = t;
}
bool RS_ActionDimension::getDiameter() const {
	return diameter;
}
void RS_ActionDimension::setDiameter(bool d) {
	diameter = d;
}

// EOF
